#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Build + apply a Ghidra Function ID (FidDb) library-signature database for
the MSVC-2005 (VC8) static CRT/STL the FFXIV 1.x client links against, so
otherwise anonymous CRT/STL functions can be distinguished from client code.

WHY THIS HELPS: a 33 MB static-MSVC binary contains thousands of CRT/STL
functions that initially look like anonymous client logic. FidDb supplies
reviewable library names without asserting identities for Square Enix code.

KEY GOTCHA (validated): `analyzeHeadless -import foo.lib` FAILS ("no load
spec") - Ghidra can't load a COFF *archive*. So we `llvm-ar x` the .lib into
its .obj members (Ghidra loads single .obj natively) and import those. macOS
BSD `ar` mangles MS COFF member names; `llvm-ar` handles them.

SCOPE: only the true third-party CRT/STL is signature-matchable. The bulk of
the binary is Square Enix's own engine (CDev/Rapture: engine_cdev ~5k, net
~2.5k, render ~900) - NOT library code; FidDb can't touch it. DX9 is
dynamically linked (d3dx9_41.dll) so D3DX isn't in the binary either. The
win is concentrated in CRT/STL.

Subcommands:
  extract   llvm-ar x the configured libs -> build/fid/objs/<lib>/
  import    analyzeHeadless-import the .obj into a FID project (FID/LID off)
  populate  CreateMultipleLibraries -> build/fid/ffxiv_vc8.fidb
  gen       extract + import + populate (the full one-time build)
  apply     attach the fidb to the ffxivgame project, re-run the Function ID
            analyzer, and re-dump symbols.json

The populate step drives Ghidra's stock CreateMultipleLibraries via a headless
.properties file. If it balks (headless ask* is finicky), the runbook
docs/resource/pe-layout.md records the resulting library identifications;
tools/README.md documents this one-time step; `apply` is fully scripted either way.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
FID_DIR = REPO_ROOT / "build" / "fid"
FIDB = FID_DIR / "ffxiv_vc8.fidb"
LANG_ID = "x86:LE:32:default"
LIB_FAMILY, LIB_VERSION, LIB_VARIANT = "MSVC", "8.0", "x86"

def find_ghidra() -> Path:
    configured = os.environ.get("GHIDRA_HOME")
    if configured:
        return Path(configured)
    cellar = Path("/opt/homebrew/Cellar/ghidra")
    if cellar.is_dir():
        vs = sorted((p for p in cellar.iterdir()
                     if (p / "libexec/support/launch.sh").exists()),
                    key=lambda p: tuple(int(x) for x in p.name.split(".") if x.isdigit()))
        if vs:
            return vs[-1] / "libexec"
    raise SystemExit("Ghidra not found; set GHIDRA_HOME to a Ghidra installation")


def find_java() -> str:
    cellar = Path("/opt/homebrew/Cellar/openjdk@21")
    if cellar.is_dir():
        vs = sorted(cellar.iterdir(), key=lambda p: tuple(int(x) for x in p.name.split(".") if x.isdigit()))
        if vs:
            return str(vs[-1] / "libexec/openjdk.jdk/Contents/Home")
    configured = os.environ.get("JAVA_HOME")
    if configured:
        return configured
    raise SystemExit("JDK 21 not found; set JAVA_HOME")


def find_llvm_ar() -> str:
    for c in ("llvm-ar", "/opt/homebrew/opt/llvm/bin/llvm-ar", "/opt/homebrew/bin/llvm-ar"):
        if shutil.which(c) or Path(c).exists():
            return c
    raise SystemExit("llvm-ar not found (brew install llvm). BSD ar cannot extract MS COFF archives.")


