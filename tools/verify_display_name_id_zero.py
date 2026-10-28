#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C)  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify the SetActorName (0x013D) display-name id facts in ffxivgame.exe.

Check groups over the pinned executable:

1. Instruction listings for the packet dispatch, the two ActorImpl slot-62
   handlers that decode the payload, the receiver constructor and apply
   routines, the name-structure writers, the text-context push and reset
   routines, the variant accessors and the macro operand decoders, recorded in
   docs/resource/display-name-id-zero.md.
2. Table slots with RTTI names, the dispatch class table and one data word.
3. The direct E8/E9 caller sets of the entries the page relies on.

Optional inputs add an export check (--text-ui-csv, --display-name-csv) of row
222 of xtx/_text_ui against the sheet xtx/displayName, and a census check
(--samples) of the retail 0x013D capture samples.

The module also holds reference models of the payload decode, the apply rule,
the text-context contents and the row 222 macro, so the tests state their
expected results without running the code they check. Without --exe the
command prints a SKIP line and exits 0.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
from typing import Mapping, NamedTuple

import extract_class_registrations as ecr

NO_DISPLAY_NAME = 0xFFFFFFFF
PAYLOAD_SIZE = 40
TEXT_OFFSET = 4
TEXT_CAPACITY = 32
FLAG_OFFSET = 0x24
CONTEXT_STRING_INDEX = 1
CONTEXT_FLAG_INDEX = 4
CONTEXT_ID_INDEX = 6

TEXT_UI_CSV = (
    "xtx__text_ui.csv",
    1284564,
    "23762a422118d99c4144a48b3495c52511b1b1b865e4fc50fdc61a973e3c6ece",
)
DISPLAY_NAME_CSV = (
    "xtx_displayName.csv",
    838262,
    "36c5dfba312695d9f4ee090dba123dfd7792feba55d88b0062c070d98abc0505",
)
DISPLAY_NAME_DATA_ROWS = 5813
ROW_222_KEY = "222"

EXPECTED_CENSUS = {
    "samples": 55,
    "id_zero": 22,
    "id_ffffffff": 5,
    "id_other": 28,
    "distinct_ids": 22,
    "distinct_other": 20,
    "payload_bytes": 40,
    "tail_after_id_all_zero": True,
    "header_word_8_min": 1356917211,
    "header_word_8_max": 1356946277,
    "header_word_12_all_zero": True,
}


class WireName(NamedTuple):
    id: int
    text: bytes
    flag: int


class Outcome(NamedTuple):
    kind: str  # "ignored", "custom" or "lookup"
    id: int
    text: bytes
    flag: int


def parse_payload(payload: bytes) -> WireName:
    """Reference model of the payload decode at 0x0076C512..0x0076C58B.

    Dword at +0, a 32-byte buffer at +4 that is read as a NUL-terminated
    string, and the byte at +0x24.
    """
    if len(payload) < FLAG_OFFSET + 1:
        raise ValueError("payload shorter than 0x25 bytes")
    ident = struct.unpack_from("<I", payload, 0)[0]
    raw = payload[TEXT_OFFSET : TEXT_OFFSET + TEXT_CAPACITY]
    return WireName(ident, raw.split(b"\0", 1)[0], payload[FLAG_OFFSET])


def apply_wire(wire: WireName) -> Outcome:
    """Reference model of 0x0076C4D0 plus the constructor 0x0089D530.

    A wire id of 0 becomes 0xFFFFFFFF. That id with an empty string and a zero
    flag byte makes no receiver. Otherwise the id 0xFFFFFFFF stores the string
    as the name and any other id stores the id for a sheet lookup.
    """
    ident = NO_DISPLAY_NAME if wire.id == 0 else wire.id
    if ident == NO_DISPLAY_NAME and wire.flag == 0 and wire.text == b"":
        return Outcome("ignored", ident, wire.text, wire.flag)
    kind = "custom" if ident == NO_DISPLAY_NAME else "lookup"
    return Outcome(kind, ident, wire.text, wire.flag)


