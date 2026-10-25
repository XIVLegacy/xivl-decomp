#!/usr/bin/env python3
"""Validate the combat result element contract, optional exe bytes and the call scans."""

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
MANIFEST = ROOT / "config" / "combat_result_element.json"
PAGE = ROOT / "docs" / "net" / "combat-result-element.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
CODE_START = 0x00401000
HEX_ADDRESS_RE = re.compile(r"^0x[0-9a-f]{8}$")
TIERS = {"STATIC", "INFERRED", "UNPROVED", "NOT FOUND"}
SECTIONS = {
    "element-resolution",
    "admission",
    "opcode-routes",
    "staging",
    "classifier",
    "drain",
    "header-line",
    "row-trigger",
    "row-processing",
    "damage-plate",
}
EXPECTED_CLAIMS = {
    "dispatcher-gate": "STATIC",
    "dispatcher-range": "STATIC",
    "element-key": "STATIC",
    "lookup-miss": "STATIC",
    "element-slot": "STATIC",
    "admission-predicate": "STATIC",
    "admission-drop": "STATIC",
    "opcode-routes": "STATIC",
    "wrapper-arguments": "STATIC",
    "stager-forms": "STATIC",
    "serial-publication": "STATIC",
    "record-fields": "STATIC",
    "row-fields": "STATIC",
    "count-unclamped": "STATIC",
    "stage-request": "STATIC",
    "stage-request-effect": "UNPROVED",
    "classifier": "STATIC",
    "category-table": "STATIC",
    "ring-push": "STATIC",
    "drain-cadence": "STATIC",
    "class-buckets": "STATIC",
    "scene-ops": "STATIC",
    "header-dispatch": "STATIC",
    "range-gate": "STATIC",
    "header-line": "STATIC",
    "drain-skips-rows": "STATIC",
    "row-trigger": "STATIC",
    "row-trigger-sender": "NOT FOUND",
    "row-trigger-classes": "STATIC",
    "second-door": "STATIC",
    "local-builder": "STATIC",
    "row-order": "STATIC",
    "log-arguments": "STATIC",
    "special-text-ids": "STATIC",
    "special-route-meaning": "UNPROVED",
    "plate-gate": "STATIC",
    "plate-anchor": "STATIC",
    "plate-pool": "STATIC",
    "plate-widgets": "STATIC",
    "plate-digits": "STATIC",
    "plate-kinds": "STATIC",
    "plate-kind-visuals": "UNPROVED",
    "status-field": "NOT FOUND",
    "delay-constants": "NOT FOUND",
}
FIXED_VALUES = {
    ("route", "dispatcherVa"): "0x004dc690",
    ("route", "opcodeRange"): ["0x012e", "0x013d"],
    ("route", "sharedPathVa"): "0x004dcfff",
    ("route", "registryLookupVa"): "0x004d9910",
    ("route", "registryMapOffset"): "0x17804",
    ("route", "elementVtableVa"): "0x00fa7c50",
    ("route", "elementSlot"): 9,
    ("route", "elementSlotTargetVa"): "0x0058cca0",
    ("route", "elementDrainSlot"): 6,
    ("route", "elementDrainTargetVa"): "0x0058df90",
    ("route", "elementEventSlot"): 8,
    ("route", "elementEventTargetVa"): "0x0058e0c0",
    ("route", "keySource"): "sub-event header dword +4",
    ("route", "keyRemap"): {
        "mask": "0x5ff80000",
        "result": "0x5ff80002 minus (key & 1)",
    },
    ("route", "elementKeyOffset"): "0x88",
    ("route", "elementKeyGetterVa"): "0x004d6750",
    ("route", "missBehavior"): "no element, no staging",
    ("route", "preGate"): {
        "byteOffset": "0x54b",
        "predicateVa": "0x00575740",
        "objectOffset": "0x510",
        "onlyOpcode": "0x0004",
    },
    ("admission", "predicateVa"): "0x004d8830",
    ("admission", "callSite"): "0x0058cce5",
    ("admission", "rejectedStateByteTableVa"): "0x0058d8c4",
    ("admission", "rejectedStateIndexBase"): "0x0137",
    ("admission", "rejectedStatePassingAbove0134"): [
        "0x0137",
        "0x0144",
        "0x0145",
        "0x0197",
        "0x01a0",
    ],
    ("admission", "droppedWhenRejected"): ["0x0139", "0x013a", "0x013b", "0x013c"],
    ("wrapper", "classifierVa"): "0x00585800",
    ("wrapper", "classifierFallback"): 6,
    ("wrapper", "routeFlag"): 1,
    ("wrapper", "ringOffset"): "0xa80",
    ("wrapper", "ringPushVa"): "0x005901d0",
    ("wrapper", "payloadViewOffset"): "0x10",
    ("record", "size"): "0x1a0",
    ("record", "headerSize"): "0x38",
    ("record", "rowStride"): "0x14",
    ("record", "rowCapacity"): 18,
    ("record", "serialCounterVa"): "0x01338d2c",
    ("record", "serialEventCommand"): "0x39",
    ("record", "wireRowStrides"): {
        "target": 4,
        "numeric": 2,
        "textId": 2,
        "effect": 4,
        "textParam": 1,
        "ordinal": 1,
    },
    ("record", "stageRequest", "loggerGetterVa"): "0x004d7470",
    ("record", "stageRequest", "requestVa"): "0x00578390",
    ("record", "stageRequest", "skipsEffectTypes"): ["0x10", "0x14"],
    ("record", "stageRequest", "skipsEffectId"): "0x7c000062",
    ("classifier", "va"): "0x00585800",
    ("classifier", "laneSplitVa"): "0x00798370",
    ("classifier", "rowReadVa"): "0x007982d0",
    ("classifier", "tableVa"): "0x00fe32d8",
    ("classifier", "rowStride"): 12,
    ("classifier", "validRows"): 34,
    ("classifier", "indexBound"): "0x22",
    ("classifier", "typeOverrides"): {"0x7c": 7, "0x6f": 8, "0x70": 9, "0x71": 10},
    ("classifier", "kindToClass"): {"8": 11, "2": 14, "3": 15, "4": 16, "5": 17},
    ("classifier", "kind1"): {
        "classInside": 13,
        "classOutside": 12,
        "middleRange": ["0x00d", "0x013"],
        "middleExtra": "0xfe9",
    },
    ("drain", "slot"): 6,
    ("drain", "slotVa"): "0x0058df90",
    ("drain", "callSite"): "0x0058dfc8",
    ("drain", "drainVa"): "0x0058da10",
    ("drain", "recordsPerPass"): 1,
    ("drain", "mainRingOffset"): "0xa80",
    ("drain", "bucketRingBase"): "0x878",
    ("drain", "bucketRingStride"): "0x14",
    ("drain", "bucketCount"): 26,
    ("drain", "dropAbove"): 25,
    ("drain", "postSize"): "0x38 + 0x14 * rowCount",
    (
        "drain",
        "caseTargets",
    ): ["0x0058da7c", "0x0058da9f", "0x0058dabf", "0x0058dadf", "0x0058db42"],
    ("drain", "headerDispatchVa"): "0x0058ca80",
    ("drain", "headerFlags"): {"drain": [1, 0], "trigger": ["0", "routeFlag & 1"]},
    ("drain", "headerTargets"): {"5": "0x00585af0", "6..10": "0x0058c220"},
    ("drain", "headerPrecondition", "va"): "0x00588e70",
    ("drain", "headerPrecondition", "rangeVa"): "0x005875a0",
    ("drain", "headerPrecondition", "radiusOffset"): "0xaa8",
    ("drain", "headerPrecondition", "setterVa"): "0x00585930",
    ("drain", "headerPrecondition", "setterCallSites"): [
        "0x00585e04",
        "0x0058758c",
        "0x0058b297",
        "0x0058bb3d",
    ],
    ("drain", "headerPrecondition", "setterSpecialValue"): 50,
    ("drain", "headerPrecondition", "helperVa"): "0x00587580",
    ("drain", "headerPrecondition", "helperConstant"): 35.0,
    ("drain", "eventCodeOffset"): "0x15",
    ("drain", "busCallVa"): "0x004d7980",
    ("events", "rowTrigger"): {
        "command": "0x44",
        "code": "0x59",
        "handlerVa": "0x0058de90",
        "publisherVa": "0x005856c0",
        "publisherCaller": "0x005871ec",
        "publisherOuterVa": "0x00587130",
        "finishCommand": "0x43",
        "finishCode": "0x58",
        "finishHandlerVa": "0x0058dbe0",
        "otherCode": "0x5a",
        "otherHandlerVa": "0x0058bc50",
        "otherPublisherVa": "0x00585740",
        "otherPublisherCaller": "0x006aca52",
    },
    ("events", "stageDoor"): {
        "command": "0x4f",
        "code": "0x64",
        "routeFlag": 0,
        "classifierFallback": 6,
        "stagerVa": "0x00587370",
    },
    ("events", "localBuilder"): {
        "va": "0x006aace0",
        "wrappers": ["0x0058c880", "0x0058c930"],
        "numericConstant": 9999,
    },
    ("events", "serial"): {"command": "0x39", "code": "0x4e"},
    ("rowProcessing", "va"): "0x0058c220",
    ("rowProcessing", "arguments"): [
        "record",
        "headerOnly",
        "allOrdinals",
    ],
    ("rowProcessing", "logVa"): "0x005759a0",
    ("rowProcessing", "loggerContainerOffset"): "0x510",
    ("rowProcessing", "soundFilterVa"): "0x005859f0",
    ("rowProcessing", "widgets"): [
        "Window_PlayerProfileWidget",
        "Window_ChainBonusEffectWidget",
    ],
    ("rowProcessing", "headerLine", "condition"): "controlValue nonzero",
    ("rowProcessing", "headerLine", "textId"): "controlValue",
    ("rowProcessing", "headerLine", "otherArguments"): "zero",
    ("plates", "listBuilderVa"): "0x00795630",
    ("plates", "listOffset"): "0xd3c",
    ("plates", "anchorVa"): "0x00794ee0",
    ("plates", "gate", "overrideElementVa"): "0x004d7430",
    ("plates", "gate", "configOption"): [2, 5],
    ("plates", "gate", "globalPointer"): "0x01356e10",
    ("plates", "gate", "globalByte"): "0x01356e14",
    ("plates", "kindShift"): 27,
    ("plates", "kindMask"): 15,
    ("plates", "anchorSourceKinds"): [7, 8, 9, 10],
    ("plates", "poolSlots"): 5,
    ("plates", "slotObjectSize"): "0x48",
    ("plates", "digitBlocks"): 5,
    ("plates", "digitDivisor"): 10,
    ("plates", "digitMax"): 99999,
    ("plates", "formPath"): "\\system\\ingame\\DamagePlateWidget.form",
    ("plates", "widgets"): [
        "IconControl_Icon",
        "TextBlock_Additional",
        "TextBlock_DamageType",
    ],
    ("plates", "animations"): ["DamagePlateAnime.Popup", "MissPlateAnime.Popup"],
}
OPCODE_ROUTES = [
    (
        "0x0139",
        "0x0058d553",
        "0x0058c880",
        "0x00587370",
        "single",
        1,
        "none, one row is always staged (record count still holds the wire u16)",
        53,
    ),
    (
        "0x013a",
        "0x0058d565",
        "0x0058c930",
        "0x005873e0",
        "ten",
        10,
        "wire u16 only",
        179,
    ),
    (
        "0x013b",
        "0x0058d577",
        "0x0058c990",
        "0x005874b0",
        "eighteen",
        18,
        "wire u16 only",
        291,
    ),
    (
        "0x013c",
        "0x0058d541",
        "0x0058c7d0",
        "0x00587210",
        "none",
        0,
        "not bounded (record count still holds the wire u16)",
        39,
    ),
]
HEADER_FIELDS = [
    ("0x00", 4, "sequenceId", "counter"),
    ("0x04", 4, "visualClass", "caller"),
    ("0x08", 4, "routeFlag", "caller"),
    ("0x0c", 4, "sourceActorId", "0x00"),
    ("0x10", 4, "effectOrAnimationId", "0x04"),
    ("0x14", 4, "preActionDword0", "0x08"),
    ("0x18", 4, "preActionDword1", "0x0c"),
    ("0x1c", 4, "preActionFloat0", "0x10"),
    ("0x20", 4, "preActionFloat1", "0x14"),
    ("0x24", 4, "preActionFloat2", "0x18"),
    ("0x28", 4, "constant", "1.0 at 0x00f54f70"),
    ("0x2c", 4, "preActionFloat3", "0x1c"),
    ("0x30", 2, "rowCount", "0x20"),
    ("0x32", 2, "controlValue", "0x22"),
    ("0x34", 2, "commandId", "0x24"),
    ("0x36", 2, "presentationFlags", "0x26"),
]
ROW_FIELDS = [
    ("0x00", 4, "targetActorId"),
    ("0x04", 4, "numericValue"),
    ("0x08", 4, "effectId"),
    ("0x0c", 2, "worldMasterTextId"),
    ("0x0e", 1, "textParam"),
    ("0x0f", 1, "rowOrdinal"),
    ("0x10", 4, "reserved"),
]
WIRE_ROWS = {
    "single": ("0x28", "0x2c", "0x2e", "0x30", "0x34", "0x35"),
    "ten": ("0x28", "0x50", "0x64", "0x78", "0xa0", "0xaa"),
    "eighteen": ("0x28", "0x70", "0x94", "0xb8", "0x100", "0x112"),
}
WIRE_ROW_KEYS = ("target", "numeric", "textId", "effect", "textParam", "ordinal")
CLASS_CASES = [
    ([0, 0], 0, 3, False),
    ([1, 1], 1, 4, False),
    ([2, 2], 2, 4, False),
    ([3, 11], 3, 4, True),
    ([12, 25], 4, 4, False),
]
CATEGORY_NAME_ROWS = {4: "lib", 10: "kao", 21: "liu", 22: "lin", 23: "lif", 24: "lil"}
CATEGORY_KINDS = {0: 0, 1: 9, 4: 1, 5: 7, 10: 8, 21: 2, 22: 3, 23: 4, 24: 5, 25: 17}
ROW_STEPS = [
    "skip the row when worldMasterTextId is zero",
    "skip the row when the ordinal filter is on, a selector is set and the row ordinal differs",
    "create a damage plate when the plate gate holds",
    "call the message sound filter 0x005859f0 and skip the rest of the row when it returns false",
    "run the special text ID switch",
    "send the default log line unless text ID 0x75b4 took the special route",
    "call the post routine 0x005785d0 with the row target and the record effect ID",
    "set the selector to the row ordinal and clear the row text ID",
]
LOG_ARGUMENTS = [
    "sourceElementId",
    "commandId",
    "flags >> 4 & 0x7f",
    "preActionDword0",
    "preActionDword1",
    "flags & 3",
    "flags >> 2 & 3",
    "preActionFloat3",
    "flags >> 11",
    "textId",
    "targetActorId",
    "numericValue low 16 bits",
    "effectId",
    "textParam",
]
SPECIAL_IDS = [
    (["0x75b1"], "0x0058c010"),
    (["0x75b4"], "0x0058c010"),
    (["0x847f"], "target equals the container-selected element"),
    (
        [f"0x{i:04x}" for i in (0x848E, 0x848F, 0x8490, 0x8491, 0x8492, 0x8494, 0x8495)]
        + [f"0x{i:04x}" for i in range(0x8499, 0x84A4)],
        "target element found",
    ),
]
DEFAULT_LINE_ONLY = ["0x8493", "0x8496", "0x8497", "0x8498"]
PLATE_KIND_TARGETS = {
    "1": "0x00795a27",
    "2": "0x00795a27",
    "3": "0x00795b74",
    "4": "0x00795c50",
    "5": "0x007963ae",
    "6": "0x00795ca6",
    "7": "0x00795a27",
    "8": "0x00795c0c",
    "9": "0x0079609b",
    "10": "0x00795c50",
}
DIGIT_BLOCK_VAS = [0x00FE1640, 0x00FE1690, 0x00FE16E0, 0x00FE1730, 0x00FE1780]
EXPECTED_CALL_SITES = {
    "0x004d8830": ["0x0058cce5"],
    "0x0058c7d0": ["0x0058d549"],
    "0x0058c880": ["0x0058d55b", "0x006aad9b"],
    "0x0058c930": ["0x0058d56d", "0x006aae74"],
    "0x0058c990": ["0x0058d57f"],
    "0x00587210": [
        "0x00587360",
        "0x0058738b",
        "0x005873fb",
        "0x005874cb",
        "0x0058c805",
    ],
    "0x00587370": [
        "0x0058c702",
        "0x0058c85a",
        "0x0058c8b5",
        "0x0058ca5d",
        "0x0058e683",
    ],
    "0x005873e0": ["0x0058c90a", "0x0058c965"],
    "0x005874b0": ["0x0058c9c5"],
    "0x0058ca80": ["0x0058db21", "0x0058dd67", "0x0058df64"],
    "0x0058c220": ["0x0058caad"],
    "0x00585af0": ["0x0058cac3"],
    "0x0058a010": ["0x0058db3b"],
    "0x0058de90": ["0x0058e481"],
    "0x0058dbe0": ["0x0058e464", "0x0058e80e"],
    "0x00795630": ["0x0058c382"],
    "0x005856c0": ["0x005871ec"],
    "0x00587130": [],
    "0x00585740": ["0x006aca52"],
    "0x005901d0": [
        "0x0058c216",
        "0x0058c712",
        "0x0058c815",
        "0x0058c86a",
        "0x0058c8c5",
        "0x0058c91a",
        "0x0058c975",
        "0x0058c9d5",
        "0x0058ca6d",
        "0x0058da83",
        "0x0058daa6",
        "0x0058dac6",
        "0x0058daea",
        "0x0058db4d",
        "0x0058e693",
    ],
    "0x0058da10": ["0x0058dfc8"],
    "0x007950a0": ["0x0058b768"],
}
EXPECTED_PUSH_SCANS = {
    "0x004d7980": ["0x00585722", "0x00585749"],
    "0x0054b0d0": [],
}
EXPECTED_DWORD_SCANS = {"0x00587130": [], "0x005856c0": [], "0x00585740": []}
REQUIRED_CHECKS = {
    "element vtable slots 6 to 9",
    "dispatcher pre-gates",
    "dispatcher range test",
    "shared lookup and slot 9 call",
    "key read from packet record",
    "key remap",
    "dispatcher call arguments",
    "sub-event source copy",
    "packet record key and target store",
    "registry map lookup",
    "registry miss",
    "registry hit",
    "element key getter",
    "admission predicate",
    "slot 9 admission call",
    "slot 9 rejected-state split",
    "slot 9 rejected-state index",
    "slot 9 admitted-state index",
    "slot 9 rejected-state byte table",
    "slot 9 rejected-state targets",
    "slot 9 admitted-state byte table",
    "slot 9 admitted-state targets",
    "case 0x013C body",
    "case 0x0139 body",
    "case 0x013A body",
    "case 0x013B body",
    "wrapper 0x0139 classify and stage",
    "wrapper 0x0139 queue",
    "wrapper 0x013A classify and stage",
    "wrapper 0x013A queue",
    "wrapper 0x013B classify and stage",
    "wrapper 0x013B queue",
    "wrapper 0x013C classify and stage",
    "wrapper 0x013C queue",
    "base stager serial",
    "base stager serial publish",
    "base stager header copy",
    "base stager control and row count",
    "base stager flags store",
    "base stager name request",
    "single-row stager copy",
    "ten-row stager entry",
    "ten-row stager row copy",
    "ten-row stager loop bound",
    "eighteen-row stager entry",
    "eighteen-row stager row copy",
    "eighteen-row stager loop bound",
    "classifier type overrides",
    "classifier kind 8",
    "classifier kind 1",
    "classifier kinds 2 to 5 and fallback",
    "lane split type 0x0B",
    "lane split ordinary",
    "category index bound",
    "category row read",
    "category table rows 0 to 33",
    "category name row 4",
    "category name row 10",
    "category name row 21",
    "category name row 22",
    "category name row 23",
    "category name row 24",
    "ring push growth",
    "ring push slot copy",
    "slot 6 drain call",
    "drain empty test",
    "drain pop and class dispatch",
    "drain case targets",
    "drain class to case table",
    "drain case 0",
    "drain case 1",
    "drain case 3 bucket push",
    "drain case 3 post and header",
    "drain case 3 range release",
    "drain case 4",
    "drain post tail",
    "drain pop",
    "header dispatch",
    "header line part 1",
    "header line part 2",
    "header line part 3",
    "drain mode flags",
    "row inert test",
    "row ordinal filter",
    "plate gate part 1",
    "plate gate part 2",
    "row sound filter",
    "special text id compare 1",
    "special text id compare 2",
    "text id 0x75B4 plate list call",
    "text id 0x75B1 plate list call",
    "text id 0x848E group call",
    "default log arguments 1",
    "default log arguments 2",
    "default log call",
    "row post and selector",
    "special text id table index",
    "special text id group targets",
    "special text id group table",
    "profile widget name",
    "chain bonus widget name",
    "range gate call",
    "range gate compare",
    "element event dispatch",
    "element event targets",
    "event 0x59 case",
    "event 0x58 case",
    "event 0x59 handler head",
    "event 0x59 handler class bound",
    "event 0x59 handler row call",
    "event publisher",
    "bus command mapping",
    "bus delivery",
    "event 0x64 stage",
    "local builder",
    "plate list lookup",
    "plate global gates",
    "plate row inputs",
    "plate slot allocation",
    "plate slot store",
    "plate form load",
    "plate pool advance",
    "plate kind dispatch",
    "plate kind bounds",
    "plate kind targets",
    "plate digit magic divide",
    "plate digit loop bound",
    "plate digit block names",
    "plate form path",
    "plate icon name",
    "plate additional text name",
    "plate damage type name",
    "plate damage animation",
    "plate miss animation",
    "plate digit block 5",
    "plate digit block 4",
    "plate digit block 3",
    "plate digit block 2",
    "plate digit block 1",
    "name request guard",
    "logger getter",
    "override getter",
    "config getter",
    "config lookup",
    "text id zero test",
}


