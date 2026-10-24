#!/usr/bin/env python3
"""Validate the sound-effect trigger contract, optional exe bytes, install banks and markup census."""

from __future__ import annotations

import argparse
import copy
import json
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "sound_effect_triggers.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
CATEGORY_COUNT = 22
CATEGORY_SPAN = 10000
BASES = [
    0x035B0000,
    0x07FE0000,
    0x07FD0000,
    *[0x5B2D0000 + 0x10000 * step for step in range(18)],
    0x8A0A0000,
]
CATEGORY_FILES = [83, 110, 425, *[55] * 18, 1]
CATEGORY_LAST = [82, 109, 463, *[54] * 18, 0]
CATEGORY_SSCF = [83, 109, 425, *[55] * 18, 1]
DIRECTORY_COUNTS = {
    "03/5B": 83,
    "07/FD": 425,
    "07/FE": 110,
    "5B/2D..5B/3E": 990,
    "8A/0A": 1,
}
FOOTSTEP_COUNTS = {"08/07": 770, "1A/46": 45}
MARKUP_TOTALS = {
    "sqexFiles": 1155,
    "templateFilesDecoded": 482,
    "filesWithAction": 200,
    "actionElements": 1012,
    "distinctFileNumbers": 61,
    "minFileNumber": 1,
    "maxFileNumber": 82,
    "indexAttributeCount": 0,
    "volumeAttributeCount": 2,
    "priorityAttributeCount": 5,
    "luaSoundCommandTriggerElements": 24,
}
MARKUP_TRIGGERS = {"EventTrigger": 616, "CommandTrigger": 395, "none": 1}
VOICE_ENTRIES = [
    [1, 30021],
    [2, 40021],
    [11, 130021],
    [12, 140021],
    [5, 70021],
    [6, 80021],
    [7, 90021],
    [8, 100021],
    [17, 190021],
    [18, 200021],
]
VOICE_ROW_GROUPS = [1, 2, 9, 3, 4, 3, 4, 5, 6, 5, 6, 8, 8, 7, 7]
VOICE_GROUP_CATEGORIES = {
    "1": [3, 4, 15],
    "2": [5, 6, 13],
    "3": [11, 12, 3],
    "4": [13, 14, 5],
    "5": [7, 8, 11],
    "6": [9, 10, 17],
    "7": [19, 20, 15],
    "8": [17, 18, 9],
    "9": [15, 16, 19],
}
ENTRY_SITES = {
    "play_number": ("0x004bbb00", ["0x005527ec", "0x0085e9fc", "0x00566073"]),
    "play_category": ("0x004b7470", ["0x004bbb23"]),
    "play_category21": ("0x004b7500", []),
    "play_footstep_pair": ("0x004b7790", ["0x007d0ea3"]),
    "play_resource_wrapper": ("0x00567070", ["0x00561a0e", "0x0056ae94"]),
    "play_action_table": ("0x0084ff50", ["0x0065b271"]),
    "play_resource_clip": (
        "0x007cf100",
        [
            "0x0081b378",
            "0x0081b3d9",
            "0x0081b7c3",
            "0x0081b5bf",
            "0x0081afcd",
            "0x0081aff2",
            "0x0081b11c",
            "0x0065ca3e",
            "0x0065e12a",
            "0x008d5b35",
            "0x0060ec97",
        ],
    ),
    "play_message_sound": ("0x005859f0", ["0x0058c395"]),
}
LITERAL_IDS = {
    ("0x035b000c", 12, "0x0056ad94"),
    ("0x035b0031", 49, "0x005619fc"),
    ("0x035b0037", 55, "0x00585ace"),
    ("0x035b0038", 56, "0x00585a65"),
    ("0x035b000f", 15, "0x00865cc2"),
    ("0x035b000e", 14, "0x00865cca"),
    ("0x035b0010", 16, "0x00865cd2"),
}
IMMEDIATE_SCANS = {
    "0x0135c2ac": ["0x009741f9", "0x00974208", "0x00974221", "0x00974260"],
    "0x00f8d700": ["0x004b748d", "0x004b74a5"],
    "0x8a0a0000": ["0x004b7524"],
    "0x035b0000": ["0x00865cfb"],
    "0x07fd0000": [],
    "0x5b2d0000": [],
    "0x08070002": ["0x004b5820", "0x004b77e3"],
    "0x1a460000": ["0x004b583e", "0x004b781a"],
    "0x0132f384": [
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
    "0x0132f3b4": [
        "0x004b73ed",
        "0x004b74e1",
        "0x004b7544",
        "0x004b77ec",
        "0x004b7820",
        "0x0056709a",
        "0x0056ad9a",
        "0x00585ad9",
        "0x007cf2ea",
        "0x0085012b",
        "0x00e5d081",
        "0x00f07751",
        "0x00f2e672",
        "0x00f2e683",
    ],
    "0x5b2e0000": [],
    "0x5b2f0000": [],
    "0x5b300000": [],
    "0x5b310000": [],
    "0x5b320000": [],
    "0x5b330000": [],
    "0x5b340000": [],
    "0x5b350000": [],
    "0x5b360000": [],
    "0x5b370000": [],
    "0x5b380000": [],
    "0x5b390000": [],
    "0x5b3a0000": [],
    "0x5b3b0000": [],
    "0x5b3c0000": ["0x0049f011", "0x00cb637d"],
    "0x5b3d0000": ["0x0043199d"],
    "0x5b3e0000": [],
    "0x0060ec70": [],
    "0x0132f3cc": ["0x004b7794", "0x00f07773"],
}
DIRECT_CALL_SCANS = {
    "0x0060ec70": [],
    "0x0065e100": ["0x007be6bb"],
    "0x007be450": ["0x007c1a41"],
    "0x007c0e10": ["0x007c2bcd"],
    "0x007c2b90": ["0x00667f33"],
    "0x004b5800": [],
    "0x004b5830": [],
}
EXPECTED_CLAIMS = {
    "action-properties": "STATIC",
    "action-run": "STATIC",
    "interface-global": "STATIC",
    "pending-priority-rule": "STATIC",
    "pending-initial-state": "STATIC",
    "frame-update-play-clear": "STATIC",
    "frame-update-skip": "STATIC",
    "frame-update-cadence": "STATIC",
    "frame-update-unread-fields": "STATIC",
    "number-split": "STATIC",
    "category-table": "STATIC",
    "volume-gate": "STATIC",
    "request-objects": "STATIC",
    "module-play": "STATIC",
    "install-categories": "OBSERVED",
    "markup-census": "OBSERVED",
    "markup-file-numbers": "OBSERVED",
    "config-texts": "STATIC",
    "config-selector": "STATIC",
    "bus-numbers": "UNPROVED",
    "literal-ids": "STATIC",
    "entry-callers": "STATIC",
    "caller-roles": "INFERRED",
    "message-sound": "STATIC",
    "message-sound-origin": "INFERRED",
    "dispatcher-reach": "NOT FOUND",
    "creation-appearance-table": "STATIC",
    "creation-list-table": "STATIC",
    "creation-key-meaning": "INFERRED",
    "gameplay-voice": "UNPROVED",
    "unconsumed-categories": "NOT FOUND",
    "footstep-arithmetic": "STATIC",
    "footstep-banks": "OBSERVED",
    "lua-sound-commands": "INFERRED",
    "footstep-gate": "STATIC",
    "message-sound-filter": "STATIC",
}
FIXED_TEXT = {
    ("soundPlayerAction", "runVa"): "0x009b3060",
    ("soundPlayerAction", "interfaceGlobalVa"): "0x0135c2ac",
    ("pendingRequest", "setterVa"): "0x0054b070",
    ("pendingRequest", "frameUpdateVa"): "0x005527a0",
    (
        "pendingRequest",
        "rule",
    ): "A request replaces the held one when its priority byte is greater than or equal to the held byte (unsigned). An empty cell holds priority 0.",
    ("playPath", "numberPlayVa"): "0x004bbb00",
    ("playPath", "categoryTableVa"): "0x00f8d700",
    ("playPath", "boundCheck"): "none",
    (
        "driverConfig",
        "selection",
    ): "Mode 4 or 5 selects the text at 0x00f542a0 and any other mode the text at 0x00f545c0.",
    (
        "install",
        "scope",
    ): "owner installation of build 2012.09.19.0001, existence and first eight bytes only",
    ("messageSound", "va"): "0x005859f0",
    ("messageSound", "exclusionTableVa"): "0x01270ba4",
}
PROPERTIES = [
    ("FileNumber", "0x0107d838", "0x44", "dword", "0"),
    ("Index", "0x0107d844", "0x4c", "dword", "0"),
    ("Volume", "0x0107d84c", "0x54", "float", "1.0"),
    ("Priority", "0x0107d854", "0x5c", "byte", "0"),
]
PENDING_FIELDS = {
    "fileNumber": "0x30c",
    "index": "0x310",
    "volume": "0x314",
    "vector": "0x31c",
    "priority": "0x32c",
    "soundModule": "0x1c",
}
PENDING_INITIAL = {
    "fileNumber": -1,
    "index": 0,
    "volume": 0.0,
    "vector": [0, 0, 0, 1],
    "priority": 0,
}
BUSES = [
    ("MUSIC", 3, "2D", "OFF"),
    ("SE", 20, "3D", "ON"),
    ("VOICE", [0, 5], "3D", "OFF"),
    ("System", 8, "2D", "OFF"),
    ("BG", 12, "3D", "ON"),
    ("Foot", 15, "3D", "ON"),
    ("PlacedNPC", 8, "3D", "OFF"),
    ("StringendoMUSIC", 2, "2D", "OFF"),
    ("NULL9", 1, "2D", "OFF"),
    ("NULL10", 1, "2D", "OFF"),
    ("HQ", 4, "2D", "OFF"),
]
REQUIRED_CHECKS = {
    "SoundPlayerAction FileNumber property init",
    "SoundPlayerAction FileNumber property constructor call",
    "SoundPlayerAction Index property init",
    "SoundPlayerAction Index property constructor call",
    "SoundPlayerAction Volume property init",
    "SoundPlayerAction Volume property constructor call",
    "SoundPlayerAction Priority property init",
    "SoundPlayerAction Priority property constructor call",
    "SoundPlayerAction run interface getter call",
    "SoundPlayerAction run null interface test",
    "SoundPlayerAction run origin vector",
    "SoundPlayerAction run slot 10 call",
    "origin vector constructor",
    "interface global store",
    "interface global clear",
    "interface global read",
    "pending request priority compare",
    "pending request file number store",
    "pending request index store",
    "pending request volume store",
    "pending request vector null test",
    "pending request vector copy",
    "pending request priority store",
    "pending request slot 10 return",
    "interface constructor pending file number",
    "interface constructor pending index",
    "interface constructor pending volume",
    "interface constructor pending vector",
    "interface constructor pending priority",
    "frame update pending file number test",
    "frame update volume load",
    "frame update play call",
    "frame update file number clear",
    "frame update priority clear",
    "frame update caller interface load",
    "frame update caller call",
    "Rapture update calls the per-frame method",
    "number split magic divide",
    "number split remainder",
    "number split call",
    "category play volume gate",
    "category play volume gate branch",
    "category play first call",
    "category play base load",
    "category play request object",
    "category play return",
    "category 21 play",
    "module getter",
    "footstep first id",
    "footstep second id",
    "footstep request object",
    "shared request object, generic clip play",
    "shared request object, wrapper",
    "shared request object, action table play",
    "action table index",
    "sound clip play call",
    "sound load clip play call, slot 1",
    "sound load clip play call, slot 2",
    "target element play number",
    "target element play call",
    "logo menu id, first",
    "logo menu id, second",
    "screenshot element id",
    "location phase base add",
    "location phase index 15",
    "location phase index 14",
    "location phase index 16",
    "message sound call",
    "message sound id 0x8475 test",
    "message sound id 0x846e test",
    "message sound 0x8475 resource",
    "message sound 0x846e resource",
    "message sound shared request object",
    "message sound exclusion table compare",
    "creation voice key compare",
    "creation voice random modulus",
    "creation voice number add",
    "creation voice play call",
    "creation voice default entry",
    "config mode read",
    "config mode decrement",
    "number split return",
    "voice list play entry call",
    "voice list position loop",
    "voice list stop slot 7",
    "voice list row table read",
    "voice list id table read",
    "voice list play slot 4",
    "message sound exclusion return false",
    "module setup request object loop",
    "module setup third request object",
    "music request object address",
    "creation key store",
    "footstep composer first id",
    "footstep composer second id",
    "music control reached from the dispatcher",
    "SoundPlayerAction property name FileNumber",
    "SoundPlayerAction property name Index",
    "SoundPlayerAction property name Volume",
    "SoundPlayerAction property name Priority",
    "SoundPlayerAction type name",
    "driver keyword MAX_SOUND",
    "driver keyword BUS",
    "driver keyword ENABLE_SCD_PRIORITY",
    "driver keyword STRINGENDO_FADE_TIMES",
    "config text one MAX_SOUND",
    "config text one MAX_BUS",
    "config text one SE bus",
    "config text one VOICE bus",
    "config text one HQ bus",
    "config text one ENABLE_SCD_PRIORITY",
    "config text one STRINGENDO_FADE_TIMES",
    "config text two MAX_SOUND",
    "config text two VOICE bus",
    "SoundPlayerAction vftable",
    "RaptureSqwtInterface vftable",
    "RaptureSoundModule vftable",
    "category base table",
    "dword after the category table",
    "creation voice number table",
    "creation voice id table",
    "message sound exclusion table",
    "Application::Rapture slot 1 cell",
    "CharaElement slot 6 cell",
    "CharaElement slot 9 cell",
    "TargetElement slot 65 cell",
    "AppearancePhase slot 22 cell",
    "LocationPhase slot 20 cell",
    "AppearanceCommonListMenu slot 14 cell",
    "SqexLogoMenu slot 1 cell",
    "ScreenshotManagerElement slot 6 cell",
    "RaptureSoundClip slot 2 cell",
    "RaptureSoundLoadClip slots 1 and 2 cells",
    "RaptureActorSoundClip slot 1 cell",
    "RaptureCharaActionSoundClip slot 1 cell",
    "RaptureFootstepTriggerClip slot 1 cell",
    "CharaActor slot 142 cell",
    "CharaActionQue slot 9 cell",
    "clip play wrapper call",
    "clip play wrapper body",
    "footstep gate test",
    "footstep gate exit",
    "footstep gate initializer",
    "footstep gate store",
    "footstep first play sixth argument",
    "footstep first play argument pushes",
    "footstep second play argument pushes",
    "config zero register",
    "config mode default",
    "config mode zero test",
    "config mode store",
    "config selector load",
    "config selector compare",
    "message sound drain filter",
    "message sound null actor skip",
    "message sound pre-play call, 0x8475",
    "message sound pre-play call, 0x846e",
    "interface slot 11 body",
    "footstep gate import cell",
    "footstep gate import name",
    "footstep gate initializer cell",
}
STRIDED_TABLES = [
    ("creationVoice", "listMenu", "rowTableVa", "rowStride", "rowGroupDwords")
]
HEX_ADDRESS_RE = re.compile(r"^0x[0-9a-f]{8}$")
ACTION_RE = re.compile(r"<SoundPlayerAction\b([^>]*)>")
BOM = chr(0xFEFF)
ATTR_RE = re.compile(r'(\w+)="([^"]*)"')


def hex8(value: int) -> str:
    return f"0x{value:08x}"


def lookup(document: dict, path: tuple) -> object:
    node: object = document
    for key in path:
        node = node.get(key, {}) if isinstance(node, dict) else {}
    return node


def category_of(resource: int) -> int:
    return (resource >> 16) - 0x5B2D + 3


def check_rows(document: dict, name: str) -> dict:
    for item in document.get("staticChecks", []):
        if isinstance(item, dict) and item.get("name") == name:
            return item
    return {}


def check_values(document: dict, name: str) -> list[int]:
    try:
        return [int(v, 16) for v in check_rows(document, name).get("values", [])]
    except (TypeError, ValueError):
        return []


def verify_sections(document: dict) -> list[str]:
    errors: list[str] = []
    action = document.get("soundPlayerAction", {})
    props = [
        (
            p.get("name"),
            p.get("nameVa"),
            p.get("fieldOffset"),
            p.get("type"),
            p.get("default"),
        )
        for p in action.get("properties", [])
        if isinstance(p, dict)
    ]
    if props != PROPERTIES:
        errors.append("soundPlayerAction properties changed")
    if (
        action.get("runSlot") != 2
        or action.get("interfaceSlot") != 10
        or action.get("interfaceSlotOffset") != "0x28"
        or action.get("interfaceGlobalSites") != IMMEDIATE_SCANS["0x0135c2ac"]
        or len(action.get("slotArguments", [])) != 5
    ):
        errors.append("soundPlayerAction call contract changed")
    pending = document.get("pendingRequest", {})
    if (
        pending.get("fields") != PENDING_FIELDS
        or pending.get("initial") != PENDING_INITIAL
        or pending.get("frameUpdateReads") != ["fileNumber", "volume"]
        or pending.get("slotCount") != 24
        or pending.get("setterSlot") != 10
    ):
        errors.append("pendingRequest contract changed")
    play = document.get("playPath", {})
    call = play.get("playCall", {})
    if (
        play.get("divisor") != CATEGORY_SPAN
        or play.get("divideMagic") != "0xd1b71759"
        or play.get("divideShift") != 13
        or play.get("categoryCount") != CATEGORY_COUNT
        or play.get("afterTableDword") != "0x4b189680"
        or call.get("moduleSlot") != 4
        or call.get("requestObjectVa") != "0x0132f3b4"
        or play.get("requestObjects")
        != {
            "arrayVa": "0x0132f384",
            "stride": "0x18",
            "count": 3,
            "selectorVa": "0x0132f32c",
            "soundEffectIndex": 2,
            "soundEffectVa": "0x0132f3b4",
        }
        or len(call.get("arguments", [])) != 7
    ):
        errors.append("playPath contract changed")
    errors += verify_install_table(document.get("install", {}))
    errors += verify_markup_table(document)
    errors += verify_config(document.get("driverConfig", {}))
    errors += verify_entries(document)
    literal = {
        (r.get("resource"), r.get("index"), r.get("site"))
        for r in document.get("literalIds", [])
        if isinstance(r, dict)
    }
    if literal != LITERAL_IDS or any(
        r.get("category") != 0 for r in document.get("literalIds", [])
    ):
        errors.append("literalIds changed")
    scans = {
        r.get("value"): r.get("sites")
        for r in document.get("immediateScans", [])
        if isinstance(r, dict)
    }
    if scans != IMMEDIATE_SCANS:
        errors.append("immediateScans changed")
    direct = {
        r.get("target"): r.get("sites")
        for r in document.get("directCallScans", [])
        if isinstance(r, dict)
    }
    if direct != DIRECT_CALL_SCANS:
        errors.append("directCallScans changed")
    sound = document.get("messageSound", {})
    ids = [(r.get("messageId"), r.get("resource")) for r in sound.get("ids", [])]
    if (
        ids != [("0x846e", "0x035b0037"), ("0x8475", "0x035b0038")]
        or sound.get("exclusionCount") != 30
        or len(check_values(document, "message sound exclusion table")) != 30
    ):
        errors.append("messageSound changed")
    if (
        sound.get("drainFilter", {}).get("va") != "0x0058c32f"
        or sound.get("prePlayCall", {}).get("va") != "0x007969d0"
        or sound.get("prePlayCall", {}).get("argumentByMessageId")
        != {"0x8475": 0, "0x846e": 1}
    ):
        errors.append("messageSound filter or pre-play call changed")
    footstep = {
        e.get("id"): e for e in document.get("entries", []) if isinstance(e, dict)
    }.get("play_footstep_pair", {})
    gate = footstep.get("gate", {})
    if (
        gate.get("flagVa") != "0x0132f3cc"
        or gate.get("initialValue") != 1
        or gate.get("initializerVa") != "0x00f07770"
        or footstep.get("slot4SixthArgument", {}).get("firstPlay") != "A9"
    ):
        errors.append("footstep gate changed")
    reach = document.get("reachability", {})
    dispatcher = reach.get("dispatcher", {})
    if (
        dispatcher.get("entriesReached") != []
        or dispatcher.get("positiveControl") != "0x004bbb30"
        or dispatcher.get("startVa") != "0x004dc690"
        or reach.get("entries") != [v for v, _ in ENTRY_SITES.values()]
    ):
        errors.append("reachability changed")
    errors += verify_voice(document)
    return errors


def verify_install_table(install: dict) -> list[str]:
    errors: list[str] = []
    cats = install.get("categories")
    if not isinstance(cats, list) or len(cats) != CATEGORY_COUNT:
        return ["install.categories is not 22 rows"]
    for index, row in enumerate(cats):
        want = (
            index,
            hex8(BASES[index]),
            CATEGORY_FILES[index],
            0,
            CATEGORY_LAST[index],
            CATEGORY_SSCF[index],
        )
        got = tuple(
            row.get(k)
            for k in (
                "category",
                "base",
                "files",
                "firstIndex",
                "lastIndex",
                "sscfFiles",
            )
        )
        if got != want:
            errors.append(f"install category {index} changed")
        if not str(row.get("role", "")).strip():
            errors.append(f"install category {index} has no role")
    if install.get("totalFiles") != sum(r.get("files", 0) for r in cats):
        errors.append("install.totalFiles disagrees with the categories")
    if install.get("directoryFileCounts") != DIRECTORY_COUNTS:
        errors.append("install.directoryFileCounts changed")
    if install.get("footstepBanks") != FOOTSTEP_COUNTS:
        errors.append("install.footstepBanks changed")
    return errors


def verify_markup_table(document: dict) -> list[str]:
    errors: list[str] = []
    markup = document.get("markup", {})
    for key, want in MARKUP_TOTALS.items():
        if markup.get(key) != want:
            errors.append(f"markup.{key} changed")
    pairs = markup.get("fileNumberCounts")
    if not isinstance(pairs, list) or any(
        not (isinstance(p, list) and len(p) == 2) for p in pairs
    ):
        return errors + ["markup.fileNumberCounts is malformed"]
    numbers = [p[0] for p in pairs]
    if numbers != sorted(set(numbers)):
        errors.append("markup.fileNumberCounts is not sorted and unique")
    if sum(p[1] for p in pairs) != markup.get("actionElements"):
        errors.append("markup.fileNumberCounts disagrees with actionElements")
    if len(pairs) != markup.get("distinctFileNumbers"):
        errors.append("markup.fileNumberCounts disagrees with distinctFileNumbers")
    if numbers and (
        numbers[0] != markup.get("minFileNumber")
        or numbers[-1] != markup.get("maxFileNumber")
    ):
        errors.append("markup.fileNumberCounts disagrees with the extremes")
    cats = document.get("install", {}).get("categories", [{}])
    if numbers and numbers[-1] > cats[0].get("lastIndex", -1):
        errors.append("a markup FileNumber lies past the category 0 files")
    if markup.get("triggerKinds") != MARKUP_TRIGGERS or sum(
        markup.get("triggerKinds", {}).values()
    ) != markup.get("actionElements"):
        errors.append("markup.triggerKinds changed")
    return errors


def verify_config(config: dict) -> list[str]:
    errors: list[str] = []
    buses = [
        (
            b.get("name"),
            b.get("number"),
            b.get("dimension"),
            b.get("flag"),
        )
        for b in config.get("buses", [])
        if isinstance(b, dict)
    ]
    if buses != BUSES or config.get("maxBus") != len(BUSES):
        errors.append("driverConfig buses changed")
    if any(
        b.get("rate") != 44100 for b in config.get("buses", []) if isinstance(b, dict)
    ):
        errors.append("driverConfig bus rate changed")
    if (
        config.get("maxSound") != 128
        or config.get("maxReverb") != 1
        or config.get("stringendoFadeTimes") != [4500] * 4
        or config.get("stringendoVolumes") != [1.0, 1.23, 1.56, 2.0]
        or config.get("stringendoSetting") != [8, 4]
        or config.get("enableScdPriority") is not True
        or config.get("volumeCurve") != "OLD"
        or config.get("textVas") != ["0x00f542a0", "0x00f545c0"]
        or config.get("textLength") != 798
        or config.get("modeConfigGroup") != 0
        or config.get("modeConfigKey") != "0x19"
    ):
        errors.append("driverConfig values changed")
    return errors


def verify_entries(document: dict) -> list[str]:
    errors: list[str] = []
    entries = document.get("entries")
    if not isinstance(entries, list) or [
        e.get("id") for e in entries if isinstance(e, dict)
    ] != list(ENTRY_SITES):
        return ["entries ids changed"]
    for entry in entries:
        va, sites = ENTRY_SITES[entry["id"]]
        found = [s.get("site") for s in entry.get("sites", []) if isinstance(s, dict)]
        if entry.get("va") != va or found != sites:
            errors.append(f"entry {entry['id']} changed")
        if any(
            not str(s.get("owner", "")).strip() or not str(s.get("kind", "")).strip()
            for s in entry.get("sites", [])
            if isinstance(s, dict)
        ):
            errors.append(f"entry {entry['id']} has a site with no owner or kind")
    classes = document.get("entryClasses", {})
    for key in ("play_footstep_pair", "play_action_table", "play_resource_clip"):
        if not classes.get(key) or classes[key] != sorted(classes[key]):
            errors.append(f"entryClasses {key} is missing or unsorted")
    return errors


def verify_voice(document: dict) -> list[str]:
    errors: list[str] = []
    voice = document.get("creationVoice", {})
    phase = voice.get("appearancePhase", {})
    menu = voice.get("listMenu", {})
    table = check_values(document, "creation voice number table")
    pairs = [[table[i], table[i + 1]] for i in range(0, len(table), 2)]
    if phase.get("entries") != VOICE_ENTRIES or pairs != VOICE_ENTRIES:
        errors.append("creationVoice appearancePhase entries changed")
    if any(number // CATEGORY_SPAN != key + 2 for key, number in VOICE_ENTRIES):
        errors.append("creationVoice entries break the key plus 2 category rule")
    if phase.get("slot") != 22 or phase.get("va") != "0x0085e970":
        errors.append("creationVoice appearancePhase identity changed")
    ids = check_values(document, "creation voice id table")
    derived = {
        str(group): [category_of(ids[6 * group + pos]) for pos in range(3)]
        for group in range(1, 10)
        if len(ids) == 60
    }
    if menu.get("groupCategories") != VOICE_GROUP_CATEGORIES:
        errors.append("creationVoice groupCategories changed")
    if derived != VOICE_GROUP_CATEGORIES:
        errors.append("creationVoice id table disagrees with groupCategories")
    if len(ids) == 60 and any(
        ids[6 * g + p] & 0xFFFF != 27 for g in range(1, 10) for p in range(3)
    ):
        errors.append(
            "creationVoice id table sample index is not 27 for positions 0 to 2"
        )
    if (
        menu.get("rowGroupDwords") != VOICE_ROW_GROUPS
        or menu.get("rowStride") != 44
        or menu.get("idRowWidth") != 6
        or menu.get("positionCount") != 3
        or menu.get("entryVa") != "0x006ac310"
    ):
        errors.append("creationVoice listMenu changed")
    return errors


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-sound-effect-triggers-v1":
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
    for path, expected in FIXED_TEXT.items():
        if lookup(document, path) != expected:
            errors.append(f"{'.'.join(path)} changed")
    claims = document.get("claims")
    if not isinstance(claims, list):
        errors.append("claims is not a list")
    else:
        found = {c.get("id"): c.get("tier") for c in claims if isinstance(c, dict)}
        if len(found) != len(claims) or found != EXPECTED_CLAIMS:
            errors.append("claim ids or tiers changed")
        if any(not str(c.get("claim", "")).strip() for c in claims):
            errors.append("a claim has no text")
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
    base_table = check_values(document, "category base table")
    if base_table != BASES:
        errors.append("category base table check disagrees with the bases")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def claim_index(document: dict, claim_id: str) -> int:
    return [c["id"] for c in document["claims"]].index(claim_id)


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("soundPlayerAction", "properties", 2, "default"), "0"),
        (("soundPlayerAction", "properties", 3, "fieldOffset"), "0x5d"),
        (("soundPlayerAction", "interfaceSlot"), 9),
        (("soundPlayerAction", "interfaceGlobalSites", 0), "0x009741fa"),
        (("pendingRequest", "initial", "fileNumber"), 0),
        (("pendingRequest", "fields", "priority"), "0x32d"),
        (("pendingRequest", "frameUpdateReads"), ["fileNumber", "index"]),
        (
            ("pendingRequest", "rule"),
            "A request replaces the held one when its priority is greater.",
        ),
        (("playPath", "divisor"), 1000),
        (("playPath", "categoryCount"), 21),
        (("playPath", "boundCheck"), "category below 22"),
        (("playPath", "playCall", "moduleSlot"), 5),
        (("install", "categories", 2, "files"), 424),
        (("install", "categories", 0, "base"), "0x035a0000"),
        (("install", "categories", 21, "lastIndex"), 1),
        (("install", "totalFiles"), 1610),
        (("install", "directoryFileCounts", "03/5B"), 84),
        (("install", "footstepBanks", "08/07"), 771),
        (("markup", "actionElements"), 1013),
        (("markup", "maxFileNumber"), 83),
        (("markup", "fileNumberCounts", 0, 1), 118),
        (("markup", "fileNumberCounts", 60, 0), 83),
        (("markup", "triggerKinds", "CommandTrigger"), 394),
        (("driverConfig", "maxSound"), 127),
        (("driverConfig", "buses", 1, "number"), 21),
        (("driverConfig", "stringendoFadeTimes", 3), 4000),
        (("driverConfig", "selection"), "Mode 4 selects the text at 0x00f542a0."),
        (("entries", 0, "sites", 0, "site"), "0x005527ed"),
        (("entries", 6, "sites", 10, "site"), "0x0060ec98"),
        (("directCallScans", 0, "sites"), ["0x0060ec97"]),
        (("directCallScans", 1, "sites"), []),
        (("entries", 3, "gate", "initialValue"), 0),
        (("messageSound", "drainFilter", "va"), "0x0058c320"),
        (("immediateScans", 15, "sites"), ["0x0049f011"]),
        (("entries", 0, "sites", 1, "owner"), " "),
        (
            ("entries", 1, "sites", 0),
            {"site": "0x00000000", "function": "0x00000000", "owner": "x", "kind": "x"},
        ),
        (("entryClasses", "play_footstep_pair", 0), "Zz"),
        (("literalIds", 0, "index"), 13),
        (("immediateScans", 0, "sites"), []),
        (("messageSound", "ids", 0, "resource"), "0x035b0038"),
        (("reachability", "dispatcher", "entriesReached"), ["0x005859f0"]),
        (("creationVoice", "appearancePhase", "entries", 0, 1), 30022),
        (("creationVoice", "listMenu", "groupCategories", "1", 0), 4),
        (("creationVoice", "listMenu", "rowGroupDwords", 0), 9),
        (("claims", 0, "tier"), "OBSERVED"),
        (("claims", claim_index(document, "bus-numbers"), "tier"), "STATIC"),
        (("claims", 3, "id"), "pending-rule"),
        (("claims", 5, "claim"), " "),
        (("playPath", "requestObjects", "soundEffectIndex"), 1),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x9b3424"),
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
        "pending request priority compare",
        "frame update priority clear",
        "number split magic divide",
        "category play first call",
        "message sound call",
        "creation voice random modulus",
        "footstep gate test",
        "message sound drain filter",
        "config mode zero test",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    voice = copy.deepcopy(document)
    for item in voice["staticChecks"]:
        if item["name"] == "creation voice id table":
            item["values"][6] = "0x5b2e001b"
    mutations.append(voice)
    bases = copy.deepcopy(document)
    for item in bases["staticChecks"]:
        if item["name"] == "category base table":
            item["values"][3] = "0x5b2e0000"
    mutations.append(bases)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def sites_of(data: bytes, value: int) -> list[int]:
    """VAs of a little-endian dword inside the executable section."""
    needle = value.to_bytes(4, "little")
    found = []
    for start, virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        if not (start <= 0x00401000 < start + max(virtual_size, raw_size)):
            continue
        end = raw_offset + raw_size
        at = data.find(needle, raw_offset, end)
        while at != -1:
            found.append(start + at - raw_offset)
            at = data.find(needle, at + 1, end)
    return found


def call_sites(data: bytes, targets: set[int]) -> dict[int, list[int]]:
    """VAs of E8 and E9 opcodes in the code section whose rel32 reaches a target."""
    found: dict[int, list[int]] = {target: [] for target in targets}
    for start, virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        if not (start <= 0x00401000 < start + max(virtual_size, raw_size)):
            continue
        end = raw_offset + raw_size - 4
        for match in re.finditer(rb"(?=[\xe8\xe9])", data[raw_offset:end], re.S):
            at = raw_offset + match.start()
            rel = int.from_bytes(data[at + 1 : at + 5], "little", signed=True)
            site = start + at - raw_offset
            target = (site + 5 + rel) & 0xFFFFFFFF
            if target in found:
                found[target].append(site)
    return found


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    errors = image_checks.verify_image(data, document, require_identity)
    if any("not a readable PE32" in error for error in errors):
        return errors
    wanted = {
        int(item["va"], 16): [
            s["site"] for s in item.get("sites", []) if isinstance(s, dict)
        ]
        for item in document.get("entries", [])
    }
    wanted.update(
        {
            int(item["target"], 16): item["sites"]
            for item in document.get("directCallScans", [])
        }
    )
    reached = call_sites(data, set(wanted))
    for target, sites in wanted.items():
        found = [hex8(site) for site in reached[target]]
        if found != sorted(sites):
            errors.append(f"direct call sites of {hex8(target)} are {found}")
    for scan in document.get("immediateScans", []):
        found = [hex8(site) for site in sites_of(data, int(scan["value"], 16))]
        if found != scan["sites"]:
            errors.append(f"immediate {scan['value']} sites are {found}")
    sections = image_checks.section_map(data)
    for section, sub, base_key, stride_key, values_key in STRIDED_TABLES:
        node = document.get(section, {}).get(sub, {})
        base = int(node.get(base_key, "0x0"), 16)
        stride = node.get(stride_key, 0)
        want = node.get(values_key, [])
        got = []
        for row in range(len(want)):
            raw = image_checks.read_va(data, sections, base + stride * row, 4)
            got.append(int.from_bytes(raw, "little") if raw else None)
        if got != want:
            errors.append(f"{values_key} read from the image is {got}")
    return errors


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def resource_path(resource: int) -> str:
    parts = [(resource >> shift) & 0xFF for shift in (24, 16, 8, 0)]
    return "data/" + "/".join(f"{part:02X}" for part in parts) + ".DAT"


def count_files(root: Path, *parts: str) -> int:
    base = root.joinpath("data", *parts)
    return sum(1 for p in base.rglob("*") if p.is_file() and p.name != ".DS_Store")


def census_category(root: Path, base: int) -> dict:
    """Files of one category: count, extreme indexes and SSCF containers."""
    found = []
    sscf = 0
    for index in range(CATEGORY_SPAN):
        path = root / resource_path(base + index)
        if path.is_file():
            found.append(index)
            with path.open("rb") as handle:
                sscf += handle.read(8) == b"SEDBSSCF"
    return {
        "files": len(found),
        "firstIndex": found[0] if found else None,
        "lastIndex": found[-1] if found else None,
        "sscfFiles": sscf,
    }


def verify_install(root: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors = []
    install = document["install"]
    for row in install["categories"]:
        got = census_category(root, int(row["base"], 16))
        for key, value in got.items():
            if row.get(key) != value:
                errors.append(f"category {row['category']} {key} is {value}")
    counts = install["directoryFileCounts"]
    found = {
        "03/5B": count_files(root, "03", "5B"),
        "07/FD": count_files(root, "07", "FD"),
        "07/FE": count_files(root, "07", "FE"),
        "5B/2D..5B/3E": sum(
            count_files(root, "5B", f"{x:02X}") for x in range(0x2D, 0x3F)
        ),
        "8A/0A": count_files(root, "8A", "0A"),
    }
    if found != counts:
        errors.append(f"directory file counts are {found}")
    steps = install["footstepBanks"]
    got_steps = {
        "08/07": count_files(root, "08", "07"),
        "1A/46": count_files(root, "1A", "46"),
    }
    if got_steps != steps:
        errors.append(f"footstep bank counts are {got_steps}")
    return errors


def trigger_kind(element: ET.Element, parents: dict) -> tuple[str, str]:
    """The nearest EventTrigger or CommandTrigger above an element and its command."""
    node = element
    while node in parents:
        node = parents[node]
        tag = node.tag.split("}")[-1]
        if tag in {"EventTrigger", "CommandTrigger"}:
            return tag, node.get("Command") or ""
    return "none", ""


def markup_counts(texts: dict[str, str]) -> dict:
    """SoundPlayerAction census of decoded Sqwt markup keyed by file name."""
    numbers: Counter = Counter()
    attrs: Counter = Counter()
    kinds: Counter = Counter()
    lua = 0
    with_action = 0
    for text in texts.values():
        found = ACTION_RE.findall(text)
        if not found:
            continue
        with_action += 1
        for body in found:
            pairs = dict(ATTR_RE.findall(body))
            numbers[int(pairs["FileNumber"])] += 1
            for key in ("Index", "Volume", "Priority"):
                attrs[key] += key in pairs
        try:
            root = ET.fromstring(text.lstrip(BOM))
        except (ET.ParseError, ValueError):
            kinds["unparsed"] += len(found)
            continue
        parents = {child: parent for parent in root.iter() for child in parent}
        for element in root.iter():
            if element.tag.split("}")[-1] != "SoundPlayerAction":
                continue
            kind, command = trigger_kind(element, parents)
            kinds[kind] += 1
            lua += kind == "CommandTrigger" and command.startswith(
                "UILuaCommands.SoundPlay"
            )
    return {
        "filesWithAction": with_action,
        "actionElements": sum(numbers.values()),
        "distinctFileNumbers": len(numbers),
        "minFileNumber": min(numbers) if numbers else None,
        "maxFileNumber": max(numbers) if numbers else None,
        "indexAttributeCount": attrs["Index"],
        "volumeAttributeCount": attrs["Volume"],
        "priorityAttributeCount": attrs["Priority"],
        "triggerKinds": dict(kinds),
        "luaSoundCommandTriggerElements": lua,
        "fileNumberCounts": [[k, numbers[k]] for k in sorted(numbers)],
    }


def decode_markup(root: Path, xivl: Path) -> tuple[dict[str, str], int]:
    """Decode every SQEX file below client/sqwt with xivl and return the texts."""
    texts: dict[str, str] = {}
    templates = 0
    base = root / "client" / "sqwt"
    with tempfile.TemporaryDirectory() as raw:
        scratch = Path(raw)
        for number, path in enumerate(
            sorted(p for p in base.rglob("*") if p.is_file())
        ):
            with path.open("rb") as handle:
                if handle.read(4) != b"SQEX":
                    continue
            out = scratch / str(number)
            subprocess.run(
                [
                    str(xivl),
                    "extract-resource",
                    str(path),
                    "--output",
                    str(out),
                    "--as",
                    "sqwt",
                ],
                check=True,
                capture_output=True,
            )
            decoded = out / "payloads" / "decoded.xml"
            texts[path.relative_to(base).as_posix()] = decoded.read_bytes().decode(
                "utf-8", "replace"
            )
            templates += path.suffix == ".tpl"
    return texts, templates


def compare_markup(texts: dict[str, str], templates: int, document: dict) -> list[str]:
    expected = document["markup"]
    got = markup_counts(texts)
    got["sqexFiles"] = len(texts)
    got["templateFilesDecoded"] = templates
    errors = []
    for key, value in got.items():
        if key == "triggerKinds":
            value = {k: v for k, v in value.items() if v}
        if expected.get(key) != value:
            errors.append(f"markup {key} is {value}")
    return errors


def verify_markup(root: Path, xivl: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    texts, templates = decode_markup(root, xivl)
    return compare_markup(texts, templates, document)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install",
        type=Path,
        help="game root whose data tree is compared with the bank counts",
    )
    parser.add_argument(
        "--xivl",
        type=Path,
        help="xivl executable that decodes client/sqwt below --install for the markup census",
    )
    args = parser.parse_args()
    if args.xivl and not args.install:
        parser.error("--xivl needs --install")
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
        if args.xivl:
            errors += verify_markup(args.install, args.xivl)
    if errors:
        print(f"FAIL: {len(errors)} sound effect trigger error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install banks"
    if args.xivl:
        suffix += " plus markup census"
    print(f"PASS: sound effect trigger manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
