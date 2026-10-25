#!/usr/bin/env python3
"""Tests for the chara presentation consumer verifier."""

from __future__ import annotations

import copy
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_chara_presentation_consumers as verifier  # noqa: E402


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


def bank_of(document: dict) -> list[dict]:
    return [verifier.new_record(document) for _ in range(3)]


def native_look(document: dict, records: list[dict], target: int, weight: float = 0.5):
    verifier.write_record(records[0], 0xC8, target, weight)


def server_look(document: dict, records: list[dict], target: int, weight: float = 1.0):
    verifier.write_record(records[1], 0x64, target, weight)


def main() -> int:
    real = real_manifest()
    check("manifest and pages verify", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    check(
        "claim tiers are pinned",
        {c["id"]: c["tier"] for c in real["claims"]} == verifier.EXPECTED_CLAIMS,
    )
    check(
        "every claim names a page section",
        all(c["section"] in verifier.SECTIONS[c["page"]] for c in real["claims"]),
    )

    # record path
    commands = {r: verifier.scene_command(real, r) for r in (0x0A, 0x26, 0x32, 0x33)}
    check(
        "records map to the commands of the contract",
        commands == {0x0A: 0x1F, 0x26: 0x3B, 0x32: 0x47, 0x33: 0x48},
    )
    check(
        "commands 0x15 to 0x115 return the record",
        verifier.container_route(real, 0x48) == ("actor", 0x33)
        and verifier.container_route(real, 0x115) == ("actor", 0x100)
        and verifier.container_route(real, 0x15) == ("actor", 0),
    )
    check(
        "commands 0x0e to 0x14 use the table",
        [verifier.container_route(real, c)[1] for c in range(0x0E, 0x15)]
        == real["scenePath"]["commandTable"]
        and verifier.container_route(real, 0x10) == ("table", "0x007c9696")
        and verifier.container_route(real, 0x11) == ("table", "0x007c96b4"),
    )
    check(
        "the pump keeps 3 to 6 and the gate drops 0 to 2, 7 and 0x117",
        [verifier.container_route(real, c)[0] for c in (3, 6, 7, 0x117, 0x116)]
        == ["pump", "pump", "dropped", "dropped", "container"]
        and verifier.container_route(real, 0x0C)[0] == "container"
        and verifier.container_route(real, 2)[0] == "dropped",
    )
    check(
        "the actor table sends the look-at records to their cases",
        [verifier.actor_case(real, r) for r in range(0x0A, 0x0F)]
        == [
            "0x006633cc",
            "0x0066340c",
            "0x00663497",
            "0x00663520",
            "0x00663569",
        ],
    )
    ignored = [
        r
        for r in range(0x64)
        if verifier.actor_case(real, r) == real["charaActor"]["ignoredCase"]
    ]
    check(
        "thirty records share the empty case",
        len(ignored) == 30
        and 0x4A in ignored
        and 0x60 in ignored
        and 0x61 not in ignored
        and 0x62 in ignored
        and verifier.actor_case(real, 0x64) == real["charaActor"]["ignoredCase"],
    )

    # server feed
    expected_feed = {
        0xD3: "0x0058d039",
        0xDB: "0x0058d1a1",
        0xDC: "0x0058d1ce",
        0xDD: "0x0058d1f8",
        0xDE: "0x0058d21b",
        0xDF: "0x0058d229",
    }
    check(
        "six opcodes reach their cases on the full route",
        all(verifier.feed_case(real, op) == va for op, va in expected_feed.items()),
    )
    check(
        "the stricter route drops them",
        all(
            verifier.feed_case(real, op, "stricterRoute") == "0x0058d67d"
            for op in expected_feed
        ),
    )
    check(
        "opcodes outside the table take the default",
        verifier.feed_case(real, 0x0E) == "0x0058d675"
        and verifier.feed_case(real, 0xE6) == "0x0058d675"
        and verifier.feed_case(real, 0xE4, "stricterRoute") == "0x0058d67d",
    )
    moved = copy.deepcopy(real)
    moved["serverFeed"]["fullRoute"]["jumpTable"].reverse()
    check(
        "a reversed jump table changes the routes and is rejected",
        verifier.feed_case(moved, 0xD3) != expected_feed[0xD3]
        and "opcode 0x00d3 case disagrees with the full route"
        in verifier.verify_sections(moved),
    )

    # arbiter
    initial = verifier.new_record(real)
    check(
        "the initial record is kind 5 with no priority",
        verifier.kind_of(initial["flags"]) == 5
        and initial["flags"] == 0x1A
        and initial["priority"] == 0
        and initial["target"] == 0xC0000000
        and initial["weight"] == 1.0,
    )
    records = bank_of(real)
    check(
        "empty records post nothing and keep their bits",
        verifier.arbitrate_bank(real, 0, records) is None
        and [r["flags"] for r in records] == [0x1A] * 3,
    )
    native_look(real, records, 0x5000)
    message = verifier.arbitrate_bank(real, 0, records)
    check(
        "a native character look posts record 0x0b with priority byte 0x64",
        message is not None
        and message["record"] == 0x0B
        and message["size"] == 12
        and message["bank"] == 0
        and message["target"] == 0x5000
        and message["weight"] == 0.5
        and message["priority"] == 0x64
        and message["group"] == 0
        and message["byte1"] & 1 == 0,
    )
    check(
        "the winner is marked and the others are cleared",
        [r["flags"] & 1 for r in records] == [1, 0, 0],
    )
    marked = bank_of(real)
    verifier.write_record(marked[1], 0x64, 0x77)
    marked[0]["flags"] |= 1
    marked[2]["flags"] |= 1
    verifier.arbitrate_bank(real, 0, marked)
    check(
        "a stale bit 0 on the losing records is cleared",
        [r["flags"] & 1 for r in marked] == [0, 1, 0],
    )
    check(
        "a marked winner posts nothing again",
        verifier.arbitrate_bank(real, 0, records) is None,
    )
    for record in records:
        record["flags"] &= 0xFFFE
    check(
        "the resend clearing bit 0 posts the message again",
        verifier.arbitrate_bank(real, 0, records) is not None,
    )
    server_look(real, records, 0x6000)
    for record in records:
        record["flags"] &= 0xFFFE
    again = verifier.arbitrate_bank(real, 0, records)
    check(
        "priority 0xC8 outranks the server priority 0x64",
        again is not None and again["target"] == 0x5000,
    )
    verifier.cancel_record(records[0])
    check(
        "the cancel posts the server target next",
        (lambda m: m is not None and m["target"] == 0x6000)(
            verifier.arbitrate_bank(real, 0, records)
        ),
    )
    verifier.cancel_record(records[1])
    check(
        "the cancel of the last record posts nothing",
        verifier.arbitrate_bank(real, 0, records) is None,
    )
    check(
        "a cancelled record has kind 5, priority 0 and weight 1.0",
        verifier.kind_of(records[0]["flags"]) == 5
        and records[0]["priority"] == 0
        and records[0]["weight"] == 1.0
        and records[0]["target"] == 0xC0000000,
    )
    tied = bank_of(real)
    verifier.write_record(tied[2], 0x64, 0x700)
    verifier.write_record(tied[1], 0x64, 0x600)
    first = verifier.arbitrate_bank(real, 1, tied)
    check(
        "equal priorities pick the lowest slot",
        first is not None
        and first["target"] == 0x600
        and first["bank"] == 1
        and [r["flags"] & 1 for r in tied] == [0, 1, 0],
    )
    decaying = bank_of(real)
    verifier.write_record(decaying[0], 0x64, 0x800)
    decaying[0]["decay"] = 0x10
    first = verifier.arbitrate_bank(real, 0, decaying)
    check(
        "the decay lowers the priority after the choice",
        first is not None
        and first["target"] == 0x800
        and decaying[0]["priority"] == 0x54,
    )
    decaying[0]["priority"] = 0x08
    verifier.arbitrate_bank(real, 0, decaying)
    check("the priority stops at 0", decaying[0]["priority"] == 0)
    gated = bank_of(real)
    verifier.write_record(gated[0], 0x64, 0x900)
    gated[0]["flags"] |= 1
    verifier.write_record(gated[1], 0x32, 0xA00)
    gated[1]["flags"] |= 1
    gated[2]["flags"] &= 0xFFFE
    before = [r["flags"] for r in gated]
    check(
        "a winner with bit 0 set posts nothing and changes no bit",
        verifier.arbitrate_bank(real, 0, gated) is None
        and [r["flags"] for r in gated] == before,
    )
    rejected = bank_of(real)
    verifier.write_record(rejected[0], 0xC8, 0x1)
    check(
        "a lower priority write is rejected",
        not verifier.write_record(rejected[0], 0x64, 0x2)
        and rejected[0]["target"] == 0x1,
    )
    check(
        "an equal priority write replaces the record",
        verifier.write_record(rejected[0], 0xC8, 0x3) and rejected[0]["target"] == 3,
    )
    own = bank_of(real)
    verifier.write_record(own[0], 0xC8, 0x1234, own_key=0x1234)
    check("the own key becomes the null target", own[0]["target"] == 0xC0000000)

    kinds = bank_of(real)
    verifier.write_record(
        kinds[0], 0xC8, 0xC0000000, 0.25, vector=(1.0, 2.0, 3.0), kind=2
    )
    position = verifier.arbitrate_bank(real, 0, kinds)
    check(
        "a position posts record 0x0d with 20 bytes",
        position is not None
        and position["record"] == 0x0D
        and position["size"] == 20
        and position["vector"] == (1.0, 2.0, 3.0)
        and position["priority"] is None
        and position["byte1"] == 1,
    )
    angles = bank_of(real)
    verifier.write_record(angles[0], 0xC8, 0xC0000000, 0.25, kind=3)
    check(
        "angles post record 0x0e with 16 bytes",
        (
            lambda m: (
                m is not None
                and m["record"] == 0x0E
                and m["size"] == 16
                and m["byte1"] == 1
            )
        )(verifier.arbitrate_bank(real, 0, angles)),
    )
    named = bank_of(real)
    verifier.write_record(named[0], 0x64, 0x55, 1.0, name=b"j_kao".ljust(16, b"\0"))
    named_message = verifier.arbitrate_bank(real, 0, named)
    check(
        "a bone name posts record 0x0c with 40 bytes",
        named_message is not None
        and named_message["record"] == 0x0C
        and named_message["size"] == 40
        and named_message["name"].startswith(b"j_kao")
        and named_message["byte1"] == 1,
    )
    named_high = bank_of(real)
    verifier.write_record(
        named_high[0], 0x64, 0x55, 1.0, name=b"j_kao".ljust(16, b"\0"), kind=4
    )
    check(
        "a named kind 4 look sets both bits of byte 1 of record 0x0c",
        (lambda m: m is not None and m["record"] == 0x0C and m["byte1"] == 3)(
            verifier.arbitrate_bank(real, 0, named_high)
        ),
    )
    high = bank_of(real)
    verifier.write_record(high[0], 0x64, 0x55, 1.0, kind=4)
    high_message = verifier.arbitrate_bank(real, 0, high)
    check(
        "kind 4 posts the group 1 priority byte 0xc8",
        high_message is not None
        and high_message["record"] == 0x0B
        and high_message["group"] == 1
        and high_message["priority"] == 0xC8
        and high_message["byte1"] == 1,
    )
    silent = bank_of(real)
    verifier.write_record(silent[0], 0x64, 0x55)
    silent[0]["flags"] = (silent[0]["flags"] & 0xFFF1) | (6 << 1)
    check(
        "kinds 5 to 7 post nothing but still mark the winner",
        verifier.arbitrate_bank(real, 0, silent) is None
        and silent[0]["flags"] & 1 == 1,
    )

    swapped = copy.deepcopy(real)
    swapped["lookAt"]["kinds"][2]["record"] = "0x0e"
    records = bank_of(swapped)
    verifier.write_record(records[0], 0xC8, 0xC0000000, 1.0, kind=2)
    wrong = verifier.arbitrate_bank(swapped, 0, records)
    check(
        "a swapped kind record changes the posted record and is rejected",
        wrong is not None
        and wrong["record"] == 0x0E
        and "lookAt.kinds.2.record changed" in verifier.verify_sections(swapped),
    )

    # controller
    flags = verifier.slot_flags_after_write(
        real, 0x10 | 0x40000 | 0x8 | 0x40 | 0x400, "0x00853b50", True, False
    )
    check(
        "a slot writer keeps the suppress and timer bits and replaces the others",
        flags == (0x10 | 0x40000 | 0x20 | 0x8 | 0x200),
    )
    check(
        "the position and angle writers set their own flag",
        verifier.slot_flags_after_write(real, 0, "0x00853c50", False, True)
        == 0x20 | 0x40 | 0x80
        and verifier.slot_flags_after_write(real, 0, "0x00853d30", True, False)
        == 0x20 | 0x8 | 0x100,
    )
    armed = 0x8 | 0x20000 | 0x40000
    check(
        "state 1 arms with the target and both timer bits and without flag 0x10",
        verifier.slot_arms(armed)
        and not verifier.slot_arms(armed | 0x10)
        and not verifier.slot_arms(armed & ~0x8)
        and not verifier.slot_arms(armed & ~0x20000)
        and not verifier.slot_arms(armed & ~0x40000),
    )
    check(
        "state 3 disarms when flag 0x10 is set or any condition bit is lost",
        not verifier.slot_disarms(armed)
        and verifier.slot_disarms(armed | 0x10)
        and verifier.slot_disarms(armed & ~0x8)
        and verifier.slot_disarms(armed & ~0x20000)
        and verifier.slot_disarms(armed & ~0x40000),
    )
    check(
        "state 3 refreshes for flag 0x20 or 0x40",
        verifier.slot_refreshes(armed | 0x20)
        and verifier.slot_refreshes(armed | 0x40)
        and not verifier.slot_refreshes(armed),
    )

    check(
        "a weight of 0 or below 0.00001 switches the IK object off",
        not verifier.ik_active(real, 0.0)
        and not verifier.ik_active(real, 0.000009)
        and verifier.ik_active(real, 0.00001)
        and verifier.ik_active(real, 1.0),
    )

    # turn
    bounds = [
        (0x000000, -1),
        (0x1C71C7, -1),
        (0x1C71C8, -2),
        (0x4E38E3, -2),
        (0x4E38E4, -3),
        (0x800000, -3),
        (0x800001, 3),
        (0xB1C71C, 3),
        (0xB1C71D, 2),
        (0xE38E39, 2),
        (0xE38E3A, 1),
        (0xFFFFFF, 1),
        (0x1000000, -1),
    ]
    check(
        "the angle classes change at the exact bounds",
        all(verifier.turn_class(real, d) == c for d, c in bounds),
    )
    check(
        "a turn back without a saved heading posts at once",
        verifier.turn_back_posts(real, 0x123456, 0, False),
    )
    edges = [
        (0x000000, True),
        (0x038E38, True),
        (0x038E39, False),
        (0x800000, False),
        (0xFC71C7, False),
        (0xFC71C8, True),
        (0xFFFFFF, True),
    ]
    check(
        "a turn back posts inside 5 degrees and turns outside",
        all(verifier.turn_back_posts(real, d, 0, True) == post for d, post in edges),
    )
    check(
        "the turn back window wraps around the circle",
        verifier.turn_back_posts(real, 0x000010, 0xFFFFF0, True)
        and not verifier.turn_back_posts(real, 0x100000, 0xFFFFF0, True),
    )
    check(
        "the timer is 10.0 or the 6 bit controller value and mode 2 keeps 10.0",
        verifier.turn_timer_start(real, 3, 1) == 10.0
        and verifier.turn_timer_start(real, 42, 1) == 42.0
        and verifier.turn_timer_start(real, 0x1FF, 0) == 63.0
        and verifier.turn_timer_start(real, 0x1FF, 2) == 10.0
        and verifier.turn_timer_start(real, 10, 1) == 10.0,
    )
    check(
        "the timer posts the event when it reaches 0",
        verifier.turn_timer_step(10.0, 1.0, 4.0) == (6.0, False)
        and verifier.turn_timer_step(6.0, 1.0, 6.0) == (0.0, True)
        and verifier.turn_timer_step(6.0, 2.0, 4.0) == (0.0, True)
        and verifier.turn_timer_step(0.0, 1.0, 4.0) == (0.0, False)
        and verifier.turn_timer_step(1.0, 0.0, 4.0) == (1.0, False),
    )
    check(
        "the event code travels with a bias of 3",
        verifier.queued_code(real, 0x4A) == 0x4D
        and verifier.delivered_event(real, 0x4D) == 0x4A
        and verifier.delivered_event(real, 2) is None
        and verifier.delivered_event(real, 3) == 0
        and verifier.delivered_event(real, 0x103) == 0x100
        and verifier.delivered_event(real, 0x104) is None,
    )
    check(
        "the call and pattern scans agree with the turn sections",
        real["turn"]["event"]["sites"]
        == verifier.scan_sites(real, real["turn"]["event"]["postVa"])
        and real["turn"]["bitSetSites"]
        == verifier.pattern_scan(real, "or byte ptr [reg+0x26d], 2")
        and real["turn"]["bitClearSites"]
        == verifier.pattern_scan(real, "and byte ptr [reg+0x26d], 0xfd"),
    )
    shifted = copy.deepcopy(real)
    shifted["turn"]["event"]["sites"].append("0x00400000")
    check(
        "an extra event site is rejected",
        "event sites differ from the call scan" in verifier.verify_sections(shifted),
    )

    # visibility, floating offset and marker
    check(
        "the hidden bit is bit 1 and the actor is visible at 0",
        verifier.hidden_byte(real, 0, False) == 2
        and verifier.hidden_byte(real, 0xFF, True) == 0xFD
        and verifier.hidden_byte(real, 1, False) == 3
        and verifier.is_visible(0)
        and not verifier.is_visible(2),
    )
    step = verifier.easing_next(real, 0.0, 10.0, 1.0)
    check(
        "the easing step is capped above and not below",
        abs(step - 0.02) < 1e-9
        and abs(verifier.easing_next(real, 0.0, 0.5, 1.0) - 0.01) < 1e-9
        and abs(verifier.easing_next(real, 0.0, -10.0, 1.0) + 0.2) < 1e-9
        and verifier.easing_next(real, 0.5, 2.0, 0.0) == 0.5
        and verifier.easing_next(real, 0.5, 2.0, -1.0) == 0.5,
    )
    check(
        "the marker number replaces the low 6 bits only",
        verifier.marker_set(real, 0xABCD1234, 0x15) == 0xABCD1215
        and verifier.marker_set(real, 0xFFFFFFFF, 0x41) == 0xFFFFFFC1
        and verifier.marker_number(real, 0xABCD123F) == 0x3F,
    )
    check(
        "the map entity uses 0, -1 and the number as the routine does",
        verifier.marker_entity_value(real, True, False, 9) == 0
        and verifier.marker_entity_value(real, False, False, 9) == -1
        and verifier.marker_entity_value(real, False, True, 0) == -1
        and verifier.marker_entity_value(real, False, True, 0x41) == -1
        and verifier.marker_entity_value(real, False, True, 0x40) == 0x40
        and verifier.marker_entity_value(real, False, True, 0x3F) == 0x3F,
    )

    # executable scans on a synthetic image
    text = bytearray(0x40)
    text[0x00] = 0xE8
    struct.pack_into("<i", text, 0x01, 0x30 - 0x05)
    text[0x10] = 0xE9
    struct.pack_into("<i", text, 0x11, 0x30 - 0x15)
    text[0x20:0x27] = bytes([0x80, 0x8E, 0x6D, 0x02, 0x00, 0x00, 0x02])
    text[0x30] = 0xC3
    data = struct.pack("<I", 0xDEADBEEF) + b"EID_LOOK_AT\0"
    image = build_image(bytes(text), data)
    sites = verifier.call_sites(image, {0x00401030, 0x00401999})
    check(
        "the call scan finds calls and jumps by rel32",
        sites[0x00401030] == [0x00401000, 0x00401010] and sites[0x00401999] == [],
    )
    check(
        "the pattern scan finds the turn bit instruction",
        verifier.pattern_sites(image, r"\x80[\x88-\x8f]\x6d\x02\x00\x00\x02")
        == [0x00401020],
    )
    check(
        "the dword scan finds a value in the data section",
        verifier.dword_sites(image, 0xDEADBEEF) == [0x00402000],
    )
    document = {
        "staticChecks": [
            {
                "name": "ret",
                "kind": "signature",
                "va": "0x00401030",
                "pattern": "c3",
                "mask": "ff",
            },
            {
                "name": "table",
                "kind": "dwords",
                "va": "0x00402000",
                "values": ["0xdeadbeef"],
            },
            {
                "name": "node",
                "kind": "string",
                "va": "0x00402004",
                "text": "EID_LOOK_AT",
            },
        ],
        "scans": {
            "callSites": [
                {"target": "0x00401030", "sites": ["0x00401000", "0x00401010"]}
            ],
            "patternSites": [
                {
                    "name": "turn bit",
                    "regex": r"\x80[\x88-\x8f]\x6d\x02\x00\x00\x02",
                    "sites": ["0x00401020"],
                }
            ],
            "dwordSites": [{"value": "0xdeadbeef", "sites": ["0x00402000"]}],
        },
    }
    check(
        "a matching synthetic image passes",
        verifier.verify_image(image, document, require_identity=False) == [],
    )
    broken = bytearray(image)
    broken[0x200 + 0x30] = 0x90
    check(
        "a changed byte fails its signature",
        len(verifier.verify_image(bytes(broken), document, require_identity=False))
        == 1,
    )
    document["scans"]["callSites"][0]["sites"] = ["0x00401000"]
    check(
        "a missing call site fails the scan",
        verifier.verify_image(image, document, require_identity=False)
        == ["call sites of 0x00401030 differ from the scan"],
    )
    check(
        "a wrong identity is reported",
        len(verifier.verify_image(image, document)) >= 2,
    )

    print(f"passed {len(PASSED)} checks")
    if FAILED:
        print("FAILED:")
        for name in FAILED:
            print(f"- {name}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
