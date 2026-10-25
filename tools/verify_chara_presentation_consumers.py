#!/usr/bin/env python3
"""Validate the chara presentation consumer contract, optional exe bytes and the scans."""

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
MANIFEST = ROOT / "config" / "chara_presentation_consumers.json"
LOOK_AT = "chara-look-at-consumers"
TURN = "chara-turn-and-field-consumers"
PAGES = {
    LOOK_AT: ROOT / "docs" / "actor" / f"{LOOK_AT}.md",
    TURN: ROOT / "docs" / "actor" / f"{TURN}.md",
}
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
CODE_START = 0x00401000
HEX_ADDRESS_RE = re.compile(r"^0x[0-9a-f]{8}$")
TIERS = {"STATIC", "INFERRED", "UNPROVED", "NOT FOUND"}
SECTIONS = {
    LOOK_AT: [
        "record-path",
        "look-at-records",
        "server-feed",
        "arbiter",
        "actor-controller",
        "ik-constraints",
    ],
    TURN: [
        "turn-request",
        "turn-completion",
        "visibility",
        "floating-offset",
        "map-marker",
        "target-fields",
    ],
}
TABLE_KEYS = {"byteTable", "jumpTable", "indexTable"}
# EXPECTED-BEGIN
EXPECTED_CLAIMS = {
    "scene-module": "STATIC",
    "scene-pump": "STATIC",
    "container-dispatch": "STATIC",
    "actor-gate": "STATIC",
    "record-cases": "STATIC",
    "record-init": "STATIC",
    "flag-bits": "STATIC",
    "native-writers": "STATIC",
    "cancel-silent": "STATIC",
    "kind-four": "NOT FOUND",
    "kind-zero": "NOT FOUND",
    "named-record": "STATIC",
    "feed-route": "STATIC",
    "feed-target": "STATIC",
    "feed-character": "STATIC",
    "feed-position": "STATIC",
    "feed-angles": "STATIC",
    "feed-cancel": "STATIC",
    "arbiter-call": "STATIC",
    "arbiter-select": "STATIC",
    "arbiter-skip": "STATIC",
    "arbiter-kinds": "STATIC",
    "arbiter-payloads": "STATIC",
    "arbiter-priority-byte": "STATIC",
    "arbiter-marker": "STATIC",
    "arbiter-resend": "STATIC",
    "resend-bits": "STATIC",
    "controller-object": "STATIC",
    "controller-writers": "STATIC",
    "zero-weight": "STATIC",
    "default-node": "STATIC",
    "node-meaning": "INFERRED",
    "state-machine": "STATIC",
    "bone-names": "STATIC",
    "controller-clear": "STATIC",
    "weight-flow": "STATIC",
    "target-flow": "STATIC",
    "ik-types": "STATIC",
    "weight-gate": "STATIC",
    "weight-role": "INFERRED",
    "ik-solver": "UNPROVED",
    "turn-request": "STATIC",
    "turn-back": "STATIC",
    "heading-post": "STATIC",
    "turn-start": "STATIC",
    "angle-classes": "STATIC",
    "turn-timer": "STATIC",
    "timer-step": "STATIC",
    "event-sites": "STATIC",
    "event-return": "STATIC",
    "turn-bit-sites": "STATIC",
    "turn-unit": "UNPROVED",
    "visibility-commands": "STATIC",
    "visibility-byte": "STATIC",
    "visibility-notify": "STATIC",
    "floating-case": "STATIC",
    "floating-easing": "STATIC",
    "floating-use": "STATIC",
    "floating-role": "INFERRED",
    "marker-number": "STATIC",
    "marker-entity": "STATIC",
    "marker-class": "STATIC",
    "marker-strings": "STATIC",
    "marker-string-readers": "NOT FOUND",
    "pending-apply": "STATIC",
    "pending-slot": "STATIC",
    "target-0d3": "STATIC",
    "target-npc": "STATIC",
    "target-npc-writer": "NOT FOUND",
}
EXPECTED_SECTIONS = {
    "scenePath": {
        "producerVa": "0x004d7980",
        "commandOffset": "0x15",
        "elementSceneOffset": "0x84",
        "elementKeyOffset": "0x88",
        "sceneModuleVtableVa": "0x00f91de8",
        "sceneModulePostSlot": 3,
        "sceneModulePostVa": "0x004e8390",
        "packetVtableVa": "0x00f8cc14",
        "pumpVa": "0x004e9700",
        "pumpOwnCommands": [3, 6],
        "gateVa": "0x0060c140",
        "gateRange": ["0x0008", "0x0116"],
        "containerDispatchVa": "0x007c93c0",
        "actorSlotOffset": "0x274",
        "actorCommandRange": ["0x0015", "0x0115"],
        "commandTableVa": "0x007c9720",
        "commandTableFirst": "0x000e",
        "commandTable": [
            "0x007c96fe",
            "0x007c9661",
            "0x007c9696",
            "0x007c96b4",
            "0x007c96d2",
            "0x007c963e",
            "0x007c961c",
        ],
        "showCommand": "0x0010",
        "hideCommand": "0x0011",
        "visibilityVa": "0x007cccc0",
    },
    "charaActor": {
        "vtableVa": "0x00fc0d34",
        "recordSlot": 157,
        "recordHandlerVa": "0x00662d30",
        "busySlot": 154,
        "busyVa": "0x00669650",
        "busyOffset": "0x2b70",
        "busyBit": 6,
        "updateSlot": 142,
        "updateVa": "0x006679c0",
        "visibleSlot": 43,
        "visibleVa": "0x0061ff40",
        "recordLimit": "0x63",
        "byteTableVa": "0x00663c08",
        "jumpTableVa": "0x00663ae8",
        "ignoredCase": "0x00663ad4",
        "records": [
            {"record": "0x0a", "caseVa": "0x006633cc", "calls": ["0x0065ceb0"]},
            {
                "record": "0x0b",
                "caseVa": "0x0066340c",
                "calls": ["0x0065ceb0", "0x0065cf00", "0x0065d020"],
            },
            {
                "record": "0x0c",
                "caseVa": "0x00663497",
                "calls": ["0x0065ceb0", "0x0065cf00"],
            },
            {"record": "0x0d", "caseVa": "0x00663520", "calls": ["0x0065cfa0"]},
            {"record": "0x0e", "caseVa": "0x00663569", "calls": ["0x006617f0"]},
            {"record": "0x26", "caseVa": "0x00663074", "calls": ["0x0065cc60"]},
            {"record": "0x30", "caseVa": "0x00662f44", "calls": ["0x0065c220"]},
            {"record": "0x31", "caseVa": "0x00662f6c", "calls": ["0x0065c250"]},
            {"record": "0x32", "caseVa": "0x00662f94", "calls": ["0x0065c220"]},
            {"record": "0x33", "caseVa": "0x00662fbc", "calls": ["0x007b0690"]},
        ],
    },
    "lookAt": {
        "recordBase": "0x290",
        "recordSize": "0x30",
        "banks": 2,
        "slots": 3,
        "recordFields": {
            "flags": "0x00",
            "priority": "0x02",
            "decay": "0x03",
            "target": "0x04",
            "name": "0x08",
            "weight": "0x18",
            "vector": "0x20",
        },
        "initializerVa": "0x0058f130",
        "initial": {
            "flags": "0x001a",
            "priority": 0,
            "decay": 0,
            "target": "0xc0000000",
            "weight": 1.0,
        },
        "flagBits": {"sent": 0, "kindLow": 1, "kindHigh": 3, "messageFlag": 4},
        "writers": {
            "native": {"slot": 0, "priority": "0xc8"},
            "server": {"slot": 1, "priority": "0x64"},
            "recordWriterVa": "0x00585f60",
            "characterWorkerVa": "0x00587f80",
            "namedWorkerVa": "0x00587fd0",
            "positionWorkerVa": "0x00588020",
            "angleWorkerVa": "0x005880f0",
            "cancelWorkerVa": "0x00587ef0",
            "targetWorkerVa": "0x00587e70",
        },
        "slot6Va": "0x0058df90",
        "arbiterVa": "0x005861b0",
        "arbiterCall": "0x0058dfc1",
        "kindTableVa": "0x00586490",
        "kindTargets": [
            "0x00586269",
            "0x0058627b",
            "0x0058637f",
            "0x005863cc",
            "0x0058627b",
        ],
        "kinds": [
            {"kind": 0, "record": "0x0a", "size": 4},
            {
                "kind": 1,
                "record": "0x0b",
                "size": 12,
                "namedRecord": "0x0c",
                "namedSize": 40,
                "payloadPriority": "0x64",
                "group": 0,
            },
            {"kind": 2, "record": "0x0d", "size": 20},
            {"kind": 3, "record": "0x0e", "size": 16},
            {
                "kind": 4,
                "record": "0x0b",
                "size": 12,
                "namedRecord": "0x0c",
                "namedSize": 40,
                "payloadPriority": "0xc8",
                "group": 1,
            },
            {"kind": 5, "record": None},
            {"kind": 6, "record": None},
            {"kind": 7, "record": None},
        ],
        "payloads": {
            "0x0a": [{"offset": "0x00", "width": 1, "name": "bank"}],
            "0x0b": [
                {"offset": "0x00", "width": 1, "name": "bank"},
                {"offset": "0x01", "width": 1, "name": "groupBit0"},
                {"offset": "0x02", "width": 1, "name": "priority"},
                {"offset": "0x04", "width": 4, "name": "target"},
                {"offset": "0x08", "width": 4, "name": "weight"},
            ],
            "0x0c": [
                {"offset": "0x00", "width": 1, "name": "bank"},
                {"offset": "0x01", "width": 1, "name": "flagBit0GroupBit1"},
                {"offset": "0x02", "width": 1, "name": "priority"},
                {"offset": "0x04", "width": 4, "name": "target"},
                {"offset": "0x08", "width": 16, "name": "boneName"},
                {"offset": "0x18", "width": 4, "name": "weight"},
                {"offset": "0x1c", "width": 12, "name": "vector"},
            ],
            "0x0d": [
                {"offset": "0x00", "width": 1, "name": "bank"},
                {"offset": "0x01", "width": 1, "name": "flagBit0"},
                {"offset": "0x04", "width": 4, "name": "weight"},
                {"offset": "0x08", "width": 12, "name": "position"},
            ],
            "0x0e": [
                {"offset": "0x00", "width": 1, "name": "bank"},
                {"offset": "0x01", "width": 1, "name": "flagBit0"},
                {"offset": "0x04", "width": 4, "name": "weight"},
                {"offset": "0x08", "width": 8, "name": "angles"},
            ],
        },
        "resend": {
            "clearVa": "0x00586150",
            "triggerVa": "0x005877e0",
            "slot6Call": "0x0058dfa5",
            "byteOffset": "0x26d",
            "bits": [6, 2],
            "setBit6": "0x00585e09",
            "setBit2": "0x0058e407",
            "eventCode": "0x55",
            "eventPostVa": "0x0065ff37",
        },
    },
    "serverFeed": {
        "dispatchVa": "0x0058cca0",
        "predicateVa": "0x004d8830",
        "opcodeOffset": "0x02",
        "payloadOffset": "0x10",
        "fullRoute": {
            "entryVa": "0x0058cd21",
            "indexTableVa": "0x0058d7e4",
            "jumpTableVa": "0x0058d788",
            "firstOpcode": "0x000f",
            "defaultVa": "0x0058d675",
        },
        "stricterRoute": {
            "entryVa": "0x0058ccee",
            "indexTableVa": "0x0058d6b0",
            "jumpTableVa": "0x0058d6a4",
            "firstOpcode": "0x000f",
            "exitVa": "0x0058d67d",
        },
        "cases": [
            {
                "opcode": "0x00d3",
                "caseVa": "0x0058d039",
                "worker": "0x00587e70",
                "slot": 1,
                "priority": "0x64",
                "payload": "target dword 0",
            },
            {
                "opcode": "0x00db",
                "caseVa": "0x0058d1a1",
                "entry": "0x00589290",
                "worker": "0x00587f80",
                "slot": 1,
                "priority": "0x64",
                "payload": "target dword 0, weight dword 1",
            },
            {
                "opcode": "0x00dc",
                "caseVa": "0x0058d1ce",
                "entry": "0x00589350",
                "worker": "0x00588020",
                "slot": 1,
                "priority": "0x64",
                "payload": "position dwords 0 to 2, weight dword 3",
            },
            {
                "opcode": "0x00dd",
                "caseVa": "0x0058d1f8",
                "entry": "0x005893b0",
                "worker": "0x005880f0",
                "slot": 1,
                "priority": "0x64",
                "payload": "angle dwords 0 and 1, weight dword 2",
            },
            {
                "opcode": "0x00de",
                "caseVa": "0x0058d21b",
                "worker": "0x00587ef0",
                "slot": 1,
                "priority": "0x00",
                "payload": "none",
            },
            {
                "opcode": "0x00df",
                "caseVa": "0x0058d229",
                "entry": "0x005892e0",
                "worker": "0x00587fd0",
                "slot": 1,
                "priority": "0x64",
                "payload": "target dword 0, weight dword 1, bone name at +0x18",
            },
        ],
        "nullTargetMask": "0xe0000000",
        "nullTargetValue": "0xc0000000",
        "targetStoreOffset": "0x12c",
        "targetInitial": "0xc0000000",
        "targetInitVa": "0x0058b56f",
    },
    "controller": {
        "actorOffset": "0x2620",
        "slotBase": "0x10",
        "slotSize": "0x40",
        "groups": 2,
        "slotsPerGroup": 3,
        "slotFields": {
            "boneIndex": "0x00",
            "flags": "0x04",
            "zeroWeight": "0x08",
            "words": "0x10",
            "weight": "0x18",
            "target": "0x1c",
            "node": "0x20",
            "vector": "0x30",
        },
        "setters": [
            {
                "va": "0x00853b50",
                "flag": "0x200",
                "wrappers": ["0x00854830", "0x00854880"],
            },
            {"va": "0x00853c50", "flag": "0x80", "wrappers": ["0x008548e0"]},
            {"va": "0x00853d30", "flag": "0x100", "wrappers": ["0x00854d20"]},
        ],
        "setterAlways": "0x20",
        "setterTargetPresent": "0x08",
        "setterFlag": "0x40",
        "suppress": "0x10",
        "defaultNode": "EID_LOOK_AT",
        "ownNode": "EID_LOOK_AT1",
        "boneNames": ["k0", "s0", "t0"],
        "boneNameTableVa": "0x012d135c",
        "bindVa": "0x008540a0",
        "stateMachineVa": "0x00854600",
        "stateTableVa": "0x00854820",
        "stateMask": 7,
        "stateTargets": ["0x00854663", "0x008546bb", "0x008546fd", "0x00854777"],
        "constraintCalls": {
            "enable": "0x00ac95d0",
            "position": "0x00ac9980",
            "second": "0x00aca250",
            "weight": "0x00ac99b0",
        },
        "constraintType": 7,
        "ikVtableVa": "0x010a92e4",
        "ikTypeSlot": 5,
        "ikTypes": {
            "LookAtIKObject": 2,
            "ConstraintDirectionIKObject": 3,
            "TwoJointIKObject": 8,
        },
        "ikPositionOffsets": {"2": "0xf0", "3": "0x90", "8": "0x60"},
        "ikWeightOffsets": {"2": "0xd4", "3": "0x5c"},
        "positionSetterVa": "0x00add6b0",
        "weightSetterVa": "0x00add760",
        "ikStepSlot": 2,
        "ikStepInstallVa": "0x00aed860",
        "ikStepVa": "0x00aed730",
        "ikSolveStepVa": "0x00aea570",
        "ikActiveMinimum": 9.999999747378752e-06,
        "ikActiveOffset": "0xd4",
        "lookAtSolveSlot": 12,
        "lookAtSolveVa": "0x00aeab00",
        "lookAtSolveSize": "0x20ac",
    },
    "turn": {
        "requestRecord": "0x33",
        "requestCommand": "0x48",
        "requestCaseVa": "0x00662fbc",
        "controllerOffset": "0xbf0",
        "turnBackVa": "0x007b0690",
        "turnStartVa": "0x007b0530",
        "headingSetterVa": "0x00587d40",
        "headingRecord": "0x32",
        "headingCaseVa": "0x00662f94",
        "kindTableVa": "0x00fc0b78",
        "kindTable": [0, 0, 1, 2, 3],
        "unitsPerCircle": "0x1000000",
        "windowUnits": "0x38e39",
        "savedFlagBit": 1,
        "savedHeadingOffset": "0x1bc",
        "currentHeadingOffset": "0x70",
        "classVa": "0x007a4e70",
        "classes": [
            {"upTo": "0x1c71c7", "class": -1},
            {"upTo": "0x4e38e3", "class": -2},
            {"upTo": "0x800000", "class": -3},
            {"upTo": "0xb1c71c", "class": 3},
            {"upTo": "0xe38e39", "class": 2},
            {"upTo": "0xffffff", "class": 1},
        ],
        "beginVa": "0x007a4cd0",
        "timer": {
            "offset": "0x38",
            "minimum": 10.0,
            "sourceOffset": "0x258",
            "sourceMask": "0x3f",
            "extraOffset": "0x1c4",
            "scaleOffset": "0xf8",
            "updateVa": "0x007a4b60",
            "updateCall": "0x00667f88",
        },
        "event": {
            "code": "0x4a",
            "postVa": "0x0065bef0",
            "sites": ["0x007a4c20", "0x007b0547", "0x007b06f6"],
            "queueVa": "0x007ce7b0",
            "codeBias": 3,
            "ringOffset": "0x17928",
            "drainVa": "0x004daa10",
            "deliverySlot": 8,
            "handlerVa": "0x0058e0c0",
            "caseVa": "0x0058e373",
            "clearMask": "0xfd",
            "byteOffset": "0x26d",
        },
        "bitSetSites": ["0x00585ec0", "0x00587df2"],
        "bitClearSites": ["0x0058e373"],
    },
    "visibility": {
        "hiddenByteOffset": "0xe8",
        "hiddenBit": 1,
        "predicateVa": "0x0061ff40",
        "notifyForwardVa": "0x00a5fa10",
        "notifyForwardOffset": "0x0c",
        "notifySlotOffset": "0x250",
        "notifyVa": "0x00712b40",
    },
    "floatingOffset": {
        "record": "0x26",
        "command": "0x3b",
        "caseVa": "0x00663074",
        "handlerVa": "0x0065cc60",
        "storeVa": "0x007a42b0",
        "objectOffset": "0x590",
        "targetOffset": "0x588",
        "currentOffset": "0x58c",
        "payloadOffset": "0x04",
        "getterVa": "0x007a42f0",
        "easingVa": "0x007a7280",
        "easingState": 0,
        "easingRate": 0.02,
        "easingCap": 0.02,
        "placementVa": "0x00660f70",
        "placementAdd": "0x00661053",
    },
    "marker": {
        "numberOffset": "0x270",
        "numberMask": "0x3f",
        "initialNumber": "0x3f",
        "classShift": 6,
        "classMask": "0x1f",
        "numberGetterVa": "0x00585300",
        "classGetterVa": "0x00585310",
        "numberSetterVa": "0x00586720",
        "readers": {
            "0x00585300": ["0x006808bc", "0x006a2ea9"],
            "0x00585310": ["0x006809e9"],
        },
        "entityVa": "0x00680810",
        "entityGateByte": "0x26d",
        "entityGateBit": 2,
        "entityMax": "0x40",
        "entityNone": "0x3f",
        "stringRecords": {
            "base": "0x3b0",
            "stride": "0x54",
            "count": 2,
            "stringOffset": "0x34",
            "stringWidth": "0x20",
        },
        "stringOffsets": ["0x3e4", "0x438"],
        "clearVa": "0x005881c0",
        "copyVa": "0x00588490",
        "applyVa": "0x0076d920",
        "pendingOffset": "0x128",
        "readyOffset": "0x5c",
        "releaseVa": "0x007218e0",
        "baseSlot12Va": "0x006e18b0",
        "npcSlot12Va": "0x006f8660",
        "slot12Offset": "0x30",
        "vtables": {
            "CharaBase": "0x00fd5cac",
            "NpcBase": "0x00fd647c",
            "PlayerBase": "0x00fd5e04",
            "MyPlayer": "0x00fd785c",
        },
    },
    "targetFields": {
        "elementTargetOffset": "0x12c",
        "elementTargetInitial": "0xc0000000",
        "npcTargetOffset": "0x154",
        "npcTargetInitial": 0,
        "npcConstructorVa": "0x006f3650",
        "npcGetterVa": "0x0072d000",
        "npcGetterSlot": 40,
    },
}
REQUIRED_CHECKS = {
    "controller id slot writer",
    "controller slot state machine",
    "server look-at position entry",
    "look-at resend trigger",
    "slot 9 stricter route dispatch",
    "record 0x0e case",
    "look-at record initializer",
    "appearance post sets bit 6",
    "scene pump dispatches commands outside 3 to 6",
    "controller position setter wrapper",
    "command 0x11 hides",
    "controller target resolver mode tests",
    "look-at arbiter selection and state dispatch",
    "event drain delivery",
    "element event dispatch range",
    "CharaActor slot 43",
    "CharaActor slot 142",
    "SceneModule vtable slot 3",
    "PlayerBase vtable slot 12",
    "floating offset getter",
    "controller bone t0",
    "visibility predicate",
    "event 0x55 post",
    "server character entry user",
    "NpcBase vtable slot 12",
    "event 0x55 sets bit 2",
    "slot 9 stricter route jump table",
    "target initial value",
    "constraint set weight",
    "actor event queue",
    "controller position slot writer",
    "native look-at position entry",
    "visibility notification slot",
    "controller group 0 id setter",
    "CharaActor slot 154",
    "LookAtIKObject type code",
    "marker number getter",
    "server look-at named entry",
    "look-at named worker",
    "marker number setter",
    "easing step cap float",
    "look-at arbiter kind 1 and 4 payloads",
    "server look-at angle entry",
    "controller bone name binding loop",
    "controller bone k0",
    "CharaActor busy bit",
    "LookAtIKObject vtable slot 2",
    "controller flag toggle wrapper",
    "LookAtIKObject step",
    "controller bone s0",
    "controller default node",
    "controller id setter wrapper",
    "element slot 6 call order",
    "turn minimum seconds double",
    "record 0x0c case",
    "look-at angle worker",
    "turn begin rotation",
    "look-at arbiter kind 2 and 3 payloads",
    "base slot 12 returns the ready byte",
    "CharaActor record handler gate and table dispatch",
    "slot 9 full route jump table",
    "turn kind table",
    "element event 0x4a case",
    "turn full circle float",
    "constraint enable call",
    "turn start",
    "opcode 0x00DC case",
    "native look-at character entry",
    "look-at kind 2 helper without a caller",
    "container command table 0x0e to 0x14",
    "controller group 1 setter wrapper",
    "container dispatch sends commands 0x15 to 0x115 to actor slot 157",
    "marker record update",
    "marker class use",
    "ConstraintDirectionIKObject type code",
    "MyPlayer vtable slot 12",
    "constraint weight call",
    "container dispatch command table jump",
    "controller bone name pointers",
    "look-at arbiter kind 0 payload",
    "handle initializer",
    "marker string copy",
    "element post builds command and calls scene slot 3",
    "turn back handler",
    "IK active minimum float",
    "LookAtIKObject vtable slot 12",
    "CharaActor record case byte table",
    "CharaActor slot 157",
    "marker entity flag test",
    "floating offset handler",
    "pending marker apply",
    "record 0x32 case",
    "turn units per circle double",
    "turn call from actor update",
    "scene pump forwards a record to the container dispatcher",
    "slot 9 full route dispatch",
    "ground placement adds the offset",
    "opcode 0x00DD case",
    "floating offset easing",
    "map entity marker use",
    "set heading",
    "native look-at named entry",
    "LookAtIKObject slot 2 installs the step",
    "element event 0x55 entry",
    "marker record clear",
    "native look-at angle entry",
    "turn angle class",
    "opcode 0x00DE case",
    "LookAtIKObject vtable slot 5",
    "command 0x10 shows",
    "CharaBase vtable slot 12",
    "look-at position worker",
    "turn timer update",
    "element event table first and 0x55 entries",
    "NpcBase constructor target field",
    "slot 9 full route index table",
    "marker class getter",
    "visibility setter",
    "opcode 0x00DB case",
    "opcode 0x00DF case",
    "controller target resolver id mode",
    "controller own node",
    "turn circle per unit double",
    "look-at record writer",
    "turn call wrapper",
    "floating offset record case",
    "container dispatch gate",
    "NpcBase target getter",
    "controller flag 0x10 toggle",
    "TwoJointIKObject type code",
    "look-at resend clears bit 0",
    "record 0x33 case",
    "look-at cancel worker",
    "constraint position call",
    "look-at character worker",
    "turn completion event post",
    "controller angle slot writer",
    "look-at record array construction",
    "record 0x30 case",
    "pending marker release",
    "opcode 0x00D3 case",
    "server look-at character entry",
    "look-at arbiter kind table",
    "look-at arbiter bit 0 bookkeeping",
    "record 0x0b case",
    "constraint set position",
    "look-at target worker",
    "turn minimum seconds float",
    "controller group 1 id setter",
    "marker apply sets number",
    "record 0x0a case",
    "easing rate double",
    "CharaActor record jump table",
    "SceneModule slot 3 queues the packet",
    "marker distance test",
    "slot 9 stricter route index table",
    "record 0x0d case",
    "floating offset store",
    "element constructor stores scene pointer and key",
    "pending marker base call",
    "marker string copy entry",
}
# EXPECTED-END


