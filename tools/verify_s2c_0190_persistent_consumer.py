#!/usr/bin/env python3
"""Validate the sanitized s2c 0x0190 persistent-consumer contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "s2c_0190_persistent_consumer.json"

EXPECTED_ROUTES = [
    ("0x018f", "0x00576c60", "0x0076be30"),
    ("0x0190", "0x00576cd0", "0x0076be60"),
    ("0x0191", "0x00576d40", "0x0076bf10"),
]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-s2c-0190-persistent-consumer-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256")
        != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
        or source.get("ghidraVersion") != "12.1.3"
        or source.get("wireContract")
        != "https://github.com/XIVLegacy/xivl-client-structs/blob/25c9d48d776135eeca8f32314fa90fb9faf9fca4/manifests/unmapped_payload_decoding.json"
    ):
        errors.append("retail evidence identity changed")
    route = document.get("route", {})
    routes = [
        (row.get("opcode"), row.get("wrapperVa"), row.get("workerVa"))
        for row in route.get("opcodes", [])
    ]
    if (
        route.get("containerDispatcherVa") != "0x004dc690"
        or route.get("stateOffset") != "0x510"
        or route.get("managerPointerOffsetFromState") != "0x24"
        or route.get("managerPointerOffsetFromContainer") != "0x534"
        or route.get("scopedKeyConstructorVa") != "0x00cc9320"
        or route.get("scopedKeyDestructorVa") != "0x00cc9330"
        or routes != EXPECTED_ROUTES
    ):
        errors.append("route or manager handoff changed")
    manager = document.get("manager", {})
    if (
        manager.get("stateConstructorVa") != "0x00577fd0"
        or manager.get("stateInitializerVa") != "0x0057a3c0"
        or manager.get("constructorVa") != "0x0076b8f0"
        or manager.get("destructorVa") != "0x00769320"
        or manager.get("primaryMapOffset") != "0x10"
        or manager.get("secondaryMapOffset") != "0x4"
        or manager.get("primaryLookupVa") != "0x0076b950"
        or manager.get("secondaryLookupVa") != "0x0076ba10"
        or manager.get("perKeyStateConstructorVa") != "0x0075f5b0"
        or manager.get("lifetime")
        != "allocated during route-state initialization, shared across all three opcodes, and replaced or destroyed with both maps during route-state reinitialization or teardown"
    ):
        errors.append("manager ownership or map layout changed")
    record = document.get("record", {})
    if (
        record.get("allocationSize") != "0x20"
        or record.get("constructorVa") != "0x00768c40"
        or record.get("headerOffsets") != ["0x8", "0xc"]
        or record.get("vectorOffset") != "0x10"
        or record.get("vectorBeginPointerOffset") != "0x14"
        or record.get("copiedApplicationRange") != "0x08..0x47"
        or record.get("copiedDwordCount") != 16
        or record.get("consumedApplicationSize") != "0x48"
        or record.get("applicationSize") != "0x68"
        or record.get("unreadTailRange") != "0x48..0x67"
        or record.get("unreadTailSize") != "0x20"
        or record.get("tailPointerRetained") is not False
    ):
        errors.append("record projection or unread-tail boundary changed")
    consumer = document.get("consumer", {})
    if (
        consumer.get("enqueueVa") != "0x00766920"
        or consumer.get("commandConstructorVa") != "0x0089b8d0"
        or consumer.get("commandClass")
        != "Application::Lua::Script::Client::Command::Item::ServerOrderUpdateWorkCommand"
        or consumer.get("commandVtableRva") != "0xc57238"
        or consumer.get("commandExecuteSlot") != 9
        or consumer.get("commandExecuteVa") != "0x0089b9e0"
        or consumer.get("selectedEntryLookupVa") != "0x00765cf0"
        or consumer.get("headerUse")
        != "the two-dword header is passed as the ordered-map lookup key"
        or consumer.get("vectorUse")
        != "the 16-dword vector object is passed to the selected entry's virtual method"
        or consumer.get("indirectCallVa") != "0x00765d49"
        or consumer.get("indirectVtableOffset") != "0x2c"
        or consumer.get("staticTargetClass") is not None
        or consumer.get("destructorVa") != "0x0089b940"
        or consumer.get("recordDestructorVa") != "0x00765a90"
    ):
        errors.append("persistent consumer or indirect boundary changed")
    finalization = document.get("finalization", {})
    if (
        finalization.get("workerVa") != "0x0076bf10"
        or finalization.get("workEndEnqueueVa") != "0x0075f910"
        or finalization.get("updateEndEnqueueVa") != "0x0075f7c0"
        or finalization.get("secondaryInsertVa") != "0x00768b10"
        or finalization.get("primaryEraseVa") != "0x007840d0"
    ):
        errors.append("finalization route changed")
    verdict = document.get("verdict", {})
    if (
        verdict.get("nativeNouns") != ["Item", "ServerOrderUpdateWorkCommand"]
        or verdict.get("unsupportedNouns")
        != ["modifier", "batch", "equipment change", "MassSetItemModifier"]
        or verdict.get("equipmentEdge") is not None
        or verdict.get("tailConsumers") != []
        or "0x00765d49" not in verdict.get("negativeBoundary", "")
    ):
        errors.append("semantic verdict or negative boundary changed")
    runtime = document.get("runtimeBoundary", {})
    if (
        runtime.get("breakpointVa") != "0x00765d49"
        or runtime.get("capture")
        != [
            "EDX indirect target",
            "ECX selected receiver",
            "stack argument pointing to the record vector",
        ]
        or runtime.get("watch")
        != [
            "record+0x08",
            "record+0x0c",
            "the 0x40-byte allocation reached through record+0x14",
        ]
    ):
        errors.append("runtime breakpoint contract changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
        "mass_set_item_modifier_cluster",
    )
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a private path or imported catalog source")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("route", "managerPointerOffsetFromState"), "0x28"),
        (("manager", "primaryMapOffset"), "0x14"),
        (("record", "unreadTailSize"), "0x30"),
        (("consumer", "commandExecuteVa"), "0x0089b9b0"),
        (("consumer", "indirectCallVa"), "0x00765d4b"),
        (("verdict", "equipmentEdge"), "invented"),
        (("source", "wireContract"), "https://example.invalid/drift"),
        (("manager", "perKeyStateConstructorVa"), "0x0075f5c0"),
        (("manager", "lifetime"), "packet-local"),
        (("consumer", "headerUse"), "invented"),
        (("consumer", "vectorUse"), "invented"),
        (("consumer", "destructorVa"), "0x0089b950"),
        (("consumer", "recordDestructorVa"), "0x00765aa0"),
    ]:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    dropped_route = copy.deepcopy(document)
    dropped_route["route"]["opcodes"].pop()
    mutations.append(dropped_route)
    invented_tail_reader = copy.deepcopy(document)
    invented_tail_reader["verdict"]["tailConsumers"].append("invented")
    mutations.append(invented_tail_reader)
    invented_class = copy.deepcopy(document)
    invented_class["consumer"]["staticTargetClass"] = "InventedModifierConsumer"
    mutations.append(invented_class)
    changed_watch = copy.deepcopy(document)
    changed_watch["runtimeBoundary"]["watch"].pop()
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
        print(f"FAIL: {len(errors)} s2c 0x0190 persistent-consumer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: s2c 0x0190 persistent-consumer manifest and 18 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