def hex8(value: int) -> str:
    return f"0x{value:08x}"


def lookup(document: dict, path: tuple) -> object:
    node: object = document
    for key in path:
        node = node.get(key, {}) if isinstance(node, dict) else {}
    return node


def check_row(document: dict, name: str) -> dict:
    for item in document.get("staticChecks", []):
        if isinstance(item, dict) and item.get("name") == name:
            return item
    return {}


def check_bytes(document: dict, name: str) -> list[int]:
    try:
        return list(bytes.fromhex(check_row(document, name).get("pattern", "")))
    except ValueError:
        return []


def check_values(document: dict, name: str) -> list[int]:
    try:
        return [int(v, 16) for v in check_row(document, name).get("values", [])]
    except (TypeError, ValueError):
        return []


def rejected_state_passes(document: dict, opcode: int) -> bool:
    """True when the slot 9 filter used in the rejecting state lets the opcode through."""
    table = check_bytes(document, "slot 9 rejected-state byte table")
    index = opcode - 0x137
    if opcode <= 0x134 or not 0 <= index < len(table):
        raise ValueError("opcode outside the rejected-state table")
    return table[index] == 0


def group_text_ids(document: dict) -> tuple[list[str], list[str]]:
    """Text IDs 0x848E to 0x84A3 that reach the group list call, and those that get the default line only."""
    compare = check_bytes(document, "special text id compare 1")
    index_code = check_bytes(document, "special text id table index")
    table = check_bytes(document, "special text id group table")
    targets = check_values(document, "special text id group targets")
    group_va = int(check_row(document, "text id 0x848E group call").get("va", "0"), 16)
    if len(compare) < 20 or len(index_code) < 10:
        return [], []
    high = int.from_bytes(bytes(compare[5:9]), "little")
    low = int.from_bytes(bytes(compare[16:20]), "little")
    base = int.from_bytes(bytes(index_code[1:5]), "little")
    limit = index_code[7]
    grouped: list[str] = []
    plain: list[str] = []
    for text_id in range(low, base + limit + 1):
        if text_id <= high:
            hit = True
        elif text_id < base or text_id - base >= len(table):
            hit = False
        else:
            hit = targets[table[text_id - base]] == group_va
        (grouped if hit else plain).append(f"0x{text_id:04x}")
    return grouped, plain


