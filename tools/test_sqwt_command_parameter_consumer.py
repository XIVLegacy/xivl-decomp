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


def upstream_slot4_tests(real: dict) -> None:
    """Corrupt independent literal call, gate, stack and secondary-COL fixtures."""
    selected = [
        ("Rapture receiver field and call", 0x00),
        ("adapter adjusted tail JMP", 0x10),
        ("record dispatch byte gates", 0x20),
        ("record dispatch receiver and arguments", 0x30),
        ("walk lookup not found branch", 0x50),
        ("walk before helper calls", 0x60),
        ("numeric conditional join", 0x80),
        ("numeric literal and dispatch", 0xA0),
        ("record constructor disabled byte", 0xB0),
        ("application unsigned range", 0xC0),
    ]
    text = bytearray(0xE0)
    literals = [
        (0x00, "8B 4E 64 E8 37 6C 02 00"),
        (0x10, "8D 4F 10 5F 5E 5D 83 C4 08 E9 3A FB FF FF"),
        (0x20, "80 79 7E 00 75 19 80 B9 80 00 00 00 00 74 10"),
        (0x30, "8B 54 24 04 8D 41 70 8B 09 50 52 E8 A1 A5 FE FF"),
        (0x50, "8B 4C 24 28 39 4B 04 75 17"),
        (0x60, "8B 0E 57 E8 85 0A 01 00 8B F8 8B 06 8B 88 C0 01 00 00 E8 96 82 02 00"),
        (0x80, "8B 54 24 18 3B 15 D0 CE 30 01 8B CF 75 07 E8 A5 C4 FD FF"),
        (0xA0, "6A 01 8B CE E8 F1 F6 FF FF"),
        (0xB0, "88 5E 7E 88 9E 80 00 00 00"),
        (0xC0, "83 C0 FC 83 F8 02 77 06 88 5C 24 17 EB 55"),
    ]
    for offset, literal in literals:
        raw = bytes.fromhex(literal)
        text[offset : offset + len(raw)] = raw
    data = struct.pack("<3I", 4, 0, 0x012695C0)
    image = build_image(bytes(text), data, len(data))
    checks = []
    for name, offset in selected:
        row = copy.deepcopy(
            next(
                item
                for item in real["staticChecks"]
                if item.get("name") == "upstream " + name
            )
        )
        row["va"] = f"{TEXT_VA + offset:#010x}"
        checks.append(row)
    col = copy.deepcopy(
        next(
            item
            for item in real["staticChecks"]
            if item.get("name") == "upstream Rapture COL coordinates and type"
        )
    )
    col["va"] = f"{DATA_VA:#010x}"
    checks.append(col)
    fixture_document = {"staticChecks": checks}
    check("literal upstream fixture passes", run(image, fixture_document) == [])
    for label, offset, value, name in (
        ("secondary receiver field stays relative to S", 0x02, 0x68, selected[0][0]),
        ("adapter preserves tail JMP", 0x19, 0xE8, selected[1][0]),
        ("record first byte gate remains zero", 0x23, 1, selected[2][0]),
        ("record enable branch remains nonzero", 0x2D, 0x75, selected[2][0]),
        ("record uses its control receiver", 0x38, 0x08, selected[3][0]),
        ("wrapper is virtual argument one", 0x39, 0x52, selected[3][0]),
        ("lookup not found branch is retained", 0x57, 0x74, selected[4][0]),
        ("walk target precedes later helper", 0x64, 0x96, selected[5][0]),
        ("numeric join remains conditional", 0x8C, 0x74, selected[6][0]),
        ("numeric literal remains one", 0xA1, 2, selected[7][0]),
        ("constructor clears record enable byte", 0xB5, 0x81, selected[8][0]),
        ("range branch remains unsigned above", 0xC6, 0x7F, selected[9][0]),
    ):
        changed = bytearray(image)
        changed[0x200 + offset] = value
        errors = run(bytes(changed), fixture_document)
        check(label, len(errors) == 1 and "upstream " + name in errors[0])
    changed = bytearray(image)
    struct.pack_into("<I", changed, 0x200 + len(text), 0)
    errors = run(bytes(changed), fixture_document)
    check(
        "nonzero secondary COL offset is enforced",
        len(errors) == 1 and "upstream Rapture COL coordinates and type" in errors[0],
    )

    route_path = ("scriptInterface", "eventQueue", "upstreamSlot4")
    mutations = [
        (("raptureReceiver", "fieldOffsetFromCompleteObject"), 100),
        (("recordWalk", "dispatchPredicate"), "lookup found"),
        (("recordDispatch", "arguments"), ["record+0x70", "engine-wrapper pointer"]),
        (("recordDispatch", "constructorEnablesDispatch"), True),
        (("localOrder", "allNotificationsOrdered"), True),
        (("numeric1", "everyPass"), True),
        (("unknowns",), ["fixed cadence is STATIC"]),
    ]
    for path, value in mutations:
        changed = copy.deepcopy(real)
        target = changed
        for key in route_path + path[:-1]:
            target = target[key]
        target[path[-1]] = value
        check(
            "upstream semantic drift rejected: " + ".".join(path),
            verifier.upstream_slot4_errors(changed) != [],
        )
    changed = copy.deepcopy(real)
    receiver = changed["scriptInterface"]["eventQueue"]["upstreamSlot4"][
        "raptureReceiver"
    ]
    receiver["completeObjectOffset"] = 0
    receiver["fieldOffsetFromCompleteObject"] = 100
    check(
        "coordinated primary-receiver fiction is rejected",
        "upstream slot4 raptureReceiver changed"
        in verifier.upstream_slot4_errors(changed),
    )
    for name in (
        "upstream adapter adjusted tail JMP",
        "upstream Rapture COL coordinates and type",
    ):
        changed = copy.deepcopy(real)
        row = next(item for item in changed["staticChecks"] if item.get("name") == name)
        if row["kind"] == "signature":
            row["mask"] = " ".join(["00"] * len(bytes.fromhex(row["pattern"])))
        else:
            changed["staticChecks"].remove(row)
        check(
            "upstream fixed anchor cannot be disabled: " + name,
            f"upstream slot4 static check changed: {name}"
            in verifier.upstream_slot4_errors(changed),
        )
    page = verifier.DOC.read_text(encoding="ascii")
    changed_page = page.replace("complete Rapture +0x68", "complete Rapture +0x64")
    check(
        "page preserves complete Rapture coordinate",
        "upstream slot4 page boundary missing: complete rapture +0x68"
        in verifier.doc_errors(real, changed_page),
    )
    changed_page = page.replace("0x00CDA437", "0x00CDA438")
    check(
        "page cites lookup-not-found comparison anchor",
        "page does not cite 0x00cda437" in verifier.doc_errors(real, changed_page),
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
    property_fragments = (
        (
            "descriptor initializer",
            "68 C4 BE 06 01 B9 54 93 35 01 E8 A1 DF 9E FF C7 05 68 93 35 01 02 00 00 00",
        ),
        (
            "constructor value default",
            "8A 86 5A 01 00 00 24 E0 0C 20 33 ED 88 86 5A 01 00 00",
        ),
        ("getter packed extraction", "8A 01 8B 4C 24 08 C0 E8 05 51 8D 54 24 07 24 01"),
        ("setter receiver and join", "51 50 81 C1 5A 01 00 00 E8 56 04 06 00"),
        ("refresh receiver and join", "8D 8E 5A 01 00 00 E8 AF 50 FF FF"),
        ("resolution deferred branch", "3D A8 6D 35 01 75 0B 80 0E 10"),
        (
            "constructor metadata bit one",
            "8B 0D 6C 93 35 01 C1 E9 07 02 C9 32 8E 5A 01 00 00 22 CB 30 8E 5A 01 00 00",
        ),
        (
            "constructor metadata bit zero",
            "8B 15 6C 93 35 01 8A 86 5A 01 00 00 C1 EA 05 32 D0 80 E2 01 32 D0 88 96 5A 01 00 00",
        ),
        ("writer refresh gate", "F6 06 10 57 8B 7C 24 20 74 0D"),
        ("writer refresh clear", "57 55 E8 27 90 F7 FF 83 C4 08 80 26 EF"),
        (
            "writer metadata gates",
            "8A 0E 8A 54 24 24 8A C1 D0 E8 24 01 74 04 84 D2 74 0D 84 C0 75 11 80 FA 01 75 0C 84 CA 75 11",
        ),
        ("writer low bit clear", "84 D2 75 05 80 E1 FE 88 0E"),
        (
            "writer value and callback gate",
            "8A 0E 8A 44 24 10 8A D1 C0 EA 05 80 E2 01 3A D0 74 2A C0 E0 05 32 C1 24 20 32 C1 0C 04 80 7C 24 28 01 88 06 75 16",
        ),
    )
    property_selected = {}
    for name, literal in property_fragments:
        property_selected["button eligibility " + name] = DATA_VA + len(data)
        data += bytes.fromhex(literal)
    property_selected["button eligibility property name"] = DATA_VA + len(data)
    data += b"IsHitTestVisible\0"
    property_selected["button eligibility getter case two"] = DATA_VA + len(data)
    data += struct.pack("<I", 0x0093A246)
    property_selected["button eligibility refresh case two"] = DATA_VA + len(data)
    data += struct.pack("<I", 0x0093D0FB)
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
    selected.update(property_selected)
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

    for name, offset, value in (
        ("descriptor initializer", 21, 3),
        ("constructor value default", 7, 0xC0),
        ("constructor value default", 9, 0x40),
        ("getter packed extraction", 8, 4),
        ("getter packed extraction", 15, 3),
        ("setter receiver and join", 4, 0x5B),
        ("refresh receiver and join", 2, 0x5B),
        ("resolution deferred branch", 5, 0x74),
        ("resolution deferred branch", 9, 0x20),
        ("constructor metadata bit one", 8, 6),
        ("constructor metadata bit zero", 14, 6),
        ("writer refresh gate", 8, 0x75),
        ("writer refresh clear", 12, 0xFF),
        ("writer metadata gates", 2, 0x8B),
        ("writer metadata gates", 5, 0x28),
        ("writer metadata gates", 16, 0x75),
        ("writer metadata gates", 24, 2),
        ("writer low bit clear", 6, 0xFF),
        ("writer value and callback gate", 5, 0x14),
        ("writer value and callback gate", 14, 0x38),
        ("writer value and callback gate", 24, 0x40),
        ("writer value and callback gate", 28, 0x08),
        ("writer value and callback gate", 33, 2),
        ("writer value and callback gate", 36, 0x74),
        ("property name", 0, ord("X")),
        ("getter case two", 0, 0x5B),
        ("refresh case two", 0, 0x25),
    ):
        label = "button eligibility " + name
        changed = bytearray(image)
        changed[0x200 + len(text) + selected[label] - DATA_VA + offset] = value
        errors = run(bytes(changed), document)
        check(label + f" literal byte {offset} is enforced", len(errors) == 1)
    for field, value in (
        ("name", "UNKNOWN"),
        ("propertyIndex", 3),
        ("elementOffset", "+0x15b"),
        ("widthBits", 32),
        ("valueMask", "0x04"),
        ("defaultOperation", "0x20"),
        ("writerCompare", "Compare old bit with V & 1."),
        ("writerReject", []),
        ("writerCallback", "Always call 0x0099c680."),
    ):
        changed = copy.deepcopy(real)
        changed["buttonSource"]["eligibilityProperty"][field] = value
        check(
            field + " eligibility property drift is rejected",
            "button eligibility property contract changed"
            in verifier.button_errors(changed),
        )
    for name in (
        "button eligibility descriptor initializer",
        "button eligibility constructor value default",
        "button eligibility writer metadata gates",
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
        if row.get("name") == "button eligibility writer value and callback gate"
    )
    row["mask"] = " ".join(["00"] * 38)
    check(
        "eligibility writer mask cannot be disabled",
        "button static check changed: button eligibility writer value and callback gate"
        in verifier.button_errors(changed),
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
    upstream_slot4_tests(real)
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