# ---- models of the traced routines


def parse_hex(text: str) -> int:
    return int(text, 16)


def hex8(value: int) -> str:
    return f"0x{value:08x}"


def scene_command(document: dict, record: int) -> int:
    """The command that 0x004D7980 gives a record number."""
    return record + parse_hex(document["scenePath"]["commandOffset"])


def container_route(document: dict, command: int) -> tuple[str, int | str | None]:
    """Where the scene thread sends a command: pump, gate drop, table entry or actor record."""
    path = document["scenePath"]
    low, high = path["pumpOwnCommands"]
    if low <= command <= high:
        return ("pump", None)
    gate_low, gate_high = (parse_hex(v) for v in path["gateRange"])
    if not gate_low <= command <= gate_high:
        return ("dropped", None)
    actor_low, actor_high = (parse_hex(v) for v in path["actorCommandRange"])
    if actor_low <= command <= actor_high:
        return ("actor", command - actor_low)
    first = parse_hex(path["commandTableFirst"])
    table = path["commandTable"]
    if 0 <= command - first < len(table):
        return ("table", table[command - first])
    return ("container", None)


def actor_case(document: dict, record: int) -> str:
    """The case address of CharaActor slot 157 for a record number."""
    actor = document["charaActor"]
    if record > parse_hex(actor["recordLimit"]):
        return actor["ignoredCase"]
    return actor["jumpTable"][bytes.fromhex(actor["byteTable"])[record]]