def run_headless(gh: Path, jh: str, proj_loc: Path, proj_name: str, rest: list[str],
                 mem: str = "8G") -> int:
    cmd = [str(gh / "support/launch.sh"), "fg", "jdk", "Ghidra-Headless", mem, "",
           "ghidra.app.util.headless.AnalyzeHeadless", str(proj_loc), proj_name, *rest]
    env = os.environ.copy()
    env["JAVA_HOME"] = jh
    env["PATH"] = f"{jh}/bin:" + env.get("PATH", "")
    env["XIVL_DECOMP_ROOT"] = str(REPO_ROOT)  # dump scripts resolve config/ against this
    print(">>>", " ".join(cmd))
    return subprocess.run(cmd, env=env).returncode


def cmd_extract(args) -> int:
    llvm_ar = find_llvm_ar()
    objs_root = FID_DIR / "objs"
    if objs_root.exists():
        shutil.rmtree(objs_root)
    total = 0
    for lib in args.libs:
        libp = Path(lib)
        if not libp.exists():
            print(f"ERROR: lib not found: {libp}", file=sys.stderr)
            return 1
        dest = objs_root / Path(lib).stem
        dest.mkdir(parents=True, exist_ok=True)
        subprocess.run([llvm_ar, "x", str(libp)], cwd=dest, check=True)
        n = len(list(dest.glob("*.obj")))
        print(f"  {libp.name}: extracted {n} .obj into {dest.relative_to(REPO_ROOT)}")
        total += n
    print(f"extracted ~{total} members under {objs_root.relative_to(REPO_ROOT)}")
    return 0


def cmd_import(args) -> int:
    gh, jh = find_ghidra(), find_java()
    fidscripts = gh / "Ghidra/Features/FunctionID/ghidra_scripts"
    objs_root = FID_DIR / "objs"
    proj_loc = FID_DIR / "proj"
    proj_loc.mkdir(parents=True, exist_ok=True)
    # import into folder /<family>/<version>/<variant> so CreateMultipleLibraries
    # (MASTER_DEPTH=3) reads name/version/variant from the path.
    proj_name = f"FidLibs/{LIB_FAMILY}/{LIB_VERSION}/{LIB_VARIANT}"
    rc = 0
    for sub in sorted(objs_root.iterdir()):
        if not sub.is_dir():
            continue
        rc = run_headless(gh, jh, proj_loc, proj_name, [
            "-import", str(sub), "-recursive",
            "-scriptPath", str(fidscripts),
            "-preScript", "FunctionIDHeadlessPrescript.java",
            "-postScript", "FunctionIDHeadlessPostscript.java",
        ])
        if rc != 0:
            return rc
    return rc


def _write_properties(fidscripts: Path):
    """Headless answers for the stock CreateMultipleLibraries prompts."""
    common = FID_DIR / "common_symbols.txt"
    common.write_text("", encoding="utf-8")  # empty = no common-symbol suppression
    props = {
        "Do Duplication Detection": "false",
        "Choose destination FidDB": FIDB.name,
        "Select root folder containing all libraries (at a depth of 3):": "/",
        "Common symbols file (optional):": str(common),
        "Enter LanguageID To Process": LANG_ID,
    }
    text = "".join(f"{k} = {v}\n" for k, v in props.items())
    (fidscripts / "CreateMultipleLibraries.properties").write_text(text, encoding="utf-8")
    (fidscripts / "CreateEmptyFidDatabase.properties").write_text(
        f"Create new FidDb file = {FIDB}\n", encoding="utf-8")


