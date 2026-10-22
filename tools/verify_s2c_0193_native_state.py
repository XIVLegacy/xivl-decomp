#!/usr/bin/env python3
"""Validate the sanitized s2c 0x0193 native-state contract."""

from __future__ import annotations

import argparse
import copy
import json
import re
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "s2c_0193_native_state.json"
EXPECTED_TIMER_APIS = [
    ("0x00..0x0f", 88, "_getOccupancyContentsTime"),
    ("0x10", 89, "_getNormalBehestTime"),
    ("0x11", 90, "_getCompanyBehestTime"),
    ("0x12", 91, "_getWarpRecastTime"),
    ("0x16", 132, "_getNMRushUpdateTime"),
]
EXPECTED_COMMANDS = [
    ["MoveCharacter", "ShiftMoveCharacter", "MoveCharacterAutoRun"],
    [
        "ChangeTargetNext",
        "ChangeTargetPrev",
        "ChangeEnemyTargetNext",
        "ChangeEnemyTargetPrev",
        "ChangeBattleTargetNext",
        "ChangeBattleTargetPrev",
        "ChangeTargetMode",
    ],
    ["LockTarget", "MoveCamera", "ChangeCameraMode", "ChangeCameraLock"],
    ["ForwardCameraOn", "ForwardCameraOff", "BackwardCameraOn", "BackwardCameraOff"],
]
EXPECTED_REGISTRATIONS = [
    ["MoveCharacter", "ShiftMoveCharacter"],
    [],
    [],
    ["MoveCamera", "ForwardCameraOff", "BackwardCameraOff"],
]
EXPECTED_GROUP_METHODS = [
    ("0x08", "0x18", "0x0054b440", "0x0054b600", "0x0054b610"),
    ("0x1c", "0x2c", "0x0054b620", "0x0054b630", "0x0054b640"),
    ("0x30", "0x40", "0x0054b650", "0x0054b660", "0x0054b670"),
    ("0x44", "0x54", "0x0054b680", "0x0054b8c0", "0x0054b8d0"),
]
REQUIRED_CHECKS = {
    "dispatcher call site arguments",
    "value sentinel and clock sum",
    "occupancy vector store",
    "extended subopcode switch",
    "extended subopcode jump table",
    "store arm 0x10",
    "store arm 0x11",
    "store arm 0x12",
    "store arm 0x16",
    "word writer +0x10",
    "word writer +0x14",
    "word writer +0x18",
    "word writer +0x1c",
    "occupancy vector writer",
    "ActionCheck sentinel branch",
    "ActionCheck raw store",
    "subopcode 0x14 arm",
    "subopcode 0x15 arm",
    "subopcode 0x15 guard",
}
EXPECTED_JUMP_TABLE = [
    "0x00578d0a",
    "0x00578d1e",
    "0x00578d32",
    "0x00578d5a",
    "0x00578f93",
    "0x00578fb8",
    "0x00578d46",
]
EXPECTED_HANDLER = {
    "handlerVa": "0x00578c90",
    "callSiteVa": "0x004dd37d",
    "arguments": [
        {"index": 0, "name": "subopcode", "source": "message+0x10"},
        {"index": 1, "name": "value", "source": "message+0x14"},
        {"index": 2, "name": "clock", "source": "message+0x08"},
    ],
    "sentinel": "0xffffffff",
    "wordRule": "sentinel stores zero, any other value stores clock plus value modulo 2^32",
    "storeSubopcodes": ["0x00..0x0f", "0x10", "0x11", "0x12", "0x16"],
    "ignoredAbove": "0x16",
    "actionCheckStore": {
        "sentinel": "query, no store",
        "otherValues": "raw value argument stored at terminal+0x38, clock not added",
    },
    "userControl": {
        "flagOffset": "0x3a",
        "excludedValues": ["0x00", "0x15", "0x16"],
        "setup": {
            "subopcode": "0x14",
            "requiresFlag": 0,
            "callVa": "0x0075b300",
            "flagAfter": 1,
        },
        "teardown": {
            "subopcode": "0x15",
            "requiresFlag": 1,
            "guardVa": "0x00576020",
            "callVa": "0x0075b360",
            "flagAfter": 0,
        },
    },
}
SENTINEL = 0xFFFFFFFF
USER_CONTROL_EXCLUDED = (0x00, 0x15, 0x16)


def stored_word(subopcode: int, value: int, clock: int) -> int | None:
    """Word the handler stores for a timer store subopcode, None when it stores none."""
    if subopcode > 0x16 or subopcode in (0x13, 0x14, 0x15):
        return None
    return 0 if value == SENTINEL else (clock + value) & 0xFFFFFFFF


