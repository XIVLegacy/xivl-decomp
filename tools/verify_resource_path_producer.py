#!/usr/bin/env python3
"""Validate the retail resource path producer and exact-build signature."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "resource_path_producer.json"
EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
EXPECTED_SIZE = 15_996_808
EXPECTED_PATTERN_OFFSET = 0x00896972
PATTERN = bytes.fromhex("6A 00 68 00 00 00 00 83 C6 04 56 8B CF E8 00 00 00 00")
MASK = bytes.fromhex("FF FF FF 00 00 00 00 FF FF FF FF FF FF FF 00 00 00 00")
CLOSE_FUNCTION_OFFSET = 0x00053000
CLOSE_FUNCTION = bytes.fromhex(
    "56 8B F1 8B 46 04 85 C0 74 19 50 E8 CF 41 58 00 8B 46 04 50 "
    "E8 2D F6 57 00 83 C4 08 C7 46 04 00 00 00 00 5E C3"
)
DIRECT_CLOSE_CALLS = {
    0x008962D5: 0x00453000,
    0x008962F5: 0x00453000,
    0x00896373: 0x00453000,
    0x00896441: 0x00453000,
    0x0089646A: 0x00453190,
}


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    source = document.get("source", {})
    if document.get("format") != "xivl-resource-path-producer-v1" or source != {
        "binary": "ffxivgame.exe retail 1.23b",
        "build": "2012.09.19.0001",
        "size": EXPECTED_SIZE,
        "binarySha256": EXPECTED_SHA256,
        "imageBase": "0x00400000",
        "ghidraVersion": "12.1.3",
        "runId": "c548-resource-path-producer-20260830",
        "priorOpenBoundary": "https://github.com/XIVLegacy/xivl-client-structs/blob/c987ecc4256da271ee61d6aa257abb9da9a31e65/manifests/resource_dat_open.json",
    }:
        errors.append("evidence identity changed")
    producer = document.get("numericProducer", {})
    if (
        producer.get("owner") != "Component::Resource::ResourceModule vtable slot 1"
        or producer.get("functionVa") != "0x00c99130"
        or producer.get("input") != "caller-supplied u32 resource id"
        or producer.get("formatterVa") != "0x0044b3a0"
        or producer.get("resourceAllocationSize") != 244
        or producer.get("resourceConstructorVa") != "0x00caedd0"
        or producer.get("resourceIdOffset") != 88
        or producer.get("resourcePathOffset") != 4
        or producer.get("initialStateOffset") != 176
        or producer.get("initialState") != 1
        or producer.get("fileThreadOffset") != 36
        or producer.get("priorityQueueOffset") != 56
        or producer.get("normalQueueOffset") != 84
        or producer.get("queueProducerVa") != "0x008edda0"
        or producer.get("queueCommitVa") != "0x008edbf0"
        or producer.get("resourceIndexVa") != "0x00c97ba0"
        or producer.get("treeInsertVa") != "0x00913c10"
        or producer.get("consumerVa") != "0x00c96850"
        or producer.get("openVa") != "0x00453c00"
        or producer.get("pathArgumentAtOpen") != "Resource+0x04"
        or producer.get("alternateConstructorCallers")
        != ["0x00c98e40", "0x00c98f80", "0x00c992d0", "0x00c99480", "0x00c99670"]
        or producer.get("boundary")
        != "Among direct callers of the Resource constructor, only 0x00c99130 also calls the numeric formatter. Four additional direct formatter callers are outside this Resource-constructor set. The other direct Resource-constructor callers accept or build paths by other routes."
    ):
        errors.append("numeric producer chain changed")
    formatter = document.get("formatter", {})
    if (
        formatter.get("functionVa") != "0x0044b3a0"
        or formatter.get("numericModeFlagVa") != "0x01266b64"
        or formatter.get("numericModeCondition") != "nonzero"
        or formatter.get("formatStringVa") != "0x00f672bc"
        or formatter.get("formatString") != "%cdata%c%02X%c%02X%c%02X%c%02X.DAT"
        or formatter.get("separator") != "0x5c"
        or formatter.get("byteOrder") != "resource-id bytes from most significant to least significant"
        or formatter.get("hexCase") != "uppercase"
        or formatter.get("relativeExampleInput") != "0x2a080017"
        or formatter.get("relativeExampleOutput") != "\\data\\2A\\08\\00\\17.DAT"
        or formatter.get("rootPathWrapperVa") != "0x0132cb98"
        or formatter.get("rootSetterVa") != "0x004b2d30"
        or formatter.get("joinVa") != "0x0044ab90"
        or formatter.get("resultCopyVa") != "0x00447450"
        or formatter.get("alternateMode")
        != "When the flag is zero, the function uses a high-16 group and low-16 index table lookup instead of the numeric format string."
        or formatter.get("gateWriters") != {
            "ownerVa": "0x004b2df0",
            "constantOneWriteVa": "0x004b2eca",
            "conditionalByteWriteVa": "0x004b3191",
        }
    ):
        errors.append("formatter contract changed")
    wrapper = document.get("pathWrapper", {})
    if (
        wrapper.get("size") != 84
        or wrapper.get("pointerOffset") != 0
        or wrapper.get("capacityOffset") != 4
        or wrapper.get("usedBytesOffset") != 8
        or wrapper.get("inlineBufferOffset") != 18
        or wrapper.get("inlineCapacity") != 64
        or wrapper.get("usedBytesIncludesNul") is not True
        or wrapper.get("heapFlagOffset") != 17
        or wrapper.get("heapFlagValue") != 0
        or wrapper.get("initializerVa") != "0x00445cf0"
        or wrapper.get("growVa") != "0x00447010"
        or wrapper.get("deepCopyVa") != "0x00447200"
        or wrapper.get("formatCopyVa") != "0x00447620"
        or wrapper.get("concatVa") != "0x00447c80"
        or wrapper.get("destructorVa") != "0x00446f50"
        or wrapper.get("resourceDestructorBodyVa") != "0x00cae7f0"
        or wrapper.get("resourceDeletingDestructorVa") != "0x00caf120"
        or wrapper.get("ownership")
        != "The Resource constructor deep-copies the producer-local path into Resource+0x04. The producer destroys its local wrapper after enqueue and indexing. Resource destruction releases its independent path wrapper."
        or wrapper.get("openUse")
        != "The FileThread consumer passes Resource+0x04 to the LocalFile open member. The open member converts it to temporary wide storage and does not destroy or mutate Resource+0x04."
    ):
        errors.append("path-wrapper ownership changed")
    lifetime = document.get("readServiceLifetime", {})
    if lifetime != {
        "functionVa": "0x00c961f0",
        "localFileOwner": "FileThread owns the LocalFile array at +0x8c; the service loop pairs one 0x2010-byte record with each queued Resource slot.",
        "streamOffset": 4,
        "closeVa": "0x00453000",
        "closeBehavior": "When LocalFile+0x04 is non-null, flush the FILE*, close it, and clear LocalFile+0x04.",
        "directCloseCallVas": {
            "allocationFailure": "0x00c962d5",
            "afterReadAttempt": "0x00c962f5",
            "rejectedBeforeRead": "0x00c96373",
        },
        "successfulReadPath": "After the read attempt returns success, 0x00c962f5 closes and clears the matching LocalFile stream before state +0xb0 is published as 2.",
        "writeServiceVa": "0x00c96380",
        "writeStreamCloseCallVa": "0x00c96441",
        "writeTemporaryTeardownCallVa": "0x00c9646a",
        "teardownVa": "0x00453190",
        "evidence": {
            "tool": "llvm-objdump 22.1.4",
            "method": "Exact-address disassembly of the pinned executable at 0x00453000, 0x00c961f0, and 0x00c96380; the executable verifier checks the close body and every listed direct call target.",
            "scope": "Exact build only. This establishes client-owned stream closure; it does not assign ownership to a launcher-supplied override buffer.",
        },
    }:
        errors.append("read-service lifetime changed")
    correlation = document.get("observedRequestCorrelation", {})
    if correlation != {
        "sanitizedRelativePath": "data\\2A\\08\\00\\17.DAT",
        "candidateResourceId": "0x2a080017",
        "status": "inferred",
        "basis": "The numeric-mode formatter maps the candidate id to the observed path, but the observed request was not captured entering 0x00c99130 or 0x0044b3a0.",
        "binaryImmediateOccurrences": 0,
    }:
        errors.append("observed-request inference boundary changed")
    signature = document.get("preOpenSignature", {})
    if (
        signature.get("status") != "SUPPORTED_EXACT_BUILD"
        or signature.get("functionVa") != "0x00c96850"
        or signature.get("patternVa") != "0x00c96972"
        or signature.get("fileOffset") != "0x00896972"
        or signature.get("pattern")
        != "6A 00 68 ?? ?? ?? ?? 83 C6 04 56 8B CF E8 ?? ?? ?? ??"
        or signature.get("callPatternOffset") != 13
        or signature.get("callVa") != "0x00c9697f"
        or signature.get("callTargetVa") != "0x00453c00"
        or signature.get("maskedFields") != [
            "bytes 3-6: absolute address of the rb mode literal",
            "bytes 14-17: call rel32 displacement",
        ]
        or signature.get("exactBuildMatchCount") != 1
        or signature.get("stableByteMutation") != {
            "patternOffset": 11,
            "from": "0x8b",
            "to": "0x8a",
            "matchCount": 0,
        }
        or signature.get("callSiteContract") != {
            "thisObject": "the matching FileThread-owned LocalFile record",
            "pathArgument": "borrowed Resource+0x04 narrow path wrapper",
            "modeArgument": "read-mode literal rb",
            "retryCount": 0,
            "returnVa": "0x00c96984",
            "returnUse": "The FileThread service loop does not branch on the LocalFile open return at this call site.",
        }
        or signature.get("scope")
        != "This signature is validated only for the pinned executable identity; cross-build stability is not claimed."
    ):
        errors.append("exact-build signature contract changed")
    gate = document.get("hookGate", {})
    if gate != {
        "stableExactBuildSignature": "SUPPORTED",
        "numericProducerChain": "SUPPORTED statically",
        "observedRequestProducerIdentity": "INFERRED",
        "pathOwnership": "SUPPORTED for the original Resource path",
        "streamOwnershipAndLifetime": "SUPPORTED - LocalFile owns +0x04; read service closes and clears it at 0x00c962f5 after the read attempt and before publishing success state 2",
        "successfulRedirectSemantics": "NOT TESTED",
        "missingFileFallthrough": "NOT TESTED",
        "originalPathIdentityForwarding": "NOT TESTED",
    }:
        errors.append("hook insufficiency boundary changed")
    adoption = document.get("bahamutAdoption", {})
    if adoption != {
        "safeNow": [
            "Gate the hook on the exact executable hash and unique pre-open signature.",
            "Treat Resource+0x04 as a borrowed narrow path wrapper at the pre-open boundary.",
            "Decode numeric resource ids as uppercase MSB-first byte groups only when the client numeric-mode gate is established.",
            "Rely on the client-owned LocalFile stream being closed and cleared after the normal read attempt; keep any launcher-owned substitute path alive across the borrowed open call.",
        ],
        "blocked": [
            "Do not claim that the observed 0x2a080017 candidate traversed the numeric producer.",
            "Do not ship redirect or missing-file fallback behavior before the designed runtime experiments establish it.",
            "Do not infer ownership for a launcher-supplied override buffer or claim cross-build signature stability.",
        ],
        "nextRuntimeDiscriminator": "On the pinned build, pass one independently owned substitute wrapper through the original LocalFile open call, observe the replacement FILE* complete the read and close at 0x00c962f5, then repeat a miss while proving the original Resource+0x04 bytes are forwarded unchanged.",
    }:
        errors.append("Bahamut adoption boundary changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]" + r":\\\\",
        "/" + "Users/",
        "/" + "home/",
        "agent-" + "islands",
    )
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine or private-island path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("numericProducer", "resourcePathOffset"), 8),
        (("numericProducer", "resourceIdOffset"), 84),
        (("formatter", "numericModeCondition"), "always"),
        (("formatter", "byteOrder"), "least significant first"),
        (("pathWrapper", "heapFlagValue"), 1),
        (("readServiceLifetime", "closeVa"), "0x00453190"),
        (("readServiceLifetime", "directCloseCallVas"), {}),
        (("observedRequestCorrelation", "status"), "verified"),
        (("observedRequestCorrelation", "binaryImmediateOccurrences"), 1),
        (("preOpenSignature", "exactBuildMatchCount"), 2),
        (("preOpenSignature", "callPatternOffset"), 14),
        (("hookGate", "missingFileFallthrough"), "SUPPORTED"),
        (("bahamutAdoption", "safeNow"), []),
        (("source", "binarySha256"), "0" * 64),
    ]
    mutations: list[dict] = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        mutations.append(changed)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "C:" + "\\Users\\maintainer\\private"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def find_matches(data: bytes, pattern: bytes = PATTERN, mask: bytes = MASK) -> list[int]:
    runs: list[tuple[int, int]] = []
    start = None
    for index, keep in enumerate(mask + b"\x00"):
        if keep and start is None:
            start = index
        elif not keep and start is not None:
            runs.append((start, index))
            start = None
    anchor_start, anchor_end = max(runs, key=lambda run: run[1] - run[0])
    anchor = pattern[anchor_start:anchor_end]
    matches: list[int] = []
    cursor = 0
    while True:
        found = data.find(anchor, cursor)
        if found < 0:
            return matches
        offset = found - anchor_start
        if (
            0 <= offset <= len(data) - len(pattern)
            and all(
                not keep or data[offset + index] == pattern[index]
                for index, keep in enumerate(mask)
            )
        ):
            matches.append(offset)
        cursor = found + 1


def resolve_rel32(data: bytes, file_offset: int) -> int | None:
    if file_offset < 0 or file_offset + 5 > len(data) or data[file_offset] != 0xE8:
        return None
    displacement = int.from_bytes(data[file_offset + 1:file_offset + 5], "little", signed=True)
    return 0x00400000 + file_offset + 5 + displacement


def verify_executable(path: Path) -> list[str]:
    errors: list[str] = []
    data = path.read_bytes()
    if len(data) != EXPECTED_SIZE:
        errors.append(f"executable size is {len(data)}, expected {EXPECTED_SIZE}")
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        errors.append(f"executable SHA-256 is {digest}, expected {EXPECTED_SHA256}")
    matches = find_matches(data)
    if matches != [EXPECTED_PATTERN_OFFSET]:
        errors.append(f"signature matches are {[hex(item) for item in matches]}")
    if len(data) >= EXPECTED_PATTERN_OFFSET + len(PATTERN):
        call_offset = EXPECTED_PATTERN_OFFSET + 13
        call_target = resolve_rel32(data, call_offset)
        if call_target != 0x00453C00:
            errors.append(f"signature call resolves to {call_target!r}")
    close_body = data[CLOSE_FUNCTION_OFFSET:CLOSE_FUNCTION_OFFSET + len(CLOSE_FUNCTION)]
    if close_body != CLOSE_FUNCTION:
        errors.append("LocalFile close body changed")
    else:
        if resolve_rel32(data, CLOSE_FUNCTION_OFFSET + 11) != 0x009D71DF:
            errors.append("LocalFile close fflush target changed")
        if resolve_rel32(data, CLOSE_FUNCTION_OFFSET + 20) != 0x009D2646:
            errors.append("LocalFile close fclose target changed")
    for call_offset, expected_target in DIRECT_CLOSE_CALLS.items():
        target = resolve_rel32(data, call_offset)
        if target != expected_target:
            errors.append(
                f"direct close call at {call_offset + 0x00400000:#010x} resolves to {target!r}"
            )
    candidate_id = (0x2A080017).to_bytes(4, "little")
    if data.count(candidate_id) != 0:
        errors.append("candidate resource id occurs as a little-endian immediate")
    mutated_pattern = bytearray(PATTERN)
    mutated_pattern[11] = 0x8A
    if find_matches(data, bytes(mutated_pattern)):
        errors.append("deliberately mutated stable byte still matches")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, help="pinned retail executable for byte checks")
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if errors:
        print(f"FAIL: {len(errors)} resource-path producer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable signature" if args.exe else ""
    print(f"PASS: resource-path producer manifest, 15 mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