def feed_case(document: dict, opcode: int, route: str = "fullRoute") -> str:
    """The case address a slot 9 route reaches for an opcode."""
    table = document["serverFeed"][route]
    index = opcode - parse_hex(table["firstOpcode"])
    indexes = bytes.fromhex(table["indexTable"])
    if not 0 <= index < len(indexes):
        return table["defaultVa" if route == "fullRoute" else "exitVa"]
    return table["jumpTable"][indexes[index]]


def kind_of(flags: int) -> int:
    return (flags >> 1) & 7


def new_record(document: dict) -> dict:
    """Record after 0x0058F130 on zeroed memory (it keeps flag bits 5 to 15)."""
    initial = document["lookAt"]["initial"]
    return {
        "flags": parse_hex(initial["flags"]),
        "priority": initial["priority"],
        "decay": initial["decay"],
        "target": parse_hex(initial["target"]),
        "weight": initial["weight"],
        "name": None,
        "vector": (0.0, 0.0, 0.0),
    }


def write_record(
    record: dict,
    priority: int,
    target: int,
    weight: float = 1.0,
    name: bytes | None = None,
    vector: tuple[float, float, float] = (0.0, 0.0, 0.0),
    kind: int = 1,
    own_key: int | None = None,
) -> bool:
    """The record writer 0x00585F60 and the kind setters of 0x00588020 and 0x005880F0."""
    if record["priority"] > priority:
        return False
    flags = record["flags"] & 0xFFFE
    if kind == 4:
        flags = (flags & 0xFFF9) | 8
    else:
        flags = (flags & 0xFFF1) | (kind << 1)
    record["flags"] = flags | 0x10
    record["priority"] = priority
    record["decay"] = 0
    record["target"] = 0xC0000000 if target == own_key else target
    record["weight"] = weight
    record["name"] = name
    record["vector"] = vector
    return True


