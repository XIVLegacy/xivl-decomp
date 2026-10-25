#!/usr/bin/env python3
"""Validate the cut scheduler clip class manifest, optional exe bytes and install census."""

from __future__ import annotations

import argparse
import collections
import copy
import csv
import io
import json
import math
import re
import struct
from pathlib import Path

import scb_clip_scheduler as scb
import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "scb_clip_classes.json"
CENSUS_CSV = ROOT / "docs" / "resource" / "cut-scheduler-clip-class-census.csv"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
HEX_RE = re.compile(r"^0x[0-9a-f]{8}$")
SCOPE = "owner installation client/cut tree"
CSV_HEADER = ["rank", "class", "clips", "schedulers", "recordBytes"]
TOTALS = {
    "files": 9593,
    "sedbHeaders": 86405,
    "schedulers": 689,
    "clips": 222491,
    "classes": 102,
}
# Counts of the three classes published in cut-collision-clip-corpus.md.
PUBLISHED_COUNTS = {
    "BindActorClip": 7528,
    "RaptureCharaVisualClip": 151,
    "RaptureMapCollisionOnOffClip": 2688,
}
CLASS_ORDER = [
    "SetPosClip",
    "MotionClip",
    "ShowHideClip",
    "RapturePhysicsLookAtClip",
    "ClipSyncClip",
    "LocalCameraClip",
]
# name: (rank, size, factory, primary vtable, slot 1, slot 2, slot 29, origin)
OBJECTS = {
    "SetPosClip": (
        1,
        0x38,
        0x0063BD90,
        0x0102F44C,
        0x0082A080,
        0x00A20F90,
        0x00829F90,
        "rapture override",
    ),
    "MotionClip": (
        2,
        0x60,
        0x0063C180,
        0x010337EC,
        0x0082F120,
        0x0082F4F0,
        0x0082FAA0,
        "rapture override",
    ),
    "ShowHideClip": (
        3,
        0x38,
        0x0063BF40,
        0x0103146C,
        0x0082DE70,
        0x00A20F90,
        0x00823CE0,
        "rapture override",
    ),
    "RapturePhysicsLookAtClip": (
        4,
        0x38,
        0x0063A5F0,
        0x0101564C,
        0x00821B40,
        0x00821A70,
        0x00823CE0,
        "rapture only",
    ),
    "ClipSyncClip": (
        5,
        0x3C,
        0x0063C570,
        0x0109604C,
        0x00A100B0,
        0x00A10120,
        0x00A10210,
        "base only",
    ),
    "LocalCameraClip": (
        6,
        0x120,
        0x00A4D950,
        0x01095874,
        0x00A0C740,
        0x00A0BFD0,
        0x00DF1040,
        "base only",
    ),
}
BASE_FACTORIES = {
    "SetPosClip": 0x00A4DB10,
    "MotionClip": 0x00A4DCD0,
    "ShowHideClip": 0x00A4DB80,
}
REGISTRY = {
    "managerSlot3Registrar": "0x00635760",
    "baseRegistrar": "0x00a4c2a0",
    "insert": "0x0063f710",
    "erase": "0x0063f920",
    "registrySingleton": "0x00a02360",
}
EXPECTED_OUTCOMES = {
    "registry-two-groups": "STATIC",
    "registry-effective-factory": "INFERRED",
    "object-and-slots": "STATIC",
    "common-header-accessors": "STATIC",
    "setpos-layout": "STATIC",
    "setpos-meaning": "INFERRED",
    "showhide-layout": "STATIC",
    "motion-layout": "STATIC",
    "motion-meaning": "INFERRED",
    "lookat-layout": "STATIC",
    "lookat-meaning": "INFERRED",
    "clipsync-layout": "STATIC",
    "localcamera-layout": "STATIC",
    "class-key-entry": "STATIC",
    "class-key-link": "INFERRED",
    "showhide-sink": "UNKNOWN",
    "undecoded-words": "UNKNOWN",
    "install-census": "OBSERVED",
    "install-invariants": "OBSERVED",
}
REQUIRED_CHECKS = {
    "base begin stores state 1 and target",
    "base update promotes state 1 to 2",
    "base accessor class ordinal",
    "base accessor actor ordinal",
    "base accessor raw value",
    "base accessor header word +0x0C",
    "base accessor header word +0x08",
    "base accessor header byte +0x0A",
    "base accessor header flag bit 0",
    "base accessor header flag bit 1",
    "base accessor header flag bit 2",
    "base accessor end state",
    "base accessor stop state",
    "SetPosClip rapture erase",
    "SetPosClip rapture insert",
    "SetPosClip base insert",
    "SetPosClip component overrides",
    "SetPosClip CharaActor cast",
    "SetPosClip CharaActor setter 0x0065e660 call",
    "ShowHideClip rapture insert",
    "ShowHideClip visibility call",
    "MotionClip rapture insert",
    "MotionClip fields 0x1c and 0x20",
    "MotionClip name prefix test",
    "PhysicsLookAtClip mode switch",
    "PhysicsLookAtClip mode 1 angles",
    "PhysicsLookAtClip mode 0 call 0x0065e730",
    "ClipSyncClip base insert",
    "ClipSyncClip all-finished loop",
    "ClipSyncClip any-finished loop",
    "ClipSync array bounds check",
    "LocalCameraClip base insert",
    "LocalCameraClip begin extension gate",
    "LocalCameraClip update reads flag bit 2",
    "LocalCameraClip update reads flag bit 0",
    "LocalCameraClip extension copy",
    "class key entry fill",
    "class key entry fill call for CCPT",
    "CCPT chunk tag",
    "clip builder key entry",
    "MotionClip actor guards",
    "MotionClip end call",
}
FLAG_OFFSETS = (0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3C, 0x3D, 0x3E)
PI_F32 = struct.unpack("<f", struct.pack("<f", math.pi))[0]


