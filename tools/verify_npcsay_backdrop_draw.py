#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C)  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify the NpcSay backdrop draw facts in ffxivgame.exe.

Check groups over the pinned executable:

1. Instruction listings for the UI image command executor, the vertex colour
   packer, the textured-quad state setup, the render-state wrappers, the Visual
   opacity defaults, the cell skin parser and the element render loop recorded
   in docs/resource/npcsay-backdrop-draw.md.
2. The skin and opacity property literals.
3. The decoded UI shader programs and the blend, compare, cull and address
   tables the state setup indexes.
4. The command and context vtable slots.
5. The direct E8/E9 caller sets of the draw primitives.
6. The render-state and sampler-state ids the 2D path never writes, scanned
   across every call of the two wrappers.

The module also holds reference models of the vertex colour pack, the colour
modulation, the pixel program, the straight alpha blend and the gamma-space
against linear-space difference, so the tests state their expected results
without running the code they check. An optional install check re-reads the
NpcSay form, the style and skin rows and the skin texture alpha. It needs the
client/sqwt directory and the xivl-tools Blowfish module, and it is the only
part that reads retail data.
"""

from __future__ import annotations

import argparse
import importlib.util
import struct
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import NamedTuple, Sequence

import d3d9_shader_decode as shader_decode
import extract_class_registrations as ecr

SIGNATURE = bytes.fromhex("5351455800000000")
MASK_ALPHA_MAX = 166
BODY_SKIN = "LAB_basis_npcLinesBody"
HEADER_SKIN = "LAB_basis_npcLinesHeader"

# D3DRENDERSTATETYPE ids the wrapper 0x004236E0 is called with, and the sampler
# state ids of wrapper 0x004236A0. The wrapper indexes a 0x100 entry cache, so
# the ids are the Direct3D 9 numbering.
RS_ALPHABLENDENABLE = 0x1B
RS_SRCBLEND = 0x13
RS_DESTBLEND = 0x14
RS_BLENDOP = 0xAB
RS_SRGBWRITEENABLE = 0xC2
SAMP_SRGBTEXTURE = 11

D3DBLEND_SRCALPHA = 5
D3DBLEND_INVSRCALPHA = 6
D3DBLEND_ONE = 2
D3DBLENDOP_ADD = 1
D3DCMP_GREATER = 5
D3DCULL_NONE = 1
D3DTADDRESS_CLAMP = 3


def clamp01(value: float) -> float:
    return min(1.0, max(0.0, value))


def pack_channel(value: float) -> int:
    """0x00CB9640: clamp to 0..1, multiply by 255.0, truncate."""
    return int(clamp01(value) * 255.0)


def modulate(state: Sequence[float], command: Sequence[float]) -> tuple[float, ...]:
    """DrawImage::Execute 0x00696740: state colour (P+0x90..0x9C) times command
    colour (+0x30..0x3C), component by component."""
    return tuple(a * b for a, b in zip(state, command))


def ui_pixel(texel: Sequence[float], vertex: Sequence[float]) -> tuple[float, ...]:
    """The UI pixel program: texture sample times interpolated vertex colour."""
    return tuple(t * v for t, v in zip(texel, vertex))


def blend_over(
    destination: Sequence[float], source: Sequence[float]
) -> tuple[float, float, float]:
    """SRCALPHA, INVSRCALPHA, ADD on the colour channels."""
    alpha = source[3]
    return tuple(s * alpha + d * (1.0 - alpha) for s, d in zip(source[:3], destination))  # type: ignore[return-value]


def srgb_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def linear_to_srgb(value: float) -> float:
    if value <= 0.0031308:
        return value * 12.92
    return 1.055 * value ** (1.0 / 2.4) - 0.055


def blend_in_linear_space(
    destination: Sequence[float], source: Sequence[float]
) -> tuple[float, ...]:
    """The same blend on an sRGB surface: destination decoded, blended, encoded."""
    alpha = source[3]
    return tuple(
        linear_to_srgb(srgb_to_linear(s) * alpha + srgb_to_linear(d) * (1.0 - alpha))
        for s, d in zip(source[:3], destination)
    )


def body_alpha(x: int, y: int, fx: Sequence[int], fy: Sequence[int]) -> int:
    """Texel alpha of the top-left body corner cell as the product of its row and
    column profiles over the plateau value. Used only to state the observed
    separability."""
    return round(fx[x] * fy[y] / MASK_ALPHA_MAX)


class CellRow(NamedTuple):
    mode: int
    x0: float
    x1: float
    y0: float
    y1: float


def cell_row(mode: str, x: float, y: float, w: float, h: float) -> CellRow:
    """The five dword record 0x009A81F0 stores per cell: mode, x, x + w, y, y + h."""
    modes = {"Stretch": 0, "Tiling": 1, "ConstantSize": 2}
    return CellRow(modes[mode], x, x + w, y, y + h)


def native_corner_tracks(total: float, first: float, last: float) -> list[float]:
    """CANDIDATE geometry, INFERRED from the texture and not read from the
    executable: outer tracks keep their texel size and the middle takes the rest."""
    scale = total / (first + last) if first + last > total > 0 else 1.0
    first, last = first * scale, last * scale
    return [first, max(0.0, total - first - last), last]


# label, VA, expected listing
CHECKS: tuple[tuple[str, int, str], ...] = (
    (
        "image_execute_texture_lookup",
        0x00696746,
        "mov eax, dword ptr [esi + 4]; push edi; mov edi, dword ptr [esp + 0x1c]; mov ecx, dword ptr [edi + 0x7c]; push eax; call 0x60b270",
    ),
    (
        "image_execute_flag_gate",
        0x0069677D,
        "mov ecx, dword ptr [esi + 0xc]; test ecx, ecx; jne 0x69682f; movss xmm1, dword ptr [esi + 0x30]; movss xmm0, dword ptr [edi + 0x90]",
    ),
    (
        "image_execute_modulate",
        0x00696788,
        "movss xmm1, dword ptr [esi + 0x30]; movss xmm0, dword ptr [edi + 0x90]; cvtps2pd xmm0, xmm0; cvtps2pd xmm1, xmm1; mulsd xmm0, xmm1",
    ),
    ("image_execute_alpha_load", 0x006967F1, "movss xmm0, dword ptr [edi + 0x9c]"),
    ("image_execute_draw_call", 0x0069681A, "call 0xca5760"),
    (
        "image_execute_alt_gate",
        0x0069682F,
        "and ecx, 0x80; cmp cl, 0x80; jne 0x6968d8; movss xmm1, dword ptr [esi + 0x30]",
    ),
    ("image_execute_alt_call", 0x006968D0, "call 0xca5970"),
    ("image_execute_return_size", 0x00696823, "mov eax, 0x40"),
    (
        "pack_alpha_clamp",
        0x00CB9640,
        "sub esp, 0xc; movss xmm0, dword ptr [0xf54f70]; comiss xmm0, dword ptr [ecx + 0xc]; xorps xmm1, xmm1; lea eax, [ecx + 0xc]; movss dword ptr [esp + 4], xmm0",
    ),
    (
        "pack_scale_truncate",
        0x00CB967B,
        "cvtps2pd xmm3, xmm2; movsd xmm2, qword ptr [0xf598b0]; mulsd xmm3, xmm2; cvttsd2si eax, xmm3",
    ),
    (
        "ui_path_select",
        0x00CA5760,
        "cmp dword ptr [0x13764a0], 0; jne 0xca57a3; cmp dword ptr [0x1376490], 1; je 0xca579e; call 0xca3a70; push 1",
    ),
    (
        "ui_state_blend_enable",
        0x00CA38E8,
        "push 1; mov byte ptr [0x1328ef0], 1; call 0x41c1f0; push ebx; mov byte ptr [0x1328f10], bl; call 0x41c340",
    ),
    (
        "ui_state_blend_factors",
        0x00CA38F6,
        "push ebx; mov byte ptr [0x1328f10], bl; call 0x41c340; mov eax, 7; push eax; mov ecx, esi; push ecx; mov edx, eax; push edx; push esi; mov dword ptr [0x1328ef8], esi; mov dword ptr [0x1328efc], eax",
    ),
    (
        "ui_state_blend_op",
        0x00CA392A,
        "xor eax, eax; push eax; push ebx; mov dword ptr [0x1328ef4], ebx; mov dword ptr [0x1328f00], ebx; call 0x41d010",
    ),
    (
        "flush_alpha_test",
        0x00CA34AE,
        "push 1; mov byte ptr [0x1328ee8], 1; call 0x41c1d0; fldz; xorps xmm0, xmm0; fstp dword ptr [esp]; mov eax, 4; push eax; mov dword ptr [0x1328ee0], eax; movss dword ptr [0x1328ee4], xmm0",
    ),
    (
        "flush_cull",
        0x00CA34DC,
        "xor eax, eax; xor ebx, ebx; push eax; mov dword ptr [0x1328f68], ebx; call 0x41d0e0; add esp, 8",
    ),
    (
        "blend_enable_wrapper",
        0x0041C1F0,
        "movzx eax, byte ptr [esp + 4]; mov ecx, dword ptr [0x132987c]; push eax; push 0x1b; call 0x4236e0",
    ),
    (
        "blend_factor_wrapper",
        0x0041D050,
        "mov eax, dword ptr [esp + 4]; mov ecx, dword ptr [eax*4 + 0xf596e0]; push ecx; mov ecx, dword ptr [0x132987c]; push 0x13; call 0x4236e0; mov edx, dword ptr [esp + 8]; mov eax, dword ptr [edx*4 + 0xf596e0]",
    ),
    (
        "blend_op_wrapper",
        0x0041D010,
        "mov eax, dword ptr [esp + 4]; mov ecx, dword ptr [eax*4 + 0xf5971c]; push ecx; mov ecx, dword ptr [0x132987c]; push 0xab",
    ),
    (
        "alpha_test_wrapper",
        0x0041C1D0,
        "movzx eax, byte ptr [esp + 4]; mov ecx, dword ptr [0x132987c]; push eax; push 0xf; call 0x4236e0",
    ),
    (
        "alpha_func_wrapper",
        0x0041CF50,
        "sub esp, 8; mov eax, dword ptr [esp + 0xc]; mov ecx, dword ptr [eax*4 + 0xf596c0]; push ecx; mov ecx, dword ptr [0x132987c]; push 0x19",
    ),
    (
        "alpha_ref_wrapper",
        0x0041CF6C,
        "fld dword ptr [esp + 0x10]; fmul qword ptr [0xf598b0]; fnstcw word ptr [esp + 0xc]; movzx eax, word ptr [esp + 0xc]; fadd qword ptr [0xf59898]",
    ),
    (
        "state_wrapper_cache",
        0x004236E0,
        "push ebx; mov ebx, dword ptr [esp + 8]; push esi; push edi; mov edi, dword ptr [esp + 0x14]; mov esi, ecx; mov ecx, dword ptr [esi + 4]; push edi; push ebx; call 0x424010; test al, al; jne 0x423706",
    ),
    (
        "state_cache_bound",
        0x00424010,
        "push esi; push edi; mov edi, dword ptr [esp + 0xc]; cmp edi, 0x100; mov esi, ecx; jb 0x42405c; test byte ptr [0x1323910], 1; jne 0x42403a",
    ),
    (
        "sampler_address_wrapper",
        0x0041CC60,
        "mov eax, dword ptr [esp + 8]; mov ecx, dword ptr [eax*4 + 0xf59750]; push esi; mov esi, dword ptr [esp + 8]; push ecx; mov ecx, dword ptr [0x132987c]; push 1",
    ),
    (
        "visual_opacity_default",
        0x00940647,
        "fld1; push edi; lea ecx, [esi + 0x48]; push ecx; fstp dword ptr [esp]; push 0x135988c",
    ),
    (
        "visual_opacity_red_default",
        0x00940666,
        "fld1; push edi; lea ecx, [esi + 0x50]; push ecx; fstp dword ptr [esp]",
    ),
    (
        "cell_parse_stretch",
        0x009A84A4,
        "push 0x107c4f4; lea ecx, [esp + 0xdc]; call 0x445d60; test al, al; je 0x9a84c2; mov dword ptr [esi - 8], 0; jmp 0x9a84fc",
    ),
    (
        "cell_parse_tiling",
        0x009A84C2,
        "push 0x107c4fc; lea ecx, [esp + 0xdc]; call 0x445d60; test al, al; je 0x9a84e0; mov dword ptr [esi - 8], 1",
    ),
    (
        "cell_parse_constant",
        0x009A84E0,
        "push 0x107c504; lea ecx, [esp + 0xdc]; call 0x445d60; test al, al; je 0x9a84fc; mov dword ptr [esi - 8], 2; lea ecx, [esp + 0xd8]",
    ),
    (
        "cell_stride",
        0x009A84FC,
        "lea ecx, [esp + 0xd8]; add esi, 0x14; mov byte ptr [esp + 0x138], 2",
    ),
    (
        "image_render_ctx_call",
        0x009B9C37,
        "mov edx, dword ptr [eax]; lea esi, [esp + 0x48]; add ecx, 0x304; push esi; push ecx; mov ecx, eax; mov eax, dword ptr [edx + 0x20]; call eax; mov ecx, dword ptr [esp + 0x5c]",
    ),
    (
        "element_context_visual_opacity",
        0x0093F918,
        "fld dword ptr [esi + 0x60]; mov ecx, dword ptr [esi + 0xa0]; mov edx, dword ptr [ecx]; mov eax, dword ptr [edx + 0x44]; sub esp, 0x10; fstp dword ptr [esp + 0xc]; fld dword ptr [esi + 0x58]; fstp dword ptr [esp + 8]; fld dword ptr [esi + 0x50]; fstp dword ptr [esp + 4]; fld dword ptr [esi + 0x48]; fstp dword ptr [esp]; call eax",
    ),
    (
        "element_context_dirty_gate",
        0x0093F900,
        "test byte ptr [esi + 0x4c], 4; jne 0x93f918; test byte ptr [esi + 0x54], 4; jne 0x93f918; test byte ptr [esi + 0x5c], 4; jne 0x93f918; test byte ptr [esi + 0x64], 4; je 0x93f943",
    ),
    (
        "context_slot17_store",
        0x0055A05B,
        "or byte ptr [eax + 0x58], 8; movss dword ptr [eax + 0x70], xmm0",
    ),
    (
        "context_slot18_store",
        0x0055A09B,
        "or byte ptr [eax + 0x58], 0x10; movss dword ptr [eax + 0x90], xmm0",
    ),
    (
        "tool_flush_colour_blocks",
        0x0054EC49,
        "push 0x10; lea edx, [esi + 0x70]; push edx; push 0; call 0x54b0d0",
    ),
    (
        "tool_flush_brush_block",
        0x0054EC76,
        "push 0x10; lea eax, [esi + 0x90]; push eax; push 1; call 0x54b0d0",
    ),
    (
        "scene_packet_index_offset",
        0x0054B11F,
        "add ecx, 0x15; push ecx; push eax; lea ecx, [esp + 0x24]; call 0x4ec080",
    ),
    (
        "control_render_background",
        0x00936DB0,
        "mov edx, dword ptr [ecx + 0x310]; fld dword ptr [edx]; fcomp qword ptr [0xfa2ee0]; fnstsw ax; test ah, 0x44",
    ),
    (
        "render_loop_render_call",
        0x0093B007,
        "mov edx, dword ptr [esi]; mov eax, dword ptr [edx + 0x88]; push edi; mov ecx, esi; call eax; mov edx, dword ptr [edi]; mov eax, dword ptr [edx + 0x10]",
    ),
    (
        "render_loop_visible_gate",
        0x0093AFE2,
        "mov edx, dword ptr [esi]; mov eax, dword ptr [edx + 0x80]; mov ecx, esi; call eax; test al, al; jne 0x93affa; or byte ptr [esi + 0x190], bl",
    ),
)

LITERALS: tuple[tuple[int, str], ...] = (
    (0x0107C4C4, "CellSkin.Cells"),
    (0x0107C4E8, "DrawMode"),
    (0x0107C4F4, "Stretch"),
    (0x0107C4FC, "Tiling"),
    (0x0107C504, "ConstantSize"),
    (0x0107C514, "HitMargin"),
    (0x0107C520, "SkinMargin"),
    (0x0107C52C, "SkinBorderThickness"),
    (0x0106C4DC, "VisualOpacity"),
    (0x0106C4EC, "VisualOpacityRed"),
    (0x0106C500, "VisualOpacityGreen"),
    (0x0106C514, "VisualOpacityBlue"),
    (0x0106B988, "Background"),
)

# table VA, expected dwords
TABLES: tuple[tuple[str, int, tuple[int, ...]], ...] = (
    ("blend factor", 0x00F596E0, (1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 14, 15, 0, 0, 1)),
    ("blend op", 0x00F5971C, (1, 4, 5, 2, 3, 1, 2, 3)),
    ("compare", 0x00F596C0, (1, 2, 3, 4, 5, 6, 7, 8)),
    ("cull", 0x00F597C8, (1, 2, 3, 189)),
    ("address", 0x00F59750, (1, 2, 3, 4, 3, 1, 2, 1, 2, 0, 2, 1)),
)

# vertex shader table, pixel shader table, byte size table: (programs, sizes)
VERTEX_PROGRAMS = (0x0130BB00, 0x01109334)
PIXEL_PROGRAMS = (0x0130BB80, 0x01109358)
UI_PROGRAM_INDEX = 1
UI_VERTEX_LISTING = (
    "vs_2_0",
    "dcl_position0 v0",
    "dcl_color0 v1",
    "dcl_texcoord0 v2",
    "mul r0, v0.yyyy, c1",
    "mad r0, c0, v0.xxxx, r0",
    "mad r0, c2, v0.zzzz, r0",
    "mad r0, c3, v0.wwww, r0",
    "mul r1, r0.yyyy, c5",
    "mad r1, c4, r0.xxxx, r1",
    "mad r1, c6, r0.zzzz, r1",
    "mad oPos, c7, r0.wwww, r1",
    "mov oD0, v1",
    "mov o0.xy, v2",
)
UI_PIXEL_LISTING = (
    "ps_2_0",
    "dcl_position0 v0",
    "dcl_position0 t0.xy",
    "dcl_2d s0",
    "texld_pp r0, t0, s0",
    "mul_pp r0, r0, v0",
    "mov_pp oC0, r0",
)

# vtable VA, slot -> target
VTABLE_SLOTS: tuple[tuple[str, int, dict[int, int]], ...] = (
    (
        "DrawingToolContext",
        0x00FA28E4,
        {7: 0x00559F30, 8: 0x00559F60, 9: 0x00559F70, 10: 0x00559F80},
    ),
    ("DrawImage", 0x00FA2A00, {1: 0x00696740, 2: 0x006968F0}),
    ("DrawImageWithMask", 0x00FA2A10, {1: 0x00696950}),
    ("PushColor", 0x00FA2A20, {1: 0x00696DC0, 2: 0x00696DD0}),
    ("PopColor", 0x00FA2A30, {1: 0x00696D90, 2: 0x00696DA0}),
    ("Label", 0x010738B4, {34: 0x00936DB0}),
    ("Window", 0x0106A0BC, {34: 0x00936DB0}),
    ("Image", 0x0108040C, {34: 0x009B9A60}),
    ("Border", 0x0107E01C, {34: 0x00776340}),
)

# target -> direct callers (E8 and E9)
CALLERS: dict[int, list[int]] = {
    0x00554190: [0x0054DEDB],
    0x00CA5760: [
        0x0069681A,
        0x00696D13,
        0x008DF7F6,
        0x008DFA72,
        0x008DFBD1,
        0x008DFD89,
        0x008E031C,
        0x008E1C57,
        0x008E205B,
    ],
    0x00CA5970: [0x006968D0, 0x008E00DA, 0x008E02A0],
    0x0041D050: [
        0x004391DC,
        0x00439D5C,
        0x00A48138,
        0x00A52FEC,
        0x00A59BF3,
        0x00BA02FC,
        0x00BA0332,
        0x00BA0BE2,
        0x00C5F65E,
        0x00C7CB25,
        0x00C7D425,
        0x00C7D87B,
        0x00C7DFD8,
        0x00C7FC97,
        0x00C89F61,
        0x00C92F23,
        0x00C934C0,
        0x00CA3925,
        0x00CA84B9,
    ],
}

RENDER_STATE_WRAPPER = 0x004236E0
SAMPLER_STATE_WRAPPER = 0x004236A0


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


def check_literals(image: ecr.Image) -> list[str]:
    return [
        f"literal at {va:#010x} is not {text!r}"
        for va, text in LITERALS
        if image.cstr(va) != text
    ]


def check_tables(image: ecr.Image) -> list[str]:
    errors = []
    for name, va, want in TABLES:
        got = tuple(read_dword(image, va + 4 * i) for i in range(len(want)))
        if got != want:
            errors.append(f"{name} table at {va:#010x} is {got}")
    return errors


def program_text(
    image: ecr.Image, tables: tuple[int, int], index: int
) -> tuple[str, ...]:
    pointer = read_dword(image, tables[0] + 4 * index)
    size = read_dword(image, tables[1] + 4 * index)
    return shader_decode.decode(image.read(pointer, size), 0).lines


def check_programs(image: ecr.Image) -> list[str]:
    errors = []
    if program_text(image, VERTEX_PROGRAMS, UI_PROGRAM_INDEX) != UI_VERTEX_LISTING:
        errors.append("UI vertex program 1 differs")
    if program_text(image, PIXEL_PROGRAMS, UI_PROGRAM_INDEX) != UI_PIXEL_LISTING:
        errors.append("UI pixel program 1 differs")
    return errors


def check_vtables(image: ecr.Image) -> list[str]:
    errors = []
    for name, base, slots in VTABLE_SLOTS:
        for slot, want in slots.items():
            got = read_dword(image, base + 4 * slot)
            if got != want:
                errors.append(f"{name} slot {slot} is {got:#010x}")
    return errors


def check_callers(image: ecr.Image) -> list[str]:
    found = ecr.find_callers(image, list(CALLERS))
    return [
        f"callers of {target:#010x} are {[hex(s) for s in found[target]]}"
        for target, want in CALLERS.items()
        if found[target] != want
    ]


def pushed_ids(image: ecr.Image, wrapper: int, index: int) -> set[int]:
    """Immediate first arguments of every direct call of a state wrapper. The
    wrapper takes (id, value) and the id is the last push before the call for
    the render-state wrapper, and the second of three for the sampler wrapper."""
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    ids: set[int] = set()
    for site in ecr.find_callers(image, [wrapper])[wrapper]:
        rows = list(md.disasm(image.read(site - 30, 30), site - 30))
        pushes = [r for r in rows if r.mnemonic == "push"]
        if len(pushes) >= index:
            operand = pushes[-index].op_str
            try:
                ids.add(int(operand, 0))
            except ValueError:
                continue
    return ids


def check_state_ids(image: ecr.Image) -> list[str]:
    errors = []
    render = pushed_ids(image, RENDER_STATE_WRAPPER, 1)
    for want in (RS_ALPHABLENDENABLE, RS_SRCBLEND, RS_DESTBLEND, RS_BLENDOP):
        if want not in render:
            errors.append(f"render state {want:#x} is never set with an immediate")
    if RS_SRGBWRITEENABLE in render:
        errors.append("render state 0xC2 is set")
    sampler = pushed_ids(image, SAMPLER_STATE_WRAPPER, 2)
    if SAMP_SRGBTEXTURE in sampler:
        errors.append("sampler state 11 is set")
    return errors


def verify(image: ecr.Image) -> list[str]:
    return (
        check_listings(image)
        + check_literals(image)
        + check_tables(image)
        + check_programs(image)
        + check_vtables(image)
        + check_callers(image)
        + check_state_ids(image)
    )


# ---- optional install check ----


def load_blowfish(path: Path):
    spec = importlib.util.spec_from_file_location("sqex_blowfish", path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_sqex(path: Path, blowfish_module) -> str:
    data = path.read_bytes()
    if data[:8] != SIGNATURE:
        raise SystemExit(f"{path.name} has no SQEX signature")
    body = blowfish_module.Blowfish(path.name.encode("utf-8")).decrypt(data[8:])
    return body.decode("utf-8").lstrip("\ufeff")


def local(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


FORBIDDEN_FORM_ATTRIBUTES = (
    "Opacity",
    "VisualOpacity",
    "VisualOpacityRed",
    "VisualOpacityGreen",
    "VisualOpacityBlue",
    "Alpha",
    "Background",
    "Foreground",
)


def form_shape(text: str) -> dict:
    """The control shape of the NpcSay form: names, kinds, styles and sizes."""
    root = ET.fromstring(text)
    rows = []
    forbidden = []
    for element in root.iter():
        kind = local(element.tag)
        attrs = element.attrib
        for name in attrs:
            if local(name) in FORBIDDEN_FORM_ATTRIBUTES:
                forbidden.append((kind, name))
        if kind in ("Window", "Label", "LogControl", "TextBlock"):
            rows.append(
                (
                    kind,
                    attrs.get("Name"),
                    attrs.get("SqwtStyle"),
                    attrs.get("Width"),
                    attrs.get("Height"),
                    attrs.get("Grid.Row"),
                )
            )
    return {"rows": rows, "forbidden": forbidden}


EXPECTED_FORM_ROWS = [
    ("Window", "Window_NpcSayWidget", "WIN_null", None, None, None),
    ("Label", "Label_NpcName", "LAB_basis_npcLinesHeader", None, None, None),
    ("TextBlock", "TextBlock_NpcName", "TBL_subheading", None, None, None),
    ("Label", None, "LAB_basis_npcLinesBody", "620", "106", "1"),
    ("LogControl", "LogControl_NpcSay", "LogControl_npcLines", None, None, None),
]


def style_rows(text: str, widget: str, style: str) -> dict[str, dict[str, str]]:
    """Part name -> condition -> skin for one style, plus its other attributes."""
    root = ET.fromstring(text)
    out: dict[str, dict[str, str]] = {}
    for holder in root:
        if holder.attrib.get("Type") != widget:
            continue
        for node in holder:
            if node.attrib.get("Name") != style:
                continue
            for part in node:
                rows: dict[str, str] = {}
                for prop in part:
                    skin = prop.attrib.get("Skin")
                    if skin is not None:
                        rows[prop.attrib.get("Condition", "")] = skin
                    for key in prop.attrib:
                        if key not in ("Condition", "Skin"):
                            rows[f"attr:{key}"] = prop.attrib[key]
                if part.attrib.get("Reference"):
                    rows["reference"] = (
                        f"{part.attrib['Reference']}:{part.attrib.get('Style')}"
                    )
                out[part.attrib.get("Name", "")] = rows
    return out


def cell_skin(text: str, name: str) -> dict:
    root = ET.fromstring(text)
    group = next(c for c in root if local(c.tag) == "CellSkins")
    for skin in group:
        if skin.attrib.get("Name") == name:
            rows = []
            for cell in skin:
                x, y, w, h = (float(v) for v in cell.attrib["Rectangle"].split(","))
                rows.append(cell_row(cell.attrib["DrawMode"], x, y, w, h))
            return {
                "source": group.attrib["Source"],
                "cells": skin.attrib["Cells"],
                "hit": skin.attrib.get("HitMargin"),
                "border": skin.attrib.get("SkinBorderThickness"),
                "rows": rows,
            }
    raise SystemExit(f"no cell skin {name}")


EXPECTED_BODY_SKIN = {
    "source": "common/default.dds",
    "cells": "3,3",
    "hit": "28,10,28,10",
    "border": "6",
    "rows": [
        cell_row("Stretch", *rect)
        for rect in (
            (0, 260, 64, 32),
            (64, 260, 4, 32),
            (68, 260, 64, 32),
            (0, 292, 64, 4),
            (64, 292, 4, 4),
            (68, 292, 64, 4),
            (0, 296, 64, 32),
            (64, 296, 4, 32),
            (68, 296, 64, 32),
        )
    ],
}


class Texture(NamedTuple):
    width: int
    height: int
    pixels: bytes


def read_gtex_a8r8g8b8(data: bytes) -> Texture:
    """Format index 4 of the GTEX header, big-endian fields, table base at 0x10
    and source base at 0x14, one surface."""
    if data[:4] != b"GTEX" or data[6] != 4:
        raise SystemExit("not a GTEX format 4 texture")
    width, height = struct.unpack_from(">HH", data, 0x0A)
    table, base = struct.unpack_from(">II", data, 0x10)
    offset, size = struct.unpack_from(">II", data, table)
    if size != width * height * 4:
        raise SystemExit("surface size differs from width * height * 4")
    start = base + offset
    return Texture(width, height, data[start : start + size])


def texel(texture: Texture, x: int, y: int) -> tuple[int, int, int, int]:
    """(r, g, b, a) of a B, G, R, A byte order surface."""
    offset = (y * texture.width + x) * 4
    b, g, r, a = texture.pixels[offset : offset + 4]
    return r, g, b, a


def rect_stats(texture: Texture, rect: tuple[int, int, int, int]) -> dict:
    x0, y0, w, h = rect
    colours = set()
    alphas = []
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            r, g, b, a = texel(texture, x, y)
            colours.add((r, g, b))
            alphas.append(a)
    return {"colours": sorted(colours), "min": min(alphas), "max": max(alphas)}


def row_profile(texture: Texture, rect: tuple[int, int, int, int]) -> list[int]:
    """Alpha along the bottom texel row of the top-left corner cell."""
    x0, y0, w, h = rect
    return [texel(texture, x0 + i, y0 + h - 1)[3] for i in range(w)]


def column_profile(texture: Texture, rect: tuple[int, int, int, int]) -> list[int]:
    """Alpha down the right texel column of the top-left corner cell."""
    x0, y0, w, h = rect
    return [texel(texture, x0 + w - 1, y0 + j)[3] for j in range(h)]


def half_point(profile: Sequence[int]) -> int:
    return next(i for i, v in enumerate(profile) if v >= MASK_ALPHA_MAX / 2)


def check_install(sqwt: Path, blowfish_path: Path) -> list[str]:
    errors = []
    module = load_blowfish(blowfish_path)
    form = form_shape(decode_sqex(sqwt / "widget" / "NpcSayWidget.form", module))
    if form["rows"] != EXPECTED_FORM_ROWS:
        errors.append(f"form rows are {form['rows']}")
    if form["forbidden"]:
        errors.append(f"form carries opacity or colour attributes {form['forbidden']}")
    style = decode_sqex(sqwt / "common" / "default.style", module)
    body = style_rows(style, "Label", BODY_SKIN)
    want_frame = {
        "SkinNormal": BODY_SKIN,
        "SkinHover": BODY_SKIN,
        "SkinActive": BODY_SKIN,
        "SkinDisabled": BODY_SKIN,
        "SkinFocused": BODY_SKIN,
        "SkinAnchored": "NULL",
        "SkinReserved": "NULL",
        "SkinDroppable": "NULL",
    }
    if body != {"Frame": want_frame}:
        errors.append(f"body style rows are {body}")
    window = style_rows(style, "Window", "WIN_null")
    if set(window["Frame"].values()) - {
        "NULL",
        "#fff3f3f3",
        "#ff262626",
        "#80f3f3f3",
        "#801a1a1a",
    }:
        errors.append("WIN_null frame names a skin")
    log = style_rows(style, "LogControl", "LogControl_npcLines")
    if log.get("Background", {}).get("reference") != "Window:WIN_null":
        errors.append("LogControl_npcLines background is not WIN_null")
    skin = decode_sqex(sqwt / "common" / "default.skin", module)
    got = cell_skin(skin, BODY_SKIN)
    if got != EXPECTED_BODY_SKIN:
        errors.append(f"body cell skin is {got}")
    texture = read_gtex_a8r8g8b8((sqwt / "common" / "default.win32.gtex").read_bytes())
    if (texture.width, texture.height) != (1024, 1024):
        errors.append("skin texture is not 1024 by 1024")
    for row in EXPECTED_BODY_SKIN["rows"]:
        rect = (int(row.x0), int(row.y0), int(row.x1 - row.x0), int(row.y1 - row.y0))
        stats = rect_stats(texture, rect)
        if stats["colours"] != [(0, 0, 0)] or stats["max"] > MASK_ALPHA_MAX:
            errors.append(f"body cell {rect} is not black with alpha at most 166")
    centre = rect_stats(texture, (64, 292, 4, 4))
    if (centre["min"], centre["max"]) != (MASK_ALPHA_MAX, MASK_ALPHA_MAX):
        errors.append("body centre cell alpha is not 166")
    fx = row_profile(texture, (0, 260, 64, 32))
    fy = column_profile(texture, (0, 260, 64, 32))
    if (half_point(fx), half_point(fy)) != (32, 10):
        errors.append(f"half points are {(half_point(fx), half_point(fy))}")
    for x in range(0, 64, 7):
        for j in range(0, 32, 5):
            predicted = body_alpha(x, j, fx, fy)
            if abs(predicted - texel(texture, x, 260 + j)[3]) > 3:
                errors.append(f"alpha at {x},{j} is not separable")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("orig/ffxivgame.exe"))
    parser.add_argument("--install", type=Path, help="client/sqwt directory")
    parser.add_argument("--blowfish", type=Path, help="xivl-tools tools/blowfish.py")
    args = parser.parse_args(argv)
    data = args.exe.read_bytes()
    ecr.verify_identity(data)
    try:
        errors = verify(ecr.Image(data))
    except ImportError:
        raise SystemExit("Capstone is required: python -m pip install capstone")
    install = "not run"
    if args.install and args.blowfish:
        extra = check_install(args.install, args.blowfish)
        errors += extra
        install = "FAIL" if extra else "ok"
    for error in errors:
        print(f"FAIL {error}")
    print(
        f"{len(CHECKS)} listings, {len(LITERALS)} literals, {len(TABLES)} tables, "
        f"2 programs, {len(VTABLE_SLOTS)} vtables, {len(CALLERS)} caller sets, "
        f"state ids, install {install}: {'FAIL' if errors else 'ok'}"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
