#!/usr/bin/env python3
"""Recover the native Lua class registrations in ffxivgame.exe.

Sites: the registrar 0x0075F9B0 and the Player registration inside 0x0078FC90.
Each builds a parent name string and then a child name string in two
frame slots, then calls 0x00CC9700 (object, name, parent) or 0x00CC9760
(name, parent). Two independent readings must agree for every row: slot
tracking of the pushed argument addresses, and the order of the two string
constructions before the call.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
from typing import Callable, NamedTuple, Sequence

EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
EXPECTED_SIZE = 15_996_808
# (first instruction VA, last byte VA, VA where the stack is balanced and
# frame offsets are measured from)
WINDOWS = (
    (0x0075F9B0, 0x007616AB, 0x0075F9C4),
    (0x0078FFAF, 0x0078FFF9, 0x0078FFAF),
)
STRING_CTOR = 0x00447260
STRING_DTOR = 0x00446F50
ALLOCATOR = 0x009D1B35
REGISTER_WITH_OBJECT = 0x00CC9700
REGISTER_SCRIPT_ONLY = 0x00CC9760


class Ins(NamedTuple):
    address: int
    mnemonic: str
    op_str: str


class Registration(NamedTuple):
    call_va: int
    callee: int
    name: str
    parent: str
    name_literal_va: int
    parent_literal_va: int
    alloc_size: int | None
    constructor_va: int | None


def _imm(text: str) -> int | None:
    return int(text, 0) if re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", text) else None


def trace(
    instructions: Sequence[Ins],
    read_cstr: Callable[[int], str],
    stack_cleanup: Callable[[int], int],
    frame_base_va: int,
) -> list[Registration]:
    """Return registrations in address order, cross-checked two ways."""
    delta = 0
    started = False
    regs: dict[str, tuple[str, int]] = {}
    pushed: list[tuple[str, int] | None] = []
    slots: dict[int, tuple[int, int]] = {}
    constructions: list[int] = []
    alloc: tuple[int, int | None] | None = None
    rows: list[Registration] = []

    for ins in instructions:
        mnem, ops = ins.mnemonic, ins.op_str
        if ins.address == frame_base_va:
            started = True
        if not started:
            continue
        if mnem == "push":
            value = _imm(ops)
            pushed.append(("imm", value) if value is not None else regs.get(ops))
            delta -= 4
        elif mnem == "pop":
            delta += 4
            if pushed:
                pushed.pop()
        elif mnem == "sub" and ops.startswith("esp, "):
            delta -= int(ops.split(", ")[1], 0)
        elif mnem == "add" and ops.startswith("esp, "):
            count = int(ops.split(", ")[1], 0)
            delta += count
            del pushed[max(0, len(pushed) - count // 4) :]
        elif mnem == "lea":
            match = re.fullmatch(r"(\w+), \[esp(?: \+ (0x[0-9a-fA-F]+|\d+))?\]", ops)
            if match:
                offset = int(match.group(2) or "0", 0)
                regs[match.group(1)] = ("frame", delta + offset)
        elif mnem == "mov":
            match = re.fullmatch(r"(\w+), (\w+)", ops)
            if match and match.group(2) in regs:
                regs[match.group(1)] = regs[match.group(2)]
        elif mnem == "call":
            target = _imm(ops)
            if target is None:
                continue
            if alloc is not None and alloc[1] is None and target != ALLOCATOR:
                alloc = (alloc[0], target)
            if target == STRING_CTOR:
                literal, slot = pushed[-1], regs.get("ecx")
                if literal and literal[0] == "imm" and slot and slot[0] == "frame":
                    slots[slot[1]] = (literal[1], ins.address)
                    constructions.append(literal[1])
                del pushed[-2:]
                delta += 8
            elif target == ALLOCATOR:
                size = pushed[-1][1] if pushed and pushed[-1] else None
                alloc = (size, None) if size is not None else None
            elif target in (REGISTER_WITH_OBJECT, REGISTER_SCRIPT_ONLY):
                with_object = target == REGISTER_WITH_OBJECT
                count = 3 if with_object else 2
                args = pushed[-count:][::-1]  # last pushed is argument 1
                name_arg, parent_arg = (args[1], args[2]) if with_object else args[:2]
                name_slot = slots[name_arg[1]]
                parent_slot = slots[parent_arg[1]]
                if constructions[-2:] != [parent_slot[0], name_slot[0]]:
                    raise ValueError(
                        f"construction order disagrees at {ins.address:#010x}"
                    )
                rows.append(
                    Registration(
                        ins.address,
                        target,
                        read_cstr(name_slot[0]),
                        read_cstr(parent_slot[0]),
                        name_slot[0],
                        parent_slot[0],
                        alloc[0] if with_object and alloc else None,
                        alloc[1] if with_object and alloc else None,
                    )
                )
                del pushed[-count:]
                delta += 4 * count
                alloc = None
                constructions.clear()
            elif target != STRING_DTOR:
                cleanup = stack_cleanup(target)
                delta += cleanup
                del pushed[max(0, len(pushed) - cleanup // 4) :]
    return rows


class Image:
    """Minimal PE32 view: section map and VA reads over the original bytes."""

    def __init__(self, data: bytes) -> None:
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        self.base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        table = pe + 24 + optional_size
        self.sections = [
            struct.unpack_from("<8sIIII", data, table + 40 * index)
            for index in range(count)
        ]

    def offset(self, va: int) -> int:
        rva = va - self.base
        for _, vsize, address, rsize, raw in self.sections:
            if address <= rva < address + max(vsize, rsize):
                return raw + rva - address
        raise ValueError(f"VA {va:#010x} is outside every section")

    def read(self, va: int, size: int) -> bytes:
        start = self.offset(va)
        return self.data[start : start + size]

    def cstr(self, va: int) -> str:
        start = self.offset(va)
        return self.data[start : self.data.index(b"\0", start)].decode("ascii")


def load_instructions(image: Image, start: int, end: int) -> list[Ins]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    code = image.read(start, end - start + 16)
    return [Ins(i.address, i.mnemonic, i.op_str) for i in md.disasm(code, start)]


def callee_cleanup(image: Image) -> Callable[[int], int]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    cache: dict[int, int] = {}

    def cleanup(target: int) -> int:
        if target not in cache:
            cache[target] = 0
            for i in md.disasm(image.read(target, 2000), target):
                if i.mnemonic == "ret":
                    cache[target] = int(i.op_str, 0) if i.op_str else 0
                    break
        return cache[target]

    return cleanup


def find_callers(image: Image, targets: Sequence[int]) -> dict[int, list[int]]:
    """Byte-scan .text for E8/E9 rel32 instructions aimed at each target."""
    found: dict[int, list[int]] = {target: [] for target in targets}
    for name, vsize, address, rsize, raw in image.sections:
        if name.rstrip(b"\0") != b".text":
            continue
        code = image.data[raw : raw + min(vsize, rsize)]
        for opcode in (0xE8, 0xE9):
            at = code.find(bytes([opcode]))
            while at != -1 and at + 5 <= len(code):
                site = image.base + address + at
                rel = struct.unpack_from("<i", code, at + 1)[0]
                target = (site + 5 + rel) & 0xFFFFFFFF
                if target in found:
                    found[target].append(site)
                at = code.find(bytes([opcode]), at + 1)
    return {target: sorted(sites) for target, sites in found.items()}


def verify_identity(data: bytes) -> None:
    if len(data) != EXPECTED_SIZE:
        raise SystemExit(f"size {len(data)} != {EXPECTED_SIZE}")
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA256:
        raise SystemExit("SHA-256 does not match retail 1.23b ffxivgame.exe")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("orig/ffxivgame.exe"))
    parser.add_argument("--json", action="store_true", help="emit JSON rows")
    parser.add_argument(
        "--callers",
        action="store_true",
        help="list every direct E8/E9 rel32 caller of the two register entries",
    )
    args = parser.parse_args(argv)
    data = args.exe.read_bytes()
    verify_identity(data)
    image = Image(data)
    if args.callers:
        callers = find_callers(image, (REGISTER_WITH_OBJECT, REGISTER_SCRIPT_ONLY))
        for target, sites in callers.items():
            print(f"{target:#010x} {len(sites)} callers")
            for site in sites:
                print(f"  {site:#010x}")
        return 0
    try:
        cleanup = callee_cleanup(image)
        rows = []
        for start, end, base in WINDOWS:
            instructions = load_instructions(image, start, end)
            rows += trace(instructions, image.cstr, cleanup, base)
    except ImportError:
        raise SystemExit("Capstone is required: python -m pip install capstone")
    if args.json:
        print(json.dumps([r._asdict() for r in rows], indent=1))
        return 0
    for r in rows:
        alloc = (
            ""
            if r.alloc_size is None
            else f" {r.alloc_size:#x} {r.constructor_va:#010x}"
        )
        print(
            f"{r.call_va:#010x} {r.callee:#010x} {r.name} <- "
            f"{r.parent or '(empty)'}{alloc}"
        )
    print(f"{len(rows)} registrations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
