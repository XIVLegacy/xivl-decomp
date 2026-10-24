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


def button_source_tests(real: dict) -> None:
    """Exercise byte gates, pointer ABI and RTTI with independently written literals."""
    text = bytearray(0xA0)
    text[0:28] = bytes.fromhex(
        "8A 9D FA 02 00 00 57 C0 EB 05 8D BD 4C FF FF FF 8D B7 AE 03 00 00 80 E3 01 F6 06 10"
    )
    text[0x20:0x2E] = bytes.fromhex("80 BD F8 02 00 00 02 75 11 80 FB 01 75 0C")
    text[0x40:0x56] = bytes.fromhex(
        "8B 8F BC 02 00 00 8B 11 8B 52 08 55 50 8B 87 CC 02 00 00 50 FF D2"
    )
    text[0x60:0x75] = bytes.fromhex(
        "8B 46 10 80 38 00 0F 85 88 00 00 00 F6 85 A6 00 00 00 20 74 7F"
    )
    text[0x80:0x90] = bytes.fromhex("C7 06 AC 3C 07 01 C7 86 B4 00 00 00 84 3B 07 01")
    text[0x90:0x9F] = bytes.fromhex("89 8F FC 02 00 00 8B CE C6 44 24 14 00 FF D0")
    data = (
        struct.pack(
            "<7I",
            0x0117D4D0,
            0xB4,
            0x0126D244,
            0x0097EFF0,
            0x0097F080,
            0x0097EDB0,
            0x00927FA0,
        )
        + b".?AVButton@Controls@Sqwt@@\0"
    )
    image = build_image(bytes(text), data, len(data))
    selected = {
        "button up snapshot before refresh": TEXT_VA,
        "button up mode and snapshot gates": TEXT_VA + 0x20,
        "button stored command slot ABI": TEXT_VA + 0x40,
        "button up handled and eligibility bytes": TEXT_VA + 0x60,
        "button constructor tables": TEXT_VA + 0x80,
        "button temporary flag pointer": TEXT_VA + 0x90,
        "button interface RTTI locator": DATA_VA,
        "button interface RTTI offset": DATA_VA + 4,
        "button interface RTTI type": DATA_VA + 8,
        "button mouse slots": DATA_VA + 0x0C,
        "button Executed slot": DATA_VA + 0x14,
        "button RoutedCommand slot": DATA_VA + 0x18,
        "button RTTI name": DATA_VA + 0x1C,
    }
    rows = []
    for name, va in selected.items():
        row = copy.deepcopy(
            next(item for item in real["staticChecks"] if item.get("name") == name)
        )
        row["va"] = f"{va:#010x}"
        rows.append(row)
    document = {"staticChecks": rows}
    check("literal Button fixture passes", run(image, document) == [])
    for label, offset, value, expected_name in (
        (
            "snapshot reads the packed byte",
            2,
            0xFC,
            "button up snapshot before refresh",
        ),
        ("snapshot uses bit five", 9, 4, "button up snapshot before refresh"),
        ("release mode is two", 0x26, 1, "button up mode and snapshot gates"),
        (
            "release requires the old snapshot",
            0x2B,
            0,
            "button up mode and snapshot gates",
        ),
        ("command uses slot two", 0x4A, 0x0C, "button stored command slot ABI"),
        (
            "record comes from the pointer holder",
            0x4F,
            0xA4,
            "button stored command slot ABI",
        ),
        (
            "event pointer is the last stack argument",
            0x4B,
            0x50,
            "button stored command slot ABI",
        ),
        (
            "eligibility is byte width",
            0x6C,
            0xF7,
            "button up handled and eligibility bytes",
        ),
        ("Button interface is at b4", 0x88, 0xB8, "button constructor tables"),
        (
            "temporary field stores a dword pointer",
            0x90,
            0x88,
            "button temporary flag pointer",
        ),
    ):
        changed = bytearray(image)
        changed[0x200 + offset] = value
        errors = run(bytes(changed), document)
        check(label, len(errors) == 1 and expected_name in errors[0])
    for label, offset, value, expected_name in (
        ("RTTI locator is preserved", 0, 0x0117D44C, "button interface RTTI locator"),
        ("RTTI offset is b4", 4, 0, "button interface RTTI offset"),
        (
            "RTTI type is the concrete Button",
            8,
            0x0126D28C,
            "button interface RTTI type",
        ),
        ("Down and Up slots are ordered", 0x0C, 0x0097F080, "button mouse slots"),
        ("Executed selects the Button body", 0x14, 0x0091D8F0, "button Executed slot"),
        (
            "RoutedCommand slot reaches the join",
            0x18,
            0x00928C90,
            "button RoutedCommand slot",
        ),
    ):
        changed = bytearray(image)
        struct.pack_into("<I", changed, 0x200 + 0xA0 + offset, value)
        errors = run(bytes(changed), document)
        check(label, len(errors) == 1 and expected_name in errors[0])
    changed = bytearray(image)
    changed[0x200 + 0xA0 + 0x1C + 4] = ord("X")
    errors = run(bytes(changed), document)
    check(
        "Button RTTI spelling is enforced", len(errors) == 1 and "string" in errors[0]
    )

    for section, field, value in (
        ("mouseGates", "snapshotBeforeRefresh", False),
        ("mouseGates", "snapshotAfterBaseCall", False),
        ("mouseGates", "handledWidthBits", 32),
        ("mouseGates", "downValueMustBeSet", True),
        ("mouseGates", "upAndMask", "0xff"),
        ("storedCommand", "pointerWidthBits", 64),
        ("storedCommand", "recordPointerOffset", "+0x3a4"),
        (
            "storedCommand",
            "slotArguments",
            [
                "stored record pointer",
                "selected target interface or None",
                "Click args pointer",
            ],
        ),
        ("storedCommand", "slotArguments", ["Click args pointer", "target", "record"]),
        ("click", "temporaryPointerWidthBits", 8),
        ("click", "defaultCommandBeforeGeneric", False),
        ("click", "triggerAttachment", "STATIC installed"),
        ("executedForwarding", "frameworkElementCallVa", "0x0097ee90"),
    ):
        changed = copy.deepcopy(real)
        changed["buttonSource"][section][field] = value
        errors = verifier.button_errors(changed)
        check(
            field + " Button drift is rejected",
            f"button {section} contract changed" in errors,
        )
    changed = copy.deepcopy(real)
    changed["buttonSource"]["mouseIngress"][1]["routingStrategy"] = 0
    check(
        "preview remains strategy two",
        "button preview ingress changed" in verifier.button_errors(changed),
    )
    changed = copy.deepcopy(real)
    changed["buttonSource"]["storedCommand"]["targetFallback"]["selected"] = (
        "returned pointer"
    )
    check(
        "fallback uses I instead of returned pointer",
        "button storedCommand contract changed" in verifier.button_errors(changed),
    )
    changed = copy.deepcopy(real)
    changed["buttonSource"]["storedCommand"]["targetFallback"]["elseTarget"] = "null"
    check(
        "null fallback remains a value rather than text",
        "button storedCommand contract changed" in verifier.button_errors(changed),
    )
    changed = copy.deepcopy(real)
    changed["buttonSource"]["unknowns"].pop()
    check(
        "runtime boundary cannot disappear",
        "button unknown boundaries changed" in verifier.button_errors(changed),
    )
    for name in (
        "button stored command slot ABI",
        "button interface RTTI type",
        "button Executed FrameworkElement call",
    ):
        changed = copy.deepcopy(real)
        changed["staticChecks"] = [
            row for row in changed["staticChecks"] if row.get("name") != name
        ]
        check(
            name + " cannot disappear",
            f"button static check changed: {name}" in verifier.button_errors(changed),
        )
    changed = copy.deepcopy(real)
    row = next(
        row
        for row in changed["staticChecks"]
        if row.get("name") == "button up snapshot before refresh"
    )
    row["mask"] = " ".join(["00"] * 28)
    check(
        "Button snapshot mask cannot be disabled",
        "button static check changed: button up snapshot before refresh"
        in verifier.button_errors(changed),
    )
    for name, field, value in (
        ("PreviewMouseLeftButtonDown", "strategy", 0),
        ("PreviewMouseLeftButtonUp", "static", "0x01359674"),
        ("Click", "id", "0x37"),
    ):
        changed = copy.deepcopy(real)
        row = next(
            row for row in changed["staticChecks"] if row.get("eventName") == name
        )
        row[field] = value
        check(
            name + " descriptor drift is rejected",
            f"routed event {name} changed" in verifier.routing_errors(changed),
        )
    page = (
        verifier.DOC.read_text(encoding="utf-8")
        .lower()
        .replace("0x013593cc", "0x013593cd")
    )
    check(
        "page retains the Up descriptor",
        "page does not cite 0x013593cc" in verifier.doc_errors(real, page),
    )