class NameState(NamedTuple):
    id: int
    text: bytes
    flag: int
    resolved: bool  # byte +0x59


DEFAULT_NAME = NameState(NO_DISPLAY_NAME, b"", 0, True)


def store_name(outcome: Outcome) -> NameState:
    """Reference model of 0x006FAFF0 for an applied outcome.

    Byte +0x59 is 1 only when the stored id is 0xFFFFFFFF.
    """
    return NameState(
        outcome.id, outcome.text, outcome.flag, outcome.id == NO_DISPLAY_NAME
    )


class Context(NamedTuple):
    string: bytes
    flag: int
    ident: int


def name_context(state: NameState) -> Context:
    """Reference model of the text-context push 0x00769AE0.

    Element 1 is a string, element 4 and element 6 are integers.
    """
    if state.id == NO_DISPLAY_NAME:
        return Context(state.text, state.flag, 0)
    return Context(b"", 0, state.id)


def row_222(context: Context, display_names: Mapping[int, bytes]) -> bytes | None:
    """Reference model of the first `08` tag of every row 222 cell.

    Integer element 6 nonzero selects the xtx/displayName row of that id.
    Zero selects string element 1. None means the sheet has no such row.
    """
    if context.ident != 0:
        return display_names.get(context.ident)
    return context.string


def speaker_name(
    wire: WireName,
    display_names: Mapping[int, bytes],
    current: NameState = DEFAULT_NAME,
) -> bytes | None:
    """Name the sink hands to the speaker field after one 0x013D packet."""
    outcome = apply_wire(wire)
    state = current if outcome.kind == "ignored" else store_name(outcome)
    if state.resolved:
        return state.text
    return row_222(name_context(state), display_names)


