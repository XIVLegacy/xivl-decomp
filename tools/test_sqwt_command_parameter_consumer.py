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


def event_release_tests(real: dict) -> None:
    """Exercise changed release facts through literal bytes, not manifest agreement."""
    selected = {
        "event listener slot 2 active clear": TEXT_VA,
        "event replay guards and flag clears": TEXT_VA + 0x10,
        "event desktop byte gate and pump caller": TEXT_VA + 0x30,
        "event listener slot 2 word pointer": TEXT_VA + 0x50,
        "event priority compare": TEXT_VA + 0x60,
        "event listener vtable": DATA_VA,
        "event thread static zero fill": DATA_VA + 0x20,
    }
    text = bytearray(0x80)
    text[0:7] = bytes.fromhex("C7 46 04 00 00 00 00")
    text[0x10:0x25] = bytes.fromhex(
        "38 5E 1C 74 53 38 5E 1D 74 4E 39 5E 08 88 5E 1C 88 5E 1D 74 43"
    )
    text[0x30:0x3F] = bytes.fromhex("80 7E 64 00 74 09 8B 4E 60 56 E8 A9 50 1B 00")
    text[0x50:0x57] = bytes.fromhex("8B 40 08 8D 55 14 52")
    text[0x60:0x78] = bytes.fromhex(
        "3B FB 0F 84 81 00 00 00 8A 81 00 01 00 00 3A 87 00 01 00 00 77 07 32 C0"
    )
    table = struct.pack("<4I", 0x0089A2B0, 0x00894F30, 0x00895010, 0x00892760)
    image = build_image(bytes(text), table, 0x30)
    rows = []
    for name, va in selected.items():
        row = copy.deepcopy(
            next(item for item in real["staticChecks"] if item.get("name") == name)
        )
        row["va"] = f"{va:#010x}"
        rows.append(row)
    document = {"staticChecks": rows}
    check("literal event release fixture passes", run(image, document) == [])
    for label, offset, value, expected_name in (
        ("active clear targets +4", 2, 8, "event listener slot 2 active clear"),
        (
            "replay needs the second flag",
            0x17,
            0x1C,
            "event replay guards and flag clears",
        ),
        (
            "replay needs a pending pointer",
            0x1C,
            4,
            "event replay guards and flag clears",
        ),
        (
            "desktop gate has byte width",
            0x30,
            0x83,
            "event desktop byte gate and pump caller",
        ),
        (
            "callback passes thread-word address",
            0x55,
            0x18,
            "event listener slot 2 word pointer",
        ),
        ("priority uses unsigned above", 0x74, 0x76, "event priority compare"),
    ):
        changed = bytearray(image)
        changed[0x200 + offset] = value
        errors = run(bytes(changed), document)
        check(label, len(errors) == 1 and expected_name in errors[0])
    changed_table = bytearray(image)
    struct.pack_into("<I", changed_table, 0x200 + 0x80 + 4, 0x00895010)
    errors = run(bytes(changed_table), document)
    check(
        "listener slot order is enforced",
        len(errors) == 1 and "event listener vtable" in errors[0],
    )
    nonzero = copy.deepcopy(document)
    nonzero["staticChecks"][-1]["values"] = ["0x00000001"]
    errors = run(image, nonzero)
    check(
        "static thread word reads zero fill",
        len(errors) == 1 and "event thread static zero fill" in errors[0],
    )

    changed = copy.deepcopy(real)
    changed["scriptInterface"]["eventQueue"]["listenerPhases"][1]["fourthArgument"] = (
        "16-bit value"
    )
    check(
        "callback pointer ABI drift is rejected",
        "event listener phases or callback ABI changed"
        in verifier.event_release_errors(changed),
    )
    for field, value, expected_error in (
        ("pointerWidthBits", 64, "event queue pointerWidthBits changed"),
        (
            "callbackThreadWordWidthBits",
            32,
            "event queue callbackThreadWordWidthBits changed",
        ),
        (
            "eventThreadWordStaticImage",
            "immutable zero",
            "event thread static-image boundary changed",
        ),
    ):
        changed = copy.deepcopy(real)
        changed["scriptInterface"]["eventQueue"][field] = value
        check(
            field + " drift is rejected",
            expected_error in verifier.event_release_errors(changed),
        )
    changed = copy.deepcopy(real)
    changed["scriptInterface"]["eventQueue"]["replayGuards"].pop(
        "pendingNonNullCompareVa"
    )
    check(
        "pending-null guard cannot disappear",
        "event replay flag or pending-pointer guards changed"
        in verifier.event_release_errors(changed),
    )
    for name in (
        "event listener slot 1 active clear",
        "event replay guards and flag clears",
        "event listener vtable",
    ):
        changed = copy.deepcopy(real)
        changed["staticChecks"] = [
            item for item in changed["staticChecks"] if item.get("name") != name
        ]
        check(
            name + " cannot disappear",
            f"event release static check changed: {name}"
            in verifier.event_release_errors(changed),
        )
    changed = copy.deepcopy(real)
    row = next(
        item
        for item in changed["staticChecks"]
        if item.get("name") == "event replay guards and flag clears"
    )
    row["mask"] = " ".join(["00"] * 21)
    check(
        "replay guard mask cannot be disabled",
        "event release static check changed: event replay guards and flag clears"
        in verifier.event_release_errors(changed),
    )
    page = verifier.DOC.read_text(encoding="utf-8").lower()
    page = page.replace("0x00894f9e", "0x00894f9f")
    check(
        "page must cite the active clear",
        "page does not cite 0x00894f9e" in verifier.doc_errors(real, page),
    )


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

    event_release_tests(real)

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(PASSED) + len(FAILED)} checks")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} command-parameter consumer checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