def eligibility_tests(real: dict) -> None:
    """Reject changed context, map, operand-width and publication observations."""
    text = bytearray(0xE0)
    text[0:17] = bytes.fromhex("8B 11 89 50 04 C6 44 24 10 01 8B 4C 24 1C 89 48 08")
    text[0x20:0x33] = bytes.fromhex(
        "8B 56 08 50 51 52 83 C6 04 56 8D 4C 24 17 E8 68 6A 14 00"
    )
    text[0x40:0x4D] = bytes.fromhex("84 C0 74 09 80 BE 61 01 00 00 00 75 38")
    text[0x60:0x64] = bytes.fromhex("0F B6 46 06")
    text[0x70:0x81] = bytes.fromhex(
        "8D 4F 14 89 44 24 54 E8 18 1B 03 00 8B 57 64 89 10"
    )
    text[0x90:0xAE] = bytes.fromhex(
        "0F BE 86 62 01 00 00 50 8D 4C 24 34 E8 C0 5D 5D 00 C6 44 24 28 03 8B 08 89 8E 28 01 00 00"
    )
    text[0xB0:0xBB] = bytes.fromhex("8B C1 8B 4C 24 04 89 08 C2 04 00")
    text[0xC0:0xC6] = bytes.fromhex("8A 86 BA 00 00 00")
    text[0xD0:0xDC] = bytes.fromhex("8A 8E BA 00 00 00 88 8F BA 00 00 00")
    data = (
        struct.pack("<4I", 0xE0000000, 0x006E3440, 0x0089B3F0, 0x012C19A4)
        + b".?AVMyPlayer@Control@Client@Script@Lua@Application@@\0"
    )
    image = build_image(bytes(text), data, len(data))
    selected = {
        "eligibility actor context and control stores": TEXT_VA,
        "eligibility root context argument": TEXT_VA + 0x20,
        "eligibility conditional player byte": TEXT_VA + 0x40,
        "eligibility descriptor key zero extension": TEXT_VA + 0x60,
        "eligibility staged map store": TEXT_VA + 0x70,
        "eligibility signed byte to dword copy": TEXT_VA + 0x90,
        "eligibility dword wrapper copy": TEXT_VA + 0xB0,
        "eligibility first entry byte return": TEXT_VA + 0xC0,
        "eligibility area byte copy": TEXT_VA + 0xD0,
        "eligibility sentinel dword": DATA_VA,
        "eligibility player publication slot": DATA_VA + 4,
        "eligibility item command slot": DATA_VA + 8,
        "eligibility myplayer RTTI type": DATA_VA + 0x0C,
        "eligibility myplayer RTTI name": DATA_VA + 0x10,
    }
    rows = []
    for name, va in selected.items():
        row = copy.deepcopy(
            next(item for item in real["staticChecks"] if item.get("name") == name)
        )
        row["va"] = f"{va:#010x}"
        rows.append(row)
    document = {"staticChecks": rows}
    check("literal eligibility fixture passes", run(image, document) == [])
    for label, offset, value, expected_name in (
        (
            "actor stores context at +4",
            4,
            8,
            "eligibility actor context and control stores",
        ),
        (
            "actor control remains at +8",
            16,
            12,
            "eligibility actor context and control stores",
        ),
        (
            "thunk passes its context wrapper",
            0x28,
            8,
            "eligibility root context argument",
        ),
        (
            "zero area byte skips the player byte",
            0x42,
            0x75,
            "eligibility conditional player byte",
        ),
        (
            "player predicate is byte width",
            0x44,
            0x83,
            "eligibility conditional player byte",
        ),
        (
            "key source is zero extended",
            0x61,
            0xBE,
            "eligibility descriptor key zero extension",
        ),
        (
            "key comes from descriptor +6",
            0x63,
            7,
            "eligibility descriptor key zero extension",
        ),
        ("writer uses staged map +14", 0x72, 8, "eligibility staged map store"),
        (
            "state copy sign extends",
            0x91,
            0xB6,
            "eligibility signed byte to dword copy",
        ),
        ("state copy reads +162", 0x93, 0x61, "eligibility signed byte to dword copy"),
        ("state copy writes +128", 0xAA, 0x2C, "eligibility signed byte to dword copy"),
        ("wrapper copies a dword", 0xB6, 0x88, "eligibility dword wrapper copy"),
        (
            "predicate reads entry +ba",
            0xC2,
            0xB8,
            "eligibility first entry byte return",
        ),
        ("registration copies to area +ba", 0xD8, 0xB9, "eligibility area byte copy"),
    ):
        changed = bytearray(image)
        changed[0x200 + offset] = value
        errors = run(bytes(changed), document)
        check(label, len(errors) == 1 and expected_name in errors[0])
    for offset, value, expected_name in (
        (0, 0, "eligibility sentinel dword"),
        (4, 0x006E3920, "eligibility player publication slot"),
        (8, 0x007036E0, "eligibility item command slot"),
        (0x0C, 0x012C1964, "eligibility myplayer RTTI type"),
    ):
        changed = bytearray(image)
        struct.pack_into("<I", changed, 0x200 + len(text) + offset, value)
        errors = run(bytes(changed), document)
        check(
            expected_name + " value is enforced",
            len(errors) == 1 and expected_name in errors[0],
        )
    changed = bytearray(image)
    changed[0x200 + len(text) + 0x10 + 4] = ord("X")
    errors = run(bytes(changed), document)
    check(
        "eligibility MyPlayer RTTI spelling is enforced",
        len(errors) == 1 and "string" in errors[0],
    )

    for section, field, value in (
        ("context", "passedWrapper", "&selectedWidget"),
        ("context", "playerOffset", "+0x10"),
        ("context", "runtimeContextIdentity", "STATIC singleton"),
        ("keyPublication", "mainMapOffset", "+0x14"),
        ("keyPublication", "stagedMapOffset", "+0x8"),
        ("keyPublication", "descriptorKeyWidthBits", 32),
        ("keyPublication", "sameRuntimePlayer", "STATIC"),
        ("keyPublication", "gateTestsTransferredByte", True),
        ("stateFields", "copySource", "+0x161"),
        ("stateFields", "copyExtension", "zero"),
        ("stateFields", "copyWrapperVa", "0x0075c420"),
        ("stateFields", "meaning", "target id"),
        ("areaFlag", "selectedWidgetByteRead", True),
        ("areaFlag", "runtimeFirstEntry", "STATIC"),
    ):
        changed = copy.deepcopy(real)
        changed["selectorGates"][section][field] = value
        check(
            field + " eligibility drift is rejected",
            f"eligibility {section} contract changed"
            in verifier.eligibility_errors(changed),
        )
    changed = copy.deepcopy(real)
    changed["selectorGates"]["gates"][3]["playerByteCheckedWhenAreaByteZero"] = True
    check(
        "conditional player byte cannot become unconditional",
        "eligibility gates contract changed" in verifier.eligibility_errors(changed),
    )
    for field in ("unknowns", "exclusions"):
        changed = copy.deepcopy(real)
        changed["selectorGates"][field].pop()
        check(
            "eligibility " + field + " cannot disappear",
            f"eligibility {field} contract changed"
            in verifier.eligibility_errors(changed),
        )
    for name in (
        "eligibility root context argument",
        "eligibility staged map store",
        "eligibility signed byte to dword copy",
        "eligibility myplayer RTTI type",
    ):
        changed = copy.deepcopy(real)
        changed["staticChecks"] = [
            row for row in changed["staticChecks"] if row.get("name") != name
        ]
        check(
            name + " cannot disappear",
            f"eligibility static check changed: {name}"
            in verifier.eligibility_errors(changed),
        )
    changed = copy.deepcopy(real)
    row = next(
        row
        for row in changed["staticChecks"]
        if row.get("name") == "eligibility signed byte to dword copy"
    )
    row["mask"] = " ".join(["00"] * 30)
    check(
        "signed-copy mask cannot be disabled",
        "eligibility static check changed: eligibility signed byte to dword copy"
        in verifier.eligibility_errors(changed),
    )

    page = verifier.DOC.read_text(encoding="utf-8")
    for claim in (
        "The gates read as liveness checks.",
        "A script object whose owner was released.",
        "This is for widget script objects and not for actor-bound ones.",
        "The script object has an owner and the widget is live.",
    ):
        changed_page = page.replace("## Script entry", "## Script entry\n\n" + claim, 1)
        check(
            "unsupported owner inference is rejected: " + claim,
            "unsupported widget eligibility inference"
            in verifier.doc_errors(real, changed_page),
        )
    changed_page = page.lower().replace("0x006f354f", "0x006f354e")
    check(
        "page cites the signed byte source",
        "page does not cite 0x006f354f" in verifier.doc_errors(real, changed_page),
    )
    changed_page = page.replace("UNKNOWN", "STATIC")
    check(
        "page retains eligibility uncertainty",
        "eligibility uncertainty is missing" in verifier.doc_errors(real, changed_page),
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
    button_source_tests(real)
    eligibility_tests(real)

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(PASSED) + len(FAILED)} checks")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} command-parameter consumer checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