# label, VA, expected listing
CHECKS: tuple[tuple[str, int, str], ...] = (
    (
        "dispatch_opcode_class",
        0x004D88C0,
        "movzx eax, word ptr [edi + 2]; add eax, 0xffffff34; cmp eax, 0xdc; ja 0x4d8bbb; movzx eax, byte ptr [eax + 0x4d8c24]; jmp dword ptr [eax*4 + 0x4d8bc0]",
    ),
    (
        "dispatch_case_slot",
        0x004D8A36,
        "mov ecx, dword ptr [esi + 0x80]; test ecx, ecx; je 0x4d8bbb; add edi, 0x10; push edi; call 0x575100",
    ),
    (
        "dispatch_slot_thunk",
        0x00575100,
        "mov ecx, dword ptr [ecx]; mov eax, dword ptr [ecx]; mov eax, dword ptr [eax + 0xf8]; jmp eax",
    ),
    (
        "lua_wire_id_map",
        0x0076C512,
        "mov eax, dword ptr [edi]; xor ebx, ebx; cmp eax, ebx; mov dword ptr [esp + 0x30], ecx; mov dword ptr [esp + 0x18], 0xffffffff; je 0x76c52a; mov dword ptr [esp + 0x18], eax; mov al, byte ptr [edi + 0x24]; push ebx",
    ),
    (
        "lua_wire_string_copy",
        0x0076C55D,
        "movq xmm0, qword ptr [edi + 4]; movq qword ptr [esi], xmm0; movq xmm0, qword ptr [edi + 0xc]; movq qword ptr [esi + 8], xmm0; movq xmm0, qword ptr [edi + 0x14]; movq qword ptr [esi + 0x10], xmm0; movq xmm0, qword ptr [edi + 0x1c]; movq qword ptr [esi + 0x18], xmm0",
    ),
    (
        "lua_wire_drop",
        0x0076C5A4,
        "or ebp, 0xffffffff; cmp dword ptr [esp + 0x18], ebp; jne 0x76c5e0; cmp byte ptr [esp + 0x17], bl; jne 0x76c5e0; mov eax, dword ptr [esp + 0x34]; cmp byte ptr [eax], bl; jne 0x76c5e0; mov byte ptr [esp + 0xf4], bl",
    ),
    (
        "lua_construct_apply",
        0x0076C5E0,
        "movzx ecx, byte ptr [edi + 0x24]; mov eax, dword ptr [edi]; push ecx; lea edx, [esp + 0x38]; push edx; push eax; lea ecx, [esp + 0x94]; call 0x89d530; mov byte ptr [esp + 0xf4], 2; mov eax, dword ptr [esp + 0x30]; mov ecx, dword ptr [eax + 8]; push ecx; add eax, 4; push eax; lea ecx, [esp + 0x90]; call 0x89d610",
    ),
    (
        "null_wire_id_map",
        0x007719CB,
        "mov eax, dword ptr [edi]; cmp eax, ebx; mov dword ptr [esp + 0x20], 0xffffffff; je 0x7719dd; mov dword ptr [esp + 0x20], eax; mov al, byte ptr [edi + 0x24]; push ebx",
    ),
    (
        "null_wire_drop",
        0x00771A57,
        "mov ebp, dword ptr [esp + 0x20]; cmp ebp, -1; jne 0x771a93; cmp byte ptr [esp + 0x1c], bl; jne 0x771a93; mov eax, dword ptr [esp + 0x34]; cmp byte ptr [eax], bl; jne 0x771a93",
    ),
    (
        "null_inert_gate",
        0x007719BE,
        "cmp byte ptr [ecx + 0x52], bl; mov dword ptr [esp + 0x14], ecx; jne 0x771b59",
    ),
    ("null_queue_gate", 0x00771A97, "cmp byte ptr [edi + 0x50], bl; je 0x771b07"),
    (
        "null_direct_store",
        0x00771269,
        "mov eax, dword ptr [esp + 0x20]; mov edi, dword ptr [esp + 0x1c]; mov byte ptr [esi + 0xb4], 1; push eax; lea ecx, [esi + 0x58]; mov dword ptr [esi + 0x54], edi; call 0x447450; cmp dword ptr [esi + 0x54], -1; mov cl, byte ptr [esp + 0x24]; sete dl; test dl, dl; mov byte ptr [esi + 0xac], cl; mov byte ptr [esi + 0xad], dl; jne 0x77133a",
    ),
    (
        "receiver_ctor_zero_map",
        0x0089D566,
        "mov eax, dword ptr [esp + 0x1c]; test eax, eax; mov dword ptr [esi], 0x1057388; jne 0x89d577; or eax, 0xffffffff; mov dword ptr [esi + 4], eax; mov eax, dword ptr [esp + 0x20]; push eax; lea ecx, [esi + 8]; call 0x447200; mov byte ptr [esp + 0x14], 1; mov cl, byte ptr [esp + 0x24]; mov byte ptr [esi + 0x5c], cl",
    ),
    (
        "receiver_apply_args",
        0x0089D633,
        "movzx eax, byte ptr [esi + 0x5c]; mov edx, dword ptr [esi + 4]; push 1; push eax; mov eax, dword ptr [esp + 0x14]; lea ecx, [esi + 8]; push ecx; push edx; push eax; mov ecx, edi; call 0x6faff0",
    ),
    (
        "apply_prologue",
        0x006FB02B,
        "cmp byte ptr [esp + 0xe8], 0; mov esi, dword ptr [esp + 0xe0]; mov ebp, dword ptr [esp + 0xd8]; mov edi, ecx; mov dword ptr [esp + 0x18], esi; je 0x6fb082; mov edx, dword ptr [edi + 0x60]; mov ecx, dword ptr [edx]; mov eax, ecx",
    ),
    (
        "apply_branch",
        0x006FB08E,
        "cmp dword ptr [esp + 0xdc], -1; mov ecx, dword ptr [eax]; mov ebx, dword ptr [ecx + 4]; jne 0x6fb1d8; mov esi, dword ptr [edi + 0x60]; mov edx, dword ptr [esp + 0x18]",
    ),
    (
        "set_name_custom",
        0x006FB0A1,
        "mov esi, dword ptr [edi + 0x60]; mov edx, dword ptr [esp + 0x18]; push edx; lea ecx, [esi + 4]; mov dword ptr [esi], 0xffffffff; call 0x447450; cmp dword ptr [esi], -1; mov al, byte ptr [esp + 0xe4]; sete cl; cmp byte ptr [esp + 0x1c], 0; mov byte ptr [esi + 0x58], al; mov byte ptr [esi + 0x59], cl; je 0x6fb24c",
    ),
    (
        "set_name_looked_up",
        0x006FB1F3,
        "mov esi, dword ptr [edi + 0x60]; movzx ecx, byte ptr [esi + 0x58]; mov edx, dword ptr [esp + 0xdc]; lea eax, [esp + 0x70]; mov byte ptr [esp + 0x17], cl; push eax; lea ecx, [esi + 4]; mov dword ptr [esi], edx; call 0x447450; cmp dword ptr [esi], -1; movzx ecx, byte ptr [esp + 0x17]; mov eax, dword ptr [esp + 0x1c]; sete dl; mov byte ptr [esi + 0x58], cl; mov byte ptr [esi + 0x59], dl; mov ebx, dword ptr [ebx + 0xe0]; push eax",
    ),
    (
        "default_name_struct",
        0x006BF44B,
        "mov dword ptr [esi], 0xffffffff; call 0x445cf0; xor eax, eax; mov dword ptr [esp + 0x14], eax; mov byte ptr [esi + 0x58], al; mov byte ptr [esi + 0x59], 1; mov dword ptr [esp + 0x14], 0xffffffff; mov eax, esi",
    ),
    (
        "resolved_callback_flag",
        0x006FB2C1,
        "mov cl, byte ptr [0xfd865e]; xor ebx, ebx; cmp byte ptr [esp + 0xd4], bl; mov eax, 1; mov byte ptr [edi + 0x58], cl; mov byte ptr [edi + 0x59], al; je 0x6fb40b; mov dword ptr [esp + 0xc4], 0xffffffff",
    ),
    (
        "chara_init_name",
        0x006E03E9,
        "push 0x5c; call 0x9d1b35; add esp, 4; mov dword ptr [esp + 0x1c], eax; mov dword ptr [esp + 0x10], 0; test eax, eax; je 0x6e040c; mov ecx, eax; call 0x6bf420; jmp 0x6e040e; xor eax, eax; mov dword ptr [esp + 0x10], 0xffffffff",
    ),
    (
        "context_reset",
        0x00769CC0,
        "push ecx; push esi; push edi; mov esi, ecx; mov ecx, dword ptr [esi]; push 0x1266b10; push 1; call 0x8a68a0; mov ecx, eax; call 0x4d3420; mov ecx, dword ptr [esi]; lea eax, [esp + 8]; push eax; xor edi, edi; push 6; mov dword ptr [esp + 0x10], edi; call 0x8a68a0; mov ecx, eax; call 0x4d3390",
    ),
    (
        "context_push_branch",
        0x00769AE6,
        "cmp dword ptr [edi], -1; mov esi, ecx; je 0x769b37; mov ecx, dword ptr [esi]",
    ),
    (
        "context_push_lookup",
        0x00769B02,
        "mov eax, dword ptr [edi]; mov ecx, eax; sub ecx, -1; neg ecx; sbb ecx, ecx; and ecx, eax; lea edx, [esp + 0xc]; push edx; mov dword ptr [esp + 0x10], ecx; mov ecx, dword ptr [esi]; push 6; call 0x8a68a0",
    ),
    (
        "context_push_custom",
        0x00769B37,
        "lea ecx, [edi + 4]; push ecx; mov ecx, dword ptr [esi]; push 1; call 0x8a68a0; mov ecx, eax; call 0x4d3420; mov ecx, dword ptr [esi]; lea edx, [esp + 0xc]; push edx",
    ),
    (
        "variant_int_accessor",
        0x00778D50,
        "sub esp, 0xc; cmp byte ptr [ecx + 8], 0; je 0x778d76; push 0xf91c08; lea ecx, [esp + 4]",
    ),
    (
        "variant_string_accessor",
        0x00779BC0,
        "sub esp, 0xc; cmp byte ptr [ecx + 8], 1; je 0x779be6; push 0xf91c08; lea ecx, [esp + 4]",
    ),
    (
        "num_expr_range",
        0x00CB2DCE,
        "cmp al, 0xed; jae 0xcb315a; cmp al, 0xe8; jb 0xcb2e86; movzx eax, al; sub eax, 0xe8; je 0xcb2e46; sub eax, 1; je 0xcb2dfd",
    ),
    (
        "num_expr_e9",
        0x00CB2DFD,
        "push 1; lea esi, [ebp + 1]; push edi; lea ebp, [ebx + 0x14]; push ebp; push esi; mov ecx, ebx; call 0xcb2d50; add dword ptr [edi], -1; add esi, eax; mov eax, dword ptr [edi]; cmp eax, dword ptr [ebx + 0x24]",
    ),
    (
        "str_expr_range",
        0x00CB2B8C,
        "cmp al, 0xed; jae 0xcb2c8b; cmp al, 0xe8; jb 0xcb2c8b; movzx eax, al; sub eax, 0xea; je 0xcb2c0e; sub eax, 1",
    ),
    (
        "str_expr_eb",
        0x00CB2BAF,
        "push 1; lea ecx, [esp + 0x1c]; push ecx; lea ebx, [edi + 0x14]; push ebx; lea esi, [ebp + 1]; push esi; mov ecx, edi",
    ),
    (
        "str_expr_eb_bound",
        0x00CB2BC7,
        "mov eax, dword ptr [esp + 0x18]; sub eax, 1; cmp eax, dword ptr [edi + 0x24]; mov dword ptr [esp + 0x18], eax; jae 0xcb2be1",
    ),
    (
        "listener_name_state",
        0x00789927,
        "lea ecx, [edi + 4]; push ecx; lea ecx, [esi + 0x34]; mov dword ptr [esi + 0x30], eax; call 0x447450; mov dl, byte ptr [edi + 0x58]; mov byte ptr [esi + 0x88], dl; mov al, byte ptr [edi + 0x59]; mov ecx, ebp; mov byte ptr [esi + 0x89], al",
    ),
    (
        "listener_request_id",
        0x00789A26,
        "mov ecx, dword ptr [edi]; mov eax, ecx; sub eax, -1; neg eax; sbb eax, eax; and eax, ecx; push eax; mov byte ptr [esp + 0xa0], 1; mov ecx, ebx",
    ),
    (
        "record_id_store",
        0x007720D8,
        "mov eax, dword ptr [edi + 0x60]; mov ecx, dword ptr [eax]; mov eax, ecx; sub eax, -1; neg eax; sbb eax, eax; and eax, ecx; push 0x48; mov dword ptr [ebp + 0x108], eax",
    ),
    (
        "request_row",
        0x0076FDFE,
        "push 0xde; push 0x134bbc0; mov ecx, ebx; call 0x76fb10; test al, al; je 0x76fe66",
    ),
)

