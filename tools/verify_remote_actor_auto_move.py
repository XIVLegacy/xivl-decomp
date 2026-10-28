#!/usr/bin/env python3
"""Validate the remote actor auto-move manifest and optional pinned PE bytes."""

from __future__ import annotations

import argparse
import copy
import json
import re
import struct
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "remote_actor_auto_move.json"
EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
REQUIRED_CHECKS = {
    "speed table receiver",
    "actor command 0x10 speed list",
    "speed slot setter",
    "speed slot getter",
    "auto-move level setter",
    "constructor speed defaults",
    "helper reset clears arrival radius",
    "command byte table 0x10",
    "command byte table 0x26",
    "command byte table 0x34 to 0x36",
    "command arm table 0x0F",
    "command arm table 0x22",
    "command arm table 0x30 to 0x32",
    "arm 0x10 to speed list walker",
    "arm 0x26 to height setter",
    "arm 0x35 to target setter",
    "arm 0x36 to level setter",
    "height setter 0x65CC60",
    "height store 0x7A42B0",
    "s2c 0x00CF case body",
    "0x58F240 first vector sender",
    "0x58F160 second vector sender",
    "target setter 0x7A76C0",
    "catch-up multiplier ramp",
    "remote mover gate",
    "arrival test",
    "arrival level write",
    "chase step",
    "hold re-engage",
    "speed ramp tick",
    "interpolation 0x7A4DB0",
    "displacement scale 0x7A7E50",
    "catch-up early return gate",
    "constant 0.0 double",
    "arrival radius writer",
    "arrival radius sum",
    "slot 166 height add",
    "constant 30.0 double",
    "constant 1/15 float",
    "constant 1/6 float",
    "constant 0.08 double",
    "constant 1.2 float",
    "constant 30.0 float",
    "constant 0.5 double",
    "constant 0.1 double",
    "constant -1.0 float",
}
EXPECTED_MODEL = {
    "wireSpeedList": {
        "opcode": "0x00D0",
        "countByteOffset": 144,
        "recordStride": 8,
        "floatOffset": 16,
        "slotOffset": 20,
        "slotWidthBytes": 2,
        "receiverCopyLimit": 16,
        "terminatorSlot": "0xFFFF",
    },
    "speedTable": {
        "slotCount": 16,
        "tableOffsets": ["0x10C", "0x14C"],
        "divisor": 30.0,
        "defaultPerSecond": [0.0, 2.0, 5.0, 5.0],
        "liveScalarOffsets": {"rampActive": "0xEC", "otherwise": "0xF4"},
        "rampLengthOffset": "0x100",
        "noRampValue": -1.0,
    },
    "autoMove": {
        "levelOffset": "0x80",
        "targetOffset": "0x50",
        "targetHistoryOffsets": ["0x90", "0xA0"],
        "stateOffset": "0x84",
        "stateHold": 3,
        "stateChase": 0,
        "arrivalRadiusOffset": "0x1DC",
        "arrivalRadiusInitial": 0.0,
        "holdReengageDistance": 0.1,
        "remoteGate": "C+0x1104 == 4",
    },
    "catchUp": {
        "triggerFlatDistance": 30.0,
        "timerSteps": 30.0,
        "multiplierCeiling": 1.2,
        "multiplierStepPerStep": 0.08,
        "multiplierOffsets": {"timer": "0x1E0", "current": "0x1E4", "applied": "0xF8"},
    },
    "secondVector": {
        "elementOffsets": ["0xB0", "0xB8"],
        "elementCommand": "0x26",
        "consumedComponent": "y",
        "heightStoreOffsets": {"target": "0x588", "current": "0x58C"},
        "heightObject": "C+0x590",
    },
}


def row_bytes(item: dict) -> bytes:
    if item["kind"] == "dwords":
        width = item.get("width", 4)
        return b"".join(int(v, 16).to_bytes(width, "little") for v in item["values"])
    return bytes.fromhex(item["pattern"])


def rows_by_name(document: dict) -> dict[str, dict]:
    return {item["name"]: item for item in document["staticChecks"]}


def constant(document: dict, name: str, fmt: str) -> float:
    return struct.unpack(fmt, row_bytes(rows_by_name(document)[name]))[0]


def per_step(wire_value: float, divisor: float = 30.0) -> float:
    """Client conversion of a wire speed to the per-step table value."""
    return wire_value / divisor


def catch_up_multiplier(steps_in_window: float, ceiling: float, rate: float) -> float:
    """Multiplier after the given steps inside the catch-up timer window."""
    return min(ceiling, 1.0 + rate * steps_in_window)


def arrival_radius_linear(speed_per_step: float, steps: int) -> float:
    """Stored radius for a clip with an easing byte of 0: sum of the speed ramp."""
    return sum(speed_per_step * (1.0 - i / steps) for i in range(steps + 1))