def le(fmt: str, record: bytes, offset: int) -> int:
    return struct.unpack_from("<" + fmt, record, offset)[0]


def clipsync_count(record: bytes) -> int:
    return le("H", record, 0x10) & 0x7FFF


# id: (class, description, check(clip, scheduler, index) -> bool | None)
# None means the record does not take part in the check.
def _all(record: bytes, offsets, values) -> bool:
    return all(record[offset] in values for offset in offsets)


def _clipsync_refs(clip, sched, index) -> bool:
    count = clipsync_count(clip.record)
    refs = struct.unpack_from(f"<{count}h", clip.record, 0x14)
    return all(0 <= ref < len(sched.clips) and ref != index for ref in refs)


def _setpos_rotation(clip, sched, index) -> bool:
    return all(
        -PI_F32 <= le("f", clip.record, offset) <= PI_F32
        for offset in (0x1C, 0x20, 0x24)
    )


def _lookat_angles(clip, sched, index):
    if le("I", clip.record, 0x1C) != 1:
        return None
    return all(-90.0 <= le("f", clip.record, offset) <= 90.0 for offset in (0x20, 0x24))


def _localcamera_ext(clip, sched, index):
    if len(clip.record) != 64:
        return None
    return (
        0 <= le("h", clip.record, 0x20) < len(sched.actors)
        and le("h", clip.record, 0x22) >= 0
    )


def _key_byte(want: int):
    return lambda clip, sched, index: sched.clip_class_bytes[clip.cls] == want