# object VA, RTTI name, slot -> target
VTABLES: tuple[tuple[int, str, dict[int, int]], ...] = (
    (0x00FDFB2C, ".?AVLuaActorImpl@Client@Script@Lua@Application@@", {62: 0x0076C4D0}),
    (0x00FE02AC, ".?AVNullActorImpl@Client@Script@Lua@Application@@", {62: 0x00771980}),
    (
        0x00FD5CAC,
        ".?AVCharaBase@Control@Client@Script@Lua@Application@@",
        {3: 0x006E03B0},
    ),
    (
        0x01057388,
        ".?AVSetDisplayNameReceiver@Network@Command@Client@Script@Lua@Application@@",
        {1: 0x0089D610},
    ),
)

# target -> direct callers (E8 and E9)
CALLERS: dict[int, list[int]] = {
    0x0089D530: [0x0076C5F4, 0x00771AC3],
    0x0089D610: [0x0076C614],
    0x006FAFF0: [0x007083B0, 0x0089D649],
    0x00769AE0: [0x0076FBC5, 0x0078512E],
    0x00769CC0: [0x0076FB58],
    0x006E03B0: [0x006E13A0, 0x006E13B0, 0x006E346F],
    0x00778D50: [0x0089FEE4, 0x0089FF15, 0x00CB2E25, 0x00CB2E6D],
    0x00779BC0: [0x00CB2C42],
    0x006FB280: [0x00759027],
    0x00575100: [0x004D8A48],
}