def admitted_case(document: dict, opcode: int) -> int:
    """Case body VA that slot 9 selects for an opcode in the admitted state."""
    index_table = check_bytes(document, "slot 9 admitted-state byte table")
    targets = check_values(document, "slot 9 admitted-state targets")
    return targets[index_table[opcode - 0x135]]


def route_class(document: dict, effect_id: int, fallback: int) -> int:
    """Visual class of an effect ID, from the category rows of the manifest."""
    classifier = document["classifier"]
    kind_type = effect_id >> 24
    overrides = {int(k, 16): v for k, v in classifier["typeOverrides"].items()}
    if kind_type in overrides:
        return overrides[kind_type]
    if kind_type >= len(classifier["rows"]):
        return fallback
    kind = classifier["rows"][kind_type]["routeKind"]
    middle = 0 if kind_type == 0x0B else (effect_id >> 12) & 0xFFF
    if kind == 1:
        lo, hi = (int(v, 16) for v in classifier["kind1"]["middleRange"])
        inside = lo <= middle <= hi or middle == int(
            classifier["kind1"]["middleExtra"], 16
        )
        return classifier["kind1"]["classInside" if inside else "classOutside"]
    return classifier["kindToClass"].get(str(kind), fallback)


def drain_case(document: dict, visual_class: int) -> int | None:
    table = check_bytes(document, "drain class to case table")
    if visual_class > document["drain"]["dropAbove"] or visual_class >= len(table):
        return None
    return table[visual_class]


