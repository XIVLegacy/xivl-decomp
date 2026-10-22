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
}


def parse_hex(text: str) -> int:
    return int(text, 16)


def parse_pattern(text: str) -> bytes:
    return bytes.fromhex(text)


def doc_errors(document: dict) -> list[str]:
    """Tie the manifest to the page: key addresses are cited and the join is labelled."""
    if not DOC.is_file():
        return ["consumer page is missing"]
    text = DOC.read_text(encoding="utf-8").lower()
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
    ]
    errors = [
        f"page does not cite {address}"
        for address in wanted
        if address and address.lower() not in text
    ]
    lead = text.split("## input and method")[0]
    if "inferred" not in lead or "0x59" not in text:
        errors.append("page lead does not label the Executed join as inferred")
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
        or handler.get("executedJoin") != "inferred"
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
            if width not in (1, 4) or not values:
                errors.append(f"{label} width or values invalid")
        elif kind == "string":
            if not check.get("text"):
                errors.append(f"{label} has no text")
        else:
            errors.append(f"{label} has unknown kind")
    if len(document.get("staticChecks", [])) < 50:
        errors.append("static check set shrank")
    names = {check.get("name") for check in document.get("staticChecks", [])}
    missing = REQUIRED_CHECKS - names
    if missing:
        errors.append(f"static checks missing: {sorted(missing)}")
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
        (("handler", "executedJoin"), "static"),
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
        (("parameterObjects", "layout", "int1"), "+0x104"),
        (("parameterObjects", "pushOrder"), "string0, string1, string2"),
        (("commandSource", "cases", 2, "textAt"), "+0x54"),
        (("source", "binarySha256"), "0" * 64),
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
    for dropped in ("request depth limit", "thunk slot 8 child none"):
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
