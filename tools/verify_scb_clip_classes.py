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
# Ranks 1 to 8 and 10 to 13 of the census. Rank 9 is BindActorClip, which the
# clip layout page documents.
CLASS_ORDER = [
    "SetPosClip",
    "MotionClip",
    "ShowHideClip",
    "RapturePhysicsLookAtClip",
    "ClipSyncClip",
    "LocalCameraClip",
    "RaptureNqMotionBlendOnOffClip",
    "RaptureFacialPoseForSchClip",
    "RaptureFacialLipSyncClip",
    "RaptureMesClip",
    "DepthOfFieldClip",
    "KillClip",
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
    "RaptureNqMotionBlendOnOffClip": (
        7,
        0x38,
        0x00639AB0,
        0x0100A83C,
        0x0081ABC0,
        0x00A20F90,
        0x00DEFC20,
        "rapture only",
    ),
    "RaptureFacialPoseForSchClip": (
        8,
        0x38,
        0x0063B370,
        0x010248DC,
        0x00827140,
        0x00A20F90,
        0x00830580,
        "rapture only",
    ),
    "RaptureFacialLipSyncClip": (
        10,
        0x38,
        0x0063B1C0,
        0x010229D4,
        0x00826AB0,
        0x00A20F90,
        0x00DEFC20,
        "rapture only",
    ),
    "RaptureMesClip": (
        11,
        0x58,
        0x00638A60,
        0x00FFB0A4,
        0x00813CF0,
        0x00A20F90,
        0x00823CE0,
        "rapture only",
    ),
    "DepthOfFieldClip": (
        12,
        0x60,
        0x00A4EAD0,
        0x011370A4,
        0x00DF0350,
        0x00DF02E0,
        0x00DF0C80,
        "base only",
    ),
    "KillClip": (
        13,
        0x38,
        0x00A4E210,
        0x01135744,
        0x00DEC7F0,
        0x00A20F90,
        0x00DEC7D0,
        "base only",
    ),
}
# Slot 8 is the clip end tick. The base method returns the start tick.
BASE_SLOT8 = 0x00A21080
SLOT8 = {
    "MotionClip": 0x0082F110,
    "LocalCameraClip": 0x00A0B830,
    "DepthOfFieldClip": 0x00DF0040,
}
# How each class derives its end tick, in terms of the record D.
END_FIELD = {
    "MotionClip": "u32(D+0x18)",
    "LocalCameraClip": "u32(D+0x10) plus clip +0x110",
    "DepthOfFieldClip": "u32(D+0x10)",
}
BASE_END_FIELD = "the start tick, u32(D+4)"
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
    "registry-continued": "STATIC",
    "registry-second-inserts": "STATIC",
    "nqblend-layout": "STATIC",
    "nqblend-meaning": "INFERRED",
    "facialpose-layout": "STATIC",
    "facialpose-meaning": "INFERRED",
    "lipsync-layout": "STATIC",
    "lipsync-meaning": "INFERRED",
    "mes-layout": "STATIC",
    "mes-sink": "UNKNOWN",
    "dof-layout": "STATIC",
    "dof-meaning": "INFERRED",
    "kill-layout": "STATIC",
    "kill-meaning": "INFERRED",
    "timing-unit": "STATIC",
    "timing-frame-source": "UNKNOWN",
    "timing-start-end": "STATIC",
    "timing-end-rule": "STATIC",
    "timing-phase-pin": "STATIC",
    "timing-begin-gate": "STATIC",
    "timing-gate-writers": "UNKNOWN",
    "timing-index-word": "STATIC",
    "timing-untraced": "UNKNOWN",
    "install-census": "OBSERVED",
    "install-invariants": "OBSERVED",
    "install-timing": "OBSERVED",
}
TIMING_UNIT = {
    "ticksPerSecond": 300000,
    "ticksPerSecondVa": "0x00f67020",
    "ticksPerFrameVa": "0x012ed240",
    "ticksPerFrameImageValue": 10010,
    "motionFrameRate": 30,
    "motionFrameRateVa": "0x00fa7b18",
}
TIMING_SCHEDULER = {
    "tick": "0x00a1d330",
    "tickCall": "0x00a1e419",
    "stepWrapper": "0x00a1e380",
    "frameStep": "0x00a1e540",
    "runningPass": "0x00a1e190",
    "clockAdvance": "0x00a204d0",
    "pendingDeltaSetter": "0x00a203c0",
    "beginPredicate": "0x00b73290",
}
TIMING_CHECKS = {
    "clip slot 30 returns true",
    "scheduler wrapper pending frame step adjust",
    "scheduler wrapper aborted ActionClip pass",
    "scheduler wrapper ActionClip name",
    "scheduler clock pending delta setter",
    "scheduler ticks per second constant",
    "scheduler motion frame rate constant",
    "scheduler ticks per frame image value",
    "scheduler ticks per frame getter",
    "scheduler ticks per frame setter",
    "scheduler frames per second helper",
    "scheduler clock advance by one frame",
    "scheduler clock time getter",
    "scheduler clock rate getter",
    "scheduler clock initial state",
    "scheduler clock time passed to tick",
    "scheduler running clip pass update and end test",
    "scheduler tick header word +0x08 filter and start compare",
    "scheduler tick begin state gate",
    "scheduler tick skip flag and header bit 2",
    "scheduler tick begun flag and header bit 0",
    "scheduler tick begin overshoot",
    "scheduler tick update before end test",
    "scheduler tick end rule",
    "scheduler tick end calls",
    "scheduler tick phase one pins the playhead",
    "scheduler held clip pass update and release",
    "scheduler wrapper held count gate",
    "scheduler wrapper held step",
    "scheduler hold list insert",
    "LocalCameraClip begin stores scheduler origin",
    "LocalCameraClip update stores origin change",
    "base begin stores elapsed and step",
    "base update adds step to elapsed",
    "base accessor elapsed",
    "base accessor phase getter",
    "base accessor abort bit getter",
    "clip index word consumer",
    "clip index lookup in scheduler list",
    "motion list frame conversion 0x007ad020",
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
    "RaptureNqMotionBlendOnOffClip rapture insert",
    "RaptureNqMotionBlendOnOffClip flag and value call",
    "CharaActor blend setter 0x0065d260",
    "RaptureFacialPoseForSchClip rapture insert",
    "RaptureFacialPoseForSchClip name and word call",
    "facial pose stores 0x007a6210",
    "RaptureFacialLipSyncClip rapture insert",
    "RaptureFacialLipSyncClip second insert",
    "RaptureFacialLipSyncClip mode and set call",
    "lip sync table and resource lookups 0x007a6fc0",
    "RaptureMesClip rapture insert",
    "RaptureMesClip key entry byte switch",
    "RaptureMesClip key 3 mask loops",
    "RaptureMesClip message dispatch and phase",
    "RaptureMesClip slot 3 scheduler flag",
    "DepthOfFieldClip base insert",
    "DepthOfFieldClip flag word routing",
    "DepthOfFieldClip parameter float reads",
    "DepthOfFieldClip slot 8 length",
    "KillClip base insert",
    "KillClip second insert",
    "KillClip target abort loop",
    "RaptureMesClip type 0xa post null guard",
} | TIMING_CHECKS
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


