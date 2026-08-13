#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Up-direction (client -> server) opcode reconnaissance for the IpcChannel
ClientPacketBuilders.

Architectural finding:

  Each *ProtoChannel::ClientPacketBuilder is a 4-slot generic vtable -
  there is no per-opcode method. The opcode is stored at offset 0x1C
  of the builder instance and is set by the constructor (which takes
  it as the first stack arg). The constructors observed:

    Lobby ClientPacketBuilder ctors:
      RVA 0x009a2b50
      RVA 0x009a2be0
    Zone  ClientPacketBuilder ctor - RVA 0x009c1c60 (141 B; 1 caller)
                                  + RVA 0x009c1cf0 (126 B; 2 callers)
    Chat  ClientPacketBuilder ctor - RVA 0x00a40a60

  Both Zone constructors do `MOV [this+0x1C], <stack-arg>`, so the
  opcode is determined dynamically per call site.

  This means there is **no compact per-opcode table** for Up packets
  analogous to the Down dispatcher's byte_table+dword_table. The
  Up-opcode space is implicit in the binary - distributed across
  dozens of "send" functions, each of which loads its opcode from
  context-specific sources (struct fields, computed values, hard-
  coded immediates).

  Fully enumerating the Up-opcode space requires Ghidra-driven cross-
  reference analysis (per-callsite constant propagation through the
  builder constructor's `arg0` slot). That's beyond what static
  byte-pattern scanning can produce reliably.

What this tool produces:

  1. A ClientPacketBuilder constructor inventory.
  2. Opcodes recovered from direct constructor call sites.
  3. A client-only Markdown report for further analysis.

Output:
  config/<binary>.up_opcodes.json
  build/wire/<binary>.up_opcodes.md
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ORIG_PE = REPO_ROOT / "orig" / "ffxivgame.exe"
PE_LAYOUT = REPO_ROOT / "build" / "pe-layout" / "ffxivgame.json"
CONFIG = REPO_ROOT / "config"
WIRE = REPO_ROOT / "build" / "wire"

# Discovered ClientPacketBuilder vtable VAs (image_base + RVA).
CPB_VTABLES = {
    "lobby": 0x01127754,
    "zone":  0x01129ae8,
    "chat":  0x0113e8d0,
}

# Constructor RVAs recovered from the vtable-store sites. Direct callers pass
# the opcode as the final stack argument before calling the constructor.
KNOWN_CPB_CTORS = {
    "lobby_a": 0x009a2b50,
    "lobby_b": 0x009a2be0,
    "zone_a":  0x009c1c60,
    "zone_b":  0x009c1cf0,
    "chat_a":  0x00a40a60,
}


def _load_pe() -> dict:
    return json.loads(PE_LAYOUT.read_text())


def _load_text() -> tuple[bytes, dict]:
    pe = _load_pe()
    text_sec = next(s for s in pe["sections"] if s["name"] == ".text")
    return ORIG_PE.read_bytes(), text_sec


def _load_syms() -> list[dict]:
    return json.loads((CONFIG / "ffxivgame.symbols.json").read_text())


def _find_fn(rva: int, syms: list[dict]) -> str | None:
    for s in syms:
        if s["rva"] <= rva < s["rva"] + s["size"]:
            return s["name"]
    return None


def find_ctor_sites(data: bytes, text_sec: dict) -> dict[str, list[dict]]:
    """For each CPB vtable VA, find places that store it into an object's
    first slot - those are constructor invocations of the CPB class
    hierarchy. We don't decode WHICH constructor, just locate them."""
    text_off = text_sec["raw_pointer"]
    text_size = text_sec["raw_size"]
    text_va_start = text_sec["virtual_address"]
    out: dict[str, list[dict]] = {ch: [] for ch in CPB_VTABLES}
    for ch, vt in CPB_VTABLES.items():
        needle = struct.pack("<I", vt)
        i = text_off
        end = text_off + text_size
        while True:
            i = data.find(needle, i, end)
            if i < 0:
                break
            # Look back 2 bytes for `c7 0?` (MOV [reg], imm32) or `c7 4?` (with disp8) etc.
            if i >= 2 and data[i-2] == 0xc7 and (data[i-2+1] & 0x07) != 0x04:
                rva = (i - text_off) + text_va_start
                out[ch].append({"rva_hex": f"0x{rva:08x}", "store_form": f"c7 {data[i-1]:02x}"})
            i += 1
    return out


def find_ctor_callers(data: bytes, text_sec: dict, ctor_rva: int) -> list[int]:
    """Find every CALL imm32 site in .text whose target is the given RVA."""
    text_off = text_sec["raw_pointer"]
    text_size = text_sec["raw_size"]
    text_va = text_sec["virtual_address"]
    hits = []
    i = text_off
    end = text_off + text_size
    while i < end - 5:
        if data[i] == 0xe8:
            rel = struct.unpack_from("<i", data, i + 1)[0]
            call_pc = (i - text_off) + text_va + 5
            if call_pc + rel == ctor_rva:
                hits.append((i - text_off) + text_va)
        i += 1
    return hits


def decode_recent_pushes(data: bytes, text_sec: dict, call_rva: int,
                         lookback: int = 80) -> list[tuple[int, str, object]]:
    """Walk back up to `lookback` bytes from a CALL site and collect the
    PUSH instructions in order. Returns list of (rva, kind, value)."""
    text_off = text_sec["raw_pointer"]
    text_va = text_sec["virtual_address"]
    call_off = call_rva - text_va + text_off
    start = max(text_off, call_off - lookback)
    pushes = []
    j = start
    while j < call_off:
        b = data[j]
        rva_here = (j - text_off) + text_va
        if b == 0x68:  # PUSH imm32
            imm = struct.unpack_from("<I", data, j + 1)[0]
            pushes.append((rva_here, "imm32", imm))
            j += 5
            continue
        if b == 0x6a:  # PUSH imm8 (signed)
            pushes.append((rva_here, "imm8", data[j + 1]))
            j += 2
            continue
        if b in (0x50, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57):
            reg_name = {0x50: "EAX", 0x51: "ECX", 0x52: "EDX", 0x53: "EBX",
                        0x55: "EBP", 0x56: "ESI", 0x57: "EDI"}[b]
            pushes.append((rva_here, "reg", reg_name))
            j += 1
            continue
        if b == 0xff and j + 1 < call_off:
            # PUSH r/m32 - most variants are mem-based with disp8
            mr = data[j + 1]
            if mr in (0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
                      0x30, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37):
                pushes.append((rva_here, "mem", f"ff {mr:02x}"))
                j += 3 if (mr & 0xf0) == 0x70 else 2
                continue
        j += 1
    return pushes


def enumerate_ctor_call_opcodes(data: bytes, text_sec: dict,
                                 syms: list[dict]) -> dict:
    """For each known CPB ctor, find direct CALL sites and decode the
    opcode arg.

    The CPB ctor signature (verified by decoding lobby_b ctor body
    FUN_00da2be0 at RVA 0x009a2be0) is `(this_ECX, arg1_OPCODE, arg2,
    arg3)`. The body loads arg1 from `[esp+0x20]` (post-prologue) and
    writes it to `[this+0x1c]`. Stack args are pushed right-to-left in
    cdecl/stdcall, so at the call site:
      PUSH arg3   ; lowest address
      PUSH arg2
      PUSH arg1   ; highest address (= opcode), pushed LAST before CALL
      MOV ECX, this
      CALL ctor

    The opcode is the LAST push of the 3 args (highest address, pushed
    last). The ctor's stack-offset arithmetic confirms this: the prologue
    pushes 7 slots = 0x1c bytes, so the original arg1 at caller [esp+4]
    is at post-prologue [esp+0x20].
    """
    out = {}
    for label, ctor_rva in KNOWN_CPB_CTORS.items():
        callers = find_ctor_callers(data, text_sec, ctor_rva)
        per_caller = []
        for c in callers:
            caller_fn = _find_fn(c, syms)
            recent = decode_recent_pushes(data, text_sec, c)[-3:]
            opcode = None
            if len(recent) == 3 and recent[-1][1] in ("imm8", "imm32"):
                # arg1 = last push (highest addr) = opcode
                opcode = recent[-1][2]
            per_caller.append({
                "call_rva_hex": f"0x{c:08x}",
                "caller_fn": caller_fn,
                "recent_pushes": [
                    {"rva_hex": f"0x{r[0]:08x}", "kind": r[1], "value": r[2]}
                    for r in recent
                ],
                "opcode": opcode,
                "opcode_hex": f"0x{opcode:04x}" if opcode is not None else None,
            })
        out[label] = {
            "ctor_rva_hex": f"0x{ctor_rva:08x}",
            "caller_count": len(callers),
            "calls": per_caller,
        }
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary", default="ffxivgame", nargs="?",
                    choices=("ffxivgame", "ffxivgame.exe"),
                    help="fixed supported client binary")
    args = ap.parse_args()
    stem = args.binary.replace(".exe", "")

    if not PE_LAYOUT.exists():
        print(f"error: {PE_LAYOUT} missing", file=sys.stderr)
        return 3

    data, text_sec = _load_text()
    syms = _load_syms()
    ctor_sites = find_ctor_sites(data, text_sec)
    ctor_calls = enumerate_ctor_call_opcodes(data, text_sec, syms)
    recovered: dict[int, list[dict]] = {}
    for label, info in ctor_calls.items():
        for call in info["calls"]:
            if call["opcode"] is None:
                continue
            recovered.setdefault(call["opcode"], []).append({
                "channel": label,
                "caller_fn": call["caller_fn"] or "?",
                "call_rva_hex": call["call_rva_hex"],
            })

    rows = [{
        "opcode": opcode,
        "opcode_hex": f"0x{opcode:04x}",
        "site_count": len(sites),
        "channels": sorted({s["channel"] for s in sites}),
        "caller_fns": sorted({s["caller_fn"] for s in sites}),
    } for opcode, sites in sorted(recovered.items())]
    summary = {"ctor_sites": ctor_sites, "ctor_callers": ctor_calls, "recovered_opcodes": rows}
    out_json = CONFIG / f"{stem}.up_opcodes.json"
    out_json.write_text(json.dumps(summary, indent=2))

    WIRE.mkdir(parents=True, exist_ok=True)
    out_md = WIRE / f"{stem}.up_opcodes.md"
    with out_md.open("w", encoding="utf-8") as f:
        f.write(f"# {stem}.exe - Up-direction opcode reconnaissance\n\n")
        f.write("Generated by `tools/extract_up_opcodes.py` from client constructor call sites.\n\n")
        f.write("The generic ClientPacketBuilder stores its opcode at offset `0x1c`; no compact per-opcode Up table was observed.\n\n")
        f.write("## Constructors\n\n| channel | constructor RVA | direct callers |\n|---|---|---:|\n")
        for label, info in ctor_calls.items():
            f.write(f"| {label} | `{info['ctor_rva_hex']}` | {info['caller_count']} |\n")
        f.write("\n## Recovered direct-call opcodes\n\n")
        f.write("| opcode | hex | channels | sites | caller functions |\n|---:|---:|---|---:|---|\n")
        for row in rows:
            f.write(f"| {row['opcode']} | `{row['opcode_hex']}` | {', '.join(row['channels'])} | {row['site_count']} | {', '.join(row['caller_fns'])} |\n")
        f.write("\nA complete inventory requires data-flow analysis through shared builder instances and indirect Send calls.\n")

    print(f"wrote: {out_json.relative_to(REPO_ROOT)}")
    print(f"       {out_md.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