def model_errors(document: dict) -> list[str]:
    errors: list[str] = []
    if document.get("model") != EXPECTED_MODEL:
        errors.append("model changed")
        return errors
    try:
        divisor = constant(document, "constant 30.0 double", "<d")
        slot1 = constant(document, "constant 1/15 float", "<f")
        slot2 = constant(document, "constant 1/6 float", "<f")
        step = constant(document, "constant 0.08 double", "<d")
        ceiling = constant(document, "constant 1.2 float", "<f")
        timer = constant(document, "constant 30.0 float", "<f")
        no_ramp = constant(document, "constant -1.0 float", "<f")
        reengage = constant(document, "constant 0.1 double", "<d")
    except (KeyError, struct.error, ValueError):
        return ["constant rows are missing or malformed"]
    model = document["model"]
    if divisor != model["speedTable"]["divisor"]:
        errors.append("speed divisor differs from the constant row")
    defaults = [0.0, slot1 * divisor, slot2 * divisor, slot2 * divisor]
    wanted = model["speedTable"]["defaultPerSecond"]
    if any(abs(a - b) > 1e-5 for a, b in zip(defaults, wanted)) or len(defaults) != len(
        wanted
    ):
        errors.append("default speeds differ from the constructor constants")
    if no_ramp != model["speedTable"]["noRampValue"]:
        errors.append("no-ramp value differs from the constant row")
    catch = model["catchUp"]
    if (
        abs(step - catch["multiplierStepPerStep"]) > 1e-6
        or abs(ceiling - catch["multiplierCeiling"]) > 1e-6
        or timer != catch["timerSteps"]
        or divisor != catch["triggerFlatDistance"]
    ):
        errors.append("catch-up constants differ from the constant rows")
    if abs(reengage - model["autoMove"]["holdReengageDistance"]) > 1e-6:
        errors.append("hold re-engage distance differs from the constant row")
    for offset in (0x110, 0x114, 0x118, 0x150, 0x154, 0x158):
        store = b"\xf3\x0f\x11\x86" + struct.pack("<I", offset)
        if (
            store.hex()
            not in row_bytes(rows_by_name(document)["constructor speed defaults"]).hex()
        ):
            errors.append(f"constructor defaults lost the store to H+{offset:#x}")
    if not row_bytes(rows_by_name(document)["arrival radius sum"]).endswith(
        bytes.fromhex("e8744ceeff")
    ):
        errors.append("arrival radius sum lost its call to the radius writer")
    return errors


def census_errors(document: dict) -> list[str]:
    errors: list[str] = []
    census = document.get("census", {})
    cf = census.get("map00CF", {})
    d0 = census.get("map00D0", {})
    if sum(cf.get("byState", {}).values()) != cf.get("total"):
        errors.append("00CF state counts do not sum to the total")
    nonzero = cf.get("secondVectorYNonzero", {})
    if sum(nonzero.get("byState", {}).values()) != nonzero.get("total"):
        errors.append("00CF second-vector counts do not sum")
    if set(nonzero.get("byState", {})) - {"0", "1"}:
        errors.append("00CF second-vector states changed")
    if sum(d0.get("byCountByte", {}).values()) != d0.get("total"):
        errors.append("00D0 count-byte counts do not sum to the total")
    if census.get("captureCount") != 54 or cf.get("subEventBytes") != 80:
        errors.append("capture identity changed")
    if d0.get("subEventBytes") != 168 or d0.get("slot0AlwaysZero") is not True:
        errors.append("00D0 layout changed")
    return errors


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-remote-actor-auto-move-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != EXPECTED_SHA256
        or source.get("bytes") != 15996808
        or source.get("imageBase") != "0x00400000"
        or source.get("addressKind") != "VA"
    ):
        errors.append("binary identity changed")
    checks = document.get("staticChecks", [])
    names = [item.get("name") for item in checks]
    if len(set(names)) != len(names):
        errors.append("static check names are not unique")
    if set(names) != REQUIRED_CHECKS:
        errors.append("static check inventory changed")
    vas = [item.get("va") for item in checks]
    if len(set(vas)) != len(vas):
        errors.append("static check addresses are not unique")
    errors += model_errors(document)
    errors += census_errors(document)
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    if re.search(r"[A-Za-z]:\\\\|(?:\d{1,3}\.){3}\d{1,3}", text):
        errors.append("manifest contains a machine path or address")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []

    def mutate(edit) -> None:
        item = copy.deepcopy(document)
        edit(item)
        mutations.append(item)

    mutate(lambda d: d["staticChecks"].pop())
    mutate(lambda d: d["model"]["speedTable"].update(divisor=60.0))
    mutate(lambda d: d["model"]["speedTable"].update(defaultPerSecond=[0.0, 2.0, 5.0]))
    mutate(lambda d: d["model"]["wireSpeedList"].update(floatOffset=24))
    mutate(lambda d: d["model"]["autoMove"].update(arrivalRadiusInitial=1.0))
    mutate(lambda d: d["model"]["autoMove"].update(remoteGate="always"))
    mutate(lambda d: d["model"]["catchUp"].update(multiplierCeiling=1.5))
    mutate(lambda d: d["model"]["secondVector"].update(consumedComponent="x"))
    mutate(lambda d: d["census"]["map00CF"]["byState"].update({"0": 1}))
    mutate(lambda d: d["census"]["map00D0"]["byCountByte"].update({"4": 1}))
    mutate(lambda d: d["census"]["map00D0"].update(slot0AlwaysZero=False))
    mutate(lambda d: d["source"].update(binarySha256="0" * 64))
    mutate(lambda d: d["staticChecks"][0].update(name="renamed"))

    def retarget(d: dict) -> None:
        item = rows_by_name(d)["constant 1/15 float"]
        item["values"] = ["0x3d000000"]

    mutate(retarget)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
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
        print(f"FAIL: {len(errors)} remote actor auto-move error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " and executable bytes" if args.exe else ""
    print(f"PASS: remote actor auto-move manifest, 14 mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