DISPATCH_CLASS_TABLE = 0x004D8C24
DISPATCH_JUMP_TABLE = 0x004D8BC0
DISPATCH_BASE_OPCODE = 0x00CC
DISPATCH_ENTRIES = 0xDD
SET_ACTOR_NAME_OPCODE = 0x013D
SET_ACTOR_NAME_CLASS = 11
SET_ACTOR_NAME_TARGET = 0x004D8A36
STRING_CODEPAGE_WORD = 0x00F67298
EMPTY_STRING_OBJECT = 0x01266B10


def disassemble(image: ecr.Image, va: int, count: int) -> list[str]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    code = image.read(va, 16 * count)
    rows = [f"{i.mnemonic} {i.op_str}".strip() for i in md.disasm(code, va)]
    return rows[:count]


def read_dword(image: ecr.Image, va: int) -> int:
    return struct.unpack("<I", image.read(va, 4))[0]


def check_listings(image: ecr.Image) -> list[str]:
    errors = []
    for label, va, text in CHECKS:
        want = text.split("; ")
        got = disassemble(image, va, len(want))
        if got != want:
            errors.append(f"listing {label} at {va:#010x} differs: {got}")
    return errors


def check_tables(image: ecr.Image) -> list[str]:
    errors = []
    for vtable, rtti, slots in VTABLES:
        locator = read_dword(image, vtable - 4)
        descriptor = read_dword(image, locator + 12)
        if image.cstr(descriptor + 8) != rtti:
            errors.append(f"vtable {vtable:#010x} is not {rtti}")
        for slot, want in slots.items():
            if read_dword(image, vtable + 4 * slot) != want:
                errors.append(f"{rtti} slot {slot} is not {want:#010x}")
    classes = image.read(DISPATCH_CLASS_TABLE, DISPATCH_ENTRIES)
    index = SET_ACTOR_NAME_OPCODE - DISPATCH_BASE_OPCODE
    if classes[index] != SET_ACTOR_NAME_CLASS:
        errors.append(f"dispatch class of 0x013D is {classes[index]}")
    if classes.count(SET_ACTOR_NAME_CLASS) != 1:
        errors.append("dispatch class 11 is shared with another opcode")
    target = read_dword(image, DISPATCH_JUMP_TABLE + 4 * SET_ACTOR_NAME_CLASS)
    if target != SET_ACTOR_NAME_TARGET:
        errors.append(f"dispatch class 11 jumps to {target:#010x}")
    return errors


