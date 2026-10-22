#!/usr/bin/env python3
"""Validate the player profile accessor contract and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import json
import re
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "player_profile_accessors.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
SECTIONS = ("dispatch", "ownerChain", "specialEventWork", "titleAndJob")
EXPECTED = {
    "dispatch": {
        "gate": {
            "byteFlagOffset": "0x54b",
            "predicateVa": "0x00575740",
            "passedOpcode": "0x0004",
            "exitVa": "0x004dd3a9",
        },
        "routeStateOffset": "0x510",
        "stateArms": [
            {
                "opcode": "0x0193",
                "armVa": "0x004dd36f",
                "handlerVa": "0x00578c90",
                "tableIndex": 42,
            },
            {
                "opcode": "0x0196",
                "armVa": "0x004dd384",
                "handlerVa": "0x00576050",
                "tableIndex": 43,
            },
            {
                "opcode": "0x01a3",
                "armVa": "0x004dd391",
                "handlerVa": "0x00576140",
                "tableIndex": 45,
            },
        ],
        "actorIdPassed": False,
        "actorLookupArm": {
            "armVa": "0x004dcfff",
            "tableIndex": 1,
            "lookupVa": "0x004d9910",
            "objectSlotOffset": "0x24",
            "opcodes": ["0x0194", "0x019d", "0x01a4"],
        },
    },
    "ownerChain": {
        "stateOffset": "0x10c",
        "producer": "[[routeState+0x4]+0x10c]",
        "reader": "[[[context+0x4] dereferenced]+0x4]+0x10c",
        "ownerSetup": {
            "constructorCallVa": "0x0057a9e9",
            "storeVa": "0x0057aa1a",
            "storeOffset": "0x4",
            "handoffCallVa": "0x0057aab1",
            "handoffVa": "0x007634b0",
        },
        "currentPlayer": {
            "getterVa": "0x00707970",
            "playerOffset": "0x0c",
            "storeVa": "0x00758e40",
        },
        "tiers": {
            "producerAndHolderShareSetupOwner": "STATIC",
            "setupContextEqualsCallbackContext": "INFERRED",
            "playerOffsetStore": "STATIC",
        },
    },
    "specialEventWork": {
        "writerVa": "0x00576050",
        "setterVa": "0x0075d2d0",
        "getterVa": "0x00707cc0",
        "flagByteSource": "payload+0x01",
        "bitOrder": "bit k to index k+1",
        "wordSources": [
            "payload+0x02",
            "payload+0x04",
            "payload+0x06",
            "payload+0x08",
            "payload+0x0a",
            "payload+0x0c",
            "payload+0x0e",
            "payload+0x10",
        ],
        "wordIndices": "9..16",
        "byteStorageOffset": "0x84",
        "wordStorageOffset": "0x8c",
        "routeStateCopy": {"offset": "0x38", "width": 2, "source": "payload+0x02"},
        "getter": {
            "booleanIndices": "1..8",
            "wordIndices": "every other index",
            "rangeCheck": False,
            "byteHelperVa": "0x0075d390",
            "wordHelperVa": "0x0075d3a0",
            "booleanWrapperVa": "0x00748870",
            "booleanConstructorVa": "0x00583e30",
            "booleanTag": 2,
            "wordWrapperVa": "0x00584f70",
            "wordConstructorVa": "0x00583d90",
            "wordTag": 0,
            "indexReaderVa": "0x005819a0",
        },
        "outOfRange": [
            {"index": 0, "address": "state+0x7a"},
            {"index": 17, "address": "state+0x9c"},
        ],
    },
    "titleAndJob": {
        "title": {
            "opcode": "0x019d",
            "receiverVa": "0x0089c5f0",
            "field": "PlayerBase+0xe8",
            "width": 4,
            "readerVa": "0x00706d70",
            "registrationVa": "0x0074f620",
            "luaApi": "_getAchievementTitle",
            "valueTag": 0,
            "nullTestOnCast": False,
        },
        "job": {
            "opcode": "0x01a4",
            "receiverVa": "0x0089cb60",
            "setterVa": "0x00706dc0",
            "field": "PlayerBase+0xf1",
            "width": 1,
            "readerVa": "0x00706d90",
            "vtableSlot": 39,
            "vtableCellVas": ["0x00fd5ea0", "0x00fd78f8"],
            "luaApi": "_getJob",
            "luaBindingClass": "CharaBaseClass",
            "thunkVa": "0x0071e020",
            "signed": True,
            "valueTag": 0,
            "storesOnlyWhenChanged": True,
            "nonPlayerStubVa": "0x00776340",
            "nonPlayerStubCellVas": ["0x00fd5d48", "0x00fd6518"],
            "nonPlayerReturnsValue": False,
        },
    },
}
REQUIRED_CHECKS = {
    "Boolean value constructor tail",
    "Boolean wrapper call",
    "_getAchievementTitle registration",
    "_getJob thunk",
    "actor-lookup arm",
    "current player getter",
    "cutscene book wrapper",
    "dispatcher arm 0x0196",
    "dispatcher arm 0x01A3",
    "dispatcher gate, byte flag",
    "dispatcher gate, route state predicate",
    "high opcode index bytes 0x0193..0x01A4",
    "high target address entries 42..46",
    "high target address entry 1",
    "job reader",
    "job reader non-player stub",
    "job reader vtable cell, CharaBase",
    "job reader vtable cell, MyPlayer",
    "job reader vtable cell, NpcBase",
    "job reader vtable cells",
    "job receiver",
    "job setter changed-value gate",
    "numeric value constructor tail",
    "numeric wrapper call",
    "owner player store",
    "route state owner handoff",
    "route state owner setup",
    "route state receiver base",
    "special event work getter",
    "special event work setter",
    "special event work writer",
    "title reader",
    "title receiver",
    "work byte helper",
    "work word helper",
}
FLAG_BYTE_BASE = 0x84
WORD_BASE = 0x8C


def work_slot(index: int) -> tuple[str, int]:
    """Kind and state offset the work getter reads for a Lua index (32-bit wrap)."""
    value = index & 0xFFFFFFFF
    if (value - 1) & 0xFFFFFFFF <= 7:
        return "byte", FLAG_BYTE_BASE + value - 1
    return "word", (WORD_BASE + 2 * ((value - 9) & 0xFFFFFFFF)) & 0xFFFFFFFF


def expand_flags(flag_byte: int) -> list[bool]:
    """Boolean work values for indices 1 to 8 from payload byte +1."""
    return [bool((flag_byte >> bit) & 1) for bit in range(8)]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-player-profile-accessors-v1":
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
    getter = document.get("specialEventWork", {}).get("getter", {})
    for probe in document.get("specialEventWork", {}).get("outOfRange", []):
        offset = work_slot(probe.get("index", 0))[1]
        if probe.get("address") != f"state+{offset:#x}":
            errors.append("out-of-range address disagrees with the getter model")
    if getter.get("booleanTag") == getter.get("wordTag"):
        errors.append("boolean and word tags are not distinct")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]:\\\\", "/" + "Users/", "/" + "home/", "agent-islands")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("dispatch", "actorIdPassed"), True),
        (("dispatch", "stateArms", 1, "handlerVa"), "0x00576140"),
        (("dispatch", "actorLookupArm", "opcodes"), ["0x0194", "0x019d"]),
        (("dispatch", "gate", "passedOpcode"), "0x0005"),
        (("ownerChain", "stateOffset"), "0x110"),
        (("ownerChain", "tiers", "setupContextEqualsCallbackContext"), "STATIC"),
        (("ownerChain", "currentPlayer", "playerOffset"), "0x10"),
        (("specialEventWork", "bitOrder"), "bit k to index 8 - k"),
        (("specialEventWork", "getter", "rangeCheck"), True),
        (("specialEventWork", "getter", "booleanTag"), 0),
        (("specialEventWork", "getter", "wordTag"), 2),
        (("specialEventWork", "getter", "booleanIndices"), "1..9"),
        (("specialEventWork", "wordIndices"), "8..15"),
        (("specialEventWork", "outOfRange", 0, "address"), "state+0x7c"),
        (("titleAndJob", "job", "signed"), False),
        (("titleAndJob", "job", "field"), "PlayerBase+0xf0"),
        (("titleAndJob", "title", "width"), 2),
        (("titleAndJob", "title", "nullTestOnCast"), True),
        (("titleAndJob", "job", "vtableSlot"), 38),
        (("titleAndJob", "job", "nonPlayerReturnsValue"), True),
        (("titleAndJob", "job", "nonPlayerStubVa"), "0x00706d90"),
        (("titleAndJob", "job", "luaBindingClass"), "PlayerBaseClass"),
        (("source", "binarySha256"), "0" * 64),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x4dc6d5"),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    for name in ("job reader", "job reader non-player stub"):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
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
        print(f"FAIL: {len(errors)} player profile accessor error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: player profile accessor manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