def cmd_populate(args) -> int:
    gh, jh = find_ghidra(), find_java()
    fidscripts = gh / "Ghidra/Features/FunctionID/ghidra_scripts"
    proj_loc = FID_DIR / "proj"
    FID_DIR.mkdir(parents=True, exist_ok=True)
    if FIDB.exists():
        FIDB.unlink()
    _write_properties(fidscripts)
    # CreateMultipleLibraries must run EXACTLY ONCE (it walks the whole
    # /MSVC/8.0/x86 folder itself). analyzeHeadless runs scripts once per
    # -process'd program, so target a single program (--anchor, read-only,
    # no re-analysis). The script ignores currentProgram and populates from
    # the root folder.
    anchor = args.anchor
    # analyzeHeadless -process searches the named project folder; the anchor
    # lives deep under /MSVC/8.0/x86, so scope the project to that subtree so
    # -process resolves. CreateMultipleLibraries still walks from root "/".
    proj_scoped = f"FidLibs/{LIB_FAMILY}/{LIB_VERSION}/{LIB_VARIANT}"
    return run_headless(gh, jh, proj_loc, proj_scoped, [
        "-process", anchor, "-readOnly", "-noanalysis",
        "-scriptPath", str(fidscripts),
        "-preScript", "CreateEmptyFidDatabase.java",
        "-postScript", "CreateMultipleLibraries.java",
    ])


def cmd_apply(args) -> int:
    gh, jh = find_ghidra(), find_java()
    fidscripts = gh / "Ghidra/Features/FunctionID/ghidra_scripts"
    proj_loc = REPO_ROOT / "build" / "ghidra"
    if not FIDB.exists():
        print(f"ERROR: {FIDB} not found - run `build_fid.py gen` first.", file=sys.stderr)
        return 1
    # Headless ask* key = dialog title + " " + approve-button label, i.e.
    # askFile("Attach existing FidDb", "Attach") -> "Attach existing FidDb Attach".
    (fidscripts / "AttachFidDatabase.properties").write_text(
        f"Attach existing FidDb Attach = {FIDB}\n", encoding="utf-8")
    print(">>> attaching FidDb + running Function ID matcher on ffxivgame, then re-dumping")
    # -scriptPath is ONE arg: a ';'-separated list of dirs. AttachFidDatabase
    # (+ its .properties) lives in the Ghidra FID scripts dir; RunFidMatch +
    # DumpFunctions live in ours.
    script_path = f"{fidscripts};{REPO_ROOT / 'tools/ghidra_scripts'}"
    # -noanalysis + RunFidMatch (force the FID analyzer to run now that the
    # fidb is attached - a plain -process won't re-run an already-run
    # analyzer). RunFidMatch applies names in-memory; DumpFunctions re-dumps.
    rc = run_headless(gh, jh, proj_loc, "ffxivgame", [
        "-process", "ffxivgame.exe", "-noanalysis",
        "-scriptPath", script_path,
        "-preScript", "AttachFidDatabase.java",
        "-postScript", "RunFidMatch.java",
        "-postScript", "DumpSymbolsOnly.java",
    ])
    if rc == 0:
        print("\nDONE. Review the updated config/ffxivgame.symbols.json catalog.")
    return rc


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    pe = sub.add_parser("extract")
    pe.add_argument("--libs", nargs="+", required=True)
    sub.add_parser("import")
    pp = sub.add_parser("populate")
    pp.add_argument("--anchor", default="crt0dat.obj",
                    help="a single (unique) imported program name to -process so the "
                         "once-only CreateMultipleLibraries script runs exactly once")
    sub.add_parser("apply")
    pg = sub.add_parser("gen")
    pg.add_argument("--libs", nargs="+", required=True)
    pg.add_argument("--anchor", default="crt0dat.obj")
    args = ap.parse_args()

    if args.cmd == "extract":
        return cmd_extract(args)
    if args.cmd == "import":
        return cmd_import(args)
    if args.cmd == "populate":
        return cmd_populate(args)
    if args.cmd == "apply":
        return cmd_apply(args)
    if args.cmd == "gen":
        for step in (cmd_extract, cmd_import, cmd_populate):
            rc = step(args)
            if rc != 0:
                print(f"gen aborted at {step.__name__} (rc={rc})", file=sys.stderr)
                return rc
        print(f"\nFidDb built: {FIDB.relative_to(REPO_ROOT)}. Next: `build_fid.py apply`.")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