def scene_op(document: dict, visual_class: int) -> int | None:
    case = drain_case(document, visual_class)
    if case is None:
        return None
    return 3 if case == 0 else 4


def header_target(visual_class: int) -> str | None:
    if visual_class == 5:
        return "0x00585af0"
    if 6 <= visual_class <= 10:
        return "0x0058c220"
    return None


def plate_kind(effect_id: int) -> int:
    return (effect_id >> 27) & 0xF


def plate_anchor_is_source(document: dict, effect_id: int) -> bool:
    return plate_kind(effect_id) in document["plates"]["anchorSourceKinds"]


def plate_digits(value: int) -> list[int | None]:
    """Digit blocks Number_5 through Number_1 for a nonnegative value."""
    blocks: list[int | None] = []
    rest = value
    for _ in range(5):
        if rest < 0:
            blocks.append(None)
            continue
        blocks.append(rest % 10)
        rest //= 10
        if rest == 0:
            rest = -1
    return blocks


def verify_header(document: dict) -> list[str]:
    errors: list[str] = []
    record = document.get("record", {})
    fields = [
        (f.get("offset"), f.get("width"), f.get("name"), f.get("source"))
        for f in record.get("header", [])
        if isinstance(f, dict)
    ]
    if fields != HEADER_FIELDS:
        errors.append("record header field map changed")
    rows = [
        (f.get("offset"), f.get("width"), f.get("name"))
        for f in record.get("rowFields", [])
        if isinstance(f, dict)
    ]
    if rows != ROW_FIELDS:
        errors.append("record row field map changed")
    wire = record.get("wireRows", {})
    for form, offsets in WIRE_ROWS.items():
        got = tuple(wire.get(form, {}).get(key) for key in WIRE_ROW_KEYS)
        if got != offsets:
            errors.append(f"wire row offsets of the {form} form changed")
    return errors


