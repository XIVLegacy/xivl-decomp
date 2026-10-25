#!/usr/bin/env python3
"""Validate the Sqwt Anime property contract, its reference model and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "sqwt_anime_property.json"
DOC = ROOT / "docs" / "resource" / "sqwt-anime-property.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
HEX8_RE = re.compile(r"^0x[0-9a-f]{8}$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
LABELS = {"STATIC", "INFERRED", "UNKNOWN", "CALIBRATION"}
PREFIX = "ApplicationCommands."
SHOW = "Show"
SHOW_INACTIVE = "ShowInactive"
VISIBILITY = {"Collapsed": 0, "Hidden": 1, "Visible": 2}
COMMANDS = [
    ("Show", 0x0106A37C, 0x01357648),
    ("ShowInactive", 0x0106A398, 0x013576E8),
    ("Hide", 0x0106A3BC, 0x01357788),
    ("Close", 0x0106A3D8, 0x01357828),
    ("Copy", 0x0106A3F4, 0x013578C8),
    ("MoveFocus", 0x0106A410, 0x01357968),
    ("MoveWindow", 0x0106A430, 0x01357A08),
    ("ResizeWindow", 0x0106A450, 0x01357AA8),
    ("DestroyWindow", 0x0106A474, 0x01357B48),
    ("InvisibleWindow", 0x0106A498, 0x01357BE8),
    ("Operate", 0x0106A4BC, 0x01357C88),
    ("OperateRepeat", 0x0106A4D8, 0x01357D28),
    ("NotMovedFocus", 0x0106A4FC, 0x01357DC8),
    ("SetProperty", 0x0106A520, 0x01357E68),
    ("GetProperty", 0x0106A540, 0x01357F08),
    ("ShowWindow", 0x0106A560, 0x01357FA8),
    ("Focus", 0x0106A580, 0x01358048),
    ("ImeOff", 0x0106A59C, 0x013580E8),
    ("ShowHelp", 0x0106A5B8, 0x01358188),
    ("CloseHelp", 0x0106A5E8, 0x013582C8),
    ("MoveHelp", 0x0106A608, 0x01358368),
    ("FocusOrderToBottom", 0x0106A628, 0x01358408),
    ("Null", 0x0106A688, 0x01358688),
]
REGISTERED = [name for name, _, _ in COMMANDS]
HANDLERS = {
    "Show": (0x01357648, 0x0092635E),
    "ShowInactive": (0x013576E8, 0x0092638E),
    "Close": (0x01357828, 0x0092641C),
    "MoveWindow": (0x01357A08, 0x00926446),
    "ResizeWindow": (0x01357AA8, 0x00926554),
    "Hide": (0x01357788, 0x00926662),
    "DestroyWindow": (0x01357B48, 0x00926700),
    "InvisibleWindow": (0x01357BE8, 0x0092675C),
    "ShowWindow": (0x01357FA8, 0x009267A1),
}
EXPECTED_SETTER = {
    "va": "0x00537de0",
    "receiverClass": "Application::Main::Element::FormElement",
    "receiverVtableVa": "0x00fa1200",
    "stackArguments": 4,
    "arguments": ["part name", "property name", "value string", "request id"],
    "partGate": {
        "recordLookupVa": "0x00535690",
        "partLookupVa": "0x00537950",
        "zeroIdRecordField": "0x2c",
    },
    "nameMatch": {
        "literal": "Anime",
        "literalVa": "0x00fa06e8",
        "idStaticVa": "0x01336b1c",
        "idFieldVa": "0x01336b24",
        "internVa": "0x009108e0",
        "hashVa": "0x009107a0",
        "upperVa": "0x009db9ed",
        "hash": "CRC-32 of the upper-cased bytes",
    },
    "keySwap": {
        "literal": "UIElement.Execute",
        "literalVa": "0x00fa06f0",
        "staticVa": "0x01336b08",
    },
    "rewrite": {
        "from": "Show",
        "fromVa": "0x00fa0704",
        "to": "ShowInactive",
        "toVa": "0x00fa070c",
        "equalsVa": "0x00445d60",
        "assignVa": "0x004489c0",
    },
    "gate": {
        "receiverOffset": "0x9c",
        "passValue": 1,
        "literal": "ShowInactive",
        "literalVa": "0x00fa071c",
        "differsVa": "0x00445e00",
        "scope": "every property",
    },
    "engine": {"va": "0x00912850", "flag": 0},
}
EXPECTED_PICKUP = {
    "builderVa": "0x005354d0",
    "commandConstructorVa": "0x0053bc30",
    "processorVtableVa": "0x00fa0bac",
    "processorRtti": "Application::Main::Element::SetPropertyProcessor",
}
EXPECTED_SLOTS = [
    (1, "0x005425b0", "int", "%d"),
    (2, "0x00542680", "bool", "True or False"),
    (3, "0x00542710", "string", "as given"),
    (4, "0x00542730", "one float", "%f"),
    (5, "0x00542800", "two floats", "%f,%f"),
    (6, "0x005428f0", "four floats", "%f,%f,%f,%f"),
]
EXPECTED_RECEIVER = {
    "offset": "0x9c",
    "initial": 1,
    "constructorVa": "0x0053af60",
    "storeVa": "0x0053afd9",
    "vtableVa": "0x00fa1200",
}
EXPECTED_EXECUTE = {
    "nameVa": "0x010783b4",
    "descriptorVa": "0x0135ff80",
    "initVa": "0x00f275c0",
    "infoVa": "0x0135ffa4",
    "infoTableVa": "0x01078610",
    "getterVa": "0x0095e690",
    "setterVa": "0x0095f6b0",
    "registrationVa": "0x00991fdb",
    "storeVa": "0x0095e500",
    "differsVa": "0x00445db0",
    "wrapperVa": "0x0095e830",
    "resolverVa": "0x00916310",
    "sentinelVa": "0x01356da8",
    "referenceMask": "0x40",
    "changedMask": "0x04",
    "sentinelMask": "0x10",
    "precedenceMasks": ["0x01", "0x02"],
    "assignVa": "0x009364d0",
    "notifyVa": "0x0095dce0",
    "thunkVa": "0x0095d890",
    "callbackVa": "0x0092df20",
    "changeHandlerVa": "0x0093bd80",
    "default": "empty string",
}
EXPECTED_DISPATCH = {
    "prefix": "ApplicationCommands.",
    "prefixVa": "0x0106c200",
    "joinVa": "0x0044ab90",
    "lookupVa": "0x00928280",
    "registryVa": "0x01358728",
    "nameField": "0x24",
    "equalsVa": "0x00445d20",
    "recordBuilderVa": "0x004ddcf0",
    "recordArgument": 0,
    "commandSlot": 2,
    "commandSlotVa": "0x00927fa0",
    "commandRtti": "Sqwt::Input::RoutedCommand",
    "targetOffset": "0xb4",
}
EXPECTED_EVENTS = {
    "args": {
        "constructorVa": "0x00929100",
        "handledPointer": "0x10",
        "handledByte": "0x19",
        "command": "0x1c",
        "recordInteger0": "0x7c",
        "text": "0x28",
    },
    "preview": {
        "name": "PreviewExecuted",
        "id": "0x29",
        "strategy": 2,
        "static": "0x0135872c",
    },
    "executed": {
        "name": "Executed",
        "id": "0x2a",
        "strategy": 0,
        "static": "0x01358738",
    },
    "shown": {"id": "0x47", "static": "0x013573b0"},
    "hiding": {"id": "0x49", "static": "0x013573c8"},
}
EXPECTED_WINDOW = {
    "className": "Sqwt::Window",
    "secondaryOffset": "0xb4",
    "vtableVa": "0x01069f94",
    "executedSlot": 65,
    "executedOffset": "0x104",
    "handlerVa": "0x00926320",
    "baseHandlerVa": "0x0092d1f0",
    "dispatchVa": "0x0091d320",
    "dispatchJumpTableVa": "0x0091d760",
    "dispatchByteTableVa": "0x0091d818",
    "forwardVa": "0x00922030",
    "chain": [
        "0x00922030",
        "0x00935380",
        "0x00938ba0",
        "0x0092d060",
        "0x00939800",
        "0x0091d320",
    ],
}
EXPECTED_SHOW = {
    "va": "0x00925aa0",
    "arguments": ["bookkeeping argument", "always raise Shown", "Visible"],
    "show": [1, 0, 1],
    "showInactive": [1, 1, 1],
    "visibleKind": 2,
    "hiddenKind": 1,
    "bookkeepingVa": "0x009237d0",
    "counterOffset": "0x43c",
    "isVisibleSlot": "0x5c",
    "visibilitySetterVa": "0x0093c2a0",
    "shownRaiseVa": "0x00925b85",
    "windowManagerVa": "0x01356e10",
}
EXPECTED_HIDE = {
    "va": "0x00921f30",
    "allowFlagOffset": "0x135",
    "allowFlagMask": 1,
    "cancelByteOffset": "0x1c",
    "hiddenKind": 1,
    "resultPointerOffset": "0x430",
    "virtualOffset": "0x134",
}
EXPECTED_VISIBILITY = {
    "descriptorVa": "0x013596bc",
    "nameVa": "0x0106beb8",
    "byteOffset": "0x158",
    "values": VISIBILITY,
    "isVisibleVa": "0x00939aa0",
    "rootVa": "0x013597f8",
    "parserVa": "0x00914580",
    "printerVa": "0x009145f0",
    "setterVa": "0x0093c2a0",
}
EXPECTED_LOG = {
    "globalVa": "0x0135708c",
    "name": "Window_LogWidget",
    "matchVa": "0x009d6090",
    "readers": ["0x009263bc", "0x00926690"],
    "writers": [],
    "initial": 0,
    "helpers": ["0x0091dec0", "0x0093c2a0"],
}
EXPECTED_IDS = {"Anime": "0xe4509bf2", "UIElement.Execute": "0xa169fd0a"}
REQUIRED_CHECKS = {
    "setter function",
    "setter lookups and part gate",
    "setter Anime name interning",
    "setter Anime id compare",
    "setter Execute name interning",
    "setter Show rewrite",
    "setter receiver gate and engine call",
    "string Anime",
    "string UIElement.Execute setter",
    "string Show",
    "string ShowInactive rewrite",
    "string ShowInactive gate",
    "string ApplicationCommands prefix",
    "string Window_LogWidget show",
    "string Window_LogWidget hide",
    "string Visibility",
    "FormElement constructor flag store",
    "pickup processor table",
    "string pickup forwards to setter",
    "command builder and pickup call",
    "name interner",
    "upper case helper",
    "string equals c string",
    "string differs from c string",
    "string differs from string",
    "string assign c string",
    "Execute name interning",
    "Execute info table install",
    "Execute registration",
    "Execute info table",
    "Execute info setter",
    "string property store",
    "Execute value compare and notify",
    "Execute info setter argument exit",
    "Execute info setter calls wrapper",
    "Execute store wrapper",
    "Execute store wrapper reference branch",
    "Execute store wrapper sentinel exit",
    "Execute store wrapper calls store",
    "Execute store wrapper plain path calls store",
    "string property store precedence exit",
    "RoutedCommand Executed skipped when handled",
    "hide routine virtual delivery",
    "Execute change thunk",
    "UIElement callback calls change handler",
    "change handler Execute descriptor test",
    "change handler command prefix",
    "change handler prefix join",
    "change handler lookup and execute",
    "command registry lookup",
    "command registry compare",
    "RoutedCommand table",
    "RoutedCommand PreviewExecuted raise",
    "RoutedCommand Executed raise",
    "event args constructor",
    "event args handled flag",
    "event args command store",
    "routed event Shown",
    "routed event Hiding",
    "routed event Executed",
    "routed event PreviewExecuted",
    "Window Executed slot",
    "Window IsVisible slot",
    "Window slot 13 calls base chain",
    "Window base calls element chain",
    "element chain calls FrameworkElement slot 13",
    "FrameworkElement slot 13 calls base",
    "dispatcher indexes by event id",
    "dispatcher Executed arm",
    "dispatcher byte for Executed",
    "dispatcher jump entry for Executed",
    "window Executed handler",
    "handler base call and handled test",
    "handler Show compare",
    "handler Show action",
    "handler ShowInactive test",
    "handler ShowInactive log widget path",
    "handler ShowInactive normal path",
    "handler Close",
    "handler MoveWindow compare",
    "handler ResizeWindow compare",
    "handler Hide test",
    "handler Hide log widget path",
    "handler Hide normal path",
    "handler DestroyWindow compare",
    "handler InvisibleWindow",
    "handler ShowWindow",
    "show routine",
    "show routine prior visibility",
    "show routine visibility write",
    "show routine bookkeeping call",
    "show routine Shown decision",
    "show routine Shown raise",
    "hide routine flag and Hiding event",
    "hide routine Hidden write",
    "is visible slot",
    "Visibility name interning",
    "Visibility setter",
    "Visibility word parser",
    "Visibility word printer",
    "absolute references to log widget global",
    "log widget global is zero filled",
    "command init Show",
    "command init ShowInactive",
    "command init Hide",
    "command init Close",
    "command init MoveWindow",
    "command init ResizeWindow",
    "command init DestroyWindow",
    "command init InvisibleWindow",
    "command init ShowWindow",
    "Null command init",
}
STATIC_KINDS = {
    "signature",
    "dwords",
    "string",
    "rangeSha256",
    "routedEvent",
    "absoluteRefs",
    "zeroFill",
}


def name_id(name: str) -> int:
    """Model of 0x009107A0 for ASCII names: CRC-32 over the bytes with a-z folded."""
    return zlib.crc32(name.encode("latin-1").upper()) & 0xFFFFFFFF


def rewrite_value(value: str) -> str:
    """Model of 0x00537F13..0x00537F36: the exact text Show becomes ShowInactive."""
    return SHOW_INACTIVE if value == SHOW else value


def gate_passes(receiver_byte: int, value: str) -> bool:
    """Model of 0x00537F3B..0x00537F53 for the rewritten value."""
    return receiver_byte == 1 or value != SHOW_INACTIVE


def command_exists(value: str) -> bool:
    """Model of 0x00928280 for the name ApplicationCommands. plus the value."""
    return value in REGISTERED


def window_effect(
    command: str,
    visibility: str,
    *,
    hide_allowed: bool = True,
    cancel_hiding: bool = False,
    log_widget: bool = False,
    log_global: int = 0,
    ancestors_visible: bool = True,
) -> tuple[str, list[str]]:
    """Model of the Window Executed handler 0x00926320 for the traced commands."""
    events: list[str] = []
    skip_shown = log_widget and log_global == 1
    if command == "Show":
        was_visible = visibility == "Visible" and ancestors_visible
        visibility = "Visible"
        if ancestors_visible and not was_visible:
            events.append("Shown")
    elif command == "ShowInactive":
        visibility = "Visible"
        if not skip_shown:
            events.append("Shown")
    elif command == "Hide":
        if skip_shown:
            visibility = "Hidden"
        elif hide_allowed:
            events.append("Hiding")
            if not cancel_hiding:
                visibility = "Hidden"
    elif command == "InvisibleWindow":
        visibility = "Hidden"
    return visibility, events


def anime_step(
    state: dict,
    value: str,
    *,
    receiver_byte: int = 1,
    hide_allowed: bool = True,
    cancel_hiding: bool = False,
    log_widget: bool = False,
    log_global: int = 0,
    store_refused: bool = False,
    preview_handled: bool = False,
) -> tuple[dict, list[str]]:
    """One _setProperty(part, "Anime", value): the latch, the lookup and the effect."""
    word = rewrite_value(value)
    if not gate_passes(receiver_byte, word):
        return dict(state), []
    if store_refused or word == state["latch"]:
        return dict(state), []
    after = {"latch": word, "visibility": state["visibility"]}
    if not command_exists(word) or preview_handled:
        return after, []
    after["visibility"], events = window_effect(
        word,
        state["visibility"],
        hide_allowed=hide_allowed,
        cancel_hiding=cancel_hiding,
        log_widget=log_widget,
        log_global=log_global,
    )
    return after, events


def run_example(example: dict) -> tuple[dict, list[str]]:
    state = dict(example["start"])
    events: list[str] = []
    for step in example["steps"]:
        state, raised = anime_step(
            state,
            step["value"],
            receiver_byte=example.get("receiverByte", 1),
            hide_allowed=example.get("hideAllowed", True),
            cancel_hiding=example.get("cancelHiding", False),
            log_widget=example.get("logWidget", False),
            log_global=example.get("logGlobal", 0),
            store_refused=example.get("storeRefused", False),
            preview_handled=example.get("previewHandled", False),
        )
        events += raised
    return state, events


def parse_hex(text: str) -> int:
    return int(text, 16)


def flatten(text: str) -> str:
    """Drop code ticks and collapse whitespace so a page may wrap and format a line."""
    return " ".join(text.replace("`", "").split())


def page_rows(document: dict) -> list[str]:
    """Table rows of the page that restate manifest values."""
    rows = []
    for name, (obj, compare) in HANDLERS.items():
        found = next(
            (item for item in document.get("handlers", []) if item["command"] == name),
            None,
        )
        if found is not None:
            rows.append(
                f"| {name} | 0x{obj:08X} | 0x{compare:08X} | {found['effect']} |"
            )
    for item in document.get("commands", []):
        rows.append(
            f"| {item['name']} | 0x{parse_hex(item['objectVa']):08X} | "
            f"0x{parse_hex(item['nameVa']):08X} |"
        )
    for word, value in document.get("visibility", {}).get("values", {}).items():
        rows.append(f"| {word} | {value} |")
    for item in document.get("pickup", {}).get("slots", []):
        rows.append(
            f"| {item['slot']} | 0x{parse_hex(item['va']):08X} | "
            f"{item['type']} | {item['format']} |"
        )
    return rows


def doc_errors(document: dict, text: str | None = None) -> list[str]:
    if text is None:
        if not DOC.exists():
            return ["page is missing"]
        text = DOC.read_text(encoding="ascii")
    errors: list[str] = []
    flat = flatten(text)
    for number, line in enumerate(document.get("contract", []), 1):
        if f"{number}. {flatten(line)}" not in flat:
            errors.append(f"page lacks contract line {number}")
    for row in page_rows(document):
        if row not in flat:
            errors.append(f"page lacks table row {row[:50]!r}")
    for label in LABELS:
        if label not in text:
            errors.append(f"page lacks the {label} label")
    return errors


def compare(errors: list[str], where: str, found: object, expected: dict) -> None:
    for key, value in expected.items():
        if not isinstance(found, dict) or found.get(key) != value:
            errors.append(f"{where} {key} changed")


def model_errors(document: dict) -> list[str]:
    errors: list[str] = []
    ids = document.get("nameIds", {})
    for name, wanted in EXPECTED_IDS.items():
        if ids.get(name) != wanted or f"0x{name_id(name):08x}" != wanted:
            errors.append(f"name id of {name} changed")
    if not name_id("anime") == name_id("ANIME") == name_id("Anime"):
        errors.append("name id is case sensitive")
    examples = document.get("examples", [])
    if len(examples) != 16:
        errors.append("examples changed")
    for item in examples:
        state, events = run_example(item)
        if (
            state.get("latch") != item.get("latch")
            or state.get("visibility") != item.get("visibility")
            or events != item.get("events")
        ):
            errors.append(f"example {item.get('name')!r} differs")
    return errors


def verify(document: dict | None = None, text: str | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors: list[str] = []
    if document.get("format") != 1:
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != EXPECTED_SHA256
        or source.get("size") != EXPECTED_SIZE
        or source.get("imageBase") != "0x00400000"
    ):
        errors.append("source identity changed")
    setter = document.get("setter", {})
    for key, expected in EXPECTED_SETTER.items():
        if isinstance(expected, dict):
            compare(errors, f"setter {key}", setter.get(key), expected)
        elif setter.get(key) != expected:
            errors.append(f"setter {key} changed")
    pickup = document.get("pickup", {})
    compare(errors, "pickup", pickup, EXPECTED_PICKUP)
    found_slots = [
        (s.get("slot"), s.get("va"), s.get("type"), s.get("format"))
        for s in pickup.get("slots", [])
    ]
    if found_slots != EXPECTED_SLOTS:
        errors.append("pickup slots changed")
    compare(errors, "receiver flag", document.get("receiverFlag"), EXPECTED_RECEIVER)
    compare(errors, "execute", document.get("executeProperty"), EXPECTED_EXECUTE)
    compare(errors, "dispatch", document.get("dispatch"), EXPECTED_DISPATCH)
    events = document.get("events", {})
    for key, expected in EXPECTED_EVENTS.items():
        compare(errors, f"events {key}", events.get(key), expected)
    compare(errors, "window", document.get("window"), EXPECTED_WINDOW)
    compare(errors, "show routine", document.get("showRoutine"), EXPECTED_SHOW)
    compare(errors, "hide routine", document.get("hideRoutine"), EXPECTED_HIDE)
    compare(errors, "visibility", document.get("visibility"), EXPECTED_VISIBILITY)
    compare(errors, "log widget", document.get("logWidget"), EXPECTED_LOG)
    found_commands = [
        (c.get("name"), parse_hex(c.get("nameVa", "0x0")), parse_hex(c["objectVa"]))
        for c in document.get("commands", [])
        if HEX8_RE.match(str(c.get("objectVa", "")))
    ]
    if found_commands != COMMANDS or len(document.get("commands", [])) != 23:
        errors.append("commands changed")
    found_handlers = {
        h.get("command"): (parse_hex(h["objectVa"]), parse_hex(h["compareVa"]))
        for h in document.get("handlers", [])
        if HEX8_RE.match(str(h.get("objectVa", "")))
        and HEX8_RE.match(str(h.get("compareVa", "")))
    }
    if found_handlers != HANDLERS:
        errors.append("handlers changed")
    for item in document.get("handlers", []):
        if item["command"] not in REGISTERED:
            errors.append(f"handler {item['command']} is not a registered command")
    errors.extend(model_errors(document))
    contract = document.get("contract", [])
    if (
        len(contract) != 12
        or "CRC-32 of its upper-cased bytes" not in contract[1]
        or "exactly Show, byte for byte" not in contract[2]
        or "latched Execute string" not in contract[3]
        or "refuse the write before the compare" not in contract[3]
        or "matched byte for byte" not in contract[4]
        or "empty parameter record" not in contract[5]
        or "marks it handled" not in contract[5]
        or "every time" not in contract[6]
        or "When no handler cancels it" not in contract[7]
        or "raise nothing" not in contract[8]
        or "Anime never produces this command" not in contract[9]
        or "neither Shown nor Hiding" not in contract[10]
    ):
        errors.append("contract changed")
    questions = document.get("open", [])
    if len(questions) != 9 or not all(
        item.get("label") in LABELS - {"STATIC"} for item in questions
    ):
        errors.append("open questions changed")
    if [item["label"] for item in questions].count("CALIBRATION") != 1:
        errors.append("calibration question count changed")
    errors.extend(static_check_errors(document))
    errors.extend(doc_errors(document, text))
    dump = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (
        r"[A-Za-z]:\\\\[A-Za-z]",
        "/" + "Users/",
        "/" + "home/",
        "agent-islands",
    )
    if any(re.search(pattern, dump, re.I) for pattern in forbidden):
        errors.append("manifest contains a private path")
    return errors


def static_check_errors(document: dict) -> list[str]:
    errors: list[str] = []
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        return ["staticChecks is not a list"]
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    if REQUIRED_CHECKS - set(names):
        errors.append(f"staticChecks lacks {sorted(REQUIRED_CHECKS - set(names))}")
    by_name = {item.get("name"): item for item in checks if isinstance(item, dict)}
    for item in checks:
        kind = item.get("kind") if isinstance(item, dict) else None
        if kind not in STATIC_KINDS:
            errors.append("staticChecks entry has an unknown kind")
        elif kind == "absoluteRefs":
            sites = item.get("sites", [])
            if not (
                HEX8_RE.match(str(item.get("value", "")))
                and sites
                and all(HEX8_RE.match(str(site)) for site in sites)
            ):
                errors.append(f"staticChecks {item.get('name')!r} reference row")
        elif not HEX8_RE.match(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif kind == "signature":
            try:
                same = len(bytes.fromhex(item["pattern"])) == len(
                    bytes.fromhex(item["mask"])
                )
            except (KeyError, ValueError):
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
        elif kind == "rangeSha256" and not (
            SHA_RE.match(str(item.get("sha256", "")))
            and isinstance(item.get("length"), int)
            and item["length"] > 0
        ):
            errors.append(f"staticChecks {item['name']!r} hash row invalid")
    for name, text in {
        "string Anime": "Anime",
        "string UIElement.Execute setter": "UIElement.Execute",
        "string UIElement.Execute registry": "UIElement.Execute",
        "string Show": "Show",
        "string ShowInactive rewrite": "ShowInactive",
        "string ShowInactive gate": "ShowInactive",
        "string ApplicationCommands prefix": "ApplicationCommands.",
        "string Window_LogWidget show": "Window_LogWidget",
        "string Window_LogWidget hide": "Window_LogWidget",
        "string Visibility": "Visibility",
        "string Collapsed word": "Collapsed",
        "string Hidden word": "Hidden",
        "string False word": "False",
        "string Visible word": "Visible",
        "string Shown": "Shown",
        "string Hiding": "Hiding",
    }.items():
        if by_name.get(name, {}).get("text") != text:
            errors.append(f"string row {name!r} changed")
    for name, obj_name, va in [
        (f"string command {n}", n, s) for n, s, _ in COMMANDS if n != "Null"
    ] + [("string command Null", "Null", 0x0106A688)]:
        row = by_name.get(name, {})
        if row.get("text") != PREFIX + obj_name or row.get("va") != f"0x{va:08x}":
            errors.append(f"string row {name!r} changed")
    wanted_table = [0x0053BD00, 0x005425B0, 0x00542680, 0x00542710, 0x00542730]
    wanted_table += [0x00542800, 0x005428F0]
    table = by_name.get("pickup processor table", {})
    if table.get("va") != "0x00fa0bac" or table.get("values") != [
        f"0x{v:08x}" for v in wanted_table
    ]:
        errors.append("pickup processor table changed")
    slot = by_name.get("Window Executed slot", {})
    if slot.get("va") != "0x0106a098" or slot.get("values") != ["0x00926320"]:
        errors.append("Window Executed slot changed")
    slot = by_name.get("Window IsVisible slot", {})
    if slot.get("va") != "0x01069ff0" or slot.get("values") != ["0x00939aa0"]:
        errors.append("Window IsVisible slot changed")
    refs = by_name.get("absolute references to log widget global", {})
    if refs.get("value") != "0x0135708c" or refs.get("sites") != [
        "0x009263bc",
        "0x00926690",
    ]:
        errors.append("log widget references changed")
    for name, (event_id, strategy, static, event_name) in {
        "routed event Shown": ("0x47", 0, "0x013573b0", "Shown"),
        "routed event Hiding": ("0x49", 0, "0x013573c8", "Hiding"),
        "routed event Executed": ("0x2a", 0, "0x01358738", "Executed"),
        "routed event PreviewExecuted": ("0x29", 2, "0x0135872c", "PreviewExecuted"),
    }.items():
        row = by_name.get(name, {})
        if (
            row.get("id"),
            row.get("strategy"),
            row.get("static"),
            row.get("eventName"),
        ) != (event_id, strategy, static, event_name):
            errors.append(f"routed event row {name!r} changed")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("setter", "gate", "receiverOffset"), "0x98"),
        (("setter", "gate", "passValue"), 0),
        (("setter", "gate", "scope"), "Anime only"),
        (("setter", "rewrite", "to"), "Show"),
        (("setter", "rewrite", "from"), "ShowInactive"),
        (("setter", "rewrite", "equalsVa"), "0x00445d20"),
        (("setter", "keySwap", "literal"), "Anime"),
        (("setter", "nameMatch", "hash"), "string compare"),
        (("setter", "nameMatch", "idFieldVa"), "0x01336b28"),
        (("setter", "stackArguments"), 3),
        (("setter", "engine", "flag"), 1),
        (("setter", "partGate", "zeroIdRecordField"), "0x30"),
        (("pickup", "slots", 2, "va"), "0x00542730"),
        (("pickup", "processorVtableVa"), "0x00fa0b74"),
        (("receiverFlag", "initial"), 0),
        (("receiverFlag", "storeVa"), "0x0053afd8"),
        (("executeProperty", "differsVa"), "0x00445d20"),
        (("executeProperty", "default"), "Show"),
        (("executeProperty", "descriptorVa"), "0x0135ff88"),
        (("dispatch", "prefix"), "RaptureCommands."),
        (("dispatch", "commandSlot"), 3),
        (("dispatch", "equalsVa"), "0x00445d60"),
        (("dispatch", "recordArgument"), 1),
        (("events", "args", "handledByte"), "0x18"),
        (("events", "executed", "strategy"), 2),
        (("events", "shown", "id"), "0x48"),
        (("window", "executedSlot"), 57),
        (("window", "chain", 2), "0x00938ba1"),
        (("window", "handlerVa"), "0x00926321"),
        (("showRoutine", "show"), [1, 1, 1]),
        (("showRoutine", "showInactive"), [1, 0, 1]),
        (("showRoutine", "visibleKind"), 1),
        (("hideRoutine", "allowFlagMask"), 2),
        (("visibility", "values", "Visible"), 0),
        (("visibility", "values", "Hidden"), 2),
        (("visibility", "byteOffset"), "0x15c"),
        (("logWidget", "initial"), 1),
        (("logWidget", "writers"), ["0x00900000"]),
        (("logWidget", "readers", 0), "0x009263bd"),
        (("nameIds", "Anime"), "0xe4509bf3"),
        (("commands", 1, "objectVa"), "0x01357648"),
        (("commands", 22, "objectVa"), "0x01358648"),
        (("handlers", 0, "compareVa"), "0x0092635f"),
        (("handlers", 5, "command"), "Show"),
        (("examples", 1, "events"), ["Shown"]),
        (("examples", 3, "visibility"), "Hidden"),
        (("examples", 11, "events"), ["Shown"]),
        (("examples", 13, "latch"), "ShowInactive"),
        (("examples", 14, "latch"), "ShowInactive"),
        (("examples", 15, "visibility"), "Visible"),
        (("executeProperty", "wrapperVa"), "0x0095e500"),
        (("executeProperty", "precedenceMasks"), ["0x01", "0x04"]),
        (("executeProperty", "referenceMask"), "0x04"),
        (("hideRoutine", "resultPointerOffset"), "0x434"),
        (("hideRoutine", "virtualOffset"), "0x138"),
        (("contract", 3), "Each part holds no latch."),
        (("contract", 6), "ShowInactive: raise Shown only when the window was hidden."),
        (
            ("contract", 5),
            "The command executes on the part with an empty parameter record.",
        ),
        (("contract", 10), "A _setProperty of Visibility raises Shown."),
        (("open", 8, "label"), "UNKNOWN"),
        (("source", "binarySha256"), "0" * 64),
    ]:
        changed = copy.deepcopy(document)
        cursor = changed
        for key in path[:-1]:
            cursor = cursor[key]
        cursor[path[-1]] = value
        mutations.append(changed)
    for name in sorted(REQUIRED_CHECKS):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    leaked = copy.deepcopy(document)
    leaked["source"]["tools"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    results = [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]
    if verify(document, "") == []:
        results.append("an empty page was accepted")
    return results


def run_check(data: bytes, sections: list, document: dict, check: dict) -> str | None:
    kind = check["kind"]
    if kind == "absoluteRefs":
        needle = struct.pack("<I", parse_hex(check["value"]))
        found: list[int] = []
        for start, _, raw_offset, raw_size in sections:
            blob = data[raw_offset : raw_offset + raw_size]
            index = blob.find(needle)
            while index >= 0:
                found.append(start + index)
                index = blob.find(needle, index + 1)
        wanted = sorted(parse_hex(site) for site in check["sites"])
        if sorted(found) != wanted:
            return f"references to {check['value']} are {[hex(v) for v in found]}"
        return None
    va = parse_hex(check["va"])
    if kind == "zeroFill":
        for start, virtual_size, _, raw_size in sections:
            if start <= va < start + virtual_size:
                if va - start < raw_size:
                    return f"{check['va']} is backed by file bytes"
                return None
        return f"{check['va']} is outside the image"
    if kind == "rangeSha256":
        raw = image_checks.read_va(data, sections, va, check["length"])
        if raw is None:
            return f"{check['va']} is outside the image"
        if hashlib.sha256(raw).hexdigest() != check["sha256"]:
            return f"range {check['name']!r} at {check['va']} differs"
        return None
    return image_checks.run_static_check(data, sections, check)


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
    for check in document["staticChecks"]:
        problem = run_check(data, sections, document, check)
        if problem:
            errors.append(problem)
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
        print(f"FAIL: {len(errors)} sqwt anime property error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: sqwt anime property manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
