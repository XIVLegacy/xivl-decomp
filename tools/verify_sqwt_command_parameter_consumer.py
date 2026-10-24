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
    "PreviewMouseLeftButtonDown": (0x0D, 2, 0x01359674),
    "PreviewMouseLeftButtonUp": (0x0F, 2, 0x013593CC),
    "Click": (0x38, 0, 0x0135E624),
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


BUTTON_CHECKS = [
    {
        "kind": "dwords",
        "name": "button interface RTTI locator",
        "va": "0x01073b80",
        "values": ["0x0117d4d0"],
    },
    {
        "kind": "dwords",
        "name": "button primary RTTI locator",
        "va": "0x01073ca8",
        "values": ["0x0117d44c"],
    },
    {
        "kind": "dwords",
        "name": "button interface RTTI offset",
        "va": "0x0117d4d4",
        "values": ["0x000000b4"],
    },
    {
        "kind": "dwords",
        "name": "button interface RTTI type",
        "va": "0x0117d4dc",
        "values": ["0x0126d244"],
    },
    {
        "kind": "dwords",
        "name": "button primary RTTI offset",
        "va": "0x0117d450",
        "values": ["0x00000000"],
    },
    {
        "kind": "dwords",
        "name": "button primary RTTI type",
        "va": "0x0117d458",
        "values": ["0x0126d244"],
    },
    {
        "kind": "string",
        "name": "button RTTI name",
        "va": "0x0126d24c",
        "text": ".?AVButton@Controls@Sqwt@@",
    },
    {
        "kind": "dwords",
        "name": "button recipient slot",
        "va": "0x01073bb8",
        "values": ["0x0097ece0"],
    },
    {
        "kind": "dwords",
        "name": "button mouse slots",
        "va": "0x01073c30",
        "values": ["0x0097eff0", "0x0097f080"],
    },
    {
        "kind": "dwords",
        "name": "button Executed slot",
        "va": "0x01073c88",
        "values": ["0x0097edb0"],
    },
    {
        "kind": "dwords",
        "name": "button primary hook slot",
        "va": "0x01073dc8",
        "values": ["0x0097eb60"],
    },
    {
        "kind": "dwords",
        "name": "button base Executed slot",
        "va": "0x01076140",
        "values": ["0x0097edb0"],
    },
    {
        "kind": "dwords",
        "name": "button command source RTTI locator",
        "va": "0x0106e368",
        "values": ["0x0117ba44"],
    },
    {
        "kind": "dwords",
        "name": "button command source RTTI type",
        "va": "0x0117ba50",
        "values": ["0x012e31a0"],
    },
    {
        "kind": "string",
        "name": "button command source RTTI name",
        "va": "0x012e31a8",
        "text": ".?AV?$ICommandSource@VContentControl@Controls@Sqwt@@@Input@Sqwt@@",
    },
    {
        "kind": "dwords",
        "name": "button RoutedCommand slot",
        "va": "0x0106a794",
        "values": ["0x00927fa0"],
    },
    {
        "kind": "dwords",
        "name": "button RoutedCommand RTTI locator",
        "va": "0x0106a788",
        "values": ["0x01179454"],
    },
    {
        "kind": "dwords",
        "name": "button RoutedCommand RTTI type",
        "va": "0x01179460",
        "values": ["0x012e0398"],
    },
    {
        "kind": "string",
        "name": "button RoutedCommand RTTI name",
        "va": "0x012e03a0",
        "text": ".?AVRoutedCommand@Input@Sqwt@@",
    },
    {
        "kind": "dwords",
        "name": "button EventTrigger slot",
        "va": "0x0107d55c",
        "values": ["0x009b1820"],
    },
    {
        "kind": "string",
        "name": "button ClickMode name",
        "va": "0x01075fbc",
        "text": "ClickMode",
    },
    {
        "kind": "string",
        "name": "button IsPressed name",
        "va": "0x01075fc8",
        "text": "IsPressed",
    },
    {
        "kind": "string",
        "name": "button Click name",
        "va": "0x01075fd4",
        "text": "Click",
    },
    {
        "kind": "dwords",
        "name": "button down event map",
        "va": "0x0091d824",
        "values": ["0x0000000c"],
        "width": 1,
    },
    {
        "kind": "dwords",
        "name": "button up event map",
        "va": "0x0091d826",
        "values": ["0x0000000e"],
        "width": 1,
    },
    {
        "kind": "dwords",
        "name": "button down dispatch arm",
        "va": "0x0091d790",
        "values": ["0x0091d4be"],
    },
    {
        "kind": "dwords",
        "name": "button up dispatch arm",
        "va": "0x0091d798",
        "values": ["0x0091d4d0"],
    },
    {
        "kind": "signature",
        "name": "button constructor tables",
        "va": "0x0097444e",
        "pattern": "C7 06 AC 3C 07 01 C7 86 B4 00 00 00 84 3B 07 01",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button primary hook return",
        "va": "0x0097eb60",
        "pattern": "C3",
        "mask": "FF",
    },
    {
        "kind": "signature",
        "name": "button mode default byte",
        "va": "0x0097f26d",
        "pattern": "C6 44 24 26 02",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up handled and eligibility bytes",
        "va": "0x0097f091",
        "pattern": "8B 46 10 80 38 00 0F 85 88 00 00 00 F6 85 A6 00 00 00 20 74 7F",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up snapshot before refresh",
        "va": "0x0097f0a7",
        "pattern": "8A 9D FA 02 00 00 57 C0 EB 05 8D BD 4C FF FF FF 8D B7 AE 03 00 00 80 E3 01 F6 06 10",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button down property predicates",
        "va": "0x0097f03f",
        "pattern": "8A 06 A8 03 74 1A A8 20 75 16",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up property predicates",
        "va": "0x0097f0de",
        "pattern": "8A 06 A8 03 74 25 A8 20 74 21",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up mode and snapshot gates",
        "va": "0x0097f109",
        "pattern": "80 BD F8 02 00 00 02 75 11 80 FB 01 75 0C",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button temporary flag pointer",
        "va": "0x0097ed13",
        "pattern": "89 8F FC 02 00 00 8B CE C6 44 24 14 00 FF D0",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button stored command slot ABI",
        "va": "0x0097ed5f",
        "pattern": "8B 8F BC 02 00 00 8B 11 8B 52 08 55 50 8B 87 CC 02 00 00 50 FF D2",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button base constructor call",
        "va": "0x0097443a",
        "pattern": "E8 C1 AD 00 00",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button command source constructor call",
        "va": "0x0097f22b",
        "pattern": "E8 80 58 FD FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button down base call",
        "va": "0x0097effe",
        "pattern": "E8 8D FB FA FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up base call",
        "va": "0x0097f08c",
        "pattern": "E8 FF AD FB FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button down Click call",
        "va": "0x0097f075",
        "pattern": "E8 26 FB FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up Click call",
        "va": "0x0097f11e",
        "pattern": "E8 7D FA FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button generic Click forwarding",
        "va": "0x0097ed96",
        "pattern": "E8 E5 65 FB FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button Executed FrameworkElement call",
        "va": "0x0097edde",
        "pattern": "E8 0D E4 FA FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button FrameworkElement base call",
        "va": "0x0092d235",
        "pattern": "E8 B6 06 FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button parameter setter call",
        "va": "0x00954f04",
        "pattern": "E8 27 F4 FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button parameter parser call",
        "va": "0x00954405",
        "pattern": "E8 E6 99 04 00",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button record store call",
        "va": "0x00954433",
        "pattern": "E8 18 FA FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button record copy call",
        "va": "0x00953f0e",
        "pattern": "E8 6D 18 BF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button local stop flag gate",
        "va": "0x0097ed77",
        "pattern": "38 44 24 14 5E 75 0F",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button Click stop byte store",
        "va": "0x0097ed85",
        "pattern": "88 45 17",
        "mask": "FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button down packed value write",
        "va": "0x0097f053",
        "pattern": "0C 24 56 8B CE 88 06",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up packed value write",
        "va": "0x0097f0ee",
        "pattern": "24 DF 68 30 E6 35 01 0C 04 51 88 06",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button down refresh mask clear",
        "va": "0x0097f03c",
        "pattern": "80 26 EF",
        "mask": "FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button up refresh mask clear",
        "va": "0x0097f0db",
        "pattern": "80 26 EF",
        "mask": "FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button target fallback predicate",
        "va": "0x0097ed43",
        "pattern": "85 C0 74 16 F6 80 5D 01 00 00 20 75 0D 8B C6 85 C0 74 07 05 B4 00 00 00 EB 02 33 C0",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
]

BUTTON_ELIGIBILITY_PROPERTY = {
    "name": "IsHitTestVisible",
    "nameVa": "0x0106bec4",
    "descriptorVa": "0x01359354",
    "initializerVa": "0x00f22930",
    "propertyIndex": 2,
    "elementOffset": "+0x15a",
    "interfaceOffset": "+0xa6",
    "widthBits": 8,
    "valueMask": "0x20",
    "getterDispatcherVa": "0x0093a200",
    "getterCaseVa": "0x0093a246",
    "getterVa": "0x0093e220",
    "getterOperation": "(P >> 5) & 1",
    "constructorChain": [
        "0x00974410",
        "0x0097f200",
        "0x00954ab0",
        "0x00936190",
        "0x00938c80",
        "0x0092f640",
        "0x0093c750",
    ],
    "defaultWriteVa": "0x0093c8bf",
    "defaultOperation": "(old & 0xe0) | 0x20",
    "metadataSourceVa": "0x0135936c",
    "metadataWrites": [
        {"va": "0x0093c8f4", "destinationBit": 1, "sourceBit": 7},
        {"va": "0x0093c910", "destinationBit": 0, "sourceBit": 5},
    ],
    "setterVa": "0x0093c300",
    "refreshDispatcherVa": "0x0093d090",
    "refreshCaseVa": "0x0093d0fb",
    "refreshHelperVa": "0x009321d0",
    "resolutionVa": "0x00916310",
    "deferredSentinelVa": "0x01356da8",
    "conversionVa": "0x00988f90",
    "writerVa": "0x0099c780",
    "writerValueStoreVa": "0x0099c7f1",
    "writerArguments": [
        "value byte V",
        "owner",
        "callback",
        "callback argument",
        "descriptor",
        "byte M",
        "byte C",
    ],
    "writerRefresh": "If P & 0x10, call 0x009157c0(owner, descriptor), then clear mask 0x10.",
    "writerReject": [
        "(P & 2) != 0 and M == 0",
        "(P & 2) == 0 and M == 1 and (P & 1) == 0",
    ],
    "writerLowBitClear": "On accepted M == 0, clear P mask 0x01 before comparing values.",
    "writerCompare": "Compare (P >> 5) & 1 with the whole byte V.",
    "writerChangedOperation": "(P & 0xdf) | ((V << 5) & 0x20) | 0x04",
    "writerCallback": "Call 0x0099c680 only when the whole-byte comparison is unequal and byte C equals 1.",
    "confidence": "STATIC",
}

BUTTON_ELIGIBILITY_CHECKS = [
    {
        "kind": "string",
        "name": "button eligibility property name",
        "va": "0x0106bec4",
        "text": "IsHitTestVisible",
    },
    {
        "kind": "signature",
        "name": "button eligibility descriptor initializer",
        "va": "0x00f22930",
        "pattern": "68 C4 BE 06 01 B9 54 93 35 01 E8 A1 DF 9E FF C7 05 68 93 35 01 02 00 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility descriptor name store",
        "va": "0x009108e0",
        "pattern": "56 8B F1 8B 4C 24 08 8B C1 57 89 0E",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "dwords",
        "name": "button eligibility getter case two",
        "va": "0x0093a358",
        "values": ["0x0093a246"],
    },
    {
        "kind": "signature",
        "name": "button eligibility getter case receiver",
        "va": "0x0093a246",
        "pattern": "8B 44 24 08 50 81 C1 5A 01 00 00 E8 CA 3F 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility getter packed extraction",
        "va": "0x0093e221",
        "pattern": "8A 01 8B 4C 24 08 C0 E8 05 51 8D 54 24 07 24 01",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility Button base call",
        "va": "0x00974434",
        "pattern": "8B F1 89 74 24 08 E8 C1 AD 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility first base call",
        "va": "0x0097f225",
        "pattern": "8B F1 89 74 24 10 E8 80 58 FD FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility second base call",
        "va": "0x00954ad4",
        "pattern": "8B F1 89 74 24 0C E8 B1 16 FE FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility third base call",
        "va": "0x009361b6",
        "pattern": "8B F1 89 74 24 14 E8 BF 2A 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility fourth base call",
        "va": "0x00938ca6",
        "pattern": "8B F1 89 74 24 18 E8 8F 69 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility fifth base call",
        "va": "0x0092f667",
        "pattern": "8B F1 89 74 24 24 E8 DE D0 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility base receiver save",
        "va": "0x0093c777",
        "pattern": "8B F1 89 74 24 18",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility constructor value default",
        "va": "0x0093c8b3",
        "pattern": "8A 86 5A 01 00 00 24 E0 0C 20 33 ED 88 86 5A 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility constructor metadata mask",
        "va": "0x0093c853",
        "pattern": "B3 02",
        "mask": "FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility constructor metadata bit one",
        "va": "0x0093c8e1",
        "pattern": "8B 0D 6C 93 35 01 C1 E9 07 02 C9 32 8E 5A 01 00 00 22 CB 30 8E 5A 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility constructor metadata bit zero",
        "va": "0x0093c8fa",
        "pattern": "8B 15 6C 93 35 01 8A 86 5A 01 00 00 C1 EA 05 32 D0 80 E2 01 32 D0 88 96 5A 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility setter descriptor",
        "va": "0x0093c30b",
        "pattern": "68 54 93 35 01",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility setter receiver and join",
        "va": "0x0093c31d",
        "pattern": "51 50 81 C1 5A 01 00 00 E8 56 04 06 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility refresh case dispatch",
        "va": "0x0093d0a0",
        "pattern": "FF 24 85 20 D3 93 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "dwords",
        "name": "button eligibility refresh case two",
        "va": "0x0093d328",
        "values": ["0x0093d0fb"],
    },
    {
        "kind": "signature",
        "name": "button eligibility refresh descriptor",
        "va": "0x0093d102",
        "pattern": "68 54 93 35 01",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility refresh receiver and join",
        "va": "0x0093d116",
        "pattern": "8D 8E 5A 01 00 00 E8 AF 50 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility resolution call",
        "va": "0x009321f2",
        "pattern": "E8 19 41 FE FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility resolution deferred branch",
        "va": "0x009321fa",
        "pattern": "3D A8 6D 35 01 75 0B 80 0E 10",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility resolution conversion call",
        "va": "0x0093221e",
        "pattern": "E8 6D 6D 05 00",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility resolution writer join",
        "va": "0x00932245",
        "pattern": "8B CE E8 34 A5 06 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer refresh gate",
        "va": "0x0099c788",
        "pattern": "F6 06 10 57 8B 7C 24 20 74 0D",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer refresh clear",
        "va": "0x0099c792",
        "pattern": "57 55 E8 27 90 F7 FF 83 C4 08 80 26 EF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer metadata gates",
        "va": "0x0099c79f",
        "pattern": "8A 0E 8A 54 24 24 8A C1 D0 E8 24 01 74 04 84 D2 74 0D 84 C0 75 11 80 FA 01 75 0C 84 CA 75 11",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer low bit clear",
        "va": "0x0099c7c6",
        "pattern": "84 D2 75 05 80 E1 FE 88 0E",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer value and callback gate",
        "va": "0x0099c7cf",
        "pattern": "8A 0E 8A 44 24 10 8A D1 C0 EA 05 80 E2 01 3A D0 74 2A C0 E0 05 32 C1 24 20 32 C1 0C 04 80 7C 24 28 01 88 06 75 16",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer callback join",
        "va": "0x0099c806",
        "pattern": "E8 75 FE FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility getter case dispatch",
        "va": "0x0093a20d",
        "pattern": "FF 24 85 50 A3 93 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "button eligibility writer receiver and owner",
        "va": "0x0099c780",
        "pattern": "55 8B 6C 24 0C 56 8B F1",
        "mask": "FF FF FF FF FF FF FF FF",
    },
]

BUTTON_FIELDS = {
    "identity": {
        "class": "Sqwt::Controls::Button",
        "typeDescriptorVa": "0x0126d244",
        "rttiName": ".?AVButton@Controls@Sqwt@@",
        "constructorVa": "0x00974410",
        "primaryVtableVa": "0x01073cac",
        "interfaceVtableVa": "0x01073b84",
        "interfaceOffset": "+0xb4",
        "interfaceCompleteObjectLocatorVa": "0x0117d4d0",
    },
    "mouseGates": {
        "handledWidthBits": 8,
        "eligibilityWidthBits": 8,
        "eligibility": "element +0x15a byte mask 0x20 is the IsHitTestVisible value and is set.",
        "stateWidthBits": 8,
        "clickModeWidthBits": 8,
        "propertyUpdateMask": "0x03",
        "downValueMustBeSet": False,
        "downOrMask": "0x24",
        "upValueMustBeSet": True,
        "upAndMask": "0xdf",
        "upOrMask": "0x04",
        "snapshotAfterBaseCall": True,
        "snapshotBeforeRefresh": True,
        "stateByte": "+0x3ae",
        "refreshMask": "0x10",
        "valueMask": "0x20",
        "clickModeOffset": "+0x3ac",
        "clickModeDefaultByte": 2,
        "clickModeDescriptorVa": "0x0135e600",
        "stateDescriptorVa": "0x0135e630",
    },
    "storedCommand": {
        "offset": "+0x370",
        "slot": 2,
        "recordPointerOffset": "+0x380",
        "recordSize": "0x78",
        "targetPointerOffset": "+0x378",
        "targetNameOffset": "+0x3a8",
        "targetHelperVa": "0x00953cd0",
        "slotArguments": [
            "stored record pointer",
            "selected target interface or null",
            "Click args pointer",
        ],
        "propertySetterVa": "0x00954e70",
        "parameterSetterCallVa": "0x00954f04",
        "parserCallVa": "0x00954405",
        "recordStoreCallVa": "0x00954433",
        "recordCopyCallVa": "0x00953f0e",
        "slotByteOffset": "+0x08",
        "pointerWidthBits": 32,
        "targetFallback": {
            "primarySlot": 44,
            "returnNonNull": True,
            "returnByteOffset": "+0x15d",
            "clearMask": "0x20",
            "selected": "element +0xb4",
            "elseTarget": None,
        },
    },
    "click": {
        "helperVa": "0x0097eba0",
        "descriptorVa": "0x0135e624",
        "id": "0x38",
        "routingStrategy": 0,
        "source": "element",
        "originalSource": "element",
        "routeTarget": "element +0xb4",
        "interfaceRecipientSlot": 13,
        "recipientVa": "0x0097ece0",
        "primaryHookSlot": 71,
        "primaryHookVa": "0x0097eb60",
        "commandCallVa": "0x0097ed73",
        "genericForwardCallVa": "0x0097ed96",
        "temporaryFlagPointerOffset": "+0x2fc",
        "temporaryPointerWidthBits": 32,
        "temporaryFlagWidthBits": 8,
        "skipGenericWhenFlagEquals": 1,
        "defaultCommandBeforeGeneric": True,
        "triggerAttachment": "UNKNOWN",
        "eventTriggerVtableVa": "0x0107d554",
        "eventTriggerSlot": 2,
        "eventTriggerFunctionVa": "0x009b1820",
    },
    "routedCommandJoin": {
        "constructorVa": "0x00928c90",
        "vtableVa": "0x0106a78c",
        "slot": 2,
        "functionVa": "0x00927fa0",
    },
    "executedForwarding": {
        "interfaceSlot": 65,
        "functionVa": "0x0097edb0",
        "frameworkElementCallVa": "0x0097edde",
        "frameworkElementFunctionVa": "0x0092d1f0",
        "baseCallVa": "0x0092d235",
        "baseFunctionVa": "0x0091d8f0",
        "baseStoreCallVa": "0x0091d915",
    },
}

BUTTON_INGRESS = [
    {
        "event": "PreviewMouseLeftButtonDown",
        "id": "0x0d",
        "dispatchArmVa": "0x0091d4be",
        "interfaceSlot": 43,
        "functionVa": "0x0097eff0",
        "baseCallVa": "0x0097effe",
        "clickModeByte": 1,
        "clickCallVa": "0x0097f075",
        "descriptorVa": "0x01359674",
        "initializerVa": "0x00f22ca0",
        "routingStrategy": 2,
    },
    {
        "event": "PreviewMouseLeftButtonUp",
        "id": "0x0f",
        "dispatchArmVa": "0x0091d4d0",
        "interfaceSlot": 44,
        "functionVa": "0x0097f080",
        "baseCallVa": "0x0097f08c",
        "clickModeByte": 2,
        "clickCallVa": "0x0097f11e",
        "descriptorVa": "0x013593cc",
        "initializerVa": "0x00f22cd0",
        "routingStrategy": 2,
    },
]


ELIGIBILITY_FIELDS = {
    "functionVa": "0x006f5640",
    "sentinelGlobalVa": "0x0130c778",
    "sentinelValue": "0xe0000000",
    "hostChainVa": "0x00cc7510",
    "confidence": "STATIC for supplied-argument, field, instruction and publication joins. Runtime instance identity and field meanings remain UNKNOWN.",
    "context": {
        "pointerWidthBits": 32,
        "actorVtableVa": "0x00fdfb2c",
        "rootSlot": 9,
        "rootThunkVa": "0x0075d4e0",
        "actorConstructorVa": "0x00759510",
        "storedContextOffset": "+0x4",
        "storedControlOffset": "+0x8",
        "passedWrapper": "&LuaActorImpl+0x4",
        "adapterVa": "0x008a3f70",
        "receiverVa": "0x006f56e0",
        "wrapperSaveVa": "0x006f5729",
        "selectorCallVa": "0x006f5774",
        "engineHolderOffset": "+0x220",
        "holderValueOffset": "+0x4",
        "playerOffset": "+0xc",
        "playerClass": "Application::Lua::Script::Client::Control::MyPlayer",
        "playerVtableVa": "0x00fd785c",
        "playerPublicationSlot": 3,
        "playerPublicationVa": "0x006e3440",
        "publisherVa": "0x00758e40",
        "playerStoreVa": "0x00758e5a",
        "getterVa": "0x00707970",
        "ordinaryWidgetVtableVa": "0x00fd5a74",
        "ordinaryWidgetCreationSlot": 27,
        "ordinaryWidgetCreationVa": "0x006e3920",
        "readyInstallerVa": "0x00774700",
        "actorFactoryVa": "0x0076c8d0",
        "holderSetupVa": "0x007634b0",
        "holderConstructorVa": "0x00778d80",
        "holderTableVa": "0x00fdffd0",
        "holderStoreVa": "0x00cd7902",
        "runtimeContextIdentity": "UNKNOWN",
    },
    "gates": [
        {"va": "0x006f5654", "condition": "P=[S+0xc] is null", "widthBits": 32},
        {
            "va": "0x006f5675",
            "condition": "main ItemManager map M+0x8 returns no nonzero value for local dword key 0",
            "keyWidthBits": 32,
        },
        {
            "va": "0x006f567d",
            "condition": "dword P+0x128 differs from dword [0x0130c778]",
            "widthBits": 32,
        },
        {
            "va": "0x006f568e",
            "condition": "the AL returned by 0x00763030(S) is nonzero and byte P+0x161 is nonzero",
            "widthBits": 8,
            "playerByteCheckedWhenAreaByteZero": False,
        },
    ],
    "keyPublication": {
        "managerOffset": "+0xd8",
        "managerSize": "0x78",
        "managerGetterVa": "0x006f51a0",
        "managerConstructorVa": "0x006f5100",
        "managerVtableVa": "0x00fd7618",
        "lookupVa": "0x006e5ba0",
        "mainMapOffset": "+0x8",
        "stagedMapOffset": "+0x14",
        "nodeValueOffset": "+0x10",
        "commandVtableVa": "0x01057130",
        "commandSlot": 9,
        "commandVa": "0x0089b3f0",
        "actorArgument": 2,
        "descriptorOffset": "+0x4",
        "writerVa": "0x006f41a0",
        "descriptorKeyOffset": "+0x6",
        "descriptorKeyWidthBits": 8,
        "keyExtension": "zero",
        "descriptorDwordOffset": "+0x0",
        "descriptorWordOffset": "+0x4",
        "descriptorWordWidthBits": 16,
        "descriptorWordExtension": "zero",
        "storedAllocationSize": "0x24",
        "storedConstructorVa": "0x006ef0d0",
        "descriptorFlagOffset": "+0x7",
        "storedFlagOffset": "+0x20",
        "stagedStoreVa": "0x006f42bb",
        "transferVa": "0x006f7880",
        "transferCallbackVa": "0x007036e0",
        "transferCallVa": "0x007036e9",
        "mainStoreVa": "0x006f79c9",
        "transferredByteOffset": "+0x21",
        "callbackByteOffset": "+0x60",
        "gateTestsTransferredByte": False,
        "gateTestsCallbackByte": False,
        "sameRuntimePlayer": "UNKNOWN",
        "actualKey0Descriptor": "UNKNOWN",
    },
    "stateFields": {
        "dwordOffset": "+0x128",
        "dwordWidthBits": 32,
        "byteOffset": "+0x161",
        "byteWidthBits": 8,
        "constructorVa": "0x006f9140",
        "initialDwordCallVa": "0x006f91e8",
        "initialByteClearVa": "0x006f92b9",
        "descriptorMethodSlot": 30,
        "descriptorMethodVa": "0x006f3420",
        "descriptorDwordOffset": "+0x4c",
        "descriptorDwordStoreVa": "0x006f3459",
        "descriptorByteOffset": "+0xc7",
        "descriptorByteStoreVa": "0x006f349c",
        "secondByteOffset": "+0x162",
        "descriptorSecondByteOffset": "+0xc8",
        "descriptorSecondByteStoreVa": "0x006f34ac",
        "copySource": "+0x162",
        "copyExtension": "sign",
        "copyReadVa": "0x006f354f",
        "copyWrapperVa": "0x00cc9320",
        "copyWrapperCallVa": "0x006f355b",
        "copyLoadVa": "0x006f3565",
        "copyStoreVa": "0x006f3567",
        "globalBranchByteValues": [1, 2, 20],
        "globalBranchVa": "0x0134b78c",
        "globalBranchStoreVa": "0x006f3592",
        "clearSlot": 66,
        "clearVa": "0x006e32f0",
        "clearStoreVa": "0x006e3326",
        "loginSlot": 4,
        "loginVa": "0x0070a350",
        "loginLiteralVa": "0x00fd4c14",
        "loginLiteral": "_onLoginEvent",
        "loginReadVas": ["0x0070a443", "0x0070a45c"],
        "loginExtension": "sign",
        "loginCallVa": "0x0070a4ad",
        "resultHelperVa": "0x0071cd30",
        "resultTestWidthBits": 8,
        "clearByteValues": [1, 2, 20],
        "byteClearVa": "0x0070a4e4",
        "furtherClearVas": ["0x0070a4eb", "0x0070a4f2"],
        "meaning": "UNKNOWN",
    },
    "areaFlag": {
        "helperVa": "0x00763030",
        "listHeadOffset": "+0x1c",
        "listCountOffset": "+0x20",
        "nodeValueOffset": "+0x8",
        "entryByteOffset": "+0xba",
        "returnWidthBits": 8,
        "emptyOrNullReturn": 0,
        "managerOffset": "+0x18",
        "managerConstructionVa": "0x00773335",
        "pendingByteOffset": "+0xba",
        "pendingClearVa": "0x007654b3",
        "setterVa": "0x00758810",
        "setterArgument": 6,
        "setterStoreVa": "0x0075884f",
        "areaVtableVa": "0x00fd63d4",
        "areaSlot": 3,
        "areaMethodVa": "0x006e1670",
        "registrationVa": "0x00768390",
        "copyReadVa": "0x00768417",
        "copyStoreVa": "0x0076841d",
        "resetVa": "0x00768449",
        "selectedWidgetByteRead": False,
        "meaning": "UNKNOWN",
        "runtimeFirstEntry": "UNKNOWN",
    },
    "sourceRefs": [
        "docs/event/player-profile-work-accessors.md#owner-join",
        "config/ffxivgame.vtable_slots.jsonl MyPlayer / LuaActorImpl / ItemManager / WidgetBase / AreaBase / ItemPackageUpdateStartCommand",
        "xivl-client-structs:docs/autotranslate-selection-contract.md AT-ENTER-012 / AT-ENTER-013 / AT-ENTER-024",
    ],
    "unknowns": [
        "The actual matching runtime context, MyPlayer, command descriptor and first area entry at the selected callback.",
        "Which item command supplies descriptor byte +0x6 equal to zero for that same MyPlayer and the individual main-map key removal policy.",
        "The meanings of MyPlayer +0x128 / +0x161 / +0x162 and the first-area +0xba byte beyond the observed operations.",
        "The full ordinary WidgetBase construction-to-close sequence and ordering of finalization, area-list mutation and callback delivery.",
    ],
    "exclusions": [
        "per-widget released-owner predicate",
        "general alive predicate",
        "inventory readiness",
        "target/reference meaning",
        "process-global singleton",
        "runtime validation",
    ],
}

ELIGIBILITY_CHECKS = [
    {
        "kind": "signature",
        "name": "selector drop gates",
        "va": "0x006f5651",
        "pattern": "8B 77 0C 85 F6 74 79 8D 4C 24 08 51 8B CE C7 44 24 0C 00 00 00 00 E8 34 FB FF FF 8B C8 E8 2D 05 FF FF 85 C0 74 5A 8B 96 28 01 00 00 3B 15 78 C7 30 01 75 4C 8B CF E8 A4 D9 06 00 84 C0 74 09 80 BE 61 01 00 00 00 75 38",
        "mask": "FF FF FF FF FF FF 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 00 00 00 00 FF FF FF 00 00 00 00 FF FF FF 00 FF FF FF FF FF FF FF FF FF FF FF FF FF 00 FF FF FF 00 00 00 00 FF FF FF 00 FF FF FF FF FF FF FF FF 00",
    },
    {
        "kind": "signature",
        "name": "eligibility actor context and control stores",
        "va": "0x0075954f",
        "pattern": "8B 11 89 50 04 C6 44 24 10 01 8B 4C 24 1C 89 48 08",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility root context argument",
        "va": "0x0075d4f5",
        "pattern": "8B 56 08 50 51 52 83 C6 04 56 8D 4C 24 17 E8 68 6A 14 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility adapter supplied wrapper",
        "va": "0x008a3f78",
        "pattern": "8B 54 24 04 50 51 8B 4C 24 10 52 E8 58 17 E5 FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility receiver wrapper save",
        "va": "0x006f5729",
        "pattern": "8B 9C 24 30 01 00 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility selector wrapper argument",
        "va": "0x006f5771",
        "pattern": "55 53 57 E8 C7 FE FF FF",
        "mask": "FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility supplied context player load",
        "va": "0x006f5641",
        "pattern": "8B 4C 24 0C 56 57 E8 C4 1E 5D 00 8B 00 8B 78 04 8B 77 0C",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility holder offset",
        "va": "0x00cd7910",
        "pattern": "8D 81 20 02 00 00 C3",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility player publication arguments",
        "va": "0x006e3474",
        "pattern": "8B CF E8 95 40 5E 00 8B 00 8B 48 04 53 56 E8 B9 59 07 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility player publication stores",
        "va": "0x00758e56",
        "pattern": "8B 54 24 08 89 41 0C 8B 02 89 41 10",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility holder publication store",
        "va": "0x00cd7902",
        "pattern": "89 86 20 02 00 00",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility factory receiver save",
        "va": "0x0076c8f7",
        "pattern": "8B E9 89 6C 24 14",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility factory context wrapper",
        "va": "0x0076c9fa",
        "pattern": "8B 7C 24 40 57 8D 75 04 56 8D 4D 08 E8 75 7A 02 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility factory actor constructor call",
        "va": "0x0076ca2f",
        "pattern": "57 56 8B C8 E8 D8 CA FE FF",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility conditional player byte",
        "va": "0x006f568c",
        "pattern": "84 C0 74 09 80 BE 61 01 00 00 00 75 38",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility main lookup member",
        "va": "0x006e5baa",
        "pattern": "8D 71 08",
        "mask": "FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility command cast type arguments",
        "va": "0x0089b3f7",
        "pattern": "68 A4 09 27 01 68 64 09 27 01",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility command actor and descriptor arguments",
        "va": "0x0089b40e",
        "pattern": "83 C6 04 56 50 8B C8 E8 86 9D E5 FF 8B C8 E8 7F 8D E5 FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility descriptor key zero extension",
        "va": "0x006f41d5",
        "pattern": "0F B6 46 06",
        "mask": "FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility descriptor unsigned word",
        "va": "0x006f4255",
        "pattern": "0F B7 4E 04 50 8B 44 24 54 0F B7 C9 51",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility staged map store",
        "va": "0x006f42ac",
        "pattern": "8D 4F 14 89 44 24 54 E8 18 1B 03 00 8B 57 64 89 10",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility main map replace",
        "va": "0x006f7985",
        "pattern": "8B 57 04 89 55 10",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility main map insert",
        "va": "0x006f79ba",
        "pattern": "8B 5F 04 57 8B CE E8 0B E4 02 00 8B 4C 24 14 89 18",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility transfer object byte",
        "va": "0x006f79df",
        "pattern": "C6 42 21 01",
        "mask": "FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility transfer callback call",
        "va": "0x007036e9",
        "pattern": "E8 92 41 FF FF",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility initial dword call",
        "va": "0x006f91e2",
        "pattern": "8D 8E 28 01 00 00 E8 23 01 5D 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility initial player byte clear",
        "va": "0x006f92b9",
        "pattern": "88 9E 61 01 00 00",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility descriptor dword copy",
        "va": "0x006f3453",
        "pattern": "8B 47 4C 56 8B CF 89 86 28 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility descriptor player byte copy",
        "va": "0x006f3494",
        "pattern": "8A 87 C7 00 00 00 3C 01 88 86 61 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility descriptor second byte copy",
        "va": "0x006f34a2",
        "pattern": "8A 8F C8 00 00 00 89 54 24 1C 88 8E 62 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility signed byte to dword copy",
        "va": "0x006f354f",
        "pattern": "0F BE 86 62 01 00 00 50 8D 4C 24 34 E8 C0 5D 5D 00 C6 44 24 28 03 8B 08 89 8E 28 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility dword wrapper copy",
        "va": "0x00cc9320",
        "pattern": "8B C1 8B 4C 24 04 89 08 C2 04 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility global dword overwrite",
        "va": "0x006f358c",
        "pattern": "8B 15 8C B7 34 01 89 96 28 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility paired dword clear",
        "va": "0x006e3321",
        "pattern": "A1 78 C7 30 01 89 86 28 01 00 00 8B 0D 78 C7 30 01 89 8E 2C 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility login signed byte read",
        "va": "0x0070a443",
        "pattern": "0F BE 96 61 01 00 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility login result AL test",
        "va": "0x0070a4c3",
        "pattern": "6A 00 8D 4C 24 30 E8 62 28 01 00 84 C0",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility login player byte clear",
        "va": "0x0070a4e4",
        "pattern": "C6 86 61 01 00 00 00",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility first list entry",
        "va": "0x00763030",
        "pattern": "83 79 20 00 56 74 15 8B 41 1C 8B 30 3B F0 75 05",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility first entry byte return",
        "va": "0x00763050",
        "pattern": "8A 86 BA 00 00 00",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility pending area byte clear",
        "va": "0x007654b3",
        "pattern": "88 9E BA 00 00 00",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility pending area byte argument",
        "va": "0x0075883f",
        "pattern": "8A 54 24 1C 88 86 B8 00 00 00 88 8E B9 00 00 00 88 96 BA 00 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility area registration arguments",
        "va": "0x006e1684",
        "pattern": "56 57 83 C1 18 E8 02 6D 08 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility area byte copy",
        "va": "0x00768417",
        "pattern": "8A 8E BA 00 00 00 88 8F BA 00 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility area pending reset",
        "va": "0x00768449",
        "pattern": "88 9E BA 00 00 00",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "dwords",
        "name": "eligibility sentinel dword",
        "va": "0x0130c778",
        "values": ["0xe0000000"],
    },
    {
        "kind": "dwords",
        "name": "eligibility player publication slot",
        "va": "0x00fd7868",
        "values": ["0x006e3440"],
    },
    {
        "kind": "dwords",
        "name": "eligibility player descriptor slot",
        "va": "0x00fd78d4",
        "values": ["0x006f3420"],
    },
    {
        "kind": "dwords",
        "name": "eligibility player clear slot",
        "va": "0x00fd7964",
        "values": ["0x006e32f0"],
    },
    {
        "kind": "dwords",
        "name": "eligibility player login slot",
        "va": "0x00fd786c",
        "values": ["0x0070a350"],
    },
    {
        "kind": "dwords",
        "name": "eligibility actor root slot",
        "va": "0x00fdfb50",
        "values": ["0x0075d4e0"],
    },
    {
        "kind": "dwords",
        "name": "eligibility ordinary creation slot",
        "va": "0x00fd5ae0",
        "values": ["0x006e3920"],
    },
    {
        "kind": "dwords",
        "name": "eligibility item command slot",
        "va": "0x01057154",
        "values": ["0x0089b3f0"],
    },
    {
        "kind": "dwords",
        "name": "eligibility area registration slot",
        "va": "0x00fd63e0",
        "values": ["0x006e1670"],
    },
    {
        "kind": "string",
        "name": "eligibility actor cast RTTI",
        "va": "0x0127096c",
        "text": ".?AVActorBase@Control@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility chara cast RTTI",
        "va": "0x012709ac",
        "text": ".?AVCharaBase@Control@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility login literal",
        "va": "0x00fd4c14",
        "text": "_onLoginEvent",
    },
    {
        "kind": "string",
        "name": "eligibility myplayer RTTI name",
        "va": "0x012c19ac",
        "text": ".?AVMyPlayer@Control@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility itemmanager RTTI name",
        "va": "0x012c2ea0",
        "text": ".?AVItemManager@Control@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility luaactorimpl RTTI name",
        "va": "0x01270b18",
        "text": ".?AVLuaActorImpl@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility widgetbase RTTI name",
        "va": "0x012c196c",
        "text": ".?AVWidgetBase@Control@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility areabase RTTI name",
        "va": "0x012c2a74",
        "text": ".?AVAreaBase@Control@Client@Script@Lua@Application@@",
    },
    {
        "kind": "string",
        "name": "eligibility core holder RTTI name",
        "va": "0x012c36e0",
        "text": ".?AV?$Holder@PAVCoreWork@Client@Script@Lua@Application@@@AnyType@Misc@Sqex@@",
    },
    {
        "kind": "string",
        "name": "eligibility itempackageupdatestartcommand RTTI name",
        "va": "0x012d7d40",
        "text": ".?AVItemPackageUpdateStartCommand@Item@Command@Client@Script@Lua@Application@@",
    },
    {
        "kind": "dwords",
        "name": "eligibility myplayer RTTI locator",
        "va": "0x00fd7858",
        "values": ["0x0115f6b4"],
    },
    {
        "kind": "dwords",
        "name": "eligibility myplayer RTTI type",
        "va": "0x0115f6c0",
        "values": ["0x012c19a4"],
    },
    {
        "kind": "dwords",
        "name": "eligibility itemmanager RTTI locator",
        "va": "0x00fd7614",
        "values": ["0x0115f4c8"],
    },
    {
        "kind": "dwords",
        "name": "eligibility itemmanager RTTI type",
        "va": "0x0115f4d4",
        "values": ["0x012c2e98"],
    },
    {
        "kind": "dwords",
        "name": "eligibility luaactorimpl RTTI locator",
        "va": "0x00fdfb28",
        "values": ["0x0115fb2c"],
    },
    {
        "kind": "dwords",
        "name": "eligibility luaactorimpl RTTI type",
        "va": "0x0115fb38",
        "values": ["0x01270b10"],
    },
    {
        "kind": "dwords",
        "name": "eligibility widgetbase RTTI locator",
        "va": "0x00fd5a70",
        "values": ["0x0115e6dc"],
    },
    {
        "kind": "dwords",
        "name": "eligibility widgetbase RTTI type",
        "va": "0x0115e6e8",
        "values": ["0x012c1964"],
    },
    {
        "kind": "dwords",
        "name": "eligibility areabase RTTI locator",
        "va": "0x00fd63d0",
        "values": ["0x0115f224"],
    },
    {
        "kind": "dwords",
        "name": "eligibility areabase RTTI type",
        "va": "0x0115f230",
        "values": ["0x012c2a6c"],
    },
    {
        "kind": "dwords",
        "name": "eligibility core holder RTTI locator",
        "va": "0x00fdffcc",
        "values": ["0x0115fee8"],
    },
    {
        "kind": "dwords",
        "name": "eligibility core holder RTTI type",
        "va": "0x0115fef4",
        "values": ["0x012c36d8"],
    },
    {
        "kind": "dwords",
        "name": "eligibility itempackageupdatestartcommand RTTI locator",
        "va": "0x0105712c",
        "values": ["0x0117274c"],
    },
    {
        "kind": "dwords",
        "name": "eligibility itempackageupdatestartcommand RTTI type",
        "va": "0x01172758",
        "values": ["0x012d7d38"],
    },
    {
        "kind": "signature",
        "name": "eligibility lazy manager load and allocation",
        "va": "0x006f51c5",
        "pattern": "8B 86 D8 00 00 00 85 C0 75 4D 6A 78",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility lazy manager pointer store",
        "va": "0x006f5214",
        "pattern": "89 BE D8 00 00 00",
        "mask": "FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility manager table and main map",
        "va": "0x006f5135",
        "pattern": "8D 4E 08 C7 06 18 76 FD 00",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility lookup pointer value",
        "va": "0x006e5be9",
        "pattern": "8B 46 10",
        "mask": "FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility callback manager byte",
        "va": "0x007036f7",
        "pattern": "C6 46 60 01",
        "mask": "FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility area manager construction",
        "va": "0x00773335",
        "pattern": "8D 4E 18 89 5E 14 E8 10 21 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility holder value copy",
        "va": "0x00778dbf",
        "pattern": "8B 11 89 50 04",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "eligibility ready factory arguments",
        "va": "0x0077473a",
        "pattern": "38 5F 5C 74 58 57 8D 44 24 18 50 E8 86 81 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
]


UPSTREAM_SLOT4 = {
    "confidence": "STATIC gated invocation and conditional local order",
    "raptureReceiver": {
        "vtableVa": "0x00f8cc1c",
        "slot": 1,
        "functionVa": "0x004b3c50",
        "colVa": "0x01140ba4",
        "typeDescriptorVa": "0x012695c0",
        "typeName": ".?AVRapture@Application@@",
        "completeObjectOffset": 4,
        "fieldOffsetFromSlotReceiver": 100,
        "fieldOffsetFromCompleteObject": 104,
        "fieldLoadVa": "0x004b3dd1",
    },
    "edges": [
        {
            "siteVa": "0x004b3dd4",
            "targetVa": "0x004daa10",
            "transfer": "call",
            "receiver": "u32[S+0x64], S is the Rapture secondary slot-1 receiver",
        },
        {
            "siteVa": "0x004dab41",
            "targetVa": "0x004da680",
            "transfer": "tail JMP",
            "receiver": "adapter incoming ECX +0x10",
        },
        {
            "siteVa": "0x004da948",
            "targetVa": "0x00578970",
            "transfer": "call",
            "receiver": "incoming ECX +0x510",
        },
        {
            "siteVa": "0x005789d3",
            "targetVa": "0x00cc98a0",
            "transfer": "call",
            "receiver": "u32[incoming ECX+0]",
        },
        {
            "siteVa": "0x00cc98a6",
            "targetVa": "0x00cda330",
            "transfer": "call",
            "receiver": "u32[wrapper+0]",
        },
        {
            "siteVa": "0x00cda452",
            "targetVa": "0x00ce2410",
            "transfer": "call",
            "receiver": "u32[node+8]",
        },
        {
            "siteVa": "0x00ce242a",
            "targetVa": "0x00ccc9d0",
            "transfer": "call",
            "receiver": "u32[record+0]",
        },
        {
            "siteVa": "0x00ccc9d5",
            "target": "receiver vptr +0x10, zero-based slot 4",
            "transfer": "tail JMP",
        },
    ],
    "outerGates": [
        "0x004da680 incoming receiver byte +0x504 ==0 and byte +0x54b ==0 and "
        "0x00575740 AL ==0",
        "local byte at stack +0x17 ==0, selected by unsigned "
        "u32(result_of_0x00443e40-4)>2",
        "0x00578970 incoming receiver byte +0x3b ==0 and 0x00cc9890 AL ==0",
    ],
    "recordWalk": {
        "stopByte": "u8[u32[engine+0x1cc]+8] ==0",
        "nonzeroCountOffset": "+0x194",
        "nodeRootOffset": "+0x190",
        "recordPointerOffset": "+8",
        "lookupVa": "0x0062f320",
        "lookupReceiverOffset": "+0x198",
        "lookupKey": "u32[node+8]",
        "dispatchPredicate": "lookup-not-found, returned iterator +4 equals "
        "saved u32[engine+0x19c]",
        "comparisonVa": "0x00cda43b",
    },
    "recordDispatch": {
        "functionVa": "0x00ce2410",
        "gates": ["u8[record+0x7e] ==0", "u8[record+0x80] !=0"],
        "receiver": "u32[record+0]",
        "arguments": ["engine-wrapper pointer", "record+0x70"],
        "vtableOffset": "+0x10",
        "zeroBasedSlot": 4,
        "constructorVa": "0x00ce2bc0",
        "controlStoreVa": "0x00ce2bf7",
        "backlinkStoreVa": "0x00ce2c90",
        "constructorEnableClearVa": "0x00ce2c86",
        "constructorEnablesDispatch": False,
    },
    "localOrder": {
        "walkCallVa": "0x00cc98a6",
        "laterCallVa": "0x00cc98b5",
        "laterHelperVa": "0x00cf1b50",
        "relation": "engine walk returns normally before later helper call, "
        "including skipped walk",
        "allNotificationsOrdered": False,
    },
    "numeric1": {
        "conditionalCallVa": "0x00cf1d16",
        "helperVa": "0x00cce1c0",
        "firstComparisonGlobalVa": "0x0130cecc",
        "secondComparisonGlobalVa": "0x0130ced0",
        "selection": "first comparison unequal, second comparison equal, subject "
        "to intervening gates",
        "dispatcherCallVa": "0x00cce21a",
        "dispatcherVa": "0x00ccd910",
        "literal": 1,
        "everyPass": False,
    },
    "unknowns": [
        "scheduling above the Rapture secondary vtable and fixed cadence",
        "every concrete widget record registration, enabling and visitation",
        "exhaustive input, numeric phase and presentation ordering",
        "other threads and reentry",
        "next-pass or eventual replay",
    ],
}

UPSTREAM_SLOT4_CHECKS = [
    {
        "kind": "signature",
        "name": "upstream Rapture secondary vptr write",
        "va": "0x004b3acb",
        "pattern": "8D 7E 04 C7 06 30 CC F8 00 C7 07 1C CC F8 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream Rapture slot receiver",
        "va": "0x004b3c92",
        "pattern": "8B F1",
        "mask": "FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream Rapture receiver field and call",
        "va": "0x004b3dd1",
        "pattern": "8B 4E 64 E8 37 6C 02 00",
        "mask": "FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream adapter adjusted tail JMP",
        "va": "0x004dab38",
        "pattern": "8D 4F 10 5F 5E 5D 83 C4 08 E9 3A FB FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream application initial byte gate",
        "va": "0x004da6a9",
        "pattern": "80 BE 04 05 00 00 00 74 16",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream application second byte gate",
        "va": "0x004da715",
        "pattern": "80 BE 4B 05 00 00 00 0F 85 5E 02 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream application AL gate",
        "va": "0x004da728",
        "pattern": "E8 13 B0 09 00 84 C0 0F 85 4B 02 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream application unsigned range",
        "va": "0x004da746",
        "pattern": "83 C0 FC 83 F8 02 77 06 88 5C 24 17 EB 55",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream application local zero store",
        "va": "0x004da763",
        "pattern": "C6 44 24 1F 00",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream application local gate and call",
        "va": "0x004da93b",
        "pattern": "80 7C 24 17 00 75 0B 8D 8E 10 05 00 00 E8 23 E0 09 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream pass byte gate",
        "va": "0x00578996",
        "pattern": "80 7E 3B 00 0F 85 F1 00 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream pass stop gate",
        "va": "0x005789a0",
        "pattern": "8B 0E E8 E9 0E 75 00 84 C0 0F 85 E2 00 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream pass wrapper call",
        "va": "0x005789cc",
        "pattern": "8B 0E 8D 54 24 0C 52 E8 C8 0E 75 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream wrapper stop pointer chain",
        "va": "0x00cc9890",
        "pattern": "8B 01 8B 88 CC 01 00 00 8A 41 08 C3",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk before helper calls",
        "va": "0x00cc98a3",
        "pattern": "8B 0E 57 E8 85 0A 01 00 8B F8 8B 06 8B 88 C0 01 00 00 E8 96 82 02 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk stop byte gate",
        "va": "0x00cda37b",
        "pattern": "8B 83 CC 01 00 00 8A 40 08 84 C0 74 16",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk count gate",
        "va": "0x00cda39e",
        "pattern": "83 BB 94 01 00 00 00 0F 84 10 02 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk node root",
        "va": "0x00cda3bc",
        "pattern": "8B 83 90 01 00 00 8B 00 8B 8B 90 01 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk saved lookup sentinel",
        "va": "0x00cda3f5",
        "pattern": "8B 93 9C 01 00 00 8D B3 98 01 00 00 89 54 24 28",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk lookup key and call",
        "va": "0x00cda416",
        "pattern": "8D 45 08 50 8D 44 24 30 50 8B CE E8 FA 4E 95 FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk lookup not found branch",
        "va": "0x00cda437",
        "pattern": "8B 4C 24 28 39 4B 04 75 17",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk record receiver and call",
        "va": "0x00cda44a",
        "pattern": "8B 4D 08 8D 54 24 18 52 E8 B9 7F 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream record control constructor argument",
        "va": "0x00ce2be9",
        "pattern": "8B 44 24 20 33 DB 89 5C 24 18 89 5C 24 20 89 06",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream record constructor disabled byte",
        "va": "0x00ce2c83",
        "pattern": "88 5E 7E 88 9E 80 00 00 00",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream record control backlink",
        "va": "0x00ce2c8e",
        "pattern": "8B 06 89 70 04",
        "mask": "FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream control record resolver",
        "va": "0x00cd7a3d",
        "pattern": "8B 81 BC 01 00 00 3B 10 74 03 8B 42 04 C2 04 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream walk engine wrapper construction",
        "va": "0x00cc77e0",
        "pattern": "8B C1 8B 4C 24 04 89 08 C2 04 00",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream record dispatch byte gates",
        "va": "0x00ce2410",
        "pattern": "80 79 7E 00 75 19 80 B9 80 00 00 00 00 74 10",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream record dispatch receiver and arguments",
        "va": "0x00ce241f",
        "pattern": "8B 54 24 04 8D 41 70 8B 09 50 52 E8 A1 A5 FE FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream slot 4 tail thunk",
        "va": "0x00ccc9d0",
        "pattern": "8B 01 8B 40 10 FF E0",
        "mask": "FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream numeric first comparison",
        "va": "0x00cf1cdc",
        "pattern": "8B 44 24 18 3B 05 CC CE 30 01 75 08",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream numeric conditional join",
        "va": "0x00cf1d08",
        "pattern": "8B 54 24 18 3B 15 D0 CE 30 01 8B CF 75 07 E8 A5 C4 FD FF",
        "mask": "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "signature",
        "name": "upstream numeric literal and dispatch",
        "va": "0x00cce216",
        "pattern": "6A 01 8B CE E8 F1 F6 FF FF",
        "mask": "FF FF FF FF FF FF FF FF FF",
    },
    {
        "kind": "dwords",
        "name": "upstream Rapture COL pointer",
        "va": "0x00f8cc18",
        "values": ["0x01140ba4"],
    },
    {
        "kind": "dwords",
        "name": "upstream Rapture COL coordinates and type",
        "va": "0x01140ba8",
        "values": ["0x00000004", "0x00000000", "0x012695c0"],
    },
    {
        "kind": "dwords",
        "name": "upstream Rapture slot 1",
        "va": "0x00f8cc20",
        "values": ["0x004b3c50"],
    },
    {
        "kind": "string",
        "name": "upstream Rapture RTTI name",
        "va": "0x012695c8",
        "text": ".?AVRapture@Application@@",
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
        *upstream_slot4_addresses(document),
        *button_addresses(document),
        *eligibility_addresses(document),
    ]
    errors = [
        f"page does not cite {address}"
        for address in wanted
        if address and address.lower() not in text
    ]
    lead = text.split("## input and method")[0]
    if "0x59" not in lead or "wildcard" not in lead:
        errors.append("page lead does not state the 0x59 wildcard join")
    script_entry = text.split("## script entry", 1)[-1].split("\n## ", 1)[0]
    script_entry = re.sub(r"\s+", " ", script_entry)
    for claim in (
        "gates read as liveness checks",
        "a script object whose owner was released",
        "for widget script objects and not for actor-bound ones",
    ):
        if claim in script_entry:
            errors.append("unsupported widget eligibility inference")
    if "the script object has an owner and the widget is live" in re.sub(
        r"\s+", " ", text
    ):
        errors.append("unsupported widget eligibility inference")
    if "unknown" not in script_entry:
        errors.append("eligibility uncertainty is missing")
    upstream = text.split("### upstream control slot 4", 1)
    section = re.sub(r"\s+", " ", upstream[-1].split("\n## ", 1)[0])
    for claim in (
        "complete rapture +0x68",
        "lookup-not-found",
        "unknown",
        "no next-pass or eventual replay guarantee",
    ):
        if len(upstream) != 2 or claim not in section:
            errors.append(f"upstream slot4 page boundary missing: {claim}")
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


def upstream_slot4_addresses(document: dict) -> list[str]:
    """Locators of the bounded route, secondary receiver and conditional join."""
    route = (
        document.get("scriptInterface", {})
        .get("eventQueue", {})
        .get("upstreamSlot4", {})
    )
    receiver = route.get("raptureReceiver", {})
    return [
        receiver.get("vtableVa"),
        receiver.get("colVa"),
        receiver.get("typeDescriptorVa"),
        *[row.get("siteVa") for row in route.get("edges", [])],
        *[row.get("va") for row in UPSTREAM_SLOT4_CHECKS],
    ]


def upstream_slot4_errors(document: dict) -> list[str]:
    """Pin the observed route while keeping scheduling and replay unknown."""
    errors: list[str] = []
    route = (
        document.get("scriptInterface", {})
        .get("eventQueue", {})
        .get("upstreamSlot4", {})
    )
    for key, expected in UPSTREAM_SLOT4.items():
        if route.get(key) != expected:
            errors.append(f"upstream slot4 {key} changed")
    receiver = route.get("raptureReceiver", {})
    offsets = [
        receiver.get(key)
        for key in (
            "completeObjectOffset",
            "fieldOffsetFromSlotReceiver",
            "fieldOffsetFromCompleteObject",
        )
    ]
    if (
        not all(type(value) is int for value in offsets)
        or offsets[0] + offsets[1] != offsets[2]
    ):
        errors.append("upstream slot4 secondary receiver coordinate changed")
    checks = document.get("staticChecks", [])
    for expected in UPSTREAM_SLOT4_CHECKS:
        found = [row for row in checks if row.get("name") == expected["name"]]
        if found != [expected]:
            errors.append(f"upstream slot4 static check changed: {expected['name']}")
    return errors


def button_addresses(document: dict) -> list[str]:
    """Locators of the concrete Button source and its conditional command join."""
    button = document.get("buttonSource", {})
    return [
        button.get("constructorVa"),
        button.get("primaryVtableVa"),
        button.get("interfaceVtableVa"),
        button.get("typeDescriptorVa"),
        button.get("interfaceCompleteObjectLocatorVa"),
        *[row.get("descriptorVa") for row in button.get("mouseIngress", [])],
        *[row.get("functionVa") for row in button.get("mouseIngress", [])],
        button.get("click", {}).get("recipientVa"),
        button.get("click", {}).get("commandCallVa"),
        button.get("storedCommand", {}).get("propertySetterVa"),
        button.get("storedCommand", {}).get("targetHelperVa"),
        button.get("routedCommandJoin", {}).get("functionVa"),
        button.get("executedForwarding", {}).get("functionVa"),
        button.get("executedForwarding", {}).get("frameworkElementCallVa"),
        button.get("executedForwarding", {}).get("baseCallVa"),
        *[
            button.get("eligibilityProperty", {}).get(key)
            for key in (
                "descriptorVa",
                "initializerVa",
                "getterDispatcherVa",
                "getterVa",
                "setterVa",
                "refreshDispatcherVa",
                "refreshHelperVa",
                "writerVa",
                "writerValueStoreVa",
            )
        ],
    ]


def button_errors(document: dict) -> list[str]:
    """Preserve the routed-ingress boundary, byte gates, pointer ABI and base call."""
    errors: list[str] = []
    button = document.get("buttonSource", {})
    for section, expected in BUTTON_FIELDS.items():
        actual = button if section == "identity" else button.get(section, {})
        if any(actual.get(key) != value for key, value in expected.items()):
            errors.append(f"button {section} contract changed")
    ingress = button.get("mouseIngress", [])
    if len(ingress) != len(BUTTON_INGRESS) or any(
        any(actual.get(key) != value for key, value in expected.items())
        for actual, expected in zip(ingress, BUTTON_INGRESS)
    ):
        errors.append("button preview ingress changed")
    if (
        button.get("confidence")
        != "STATIC for instructions, pointer/RTTI data and conditions. INFERRED for naming the IsPressed value bit and for Click EventTrigger ordering conditional on attachment."
    ):
        errors.append("button confidence boundary changed")
    if button.get("unknowns") != [
        "OS input, hit testing and capture before routed preview ingress.",
        "Exhaustive IsHitTestVisible aliases and writers, late property-resolution inputs and dynamic metadata defaults.",
        "All packed-state writers, ClickMode binding changes and actual attached Click EventTriggers.",
        "Other ICommand bodies and other specialised control families.",
        "Runtime activation, Lua result, frame timing, network and server behavior.",
    ]:
        errors.append("button unknown boundaries changed")
    checks = document.get("staticChecks", [])
    if button.get("eligibilityProperty") != BUTTON_ELIGIBILITY_PROPERTY:
        errors.append("button eligibility property contract changed")
    for expected in BUTTON_CHECKS + BUTTON_ELIGIBILITY_CHECKS:
        found = [row for row in checks if row.get("name") == expected["name"]]
        if found != [expected]:
            errors.append(f"button static check changed: {expected['name']}")
    return errors


def eligibility_addresses(document: dict) -> list[str]:
    """Locators for the supplied-context player, conditional key and raw writers."""
    selector = document.get("selectorGates", {})
    context = selector.get("context", {})
    key = selector.get("keyPublication", {})
    state = selector.get("stateFields", {})
    area = selector.get("areaFlag", {})
    return [
        context.get("actorConstructorVa"),
        context.get("rootThunkVa"),
        context.get("playerPublicationVa"),
        context.get("publisherVa"),
        context.get("holderStoreVa"),
        key.get("commandVa"),
        key.get("writerVa"),
        key.get("transferVa"),
        state.get("copyReadVa"),
        state.get("copyWrapperVa"),
        state.get("copyStoreVa"),
        area.get("copyStoreVa"),
        area.get("resetVa"),
    ]


def eligibility_errors(document: dict) -> list[str]:
    """Keep raw supplied-context facts separate from unknown live meanings."""
    selector = document.get("selectorGates", {})
    errors = [
        f"eligibility {key} contract changed"
        for key, expected in ELIGIBILITY_FIELDS.items()
        if selector.get(key) != expected
    ]
    checks = document.get("staticChecks", [])
    for expected in ELIGIBILITY_CHECKS:
        rows = [row for row in checks if row.get("name") == expected["name"]]
        if rows != [expected]:
            errors.append(f"eligibility static check changed: {expected['name']}")
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
        document.get("selectorGates", {}).get("sentinelGlobalVa"),
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
        selector.get("sentinelValue") != "0xe0000000"
        or selector.get("sentinelGlobalVa") != "0x0130c778"
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
    errors.extend(button_errors(document))
    errors.extend(eligibility_errors(document))
    errors.extend(event_release_errors(document))
    errors.extend(upstream_slot4_errors(document))
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
        (("buttonSource", "mouseGates", "snapshotBeforeRefresh"), False),
        (("buttonSource", "mouseGates", "handledWidthBits"), 32),
        (("buttonSource", "storedCommand", "recordPointerOffset"), "+0x3a4"),
        (("buttonSource", "click", "defaultCommandBeforeGeneric"), False),
        (("buttonSource", "click", "triggerAttachment"), "STATIC installed"),
        (("buttonSource", "executedForwarding", "interfaceSlot"), 64),
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
        (("selectorGates", "sentinelValue"), "0xffffffff"),
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
