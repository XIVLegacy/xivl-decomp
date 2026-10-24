#!/usr/bin/env python3
"""Validate the Map 0x0003 producer contract and optional pinned PE body ranges."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import struct
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "map_0003_emitter.json"
EXPECTED = {
    "format": "xivl-map-0003-emitter-v2",
    "source": {
        "binary": "ffxivgame.exe",
        "bytes": 15996808,
        "sha256": "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9",
        "imageBase": "0x00400000",
        "addressKind": "VA",
        "instrument": "GhidraCLI disassembly with explicit project and program selection",
        "toolVersion": None,
    },
    "route": {
        "lane": "Map",
        "direction": "serverbound",
        "opcode": "0x0003",
        "recordValue": 3,
        "emitterVa": "0x004e0320",
        "callerVa": "0x004d8160",
        "callSiteVa": "0x004d8279",
        "bridgeVa": "0x00dae010",
    },
    "geometry": {
        "internalGameMessageBytes": 560,
        "gameHeaderBytes": 16,
        "applicationBytes": 544,
        "recordApplicationOffset": 24,
        "dataApplicationOffset": 16,
        "normalizedViewBytes": 552,
        "normalizedViewApplicationOffset": 8,
        "normalizedSelectorOffset": 32,
        "normalizedTextOffset": 36,
        "normalizedTailOffset": 548,
    },
    "stack": {
        "recordOffsetFromS": 132,
        "recordStoreEspDelta": -12,
        "valueStoreDisplacement": 144,
        "lengthStoreDisplacement": 148,
        "applicationOffsetFromS": 156,
        "textStageOffsetFromS": 2340,
        "textStageBytes": 512,
        "textCopyOffsetFromS": 184,
        "textCopyEndFromS": 696,
        "tailOffsetFromS": 696,
        "tailEndFromS": 700,
    },
    "fields": [
        {
            "offset": 0,
            "bytes": 4,
            "source": "zero-u32",
            "sourceVas": [],
            "writeVas": [
                "0x004e0409",
            ],
            "value": 0,
        },
        {
            "offset": 4,
            "bytes": 4,
            "source": "emitter-argument-1",
            "sourceVas": [
                "0x004e0373",
            ],
            "writeVas": [
                "0x004e0410",
            ],
            "value": None,
        },
        {
            "offset": 8,
            "bytes": 4,
            "source": "emitter-argument-2-pointee+0",
            "sourceVas": [
                "0x004e037d",
            ],
            "writeVas": [
                "0x004e0410",
            ],
            "value": None,
        },
        {
            "offset": 12,
            "bytes": 4,
            "source": "emitter-argument-2-pointee+4",
            "sourceVas": [
                "0x004e0387",
            ],
            "writeVas": [
                "0x004e0445",
            ],
            "value": None,
        },
        {
            "offset": 16,
            "bytes": 4,
            "source": "emitter-argument-2-pointee+8",
            "sourceVas": [
                "0x004e0392",
            ],
            "writeVas": [
                "0x004e0445",
            ],
            "value": None,
        },
        {
            "offset": 20,
            "bytes": 4,
            "source": "emitter-argument-3-float-bits",
            "sourceVas": [
                "0x004e03a6",
            ],
            "writeVas": [
                "0x004e044e",
            ],
            "value": None,
        },
        {
            "offset": 24,
            "bytes": 2,
            "source": "low16-emitter-argument-5",
            "sourceVas": [
                "0x004e03fb",
            ],
            "writeVas": [
                "0x004e0427",
            ],
            "value": None,
        },
        {
            "offset": 26,
            "bytes": 2,
            "source": "zero-u16",
            "sourceVas": [],
            "writeVas": [
                "0x004e041f",
            ],
            "value": 0,
        },
        {
            "offset": 28,
            "bytes": 512,
            "source": "staged-text-bytes",
            "sourceVas": [
                "0x004e045a",
            ],
            "writeVas": [
                "0x004e0455",
                "0x004e0468",
            ],
            "value": None,
        },
        {
            "offset": 540,
            "bytes": 4,
            "source": "unknown-source-not-initialized-by-producer",
            "sourceVas": [],
            "writeVas": [],
            "value": None,
        },
    ],
    "selector": {
        "applicationOffset": 24,
        "bytes": 2,
        "emitterArgument": 5,
        "callerArgument": 4,
        "callerFrameOffset": 20,
        "source": "low16(W)",
        "fixedValue": None,
        "floatHelperVa": "0x004dd8b0",
        "floatHelperLiteral": 1,
        "floatHelperCleanupBytes": 4,
        "floatHelperRetVas": [
            "0x004dd921",
            "0x004dd92b",
        ],
    },
    "caller": {
        "contextSource": "[B+0x98] ? [[B+0x98]+0x88] : 0",
        "contextMeaning": None,
        "threeDwordOffsets": [
            156,
            160,
            164,
        ],
        "dwordMeaning": None,
        "floatMeaning": None,
        "textSource": "resulting-local-string-record",
        "textProcessingHelperVa": "0x00c99e40",
        "optionalTextHelperVa": "0x004ce760",
        "optionalTextHelperResultGate": False,
        "textPolicy": None,
        "universalInFieldTerminator": False,
    },
    "localResult": {
        "callerGates": [
            "T+0x174ec nonnull",
            "cast helper result nonnull",
            "0x00c99e40 AL nonzero",
        ],
        "emitterGates": [
            "emitter-this+0x234 nonnull",
            "that object+0x70 nonnull",
        ],
        "bridgeStateForRecord3": 3,
        "bridgeGates": [
            "0x004e4c10 result nonnull",
            "0x00daf850 result nonzero",
        ],
        "finalizerVa": "0x00db06a0",
        "bridgeSuccessVa": "0x00dae0df",
        "finalizerResultTested": False,
        "propagation": "bridge AL through emitter and immediate caller after local string destruction",
        "scope": "local normal-path construction and finalization result",
        "deliveryProven": False,
        "serverAcceptanceProven": False,
    },
    "unknowns": [
        "tail byte values",
        "text transformation and optional-helper effects",
        "context population and meaning",
        "runtime occurrence of the conditional selector-one route",
        "indirect-caller completeness",
        "exception and fault behavior",
        "submission queue lifetime",
        "transport and delivery",
        "server acceptance and tolerance",
        "upstream control-byte population, meaning and lifetime",
        "opaque upstream argument and receiver population",
    ],
    "references": {
        "zoneHeader": "docs/net/zone-outbound-header.md",
        "capture": "xivl-captures:studies/m9-corpus-research/derived/verdicts.md#Map main client-to-server 0x0003",
    },
    "bodyRanges": [
        {
            "name": "emitter",
            "startVa": "0x004e0320",
            "endVa": "0x004e04d6",
            "sha256": "4b56b6dcc89cc550bdcab2be9d9f938151219dac2f39474d03429b693b8e9f2c",
        },
        {
            "name": "caller",
            "startVa": "0x004d8160",
            "endVa": "0x004d82d1",
            "sha256": "252d230ead1f020eee7741f8838c5107558c7def46afe26698b8d46ee0176a3a",
        },
        {
            "name": "float-helper",
            "startVa": "0x004dd8b0",
            "endVa": "0x004dd92e",
            "sha256": "61d5b323e73da453c3d1721823776c87b31ef8237e19e605eb0ab8527000ca56",
        },
        {
            "name": "bridge",
            "startVa": "0x00dae010",
            "endVa": "0x00dae1d3",
            "sha256": "d3f7ce4b5b1f3a382c07066a46006b95a5312b1cb06da7b61085e3bba49797a3",
        },
        {
            "name": "upstream-W1",
            "startVa": "0x0075bef0",
            "endVa": "0x0075bfc5",
            "sha256": "210f8d6e7023f20093f96ee332f8966f499441fe68f4ceee7b37e64cad791f69",
        },
        {
            "name": "upstream-dispatch",
            "startVa": "0x006e91f0",
            "endVa": "0x006e9427",
            "sha256": "c25e9ef201fab33ef27ba7a13defbad5d112c2ffddd34b0e32bd617d0e44d184",
        },
        {
            "name": "control-byte",
            "startVa": "0x00763030",
            "endVa": "0x00763058",
            "sha256": "e4a7ef572763cf5a91852bdc48ee84543025a6b73a3936469ea21063e8fff29a",
        },
        {
            "name": "byte-equality",
            "startVa": "0x00445d60",
            "endVa": "0x00445da1",
            "sha256": "a5baafd37b78fa9a166105f1a64516b7c59056c878a300413942c3c932a627c8",
        },
    ],
    "upstream": {
        "staticConditionalCallProven": True,
        "runtimeInvocationObserved": False,
        "wrapperVa": "0x0075bef0",
        "callSiteVa": "0x0075bfa0",
        "calleeVa": "0x004d8160",
        "wLiteral": 1,
        "wPushVa": "0x0075bf9b",
        "gate": {
            "wrapperArgument": 4,
            "stackOffset": 132,
            "testVa": "0x0075bf21",
            "branchVa": "0x0075bf3a",
            "zeroTargetVa": "0x0075bf8f",
            "predicate": "low-byte-equal-zero",
            "bytes": 1,
            "nonzeroBranchEmissionPolicy": None,
            "modeMeaning": None,
        },
        "dispatcher": {
            "functionVa": "0x006e91f0",
            "callSiteVa": "0x006e92ed",
            "comparisonCallVa": "0x006e92c4",
            "equalityBranchVa": "0x006e92cb",
            "comparisonHelperVa": "0x00445d60",
            "literalVa": "0x00fd4bf0",
            "literal": "say",
            "terminatorRequired": True,
            "comparison": "exact-byte-equality",
        },
        "control": {
            "helperVa": "0x00763030",
            "establishedRegister": "AL",
            "pushedRegister": "EAX",
            "consumedBytes": 1,
            "upperEaxBitsEstablished": False,
            "ordinaryZeroCases": [
                "dword[D+0x20]==0",
                "Z is null",
            ],
            "ordinaryChain": [
                "H=dword[D+0x1c]",
                "N=dword[H]",
                "Z=dword[N+8]",
            ],
            "ordinaryOtherSource": "byte[Z+0xba]",
            "consistencyHelperVa": "0x009d22b4",
            "consistencyHelperBehavior": None,
            "meaning": None,
            "population": None,
            "lifetime": None,
        },
        "arguments": {
            "qHelperVa": "0x0071ce70",
            "q0Index": 0,
            "q1Index": 1,
            "qMeaning": None,
            "comparisonSource": "Q1",
            "lSource": "[C+0x60]+4",
            "lConstructorVa": "0x00447200",
            "kSource": "dword[P+0x0c]",
            "dispatcherArguments": [
                "&K",
                "&L",
                "Q0",
                "EAX",
            ],
            "wrapperReceiver": "U=[D]",
            "calleeReceiver": "T=[U]",
            "calleeArguments": [
                "R",
                "&L",
                "Q0",
                1,
            ],
            "rSource": "opaque 0x004d9970(T,K) return",
            "rMeaning": None,
            "vSource": "opaque 0x00cc7510([P+4]) return",
            "dSource": "[[V]+4]",
            "sourceMeaning": None,
            "originalTextUnchangedProven": False,
            "textPolicy": None,
            "savedKStackOffset": 24,
            "keyAddressLeaVa": "0x006e92e6",
            "keyAddressEspOffset": 36,
            "pendingPushBytes": 12,
        },
        "downstreamGatesProvenSatisfied": False,
        "outerResultContract": "no explicit test of the selected call result before cleanup and epilogue",
        "sourcePopulation": None,
    },
}


def range_errors(rows: object) -> list[str]:
    """Check declarative range shape without reading or decoding instructions."""
    if not isinstance(rows, list) or not rows:
        return ["bodyRanges must be a nonempty list"]
    errors: list[str] = []
    names: set[str] = set()
    for row in rows:
        if not isinstance(row, dict) or set(row) != {
            "name",
            "startVa",
            "endVa",
            "sha256",
        }:
            errors.append("body range fields changed")
            continue
        name = row["name"]
        if not isinstance(name, str) or not name or name in names:
            errors.append("body range names must be nonempty and unique")
        elif isinstance(name, str):
            names.add(name)
        if not all(
            isinstance(row[key], str) and re.fullmatch(r"0x[0-9a-f]{8}", row[key])
            for key in ("startVa", "endVa")
        ):
            errors.append("body range VA is malformed")
        elif int(row["endVa"], 16) <= int(row["startVa"], 16):
            errors.append("body range extent is empty or reversed")
        if not isinstance(row["sha256"], str) or not re.fullmatch(
            r"[0-9a-f]{64}", row["sha256"]
        ):
            errors.append("body range digest is malformed")
    return errors


def field_errors(rows: object) -> list[str]:
    if not isinstance(rows, list):
        return ["application fields must be a list"]
    errors: list[str] = []
    cursor = 0
    written: set[int] = set()
    for row in rows:
        if not isinstance(row, dict):
            errors.append("application field is not an object")
            continue
        offset, width = row.get("offset"), row.get("bytes")
        if (
            type(offset) is not int
            or type(width) is not int
            or offset < 0
            or width <= 0
            or offset + width > 544
        ):
            errors.append("application field extent is invalid")
            continue
        if offset != cursor:
            errors.append("application fields do not form an ordered partition")
        cursor = offset + width
        if row.get("source") != "unknown-source-not-initialized-by-producer":
            written.update(range(offset, cursor))
    if cursor != 544:
        errors.append("application fields do not end at 544")
    if written != set(range(540)):
        errors.append("written application union is not [0,540)")
    return errors


def verify(document: dict | None = None) -> list[str]:
    if document is None:
        try:
            document = json.loads(MANIFEST.read_text(encoding="ascii"))
        except (OSError, ValueError) as exc:
            return [f"cannot read manifest: {exc}"]
    if not isinstance(document, dict):
        return ["catalog must be an object"]
    errors: list[str] = []
    if set(document) != set(EXPECTED):
        errors.append("catalog sections changed")
    for section, expected in EXPECTED.items():
        if json.dumps(document.get(section), sort_keys=True) != json.dumps(
            expected, sort_keys=True
        ):
            errors.append(f"{section} contract changed")
    errors += field_errors(document.get("fields"))
    errors += range_errors(document.get("bodyRanges"))
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors: list[str] = []
    mutations = [
        ("upstream", "staticConditionalCallProven", False, "upstream contract changed"),
        ("upstream", "wLiteral", 2, "upstream contract changed"),
        ("upstream", "callSiteVa", "0x0075bfa1", "upstream contract changed"),
        (
            "upstream",
            "downstreamGatesProvenSatisfied",
            True,
            "upstream contract changed",
        ),
        ("geometry", "applicationBytes", 552, "geometry contract changed"),
        ("geometry", "recordApplicationOffset", 16, "geometry contract changed"),
        ("geometry", "internalGameMessageBytes", 552, "geometry contract changed"),
        ("selector", "bytes", 4, "selector contract changed"),
        ("selector", "fixedValue", 1, "selector contract changed"),
        ("selector", "callerArgument", 3, "selector contract changed"),
        ("selector", "source", "float-helper-literal", "selector contract changed"),
        ("selector", "floatHelperCleanupBytes", 0, "selector contract changed"),
        (
            "upstream",
            "runtimeInvocationObserved",
            True,
            "upstream contract changed",
        ),
        ("caller", "contextMeaning", "actor identity", "caller contract changed"),
        ("caller", "textPolicy", "unchanged input", "caller contract changed"),
        ("localResult", "deliveryProven", True, "localResult contract changed"),
        ("localResult", "serverAcceptanceProven", True, "localResult contract changed"),
    ]
    for section, key, value, expected_error in mutations:
        changed = copy.deepcopy(document)
        changed[section][key] = value
        if expected_error not in verify(changed):
            errors.append(f"mutation {section}.{key} missed its intended assertion")
    for label, change in (
        ("tail value", lambda d: d["fields"][-1].update(value=0)),
        ("dropped field", lambda d: d["fields"].pop(2)),
        ("overlapping field", lambda d: d["fields"][1].update(offset=3)),
        ("source field", lambda d: d["fields"][4].update(source="invented meaning")),
    ):
        changed = copy.deepcopy(document)
        change(changed)
        if "fields contract changed" not in verify(changed):
            errors.append(f"mutation {label} missed its intended assertion")
    old_format = copy.deepcopy(document)
    old_format["format"] = "xivl-map-0003-emitter-v1"
    if "format contract changed" not in verify(old_format):
        errors.append("legacy format was accepted")
    for value in (False, True):
        legacy = copy.deepcopy(document)
        legacy["selector"]["upstreamValueOneInvocationProven"] = value
        if "selector contract changed" not in verify(legacy):
            errors.append("ambiguous legacy selector field was accepted")
    for path, value in [
        (("gate", "predicate"), "nonzero"),
        (("gate", "bytes"), 4),
        (("gate", "zeroTargetVa"), "0x0075bf3c"),
        (("gate", "nonzeroBranchEmissionPolicy"), "suppresses all emission"),
        (("control", "establishedRegister"), "EAX"),
        (("control", "upperEaxBitsEstablished"), True),
        (("control", "meaning"), "invented mode"),
        (("dispatcher", "literal"), "Say"),
        (("dispatcher", "terminatorRequired"), False),
        (("arguments", "comparisonSource"), "Q0"),
        (("arguments", "dispatcherArguments"), ["&L", "&K", "Q1", "EAX"]),
        (("arguments", "rMeaning"), "invented identity"),
        (("arguments", "originalTextUnchangedProven"), True),
    ]:
        changed = copy.deepcopy(document)
        changed["upstream"][path[0]][path[1]] = value
        if "upstream contract changed" not in verify(changed):
            errors.append(f"upstream mutation {'.'.join(path)} missed its assertion")
    lost_unknown = copy.deepcopy(document)
    lost_unknown["unknowns"].remove(
        "runtime occurrence of the conditional selector-one route"
    )
    if "unknowns contract changed" not in verify(lost_unknown):
        errors.append("runtime occurrence uncertainty was dropped")
    for name in ("upstream-W1", "upstream-dispatch", "control-byte", "byte-equality"):
        index = next(
            i for i, row in enumerate(document["bodyRanges"]) if row["name"] == name
        )
        for operation in ("drop", "duplicate", "boundary", "digest"):
            changed = copy.deepcopy(document)
            rows = changed["bodyRanges"]
            if operation == "drop":
                rows.pop(index)
            elif operation == "duplicate":
                rows.append(copy.deepcopy(rows[index]))
            elif operation == "boundary":
                rows[index]["endVa"] = f"0x{int(rows[index]['endVa'], 16) + 1:08x}"
            else:
                rows[index]["sha256"] = "0" * 64
            if "bodyRanges contract changed" not in verify(changed):
                errors.append(f"{name} {operation} missed its range assertion")
    return errors


def file_sections(data: bytes) -> list[tuple[int, int, int, int]]:
    """Reuse section enumeration but require an actual x86 PE32 file envelope."""
    if len(data) < 64 or data[:2] != b"MZ":
        raise ValueError("missing MZ signature")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if pe + 24 > len(data) or data[pe : pe + 4] != b"PE\0\0":
        raise ValueError("missing or truncated PE signature")
    machine, count = struct.unpack_from("<HH", data, pe + 4)
    if machine != 0x14C:
        raise ValueError("machine is not x86")
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if optional_size < 32 or optional + optional_size > len(data):
        raise ValueError("optional header is truncated")
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("optional header is not PE32")
    if struct.unpack_from("<I", data, optional + 28)[0] != 0x00400000:
        raise ValueError("image base changed")
    if not count or optional + optional_size + 40 * count > len(data):
        raise ValueError("section table is empty or truncated")
    sections = image_checks.section_map(data)
    if any(raw + size > len(data) for _, _, raw, size in sections):
        raise ValueError("section raw range is outside the file")
    return sections


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    """Check body hashes. Identity may be disabled only by in-process fixture tests."""
    errors: list[str] = []
    if require_identity:
        if len(data) != 15996808:
            errors.append("executable size changed")
        if hashlib.sha256(data).hexdigest() != EXPECTED["source"]["sha256"]:
            errors.append("executable SHA256 changed")
        if errors:
            return errors
    rows = document.get("bodyRanges") if isinstance(document, dict) else None
    errors += range_errors(rows)
    if errors:
        return errors
    try:
        sections = file_sections(data)
    except (ValueError, struct.error) as exc:
        return [f"image is not a readable PE32: {exc}"]
    for row in rows:
        start, end = int(row["startVa"], 16), int(row["endVa"], 16)
        matches = [
            (raw + start - va, end - start)
            for va, _, raw, raw_size in sections
            if va <= start and end <= va + raw_size
        ]
        if len(matches) != 1:
            errors.append(f"range {row['name']} is not uniquely file-backed")
            continue
        offset, length = matches[0]
        found = hashlib.sha256(data[offset : offset + length]).hexdigest()
        if found != row["sha256"]:
            errors.append(f"range {row['name']} digest changed")
    return errors


def verify_executable(path: Path) -> list[str]:
    try:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
        return verify_image(path.read_bytes(), document)
    except (OSError, ValueError) as exc:
        return [f"cannot read executable or manifest: {exc}"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--exe", type=Path, help="exact-build executable for static checks"
    )
    args = parser.parse_args()
    errors = verify()
    if not errors:
        errors += mutation_test()
    if not errors and args.exe is not None:
        errors += verify_executable(args.exe)
    if errors:
        print(f"FAIL: {len(errors)} Map 0x0003 emitter error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus exact-build file-backed range checks" if args.exe else ""
    print(f"PASS: Map 0x0003 emitter manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