def cancel_record(record: dict) -> None:
    """The cancel worker 0x00587EF0 for one record."""
    record["flags"] = (record["flags"] & 0xFFFA) | 0x1A
    record["priority"] = 0
    record["decay"] = 0
    record["target"] = 0xC0000000
    record["weight"] = 1.0


def arbitrate_bank(document: dict, bank: int, records: list[dict]) -> dict | None:
    """One bank of the arbiter 0x005861B0. Returns the posted message or None."""
    best_slot, best = 3, 0
    for slot, record in enumerate(records):
        if record["priority"] > best:
            best, best_slot = record["priority"], slot
        if record["priority"] > 0:
            record["priority"] = max(0, record["priority"] - record["decay"])
    if best == 0:
        return None
    winner = records[best_slot]
    if winner["flags"] & 1:
        return None
    kind = kind_of(winner["flags"])
    message = None
    spec = document["lookAt"]["kinds"][kind]
    if spec["record"] is not None:
        named = spec.get("namedRecord") if winner["name"] is not None else None
        record_number = parse_hex(named or spec["record"])
        flag_bit = (winner["flags"] >> 4) & 1
        # payload byte 1 per record: 0x0B carries only the kind 4 bit
        byte1 = {
            0x0B: int(kind == 4),
            0x0C: flag_bit | (int(kind == 4) << 1),
            0x0D: flag_bit,
            0x0E: flag_bit,
        }.get(record_number)
        message = {
            "record": record_number,
            "size": spec["namedSize"] if named else spec["size"],
            "bank": bank,
            "byte1": byte1,
            "weight": winner["weight"],
            "target": winner["target"],
            "name": winner["name"],
            "vector": winner["vector"],
            "priority": parse_hex(spec["payloadPriority"])
            if "payloadPriority" in spec
            else None,
            "group": spec.get("group"),
        }
    for slot, record in enumerate(records):
        if slot == best_slot:
            record["flags"] |= 1
        else:
            record["flags"] &= 0xFFFE
    return message


