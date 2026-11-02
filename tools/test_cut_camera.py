#!/usr/bin/env python3
"""Tests for the CCB camera container parser on a synthetic container."""

from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_cut_camera as ccb  # noqa: E402

PASSED: list[str] = []
FAILED: list[str] = []


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def pad(data: bytes, align: int = 16) -> bytes:
    return data + bytes(-len(data) % align)


def constant(value: float) -> bytes:
    return struct.pack("<BBHf", 0, 0, 0, value)


def fixed(keys: list[tuple[float, float, float, float]]) -> bytes:
    body = b"".join(struct.pack("<4f", *key) for key in keys)
    return struct.pack("<BBH", 2, 0, len(keys)) + body


def step(keys: list[tuple[float, float]]) -> bytes:
    body = b"".join(struct.pack("<2f", *key) for key in keys)
    return struct.pack("<BBH", 7, 0, len(keys)) + body


def curve_section(group: int, mask: int, curves: list[bytes]) -> bytes:
    head = struct.pack("<I", 0x80000000 | group) + struct.pack("<HH", len(curves), mask)
    return head + b"".join(curves)


def build_mtb(groups: list[bytes], sections: list[bytes], curve_sections: int) -> bytes:
    section_names = [b"Header", b"Curve"]
    header = struct.pack("<ffHHB", 30.0, 280.0, curve_sections, 0, 0) + bytes(3)
    blobs = [(0, header)] + [(1, body) for body in sections]
    tables_len = (
        (4 + 4 * len(blobs)) + (4 + 4 * len(section_names)) + (4 + 4 * len(groups))
    )
    first = 0x40 + tables_len
    first += -first % 16
    placed = bytearray()
    offsets = []
    for name, body in blobs:
        offsets.append(first + len(placed))
        placed += pad(struct.pack("<I", name) + bytes(12) + body)
    strings_at = first + len(placed)
    strings = bytearray()
    name_offsets = []
    for text in section_names + groups:
        name_offsets.append(strings_at + len(strings))
        strings += text + b"\x00"
    tables = struct.pack("<I", len(blobs)) + b"".join(
        struct.pack("<I", o) for o in offsets
    )
    tables += struct.pack("<I", 2) + b"".join(
        struct.pack("<I", o) for o in name_offsets[:2]
    )
    tables += struct.pack("<I", len(groups))
    tables += b"".join(struct.pack("<I", o) for o in name_offsets[2:])
    head = bytearray(0x40)
    head[0:8] = b"SEDBmtb\x00"
    struct.pack_into("<IHH", head, 8, 0x43, 0, 0x40)
    out = bytes(head) + tables
    out += bytes(first - len(out)) + bytes(placed) + bytes(strings)
    return out


def build_ccb(
    selector: int = 1, duration: int = 2_800_000, offset: float = 0.0
) -> bytes:
    groups = [b"cam_weight", b"cam", b"cam_interest", b"cam_flen", b"cam_rol", b"root"]
    sections = [
        curve_section(0, 1, [step([(0.0, 0.0)])]),
        curve_section(
            1,
            0x0007,
            [
                constant(1.0),
                constant(2.0),
                fixed([(0.0, 3.0, 0.0, 0.0), (10.0, 4.0, 0.0, 0.0)]),
            ],
        ),
        curve_section(2, 0x0007, [constant(0.0), constant(0.0), constant(-4.0)]),
        curve_section(3, 0x0002, [constant(25.0)]),
        curve_section(4, 0x2000, [constant(0.0)]),
        curve_section(5, 0x0001 | 0x4000, [constant(5.0), constant(90.0)]),
    ]
    mtb = build_mtb(groups, sections, len(sections))
    fcpa_at = ccb.FCMT_AT + 0x10 + len(mtb)
    fcpa_at += -fcpa_at % 16
    # The chain lists follow the single camera record in the FCIL chunk.
    lists = struct.pack("<4H", 5, 1, 2, 3)
    record = bytearray(ccb.RECORD_SIZE)
    record[0:3] = b"cam"
    struct.pack_into("<4f", record, 0x20, 14.0, 24.89, 0.1, 6000.0)
    record[0x30:0x34] = bytes([1, 2, 1, 0])
    chain_base = 0x10 + ccb.RECORD_SIZE + 0x10
    struct.pack_into("<3I", record, 0x34, chain_base, chain_base + 2, chain_base + 6)
    struct.pack_into("<HH", record, 0x40, 3, 4)
    fchd = b"@FCHD\x00\x01\x00" + bytes(0x28)
    fcco = (
        b"@FCCO\x00"
        + struct.pack("<H", selector)
        + struct.pack("<If", duration, offset)
    )
    fcco += bytes(0x10)
    fcmt = b"@FCMT\x00\x00\x00" + struct.pack("<II", len(mtb), fcpa_at - ccb.FCMT_AT)
    body = fchd + fcco + fcmt + mtb
    body += bytes(fcpa_at - ccb.FCMT_AT - 0x10 - len(mtb))
    fcpa = b"@FCPA\x00" + struct.pack("<H", 1) + bytes(8) + bytes(record)
    fcil = b"@FCIL\x00\x00\x00" + bytes(8) + pad(lists)
    full = body + fcpa + fcil
    head = bytearray(ccb.HEADER_SIZE)
    head[0:8] = b"SEDBCCB\x00"
    struct.pack_into("<HH", head, 0x0C, 0x0401, ccb.HEADER_SIZE)
    struct.pack_into("<I", head, 0x10, ccb.HEADER_SIZE + len(full))
    return bytes(head) + full