def _order(value: int, start: int) -> str:
    return "after" if value > start else "equal" if value == start else "before"


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


def _clip_index_word(clip, sched, index) -> bool:
    return le("h", clip.record, 0x0C) == index


def _start_order(clip, sched, index):
    if index == 0 or sched.clips[index - 1].block != clip.block:
        return None
    return clip.raw >= sched.clips[index - 1].raw


def _end_not_before_start(offset: int):
    return lambda clip, sched, index: le("I", clip.record, offset) >= clip.raw


def _facialpose_name(clip, sched, index) -> bool:
    name = clip.record[0x10:0x30]
    end = name.find(b"\0")
    return (
        0 <= end < 16
        and name.startswith(b"cfxf_")
        and name[end:] == bytes(len(name) - end)
        and all(0x20 <= byte < 0x7F for byte in name[:end])
    )


def _kill_count(record: bytes) -> int:
    return le("h", record, 0x10)


def _kill_refs(clip, sched, index) -> bool:
    count = _kill_count(clip.record)
    refs = struct.unpack_from(f"<{count}h", clip.record, 0x14)
    return all(
        ref == -1 or (0 <= ref < len(sched.clips) and ref != index) for ref in refs
    )


def _kill_pad(clip, sched, index) -> bool:
    used = 0x14 + 2 * _kill_count(clip.record)
    return le("h", clip.record, 0x12) == 0 and clip.record[used:] == bytes(
        len(clip.record) - used
    )


