#!/usr/bin/env python3
"""Validate the client music initiator contract, optional exe bytes, install files and sheet definitions."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

import verify_sound_effect_triggers as scans
import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "client_music_initiators.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
HEX8_RE = re.compile(r"^0x[0-9a-f]{8}$")
CLUSTER = (0x004B5690, 0x004C3980)
TITLE_BASE = 0x27950000
SHEET_TYPES = {"s32", "u16", "u8"}
REQUIRED_CHECKS = {
    "release entry",
    "control entry",
    "volume entry",
    "parameter entry",
    "hold wrapper",
    "requested id entry",
    "swap entry",
    "gate entry",
    "reset stops the module first",
    "reset clears pending cache gate and cells",
    "receiver module table",
    "request by table index",
    "request table head",
    "request table tail",
    "release both slots",
    "boot manager song request",
    "login phase song request",
    "title phase song gate and request",
    "character selection song gate and request",
    "title song constant",
    "patch phase track selection",
    "patch phase request result",
    "patch phase entry gates",
    "patch phase track table",
    "patch phase vtable",
    "title phase slot 6 cell",
    "character selection slot 6 cell",
    "login phase slot 0 and 4 cells",
    "login phase destructor release",
    "login phase release",
    "title phase release, mode 1",
    "title phase release, mode 2",
    "title release duration push",
    "credit song request",
    "credit duration register",
    "credit release",
    "credit vtable",
    "menu song request",
    "cutscene begin disables the gate",
    "cutscene begin sets state",
    "cutscene end releases the track",
    "cutscene end resumes the held song",
    "cutscene end enables the gate",
    "cutscene end mode gate",
    "cutscene play",
    "cutscene stop and resume",
    "cutscene play wrapper",
    "cutscene stop wrapper",
    "cutscene volume member",
    "cutscene volume wrapper",
    "bgm clip arguments",
    "bgm clip flag compare",
    "bgm clip proxy actor cell",
    "proxy actor name",
    "unreferenced play helper",
    "unreferenced stop helper",
    "bgm clip slot 1 cell",
    "bgm clip stop name",
    "bgm clip volume name",
    "bgm clip name",
    "parameter driver body",
    "parameter driver scale",
    "chara element slot 6 cell",
    "parameter driver call",
    "reset site, DeleteAllActors",
    "reset site, RemoveActor",
    "reset site, opcode 0x0004",
    "reset site, state helper",
    "reset site, application update",
    "receive dispatcher low targets",
    "opcode index bytes 0x0004 to 0x0007",
    "opcode index byte 0x00cb",
    "env sound clip update cell",
    "env sound clip error text",
    "env sound clip type check",
    "env sound clip window check",
    "env sound clip enable hand-off",
    "env sound enable hand-off is conditional",
    "env sound enable byte store",
    "env sound vtables",
    "env sound fade rate",
    "env sound fade step",
    "env sound fade out step",
    "env sound enable byte reset",
    "env sound fade constants",
    "env sound tick divisor",
    "cutscene begin active guard",
    "cutscene end active guard",
    "cutscene end clears active",
    "cutscene end module calls",
    "cut message dispatch",
    "cut message case 0",
    "cut message case 2",
    "cut message begin call",
    "reset site, opcode 0x0004 toggle",
    "reset site, opcode 0x0004 loop entry",
    "cut message mode setter",
    "cut manager slot 141 gate",
    "cut manager slot 141 end call",
    "generic sound request refuses class 0x07fe",
    "cut message jump table",
    "cut manager slot 141 cell",
    "cut manager slot 157 cell",
}
EXPECTED_ENTRIES = {
    "control entry": (
        "0x004bbb30",
        ["0x004c3941", "0x004dcc22", "0x0075c945"],
    ),
    "request": (
        "0x004b7650",
        [
            "0x004bbb8c",
            "0x004bbbc1",
            "0x004bbc1f",
            "0x004bbc5e",
            "0x004bbcbd",
            "0x004bbcf4",
            "0x004bbd2c",
            "0x004bbd62",
            "0x004bbe42",
            "0x0060f6d7",
            "0x00687c57",
            "0x006a60b9",
            "0x006a8e7a",
            "0x006b3ad6",
            "0x00878c4b",
            "0x0087c809",
            "0x0088679d",
            "0x0088c29c",
        ],
    ),
    "release": (
        "0x004b5690",
        [
            "0x004bbc88",
            "0x0060f64f",
            "0x0060f746",
            "0x0061194c",
            "0x00611dd7",
            "0x00687c6f",
            "0x006a6127",
            "0x008759c8",
            "0x00886632",
            "0x00886702",
        ],
    ),
    "control": (
        "0x004b56c0",
        [
            "0x004bbbf1",
            "0x004bbc39",
            "0x0060f690",
            "0x0060f77d",
            "0x00611983",
            "0x00611e0f",
        ],
    ),
    "volume": (
        "0x004b56e0",
        ["0x0060f70a", "0x006119b2"],
    ),
    "mute": (
        "0x004b5710",
        ["0x004bbcd6", "0x004bbd0d"],
    ),
    "parameter": (
        "0x004b5740",
        ["0x004bbd43", "0x00588fc6", "0x00588fef"],
    ),
    "hold wrapper": (
        "0x004b57a0",
        ["0x0060f6bd"],
    ),
    "requested id": (
        "0x004b57b0",
        ["0x004bbc6a", "0x0060f661"],
    ),
    "swap": (
        "0x004b57d0",
        [
            "0x004bbc02",
            "0x004bbc2b",
            "0x0060f6a2",
            "0x0060f768",
            "0x0061196e",
            "0x00611df7",
        ],
    ),
    "gate": (
        "0x004c3900",
        ["0x00611d32", "0x00611e23"],
    ),
    "request by index": (
        "0x004b7560",
        [],
    ),
    "release both": (
        "0x004b7720",
        [],
    ),
    "module reset": (
        "0x004b7340",
        ["0x004d6a5c", "0x004db45d", "0x004dca7d", "0x004dcd38", "0x004dce7b"],
    ),
}
EXPECTED_IMMEDIATES = {
    "0x0132f384": (
        [
            "0x004b56b0",
            "0x004b56d3",
            "0x004b5700",
            "0x004b5730",
            "0x004b5767",
            "0x004b5785",
            "0x004b57c3",
            "0x004b7582",
            "0x004b75ba",
            "0x004b766e",
            "0x004b76a5",
            "0x004b7743",
            "0x004b777d",
            "0x00f0772f",
            "0x00f2e63a",
        ],
        ["0x00f0772f", "0x00f2e63a"],
    ),
    "0x0132f398": (
        ["0x004b7383"],
        [],
    ),
    "0x0132f32c": (
        [
            "0x004b5694",
            "0x004b56c3",
            "0x004b56e3",
            "0x004b5713",
            "0x004b5743",
            "0x004b57b3",
            "0x004b57d3",
            "0x004b57e6",
            "0x004b7378",
            "0x004b7570",
            "0x004b75a5",
            "0x004b7660",
            "0x004b7690",
            "0x004b772c",
            "0x004b7750",
            "0x004b775f",
            "0x004b776c",
            "0x00f07713",
        ],
        ["0x00f07713"],
    ),
    "0x0132f320": (
        ["0x004b575c", "0x004b577a", "0x004b7446"],
        [],
    ),
    "0x0132f324": (
        ["0x004b744b"],
        [],
    ),
    "0x0132f3d0": (
        [
            "0x004b743d",
            "0x004bbb34",
            "0x004bbe0c",
            "0x004c390e",
            "0x004c391e",
            "0x004c3966",
            "0x00f07783",
        ],
        ["0x00f07783"],
    ),
    "0x0132f3d4": (
        [
            "0x004b7428",
            "0x004bbba3",
            "0x004bbc8f",
            "0x004bbdc6",
            "0x004bbdf8",
            "0x00f07793",
        ],
        ["0x00f07793"],
    ),
    "0x0132f3d8": (
        ["0x004bbc9a", "0x00f077a1"],
        ["0x00f077a1"],
    ),
    "0x012695ec": (
        ["0x004b742e", "0x004bbb4e", "0x004bbe1a", "0x004c392e", "0x004c3948"],
        [],
    ),
    "0x00f8d668": (
        ["0x004b7590", "0x004b75fc", "0x004b762e"],
        [],
    ),
    "0x012d63d8": (
        ["0x0087c7f7"],
        ["0x0087c7f7"],
    ),
}
EXPECTED_INITIATORS = {
    "boot-manager-title": ("0x006b3ad6", "0x07fe001c", 2000),
    "login-phase-title": ("0x00878c4b", "0x07fe001c", 2000),
    "title-phase-title": ("0x0088679d", "0x07fe001c", 2000),
    "character-selection-title": ("0x0088c29c", "0x07fe001c", 2000),
    "patch-phase-title": ("0x0087c809", "0x27950015..0x27950017", 2000),
    "credit-song": ("0x006a60b9", "0x07fe003f", 0),
    "menu-song": ("0x006a8e7a", "0x07fe0046", 400),
    "cutscene-bgm": ("0x0060f6d7", "clip data +0x20", "clip data +0x14"),
}
EXPECTED_RELEASES = {
    "login-phase-release": ("0x008759c8", 2000),
    "title-phase-release-1": ("0x00886632", 2000),
    "title-phase-release-2": ("0x00886702", 2000),
    "credit-release": ("0x006a6127", 2000),
    "cutscene-end-release": ("0x00611dd7", 2000),
}
EXPECTED_UNREFERENCED = [
    "0x004b7560",
    "0x004b7720",
    "0x00687c40",
    "0x00687c60",
    "0x0060f6f0",
    "0x0060f720",
    "0x00611930",
    "0x00611990",
]
EXPECTED_CUT_CHAIN = {
    "cut manager begin": ["0x00611c30", ["0x0061215a", "0x0061c587"], []],
    "cut manager end": ["0x00611d60", ["0x0061c7ea"], []],
    "cut message begin body": ["0x0061c4d0", ["0x0061c6e7"], []],
    "cut message mode setter": ["0x0060f3f0", ["0x0061c6fe"], []],
    "cut message handler": ["0x0061c6c0", [], ["0x00fb9018"]],
    "cut manager slot 141": ["0x0061c790", [], ["0x00fb8fd8"]],
}
EXPECTED_RESET_CONDITIONS = {
    "0x004dca7d": "the removed actor equals the local actor at owner +0x17838",
    "0x004dcd38": "the actor equals the local actor at owner +0x17838",
    "0x004dce7b": "the byte at owner +0x54b is clear: the handler sets it, walks the actor map and then resets unconditionally. When the byte is set, the handler clears it, calls 0x004d8080(1) and exits without a reset",
    "0x004d6a5c": "state helper 0x004d6a50, reached from 0x004d9980 with id 0xc0000000",
    "0x004db45d": "application update 0x004daf00, condition unresolved",
}
EXPECTED_CLAIMS = {
    "entry-census": "STATIC",
    "slot-array-closure": "STATIC",
    "packet-lua-gate-callers": "STATIC",
    "boot-title-song": "STATIC",
    "patch-phase-tracks": "STATIC",
    "title-track-files": "OBSERVED",
    "credit-song": "STATIC",
    "menu-song": "STATIC",
    "menu-song-owner": "INFERRED",
    "login-releases": "STATIC",
    "cutscene-gate": "STATIC",
    "cutscene-bgm-clip": "STATIC",
    "cutscene-hold-and-restore": "STATIC",
    "cutscene-owner-class": "STATIC",
    "engine-bgm-clips": "NOT FOUND",
    "parameter-cells": "STATIC",
    "parameter-states": "UNPROVED",
    "module-reset-sites": "STATIC",
    "reset-stop-semantics": "INFERRED",
    "unreferenced-entries": "STATIC",
    "zone-sheet-shapes": "OBSERVED",
    "no-zone-time-mount-battle": "NOT FOUND",
    "ambient-clip": "STATIC",
    "ambient-fade": "STATIC",
    "ambient-meaning": "INFERRED",
    "ambient-selection": "UNPROVED",
    "cutscene-pairing": "STATIC",
    "cutscene-mode-setter": "STATIC",
    "cutscene-play-owner": "INFERRED",
    "cutscene-end-module-calls": "STATIC",
    "cutscene-end-stop-semantics": "INFERRED",
    "reset-opcode-4-toggle": "STATIC",
    "engine-bgm-clip-route": "UNPROVED",
    "music-class-rejection": "STATIC",
}
TITLE_TRACKS = [
    (21, 2874442, "efd2da18ea21da5b5411d88861fe13daf99e0f46ae4416b12dbad48583755845"),
    (22, 2996059, "4469f6df4c8cd26823cb9971534b3db2e64e3420b5d9579c314d84997ca71a8e"),
    (23, 2684944, "eee0c6845ceddc6ca5c7546b0aad3157b143ecfb5f90a400b8fab412151b476e"),
]
EXPECTED_SHEETS = {
    "_zoneParam": ("0x01030025", 1, 1, ["s32"], [0], 0, 501),
    "_region": ("0x010304d3", 1, 2, ["u16"], [1], 0, 931),
    "regionParam": ("0x0103059c", 3, 3, ["s32"] * 3, [0, 1, 2], 1, 6),
    "zoneGroupParam": ("0x010305a0", 6, 6, ["s32"] * 6, list(range(6)), 100, 502),
    "_layout": ("0x0103045f", 3, 4, ["u8", "s32", "u16"], [1, 2, 3], 1, 8085),
}
FIXED = {
    ("module", "getterVa"): "0x004d7500",
    ("module", "slotArray"): "0x0132f384",
    ("module", "slotStride"): "0x18",
    ("module", "slotCount"): 2,
    ("module", "selectedSlotCell"): "0x0132f32c",
    ("module", "clusterRange"): ["0x004b5690", "0x004c3980"],
    ("titleTracks", "selectorVa"): "0x0087c750",
    ("titleTracks", "tableVa"): "0x012d63d8",
    ("titleTracks", "tableValues"): [21, 22, 23],
    ("titleTracks", "resourceBase"): "0x27950000",
    ("titleTracks", "retryLimit"): 30,
    ("titleTracks", "retryCounterVa"): "0x0135107c",
    ("titleTracks", "startedFlagVa"): "0x01351080",
    ("titleTracks", "durationArgument"): 2000,
    ("gate", "cellVa"): "0x0132f3d0",
    ("gate", "cachedPairVa"): "0x012695ec",
    ("gate", "disabledBy"): "0x00611c30 (cut manager begin, argument 0)",
    ("gate", "enabledBy"): "0x00611d60 (cut manager end, argument 1)",
    ("cutManager", "endCallerVa"): "0x0061c7ea",
    ("cutManager", "beginCallers"): ["0x0061215a", "0x0061c587"],
    ("cutManager", "fields"): {
        "holdByte": "0x160",
        "playingByte": "0x161",
        "activeByte": "0x180",
        "modeDword": "0x1e8",
    },
    ("cutManager", "clipDataOffsets"): {
        "resourceId": "0x20",
        "duration": "0x14",
        "unused": "0x18",
    },
    ("cutManager", "endReleaseDuration"): 2000,
    ("cutManager", "endResumeControl"): [2000, 0],
    ("parameters", "writerVa"): "0x004b5740",
    ("parameters", "cells"): ["0x0132f320", "0x0132f324"],
    ("parameters", "driverVa"): "0x00588f60",
    ("parameters", "stateMeaning"): "UNPROVED",
    ("reset", "moduleResetVa"): "0x004b7340",
    ("reset", "stopCall"): {"receiverSlot": 7, "arguments": [0, 500]},
    ("reset", "opcodeNames"): {
        "0x0007": "DeleteAllActors",
        "0x00cb": "RemoveActor",
        "0x0004": "unnamed",
    },
    ("cutManager", "beginCallerOwners"): {
        "0x0061215a": "untraced",
        "0x0061c587": "0x0061c4d0, called by case 0 of the 0x0061c6c0 message handler",
    },
    ("cutManager", "messageHandler"): {
        "va": "0x0061c6c0",
        "owner": "Application::Scene::Actor::System::CutManagerActor slot 157",
        "tableVa": "0x0061c780",
        "caseCount": 4,
        "cases": {
            "0": "0x0061c4d0 stores the message and calls begin 0x00611c30",
            "1": "0x0060f440 untraced",
            "2": "0x0060f3f0 mode setter",
            "3": "0x0061c72b untraced",
        },
    },
    ("cutManager", "modeSetter"): {
        "va": "0x0060f3f0",
        "rule": "+0x1e8 becomes the larger of its value and the dword the argument points at",
        "writers": ["0x00611c79 begin, writes 0", "0x0060f401 mode setter, raises"],
        "armsEnd": "When virtual +0x0c of the object at +0x174 returns true it also sets the dword +0x1e4, which slot 141 tests before it runs the end routine.",
        "senderOfValue": "untraced",
    },
    (
        "cutManager",
        "pairing",
    ): "Begin returns false at once while +0x180 is set, and otherwise sets +0x180, clears +0x160 and +0x161, zeroes +0x1e8 and closes the gate. End returns at once while +0x180 is clear, and otherwise clears +0x180 after its restore steps.",
    ("cutManager", "endModuleCalls"): {
        "objectGetterVa": "0x0060b280",
        "first": {
            "vtableOffset": "0x08",
            "arguments": ["1.0", "1000"],
            "meaning": "untraced",
        },
        "second": {
            "vtableOffset": "0x1c",
            "arguments": ["0x0b", "1000"],
            "receiverSlot": 7,
            "slotVa": "0x00441b00",
            "tier": "INFERRED",
        },
    },
    (
        "cutManager",
        "endModeFour",
    ): "End passes 0 instead of -1 to 0x00610040 when +0x1e8 is 4, and the effect is untraced.",
    (
        "gate",
        "pairing",
    ): "The gate closes only on a begin that sets +0x180 and reopens only on an end that finds +0x180 set.",
    ("engineClips", "route"): "UNPROVED",
    ("engineClips", "retailCutData"): "NOT FOUND",
    ("ambient", "tickVa"): "0x008344a0",
    ("ambient", "instanceTypeId"): 6,
    ("ambient", "managerGetterVa"): "0x00807d20",
    ("ambient", "handoffVa"): "0x00808430",
    (
        "ambient",
        "handoffRule",
    ): "0x00808430 stores the enable byte into the manager entry only when it is non-zero, and the entry clears it on every update.",
    ("ambient", "environmentSoundUpdateVa"): "0x00807e80",
    ("ambient", "fadeDivisor"): 3.0,
    ("ambient", "fadeDivisorFlagged"): 1.0,
    ("ambient", "tickDivisor"): 300000.0,
    ("ambient", "volumeMax"): 1.0,
    ("ambient", "fields"): {"handle": "0x04", "enableByte": "0x0c", "volume": "0x10"},
    ("policy", "nativeZoneMusic"): "none found",
    ("policy", "nativeRegionMusic"): "none found",
    ("policy", "nativeTimeOfDayMusic"): "none found",
    ("policy", "nativeMountMusic"): "none found",
    ("policy", "nativeBattleStartMusic"): "none found",
    ("policy", "nativeAmbientSelection"): "layout data",
    (
        "install",
        "scope",
    ): "owner installation of build 2012.09.19.0001, existence, size, SHA-256 and SEDBSSCF magic only",
}


def hex8(value: int) -> str:
    return f"{value:#010x}"


def node_at(document: dict, path: tuple) -> object:
    node: object = document
    for key in path:
        node = node.get(key, {}) if isinstance(node, dict) else {}
    return node


def verify_checks(document: dict) -> list[str]:
    errors: list[str] = []
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        return ["staticChecks is not a list"]
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    missing = REQUIRED_CHECKS - set(names)
    if missing:
        errors.append(f"staticChecks lacks {sorted(missing)}")
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not HEX8_RE.match(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            pattern, mask = item.get("pattern", ""), item.get("mask", "")
            try:
                same = len(bytes.fromhex(pattern)) == len(bytes.fromhex(mask))
            except ValueError:
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
        elif item["kind"] == "dwords" and not item.get("values"):
            errors.append(f"staticChecks {item['name']!r} has no values")
    return errors


def site_set(document: dict, entry_name: str) -> set[str]:
    return {
        site["site"]
        for entry in document.get("entries", [])
        if entry.get("name") == entry_name
        for site in entry.get("sites", [])
        if isinstance(site, dict)
    }


def verify_entries(document: dict) -> list[str]:
    errors: list[str] = []
    entries = document.get("entries")
    if not isinstance(entries, list):
        return ["entries is not a list"]
    if [e.get("name") for e in entries] != list(EXPECTED_ENTRIES):
        errors.append("entry names or order changed")
    for entry in entries:
        name = entry.get("name")
        if name not in EXPECTED_ENTRIES:
            continue
        va, wanted_sites = EXPECTED_ENTRIES[name]
        if entry.get("va") != va:
            errors.append(f"entry {name!r} va changed")
        sites = entry.get("sites")
        if (
            not isinstance(sites, list)
            or [site.get("site") for site in sites if isinstance(site, dict)]
            != wanted_sites
        ):
            errors.append(f"entry {name!r} sites changed")
            continue
        for site in sites:
            if (
                not HEX8_RE.match(str(site.get("site", "")))
                or not str(site.get("role", "")).strip()
            ):
                errors.append(f"entry {name!r} has a malformed site")
        if entry.get("dataRefs") != []:
            errors.append(f"entry {name!r} dataRefs changed")
    check_names = {c.get("name") for c in document.get("staticChecks", [])}
    initiators = document.get("initiators")
    if not isinstance(initiators, list):
        errors.append("initiators is not a list")
    else:
        found = {
            i.get("id"): (i.get("site"), i.get("resource"), i.get("duration"))
            for i in initiators
        }
        if len(found) != len(initiators) or found != EXPECTED_INITIATORS:
            errors.append("initiator ids, sites, resources or durations changed")
        for item in initiators:
            if item.get("site") not in site_set(document, "request"):
                errors.append(f"initiator {item.get('id')!r} is not a request site")
            if item.get("check") not in check_names:
                errors.append(f"initiator {item.get('id')!r} names no static check")
            if item.get("tier") != "STATIC":
                errors.append(f"initiator {item.get('id')!r} tier changed")
    releases = document.get("releases")
    if not isinstance(releases, list):
        errors.append("releases is not a list")
    else:
        found_r = {r.get("id"): (r.get("site"), r.get("duration")) for r in releases}
        if len(found_r) != len(releases) or found_r != EXPECTED_RELEASES:
            errors.append("release ids, sites or durations changed")
        for item in releases:
            if item.get("site") not in site_set(document, "release"):
                errors.append(f"release {item.get('id')!r} is not a release site")
            if item.get("check") not in check_names:
                errors.append(f"release {item.get('id')!r} names no static check")
    immediates = document.get("immediateScans")
    if not isinstance(immediates, list):
        errors.append("immediateScans is not a list")
    else:
        found_i = {
            s.get("value"): (s.get("sites"), s.get("outsideCluster"))
            for s in immediates
        }
        if len(found_i) != len(immediates) or found_i != EXPECTED_IMMEDIATES:
            errors.append("immediate scans changed")
        for scan in immediates:
            sites = scan.get("sites", [])
            outside = [
                hex8(s)
                for s in (int(x, 16) for x in sites)
                if not CLUSTER[0] <= s < CLUSTER[1]
            ]
            if outside != scan.get("outsideCluster"):
                errors.append(f"immediate {scan.get('value')} outside list disagrees")
    unreferenced = document.get("unreferenced")
    if (
        not isinstance(unreferenced, list)
        or [u.get("va") for u in unreferenced] != EXPECTED_UNREFERENCED
    ):
        errors.append("unreferenced function list changed")
    reset_sites = document.get("reset", {}).get("sites")
    if not isinstance(reset_sites, list) or {
        s.get("site") for s in reset_sites
    } != site_set(document, "module reset"):
        errors.append("reset sites disagree with the module reset entry")
    elif {
        s.get("site"): s.get("condition") for s in reset_sites
    } != EXPECTED_RESET_CONDITIONS:
        errors.append("reset site conditions changed")
    errors += verify_cut_chain(document)
    return errors


def verify_cut_chain(document: dict) -> list[str]:
    chain = document.get("cutChain")
    if not isinstance(chain, list):
        return ["cutChain is not a list"]
    found = {
        c.get("name"): [
            c.get("va"),
            [s.get("site") for s in c.get("sites", []) if isinstance(s, dict)],
            c.get("dataRefs"),
        ]
        for c in chain
    }
    errors = []
    if len(found) != len(chain) or found != EXPECTED_CUT_CHAIN:
        errors.append("cut chain routines, sites or references changed")
    for item in chain:
        for site in item.get("sites", []):
            if not str(site.get("role", "")).strip():
                errors.append(
                    f"cut chain {item.get('name')!r} has a site without a role"
                )
    return errors


def verify_install_fields(document: dict) -> list[str]:
    errors: list[str] = []
    install = document.get("install", {})
    tracks = install.get("titleTrackFiles")
    if not isinstance(tracks, list) or len(tracks) != len(TITLE_TRACKS):
        return ["install.titleTrackFiles changed"]
    for row, (index, size, digest) in zip(tracks, TITLE_TRACKS):
        if (
            row.get("index") != index
            or row.get("resource") != hex8(TITLE_BASE + index)
            or row.get("size") != size
            or row.get("sha256") != digest
            or row.get("streams") != 1
        ):
            errors.append(f"title track {index} row changed")
    sheets = install.get("sheets")
    if not isinstance(sheets, list):
        return errors + ["install.sheets is not a list"]
    found = {}
    for sheet in sheets:
        found[sheet.get("name")] = (
            sheet.get("resource"),
            sheet.get("columnCount"),
            sheet.get("columnMax"),
            sheet.get("types"),
            sheet.get("index"),
            sheet.get("blockBegin"),
            sheet.get("blockCount"),
        )
        types = sheet.get("types")
        if (
            not isinstance(types, list)
            or len(types) != sheet.get("columnCount")
            or any(t not in SHEET_TYPES for t in types)
        ):
            errors.append(f"sheet {sheet.get('name')!r} types are malformed")
    if len(found) != len(sheets) or found != EXPECTED_SHEETS:
        errors.append("sheet definitions changed")
    return errors


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-client-music-initiators-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binary") != "ffxivgame.exe retail 1.23b"
        or source.get("build") != "2012.09.19.0001"
        or source.get("size") != EXPECTED_SIZE
        or source.get("binarySha256") != EXPECTED_SHA256
        or source.get("imageBase") != "0x00400000"
    ):
        errors.append("evidence identity changed")
    errors += verify_checks(document)
    errors += verify_entries(document)
    errors += verify_install_fields(document)
    for path, expected in FIXED.items():
        if node_at(document, path) != expected:
            errors.append(f"{'.'.join(path)} changed")
    claims = document.get("claims")
    if not isinstance(claims, list):
        errors.append("claims is not a list")
    else:
        found = {c.get("id"): c.get("tier") for c in claims}
        if len(found) != len(claims) or found != EXPECTED_CLAIMS:
            errors.append("claim ids or tiers changed")
        if any(not str(c.get("claim", "")).strip() for c in claims):
            errors.append("a claim has no text")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def mutate(document: dict, path: tuple, value: object) -> dict:
    changed = copy.deepcopy(document)
    node = changed
    for key in path[:-1]:
        node = node[key]
    node[path[-1]] = value
    return changed


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("module", "slotStride"), "0x20"),
        (("module", "clusterRange", 1), "0x004c3a80"),
        (("entries", 1, "va"), "0x004b7651"),
        (("entries", 1, "sites", 9, "site"), "0x0060f6d8"),
        (("entries", 1, "sites", 3, "role"), " "),
        (("entries", 1, "dataRefs"), ["0x00fd0618"]),
        (("entries", 0, "sites", 1, "site"), "0x004dcc23"),
        (("entries", 10, "sites", 0, "site"), "0x00611d33"),
        (
            ("entries", 11, "sites"),
            [{"site": "0x00000000", "role": "x", "function": "x"}],
        ),
        (("immediateScans", 0, "sites", 0), "0x004b56b1"),
        (("immediateScans", 0, "outsideCluster"), ["0x00f0772f"]),
        (("immediateScans", 10, "outsideCluster"), []),
        (("initiators", 0, "resource"), "0x07fe001d"),
        (("initiators", 4, "duration"), 1000),
        (("initiators", 5, "duration"), 2000),
        (("initiators", 6, "resource"), "0x07fe0047"),
        (("initiators", 7, "resource"), "clip data +0x24"),
        (("initiators", 2, "check"), "no such check"),
        (("initiators", 1, "tier"), "INFERRED"),
        (("releases", 3, "duration"), 0),
        (("releases", 4, "site"), "0x00611dd8"),
        (("titleTracks", "tableValues"), [21, 22, 24]),
        (("titleTracks", "retryLimit"), 31),
        (("titleTracks", "resourceBase"), "0x27960000"),
        (("gate", "disabledBy"), "0x00611d60 (cut manager end, argument 1)"),
        (("cutManager", "clipDataOffsets", "resourceId"), "0x24"),
        (("cutManager", "endResumeControl"), [2000, 1]),
        (("cutManager", "fields", "holdByte"), "0x161"),
        (("parameters", "cells"), ["0x0132f320"]),
        (("parameters", "stateMeaning"), "STATIC"),
        (("reset", "stopCall", "arguments"), [0, 501]),
        (("reset", "sites", 0, "site"), "0x004dca7e"),
        (("reset", "opcodeNames", "0x0007"), "RemoveActor"),
        (("unreferenced", 0, "va"), "0x004b7561"),
        (("engineClips", "route"), "STATIC"),
        (("engineClips", "retailCutData"), "OBSERVED"),
        (("gate", "pairing"), "The gate reopens in every case."),
        (("cutManager", "pairing"), "End always reopens the gate."),
        (("cutManager", "modeSetter", "writers"), ["0x00611c79 begin, writes 0"]),
        (("cutManager", "messageHandler", "cases", "2"), "0x0060f440 untraced"),
        (("cutManager", "endModuleCalls", "second", "tier"), "STATIC"),
        (("reset", "sites", 2, "condition"), "always"),
        (("cutChain", 3, "sites", 0, "site"), "0x0061c6ff"),
        (("cutChain", 4, "dataRefs"), []),
        (("claims", 13, "tier"), "INFERRED"),
        (("claims", 28, "tier"), "STATIC"),
        (("ambient", "instanceTypeId"), 7),
        (("ambient", "fadeDivisor"), 2.0),
        (("ambient", "tickDivisor"), 30000.0),
        (("ambient", "fields", "enableByte"), "0x10"),
        (("policy", "nativeZoneMusic"), "per zone"),
        (("policy", "nativeBattleStartMusic"), "combat song"),
        (("policy", "nativeAmbientSelection"), "music control"),
        (("claims", 0, "tier"), "OBSERVED"),
        (("claims", 21, "tier"), "STATIC"),
        (("claims", 6, "id"), "credit-songs"),
        (("claims", 10, "claim"), " "),
        (("install", "titleTrackFiles", 0, "size"), 2874443),
        (("install", "titleTrackFiles", 1, "sha256"), "0" * 63),
        (("install", "titleTrackFiles", 2, "resource"), "0x27950018"),
        (("install", "sheets", 0, "types"), ["s32", "s32"]),
        (("install", "sheets", 4, "types", 2), "s32"),
        (("install", "sheets", 1, "blockCount"), 930),
        (("install", "scope"), "everything"),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x4b56c0"),
    ]
    mutations = [mutate(document, path, value) for path, value in cases]
    for name in (
        "release entry",
        "gate entry",
        "patch phase track selection",
        "cutscene play",
        "cutscene end enables the gate",
        "parameter driver body",
        "reset site, RemoveActor",
        "env sound fade step",
        "env sound clip type check",
        "bgm clip arguments",
        "cutscene begin active guard",
        "cutscene end active guard",
        "cut message mode setter",
        "cut message jump table",
        "reset site, opcode 0x0004 toggle",
        "generic sound request refuses class 0x07fe",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    for key in (
        "initiators",
        "releases",
        "claims",
        "entries",
        "unreferenced",
        "cutChain",
    ):
        short = copy.deepcopy(document)
        short[key].pop()
        mutations.append(short)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def image_dword_refs(data: bytes, value: int) -> list[int]:
    """VAs of a little-endian dword in any section of the image."""
    needle = value.to_bytes(4, "little")
    found = []
    for start, _virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        end = raw_offset + raw_size
        at = data.find(needle, raw_offset, end)
        while at != -1:
            found.append(start + at - raw_offset)
            at = data.find(needle, at + 1, end)
    return found


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    errors = image_checks.verify_image(data, document, require_identity)
    if any("not a readable PE32" in error for error in errors):
        return errors
    entry_vas = {int(e["va"], 16) for e in document["entries"]}
    targets = entry_vas | {int(u["va"], 16) for u in document["unreferenced"]}
    reached = scans.call_sites(data, targets)
    for entry in document["entries"]:
        va = int(entry["va"], 16)
        found = [hex8(site) for site in reached[va]]
        if found != [site["site"] for site in entry["sites"]]:
            errors.append(f"direct call sites of {entry['name']} are {found}")
        refs = [hex8(site) for site in image_dword_refs(data, va)]
        if refs != entry["dataRefs"]:
            errors.append(f"dword references to {entry['name']} are {refs}")
    for item in document["unreferenced"]:
        va = int(item["va"], 16)
        if va not in entry_vas and (reached[va] or image_dword_refs(data, va)):
            errors.append(f"unreferenced function {item['va']} has a reference")
    for scan in document["immediateScans"]:
        found = [hex8(site) for site in scans.sites_of(data, int(scan["value"], 16))]
        if found != scan["sites"]:
            errors.append(f"immediate {scan['value']} sites are {found}")
    chain = document["cutChain"]
    chain_reached = scans.call_sites(data, {int(c["va"], 16) for c in chain})
    for item in chain:
        va = int(item["va"], 16)
        found = [hex8(site) for site in chain_reached[va]]
        if found != [s["site"] for s in item["sites"]]:
            errors.append(f"direct call sites of {item['name']} are {found}")
        refs = [hex8(site) for site in image_dword_refs(data, va)]
        if refs != item["dataRefs"]:
            errors.append(f"dword references to {item['name']} are {refs}")
    return errors


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def verify_install(root: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors = []
    for row in document["install"]["titleTrackFiles"]:
        path = root / scans.resource_path(int(row["resource"], 16))
        if not path.is_file():
            errors.append(f"title track {row['index']} file is missing")
            continue
        data = path.read_bytes()
        if (
            len(data) != row["size"]
            or hashlib.sha256(data).hexdigest() != row["sha256"]
        ):
            errors.append(f"title track {row['index']} size or SHA-256 differs")
        if data[:8] != b"SEDBSSCF":
            errors.append(f"title track {row['index']} lacks the SEDBSSCF magic")
        elif int.from_bytes(data[0x34:0x36], "little") != row["streams"]:
            errors.append(f"title track {row['index']} stream count differs")
    return errors


def parse_sheet(text: str) -> dict:
    """Name, column shape and first block of a decoded SSD sheet definition."""
    sheet = ET.fromstring(text.lstrip(chr(0xFEFF))).find("sheet")
    if sheet is None:
        raise ValueError("no sheet element")
    block = sheet.find("./block/file")
    return {
        "name": sheet.get("name"),
        "columnCount": int(sheet.get("column_count")),
        "columnMax": int(sheet.get("column_max")),
        "types": [p.text for p in sheet.findall("./type/param")],
        "index": [int(p.text) for p in sheet.findall("./index/param")],
        "blockBegin": int(block.get("begin")),
        "blockCount": int(block.get("count")),
    }


def verify_sheets(root: Path, xivl: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors = []
    for sheet in document["install"]["sheets"]:
        path = root / scans.resource_path(int(sheet["resource"], 16))
        with tempfile.TemporaryDirectory() as raw:
            try:
                result = subprocess.run(
                    [str(xivl), "extract-resource", str(path), "--output", raw]
                    + ["--as", "scrambled-xml"],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                failed = result.returncode != 0
            except OSError:
                failed = True
            decoded = Path(raw) / "payloads" / "decoded.xml"
            if failed or not decoded.is_file():
                errors.append(f"sheet {sheet['name']} did not decode")
                continue
            found = parse_sheet(decoded.read_text(encoding="utf-8"))
        for key in (
            "name",
            "columnCount",
            "columnMax",
            "types",
            "index",
            "blockBegin",
            "blockCount",
        ):
            if found[key] != sheet[key]:
                errors.append(f"sheet {sheet['name']} {key} is {found[key]}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install", type=Path, help="game root for the title track file checks"
    )
    parser.add_argument(
        "--xivl", type=Path, help="xivl executable that decodes the sheet definitions"
    )
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
        if args.xivl:
            errors += verify_sheets(args.install, args.xivl)
    if errors:
        print(f"FAIL: {len(errors)} client music initiator error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install files"
        if args.xivl:
            suffix += " plus sheet definitions"
    print(f"PASS: client music initiator manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
