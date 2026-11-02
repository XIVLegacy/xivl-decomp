#!/usr/bin/env python3
"""Parse the CCB camera containers of an install's cut scenes and check the census figures.

The layout is described in docs/resource/cut-camera-ccb.md. Nothing but counts is kept from the install.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "cut_camera.json"

CCB_MAGIC = b"SEDBCCB\x00"
HEADER_SIZE = 0x30
FCHD_AT, FCCO_AT, FCMT_AT, MTB_AT = 0x30, 0x60, 0x80, 0x90
RECORD_SIZE = 0x50

# Mask bit to property name, in the order of the selector at 0x00a30110. The names are the literals
# that the initializer at 0x00a2fe50 stores into the table at 0x01365160.
MASK_NAMES = (
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


class CcbError(ValueError):
    """Raised for bytes that do not follow the container layout."""


def _u16(data: bytes, at: int) -> int:
    return struct.unpack_from("<H", data, at)[0]


def _u32(data: bytes, at: int) -> int:
    return struct.unpack_from("<I", data, at)[0]


def _f32(data: bytes, at: int) -> float:
    return struct.unpack_from("<f", data, at)[0]


def _cstring(data: bytes, at: int) -> bytes:
    end = data.index(b"\x00", at)
    return data[at:end]


def walk_mtb(
    data: bytes, base: int
) -> tuple[int, list[tuple[int, list[tuple[str, str]]]], list[bytes]]:
    """Walk the Curve sections of the MTB at `base`.

    Returns the Header curve section count, then per Curve section its group index and the
    (property name, curve kind) pairs, then the group names.
    """
    header_size = _u16(data, base + 0x0E)
    cursor = base + header_size
    tables = []
    for _ in range(3):
        count = _u32(data, cursor)
        tables.append([_u32(data, cursor + 4 + 4 * k) for k in range(count)])
        cursor += 4 + 4 * count
    section_names = [_cstring(data, base + at) for at in tables[1]]
    group_names = [_cstring(data, base + at) for at in tables[2]]
    sections = []
    wide = False
    header_sections = 0
    for offset in tables[0]:
        start = base + offset
        name = section_names[_u32(data, start)]
        payload = start + 0x10
        if name == b"Header":
            wide = bool(data[payload + 0x0C] & 8)
            header_sections = _u16(data, payload + 8)
            continue
        if name != b"Curve":
            raise CcbError(f"unexpected section {name!r}")
        group = _u32(data, payload) & 0x7FFFFFFF
        at = payload + (8 if wide else 4)
        count, mask = _u16(data, at), _u16(data, at + 2)
        at += 4
        props = []
        for _ in range(count):
            label = None
            for bit, text in MASK_NAMES:
                if mask & bit:
                    mask &= ~bit
                    label = text
                    break
            if label is None:
                raise CcbError("property named by a table index")
            kind = data[at]
            keys = _u16(data, at + 2)
            if kind == 0:
                size, text = 8, "constant"
            elif kind == 2:
                size, text = 4 + 16 * keys, "fixed"
            elif kind == 4:
                size, text = 4 + 8 * keys, "linear16"
            elif kind == 7:
                size, text = 4 + 8 * keys, "step"
            else:
                raise CcbError(f"curve type {kind}")
            props.append((label, text))
            at += size
        sections.append((group, props))
    return header_sections, sections, group_names


def parse_ccb(data: bytes, at: int) -> dict:
    """Parse the container at `at` and return the checked fields."""
    if data[at : at + 8] != CCB_MAGIC:
        raise CcbError("not a CCB")
    size = _u32(data, at + 0x10)
    if _u16(data, at + 0x0E) != HEADER_SIZE:
        raise CcbError("header size")
    tags = [data[at + off : at + off + 5] for off in (FCHD_AT, FCCO_AT, FCMT_AT)]
    if tags != [b"@FCHD", b"@FCCO", b"@FCMT"]:
        raise CcbError("chunk tags")
    if data[at + MTB_AT : at + MTB_AT + 8] != b"SEDBmtb\x00":
        raise CcbError("embedded mtb")
    selector = _u16(data, at + FCCO_AT + 6)
    duration = _u32(data, at + FCCO_AT + 8)
    offset_frames = _f32(data, at + FCCO_AT + 0x0C)
    mtb_len = _u32(data, at + FCMT_AT + 8)
    fcpa = FCMT_AT + _u32(data, at + FCMT_AT + 0x0C)
    if data[at + fcpa : at + fcpa + 5] != b"@FCPA":
        raise CcbError("FCPA chunk")
    cameras = _u16(data, at + fcpa + 6)
    fcil = fcpa + 0x10 + RECORD_SIZE * cameras
    if data[at + fcil : at + fcil + 5] != b"@FCIL" or fcil + 0x10 > size:
        raise CcbError("FCIL chunk")
    header_sections, sections, group_names = walk_mtb(data, at + MTB_AT)
    records = []
    for k in range(cameras):
        rec = at + fcpa + 0x10 + RECORD_SIZE * k
        counts = tuple(data[rec + 0x30 + j] for j in range(4))
        chains = []
        for j in range(3):
            list_at = at + fcpa + _u32(data, rec + 0x34 + 4 * j)
            if not at + fcil + 0x10 <= list_at <= at + size - 2 * counts[j]:
                raise CcbError("chain list outside the FCIL chunk")
            chains.append([_u16(data, list_at + 2 * i) for i in range(counts[j])])
        records.append(
            {
                "name": _cstring(data, rec),
                "film": (_f32(data, rec + 0x20), _f32(data, rec + 0x24)),
                "near_far": (_f32(data, rec + 0x28), _f32(data, rec + 0x2C)),
                "counts": counts,
                "chains": chains,
                "focal_group": _u16(data, rec + 0x40),
                "roll_group": _u16(data, rec + 0x42),
            }
        )
    for record in records:
        groups = [g for chain in record["chains"] for g in chain]
        groups += [record["focal_group"], record["roll_group"]]
        if any(g >= len(group_names) for g in groups):
            raise CcbError("group index past the name table")
    return {
        "size": size,
        "selector": selector,
        "duration": duration,
        "offset_frames": offset_frames,
        "mtb_len": mtb_len,
        "header_sections": header_sections,
        "sections": sections,
        "records": records,
    }


def scan_file(data: bytes) -> list[dict]:
    parsed = []
    at = data.find(CCB_MAGIC)
    while at >= 0:
        parsed.append(parse_ccb(data, at))
        at = data.find(CCB_MAGIC, at + 8)
    return parsed


def census(root: Path) -> dict:
    cut = root / "client" / "cut"
    scenes = containers = 0
    cameras: Counter = Counter()
    curves: Counter = Counter()
    chain_shapes: Counter = Counter()
    film: Counter = Counter()
    selectors: Counter = Counter()
    ordered = 0
    offsets = 0
    for scene in sorted(cut.iterdir()):
        path = scene / scene.name
        if not path.is_file():
            continue
        scenes += 1
        for ccb in scan_file(path.read_bytes()):
            containers += 1
            cameras[len(ccb["records"])] += 1
            selectors[ccb["selector"]] += 1
            offsets += ccb["offset_frames"] != 0.0
            ordered += [g for g, _ in ccb["sections"]] == list(
                range(len(ccb["sections"]))
            )
            for _, props in ccb["sections"]:
                for _, kind in props:
                    curves[kind] += 1
            for record in ccb["records"]:
                chain_shapes[record["counts"]] += 1
                film[record["film"]] += 1
    return {
        "scenes": scenes,
        "containers": containers,
        "cameras": {str(k): v for k, v in sorted(cameras.items())},
        "curves": dict(sorted(curves.items())),
        "selectors": {str(k): v for k, v in sorted(selectors.items())},
        "ordered_sections": ordered,
        "nonzero_frame_offsets": offsets,
        "film_gates": len(film),
        "top_chain_shape": list(chain_shapes.most_common(1)[0][0]),
    }


def compare(found: dict, expected: dict) -> list[str]:
    return [
        f"{key}: expected {expected[key]!r}, found {found.get(key)!r}"
        for key in expected
        if found.get(key) != expected[key]
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--install", type=Path, required=True, help="retail install root"
    )
    parser.add_argument("--print", action="store_true", help="print the census as JSON")
    args = parser.parse_args()
    found = census(args.install)
    if args.print:
        print(json.dumps(found, indent=2, sort_keys=True))
    expected = json.loads(MANIFEST.read_text(encoding="utf-8"))["census"]
    errors = compare(found, expected)
    for error in errors:
        print(error, file=sys.stderr)
    print("ok" if not errors else "census mismatch")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