INVARIANTS = {
    "setpos-key-byte": (
        "SetPosClip",
        "the @CCPT entry byte +2 of the class is 1",
        _key_byte(1),
    ),
    "showhide-key-byte": (
        "ShowHideClip",
        "the @CCPT entry byte +2 of the class is 1",
        _key_byte(1),
    ),
    "motion-key-byte": (
        "MotionClip",
        "the @CCPT entry byte +2 of the class is 2",
        _key_byte(2),
    ),
    "lookat-key-byte": (
        "RapturePhysicsLookAtClip",
        "the @CCPT entry byte +2 of the class is 0",
        _key_byte(0),
    ),
    "clipsync-key-byte": (
        "ClipSyncClip",
        "the @CCPT entry byte +2 of the class is 1",
        _key_byte(1),
    ),
    "localcamera-key-byte": (
        "LocalCameraClip",
        "the @CCPT entry byte +2 of the class is 4, which passes the key entry byte +4 gate of the extension",
        _key_byte(4),
    ),
    "setpos-link": (
        "SetPosClip",
        "record bytes are 0x40",
        lambda c, s, i: c.link == 0x40,
    ),
    "setpos-flags": (
        "SetPosClip",
        "bytes +0x34 to +0x3a and +0x3c to +0x3e are 0 or 1",
        lambda c, s, i: _all(c.record, FLAG_OFFSETS, (0, 1)),
    ),
    "setpos-unread": (
        "SetPosClip",
        "bytes +0x3b and +0x3f are 0",
        lambda c, s, i: c.record[0x3B] == 0 and c.record[0x3F] == 0,
    ),
    "setpos-rotation": (
        "SetPosClip",
        "floats +0x1c, +0x20 and +0x24 are within plus or minus pi",
        _setpos_rotation,
    ),
    "showhide-link": (
        "ShowHideClip",
        "record bytes are 0x14",
        lambda c, s, i: c.link == 0x14,
    ),
    "showhide-state": (
        "ShowHideClip",
        "byte +0x10 is 0 or 1",
        lambda c, s, i: c.record[0x10] in (0, 1),
    ),
    "showhide-pad": (
        "ShowHideClip",
        "bytes +0x11 to +0x13 are 0",
        lambda c, s, i: c.record[0x11:0x14] == bytes(3),
    ),
    "motion-link": (
        "MotionClip",
        "record bytes are 0x2c",
        lambda c, s, i: c.link == 0x2C,
    ),
    "motion-name-index": (
        "MotionClip",
        "s32 +0x10 is not negative",
        lambda c, s, i: le("i", c.record, 0x10) >= 0,
    ),
    "motion-time-units": (
        "MotionClip",
        "s32 +0x18, +0x1c and +0x20 are multiples of 10000",
        lambda c, s, i: all(
            le("i", c.record, offset) % 10000 == 0 for offset in (0x18, 0x1C, 0x20)
        ),
    ),
    "motion-flag": (
        "MotionClip",
        "byte +0x26 is 0 or 1",
        lambda c, s, i: c.record[0x26] in (0, 1),
    ),
    "motion-unread": (
        "MotionClip",
        "bytes +0x16, +0x17 and +0x27 are 0",
        lambda c, s, i: (
            c.record[0x16] == 0 and c.record[0x17] == 0 and c.record[0x27] == 0
        ),
    ),
    "lookat-link": (
        "RapturePhysicsLookAtClip",
        "record bytes are 0x34",
        lambda c, s, i: c.link == 0x34,
    ),
    "lookat-mode": (
        "RapturePhysicsLookAtClip",
        "u32 +0x1c is 0, 1 or 2",
        lambda c, s, i: le("I", c.record, 0x1C) in (0, 1, 2),
    ),
    "lookat-angles": (
        "RapturePhysicsLookAtClip",
        "in mode 1 the floats +0x20 and +0x24 are within plus or minus 90",
        _lookat_angles,
    ),
    "lookat-flags": (
        "RapturePhysicsLookAtClip",
        "bytes +0x14 to +0x17 are 0 or 1",
        lambda c, s, i: _all(c.record, (0x14, 0x15, 0x16, 0x17), (0, 1)),
    ),
    "clipsync-link": (
        "ClipSyncClip",
        "record bytes are (0x14 + 2n + 3) & ~3 for n = u16 +0x10 & 0x7fff",
        lambda c, s, i: c.link == (0x14 + 2 * clipsync_count(c.record) + 3) & ~3,
    ),
    "clipsync-pad": (
        "ClipSyncClip",
        "u16 +0x12 is 0",
        lambda c, s, i: le("H", c.record, 0x12) == 0,
    ),
    "clipsync-refs": (
        "ClipSyncClip",
        "every listed index is inside the scheduler clip list and is not the clip itself",
        _clipsync_refs,
    ),
    "localcamera-link": (
        "LocalCameraClip",
        "record bytes are 0x40 when bit 0 of +0x1a is set and 0x20 otherwise",
        lambda c, s, i: c.link == (0x40 if c.record[0x1A] & 1 else 0x20),
    ),
    "localcamera-ext": (
        "LocalCameraClip",
        "the extension actor ordinal is a scheduler actor and the part index is not negative",
        _localcamera_ext,
    ),
    "localcamera-pad": (
        "LocalCameraClip",
        "u16 +0x16 and u16 +0x1e are 0",
        lambda c, s, i: le("H", c.record, 0x16) == 0 and le("H", c.record, 0x1E) == 0,
    ),
}


