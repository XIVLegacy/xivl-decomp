#!/usr/bin/env python3
"""Validate the Sqwt XPath data source contract, its reference model and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks
import verify_sqwt_form_locale as locale_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "sqwt_xpath_binding.json"
DOC = ROOT / "docs" / "resource" / "sqwt-xpath-data-sources.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
HEX8_RE = locale_checks.HEX8_RE
LABELS = {"STATIC", "INFERRED", "UNKNOWN", "CALIBRATION", "MEASURED"}
FORM_NAMES = ("MainMenuWidget", "MiniMapWidget", "AchievementPopupWidget")
FORM_FILES = {
    "MainMenuWidget": ["widget/MainMenuWidget.form", "widget/MainMenuWidget.tpl"],
    "MiniMapWidget": [
        "widget/MiniMapWidget.form",
        "widget/MiniMapWidget.tpl",
        "common/MapMarker.tpl",
    ],
    "AchievementPopupWidget": [
        "widget/AchievementPopupWidget.form",
        "widget/AchievementPopupWidget.tpl",
        "widget/AchievementPanel.tpl",
    ],
}
EXPECTED_RESOURCE_PROPERTIES = [
    (0, "Document", "0x3c"),
    (1, "x:Key", "0x4c"),
    (2, "XPath", "0x54"),
    (3, "XPathNode", "0x44"),
    (4, "Source", "0x5c"),
    (5, "IsReadOnly", "0x64"),
]
EXPECTED_BINDING_FIELDS = {
    "source": "0x3c",
    "targetObject": "0x44",
    "converter": "0x4c",
    "xpath": "0x54",
    "sourceProperty": "0x5c",
    "targetProperty": "0x60",
    "elementName": "0x64",
    "converterParameter": "0x68",
    "backConverterParameter": "0x6c",
    "path": "0x70",
    "sourceKey": "0x74",
    "name": "0x78",
}
EXPECTED_ITEMS_PROPERTIES = [
    (0, "SortKeyAttr"),
    (1, "ItemsPanel"),
    (2, "ItemContainerGenerator"),
    (3, "ItemCount"),
    (4, "MaxVisibleItems"),
    (5, "MinVisibleItems"),
    (6, "ItemsSource"),
    (7, "ItemTemplate"),
    (8, "ItemTemplateSelector"),
    (9, "SqwtFocusedIndex"),
    (10, "SourceFirstIndex"),
    (11, "SourceCount"),
]
EXPECTED_DECLARATION_ATTRIBUTES = {
    "SqwtXmlDataMaker": ["x:Key", "XPath", "Name", "Source"],
    "RaptureSsdDataReader": ["x:Key", "XPath", "Name", "SheetName", "XmlFormat", "Row"],
}
EXPECTED_XPATH_FORMS = [
    "/N",
    "/N/N",
    "/N/N[@N=N]",
    "/N[position() ge N and position() le N]",
    "@N",
    "text()",
]
EXPECTED_BINDING_SHAPES = ["/N", "/N/N", "/N/N[@N=N]", "@N"]
REQUIRED_CHECKS = {
    "binding parser XPath key",
    "binding XPath field store",
    "binding ElementName field",
    "binding Path field",
    "binding Source field",
    "resolve XPath nonempty",
    "resolve default source is the target",
    "resolve slash test",
    "resolve XPathNode lookup",
    "resolve DataContext lookup",
    "RTTI SqwtXmlDataMaker bases",
    "RTTI RaptureSsdDataReader bases",
    "RTTI ResourceDictionary name",
    "evaluate no source property",
    "evaluate no-property path",
    "evaluate property read",
    "evaluate XPath and node lookup",
    "evaluate range reset",
    "evaluate slash and at tests",
    "evaluate attribute branch",
    "evaluate text branch",
    "evaluate source XPath prefix compare",
    "evaluate mismatch skips the write",
    "evaluate bracket search",
    "evaluate attribute predicate parse",
    "evaluate attribute predicate lookup",
    "evaluate range token position",
    "evaluate range token ge",
    "evaluate range token le",
    "evaluate range token and",
    "evaluate range initial bounds",
    "evaluate range op ge",
    "evaluate range op le",
    "evaluate range number first",
    "evaluate range number after position",
    "evaluate range position after number",
    "evaluate range position first",
    "evaluate range clamp",
    "evaluate range apply",
    "evaluate target write",
    "handle registry lookup",
    "XmlElement GetAttribute",
    "XmlElement children list",
    "XmlElemenBase SetRange",
    "ResourceDictionary XPathNode getter",
    "ResourceDictionary XPath changed",
    "ResourceDictionary XPath selects the node",
    "SelectNode absolute test",
    "SelectNode segment cut",
    "SqwtXmlDataMaker row root create flag",
    "RaptureSsdDataReader row root create flag",
    "RaptureSsdDataReader row gate",
    "RaptureSsdDataReader node reselect",
    "ItemsControl ItemsSource index",
    "ItemsControl source store",
    "ItemsControl setter table",
    "ItemsControl getter table",
    "ItemsControl row visibility test",
    "ItemsControl row template attribute",
    "ItemsControl row template fallback",
    "ItemsControl selector gate",
    "ItemsControl row DataContext set",
    "ItemsControl row DataContext call",
    "DataContext property index",
    "ResourceDictionary getter table",
    "ResourceDictionary setter table",
    "ResourceDictionary XPath index",
    "ResourceDictionary XPathNode index",
    "ResourceDictionary changed table",
    "string XPath binding key",
    "string XPathNode binding",
    "string DataContext binding",
    "string XPath evaluator",
    "string text()",
    "string predicate delimiters",
    "string position()",
    "string ge",
    "string le",
    "string and",
    "string visibility row",
    "string False",
    "string template row",
    "string ssdindex",
    "string ItemsSource",
    "string ItemTemplate",
    "string ItemTemplateSelector",
    "string ResourceDictionary XPath",
    "string ResourceDictionary XPathNode",
    "string SheetName",
    "string XmlFormat",
    "string Row",
    "RTTI DOM_Node name",
    "RTTI XmlElement name",
    "RTTI XmlElemenBase name",
}
STATIC_KINDS = {"signature", "dwords", "string"}


class Node:
    """A parsed XML element as the evaluator sees it: name, attributes, children, text."""

    def __init__(
        self,
        name: str,
        attrs: dict[str, str] | None = None,
        children: list[Node] | None = None,
        text: str = "",
    ):
        self.name = name
        self.attrs = dict(attrs or {})
        self.children = list(children or [])
        self.text = text
        self.range = (0, 0xFFFF)


def select_node(xpath: str, node: Node, create: bool = False) -> Node | None:
    """Reference model of the node select: absolute path of child names."""
    if not xpath.startswith("/"):
        return None
    cut = xpath.find("/", 1)
    cut = len(xpath) if cut == -1 else cut
    if cut <= 1:
        return node
    name, rest = xpath[1:cut], xpath[cut:]
    child = next((item for item in node.children if item.name == name), None)
    if child is None:
        if not create:
            return None
        child = Node(name)
        node.children.append(child)
    return select_node(rest, child, create) if rest else child


def tokens(text: str) -> list[str]:
    """Split the text after the bracket on bracket and space delimiters."""
    return [item for item in re.split(r"[\[\] ]", text) if item]


def atol(text: str) -> int:
    match = re.match(r"\s*[+-]?\d+", text)
    return int(match.group()) if match else 0


def range_bounds(predicate: str, count: int) -> tuple[int, int]:
    """Reference model of the position range state machine."""
    last = count - 1
    lo, hi = 0, last
    number, pending, op, seen = 0, False, 0, False
    for token in tokens(predicate):
        if token == "position()":
            if not pending:
                seen = True
                continue
            if op == 1:
                lo = min(lo, number)
                hi = min(hi, number)
            elif op == 2:
                lo = max(lo, number)
                hi = max(hi, number)
            op, pending = 0, False
        elif token == "ge":
            op = 1
        elif token == "le":
            op = 2
        elif token == "and":
            pending, seen, op = False, False, 0
        elif token[0] in "0123456789":
            if op == 0:
                number, pending = atol(token), True
            elif seen:
                number = atol(token)
                if op == 1:
                    lo = max(lo, number)
                    hi = max(hi, number)
                elif op == 2:
                    lo = min(lo, number)
                    hi = min(hi, number)
                seen, op = False, 0
            else:
                pending, seen, op = False, False, 0
    lo, hi = max(lo, 0), max(hi, 0)
    return min(lo, last), min(hi, last)


def evaluate(xpath: str, source_xpath: str, node: Node | None):
    """Reference model of one binding evaluation over a resolved node.

    Returns ("none",), ("handle", node) or ("value", text).
    """
    if node is None or not xpath:
        return ("handle", node)
    node.range = (0, 0xFFFF)
    if xpath[0] == "/":
        if not xpath.startswith(source_xpath):
            return ("none",)
        bracket = xpath.find("[")
        if bracket == -1:
            return ("handle", node)
        predicate = xpath[bracket + 1 :]
        if predicate.startswith("@"):
            body = predicate[1:]
            equals = body.find("=")
            close = body.find("]", equals + 1) if equals != -1 else -1
            if equals == -1 or close == -1:
                return ("handle", node)
            name, value = body[:equals], body[equals + 1 : close]
            for child in node.children:
                if child.attrs.get(name) == value:
                    return ("handle", child)
            return ("handle", node)
        node.range = range_bounds(predicate, len(node.children))
        return ("handle", node)
    if xpath[0] == "@":
        return ("value", node.attrs.get(xpath[1:], ""))
    if xpath == "text()":
        return ("value", node.text)
    return ("handle", node)


def visible_rows(children: list[Node]) -> list[Node]:
    """Rows an items control builds: a visibility attribute of False hides a row."""
    return [row for row in children if row.attrs.get("visibility", "") != "False"]


def row_template(row: Node, item_template: str, selector: str) -> str:
    """Template name an items control applies to one row."""
    if selector:
        return row.attrs.get("template", "") or item_template
    return item_template


def row_spec(text: str) -> list[int]:
    """Encode a Row value: -1 joins a range, -2 ends an open range."""
    spec: list[int] = []
    for token in [item for item in re.split(r"[ ,]", text) if item]:
        first = atol(token)
        dash = token.find("-")
        second = atol(token[dash + 1 :]) if dash != -1 else first
        if dash == -1 or first == second:
            spec.append(first)
        elif second == 0:
            spec += [first, -2]
        else:
            low, high = min(first, second), max(first, second)
            spec += [low, -1, high]
    return spec


def expand_rows(spec: list[int], row_count: int) -> list[int]:
    """Rows a spec reads from a sheet of row_count rows, ranges inclusive."""
    rows: list[int] = []
    index = 0
    while index < len(spec):
        value = spec[index]
        if index + 1 < len(spec) and spec[index + 1] == -2:
            rows += range(value, row_count)
            index += 2
        elif index + 2 < len(spec) and spec[index + 1] == -1:
            rows += range(value, spec[index + 2] + 1)
            index += 3
        else:
            rows.append(value)
            index += 1
    return [row for row in rows if 0 <= row < row_count]


def shape(text: str) -> str:
    return re.sub(r"[A-Za-z0-9_]+", "N", text)


def split_top(body: str) -> list[str]:
    """Split a binding body on commas outside nested braces."""
    parts, depth, current = [], 0, ""
    for char in body:
        depth += char == "{"
        depth -= char == "}"
        if char == "," and depth == 0:
            parts.append(current.strip())
            current = ""
        else:
            current += char
    if current.strip():
        parts.append(current.strip())
    return parts


def parse_binding(value: str) -> dict[str, str] | None:
    value = value.strip()
    if not (value.startswith("{Binding") and value.endswith("}")):
        return None
    pairs: dict[str, str] = {}
    for part in split_top(value[len("{Binding") : -1]):
        key, _, rest = part.partition("=")
        pairs[key.strip()] = rest.strip()
    return pairs


ATTRIBUTE_RE = re.compile(r'([A-Za-z_][A-Za-z0-9_:.]*)="([^"]*)"')
TAG_RE = re.compile(r"<([A-Za-z_][A-Za-z0-9_.]*)\b([^>]*)>")
DECLARATION_RE = re.compile(r"<(SqwtXmlDataMaker|RaptureSsdDataReader)\b([^>]*?)/?>")


def markup_census(texts: dict[str, str]) -> dict:
    """Count the data source declarations and XPath shapes of decoded markup."""
    declared: dict[str, dict[str, int]] = {}
    declaration_shapes: dict[str, int] = {}
    binding_shapes: dict[str, int] = {}
    bindings = {"total": 0, "withSource": 0, "oneWay": 0}
    items_sources: dict[str, int] = {}
    for text in texts.values():
        for match in DECLARATION_RE.finditer(text):
            attrs = dict(ATTRIBUTE_RE.findall(match.group(2)))
            key = match.group(1) + ":" + ",".join(sorted(attrs))
            declared.setdefault(match.group(1), {})
            declared[match.group(1)][key.split(":", 1)[1]] = (
                declared[match.group(1)].get(key.split(":", 1)[1], 0) + 1
            )
            if "XPath" in attrs:
                token = shape(attrs["XPath"])
                declaration_shapes[token] = declaration_shapes.get(token, 0) + 1
        for tag_match in TAG_RE.finditer(text):
            tag = tag_match.group(1)
            for name, value in ATTRIBUTE_RE.findall(tag_match.group(2)):
                pairs = parse_binding(value)
                if pairs is None or "XPath" not in pairs:
                    continue
                bindings["total"] += 1
                bindings["withSource"] += "Source" in pairs
                bindings["oneWay"] += pairs.get("Mode") == "OneWay"
                token = shape(pairs["XPath"])
                binding_shapes[token] = binding_shapes.get(token, 0) + 1
                if name == "ItemsSource":
                    items_sources[tag] = items_sources.get(tag, 0) + 1
    return {
        "files": len(texts),
        "declarationAttributeSets": {
            tag: dict(sorted(counts.items()))
            for tag, counts in sorted(declared.items())
        },
        "declarationXPathShapes": dict(sorted(declaration_shapes.items())),
        "bindingXPathShapes": dict(sorted(binding_shapes.items())),
        "bindings": bindings,
        "itemsSourceBindings": dict(sorted(items_sources.items())),
    }


def attribute_names_in_templates(root: ET.Element) -> dict[str, list[str]]:
    """Row attribute names read by the @ bindings of each keyed template."""
    result: dict[str, list[str]] = {}
    for element in root.iter():
        tag = element.tag.split("}")[-1]
        if tag not in ("DataTemplate", "ControlTemplate"):
            continue
        keys = [v for k, v in element.attrib.items() if k.endswith("Key")]
        if not keys:
            continue
        names: set[str] = set()
        for inner in element.iter():
            for value in inner.attrib.values():
                pairs = parse_binding(value)
                if pairs and pairs.get("XPath", "").startswith("@"):
                    names.add(pairs["XPath"][1:])
        if names:
            result[keys[0]] = sorted(names)
    return result


def form_facts(texts: dict[str, str], name: str) -> dict:
    """Data sources, XPath bindings and template row attributes of one form."""
    form = ET.fromstring(texts[FORM_FILES[name][0]])
    sources = []
    bindings = []
    for element in form.iter():
        tag = element.tag.split("}")[-1]
        if tag in EXPECTED_DECLARATION_ATTRIBUTES:
            row = {"tag": tag}
            for key, value in element.attrib.items():
                row["x:Key" if key.endswith("Key") else key] = value
            sources.append(row)
        for key, value in element.attrib.items():
            pairs = parse_binding(value)
            if pairs and "XPath" in pairs:
                source = pairs.get("Source", "")
                bindings.append(
                    {
                        "element": tag,
                        "name": element.attrib.get("Name", ""),
                        "property": key,
                        "source": source[len("{StaticResource ") : -1]
                        if source.startswith("{StaticResource ")
                        else source,
                        "xpath": pairs["XPath"],
                        "itemTemplate": element.attrib.get("ItemTemplate", ""),
                        "selector": "ItemTemplateSelector" in element.attrib,
                    }
                )
    rows: dict[str, list[str]] = {}
    for path in FORM_FILES[name][1:]:
        rows.update(attribute_names_in_templates(ET.fromstring(texts[path])))
    return {"sources": sources, "bindings": bindings, "templateRowAttributes": rows}


def decode_markup(root: Path, xivl: Path, only: list[str] | None = None):
    """Decode SQEX files below client/sqwt with xivl and return their texts."""
    base = root / "client" / "sqwt"
    paths = (
        [base / item for item in only]
        if only
        else sorted(p for p in base.rglob("*") if p.is_file())
    )
    texts: dict[str, str] = {}
    with tempfile.TemporaryDirectory() as raw:
        scratch = Path(raw)
        for number, path in enumerate(paths):
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
    return texts


def flatten(text: str) -> str:
    return locale_checks.flatten(text)


def page_rows(document: dict) -> list[str]:
    """Table rows of the page that restate manifest values."""
    rows = [
        f"| {index} | {name} | {offset} |"
        for index, name, offset in EXPECTED_RESOURCE_PROPERTIES
    ]
    for name, offset in document.get("binding", {}).get("fields", {}).items():
        rows.append(f"| B+{offset} | {name} |")
    rows += [f"| {index} | {name} |" for index, name in EXPECTED_ITEMS_PROPERTIES]
    for row in document.get("xpathForms", []):
        rows.append(f"| {row['shape']} | {row['evaluation']} | {row['label']} |")
    census = document.get("census", {})
    for token, count in census.get("bindingXPathShapes", {}).items():
        rows.append(f"| {token} | {count} |")
    for name in FORM_NAMES:
        for source in document.get("forms", {}).get(name, {}).get("sources", []):
            rows.append(f"| {source['x:Key']} | {source['XPath']} |")
    return rows


def doc_errors(document: dict, text: str | None = None) -> list[str]:
    if text is None:
        if not DOC.exists():
            return ["page is missing"]
        text = DOC.read_text(encoding="ascii")
    errors = []
    flat = flatten(text)
    for line in document.get("contract", []) + document.get("rule", []):
        if flatten(line) not in flat:
            errors.append(f"page lacks line {line[:48]!r}")
    for row in page_rows(document):
        if row not in flat:
            errors.append(f"page lacks table row {row[:60]!r}")
    for label in ("STATIC", "INFERRED", "UNKNOWN", "CALIBRATION", "MEASURED"):
        if label not in text:
            errors.append(f"page lacks the {label} label")
    return errors


def example_errors(document: dict) -> list[str]:
    """Run the reference model over the manifest examples."""
    errors = []
    for case in document.get("rangeExamples", []):
        got = list(range_bounds(case["predicate"], case["count"]))
        if got != case["bounds"]:
            errors.append(f"range example {case['predicate']!r} gives {got}")
    for case in document.get("rowExamples", []):
        got = row_spec(case["text"])
        if got != case["spec"]:
            errors.append(f"row example {case['text']!r} encodes {got}")
        rows = expand_rows(case["spec"], case["sheetRows"])
        if rows != case["rows"]:
            errors.append(f"row example {case['text']!r} expands to {rows}")
    for case in document.get("selectExamples", []):
        root = Node("", children=[Node("root", children=[Node("marker")])])
        found = select_node(case["xpath"], root)
        want = case["found"]
        if (found.name if found is not None else None) != want:
            errors.append(f"select example {case['xpath']!r} finds {found}")
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
    rd = document.get("resourceDictionary", {})
    got = [(r["index"], r["name"], r["offset"]) for r in rd.get("properties", [])]
    if got != EXPECTED_RESOURCE_PROPERTIES:
        errors.append("resource dictionary properties changed")
    for key, value in {
        "getterVa": "0x00982c80",
        "setterVa": "0x00983c50",
        "changedVa": "0x009835e0",
        "xpathChangedVa": "0x00982da0",
        "selectNodeVa": "0x00982a30",
        "derived": ["SqwtXmlDataMaker", "RaptureSsdDataReader"],
    }.items():
        if rd.get(key) != value:
            errors.append(f"resource dictionary {key} changed")
    declarations = document.get("declarations", {})
    for tag, names in EXPECTED_DECLARATION_ATTRIBUTES.items():
        if declarations.get(tag, {}).get("attributes") != names:
            errors.append(f"declaration attributes of {tag} changed")
    reader = declarations.get("RaptureSsdDataReader", {})
    if reader.get("properties") != ["SheetName", "XmlFormat", "Row"]:
        errors.append("reader properties changed")
    if reader.get("rowSpecMarkers") != {"range": -1, "openEnd": -2}:
        errors.append("reader row markers changed")
    if reader.get("rowDelimiters") != " ,":
        errors.append("reader row delimiters changed")
    if reader.get("rowAttribute") != "ssdindex":
        errors.append("reader row attribute changed")
    binding = document.get("binding", {})
    if binding.get("fields") != EXPECTED_BINDING_FIELDS:
        errors.append("binding fields changed")
    for key, value in {
        "parserVa": "0x00912340",
        "resolveVa": "0x00911f50",
        "evaluateVa": "0x009993c0",
        "slashProperty": "XPathNode",
        "otherProperty": "DataContext",
    }.items():
        if binding.get(key) != value:
            errors.append(f"binding {key} changed")
    forms = [row["shape"] for row in document.get("xpathForms", [])]
    if forms != EXPECTED_XPATH_FORMS:
        errors.append("xpath forms changed")
    items = document.get("itemsControl", {})
    got = [(r["index"], r["name"]) for r in items.get("properties", [])]
    if got != EXPECTED_ITEMS_PROPERTIES:
        errors.append("items control properties changed")
    for key, value in {
        "sourceOffset": "0x354",
        "payloadOffset": "0x46c",
        "itemTemplateOffset": "0x344",
        "selectorOffset": "0x34c",
        "hiddenValue": "False",
        "hiddenAttribute": "visibility",
        "templateAttribute": "template",
    }.items():
        if items.get(key) != value:
            errors.append(f"items control {key} changed")
    census = document.get("census", {})
    if sorted(census.get("bindingXPathShapes", {})) != sorted(EXPECTED_BINDING_SHAPES):
        errors.append("census binding shapes changed")
    if census.get("bindings", {}).get("oneWay") != census.get("bindings", {}).get(
        "total"
    ):
        errors.append("census modes changed")
    if set(document.get("forms", {})) != set(FORM_NAMES):
        errors.append("forms changed")
    errors.extend(example_errors(document))
    errors.extend(static_check_errors(document))
    for item in document.get("open", []):
        if item.get("label") not in LABELS:
            errors.append("open item label invalid")
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
    for item in checks:
        kind = item.get("kind") if isinstance(item, dict) else None
        if kind not in STATIC_KINDS:
            errors.append("staticChecks entry has an unknown kind")
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
        elif kind == "dwords" and not item.get("values"):
            errors.append(f"staticChecks {item['name']!r} has no values")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("source", "binarySha256"), "0" * 64),
        (("resourceDictionary", "properties", 2, "offset"), "0x58"),
        (("resourceDictionary", "properties", 3, "name"), "XPathValue"),
        (("resourceDictionary", "selectNodeVa"), "0x00982a40"),
        (("declarations", "SqwtXmlDataMaker", "attributes"), ["x:Key", "XPath"]),
        (("declarations", "RaptureSsdDataReader", "properties"), ["SheetName", "Row"]),
        (("declarations", "RaptureSsdDataReader", "rowDelimiters"), ","),
        (("declarations", "RaptureSsdDataReader", "rowSpecMarkers", "openEnd"), -3),
        (("declarations", "RaptureSsdDataReader", "rowAttribute"), "ssd_index"),
        (("binding", "fields", "xpath"), "0x58"),
        (("binding", "slashProperty"), "DataContext"),
        (("binding", "otherProperty"), "XPathNode"),
        (("binding", "evaluateVa"), "0x009993c1"),
        (("itemsControl", "properties", 6, "name"), "ItemSource"),
        (("itemsControl", "hiddenValue"), "false"),
        (("itemsControl", "templateAttribute"), "Template"),
        (("itemsControl", "payloadOffset"), "0x470"),
        (("census", "bindingXPathShapes", "@N"), 641),
        (("census", "bindings", "oneWay"), 1),
        (("rangeExamples", 0, "bounds"), [0, 0]),
        (("rangeExamples", 2, "bounds"), [1, 1]),
        (("rowExamples", 0, "spec"), [0, 33]),
        (("rowExamples", 1, "rows"), [5, 6, 7]),
        (("selectExamples", 0, "found"), "root"),
        (("contract", 2), "A slash XPath writes the node handle."),
        (("rule", 0), "Evaluate every binding on every frame."),
        (("open", 0, "label"), "STATIC-ish"),
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
    leaked["source"]["install"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    results = [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]
    if verify(document, "") == []:
        results.append("an empty page was accepted")
    return results


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
        problem = image_checks.run_static_check(data, sections, check)
        if problem:
            errors.append(problem)
    return errors


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def verify_markup(root: Path, xivl: Path, document: dict | None = None) -> list[str]:
    """Recompute the census and the form facts from an install."""
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    texts = decode_markup(root, xivl)
    errors = []
    got = markup_census(texts)
    for key, value in got.items():
        if document["census"].get(key) != value:
            errors.append(f"markup census {key} is {value}")
    for name in FORM_NAMES:
        if form_facts(texts, name) != {
            key: document["forms"][name][key]
            for key in ("sources", "bindings", "templateRowAttributes")
        }:
            errors.append(f"form facts of {name} differ")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install", type=Path, help="game root whose client/sqwt files are decoded"
    )
    parser.add_argument(
        "--xivl", type=Path, help="xivl executable that decodes client/sqwt"
    )
    args = parser.parse_args()
    if bool(args.install) != bool(args.xivl):
        parser.error("--install and --xivl go together")
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_markup(args.install, args.xivl)
    if errors:
        print(f"FAIL: {len(errors)} sqwt xpath binding error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus markup census"
    print(f"PASS: sqwt xpath binding manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
