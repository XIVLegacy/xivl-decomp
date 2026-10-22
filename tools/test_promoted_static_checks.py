#!/usr/bin/env python3
"""Tests for the timer handler, player profile and sun and moon static checks."""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_player_profile_accessors as profile  # noqa: E402
import verify_s2c_0193_native_state as timer  # noqa: E402
import verify_sun_moon_orientation as moon  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def check_bytes(item: dict) -> bytes:
    """The bytes a manifest row expects at its address."""
    if item["kind"] == "dwords":
        width = item.get("width", 4)
        return b"".join(int(v, 16).to_bytes(width, "little") for v in item["values"])
    pattern, mask = bytes.fromhex(item["pattern"]), bytes.fromhex(item["mask"])
    return bytes(p if m else 0 for p, m in zip(pattern, mask))


def build_image(rows: list[tuple[int, bytes]]) -> bytes:
    """A PE32 image with one section per row, each placed at its own VA."""
    first_raw = 0x1000
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(rows))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    table = 0x80 + 24 + 0xE0
    body = bytearray()
    for index, (va, data) in enumerate(rows):
        raw_offset = first_raw + len(body)
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII", header, base + 8, len(data), va - IMAGE_BASE, len(data), raw_offset
        )
        body += data
    return bytes(header) + bytes(body)


def rows_of(document: dict) -> list[tuple[int, bytes]]:
    return [(int(i["va"], 16), check_bytes(i)) for i in document["staticChecks"]]


def exercise(label: str, module, manifest: Path) -> None:
    document = json.loads(manifest.read_text(encoding="ascii"))
    rows = rows_of(document)
    image = build_image(rows)
    errors = module.image_checks.verify_image(image, document, require_identity=False)
    check(f"{label}: an image built from the rows passes", errors == [])
    check(
        f"{label}: identity is enforced",
        len(module.image_checks.verify_image(image, document)) >= 2,
    )
    missed = []
    for index, (_, data) in enumerate(rows):
        start = 0x1000 + sum(len(d) for _, d in rows[:index])
        flipped = bytearray(image)
        flipped[start] ^= 0x01
        if not module.image_checks.verify_image(
            bytes(flipped), document, require_identity=False
        ):
            missed.append(document["staticChecks"][index]["name"])
    check(f"{label}: every row reports a flipped byte (missed {missed})", not missed)
    moved = [(va + 1, data) for va, data in rows]
    check(
        f"{label}: rows at shifted addresses are reported",
        len(
            module.image_checks.verify_image(
                build_image(moved), document, require_identity=False
            )
        )
        >= 1,
    )


def main() -> int:
    exercise("timer handler", timer, timer.MANIFEST)
    exercise("player profile", profile, profile.MANIFEST)
    exercise("sun and moon", moon, moon.MANIFEST)

    sentinel = 0xFFFFFFFF
    check(
        "sentinel on a store subopcode stores zero",
        timer.stored_word(0x12, sentinel, 1_700_000_000) == 0,
    )
    check(
        "sentinel stores zero for the occupancy vector",
        timer.stored_word(0x03, sentinel, 5) == 0,
    )
    check(
        "store adds the clock",
        timer.stored_word(0x12, 900, 1_700_000_000) == 1_700_000_900,
    )
    check("store wraps modulo 2^32", timer.stored_word(0x10, 0x20, 0xFFFFFFF0) == 0x10)
    check("0x13 is not a clock store", timer.stored_word(0x13, 5, 7) is None)
    check("0x14 is not a timer store", timer.stored_word(0x14, 2, 7) is None)
    check("subopcodes above 0x16 store nothing", timer.stored_word(0x17, 1, 1) is None)
    check("ActionCheck sentinel is a query", timer.action_check_store(sentinel) is None)
    check(
        "ActionCheck stores the raw value",
        timer.action_check_store(0x7C000062) == 0x7C000062,
    )
    check(
        "0x14 acts while the flag is clear", timer.user_control_effect(0x14, 2, 0) == 1
    )
    check(
        "0x14 does nothing while the flag is set",
        timer.user_control_effect(0x14, 2, 1) is None,
    )
    check("0x15 acts while the flag is set", timer.user_control_effect(0x15, 2, 1) == 0)
    check(
        "0x15 does nothing while the flag is clear",
        timer.user_control_effect(0x15, 2, 0) is None,
    )
    check(
        "the exclusion set is shared",
        all(
            timer.user_control_effect(op, value, flag) is None
            for op, flag in ((0x14, 0), (0x15, 1))
            for value in (0x00, 0x15, 0x16)
        ),
    )

    check(
        "work index kinds and offsets",
        [profile.work_slot(i) for i in (1, 8, 9, 16)]
        == [("byte", 0x84), ("byte", 0x8B), ("word", 0x8C), ("word", 0x9A)],
    )
    check(
        "work index outside 1 to 16 reads adjacent memory",
        profile.work_slot(0) == ("word", 0x7A)
        and profile.work_slot(17) == ("word", 0x9C),
    )
    check(
        "negative work index wraps",
        profile.work_slot(-1) == ("word", (0x8C + 2 * 0xFFFFFFF6) & 0xFFFFFFFF),
    )
    flags = profile.expand_flags(0b1000_0101)
    check(
        "bit k is index k + 1",
        flags == [True, False, True, False, False, False, False, True],
    )

    vectors = [
        (0, -1, 0),
        (302_399, -1, 0),
        (302_400, -1, 1),
        (2_376_000, -1, 7),
        (2_721_600, -1, 0),
        (0, 5, 5),
        (0, 9, 9),
        (302_400, 0xFFFFFFFF, 1),
    ]
    check(
        "moon phase index vectors",
        all(moon.moon_phase_index(t, o) == want for t, o, want in vectors),
    )
    check(
        "moon phase repeats every 32 Eorzea days",
        moon.moon_phase_index(12345) == moon.moon_phase_index(12345 + 32 * 86400),
    )

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} promoted static check tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
