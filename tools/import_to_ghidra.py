#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Drive Ghidra in headless mode to:
  1. Create a project at build/ghidra/<binary>.gpr
  2. Import the binary from orig/<binary>.exe or an absolute path
  3. Run auto-analysis (with the Microsoft RTTI Analyzer enabled)
  4. Run our post-analysis scripts in tools/ghidra_scripts/:
       - DumpFunctions.java
       - DumpStrings.java
       - DumpRtti.java

Outputs:
  build/ghidra/<binary>.gpr    Ghidra project (re-runnable via -process)
  asm/<binary>/                one .s per function (RVA-prefixed filename)
  config/<binary>.symbols.json full function list with sizes + sections
  config/<binary>.strings.json every defined string with seed-hint flags
  config/<binary>.rtti.json    every recovered vtable / class

Usage:
  GHIDRA_HOME=/opt/homebrew/Cellar/ghidra/12.0.4/libexec \\
  python3 tools/import_to_ghidra.py ffxivlogin.exe
  python3 tools/import_to_ghidra.py /path/to/ffxivgame.exe --project-dir build/ghidra

Windows uses Ghidra's support/analyzeHeadless.bat and its configured JDK:
  python tools/import_to_ghidra.py "C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV\\ffxivgame.exe" --ghidra-home C:\\Tools\\ghidra_12.1_PUBLIC

On Windows, GHIDRA_HOME defaults to the newest ghidra_*_PUBLIC under ~/Tools.
On POSIX, it defaults to the brew install path if unset.
--java-home is retained for POSIX launch.sh use. Windows leaves JAVA_HOME and
PATH inherited so analyzeHeadless.bat can select Ghidra's configured JDK.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def _ghidra_version(path: Path) -> tuple[int, ...]:
    name = path.name.lower().removeprefix("ghidra_").split("_", 1)[0]
    try:
        return tuple(int(part) for part in name.split("."))
    except ValueError:
        return ()


def _autodetect_ghidra_home() -> str:
    """Find a native Ghidra installation, with the historical brew fallback."""
    if os.name == "nt":
        tool_roots = {Path.home() / "Tools", Path.home() / "tools"}
        installs = sorted(
            (
                p
                for root in tool_roots
                if root.is_dir()
                for p in root.glob("ghidra_*_PUBLIC")
                if (p / "support" / "analyzeHeadless.bat").is_file()
            ),
            key=_ghidra_version,
            reverse=True,
        )
        if installs:
            return str(installs[0])

    cellar = Path("/opt/homebrew/Cellar/ghidra")
    if cellar.is_dir():
        versions = sorted(
            (p for p in cellar.iterdir() if p.is_dir() and (p / "libexec" / "support" / "launch.sh").exists()),
            key=lambda p: tuple(int(x) for x in p.name.split(".") if x.isdigit()),
            reverse=True,
        )
        if versions:
            return str(versions[0] / "libexec")
    return "/opt/homebrew/Cellar/ghidra/12.0.4/libexec"


DEFAULT_GHIDRA_HOME = _autodetect_ghidra_home()


def _autodetect_java_home() -> str:
    """Find the newest brew-installed openjdk@21 cellar, fall back to a hardcoded path."""
    cellar = Path("/opt/homebrew/Cellar/openjdk@21")
    if cellar.is_dir():
        versions = sorted(
            (p for p in cellar.iterdir() if p.is_dir()),
            key=lambda p: tuple(int(x) for x in p.name.split(".") if x.isdigit()),
            reverse=True,
        )
        if versions:
            return str(versions[0] / "libexec" / "openjdk.jdk" / "Contents" / "Home")
    return "/opt/homebrew/Cellar/openjdk@21/21.0.11/libexec/openjdk.jdk/Contents/Home"


DEFAULT_JAVA_HOME = _autodetect_java_home()


def _find_launcher(ghidra: Path) -> tuple[Path, bool] | None:
    """Return the native headless launcher and whether it is the Windows batch file."""
    launch_sh = ghidra / "support" / "launch.sh"
    analyze_bat = ghidra / "support" / "analyzeHeadless.bat"
    if os.name == "nt":
        candidates = ((analyze_bat, True),)
    else:
        candidates = ((launch_sh, False),)
    for launcher, is_windows in candidates:
        if launcher.is_file():
            return launcher, is_windows
    return None


