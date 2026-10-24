#!/usr/bin/env python3
"""Tests for the timer handler, player profile, sun and moon, camera and status static checks."""

from __future__ import annotations

import copy
import json
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_camera_input_chain as camera  # noqa: E402
import verify_map_0003_emitter as map_emitter  # noqa: E402
import verify_player_profile_accessors as profile  # noqa: E402
import verify_s2c_0193_native_state as timer  # noqa: E402
import verify_status_wire_translation as status  # noqa: E402
import verify_sun_moon_orientation as moon  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def check_bytes(item: dict) -> bytes:
    """The bytes a manifest row expects at its address."""
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "dwords":
        width = item.get("width", 4)
        return b"".join(int(v, 16).to_bytes(width, "little") for v in item["values"])
    pattern, mask = bytes.fromhex(item["pattern"]), bytes.fromhex(item["mask"])
    return bytes(p if m else 0 for p, m in zip(pattern, mask))


def build_image(rows: list[tuple[int, bytes]]) -> bytes:
    """A PE32 image with one section per row, each placed at its own VA."""
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(rows) + 0xFFF) & ~0xFFF
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(rows))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
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


def first_raw(count: int) -> int:
    return (0x80 + 24 + 0xE0 + 40 * count + 0xFFF) & ~0xFFF


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
        start = first_raw(len(rows)) + sum(len(d) for _, d in rows[:index])
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


def row_float(document: dict, name: str, fmt: str, index: int = 0) -> float:
    """Decode a constant from a manifest row by name."""
    item = next(i for i in document["staticChecks"] if i["name"] == name)
    raw = check_bytes(item)
    width = struct.calcsize(fmt)
    return struct.unpack_from(fmt, raw, index * width)[0]