# id: (class, bucket(record) -> str)
DISTRIBUTIONS = {
    "showhide-state": ("ShowHideClip", lambda r: str(r[0x10])),
    "motion-speed": (
        "MotionClip",
        lambda r: "100" if le("H", r, 0x24) == 100 else "other",
    ),
    "lookat-mode": ("RapturePhysicsLookAtClip", lambda r: str(le("I", r, 0x1C))),
    "clipsync-count-parity": (
        "ClipSyncClip",
        lambda r: "even" if clipsync_count(r) % 2 == 0 else "odd",
    ),
}
# id: (class, float offsets). The manifest holds min and max rounded to one decimal.
RANGES = {
    "setpos-position": ("SetPosClip", (0x10, 0x14, 0x18)),
    "setpos-scale": ("SetPosClip", (0x28, 0x2C, 0x30)),
}


def parse_hex(text: str) -> int:
    return int(text, 16)


def csv_rows(text: str) -> list[dict]:
    return list(csv.DictReader(io.StringIO(text)))


def csv_errors(text: str, document: dict) -> list[str]:
    errors = []
    reader = csv.reader(io.StringIO(text))
    if next(reader, None) != CSV_HEADER:
        return ["census csv header changed"]
    rows = csv_rows(text)
    if len(rows) != TOTALS["classes"]:
        errors.append("census csv row count changed")
    names = [row["class"] for row in rows]
    if len(set(names)) != len(names):
        errors.append("census csv repeats a class")
    clips = [int(row["clips"]) for row in rows]
    if sum(clips) != TOTALS["clips"]:
        errors.append("census csv clip total changed")
    order = sorted(zip(clips, names), key=lambda item: (-item[0], item[1]))
    if [(int(r["clips"]), r["class"]) for r in rows] != order:
        errors.append("census csv is not sorted by clips then class")
    if [int(row["rank"]) for row in rows] != list(range(1, len(rows) + 1)):
        errors.append("census csv ranks are not 1 to n")
    by_name = {row["class"]: row for row in rows}
    for name, count in PUBLISHED_COUNTS.items():
        if int(by_name.get(name, {"clips": -1})["clips"]) != count:
            errors.append(f"census csv count for {name} changed")
    for row in rows:
        sizes = [item.split(":") for item in row["recordBytes"].split(";")]
        if sum(int(count) for _, count in sizes) != int(row["clips"]):
            errors.append(
                f"census csv recordBytes for {row['class']} do not sum to clips"
            )
        if int(row["schedulers"]) > TOTALS["schedulers"]:
            errors.append(f"census csv schedulers for {row['class']} exceed the total")
    for item in document.get("classes", []):
        row = by_name.get(item.get("name"))
        if row is None:
            errors.append(f"census csv lacks {item.get('name')}")
        elif (
            int(row["rank"]) != item.get("rank")
            or int(row["clips"]) != item.get("clips")
            or int(row["schedulers"]) != item.get("schedulers")
        ):
            errors.append(
                f"census csv row for {item.get('name')} differs from the manifest"
            )
    return errors


