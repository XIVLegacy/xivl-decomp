#!/usr/bin/env python3
"""Validate the Qix effect runtime manifest, optional exe bytes and install census."""

from __future__ import annotations

import argparse
import collections
import copy
import json
import re
import struct
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks
import vfx_effect_body as body
import vfx_qix_resource as qix


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "vfx_effect_runtime.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
HEX_RE = re.compile(r"^0x[0-9a-f]{8}$")
SIG_RE = re.compile(r"^[0-9a-f]{12}$")
TIERS = {"STATIC", "INFERRED", "UNKNOWN"}
DESCRIPTOR_SIZE = 0xE8
FAMILY_SIZE = 24
GRAPH_SIZE = 0x30
SLOT_OFFSETS = tuple(f"0x{offset:02x}" for offset in range(0x68, 0xE0, 4))
ENGINE_ROW = 0x20
ENGINE_COUNT = 21
SCOPE = (
    "owner installation of build 2012.09.19.0001, client/vfx tree,"
    " counts and structure only"
)
REQUIRED_CHECKS = set(
    """context graph tables
Rapture module table passed to the registrar
CDev module table passed to the registrar
table pair registrar calls the module and family setters
module copy uses descriptor size 0xe8
module registration reads the name and stores the index
module registration stores the parent descriptor
declaration compare reads six signature bytes
declaration compare returns the module index
unknown control path
family alias compare reads former name and revision bytes
veff version gate
veff control array reads
veff declaration table read
veff link array read
veff name counts read
veff unit array read
veff instance count and size read
veff unit loop count read
veff unit pointer read
veff instance loop count read
failed root falls back to the context default
fallback builds from context offset 0x24
fallback lifetime store
lifetime word and endless flag
instance flag and unit index read
unit end byte test
graph A module lookup
graph B module lookup
graph record binding walk
graph overflow id check
control slot record reads
slot pointer array start
slot pointer count check
polynomial segment init
polynomial segment evaluate
polynomial segment derivative
half constant
material element list read
material element state reads
depth bias state
depth bias setter
alpha reference state
Position3D calc reads the slot vector
depth bias divisor
creator leafins path
creator leaf path
EffectClip begin reaches the listener
EffectClip name pointer
kill log reads the root path
context default root accessor
context default root store
embedded root effect names
embedded root effect version
embedded root effect lifetime
context update calls effect slot 0x118
effect time scale
effect adds the tick delta
effect life control lookup
endless effect kill threshold
control construct slot
control leaf construct slot
control draw construct slot
control destruct slot
control draw destruct slot
control calc slot
control phase 0x12 slot
child generation record read
creator dispatch
cut listener create calls the creator
layout manipulator create calls the creator
factory kind dispatch
DrawResource alpha gate and link reads
DrawResource calc flipbook advance
flipbook wrap
flipbook cell and scale math
blend byte map
blend byte map result 8
per draw state apply
blend mode setter dispatch
blend type setter
blend enable state
depth write state
alpha test state
depth enable state
depth function state
cull state
blend op state
blend factor state
alpha function state
control color slot store
Rapture module registry struct
CDev module registry struct
EffectFactory vtable
CutEffectListener vtable
LayEffectManipulator creator slot
blend factor table
blend op table
compare function table
cull mode table
blend mode jump table
blend type jump table
blend byte jump table
tick constants
second clock constant
declaration version error
unknown control message
former family message
veff version error
missing life message
malloc error message
EffectClip clip name
bounds read
culling type read
shader parameter hint size
link code resolver
control link storage offsets
link provider slot read
default link target read
expression link storage offset read
blend mode 0 body
blend mode 1 body
blend mode 2 body
blend mode 3 body
blend mode 4 body
blend mode 6 body
blend mode 7 body
blend type 0 body
blend type 1 body
blend type 2 body
blend type 3 body
blend type 4 body
blend type 7 body""".splitlines()
)
EXPECTED_OUTCOMES = {
    "module-registry-tables": "STATIC",
    "module-descriptor-layout": "STATIC",
    "declaration-compliance": "STATIC",
    "family-aliases": "STATIC",
    "graph-module-tables": "STATIC",
    "graph-polynomial-segments": "STATIC",
    "control-slot-references": "STATIC",
    "veff-version-gate-and-fallback": "STATIC",
    "veff-root-directory": "STATIC",
    "effect-lifetime-and-bounds": "STATIC",
    "link-code-resolution": "STATIC",
    "control-phase-slots": "STATIC",
    "effect-clock-constants": "STATIC",
    "effect-update-order": "STATIC",
    "spawn-routes": "STATIC",
    "drawresource-gate-and-links": "STATIC",
    "drawresource-flipbook": "STATIC",
    "material-element-state": "STATIC",
    "render-state-application": "STATIC",
    "blend-modes": "STATIC",
    "engine-material-constants": "STATIC",
    "vmdl-shader-records": "INFERRED",
    "unit-tree-shape": "INFERRED",
    "control-and-link-roles": "INFERRED",
    "tick-rate": "INFERRED",
    "graph-evaluation-time": "UNKNOWN",
    "control-parameter-blocks": "UNKNOWN",
    "vfxm-chunk-layout": "UNKNOWN",
    "material-parameter-types": "UNKNOWN",
    "frame-delta-source": "UNKNOWN",
}
VEFF_COUNT_KEYS = (
    "controls",
    "links",
    "units",
    "instances",
    "decls",
)


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-vfx-effect-runtime-v1":
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
    errors += registry_errors(document)
    errors += module_errors(document)
    errors += graph_errors(document)
    errors += engine_errors(document)
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
        elif not HEX_RE.fullmatch(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            pattern, mask = item.get("pattern", ""), item.get("mask", "")
            try:
                same = len(bytes.fromhex(pattern)) == len(bytes.fromhex(mask)) > 0
            except ValueError:
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
        elif item["kind"] == "dwords" and not all(
            HEX_RE.fullmatch(str(value)) for value in item.get("values", [None])
        ):
            errors.append(f"staticChecks {item['name']!r} has a malformed value")
    known = set(names)
    for label in ("descriptorFields", "veffFields"):
        errors += field_errors(document.get(label), label, known)
    elements = document.get("elements")
    if not isinstance(elements, dict) or sorted(elements) != [
        "control",
        "graphBinding",
        "graphRecord",
        "graphSegment",
        "instance",
        "link",
        "materialElement",
        "slot",
        "unit",
    ]:
        errors.append("elements keys changed")
    else:
        for label, rows in elements.items():
            errors += field_errors(rows, f"elements.{label}", known)
    codes = document.get("linkCodes", [])
    if [row.get("top") for row in codes] != ["0x0", "0x1", "0x2", "0x3"]:
        errors.append("linkCodes are not the four top nibbles in order")
    if any(row.get("tier") not in TIERS for row in codes):
        errors.append("linkCodes has an unknown tier")
    outcomes = document.get("outcomes")
    if not isinstance(outcomes, list):
        errors.append("outcomes is not a list")
    else:
        found = {item.get("id"): item.get("tier") for item in outcomes}
        if len(found) != len(outcomes) or found != EXPECTED_OUTCOMES:
            errors.append("outcome ids or tiers changed")
        if any(not str(item.get("claim", "")).strip() for item in outcomes):
            errors.append("an outcome has no claim")
    errors += install_errors(document.get("install", {}))
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def field_errors(rows, label: str, known: set[str]) -> list[str]:
    if not isinstance(rows, list) or not rows:
        return [f"{label} is not a non-empty list"]
    errors = []
    for row in rows:
        if row.get("tier") not in TIERS:
            errors.append(f"{label} {row.get('field')!r} has an unknown tier")
        elif row["tier"] == "STATIC" and row.get("check") not in known:
            errors.append(f"{label} {row.get('field')!r} names no static check")
        if not re.fullmatch(r"0x[0-9a-f]{2,3}", str(row.get("offset", ""))):
            errors.append(f"{label} {row.get('field')!r} has a malformed offset")
    return errors


FIXED_TABLES = {
    "cdev": {
        "structVa": "0x010b9da8",
        "moduleTable": "0x01300178",
        "moduleCount": 56,
        "familyTable": "0x010b9d30",
        "familyCount": 5,
        "registrar": "0x00b94a20",
        "registrarCall": "0x00b94d9c",
    },
    "rapture": {
        "structVa": "0x0111cbb8",
        "moduleTable": "0x0130f4e8",
        "moduleCount": 162,
        "familyTable": "0x0111caf8",
        "familyCount": 8,
        "registrar": "0x004e8870",
        "registrarCall": "0x004e888c",
    },
}
FIXED_GRAPHS = {"standard": "0x01304350", "random": "0x01304f50"}


def registry_errors(document: dict) -> list[str]:
    registry = document.get("registry", {})
    errors = []
    for row in registry.get("tables", []):
        if row != {"id": row.get("id"), **FIXED_TABLES.get(row.get("id"), {})}:
            errors.append(f"registry table {row.get('id')} changed")
    for row in registry.get("graphTables", []):
        if row.get("va") != FIXED_GRAPHS.get(row.get("id")) or row.get("count") != 64:
            errors.append(f"registry graph table {row.get('id')} changed")
    if registry.get("engineTable") != "0x013086a0":
        errors.append("registry.engineTable changed")
    prefix = registry.get("namePrefix", {})
    if sorted(prefix) != ["cdev", "rapture"] or any(
        not value.endswith("/Controls/") for value in prefix.values()
    ):
        errors.append("registry.namePrefix changed")
    tables = registry.get("tables", [])
    if [row.get("id") for row in tables] != ["cdev", "rapture"]:
        errors.append("registry.tables is not cdev then rapture")
    for row in tables:
        for key in ("structVa", "moduleTable", "familyTable", "registrarCall"):
            if not HEX_RE.fullmatch(str(row.get(key, ""))):
                errors.append(f"registry table {row.get('id')} {key} is malformed")
    graphs = registry.get("graphTables", [])
    if [row.get("id") for row in graphs] != ["standard", "random"]:
        errors.append("registry.graphTables is not standard then random")
    if (
        registry.get("descriptorSize") != DESCRIPTOR_SIZE
        or registry.get("graphEntrySize") != GRAPH_SIZE
    ):
        errors.append("registry entry sizes changed")
    root = registry.get("rootEffect", {})
    if root.get("record") != "0x01303e68" or root.get("names") != "0x01303e24":
        errors.append("registry.rootEffect changed")
    return errors


def module_errors(document: dict) -> list[str]:
    errors = []
    modules = document.get("modules")
    tables = document.get("registry", {}).get("tables", [])
    if not isinstance(modules, list):
        return ["modules is not a list"]
    counts = {row["id"]: row["moduleCount"] for row in tables if "id" in row}
    seen: dict[str, set[str]] = {key: set() for key in counts}
    index_of: dict[str, int] = {key: 0 for key in counts}
    for row in modules:
        table = row.get("table")
        if table not in counts:
            errors.append("a module row names an unknown table")
            continue
        if row.get("index") != index_of[table]:
            errors.append(f"module {table} rows are not in index order")
        index_of[table] += 1
        name = row.get("name")
        if not isinstance(name, str) or not name or name in seen[table]:
            errors.append(f"module {table} name {name!r} is empty or repeated")
        seen[table].add(str(name))
        if not SIG_RE.fullmatch(str(row.get("signature", ""))):
            errors.append(f"module {name!r} signature is malformed")
        for key in (
            "kind",
            "unitFlags",
            "workSize",
            "workAlign",
            "slotArrayOffset",
            "staticLinkOffset",
            "exprLinkOffset",
            "revision",
        ):
            if not isinstance(row.get(key), int):
                errors.append(f"module {name!r} field {key} is not an integer")
        slots = row.get("slots", {})
        if any(
            key not in SLOT_OFFSETS or not HEX_RE.fullmatch(str(value))
            for key, value in slots.items()
        ):
            errors.append(f"module {name!r} slots are malformed")
    if {key: index_of[key] for key in counts} != counts:
        errors.append("module row counts differ from the registry tables")
    prefix = document["registry"]["namePrefix"]
    every = set()
    for table, names in seen.items():
        every |= {prefix.get(table, "") + name for name in names}
    for row in modules:
        parent = row.get("parent")
        if parent is not None:
            wanted = (
                prefix["rapture"] + parent[2:]
                if parent.startswith("R:")
                else prefix["cdev"] + parent
            )
            if wanted not in every:
                errors.append(f"module {row.get('name')!r} parent is not a module")
    families = document.get("families", [])
    expected_families = {row["id"]: row["familyCount"] for row in tables if "id" in row}
    for table, count in expected_families.items():
        rows = [row for row in families if row.get("table") == table]
        if len(rows) != count or [row.get("index") for row in rows] != list(
            range(count)
        ):
            errors.append(f"families for {table} are not indexed 0 to {count - 1}")
        for row in rows:
            if row.get("current") not in seen.get(table, set()):
                errors.append(f"family {row.get('family')!r} current is not a module")
            if (row.get("former") is None) != (row.get("formerRevision") == 0):
                errors.append(f"family {row.get('family')!r} former pair is odd")
    return errors


def graph_errors(document: dict) -> list[str]:
    rows = document.get("graphModules")
    if not isinstance(rows, list) or not rows:
        return ["graphModules is not a non-empty list"]
    errors = []
    seen = set()
    for row in rows:
        key = (row.get("table"), row.get("index"))
        if key in seen or row.get("table") not in ("standard", "random"):
            errors.append(f"graph module {key} is repeated or in an unknown table")
        seen.add(key)
        if not isinstance(row.get("index"), int) or not 0 <= row["index"] < 64:
            errors.append(f"graph module {key} index is out of range")
        if not re.fullmatch(r"(Standard|Random)[A-Za-z0-9]+", str(row.get("name"))):
            errors.append(f"graph module {key} name is malformed")
        if len(row.get("slots", [])) != 7 or any(
            not HEX_RE.fullmatch(str(value)) for value in row["slots"]
        ):
            errors.append(f"graph module {key} slots are malformed")
        expected = "Standard" if row.get("table") == "standard" else "Random"
        if not str(row.get("name")).startswith(expected):
            errors.append(f"graph module {key} sits in the wrong table")
    return errors


def engine_errors(document: dict) -> list[str]:
    rows = document.get("engineConstants")
    if not isinstance(rows, list) or len(rows) != ENGINE_COUNT:
        return ["engineConstants is not 21 rows"]
    errors = []
    if len({row.get("name") for row in rows}) != ENGINE_COUNT:
        errors.append("engineConstants names are repeated")
    for row in rows:
        if row.get("bytes") not in (8, 12, 16, 64):
            errors.append(f"engine constant {row.get('name')!r} size is odd")
        if len(row.get("defaults", [])) != 4:
            errors.append(f"engine constant {row.get('name')!r} lacks four defaults")
    return errors


def install_errors(install: dict) -> list[str]:
    errors = []
    if install.get("scope") != SCOPE:
        errors.append("install.scope changed")
    veff = install.get("veff", {})
    count = veff.get("count")
    versions = veff.get("versions", {})
    current = versions.get("0x1d", 0)
    if count != sum(versions.values()):
        errors.append("install.veff versions disagree with the count")
    if current != sum(veff.get("cullingTypes", {}).values()):
        errors.append("install.veff culling types disagree with the current count")
    for key, value in veff.get("relations", {}).items():
        if value > current:
            errors.append(f"install.veff relation {key} exceeds the current count")
    totals = veff.get("totals", {})
    if sorted(totals) != sorted(VEFF_COUNT_KEYS):
        errors.append("install.veff totals are not the five arrays")
    elif (
        totals["controls"] != totals["links"] or totals["units"] != totals["instances"]
    ):
        errors.append("install.veff totals break the control and unit pairing")
    decls = install.get("decls", {})
    for label in ("veff", "veffOlder", "leaf", "vins"):
        row = decls.get(label, {})
        if row.get("entries") != sum(row.get("names", {}).values()):
            errors.append(f"install.decls {label} entries disagree with the names")
        if row.get("entries") != (
            row.get("signatureMatch", 0)
            + row.get("signatureMismatch", 0)
            + row.get("unresolved", 0)
        ):
            errors.append(f"install.decls {label} signature counts disagree")
    if decls.get("veff", {}).get("entries") != totals.get("decls"):
        errors.append("install.decls veff entries differ from the veff totals")
    draw = install.get("drawResource", {})
    if draw.get("blocks") != sum(draw.get("kinds", {}).values()):
        errors.append("install.drawResource kinds disagree with the block count")
    links = draw.get("links", {})
    if links.get("nodes") != draw.get("blocks"):
        errors.append("install.drawResource link nodes differ from the blocks")
    if (
        not links.get("bothLocal", 0)
        <= links.get("twoLinks", 0)
        <= links.get("nodes", 0)
    ):
        errors.append("install.drawResource link counts are not nested")
    for key in ("color", "matrix"):
        if sum(links.get(key, {}).values()) != links.get("bothLocal"):
            errors.append(f"install.drawResource {key} names disagree with the links")
    kinds = draw.get("kinds", {})
    if draw.get("modelRecords") != sum(kinds.get(key, 0) for key in ("1", "3", "8")):
        errors.append("install.drawResource model records disagree with the kinds")
    if max(
        draw.get("modelTag", 0),
        draw.get("modelFlagsClear", 0),
        draw.get("modelInReferences", 0),
    ) > draw.get("modelRecords", 0):
        errors.append("install.drawResource model relations exceed the record count")
    vmdl = install.get("vmdl", {})
    sets = vmdl.get("shaderSets", {})
    if vmdl.get("count") != sum(sets.values()):
        errors.append("install.vmdl shader set histogram disagrees with the count")
    if vmdl.get("shaderRecords") != 2 * sum(
        int(key) * value for key, value in sets.items()
    ):
        errors.append("install.vmdl shader record count disagrees")
    for key in ("vertexTokens", "pixelTokens"):
        if sum(vmdl.get(key, {}).values()) * 2 != vmdl.get("shaderRecords"):
            errors.append(f"install.vmdl {key} disagree with the record count")
    slots = install.get("slots", {})
    if slots.get("records") != slots.get("randomNone", 0) + slots.get(
        "randomInRange", 0
    ):
        errors.append("install.slots random references disagree with the record count")
    graphs = install.get("graphs", {})
    if graphs.get("segments") != sum(graphs.get("degrees", {}).values()):
        errors.append("install.graphs degrees disagree with the segment count")
    if graphs.get("records") != sum(graphs.get("modules", {}).values()):
        errors.append("install.graphs modules disagree with the record count")
    if graphs.get("startsAtZero", 0) > graphs.get("bindings", 0):
        errors.append("install.graphs starts exceed the binding count")
    if sum(vmdl.get("textureRefs", {}).values()) != vmdl.get("count"):
        errors.append("install.vmdl texture reference histogram disagrees")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("registry", "tables", 0, "moduleTable"), "0x01300179"),
        (("registry", "tables", 1, "moduleCount"), 161),
        (("registry", "namePrefix", "cdev"), "SQEX/Other"),
        (("registry", "rootEffect", "record"), "0x01303e6c"),
        (("modules", 0, "signature"), "03000000000"),
        (("modules", 3, "name"), "AbstractPosition3D"),
        (("modules", 5, "slots", "0x68"), "0x00e39db"),
        (("modules", 5, "slots", "0x67"), "0x00e39db0"),
        (("modules", 217, "table"), "other"),
        (("modules", 10, "slotArrayOffset"), "32"),
        (("modules", 20, "parent"), "NoSuchModule"),
        (("families", 2, "current"), "NoSuchModule"),
        (("families", 3, "formerRevision"), 5),
        (("graphModules", 0, "name"), "RandomImmediateObjectRef"),
        (("graphModules", 1, "slots"), ["0x1"]),
        (("engineConstants", 0, "bytes"), 3),
        (("engineConstants", 1, "name"), "controlColor"),
        (("engineConstants", 2, "defaults"), [1.0]),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0xb936d9"),
        (("staticChecks", 2, "mask"), "ff"),
        (("descriptorFields", 0, "tier"), "GUESSED"),
        (("descriptorFields", 0, "check"), "no such check"),
        (("veffFields", 3, "offset"), "30"),
        (("elements", "unit", 2, "check"), "no such check"),
        (("linkCodes", 0, "top"), "0x4"),
        (("outcomes", 0, "tier"), "INFERRED"),
        (("outcomes", 3, "claim"), " "),
        (("install", "scope"), "owner installation"),
        (("install", "veff", "count"), 1080),
        (("install", "veff", "versions", "0x1d"), 1068),
        (("install", "veff", "relations", "controlsEqualLinks"), 99999),
        (("install", "veff", "totals", "units"), None),
        (("install", "decls", "veff", "entries"), 1),
        (("install", "decls", "veff", "signatureMatch"), 1),
        (("install", "decls", "veffOlder", "names"), {}),
        (("install", "drawResource", "blocks"), 1),
        (("install", "drawResource", "links", "color"), {}),
        (("install", "vmdl", "textureRefs", "1"), 1),
        (("install", "graphs", "segments"), 1),
        (("install", "slots", "records"), 1),
        (("install", "graphs", "modules", "StandardPolynomialFloat32"), 1),
        (("elements", "graphRecord", 0, "check"), "no such check"),
        (("install", "vmdl", "shaderRecords"), 1),
        (("install", "vmdl", "vertexTokens", "0xfffe0300"), 1),
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
        "veff version gate",
        "declaration compare reads six signature bytes",
        "blend mode 1 body",
        "EffectFactory vtable",
        "tick constants",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    short = copy.deepcopy(document)
    short["outcomes"].pop()
    mutations.append(short)
    fewer = copy.deepcopy(document)
    fewer["modules"].pop()
    mutations.append(fewer)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def read_cstring(data: bytes, sections, va: int) -> str | None:
    return image_checks.read_cstring(data, sections, va)


