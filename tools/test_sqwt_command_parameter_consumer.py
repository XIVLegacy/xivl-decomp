#!/usr/bin/env python3
"""Tests for the Sqwt command-parameter consumer verifier."""

from __future__ import annotations

import copy
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_sqwt_command_parameter_consumer as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
TEXT_VA = 0x00401000
DATA_VA = 0x00402000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def build_image(text: bytes, data: bytes, data_virtual_size: int) -> bytes:
    """A two-section PE32 image: raw .text at 0x401000, .data at 0x402000."""
    header = bytearray(0x200)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, 2)
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    table = 0x80 + 24 + 0xE0
    for index, (name, virtual, raw_size, raw_offset, virtual_address) in enumerate(
        (
            (b".text", len(text), len(text), 0x200, 0x1000),
            (b".data", data_virtual_size, len(data), 0x200 + len(text), 0x2000),
        )
    ):
        base = table + 40 * index
        header[base : base + len(name)] = name
        struct.pack_into(
            "<IIII", header, base + 8, virtual, virtual_address, raw_size, raw_offset
        )
    return bytes(header) + text + data


def fixture() -> tuple[bytes, dict]:
    text = bytes.fromhex("8B 47 24 83 F8 08 0F 87 E0 00 00 00")
    text += bytes.fromhex(
        "68 1F 20 40 00 51 8B C4 51 C6 00 2A 8B C4 B9 38 87 35 01 C6 00 00 E8 00 00 00 00"
    )
    text += bytes.fromhex(
        "6A 00 51 8B C4 51 C6 00 59 8B C4 B9 84 6A 33 01 C6 00 00 E8 00 00 00 00 90"
    )
    table = struct.pack("<3I", 0x401000, 0x401004, 0x401008)
    data = table + b"_onUICommandEvent\0" + b"\x01" + b"Executed\0"
    image = build_image(text, data, len(data) + 0x20)
    document = {
        "staticChecks": [
            {
                "kind": "signature",
                "name": "guard",
                "va": "0x00401000",
                "pattern": "8B 47 24 83 F8 08 0F 87 E0 00 00 00",
                "mask": "FF FF FF FF FF FF FF FF 00 00 00 00",
            },
            {
                "kind": "dwords",
                "name": "table",
                "va": "0x00402000",
                "values": ["0x00401000", "0x00401004", "0x00401008"],
            },
            {"kind": "string", "va": "0x0040200c", "text": "_onUICommandEvent"},
            {
                "kind": "routedEvent",
                "name": "named",
                "va": "0x0040100c",
                "id": "0x2a",
                "strategy": 0,
                "static": "0x01358738",
                "eventName": "Executed",
            },
            {
                "kind": "routedEvent",
                "name": "unnamed",
                "va": "0x00401027",
                "id": "0x59",
                "strategy": 0,
                "static": "0x01336a84",
                "eventName": None,
            },
            {
                "kind": "dwords",
                "name": "flag byte",
                "va": "0x0040201e",
                "width": 1,
                "values": ["0x01"],
            },
            {
                "kind": "dwords",
                "name": "virtual-only byte",
                "va": "0x00402030",
                "width": 1,
                "values": ["0x00"],
            },
        ]
    }
    return image, document


def run(image: bytes, document: dict) -> list[str]:
    return verifier.verify_image(image, document, require_identity=False)


def main() -> int:
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])

    image, document = fixture()
    check("matching fixture passes", run(image, document) == [])

    wildcard = bytearray(image)
    wildcard[0x200 + 8] = 0x11
    check("masked byte is ignored", run(bytes(wildcard), document) == [])

    stable = bytearray(image)
    stable[0x200 + 5] = 0x07
    check("stable signature byte is enforced", len(run(bytes(stable), document)) == 1)

    table = bytearray(image)
    table[0x200 + 12 + 4] ^= 1
    check("jump table entry is enforced", len(run(bytes(table), document)) == 1)

    text = bytearray(image)
    text_size = 12
    text[0x200 + text_size + 12 + 3] = ord("X")
    check("string text is enforced", len(run(bytes(text), document)) == 1)

    outside = copy.deepcopy(document)
    outside["staticChecks"][0]["va"] = "0x00500000"
    check("address outside the image is reported", len(run(image, outside)) == 1)

    check("virtual-only bytes read as zero", run(image, document) == [])

    text_base = 0x200
    wrong_id = bytearray(image)
    wrong_id[text_base + 12 + 11] = 0x2B
    check("named initializer id is enforced", len(run(bytes(wrong_id), document)) == 1)

    wrong_strategy = bytearray(image)
    wrong_strategy[text_base + 12 + 21] = 0x02
    check(
        "named initializer strategy is enforced",
        len(run(bytes(wrong_strategy), document)) == 1,
    )

    wrong_static = bytearray(image)
    wrong_static[text_base + 12 + 15] = 0x39
    check(
        "named initializer static is enforced",
        len(run(bytes(wrong_static), document)) == 1,
    )

    renamed = bytearray(image)
    renamed[text_base + 12 + 27 + 25 + 31] = ord("X")
    check("initializer name is enforced", len(run(bytes(renamed), document)) == 1)

    unnamed_id = bytearray(image)
    unnamed_id[text_base + 12 + 27 + 8] = 0x58
    check(
        "unnamed initializer id is enforced", len(run(bytes(unnamed_id), document)) == 1
    )

    broken = bytearray(image)
    broken[text_base + 12 + 27] = 0x90
    check("non initializer bytes are reported", len(run(bytes(broken), document)) == 1)

    nonzero = copy.deepcopy(document)
    nonzero["staticChecks"][-1]["values"] = ["0x02"]
    check("virtual-only byte value is enforced", len(run(image, nonzero)) == 1)

    check("identity is enforced", len(verifier.verify_image(image, document)) >= 2)
    check(
        "non-PE input is reported",
        len(run(b"\x00" * 0x400, document)) == 1,
    )

    real = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    kinds = {item["kind"] for item in real["staticChecks"]}
    check(
        "manifest carries all check kinds",
        kinds == {"signature", "dwords", "string", "routedEvent"},
    )

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(PASSED) + len(FAILED)} checks")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} command-parameter consumer checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