def class_errors(document: dict) -> list[str]:
    errors = []
    rows = document.get("classes")
    if not isinstance(rows, list) or [row.get("name") for row in rows] != CLASS_ORDER:
        return ["classes are not the six documented classes in rank order"]
    check_names = {
        item.get("name")
        for item in document.get("staticChecks", [])
        if isinstance(item, dict)
    }
    for row in rows:
        name = row["name"]
        rank, size, factory, vtable, slot1, slot2, slot29, origin = OBJECTS[name]
        obj = row.get("object", {})
        want = {
            "size": f"0x{size:08x}",
            "primaryVtable": f"0x{vtable:08x}",
            "slot1": f"0x{slot1:08x}",
            "slot2": f"0x{slot2:08x}",
            "slot29": f"0x{slot29:08x}",
        }
        for key, value in want.items():
            if obj.get(key) != value:
                errors.append(f"class {name} object field {key} changed")
        if row.get("rank") != rank:
            errors.append(f"class {name} rank changed")
        tables = {
            item["va"]: item.get("values", [])
            for item in document.get("staticChecks", [])
            if isinstance(item, dict) and item.get("kind") == "dwords"
        }
        slots = tables.get(f"0x{vtable:08x}", [])
        if slots[1:3] != [f"0x{slot1:08x}", f"0x{slot2:08x}"]:
            errors.append(f"class {name} vtable slot check differs from the object")
        if tables.get(f"0x{vtable + 4 * 29:08x}") != [f"0x{slot29:08x}"]:
            errors.append(f"class {name} slot 29 check differs from the object")
        reg = row.get("registration", {})
        if reg.get("origin") != origin:
            errors.append(f"class {name} registration origin changed")
        if origin == "base only":
            if parse_hex(reg.get("baseFactory", "0x0")) != factory:
                errors.append(f"class {name} base factory changed")
        else:
            if (
                origin == "rapture override"
                and parse_hex(reg.get("baseFactory", "0x0")) != BASE_FACTORIES[name]
            ):
                errors.append(f"class {name} base factory changed")
            if parse_hex(reg.get("factory", "0x0")) != factory:
                errors.append(f"class {name} factory changed")
        for key in ("eraseSite", "insertSite", "baseInsertSite"):
            if key in reg and not HEX_RE.fullmatch(reg[key]):
                errors.append(f"class {name} {key} is malformed")
        fields = row.get("fields")
        if not fields:
            errors.append(f"class {name} has no fields")
            continue
        for field in fields:
            if field.get("check") not in check_names:
                errors.append(
                    f"class {name} field {field.get('field')!r} cites a missing check"
                )
            if field.get("tier") != "STATIC":
                errors.append(
                    f"class {name} field {field.get('field')!r} is not STATIC"
                )
            offset = parse_hex(field["offset"])
            if offset + field["width"] > row["record"]["span"]:
                errors.append(
                    f"class {name} field {field.get('field')!r} is outside the record"
                )
            if "meaning" in field and field.get("meaningTier") != "INFERRED":
                errors.append(
                    f"class {name} field {field.get('field')!r} meaning is not INFERRED"
                )
        spans = collections.defaultdict(list)
        for field in fields:
            spans[field.get("when")].append(
                (parse_hex(field["offset"]), field["width"], field["field"])
            )
        for group in spans.values():
            group.sort()
            for (a, aw, an), (b, _, bn) in zip(group, group[1:]):
                if a + aw > b:
                    errors.append(f"class {name} fields {an!r} and {bn!r} overlap")
    return errors


def invariant_errors(document: dict) -> list[str]:
    listed = document.get("invariants")
    if not isinstance(listed, list):
        return ["invariants is not a list"]
    found = {item.get("id"): (item.get("class"), item.get("tier")) for item in listed}
    wanted = {key: (value[0], "OBSERVED") for key, value in INVARIANTS.items()}
    errors = []
    if len(found) != len(listed) or found != wanted:
        errors.append("invariant ids, classes or tiers changed")
    if any(not str(item.get("claim", "")).strip() for item in listed):
        errors.append("an invariant has no claim")
    return errors


