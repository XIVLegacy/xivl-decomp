#!/usr/bin/env python3
"""Tests for the combat result element verifier."""

from __future__ import annotations

import copy
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_combat_result_element as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def build_image(text: bytes, data: bytes = b"") -> bytes:
    """A PE32 image with raw .text at 0x401000 and an optional raw .data at 0x402000."""
    header = bytearray(0x200)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    sections = 2 if data else 1
    struct.pack_into("<H", header, 0x80 + 6, sections)
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    base = 0x80 + 24 + 0xE0
    padded = (len(text) + 0x1FF) // 0x200 * 0x200
    header[base : base + 5] = b".text"
    struct.pack_into("<IIII", header, base + 8, len(text), 0x1000, len(text), 0x200)
    if data:
        header[base + 40 : base + 45] = b".data"
        struct.pack_into(
            "<IIII", header, base + 48, len(data), 0x2000, len(data), 0x200 + padded
        )
    body = text.ljust(padded, b"\0") + data
    return bytes(header) + body


def real_manifest() -> dict:
    return json.loads(verifier.MANIFEST.read_text(encoding="ascii"))


def hex_number(text: str) -> int:
    return int(text, 16)


def main() -> int:
    check("manifest and page verify", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    real = real_manifest()
    check(
        "claim tiers are pinned",
        {c["id"]: c["tier"] for c in real["claims"]} == verifier.EXPECTED_CLAIMS,
    )

    check(
        "the four combat opcodes are dropped in the rejecting state",
        not any(verifier.rejected_state_passes(real, op) for op in range(0x139, 0x13D)),
    )
    check(
        "movement and profile opcodes pass in the rejecting state",
        all(
            verifier.rejected_state_passes(real, op)
            for op in (0x137, 0x144, 0x145, 0x197, 0x1A0)
        )
        and not any(
            verifier.rejected_state_passes(real, op) for op in (0x138, 0x146, 0x198)
        ),
    )
    try:
        verifier.rejected_state_passes(real, 0x134)
        refused = False
    except ValueError:
        refused = True
    check("an opcode outside the rejected-state table is refused", refused)
    check(
        "the admitted-state tables select the staging cases",
        [verifier.admitted_case(real, op) for op in (0x139, 0x13A, 0x13B, 0x13C)]
        == [0x0058D553, 0x0058D565, 0x0058D577, 0x0058D541],
    )

    grouped, plain = verifier.group_text_ids(real)
    check(
        "the group list route covers 0x848E to 0x8492, 0x8494, 0x8495 and 0x8499 to 0x84A3",
        grouped
        == ["0x848e", "0x848f", "0x8490", "0x8491", "0x8492", "0x8494", "0x8495"]
        + [f"0x{i:04x}" for i in range(0x8499, 0x84A4)],
    )
    check(
        "0x8493 and 0x8496 to 0x8498 get the default line only",
        plain == ["0x8493", "0x8496", "0x8497", "0x8498"],
    )
    retargeted = copy.deepcopy(real)
    for item in retargeted["staticChecks"]:
        if item["name"] == "special text id group targets":
            item["values"] = item["values"][::-1]
    check(
        "swapped jump targets are rejected",
        verifier.group_text_ids(retargeted)[0] != grouped
        and "group text IDs disagree with the jump table"
        in verifier.verify_sections(retargeted),
    )

    classes = {
        0x7C000062: (6, 7),
        0x6F000001: (6, 8),
        0x70123456: (5, 9),
        0x71000000: (6, 10),
        0x0400D000: (6, 13),
        0x04013FFF: (6, 13),
        0x04014000: (6, 12),
        0x04FE9000: (6, 13),
        0x04000000: (6, 12),
        0x0A000000: (6, 11),
        0x15000000: (6, 14),
        0x16000000: (6, 15),
        0x17000000: (6, 16),
        0x18000000: (6, 17),
        0x01000000: (6, 6),
        0x01000000 + 0x0F000000: (5, 5),
        0x0B123456: (6, 6),
        0x22000000: (6, 6),
        0xFF000000: (5, 5),
    }
    check(
        "effect IDs classify by type lane and route kind",
        all(
            verifier.route_class(real, effect, fallback) == expected
            for effect, (fallback, expected) in classes.items()
        ),
    )
    check(
        "kind 1 splits on the middle lane bounds",
        verifier.route_class(real, 0x0400C000, 6) == 12
        and verifier.route_class(real, 0x0400D000, 6) == 13
        and verifier.route_class(real, 0x04013000, 6) == 13
        and verifier.route_class(real, 0x04014000, 6) == 12,
    )

    check(
        "the drain routes classes to cases and scene commands",
        [verifier.drain_case(real, c) for c in (0, 1, 2, 3, 11, 12, 25, 26)]
        == [0, 1, 2, 3, 3, 4, 4, None]
        and [verifier.scene_op(real, c) for c in (0, 1, 2, 3, 12, 25, 26)]
        == [3, 4, 4, 4, 4, 4, None],
    )
    check(
        "the header routine covers class 5 and classes 6 to 10",
        [verifier.header_target(c) for c in (4, 5, 6, 10, 11)]
        == [None, "0x00585af0", "0x0058c220", "0x0058c220", None],
    )

    check(
        "the plate kind is effect ID bits 27 to 30",
        [
            verifier.plate_kind(e)
            for e in (0x04000000, 0x08000000, 0x38000000, 0x50000000, 0x78000000)
        ]
        == [0, 1, 7, 10, 15],
    )
    check(
        "kinds 7 to 10 anchor on the source",
        [
            verifier.plate_anchor_is_source(real, e)
            for e in (0x30000000, 0x38000000, 0x48000000, 0x50000000, 0x58000000)
        ]
        == [False, True, True, True, False],
    )
    check(
        "plate digits run least significant first and stop after five",
        verifier.plate_digits(0) == [0, None, None, None, None]
        and verifier.plate_digits(7) == [7, None, None, None, None]
        and verifier.plate_digits(120) == [0, 2, 1, None, None]
        and verifier.plate_digits(99999) == [9, 9, 9, 9, 9]
        and verifier.plate_digits(100000) == [0, 0, 0, 0, 0]
        and verifier.plate_digits(123456) == [6, 5, 4, 3, 2],
    )

    check(
        "a row with effect ID 0 gets no plate and any other row gets its kind",
        verifier.plate_for_row(real, 0) is None
        and verifier.plate_for_row(real, 0x00000001) == 0
        and verifier.plate_for_row(real, 0x38000000) == 7
        and verifier.plate_for_row(real, 0x78000000) == 15,
    )
    zero = verifier.plate_zero_return(real)
    check(
        "the plate zero return is decoded from the routine bytes",
        zero is not None
        and (
            zero["loadVa"],
            zero["branchVa"],
            zero["returnVa"],
            zero["rowEffectOffset"],
            zero["rowStride"],
        )
        == (0x007956D4, 0x007956E5, 0x007967A2, 0x40, 0x14),
    )
    check(
        "the zero branch follows the anchor calls and precedes the inline kind decode",
        verifier.plate_pre_branch_calls(real)
        == ["0x00794f30", "0x00794ee0", "0x004d86b0", "0x004d9970"]
        and zero is not None
        and zero["kindDecodeVa"] == 0x007956ED,
    )
    retargeted = copy.deepcopy(real)
    for item in retargeted["staticChecks"]:
        if item["name"] == "plate pre-branch calls":
            item["pattern"] = item["pattern"].replace(
                "e8 f3 2f d4 ff", "e8 f4 2f d4 ff", 1
            )
    check(
        "a changed call before the zero branch is reported",
        "plate routine calls before the zero branch disagree with the manifest"
        in verifier.verify_plate_zero(retargeted),
    )
    moved = copy.deepcopy(real)
    for item in moved["staticChecks"]:
        if item["name"] == "plate row inputs":
            item["pattern"] = item["pattern"].replace("8b 44 8b 40", "8b 44 8b 3c", 1)
    check(
        "a different row field offset is reported",
        "plate effect load is not the row effect field"
        in verifier.verify_plate_zero(moved),
    )

    check(
        "the radius is read as an unsigned dword",
        verifier.radius_conversion(real) == "unsigned 32-bit"
        and verifier.range_gate_passes(real, 0x80000000, 2.0e9)
        and verifier.range_gate_passes(real, 0xFFFFFFFF, 4.0e9)
        and not verifier.range_gate_passes(real, 0x80000000, 3.0e9),
    )
    check(
        "the range gate is strict",
        verifier.range_gate_rule(real) == "radius greater than distance"
        and verifier.range_gate_passes(real, 50, 49.5)
        and not verifier.range_gate_passes(real, 50, 50.0)
        and not verifier.range_gate_passes(real, 50, 50.5)
        and not verifier.range_gate_passes(real, 0, 0.0),
    )
    signed = copy.deepcopy(real)
    for item in signed["staticChecks"]:
        if item["name"] == "radius unsigned read":
            item["pattern"] = item["pattern"].replace("85 c9 7d 06", "85 c9 7c 06", 1)
    check(
        "a signed radius read is reported",
        verifier.radius_conversion(signed) is None
        and "radius conversion disagrees with the range gate bytes"
        in verifier.verify_range_gate(signed),
    )
    check(
        "the setter stores the low dword of a truncated 64-bit integer",
        verifier.radius_store(real)
        == "low dword of the toward-zero truncated 64-bit integer",
    )

    layout = verifier.sound_table_layout(real)
    ids = verifier.sound_table_ids(real)
    check(
        "the sound table layout is decoded from the filter scan",
        layout is not None
        and (layout["va"], layout["stride"], layout["count"]) == (0x01270BA4, 2, 30)
        and (layout["hitVa"], layout["missVa"], layout["hitMissVa"], layout["getterVa"])
        == (0x00585A6C, 0x00585A17, 0x00585A17, 0x004D7490)
        and len(ids) == 30
        and len(set(ids)) == 30,
    )
    local = 0x10000001
    other = 0x20000002
    check(
        "a table hit for another actor ends the row and every other row continues",
        not verifier.sound_row_continues(real, other, 0x848E, local)
        and not verifier.sound_row_continues(real, other, 0x8493, local)
        and verifier.sound_row_continues(real, local, 0x848E, local)
        and verifier.sound_row_continues(real, other, 0x8496, local)
        and verifier.sound_row_continues(real, other, 0x75B1, local)
        and verifier.sound_row_continues(real, other, 0x846E, local)
        and verifier.sound_row_continues(real, other, 0x8475, local),
    )
    grouped_ids = [int(i, 16) for i in grouped] + [0x847F]
    check(
        "every extra route text ID of the group list and 0x847F is in the table",
        all(i in ids for i in grouped_ids)
        and not any(i in ids for i in (0x75B1, 0x75B4, 0x8496, 0x8497, 0x8498)),
    )
    stepped = copy.deepcopy(real)
    for item in stepped["staticChecks"]:
        if item["name"] == "message sound table scan":
            item["pattern"] = item["pattern"].replace("83 c0 02", "83 c0 04", 1)
    check(
        "a changed table stride is reported",
        "sound filter layout disagrees with the manifest"
        in verifier.verify_sound_filter(stepped),
    )
    inverted = copy.deepcopy(real)
    for item in inverted["staticChecks"]:
        if item["name"] == "message sound table hit":
            item["pattern"] = item["pattern"].replace("32 c0", "b0 01", 1)
    check(
        "a table hit that returns true is reported",
        verifier.sound_table_layout(inverted) is None
        and verifier.verify_sound_filter(inverted)
        == ["sound filter bytes are unreadable"],
    )

    record = real["record"]
    check(
        "eighteen rows fill the record",
        hex_number(record["headerSize"])
        + record["rowCapacity"] * hex_number(record["rowStride"])
        == hex_number(record["size"]),
    )
    ends = {}
    for form, capacity in (("single", 1), ("ten", 10), ("eighteen", 18)):
        wire = record["wireRows"][form]
        strides = record["wireRowStrides"]
        order = ("target", "numeric", "textId", "effect", "textParam", "ordinal")
        contiguous = all(
            hex_number(wire[a]) + strides[a] * capacity == hex_number(wire[b])
            for a, b in zip(order, order[1:])
        )
        last = hex_number(wire["ordinal"]) + capacity - 1
        ends[form] = (contiguous, last)
    check(
        "wire row arrays are contiguous where the form has more than one row",
        ends["ten"][0] and ends["eighteen"][0],
    )
    check(
        "the last payload byte read follows from the row maps",
        [ends["single"][1], ends["ten"][1], ends["eighteen"][1]] == [53, 179, 291]
        and [o["lastPayloadByteRead"] for o in real["opcodes"]] == [53, 179, 291, 39],
    )
    check(
        "the bucket rings end at the main ring",
        hex_number(real["drain"]["bucketRingBase"])
        + real["drain"]["bucketCount"] * hex_number(real["drain"]["bucketRingStride"])
        == hex_number(real["drain"]["mainRingOffset"]),
    )

    text = bytes.fromhex("E8 0B 00 00 00 E9 06 00 00 00 90 90 90 90 90 C3")
    calls = build_image(text)
    check(
        "rel32 call and jump targets are collected",
        verifier.call_sites(calls, {0x00401010, 0x00401099})
        == {0x00401010: [0x00401000, 0x00401005], 0x00401099: []},
    )
    pushes = build_image(
        bytes.fromhex("6A 44 E8 09 00 00 00")
        + b"\x90" * 9
        + b"\xc3"
        + b"\x90" * 18
        + bytes.fromhex("44 E8 E7 FF FF FF")
    )
    target = 0x00401010
    check(
        "only the call with a close push is listed",
        verifier.push_sites(pushes, target, {0x43, 0x44, 0x45}, 14) == [0x00401002],
    )
    check(
        "the window and the value limit the push scan",
        verifier.push_sites(pushes, target, {0x44}, 2) == [0x00401002]
        and verifier.push_sites(pushes, target, {0x44}, 1) == []
        and verifier.push_sites(pushes, target, {0x46}, 14) == [],
    )

    data = struct.pack("<4I", 0x12345678, 0, 0x12345678, 7)
    two = build_image(b"\x90" * 16, data)
    check(
        "dword sites cover every section",
        verifier.dword_sites(two, 0x12345678) == [0x00402000, 0x00402008]
        and verifier.dword_sites(two, 0x0BADF00D) == [],
    )

    wide = {"name": "w", "kind": "wideString", "va": "0x00402000", "text": "AB"}
    wide_image = build_image(b"\x90" * 16, "AB".encode("utf-16-le") + b"\0\0")
    sections = verifier.image_checks.section_map(wide_image)
    check(
        "a wide string check passes",
        verifier.wide_check(wide_image, sections, wide) is None,
    )
    narrow_image = build_image(b"\x90" * 16, b"AB\0")
    check(
        "an ASCII string does not satisfy a wide string check",
        verifier.wide_check(
            narrow_image, verifier.image_checks.section_map(narrow_image), wide
        )
        is not None,
    )

    scanned = {
        "staticChecks": [wide],
        "scans": {
            "callSites": [
                {"target": "0x00401010", "sites": ["0x00401000", "0x00401005"]}
            ],
            "pushScans": [],
            "dwordSites": [{"value": "0x12345678", "sites": []}],
        },
    }
    both = build_image(text, "AB".encode("utf-16-le") + b"\0\0")
    check(
        "a matching image and scan list pass",
        verifier.verify_image(both, scanned, require_identity=False) == [],
    )
    scanned["scans"]["callSites"][0]["sites"].pop()
    check(
        "an unlisted call site is reported",
        len(verifier.verify_image(both, scanned, require_identity=False)) == 1,
    )
    scanned["scans"]["callSites"][0]["sites"].append("0x00401005")
    wide["text"] = "AC"
    check(
        "a changed wide string is reported",
        len(verifier.verify_image(both, scanned, require_identity=False)) == 1,
    )
    check(
        "identity is enforced",
        len(verifier.verify_image(both, scanned, require_identity=True)) >= 2,
    )

    page = verifier.PAGE.read_text(encoding="utf-8")
    check(
        "the page verifies against the manifest", verifier.verify_page(real, page) == []
    )
    check(
        "a changed tier in the verdict table is reported",
        len(verifier.verify_page(real, page.replace("| STATIC |", "| UNPROVED |", 1)))
        >= 1,
    )
    check(
        "a dropped consumer bullet is reported",
        len(
            verifier.verify_page(
                real, page.replace("- Treat a row with text ID 0", "- Treat a row", 1)
            )
        )
        >= 1,
    )
    check(
        "heading anchors follow the rendered slug",
        verifier.heading_anchors("## Damage plate\n### Row trigger!\n")
        == {"damage-plate", "row-trigger"},
    )
    swapped = copy.deepcopy(real)
    swapped["claims"][0], swapped["claims"][1] = (
        swapped["claims"][1],
        swapped["claims"][0],
    )
    check(
        "swapped claim order disagrees with the page",
        len(verifier.verify_page(swapped, page)) >= 1,
    )

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} combat result element tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