def _dof_link(clip, sched, index) -> bool:
    return clip.link == (0x1C if le("I", clip.record, 0x14) & 2 else 0x34)


def _dof_preset_pad(clip, sched, index):
    if not le("I", clip.record, 0x14) & 2:
        return None
    return le("H", clip.record, 0x1A) == 0


def _dof_end(clip, sched, index) -> bool:
    flags = le("I", clip.record, 0x14)
    end = le("I", clip.record, 0x10)
    return end >= clip.raw and (flags & 2 != 0 or end == clip.raw)


MES_MASK_BITS = 0xFE | (0x1F << 20)
DOF_FLAG_BITS = 0x3 | (0x7F << 4)


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
    "nqblend-key-byte": (
        "RaptureNqMotionBlendOnOffClip",
        "the @CCPT entry byte +2 of the class is 1",
        _key_byte(1),
    ),
    "nqblend-link": (
        "RaptureNqMotionBlendOnOffClip",
        "record bytes are 0x18",
        lambda c, s, i: c.link == 0x18,
    ),
    "nqblend-flag": (
        "RaptureNqMotionBlendOnOffClip",
        "u32 +0x10 is 0 or 1",
        lambda c, s, i: le("I", c.record, 0x10) in (0, 1),
    ),
    "nqblend-value": (
        "RaptureNqMotionBlendOnOffClip",
        "u32 +0x14 is at most 500",
        lambda c, s, i: le("I", c.record, 0x14) <= 500,
    ),
    "facialpose-key-byte": (
        "RaptureFacialPoseForSchClip",
        "the @CCPT entry byte +2 of the class is 0",
        _key_byte(0),
    ),
    "facialpose-link": (
        "RaptureFacialPoseForSchClip",
        "record bytes are 0x38",
        lambda c, s, i: c.link == 0x38,
    ),
    "facialpose-name": (
        "RaptureFacialPoseForSchClip",
        "bytes +0x10 to +0x2f hold an ASCII name that starts with cfxf_, ends before byte 16 and is zero filled",
        _facialpose_name,
    ),
    "facialpose-words": (
        "RaptureFacialPoseForSchClip",
        "s32 +0x30 is within 0 to 9 and s32 +0x34 is within 0 to 1200",
        lambda c, s, i: (
            0 <= le("i", c.record, 0x30) <= 9 and 0 <= le("i", c.record, 0x34) <= 1200
        ),
    ),
    "lipsync-key-byte": (
        "RaptureFacialLipSyncClip",
        "the @CCPT entry byte +2 of the class is 0",
        _key_byte(0),
    ),
    "lipsync-link": (
        "RaptureFacialLipSyncClip",
        "record bytes are 0x18",
        lambda c, s, i: c.link == 0x18,
    ),
    "lipsync-mode": (
        "RaptureFacialLipSyncClip",
        "u32 +0x10 is within 0 to 7",
        lambda c, s, i: le("I", c.record, 0x10) <= 7,
    ),
    "lipsync-set": (
        "RaptureFacialLipSyncClip",
        "u32 +0x14 is within 0 to 2",
        lambda c, s, i: le("I", c.record, 0x14) <= 2,
    ),
    "mes-key-byte": (
        "RaptureMesClip",
        "the @CCPT entry byte +2 of the class is 3",
        _key_byte(3),
    ),
    "mes-link": (
        "RaptureMesClip",
        "record bytes are 0x24",
        lambda c, s, i: c.link == 0x24,
    ),
    "mes-lead": (
        "RaptureMesClip",
        "u32 +0x10 is 1",
        lambda c, s, i: le("I", c.record, 0x10) == 1,
    ),
    "mes-flags": (
        "RaptureMesClip",
        "bytes +0x1c, +0x1d and +0x1e are 0 or 1 and byte +0x1f is 0",
        lambda c, s, i: (
            _all(c.record, (0x1C, 0x1D, 0x1E), (0, 1)) and c.record[0x1F] == 0
        ),
    ),
    "mes-mask": (
        "RaptureMesClip",
        "u32 +0x20 only sets bits 1 to 7 and bits 20 to 24",
        lambda c, s, i: le("I", c.record, 0x20) & ~MES_MASK_BITS == 0,
    ),
    "dof-key-byte": (
        "DepthOfFieldClip",
        "the @CCPT entry byte +2 of the class is 3",
        _key_byte(3),
    ),
    "dof-link": (
        "DepthOfFieldClip",
        "record bytes are 0x1c when bit 1 of u32 +0x14 is set and 0x34 otherwise",
        _dof_link,
    ),
    "dof-flags": (
        "DepthOfFieldClip",
        "u32 +0x14 only sets bits 0, 1 and 4 to 10",
        lambda c, s, i: le("I", c.record, 0x14) & ~DOF_FLAG_BITS == 0,
    ),
    "dof-end": (
        "DepthOfFieldClip",
        "u32 +0x10 is not below the start tick and equals it when bit 1 of u32 +0x14 is clear",
        _dof_end,
    ),
    "dof-preset-pad": (
        "DepthOfFieldClip",
        "u16 +0x1a is 0 when bit 1 of u32 +0x14 is set",
        _dof_preset_pad,
    ),
    "kill-key-byte": (
        "KillClip",
        "the @CCPT entry byte +2 of the class is 1",
        _key_byte(1),
    ),
    "kill-count": (
        "KillClip",
        "s16 +0x10 is within 0 to 19",
        lambda c, s, i: 0 <= _kill_count(c.record) <= 19,
    ),
    "kill-link": (
        "KillClip",
        "record bytes are (0x14 + 2n + 3) & ~3 for n = s16 +0x10",
        lambda c, s, i: c.link == (0x14 + 2 * _kill_count(c.record) + 3) & ~3,
    ),
    "kill-pad": (
        "KillClip",
        "s16 +0x12 and every byte after the last listed index are 0",
        _kill_pad,
    ),
    "kill-refs": (
        "KillClip",
        "every listed index is -1 or inside the scheduler clip list and is not the clip itself",
        _kill_refs,
    ),
    "localcamera-end": (
        "LocalCameraClip",
        "u32 +0x10 is not below the start tick",
        _end_not_before_start(0x10),
    ),
    "clip-start-unit": (
        "*",
        "the start tick u32 +4 is a multiple of 10000",
        lambda c, s, i: c.raw % 10000 == 0,
    ),
    "clip-start-order": (
        "*",
        "the start tick u32 +4 is not below that of the previous clip in the same block",
        _start_order,
    ),
    "clip-index-word": (
        "*",
        "s16 +0x0c is the index of the clip in the scheduler clip list",
        _clip_index_word,
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
    "motion-end-vs-start": (
        "MotionClip",
        lambda r: _order(le("I", r, 0x18), le("I", r, 4)),
    ),
    "motion-end-unit": (
        "MotionClip",
        lambda r: "unit" if le("I", r, 0x18) % 10000 == 0 else "other",
    ),
    "localcamera-drift-bit": (
        "LocalCameraClip",
        lambda r: "set" if r[0x1A] & 4 else "clear",
    ),
    "nqblend-flag": ("RaptureNqMotionBlendOnOffClip", lambda r: str(le("I", r, 0x10))),
    "lipsync-set": ("RaptureFacialLipSyncClip", lambda r: str(le("I", r, 0x14))),
    "dof-mode": (
        "DepthOfFieldClip",
        lambda r: "preset" if le("I", r, 0x14) & 2 else "parameters",
    ),
    "kill-count": ("KillClip", lambda r: str(_kill_count(r))),
    "kill-missing-index": (
        "KillClip",
        lambda r: (
            "minus-one"
            if -1 in struct.unpack_from(f"<{_kill_count(r)}h", r, 0x14)
            else "none"
        ),
    ),
    "nqblend-value-15": (
        "RaptureNqMotionBlendOnOffClip",
        lambda r: "15" if le("I", r, 0x14) == 15 else "other",
    ),
    "lipsync-mode": ("RaptureFacialLipSyncClip", lambda r: str(le("I", r, 0x10))),
    "dof-flags": ("DepthOfFieldClip", lambda r: hex(le("I", r, 0x14))),
    "mes-route": ("RaptureMesClip", lambda r: f"{r[0x1C]}{r[0x1D]}"),
}
# id: (class, float offsets). The manifest holds min and max rounded to one decimal.
RANGES = {
    "setpos-position": ("SetPosClip", (0x10, 0x14, 0x18)),
    "setpos-scale": ("SetPosClip", (0x28, 0x2C, 0x30)),
}
# id: (class, int offsets). The manifest holds the minimum and maximum.
INT_RANGES = {
    "nqblend-value": ("RaptureNqMotionBlendOnOffClip", (0x14,), "I"),
    "facialpose-words": ("RaptureFacialPoseForSchClip", (0x30, 0x34), "i"),
    "lipsync-mode": ("RaptureFacialLipSyncClip", (0x10,), "I"),
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
        return ["classes are not the documented classes in rank order"]
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
            "slot8": f"0x{SLOT8.get(name, BASE_SLOT8):08x}",
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
        if slots[1:3] != [f"0x{slot1:08x}", f"0x{slot2:08x}"] or slots[8:9] != [
            f"0x{SLOT8.get(name, BASE_SLOT8):08x}"
        ]:
            errors.append(f"class {name} vtable slot check differs from the object")
        if tables.get(f"0x{vtable + 4 * 29:08x}") != [f"0x{slot29:08x}"]:
            errors.append(f"class {name} slot 29 check differs from the object")
        if tables.get(f"0x{vtable + 4 * 30:08x}") != ["0x00b73290"]:
            errors.append(f"class {name} slot 30 check differs from the predicate")
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
        for key in ("eraseSite", "insertSite", "baseInsertSite", "secondInsertSite"):
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
        errors.append("install.perClass is not keyed by the documented classes")
    for name, row in per_class.items():
        if row.get("clips") != sum(row.get("recordBytes", {}).values()):
            errors.append(f"install {name} recordBytes do not sum to clips")
        if row.get("schedulers", 0) > install.get("schedulers", 0):
            errors.append(f"install {name} schedulers exceed the total")
    for section, wanted in (
        ("distributions", DISTRIBUTIONS),
        ("ranges", RANGES),
        ("intRanges", INT_RANGES),
    ):
        if sorted(install.get(section, {})) != sorted(wanted):
            errors.append(f"install.{section} is not keyed by its ids")
    for key, row in install.get("distributions", {}).items():
        owner = DISTRIBUTIONS.get(key, ("",))[0]
        if sum(row.values()) != per_class.get(owner, {}).get("clips"):
            errors.append(f"install distribution {key} does not sum to its class")
    for section in ("ranges", "intRanges"):
        for key, row in install.get(section, {}).items():
            if len(row) != 2 or row[0] > row[1]:
                errors.append(f"install range {key} is malformed")
    checks = install.get("invariants", {})
    if sorted(checks) != sorted(INVARIANTS):
        errors.append("install.invariants is not keyed by the invariant ids")
    for key, row in checks.items():
        owner = INVARIANTS.get(key, ("",))[0]
        clips = (
            install.get("clips")
            if owner == "*"
            else per_class.get(owner, {}).get("clips")
        )
        if row.get("violations") != 0:
            errors.append(f"install invariant {key} records violations")
        if not 0 < row.get("checked", 0) <= (clips or 0):
            errors.append(f"install invariant {key} checked count is out of range")
    return errors


def timing_errors(document: dict) -> list[str]:
    timing = document.get("timing")
    if not isinstance(timing, dict):
        return ["timing is not an object"]
    errors = []
    unit = timing.get("unit", {})
    if {key: unit.get(key) for key in TIMING_UNIT} != TIMING_UNIT:
        errors.append("timing.unit changed")
    scheduler = timing.get("scheduler", {})
    if {key: scheduler.get(key) for key in TIMING_SCHEDULER} != TIMING_SCHEDULER:
        errors.append("timing.scheduler changed")
    names = {
        item.get("name")
        for item in document.get("staticChecks", [])
        if isinstance(item, dict)
    }
    if set(timing.get("checks", [])) != TIMING_CHECKS or not TIMING_CHECKS <= names:
        errors.append("timing.checks is not the timing check set")
    rows = timing.get("classes")
    if not isinstance(rows, list) or [r.get("name") for r in rows] != CLASS_ORDER:
        return errors + ["timing.classes are not the documented classes in rank order"]
    objects = {row.get("name"): row.get("object", {}) for row in document["classes"]}
    for row in rows:
        name = row["name"]
        if row.get("start") != "u32(D+4)":
            errors.append(f"timing {name} start changed")
        if row.get("end") != END_FIELD.get(name, BASE_END_FIELD):
            errors.append(f"timing {name} end changed")
        want = f"0x{SLOT8.get(name, BASE_SLOT8):08x}"
        if row.get("endVa") != want or objects.get(name, {}).get("slot8") != want:
            errors.append(f"timing {name} end method differs from the object")
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
    errors += timing_errors(document)
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
        (("classes", 6, "object", "size"), "0x40"),
        (("classes", 7, "registration", "factory"), "0x0063b371"),
        (("classes", 8, "registration", "secondInsertSite"), "0x6366"),
        (("classes", 9, "object", "slot1"), "0x00813cf1"),
        (("classes", 10, "object", "slot8"), "0x00df0041"),
        (("classes", 11, "registration", "baseFactory"), "0x00a4e211"),
        (("classes", 11, "fields", 0, "tier"), "INFERRED"),
        (("classes", 10, "fields", 1, "offset"), "0x34"),
        (("timing", "unit", "ticksPerSecond"), 30000),
        (("timing", "unit", "ticksPerFrameImageValue"), 10000),
        (("timing", "scheduler", "tick"), "0x00a1d331"),
        (("timing", "scheduler", "beginPredicate"), "0x00b73291"),
        (("timing", "checks"), []),
        (("timing", "classes", 1, "end"), "u32(D+0x1c)"),
        (("timing", "classes", 10, "endVa"), "0x00a21080"),
        (("timing", "classes", 6, "start"), "u32(D+8)"),
        (("install", "invariants", "clip-index-word", "violations"), 1),
        (("install", "invariants", "clip-start-order", "checked"), 0),
        (("install", "invariants", "kill-refs", "checked"), 4996),
        (("install", "intRanges", "nqblend-value"), [501, 0]),
        (("install", "distributions", "dof-mode", "preset"), 464),
        (("install", "distributions", "motion-end-vs-start", "before"), 58),
        (("install", "perClass", "KillClip", "clips"), 4994),
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
        "DepthOfFieldClip flag word routing",
        "KillClip target abort loop",
        "scheduler tick end rule",
        "scheduler clock time passed to tick",
        "scheduler wrapper aborted ActionClip pass",
        "scheduler clock pending delta setter",
        "clip slot 30 returns true",
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
    for name in ("SetPosClip", "KillClip"):
        changed_slot = copy.deepcopy(document)
        for item in changed_slot["staticChecks"]:
            if item["name"] == f"{name} vtable slot 30":
                item["values"][0] = "0x00b73291"
        mutations.append((changed_slot, csv_text))
    for name in ("DepthOfFieldClip", "KillClip"):
        changed_slot = copy.deepcopy(document)
        for item in changed_slot["staticChecks"]:
            if item["name"] == f"{name} vtable slots 0 to 8":
                item["values"][8] = "0x00a21081"
        mutations.append((changed_slot, csv_text))
    for outcome in ("timing-unit", "mes-sink", "registry-second-inserts"):
        changed = copy.deepcopy(document)
        for item in changed["outcomes"]:
            if item["id"] == outcome:
                item["tier"] = "OBSERVED"
        mutations.append((changed, csv_text))
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


def note_range(seen: list, fmt: str, record: bytes, offsets) -> None:
    """Widen seen by the values at offsets. A record too short for a field adds nothing."""
    for offset in offsets:
        try:
            value = le(fmt, record, offset)
        except struct.error:
            continue
        seen[0] = min(seen[0], value)
        seen[1] = max(seen[1], value)


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
                if owner not in (name, "*"):
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
                    try:
                        out["distributions"][key][bucket(clip.record)] += 1
                    except (IndexError, struct.error):
                        out["distributions"][key]["short"] += 1
            for key, (owner, offsets) in RANGES.items():
                if owner == name:
                    seen_range = out["ranges"].setdefault(key, [math.inf, -math.inf])
                    note_range(seen_range, "f", clip.record, offsets)
            for key, (owner, offsets, fmt) in INT_RANGES.items():
                if owner == name:
                    seen_range = out["intRanges"].setdefault(key, [math.inf, -math.inf])
                    note_range(seen_range, fmt, clip.record, offsets)
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
        "intRanges": {},
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
        "intRanges": {key: census["intRanges"][key] for key in INT_RANGES},
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