def install_errors(install: dict) -> list[str]:
    errors = []
    if install.get("scope") != SCOPE:
        errors.append("install.scope changed")
    for key, value in TOTALS.items():
        if install.get(key) != value:
            errors.append(f"install.{key} changed")
    per_class = install.get("perClass", {})
    if sorted(per_class) != sorted(CLASS_ORDER):
        errors.append("install.perClass is not keyed by the six classes")
    for name, row in per_class.items():
        if row.get("clips") != sum(row.get("recordBytes", {}).values()):
            errors.append(f"install {name} recordBytes do not sum to clips")
        if row.get("schedulers", 0) > install.get("schedulers", 0):
            errors.append(f"install {name} schedulers exceed the total")
    for section, wanted in (("distributions", DISTRIBUTIONS), ("ranges", RANGES)):
        if sorted(install.get(section, {})) != sorted(wanted):
            errors.append(f"install.{section} is not keyed by its ids")
    for key, row in install.get("distributions", {}).items():
        owner = DISTRIBUTIONS.get(key, ("",))[0]
        if sum(row.values()) != per_class.get(owner, {}).get("clips"):
            errors.append(f"install distribution {key} does not sum to its class")
    for key, row in install.get("ranges", {}).items():
        if len(row) != 2 or row[0] > row[1]:
            errors.append(f"install range {key} is malformed")
    checks = install.get("invariants", {})
    if sorted(checks) != sorted(INVARIANTS):
        errors.append("install.invariants is not keyed by the invariant ids")
    for key, row in checks.items():
        clips = per_class.get(INVARIANTS.get(key, ("",))[0], {}).get("clips")
        if row.get("violations") != 0:
            errors.append(f"install invariant {key} records violations")
        if not 0 < row.get("checked", 0) <= (clips or 0):
            errors.append(f"install invariant {key} checked count is out of range")
    return errors