def verify_opcodes(document: dict) -> list[str]:
    errors: list[str] = []
    got = [
        (
            o.get("opcode"),
            o.get("caseVa"),
            o.get("wrapperVa"),
            o.get("stagerVa"),
            o.get("rowForm"),
            o.get("rowCapacity"),
            o.get("countBound"),
            o.get("lastPayloadByteRead"),
        )
        for o in document.get("opcodes", [])
        if isinstance(o, dict)
    ]
    if got != OPCODE_ROUTES:
        errors.append("opcode routes changed")
    try:
        for row in OPCODE_ROUTES:
            if hex8(admitted_case(document, int(row[0], 16))) != row[1]:
                errors.append(f"opcode {row[0]} disagrees with the slot 9 tables")
    except (IndexError, KeyError, TypeError):
        errors.append("slot 9 tables are unreadable")
    return errors


def verify_admission(document: dict) -> list[str]:
    errors: list[str] = []
    admission = document.get("admission", {})
    try:
        dropped = [
            f"0x{op:04x}"
            for op in range(0x139, 0x13D)
            if not rejected_state_passes(document, op)
        ]
        passing = [
            f"0x{op:04x}"
            for op in range(0x137, 0x137 + 0x6A)
            if rejected_state_passes(document, op)
        ]
    except ValueError:
        return errors + ["rejected-state table is unreadable"]
    if dropped != admission.get("droppedWhenRejected"):
        errors.append("dropped opcodes disagree with the rejected-state table")
    if passing != ["0x0137", "0x0144", "0x0145", "0x0197", "0x01a0"]:
        errors.append("rejected-state table passes other opcodes")
    if len(admission.get("rejectsWhen", [])) != 3:
        errors.append("admission conditions changed")
    return errors


