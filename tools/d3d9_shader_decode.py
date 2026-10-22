#!/usr/bin/env python3
"""Decode Direct3D 9 shader model 2 and 3 token streams into canonical text.

The canonical text is the contract other tools hash: one line per instruction
in the form `op[_modifier] dst[.mask], src, ...`, `def` and `dcl` lines
included, comment blocks excluded. The constant table (CTAB) comes from the
comment block tagged `CTAB`. Input is a byte buffer and an offset to the
version token. Nothing here reads or writes retail files on its own.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

OPCODES = {
    0: "nop",
    1: "mov",
    2: "add",
    3: "sub",
    4: "mad",
    5: "mul",
    6: "rcp",
    7: "rsq",
    8: "dp3",
    9: "dp4",
    10: "min",
    11: "max",
    12: "slt",
    13: "sge",
    14: "exp",
    15: "log",
    16: "lit",
    17: "dst",
    18: "lrp",
    19: "frc",
    20: "m4x4",
    21: "m4x3",
    22: "m3x4",
    23: "m3x3",
    24: "m3x2",
    25: "call",
    26: "callnz",
    27: "loop",
    28: "ret",
    29: "endloop",
    30: "label",
    31: "dcl",
    32: "pow",
    33: "crs",
    34: "sgn",
    35: "abs",
    36: "nrm",
    37: "sincos",
    38: "rep",
    39: "endrep",
    40: "if",
    41: "ifc",
    42: "else",
    43: "endif",
    44: "break",
    45: "breakc",
    46: "mova",
    47: "defb",
    48: "defi",
    64: "texcoord",
    65: "texkill",
    66: "texld",
    81: "def",
    88: "cmp",
    90: "dp2add",
    91: "dsx",
    92: "dsy",
    93: "texldd",
    94: "setp",
    95: "texldl",
    96: "breakp",
}
COMPARES = {1: "gt", 2: "eq", 3: "ge", 4: "lt", 5: "ne", 6: "le"}
REGISTER_PREFIX = {
    0: "r",
    1: "v",
    2: "c",
    4: "rast",
    5: "oD",
    6: "o",
    7: "i",
    8: "oC",
    9: "oDepth",
    10: "s",
    11: "c",
    12: "c",
    13: "c",
    14: "b",
    15: "aL",
    18: "l",
    19: "p",
}
SOURCE_MODIFIERS = {
    0: "{}",
    1: "-{}",
    2: "{}_bias",
    3: "-{}_bias",
    4: "{}_bx2",
    5: "-{}_bx2",
    6: "1-{}",
    7: "{}_x2",
    8: "-{}_x2",
    9: "{}_dz",
    10: "{}_dw",
    11: "abs({})",
    12: "-abs({})",
    13: "!{}",
}
USAGES = (
    "position",
    "blendweight",
    "blendindices",
    "normal",
    "psize",
    "texcoord",
    "tangent",
    "binormal",
    "tessfactor",
    "positiont",
    "color",
    "fog",
    "depth",
    "sample",
)
TEXTURE_TYPES = {2: "2d", 3: "cube", 4: "volume"}
REGISTER_SETS = {0: "b", 1: "i", 2: "c", 3: "s"}
NO_DESTINATION = {0, 25, 26, 27, 28, 29, 30, 38, 39, 40, 41, 42, 43, 44, 45, 65, 96}
SWIZZLE = "xyzw"
END_TOKEN = 0xFFFF
COMMENT_TOKEN = 0xFFFE


@dataclass(frozen=True)
class Constant:
    name: str
    register_set: str
    index: int
    count: int
    default: tuple[float, ...] | None


@dataclass(frozen=True)
class Shader:
    version: str
    lines: tuple[str, ...]
    constants: tuple[Constant, ...]
    length: int

    def text(self) -> str:
        return "\n".join(self.lines)

    def sha256(self) -> str:
        return hashlib.sha256(self.text().encode("ascii")).hexdigest()

    def table_text(self) -> str:
        """Constant table as `name set index count` rows sorted by register."""
        rows = sorted(self.constants, key=lambda c: (c.register_set, c.index, c.name))
        return "\n".join(f"{c.name} {c.register_set}{c.index} x{c.count}" for c in rows)

    def table_sha256(self) -> str:
        return hashlib.sha256(self.table_text().encode("ascii")).hexdigest()

    def constant(self, name: str) -> Constant:
        return next(item for item in self.constants if item.name == name)


def register_type(token: int) -> int:
    return ((token >> 28) & 7) | ((token >> 8) & 0x18)


def register_name(token: int, vertex: bool) -> str:
    kind = register_type(token)
    number = token & 0x7FF
    if kind == 3:
        return f"a{number}" if vertex else f"t{number}"
    if kind == 4:
        return ("oPos", "oFog", "oPts")[number]
    if kind == 15:
        return "aL"
    if kind == 17:
        return "vPos" if number == 0 else "vFace"
    return f"{REGISTER_PREFIX.get(kind, '?' + str(kind))}{number}"


def swizzle_text(token: int) -> str:
    packed = (token >> 16) & 0xFF
    text = "".join(SWIZZLE[(packed >> (2 * i)) & 3] for i in range(4))
    return "" if text == SWIZZLE else "." + text


def write_mask_text(token: int) -> str:
    mask = (token >> 16) & 0xF
    if mask == 0xF:
        return ""
    return "." + "".join(c for i, c in enumerate(SWIZZLE) if mask >> i & 1)


def float_of(word: int) -> float:
    return struct.unpack("<f", struct.pack("<I", word))[0]


def source_text(operand: tuple[int, int | None], vertex: bool) -> str:
    token, relative = operand
    text = register_name(token, vertex)
    if relative is not None:
        index = (
            "aL" if register_type(relative) == 15 else register_name(relative, vertex)
        )
        if index != "aL":
            index += swizzle_text(relative)[:2]
        text += f"[{index}]"
    return SOURCE_MODIFIERS.get((token >> 24) & 0xF, "{}").format(
        text + swizzle_text(token)
    )


def split_operands(tokens: list[int]) -> list[tuple[int, int | None]]:
    operands: list[tuple[int, int | None]] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        relative = None
        if (token >> 13) & 1 and register_type(token) != 3:
            relative = tokens[index + 1]
            index += 1
        operands.append((token, relative))
        index += 1
    return operands


def declaration_text(tokens: list[int], vertex: bool) -> str:
    usage, target = tokens[0], tokens[1]
    if register_type(target) == 10:
        kind = TEXTURE_TYPES.get((usage >> 27) & 15, "?")
        return f"dcl_{kind} {register_name(target, vertex)}"
    name = USAGES[usage & 0x1F]
    return (
        f"dcl_{name}{(usage >> 16) & 0xF} "
        f"{register_name(target, vertex)}{write_mask_text(target)}"
    )


def instruction_text(op: int, head: int, tokens: list[int], vertex: bool) -> str:
    name = OPCODES.get(op, f"op{op}")
    predicate = "(p0) " if (head >> 28) & 1 else ""
    if op == 31:
        return declaration_text(tokens, vertex)
    if op == 81:
        values = ", ".join(f"{float_of(w):.7g}" for w in tokens[1:5])
        return f"def {register_name(tokens[0], vertex)}, {values}"
    if op == 48:
        values = ", ".join(
            str(struct.unpack("<i", struct.pack("<I", w))[0]) for w in tokens[1:5]
        )
        return f"defi {register_name(tokens[0], vertex)}, {values}"
    if op == 47:
        return f"defb {register_name(tokens[0], vertex)}, {tokens[1]}"
    operands = split_operands(tokens)
    if op in NO_DESTINATION:
        if op in (41, 45):
            name += "_" + COMPARES.get((head >> 16) & 7, "?")
        body = ", ".join(source_text(o, vertex) for o in operands)
        return predicate + name + (" " + body if body else "")
    dest = operands[0][0]
    modifier = (dest >> 20) & 0xF
    suffix = (
        ("_sat" if modifier & 1 else "")
        + ("_pp" if modifier & 2 else "")
        + ("_centroid" if modifier & 4 else "")
    )
    if op == 94:
        name = "setp_" + COMPARES.get((head >> 16) & 7, "?")
    if op == 66:
        name = {0: "texld", 1: "texldp", 2: "texldb"}.get((head >> 16) & 3, "texld")
    parts = [register_name(dest, vertex) + write_mask_text(dest)]
    parts += [source_text(o, vertex) for o in operands[1:]]
    return predicate + name + suffix + " " + ", ".join(parts)


def parse_constant_table(table: bytes) -> tuple[Constant, ...]:
    _, _, _, count, info, _, _ = struct.unpack_from("<7I", table, 0)
    constants = []
    for index in range(count):
        name_at, reg_set, reg_index, reg_count, _, _, default_at = struct.unpack_from(
            "<IHHHHII", table, info + 20 * index
        )
        name = table[name_at : table.index(b"\0", name_at)].decode("ascii")
        default = None
        if default_at:
            default = struct.unpack_from(f"<{reg_count * 4}f", table, default_at)
        constants.append(
            Constant(
                name, REGISTER_SETS.get(reg_set, "?"), reg_index, reg_count, default
            )
        )
    return tuple(constants)


def decode(data: bytes, offset: int = 0) -> Shader:
    """Decode the shader whose version token is at `offset`."""
    position = offset
    (version,) = struct.unpack_from("<I", data, position)
    position += 4
    kind = version >> 16
    if kind not in (0xFFFE, 0xFFFF):
        raise ValueError(f"no shader version token at {offset:#x}")
    vertex = kind == 0xFFFE
    lines = [f"{'vs' if vertex else 'ps'}_{(version >> 8) & 255}_{version & 255}"]
    constants: tuple[Constant, ...] = ()
    while True:
        (head,) = struct.unpack_from("<I", data, position)
        op = head & 0xFFFF
        if op == END_TOKEN:
            break
        if op == COMMENT_TOKEN:
            words = (head >> 16) & 0x7FFF
            body = data[position + 4 : position + 4 + 4 * words]
            if body[:4] == b"CTAB":
                constants = parse_constant_table(body[4:])
            position += 4 + 4 * words
            continue
        words = (head >> 24) & 0xF
        tokens = list(struct.unpack_from(f"<{words}I", data, position + 4))
        position += 4 + 4 * words
        lines.append(instruction_text(op, head, tokens, vertex))
    return Shader(lines[0], tuple(lines), constants, position + 4 - offset)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path, help="file holding the shader")
    parser.add_argument("offset", help="file offset of the version token")
    parser.add_argument("--listing", action="store_true", help="print the decoded text")
    args = parser.parse_args()
    shader = decode(args.file.read_bytes(), int(args.offset, 0))
    print(f"{shader.version} {len(shader.lines) - 1} lines sha256 {shader.sha256()}")
    for item in shader.constants:
        print(f"  {item.name} {item.register_set}{item.index} x{item.count}")
    if args.listing:
        print(shader.text())
    return 0


if __name__ == "__main__":
    sys.exit(main())