def verify(document: dict | None = None, csv_text: str | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if csv_text is None:
        csv_text = CENSUS_CSV.read_text(encoding="ascii")
    if document.get("format") != "xivl-scb-clip-classes-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binary") != "ffxivgame.exe retail 1.23b"
        or source.get("build") != "2012.09.19.0001"
        or source.get("size") != EXPECTED_SIZE
        or source.get("binarySha256") != EXPECTED_SHA256
        or source.get("imageBase") != "0x00400000"
    ):
        errors.append("evidence identity changed")
    if document.get("registry") != {
        **REGISTRY,
        "boundary": document.get("registry", {}).get("boundary"),
    }:
        errors.append("registry changed")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    missing = REQUIRED_CHECKS - set(names)
    if missing:
        errors.append(f"staticChecks lacks {sorted(missing)}")
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not HEX_RE.fullmatch(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            pattern, mask = item.get("pattern", ""), item.get("mask", "")
            try:
                same = len(bytes.fromhex(pattern)) == len(bytes.fromhex(mask)) > 0
            except ValueError:
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
        elif item["kind"] == "dwords":
            if not item.get("values") or not all(
                HEX_RE.fullmatch(str(v)) for v in item["values"]
            ):
                errors.append(f"staticChecks {item['name']!r} values invalid")
        elif not str(item.get("text", "")):
            errors.append(f"staticChecks {item['name']!r} text is empty")
    errors += class_errors(document)
    errors += csv_errors(csv_text, document)
    errors += invariant_errors(document)
    outcomes = document.get("outcomes")
    if not isinstance(outcomes, list):
        errors.append("outcomes is not a list")
    else:
        found = {item.get("id"): item.get("tier") for item in outcomes}
        if len(found) != len(outcomes) or found != EXPECTED_OUTCOMES:
            errors.append("outcome ids or tiers changed")
        if any(not str(item.get("claim", "")).strip() for item in outcomes):
            errors.append("an outcome has no claim")
    errors += install_errors(document.get("install", {}))
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    csv_text = CENSUS_CSV.read_text(encoding="ascii")
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("registry", "erase"), "0x0063f921"),
        (("classes", 0, "object", "slot1"), "0x0082a081"),
        (("classes", 1, "object", "size"), "0x68"),
        (("classes", 2, "object", "primaryVtable"), "0x0103146d"),
        (("classes", 3, "registration", "factory"), "0x0063a5f1"),
        (("classes", 4, "registration", "origin"), "rapture only"),
        (("classes", 5, "rank"), 7),
        (("classes", 0, "clips"), 33077),
        (("classes", 0, "registration", "baseFactory"), "0x00a4db11"),
        (("classes", 0, "fields", 0, "check"), "no such check"),
        (("classes", 0, "fields", 0, "tier"), "INFERRED"),
        (("classes", 0, "fields", 0, "width"), 64),
        (("classes", 1, "fields", 1, "offset"), "0x10"),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0xa21030"),
        (("staticChecks", 2, "mask"), "ff"),
        (("invariants", 0, "tier"), "STATIC"),
        (("invariants", 3, "class"), "MotionClip"),
        (("outcomes", 0, "tier"), "INFERRED"),
        (("outcomes", 5, "tier"), "STATIC"),
        (("outcomes", 3, "claim"), " "),
        (("install", "scope"), "owner installation"),
        (("install", "schedulers"), 688),
        (("install", "clips"), 222490),
        (("install", "perClass", "SetPosClip", "clips"), 33077),
        (("install", "perClass", "ClipSyncClip", "recordBytes", "24"), 13913),
        (("install", "invariants", "setpos-flags", "violations"), 1),
        (("install", "invariants", "clipsync-refs", "checked"), 0),
        (("install", "invariants", "localcamera-key-byte", "violations"), 1),
        (("install", "distributions", "showhide-state", "1"), 12181),
        (("install", "distributions", "lookat-mode", "2"), 131),
        (("install", "ranges", "setpos-scale"), [8.1, 0.1]),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append((changed, csv_text))
    for name in (
        "SetPosClip component overrides",
        "ShowHideClip visibility call",
        "ClipSyncClip any-finished loop",
        "LocalCameraClip begin extension gate",
        "base accessor raw value",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append((dropped, csv_text))
    changed_slot = copy.deepcopy(document)
    for item in changed_slot["staticChecks"]:
        if item["name"] == "MotionClip vtable slots 0 to 8":
            item["values"][1] = "0x0082f121"
    mutations.append((changed_slot, csv_text))
    short = copy.deepcopy(document)
    short["outcomes"].pop()
    mutations.append((short, csv_text))
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append((leaked, csv_text))
    for old, new in (
        ("1,SetPosClip,33078", "1,SetPosClip,33077"),
        ("2,MotionClip", "3,MotionClip"),
        ("BindActorClip,7528", "BindActorClip,7529"),
    ):
        mutations.append((document, csv_text.replace(old, new, 1)))
    mutations.append((document, "\n".join(csv_text.splitlines()[:-1]) + "\n"))
    return [
        f"mutation {index} was accepted"
        for index, (item, text) in enumerate(mutations, 1)
        if not verify(item, text)
    ]


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    return image_checks.verify_image(data, document, require_identity)


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def cut_files(root: Path) -> list[Path]:
    return sorted(
        path
        for path in (root / "client" / "cut").rglob("*")
        if path.is_file() and path.name != ".DS_Store"
    )


def census_data(data: bytes, out: dict) -> None:
    """Add one file's counts to the census. No bytes are kept."""
    out["files"] += 1
    out["sedbHeaders"] += scb.count_sedb_headers(data)
    for start in scb.find_scheduler_starts(data):
        sched = scb.parse_scheduler(data[start:], start)
        out["schedulers"] += 1
        seen = set()
        for index, clip in enumerate(sched.clips):
            out["clips"] += 1
            name = clip.class_name
            out["classClips"][name] += 1
            out["classLinks"][name][clip.link] += 1
            seen.add(name)
            for key, (owner, _, check) in INVARIANTS.items():
                if owner != name:
                    continue
                try:
                    result = check(clip, sched, index)
                except (IndexError, struct.error):
                    result = False
                if result is None:
                    continue
                out["checked"][key] += 1
                out["violations"][key] += not result
            for key, (owner, bucket) in DISTRIBUTIONS.items():
                if owner == name:
                    out["distributions"][key][bucket(clip.record)] += 1
            for key, (owner, offsets) in RANGES.items():
                if owner == name:
                    seen_range = out["ranges"].setdefault(key, [math.inf, -math.inf])
                    for offset in offsets:
                        value = le("f", clip.record, offset)
                        seen_range[0] = min(seen_range[0], value)
                        seen_range[1] = max(seen_range[1], value)
        out["classSchedulers"].update(seen)


def blank_census() -> dict:
    return {
        "files": 0,
        "sedbHeaders": 0,
        "schedulers": 0,
        "clips": 0,
        "classClips": collections.Counter(),
        "classSchedulers": collections.Counter(),
        "classLinks": collections.defaultdict(collections.Counter),
        "checked": collections.Counter(),
        "violations": collections.Counter(),
        "distributions": collections.defaultdict(collections.Counter),
        "ranges": {},
    }


def census_install(root: Path) -> dict:
    """Counts and invariant results for every file below client/cut."""
    out = blank_census()
    for path in cut_files(root):
        census_data(path.read_bytes(), out)
    return out


def census_rows(census: dict) -> list[dict]:
    names = sorted(census["classClips"], key=lambda n: (-census["classClips"][n], n))
    rows = []
    for rank, name in enumerate(names, 1):
        links = census["classLinks"][name]
        rows.append(
            {
                "rank": rank,
                "class": name,
                "clips": census["classClips"][name],
                "schedulers": census["classSchedulers"][name],
                "recordBytes": ";".join(
                    f"{size}:{links[size]}" for size in sorted(links)
                ),
            }
        )
    return rows


def census_csv(census: dict) -> str:
    out = io.StringIO()
    writer = csv.DictWriter(out, CSV_HEADER, lineterminator="\n")
    writer.writeheader()
    writer.writerows(census_rows(census))
    return out.getvalue()


def census_manifest(census: dict) -> dict:
    """The install section of the manifest."""
    return {
        "scope": SCOPE,
        "files": census["files"],
        "sedbHeaders": census["sedbHeaders"],
        "schedulers": census["schedulers"],
        "clips": census["clips"],
        "classes": len(census["classClips"]),
        "perClass": {
            name: {
                "clips": census["classClips"][name],
                "schedulers": census["classSchedulers"][name],
                "recordBytes": {
                    str(size): count
                    for size, count in sorted(census["classLinks"][name].items())
                },
            }
            for name in CLASS_ORDER
        },
        "invariants": {
            key: {
                "checked": census["checked"][key],
                "violations": census["violations"][key],
            }
            for key in INVARIANTS
        },
        "distributions": {
            key: dict(sorted(census["distributions"][key].items()))
            for key in DISTRIBUTIONS
        },
        "ranges": {
            key: [round(value, 1) for value in census["ranges"][key]] for key in RANGES
        },
    }


def flatten(node, prefix: str = "") -> dict:
    if isinstance(node, dict):
        flat = {}
        for key, value in node.items():
            flat.update(flatten(value, f"{prefix}{key}."))
        return flat
    return {prefix.rstrip("."): node}


def compare_census(census: dict, document: dict, csv_text: str) -> list[str]:
    errors = []
    found = flatten(census_manifest(census))
    wanted = flatten(document["install"])
    for key in sorted(set(found) | set(wanted)):
        if found.get(key) != wanted.get(key):
            errors.append(
                f"install {key} is {found.get(key)}, manifest has {wanted.get(key)}"
            )
    if census_csv(census) != csv_text:
        errors.append("census csv differs from the install census")
    return errors


def verify_install(root: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    try:
        census = census_install(root)
    except (scb.ParseError, OSError, struct.error) as exc:
        return [f"install cannot be walked: {exc}"]
    return compare_census(census, document, CENSUS_CSV.read_text(encoding="ascii"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install",
        type=Path,
        help="game root whose client/cut files are compared with the census",
    )
    parser.add_argument(
        "--print-install",
        type=Path,
        metavar="ROOT",
        help="print the manifest install section of ROOT as JSON and exit",
    )
    parser.add_argument(
        "--print-census",
        type=Path,
        metavar="ROOT",
        help="print the class census csv of ROOT and exit",
    )
    args = parser.parse_args()
    if args.print_install:
        print(json.dumps(census_manifest(census_install(args.print_install)), indent=2))
        return 0
    if args.print_census:
        print(census_csv(census_install(args.print_census)), end="")
        return 0
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
    if errors:
        print(f"FAIL: {len(errors)} cut scheduler clip class error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install census"
    print(f"PASS: cut scheduler clip class manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
