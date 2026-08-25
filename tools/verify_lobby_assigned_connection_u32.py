#!/usr/bin/env python3
"""Validate the sanitized lobby assigned-connection-u32 manifest."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "lobby_assigned_connection_u32.json"
EXPECTED_REFERENCES = [
    ("0x00da1efc", "write-zero"),
    ("0x00da1ae0", "read-compare-zero"),
    ("0x00da1b47", "read-compare-zero"),
    ("0x00db34cd", "read-compare-zero"),
    ("0x00db3590", "write-assigned"),
    ("0x00db3598", "read-callback"),
    ("0x00db35ae", "read-callback"),
    ("0x00da146d", "write-callback"),
]
EXPECTED_FALSE_OWNERS = [
    "0x00da0d23",
    "0x00da14e5",
    "0x00da19a5",
    "0x00da2a62",
    "0x00db3525",
    "0x00db3528",
]
EXPECTED_STAGES = ["construct", "assign", "notify", "live", "replace", "destroy"]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-lobby-assigned-connection-u32-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if source.get("binarySha256") != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9":
        errors.append("binary identity changed")
    if source.get("ghidraVersion") != "12.1.3":
        errors.append("Ghidra version changed")
    owner = document.get("owner", {})
    if (
        owner.get("vtableRva") != "0x00d276e8"
        or owner.get("fieldOffset") != 4
        or owner.get("fieldWidth") != 4
        or owner.get("fieldId") != "assigned_connection_u32"
        or not str(owner.get("class", "")).endswith("::ConsumerConnection")
    ):
        errors.append("field ownership changed")
    references = document.get("directReferences", [])
    if [(item.get("va"), item.get("access")) for item in references] != EXPECTED_REFERENCES:
        errors.append("direct instruction references changed")
    if len({item.get("va") for item in references}) != len(EXPECTED_REFERENCES):
        errors.append("direct instruction references are not unique")
    false_owners = document.get("sameDisplacementFalseOwners", [])
    if [item.get("va") for item in false_owners] != EXPECTED_FALSE_OWNERS:
        errors.append("same-displacement ownership exclusions changed")
    helper_filter = document.get("sharedHelperOwnershipFilter", {})
    if (
        helper_filter.get("functionVa") != "0x00db34a0"
        or helper_filter.get("lobbyCallSites") != ["0x00da273c", "0x00da2792"]
        or helper_filter.get("nonLobbyCallSites") != ["0x00dafb9c", "0x00dafbf2", "0x00db39ec", "0x00db3a42"]
    ):
        errors.append("shared-helper ownership filter changed")
    lifecycle = document.get("lifecycle", [])
    if [item.get("stage") for item in lifecycle] != EXPECTED_STAGES:
        errors.append("lifecycle coverage changed")
    capture = document.get("captureBoundary", {})
    if capture.get("retainedSuccessfulLobbySessions") != 2 or capture.get("publishedValues") is not False:
        errors.append("capture privacy boundary changed")
    contract = document.get("contract", {})
    expected_contract = {
        "required": "nonzero per newly assigned live connection",
        "stableWithinLiveConnection": True,
        "stableAcrossReconnect": False,
        "uniquenessRequiredByStaticClient": False,
        "restrictedValueDomain": False,
        "fixedNonzeroFailsTracedClientBranch": False,
        "directOutgoingSerializationObserved": False,
        "staticConfidence": "high",
        "liveFixedValueConfidence": "low",
    }
    if contract != expected_contract:
        errors.append("static or live contract changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = [r"(?:\d{1,3}\.){3}\d{1,3}", r"[A-Za-z]:\\\\"]
    if any(re.search(pattern, text, re.IGNORECASE) for pattern in forbidden):
        errors.append("manifest contains private plaintext or a machine path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    dropped = copy.deepcopy(document)
    dropped["directReferences"].pop()
    mutations.append(dropped)
    promoted = copy.deepcopy(document)
    promoted["directReferences"].append(promoted["sameDisplacementFalseOwners"].pop())
    mutations.append(promoted)
    crossed_owner = copy.deepcopy(document)
    crossed_owner["sharedHelperOwnershipFilter"]["lobbyCallSites"].append("0x00dafb9c")
    mutations.append(crossed_owner)
    moved = copy.deepcopy(document)
    moved["owner"]["fieldOffset"] = 8
    mutations.append(moved)
    unique = copy.deepcopy(document)
    unique["contract"]["uniquenessRequiredByStaticClient"] = True
    mutations.append(unique)
    live = copy.deepcopy(document)
    live["contract"]["liveFixedValueConfidence"] = "high"
    mutations.append(live)
    leaked = copy.deepcopy(document)
    leaked["captureBoundary"]["observedValue"] = "192.0.2.1"
    mutations.append(leaked)
    return [f"mutation {index} was accepted" for index, item in enumerate(mutations, 1) if not verify(item)]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} lobby assigned-u32 error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: lobby assigned-u32 manifest and 7 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
