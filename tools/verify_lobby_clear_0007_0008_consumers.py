#!/usr/bin/env python3
"""Validate the sanitized lobby clear type 7/8 consumer contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "lobby_clear_0007_0008_consumers.json"
PROMOTED_ARTIFACTS = [
    MANIFEST,
    ROOT / "docs" / "net" / "lobby-clear-0007-0008-consumers.md",
]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))

    if document.get("format") != "xivl-lobby-clear-0007-0008-consumers-v1":
        errors.append("format changed")
    sources = document.get("sources", {})
    expected_sources = {
        "binarySha256": "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9",
        "captureCommit": "32a39d2a92f2268d64ab3586b8d791fa93ed19f1",
        "capturePath": "studies/lobby-handshake-triage/derived/lobby-record-census.json",
        "captureCensusSha256": "50f59c4f186be104d5d45d955560eea703c9e409ddc6e6fef4d826767bfb3d85",
        "captureSourceSha256": "28e06b54fe559870031f077f8549b9244caafa7e5177dbca08a7feae6c2b1b62",
        "retainedSessionCount": 2,
        "completeFrameCount": 16,
        "subrecordCount": 20,
    }
    for key, expected in expected_sources.items():
        if sources.get(key) != expected:
            errors.append(f"source {key} changed")
    if sources.get("ghidraVersion") != "12.1.3":
        errors.append("Ghidra version changed")

    parser = document.get("parser", {})
    if (parser.get("functionVa"), parser.get("type7And8TargetVa")) != (
        "0x00da2330",
        "0x00da2491",
    ):
        errors.append("parser route changed")
    if (
        parser.get("exactCopyLength"),
        parser.get("subrecordHeaderLength"),
        parser.get("payloadLength"),
    ) != (24, 16, 8):
        errors.append("parser length contract changed")
    if "not dispatch" not in parser.get("boundary", ""):
        errors.append("copy-versus-dispatch boundary changed")

    dispatcher = document.get("dispatcher", {})
    expected_targets = [
        "0x00da26a9",
        "0x00da2730",
        "0x00da27d5",
        "0x00da27d5",
        "0x00da27d5",
        "0x00da27d5",
        "0x00da27b0",
        "0x00da2678",
        "0x00da26d4",
        "0x00da275a",
    ]
    if (
        dispatcher.get("functionVa") != "0x00da25d0"
        or dispatcher.get("jumpTableVa") != "0x00da27f0"
    ):
        errors.append("dispatcher locator changed")
    if dispatcher.get("targetsByType") != expected_targets:
        errors.append("dispatcher target table changed")
    expected_callers = [
        {"functionVa": "0x00da1ab0", "callVa": "0x00da1b7e"},
        {"functionVa": "0x00dac490", "callVa": "0x00dac6df"},
        {"functionVa": "0x00dac750", "callVa": "0x00dac8a9"},
        {"functionVa": "0x00dac900", "callVa": "0x00daca8b"},
    ]
    if dispatcher.get("directCallers") != expected_callers:
        errors.append("dispatcher direct callers changed")

    state = document.get("connectionState", {})
    if (
        state.get("pendingFieldOffset"),
        state.get("pendingFieldWidth"),
        state.get("initialValue"),
    ) != (56, 4, 0):
        errors.append("pending field contract changed")
    if (
        state.get("constructorVa"),
        state.get("constructorWriteVa"),
        state.get("scalarConstructorVa"),
        state.get("replacementVa"),
    ) != ("0x00da1ea0", "0x00da1eef", "0x00452a40", "0x00da12a0"):
        errors.append("pending field lifecycle route changed")
    if state.get("assignedConnectionFieldOffset") != 4:
        errors.append("assigned-connection boundary changed")

    cases = {row.get("type"): row for row in document.get("receiveCases", [])}
    type7 = cases.get(7, {})
    type8 = cases.get(8, {})
    if (
        type7.get("caseVa"),
        type7.get("exchangeCallVa"),
        type7.get("importSlotVa"),
        type7.get("importName"),
        type7.get("returns"),
    ) != (
        "0x00da27b0",
        "0x00da27b6",
        "0x00f3e148",
        "KERNEL32.InterlockedExchange",
        True,
    ):
        errors.append("type-7 receive route changed")
    if type7.get("stateEffect") != (
        "Atomically writes 1 to connection+0x38 and ignores the old value."
    ):
        errors.append("type-7 state effect changed")
    if (type8.get("caseVa"), type8.get("returns")) != ("0x00da2678", True):
        errors.append("type-8 receive route changed")
    if type8.get("stateEffect") != "Performs no case-specific call or state write.":
        errors.append("type-8 negative state contract changed")
    if any(
        row.get("payloadEffect") != "Reads neither payload dword."
        for row in cases.values()
    ):
        errors.append("receive payload-consumer boundary changed")

    selection = document.get("sendSelection", {})
    expected_selection = {
        "route": ["0x00db3300", "0x00db3280", "0x00db3020"],
        "eligibleGateFieldOffset": 20,
        "eligibleCheckVa": "0x00db302d",
        "consumeExchangeCallVa": "0x00db304b",
        "consumeExchangeValue": 0,
        "nonzeroBuilderType": 8,
        "nonzeroBuilderVa": "0x00db8090",
        "nonzeroBuilderCallVa": "0x00db305c",
        "zeroTimedBuilderType": 7,
        "zeroTimedBuilderVa": "0x00da1d70",
        "zeroTimedBuilderCallVa": "0x00db307a",
        "lastEmissionTimeFieldOffset": 48,
        "lastEmissionTimeWriteVa": "0x00db3081",
        "builderRecordLength": 24,
        "builderPayloadOffsets": [16, 20],
        "builderTimeSource": "Low dword of __time64 at payload+0x14",
        "payloadForwarding": False,
    }
    for key, expected in expected_selection.items():
        if selection.get(key) != expected:
            errors.append(f"send-selection {key} changed")

    ordering = document.get("pollOrdering", {})
    if (
        ordering.get("functionVa"),
        ordering.get("sendPumpCallVa"),
        ordering.get("dispatcherCallVa"),
    ) != (
        "0x00da1ab0",
        "0x00da1b38",
        "0x00da1b7e",
    ):
        errors.append("poll ordering changed")
    census = document.get("captureCensus", {})
    if census.get("serverToClient") != [
        {"session": 1, "firstType": 7, "nextType": 10},
        {"session": 2, "firstType": 7, "nextType": 10},
    ]:
        errors.append("capture server-to-client order changed")
    if census.get("clientToServerType8Count") != 2 or census.get(
        "type8OrderRelativeToEncryptedApplicationRecord"
    ) != ["after", "before"]:
        errors.append("capture type-8 order changed")
    if census.get("preKeyPlaintextClearTypes") != [7, 9]:
        errors.append("capture pre-key clear-type classification changed")
    if census.get("observedZeroExtentClearTypes") != [8]:
        errors.append("capture zero-extent clear-type classification changed")

    ack = document.get("acknowledgementBoundary", {})
    if (ack.get("type"), ack.get("assignedConnectionFieldOffset")) != (10, 4):
        errors.append("type-10 boundary changed")
    if "do not read or write" not in ack.get("relationship", ""):
        errors.append("type-10 separation changed")

    promoted_text = "\n".join(
        path.read_text(encoding="ascii")
        for path in PROMOTED_ARTIFACTS
        if path != MANIFEST
    )
    text = json.dumps(document, sort_keys=True, ensure_ascii=True) + promoted_text
    downstream_name = bytes.fromhex("426168616d7574").decode("ascii")
    forbidden = [
        r"(?:\d{1,3}\.){3}\d{1,3}",
        r"[A-Za-z]:\\",
        r"docs[/\\]ai_agents[/\\]local",
        re.escape(downstream_name),
    ]
    if any(re.search(pattern, text, re.IGNORECASE) for pattern in forbidden):
        errors.append("manifest contains private or downstream-specific text")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("dispatcher", "targetsByType", 6), "0x00da2678"),
        (("dispatcher", "directCallers", 0, "functionVa"), "0x00da1960"),
        (("connectionState", "pendingFieldOffset"), 60),
        (
            ("receiveCases", 0, "stateEffect"),
            "Atomically writes 2 to connection+0x38 and ignores the old value.",
        ),
        (("receiveCases", 1, "stateEffect"), "Writes state."),
        (("parser", "boundary"), "Length 0x18 gates dispatch."),
        (("pollOrdering", "functionVa"), "0x00da1960"),
        (
            ("captureCensus", "type8OrderRelativeToEncryptedApplicationRecord"),
            ["after", "after"],
        ),
        (("captureCensus", "preKeyPlaintextClearTypes"), [7, 8, 9]),
        (("acknowledgementBoundary", "assignedConnectionFieldOffset"), 56),
        (("remainingBoundary",), "C:\\private"),
    ]:
        item = copy.deepcopy(document)
        cursor = item
        for key in path[:-1]:
            cursor = cursor[key]
        cursor[path[-1]] = value
        mutations.append(item)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} lobby clear consumer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: lobby clear type 7/8 manifest and 11 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
