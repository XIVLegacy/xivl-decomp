#!/usr/bin/env python3
"""Validate the sun and moon orientation contract and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import json
import re
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "sun_moon_orientation.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
SECTIONS = ("orientation", "moonPhase", "roll")
EXPECTED = {
    "orientation": {
        "functionVa": "0x008d3400",
        "updateVa": "0x008d3990",
        "callSiteVa": "0x008d3a09",
        "skipWhen": [
            "clip +0x40 equals 4",
            "record +0x28 equals zero",
            "container lookup returns null",
            "container +0x64 is null",
        ],
        "modeOffset": "0x40",
        "recordFlagOffset": "0x28",
        "cameraSlot": {
            "containerOffset": "0x64",
            "cacheType": "0x0c",
            "factoryVa": "0x0079f7e0",
            "constructorVa": "0x0061b5a0",
            "positionVtableOffset": "0x88",
        },
        "lookAt": {
            "builderVa": "0x008d2c90",
            "fourthArgument": "normalized camera minus node vector",
            "fifthArgument": "zero vector",
            "sixthArgument": "0,1,0,0",
            "tier": "INFERRED",
        },
        "rotationSetVtableOffset": "0x3c",
    },
    "moonPhase": {
        "clockSelectorVa": "0x007e0430",
        "managerGlobal": "0x0133def4",
        "managerOffset": "0x158",
        "clockOffsets": ["0x108", "0x120"],
        "clockModeOffset": "0xf4",
        "clockUnit": "Eorzea seconds",
        "offsetSeconds": 43200,
        "divisors": [60, 60, 24],
        "shift": 2,
        "mask": 7,
        "formula": "(((T + 43200) / 86400) >> 2) & 7",
        "override": {
            "offset": "0x130",
            "default": "0xffffffff",
            "setterVa": "0x007e06d0",
            "getterVa": "0x007e06e0",
            "message": 116,
            "receiverVa": "0x0062cda0",
            "usedWhen": "signed value is nonnegative",
            "masked": False,
        },
    },
    "roll": {
        "builderVa": "0x0042fdc0",
        "axis": "Y",
        "angle": "index * 45 * pi / 180 radians",
        "matrix": "(cos 0 -sin 0)(0 1 0 0)(sin 0 cos 0)(0 0 0 1)",
        "composeVa": "0x0061d050",
        "quaternionVa": "0x007dd840",
    },
}
REQUIRED_CHECKS = {
    "Y rotation builder",
    "camera delta",
    "camera factory construction",
    "camera factory registration",
    "camera object lookup",
    "clock selector",
    "constant 1.0",
    "constant 180.0",
    "constant 45.0",
    "constant pi",
    "container cache jump entry 1",
    "container cache store",
    "container cache type index",
    "cosine routine",
    "look-at arguments",
    "manager accessor",
    "manager global getter",
    "moon phase index",
    "node position",
    "orientation entry gates",
    "override default",
    "override getter",
    "override setter",
    "roll angle",
    "rotation composition",
    "scene message 116 arm",
    "scene message 116 index",
    "scene message 116 target",
    "scene message switch",
    "sine routine",
    "update calls orientation",
}
U64 = (1 << 64) - 1
SECONDS_OFFSET = 43200
SECONDS_PER_DAY = 86400
PHASE_COUNT = 8
DAYS_PER_PHASE = 4


def moon_phase_index(eorzea_seconds: int, override: int = -1) -> int:
    """Index 0x008D3400 feeds the roll angle for an Eorzea clock and the override cell."""
    signed = override - (1 << 32) if override >= 1 << 31 else override
    if signed >= 0:
        return signed
    day = ((eorzea_seconds + SECONDS_OFFSET) & U64) // SECONDS_PER_DAY
    return (day // DAYS_PER_PHASE) % PHASE_COUNT


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-sun-moon-orientation-v1":
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
    phase = document.get("moonPhase", {})
    if (
        phase.get("offsetSeconds") != SECONDS_OFFSET
        or phase.get("shift") != 2
        or phase.get("mask") != PHASE_COUNT - 1
        or phase.get("divisors") != [60, 60, 24]
    ):
        errors.append("moon phase constants disagree with the model")
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
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("orientation", "skipWhen", 1), "record +0x28 is nonzero"),
        (("orientation", "recordFlagOffset"), "0x2c"),
        (("orientation", "modeOffset"), "0x44"),
        (("orientation", "cameraSlot", "containerOffset"), "0x68"),
        (("orientation", "cameraSlot", "cacheType"), "0x0d"),
        (("orientation", "lookAt", "tier"), "STATIC"),
        (("orientation", "lookAt", "sixthArgument"), "0,0,1,0"),
        (("moonPhase", "offsetSeconds"), 0),
        (("moonPhase", "shift"), 3),
        (("moonPhase", "mask"), 3),
        (("moonPhase", "divisors"), [60, 60, 12]),
        (("moonPhase", "clockOffsets"), ["0x108", "0x128"]),
        (("moonPhase", "override", "default"), "0x00000000"),
        (("moonPhase", "override", "masked"), True),
        (("moonPhase", "override", "message"), 106),
        (("moonPhase", "override", "usedWhen"), "signed value is positive"),
        (("roll", "axis"), "Z"),
        (("roll", "angle"), "index * 90 * pi / 180 radians"),
        (("source", "binarySha256"), "0" * 64),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x8d3425"),
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
        item for item in dropped["staticChecks"] if item["name"] != "moon phase index"
    ]
    mutations.append(dropped)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
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
        print(f"FAIL: {len(errors)} sun and moon orientation error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: sun and moon orientation manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