def check_data(image: ecr.Image) -> list[str]:
    errors = []
    if read_dword(image, STRING_CODEPAGE_WORD) != 0xFFFFFFFF:
        errors.append("string codepage word is not 0xffffffff")
    text = read_dword(image, EMPTY_STRING_OBJECT)
    if image.read(text, 1) != b"\0":
        errors.append("global string object 0x01266b10 is not empty")
    return errors


def check_callers(image: ecr.Image) -> list[str]:
    found = ecr.find_callers(image, list(CALLERS))
    return [
        f"callers of {target:#010x} are {[hex(s) for s in found[target]]}"
        for target, want in CALLERS.items()
        if found[target] != want
    ]


def verify(image: ecr.Image) -> list[str]:
    return (
        check_listings(image)
        + check_tables(image)
        + check_data(image)
        + check_callers(image)
    )


# Macro cell reader for the exported `[@name:hex]` cells of xtx/_text_ui.

Expr = tuple


def read_expr(data: bytes, at: int) -> tuple[Expr, int]:
    lead = data[at]
    if 1 <= lead < 0xE0:
        return ("lit", lead - 1), at + 1
    if lead in (0xE8, 0xE9, 0xEA, 0xEB, 0xEC):
        operand, end = read_expr(data, at + 1)
        return (f"{lead:02X}", operand), end
    if lead in range(0xE0, 0xE6):
        left, middle = read_expr(data, at + 1)
        right, end = read_expr(data, middle)
        return (f"cmp{lead:02X}", left, right), end
    if lead == 0xFF:
        length, start = read_expr(data, at + 1)
        end = start + length[1]
        return ("str", read_items(data[start:end])), end
    raise ValueError(f"unknown expression lead {lead:#04x} at {at}")


