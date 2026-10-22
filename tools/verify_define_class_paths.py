#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C)  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify the _defineClass and _defineBaseClass control flow in ffxivgame.exe.

Three checks, all over the pinned executable:

1. Instruction listings at the registration, argument, link, close and failure
   sites recorded in docs/script/define-class-repeat-definition.md.
2. The direct E8/E9 caller sets of the entries that page relies on.
3. The three ErrorHandler vtables, whose failure-report slots are empty.

With --scripts it also decodes every shipped script under <install>/client/script
and counts the _defineClass and _defineBaseClass call sites.
"""

from __future__ import annotations

import argparse
import struct
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterable, NamedTuple, Sequence

import extract_class_registrations as ecr

DIRECT_CALLERS = {
    0x0078C2A0: [0x006DCC6B],
    0x0078C330: [0x006DCCDB],
    0x00CC7050: [0x0078C311, 0x0078C3A1],
    0x00CD91E0: [0x00CDA600, 0x00CDA641],
    0x00CE1600: [0x00CD919F, 0x00CD933D, 0x00CD9717],
    0x00CD9C10: [0x00CC71F2],
    0x00CD9BC0: [0x00CC71E2],
}
# Shim callers outside the 36-class registry function, and the count inside it.
SHIM_CALLERS = {
    0x00CC71F0: ([0x00763B14, 0x00763B4F, 0x00772DB0, 0x00772DBC, 0x0078C31A], 12),
    0x00CC71E0: ([0x00763A9F, 0x00763AD9, 0x0078C3AA], 24),
}
REGISTRY_RANGE = (0x0078E3A0, 0x0078EB62)
CLOSE_CLASS = 0x00CC71F0
CLOSE_BASE = 0x00CC71E0
# vtable VA, RTTI class name
ERROR_HANDLERS = (
    (0x0110E564, ".?AVErrorHandler@GameEngine@Lua@Component@@"),
    (0x00FE0184, ".?AVErrorHandler@Client@Script@Lua@Application@@"),
    (0x00FE0FAC, ".?AVErrorHandler@Common@Script@Lua@Application@@"),
)
REPORT_SLOTS = (0x58, 0x64, 0x68, 0x80)
RET_8 = bytes.fromhex("C20800")

LITERALS = ((0x00FD7354, "_defineClass"), (0x00FD7364, "_defineBaseClass"))


CHECKS: tuple[tuple[str, int, str], ...] = (
    ("define_registration", 0x0073C2A7, "mov esi, 0x6dcc30"),
    ("define_name_push", 0x0073C359, "push 0xfd7354"),
    ("define_register_call", 0x0073C389, "call 0xcccad0"),
    ("base_registration", 0x0073C3F7, "mov esi, 0x6dcca0"),
    ("base_name_push", 0x0073C4A9, "push 0xfd7364"),
    ("base_register_call", 0x0073C4D9, "call 0xcccad0"),
    ("thunk_define", 0x006DCC6B, "call 0x78c2a0"),
    ("thunk_base", 0x006DCCDB, "call 0x78c330"),
    (
        "inner_define",
        0x0078C30C,
        "mov ecx, dword ptr [edi + 4]; push eax; push ebp; call 0xcc7050; mov ecx, dword ptr [edi + 4]; push ebp; call 0xcc71f0; pop edi; pop esi; pop ebp; pop ebx; ret 4",
    ),
    (
        "inner_base",
        0x0078C39C,
        "mov ecx, dword ptr [edi + 4]; push eax; push ebp; call 0xcc7050; mov ecx, dword ptr [edi + 4]; push ebp; call 0xcc71e0; pop edi; pop esi; pop ebp; pop ebx; ret 4",
    ),
    ("link_entry", 0x00CC7050, "mov ecx, dword ptr [ecx]; jmp 0xcda600"),
    ("link_forward", 0x00CDA600, "jmp 0xcd91e0"),
    ("close_class_entry", 0x00CC71F0, "mov ecx, dword ptr [ecx]; jmp 0xcd9c10"),
    ("close_base_entry", 0x00CC71E0, "mov ecx, dword ptr [ecx]; jmp 0xcd9bc0"),
    (
        "close_class",
        0x00CD9C10,
        "mov eax, dword ptr [esp + 4]; push 1; push eax; call 0xcd8870; test eax, eax; je 0xcd9c24",
    ),
    ("close_class_flag", 0x00CD9C20, "mov byte ptr [eax + 0x7c], 0"),
    (
        "close_base",
        0x00CD9BC0,
        "mov eax, dword ptr [esp + 4]; push 1; push eax; call 0xcd8870; test eax, eax; je 0xcd9bd4",
    ),
    ("close_base_flag", 0x00CD9BD0, "mov byte ptr [eax + 0x7b], 0"),
    (
        "parent_lookup",
        0x00CD9209,
        "mov edi, dword ptr [esp + 0x3c]; push 1; push edi; call 0xcd8870",
    ),
    ("parent_missing", 0x00CD9217, "test ebp, ebp; je 0xcd9342; mov ecx, ebp"),
    (
        "parent_kind",
        0x00CD921F,
        "mov ecx, ebp; call 0xce16c0; test al, al; je 0xcd923b; mov ecx, dword ptr [esi + 0x1cc]; push edi",
    ),
    (
        "parent_kind_report",
        0x00CD922A,
        "mov ecx, dword ptr [esi + 0x1cc]; push edi; call 0xcd5120; jmp 0xcd9342",
    ),
    ("parent_flag", 0x00CD923B, "cmp byte ptr [ebp + 0x7c], 0; jne 0xcd9252"),
    (
        "parent_flag_report",
        0x00CD9241,
        "mov ecx, dword ptr [esi + 0x1cc]; push edi; call 0xcd52a0; jmp 0xcd9342",
    ),
    (
        "global_probe",
        0x00CD9256,
        "lea edi, [esi + 8]; push ebx; mov ecx, edi; call 0xcf3340; mov ecx, edi; call 0xcf3a50; push -1; mov ecx, edi; call 0xcf3740; test al, al",
    ),
    ("table_branch", 0x00CD9271, "test al, al; jne 0xcd9321; push 1"),
    (
        "repeat_path",
        0x00CD9321,
        "push -1; push edi; mov ecx, esi; call 0xcd77d0; push 1; mov ecx, edi; mov esi, eax; call 0xcf3860; test esi, esi; je 0xcd9342; push ebp; mov ecx, esi; call 0xce1600; mov ecx, dword ptr [esp + 0x28]",
    ),
    (
        "define_new_lookup",
        0x00CD9282,
        "push ebx; lea eax, [esp + 0x3c]; push eax; mov ecx, ebp; call 0xcde4c0; push eax; mov ecx, esi; call 0xcd7970; mov edi, eax; test edi, edi; push ebx",
    ),
    (
        "new_refused",
        0x00CD929B,
        "push ebx; jne 0xcd92ae; mov ecx, dword ptr [esi + 0x1cc]; call 0xcd5620; jmp 0xcd9342; mov ecx, dword ptr [esi + 0x18]",
    ),
    (
        "new_register",
        0x00CD92F5,
        "mov edx, dword ptr [esp + 0x3c]; push edx; push ecx; mov eax, esp; mov ecx, esi; mov dword ptr [esp + 0x44], esp; mov dword ptr [esp + 0x40], ebx; mov dword ptr [eax], edi; call 0xcd9160; lea ecx, [esp + 0x38]",
    ),
    (
        "nil_probe",
        0x00CD7988,
        "call 0xcf3340; mov ecx, esi; call 0xcf3a50; push -1; mov ecx, esi; call 0xcf3760; test al, al; push 1; mov ecx, esi; sete bl; call 0xcf3860; test bl, bl; je 0xcd79b8; pop edi; pop esi",
    ),
    (
        "publish",
        0x00CD79F7,
        "push ebp; mov ecx, esi; call 0xcf3340; push -2; mov ecx, esi; call 0xcf3380; mov ecx, esi; call 0xcf3a70; push 2; mov ecx, esi",
    ),
    (
        "relink",
        0x00CE1600,
        "mov eax, dword ptr [esp + 4]; test eax, eax; mov dword ptr [ecx + 0xc], eax; je 0xce161a; add eax, 0x50; mov dword ptr [esp + 4], eax; add ecx, 0x60; jmp 0xcd29d0",
    ),
    (
        "record_ctor_flags",
        0x00CE2C66,
        "mov byte ptr [esi + 0x7a], al; mov al, 1; mov byte ptr [esi + 0x7b], al; mov byte ptr [esi + 0x7c], al; mov byte ptr [esi + 0x7f], al; mov word ptr [esi + 0x74], bx; mov word ptr [esi + 0x76], bx; mov word ptr [esi + 0x78], bx; mov byte ptr [esi + 0x7d], bl; mov byte ptr [esi + 0x7e], bl; mov byte ptr [esi + 0x80], bl; mov dword ptr [ecx], esi",
    ),
    (
        "kind_test",
        0x00CE16C0,
        "mov eax, dword ptr [ecx + 8]; cmp eax, dword ptr [0x130d4fc]; je 0xce16de; cmp eax, dword ptr [0x130d500]; je 0xce16de; cmp eax, dword ptr [0x130d504]; je 0xce16de; xor eax, eax; ret; mov eax, 1",
    ),
    (
        "globals_rawget",
        0x00CF3A50,
        "mov eax, dword ptr [ecx]; push 0xffffd8ee; push eax; call 0xdce460; add esp, 8",
    ),
    (
        "is_table",
        0x00CF3740,
        "mov eax, dword ptr [esp + 4]; mov ecx, dword ptr [ecx]; push eax; push ecx; call 0xdcdce0; xor edx, edx; add esp, 8; cmp eax, 5; sete dl",
    ),
    (
        "is_nil",
        0x00CF3760,
        "mov eax, dword ptr [esp + 4]; mov ecx, dword ptr [ecx]; push eax; push ecx; call 0xdcdce0; add esp, 8; neg eax; sbb eax, eax; add eax, 1",
    ),
    (
        "report_missing_lookup",
        0x00CD8944,
        "cmp byte ptr [esp + 0x48], 0; je 0xcd895d; mov ecx, dword ptr [esp + 0x44]; mov esi, dword ptr [esi + 0x1cc]; push ecx; mov ecx, esi; call 0xcd5320; lea ecx, [esp + 0x24]; mov dword ptr [esp + 0x3c], 0xffffffff",
    ),
    (
        "report58",
        0x00CD515B,
        "mov edx, dword ptr [esi]; mov edx, dword ptr [edx + 0x58]; push ecx",
    ),
    (
        "report64",
        0x00CD52DB,
        "mov edx, dword ptr [esi]; mov edx, dword ptr [edx + 0x64]; push ecx",
    ),
    (
        "report68",
        0x00CD535B,
        "mov edx, dword ptr [esi]; mov edx, dword ptr [edx + 0x68]; push ecx",
    ),
    (
        "report80",
        0x00CD565B,
        "mov edx, dword ptr [esi]; mov edx, dword ptr [edx + 0x80]; push ecx",
    ),
)


class Site(NamedTuple):
    api: str
    child: str | None
    parent: str | None


def disassemble(image: ecr.Image, va: int, count: int) -> list[str]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    code = image.read(va, 16 * count)
    rows = [f"{i.mnemonic} {i.op_str}".strip() for i in md.disasm(code, va)]
    return rows[:count]


def check_listings(image: ecr.Image) -> list[str]:
    errors = []
    for label, va, text in CHECKS:
        want = text.split("; ")
        got = disassemble(image, va, len(want))
        if got != want:
            errors.append(f"listing {label} at {va:#010x} differs: {got}")
    for va, text in LITERALS:
        if image.cstr(va) != text:
            errors.append(f"literal at {va:#010x} is not {text}")
    return errors


def check_callers(image: ecr.Image) -> list[str]:
    errors = []
    targets = [*DIRECT_CALLERS, *SHIM_CALLERS]
    found = ecr.find_callers(image, targets)
    for target, want in DIRECT_CALLERS.items():
        if found[target] != want:
            errors.append(f"callers of {target:#010x} are {found[target]}")
    low, high = REGISTRY_RANGE
    for target, (outside, inside) in SHIM_CALLERS.items():
        sites = found[target]
        if [s for s in sites if not low <= s < high] != outside:
            errors.append(f"callers of {target:#010x} outside the registry differ")
        if len([s for s in sites if low <= s < high]) != inside:
            errors.append(f"registry call count of {target:#010x} differs")
    return errors


def check_error_handlers(image: ecr.Image) -> list[str]:
    errors = []
    for vtable, rtti in ERROR_HANDLERS:
        locator = struct.unpack("<I", image.read(vtable - 4, 4))[0]
        descriptor = struct.unpack("<I", image.read(locator + 12, 4))[0]
        if image.cstr(descriptor + 8) != rtti:
            errors.append(f"vtable {vtable:#010x} is not {rtti}")
        for slot in REPORT_SLOTS:
            target = struct.unpack("<I", image.read(vtable + slot, 4))[0]
            if image.read(target, 3) != RET_8:
                errors.append(f"vtable {vtable:#010x} slot {slot:#x} is not ret 8")
    return errors


def verify(image: ecr.Image) -> list[str]:
    return check_listings(image) + check_callers(image) + check_error_handlers(image)


class Reader:
    """Lua 5.1 bytecode reader for the shipped layout: 4-byte int, size_t, instruction."""

    def __init__(self, data: bytes) -> None:
        if data[:5] != b"\x1bLua\x51" or data[7:12] != bytes([4, 4, 4, 8, 0]):
            raise ValueError("not the shipped Lua 5.1 layout")
        self.data = data
        self.pos = 12
        self.fmt = "<I" if data[6] == 1 else ">I"

    def u8(self) -> int:
        self.pos += 1
        return self.data[self.pos - 1]

    def u32(self) -> int:
        self.pos += 4
        return struct.unpack_from(self.fmt, self.data, self.pos - 4)[0]

    def take(self, size: int) -> bytes:
        self.pos += size
        return self.data[self.pos - size : self.pos]

    def string(self) -> bytes | None:
        size = self.u32()
        return self.take(size)[:-1] if size else None

    def function(self) -> tuple[list[int], list[object], list[tuple]]:
        self.string()
        self.pos += 8
        self.pos += 4
        code = [self.u32() for _ in range(self.u32())]
        constants: list[object] = []
        for _ in range(self.u32()):
            kind = self.u8()
            if kind == 0:
                constants.append(None)
            elif kind == 1:
                constants.append(bool(self.u8()))
            elif kind == 3:
                constants.append(self.take(8))
            elif kind == 4:
                constants.append(self.string())
            else:
                raise ValueError(f"constant type {kind}")
        children = [self.function() for _ in range(self.u32())]
        for _ in range(self.u32()):
            self.u32()
        for _ in range(self.u32()):
            self.string()
            self.pos += 8
        for _ in range(self.u32()):
            self.string()
        return code, constants, children


GETGLOBAL, LOADK, CALL = 5, 1, 28


def define_sites(chunk: bytes) -> list[Site]:
    """Every GETGLOBAL of a define native, then LOADK, LOADK, CALL (B=3, C=1) on the next registers."""
    out: list[Site] = []
    pending = [Reader(chunk).function()]
    while pending:
        code, constants, children = pending.pop()
        pending.extend(children)
        for at, word in enumerate(code):
            if word & 0x3F != GETGLOBAL:
                continue
            name = constants[word >> 14]
            if name not in (b"_defineClass", b"_defineBaseClass"):
                continue
            reg = (word >> 6) & 0xFF
            window = code[at + 1 : at + 4]
            literal = (
                len(window) == 3
                and all(
                    w & 0x3F == op and (w >> 6) & 0xFF == r
                    for w, op, r in zip(
                        window, (LOADK, LOADK, CALL), (reg + 1, reg + 2, reg)
                    )
                )
                and window[2] >> 23 == 3
                and (window[2] >> 14) & 0x1FF == 1
            )
            child = parent = None
            if literal:
                child, parent = (constants[w >> 14] for w in window[:2])
            if isinstance(child, bytes) and isinstance(parent, bytes):
                out.append(Site(name.decode(), child.decode(), parent.decode()))
            else:
                out.append(Site(name.decode(), None, None))
    return out


def summarize(
    sites: Sequence[Site], native: Iterable[str], class_closed: Iterable[str] = ()
) -> dict[str, int]:
    native = set(native)
    named = [s for s in sites if s.child is not None]
    by_child: dict[str, set[tuple[str, str | None]]] = defaultdict(set)
    count: dict[str, int] = defaultdict(int)
    for site in named:
        by_child[site.child].add((site.api, site.parent))
        count[site.child] += 1
    repeated = [c for c, n in count.items() if n > 1]
    class_defined = {s.child for s in named if s.api == "_defineClass"}
    base_defined = {s.child for s in named if s.api == "_defineBaseClass"}
    parents = {s.parent for s in named}
    return {
        "sites": len(sites),
        "_defineClass": sum(s.api == "_defineClass" for s in sites),
        "_defineBaseClass": sum(s.api == "_defineBaseClass" for s in sites),
        "non-literal sites": len(sites) - len(named),
        "distinct child names": len(count),
        "names at more than one site": len(repeated),
        "most sites for one name": max(count.values(), default=0),
        "repeated with the same parent": sum(
            len({p for _, p in by_child[c]}) == 1 for c in repeated
        ),
        "repeated with different parents": sum(
            len({p for _, p in by_child[c]}) > 1 for c in repeated
        ),
        "repeated across both define natives": sum(
            len({a for a, _ in by_child[c]}) > 1 for c in repeated
        ),
        "child names that are native registrations": len(set(count) & native),
        "parents defined by _defineClass": len(parents & class_defined),
        "parents neither native nor _defineBaseClass": len(
            parents - native - base_defined
        ),
        "parents closed through 0x00CC71F0 by the registry function": len(
            parents & set(class_closed)
        ),
    }


def registry_closures(image: ecr.Image) -> dict[int, list[str]]:
    """Class names the registry function closes, by shim: the literal pushed before each call."""
    import re

    low, high = REGISTRY_RANGE
    out: dict[int, list[str]] = {CLOSE_CLASS: [], CLOSE_BASE: []}
    literal = None
    for ins in ecr.load_instructions(image, low, high):
        if ins.address >= high:
            break
        if ins.mnemonic == "push" and re.fullmatch(r"0x[0-9a-f]{6,8}", ins.op_str):
            literal = int(ins.op_str, 16)
        elif ins.mnemonic == "call" and literal is not None:
            target = int(ins.op_str, 0) if ins.op_str.startswith("0x") else 0
            if target in out:
                out[target].append(image.cstr(literal))
    return out


def census(
    root: Path, native: Iterable[str], class_closed: Iterable[str] = ()
) -> dict[str, int]:
    import decode_lpb

    sites: list[Site] = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        chunk = decode_lpb.decode_lpb(path.read_bytes())
        if chunk is not None:
            sites += define_sites(chunk)
    return summarize(sites, native, class_closed)


def native_names(image: ecr.Image) -> list[str]:
    cleanup = ecr.callee_cleanup(image)
    names: list[str] = []
    for start, end, base in ecr.WINDOWS:
        rows = ecr.trace(
            ecr.load_instructions(image, start, end), image.cstr, cleanup, base
        )
        names += [row.name for row in rows]
    return names


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("orig/ffxivgame.exe"))
    parser.add_argument(
        "--scripts",
        type=Path,
        help="<install>/client/script, to count the define call sites",
    )
    args = parser.parse_args(argv)
    data = args.exe.read_bytes()
    ecr.verify_identity(data)
    image = ecr.Image(data)
    try:
        errors = verify(image)
        names = native_names(image) if args.scripts else []
        closed = registry_closures(image)[CLOSE_CLASS] if args.scripts else []
    except ImportError:
        raise SystemExit("Capstone is required: python -m pip install capstone")
    for error in errors:
        print(f"FAIL {error}")
    print(
        f"{len(CHECKS)} listings, {len(DIRECT_CALLERS) + len(SHIM_CALLERS)} caller sets, "
        f"{len(ERROR_HANDLERS)} vtables: {'FAIL' if errors else 'ok'}"
    )
    if args.scripts:
        for label, value in census(args.scripts, names, closed).items():
            print(f"{label}: {value}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