def read_u32(data: bytes, sections, va: int) -> int | None:
    raw = image_checks.read_va(data, sections, va, 4)
    return None if raw is None else struct.unpack("<I", raw)[0]


def image_rows(data: bytes, document: dict) -> tuple[list, list, list, list]:
    """Module, family, graph and engine constant rows read from the image."""
    sections = image_checks.section_map(data)
    prefix = document["registry"]["namePrefix"]
    modules, families = [], []
    for table in document["registry"]["tables"]:
        pre = prefix[table["id"]]
        base = int(table["moduleTable"], 16)
        for index in range(table["moduleCount"]):
            raw = image_checks.read_va(
                data, sections, base + index * DESCRIPTOR_SIZE, DESCRIPTOR_SIZE
            )
            if raw is None:
                raise ValueError("module table is outside the image")
            words = struct.unpack("<58I", raw)
            name = read_cstring(data, sections, words[0x5C // 4]) or ""
            parent_va = words[0x64 // 4]
            parent = read_cstring(data, sections, parent_va) if parent_va else None
            slots = {
                f"0x{offset:02x}": f"0x{words[offset // 4]:08x}"
                for offset in range(0x68, 0xE0, 4)
                if words[offset // 4]
            }
            row = {
                "table": table["id"],
                "index": index,
                "name": name[len(pre) :] if name.startswith(pre) else name,
                "signature": raw[:6].hex(),
                "kind": struct.unpack_from("<H", raw, 6)[0],
                "unitFlags": words[0x18 // 4],
                "workSize": words[0x1C // 4],
                "workAlign": words[0x20 // 4],
                "slotArrayOffset": words[0x2C // 4],
                "staticLinkOffset": words[0x34 // 4],
                "exprLinkOffset": words[0x3C // 4],
                "revision": words[0x60 // 4],
                "slots": slots,
            }
            if parent:
                for key, value in prefix.items():
                    if parent.startswith(value):
                        row["parent"] = (
                            parent[len(value) :]
                            if key == "cdev"
                            else "R:" + parent[len(value) :]
                        )
            modules.append(row)
        base = int(table["familyTable"], 16)
        for index in range(table["familyCount"]):
            raw = image_checks.read_va(
                data, sections, base + index * FAMILY_SIZE, FAMILY_SIZE
            )
            if raw is None:
                raise ValueError("family table is outside the image")
            words = struct.unpack("<6I", raw)
            names = [
                read_cstring(data, sections, words[slot]) if words[slot] else None
                for slot in (0, 1, 3)
            ]
            families.append(
                {
                    "table": table["id"],
                    "index": index,
                    "family": (names[0] or "")[len(pre) :],
                    "current": (names[1] or "")[len(pre) :],
                    "revision": words[2],
                    "former": names[2][len(pre) :] if names[2] else None,
                    "formerRevision": words[4],
                }
            )
    graphs = []
    for table in document["registry"]["graphTables"]:
        base = int(table["va"], 16)
        for index in range(table["count"]):
            raw = image_checks.read_va(
                data, sections, base + index * GRAPH_SIZE, GRAPH_SIZE
            )
            if raw is None:
                raise ValueError("graph table is outside the image")
            words = struct.unpack("<12I", raw)
            if words[0] == 0 and words[1] == 0 and words[5] == 0:
                continue
            graphs.append(
                {
                    "table": table["id"],
                    "index": index,
                    "name": read_cstring(data, sections, words[4]) or "",
                    "idWord": f"0x{words[0]:08x}",
                    "sizeWord": f"0x{words[1]:08x}",
                    "dataWord": f"0x{words[2]:08x}",
                    "dataBytes": words[3],
                    "slots": [f"0x{value:08x}" for value in words[5:12]],
                }
            )
    engine = []
    base = int(document["registry"]["engineTable"], 16)
    for index in range(ENGINE_COUNT):
        raw = image_checks.read_va(
            data, sections, base + index * ENGINE_ROW, ENGINE_ROW
        )
        if raw is None:
            raise ValueError("engine constant table is outside the image")
        words = struct.unpack("<8I", raw)
        engine.append(
            {
                "name": read_cstring(data, sections, words[0]) or "",
                "bytes": words[1],
                "defaults": list(struct.unpack_from("<4f", raw, 0x10)),
            }
        )
    return modules, families, graphs, engine


def table_errors(data: bytes, document: dict) -> list[str]:
    try:
        found = image_rows(data, document)
    except (ValueError, struct.error) as exc:
        return [f"image tables cannot be read: {exc}"]
    errors = []
    for label, rows, want in zip(
        ("modules", "families", "graphModules", "engineConstants"),
        found,
        (
            document["modules"],
            document["families"],
            document["graphModules"],
            document["engineConstants"],
        ),
    ):
        if rows != want:
            changed = [index for index, (a, b) in enumerate(zip(rows, want)) if a != b]
            errors.append(
                f"{label} differ from the image at rows {changed[:5]}"
                f" ({len(rows)} read, {len(want)} recorded)"
            )
    return errors


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    errors = image_checks.verify_image(data, document, require_identity)
    return errors + table_errors(data, document)


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def bank_files(root: Path) -> list[Path]:
    return sorted(
        path
        for path in (root / "client" / "vfx").rglob("*")
        if path.is_file() and path.name != ".DS_Store"
    )


def catalog_index(document: dict) -> tuple[dict, dict, dict]:
    """Module signatures, former name aliases and graph module names."""
    prefix = document["registry"]["namePrefix"]
    modules = {}
    for row in document["modules"]:
        modules[prefix[row["table"]] + row["name"]] = (
            bytes.fromhex(row["signature"]),
            row["revision"],
            row["kind"],
        )
    aliases = {}
    for row in document["families"]:
        if row["former"]:
            pre = prefix[row["table"]]
            aliases[pre + row["former"]] = pre + row["current"]
    graphs = {
        (row["table"], row["index"]): row["name"] for row in document["graphModules"]
    }
    return modules, aliases, graphs


def count_decls(rows: list, catalog: tuple, out: dict) -> None:
    modules, aliases, _ = catalog
    for row in rows:
        name = row["name"] or ""
        out["entries"] += 1
        out["names"][name] = out["names"].get(name, 0) + 1
        target = name if name in modules else aliases.get(name)
        if target is None:
            out["unresolved"] += 1
            continue
        if name != target:
            out["viaFormerName"] += 1
        signature, revision, kind = modules[target]
        out[
            "signatureMatch" if row["signature"] == signature else "signatureMismatch"
        ] += 1
        out["revisionMatch"] += row["tail"] == revision
        out["kindMatch"] += row["kind"] == kind


def blank_decls() -> dict:
    return {
        "entries": 0,
        "names": {},
        "unresolved": 0,
        "viaFormerName": 0,
        "signatureMatch": 0,
        "signatureMismatch": 0,
        "revisionMatch": 0,
        "kindMatch": 0,
    }


def census_veff(data: bytes, info: dict, root: int, out: dict, catalog) -> None:
    veff = body.parse_veff(data, info, root)
    relations = body.veff_relations(data, info, root, veff)
    row = out["veff"]
    base = info["payload"]
    arrays = veff["arrays"]
    row["count"] += 1
    version = f"{veff['version']:#04x}"
    row["versions"][version] = row["versions"].get(version, 0) + 1
    current = veff["version"] == body.VEFF_VERSION
    count_decls(
        veff["decls"], catalog, out["decls"]["veff" if current else "veffOlder"]
    )
    if not current:
        return
    culling = str(veff["cullingType"])
    row["cullingTypes"][culling] = row["cullingTypes"].get(culling, 0) + 1
    rel = row["relations"]
    rel["arraysInside"] += all(item["inside"] for item in arrays.values())
    rel["controlsEqualLinks"] += arrays["controls"]["count"] == arrays["links"]["count"]
    rel["unitsEqualInstances"] += (
        arrays["units"]["count"] == arrays["instances"]["count"]
    )
    for key, value in relations.items():
        rel[key] += value
    for key, name in zip(
        VEFF_COUNT_KEYS, ("controls", "links", "units", "instances", "decls")
    ):
        row["totals"][key] += arrays[name]["count"]
    rel["endlessLifetime"] += veff["lifetimeTicks"] == 0x7FFFFFFF
    rel["degenerateBounds"] += veff["boundsMin"] == veff["boundsMax"]
    rel["sourcePathPrintable"] += veff["sourcePath"] is not None
    # unit tree relations
    unit_at = base + arrays["units"]["pointer"]
    inst_at = base + arrays["instances"]["pointer"]
    unit_count, inst_count = arrays["units"]["count"], arrays["instances"]["count"]
    link_count = arrays["links"]["count"]
    referenced: collections.Counter = collections.Counter()
    lists_ok = schedule_ok = sorted_ok = pad_ok = graph_ok = end_ok = True
    sums = [0, 0, 0]
    for index in range(unit_count):
        at = unit_at + 0x38 * index
        c_ptr, c_n, i_ptr, i_n, s_ptr, s_n, a_ptr, a_n, b_ptr, b_n = struct.unpack_from(
            "<10I", data, at
        )
        sums[0] += c_n
        sums[1] += a_n
        sums[2] += b_n
        lists_ok &= all(
            qix.u16(data, base + c_ptr + 2 * k) < link_count for k in range(c_n)
        )
        for k in range(i_n):
            value = qix.u16(data, base + i_ptr + 2 * k)
            lists_ok &= value < inst_count
            referenced[value] += 1
        previous = None
        for k in range(s_n):
            start, child, pad = struct.unpack_from("<iHH", data, base + s_ptr + 8 * k)
            schedule_ok &= child < i_n
            pad_ok &= pad == 0
            sorted_ok &= previous is None or previous <= start
            previous = start
        graph_ok &= a_ptr + 12 * a_n <= info["payloadSpan"]
        graph_ok &= b_ptr + 12 * b_n <= info["payloadSpan"]
        end_ok &= data[at + 0x34] == 0xFF
    rel["unitListsInRange"] += lists_ok
    rel["scheduleInRange"] += schedule_ok
    rel["scheduleSorted"] += sorted_ok
    rel["schedulePadZero"] += pad_ok
    rel["graphListsInside"] += graph_ok
    rel["unitEndByteFF"] += end_ok
    rel["graphTotalsMatch"] += sums[1] == veff["nameA"] and sums[2] == veff["nameB"]
    rel["linkTotalMatchesUnitLists"] += sums[0] == link_count
    rel["childrenReferencedOnce"] += (
        inst_count > 0
        and referenced[0] == 0
        and all(referenced[k] == 1 for k in range(1, inst_count))
    )
    rel["rootInstanceIsUnitZero"] += inst_count > 0 and qix.u16(data, inst_at) == 0
    census_graphs(data, info, unit_at, unit_count, catalog[2], out["graphs"])
    census_slots(data, info, arrays, out["slots"])
    census_draw_links(data, info, veff, out["drawResource"]["links"])
    control_at = base + arrays["controls"]["pointer"]
    link_at = base + arrays["links"]["pointer"]
    indices = [
        qix.u16(data, link_at + 0x1C * k + 0x14)
        for k in range(arrays["links"]["count"])
    ]
    rel["linkControlPermutation"] += sorted(indices) == list(
        range(arrays["controls"]["count"])
    )
    draw = out["drawResource"]
    for index in range(arrays["controls"]["count"]):
        at = control_at + 0x24 * index
        decl = veff["decls"][qix.u16(data, at + 0x1C)]
        if (decl["name"] or "").endswith("Controls/DrawResource"):
            block = qix.u32(data, at + 0x18)
            if block + 8 <= info["payloadSpan"]:
                kind = data[base + block + 4]
                draw["kinds"][str(kind)] = draw["kinds"].get(str(kind), 0) + 1
                draw["blocks"] += 1
                if kind in (1, 3, 8):
                    record = base + qix.u32(data, base + block)
                    name = qix.cstring(data, record + 8, 16)
                    draw["modelRecords"] += 1
                    draw["modelTag"] += data[record + 0x18 : record + 0x1C] == b"ldmv"
                    draw["modelFlagsClear"] += data[record + 0x1C] & 2 == 0
                    draw["modelInReferences"] += name in veff["modelNames"]


def census_graphs(
    data: bytes, info: dict, unit_at: int, units: int, names: dict, out: dict
) -> None:
    """Graph records of every unit. Polynomial bindings are walked segment by segment."""
    base, span = info["payload"], info["payloadSpan"]
    widths = {"Float32": 1, "Float32X2": 2, "Float32X3": 3, "Float32X4": 4}
    for index in range(units):
        at = unit_at + 0x38 * index
        for table, list_at in (("standard", 0x18), ("random", 0x20)):
            pointer, count = (
                qix.u32(data, at + list_at),
                qix.u32(data, at + list_at + 4),
            )
            for number in range(count):
                module, overflow, pad, bindings_at, bindings, high = struct.unpack_from(
                    "<HBBIHH", data, base + pointer + 12 * number
                )
                out["overflowIdValid"] += overflow <= 1
                out["recordPadZero"] += pad == 0 and high == 0
                name = names.get((table, module), "")
                out["records"] += 1
                out["modules"][name] = out["modules"].get(name, 0) + 1
                shape = name.removeprefix("Standard").removeprefix("Random")
                if not shape.startswith("Polynomial") or "Genesis" in shape:
                    continue
                width = widths[shape.removeprefix("Polynomial")]
                for slot in range(bindings):
                    first_at, starts_at, segments = struct.unpack_from(
                        "<III", data, base + bindings_at + 12 * slot
                    )
                    out["bindings"] += 1
                    out["segments"] += segments
                    cursor = qix.u32(data, base + first_at)
                    contiguous = True
                    for _ in range(segments):
                        degree, components = (
                            data[base + cursor],
                            data[base + cursor + 1],
                        )
                        contiguous &= components == width and degree <= 2
                        out["degrees"][str(degree)] = (
                            out["degrees"].get(str(degree), 0) + 1
                        )
                        cursor += 4 + 4 * components * (degree + 1)
                    out["segmentsContiguous"] += contiguous and cursor <= span
                    starts = [
                        qix.u32(data, base + starts_at + 4 * k) for k in range(segments)
                    ]
                    out["startsAtZero"] += starts[0] == 0
                    out["startsNondecreasing"] += all(
                        starts[k] <= starts[k + 1] for k in range(segments - 1)
                    )


def census_draw_links(data: bytes, info: dict, veff: dict, out: dict) -> None:
    """Static link codes of every DrawResource node, resolved to the module they name."""
    base = info["payload"]
    arrays = veff["arrays"]
    control_at = base + arrays["controls"]["pointer"]
    link_at = base + arrays["links"]["pointer"]
    unit_at = base + arrays["units"]["pointer"]
    names = [
        (
            veff["decls"][qix.u16(data, control_at + 0x24 * index + 0x1C)]["name"] or ""
        ).rsplit("/", 1)[-1]
        for index in range(arrays["controls"]["count"])
    ]
    owner = [
        qix.u16(data, link_at + 0x1C * index + 0x14)
        for index in range(arrays["links"]["count"])
    ]
    for unit in range(arrays["units"]["count"]):
        at = unit_at + 0x38 * unit
        nodes_at, nodes = qix.u32(data, at), qix.u32(data, at + 4)
        listed = [qix.u16(data, base + nodes_at + 2 * k) for k in range(nodes)]
        for link in listed:
            if names[owner[link]] != "DrawResource":
                continue
            out["nodes"] += 1
            codes_at, count = (
                qix.u32(data, link_at + 0x1C * link),
                qix.u32(data, link_at + 0x1C * link + 4),
            )
            codes = [qix.u32(data, base + codes_at + 4 * k) for k in range(count)]
            out["twoLinks"] += count == 2
            local = [
                (code >> 28) == 0
                and (code >> 16) & 0xFFF == unit
                and (code & 0xFFFF) < nodes
                for code in codes
            ]
            out["bothLocal"] += count == 2 and all(local)
            if count == 2 and all(local):
                for label, code in zip(("color", "matrix"), codes):
                    name = names[owner[listed[code & 0xFFFF]]]
                    out[label][name] = out[label].get(name, 0) + 1


def census_slots(data: bytes, info: dict, arrays: dict, out: dict) -> None:
    """Control slot records against the graph lists of the unit that owns the control."""
    base = info["payload"]
    unit_at = base + arrays["units"]["pointer"]
    link_at = base + arrays["links"]["pointer"]
    control_at = base + arrays["controls"]["pointer"]
    for index in range(arrays["units"]["count"]):
        at = unit_at + 0x38 * index
        nodes_at, nodes = qix.u32(data, at), qix.u32(data, at + 4)
        a_at, a_count = qix.u32(data, at + 0x18), qix.u32(data, at + 0x1C)
        b_count = qix.u32(data, at + 0x24)
        bindings = [qix.u16(data, base + a_at + 12 * k + 8) for k in range(a_count)]
        for node in range(nodes):
            link = qix.u16(data, base + nodes_at + 2 * node)
            control = qix.u16(data, link_at + 0x1C * link + 0x14)
            entry = control_at + 0x24 * control
            list_at, slots = qix.u32(data, entry), qix.u32(data, entry + 4)
            for slot in range(slots):
                record = base + list_at + 16 * slot
                graph, binding, random = (
                    data[record + 9],
                    data[record + 10],
                    data[record + 11],
                )
                out["records"] += 1
                out["graphInRange"] += graph < a_count
                out["bindingInRange"] += graph < a_count and binding < bindings[graph]
                out["randomNone"] += random == 0xFF
                out["randomInRange"] += random != 0xFF and random < b_count


def census_leaf(
    data: bytes, info: dict, root: int, label: str, out: dict, catalog
) -> None:
    rows = body.parse_leaf_decls(data, info, root)
    out["decls"][label]["blobs"] += 1
    count_decls(rows, catalog, out["decls"][label])
    out["leafVersion"][str(qix.u16(data, root + 0x1C))] = (
        out["leafVersion"].get(str(qix.u16(data, root + 0x1C)), 0) + 1
    )


def census_vmdl(data: bytes, info: dict, root: int, out: dict, seen: set) -> None:
    row = out["vmdl"]
    base = info["payload"]
    vmdl = body.parse_vmdl(data, info, root)
    row["count"] += 1
    key = str(vmdl["shaderSetCount"])
    row["shaderSets"][key] = row["shaderSets"].get(key, 0) + 1
    tex = str(vmdl["textureRefCount"])
    row["textureRefs"][tex] = row["textureRefs"].get(tex, 0) + 1
    for label, value in (("type", vmdl["type"]), ("variant", vmdl["variant"])):
        text = str(value)
        row[label + "s"][text] = row[label + "s"].get(text, 0) + 1
    first = vmdl["shaderSets"][0]
    row["rootMirrorsFirstSet"] += (
        first["vertex"] == vmdl["vertexShader"]
        and first["pixel"] == vmdl["pixelShader"]
    )
    row["texturesNamed"] += all(
        item["name"] is not None and item["tag"] == b"xetv"
        for item in vmdl["textureRefs"]
    )
    for label in ("modelChunk", "instanceChunk", "animationChunk"):
        pointer, size = vmdl[label]
        if pointer:
            magic = bytes(data[base + pointer + 4 : base + pointer + 8])
            text = magic.decode("latin-1")
            row[label][text] = row[label].get(text, 0) + 1
    for entry in vmdl["shaderSets"]:
        row["shaderRecords"] += 2
        row["setFlagsZero"] += entry["flags"] == 0
        for label, (pointer, size), tokens in (
            ("vertex", entry["vertex"], "vertexTokens"),
            ("pixel", entry["pixel"], "pixelTokens"),
        ):
            blob = bytes(data[base + pointer : base + pointer + size])
            shader = body.parse_shader(blob)
            token = f"{shader['versionToken']:#010x}"
            row[tokens][token] = row[tokens].get(token, 0) + 1
            row["lengthMatches"] += shader["lengthMatches"]
            row["padZero"] += shader["padZero"]
            if blob in seen:
                continue
            seen.add(blob)
            row["uniqueRecords"] += 1
            target = row[label + "Constants"]
            for name, register_set, _, _ in shader["constants"]:
                item = f"{name}:{register_set}"
                target[item] = target.get(item, 0) + 1


def blank_census() -> dict:
    return {
        "scope": SCOPE,
        "veff": {
            "count": 0,
            "versions": {},
            "cullingTypes": {},
            "totals": dict.fromkeys(VEFF_COUNT_KEYS, 0),
            "relations": collections.defaultdict(int),
        },
        "decls": {
            "veff": blank_decls(),
            "veffOlder": blank_decls(),
            "leaf": {**blank_decls(), "blobs": 0},
            "vins": {**blank_decls(), "blobs": 0},
        },
        "graphs": {
            "records": 0,
            "overflowIdValid": 0,
            "recordPadZero": 0,
            "modules": {},
            "bindings": 0,
            "segments": 0,
            "segmentsContiguous": 0,
            "startsAtZero": 0,
            "startsNondecreasing": 0,
            "degrees": {},
        },
        "slots": {
            "records": 0,
            "graphInRange": 0,
            "bindingInRange": 0,
            "randomNone": 0,
            "randomInRange": 0,
        },
        "leafVersion": {},
        "drawResource": {
            "blocks": 0,
            "kinds": {},
            "modelRecords": 0,
            "modelTag": 0,
            "modelFlagsClear": 0,
            "modelInReferences": 0,
            "links": {
                "nodes": 0,
                "twoLinks": 0,
                "bothLocal": 0,
                "color": {},
                "matrix": {},
            },
        },
        "vmdl": {
            "count": 0,
            "shaderSets": {},
            "textureRefs": {},
            "types": {},
            "variants": {},
            "rootMirrorsFirstSet": 0,
            "texturesNamed": 0,
            "modelChunk": {},
            "instanceChunk": {},
            "animationChunk": {},
            "shaderRecords": 0,
            "setFlagsZero": 0,
            "vertexTokens": {},
            "pixelTokens": {},
            "lengthMatches": 0,
            "padZero": 0,
            "uniqueRecords": 0,
            "vertexConstants": {},
            "pixelConstants": {},
        },
    }


def census_install(root: Path, document: dict | None = None) -> dict:
    """Counts and structure of every effect body below client/vfx. No bytes are kept."""
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    catalog = catalog_index(document)
    out = blank_census()
    seen: set = set()
    for path in bank_files(root):
        data = path.read_bytes()
        for blob in qix.walk_bank(data)["blobs"]:
            if blob["tag"] not in ("veff", "leaf", "vins", "vmdl"):
                continue
            info = qix.parse_blob(data, blob)
            record = qix.root_record(data, info)
            if blob["tag"] == "veff":
                census_veff(data, info, record, out, catalog)
            elif blob["tag"] == "vmdl":
                census_vmdl(data, info, record, out, seen)
            else:
                census_leaf(data, info, record, blob["tag"], out, catalog)
    out["veff"]["relations"] = dict(sorted(out["veff"]["relations"].items()))
    for key in ("versions", "cullingTypes"):
        out["veff"][key] = dict(sorted(out["veff"][key].items()))
    for label in ("veff", "veffOlder", "leaf", "vins"):
        out["decls"][label]["names"] = dict(
            sorted(out["decls"][label]["names"].items())
        )
    for key in list(out["vmdl"]):
        if isinstance(out["vmdl"][key], dict):
            out["vmdl"][key] = dict(sorted(out["vmdl"][key].items()))
    out["drawResource"]["kinds"] = dict(sorted(out["drawResource"]["kinds"].items()))
    for key in ("color", "matrix"):
        links = out["drawResource"]["links"]
        links[key] = dict(sorted(links[key].items()))
    for key in ("modules", "degrees"):
        out["graphs"][key] = dict(sorted(out["graphs"][key].items()))
    return out


def flatten(node, prefix: str = "") -> dict:
    if isinstance(node, dict):
        flat = {}
        for key, value in node.items():
            flat.update(flatten(value, f"{prefix}{key}."))
        return flat
    return {prefix.rstrip("."): node}


def verify_install(root: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    try:
        found = flatten(census_install(root, document))
    except (qix.ParseError, OSError, struct.error, ValueError, IndexError) as exc:
        return [f"install cannot be walked: {exc}"]
    wanted = flatten(document["install"])
    errors = []
    for key in sorted(set(found) | set(wanted)):
        if found.get(key) != wanted.get(key):
            errors.append(
                f"install {key} is {found.get(key)}, manifest has {wanted.get(key)}"
            )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install",
        type=Path,
        help="game root whose client/vfx banks are compared with the census",
    )
    parser.add_argument(
        "--print-install",
        type=Path,
        metavar="ROOT",
        help="print the census of ROOT as JSON and exit",
    )
    args = parser.parse_args()
    if args.print_install:
        print(json.dumps(census_install(args.print_install), indent=2, sort_keys=True))
        return 0
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
    if errors:
        print(f"FAIL: {len(errors)} Qix effect runtime error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install census"
    print(f"PASS: Qix effect runtime manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
