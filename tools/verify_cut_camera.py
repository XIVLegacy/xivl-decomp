#!/usr/bin/env python3
"""Parse the CCB camera containers of an install's cut scenes and check the census figures.

The layout is described in docs/resource/cut-camera-ccb.md. Nothing but counts is kept from the install.
"""

from __future__ import annotations

import argparse
import itertools
import json
import math
import struct
import sys
from collections import Counter
from pathlib import Path

from cut_camera_rig import Rig, rotating_axes
from scb_clip_scheduler import find_scheduler_starts, parse_scheduler

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


def walk_mtb(data: bytes, base: int) -> dict:
    """Walk the Curve sections of the MTB at `base`.

    Returns the Header curve section count and f32 at payload +4, the end of the last name string
    from `base`, then per Curve section its group-name index, the (property name, curve kind)
    pairs and the curve data, then the group names.
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
    name_end = max(
        at + len(text) + 1
        for at, text in zip(tables[1] + tables[2], section_names + group_names)
    )
    sections = []
    curves = []
    wide = False
    header_sections = 0
    header_length = 0.0
    for offset in tables[0]:
        start = base + offset
        name = section_names[_u32(data, start)]
        payload = start + 0x10
        if name == b"Header":
            wide = bool(data[payload + 0x0C] & 8)
            header_sections = _u16(data, payload + 8)
            header_length = _f32(data, payload + 4)
            continue
        if name != b"Curve":
            raise CcbError(f"unexpected section {name!r}")
        group = _u32(data, payload) & 0x7FFFFFFF
        at = payload + (8 if wide else 4)
        count, mask = _u16(data, at), _u16(data, at + 2)
        at += 4
        props = []
        section_curves = []
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
            flag = data[at + 1]
            keys = _u16(data, at + 2)
            if kind == 0:
                size, text = 8, "constant"
                curve = ("constant", _f32(data, at + 4))
            elif kind == 2:
                size, text = 4 + 16 * keys, "fixed"
                curve = (
                    "fixed",
                    [
                        struct.unpack_from("<4f", data, at + 4 + 16 * k)
                        for k in range(keys)
                    ],
                )
            elif kind == 4:
                size, text = 4 + 8 * keys, "linear16"
                curve = (
                    "linear16",
                    [
                        (
                            float(struct.unpack_from("<h", data, at + 8 + 8 * k)[0]),
                            _f32(data, at + 4 + 8 * k),
                        )
                        for k in range(keys)
                    ],
                )
            elif kind == 7:
                size, text = 4 + 8 * keys, "step"
                curve = (
                    "step",
                    flag,
                    [
                        struct.unpack_from("<2f", data, at + 4 + 8 * k)
                        for k in range(keys)
                    ],
                )
            else:
                raise CcbError(f"curve type {kind}")
            props.append((label, text))
            section_curves.append(curve)
            at += size
        sections.append((group, props))
        curves.append(section_curves)
    return {
        "header_sections": header_sections,
        "header_length": header_length,
        "name_end": name_end,
        "sections": sections,
        "curves": curves,
        "group_names": group_names,
    }


def parse_ccb(data: bytes, at: int) -> dict:
    """Parse the container at `at` and return the checked fields.

    Every malformed input raises CcbError.
    """
    try:
        return _parse_ccb(data, at)
    except (struct.error, ValueError, IndexError) as error:
        if isinstance(error, CcbError):
            raise
        raise CcbError(f"truncated or malformed container: {error}") from error


def _parse_ccb(data: bytes, at: int) -> dict:
    if data[at : at + 8] != CCB_MAGIC:
        raise CcbError("not a CCB")
    size = _u32(data, at + 0x10)
    if _u16(data, at + 0x0E) != HEADER_SIZE:
        raise CcbError("header size")
    if size < MTB_AT or at + size > len(data):
        raise CcbError("container size")
    data = data[: at + size]
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
    walked = walk_mtb(data, at + MTB_AT)
    group_names = walked["group_names"]
    sections = walked["sections"]
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
        if any(g >= len(sections) for g in groups):
            raise CcbError("group index past the Curve sections")
    if selector >= len(sections):
        raise CcbError("selector group past the Curve sections")
    return {
        "size": size,
        "selector": selector,
        "duration": duration,
        "offset_frames": offset_frames,
        "mtb_len": mtb_len,
        "mtb_end_aligned": (fcpa == (MTB_AT + mtb_len + 15) // 16 * 16),
        "name_end": walked["name_end"],
        "header_length": walked["header_length"],
        "header_sections": walked["header_sections"],
        "sections": sections,
        "group_curves": [
            ([name for name, _ in props], curves)
            for (_, props), curves in zip(sections, walked["curves"], strict=True)
        ],
        "group_names": group_names,
        "fchd_payload": bytes(data[at + FCHD_AT + 0x10 : at + FCCO_AT]),
        "records": records,
    }


def scan_file(data: bytes) -> list[dict]:
    parsed = []
    at = data.find(CCB_MAGIC)
    while at >= 0:
        parsed.append(parse_ccb(data, at))
        at = data.find(CCB_MAGIC, at + 8)
    return parsed


def _pairing(scheduler, containers: list[dict]) -> tuple[list[int], list]:
    """The sorted distinct LocalCameraClip resource values and the clips that use them."""
    clips = [c for c in scheduler.clips if c.class_name == "LocalCameraClip"]
    values = sorted({_u16(c.record, 0x14) for c in clips})
    return values, clips


def _near_horizontal(point: list[float], actors: list[tuple[float, ...]]) -> float:
    return min(math.hypot(point[0] - a[0], point[2] - a[2]) for a in actors)


def census(root: Path) -> dict:
    """Measure the install. The OBSERVED figures of the page come from here."""
    cut = root / "client" / "cut"
    scenes = containers = 0
    cameras: Counter = Counter()
    curves: Counter = Counter()
    chain_shapes: Counter = Counter()
    film: Counter = Counter()
    selectors: Counter = Counter()
    selector_keys: Counter = Counter()
    selector_values: Counter = Counter()
    selector_curves: Counter = Counter()
    focal: Counter = Counter()
    focal_constants = []
    film_pairs: Counter = Counter()
    near_values: set = set()
    structure: Counter = Counter()
    fchd_patterns: set = set()
    pairing: Counter = Counter()
    durations: Counter = Counter()
    fits: dict[str, list[float]] = {}
    nearer = fit_n = 0
    ordered = 0
    offsets = 0
    entries = sorted(cut.iterdir())
    for position, scene in enumerate(entries):
        path = scene / scene.name
        if not path.is_file():
            continue
        scenes += 1
        data = path.read_bytes()
        found = scan_file(data)
        for ccb in found:
            containers += 1
            cameras[len(ccb["records"])] += 1
            selectors[ccb["selector"]] += 1
            offsets += ccb["offset_frames"] != 0.0
            ordered += [g for g, _ in ccb["sections"]] == list(
                range(len(ccb["sections"]))
            )
            structure["sections_equal_names"] += len(ccb["sections"]) == len(
                ccb["group_names"]
            )
            ticks = round(_f32_round(ccb["header_length"]) * 10000)
            structure["duration_differs_from_header"] += ticks != ccb["duration"]
            structure["mtb_length_differs_from_names"] += (
                ccb["mtb_len"] != ccb["name_end"]
            )
            structure["fcpa_not_after_mtb_boundary"] += not ccb["mtb_end_aligned"]
            if any(ccb["fchd_payload"]):
                structure["fchd_payload_nonzero"] += 1
                fchd_patterns.add(ccb["fchd_payload"])
            names, group = ccb["group_curves"][ccb["selector"]]
            selector_curves[(tuple(names), group[0][0])] += 1
            selector_keys[len(group[0][2])] += 1
            for _, value in group[0][2]:
                selector_values[value] += 1
                structure["selector_value_not_a_record"] += not (
                    0 <= math.trunc(value) < len(ccb["records"])
                )
            for _, props in ccb["sections"]:
                for _, kind in props:
                    curves[kind] += 1
            for record in ccb["records"]:
                chain_shapes[record["counts"]] += 1
                film[record["film"]] += 1
                film_pairs[tuple(round(x, 3) for x in record["film"])] += 1
                near_values.add(round(record["near_far"][0], 4))
                names, group = ccb["group_curves"][record["focal_group"]]
                if "ty" not in names:
                    focal["missing"] += 1
                    continue
                kind = group[names.index("ty")]
                focal[kind[0]] += 1
                if kind[0] == "constant":
                    focal_constants.append(kind[1])
        schedulers = find_scheduler_starts(data)
        if not schedulers:
            continue
        sched = parse_scheduler(data[schedulers[0] :], schedulers[0])
        pairing["schedulers"] += 1
        values, clips = _pairing(sched, found)
        pairing["with_local_camera"] += bool(clips)
        if len(values) != len(found):
            pairing["refused"] += 1
            pairing["refused_with_local_camera"] += bool(clips)
            continue
        if not clips:
            continue
        pairing["paired"] += 1
        multi = len(found) > 1
        pairing["paired_multi_container"] += multi
        for clip in clips:
            start, end = _u32(clip.record, 4), _u32(clip.record, 0x10)
            mapped = found[values.index(_u16(clip.record, 0x14))]["duration"]
            tag = ("start_above_0" if start > 0 else "start_0") + (
                "_multi" if multi else "_single"
            )
            durations[tag] += 1
            hit_span = end - start == mapped
            hit_end = end == mapped
            durations[tag + "_end_minus_start"] += hit_span
            durations[tag + "_end"] += hit_end
            if not (hit_span or hit_end):
                other = any(
                    end - start == c["duration"] or end == c["duration"] for c in found
                )
                durations[tag + ("_other_container" if other else "_no_container")] += 1
        if position % 2:
            continue
        rigs = [Rig(c) for c in found]
        for block in sorted({c.block for c in sched.clips}):
            actors = [
                struct.unpack_from("<3f", c.record, 0x10)
                for c in sched.clips
                if c.block == block and c.class_name == "SetPosClip"
            ]
            before = [
                struct.unpack_from("<3f", c.record, 0x10)
                for c in sched.clips
                if c.block == block - 1 and c.class_name == "SetPosClip"
            ]
            if not actors:
                continue
            for clip in sched.clips:
                if clip.block != block or clip.class_name != "LocalCameraClip":
                    continue
                rig = rigs[values.index(_u16(clip.record, 0x14))]
                frame = rig.frame_of(0)
                camera = rig.selected_camera(frame)
                if rotating_axes(rig, camera) < 2:
                    continue
                fit_n += 1
                here = rig.points(camera, frame)
                fits.setdefault("interest", []).append(
                    _near_horizontal(here["interest"], actors)
                )
                fits.setdefault("eye", []).append(_near_horizontal(here["eye"], actors))
                fits.setdefault("chain_a_only", []).append(
                    _near_horizontal(here["world"], actors)
                )
                nearer += _near_horizontal(here["interest"], actors) < (
                    _near_horizontal(here["eye"], actors)
                )
                if before:
                    fits.setdefault("interest_previous_block", []).append(
                        _near_horizontal(here["interest"], before)
                    )
                fits.setdefault("interest_no_rotation", []).append(
                    _near_horizontal(
                        rig.points(camera, frame, rotate=False)["interest"], actors
                    )
                )
                for order in itertools.permutations(range(3)):
                    name = "".join("xyz"[axis] for axis in order)
                    for sign, label in ((1.0, "order_"), (-1.0, "negated_order_")):
                        fits.setdefault(label + name, []).append(
                            _near_horizontal(
                                rig.points(camera, frame, order=order, sign=sign)[
                                    "interest"
                                ],
                                actors,
                            )
                        )
    found_fit = {
        key: round(sorted(values)[len(values) // 2], 3)
        for key, values in sorted(fits.items())
    }
    return {
        "scenes": scenes,
        "containers": containers,
        "cameras": {str(k): v for k, v in sorted(cameras.items())},
        "curves": dict(sorted(curves.items())),
        "selectors": {str(k): v for k, v in sorted(selectors.items())},
        "selector_curves": {
            f"{'+'.join(names)}:{kind}": v
            for (names, kind), v in selector_curves.items()
        },
        "selector_keys": {str(k): v for k, v in sorted(selector_keys.items())},
        "selector_values": {str(int(k)): v for k, v in sorted(selector_values.items())},
        "focal_curves": dict(sorted(focal.items())),
        "focal_constant_range": [min(focal_constants), max(focal_constants)],
        "film_pairs": {f"{h}x{w}": v for (h, w), v in sorted(film_pairs.items())},
        "near_values": sorted(near_values),
        "structure": dict(sorted(structure.items())),
        "fchd_payload_patterns": len(fchd_patterns),
        "ordered_sections": ordered,
        "nonzero_frame_offsets": offsets,
        "film_gates": len(film),
        "top_chain_shape": list(chain_shapes.most_common(1)[0][0]),
        "pairing": dict(sorted(pairing.items())),
        "durations": dict(sorted(durations.items())),
        "fit_shots": fit_n,
        "fit_interest_nearer_than_eye": nearer,
        "fit_medians": found_fit,
    }


def _f32_round(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def compare(found: dict, expected: dict) -> list[str]:
    """Differences from the manifest. Fit medians match to 0.005, everything else exactly."""
    errors = []
    for key in expected:
        if key == "fit_medians":
            for name, value in expected[key].items():
                got = found.get(key, {}).get(name)
                if got is None or abs(got - value) > 0.005:
                    errors.append(f"{key}.{name}: expected {value!r}, found {got!r}")
        elif found.get(key) != expected[key]:
            errors.append(
                f"{key}: expected {expected[key]!r}, found {found.get(key)!r}"
            )
    return errors


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