def verify_classifier(document: dict) -> list[str]:
    errors: list[str] = []
    classifier = document.get("classifier", {})
    rows = classifier.get("rows", [])
    values = check_values(document, "category table rows 0 to 33")
    if len(rows) != 34 or len(values) != 102:
        return errors + ["category table has the wrong row count"]
    for index, row in enumerate(rows):
        name_va, packed, kind = values[3 * index : 3 * index + 3]
        expected = {
            "row": index,
            "characterBank": packed & 0xFF,
            "effectBank": (packed >> 8) & 0xFF,
            "routeKind": kind,
        }
        if packed >> 16:
            errors.append(f"category row {index} has padding bits")
        if any(row.get(key) != value for key, value in expected.items()):
            errors.append(f"category row {index} disagrees with the table words")
        if not isinstance(row.get("name"), str) or len(row["name"]) != 3:
            errors.append(f"category row {index} has no three-letter name")
        if name_va != 0x00FE3250 + 4 * index:
            errors.append(f"category row {index} name pointer is out of sequence")
    for index, name in CATEGORY_NAME_ROWS.items():
        string = check_row(document, f"category name row {index}")
        if rows[index]["name"] != name or string.get("text") != name:
            errors.append(f"category name of row {index} changed")
        if string.get("va") != hex8(values[3 * index]):
            errors.append(f"category name row {index} is not at its row pointer")
    for index, kind in CATEGORY_KINDS.items():
        if rows[index]["routeKind"] != kind:
            errors.append(f"route kind of category row {index} changed")
    return errors


def verify_drain(document: dict) -> list[str]:
    errors: list[str] = []
    drain = document.get("drain", {})
    cases = [
        (c.get("classes"), c.get("case"), c.get("sceneOp"), c.get("headerDispatch"))
        for c in drain.get("classCases", [])
        if isinstance(c, dict)
    ]
    if cases != CLASS_CASES:
        errors.append("drain class cases changed")
    for classes, case, op, header in CLASS_CASES:
        for visual_class in range(classes[0], classes[1] + 1):
            if drain_case(document, visual_class) != case:
                errors.append(f"class {visual_class} disagrees with the case table")
            if scene_op(document, visual_class) != op:
                errors.append(f"class {visual_class} scene command changed")
    if [hex8(v) for v in check_values(document, "drain case targets")] != drain.get(
        "caseTargets"
    ):
        errors.append("drain case targets disagree with the table")
    if drain_case(document, 26) is not None:
        errors.append("a class above 25 is not dropped")
    return errors


def verify_events(document: dict) -> list[str]:
    errors: list[str] = []
    targets = check_values(document, "element event targets")
    if len(targets) != 27:
        return errors + ["element event table has the wrong length"]
    wanted = {0x59: 0x0058E480, 0x58: 0x0058E463, 0x64: 0x0058E663}
    for code, va in wanted.items():
        if targets[code - 0x4A] != va:
            errors.append(f"element event 0x{code:02x} target changed")
    slots = check_values(document, "element vtable slots 6 to 9")
    route = document.get("route", {})
    if [hex8(v) for v in (slots[0], slots[2], slots[3])] != [
        route.get("elementDrainTargetVa"),
        route.get("elementEventTargetVa"),
        route.get("elementSlotTargetVa"),
    ]:
        errors.append("element vtable slots disagree with the route")
    command = document["events"]["rowTrigger"]
    if int(command["command"], 16) + 0x15 != int(command["code"], 16):
        errors.append("row trigger command and code disagree")
    if int(command["finishCommand"], 16) + 0x15 != int(command["finishCode"], 16):
        errors.append("finish command and code disagree")
    return errors


def verify_plates(document: dict) -> list[str]:
    errors: list[str] = []
    plates = document.get("plates", {})
    if plates.get("kindTargets") != PLATE_KIND_TARGETS:
        errors.append("plate kind targets changed")
    bounds = check_bytes(document, "plate kind bounds")
    fallthrough = plates.get("kindFallthrough", {})
    if len(bounds) < 12:
        errors.append("plate kind bounds are missing")
    else:
        last = bounds[5] + 1
        target = 0x00795A1A + 6 + int.from_bytes(bytes(bounds[8:12]), "little")
        if bounds[2] != 0xFF or fallthrough != {
            "kinds": [0] + list(range(last + 1, 16)),
            "target": hex8(target),
        }:
            errors.append("plate kind fallthrough disagrees with the bounds check")
    table = [hex8(v) for v in check_values(document, "plate kind targets")]
    if table != [PLATE_KIND_TARGETS[str(k)] for k in range(1, 11)]:
        errors.append("plate kind targets disagree with the jump table")
    if check_values(document, "plate digit block names") != DIGIT_BLOCK_VAS:
        errors.append("plate digit block pointers changed")
    names = plates.get("digitBlockNames", [])
    for number, (va, name) in enumerate(zip(DIGIT_BLOCK_VAS, names), 1):
        row = check_row(document, f"plate digit block {6 - number}")
        if row.get("va") != hex8(va) or row.get("text") != name:
            errors.append(f"plate digit block {6 - number} string changed")
        if not name.endswith(f"Number_{6 - number}"):
            errors.append(f"plate digit block name {name!r} is out of order")
    for key, name in (
        ("plate form path", plates.get("formPath")),
        ("plate icon name", "IconControl_Icon"),
        ("plate additional text name", "TextBlock_Additional"),
        ("plate damage type name", "TextBlock_DamageType"),
        ("plate damage animation", "DamagePlateAnime.Popup"),
        ("plate miss animation", "MissPlateAnime.Popup"),
    ):
        if check_row(document, key).get("text") != name:
            errors.append(f"{key} disagrees with the plate section")
    return errors


