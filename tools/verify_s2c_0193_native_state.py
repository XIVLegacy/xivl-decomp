#!/usr/bin/env python3
"""Validate the sanitized s2c 0x0193 native-state contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


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
    ["ChangeTargetNext", "ChangeTargetPrev", "ChangeEnemyTargetNext", "ChangeEnemyTargetPrev", "ChangeBattleTargetNext", "ChangeBattleTargetPrev", "ChangeTargetMode"],
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


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-s2c-0193-native-state-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
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
        or owner.get("raptureUserControlClass") != "Application::Main::SqwtInterface::RaptureUserControl"
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
        "0x0075f420", "0x0075d220", "0x0075d240", "0x0075d260", "0x0075d280"
    ]:
        errors.append("timer reader map changed")
    groups = document.get("raptureUserControl", {}).get("groups", [])
    methods = [
        (row.get("recordOffset"), row.get("countOffset"), row.get("setupVa"), row.get("decrementVa"), row.get("countReaderVa"))
        for row in groups
    ]
    if [row.get("ordinal") for row in groups] != [1, 2, 3, 4] or methods != EXPECTED_GROUP_METHODS:
        errors.append("RaptureUserControl group pairing changed")
    if [row.get("commands") for row in groups] != EXPECTED_COMMANDS:
        errors.append("RaptureUserControl command membership changed")
    if [row.get("setupRegistrations") for row in groups] != EXPECTED_REGISTRATIONS:
        errors.append("RaptureUserControl setup registrations changed")
    action = document.get("actionCheck", {})
    consumers = action.get("nativeConsumers", [])
    if (
        action.get("subopcode") != "0x13"
        or action.get("fieldPath") != ["state+0x4", "+0xec", "+0x38"]
        or [row.get("consumerVa") for row in consumers] != ["0x00578390", "0x005785d0"]
        or [row.get("gatedCalleeVa") for row in consumers] != ["0x00582bc0", "0x00583290"]
        or action.get("luaOrNapiConsumers") != []
        or "computed indirect" not in action.get("negativeBoundary", "")
    ):
        errors.append("ActionCheck consumer boundary changed")
    unresolved = document.get("unresolved", [])
    if not any("timer units" == item for item in unresolved) or not any("high-level nouns" in item for item in unresolved):
        errors.append("unresolved semantic boundary changed")
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
    invented_registration["raptureUserControl"]["groups"][1]["setupRegistrations"].append("ChangeTargetNext")
    mutations.append(invented_registration)
    dropped_consumer = copy.deepcopy(document)
    dropped_consumer["actionCheck"]["nativeConsumers"].pop()
    mutations.append(dropped_consumer)
    leaked = copy.deepcopy(document)
    leaked["source"]["path"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    return [f"mutation {index} was accepted" for index, item in enumerate(mutations, 1) if not verify(item)]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} s2c 0x0193 native-state error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: s2c 0x0193 native-state manifest and 9 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
