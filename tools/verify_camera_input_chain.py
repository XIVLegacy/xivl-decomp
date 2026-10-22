#!/usr/bin/env python3
"""Validate the camera input chain contract and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import json
import math
import re
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "camera_input_chain.json"
DOC = ROOT / "docs" / "actor" / "camera-input-chain.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
SECTIONS = (
    "contexts",
    "defaults",
    "pipeline",
    "commands",
    "pointer",
    "zoom",
    "rotation",
    "follow",
    "lens",
    "smoothing",
)

RADIUS_MIN = 1.5
RADIUS_MAX = 10.0
RADIUS_DEFAULT = 4.5
WHEEL_NOTCH = 120
WHEEL_SCALE = 200
THIRD_PERSON_DIVISOR = 500.0
FIRST_PERSON_DIVISOR = 1000.0
PITCH_LIMIT_THIRD = 0.7853981852531433
PITCH_LIMIT_FIRST = 1.3962634801864624
SENSOR = 15.087599754333496
FOCAL_MIN = 18.0
FOCAL_MAX = 28.0
FIRST_PERSON_FOV_BASE = 0.5235987901687622
FIRST_PERSON_FOV_SCALE = 0.4363323450088501
BLEND_START = 30.0
DT_NUMERATOR = 30.0
DT_DENOMINATOR = 300000.0

KEY_CLASSES = [
    (0, "CameraActorContextTPS", "0x007f3ad0", "0x00ff10bc"),
    (1, "CameraActorContextFPS", "0x007f1730", "0x00ff101c"),
    (2, "CameraActorContextWowTPS", "0x007f1fc0", "0x00ff1204"),
    (3, "CameraActorContextWowFPS", "0x007f1dc0", "0x00ff1164"),
    (4, "CameraActorContextTPS_Lock", "0x007f4a40", "0x00ff110c"),
    (5, "CameraActorContextFPS_Lock", "0x007f1840", "0x00ff106c"),
    (6, "CameraActorContextWowTPS_Lock", "0x007f5c10", "0x00ff125c"),
    (7, "CameraActorContextWowFPS_Lock", "0x007f1ec0", "0x00ff11b4"),
    (8, "CameraActorContextCharaSelect", "0x007f2400", "0x00ff0f24"),
    (9, "CameraActorContextCharaCreate", "0x007f26d0", "0x00ff0f74"),
    (10, "CameraActorContextCharaFocus", "0x007f2c70", "0x00ff0fcc"),
]

EXPECTED = {
    "contexts": {
        "constructorVa": "0x0061b5a0",
        "registryInsertVa": "0x00621be0",
        "selectedKeyOffset": "0x434",
        "previousKeyOffset": "0x438",
        "noneKey": 11,
        "keys": [
            {"key": key, "class": name, "constructorVa": ctor, "vtableVa": vtable}
            for key, name, ctor, vtable in KEY_CLASSES
        ],
        "familyIndex": "key & 3 for keys below 8: 0 TPS, 1 FPS, 2 WowTPS, 3 WowFPS",
        "lockOffset": 4,
        "freePredicateVa": "0x00616f90",
        "freePredicateNonzero": {"4": 0, "5": 1, "6": 2, "7": 3},
        "freePredicateZero": {"0": 4, "1": 5, "2": 6, "3": 7},
        "remapPredicateVa": "0x007a3a60",
        "remapWhenTrue": {
            "0": 0,
            "1": 0,
            "2": 2,
            "3": 2,
            "4": 0,
            "5": 0,
            "6": 2,
            "7": 2,
        },
        "setKeyVa": "0x00618650",
        "fpsForbidden": {"offset": "0x448", "mask": "0x10", "keys": [1, 3, 5, 7]},
        "familyToggleVa": "0x006187f0",
        "familyToggle": {"0": [1, 5], "1": [0, 4], "2": [3, 7], "3": [2, 6]},
        "initial": {
            "constructorWrite": 1,
            "predicateNonzero": 0,
            "predicateZero": 4,
            "remapKeepsThirdPerson": True,
            "tier": "STATIC",
        },
    },
    "defaults": {
        "radiusSlotOffsets": ["0x3f4", "0x3f8", "0x3fc", "0x400"],
        "radiusMirrorOffset": "0x3f0",
        "radiusDefault": 4.5,
        "radiusDefaultVa": "0x00fb70b8",
        "getterVa": "0x0060e970",
        "setterVa": "0x00611350",
        "radiusMin": 1.5,
        "radiusMax": 10.0,
        "resetVa": "0x006179b0",
        "resetCommand": 3,
        "firstPersonZoom": {"offset": "0x428", "initial": 0.75, "min": 0.0, "max": 1.0},
        "sensitivity": {"offset": "0x1d8", "initial": 1.0, "command": 5},
        "focalByte": {"offset": "0x1c4", "initial": 18},
        "offsetByte": {"offset": "0x1c5"},
        "invertByte": {
            "offset": "0x1c6",
            "initial": 0,
            "thirdPerson": {"x": 8, "y": 4},
            "firstPerson": {"x": 2, "y": 1},
        },
    },
    "pipeline": {
        "updateVa": "0x006195d0",
        "dt": "elapsed * 30.0 / 300000.0",
        "elapsedSource": "first dword of the update argument",
        "elapsedUnit": "unresolved",
        "tickQueuesEndpointKeys": [2, 3, 6, 7, 10],
        "singleUpdateKeys": [0, 1, 4, 5, 8, 9],
        "singleUpdateOtherwise": True,
        "contextSlots": {
            "singleUpdate": 1,
            "tick": 7,
            "queueA": 8,
            "queueB": 9,
            "endpoint": 10,
        },
        "queueA": {
            "containerOffset": "0x1dc",
            "pushVa": "0x00618bf0",
            "frontVa": "0x007f89f0",
            "item": "pointer to the command payload",
        },
        "queueB": {
            "containerOffset": "0x1f4",
            "pushVa": "0x00618c50",
            "frontVa": "0x007f8a50",
            "item": "first 16-bit word of the command payload",
        },
        "pushRequestsKey": {"0": 2, "1": 3, "2": 2, "3": 3},
        "firstPersonFlag": {
            "offset": "0x448",
            "mask": "0x02",
            "consumerVa": "0x006187f0",
        },
    },
    "commands": {
        "receiverVa": "0x0061a6c0",
        "vtableSlot": 157,
        "maxId": 48,
        "tableVa": "0x0061ae48",
        "rows": [
            {
                "id": 1,
                "effect": "stores 12 payload bytes at +0x1BC, then requests key 0 or 1",
            },
            {"id": 3, "effect": "reset: radius slots 4.5, first-person zoom 0.75"},
            {"id": 4, "effect": "float to +0x40C"},
            {"id": 5, "effect": "float to +0x1D8"},
            {"id": 6, "effect": "float to +0x42C"},
            {"id": 7, "effect": "float to +0x430"},
            {"id": 16, "effect": "set context key"},
            {"id": 17, "effect": "toggle TPS and FPS family"},
            {"id": 20, "effect": "float to +0x404"},
            {"id": 23, "effect": "float to +0x424"},
            {"id": 40, "effect": "queue A push"},
            {"id": 41, "effect": "key request, payload not traced"},
            {"id": 42, "effect": "queue B push"},
            {"id": 43, "effect": "+0x448 bit 0x04"},
            {"id": 44, "effect": "calls 0x00618B40"},
            {"id": 45, "effect": "+0x448 bit 0x10"},
            {
                "id": 47,
                "effect": "slot 0 to payload float 0, then 0x00619F80 with float 1",
            },
            {"id": 48, "effect": "calls 0x00619FE0 with two floats"},
        ],
        "dragGate": "commands 40, 41, 42 return while +0x1BC or +0x1BE is nonzero",
    },
    "pointer": {
        "elementClass": "Application::Main::Element::System::CameraElement",
        "elementConstructorVa": "0x00566380",
        "subscriptions": [
            {
                "event": "PreviewMouseMove",
                "eventId": "0x11",
                "strategy": 2,
                "handlerVa": "0x0055c9b0",
            },
            {
                "event": "PreviewMouseUp",
                "eventId": "0x17",
                "strategy": 2,
                "handlerVa": "0x0055cb60",
            },
            {
                "event": "PreviewMouseDown",
                "eventId": "0x07",
                "strategy": 2,
                "handlerVa": "0x0055cc90",
            },
            {
                "event": "MouseWheel",
                "eventId": "0x1e",
                "strategy": 0,
                "handlerVa": "0x0055cdb0",
            },
        ],
        "windowsMessages": {"0x201": 1, "0x204": 4, "0x207": 2},
        "buttonStateBytes": {"left": "0x8", "middle": "0x9", "right": "0xa"},
        "rightButton": {"command": "0x28", "payloadBytes": 10},
        "middleButton": {"command": "0x29", "payloadBytes": 4},
        "leftButton": "unused by the element",
        "dragPayloadWords": ["pressed", "dx", "dy", "x", "y"],
        "wheel": {
            "message": "0x20a",
            "notchDivisor": 120,
            "payloadScale": 200,
            "payloadBytes": 2,
            "command": "0x2a",
        },
        "commandHandlerVa": "0x00566d10",
        "moveEventId": 1,
        "axisEventId": "0x17",
        "forwardOn": -1.0,
        "backwardOn": 1.0,
        "zoomScale": 0.015625,
        "defaultPositionRadius": 4.5,
    },
    "zoom": {
        "thirdPerson": {
            "handlerVa": "0x007f7ef0",
            "lockHandlerVa": "0x007f8200",
            "slot": 0,
            "lockSlot": 1,
            "divisor": 500.0,
            "stepPerNotch": 0.4,
            "min": 1.5,
            "max": 10.0,
            "firstPersonAtOrBelow": 1.5,
        },
        "firstPerson": {
            "handlerVa": "0x007f7af0",
            "offset": "0x428",
            "divisor": 1000.0,
            "stepPerNotch": 0.2,
            "min": 0.0,
            "max": 1.0,
            "thirdPersonAtOrAbove": 1.0,
        },
        "axis": {
            "command": 23,
            "offset": "0x424",
            "rule": "slot 0 += value * dt / 10.0",
            "tickVa": "0x007f5450",
        },
    },
    "rotation": {
        "wowThirdPerson": {
            "handlerVa": "0x007f7d40",
            "pitchOffset": "0x8",
            "yawOffset": "0xc",
            "rate": "delta / 100.0 * 0.2 * sensitivity",
            "pitchLimit": 0.7853981852531433,
            "yawRange": "-pi to pi",
            "yawGateVa": "0x007f7e17",
            "yawGateFlagOffset": "0x10",
            "yawUpdatedWhileFlagSet": False,
        },
        "wowFirstPerson": {
            "handlerVa": "0x007f7940",
            "pitchOffset": "0x4",
            "yawOffset": "0x8",
            "rate": "delta / 100.0 * 0.15 * sensitivity",
            "pitchLimit": 1.3962634801864624,
            "yawRange": "-pi to pi",
        },
        "axes": {
            "pitchBuilderVa": "0x0061f250",
            "yawBuilderVa": "0x0060d290",
            "pitchAxis": "X",
            "yawAxis": "Y",
        },
        "tier": "STATIC",
    },
    "follow": {
        "tickVa": "0x007f5450",
        "predicateVa": "0x007a4610",
        "predicate": "bit 3 of the dword at helper +4",
        "blendWeight": "min(1.0, 0.2 * dt)",
        "targetYaw": "actor heading lane minus pi, wrapped",
        "flagOffset": "0x10",
        "flagSetVa": "0x007f5653",
        "flagClearVa": "0x007f565c",
        "flagRule": "1 while the predicate holds, else 0, rewritten every tick",
        "tier": "INFERRED",
    },
    "lens": {
        "contextOffset": "0x40c",
        "formula": "2 * atan(15.0876 / (2 * f))",
        "focalMin": 18,
        "focalMax": 28,
        "firstPerson": "0.5235988 + 0.4363323 * zoom",
        "armByKey": [0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 0],
        "projection": {
            "setupVa": "0x0060e860",
            "callerVa": "0x00619904",
            "argument": 0,
            "fovOffset": "0x15c",
            "fovRule": "+0x40c + 0.5235988",
            "nearOffset": "0x154",
            "farOffset": "0x158",
            "near": 0.25,
            "far": 10000.0,
            "aspectOffset": "0x150",
            "builderVa": "0x00a5adb0",
            "flagBranch": {"near": 0.01, "far": 1000.0},
        },
        "tier": "STATIC",
    },
    "smoothing": {
        "timerOffset": "0x43c",
        "start": 30.0,
        "interpolatorVa": "0x0060e4c0",
        "weight": "timer / 30.0",
        "decrement": "dt",
        "tier": "STATIC",
    },
}

REQUIRED_CHECKS = (
    {f"key {k} constructor call" for k, *_ in KEY_CLASSES}
    | {f"key {k} store and insert" for k, *_ in KEY_CLASSES}
    | {f"{name.split('Context')[-1]} vtable store" for _, name, *_ in KEY_CLASSES}
    | {
        "radius default 4.5",
        "radius clamp bounds",
        "constructor radius stores",
        "constructor initial selection",
        "set context key",
        "family toggle dispatch",
        "interpolator",
        "update key dispatch",
        "update tick queues endpoint arm",
        "command table",
        "camera command slot",
        "queue A push",
        "queue B push",
        "WowTPS wheel",
        "WowTPS_Lock wheel",
        "WowFPS wheel",
        "WowTPS drag",
        "WowFPS drag",
        "WowTPS tick",
        "wheel handler",
        "win32 wheel accumulation",
        "win32 right button bit",
        "button state bytes",
        "element registrations",
        "lens arm key family 0 and 2",
        "projection fov setup",
        "pitch builder",
        "yaw builder",
    }
)

CITED_ADDRESSES = [
    "0x0061b5a0",
    "0x00621be0",
    "0x0061a6c0",
    "0x0061ae48",
    "0x00618650",
    "0x006187f0",
    "0x00618bf0",
    "0x00618c50",
    "0x006195d0",
    "0x0060e4c0",
    "0x007f7ef0",
    "0x007f8200",
    "0x007f7af0",
    "0x007f7d40",
    "0x007f7940",
    "0x007f5450",
    "0x007a4610",
    "0x00566380",
    "0x00566d10",
    "0x0055c9b0",
    "0x0055cb60",
    "0x0055cc90",
    "0x0055cdb0",
    "0x00d31d25",
    "0x00547fba",
    "0x0060e860",
    "0x0061f250",
    "0x0060d290",
    "0x00619922",
    "0x00619a2d",
    "0x00619a22",
    "0x00619904",
    "0x007f7e17",
    "0x007f5653",
    "0x007f565c",
    "0x004d7580",
    "0x01378398",
    "0x013783a8",
]


def to_int16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value >= 0x8000 else value


def wheel_payload(notches: int) -> int:
    """The 16-bit value command 0x2A carries for a whole number of wheel notches."""
    return to_int16(notches * WHEEL_SCALE)


def clamp(value: float, low: float, high: float) -> float:
    return low if value < low else high if value > high else value


def third_person_zoom(radius: float, payload: int) -> tuple[float, bool]:
    """Radius after one queue B item and whether the first-person flag is raised."""
    flag = payload > 0 and radius <= RADIUS_MIN
    return clamp(radius - payload / THIRD_PERSON_DIVISOR, RADIUS_MIN, RADIUS_MAX), flag


def first_person_zoom(zoom: float, payload: int) -> tuple[float, bool]:
    """First-person zoom after one queue B item and whether the family flips back."""
    flag = payload < 0 and zoom >= 1.0
    return clamp(zoom - payload / FIRST_PERSON_DIVISOR, 0.0, 1.0), flag


def wrap_angle(angle: float) -> float:
    while angle < -math.pi:
        angle += 2 * math.pi
    while angle > math.pi:
        angle -= 2 * math.pi
    return angle


def third_person_drag(
    pitch: float,
    yaw: float,
    dx: int,
    dy: int,
    sensitivity: float = 1.0,
    invert: int = 0,
    turn_flag: bool = False,
) -> tuple[float, float]:
    """Wow third-person context pitch and yaw after one queue A item.

    ctx +0x10 (turn_flag) mirrors the follow predicate. While set, the handler
    turns the actor instead and leaves yaw alone.
    """
    if invert & 8:
        dx = -dx
    if invert & 4:
        dy = -dy
    pitch = clamp(
        pitch + dy / 100.0 * 0.2 * sensitivity, -PITCH_LIMIT_THIRD, PITCH_LIMIT_THIRD
    )
    if turn_flag:
        return pitch, yaw
    return pitch, wrap_angle(yaw + dx / 100.0 * 0.2 * sensitivity)


def lens_fov(focal: float) -> float:
    """Vertical field of view in radians for the clamped focal byte."""
    focal = clamp(focal, FOCAL_MIN, FOCAL_MAX)
    return 2.0 * math.atan(SENSOR / (2.0 * focal))


def first_person_fov(zoom: float) -> float:
    return FIRST_PERSON_FOV_BASE + FIRST_PERSON_FOV_SCALE * zoom


def projection_fov(context_value: float) -> float:
    """The +0x15C field 0x0060E860 stores for a +0x40C value."""
    return context_value + FIRST_PERSON_FOV_BASE


def dt_from_elapsed(elapsed: int) -> float:
    return elapsed * DT_NUMERATOR / DT_DENOMINATOR


def blend_step(timer: float, dt: float) -> tuple[float, float]:
    """Timer after one interpolator call and the weight kept on the old endpoint."""
    if timer > 0:
        timer = max(timer - dt, 0.0)
        return timer, timer / BLEND_START
    return timer, 0.0


def normalize_key(
    requested: int, free: bool, remap: bool = False, fps_forbidden: bool = False
) -> int | None:
    """The key 0x00618650 stores for a request, or None when the request returns."""
    if fps_forbidden and requested in (1, 3, 5, 7):
        return None
    key = requested
    if free:
        if 4 <= key <= 7:
            key -= 4
    elif 0 <= key <= 3:
        key += 4
    if remap and key <= 7:
        key = 0 if key in (0, 1, 4, 5) else 2
    return key


def family_toggle(key: int, free: bool) -> int | None:
    """Target key 0x006187F0 requests for a current key below 8."""
    if not 0 <= key < 8:
        return None
    unlocked, locked = EXPECTED["contexts"]["familyToggle"][str(key & 3)]
    return unlocked if free else locked


def pointer_request_key(key: int) -> int | None:
    """Key a queue push requests before storing its item."""
    if not 0 <= key < 8:
        return None
    return EXPECTED["pipeline"]["pushRequestsKey"][str(key & 3)]


def move_request_key(key: int) -> int | None:
    """Key command 1 requests for a nonzero move payload."""
    if not 0 <= key < 8:
        return None
    return (key & 3) & 1


def initial_key(free: bool) -> int:
    return 0 if free else 4


def update_arm(key: int) -> str:
    return "tick" if key in (2, 3, 6, 7, 10) else "single"


def doc_errors() -> list[str]:
    if not DOC.is_file():
        return ["camera input chain page is missing"]
    text = DOC.read_text(encoding="utf-8").lower()
    errors = [
        f"page does not cite {address}"
        for address in CITED_ADDRESSES
        if address.lower() not in text
    ]
    for tier in ("static", "inferred"):
        if tier not in text:
            errors.append(f"page does not use the {tier.upper()} tier label")
    if "what bahamut-client can change" not in text:
        errors.append("page lacks the client-change notes")
    return errors


def verify(document: dict | None = None, check_doc: bool = True) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-camera-input-chain-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != EXPECTED_SHA256
        or source.get("size") != EXPECTED_SIZE
        or source.get("imageBase") != "0x00400000"
        or source.get("build") != "2012.09.19.0001"
    ):
        errors.append("evidence identity changed")
    for section in SECTIONS:
        if document.get(section) != EXPECTED[section]:
            errors.append(f"{section} contract changed")
    defaults = document.get("defaults", {})
    if (
        defaults.get("radiusDefault") != RADIUS_DEFAULT
        or defaults.get("radiusMin") != RADIUS_MIN
        or defaults.get("radiusMax") != RADIUS_MAX
    ):
        errors.append("radius constants disagree with the model")
    zoom = document.get("zoom", {}).get("thirdPerson", {})
    if abs(zoom.get("stepPerNotch", 0) - WHEEL_SCALE / THIRD_PERSON_DIVISOR) > 1e-12:
        errors.append("wheel step disagrees with the payload scale and divisor")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    if REQUIRED_CHECKS - set(names):
        errors.append(f"staticChecks lacks {sorted(REQUIRED_CHECKS - set(names))}")
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not re.fullmatch(r"0x[0-9a-f]{8}", str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            try:
                same = len(bytes.fromhex(item["pattern"])) == len(
                    bytes.fromhex(item["mask"])
                )
            except (KeyError, ValueError):
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
    if not document.get("unresolved"):
        errors.append("unresolved list is empty")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]:\\\\", "/" + "Users/", "/" + "home/", "agent-islands")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    if check_doc:
        errors += doc_errors()
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("contexts", "keys", 1, "class"), "CameraActorContextTPS"),
        (("contexts", "keys", 2, "vtableVa"), "0x00ff1164"),
        (("contexts", "noneKey"), 12),
        (("contexts", "familyToggle", "0"), [2, 6]),
        (("contexts", "initial", "predicateZero"), 0),
        (("contexts", "freePredicateZero", "0"), 5),
        (("defaults", "radiusDefault"), 6.0),
        (("defaults", "radiusMin"), 2.0),
        (("defaults", "radiusMax"), 20.0),
        (("defaults", "firstPersonZoom", "initial"), 0.5),
        (("defaults", "invertByte", "thirdPerson", "x"), 4),
        (("pipeline", "tickQueuesEndpointKeys"), [2, 3, 6, 7]),
        (("pipeline", "pushRequestsKey", "1"), 2),
        (("pipeline", "queueB", "containerOffset"), "0x1dc"),
        (("commands", "receiverVa"), "0x0061a6d0"),
        (("commands", "maxId"), 49),
        (("pointer", "buttonStateBytes", "right"), "0x9"),
        (("pointer", "windowsMessages", "0x204"), 2),
        (("pointer", "wheel", "payloadScale"), 120),
        (("pointer", "rightButton", "command"), "0x29"),
        (("pointer", "subscriptions", 3, "strategy"), 2),
        (("zoom", "thirdPerson", "divisor"), 400.0),
        (("zoom", "thirdPerson", "stepPerNotch"), 0.1),
        (("zoom", "thirdPerson", "lockSlot"), 0),
        (("zoom", "firstPerson", "divisor"), 500.0),
        (("rotation", "wowThirdPerson", "pitchLimit"), 1.3962634801864624),
        (("rotation", "wowThirdPerson", "pitchOffset"), "0xc"),
        (("rotation", "axes", "pitchAxis"), "Y"),
        (("follow", "tier"), "STATIC"),
        (("follow", "flagClearVa"), "0x007f5660"),
        (("rotation", "wowThirdPerson", "yawUpdatedWhileFlagSet"), True),
        (("lens", "focalMax"), 35),
        (("lens", "projection", "far"), 1000.0),
        (("lens", "projection", "fovRule"), "+0x40c"),
        (("smoothing", "start"), 60.0),
        (("source", "binarySha256"), "0" * 64),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x61b97a"),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    dropped = copy.deepcopy(document)
    dropped["staticChecks"] = [
        item for item in dropped["staticChecks"] if item["name"] != "WowTPS wheel"
    ]
    mutations.append(dropped)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item, check_doc=False)
    ]


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return image_checks.verify_image(path.read_bytes(), document)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if errors:
        print(f"FAIL: {len(errors)} camera input chain error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: camera input chain manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