def read_items(data: bytes) -> list:
    items: list = []
    at = 0
    while at < len(data):
        if data[at] == 2:
            code = data[at + 1]
            size = data[at + 2] - 1
            body = data[at + 3 : at + 3 + size]
            items.append(("tag", code, read_operands(body)))
            at += 3 + size
        else:
            items.append(("byte", data[at]))
            at += 1
    return items


def read_operands(body: bytes) -> list[Expr]:
    operands: list[Expr] = []
    at = 0
    while at < len(body) and not (body[at] == 3 and at == len(body) - 1):
        expr, at = read_expr(body, at)
        operands.append(expr)
    return operands


def find_tag(items: list, code: int):
    for item in items:
        if item[0] != "tag":
            continue
        if item[1] == code:
            return item
        for operand in item[2]:
            if operand[0] == "str":
                found = find_tag(operand[1], code)
                if found:
                    return found
    return None


def contains(expr, wanted: Expr) -> bool:
    if expr == wanted:
        return True
    if isinstance(expr, tuple):
        return any(contains(part, wanted) for part in expr[1:])
    if isinstance(expr, list):
        return any(contains(part, wanted) for part in expr)
    return False


def tag_contains(tag, wanted: Expr) -> bool:
    return any(contains(operand, wanted) for operand in tag[2])


def cell_bytes(cell: str) -> bytes:
    match = re.fullmatch(r"\[@[\w-]+:([0-9a-f]+)\]", cell)
    if not match:
        raise ValueError(f"not a macro cell: {cell[:40]}")
    return bytes.fromhex(match.group(1))


def check_cell(cell: str) -> list[str]:
    """The `08` tag tests integer element 6 and falls back to string element 1."""
    tree = read_items(cell_bytes(cell))
    tag = find_tag(tree, 0x08)
    if tag is None:
        return ["no 08 tag"]
    errors = []
    if tag[2][0] != ("E9", ("lit", 7)):
        errors.append(f"condition is {tag[2][0]}")
    if not tag_contains(("tag", 8, tag[2][2:]), ("EB", ("lit", 2))):
        errors.append("else branch does not read string element 1")
    sheet = find_tag(tree, 0x28) or find_tag(tree, 0x32)
    if sheet is None or not contains(sheet[2], ("E9", ("lit", 7))):
        errors.append("sheet lookup row is not integer element 6")
    return errors


def check_export(text_ui: Path | None, display_name: Path | None) -> list[str]:
    errors = []
    if text_ui is not None:
        errors += check_file(text_ui, TEXT_UI_CSV)
        with text_ui.open(encoding="utf-8", newline="") as handle:
            rows = [r for r in csv.reader(handle) if r and r[0] == ROW_222_KEY]
        if len(rows) != 1 or len(rows[0]) != 6:
            errors.append("row 222 is not one row of five cells")
        else:
            for column, cell in enumerate(rows[0][1:]):
                errors += [f"column {column}: {e}" for e in check_cell(cell)]
    if display_name is not None:
        errors += check_file(display_name, DISPLAY_NAME_CSV)
        with display_name.open(encoding="utf-8", newline="") as handle:
            rows = list(csv.reader(handle))
        if len(rows) - 2 != DISPLAY_NAME_DATA_ROWS:
            errors.append(f"displayName has {len(rows) - 2} data rows")
        by_key = {r[0]: r for r in rows[2:]}
        if by_key.get("0", [None, None, None])[2] != "???":
            errors.append("displayName row 0 column 1 is not '???'")
        if by_key.get("1000001", [None, None, None])[2] != "Bardolf":
            errors.append("displayName row 1000001 column 1 is not 'Bardolf'")
    return errors


