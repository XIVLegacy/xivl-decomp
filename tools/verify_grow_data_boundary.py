#!/usr/bin/env python3
"""Validate the retail grow-data boundary contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "grow_data_boundary.json"


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-grow-data-boundary-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binary") != "ffxivgame.exe retail 1.23b"
        or source.get("binarySha256")
        != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
        or source.get("ghidraVersion") != "12.1.3"
        or source.get("runId") != "lane3-grow-boundary-20260829-closing"
        or source.get("luaContract")
        != "https://github.com/XIVLegacy/xivl-client-structs/blob/ef57c20da0f3b39ee7b3462450d5389f3981f44f/manifests/lua_api_contract.json"
        or source.get("luaRegistry")
        != "https://github.com/XIVLegacy/xivl-client-scripts/blob/f351794098124d00a673ef2714336ca23eb48d85/lua/registry.json"
        or source.get("luaScriptIdentity")
        != "https://github.com/XIVLegacy/xivl-client-scripts/blob/f351794098124d00a673ef2714336ca23eb48d85/manifests/scripts.json"
        or source.get("luaFormulaBoundary")
        != "https://github.com/XIVLegacy/xivl-client-scripts/blob/f351794098124d00a673ef2714336ca23eb48d85/docs/equipment-parameter-formulas.md"
        or source.get("itemDataBoundary")
        != "https://github.com/XIVLegacy/xivl-client-data/blob/76d68d2036dc99bdda2917e65efcdef4f62f4b63/docs/item-equipment-columns.md"
    ):
        errors.append("evidence identity changed")
    judge = document.get("judgeGrowColumn", {})
    if (
        judge.get("owner") != "CharaBaseClass retail Lua"
        or judge.get("script") != "chara/charabaseclass_parameter"
        or judge.get("functionLine") != 513
        or judge.get("assignmentLine") != 569
        or judge.get("arityIncludingSelf") != 3
        or judge.get("arguments") != ["receiver actor", "comparison actor", "grow selector"]
        or judge.get("nonPlayerToPlayer")
        != [[69, 19], [73, 23], [77, 27], [81, 31], [85, 35], [91, 41], [95, 45], [99, 49], [89, 39]]
        or judge.get("nonPlayerToNonPlayer")
        != [[19, 69], [23, 73], [27, 77], [31, 81], [35, 85], [41, 91], [45, 95], [49, 99], [39, 89]]
        or judge.get("playerReceiverResult") != "selector unchanged"
        or judge.get("unknownSelectorResult") != "selector unchanged"
        or judge.get("validation")
        != "no explicit nil, type, integer, or range validation"
        or judge.get("nativeImplementationVa") is not None
        or judge.get("nativeSlot") is not None
    ):
        errors.append("Lua selector contract changed")
    consumers = document.get("directConsumers", {})
    if (
        consumers.get("itemGrowSourceColumns") != [49, 52, 55, 58]
        or consumers.get("commandGrowSourceColumns") != [42, 47, 52, 57]
        or consumers.get("statusGrowSourceColumns") != [30, 34, 38, 26, 48]
        or consumers.get("negativeSourceSelector")
        != "converted to nil before judgeGrowColumn"
        or consumers.get("getGrowDataArguments")
        != ["lookup receiver", "level", "translated grow selector"]
        or consumers.get("levelArguments")
        != "item or command level and actor skill level adjusted only by the caller"
        or consumers.get("callerLevelDistanceLimits") != [-1, 15]
        or consumers.get("callerHighLevelClamp")
        != "item or command level plus min(15, actor level minus item or command level)"
        or consumers.get("callerRounding") != "none"
    ):
        errors.append("direct consumer or caller-level contract changed")
    boundary = document.get("staticBoundary", {})
    unresolved = (
        "nameToNativeJoin",
        "getGrowDataImplementationVa",
        "tableAddress",
        "tableElementWidth",
        "tableShape",
        "tableProducer",
        "tableOwner",
        "tableLifetime",
        "levelIndexBase",
        "levelClamp",
        "interpolation",
        "lookupRounding",
    )
    if (
        boundary.get("classBootstrapVa") != "0x0078e3a0"
        or boundary.get("classNameVa") != "0x00fe0670"
        or boundary.get("className") != "CharaBaseClass"
        or boundary.get("classPathBuilderVa") != "0x0078eb70"
        or boundary.get("classPathVa") != "0x00fe0a94"
        or boundary.get("classProgPathVa") != "0x00fe0aac"
        or boundary.get("progLoaderVa") != "0x00d0fd70"
        or boundary.get("actorBaseVtableRva") != "0xbd4fe4"
        or boundary.get("charaBaseVtableRva") != "0xbd5cac"
        or boundary.get("misleadingSharedSlot") != 25
        or boundary.get("misleadingSharedSlotVa") != "0x005c5c80"
        or any(boundary.get(key) is not None for key in unresolved)
        or "not native vtable slot 25" not in boundary.get("negativeBoundary", "")
    ):
        errors.append("static negative boundary changed")
    runtime = document.get("runtimeBoundary", {})
    if (
        runtime.get("loaderBreakpointVa") != "0x00d0fd70"
        or runtime.get("loaderPathCondition") != "/Chara/CharaBaseClass.prog"
        or runtime.get("loaderIndirectCallSites")
        != ["0x00d0fe03", "0x00d0fe10", "0x00d0fe26", "0x00d0fe39"]
        or runtime.get("lookupCapture")
        != [
            "break on Lua method lookup for key getGrowData",
            "record the receiver userdata or table and resolved callable",
            "record both numeric arguments before conversion",
            "step through the callable to its first non-VM native frame",
        ]
        or runtime.get("tableWatch")
        != [
            "record every address used to derive the returned numeric value",
            "break on the first write to the backing range in a fresh process",
            "capture allocating owner, element stride, bounds checks, and destruction",
        ]
    ):
        errors.append("runtime recovery plan changed")
    authority = document.get("authority", {})
    if authority != {
        "clientPredictionOnly": True,
        "serverAuthorityEstablished": False,
        "bahamutGrowFormulaReady": False,
    }:
        errors.append("authority boundary changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
    )
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine or private-island path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("judgeGrowColumn", "nativeImplementationVa"), "0x005c5c80"),
        (("judgeGrowColumn", "arityIncludingSelf"), 2),
        (("directConsumers", "callerRounding"), "floor"),
        (("staticBoundary", "getGrowDataImplementationVa"), "invented"),
        (("staticBoundary", "tableElementWidth"), 4),
        (("staticBoundary", "levelIndexBase"), 1),
        (("staticBoundary", "interpolation"), "linear"),
        (("runtimeBoundary", "loaderBreakpointVa"), "0x00d0fd71"),
        (("authority", "serverAuthorityEstablished"), True),
        (("source", "luaRegistry"), "https://example.invalid/registry"),
        (("judgeGrowColumn", "unknownSelectorResult"), "nil"),
        (("directConsumers", "callerHighLevelClamp"), "invented"),
    ]:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    dropped_mapping = copy.deepcopy(document)
    dropped_mapping["judgeGrowColumn"]["nonPlayerToPlayer"].pop()
    mutations.append(dropped_mapping)
    dropped_consumer = copy.deepcopy(document)
    dropped_consumer["directConsumers"]["statusGrowSourceColumns"].pop()
    mutations.append(dropped_consumer)
    changed_lookup = copy.deepcopy(document)
    changed_lookup["runtimeBoundary"]["lookupCapture"][0] = "invented lookup"
    mutations.append(changed_lookup)
    changed_watch = copy.deepcopy(document)
    changed_watch["runtimeBoundary"]["tableWatch"][0] = "invented address"
    mutations.append(changed_watch)
    leaked = copy.deepcopy(document)
    leaked["source"]["path"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} grow-data boundary error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: grow-data boundary manifest and 17 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
