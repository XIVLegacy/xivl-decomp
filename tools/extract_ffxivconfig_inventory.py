#!/usr/bin/env python3
"""Extract the exact-build ffxivconfig resource and settings inventory."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path
from typing import Any


CONFIG_SIZE = 3_471_240
CONFIG_SHA256 = "e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977"
IMAGE_BASE = 0x00400000
RT_DIALOG = 5
RT_TYPE_NAMES = {
    2: "RT_BITMAP",
    3: "RT_ICON",
    5: "RT_DIALOG",
    14: "RT_GROUP_ICON",
    16: "RT_VERSION",
    24: "RT_MANIFEST",
}
CLASS_NAMES = {
    0x80: "BUTTON",
    0x81: "EDIT",
    0x82: "STATIC",
    0x83: "LISTBOX",
    0x84: "SCROLLBAR",
    0x85: "COMBOBOX",
}
DS_SETFONT = 0x40
LANGUAGE_RESOURCE_IDS = (0x74, 0x71, 0x73, 0x72, 0x85)
LANGUAGE_NAMES = {
    0: "Japanese",
    1: "English",
    2: "German",
    3: "French",
    4: "Chinese",
}

# These are direct FUN_0040D350 call arguments paired with control-caption
# writes in the focused init routines. A missing entry remains unknown.
DIALOG_CAPTION_KEYS = {
    101: 0x76,
    102: 0x8C,
    103: 0x64,
    110: 0xB3,
    111: 0x6F,
    112: 0x70,
    126: 0x9A,
    127: 0xF6,
    128: 0xF7,
    132: 0x10A,
}
CONTROL_CAPTION_KEYS = {
    101: {
        1: 0x6C,
        1069: 0x77,
        1070: 0x79,
        1071: 0x78,
        1072: 0x7A,
        1073: 0x7B,
        1074: 0x7E,
        1075: 0x7F,
        1076: 0x80,
        1077: 0x81,
        1078: 0x82,
        1079: 0x83,
        1080: 0x84,
        1081: 0x85,
        1082: 0x86,
        1083: 0x87,
        1084: 0x88,
        1085: 0x89,
        1086: 0x7C,
        1017: 0x8A,
        1037: 0x7D,
        1038: 0xF0,
        1110: 0x116,
        1119: 0x117,
        1120: 0x11E,
        1121: 0x83,
        1122: 0x82,
        1123: 0x89,
        1124: 0x86,
        1125: 0x85,
        1126: 0x88,
        1127: 0x7E,
        1128: 0x7F,
        1129: 0x80,
        1130: 0x81,
        1131: 0x78,
        1132: 0x79,
        1133: 0x7A,
        1134: 0x84,
        1135: 0x87,
        1136: 0x11F,
    },
    102: {1: 0x90, 1045: 0x8D, 1046: 0x8E, 1047: 0x8F},
    103: {1: 0x6C, 1018: 0x6B, 1019: 0x6A, 1020: 0xB3, 1038: 0x65},
    110: {1: 0x6C, 2: 0x97, 1089: 0xF8},
    111: {1: 0x6C, 2: 0x97},
    117: {6: 0x95, 7: 0x96},
    118: {1: 0x6C},
    119: {6: 0x95, 7: 0x96, 2: 0x97},
    112: {1048: 0x71, 1049: 0x75},
    128: {1110: 0xF5, 1111: 0xFC},
    132: {1106: 0x105, 1108: 0x106, 1107: 0x107, 1099: 0x108, 1100: 0x109},
    126: {
        1098: 0x9B,
        1097: 0x9C,
        1101: 0x9F,
        1103: 0xA0,
        1105: 0x102,
        1107: 0xA7,
        1109: 0x103,
    },
    127: {
        1102: 0xF1,
        1103: 0xF2,
        1108: 0x100,
        1115: 0x101,
        1110: 0x104,
        1105: 0xFE,
        1107: 0xFF,
        1142: 0x121,
        1143: 0x120,
    },
}

CONTROL_OPTION_KEYS = {
    (103, 1039): (0x66, 0x67, 0x68, 0x69),
    (112, 1042): (0x72, 0x73, 0x74),
    (132, 1117): (0xAC, 0xAB, 0xAA, 0xA9, 0xA8),
    (132, 1118): (0x10B, 0x10C, 0x10D, 0x10E),
    (126, 1099): (0x9D, 0x9E),
    (126, 1106): (0xA8, 0xA9, 0xAA, 0xAB, 0xAC),
    (127, 1104): (0xAB, 0xAA, 0xA9),
    (127, 1106): (0xAC, 0xAB, 0xAA, 0xA9),
}


class Reader:
    """Bounds checked little-endian reader for a resource payload."""

    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def take(self, size: int) -> bytes:
        if size < 0 or self.pos + size > len(self.data):
            raise ValueError("dialog template ends before its declared data")
        result = self.data[self.pos : self.pos + size]
        self.pos += size
        return result

    def u8(self) -> int:
        return self.take(1)[0]

    def u16(self) -> int:
        return struct.unpack("<H", self.take(2))[0]

    def i16(self) -> int:
        return struct.unpack("<h", self.take(2))[0]

    def u32(self) -> int:
        return struct.unpack("<I", self.take(4))[0]

    def align4(self) -> None:
        self.pos = (self.pos + 3) & ~3
        if self.pos > len(self.data):
            raise ValueError("dialog control is not four-byte aligned within payload")

    def sz_or_ord(self) -> dict[str, Any] | None:
        first = self.u16()
        if first == 0:
            return None
        if first == 0xFFFF:
            return {"kind": "ordinal", "ordinal": self.u16()}
        chars = [first]
        while chars[-1] != 0:
            chars.append(self.u16())
        value = bytes(struct.pack("<" + "H" * len(chars), *chars)).decode(
            "utf-16le", errors="replace"
        )[:-1]
        return {"kind": "string", "value": value}


def _hex(value: int, width: int = 8) -> str:
    return f"0x{value:0{width}x}"


def _rva(va: int) -> str:
    if va < IMAGE_BASE:
        raise ValueError(f"VA {_hex(va)} precedes image base {_hex(IMAGE_BASE)}")
    return _hex(va - IMAGE_BASE)


def ensure_distinct_output(binary: Path, output: Path) -> None:
    """Reject an output path that aliases the read-only input image."""

    binary_resolved = binary.resolve(strict=False)
    output_resolved = output.resolve(strict=False)
    same_file = False
    try:
        same_file = binary_resolved.samefile(output_resolved)
    except (FileNotFoundError, OSError):
        pass
    if same_file or binary_resolved == output_resolved:
        raise ValueError("output path must differ from input binary")


def _var_value(value: dict[str, Any] | None) -> Any:
    if value is None:
        return None
    if value["kind"] == "string":
        return value["value"]
    return None


def _var_metadata(value: dict[str, Any] | None) -> dict[str, Any] | None:
    if value is None:
        return None
    if value["kind"] == "string":
        return {"kind": "string", "value": value["value"]}
    return {"kind": "ordinal", "ordinal": value["ordinal"]}


def _pex_bits(data: bytes, bit_offset: int, width: int) -> tuple[int, int]:
    if width < 0 or bit_offset < 0 or bit_offset + width > len(data) * 8:
        raise ValueError("PEX bitstream ends before its declared output")
    value = 0
    for index in range(width):
        source = bit_offset + index
        value |= ((data[source // 8] >> (source & 7)) & 1) << index
    return value, bit_offset + width


def decode_pex(payload: bytes) -> tuple[bytes, int]:
    """Decode the small PEX bitstream used by the BINARY language tables.

    This is a static translation of FUN_0040CCC0.  The first 0x1000 output
    bytes are a zero history window; match offsets are 12 bits and lengths are
    four bits plus two.  FUN_0040CB20 subsequently moves the decoded bytes to
    the start of the allocation, which does not change their contents.
    """

    if len(payload) < 16 or payload[:4] != b"PEX\0":
        raise ValueError("BINARY localization payload is not a PEX stream")
    output_size = struct.unpack_from("<I", payload, 4)[0]
    if output_size == 0 or output_size > 16 * 1024 * 1024:
        raise ValueError("PEX output size is outside the supported static bound")
    bitstream = payload[16:]
    bit_offset = 0
    output = bytearray()
    while len(output) < output_size:
        flag, bit_offset = _pex_bits(bitstream, bit_offset, 1)
        if flag == 0:
            value, bit_offset = _pex_bits(bitstream, bit_offset, 8)
            output.append(value)
            continue
        offset, bit_offset = _pex_bits(bitstream, bit_offset, 12)
        length, bit_offset = _pex_bits(bitstream, bit_offset, 4)
        length += 2
        for _ in range(length):
            source = len(output) + offset - 0x1000
            output.append(0 if source < 0 else output[source])
            if len(output) > output_size:
                raise ValueError("PEX match overruns its declared output")
    return bytes(output), bit_offset


def decode_pex_records(decoded: bytes, bit_count: int) -> dict[str, Any]:
    if len(decoded) < 0x10 or decoded[:4] != b"STRT":
        raise ValueError("decoded BINARY localization table has no STRT header")
    records: list[dict[str, Any]] = []
    offset = 0x10
    while offset + 2 <= len(decoded):
        length = struct.unpack_from("<H", decoded, offset)[0]
        if length == 0:
            return {
                "decoded_size": len(decoded),
                "bit_count": bit_count,
                "record_count": len(records),
                "terminator_offset": _hex(offset, 4),
                "records": records,
            }
        if length < 6 or length & 1 or offset + length > len(decoded):
            raise ValueError("decoded BINARY localization record is malformed")
        key = struct.unpack_from("<H", decoded, offset + 2)[0]
        text_bytes = decoded[offset + 4 : offset + length]
        try:
            text = text_bytes.decode("utf-16le")
        except UnicodeDecodeError as error:
            raise ValueError(
                "decoded BINARY localization text is not UTF-16LE"
            ) from error
        records.append(
            {
                "offset": _hex(offset, 4),
                "length": length,
                "key": key,
                "text": text.rstrip("\0"),
            }
        )
        offset += length
    raise ValueError("decoded BINARY localization table has no terminator")


def parse_dialog(payload: bytes) -> dict[str, Any]:
    reader = Reader(payload)
    version = reader.u16()
    signature = reader.u16()
    extended = signature == 0xFFFF
    if extended:
        help_id = reader.u32()
        ex_style = reader.u32()
        style = reader.u32()
        count = reader.u16()
        x, y, width, height = (reader.i16() for _ in range(4))
        menu = reader.sz_or_ord()
        window_class = reader.sz_or_ord()
        title = reader.sz_or_ord()
        font: dict[str, Any] | None = None
        if style & DS_SETFONT:
            font = {
                "point_size": reader.u16(),
                "weight": reader.u16(),
                "italic": reader.u8(),
                "charset": reader.u8(),
                "face": _var_value(reader.sz_or_ord()),
            }
    else:
        # RT_DIALOG normally uses an extended template in this image. Keep a
        # classic parser so the extractor remains deterministic on a variant.
        help_id = 0
        style = version | signature << 16
        ex_style = reader.u32()
        count = reader.u16()
        x, y, width, height = (reader.i16() for _ in range(4))
        menu = reader.sz_or_ord()
        window_class = reader.sz_or_ord()
        title = reader.sz_or_ord()
        font = None
        if style & DS_SETFONT:
            font = {"point_size": reader.u16(), "face": _var_value(reader.sz_or_ord())}

    controls: list[dict[str, Any]] = []
    for index in range(count):
        reader.align4()
        offset = reader.pos
        if extended:
            control_help = reader.u32()
            control_ex_style = reader.u32()
            control_style = reader.u32()
            control_x, control_y, control_width, control_height = (
                reader.i16() for _ in range(4)
            )
            control_id = reader.u32()
        else:
            control_style = reader.u32()
            control_ex_style = reader.u32()
            control_x, control_y, control_width, control_height = (
                reader.i16() for _ in range(4)
            )
            control_id = reader.u16()
            control_help = 0
        class_value = reader.sz_or_ord()
        title_value = reader.sz_or_ord()
        extra_size = reader.u16()
        if extended:
            extra = reader.take(extra_size)
        elif extra_size == 0:
            extra = b""
        else:
            if extra_size < 2:
                raise ValueError(
                    "classic dialog creation data size must include its size word"
                )
            extra = reader.take(extra_size - 2)
        if class_value and class_value["kind"] == "ordinal":
            class_name = CLASS_NAMES.get(
                class_value["ordinal"], f"ordinal:{class_value['ordinal']}"
            )
        elif class_value:
            class_name = class_value["value"]
        else:
            class_name = None
        controls.append(
            {
                "index": index,
                "template_offset": offset,
                "help_id": control_help,
                "id": control_id,
                "id_hex": _hex(control_id, 4),
                "class": _var_metadata(class_value),
                "class_name": class_name,
                "title": _var_value(title_value),
                "title_descriptor": _var_metadata(title_value),
                "x": control_x,
                "y": control_y,
                "width": control_width,
                "height": control_height,
                "style": control_style,
                "style_hex": _hex(control_style),
                "extended_style": control_ex_style,
                "extended_style_hex": _hex(control_ex_style),
                "extra_hex": extra.hex(),
            }
        )
    if reader.pos != len(payload):
        raise ValueError(
            f"dialog template has {len(payload) - reader.pos} trailing bytes after controls"
        )
    result = {
        "extended_template": extended,
        "dialog_version": version if extended else None,
        "signature": signature if extended else None,
        "help_id": help_id,
        "style": style,
        "style_hex": _hex(style),
        "extended_style": ex_style,
        "extended_style_hex": _hex(ex_style),
        "control_count": count,
        "x": x,
        "y": y,
        "width": width,
        "height": height,
        "menu": _var_metadata(menu),
        "window_class": _var_metadata(window_class),
        "title": _var_value(title),
        "title_descriptor": _var_metadata(title),
        "font": font,
        "controls": controls,
    }
    return result


class Image:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if len(self.data) != CONFIG_SIZE:
            raise ValueError(f"ffxivconfig.exe: expected {CONFIG_SIZE} bytes")
        digest = hashlib.sha256(self.data).hexdigest()
        if digest != CONFIG_SHA256:
            raise ValueError("ffxivconfig.exe: input SHA-256 mismatch")
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe : pe + 4] != b"PE\0\0":
            raise ValueError("PE signature mismatch")
        section_count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        optional = pe + 24
        if struct.unpack_from("<H", self.data, optional)[0] != 0x10B:
            raise ValueError("PE32 required")
        self.base = struct.unpack_from("<I", self.data, optional + 28)[0]
        self.sections: list[tuple[int, int, int, int, str]] = []
        section_table = optional + optional_size
        for index in range(section_count):
            row = section_table + index * 40
            name = self.data[row : row + 8].rstrip(b"\0").decode("ascii", "replace")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from(
                "<IIII", self.data, row + 8
            )
            self.sections.append((rva, virtual_size, raw_size, raw_offset, name))
        if self.base != IMAGE_BASE:
            raise ValueError(f"unexpected image base {_hex(self.base)}")
        directory = optional + 96 + 8 * 2
        self.resource_rva, self.resource_size = struct.unpack_from(
            "<II", self.data, directory
        )

    def rva_to_offset(self, rva: int, size: int = 1) -> int:
        if rva < 0 or size < 0:
            raise ValueError("RVA and size must be non-negative")
        for section_rva, _virtual_size, raw_size, raw, _name in self.sections:
            if section_rva <= rva and rva + size <= section_rva + raw_size:
                offset = raw + rva - section_rva
                if offset + size <= len(self.data):
                    return offset
        raise ValueError(f"RVA {_hex(rva)} is not file-backed")

    def read_rva(self, rva: int, size: int) -> bytes:
        offset = self.rva_to_offset(rva, size)
        return self.data[offset : offset + size]

    @staticmethod
    def _resource_key(raw: int, root_offset: int, data: bytes) -> tuple[str, Any]:
        if raw & 0x80000000:
            offset = root_offset + (raw & 0x7FFFFFFF)
            length = struct.unpack_from("<H", data, offset)[0]
            chars = struct.unpack_from("<" + "H" * length, data, offset + 2)
            return "name", bytes(struct.pack("<" + "H" * length, *chars)).decode(
                "utf-16le", errors="replace"
            )
        return "id", raw

    def resources(self) -> list[dict[str, Any]]:
        root_offset = self.rva_to_offset(self.resource_rva)
        root = self.data
        rows: list[dict[str, Any]] = []

        def walk(directory_offset: int, path: list[tuple[str, Any]]) -> None:
            directory = root_offset + directory_offset
            named, ids = struct.unpack_from("<HH", root, directory + 12)
            total = named + ids
            for index in range(total):
                entry = directory + 16 + index * 8
                key = self._resource_key(
                    struct.unpack_from("<I", root, entry)[0], root_offset, root
                )
                child = struct.unpack_from("<I", root, entry + 4)[0]
                if child & 0x80000000:
                    walk(child & 0x7FFFFFFF, path + [key])
                    continue
                data_entry = root_offset + child
                data_rva, size, codepage, reserved = struct.unpack_from(
                    "<IIII", root, data_entry
                )
                leaf_path = path + [key]
                if len(leaf_path) != 3:
                    raise ValueError(
                        f"resource leaf does not have type/name/language: {leaf_path}"
                    )
                type_kind, resource_type = leaf_path[0]
                name_kind, resource_name = leaf_path[1]
                language_kind, language = leaf_path[2]
                payload_offset = self.rva_to_offset(data_rva, size)
                rows.append(
                    {
                        "type": resource_type,
                        "type_kind": type_kind,
                        "type_name": RT_TYPE_NAMES.get(resource_type, resource_type)
                        if type_kind == "id"
                        else resource_type,
                        "name": resource_name,
                        "name_kind": name_kind,
                        "language": language,
                        "language_kind": language_kind,
                        "data_rva": _hex(data_rva),
                        "data_offset": _hex(payload_offset),
                        "size": size,
                        "codepage": codepage,
                        "reserved": reserved,
                        "payload_sha256": hashlib.sha256(
                            self.data[payload_offset : payload_offset + size]
                        ).hexdigest(),
                    }
                )

        walk(0, [])
        rows.sort(
            key=lambda row: (
                str(row["type_name"]),
                str(row["name"]),
                int(row["language"]) if row["language_kind"] == "id" else 0,
            )
        )
        return rows


def _localization_tables(
    image: Image, binary_rows: list[dict[str, Any]]
) -> dict[str, Any]:
    by_id = {row["name"]: row for row in binary_rows if row["name_kind"] == "id"}
    tables: list[dict[str, Any]] = []
    for language_index, resource_id in enumerate(LANGUAGE_RESOURCE_IDS):
        row = by_id.get(resource_id)
        if row is None:
            raise ValueError(
                f"BINARY localization resource {_hex(resource_id, 4)} is missing"
            )
        payload = image.read_rva(int(row["data_rva"], 16), row["size"])
        decoded, bit_count = decode_pex(payload)
        table = decode_pex_records(decoded, bit_count)
        table.update(
            {
                "language_index": language_index,
                "language_name": LANGUAGE_NAMES.get(language_index, "unknown"),
                "resource_id": resource_id,
                "resource_locator": {
                    "type": "BINARY",
                    "name": resource_id,
                    "language": row["language"],
                    "payload_rva": row["data_rva"],
                    "payload_offset": row["data_offset"],
                    "payload_size": row["size"],
                    "payload_sha256": row["payload_sha256"],
                },
            }
        )
        tables.append(table)
    return {
        "resource_ids_by_index": list(LANGUAGE_RESOURCE_IDS),
        "tables": tables,
        "record_format": {
            "decoded_base": _hex(0x10, 2),
            "length_offset": 0,
            "key_offset": 2,
            "text_offset": 4,
            "length_unit": "bytes including the 4-byte length/key header",
            "terminator": "first u16 length equal to zero",
        },
    }


def _known_runtime(dialog_id: int) -> dict[str, Any]:
    runtime: dict[int, dict[str, Any]] = {
        101: {
            "dialog_proc": {
                "name": "FUN_00432890",
                "va": _hex(0x00432890),
                "rva": _hex(0x32890),
            },
            "init_proc": {
                "name": "FUN_00431f10",
                "va": _hex(0x00431F10),
                "rva": _hex(0x31F10),
            },
            "command_proc": {
                "name": "FUN_004318b0",
                "va": _hex(0x004318B0),
                "rva": _hex(0x318B0),
            },
            "caption_source": "runtime_localized_lookup_FUN_0040d350",
        },
        102: {
            "dialog_proc": {
                "name": "FUN_00431510",
                "va": _hex(0x00431510),
                "rva": _hex(0x31510),
            },
            "caption_source": "runtime_localized_lookup_FUN_0040d350",
        },
        103: {
            "dialog_proc": {
                "name": "FUN_004335a0",
                "va": _hex(0x004335A0),
                "rva": _hex(0x335A0),
            },
            "init_proc": {
                "name": "FUN_004331e0",
                "va": _hex(0x004331E0),
                "rva": _hex(0x331E0),
            },
            "command_proc": {
                "name": "FUN_004334d0",
                "va": _hex(0x004334D0),
                "rva": _hex(0x334D0),
            },
            "caption_source": "runtime_localized_lookup_FUN_0040d350",
        },
        110: {
            "dialog_proc": {"name": "LAB_00441ea0", "va": None, "rva": None},
            "callback_pointer": {
                "va": _hex(0x00441EA0),
                "rva": _rva(0x00441EA0),
                "function_start_unresolved": True,
            },
            "init_proc": {
                "name": "FUN_00441d40",
                "va": _hex(0x00441D40),
                "rva": _hex(0x41D40),
            },
        },
        111: {
            "dialog_proc": {"name": "LAB_00442570", "va": None, "rva": None},
            "callback_pointer": {
                "va": _hex(0x00442570),
                "rva": _rva(0x00442570),
                "function_start_unresolved": True,
            },
            "init_proc": {
                "name": "FUN_00442450",
                "va": _hex(0x00442450),
                "rva": _hex(0x42450),
            },
        },
        112: {
            "dialog_proc": {"name": "LAB_004352d0", "va": None, "rva": None},
            "callback_pointer": {
                "va": _hex(0x004352D0),
                "rva": _hex(0x352D0),
                "function_start_unresolved": True,
            },
            "init_proc": {
                "name": "FUN_004353e0",
                "va": _hex(0x004353E0),
                "rva": _hex(0x353E0),
            },
        },
        126: {
            "dialog_proc": {
                "name": "FUN_00434fb0",
                "va": _hex(0x00434FB0),
                "rva": _hex(0x34FB0),
            },
            "init_proc": {
                "name": "FUN_00435000",
                "va": _hex(0x00435000),
                "rva": _hex(0x35000),
            },
        },
        127: {
            "dialog_proc": {
                "name": "FUN_00435c60",
                "va": _hex(0x00435C60),
                "rva": _hex(0x35C60),
            },
            "init_proc": {
                "name": "FUN_00435cb0",
                "va": _hex(0x00435CB0),
                "rva": _hex(0x35CB0),
            },
        },
        128: {
            "dialog_proc": {"name": "LAB_00433a80", "va": None, "rva": None},
            "callback_pointer": {
                "va": _hex(0x00433A80),
                "rva": _hex(0x33A80),
                "function_start_unresolved": True,
            },
            "init_proc": {
                "name": "FUN_00433b20",
                "va": _hex(0x00433B20),
                "rva": _hex(0x33B20),
            },
        },
        132: {
            "dialog_proc": {"name": "LAB_00436720", "va": None, "rva": None},
            "callback_pointer": {
                "va": _hex(0x00436720),
                "rva": _hex(0x36720),
                "function_start_unresolved": True,
            },
            "init_proc": {
                "name": "FUN_00436770",
                "va": _hex(0x00436770),
                "rva": _hex(0x36770),
            },
        },
        117: {"dialog_proc": {"name": "LAB_00432e10", "va": None, "rva": None}},
        118: {"dialog_proc": {"name": "LAB_00432e10", "va": None, "rva": None}},
        119: {"dialog_proc": {"name": "LAB_00432e10", "va": None, "rva": None}},
    }
    if dialog_id in (122, 123, 124, 125):
        return {
            "role": "system_information_report_container",
            "parent_dialog_resource": 110,
            "dynamic_controls_reference": "application_behavior.system_information.report_pages",
            "evidence": "Resource 110 factories create these modeless containers and populate dynamic list views; their templates contain no controls",
        }
    return runtime.get(
        dialog_id,
        {
            "dialog_proc": None,
            "caption_source": "resource_template_only_unknown_runtime_join",
        },
    )


def _control_join(
    dialog_id: int, control_id: int, map_pairs: list[tuple[int, int]]
) -> dict[str, Any]:
    unknown = {
        "status": "unknown",
        "reason": "No focused persisted-field or command-handler join is recorded; caption evidence is recorded separately.",
    }
    if dialog_id == 101:
        for map_index, (mapped_id, next_index) in enumerate(map_pairs):
            if control_id == mapped_id:
                return {
                    "status": "observed",
                    "kind": "signed_mapping_byte",
                    "file": "config.pad",
                    "file_offset": _hex(0x2C + map_index, 2),
                    "width": 1,
                    "map_index": map_index,
                    "next_index": next_index,
                    "evidence": "FUN_00430e20/FUN_004318b0 and table VA 0x0044d6c0",
                }
        direct = {
            1001: {
                "status": "observed",
                "kind": "preset_row_selector",
                "backing_status": "choice_consumed_by_preset_command",
                "evidence": "FUN_00431f10 inserts three labels from table VA 0x00457450; command 0x456 in FUN_004318b0 reads combo 0x3e9 and copies the selected 24-byte row from VA 0x00457408",
            },
            1: {
                "status": "observed",
                "kind": "save_cancel_command",
                "file": "config.pad",
                "file_size": _hex(0x148, 3),
                "evidence": "FUN_004318b0 compares and writes the 0x148-byte snapshot",
            },
            1017: {
                "status": "observed",
                "kind": "input_test_dialog",
                "dialog_resource": 102,
                "evidence": "FUN_004318b0 opens resource 102 with FUN_00431510; caption key 0x8c is Test Gamepad",
            },
            1018: {
                "status": "observed",
                "kind": "selected_gamepad_record",
                "evidence": "FUN_004318b0 and FUN_00430b30 use the selected record; exact record layout is controller evidence",
            },
            1019: {
                "status": "observed",
                "kind": "deadzone_selector",
                "file": "config.pad",
                "file_offset": _hex(0x28, 2),
                "width": 4,
                "conversion": "selector 0..5 is consumed as a 0.1 step by the game",
                "evidence": "FUN_00430e70 reads combo 0x3fb into DAT_0045c2b8",
            },
            1037: {
                "status": "observed",
                "kind": "checked_boolean",
                "file": "config.pad",
                "file_offset": _hex(0x24, 2),
                "width": 4,
                "meaning": "Enable gamepad when application is in background; direct caption key 0x7d",
                "evidence": "FUN_00430e70 reads IsDlgButtonChecked(0x40d)",
            },
            1038: {
                "status": "observed",
                "kind": "checked_boolean",
                "file": "config.pad",
                "file_offset": _hex(0x44, 2),
                "width": 4,
                "meaning": "Activate gamepad vibration; direct caption key 0xf0",
                "evidence": "FUN_00430e70 reads IsDlgButtonChecked(0x40e)",
            },
            1110: {
                "status": "observed",
                "kind": "preset_reset_command",
                "backing_fields": "config.pad mapping bytes 0x2c..0x43",
                "evidence": "FUN_004318b0 copies DAT_00457408 preset row then calls FUN_00430e20",
            },
            1119: {
                "status": "observed",
                "kind": "default_reset_command",
                "backing_fields": "config.pad temporary 0x148-byte block",
                "evidence": "FUN_004318b0 calls FUN_00430dd0 then FUN_00430e20",
            },
        }
        if control_id in direct:
            return direct[control_id]
        if control_id in (1111, 1112, 1113, 1114, 1137):
            return {
                "status": "observed",
                "kind": "template_group_box",
                "backing_status": "none_recorded",
                "evidence": "Resource 101 declares these BUTTON controls with style 0x50000007 (BS_GROUPBOX); no focused field or command join is recorded",
            }
        if control_id in CONTROL_CAPTION_KEYS[101]:
            return {
                "status": "observed",
                "kind": "localized_caption_only",
                "backing_status": "none",
                "evidence": "FUN_00431f10 directly writes this control's localized caption; labels are separate from the assignment controls and stored bytes",
            }
    if dialog_id == 103:
        if control_id == 1038:
            return {
                "status": "observed",
                "kind": "localized_caption_only",
                "backing_status": "none",
                "evidence": "FUN_004331e0 writes Language key 0x65 to static control 0x40e; combo 0x40f owns selection",
            }
        direct = {
            1018: {
                "status": "observed",
                "kind": "open_controller_dialog",
                "dialog_resource": 101,
                "evidence": "FUN_004334d0 calls FUN_00432b10 for command 0x3fa",
            },
            1019: {
                "status": "observed",
                "kind": "open_config_sys_dialog",
                "dialog_resource": 111,
                "evidence": "FUN_004334d0 calls FUN_004428d0 for command 0x3fb",
            },
            1020: {
                "status": "observed",
                "kind": "open_other_settings_dialog",
                "dialog_resource": 110,
                "evidence": "FUN_004334d0 opens resource 110 with LAB_00441ea0",
            },
            1039: {
                "status": "observed",
                "kind": "language_selector",
                "file": "config.lng",
                "file_offset": _hex(0x04, 2),
                "width": 1,
                "evidence": "FUN_00433020 fills four language choices; FUN_004332a0 writes the 8-byte file",
            },
            1: {
                "status": "observed",
                "kind": "language_save_or_cancel_command",
                "file": "config.lng",
                "file_size": _hex(8, 2),
                "evidence": "FUN_004332a0 compares and writes DAT_0045912c",
            },
        }
        if control_id in direct:
            return direct[control_id]
    config_sys_fields = {
        112: {
            1042: (0x04, "combo_selection"),
            1044: (0x08, "numeric_edit_text"),
        },
        126: {
            1043: (0x0C, "combo_selection"),
            1099: (0x10, "combo_selection"),
            1100: ((0x14, 0x18), "table_selected_pair"),
            1102: ((0x24, 0x28), "table_selected_pair"),
            1104: (0x1C, "combo_selection"),
            1108: (0x20, "combo_selection"),
            1106: (0x2C, "combo_selection"),
        },
        127: {
            1102: (0x30, "checked_boolean"),
            1103: (0x34, "checked_boolean"),
            1108: (0x38, "checked_boolean"),
            1104: (0x3C, "combo_selection"),
            1106: (0x40, "combo_selection"),
            1109: (0x44, "combo_selection"),
            1115: (0x50, "checked_boolean"),
            1141: (0xA0, "utf16_edit_text"),
        },
        128: {1110: (0x48, "checked_boolean"), 1111: (0x4C, "checked_boolean")},
        132: {
            1106: (0x54, "checked_boolean"),
            1117: (0x58, "combo_selection"),
            1118: (0x5C, "combo_selection"),
            1116: (0x60, "selected_utf16_font"),
        },
    }
    if dialog_id in config_sys_fields and control_id in config_sys_fields[dialog_id]:
        offsets, kind = config_sys_fields[dialog_id][control_id]
        if isinstance(offsets, int):
            offsets = (offsets,)
        join = {
            "status": "observed",
            "kind": kind,
            "file": "config.sys",
            "evidence": "config.sys child capture handler and FUN_004420B0 save path",
        }
        if len(offsets) == 1:
            join["file_offset"] = _hex(offsets[0], 2)
            join["width"] = 4 if offsets[0] < 0x60 else "UTF-16LE string"
        else:
            join["file_offsets"] = _offset_list(list(offsets))
            join["width"] = "4 bytes each"
            join["value_count"] = len(offsets)
        return join
    if dialog_id == 102:
        if control_id == 1:
            return {
                "status": "observed",
                "kind": "close_command",
                "backing_status": "none",
                "meaning": "closes the input-test dialog; no saved-state write is observed",
                "evidence": "FUN_00431510 handles resource 102 control ID 1 as the close command",
            }
        if control_id in (0x3FD, 0x3FE, 0x3FF):
            base = (control_id - 0x3FD) * 4
            return {
                "status": "observed",
                "kind": "input_test_visual_control",
                "backing_status": "read_only_runtime_values",
                "mapping_indexes": [base + 2, base + 3, base, base + 1],
                "meaning": "signed config.pad mapping bytes select floats at polled record+0x20+source*4 for custom GDI drawing; no persisted visual field or write",
                "evidence": "FUN_00430fe0 initializes visual IDs and twelve source indexes, calls FUN_004405f0, then draws each control; expanded in docs/input/controller-mapping.md",
            }
        if control_id in (0x415, 0x416, 0x417):
            return {
                "status": "observed",
                "kind": "localized_caption_only",
                "backing_status": "none",
                "evidence": "FUN_00431510 sets keys 0x8d..0x8f on IDs 0x415..0x417; resource geometry associates these labels with the three visuals",
            }
    if dialog_id == 110:
        if control_id == 1:
            return {
                "status": "observed",
                "kind": "close_command",
                "backing_status": "none",
                "evidence": "Callback VA 0x00441ea0 command-1 branch at VA 0x0044205f..0x00442066 calls EndDialog with result 1",
            }
        if control_id == 1089:
            return {
                "status": "observed",
                "kind": "clipboard_report_command",
                "backing_status": "none",
                "helper_va": _hex(0x00441B10),
                "helper_rva": _rva(0x00441B10),
                "evidence": "Callback VA 0x00441ea0 dispatch at VA 0x00442025..0x0044203b selects control 0x441 and calls FUN_00441b10",
            }
        if control_id == 1040:
            return {
                "status": "observed",
                "kind": "system_information_tabs",
                "backing_status": "none",
                "child_resources_by_tab_index": [122, 125, 124, 123],
                "evidence": "Callback VA 0x00441ea0 handles TCN_SELCHANGE at VA 0x00441ec8..0x00441fed; jump table VA 0x00442078 selects which child HWND is shown",
            }
    if dialog_id == 111:
        if control_id in (1, 2):
            return {
                "status": "observed",
                "kind": "config_sys_save_or_cancel_command",
                "file": "config.sys",
                "handler_parameter": control_id,
                "evidence": "Callback VA 0x00442570 command branch at VA 0x00442893..0x004428a2 passes the full wParam to FUN_004420b0; its command-1/2 paths own capture, comparison and confirmation",
            }
        if control_id == 1040:
            return {
                "status": "observed",
                "kind": "config_sys_page_tabs",
                "backing_status": "none",
                "child_resources_by_tab_index": [126, 112, 127, 128, 132],
                "evidence": "Callback VA 0x00442570 TCN_SELCHANGE branch reads TCM_GETCURSEL at VA 0x004425c4, bounds it to 0..4 at VA 0x004425ca, and selects ShowWindow calls through table VA 0x004428b0",
            }
    if dialog_id in (117, 118, 119):
        if control_id in (1, 2, 6, 7):
            return {
                "status": "observed",
                "kind": "prompt_result_command",
                "backing_status": "none",
                "dialog_result": control_id,
                "evidence": "Callback VA 0x00432e10 checks command IDs 1..7 and dispatches through FUN_00432c70; table VA 0x00432ca0 maps these four IDs to EndDialog with the same result",
            }
        if control_id == 1087:
            return {
                "status": "observed",
                "kind": "caller_supplied_prompt_message",
                "backing_status": "none",
                "evidence": "FUN_00432cc0 VA 0x00432d31..0x00432d53 selects the message string at DAT_004579c0 and writes it to control 0x43f",
            }
    if dialog_id == 127 and control_id in (1140, 1142):
        return {
            "status": "observed",
            "kind": "directory_browse_command"
            if control_id == 1140
            else "directory_default_command",
            "backing_status": "updates_edit_before_capture",
            "target_edit_control_id": 1141,
            "evidence": "FUN_004357e0 compares low command IDs 0x474/0x476; 0x474 requests SHBrowseForFolderW/SHGetPathFromIDListW and writes edit 0x475, while 0x476 calls FUN_00435760 to set the default edit text",
        }
    if dialog_id == 127 and control_id == 1143:
        return {
            "status": "observed",
            "kind": "localized_group_box",
            "backing_status": "none",
            "evidence": "FUN_004358d0 writes caption key 0x120 to group box 0x477; FUN_004357e0 has no command case for this ID",
        }
    if control_id in CONTROL_CAPTION_KEYS.get(dialog_id, {}) and dialog_id in (
        112,
        126,
        127,
        132,
    ):
        return {
            "status": "observed",
            "kind": "localized_static_caption",
            "backing_status": "none",
            "evidence": "Direct initializer caption-key/control-ID pair; no persisted-field capture for this static label",
        }
    return unknown


def _localized_texts(
    key: int, localized_strings: dict[int, dict[int, str]]
) -> dict[str, str]:
    return {
        str(language_index): strings[key]
        for language_index, strings in sorted(localized_strings.items())
        if key in strings
    }


def _offset_list(offsets: list[int]) -> list[str]:
    return [_hex(offset, 2) for offset in offsets]


_CONFIG_SYS_SAVE_CONSUMER = (
    "FUN_004420b0(1) captures network, display, graphics, sound and font; its "
    "global unsigned gate requires snapshot offset 0x08 in 0xd800..0xd8ff "
    "before comparing current VA 0x0045c600 with snapshot VA 0x0045c8b0; "
    "confirmed save passes the 0x2ac-byte snapshot to FUN_00441910"
)


def _application_behavior(image: Image) -> dict[str, Any]:
    pages = []
    for (
        tab_index,
        resource,
        key,
        factory,
        create_call,
        callback,
        child,
        view,
        class_va,
    ) in (
        (
            0,
            122,
            0xB3,
            0x00439C90,
            0x0043A088,
            0x00437680,
            0x00458CB0,
            0x00458C9C,
            0x0044DDD4,
        ),
        (
            1,
            125,
            0xEE,
            0x0043F160,
            0x0043F2FC,
            0x0043EF60,
            0x00458C98,
            0x00458C84,
            0x0044EED0,
        ),
        (
            2,
            124,
            0xE9,
            0x004368B0,
            0x00436A5A,
            0x00436850,
            0x00458CE0,
            0x00458CCC,
            0x0044DBF8,
        ),
        (
            3,
            123,
            0xEC,
            0x00436FB0,
            0x0043715B,
            0x00436F50,
            0x00458CC8,
            0x00458CB4,
            0x0044DCA8,
        ),
    ):
        pages.append(
            {
                "tab_index": tab_index,
                "child_dialog_resource": resource,
                "tab_caption_key": key,
                "factory_va": _hex(factory),
                "factory_rva": _rva(factory),
                "callback_pointer_va": _hex(callback),
                "callback_pointer_rva": _rva(callback),
                "child_hwnd_global_va": _hex(child),
                "child_hwnd_global_rva": _rva(child),
                "list_view_hwnd_global_va": _hex(view),
                "list_view_hwnd_global_rva": _rva(view),
                "list_view_create_call_va": _hex(create_call),
                "list_view_create_call_rva": _rva(create_call),
                "class_name": image.read_rva(class_va - IMAGE_BASE, 28)
                .decode("utf-16le")
                .rstrip("\0"),
                "class_name_va": _hex(class_va),
                "class_name_rva": _rva(class_va),
                "style": _hex(0x50000001),
                "menu_or_control_id": 0,
                "evidence": "Parent factories create modeless child dialogs and list views; callback selection maps the stored child HWNDs to the final tab index",
            }
        )
    return {
        "startup": {
            "routine_va": _hex(0x00433660),
            "routine_rva": _rva(0x00433660),
            "initialization_calls": "VA 0x00433684 requests CoInitialize(NULL); VA 0x00433692 requests timeBeginPeriod(1); language setup precedes the dialog calls",
            "mutex_name": image.read_rva(0x4D840, 74).decode("utf-16le").rstrip("\0"),
            "mutex_name_va": _hex(0x0044D840),
            "mutex_name_rva": _rva(0x0044D840),
            "dialog_selection": "CreateMutexW call VA 0x004338b1 and GetLastError call VA 0x004338b9 precede comparison with 0xb7; equality opens prompt resource 118 at VA 0x00433934, otherwise VA 0x004339d5 opens main resource 103 with callback FUN_004335a0",
            "dialog_completion": "VA 0x00433953 requests timeEndPeriod(1), VA 0x00433959 requests CoUninitialize, and nonnull mutex handles are closed; this cleanup branch is not reached when GetModuleFileNameW fails earlier",
            "limit": "These are application startup call/branch observations, not API success or a recovered CRT entry implementation",
        },
        "confirmation_dialogs": {
            "resources": [117, 118, 119],
            "callback_pointer_va": _hex(0x00432E10),
            "callback_pointer_rva": _rva(0x00432E10),
            "initializer_va": _hex(0x00432CC0),
            "initializer_rva": _rva(0x00432CC0),
            "message_control_id": 1087,
            "message_source": "Caller-supplied UTF-16 string objects at DAT_004579c0 (message) and DAT_004579a4 (title); FUN_00432cc0 selects inline or heap storage by each object's capacity",
            "result_commands": {"1": 1, "2": 2, "6": 6, "7": 7},
            "command_slice": "VA 0x00432e2a..0x00432e40 range-checks the low command word; jump table VA 0x00432ca0 selects EndDialog result code",
            "window_close_result": 2,
            "window_close_call_va": _hex(0x00432E62),
            "window_close_call_rva": _rva(0x00432E62),
            "limit": "Prompt results do not independently write any configuration file; the caller owns save/discard behavior",
        },
        "language_change": {
            "dialog_resource": 103,
            "control_id": 1039,
            "offered_indexes": [0, 1, 2, 3],
            "option_caption_keys": [0x66, 0x67, 0x68, 0x69],
            "refresh_va": _hex(0x00433020),
            "refresh_rva": _rva(0x00433020),
            "notification_slice": "VA 0x0043355c..0x00433595, exclusive end; HIWORD(WPARAM)==1 reads combo 0x40f, stores AL at VA 0x00459130 and calls FUN_00433020",
            "live_effect": "Reloads the selected PEX strings and repopulates dialog captions and the four combo items in memory",
            "persistence": "FUN_004332a0 handles current/original comparison, confirmation and the eight-byte WriteFile separately; a selection notification is not a file write",
            "chinese_resource_limit": "The fifth decoded language resource is not inserted into this four-item dropdown",
        },
        "settings_page_behavior": {
            "general_tab_resources_by_index": [126, 112, 127, 128, 132],
            "general_tab_dispatch": "Callback VA 0x00442570 bounds TCM_GETCURSEL to 0..4 and dispatches ShowWindow through table VA 0x004428b0",
            "network": {
                "dialog_resource": 112,
                "caption_and_options_helper_va": _hex(0x00435190),
                "caption_and_options_helper_rva": _rva(0x00435190),
                "command": "Callback VA 0x004352d0 handles WM_COMMAND 0x111 only when HIWORD(wParam)==1; FUN_00435140 enables edit 1044 iff combo 1042 selection != 0",
                "initial_edit": "FUN_00435190 formats field +0x08 as %u and enables edit 1044 iff +0x04 != 0",
            },
            "display": {
                "dialog_resource": 126,
                "caption_and_options_helper_va": _hex(0x004346B0),
                "caption_and_options_helper_rva": _rva(0x004346B0),
                "command": "WM_COMMAND with HIWORD(wParam)==1 calls FUN_00434610; IDs 1043,1099,1100,1102 dispatch to FUN_00434430,FUN_00433e20,FUN_00433ff0,FUN_00433c20 respectively",
                "runtime_lists": "FUN_004346b0 loads d3d9.dll and Direct3DCreate9 for adapter names/capabilities; FUN_004340e0 adds unique adapter modes with width >=1024 and height >=720; exact adapter/resolution/multisampling lists require runtime enumeration",
            },
            "graphics": {
                "dialog_resource": 127,
                "caption_and_options_helper_va": _hex(0x004358D0),
                "caption_and_options_helper_rva": _rva(0x004358D0),
                "command": "Callback FUN_00435c60 sends WM_COMMAND to FUN_004357e0; only browse 1140 and default 1142 have command cases",
                "directory": "FUN_004358d0 writes the saved directory or calls FUN_00435760 when empty; browse/default update edit 1141 before FUN_004354b0 captures it",
                "default_directory": {
                    "success_path": "<Personal>\\My Games\\FINAL FANTASY XIV[ LATEST]\\screenshots",
                    "caller_va": _hex(0x00435760),
                    "caller_rva": _rva(0x00435760),
                    "path_helper_va": _hex(0x0043FA40),
                    "path_helper_rva": _rva(0x0043FA40),
                    "root_helper_va": _hex(0x0043F950),
                    "root_helper_rva": _rva(0x0043F950),
                    "folder_provider": "FUN_00415a80 uses SHGetSpecialFolderLocation(NULL,5,&pidl), then SHGetPathFromIDListW when its destination is empty; 5 is the Personal folder",
                    "variant": "FUN_0043f7c0 constructs module-directory\\latest.txt; FUN_0040e7e0 tests GetFileAttributesW != 0xffffffff and absence of directory bit 0x10; the cached result selects FINAL FANTASY XIV LATEST or FINAL FANTASY XIV",
                    "module_directory": "Startup FUN_00433660 removes the final path component from GetModuleFileNameW and assigns the directory to the string object at VA 0x0045cb60; FUN_0043f950 requires its nonempty first character",
                    "conversion": "FUN_00435640 calls MultiByteToWideChar with code page 0, flags 0 and input length -1 at VAs 0x004356af and 0x004356ea",
                    "failure_limit": "When FUN_0043f950 returns false, FUN_0043fa40 does not copy its output; FUN_00435760 calls FUN_0043fa40 at VA 0x0043577a then converts the caller buffer through FUN_00435640 at VA 0x00435785 without checking that result; an empty/safe failure fallback is not established",
                    "folder_api_limit": "FUN_00415a80 does not test SHGetPathFromIDListW's BOOL at VA 0x00415b18 before reaching its true return path; a true helper result does not establish that API's success",
                    "runtime_dependencies": "Personal-folder redirection, module location, cached marker-file attributes, conversion/API outcomes and failed-lookup output are unobserved",
                },
            },
            "sound": {
                "dialog_resource": 128,
                "caption_and_options_helper_va": _hex(0x00433A00),
                "caption_and_options_helper_rva": _rva(0x00433A00),
                "callback_dispatch": "VA 0x00433a89 JE targets 0x00433a95 for WM_INITDIALOG 0x110, reaching call 0x00433a9a to FUN_00433a00; VA 0x00433a8e JE targets 0x00433aa0 for WM_COMMAND 0x111, returning 1 without calling init or capture",
                "capture": "FUN_00433ab0 reads current checkbox states only through the root capture orchestration; command notifications do not rewrite the config block",
            },
            "font": {
                "dialog_resource": 132,
                "caption_and_options_helper_va": _hex(0x00435FC0),
                "caption_and_options_helper_rva": _rva(0x00435FC0),
                "command": "Callback VA 0x00436720 sends WM_COMMAND low ID 1106 to FUN_00435d80, which enables combos 1116,1117,1118 iff IsDlgButtonChecked(1106)==1; init disables the three combos when +0x54==0",
                "enumeration": "FUN_00435fc0 uses EnumFontFamiliesExW callback FUN_00435f10, omits names beginning @, and removes records when GetGlyphOutlineW('A',format=5) returns 0xffffffff for a temporary 24-pixel weight-400 font",
                "selection": "The MS UI Gothic literal provides a fallback selection; saved +0x60 can replace it; CB_SETCURSEL receives -1 if neither matches",
                "capture_when_disabled": "FUN_00435e20 stores checkbox +0x54 and then unconditionally captures all three combos; disabled controls are still read",
                "invalid_selection": "VA 0x00435ece calls FUN_0041d2db for a null list base or selected unsigned index not below the record count; if it returns, VA 0x00435ed3 reloads the base and VA 0x00435ee2 reads record+0x18 without another bounds/null check",
                "error_dispatch": "FUN_0041d2b7 decodes global VA 0x004581b4; nonnull result reaches tail JMP EAX at VA 0x0041d2cb, while the null branch reaches FUN_0041d1bb and a TerminateProcess(GetCurrentProcess(),0xc000000d) request at VA 0x0041d29b; callback state and API results are unobserved",
                "face_copy": "Caller VA 0x00435f03 invokes FUN_0041dad1 with destination +0x60 and count 0x20, then ignores its result; invalid arguments and exhausted count dispatch errors 0x16/0x22, conditionally returning those codes if the dispatcher returns",
                "error_finding": "docs/input/config-file-error-handling.md",
                "limit": "Enumeration/probe calls do not establish a displayed preview or game renderer behavior; malformed source accessibility, error callback/decoder state and actual process/display outcomes remain unobserved",
            },
        },
        "system_information": {
            "collection_finding": "docs/input/config-system-information.md",
            "dialog_resource": 110,
            "tab_control_id": 1040,
            "report_pages": pages,
            "factory_call_order": [123, 124, 125, 122],
            "tab_order_reason": "Each factory inserts its tab at index 0, so the last inserted System Information tab becomes index 0",
            "selection_slice": "VA 0x00441ec8..0x00441fed checks TCN_SELCHANGE (-551), reads TCM_GETCURSEL, bounds the result to 0..3 and selects ShowWindow calls using the table at VA 0x00442078",
            "clipboard": {
                "control_id": 1089,
                "helper_va": _hex(0x00441B10),
                "helper_rva": _rva(0x00441B10),
                "report_append_calls": [
                    "0x00437b90",
                    "0x0043ef90",
                    "0x00436f80",
                    "0x00436880",
                ],
                "encoding_helper_va": _hex(0x004419F0),
                "encoding_helper_rva": _rva(0x004419F0),
                "encoding": "WideCharToMultiByte with code page 0 and flags 0 at VAs 0x00441a61 and 0x00441a9e",
                "allocation_flags": _hex(0x42, 2),
                "clipboard_format": 1,
                "evidence": "VA 0x00441c4e calls GlobalAlloc; nonnull allocation/lock reach OpenClipboard, EmptyClipboard, SetClipboardData(1,handle) at VA 0x00441ca3, and CloseClipboard",
                "limit": "The static calls do not establish conversion or clipboard API success",
            },
            "provider_limit": "Report values come from registry/process/application/system collection paths; OS API results and complete platform/library internals were not observed",
        },
    }


def _config_sys_field_notes() -> dict[tuple[int, int], dict[str, str]]:
    cb_no_range = "CB_GETCURSEL result; no explicit range check in the page handler"
    checked = "IsDlgButtonChecked result is reduced to the stored value 0 or 1"
    notes: dict[tuple[int, int], dict[str, str]] = {}

    def add(
        dialog_resource: int,
        control_ids: list[int],
        kind: str,
        meaning: str,
        bounds: str,
        status: str = "value_shape_observed_semantic_unknown",
    ) -> None:
        for control_id in control_ids:
            notes[(dialog_resource, control_id)] = {
                "kind": kind,
                "meaning_status": status,
                "meaning": meaning,
                "bounds": bounds,
                "consumer": _CONFIG_SYS_SAVE_CONSUMER,
            }

    add(
        112,
        [1042],
        "combo_selection",
        "UPnP option indexes 0=Automatic, 1=Use, 2=Do not use; direct option keys 0x72..0x74 in FUN_00435190",
        f"{cb_no_range} in FUN_00435320",
        status="producer_options_observed_game_effect_unresolved",
    )
    add(
        112,
        [1044],
        "numeric_edit_text",
        "Network numeric edit; separate static 1049 has Port (55296 - 55551), key 0x75; the save gate independently uses that range, but there is no direct caption call to this edit",
        "GetWindowTextW cchTextMax=0x80 and decimal _wcstol conversion through thunk_FUN_0041db3d; capture has no range check, while the root OK path requires unsigned 0xd800..0xd8ff",
        status="producer_value_and_separate_label_observed",
    )
    add(
        126,
        [1043],
        "combo_selection",
        "D3D9 adapter index; FUN_004346b0 obtains runtime adapter identifier names; separate label 1098 is Devices",
        f"{cb_no_range} in FUN_00434210",
        status="producer_dynamic_options_observed",
    )
    for control_id, meaning in (
        (
            1099,
            "Display Mode indexes 0=Windowed, 1=Full Screen; direct option keys 0x9d/0x9e",
        ),
        (
            1104,
            "General Drawing Quality indexes 0..9 display 1..10; localized qualifiers are Lowest at 0, Standard at 7, Highest at 9",
        ),
        (
            1108,
            "Background Drawing Quality indexes 0..4 display 1..5; localized qualifiers are Low at 0, Standard at 2, High at 4",
        ),
        (
            1106,
            "Shadow Detail indexes 0..4 are Lowest, Low, Standard, High, Highest; direct keys 0xa8..0xac",
        ),
    ):
        add(
            126,
            [control_id],
            "combo_selection",
            meaning,
            f"{cb_no_range} in FUN_00434210",
            status="producer_options_observed_game_effect_qualified",
        )
    add(
        126,
        [1100],
        "table_selected_pair",
        "Window Size selection supplies width/height at offsets 0x14/0x18; FUN_00433e20 formats each FUN_00408f40 row as %ux%u",
        "CB_GETCURSEL index is checked against the FUN_00408f40 table count; one selected 8-byte entry supplies both dwords and the exact count is data-dependent",
        status="producer_dynamic_pair_observed",
    )
    add(
        126,
        [1102],
        "table_selected_pair",
        "Multisampling selection supplies the first two dwords of a FUN_00408e40 row to offsets 0x24/0x28; row +8 supplies the option text; capability rows are data-dependent",
        "CB_GETCURSEL index is checked against the FUN_00408e40 table count; one selected 0x48-byte entry supplies both dwords and the exact count is data-dependent",
        status="producer_dynamic_pair_observed_game_effect_unresolved",
    )
    for control_id, meaning in (
        (1102, "Ambient Occlusion; direct caption key 0xf1"),
        (1103, "Depth of Field; direct caption key 0xf2"),
        (1108, "Disable cutscene effects; direct caption key 0x100"),
        (1115, "Enable hardware mouse cursor; direct caption key 0x101"),
    ):
        add(
            127,
            [control_id],
            "checked_boolean",
            meaning,
            checked,
            status="producer_caption_observed_game_effect_unresolved",
        )
    for control_id, meaning in (
        (
            1104,
            "Texture Quality indexes 0=High, 1=Standard, 2=Low; direct keys 0xab,0xaa,0xa9",
        ),
        (
            1106,
            "Texture Filtering indexes 0=Highest, 1=High, 2=Standard, 3=Low; direct keys 0xac..0xa9",
        ),
        (
            1109,
            "Frame Rate Limit indexes 0..4 offer literal strings 60,30,20,15,10 at VAs 0x0044db38..0x0044db58",
        ),
    ):
        add(
            127,
            [control_id],
            "combo_selection",
            meaning,
            f"{cb_no_range} in FUN_004354b0",
            status="producer_options_observed_game_effect_unresolved",
        )
    add(
        127,
        [1141],
        "utf16_edit_text",
        "Directory edit inside group box 1143, caption key 0x120 Screenshot Directory; browse/default commands target this edit, whose own caption remains absent",
        "GetWindowTextW cchTextMax=0x104 copies at most 0x103 UTF-16 code units before NUL to current offset 0xa0",
        status="producer_group_and_command_join_observed_game_effect_unresolved",
    )
    add(
        128,
        [1110],
        "checked_boolean",
        "Disable Sound Device; direct caption key 0xf5 in FUN_00433a00",
        checked,
        status="producer_caption_observed_game_effect_unresolved",
    )
    add(
        128,
        [1111],
        "checked_boolean",
        "Play sound when application is in background; direct caption key 0xfc in FUN_00433a00",
        checked,
        status="producer_caption_observed_game_effect_unresolved",
    )
    add(
        132,
        [1106],
        "checked_boolean",
        "Use Windows system font; direct caption key 0x105 in FUN_00435fc0",
        checked,
        status="producer_caption_observed_game_effect_unresolved",
    )
    add(
        132,
        [1117],
        "combo_selection",
        "Font quality option indexes 0=Highest, 1=High, 2=Standard, 3=Low, 4=Lowest; direct option keys 0xac..0xa8 in FUN_00435fc0",
        f"{cb_no_range} in FUN_00435e20",
        status="producer_options_observed_game_effect_unresolved",
    )
    add(
        132,
        [1118],
        "combo_selection",
        "Font thickness option indexes 0=Fine, 1=Normal, 2=Thick, 3=Thickest; direct option keys 0x10b..0x10e in FUN_00435fc0",
        f"{cb_no_range} in FUN_00435e20",
        status="producer_options_observed_game_effect_unresolved",
    )
    add(
        132,
        [1116],
        "selected_utf16_font",
        "Selected font name copied from the font list",
        "A null base or selected unsigned index not below (DAT_0045cbfc-DAT_0045cbf8)/0x1c calls the error dispatcher; a returning dispatcher allows indexing without another check. FUN_0041dad1 destination count is 0x20; the caller ignores its result. The list count is data-dependent",
        status="observed",
    )
    return notes


def _config_sys_metadata() -> dict[str, Any]:
    dword_offsets = [
        0x04,
        0x08,
        0x0C,
        0x10,
        0x14,
        0x18,
        0x1C,
        0x20,
        0x24,
        0x28,
        0x2C,
        0x30,
        0x34,
        0x38,
        0x3C,
        0x40,
        0x44,
        0x48,
        0x4C,
        0x50,
        0x54,
        0x58,
        0x5C,
    ]
    child_field_joins = [
        {
            "dialog_resource": 112,
            "page": "network",
            "handler_va": _hex(0x00435320),
            "handler_rva": _hex(0x35320),
            "controls": [
                {
                    "control_id": 1042,
                    "current_offset": _hex(0x04, 2),
                    "kind": "combo_selection",
                },
                {
                    "control_id": 1044,
                    "current_offset": _hex(0x08, 2),
                    "kind": "numeric_edit_text",
                },
            ],
            "non_field_control_ids": [1048, 1049],
        },
        {
            "dialog_resource": 126,
            "page": "display",
            "handler_va": _hex(0x00434210),
            "handler_rva": _hex(0x34210),
            "controls": [
                {"control_id": 1043, "current_offset": _hex(0x0C, 2)},
                {"control_id": 1099, "current_offset": _hex(0x10, 2)},
                {
                    "control_id": 1100,
                    "current_offsets": _offset_list([0x14, 0x18]),
                },
                {
                    "control_id": 1102,
                    "current_offsets": _offset_list([0x24, 0x28]),
                },
                {"control_id": 1104, "current_offset": _hex(0x1C, 2)},
                {"control_id": 1108, "current_offset": _hex(0x20, 2)},
                {"control_id": 1106, "current_offset": _hex(0x2C, 2)},
            ],
            "table_lookup_helpers": [
                _hex(0x00408D20),
                _hex(0x00408E40),
                _hex(0x00408F40),
            ],
            "non_field_control_ids": [1097, 1098, 1101, 1103, 1105, 1107, 1109],
        },
        {
            "dialog_resource": 127,
            "page": "graphics",
            "handler_va": _hex(0x004354B0),
            "handler_rva": _hex(0x354B0),
            "controls": [
                {"control_id": 1102, "current_offset": _hex(0x30, 2)},
                {"control_id": 1103, "current_offset": _hex(0x34, 2)},
                {"control_id": 1108, "current_offset": _hex(0x38, 2)},
                {"control_id": 1104, "current_offset": _hex(0x3C, 2)},
                {"control_id": 1106, "current_offset": _hex(0x40, 2)},
                {"control_id": 1109, "current_offset": _hex(0x44, 2)},
                {"control_id": 1115, "current_offset": _hex(0x50, 2)},
                {
                    "control_id": 1141,
                    "current_offset": _hex(0xA0, 2),
                    "kind": "utf16_edit_text",
                },
            ],
            "non_field_control_ids": [1110, 1105, 1107, 1140, 1142, 1143],
        },
        {
            "dialog_resource": 128,
            "page": "sound",
            "handler_va": _hex(0x00433AB0),
            "handler_rva": _hex(0x33AB0),
            "controls": [
                {"control_id": 1110, "current_offset": _hex(0x48, 2)},
                {"control_id": 1111, "current_offset": _hex(0x4C, 2)},
            ],
        },
        {
            "dialog_resource": 132,
            "page": "font",
            "handler_va": _hex(0x00435E20),
            "handler_rva": _hex(0x35E20),
            "controls": [
                {"control_id": 1106, "current_offset": _hex(0x54, 2)},
                {"control_id": 1117, "current_offset": _hex(0x58, 2)},
                {"control_id": 1118, "current_offset": _hex(0x5C, 2)},
                {
                    "control_id": 1116,
                    "current_offset": _hex(0x60, 2),
                    "kind": "selected_utf16_font",
                },
            ],
            "non_field_control_ids": [1099, 1100, 1107, 1108],
        },
    ]
    field_notes = _config_sys_field_notes()
    for page in child_field_joins:
        for control in page["controls"]:
            note = field_notes.get((page["dialog_resource"], control["control_id"]))
            if note is None:
                raise ValueError(
                    "config.sys child field has no static meaning/bounds note"
                )
            control.update(note)
    return {
        "child_field_joins": child_field_joins,
        "unjoined_current_dword_offsets": [],
        "current_string_fields": [
            {
                "current_offset": _hex(0x60, 2),
                "capacity_wchars": 0x20,
                "producer": "FUN_00435e20",
                "meaning": "selected font name",
            },
            {
                "current_offset": _hex(0xA0, 2),
                "capacity_wchars": 0x104,
                "producer": "FUN_004354b0",
                "meaning": "directory edit within the Screenshot Directory group; downstream game setting name/effect unresolved",
            },
        ],
        "unobserved_current_ranges": [
            {
                "start": _hex(0x2A8, 2),
                "end_exclusive": _hex(0x2AC, 3),
                "reason": "not touched by focused child capture helpers; save still copies the full current block",
            }
        ],
        "save_cancel_paths": {
            "ok_save": {
                "handler_parameter": 1,
                "page_capture_order": [
                    "network",
                    "display",
                    "graphics",
                    "sound",
                    "font",
                ],
                "global_gate": {
                    "snapshot_offset": _hex(0x08, 2),
                    "accepted_unsigned_min": _hex(0xD800, 4),
                    "accepted_unsigned_max": _hex(0xD8FF, 4),
                    "condition": "FUN_004420B0 rejects the OK path before current/snapshot comparison and save prompt when snapshot +0x08 is outside the inclusive unsigned range",
                    "warning_dialog_resource": 118,
                    "evidence": "FUN_004420B0 VA 0x004420b0 instructions around VA 0x00442212..0x00442224",
                },
                "comparison": "memcmp-style comparison of current VA 0x0045c600 and snapshot VA 0x0045c8b0 over 0x2ac bytes",
                "confirmed_action": "CreateFileW then FUN_00441910 writes the snapshot when the save prompt returns 6",
                "declined_action": "EndDialog result 1 without the write when the save prompt returns 7",
            },
            "cancel_discard": {
                "handler_parameter": 2,
                "page_capture_order": ["network"],
                "comparison": "memcmp-style comparison of current VA 0x0045c600 and snapshot VA 0x0045c8b0 over 0x2ac bytes",
                "prompt_dialog_resource": 117,
                "confirmed_action": "EndDialog result 2 when the discard prompt returns 6",
                "declined_action": "returns without closing when the discard prompt does not return 6",
                "evidence": "FUN_004420B0 VA 0x004420b0 command-2 branch",
            },
            "window_close": {
                "status": "observed",
                "callback": "LAB_00442570",
                "callback_pointer_va": _hex(0x00442570),
                "function_start_unresolved": True,
                "wm_close_branch": "VA 0x00442587 compares WM_CLOSE (10) and branches to VA 0x0044271D",
                "page_capture_order": [
                    "network",
                    "display",
                    "graphics",
                    "sound",
                    "font",
                ],
                "comparison": "memcmp-style comparison of current VA 0x0045c600 and snapshot VA 0x0045c8b0 over 0x2ac bytes",
                "changed_action": "changed state prompts resource 117 at VA 0x0044282B; result 6 at VA 0x00442845 calls EndDialog at VA 0x0044284D without a WriteFile",
                "unchanged_action": "unchanged state closes at VA 0x0044285B..0x00442862",
                "evidence": "LAB_00442570 callback pointer and raw instructions at VA 0x00442587..0x00442862; callback label is retained because the function-start boundary is unresolved",
            },
        },
        "conversion_maps": {
            "20100706": {
                "producer": "FUN_004428D0 instruction sites VA 0x00442A56..0x00442A7A read selector table [7,9,2,0,4,1] from ESP+0x14 and index ESP+0x14+(legacy_value*4); the decompiler's local +1 includes the preceding ReadFile count",
                "legacy_dword_to_current": [
                    {
                        "legacy_offset": _hex(offset, 2),
                        "current_offset": _hex(offset, 2),
                    }
                    for offset in [0x04, 0x08, 0x0C, 0x10, 0x14, 0x18]
                ]
                + [
                    {
                        "legacy_offset": _hex(0x24, 2),
                        "current_offset": _hex(0x1C, 2),
                        "conversion": "selector table [7,9,2,0,4,1] indexed by the legacy dword value",
                    }
                ]
                + [
                    {
                        "legacy_offset": _hex(legacy_offset, 2),
                        "current_offset": _hex(current_offset, 2),
                    }
                    for current_offset, legacy_offset in [
                        (0x24, 0x1C),
                        (0x28, 0x20),
                        (0x2C, 0x28),
                        (0x30, 0x2C),
                        (0x34, 0x30),
                        (0x38, 0x34),
                        (0x3C, 0x38),
                        (0x40, 0x3C),
                        (0x48, 0x40),
                        (0x4C, 0x44),
                        (0x50, 0x48),
                    ]
                ],
                "legacy_string_to_current": [],
                "default_preserved_current_offsets": _offset_list(
                    [0x20, 0x44, 0x54, 0x58, 0x5C]
                ),
            },
            "20101020": {
                "legacy_dword_to_current": [
                    {
                        "legacy_offset": _hex(offset, 2),
                        "current_offset": _hex(offset, 2),
                    }
                    for offset in dword_offsets
                ],
                "legacy_string_to_current": [
                    {"legacy_offset": _hex(0x60, 2), "current_offset": _hex(0x60, 2)}
                ],
            },
            "20120419": {
                "legacy_dword_to_current": [
                    {
                        "legacy_offset": _hex(offset, 2),
                        "current_offset": _hex(offset, 2),
                    }
                    for offset in dword_offsets
                ],
                "legacy_string_to_current": [
                    {"legacy_offset": _hex(0x60, 2), "current_offset": _hex(0x60, 2)},
                    {"legacy_offset": _hex(0xA0, 2), "current_offset": _hex(0xA0, 2)},
                ],
            },
        },
    }


def _dialog_record(
    image: Image,
    row: dict[str, Any],
    map_pairs: list[tuple[int, int]],
    localized_strings: dict[int, dict[int, str]],
) -> dict[str, Any]:
    payload = image.read_rva(int(row["data_rva"], 16), row["size"])
    parsed = parse_dialog(payload)
    dialog_id = row["name"] if row["name_kind"] == "id" else row["name"]
    if not isinstance(dialog_id, int):
        raise ValueError("RT_DIALOG name is not a numeric resource ID")
    parsed.update(
        {
            "resource_id": dialog_id,
            "resource_id_hex": _hex(dialog_id, 4),
            "resource_language": row["language"],
            "resource_locator": {
                "type": "RT_DIALOG",
                "name": dialog_id,
                "language": row["language"],
                "payload_rva": row["data_rva"],
                "payload_offset": row["data_offset"],
                "payload_size": row["size"],
            },
            "payload_sha256": row["payload_sha256"],
            "runtime": _known_runtime(dialog_id),
        }
    )
    dialog_caption_key = DIALOG_CAPTION_KEYS.get(dialog_id)
    if dialog_caption_key is not None:
        parsed["caption_key"] = dialog_caption_key
        parsed["caption_key_hex"] = _hex(dialog_caption_key, 4)
        parsed["localized_titles"] = _localized_texts(
            dialog_caption_key, localized_strings
        )
    for control in parsed["controls"]:
        control["join"] = _control_join(dialog_id, control["id"], map_pairs)
        caption_key = CONTROL_CAPTION_KEYS.get(dialog_id, {}).get(control["id"])
        if caption_key is not None:
            control["caption_key"] = caption_key
            control["caption_key_hex"] = _hex(caption_key, 4)
            control["localized_captions"] = _localized_texts(
                caption_key, localized_strings
            )
        option_keys = CONTROL_OPTION_KEYS.get((dialog_id, control["id"]))
        if option_keys is not None:
            control["option_caption_keys"] = list(option_keys)
            control["localized_options"] = {
                str(language_index): [
                    localized_strings[language_index][key] for key in option_keys
                ]
                for language_index in sorted(localized_strings)
            }
            control["option_source_evidence"] = (
                "Focused page helper pairs these localization keys with CB_INSERTSTRING (0x14a) and explicit indexes on this control; insertion order is preserved"
            )
        if dialog_id == 127 and control["id"] == 1109:
            addresses = (0x0044DB38, 0x0044DB40, 0x0044DB48, 0x0044DB50, 0x0044DB58)
            control["literal_options"] = [
                {
                    "index": index,
                    "text": image.read_rva(va - IMAGE_BASE, 6)
                    .decode("utf-16le")
                    .rstrip("\0"),
                    "va": _hex(va),
                    "rva": _rva(va),
                }
                for index, va in enumerate(addresses)
            ]
            control["option_source_evidence"] = (
                "FUN_004358d0 adds these literal strings to combo 0x455 in this order"
            )
        if caption_key is not None:
            control["caption_source"] = "runtime_localized_lookup_FUN_0040d350"
            control["caption_source_evidence"] = (
                f"FUN_0040d350 direct key {_hex(caption_key, 4)}"
            )
        elif dialog_id == 101 and control["id"] in {
            mapped_id for mapped_id, _next_index in map_pairs
        }:
            control["caption_source"] = "generated_mapping_text_or_literal_dash"
            control["caption_source_evidence"] = (
                "FUN_00430e20 writes generated labels at DAT_00459140 or the "
                "literal '- - -'"
            )
        elif dialog_id == 101 and control["id"] == 1018:
            control["caption_source"] = "device_record_name"
            control["caption_source_evidence"] = (
                "FUN_00431f10 copies the selected device record name from record +0x3cc"
            )
        elif dialog_id == 132 and control["id"] == 1116:
            control["caption_source"] = "enumerated_windows_font_face_names"
            control["caption_source_evidence"] = (
                "FUN_00435fc0 enumerates face names, filters vertical aliases and GetGlyphOutlineW failures, and populates control 0x45c"
            )
        elif dialog_id in (117, 118, 119) and control["id"] == 1087:
            control["caption_source"] = "caller_supplied_prompt_message"
            control["caption_source_evidence"] = (
                "FUN_00432cc0 writes the caller-supplied message string to control 0x43f; no single localization key represents every caller"
            )
        elif dialog_id == 102 and control["id"] in (0x3FD, 0x3FE, 0x3FF):
            control["caption_source"] = "custom_gdi_drawing"
            control["caption_source_evidence"] = (
                "FUN_00430fe0 draws a frame, crosshair and mapped-state point; "
                "no runtime text or localized caption write is established for this visual"
            )
        elif (dialog_id == 101 and control["id"] in {1001, 1019}) or (
            dialog_id == 103 and control["id"] == 1039
        ):
            control["caption_source"] = "other_runtime_text"
            control["caption_source_evidence"] = (
                "focused initializer or visual update path writes runtime text; "
                "no direct caption key is assigned"
            )
        else:
            control["caption_source"] = "unknown_or_static_template"
            control["caption_source_evidence"] = (
                "No focused runtime caption write or direct localization key join"
            )
    return parsed


def extract(path: Path) -> dict[str, Any]:
    image = Image(path)
    rows = image.resources()
    dialogs = [row for row in rows if row["type"] == RT_DIALOG]
    map_values = struct.unpack("<48I", image.read_rva(0x4D6C0, 24 * 8))
    map_pairs = [
        (map_values[index * 2], map_values[index * 2 + 1]) for index in range(24)
    ]
    if any(next_index >= 24 for _control_id, next_index in map_pairs):
        raise ValueError(
            "meaningful controller map table contains an invalid next index"
        )
    binary_rows = [row for row in rows if row["type_name"] == "BINARY"]
    localization_tables = _localization_tables(image, binary_rows)
    localized_strings = {
        table["language_index"]: {
            record["key"]: record["text"] for record in table["records"]
        }
        for table in localization_tables["tables"]
    }
    parsed_dialogs = sorted(
        (_dialog_record(image, row, map_pairs, localized_strings) for row in dialogs),
        key=lambda item: item["resource_id"],
    )
    control_count = sum(item["control_count"] for item in parsed_dialogs)
    return {
        "format": "xivl-ffxivconfig-inventory-v1",
        "inputs": {
            "ffxivconfig.exe": {
                "bytes": CONFIG_SIZE,
                "sha256": CONFIG_SHA256,
                "image_base": _hex(image.base),
            }
        },
        "producer": {
            "script": "tools/extract_ffxivconfig_inventory.py",
            "resource_parser": "stdlib PE32 resource directory and DLGTEMPLATEEX parser",
            "binary_identity_gate": "size and SHA-256 are checked before extraction",
        },
        "coverage": {
            "resource_leaf_count": len(rows),
            "dialog_count": len(parsed_dialogs),
            "control_count": control_count,
            "observed_dynamic_list_view_factories": 4,
            "dialog_languages": sorted(
                {item["resource_language"] for item in parsed_dialogs}
            ),
            "all_dialogs_have_payload_consumption": True,
        },
        "resource_directory": {
            "rva": _hex(image.resource_rva),
            "size": image.resource_size,
            "entries": rows,
            "framework_exclusion": "No RT_DIALOG entry is omitted; zero-control templates and standard Win32 controls remain cataloged. Standard class plumbing has no inferred application setting join unless a code observation below proves one.",
        },
        "localization": {
            "language_file": {
                "name": "config.lng",
                "suffix_va": _hex(0x0044D81C),
                "suffix_rva": _rva(0x0044D81C),
                "read_va": _hex(0x00433660),
                "read_rva": _hex(0x33660),
                "fallback_va": _hex(0x0043FB30),
                "fallback_rva": _hex(0x3FB30),
                "magic": _hex(0x20100706),
                "size": 8,
                "layout": {
                    "magic_offset": 0,
                    "language_index_offset": 4,
                    "language_index_width": 1,
                },
                "accepted_index_range": "Primary 8-byte record index is not bounded; fallback 5-byte candidate accepts 0..5, but startup discards fallback EAX; only five BINARY resource IDs are installed",
                "fallback_result": "Call VA 0x0043383c is followed by loads/stores copying primary globals 0x00459124/0x00459128 to 0x0045912c/0x00459130; candidate outputs stay local to FUN_0043fb30",
                "error_finding": "docs/input/config-file-error-handling.md",
                "save_va": _hex(0x004332A0),
                "save_rva": _hex(0x332A0),
            },
            "binary_resource_loader": {
                "type": "BINARY",
                "type_locator_va": _hex(0x0044B4B0),
                "type_locator_rva": _rva(0x0044B4B0),
                "loader_va": _hex(0x0040E350),
                "loader_rva": _hex(0xE350),
                "lookup_va": _hex(0x0040D350),
                "lookup_rva": _hex(0xD350),
                "resource_id_table_va": _hex(0x00457F04),
                "resource_id_table_rva": _hex(0x57F04),
                "resource_ids_by_index": list(LANGUAGE_RESOURCE_IDS),
                "index_bound": "VA 0x0040e401 loads ushort from table+index*4 without an index guard; index 5 is beyond the five-ID table; later resource/PEX checks do not bound this read",
                "payload_header": "PEX",
                "decode_pipeline": "FUN_0040e350 calls FUN_0040ccc0; FUN_0040ccc0 calls FUN_0040cb20 internally; FUN_0040e350 then calls FUN_0040dd00",
                "dialog_caption_join": "FUN_0040d350 returns a decoded localized string; RT_DIALOG template literals are placeholders where runtime SetWindowTextW is observed",
                "resource_rows": binary_rows,
                "decoded_tables": localization_tables,
            },
        },
        "application_behavior": _application_behavior(image),
        "config_sys": {
            "file": "config.sys",
            "suffix_va": _hex(0x0044D0E8),
            "suffix_rva": _rva(0x0044D0E8),
            "reader_va": _hex(0x004428D0),
            "reader_rva": _hex(0x428D0),
            "dialog_resource": 111,
            "dialog_callback": "LAB_00442570",
            "current_block_va": _hex(0x0045C600),
            "current_block_rva": _rva(0x0045C600),
            "snapshot_block_va": _hex(0x0045C8B0),
            "snapshot_block_rva": _rva(0x0045C8B0),
            "read_buffer_va": _hex(0x0045CC78),
            "read_buffer_rva": _rva(0x0045CC78),
            "current_size": _hex(0x2AC, 3),
            "save_handler_va": _hex(0x004420B0),
            "save_handler_rva": _hex(0x420B0),
            "write_wrapper_va": _hex(0x00441910),
            "write_wrapper_rva": _hex(0x41910),
            "accepted_magics": [_hex(0x20100706), _hex(0x20101020), _hex(0x20120419)],
            "reader_limits": {
                "error_finding": "docs/input/config-file-error-handling.md",
                "file_read": "FUN_004428D0 passes the GetFileSize result directly to ReadFile with destination VA 0x0045cc78; no focused BOOL or returned-byte-count check precedes magic inspection",
                "short_or_oversize_input": "short or failed reads still reach magic inspection; the file-size read request has no capacity guard and the global object's exact capacity is unresolved; actual API bytes and overwrite outcomes are unobserved",
                "legacy_selector": "FUN_004428D0 instruction sites VA 0x00442A56..0x00442A7A index the selector table by the legacy dword value without a focused bounds check",
                "string_copy": "formats 0x20101020 and 0x20120419 use NUL-terminated UTF-16 loops without source-size or destination-capacity comparisons; 0x20100706 copies no strings",
                "validation_scope": "defaults and conversion observations are not a safe-file validation contract",
            },
            "default_dwords": [
                {"offset": _hex(offset, 2), "value": value}
                for offset, value in (
                    (0x00, 0x20120419),
                    (0x04, 0),
                    (0x08, 0xD800),
                    (0x0C, 0),
                    (0x10, 0),
                    (0x14, 0x500),
                    (0x18, 0x2D0),
                    (0x1C, 7),
                    (0x20, 4),
                    (0x24, 0),
                    (0x28, 0),
                    (0x2C, 2),
                    (0x30, 0),
                    (0x34, 0),
                    (0x38, 0),
                    (0x3C, 1),
                    (0x40, 2),
                    (0x44, 0),
                    (0x48, 0),
                    (0x4C, 0),
                    (0x50, 1),
                    (0x54, 0),
                    (0x58, 2),
                    (0x5C, 1),
                )
            ],
            "field_joins": _config_sys_metadata(),
        },
        "controller_mapping_helper": {
            "table_va": _hex(0x0044D6C0),
            "table_rva": _rva(0x0044D6C0),
            "meaningful_pair_count": len(map_pairs),
            "pairs": [
                {"map_index": index, "control_id": control_id, "next_index": next_index}
                for index, (control_id, next_index) in enumerate(map_pairs)
            ],
            "loop_anomaly": "FUN_00430e20, FUN_00430b30 and FUN_004318b0 use 0x30 iterations or adjacent reads although only 24 valid {control_id,next_index} pairs precede the config.pad suffix at VA 0x0044d780; joins beyond pair 23 are unknown and are not invented.",
        },
        "dialogs": parsed_dialogs,
        "limits": "Static resource and instruction observations only. No runtime controls, device availability, localization rendering, config.sys field semantics, or file persistence are claimed beyond the cited code joins.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    destination = parser.add_mutually_exclusive_group(required=True)
    destination.add_argument("--output", type=Path)
    destination.add_argument("--check", type=Path)
    args = parser.parse_args()
    try:
        if args.output is not None:
            ensure_distinct_output(args.binary, args.output)
        document = extract(args.binary)
        if args.check:
            expected = json.loads(args.check.read_text(encoding="ascii"))
            if expected != document:
                raise ValueError("inventory differs from exact-build extraction")
            print("ffxivconfig resource and settings inventory: PASS")
        else:
            args.output.write_text(
                json.dumps(document, indent=2, ensure_ascii=True) + "\n",
                encoding="ascii",
            )
    except (OSError, ValueError, struct.error, json.JSONDecodeError) as error:
        parser.exit(1, f"{error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
