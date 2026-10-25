#!/usr/bin/env python3
"""Count TabControl, TabItem, TextBox, ScrollViewer and LogControl uses in an install's Sqwt files."""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import sys
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path

TYPES = ("TabControl", "TabItem", "TextBox", "ScrollViewer", "LogControl")
SIGNATURE = b"SQEX" + bytes(4)
BYTE_ORDER_MARK = chr(0xFEFF)
SUFFIXES = {".form": "forms", ".tpl": "templates", ".style": "styles", ".skin": "skins"}
LOG_FORM = "widget/LogWidget.form"
LOG_TEMPLATE = "widget/LogWidget.tpl"
CONDITIONS = (
    "SkinNormal",
    "SkinActive",
    "SkinDisabled",
    "SkinFocused",
    "SkinHover",
    "SkinReserved",
    "SkinAnchored",
)
TARGET_ATTRIBUTES = ("TargetName", "Storyboard.TargetName")
ELEMENT_NAME_RE = re.compile(r"ElementName=([^,}\s]+)")
MANIFEST = (
    Path(__file__).resolve().parent.parent / "config" / "sqwt_tab_scroll_text.json"
)


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def parse(text: str) -> ET.Element | None:
    """Parse one document, refusing DOCTYPE and ENTITY declarations."""
    if "<!DOCTYPE" in text or "<!ENTITY" in text:
        return None
    try:
        return ET.fromstring(text)
    except ET.ParseError:
        return None


def style_tables(
    roots: dict[str, ET.Element], only: str | None = None
) -> tuple[dict, dict]:
    """Return ({(type, style): {part: (reference type, style)}}, {(type, style, part): conditions}).

    A later file in path order replaces the rows of an earlier one, so the
    all-files tables serve only the definition counts. `only` limits the walk to one style file.
    """
    parts: dict[tuple[str, str], dict[str, tuple[str, str]]] = {}
    frames: dict[tuple[str, str, str], dict[str, str]] = {}
    for path in sorted(roots):
        if not path.endswith(".style") or (only is not None and path != only):
            continue
        for widget in roots[path].iter("Widget"):
            kind = widget.get("Type", "")
            for style in widget.findall("SqwtStyle"):
                key = (kind, style.get("Name", ""))
                found = parts.setdefault(key, {})
                for part in style.findall("Parts"):
                    if part.get("Reference"):
                        found[part.get("Name", "")] = (
                            part.get("Reference", ""),
                            part.get("Style", ""),
                        )
                    else:
                        frames[(*key, part.get("Name", ""))] = {
                            row.get("Condition", ""): row.get("Skin", "")
                            for row in part.findall("Property")
                        }
    return parts, frames


def closure(
    kind: str,
    style: str,
    parts: dict,
    frames: dict,
    seen: set | None = None,
    rows: list | None = None,
    leaves: list | None = None,
) -> tuple[list, list]:
    """Walk the part references of one style depth first."""
    seen = set() if seen is None else seen
    rows = [] if rows is None else rows
    leaves = [] if leaves is None else leaves
    key = (kind, style)
    if key in seen:
        return rows, leaves
    seen.add(key)
    for (frame_kind, frame_style, name), conditions in sorted(frames.items()):
        if (frame_kind, frame_style) == key:
            leaves.append((kind, style, name, conditions))
    for name, (ref_kind, ref_style) in parts.get(key, {}).items():
        rows.append([kind, style, name, ref_kind, ref_style])
        closure(ref_kind, ref_style, parts, frames, seen, rows, leaves)
    return rows, leaves


def skin_kinds(skin_root: ET.Element | None) -> dict[str, str]:
    kinds: dict[str, str] = {}
    if skin_root is not None:
        for section in skin_root:
            for element in section:
                if element.get("Name"):
                    kinds.setdefault(element.get("Name", ""), section.tag)
    return kinds


def census_texts(files: dict[str, str]) -> dict:
    """Census of decoded documents keyed by sqwt-relative path."""
    roots: dict[str, ET.Element] = {}
    unparsed = 0
    counts = Counter()
    for path in sorted(files):
        suffix = Path(path).suffix
        if suffix not in SUFFIXES:
            continue
        counts[SUFFIXES[suffix]] += 1
        root = parse(files[path])
        if root is None:
            unparsed += 1
        else:
            roots[path] = root
    parts, _ = style_tables(roots)
    defined = {kind: {name for (k, name) in parts if k == kind} for kind in TYPES}
    elements = {
        kind: {"elements": 0, "files": 0, "attributes": Counter(), "styles": Counter()}
        for kind in TYPES
    }
    for path, root in roots.items():
        if not path.endswith((".form", ".tpl")):
            continue
        seen_here: set[str] = set()
        for element in root.iter():
            kind = local_name(element.tag)
            if kind not in TYPES:
                continue
            entry = elements[kind]
            entry["elements"] += 1
            seen_here.add(kind)
            for name, value in element.attrib.items():
                entry["attributes"][name] += 1
                if name == "SqwtStyle":
                    entry["styles"][value] += 1
        for kind in seen_here:
            elements[kind]["files"] += 1
    resolution = {}
    for kind in TYPES:
        used = elements[kind]["styles"]
        resolution[kind] = {
            "distinct": len(used),
            "defined": sum(1 for name in used if name in defined[kind]),
        }
    result = {
        "corpus": {
            **{name: counts[name] for name in SUFFIXES.values()},
            "unparsed": unparsed,
        },
        "elements": {
            kind: {
                "elements": entry["elements"],
                "files": entry["files"],
                "attributes": dict(sorted(entry["attributes"].items())),
                "styles": dict(sorted(entry["styles"].items())),
            }
            for kind, entry in elements.items()
        },
        "styleResolution": resolution,
    }
    result["logWidget"] = log_widget(roots)
    return result


