#!/usr/bin/env python3
"""Validate the sanitized s2c 0x018D client-consumer contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "s2c_018d_client_consumer.json"
EXPECTED_PROJECTION = [
    ("+0x00", "+0x00"),
    ("+0x08", "+0x08"),
    ("+0x0c", "+0x0c"),
    ("+0x14", "+0x10"),
    ("+0x18", "+0x14"),
    ("+0x1c", "+0x18"),
]
EXPECTED_HEADER = [
    ("+0x00", "+0x08", 4, None),
    ("+0x04", "+0x0c", 4, None),
    ("+0x08", "+0x10", 4, None),
    ("+0x290", "+0x14", 1, "signed-byte widen"),
]
EXPECTED_READERS = [
    ("+0x08,+0x0c,+0x10", ("0x0055cf70",), (), (), "no outward reader in the exact direct, field, data, vtable, or generated-call census"),
    ("+0x14", ("0x0055f830", "0x0055cf70"), ("0x0055d020", "0x0055d0d0"), ("0x00671400", "0x00691f30"), None),
    ("+0x00", (), ("0x0055d090",), ("0x00671400",), None),
    ("+0x08,+0x0c", (), ("0x0055d0b0",), ("0x00671400",), None),
    ("+0x10,+0x18", (), ("0x0055d050",), ("0x00671400",), "+0x14 is projected but has no separate load in the first consumer"),
    ("+0x20", (), ("0x0055d030",), ("0x00671400",), None),
    ("+0x74", (), ("0x0055d070",), ("0x00671400",), None),
    ("remaining tail", (), (), (), "no additional first-consumer load is proven outside the +0x20 helper object and +0x74 scalar"),
    ("+0x798", ("0x0055f830", "0x0055cf70", "0x0055d0f0"), ("0x0055d0d0",), ("0x00691f30",), None),
]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-s2c-018d-client-consumer-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
        or source.get("ghidraVersion") != "12.1.3"
    ):
        errors.append("retail evidence identity changed")
    wire = document.get("wireContract", {})
    if (
        wire.get("opcode") != "0x018D"
        or wire.get("neutralName") != "_0x018D"
        or wire.get("commit") != "67b709d5ffd90b8dc10a699e608fa1216e40660d"
        or wire.get("applicationSize") != "0x298"
        or wire.get("wireRecordOffset") != "0x10"
        or wire.get("wireRecordStride") != "0x28"
        or wire.get("physicalRecordCapacity") != 16
        or wire.get("countOffset") != "0x290"
        or "not a safe runtime count" not in wire.get("countBoundary", "")
    ):
        errors.append("wire contract or unsafe-count boundary changed")
    route = document.get("route", {})
    if (
        route.get("ownerPointerLoadVa") != "0x004dd167"
        or route.get("storageAdjustVa") != "0x004dd18f"
        or route.get("wireArgumentVa") != "0x004dd1a6"
        or route.get("storageThisVa") != "0x004dd1a7"
        or route.get("applyCallVa") != "0x004dd1a9"
        or route.get("applyVa") != "0x0055cf70"
    ):
        errors.append("dispatcher wire/storage alias route changed")
    owner = document.get("owner", {})
    if (
        owner.get("containerClass") != "Application::Main::RaptureElementContainer"
        or owner.get("containerPointerOffset") != "0x4d8"
        or owner.get("elementClass") != "ClientWorkElement"
        or owner.get("elementAllocationSize") != "0x838"
        or owner.get("storageOffset") != "0x98"
        or owner.get("storageSize") != "0x7a0"
        or owner.get("physicalRecordCount") != 16
        or owner.get("recordStride") != "0x78"
        or owner.get("elementConstructorVa") != "0x0055f8b0"
        or owner.get("elementDestructorVa") != "0x0055d100"
        or owner.get("storageConstructorVa") != "0x0055f830"
        or owner.get("storageDestructorVa") != "0x0055cf20"
    ):
        errors.append("owner, lifetime, or storage layout changed")
    projection = document.get("projection", {})
    projection_map = [
        (row.get("wire"), row.get("storageRecord"))
        for row in projection.get("recordFields", [])
    ]
    header_map = [
        (row.get("wire"), row.get("storage"), row.get("width"), row.get("operation"))
        for row in projection.get("header", [])
    ]
    if (
        projection.get("recordBase") != "+0x18"
        or projection.get("recordStride") != "0x78"
        or projection.get("wireRecordStride") != "0x28"
        or header_map != EXPECTED_HEADER
        or projection_map != EXPECTED_PROJECTION
        or [row.get("storageRecord") for row in projection.get("helperOutputs", [])] != ["+0x20", "+0x74"]
        or "no RTTI class name is proven" not in projection.get("helperBoundary", "")
    ):
        errors.append("wire-to-storage projection or helper boundary changed")
    readers = []
    for row in document.get("readerCensus", []):
        location = row.get("storage", row.get("storageRecord"))
        consumers = row.get("consumers")
        if consumers is None:
            consumer = row.get("consumer")
            consumers = [] if consumer is None else [consumer]
        readers.append(
            (
                location,
                tuple(row.get("writers", [])),
                tuple(row.get("readers", [])),
                tuple(consumers),
                row.get("boundary"),
            )
        )
    if readers != EXPECTED_READERS:
        errors.append("reader census changed")
    first = document.get("firstOutwardOperation", {})
    if (
        first.get("applyVa") != "0x0055cf70"
        or first.get("consumerClass") != "MapScreenControl"
        or first.get("consumerVa") != "0x00671400"
        or first.get("retailStrings") != ["MapScreenControl", "group_marker_data", "MapMarkerParty", "Update"]
        or "synchronously" not in first.get("operation", "")
    ):
        errors.append("first outward UI operation changed")
    deferred = document.get("deferredGate", {})
    if (
        deferred.get("vtableOwner") != "PcSearchWidgetOperator"
        or deferred.get("vtableSlot") != 29
        or deferred.get("methodVa") != "0x00691f30"
        or deferred.get("condition") != "+0x798 is nonzero and +0x14 equals one"
        or "not the first outward consumer" not in deferred.get("boundary", "")
    ):
        errors.append("deferred refresh gate changed")
    verdict = document.get("verdict", {})
    if (
        verdict.get("uiConsumers") != ["MapScreenControl::0x00671400"]
        or verdict.get("luaOrNapiConsumers") != []
        or verdict.get("networkEmissions") != []
        or len(verdict.get("rejectedInterpretations", [])) != 4
        or "computed or dynamic indirect" not in verdict.get("remainingBoundary", "")
    ):
        errors.append("consumer verdict or remaining boundary changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
    )
    rejected_packet_name = "PartyMapMarker" + "UpdatePacket"
    if (
        any(re.search(pattern, text, re.I) for pattern in forbidden)
        or rejected_packet_name.lower() in text.lower()
    ):
        errors.append("manifest contains a private path or rejected packet name")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("wireContract", "neutralName"), "invented-name"),
        (("wireContract", "countBoundary"), "sixteen rows are safe"),
        (("route", "storageThisVa"), "0x004dd1a6"),
        (("owner", "recordStride"), "0x28"),
        (("projection", "helperBoundary"), "NamedLookupContext"),
        (("firstOutwardOperation", "consumerClass"), "Unknown"),
        (("deferredGate", "vtableSlot"), 28),
        (("verdict", "luaOrNapiConsumers"), ["invented"]),
        (("verdict", "networkEmissions"), ["invented"]),
    ]:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    changed_projection = copy.deepcopy(document)
    changed_projection["projection"]["recordFields"][3]["storageRecord"] = "+0x14"
    mutations.append(changed_projection)
    changed_header = copy.deepcopy(document)
    changed_header["projection"]["header"][2]["storage"] = "+0x14"
    mutations.append(changed_header)
    dropped_reader = copy.deepcopy(document)
    dropped_reader["readerCensus"].pop(4)
    mutations.append(dropped_reader)
    changed_writer = copy.deepcopy(document)
    changed_writer["readerCensus"][0]["writers"] = []
    mutations.append(changed_writer)
    changed_boundary = copy.deepcopy(document)
    changed_boundary["readerCensus"][7]["boundary"] = "complete"
    mutations.append(changed_boundary)
    leaked = copy.deepcopy(document)
    leaked["source"]["path"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    rejected_name = copy.deepcopy(document)
    rejected_name["wireContract"]["neutralName"] = "invented-party-marker-packet"
    mutations.append(rejected_name)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} s2c 0x018D client-consumer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: s2c 0x018D client-consumer manifest and 16 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
