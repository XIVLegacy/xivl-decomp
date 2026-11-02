#!/usr/bin/env python3
"""Tests for the CCB camera container parser on a synthetic container."""

from __future__ import annotations

import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import cut_camera_rig as rig  # noqa: E402
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
    selector: int = 1,
    duration: int = 2_800_000,
    offset: float = 0.0,
    selector_value: float = 0.0,
) -> bytes:
    groups = [b"cam_weight", b"cam", b"cam_interest", b"cam_flen", b"cam_rol", b"root"]
    sections = [
        curve_section(0, 1, [step([(0.0, selector_value)])]),
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


# Independent of ccb.MASK_NAMES. The bits in the order that the selector at 0x00a30110 tests them,
# with the name the initializer at 0x00a2fe50 stores for the branch that clears the bit.
SELECTOR_ORDER = (
    (0x0001, "tx"),
    (0x0002, "ty"),
    (0x0004, "tz"),
    (0x2000, "rx"),
    (0x4000, "ry"),
    (0x8000, "rz"),
    (0x0010, "sx"),
    (0x0020, "sy"),
    (0x0040, "sz"),
    (0x0100, "qrot"),
    (0x0200, "qx"),
    (0x0400, "qy"),
    (0x0800, "qz"),
    (0x1000, "qw"),
)


def names_of(mask: int, count: int) -> list[str]:
    """The names that the walk gives to a Curve section with this mask and count constants."""
    section = curve_section(0, mask, [constant(float(i)) for i in range(count)])
    mtb = build_mtb([b"group"], [section], 1)
    _, props = ccb.walk_mtb(mtb, 0)["sections"][0]
    return [name for name, _ in props]


def test_mask_table() -> None:
    check("selector order and names", tuple(ccb.MASK_NAMES) == SELECTOR_ORDER)
    for bit, text in SELECTOR_ORDER:
        check(f"single bit {bit:#06x}", names_of(bit, 1) == [text])
    # The order of the tests decides which name a property gets, not the numeric order.
    check("qrot before qx", names_of(0x0300, 2) == ["qrot", "qx"])
    check("rx before sx", names_of(0x2010, 2) == ["rx", "sx"])
    check("rz before sx", names_of(0x8010, 2) == ["rz", "sx"])
    check("translation then rotation", names_of(0x2005, 3) == ["tx", "tz", "rx"])
    check(
        "all fourteen",
        names_of(0xFFFF & ~0x88, 14) == [text for _, text in SELECTOR_ORDER],
    )


def refused(data: bytes, length: int | None = None) -> bool:
    """Whether the parser refuses the bytes with CcbError and with no other exception."""
    try:
        ccb.parse_ccb(data if length is None else data[:length], 0)
    except ccb.CcbError:
        return True
    except Exception:
        return False
    return False


def test_refusals() -> None:
    data = bytearray(build_ccb())
    for name, offset, value in (
        ("bad magic", 3, b"X"),
        ("bad chunk tag", ccb.FCCO_AT + 1, b"X"),
        ("bad mtb magic", ccb.MTB_AT + 4, b"X"),
    ):
        bad = bytearray(data)
        bad[offset : offset + len(value)] = value
        check(name, refused(bytes(bad)))
    # Every prefix of the container is refused with CcbError, never a struct or index error.
    check(
        "every truncation is a CcbError",
        all(refused(bytes(data), n) for n in range(0, len(data), 7)),
    )
    short = bytearray(data)
    struct.pack_into("<I", short, 0x10, len(data) - 40)
    check("declared size too small", refused(bytes(short)))
    fcpa = ccb.FCMT_AT + struct.unpack_from("<I", data, ccb.FCMT_AT + 0x0C)[0]
    record = fcpa + 0x10
    for name, at, value in (
        ("chain list past the container", record + 0x34, struct.pack("<I", 0x7FFF)),
        ("chain list inside the records", record + 0x34, struct.pack("<I", 0x10)),
        ("focal group past the sections", record + 0x40, struct.pack("<H", 99)),
        ("roll group past the sections", record + 0x42, struct.pack("<H", 99)),
        ("selector group past the sections", ccb.FCCO_AT + 6, struct.pack("<H", 99)),
    ):
        bad = bytearray(data)
        bad[at : at + len(value)] = value
        check(name, refused(bytes(bad)))
    check("the unpatched container parses", not refused(bytes(data)))
    scan = ccb.scan_file(b"\x00" * 16 + bytes(data) + b"\x00" * 16 + bytes(data))
    check("scan finds both", len(scan) == 2)
    check(
        "compare reports mismatch",
        ccb.compare({"a": 1}, {"a": 2}) == ["a: expected 2, found 1"],
    )
    near = {"fit_medians": {"x": 2.455}}
    check(
        "fit median within 0.005",
        ccb.compare(near, {"fit_medians": {"x": 2.459}}) == [],
    )
    check(
        "fit median beyond 0.005",
        len(ccb.compare(near, {"fit_medians": {"x": 2.47}})) == 1,
    )


def test_sampling() -> None:
    keys = [(0.0, 1.0), (10.0, 2.0), (20.0, 3.0)]
    check("step before the first key", rig.sample_step(0, keys, -5.0) == 1.0)
    check("step holds", rig.sample_step(0, keys, 15.0) == 2.0)
    check("step after the last key", rig.sample_step(0, keys, 99.0) == 3.0)
    check("step wraps past the last time", rig.sample_step(1, keys, 35.0) == 2.0)
    check("step wraps several times", rig.sample_step(1, keys, 95.0) == 2.0)
    check("step does not wrap a negative", rig.sample_step(1, keys, -25.0) == 1.0)
    check(
        "step wrap with a tiny last time",
        rig.sample_step(1, [(0.0, 1.0), (0.001, 2.0)], 200000.0) in (1.0, 2.0),
    )
    fixed_keys = [(0.0, 0.0, 0.0, 0.0), (10.0, 10.0, 0.0, 0.0), (20.0, 0.0, 0.0, 0.0)]
    check("fixed at a key", rig.sample_fixed(fixed_keys, 10.0) == 10.0)
    check("fixed midpoint", abs(rig.sample_fixed(fixed_keys, 5.0) - 5.0) < 1e-9)
    check(
        "linear16 midpoint", rig.sample_linear16([(0.0, 0.0), (4.0, 8.0)], 1.0) == 2.0
    )


def selected(value: float) -> int:
    parsed = ccb.parse_ccb(build_ccb(selector=0, selector_value=value), 0)
    return rig.Rig(parsed).selected_camera(0.0)


def test_rig() -> None:
    camera = rig.Rig(ccb.parse_ccb(build_ccb(offset=12.0), 0))
    # 0x009d6600 truncates toward zero.
    for value in (0.0, 0.6, 0.9, -0.6, -0.99):
        check(f"selector {value} picks record 0", selected(value) == 0)
    for value in (1.0, 1.4, -1.0, -1.5):
        try:
            selected(value)
            check(f"selector {value} is outside", False)
        except ValueError:
            check(f"selector {value} is outside", True)
    # 16,777,217 is not representable in single precision and rounds to 16,777,216.
    check(
        "ticks are rounded to single precision first",
        camera.frame_of(16_777_217) == camera.frame_of(16_777_216),
    )
    check("frame offset", camera.frame_of(0) == 12.0)
    check("one frame is 10000 ticks", camera.frame_of(10_000) == 13.0)
    # The 8 bytes at 0x00fef0c0 are the single precision pi/180 widened, not the nearest double.
    check(
        "degrees constant is the widened single",
        rig.DEGREES_TO_RADIANS == rig.f32(math.pi / 180.0),
    )
    check(
        "degrees constant is not the double", rig.DEGREES_TO_RADIANS != math.pi / 180.0
    )
    points = camera.points(0, 0.0)
    check("rig has a world row", all(math.isfinite(x) for x in points["world"]))


def main() -> int:
    for test in (
        test_parse,
        test_mask_table,
        test_refusals,
        test_sampling,
        test_rig,
    ):
        test()
    for name in FAILED:
        print(f"FAILED {name}", file=sys.stderr)
    print(f"{len(PASSED)} passed, {len(FAILED)} failed")
    return 1 if FAILED else 0


if __name__ == "__main__":
    raise SystemExit(main())