def _windows_short_path(path: Path) -> Path:
    """Use an existing 8.3 spelling for batch/Ghidra paths with spaces or dot parts."""
    if os.name != "nt":
        return path
    import ctypes

    get_short_path = ctypes.windll.kernel32.GetShortPathNameW
    source = str(path)
    size = get_short_path(source, None, 0)
    if not size:
        return path
    buffer = ctypes.create_unicode_buffer(size + 1)
    if not get_short_path(source, buffer, size + 1):
        return path
    return Path(buffer.value)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary", help="binary name in orig/ or an absolute executable path")
    ap.add_argument(
        "--ghidra-home",
        default=os.environ.get("GHIDRA_HOME", DEFAULT_GHIDRA_HOME),
        help=f"path to Ghidra install root (default: $GHIDRA_HOME or {DEFAULT_GHIDRA_HOME})",
    )
    ap.add_argument(
        "--java-home",
        default=os.environ.get("JAVA_HOME", DEFAULT_JAVA_HOME),
        help=f"POSIX JDK 21 path (ignored on Windows; default: $JAVA_HOME or {DEFAULT_JAVA_HOME})",
    )
    ap.add_argument(
        "--reanalyze",
        action="store_true",
        help="overwrite the existing Ghidra project and re-import the binary",
    )
    ap.add_argument(
        "--skip-import",
        action="store_true",
        help="re-run scripts only against an existing project (skip auto-analysis)",
    )
    ap.add_argument(
        "--analysis-timeout",
        type=int,
        default=0,
        help="cap auto-analysis at N seconds (0 = unlimited)",
    )
    ap.add_argument(
        "--max-memory",
        default="8G",
        help="JVM max heap (-Xmx) (default: 8G)",
    )
    ap.add_argument(
        "--project-dir",
        default=str(REPO_ROOT / "build" / "ghidra"),
        help="Ghidra project location (default: build/ghidra)",
    )
    ap.add_argument(
        "--scripts",
        default="DumpFunctions.java,DumpStrings.java,DumpRtti.java",
        help="comma-separated Ghidra post-scripts to run (default: all three Phase-1 dumps)",
    )
    args = ap.parse_args()

    ghidra = Path(args.ghidra_home)
    launcher_info = _find_launcher(ghidra)
    if launcher_info is None:
        print(
            f"error: not a Ghidra install: {ghidra}  "
            f"(missing native support/{'analyzeHeadless.bat' if os.name == 'nt' else 'launch.sh'})",
            file=sys.stderr,
        )
        return 1

    launch, windows_launcher = launcher_info
    requested = Path(args.binary)
    src = requested if requested.is_absolute() else REPO_ROOT / "orig" / requested
    if not src.exists():
        print(f"error: missing {src}; supply the retail executable under orig/",
              file=sys.stderr)
        return 1

    project_dir = Path(args.project_dir)
    if not project_dir.is_absolute():
        project_dir = REPO_ROOT / project_dir
    project_dir.mkdir(parents=True, exist_ok=True)
    binary_name = src.name
    project_name = src.stem
    scripts = REPO_ROOT / "tools" / "ghidra_scripts"

    # If reanalyzing, remove only this Ghidra project's known artifacts.
    project_marker = project_dir / f"{project_name}.gpr"
    if args.reanalyze:
        project_artifacts = [
            project_marker,
            project_dir / f"{project_name}.rep",
            project_dir / f"{project_name}.lock",
            project_dir / f"{project_name}.lock~",
        ]
        for p in project_artifacts:
            if p.is_file() or p.is_symlink():
                p.unlink()
            elif p.is_dir():
                shutil.rmtree(p)

    has_project = project_marker.exists()
    command_project_dir = _windows_short_path(project_dir)
    command_src = _windows_short_path(src)
    command_scripts = _windows_short_path(scripts)
    headless_args: list[str] = [str(command_project_dir), project_name]
    if has_project and args.skip_import:
        headless_args += ["-process", binary_name, "-noanalysis"]
    elif has_project:
        headless_args += ["-process", binary_name]
    else:
        headless_args += ["-import", str(command_src)]

    headless_args += ["-scriptPath", str(command_scripts)]
    for script_name in args.scripts.split(","):
        script_name = script_name.strip()
        if script_name:
            headless_args += ["-postScript", script_name]
    if args.analysis_timeout:
        headless_args += ["-analysisTimeoutPerFile", str(args.analysis_timeout)]

    if windows_launcher:
        # analyzeHeadless.bat has no heap argument. It reads this documented variable.
        cmd = [str(_windows_short_path(launch)), *headless_args]
    else:
        # launch.sh signature:
        #   launch.sh <mode> <java-type> <name> <max-memory> <vmarg-list> <classname> <args>...
        cmd = [
            str(launch),
            "fg",
            "jdk",
            "Ghidra-Headless",
            args.max_memory,
            "",  # no extra vmargs
            "ghidra.app.util.headless.AnalyzeHeadless",
            *headless_args,
        ]

    env = os.environ.copy()
    env["XIVL_DECOMP_ROOT"] = str(REPO_ROOT)
    if windows_launcher:
        env["GHIDRA_HEADLESS_MAXMEM"] = args.max_memory
    else:
        env["JAVA_HOME"] = args.java_home
        env["PATH"] = f"{args.java_home}/bin:" + env.get("PATH", "")

    if not windows_launcher:
        print(f">>> JAVA_HOME={args.java_home}")
    print(f">>> XIVL_DECOMP_ROOT={REPO_ROOT}")
    if windows_launcher:
        print(f">>> GHIDRA_HEADLESS_MAXMEM={args.max_memory}")
    print(">>> ghidra:", " ".join(cmd))
    try:
        subprocess.run(cmd, check=True, env=env)
    except subprocess.CalledProcessError as e:
        print(f"error: ghidra headless failed (exit {e.returncode})", file=sys.stderr)
        return e.returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