def camera_checks() -> None:
    document = json.loads(camera.MANIFEST.read_text(encoding="ascii"))

    def near(a: float, b: float, tol: float = 1e-5) -> bool:
        return abs(a - b) <= tol

    check(
        "camera model constants equal the manifest rows",
        row_float(document, "radius default 4.5", "<f") == camera.RADIUS_DEFAULT
        and row_float(document, "radius clamp bounds", "<f", 0) == camera.RADIUS_MAX
        and row_float(document, "radius clamp bounds", "<f", 1) == camera.RADIUS_MIN
        and row_float(document, "constant 500.0", "<d") == camera.THIRD_PERSON_DIVISOR
        and row_float(document, "constant 1000.0", "<d") == camera.FIRST_PERSON_DIVISOR
        and near(row_float(document, "constant 0.2", "<d"), 0.2)
        and near(row_float(document, "constant 0.15", "<d"), 0.15)
        and near(row_float(document, "constant 0.1", "<d"), 0.1)
        and row_float(document, "constant 100.0", "<d") == 100.0
        and row_float(document, "pitch limit floats", "<f", 1)
        == camera.PITCH_LIMIT_THIRD
        and row_float(document, "first-person pitch limits", "<f", 1)
        == camera.PITCH_LIMIT_FIRST
        and row_float(document, "sensor constant", "<d") == camera.SENSOR
        and row_float(document, "focal lower double", "<d") == camera.FOCAL_MIN
        and row_float(document, "focal upper double", "<d") == camera.FOCAL_MAX
        and row_float(document, "first-person fov base", "<d")
        == camera.FIRST_PERSON_FOV_BASE
        and row_float(document, "first-person fov coefficients", "<d")
        == camera.FIRST_PERSON_FOV_SCALE
        and row_float(document, "constant 30.0 float", "<f") == camera.BLEND_START
        and row_float(document, "constant 30.0 double", "<d") == camera.DT_NUMERATOR
        and row_float(document, "constant 300000.0 double", "<d")
        == camera.DT_DENOMINATOR
        and row_float(document, "projection near", "<f") == 0.25
        and row_float(document, "projection far", "<f") == 10000.0,
    )

    check("one notch is 200", camera.wheel_payload(1) == 200)
    check("a notch away is -200", camera.wheel_payload(-1) == -200)
    check("the payload truncates to 16 bits", camera.wheel_payload(165) == -32536)
    radius, flag = camera.third_person_zoom(4.5, camera.wheel_payload(1))
    check("one notch moves the radius by 0.4", near(radius, 4.1) and not flag)
    radius = camera.RADIUS_DEFAULT
    flags = []
    for _ in range(9):
        radius, flag = camera.third_person_zoom(radius, camera.wheel_payload(1))
        flags.append(flag)
    check(
        "the eighth notch reaches 1.5 and the ninth raises the flag",
        near(radius, 1.5) and flags == [False] * 8 + [True],
    )
    check(
        "zooming out stops at 10.0 without the flag",
        camera.third_person_zoom(9.9, camera.wheel_payload(-1)) == (10.0, False),
    )
    check(
        "a negative notch at the floor does not raise the flag",
        camera.third_person_zoom(1.5, camera.wheel_payload(-1))[1] is False,
    )
    zoom, flag = camera.first_person_zoom(0.75, camera.wheel_payload(-1))
    check("first person moves by 0.2 per notch", near(zoom, 0.95) and not flag)
    check(
        "first person flips back at 1.0 on a negative notch",
        camera.first_person_zoom(1.0, camera.wheel_payload(-1)) == (1.0, True),
    )
    check(
        "first person clamps at 0",
        camera.first_person_zoom(0.1, camera.wheel_payload(1))[0] == 0.0,
    )

    pitch, yaw = camera.third_person_drag(0.0, 0.0, 100, 100)
    check("100 cursor units give 0.2 radians", near(pitch, 0.2) and near(yaw, 0.2))
    pitch, yaw = camera.third_person_drag(0.0, 0.0, 1, 1)
    check(
        "one cursor unit gives 0.002 radians", near(pitch, 0.002) and near(yaw, 0.002)
    )
    check(
        "pitch stops at 45 degrees",
        camera.third_person_drag(0.7, 0.0, 0, 1000)[0] == camera.PITCH_LIMIT_THIRD
        and camera.third_person_drag(-0.7, 0.0, 0, -1000)[0]
        == -camera.PITCH_LIMIT_THIRD,
    )
    check(
        "yaw wraps into -pi to pi",
        near(camera.third_person_drag(0.0, 3.0, 100, 0)[1], 3.2 - 2 * math.pi)
        and near(camera.third_person_drag(0.0, -3.0, -100, 0)[1], -3.2 + 2 * math.pi),
    )
    check(
        "invert bit 8 flips dx and bit 4 flips dy",
        camera.third_person_drag(0.0, 0.0, 100, 100, 1.0, 8)[1] < 0
        and camera.third_person_drag(0.0, 0.0, 100, 100, 1.0, 8)[0] > 0
        and camera.third_person_drag(0.0, 0.0, 100, 100, 1.0, 4)[0] < 0
        and camera.third_person_drag(0.0, 0.0, 100, 100, 1.0, 4)[1] > 0,
    )
    check(
        "the turn flag blocks the yaw orbit and keeps the pitch",
        camera.third_person_drag(0.0, 1.0, 100, 100, 1.0, 0, True)
        == (camera.third_person_drag(0.0, 1.0, 0, 100)[0], 1.0)
        and near(camera.third_person_drag(0.0, 1.0, 100, 0, 1.0, 0, False)[1], 1.2),
    )
    check(
        "sensitivity scales the rate",
        near(camera.third_person_drag(0.0, 0.0, 100, 0, 2.0)[1], 0.4),
    )

    check(
        "focal 18 and 28 field of view",
        near(camera.lens_fov(18), 0.7937254) and near(camera.lens_fov(28), 0.5263450),
    )
    check(
        "the focal byte is limited to 18..28",
        camera.lens_fov(10) == camera.lens_fov(18)
        and camera.lens_fov(40) == camera.lens_fov(28),
    )
    check(
        "first person field of view spans 60 to 85 degrees in the projection",
        near(camera.projection_fov(camera.first_person_fov(0.0)), math.pi / 3)
        and near(
            math.degrees(camera.projection_fov(camera.first_person_fov(1.0))),
            85.0,
            1e-4,
        ),
    )
    check(
        "default third-person projection field of view",
        near(camera.projection_fov(camera.lens_fov(18)), 1.3173242),
    )
    check("dt from elapsed", camera.dt_from_elapsed(300000) == 30.0)
    check(
        "blend weight falls linearly and floors at zero",
        camera.blend_step(30.0, 15.0) == (15.0, 0.5)
        and camera.blend_step(10.0, 40.0) == (0.0, 0.0)
        and camera.blend_step(0.0, 1.0) == (0.0, 0.0),
    )

    check(
        "lock polarity maps keys both ways",
        [camera.normalize_key(k, True) for k in (4, 5, 6, 7, 0, 3)]
        == [0, 1, 2, 3, 0, 3]
        and [camera.normalize_key(k, False) for k in (0, 1, 2, 3, 4)]
        == [4, 5, 6, 7, 4],
    )
    check(
        "the remap predicate removes first person",
        [camera.normalize_key(k, True, remap=True) for k in range(8)]
        == [0, 0, 2, 2, 0, 0, 2, 2],
    )
    check(
        "the fps-forbidden bit blocks keys 1, 3, 5, 7 only",
        all(
            camera.normalize_key(k, True, fps_forbidden=True) is None
            for k in (1, 3, 5, 7)
        )
        and camera.normalize_key(2, True, fps_forbidden=True) == 2,
    )
    check(
        "family toggle",
        [camera.family_toggle(k, True) for k in range(4)] == [1, 0, 3, 2]
        and [camera.family_toggle(k, False) for k in range(4)] == [5, 4, 7, 6]
        and camera.family_toggle(9, True) is None,
    )
    check(
        "a pointer push requests key 2 or 3",
        [camera.pointer_request_key(k) for k in range(9)]
        == [2, 3, 2, 3, 2, 3, 2, 3, None],
    )
    check(
        "a move payload requests key 0 or 1",
        [camera.move_request_key(k) for k in range(8)] == [0, 1, 0, 1, 0, 1, 0, 1],
    )
    check(
        "a fresh session starts on key 0 or 4",
        camera.initial_key(True) == 0 and camera.initial_key(False) == 4,
    )
    check(
        "update arms by key",
        [camera.update_arm(k) for k in range(12)]
        == ["single", "single", "tick", "tick", "single", "single", "tick", "tick"]
        + ["single", "single", "tick", "single"],
    )
    check(
        "wheel step equals payload scale over divisor",
        near(
            camera.WHEEL_SCALE / camera.THIRD_PERSON_DIVISOR,
            document["zoom"]["thirdPerson"]["stepPerNotch"],
            1e-12,
        ),
    )