def turn_class(document: dict, delta: int) -> int:
    """The class of 0x007A4E70 for a 24 bit heading difference."""
    delta &= parse_hex(document["turn"]["unitsPerCircle"]) - 1
    for entry in document["turn"]["classes"]:
        if delta <= parse_hex(entry["upTo"]):
            return entry["class"]
    raise ValueError("delta outside every class")


def turn_back_posts(document: dict, current: int, saved: int, has_saved: bool) -> bool:
    """Whether 0x007B0690 posts event 0x4A at once (True) or starts a turn (False)."""
    turn = document["turn"]
    if not has_saved:
        return True
    circle = parse_hex(turn["unitsPerCircle"])
    window = parse_hex(turn["windowUnits"])
    delta = (current - saved) & (circle - 1)
    return not window <= delta <= circle - window


def turn_timer_start(document: dict, controller_dword: int, mode: int) -> float:
    """The value 0x007B0530 stores at controller +0x38."""
    timer = document["turn"]["timer"]
    value = float(controller_dword & parse_hex(timer["sourceMask"]))
    extra = 0.0 if mode == 2 else max(timer["minimum"], value)
    return extra if extra > timer["minimum"] else timer["minimum"]


def turn_timer_step(timer: float, scale: float, delta: float) -> tuple[float, bool]:
    """One call of 0x007A4B60: the new timer and whether it posts event 0x4A."""
    if timer <= 0.0:
        return (timer, False)
    remaining = timer - scale * delta
    if remaining <= 0.0:
        return (0.0, True)
    return (remaining, False)


def queued_code(document: dict, event: int) -> int:
    return event + document["turn"]["event"]["codeBias"]


def delivered_event(document: dict, packet_code: int) -> int | None:
    """The element event that 0x004DAA10 delivers for a queued code."""
    bias = document["turn"]["event"]["codeBias"]
    if packet_code == 2 or not 0 <= packet_code - bias <= 0x100:
        return None
    return packet_code - bias


def hidden_byte(document: dict, value: int, visible: bool) -> int:
    bit = 1 << document["visibility"]["hiddenBit"]
    return (value & ~bit & 0xFF) if visible else (value | bit)


def is_visible(value: int) -> bool:
    return value == 0


def easing_next(document: dict, current: float, target: float, delta: float) -> float:
    """One state 0 call of 0x007A7280."""
    floating = document["floatingOffset"]
    if not delta > 0.0:
        return current
    step = (target - current) * floating["easingRate"] * delta
    return current + min(step, floating["easingCap"])


def marker_set(document: dict, value: int, number: int) -> int:
    mask = parse_hex(document["marker"]["numberMask"])
    return value ^ ((value ^ number) & mask)


def marker_number(document: dict, value: int) -> int:
    return value & parse_hex(document["marker"]["numberMask"])


def marker_entity_value(
    document: dict, is_local: bool, ready: bool, number: int
) -> int:
    """The marker value 0x00680810 stores for a chara."""
    if is_local:
        return 0
    if not ready or number > parse_hex(document["marker"]["entityMax"]) or number == 0:
        return -1
    return number


def slot_flags_after_write(
    document: dict, previous: int, setter: str, has_target: bool, flag: bool
) -> int:
    """The flag dword after a controller slot writer."""
    controller = document["controller"]
    flags = (previous & 0xFFFC0037) | parse_hex(controller["setterAlways"])
    if has_target:
        flags |= parse_hex(controller["setterTargetPresent"])
    if flag:
        flags |= parse_hex(controller["setterFlag"])
    for setter_row in controller["setters"]:
        if setter_row["va"] == setter:
            flags |= parse_hex(setter_row["flag"])
    return flags


def ik_active(document: dict, weight: float) -> bool:
    """Whether the LookAtIKObject step 0x00AED730 runs its solve step."""
    return document["controller"]["ikActiveMinimum"] <= weight


def slot_arms(flags: int) -> bool:
    """State 1 to 2 of 0x00854600."""
    shifted = flags >> 3
    return (shifted & 2) != 2 and (shifted & 0xC001) == 0xC001


def slot_disarms(flags: int) -> bool:
    """State 3 to 0 of 0x00854600."""
    shifted = flags >> 3
    return (shifted & 2) == 2 or (shifted & 0x8001) != 0x8001 or not shifted & 0x4000