def check_file(path: Path, expected: tuple[str, int, str]) -> list[str]:
    _, size, digest = expected
    data = path.read_bytes()
    errors = []
    if len(data) != size:
        errors.append(f"{path.name} is {len(data)} bytes, not {size}")
    if hashlib.sha256(data).hexdigest() != digest:
        errors.append(f"{path.name} SHA-256 differs")
    return errors


def census(samples: dict, display_names: Mapping[int, bytes] | None = None) -> dict:
    """Census of the retail 0x013D samples (xivl-opcodes payload_samples.json)."""
    entries = samples["samples"]["s2c"]["0x013d"]["samples"]
    ids: list[int] = []
    words8: list[int] = []
    result = {
        "samples": len(entries),
        "tail_after_id_all_zero": True,
        "header_word_12_all_zero": True,
        "payload_bytes": set(),
    }
    for entry in entries:
        raw = bytes.fromhex(entry["bytes"])
        result["payload_bytes"].add(len(raw) - 16)
        ids.append(struct.unpack_from("<I", raw, 16)[0])
        words8.append(struct.unpack_from("<I", raw, 8)[0])
        if any(raw[20:]):
            result["tail_after_id_all_zero"] = False
        if struct.unpack_from("<I", raw, 12)[0] != 0:
            result["header_word_12_all_zero"] = False
    sizes = sorted(result["payload_bytes"])
    result["payload_bytes"] = sizes[0] if len(sizes) == 1 else sizes
    others = [i for i in ids if i not in (0, NO_DISPLAY_NAME)]
    result.update(
        id_zero=ids.count(0),
        id_ffffffff=ids.count(NO_DISPLAY_NAME),
        id_other=len(others),
        distinct_ids=len(set(ids)),
        distinct_other=len(set(others)),
        header_word_8_min=min(words8),
        header_word_8_max=max(words8),
    )
    if display_names is not None:
        result["other_ids_without_row"] = sorted(
            {i for i in others if i not in display_names}
        )
    return result


def check_census(samples_path: Path, display_name: Path | None) -> list[str]:
    names = None
    if display_name is not None:
        with display_name.open(encoding="utf-8", newline="") as handle:
            names = {int(r[0]): r[2].encode() for r in list(csv.reader(handle))[2:]}
    got = census(json.loads(samples_path.read_text()), names)
    errors = [
        f"census {key} is {got.get(key)!r}, not {want!r}"
        for key, want in EXPECTED_CENSUS.items()
        if got.get(key) != want
    ]
    if got.get("other_ids_without_row"):
        errors.append(f"ids without a displayName row: {got['other_ids_without_row']}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("orig/ffxivgame.exe"))
    parser.add_argument("--text-ui-csv", type=Path, help="exported xtx__text_ui.csv")
    parser.add_argument("--display-name-csv", type=Path, help="xtx_displayName.csv")
    parser.add_argument("--samples", type=Path, help="payload_samples.json")
    args = parser.parse_args(argv)
    errors: list[str] = []
    ran = []
    if args.exe.is_file():
        data = args.exe.read_bytes()
        ecr.verify_identity(data)
        try:
            errors += verify(ecr.Image(data))
        except ImportError:
            raise SystemExit("Capstone is required: python -m pip install capstone")
        ran.append(
            f"{len(CHECKS)} listings, {len(VTABLES)} vtables, {len(CALLERS)} caller sets"
        )
    else:
        print(f"SKIP executable checks: {args.exe} not found")
    if args.text_ui_csv or args.display_name_csv:
        errors += check_export(args.text_ui_csv, args.display_name_csv)
        ran.append("sheet export")
    if args.samples:
        errors += check_census(args.samples, args.display_name_csv)
        ran.append("capture census")
    for error in errors:
        print(f"FAIL {error}")
    if not ran:
        print("nothing checked: pass --exe, --text-ui-csv or --samples")
        return 0
    print(f"{', '.join(ran)}: {'FAIL' if errors else 'ok'}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