def status_checks() -> None:
    document = json.loads(status.MANIFEST.read_text(encoding="ascii"))
    row = next(
        i for i in document["staticChecks"] if i["name"] == "translation sequence"
    )
    code = bytes.fromhex(row["pattern"])

    def minus_key(wire: int) -> int:
        return 200000 + wire - (0x4350 if wire > 0x8000 else 0)

    check(
        "the manifest translation bytes reproduce the model at every edge",
        all(
            status.run_translation(code, wire) == key
            for wire, key in status.EDGES
            if wire
        ),
    )
    check(
        "the add in the translation bytes is load bearing",
        status.run_translation(code[:-2] + b"\x2b\xc1", 0x8001) != 250001
        and status.run_translation(code[:-2] + b"\x2b\xc1", 0x0001) != 200001,
    )
    check(
        "the 0x8000 floor in the translation bytes is load bearing",
        code[:2] == b"\x66\xba"
        and status.run_translation(b"\x66\xba\xff\x7f" + code[4:], 0x8000)
        != status.status_key(0x8000),
    )
    check(
        "the strict comparison sends 0x8000 to the low branch",
        status.status_key(0x8000) == 232768 and status.status_key(0x8001) == 250001,
    )
    check(
        "the zero id has no key and a wider id is rejected",
        status.status_key(0) is None
        and all(
            _raises(lambda value=value: status.status_key(value))
            for value in (-1, 0x10000)
        ),
    )
    keys = {wire: status.status_key(wire) for wire in range(1, 0x10000)}
    check(
        "no two wire ids share a key and every key round trips",
        len(set(keys.values())) == len(keys)
        and all(status.wire_for_key(key) == wire for wire, key in keys.items()),
    )
    check(
        "the images are 200001 to 232768 and 250001 to 282767",
        min(keys.values()) == 200001
        and max(keys.values()) == 282767
        and keys[0x8000] == 232768
        and not any(232768 < key < 250001 for key in keys.values()),
    )
    check(
        "the three observed joins hold under both forms",
        all(
            status.status_key(wire) == minus_key(wire) == row_id
            for wire, row_id in ((0x5A59, 223129), (0x5ADF, 223263), (0x5DBF, 223999))
        ),
    )
    check(
        "the four high sheet rows have wire ids only under the added form",
        all(
            keys[wire] == row_id
            for row_id, wire in zip(
                status.SHEET_HIGH_ROWS, (0x83E9, 0x8BB9, 0x8BBA, 0x8BBB), strict=True
            )
        )
        and not {251001, 253001, 253002, 253003}
        & {minus_key(wire) for wire in range(1, 0x10000)},
    )
    check(
        "the subtracting form puts 0x9DA9 on a different row than the added form",
        minus_key(0x9DA9) == 223129 and status.status_key(0x9DA9) == 257593,
    )
    check(
        "the sheet low rows lie in the low image",
        all(
            status.wire_for_key(key) == key - 200000
            for key in (
                status.SHEET_LOW_ROWS["min"],
                status.SHEET_LOW_ROWS["max"],
            )
        ),
    )
    check(
        "keys in the gap have no wire id",
        status.wire_for_key(232769) is None
        and status.wire_for_key(250000) is None
        and status.wire_for_key(200000) is None
        and status.wire_for_key(282768) is None,
    )


