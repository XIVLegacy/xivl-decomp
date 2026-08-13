#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Extract the binary's per-channel opcode -> vtable-slot dispatch tables
from the IpcChannel Down callback dispatchers.

Each Down direction (server -> client) has its dispatcher at
*ProtoDownDummyCallback::vtable[1] (slot 0 = destructor, slot 1 =
the dispatcher's own implementation override; slots 2..N = the
per-opcode handlers).

The dispatcher's prologue follows a uniform shape (validated for
ZoneProtoDown @ RVA 0x009bfd10):

  MOV EDX, [ESP+8]            ; arg = incoming packet pointer
  MOV EAX, [EDX+8]            ; deref to packet header
  MOV EAX, [EAX+0x24]         ; offset to inner field carrying opcode
  PUSH ESI
  MOVZX ESI, word [EAX+0x2]   ; ESI = opcode (u16)
  ADD ESI, -<base>            ; normalize to 0..N
  CMP ESI, <count-1>
  JA  default_handler         ; out-of-range: fall through to default
  MOVZX ESI, byte [ESI + <byte_table_va>]   ; secondary lookup
  JMP  dword [ESI*4 + <dword_table_va>]     ; primary lookup -> handler

The byte_table at <byte_table_va> is `count` bytes, each a case
index 0..(num_cases-1). The dword_table at <dword_table_va> is
`num_cases` 4-byte addresses, each pointing at a sequential case
body.

Each case body is exactly 21 bytes long, of the shape
  MOV ESI, [ECX]            ; load `this`'s vtable
  ADD EAX, 0x10
  PUSH EAX
  MOV EAX, [ESI+<vtable_offset>]
  PUSH EDX
  MOV EDX, [ESP+0x10]
  PUSH EDX
  CALL EAX
  POP ESI
  RET 0x8
The `<vtable_offset>` is `(slot_index)*4`. Slot 2 = offset 0x8,
slot 3 = offset 0xc, etc.

This script derives:
  byte_table[opcode - opcode_base] = case_index
  case_body_addr = case_table_start + case_index * 21
  vtable_offset_at_case = read offset from MOV EAX,[ESI+<imm>] at +0xc
  vtable_slot = vtable_offset_at_case / 4


Output:
  config/<binary>.opcodes.json          machine-readable
  build/wire/<binary>.opcodes.md        human-readable client report
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ORIG_PE = REPO_ROOT / "orig" / "ffxivgame.exe"
PE_LAYOUT = REPO_ROOT / "build" / "pe-layout" / "ffxivgame.json"
CONFIG = REPO_ROOT / "config"
WIRE = REPO_ROOT / "build" / "wire"

# Per-channel dispatcher metadata. Each Down direction's dispatcher is the
# vtable[1] of the *DownDummyCallback class. The byte-table / dword-table
# / case-table metadata is derived by reading the dispatcher's asm
# prologue + first case body (script auto-derives - config below is the
# entry-point info).
CHANNELS: dict[str, dict] = {
    "zone": {
        "dispatcher_rva":   0x009bfd10,
        "callback_iface":   "Application::Network::ZoneProtoChannel::ZoneProtoDownCallbackInterface",
        "dummy_callback":   "Application::Network::ZoneClient::ZoneProtoDownDummyCallback",
        "expected_slots":   199,
    },
    "lobby": {
        "dispatcher_rva":   0x009a4160,    # slot 1 of LobbyProtoDownCallbackInterface
        "callback_iface":   "Application::Network::LobbyProtoChannel::LobbyProtoDownCallbackInterface",
        "dummy_callback":   "Application::Network::LobbyClient::LobbyProtoDownDummyCallback",
        "expected_slots":   14,
    },
    "chat": {
        "dispatcher_rva":   0x00a40630,    # slot 1 of ChatProtoDownDummyCallback
        "callback_iface":   "Application::Network::ChatProtoChannel::ChatProtoDownCallbackInterface",
        "dummy_callback":   "Application::Network::ChatClient::ChatProtoDownDummyCallback",
        "expected_slots":   10,
    },
}


def _va_to_off(pe: dict, va: int) -> int | None:
    rva = va - 0x400000
    for s in pe["sections"]:
        if s["virtual_address"] <= rva < s["virtual_address"] + max(s["virtual_size"], s["raw_size"]):
            return s["raw_pointer"] + (rva - s["virtual_address"])
    return None


def _read_dispatcher_asm(rva: int) -> str:
    asm_dir = REPO_ROOT / "asm" / "ffxivgame"
    matches = list(asm_dir.glob(f"{rva:08x}_*.s"))
    if not matches:
        raise RuntimeError(f"asm for dispatcher rva {rva:#x} not found")
    return matches[0].read_text()