def action_check_store(value: int) -> int | None:
    """Raw word stored at terminal+0x38 for subopcode 0x13, None for the query."""
    return None if value == SENTINEL else value


def user_control_effect(subopcode: int, value: int, flag: int) -> int | None:
    """New value of the flag byte at +0x3a, None when the handler leaves it alone."""
    if value in USER_CONTROL_EXCLUDED:
        return None
    if subopcode == 0x14 and flag == 0:
        return 1
    if subopcode == 0x15 and flag != 0:
        return 0
    return None


EXPECTED_ACTION_CONSUMERS = [
    (
        "0x00578390",
        "0x00582bc0",
        "insert caller-supplied actor handle if absent",
        "allocates and links one 0x14-byte ordered-container node only when the key is absent",
        ["0x00587210", "0x00587370", "0x005873e0", "0x005874b0"],
        "returned iterator remains local and unused",
        "record staging before queue insertion",
    ),
    (
        "0x005785d0",
        "0x00583290",
        "erase every ordered-container entry equal to the caller-supplied actor handle",
        "unlinks and frees the equal range and decrements the container count",
        ["0x00585af0", "0x0058c220"],
        "erased-entry count is ignored",
        "queued-record drain",
    ),
]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-s2c-0193-native-state-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256")
        != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
        or source.get("ghidraVersion") != "12.1.3"
    ):
        errors.append("retail evidence identity changed")
    route = document.get("route", {})
    if (
        route.get("opcode") != "0x0193"
        or route.get("containerDispatcherVa") != "0x004dc690"
        or route.get("stateDispatcherVa") != "0x00578c90"
        or route.get("stateMemberOffset") != "0x510"
        or route.get("stateMemberSize") != "0x3c"
    ):
        errors.append("route or embedded state boundary changed")
    owner = document.get("owner", {})
    if (
        owner.get("allocationSize") != "0x17d58"
        or owner.get("containerOffset") != "0x10"
        or owner.get("containerClass") != "Application::Main::RaptureElementContainer"
        or owner.get("raptureUserControlOffset") != "0x17758"
        or owner.get("raptureUserControlSize") != "0x58"
        or owner.get("raptureUserControlClass")
        != "Application::Main::SqwtInterface::RaptureUserControl"
        or owner.get("raptureUserControlAccessorVa") != "0x004d7580"
        or owner.get("nextContainerMemberOffset") != "0x177b0"
    ):
        errors.append("owner lineage or layout changed")
    timer = document.get("timerState", {})
    fields = timer.get("fields", [])
    timer_apis = [
        (row.get("subopcodes"), row.get("myPlayerVtableSlot"), row.get("luaApi"))
        for row in fields
    ]
    if timer.get("terminalStateClass") is not None or timer_apis != EXPECTED_TIMER_APIS:
        errors.append("timer consumer map or unresolved class boundary changed")
    if [row.get("readerVa") for row in fields] != [
        "0x0075f420",
        "0x0075d220",
        "0x0075d240",
        "0x0075d260",
        "0x0075d280",
    ]:
        errors.append("timer reader map changed")
    groups = document.get("raptureUserControl", {}).get("groups", [])
    methods = [
        (
            row.get("recordOffset"),
            row.get("countOffset"),
            row.get("setupVa"),
            row.get("decrementVa"),
            row.get("countReaderVa"),
        )
        for row in groups
    ]
    if [row.get("ordinal") for row in groups] != [
        1,
        2,
        3,
        4,
    ] or methods != EXPECTED_GROUP_METHODS:
        errors.append("RaptureUserControl group pairing changed")
    if [row.get("commands") for row in groups] != EXPECTED_COMMANDS:
        errors.append("RaptureUserControl command membership changed")
    if [row.get("setupRegistrations") for row in groups] != EXPECTED_REGISTRATIONS:
        errors.append("RaptureUserControl setup registrations changed")
    action = document.get("actionCheck", {})
    consumers = action.get("nativeConsumers", [])
    consumer_map = [
        (
            row.get("consumerVa"),
            row.get("gatedCalleeVa"),
            row.get("operation"),
            row.get("mutation"),
            row.get("directCallers"),
            row.get("resultUse"),
            row.get("callerPhase"),
        )
        for row in consumers
    ]
    if (
        action.get("subopcode") != "0x13"
        or action.get("fieldPath") != ["state+0x4", "+0xec", "+0x38"]
        or action.get("stateObjectConstructorVa") != "0x00773270"
        or action.get("terminalObjectOffset") != "0xec"
        or action.get("terminalObjectSize") != "0x3c"
        or action.get("terminalObjectConstructorVa") != "0x0076fc60"
        or action.get("terminalObjectClass") is not None
        or action.get("orderedContainerOffset") != "0x18"
        or action.get("predicate") != "signed-greater-than-zero"
        or action.get("suppressedValues") != "zero and negative"
        or action.get("selectorExclusions")
        != [
            "0x7c000062",
            "0x10000000..0x10ffffff",
            "0x14000000..0x14ffffff",
        ]
        or consumer_map != EXPECTED_ACTION_CONSUMERS
        or action.get("positiveEffect")
        != "executes the listed local ordered-container mutation"
        or action.get("suppressedEffect")
        != "returns without changing that ordered container"
        or action.get("networkEmissions") != []
        or action.get("luaOrNapiConsumers") != []
        or action.get("publicResultConsumers") != []
        or "not gated" not in action.get("surroundingOperationsBoundary", "")
        or "computed or dynamic indirect" not in action.get("negativeBoundary", "")
    ):
        errors.append("ActionCheck consumer boundary changed")
    unresolved = document.get("unresolved", [])
    if not any("timer units" == item for item in unresolved) or not any(
        "high-level nouns" in item for item in unresolved
    ):
        errors.append("unresolved semantic boundary changed")
    if document.get("timerHandler") != EXPECTED_HANDLER:
        errors.append("timer handler store rules changed")
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
    tables = [
        item for item in checks if item.get("name") == "extended subopcode jump table"
    ]
    if [item.get("values") for item in tables] != [EXPECTED_JUMP_TABLE]:
        errors.append("extended subopcode jump table changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
        "xivl-opcodes",
        "SetControlStatePacket",
    )
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a private path or downstream import")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("route", "stateMemberOffset"), "0x514"),
        (("owner", "raptureUserControlOffset"), "0x1775c"),
        (("timerState", "terminalStateClass"), "InventedState"),
        (("actionCheck", "luaOrNapiConsumers"), ["invented"]),
        (("actionCheck", "predicate"), "nonzero"),
        (("actionCheck", "networkEmissions"), ["invented"]),
    ]:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    dropped_timer = copy.deepcopy(document)
    dropped_timer["timerState"]["fields"].pop()
    mutations.append(dropped_timer)
    moved_command = copy.deepcopy(document)
    moved_command["raptureUserControl"]["groups"][2]["commands"].remove("LockTarget")
    moved_command["raptureUserControl"]["groups"][1]["commands"].append("LockTarget")
    mutations.append(moved_command)
    invented_registration = copy.deepcopy(document)
    invented_registration["raptureUserControl"]["groups"][1][
        "setupRegistrations"
    ].append("ChangeTargetNext")
    mutations.append(invented_registration)
    dropped_consumer = copy.deepcopy(document)
    dropped_consumer["actionCheck"]["nativeConsumers"].pop()
    mutations.append(dropped_consumer)
    changed_operation = copy.deepcopy(document)
    changed_operation["actionCheck"]["nativeConsumers"][0]["operation"] = "emit action"
    mutations.append(changed_operation)
    changed_phase = copy.deepcopy(document)
    changed_phase["actionCheck"]["nativeConsumers"][1]["callerPhase"] = "packet send"
    mutations.append(changed_phase)
    leaked = copy.deepcopy(document)
    leaked["source"]["path"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    for path, value in [
        (("timerHandler", "sentinel"), "0x00000000"),
        (("timerHandler", "wordRule"), "sentinel leaves the slot unchanged"),
        (("timerHandler", "ignoredAbove"), "0x17"),
    ]:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    raw_clock = copy.deepcopy(document)
    raw_clock["timerHandler"]["actionCheckStore"]["otherValues"] = "clock sum stored"
    mutations.append(raw_clock)
    guard = copy.deepcopy(document)
    guard["timerHandler"]["userControl"]["excludedValues"] = ["0x00"]
    mutations.append(guard)
    inverted = copy.deepcopy(document)
    inverted["timerHandler"]["userControl"]["teardown"]["requiresFlag"] = 0
    mutations.append(inverted)
    table = copy.deepcopy(document)
    for item in table["staticChecks"]:
        if item["name"] == "extended subopcode jump table":
            item["values"][3] = "0x00578d5b"
    mutations.append(table)
    dropped_check = copy.deepcopy(document)
    dropped_check["staticChecks"] = [
        item
        for item in dropped_check["staticChecks"]
        if item["name"] != "ActionCheck raw store"
    ]
    mutations.append(dropped_check)
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
        print(f"FAIL: {len(errors)} s2c 0x0193 native-state error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: s2c 0x0193 native-state manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
