#!/usr/bin/env python3
"""Validate the Sqwt command-parameter consumer contract and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "sqwt_command_parameter_consumer.json"
DOC = ROOT / "docs" / "resource" / "sqwt-command-parameter-consumer.md"
EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
EXPECTED_SIZE = 15_996_808
IMAGE_BASE = 0x00400000
HEX_RE = re.compile(r"^0x[0-9a-f]{8}$")
EXPECTED_VARIANTS = [
    ([0], "UICommandEventParameter", [], 9, 8),
    ([1], "UICommandEventParameterWithInt", ["+0x7c"], 11, 10),
    ([2, 3], "UICommandEventParameterWithTwoInt", ["+0x7c", "+0x80"], 15, 14),
    ([8], "UICommandEventParameterWithString", ["&+0x28"], 13, 12),
]
EXPECTED_DEBOUNCED = [
    "UILuaCommands.Operate",
    "UILuaCommands.Cancel",
    "UILuaCommands.WidgetClose",
    "UILuaCommands.Selection",
    "UILuaCommands.SelectionChanged",
    "UILuaCommands.SelectionChangedOnce",
]


REQUIRED_CHECKS = {
    f"thunk slot {slot} {side} {kind}"
    for slot, side, kind in (
        (8, "child", "none"),
        (9, "root", "none"),
        (10, "child", "int"),
        (11, "root", "int"),
        (12, "child", "string"),
        (13, "root", "string"),
        (14, "child", "two ints"),
        (15, "root", "two ints"),
    )
} | {
    "condition object byte stores",
    "parameter base layout",
    "event priority compare",
    "request depth limit",
    "selector drop gates",
    "pad one shot gate",
    "root command special case",
    "unnamed event 0x59 initializer",
    "Executed event initializer",
    "route traversal strategy branches",
    "bubble traversal from the list end",
    "ExecuteCommandAction target selection",
    "event id to virtual switch",
    "Executed arm calls virtual slot 65",
    "virtual slot 65 base dispatch",
    "handler store dispatcher match",
    "handler store dispatcher handled filter and invoke",
    "handler invoke thunk",
    "handler event and handled-events fields",
    "remove before add",
    "gate dispatch before the lookup",
    "gate creation",
    "gate class selector name slice",
    "NumberInputBox command handler",
    "one integer record builder zeroes the rest",
    "routed event Executed",
    "routed event unnamed 0x59",
}
EXPECTED_ROUTED = {
    "Executed": (0x2A, 0, 0x01358738),
    "PreviewExecuted": (0x29, 2, 0x0135872C),
    "LocationChanged": (0x48, 0, 0x013573BC),
    "RenderSizeChanged": (0x4B, 0, 0x013594B4),
}
# Event descriptor initializer: push the name (or push 0), then stores of the
# id and the routing strategy into the stack arguments of the constructor call.
ROUTED_NAMED = re.compile(
    rb"\x68(....)\x51\x8b\xc4\x51\xc6\x00(.)\x8b\xc4\xb9(....)\xc6\x00(.)\xe8", re.S
)
ROUTED_UNNAMED = re.compile(
    rb"\x6a\x00\x51\x8b\xc4\x51\xc6\x00(.)\x8b\xc4\xb9(....)\xc6\x00(.)\xe8", re.S
)


EVENT_RELEASE_CHECKS = [
    {
        "kind": "signature",
        "name": "event priority compare",
        "va": "0x00896b2a",
        "pattern": "3B FB 0F 84 81 00 00 00 8A 81 00 01 00 00 3A 87 00 01 00 00 77 07 32 C0",
        "mask": "FF FF FF FF 00 00 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF 00 FF FF",
    },
    {
        "kind": "signature",
        "name": "event holder constructor call",
        "va": "0x006e37cb",
        "pattern": "E8 E0 19 1B 00",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event holder control store",
        "va": "0x006e37ef",
        "pattern": "89 5F 60",
        "mask": "FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event holder vtable store",
        "va": "0x008951e5",
        "pattern": "C7 06 E0 6F 05 01",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event saved pending priority compare",
        "va": "0x00896b47",
        "pattern": "8B 7E 08 3B FB 74 08 3A 87 00 01 00 00 76 EA",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener registration",
        "va": "0x00cd0940",
        "pattern": "8B 44 24 10 56 8B F1 8B 0E 50 E8 51 D4 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener phase branches",
        "va": "0x00ccd9f0",
        "pattern": "8B 44 24 40 83 E8 00 74 5A 83 E8 01 74 1C 83 E8 01 75 74",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 1 word compare",
        "va": "0x00894f6d",
        "pattern": "66 8B 08 66 3B 0D 40 BC 34 01",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 1 active clear",
        "va": "0x00894f9e",
        "pattern": "C7 46 04 00 00 00 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 2 word compare",
        "va": "0x00895010",
        "pattern": "8B 44 24 10 56 8B F1 66 8B 08 66 3B 0D 40 BC 34 01 75 26",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 2 replay flag",
        "va": "0x00895023",
        "pattern": "B8 01 00 00 00 38 46 1C 75 03 88 46 1D",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 2 active clear",
        "va": "0x0089503e",
        "pattern": "C7 46 04 00 00 00 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 3 tail dispatch",
        "va": "0x00892760",
        "pattern": "8B 01 8B 40 08 FF E0",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 2 word pointer",
        "va": "0x00ccda29",
        "pattern": "8B 40 08 8D 55 14 52",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event listener slot 3 word pointer",
        "va": "0x00ccda12",
        "pattern": "8B 52 0C 8D 45 14 EB 4F",
        "mask": "FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event pending flag store",
        "va": "0x00896baa",
        "pattern": "C6 46 1C 01",
        "mask": "FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event replay guards and flag clears",
        "va": "0x00896ef8",
        "pattern": "38 5E 1C 74 53 38 5E 1D 74 4E 39 5E 08 88 5E 1C 88 5E 1D 74 43",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event pending detach",
        "va": "0x00896f0d",
        "pattern": "8B 7E 08 89 5E 08",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event replay call",
        "va": "0x00896f35",
        "pattern": "E8 B6 FB FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event base pump caller",
        "va": "0x006e13f0",
        "pattern": "51 8B 49 60 E8 D7 5A 1B 00",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event desktop byte gate and pump caller",
        "va": "0x006e1e18",
        "pattern": "80 7E 64 00 74 09 8B 4E 60 56 E8 A9 50 1B 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event invocation entry gates",
        "va": "0x00ccf9b3",
        "pattern": "8B 46 0C 8B 88 CC 01 00 00 80 79 08 00 75 42 53 8B 5C 24 0C 80 7B 7F 00 "
        "74 36",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF "
        "FF",
    },
    {
        "kind": "signature",
        "name": "event invocation admission",
        "va": "0x00ccf9e3",
        "pattern": "E8 98 D3 FF FF 84 C0 74 0D 8B 44 24 1C 50 53 8B CE E8 37 F4 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event invocation initial predicate",
        "va": "0x00ccee91",
        "pattern": "E8 2A 4B 02 00 84 C0 0F 84 24 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event invocation second predicate",
        "va": "0x00ccf08e",
        "pattern": "8B CF E8 2B 49 02 00 84 C0 0F 85 27 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event invocation phase 0 route",
        "va": "0x00ccf1b9",
        "pattern": "8B CE E8 20 F7 FF FF",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event invocation phase 2 local gate",
        "va": "0x00ccf23b",
        "pattern": "8A 5C 24 16 84 DB 74 16 8B CE E8 76 E9 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event phase 0 dispatch",
        "va": "0x00cce9b1",
        "pattern": "6A 00 8B CE E8 56 EF FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "event phase 2 dispatch",
        "va": "0x00ccdbf7",
        "pattern": "6A 02 8B CE E8 10 FD FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "dwords",
        "name": "event listener vtable",
        "va": "0x01056fe0",
        "values": ["0x0089a2b0", "0x00894f30", "0x00895010", "0x00892760"],
    },
    {
        "kind": "dwords",
        "name": "event WidgetBase pump slot",
        "va": "0x00fd5a84",
        "values": ["0x006e13f0"],
    },
    {
        "kind": "dwords",
        "name": "event DesktopWidget pump slot",
        "va": "0x00fd5b9c",
        "values": ["0x006e1de0"],
    },
    {
        "kind": "dwords",
        "name": "event DesktopUtil pump slot",
        "va": "0x00fdfcac",
        "values": ["0x006e13f0"],
    },
    {
        "kind": "dwords",
        "name": "event holder RTTI locator",
        "va": "0x01056fdc",
        "values": ["0x0117241c"],
    },
    {
        "kind": "dwords",
        "name": "event holder RTTI type descriptor",
        "va": "0x01172428",
        "values": ["0x012d7a24"],
    },
    {
        "kind": "dwords",
        "name": "event thread static zero fill",
        "va": "0x0134bc40",
        "values": ["0x00000000"],
        "width": 2,
    },
    {
        "kind": "string",
        "name": "event holder RTTI name",
        "va": "0x012d7a2c",
        "text": ".?AVFormManager@Event@Client@Script@Lua@Application@@",
    },
]


def parse_hex(text: str) -> int:
    return int(text, 16)


def parse_pattern(text: str) -> bytes:
    return bytes.fromhex(text)


def doc_errors(document: dict, text: str | None = None) -> list[str]:
    """Tie the manifest to the page: key addresses are cited and the join is labelled."""
    if text is None:
        if not DOC.is_file():
            return ["consumer page is missing"]
        text = DOC.read_text(encoding="utf-8")
    text = text.lower()
    handler = document.get("handler", {})
    switch = document.get("kindSwitch", {})
    iface = document.get("scriptInterface", {})
    wanted = [
        handler.get("handlerVa"),
        handler.get("subscriptionAddSite", {}).get("eventStaticVa"),
        handler.get("subscriptionAddSite", {}).get("initializerVa"),
        handler.get("routedEvent", {}).get("initializerVa"),
        switch.get("jumpTableVa"),
        iface.get("vtableVa"),
        iface.get("eventSelector", {}).get("functionVa"),
        *[row.get("va") for row in switch.get("thunks", [])],
        *iface.get("slotTable", {}).get("functionVas", []),
        *routing_addresses(document),
        *event_release_addresses(document),
    ]
    errors = [
        f"page does not cite {address}"
        for address in wanted
        if address and address.lower() not in text
    ]
    lead = text.split("## input and method")[0]
    if "0x59" not in lead or "wildcard" not in lead:
        errors.append("page lead does not state the 0x59 wildcard join")
    return errors


def event_release_addresses(document: dict) -> list[str]:
    """Locators for the listener clears, replay guards and direct caller tables."""
    queue = document.get("scriptInterface", {}).get("eventQueue", {})
    return [
        queue.get("holderConstructorVa"),
        queue.get("holderVtableVa"),
        queue.get("controlHolderStoreVa"),
        queue.get("listenerRegistrationCallVa"),
        queue.get("listenerDispatchVa"),
        queue.get("eventThreadWordGlobalVa"),
        queue.get("replayGuards", {}).get("pendingNonNullCompareVa"),
        queue.get("replayGuards", {}).get("replayCallVa"),
        *[row.get("functionVa") for row in queue.get("listenerPhases", [])],
        *[row.get("matchingActiveClearVa") for row in queue.get("listenerPhases", [])],
        *[row.get("callVa") for row in queue.get("flushDirectCallSites", [])],
        *[
            va
            for row in queue.get("flushDirectCallSites", [])
            for va in row.get("vtableEntryVas", [])
        ],
    ]


def event_release_errors(document: dict) -> list[str]:
    """Validate the bounded callback and replay contract against fixed evidence."""
    errors: list[str] = []
    queue = document.get("scriptInterface", {}).get("eventQueue", {})
    fields = {
        "rejectsNotAboveActivePriority": "0x00896b32..0x00896b42",
        "rejectsNotAboveSavedPendingPriority": "0x00896b47..0x00896b54",
        "priorityByte": "parameter +0x100",
        "flushVa": "0x00896ed0",
        "holderClass": "Application::Lua::Script::Client::Event::FormManager",
        "holderConstructorVa": "0x008951b0",
        "holderVtableVa": "0x01056fe0",
        "controlHolderStoreVa": "0x006e37ef",
        "controlHolderOffset": "+0x60",
        "activeParameterOffset": "+0x4",
        "pendingParameterOffset": "+0x8",
        "pointerWidthBits": 32,
        "listenerRegistrationCallVa": "0x00cd094a",
        "listenerDispatchVa": "0x00ccd910",
        "callbackThreadWordOffset": "+0x14",
        "callbackThreadWordWidthBits": 16,
        "eventThreadWordGlobalVa": "0x0134bc40",
        "pendingFlagStoreVa": "0x00896baa",
        "pendingFlagOffset": "+0x1c",
        "replayReadyStoreVa": "0x0089502d",
        "replayReadyOffset": "+0x1d",
        "flagWidthBits": 8,
    }
    for key, expected in fields.items():
        if queue.get(key) != expected:
            errors.append(f"event queue {key} changed")
    if "rejectsNotAbovePendingPriority" in queue:
        errors.append("event queue confuses active and saved pending priority")
    phases = queue.get("listenerPhases", [])
    phase_values = [
        (
            row.get("phase"),
            row.get("slot"),
            row.get("functionVa"),
            row.get("matchingActiveClearVa"),
            row.get("setsReplayReady"),
            row.get("tailDispatchSlot"),
            row.get("fourthArgument"),
        )
        for row in phases
    ]
    if phase_values != [
        (
            0,
            1,
            "0x00894f30",
            "0x00894f9e",
            False,
            None,
            "32-bit pointer to a wrapper holding the selected thread pointer",
        ),
        (
            1,
            2,
            "0x00895010",
            "0x0089503e",
            "only when the callback word matches the global and byte holder +0x1c equals exactly 1",
            None,
            "32-bit pointer to the 16-bit word at thread +0x14",
        ),
        (
            2,
            3,
            "0x00892760",
            None,
            None,
            2,
            "32-bit pointer to the 16-bit word at thread +0x14",
        ),
    ]:
        errors.append("event listener phases or callback ABI changed")
    if queue.get("replayGuards") != {
        "bothFlagsNonzeroRange": "0x00896ef8..0x00896f00",
        "pendingNonNullCompareVa": "0x00896f02",
        "pendingNullExitVa": "0x00896f0b",
        "flagClearVas": ["0x00896f05", "0x00896f08"],
        "detachVa": "0x00896f10",
        "replayCallVa": "0x00896f35",
    }:
        errors.append("event replay flag or pending-pointer guards changed")
    if queue.get("flushDirectCallSites") != [
        {
            "callVa": "0x006e13f4",
            "functionVa": "0x006e13f0",
            "controlSlot": 4,
            "vtableEntryVas": ["0x00fd5a84", "0x00fdfcac"],
        },
        {
            "callVa": "0x006e1e22",
            "functionVa": "0x006e1de0",
            "controlSlot": 4,
            "vtableEntryVas": ["0x00fd5b9c"],
            "gate": "control byte +0x64 nonzero",
            "gateWidthBits": 8,
        },
    ]:
        errors.append("event pump caller slots or byte gate changed")
    release = queue.get("conditionalRelease", {})
    if (
        release.get("confidence")
        != "INFERRED conditional same-native-invocation reachability, not observed execution or guaranteed Lua invocation or release"
    ):
        errors.append("event release inference boundary changed")
    if (
        queue.get("eventThreadWordStaticImage")
        != "zero-filled word, runtime mutations and immutability UNKNOWN"
    ):
        errors.append("event thread static-image boundary changed")
    checks = document.get("staticChecks", [])
    for expected in EVENT_RELEASE_CHECKS:
        found = [row for row in checks if row.get("name") == expected["name"]]
        if found != [expected]:
            errors.append(f"event release static check changed: {expected['name']}")
    return errors


def routing_addresses(document: dict) -> list[str]:
    """Addresses of the routing, gate and selector findings that the page must cite."""
    routing = document.get("routing", {})
    dispatch = routing.get("dispatch", {})
    registration = routing.get("registration", {})
    return [
        routing.get("wildcard", {}).get("matchVa"),
        routing.get("command", {}).get("executeVa"),
        routing.get("target", {}).get("invokeSiteVa"),
        routing.get("route", {}).get("routeVa"),
        routing.get("route", {}).get("traversalVa"),
        routing.get("interface", {}).get("constructorVa"),
        dispatch.get("genericSwitchVa"),
        dispatch.get("executedArm", {}).get("va"),
        dispatch.get("fallbackArmVa"),
        dispatch.get("slot65", {}).get("base"),
        dispatch.get("slot65", {}).get("frameworkElement"),
        dispatch.get("storeDispatcherVa"),
        routing.get("handlerObject", {}).get("invokeThunkVa"),
        registration.get("addVa"),
        registration.get("removeVa"),
        registration.get("removeThunkVa"),
        document.get("formGate", {}).get("factoryVa"),
        document.get("formGate", {}).get("classSelectorVa"),
        document.get("popupHelp", {}).get("positionVa"),
        document.get("popupHelp", {}).get("commandStaticVa"),
        document.get("selectorGates", {}).get("nullIdGlobalVa"),
        document.get("entryFlag", {}).get("flagVa"),
        document.get("numberInputBox", {}).get("handlerVa"),
        document.get("numberInputBox", {}).get("commandStaticVa"),
        *[row.get("va") for row in document.get("selectorGates", {}).get("gates", [])],
        *[row.get("va") for row in registration.get("sites", [])],
    ]


def routing_errors(document: dict) -> list[str]:
    """Check the routing, registration, gate and selector findings."""
    errors: list[str] = []
    routing = document.get("routing", {})
    descriptor = routing.get("descriptor", {})
    wildcard = routing.get("wildcard", {})
    command = routing.get("command", {})
    target = routing.get("target", {})
    route = routing.get("route", {})
    iface = routing.get("interface", {})
    dispatch = routing.get("dispatch", {})
    handler = routing.get("handlerObject", {})
    registration = routing.get("registration", {})
    if (
        descriptor.get("constructorVa") != "0x0097f6c0"
        or descriptor.get("strategyOffset") != "+0x4"
        or descriptor.get("idOffset") != "+0x5"
        or descriptor.get("strategies") != {"0": "bubble", "1": "direct", "2": "tunnel"}
    ):
        errors.append("event descriptor layout or strategy names changed")
    if (
        wildcard.get("eventId") != "0x59"
        or wildcard.get("matchVa") != "0x005715d7"
        or wildcard.get("statics") != ["0x01336a84", "0x0133f8f4", "0x01341904"]
    ):
        errors.append("wildcard event contract changed")
    if (
        command.get("executeVa") != "0x00927fa0"
        or command.get("preview", {}).get("id") != "0x29"
        or command.get("preview", {}).get("strategy") != 2
        or command.get("executed", {}).get("id") != "0x2a"
        or command.get("executed", {}).get("strategy") != 0
        or command.get("executed", {}).get("staticVa") != "0x01358738"
        or command.get("executedRaisedOnlyIfPreviewUnhandled") is not True
        or command.get("argumentLayout", {}).get("handledByte") != "+0x19"
        or command.get("argumentLayout", {}).get("stopByte") != "+0x17"
    ):
        errors.append("command raise contract changed")
    if (
        target.get("invokeSiteVa") != "0x009b2b40"
        or target.get("commandTargetOffset") != "+0x4c"
        or target.get("castTypeDescriptorVa") != "0x01269e8c"
        or target.get("interfaceAdjust") != "+0xb4"
    ):
        errors.append("command target selection changed")
    if (
        route.get("routeVa") != "0x0091e1c0"
        or route.get("traversalVa") != "0x0091d210"
        or route.get("parentOffset") != "+0x60"
        or route.get("recipientSlot") != 13
        or "list end" not in route.get("bubble", "")
        or "list start" not in route.get("tunnel", "")
    ):
        errors.append("route construction or traversal changed")
    if (
        iface.get("offset") != "+0xb4"
        or iface.get("senderOffset") != "+0x64"
        or iface.get("handlerStoreOffset") != "+0x4c"
        or iface.get("baseVtableVa") != "0x0106c2a4"
    ):
        errors.append("interface layout changed")
    if (
        dispatch.get("genericSwitchVa") != "0x0091d320"
        or dispatch.get("executedArm", {}).get("slot") != 65
        or dispatch.get("previewExecutedArm", {}).get("slot") != 64
        or dispatch.get("fallbackArmVa") != "0x0091d73a"
        or dispatch.get("slot65", {}).get("base") != "0x0091d8f0"
        or dispatch.get("slot65", {}).get("frameworkElement") != "0x0092d1f0"
        or dispatch.get("storeDispatcherVa") != "0x00571520"
    ):
        errors.append("dispatch chain changed")
    if (
        handler.get("vtableVa") != "0x00fa106c"
        or handler.get("slots") != ["0x00543b40", "0x0094c3a0", "0x0088ab00"]
        or handler.get("eventOffset") != "+0x18"
        or handler.get("handledEventsTooOffset") != "+0x1c"
        or handler.get("handledEventsTooInitial") != 0
        or handler.get("functionOffset") != "+0x28"
    ):
        errors.append("handler object layout changed")
    if (
        registration.get("addSlot") != 9
        or registration.get("removeSlot") != 12
        or registration.get("removeThunkSlot") != 11
        or len(registration.get("sites", [])) != 5
    ):
        errors.append("handler registration contract changed")
    gate = document.get("formGate", {})
    if (
        gate.get("offset") != "+0x27c"
        or gate.get("rootNameSliceStart") != 7
        or gate.get("executedSlot") != 42
        or gate.get("otherEventSlot") != 43
        or len(gate.get("names", [])) != 8
        or gate.get("names", ["", ""])[1:3] != ["Test", "PcSearchWidget"]
    ):
        errors.append("form gate contract changed")
    popup = document.get("popupHelp", {})
    if (
        popup.get("commandName") != "PopupShowHelp"
        or popup.get("commandStaticVa") != "0x0133bd38"
        or popup.get("positionVa") != "0x00535d00"
        or popup.get("windowName") != "Window_PopupHelpWidget"
    ):
        errors.append("popup help contract changed")
    selector = document.get("selectorGates", {})
    if (
        selector.get("nullIdValue") != "0xe0000000"
        or selector.get("nullIdGlobalVa") != "0x0130c778"
        or [row.get("va") for row in selector.get("gates", [])]
        != ["0x006f5654", "0x006f5675", "0x006f567d", "0x006f568e"]
    ):
        errors.append("selector gates changed")
    entry = document.get("entryFlag", {})
    if entry.get("flagVa") != "0x01356e14" or len(entry.get("writeVas", [])) != 2:
        errors.append("entry flag contract changed")
    box = document.get("numberInputBox", {})
    if (
        box.get("handlerVa") != "0x0069c1f0"
        or box.get("commandStaticVa") != "0x013609b0"
        or box.get("commandName") != "Slider.AddCommandParameter"
        or box.get("valueChangedCommandVa") != "0x01344ca8"
        or box.get("valueChangedReferencedByCode") is not False
    ):
        errors.append("NumberInputBox contract changed")
    routed = {
        row.get("eventName"): row
        for row in document.get("staticChecks", [])
        if row.get("kind") == "routedEvent" and row.get("eventName")
    }
    for name, (event_id, strategy, static) in EXPECTED_ROUTED.items():
        row = routed.get(name, {})
        if (
            row.get("id") != f"{event_id:#04x}"
            or row.get("strategy") != strategy
            or row.get("static") != f"{static:#010x}"
        ):
            errors.append(f"routed event {name} changed")
    return errors


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-sqwt-command-parameter-consumer-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("size") != EXPECTED_SIZE
        or source.get("binarySha256") != EXPECTED_SHA256
        or source.get("imageBase") != "0x00400000"
        or source.get("build") != "2012.09.19.0001"
    ):
        errors.append("evidence identity changed")

    handler = document.get("handler", {})
    event = handler.get("routedEvent", {})
    layout = handler.get("argumentLayout", {})
    if (
        handler.get("class") != "Application::Main::Element::FormElement"
        or handler.get("handlerVa") != "0x00536280"
        or handler.get("registrationVa") != "0x0053afaf"
        or handler.get("handlerObjectOffset") != "+0xa0"
        or handler.get("handlerObjectVtableVa") != "0x00fa106c"
        or handler.get("subscriptionAddSite", {}).get("eventId") != "0x59"
        or handler.get("subscriptionAddSite", {}).get("namePushed") is not False
        or handler.get("executedJoin") != "static"
        or event.get("branchVa") != "0x0053641c"
        or event.get("name") != "Executed"
        or event.get("id") != "0x2a"
        or layout.get("recordKind") != "+0x24"
        or layout.get("recordString") != "+0x28"
        or layout.get("recordInt0") != "+0x7c"
        or layout.get("recordInt1") != "+0x80"
        or layout.get("command") != "+0x20"
    ):
        errors.append("handler registration or argument layout changed")
    arguments = handler.get("thunkArguments", {})
    if (
        arguments.get("child") != "template, control, subscription"
        or arguments.get("root") != "control, subscription"
        or "pointer to a holder" not in arguments.get("scriptObject", "")
    ):
        errors.append("thunk argument contract changed")
    debounce = handler.get("frameDebounce", {})
    if (
        [row.get("name") for row in debounce.get("commands", [])] != EXPECTED_DEBOUNCED
        or debounce.get("sharedFlagOffset") != "+0x1ec"
        or debounce.get("storedFrameOffset") != "+0x1e8"
        or debounce.get("frameCounterOffset") != "+0x46c"
    ):
        errors.append("per-frame debounce set changed")

    gate = document.get("subscriptionGate", {})
    literal = gate.get("rootNameLiteral", {})
    condition = gate.get("conditionBytes", {})
    if (
        gate.get("lookupVa") != "0x00535b70"
        or gate.get("mapOffset") != "+0xf8"
        or gate.get("keys") != ["template name", "control name", "command name"]
        or gate.get("unmatched") != "return without a Lua call"
        or gate.get("rootNameOffset") != "+0x164"
        or gate.get("registration", {}).get("templateMapOffset") != "+0x104"
        or literal.get("text") != "_widget"
        or condition.get("storeOffsets") != ["+0x54", "+0x55"]
        or condition.get("byteAt0x0134c3fe") != 0
        or condition.get("byteAt0x012c3f78") != 1
        or condition.get("numberAbsent", {}).get("+0x55") != "0"
    ):
        errors.append("subscription gate or condition bytes changed")

    switch = document.get("kindSwitch", {})
    targets = switch.get("targets", [])
    default = switch.get("defaultTarget")
    variants = [
        (
            row.get("kinds"),
            row.get("variant"),
            row.get("values"),
            row.get("rootSlot"),
            row.get("childSlot"),
        )
        for row in switch.get("variants", [])
    ]
    if variants != EXPECTED_VARIANTS:
        errors.append("kind to variant table changed")
    if len(targets) != 9 or any(not HEX_RE.match(item) for item in targets):
        errors.append("jump table is not nine addresses")
    else:
        by_target: dict[str, list[int]] = {}
        for kind, target in enumerate(targets):
            by_target.setdefault(target, []).append(kind)
        grouped = {
            tuple(kinds) for target, kinds in by_target.items() if target != default
        }
        wanted = {tuple(row[0]) for row in EXPECTED_VARIANTS}
        if grouped != wanted:
            errors.append("jump table grouping disagrees with the variant table")
        if by_target.get(default) != switch.get("noEventKinds"):
            errors.append("default target kinds disagree with noEventKinds")
    thunks = {row.get("slot"): row for row in switch.get("thunks", [])}
    if sorted(thunks) != list(range(8, 16)):
        errors.append("thunk slots are not 8 through 15")
    for slot, row in thunks.items():
        if row.get("vtableOffset") != f"+{4 * slot:#x}":
            errors.append(f"thunk slot {slot} offset is not four times the slot")
    if switch.get("scriptObjectOffset") != "+0x80":
        errors.append("script object offset changed")

    iface = document.get("scriptInterface", {})
    slots = iface.get("slotTable", {})
    selector = iface.get("eventSelector", {})
    if (
        iface.get("vtableVa") != "0x00fdfb2c"
        or slots.get("firstSlot") != 8
        or len(slots.get("functionVas", [])) != 8
        or selector.get("equalPath", {}).get("name") != "_onUICommandEvent"
        or selector.get("unequalPath", {}).get("name") != "_onUICommandRequest"
        or iface.get("requestDepthLimit", "")[:10] != "0x00896d33"
    ):
        errors.append("script interface or event selector changed")

    objects = document.get("parameterObjects", {})
    if (
        [row.get("name") for row in objects.get("classes", [])]
        != [row[1] for row in EXPECTED_VARIANTS]
        or objects.get("layout", {}).get("int0") != "+0x104"
        or objects.get("layout", {}).get("int1") != "+0x108"
        or objects.get("layout", {}).get("string2") != "+0xac"
        or "nil when empty" not in objects.get("pushOrder", "")
    ):
        errors.append("parameter object layout or push order changed")

    cases = {
        row.get("case"): row
        for row in document.get("commandSource", {}).get("cases", [])
    }
    if (
        cases.get(2, {}).get("textAt") != "+0x78"
        or cases.get(2, {}).get("recordHolderAt") != "+0x54"
        or cases.get(0, {}).get("storedAt") != "+0x44"
    ):
        errors.append("command source layout changed")

    for index, check in enumerate(document.get("staticChecks", [])):
        label = f"static check {index}"
        if not HEX_RE.match(check.get("va", "")):
            errors.append(f"{label} has no address")
        kind = check.get("kind")
        if kind == "signature":
            try:
                pattern = parse_pattern(check["pattern"])
                mask = parse_pattern(check["mask"])
            except (KeyError, ValueError):
                errors.append(f"{label} pattern or mask is not hex")
                continue
            if len(pattern) != len(mask) or not pattern or set(mask) - {0, 255}:
                errors.append(f"{label} pattern and mask disagree")
        elif kind == "dwords":
            width = check.get("width", 4)
            values = check.get("values", [])
            if width not in (1, 2, 4) or not values:
                errors.append(f"{label} width or values invalid")
        elif kind == "string":
            if not check.get("text"):
                errors.append(f"{label} has no text")
        elif kind == "routedEvent":
            if not (
                HEX_RE.match(check.get("static", ""))
                and re.match(r"^0x[0-9a-f]{2}$", check.get("id", ""))
                and check.get("strategy") in (0, 1, 2)
            ):
                errors.append(f"{label} routed event fields invalid")
        else:
            errors.append(f"{label} has unknown kind")
    if len(document.get("staticChecks", [])) < 50:
        errors.append("static check set shrank")
    names = {check.get("name") for check in document.get("staticChecks", [])}
    missing = REQUIRED_CHECKS - names
    if missing:
        errors.append(f"static checks missing: {sorted(missing)}")
    event_ids = sorted(
        int(check["id"], 16)
        for check in document.get("staticChecks", [])
        if check.get("kind") == "routedEvent" and HEX_RE.match(check.get("va", ""))
    )
    if event_ids != [i for i in range(1, 0x5B) if i != 0x21]:
        errors.append("routed event rows are not ids 0x01..0x5a without 0x21")
    errors.extend(event_release_errors(document))
    errors.extend(routing_errors(document))
    errors.extend(doc_errors(document))

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
        (("handler", "handlerVa"), "0x00536290"),
        (("handler", "routedEvent", "id"), "0x28"),
        (("handler", "handlerObjectVtableVa"), "0x00fa10dc"),
        (("handler", "subscriptionAddSite", "eventId"), "0x2a"),
        (("handler", "subscriptionAddSite", "namePushed"), True),
        (("handler", "executedJoin"), "inferred"),
        (("handler", "thunkArguments", "child"), "control, subscription"),
        (("handler", "routedEvent", "initializerVa"), "0x00f21cb1"),
        (("handler", "argumentLayout", "recordInt1"), "+0x84"),
        (("handler", "frameDebounce", "sharedFlagOffset"), "+0x1f0"),
        (("subscriptionGate", "keys"), ["control name", "command name"]),
        (("subscriptionGate", "unmatched"), "deliver to the form"),
        (("subscriptionGate", "rootNameLiteral", "text"), "_root"),
        (("subscriptionGate", "registration", "templateMapOffset"), "+0xf8"),
        (("subscriptionGate", "conditionBytes", "byteAt0x0134c3fe"), 1),
        (("kindSwitch", "targets", 1), "0x005366da"),
        (("kindSwitch", "targets", 4), "0x00536676"),
        (("kindSwitch", "noEventKinds"), [4, 5, 6]),
        (("kindSwitch", "variants", 2, "kinds"), [2]),
        (("kindSwitch", "variants", 1, "childSlot"), 12),
        (("kindSwitch", "thunks", 0, "vtableOffset"), "+0x20"),
        (
            ("scriptInterface", "eventSelector", "equalPath", "name"),
            "_onUICommandRequest",
        ),
        (("scriptInterface", "eventQueue", "activeParameterOffset"), "+0x8"),
        (("scriptInterface", "eventQueue", "pendingParameterOffset"), "+0x4"),
        (("scriptInterface", "eventQueue", "callbackThreadWordWidthBits"), 32),
        (("scriptInterface", "eventQueue", "listenerPhases", 1, "slot"), 3),
        (
            (
                "scriptInterface",
                "eventQueue",
                "replayGuards",
                "pendingNonNullCompareVa",
            ),
            "0x00896f05",
        ),
        (
            (
                "scriptInterface",
                "eventQueue",
                "flushDirectCallSites",
                1,
                "gateWidthBits",
            ),
            32,
        ),
        (
            ("scriptInterface", "eventQueue", "conditionalRelease", "confidence"),
            "STATIC guaranteed release",
        ),
        (("parameterObjects", "layout", "int1"), "+0x104"),
        (("parameterObjects", "pushOrder"), "string0, string1, string2"),
        (("commandSource", "cases", 2, "textAt"), "+0x54"),
        (("source", "binarySha256"), "0" * 64),
        (("routing", "wildcard", "eventId"), "0x58"),
        (("routing", "wildcard", "matchVa"), "0x005715d8"),
        (
            ("routing", "descriptor", "strategies"),
            {"0": "tunnel", "1": "direct", "2": "bubble"},
        ),
        (("routing", "command", "executed", "strategy"), 2),
        (("routing", "command", "executedRaisedOnlyIfPreviewUnhandled"), False),
        (("routing", "target", "commandTargetOffset"), "+0x44"),
        (("routing", "route", "bubble"), "traversed from the list start"),
        (("routing", "interface", "senderOffset"), "+0x68"),
        (("routing", "dispatch", "executedArm", "slot"), 64),
        (("routing", "dispatch", "slot65", "base"), "0x0091cc10"),
        (("routing", "handlerObject", "handledEventsTooInitial"), 1),
        (("routing", "registration", "addSlot"), 10),
        (("formGate", "rootNameSliceStart"), 6),
        (("formGate", "executedSlot"), 43),
        (("popupHelp", "commandStaticVa"), "0x0133bd98"),
        (("selectorGates", "nullIdValue"), "0xffffffff"),
        (("entryFlag", "flagVa"), "0x01356e18"),
        (("numberInputBox", "commandStaticVa"), "0x01344ca8"),
        (("numberInputBox", "valueChangedReferencedByCode"), True),
    ]
    mutations: list[dict] = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    shrunk = copy.deepcopy(document)
    shrunk["staticChecks"] = shrunk["staticChecks"][:5]
    mutations.append(shrunk)
    for event_name, field, value in (
        ("Executed", "strategy", 2),
        ("PreviewExecuted", "id", "0x2a"),
        ("LocationChanged", "static", "0x013573b0"),
    ):
        changed = copy.deepcopy(document)
        for row in changed["staticChecks"]:
            if row.get("eventName") == event_name:
                row[field] = value
        mutations.append(changed)
    for dropped in (
        "request depth limit",
        "event listener slot 1 active clear",
        "event replay guards and flag clears",
        "event listener vtable",
        "thunk slot 8 child none",
        "handler store dispatcher match",
        "routed event Executed",
        "routed event PreviewKeyDown",
    ):
        trimmed = copy.deepcopy(document)
        trimmed["staticChecks"] = [
            check for check in trimmed["staticChecks"] if check.get("name") != dropped
        ]
        mutations.append(trimmed)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "C:" + "\\Users\\maintainer\\private"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def section_map(data: bytes) -> list[tuple[int, int, int, int]]:
    """Return (virtual address, virtual size, raw offset, raw size) per section."""
    if data[:2] != b"MZ":
        raise ValueError("not a PE image")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("missing PE signature")
    count = struct.unpack_from("<H", data, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    table = pe_offset + 24 + optional_size
    sections = []
    for index in range(count):
        base = table + 40 * index
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, base + 8
        )
        sections.append(
            (IMAGE_BASE + virtual_address, virtual_size, raw_offset, raw_size)
        )
    return sections


def read_va(
    data: bytes, sections: list[tuple[int, int, int, int]], va: int, length: int
) -> bytes | None:
    for start, virtual_size, raw_offset, raw_size in sections:
        if start <= va and va + length <= start + max(virtual_size, raw_size):
            out = bytearray(length)
            for index in range(length):
                relative = va - start + index
                if relative < raw_size:
                    out[index] = data[raw_offset + relative]
            return bytes(out)
    return None


def read_cstring(
    data: bytes, sections: list[tuple[int, int, int, int]], va: int
) -> str | None:
    out = bytearray()
    while len(out) < 128:
        byte = read_va(data, sections, va + len(out), 1)
        if byte is None:
            return None
        if byte == b"\0":
            return out.decode("latin-1")
        out += byte
    return None


def run_routed_event_check(
    data: bytes, sections: list[tuple[int, int, int, int]], check: dict
) -> str | None:
    """Parse an event descriptor initializer and compare id, strategy, static and name."""
    va = parse_hex(check["va"])
    raw = read_va(data, sections, va, 23)
    if raw is None:
        return f"{check['va']} is outside the image"
    named = ROUTED_NAMED.fullmatch(raw)
    unnamed = ROUTED_UNNAMED.fullmatch(raw[:20])
    if named:
        name_va = struct.unpack("<I", named.group(1))[0]
        event_id, static, strategy = (
            named.group(2)[0],
            named.group(3),
            named.group(4)[0],
        )
        found_name = read_cstring(data, sections, name_va)
    elif unnamed:
        event_id, static, strategy = (
            unnamed.group(1)[0],
            unnamed.group(2),
            unnamed.group(3)[0],
        )
        found_name = None
    else:
        return f"{check['va']} is not an event descriptor initializer"
    found = (event_id, strategy, struct.unpack("<I", static)[0], found_name)
    wanted = (
        parse_hex(check["id"]),
        check["strategy"],
        parse_hex(check["static"]),
        check.get("eventName"),
    )
    if found != wanted:
        return f"routed event at {check['va']} is {found}, expected {wanted}"
    return None


def run_static_check(
    data: bytes, sections: list[tuple[int, int, int, int]], check: dict
) -> str | None:
    va = parse_hex(check["va"])
    kind = check["kind"]
    if kind == "signature":
        pattern = parse_pattern(check["pattern"])
        mask = parse_pattern(check["mask"])
        actual = read_va(data, sections, va, len(pattern))
        if actual is None:
            return f"{check['va']} is outside the image"
        for index, keep in enumerate(mask):
            if keep and actual[index] != pattern[index]:
                return f"signature {check['name']!r} differs at {va + index:#010x}"
        return None
    if kind == "dwords":
        width = check.get("width", 4)
        expected = [parse_hex(item) for item in check["values"]]
        actual = read_va(data, sections, va, width * len(expected))
        if actual is None:
            return f"{check['va']} is outside the image"
        found = [
            int.from_bytes(actual[index : index + width], "little")
            for index in range(0, len(actual), width)
        ]
        if found != expected:
            return (
                f"table {check['name']!r} at {check['va']} is {[hex(v) for v in found]}"
            )
        return None
    if kind == "routedEvent":
        return run_routed_event_check(data, sections, check)
    expected_text = check["text"].encode("ascii") + b"\0"
    actual = read_va(data, sections, va, len(expected_text))
    if actual != expected_text:
        return f"string at {check['va']} is not {check['text']!r}"
    return None


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
        sections = section_map(data)
    except (ValueError, struct.error) as exc:
        return errors + [f"image is not a readable PE32: {exc}"]
    for check in document["staticChecks"]:
        problem = run_static_check(data, sections, check)
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
        print(f"FAIL: {len(errors)} command-parameter consumer error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: command-parameter consumer manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