def _raises(call) -> bool:
    try:
        call()
    except ValueError:
        return True
    return False


def map_emitter_checks() -> None:
    document = json.loads(map_emitter.MANIFEST.read_text(encoding="ascii"))
    check("Map emitter catalog passes", map_emitter.verify(document) == [])
    geometry = document["geometry"]
    check(
        "Map emitter uses actual application geometry",
        (
            geometry["internalGameMessageBytes"],
            geometry["gameHeaderBytes"],
            geometry["applicationBytes"],
            geometry["normalizedViewBytes"],
            geometry["recordApplicationOffset"],
            geometry["dataApplicationOffset"],
        )
        == (560, 16, 544, 552, 24, 16),
    )
    check(
        "Map emitter application partition is literal and complete",
        [(row["offset"], row["bytes"]) for row in document["fields"]]
        == [
            (0, 4),
            (4, 4),
            (8, 4),
            (12, 4),
            (16, 4),
            (20, 4),
            (24, 2),
            (26, 2),
            (28, 512),
            (540, 4),
        ]
        and document["fields"][-1]["value"] is None
        and document["fields"][-1]["writeVas"] == [],
    )
    check(
        "Map normalized offsets retain eight header bytes",
        (
            geometry["normalizedViewApplicationOffset"],
            geometry["normalizedSelectorOffset"],
            geometry["normalizedTextOffset"],
            geometry["normalizedTailOffset"],
        )
        == (8, 32, 36, 548),
    )
    selector = document["selector"]
    check(
        "Map selector comes from W while literal one is cleaned by the helper",
        (
            selector["callerArgument"],
            selector["emitterArgument"],
            selector["bytes"],
            selector["floatHelperCleanupBytes"],
        )
        == (4, 5, 2, 4)
        and selector["source"] == "low16(W)"
        and selector["fixedValue"] is None
        and selector["upstreamValueOneInvocationProven"] is False,
    )

    for section, key, value, expected in [
        ("geometry", "applicationBytes", 552, "geometry contract changed"),
        ("geometry", "recordApplicationOffset", 16, "geometry contract changed"),
        ("geometry", "internalGameMessageBytes", 559, "geometry contract changed"),
        ("selector", "fixedValue", 1, "selector contract changed"),
        ("selector", "callerArgument", 3, "selector contract changed"),
        ("selector", "source", "float-helper-literal", "selector contract changed"),
        ("selector", "floatHelperCleanupBytes", 0, "selector contract changed"),
        (
            "selector",
            "upstreamValueOneInvocationProven",
            True,
            "selector contract changed",
        ),
        ("localResult", "deliveryProven", True, "localResult contract changed"),
        ("localResult", "serverAcceptanceProven", True, "localResult contract changed"),
        ("caller", "contextMeaning", "invented identity", "caller contract changed"),
        ("caller", "textPolicy", "unchanged input", "caller contract changed"),
    ]:
        changed = copy.deepcopy(document)
        changed[section][key] = value
        check(f"Map rejects {section}.{key}", map_emitter.verify(changed) == [expected])
    for label, change, expected in [
        (
            "tail zero",
            lambda d: d["fields"][-1].update(value=0),
            "fields contract changed",
        ),
        (
            "dropped field",
            lambda d: d["fields"].pop(2),
            "application fields do not form an ordered partition",
        ),
        (
            "overlap",
            lambda d: d["fields"][1].update(offset=3),
            "application fields do not form an ordered partition",
        ),
        (
            "shortened text copy",
            lambda d: d["fields"][8].update(bytes=511),
            "application fields do not form an ordered partition",
        ),
        (
            "shifted store anchor",
            lambda d: d["fields"][0].update(writeVas=["0x004e040a"]),
            "fields contract changed",
        ),
        (
            "selector width",
            lambda d: d["fields"][6].update(bytes=4),
            "fields contract changed",
        ),
        (
            "missing body",
            lambda d: d["bodyRanges"].pop(),
            "bodyRanges contract changed",
        ),
        (
            "duplicate body",
            lambda d: d["bodyRanges"].append(copy.deepcopy(d["bodyRanges"][0])),
            "body range names must be nonempty and unique",
        ),
        (
            "body boundary",
            lambda d: d["bodyRanges"][0].update(endVa="0x004e04d5"),
            "bodyRanges contract changed",
        ),
        (
            "bad digest",
            lambda d: d["bodyRanges"][0].update(sha256="invalid"),
            "body range digest is malformed",
        ),
        (
            "extra claim",
            lambda d: d.update(unprovedFact="invented"),
            "catalog sections changed",
        ),
    ]:
        changed = copy.deepcopy(document)
        change(changed)
        check(
            f"Map rejects {label} for its own reason",
            expected in map_emitter.verify(changed),
        )

    # Authored text fixture and literal SHA256, independent of retail catalog ranges.
    fixture = {
        "bodyRanges": [
            {
                "name": "authored-text",
                "startVa": "0x00401000",
                "endVa": "0x00401003",
                "sha256": "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            }
        ]
    }

    def fixture_image(rows: list[tuple[int, bytes]]) -> bytes:
        data = bytearray(build_image(rows))
        struct.pack_into("<H", data, 0x84, 0x14C)
        struct.pack_into("<H", data, 0x98, 0x10B)
        struct.pack_into("<I", data, 0xB4, 0x00400000)
        return bytes(data)

    image = fixture_image([(0x00401000, b"abc")])
    check(
        "Map authored fixture passes range checks",
        map_emitter.verify_image(image, fixture, require_identity=False) == [],
    )
    check(
        "Map image identity is enforced by default",
        map_emitter.verify_image(image, fixture)
        == ["executable size changed", "executable SHA256 changed"],
    )
    flipped = bytearray(image)
    flipped[first_raw(1)] ^= 1
    check(
        "Map changed payload fails the digest",
        map_emitter.verify_image(bytes(flipped), fixture, require_identity=False)
        == ["range authored-text digest changed"],
    )
    shifted = copy.deepcopy(fixture)
    shifted["bodyRanges"][0].update(startVa="0x00401001", endVa="0x00401004")
    check(
        "Map shifted VA fails backing",
        map_emitter.verify_image(image, shifted, require_identity=False)
        == ["range authored-text is not uniquely file-backed"],
    )
    bad_digest = copy.deepcopy(fixture)
    bad_digest["bodyRanges"][0]["sha256"] = "0" * 64
    check(
        "Map independent bad expected digest fails",
        map_emitter.verify_image(image, bad_digest, require_identity=False)
        == ["range authored-text digest changed"],
    )
    table = 0x178
    virtual_tail = bytearray(image)
    struct.pack_into("<I", virtual_tail, table + 8, 4)
    tail_range = {
        "bodyRanges": [
            {
                "name": "virtual-zero",
                "startVa": "0x00401003",
                "endVa": "0x00401004",
                "sha256": "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d",
            }
        ]
    }
    check(
        "Map virtual zeros are not file-backed evidence",
        map_emitter.verify_image(
            bytes(virtual_tail), tail_range, require_identity=False
        )
        == ["range virtual-zero is not uniquely file-backed"],
    )
    outside = bytearray(image)
    struct.pack_into("<I", outside, table + 16, 4)
    check(
        "Map raw extent beyond EOF fails",
        map_emitter.verify_image(bytes(outside), fixture, require_identity=False)
        == ["image is not a readable PE32: section raw range is outside the file"],
    )
    check(
        "Map truncated section table fails",
        map_emitter.verify_image(image[:0x190], fixture, require_identity=False)
        == ["image is not a readable PE32: section table is empty or truncated"],
    )
    for label, offset, fmt, value, reason in [
        ("MZ", 0, "<H", 0, "missing MZ signature"),
        ("PE signature", 0x80, "<I", 0, "missing or truncated PE signature"),
        ("machine", 0x84, "<H", 0x8664, "machine is not x86"),
        ("PE32", 0x98, "<H", 0x20B, "optional header is not PE32"),
        ("image base", 0xB4, "<I", 0x00500000, "image base changed"),
        ("optional header", 0x94, "<H", 31, "optional header is truncated"),
    ]:
        changed_image = bytearray(image)
        struct.pack_into(fmt, changed_image, offset, value)
        check(
            f"Map rejects wrong {label}",
            map_emitter.verify_image(
                bytes(changed_image), fixture, require_identity=False
            )
            == [f"image is not a readable PE32: {reason}"],
        )
    ambiguous = fixture_image([(0x00401000, b"abc"), (0x00401000, b"abc")])
    check(
        "Map ambiguous backing fails",
        map_emitter.verify_image(ambiguous, fixture, require_identity=False)
        == ["range authored-text is not uniquely file-backed"],
    )
    check(
        "Map no range list fails explicitly",
        map_emitter.verify_image(image, {"bodyRanges": []}, require_identity=False)
        == ["bodyRanges must be a nonempty list"],
    )


def main() -> int:
    exercise("timer handler", timer, timer.MANIFEST)
    exercise("player profile", profile, profile.MANIFEST)
    exercise("sun and moon", moon, moon.MANIFEST)
    exercise("camera input chain", camera, camera.MANIFEST)
    exercise("status wire translation", status, status.MANIFEST)

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

    camera_checks()
    status_checks()
    map_emitter_checks()

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} promoted static check tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