def verify_scans(document: dict) -> list[str]:
    errors: list[str] = []
    scans = document.get("scans", {})
    calls = {
        s.get("target"): s.get("sites")
        for s in scans.get("callSites", [])
        if isinstance(s, dict)
    }
    pushes = {
        s.get("target"): s.get("sites")
        for s in scans.get("pushScans", [])
        if isinstance(s, dict)
    }
    dwords = {
        s.get("value"): s.get("sites")
        for s in scans.get("dwordSites", [])
        if isinstance(s, dict)
    }
    if calls != EXPECTED_CALL_SITES:
        errors.append("call site scans changed")
    if pushes != EXPECTED_PUSH_SCANS:
        errors.append("push scans changed")
    if dwords != EXPECTED_DWORD_SCANS:
        errors.append("dword scans changed")
    for scan in scans.get("pushScans", []):
        if scan.get("pushes") != ["0x43", "0x44", "0x45"] or scan.get("window") != 14:
            errors.append("push scan parameters changed")
    return errors


def verify_sections(document: dict) -> list[str]:
    errors: list[str] = []
    for path, expected in FIXED_VALUES.items():
        if lookup(document, path) != expected:
            errors.append(f"{'.'.join(path)} changed")
    if document.get("rowProcessing", {}).get("steps") != ROW_STEPS:
        errors.append("row processing steps changed")
    if document.get("rowProcessing", {}).get("logArguments") != LOG_ARGUMENTS:
        errors.append("log arguments changed")
    special = [
        (s.get("ids"), s.get("gate"))
        for s in document.get("rowProcessing", {}).get("specialTextIds", [])
        if isinstance(s, dict)
    ]
    if special != SPECIAL_IDS:
        errors.append("special text IDs changed")
    grouped, plain = group_text_ids(document)
    entries = document.get("rowProcessing", {}).get("specialTextIds", [])
    listed = entries[3] if len(entries) > 3 and isinstance(entries[3], dict) else {}
    if grouped != SPECIAL_IDS[3][0] or listed.get("ids") != grouped:
        errors.append("group text IDs disagree with the jump table")
    if plain != DEFAULT_LINE_ONLY or listed.get("defaultLineOnly") != plain:
        errors.append("default-line text IDs disagree with the jump table")
    errors += verify_header(document)
    errors += verify_opcodes(document)
    errors += verify_admission(document)
    errors += verify_classifier(document)
    errors += verify_drain(document)
    errors += verify_events(document)
    errors += verify_plates(document)
    errors += verify_scans(document)
    return errors


def heading_anchors(text: str) -> set[str]:
    anchors = set()
    for line in text.splitlines():
        match = re.match(r"^#{2,6}\s+(.*?)\s*$", line)
        if match:
            slug = re.sub(r"[^a-z0-9 -]", "", match.group(1).lower())
            anchors.add(slug.replace(" ", "-"))
    return anchors


def verify_page(document: dict, text: str | None = None) -> list[str]:
    errors: list[str] = []
    if text is None:
        text = PAGE.read_text(encoding="utf-8")
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
        if isinstance(c, dict)
    ]
    if rows != claims:
        errors.append("the verdict table disagrees with the manifest claims")
    anchors = heading_anchors(text)
    for section in SECTIONS:
        if section not in anchors:
            errors.append(f"the page has no section {section!r}")
    consumer = document.get("consumer", {})
    for key in ("implementNow", "blocked"):
        for item in consumer.get(key, []):
            if f"- {item}" not in text.splitlines():
                errors.append(f"the page lacks consumer item {item[:40]!r}")
    if "\t" in text or any(ord(ch) > 127 for ch in text):
        errors.append("the page has non-ASCII or tab characters")
    return errors