def slot_refreshes(flags: int) -> bool:
    """State 3 refresh of 0x00854600."""
    return bool((flags >> 3) & 0xC)


# ---- manifest checks


def lookup(document: dict, path: tuple) -> object:
    value: object = document
    for key in path:
        value = value[key]  # type: ignore[index]
    return value


def check_row(document: dict, name: str) -> dict:
    for item in document.get("staticChecks", []):
        if item.get("name") == name:
            return item
    return {}


def check_bytes(document: dict, name: str) -> bytes:
    pattern = check_row(document, name).get("pattern", "")
    try:
        return bytes.fromhex(pattern)
    except ValueError:
        return b""


def check_values(document: dict, name: str) -> list[str]:
    return list(check_row(document, name).get("values", []))


def diff_paths(found: object, wanted: object, prefix: str = "") -> list[str]:
    if isinstance(wanted, dict) and isinstance(found, dict):
        out = []
        for key in sorted(set(wanted) | set(found)):
            if key not in found or key not in wanted:
                out.append(f"{prefix}{key}")
            else:
                out += diff_paths(found[key], wanted[key], f"{prefix}{key}.")
        return out
    if isinstance(wanted, list) and isinstance(found, list):
        if len(wanted) != len(found):
            return [prefix.rstrip(".")]
        out = []
        for index, (left, right) in enumerate(zip(found, wanted)):
            out += diff_paths(left, right, f"{prefix}{index}.")
        return out
    return (
        [] if found == wanted and type(found) is type(wanted) else [prefix.rstrip(".")]
    )


def strip_tables(section: object) -> object:
    if isinstance(section, dict):
        return {k: strip_tables(v) for k, v in section.items() if k not in TABLE_KEYS}
    if isinstance(section, list):
        return [strip_tables(v) for v in section]
    return section


def verify_tables(document: dict) -> list[str]:
    errors: list[str] = []
    actor = document.get("charaActor", {})
    byte_table = check_bytes(document, "CharaActor record case byte table")
    if bytes.fromhex(actor.get("byteTable", "00")) != byte_table:
        errors.append("record case byte table differs from its static check")
    if actor.get("jumpTable") != check_values(document, "CharaActor record jump table"):
        errors.append("record jump table differs from its static check")
    feed = document.get("serverFeed", {})
    full = feed.get("fullRoute", {})
    if bytes.fromhex(full.get("indexTable", "00")) != check_bytes(
        document, "slot 9 full route index table"
    ):
        errors.append("full route index table differs from its static check")
    if full.get("jumpTable") != check_values(document, "slot 9 full route jump table"):
        errors.append("full route jump table differs from its static check")
    strict = feed.get("stricterRoute", {})
    if bytes.fromhex(strict.get("indexTable", "00")) != check_bytes(
        document, "slot 9 stricter route index table"
    ):
        errors.append("stricter route index table differs from its static check")
    if strict.get("jumpTable") != check_values(
        document, "slot 9 stricter route jump table"
    ):
        errors.append("stricter route jump table differs from its static check")
    path = document.get("scenePath", {})
    if path.get("commandTable") != check_values(
        document, "container command table 0x0e to 0x14"
    ):
        errors.append("command table differs from its static check")
    if check_values(document, "SceneModule vtable slot 3") != [
        path.get("sceneModulePostVa")
    ]:
        errors.append("SceneModule slot 3 differs from its static check")
    slots = {
        "CharaActor slot 157": actor.get("recordHandlerVa"),
        "CharaActor slot 154": actor.get("busyVa"),
        "CharaActor slot 142": actor.get("updateVa"),
        "CharaActor slot 43": actor.get("visibleVa"),
    }
    for name, value in slots.items():
        if check_values(document, name) != [value]:
            errors.append(f"{name} differs from its static check")
    kinds = document.get("lookAt", {}).get("kindTargets")
    if kinds != check_values(document, "look-at arbiter kind table"):
        errors.append("arbiter kind table differs from its static check")
    controller = document.get("controller", {})
    if check_values(document, "controller bone name pointers") != [
        "0x010427ec",
        "0x010427e8",
        "0x010427e4",
    ]:
        errors.append("bone name pointers changed")
    minimum = struct.pack("<f", controller.get("ikActiveMinimum", 0.0))
    if check_values(document, "IK active minimum float") != [
        hex8(int.from_bytes(minimum, "little"))
    ]:
        errors.append("IK active minimum differs from its static check")
    if check_values(document, "LookAtIKObject vtable slot 2") != [
        controller.get("ikStepInstallVa")
    ]:
        errors.append("LookAtIKObject slot 2 differs from its static check")
    names = [string for string in controller.get("boneNames", [])]
    texts = [check_row(document, f"controller bone {n}").get("text") for n in names]
    if texts != names:
        errors.append("bone names differ from their string checks")
    for key, name in (
        ("defaultNode", "controller default node"),
        ("ownNode", "controller own node"),
    ):
        if check_row(document, name).get("text") != controller.get(key):
            errors.append(f"{key} differs from its string check")
    return errors


def verify_models(document: dict) -> list[str]:
    errors: list[str] = []
    path = document["scenePath"]
    for record, command in ((0x33, 0x48), (0x26, 0x3B), (0x0A, 0x1F), (0x0E, 0x23)):
        if scene_command(document, record) != command:
            errors.append(f"record {record:#x} does not map to command {command:#x}")
        if container_route(document, command) != ("actor", record):
            errors.append(
                f"command {command:#x} does not reach actor record {record:#x}"
            )
    show = container_route(document, parse_hex(path["showCommand"]))
    hide = container_route(document, parse_hex(path["hideCommand"]))
    if show != ("table", "0x007c9696") or hide != ("table", "0x007c96b4"):
        errors.append("visibility commands do not reach their table entries")
    gate = [container_route(document, c)[0] for c in (4, 7, 0x117)]
    if gate != ["pump", "dropped", "dropped"]:
        errors.append("command gate changed")
    for row in document["charaActor"]["records"]:
        if actor_case(document, parse_hex(row["record"])) != row["caseVa"]:
            errors.append(f"record {row['record']} case disagrees with the tables")
    if actor_case(document, 0x64) != document["charaActor"]["ignoredCase"]:
        errors.append("records above the limit are not ignored")
    feed = document["serverFeed"]
    for case in feed["cases"]:
        opcode = parse_hex(case["opcode"])
        if feed_case(document, opcode) != case["caseVa"]:
            errors.append(f"opcode {case['opcode']} case disagrees with the full route")
        if (
            feed_case(document, opcode, "stricterRoute")
            != feed["stricterRoute"]["exitVa"]
        ):
            errors.append(
                f"opcode {case['opcode']} is not dropped by the stricter route"
            )
        if case["slot"] != 1:
            errors.append(f"opcode {case['opcode']} does not write slot 1")
    look = document["lookAt"]
    for index, kind in enumerate(look["kinds"]):
        if kind["kind"] != index:
            errors.append("kinds are out of order")
    if look["kindTargets"][1] != look["kindTargets"][4]:
        errors.append("kinds 1 and 4 do not share a payload builder")
    for kind in look["kinds"]:
        for key in ("record", "namedRecord"):
            record = kind.get(key)
            if record is None:
                continue
            fields = look["payloads"].get(record)
            size = kind["size"] if key == "record" else kind["namedSize"]
            if not fields:
                errors.append(f"record {record} has no payload layout")
                continue
            end = max(parse_hex(f["offset"]) + f["width"] for f in fields)
            if end > size:
                errors.append(f"record {record} layout exceeds its size")
    turn = document["turn"]
    previous = -1
    for entry in turn["classes"]:
        upper = parse_hex(entry["upTo"])
        if upper <= previous:
            errors.append("angle classes are not increasing")
        previous = upper
    if previous != parse_hex(turn["unitsPerCircle"]) - 1:
        errors.append("angle classes do not cover the circle")
    if turn["event"]["sites"] != scan_sites(document, turn["event"]["postVa"]):
        errors.append("event sites differ from the call scan")
    if turn["bitSetSites"] != pattern_scan(document, "or byte ptr [reg+0x26d], 2"):
        errors.append("turn bit set sites differ from the pattern scan")
    if turn["bitClearSites"] != pattern_scan(
        document, "and byte ptr [reg+0x26d], 0xfd"
    ):
        errors.append("turn bit clear sites differ from the pattern scan")
    marker = document["marker"]
    for target, sites in marker["readers"].items():
        if sites != scan_sites(document, target):
            errors.append(f"readers of {target} differ from the call scan")
    return errors