def shadowing(roots: dict, style_file: str, frames: dict, leaves: set) -> dict:
    """How many style files define each leaf style of the closure, and where their rows differ."""
    other: dict[str, dict] = {}
    for path in sorted(roots):
        if path.endswith(".style") and path != style_file:
            other[path] = style_tables(roots, path)[1]
    counts = []
    differing = 0
    for kind, style in sorted(leaves):
        rows = {
            name: v for (k, s, name), v in frames.items() if (k, s) == (kind, style)
        }
        defining = 1
        differs = False
        for table in other.values():
            found = {
                name: v for (k, s, name), v in table.items() if (k, s) == (kind, style)
            }
            if found:
                defining += 1
                differs = differs or found != rows
        counts.append(defining)
        differing += differs
    return {
        "leafStyles": len(leaves),
        "fewestFiles": min(counts, default=0),
        "mostFiles": max(counts, default=0),
        "differing": differing,
    }


def log_widget(roots: dict) -> dict:
    form = roots.get(LOG_FORM)
    if form is None:
        return {}
    style_file = form.get("SqwtStyleFile", "")
    parts, frames = style_tables(roots, style_file)
    style_root = roots.get(style_file)
    skin_file = style_root.get("Source", "") if style_root is not None else ""
    skin_names = skin_kinds(roots.get(skin_file))
    used: dict[str, list[str]] = {kind: [] for kind in TYPES}
    counts = Counter()
    attributes = {kind: Counter() for kind in TYPES}
    for path in (LOG_FORM, LOG_TEMPLATE):
        root = roots.get(path)
        if root is None:
            continue
        for element in root.iter():
            kind = local_name(element.tag)
            if kind in TYPES:
                counts[kind] += 1
                attributes[kind].update(element.attrib.keys())
                style = element.get("SqwtStyle")
                if style and style not in used[kind]:
                    used[kind].append(style)
    targets: Counter[str] = Counter()
    for path in (LOG_FORM, LOG_TEMPLATE):
        root = roots.get(path)
        if root is None:
            continue
        for element in root.iter():
            values = [element.get(name, "") for name in TARGET_ATTRIBUTES]
            for value in element.attrib.values():
                values.extend(ELEMENT_NAME_RE.findall(value))
            targets.update(value for value in values if ":" in value)
    part_rows: list[list[str]] = []
    leaf_rows: list[list[str]] = []
    seen: set = set()
    for kind in TYPES:
        for style in used[kind]:
            rows, leaves = closure(kind, style, parts, frames, seen)
            part_rows.extend(rows)
            for owner, name, part, conditions in leaves:
                leaf_rows.append(
                    [owner, name, part] + [conditions.get(c, "") for c in CONDITIONS]
                )
    distinct = sorted(
        {skin for row in leaf_rows for skin in row[3:] if skin and skin != "NULL"}
    )
    return {
        "shadowing": shadowing(
            roots, style_file, frames, {(row[0], row[1]) for row in leaf_rows}
        ),
        "styleFile": style_file,
        "skinFile": skin_file,
        "elements": {kind: counts[kind] for kind in TYPES},
        "attributes": {kind: dict(sorted(attributes[kind].items())) for kind in TYPES},
        "styles": used,
        "targets": dict(sorted(targets.items())),
        "parts": part_rows,
        "frames": leaf_rows,
        "skinNames": len(distinct),
        "skinKinds": dict(
            sorted(Counter(skin_names.get(s, "missing") for s in distinct).items())
        ),
    }


def load_blowfish(path: Path):
    spec = importlib.util.spec_from_file_location("sqex_blowfish", path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_install(root: Path, blowfish_path: Path) -> dict[str, str]:
    """Decode every SQEX form, template, style and skin below the client/sqwt directory."""
    cipher_module = load_blowfish(blowfish_path)
    files: dict[str, str] = {}
    for path in sorted(root.rglob("*")):
        if path.suffix not in SUFFIXES or not path.is_file():
            continue
        data = path.read_bytes()
        if data[:8] != SIGNATURE:
            continue
        body = cipher_module.Blowfish(path.name.encode("utf-8")).decrypt(data[8:])
        files[path.relative_to(root).as_posix()] = body.decode("utf-8").lstrip(
            BYTE_ORDER_MARK
        )
    return files


def compare(found: dict, manifest: dict) -> list[str]:
    expected = manifest.get("census", {})
    errors = []
    for key in ("corpus", "elements", "styleResolution", "logWidget"):
        if found.get(key) != expected.get(key):
            errors.append(f"census {key} differs from the manifest")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--install", type=Path, required=True, help="client/sqwt directory"
    )
    parser.add_argument(
        "--blowfish", type=Path, required=True, help="xivl-tools tools/blowfish.py"
    )
    parser.add_argument("--print", action="store_true", dest="show")
    args = parser.parse_args()
    found = census_texts(decode_install(args.install, args.blowfish))
    if args.show:
        json.dump(found, sys.stdout, indent=2, sort_keys=True)
        print()
    errors = compare(found, json.loads(MANIFEST.read_text(encoding="ascii")))
    for error in errors:
        print(f"- {error}")
    print("FAIL" if errors else "PASS: census matches the manifest")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