def verify(document: dict | None = None, text: str | None = None) -> list[str]:
    errors: list[str] = []
    check_page = document is None
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-combat-result-element-v1":
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
        if any(c.get("section") not in SECTIONS for c in claims):
            errors.append("a claim has an unknown section")
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
            "wideString",
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
    if not consumer.get("implementNow") or not consumer.get("blocked"):
        errors.append("consumer lists are empty")
    dump = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, dump, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    if check_page or text is not None:
        errors += verify_page(document, text)
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("route", "opcodeRange", 1), "0x013e"),
        (("route", "keySource"), "sub-event header dword +8"),
        (("route", "keyRemap", "mask"), "0x5ff00000"),
        (("route", "elementSlot"), 8),
        (("admission", "predicateVa"), "0x004d8840"),
        (("admission", "droppedWhenRejected", 0), "0x0138"),
        (("admission", "rejectedStatePassingAbove0134", 2), "0x0146"),
        (("opcodes", 0, "wrapperVa"), "0x0058c930"),
        (("opcodes", 1, "rowCapacity"), 18),
        (("opcodes", 2, "countBound"), "none, one row is always staged"),
        (("opcodes", 3, "stagerVa"), "0x00587370"),
        (("wrapper", "classifierFallback"), 5),
        (("wrapper", "routeFlag"), 0),
        (("record", "header", 0, "source"), "caller"),
        (("record", "header", 12, "offset"), "0x32"),
        (("record", "rowFields", 1, "offset"), "0x08"),
        (("record", "wireRows", "ten", "ordinal"), "0xab"),
        (("record", "wireRows", "eighteen", "numeric"), "0x6e"),
        (("record", "stageRequest", "skipsEffectId"), "0x7c000063"),
        (("classifier", "typeOverrides", "0x7c"), 6),
        (("classifier", "kindToClass", "8"), 10),
        (("classifier", "kind1", "middleExtra"), "0xfea"),
        (("classifier", "rows", 4, "routeKind"), 7),
        (("classifier", "rows", 21, "name"), "lix"),
        (("drain", "recordsPerPass"), 2),
        (("drain", "bucketRingBase"), "0x87c"),
        (("drain", "dropAbove"), 26),
        (("drain", "classCases", 0, "sceneOp"), 4),
        (("drain", "classCases", 3, "classes", 1), 12),
        (("drain", "headerFlags", "drain"), [0, 1]),
        (("drain", "headerTargets", "5"), "0x0058c220"),
        (("drain", "headerPrecondition", "radiusOffset"), "0xaac"),
        (("events", "rowTrigger", "code"), "0x58"),
        (("events", "rowTrigger", "publisherCaller"), "0x00587130"),
        (("events", "stageDoor", "routeFlag"), 1),
        (("rowProcessing", "steps", 3), "skip the rest of the row"),
        (("rowProcessing", "logArguments", 9), "numericValue"),
        (("rowProcessing", "specialTextIds", 1, "ids", 0), "0x75b5"),
        (("rowProcessing", "specialTextIds", 3, "ids", 5), "0x8493"),
        (("rowProcessing", "specialTextIds", 3, "defaultLineOnly", 1), "0x8494"),
        (("plates", "poolSlots"), 16),
        (("plates", "anchorSourceKinds", 0), 6),
        (("plates", "kindTargets", "5"), "0x00795a27"),
        (("plates", "kindFallthrough", "kinds", 1), 10),
        (("plates", "kindFallthrough", "target"), "0x00795a27"),
        (("plates", "gate", "configOption", 1), 6),
        (("plates", "digitBlocks"), 4),
        (("plates", "digitBlockNames", 0), "Label_DamageDisplay:TextBlock_Number_1"),
        (("scans", "callSites", 17, "sites"), ["0x00587130"]),
        (("scans", "pushScans", 0, "sites"), ["0x00585722"]),
        (("scans", "dwordSites", 0, "sites"), ["0x00401000"]),
        (("claims", 0, "tier"), "INFERRED"),
        (("claims", 27, "tier"), "STATIC"),
        (("claims", 3, "id"), "slot-nine"),
        (("claims", 5, "claim"), " "),
        (("claims", 7, "section"), "nowhere"),
        (("consumer", "implementNow"), []),
        (("staticChecks", 1, "kind"), "other"),
        (("staticChecks", 2, "va"), "0x4dd005"),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    for name in (
        "admission predicate",
        "slot 9 rejected-state byte table",
        "wrapper 0x0139 classify and stage",
        "base stager serial",
        "ten-row stager loop bound",
        "classifier type overrides",
        "drain class to case table",
        "header dispatch",
        "drain mode flags",
        "row inert test",
        "plate gate part 1",
        "event publisher",
        "plate digit loop bound",
        "plate form load",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    swapped = copy.deepcopy(document)
    for item in swapped["staticChecks"]:
        if item["name"] == "slot 9 rejected-state byte table":
            item["pattern"] = "00 " + item["pattern"][3:]
        if item["name"] == "drain class to case table":
            item["pattern"] = item["pattern"][:-2] + "03"
    mutations.append(swapped)
    retargeted = copy.deepcopy(document)
    for item in retargeted["staticChecks"]:
        if item["name"] == "special text id group targets":
            item["values"] = item["values"][::-1]
    mutations.append(retargeted)
    shifted = copy.deepcopy(document)
    for item in shifted["staticChecks"]:
        if item["name"] == "special text id group table":
            item["pattern"] = "00 " + item["pattern"][:-3]
    mutations.append(shifted)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    text = PAGE.read_text(encoding="utf-8")
    page_cases = [
        text.replace("| STATIC |", "| INFERRED |", 1),
        text.replace("## Damage plate", "## Plates", 1),
        text.replace("- Stage one row", "- Stage two rows", 1),
    ]
    errors = [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]
    errors += [
        f"page mutation {index} was accepted"
        for index, item in enumerate(page_cases, 1)
        if not verify_page(document, item)
    ]
    return errors


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


def push_sites(data: bytes, target: int, pushes: set[int], window: int) -> list[int]:
    """Call sites of a target with a 6A imm8 push of a listed value in the bytes before it."""
    found = []
    for start, virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        if not (start <= CODE_START < start + max(virtual_size, raw_size)):
            continue
        for site in call_sites(data, {target})[target]:
            at = raw_offset + site - start
            before = data[max(raw_offset, at - window) : at]
            if any(
                before[i] == 0x6A and before[i + 1] in pushes
                for i in range(len(before) - 1)
            ):
                found.append(site)
    return found


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


def wide_check(data: bytes, sections: list, check: dict) -> str | None:
    expected = check["text"].encode("utf-16-le") + b"\0\0"
    actual = image_checks.read_va(data, sections, int(check["va"], 16), len(expected))
    if actual != expected:
        return f"wide string at {check['va']} is not {check['text']!r}"
    return None


def scan_errors(data: bytes, document: dict) -> list[str]:
    errors: list[str] = []
    scans = document["scans"]
    wanted = {int(s["target"], 16): s["sites"] for s in scans["callSites"]}
    found = call_sites(data, set(wanted))
    for target, sites in wanted.items():
        if [hex8(s) for s in found[target]] != sites:
            errors.append(f"call sites of {hex8(target)} differ from the scan")
    for scan in scans["pushScans"]:
        values = {int(v, 16) for v in scan["pushes"]}
        got = push_sites(data, int(scan["target"], 16), values, scan["window"])
        if [hex8(s) for s in got] != scan["sites"]:
            errors.append(f"push scan of {scan['target']} differs")
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
        if check["kind"] == "wideString":
            problem = wide_check(data, sections, check)
        else:
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
        print(f"FAIL: {len(errors)} combat result element error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: combat result element manifest, page, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