def scan_sites(document: dict, target: str) -> list[str]:
    for scan in document.get("scans", {}).get("callSites", []):
        if scan.get("target") == target:
            return scan.get("sites", [])
    return []


def pattern_scan(document: dict, name: str) -> list[str]:
    for scan in document.get("scans", {}).get("patternSites", []):
        if scan.get("name") == name:
            return scan.get("sites", [])
    return []


def verify_sections(document: dict) -> list[str]:
    errors: list[str] = []
    for name, wanted in EXPECTED_SECTIONS.items():
        found = strip_tables(document.get(name))
        for path in diff_paths(found, wanted):
            errors.append(f"{name}.{path} changed")
    try:
        errors += verify_tables(document)
        errors += verify_models(document)
    except (KeyError, IndexError, TypeError, ValueError) as exc:
        errors.append(f"manifest sections are malformed: {exc!r}")
    return errors


def heading_anchors(text: str) -> set[str]:
    anchors = set()
    for line in text.splitlines():
        match = re.match(r"^#{2,6}\s+(.*?)\s*$", line)
        if match:
            slug = re.sub(r"[^a-z0-9 -]", "", match.group(1).lower())
            anchors.add(slug.replace(" ", "-"))
    return anchors


def verify_page(document: dict, page: str, text: str | None = None) -> list[str]:
    errors: list[str] = []
    if text is None:
        text = PAGES[page].read_text(encoding="utf-8")
    rows = []
    for line in text.splitlines():
        match = re.match(
            r"^\| (.+) \| (STATIC|INFERRED|UNPROVED|NOT FOUND) \| \[(.+)\]\(#(.+)\) \|$",
            line,
        )
        if match:
            rows.append((match.group(1), match.group(2), match.group(4)))
    claims = [
        (c.get("claim"), c.get("tier"), c.get("section"))
        for c in document.get("claims", [])
        if isinstance(c, dict) and c.get("page") == page
    ]
    if rows != claims:
        errors.append(f"{page}: the verdict table disagrees with the manifest claims")
    anchors = heading_anchors(text)
    for section in SECTIONS[page]:
        if section not in anchors:
            errors.append(f"{page}: the page has no section {section!r}")
    consumer = document.get("consumer", {}).get(page, {})
    for key in ("implementNow", "blocked"):
        for item in consumer.get(key, []):
            if f"- {item}" not in text.splitlines():
                errors.append(f"{page}: the page lacks consumer item {item[:40]!r}")
    for item in document.get("boundaries", {}).get(page, []):
        if f"- {item}" not in text.splitlines():
            errors.append(f"{page}: the page lacks open question {item[:40]!r}")
    if "\t" in text or any(ord(ch) > 127 for ch in text):
        errors.append(f"{page}: the page has non-ASCII or tab characters")
    return errors