def _parse_dispatcher(asm: str) -> dict:
    """Parse the dispatcher's prologue + first case body to derive
    table addresses + cadence.

    Two shapes observed:
      Zone (simpler):
          ADD ESI, -<base>
          CMP ESI, <count-1>
          JA  default
          MOVZX ESI, byte [ESI + <byte_table>]
          JMP  dword [ESI*4 + <dword_table>]

      Lobby (with sparse high-opcode special case):
          CMP  ESI, <hi_special>
          JG   default       ; ESI > hi_special -> default
          JZ   high_handler  ; ESI == hi_special -> its own handler
          SUB  ESI, 1        ; normalise low-range
          CMP  ESI, <count-1>
          JA   default
          MOVZX ESI, byte [...]
          JMP   dword [...]

    Both reduce to the same byte_table + dword_table mechanism for the
    primary low-opcode range. We extract those + opcode_base from the
    SUB / ADD instruction; the high special-case (lobby) is noted but
    not currently modelled - its handler lives at a different VA we
    could decode separately.
    """
    info: dict = {}

    # Try simpler ADD-style shape first.
    sub_m = re.search(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} )+\s+(?:ADD|SUB)\s+ESI,(-?0x[0-9a-fA-F]+|-?\d+)\s*$",
                      asm, re.MULTILINE)
    cmp_ms = list(re.finditer(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} )+\s+CMP\s+ESI,(0x[0-9a-fA-F]+|\d+)\s*$",
                               asm, re.MULTILINE))
    bt_m  = re.search(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} )+\s+MOVZX\s+ESI,byte ptr \[ESI \+ (0x[0-9a-fA-F]+)\]\s*$",
                      asm, re.MULTILINE)
    dt_m  = re.search(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} )+\s+JMP\s+dword ptr \[ESI\*0x4 \+ (0x[0-9a-fA-F]+)\]\s*$",
                      asm, re.MULTILINE)
    if not (sub_m and cmp_ms and bt_m and dt_m):
        raise RuntimeError(f"dispatcher prologue didn't match expected shape\n"
                           f"sub={sub_m} cmps={cmp_ms} bt={bt_m} dt={dt_m}")

    # The CMP that sets the count is the one IMMEDIATELY after the SUB/ADD -
    # for the lobby dual-range case, the FIRST CMP is the high-special bound,
    # and the SECOND is the low-range count. Pick the CMP that comes after
    # the SUB by file position.
    sub_pos = sub_m.start()
    count_cmp = next((m for m in cmp_ms if m.start() > sub_pos), cmp_ms[-1])

    info["base_offset"] = int(sub_m.group(1), 0)
    info["count"] = int(count_cmp.group(1), 0) + 1
    info["byte_table_va"] = int(bt_m.group(1), 0)
    info["dword_table_va"] = int(dt_m.group(1), 0)
    # opcode_base is the value subtracted: ADD ESI, -K -> base = K; SUB ESI, K -> base = K.
    info["opcode_base"] = abs(info["base_offset"])

    # First case body: find first `MOV EAX,dword ptr [ESI + 0x8]` after the JMP.
    # We use slot-2's offset (0x8) as anchor.
    case0_m = re.search(r"(\s*)([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s+MOV\s+EAX,dword ptr \[ESI \+ 0x8\]\s*$",
                        asm, re.MULTILINE)
    if case0_m is None:
        raise RuntimeError("first case body (MOV EAX, [ESI+0x8]) not found")

    # The case body starts a few bytes BEFORE the MOV EAX line - it begins with
    # `MOV ESI, [ECX]` (loading vtable). Find that line's address.
    m2 = re.search(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s+MOV\s+ESI,dword ptr \[ECX\]\s*\n"
                   r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s+ADD\s+EAX,0x10\s*\n"
                   r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s+PUSH\s+EAX\s*\n"
                   r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s+MOV\s+EAX,dword ptr \[ESI \+ 0x8\]",
                   asm, re.MULTILINE)
    if m2 is None:
        raise RuntimeError("first case body's full prologue not matched")
    info["case0_rva"] = int(m2.group(1), 16)
    return info


def _read_byte_table(pe: dict, va: int, count: int) -> bytes:
    data = ORIG_PE.read_bytes()
    off = _va_to_off(pe, va)
    if off is None:
        raise RuntimeError(f"byte table VA {va:#x} not in any section")
    return data[off : off + count]


def _read_dword_table(pe: dict, va: int, count: int) -> list[int]:
    data = ORIG_PE.read_bytes()
    off = _va_to_off(pe, va)
    if off is None:
        raise RuntimeError(f"dword table VA {va:#x} not in any section")
    return [struct.unpack_from("<I", data, off + i * 4)[0] for i in range(count)]


