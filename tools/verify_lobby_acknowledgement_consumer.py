#!/usr/bin/env python3
"""Validate the sanitized lobby acknowledgement consumer manifest."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "lobby_acknowledgement_consumer.json"

EXPECTED_GROUPS = [
    [0, 336],
    [8, 312],
    [128, 344, 464, 488, 512, 568],
    [240, 304],
    [264, 328],
    [368, 448, 496],
    [416, 432],
    [520, 552, 600, 632],
]
EXPECTED_DYNAMIC_RUNS = [
    (0, 2),
    (8, 3),
    (20, 1),
    (23, 1),
    (115, 2),
    (120, 3),
    (128, 3),
    (184, 2),
    (216, 1),
    (276, 2),
    (280, 1),
    (312, 3),
    (324, 1),
    (327, 1),
    (336, 2),
    (344, 3),
    (362, 2),
    (403, 1),
    (464, 3),
    (488, 3),
    (512, 3),
    (548, 2),
    (560, 2),
    (568, 3),
    (592, 2),
    (596, 2),
    (624, 1),
    (628, 2),
]


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))

    if document.get("format") != "xivl-lobby-acknowledgement-consumer-v1":
        errors.append("format changed")

    sources = document.get("sources", {})
    if sources.get("binarySha256") != "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9":
        errors.append("binary identity changed")
    if sources.get("captureCommit") != "32a39d2a92f2268d64ab3586b8d791fa93ed19f1":
        errors.append("capture evidence revision changed")
    if sources.get("capturePath") != (
        "studies/lobby-handshake-triage/derived/lobby-record-census.json"
    ):
        errors.append("capture evidence path changed")
    if sources.get("captureLocator") != "crossSession.acknowledgementComparison":
        errors.append("capture evidence locator changed")
    if sources.get("retainedSessionCount") != 2 or sources.get("retainedNewCharacterSessionCount") != 0:
        errors.append("capture-session boundary changed")

    wire = document.get("wire", {})
    expected_wire = {
        "outerLength": 672,
        "outerHeaderLength": 16,
        "subrecordOffset": 16,
        "subrecordLength": 656,
        "subrecordHeaderLength": 16,
        "subrecordType": 10,
        "encryptedPayloadOffset": 32,
        "encryptedPayloadLength": 640,
        "blockSize": 8,
    }
    if wire != expected_wire:
        errors.append("wire boundary changed")

    fields = document.get("payloadFields", [])
    cursor = 0
    dynamic_bytes: set[int] = set()
    dynamic_runs: list[tuple[int, int]] = []
    for field in fields:
        offset = field.get("offset")
        width = field.get("width")
        if offset != cursor or not isinstance(width, int) or width <= 0:
            errors.append("payload fields are not a contiguous positive-width partition")
            break
        cursor += width
        if field.get("crossSession") != "mixed":
            errors.append(f"field {field.get('id')} lost mixed-session classification")
        for run in field.get("dynamicRuns", []):
            start = run.get("offset")
            length = run.get("length")
            if (
                not isinstance(start, int)
                or not isinstance(length, int)
                or length <= 0
                or start < offset
                or start + length > offset + width
            ):
                errors.append(f"field {field.get('id')} has an invalid dynamic run")
                continue
            for byte_offset in range(start, start + length):
                if byte_offset in dynamic_bytes:
                    errors.append("dynamic runs overlap")
                dynamic_bytes.add(byte_offset)
            dynamic_runs.append((start, length))
    if cursor != 640:
        errors.append("payload fields do not cover all 640 bytes")
    if len(dynamic_bytes) != 57:
        errors.append("cross-session dynamic-byte count changed")
    if dynamic_runs != EXPECTED_DYNAMIC_RUNS:
        errors.append("cross-session dynamic runs changed")

    direct = [field for field in fields if field.get("consumerStatus") == "direct"]
    if len(direct) != 1 or direct[0].get("id") != "assigned_connection_u32" or direct[0].get("offset") != 0 or direct[0].get("width") != 4:
        errors.append("direct consumer field changed")
    if set(range(0, 2)) - dynamic_bytes or set(range(2, 4)) & dynamic_bytes:
        errors.append("assigned connection u32 variance changed")

    groups = document.get("repeatedValueGroups", [])
    if [group.get("payloadOffsets") for group in groups] != EXPECTED_GROUPS:
        errors.append("repeated-value groups changed")
    for group in groups:
        offsets = group.get("payloadOffsets", [])
        if group.get("width") != 8 or any(offset % 8 or offset + 8 > 640 for offset in offsets):
            errors.append("repeated-value group alignment changed")
        if not str(group.get("producer", "")).startswith("remote-server-only"):
            errors.append("unsupported repeated-value producer was assigned")

    boundary = document.get("staticAcceptanceBoundary", {})
    if "nonzero little-endian u32 at payload+0x00" not in " ".join(boundary.get("required", [])):
        errors.append("nonzero assignment gate missing")
    if boundary.get("confidence") != "high-static-low-fixed-value-live":
        errors.append("static/live confidence boundary changed")

    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = [
        r"(?:\d{1,3}\.){3}\d{1,3}",
        r"[A-Za-z]:\\\\",
    ]
    if any(re.search(pattern, text, re.IGNORECASE) for pattern in forbidden):
        errors.append("manifest contains a private value or consumer-project name")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations = []

    shifted = copy.deepcopy(document)
    shifted["payloadFields"][1]["offset"] += 1
    mutations.append(shifted)

    widened = copy.deepcopy(document)
    widened["payloadFields"][-1]["width"] += 1
    mutations.append(widened)

    promoted = copy.deepcopy(document)
    promoted["payloadFields"][1]["consumerStatus"] = "direct"
    mutations.append(promoted)

    renamed = copy.deepcopy(document)
    renamed["repeatedValueGroups"][0]["producer"] = "session pointer"
    mutations.append(renamed)

    moved_run = copy.deepcopy(document)
    moved_run["payloadFields"][-1]["dynamicRuns"][0]["offset"] += 1
    mutations.append(moved_run)

    return [f"mutation {index} was accepted" for index, item in enumerate(mutations, 1) if not verify(item)]


def main() -> int:
    errors = verify() + mutation_test()
    if errors:
        print(f"FAIL: {len(errors)} lobby acknowledgement error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: lobby acknowledgement consumer manifest and 5 mutations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