def verify(
    document: dict | None = None, texts: dict[str, str] | None = None
) -> list[str]:
    errors: list[str] = []
    check_pages = document is None
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-chara-presentation-consumers-v1":
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
    errors += verify_sections(document)
    claims = document.get("claims")
    if not isinstance(claims, list):
        errors.append("claims is not a list")
    else:
        found = {c.get("id"): c.get("tier") for c in claims if isinstance(c, dict)}
        if len(found) != len(claims) or found != EXPECTED_CLAIMS:
            errors.append("claim ids or tiers changed")
        if any(not str(c.get("claim", "")).strip() for c in claims):
            errors.append("a claim has no text")
        if any(c.get("tier") not in TIERS for c in claims):
            errors.append("a claim has an unknown tier")
        for c in claims:
            if c.get("page") not in SECTIONS or c.get("section") not in SECTIONS.get(
                c.get("page"), []
            ):
                errors.append(f"claim {c.get('id')} has an unknown page or section")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
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
        elif not HEX_ADDRESS_RE.match(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            pattern, mask = item.get("pattern", ""), item.get("mask", "")
            try:
                same = len(bytes.fromhex(pattern)) == len(bytes.fromhex(mask))
            except ValueError:
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
    consumer = document.get("consumer", {})
    for page in SECTIONS:
        lists = consumer.get(page, {})
        if not lists.get("implementNow") or not lists.get("blocked"):
            errors.append(f"consumer lists of {page} are empty")
    dump = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, dump, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    if check_pages or texts is not None:
        for page in SECTIONS:
            text = texts.get(page) if texts else None
            errors += verify_page(document, page, text)
    return errors


MUTATIONS = [
    (("source", "binarySha256"), "0" * 64),
    (("scenePath", "commandOffset"), "0x16"),
    (("scenePath", "commandTable", 2), "0x007c9697"),
    (("scenePath", "actorSlotOffset"), "0x270"),
    (("scenePath", "gateRange", 1), "0x0115"),
    (("charaActor", "recordSlot"), 156),
    (("charaActor", "records", 0, "caseVa"), "0x006633cd"),
    (("charaActor", "byteTable"), "ff" * 100),
    (("charaActor", "busyBit"), 5),
    (("lookAt", "initial", "flags"), "0x0000"),
    (("lookAt", "initial", "priority"), 1),
    (("lookAt", "kinds", 2, "record"), "0x0e"),
    (("lookAt", "kinds", 1, "payloadPriority"), "0xc8"),
    (("lookAt", "kindTargets", 3), "0x00586270"),
    (("lookAt", "writers", "server", "priority"), "0xc8"),
    (("lookAt", "writers", "native", "slot"), 1),
    (("lookAt", "resend", "bits", 0), 5),
    (("serverFeed", "cases", 1, "slot"), 0),
    (("serverFeed", "cases", 1, "caseVa"), "0x0058d1a2"),
    (("serverFeed", "fullRoute", "indexTable"), "00" * 215),
    (("serverFeed", "stricterRoute", "jumpTable", 0), "0x0058d1a1"),
    (("serverFeed", "nullTargetMask"), "0xc0000000"),
    (("controller", "setters", 0, "flag"), "0x100"),
    (("controller", "ikTypes", "LookAtIKObject"), 3),
    (("controller", "ikWeightOffsets", "2"), "0xd8"),
    (("controller", "ikActiveMinimum"), 0.0001),
    (("controller", "ikStepVa"), "0x00aed734"),
    (("controller", "boneNames", 0), "t0"),
    (("controller", "defaultNode"), "EID_LOOK_AT1"),
    (("turn", "classes", 0, "upTo"), "0x1c71c8"),
    (("turn", "windowUnits"), "0x38e3a"),
    (("turn", "timer", "minimum"), 9.0),
    (("turn", "event", "sites", 0), "0x007a4c21"),
    (("turn", "event", "codeBias"), 2),
    (("turn", "bitClearSites", 0), "0x0058e374"),
    (("visibility", "hiddenBit"), 0),
    (("floatingOffset", "easingCap"), 0.03),
    (("floatingOffset", "easingRate"), 0.2),
    (("marker", "numberMask"), "0x7f"),
    (("marker", "entityNone"), "0x40"),
    (("marker", "readers", "0x00585300", 0), "0x006808bd"),
    (("marker", "stringOffsets", 1), "0x43c"),
    (("targetFields", "npcTargetInitial"), 1),
    (("targetFields", "elementTargetInitial"), "0x00000000"),
]


def mutate(document: dict, path: tuple, value: object) -> dict:
    result = copy.deepcopy(document)
    target = result
    for key in path[:-1]:
        target = target[key]
    target[path[-1]] = value
    return result


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations = [mutate(document, path, value) for path, value in MUTATIONS]
    tier_flip = copy.deepcopy(document)
    tier_flip["claims"][0]["tier"] = "INFERRED"
    mutations.append(tier_flip)
    dropped = copy.deepcopy(document)
    dropped["claims"].pop()
    mutations.append(dropped)
    no_check = copy.deepcopy(document)
    no_check["staticChecks"].pop()
    mutations.append(no_check)
    scan = copy.deepcopy(document)
    row = next(r for r in scan["scans"]["callSites"] if r["target"] == "0x0065bef0")
    row["sites"] = []
    mutations.append(scan)
    empty = copy.deepcopy(document)
    empty["consumer"][LOOK_AT]["implementNow"] = []
    mutations.append(empty)
    leaked = copy.deepcopy(document)
    leaked["source"]["method"] += " /" + "Users/someone/ffxivgame.exe"
    mutations.append(leaked)
    errors = [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify_sections_only(item)
    ]
    texts = {page: PAGES[page].read_text(encoding="utf-8") for page in PAGES}
    page_cases = []
    for page in PAGES:
        text = texts[page]
        lines = text.splitlines()
        rows = [i for i, line in enumerate(lines) if "| STATIC |" in line]
        flipped = list(lines)
        flipped[rows[0]] = flipped[rows[0]].replace("| STATIC |", "| INFERRED |")
        page_cases.append((page, "\n".join(flipped) + "\n"))
        heading = "## " + SECTIONS[page][0].replace("-", " ").capitalize()
        page_cases.append((page, text.replace(heading, heading + " renamed", 1)))
        items = document["consumer"][page]["implementNow"]
        page_cases.append((page, text.replace(f"- {items[0]}\n", "", 1)))
        page_cases.append((page, text + "\u2014\n"))
    errors += [
        f"page mutation {index} was accepted"
        for index, (page, item) in enumerate(page_cases, 1)
        if not verify_page(document, page, item)
    ]
    return errors


def verify_sections_only(document: dict) -> list[str]:
    return verify(document, None)


# ---- executable checks


def call_sites(data: bytes, targets: set[int]) -> dict[int, list[int]]:
    """VAs of E8 and E9 opcodes in the code section whose rel32 reaches a target."""
    found: dict[int, list[int]] = {target: [] for target in targets}
    for start, virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        if not (start <= CODE_START < start + max(virtual_size, raw_size)):
            continue
        end = raw_offset + raw_size - 4
        for match in re.finditer(rb"(?=[\xe8\xe9])", data[raw_offset:end], re.S):
            at = raw_offset + match.start()
            rel = int.from_bytes(data[at + 1 : at + 5], "little", signed=True)
            site = start + at - raw_offset
            target = (site + 5 + rel) & 0xFFFFFFFF
            if target in found:
                found[target].append(site)
    return {key: sorted(set(value)) for key, value in found.items()}


def pattern_sites(data: bytes, regex: str) -> list[int]:
    """VAs in the code section where a byte regex matches."""
    found = []
    for start, virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        if not (start <= CODE_START < start + max(virtual_size, raw_size)):
            continue
        blob = data[raw_offset : raw_offset + raw_size]
        for match in re.finditer(regex.encode("latin-1"), blob, re.S):
            found.append(start + match.start())
    return sorted(found)


def dword_sites(data: bytes, value: int) -> list[int]:
    """VAs of a little-endian dword in the raw bytes of any section."""
    needle = value.to_bytes(4, "little")
    found = []
    for start, _virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        at = data.find(needle, raw_offset, raw_offset + raw_size)
        while at != -1:
            found.append(start + at - raw_offset)
            at = data.find(needle, at + 1, raw_offset + raw_size)
    return sorted(found)


def scan_errors(data: bytes, document: dict) -> list[str]:
    errors: list[str] = []
    scans = document["scans"]
    wanted = {int(s["target"], 16): s["sites"] for s in scans["callSites"]}
    found = call_sites(data, set(wanted))
    for target, sites in wanted.items():
        if [hex8(s) for s in found[target]] != sites:
            errors.append(f"call sites of {hex8(target)} differ from the scan")
    for scan in scans["patternSites"]:
        got = pattern_sites(data, scan["regex"])
        if [hex8(s) for s in got] != scan["sites"]:
            errors.append(f"pattern scan {scan['name']!r} differs")
    for scan in scans["dwordSites"]:
        got = dword_sites(data, int(scan["value"], 16))
        if [hex8(s) for s in got] != scan["sites"]:
            errors.append(f"dword scan of {scan['value']} differs")
    return errors


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    errors: list[str] = []
    if require_identity:
        if len(data) != EXPECTED_SIZE:
            errors.append(f"executable size is {len(data)}, expected {EXPECTED_SIZE}")
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXPECTED_SHA256:
            errors.append(f"executable SHA-256 is {digest}, expected {EXPECTED_SHA256}")
    try:
        sections = image_checks.section_map(data)
    except (ValueError, struct.error) as exc:
        return errors + [f"image is not a readable PE32: {exc}"]
    for check in document.get("staticChecks", []):
        problem = image_checks.run_static_check(data, sections, check)
        if problem:
            errors.append(problem)
    if "scans" in document:
        errors += scan_errors(data, document)
    return errors


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if errors:
        print(f"FAIL: {len(errors)} chara presentation consumer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: chara presentation consumer manifest, pages, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