def test_parse() -> None:
    data = build_ccb(offset=12.0)
    parsed = ccb.parse_ccb(data, 0)
    check("selector group", parsed["selector"] == 1)
    check("duration ticks", parsed["duration"] == 2_800_000)
    check("frame offset", parsed["offset_frames"] == 12.0)
    check("container size", parsed["size"] == len(data))
    check("one camera", len(parsed["records"]) == 1)
    record = parsed["records"][0]
    check("record name", record["name"] == b"cam")
    check(
        "film gate",
        record["film"]
        == (
            struct.unpack("<f", struct.pack("<f", 14.0))[0],
            struct.unpack("<f", struct.pack("<f", 24.89))[0],
        ),
    )
    check("chain counts", record["counts"] == (1, 2, 1, 0))
    check("chains", record["chains"] == [[5], [1, 2], [3]])
    check(
        "focal and roll groups", (record["focal_group"], record["roll_group"]) == (3, 4)
    )
    names = [[label for label, _ in props] for _, props in parsed["sections"]]
    check("step selector", parsed["sections"][0][1] == [("tx", "step")])
    check("translation names", names[1] == ["tx", "ty", "tz"])
    check("roll is rx", names[4] == ["rx"])
    check("rotation after translation", names[5] == ["tx", "ry"])
    check(
        "curve kinds",
        [kind for _, kind in parsed["sections"][1][1]]
        == ["constant", "constant", "fixed"],
    )


def test_mask_table() -> None:
    labels = dict((bit, text) for bit, text in ccb.MASK_NAMES)
    check("fourteen names", len(ccb.MASK_NAMES) == 14)
    check("translation bits", (labels[1], labels[2], labels[4]) == ("tx", "ty", "tz"))
    check(
        "rotation bits",
        (labels[0x2000], labels[0x4000], labels[0x8000]) == ("rx", "ry", "rz"),
    )
    check(
        "scale bits", (labels[0x10], labels[0x20], labels[0x40]) == ("sx", "sy", "sz")
    )
    check(
        "quaternion bits",
        (labels[0x200], labels[0x400], labels[0x800], labels[0x1000])
        == ("qx", "qy", "qz", "qw"),
    )
    check("qrot bit", labels[0x100] == "qrot")


def test_refusals() -> None:
    data = bytearray(build_ccb())
    for name, offset, value in (
        ("bad magic", 3, b"X"),
        ("bad chunk tag", ccb.FCCO_AT + 1, b"X"),
        ("bad mtb magic", ccb.MTB_AT + 4, b"X"),
    ):
        bad = bytearray(data)
        bad[offset : offset + len(value)] = value
        try:
            ccb.parse_ccb(bytes(bad), 0)
            check(name, False)
        except ccb.CcbError:
            check(name, True)
    scan = ccb.scan_file(b"\x00" * 16 + bytes(data) + b"\x00" * 16 + bytes(data))
    check("scan finds both", len(scan) == 2)
    check(
        "compare reports mismatch",
        ccb.compare({"a": 1}, {"a": 2}) == ["a: expected 2, found 1"],
    )


def main() -> int:
    for test in (test_parse, test_mask_table, test_refusals):
        test()
    for name in FAILED:
        print(f"FAILED {name}", file=sys.stderr)
    print(f"{len(PASSED)} passed, {len(FAILED)} failed")
    return 1 if FAILED else 0


if __name__ == "__main__":
    raise SystemExit(main())