def _read_case_vtable_offset(pe: dict, case_addr_va: int) -> int:
    """Each case body, when entered, executes:
        MOV ESI, [ECX]         ; 2 bytes
        ADD EAX, 0x10          ; 3 bytes
        PUSH EAX               ; 1 byte
        MOV EAX, [ESI + <imm8 or imm32>]  ; we want this displacement
    The MOV EAX instruction is at case_start + 6. For displacements <= 127
    it's encoded `8B 46 <imm8>` (3 bytes); larger uses `8B 86 <imm32>` (6 bytes).
    """
    data = ORIG_PE.read_bytes()
    off = _va_to_off(pe, case_addr_va)
    if off is None:
        raise RuntimeError(f"case body VA {case_addr_va:#x} not in any section")
    insn_off = off + 6
    op0 = data[insn_off]
    if op0 == 0x8B and data[insn_off + 1] == 0x46:
        return data[insn_off + 2]                    # imm8
    if op0 == 0x8B and data[insn_off + 1] == 0x86:
        return struct.unpack_from("<I", data, insn_off + 2)[0]   # imm32
    raise RuntimeError(f"case body at {case_addr_va:#x} has unexpected MOV EAX form: {data[insn_off:insn_off+8].hex()}")


def extract_channel(channel: str, info: dict, pe: dict) -> dict:
    asm = _read_dispatcher_asm(info["dispatcher_rva"])
    parsed = _parse_dispatcher(asm)
    byte_table = _read_byte_table(pe, parsed["byte_table_va"], parsed["count"])
    num_cases = len(set(byte_table))
    dword_table = _read_dword_table(pe, parsed["dword_table_va"], num_cases)

    # Derive case_index -> vtable_slot by reading each case body's MOV EAX, [ESI+IMM].
    case_to_slot: list[int | None] = [None] * num_cases
    for i in range(num_cases):
        case_va = dword_table[i]
        try:
            off = _read_case_vtable_offset(pe, case_va)
            case_to_slot[i] = off // 4
        except Exception:
            case_to_slot[i] = None

    # opcode -> (case_index, slot_index)
    rows: list[dict] = []
    for i in range(parsed["count"]):
        opcode = parsed["opcode_base"] + i
        case_idx = byte_table[i]
        slot = case_to_slot[case_idx] if case_idx < len(case_to_slot) else None
        rows.append({"opcode": opcode, "opcode_hex": f"0x{opcode:04x}", "case_idx": case_idx, "vtable_slot": slot})

    return {
        "channel": channel,
        "direction": "down",
        "callback_iface": info["callback_iface"],
        "dispatcher_rva": f"0x{info['dispatcher_rva']:08x}",
        "opcode_base": parsed["opcode_base"],
        "opcode_count": parsed["count"],
        "byte_table_va": f"0x{parsed['byte_table_va']:08x}",
        "dword_table_va": f"0x{parsed['dword_table_va']:08x}",
        "num_cases": num_cases,
        "default_case_idx": num_cases - 1,    # by convention, last case = default no-op
        "real_opcode_count": sum(1 for r in rows if r["case_idx"] != num_cases - 1),
        "opcodes": rows,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary", default="ffxivgame", nargs="?",
                    choices=("ffxivgame", "ffxivgame.exe"),
                    help="fixed supported client binary")
    args = ap.parse_args()
    stem = args.binary.replace(".exe", "")

    if not PE_LAYOUT.exists():
        print(f"error: {PE_LAYOUT} missing - run tools/extract_pe.py", file=sys.stderr)
        return 3
    pe = json.loads(PE_LAYOUT.read_text())

    channels: list[dict] = []
    for name, info in CHANNELS.items():
        try:
            ch = extract_channel(name, info, pe)
            channels.append(ch)
            print(f"  {name}: {ch['real_opcode_count']} real of {ch['opcode_count']} opcodes")
        except Exception as exc:
            print(f"  {name}: FAILED - {exc}", file=sys.stderr)

    out_json = CONFIG / f"{stem}.opcodes.json"
    out_json.write_text(json.dumps({"channels": channels}, indent=2))

    WIRE.mkdir(parents=True, exist_ok=True)
    out_md = WIRE / f"{stem}.opcodes.md"
    with out_md.open("w", encoding="utf-8") as f:
        f.write(f"# {stem}.exe - opcode to vtable-slot map\n\n")
        f.write("Generated by `tools/extract_opcode_dispatch.py` from the client dispatch tables.\n\n")
        f.write("## Summary\n\n")
        f.write(f"- {sum(c['real_opcode_count'] for c in channels)} real Down opcodes across {len(channels)} channels.\n\n")
        for ch in channels:
            real = [r for r in ch["opcodes"] if r["case_idx"] != ch["default_case_idx"]]
            f.write(f"## {ch['channel']} Down channel\n\n")
            f.write(f"- Callback interface: `{ch['callback_iface']}`\n")
            f.write(f"- Dispatcher RVA: `{ch['dispatcher_rva']}`\n")
            f.write(f"- Opcode range: `{ch['opcode_base']:#x}` through `{ch['opcode_base'] + ch['opcode_count'] - 1:#x}`\n\n")
            f.write("| opcode | hex | case | vtable slot |\n")
            f.write("|---:|---:|---:|---:|\n")
            for row in real:
                f.write(f"| {row['opcode']} | `{row['opcode_hex']}` | {row['case_idx']} | {row['vtable_slot']} |\n")
            f.write("\n")

    print(f"wrote: {out_json.relative_to(REPO_ROOT)}")
    print(f"       {out_md.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
