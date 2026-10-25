#!/usr/bin/env python3
"""Validate the Sqwt form locale resolution contract, its reference model and optional exe bytes."""

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
MANIFEST = ROOT / "config" / "sqwt_form_locale.json"
DOC = ROOT / "docs" / "resource" / "sqwt-form-locale-resolution.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
HEX8_RE = re.compile(r"^0x[0-9a-f]{8}$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
LABELS = {"STATIC", "INFERRED", "UNKNOWN", "CALIBRATION"}
LANGUAGE_TOKENS = ("ja", "en", "de", "fr", "chs", "cht")
FORM_SUFFIX = ".form"
TEMPLATE_SUFFIX = ".tpl"
DESKTOP_NAME = "Desktop.form"
DESKTOP_PATH = "\\common\\Desktop.form"
DIRECTORY_PREFIXES = ("\\widget\\", "\\debug\\", "\\")
MINIMUM_LENGTH = 6
EXPECTED_LANGID = {
    1: [
        0x409,
        0x809,
        0xC09,
        0x1009,
        0x1409,
        0x1809,
        0x1C09,
        0x2009,
        0x2409,
        0x2809,
        0x2C09,
        0x3009,
        0x3409,
    ],
    2: [0x407, 0x807, 0xC07, 0x1007, 0x1407],
    3: [0x40C, 0x80C, 0xC0C, 0x100C, 0x140C, 0x180C],
    4: [0x804, 0x1004],
    5: [0x404, 0xC04],
}
EXPECTED_REQUEST = {
    "setFilename": {
        "callbackVa": "0x006eb640",
        "formVa": "0x0075b730",
        "storeVa": "0x00533ef0",
        "field": "0x110",
    },
    "loadForm": {
        "callbackVa": "0x006e3760",
        "formVa": "0x0075b750",
        "implVa": "0x00537d50",
        "suffix": ".form",
        "requestVa": "0x005377c0",
        "idField": "0x98",
        "counterField": "0x94",
    },
    "builder": {
        "va": "0x00533b90",
        "modes": [
            {"mode": 0, "prefix": "\\widget\\"},
            {"mode": 1, "prefix": "\\debug\\"},
            {"mode": 2, "prefix": "\\"},
        ],
    },
    "loop": {"attempts": 3, "order": [0, 1, 2], "stopOnOpen": True, "failureId": -1},
}
EXPECTED_OPENER = {
    "va": "0x0096ba70",
    "insert": {
        "va": "0x0096a3b0",
        "minimumLength": MINIMUM_LENGTH,
        "extensions": [TEMPLATE_SUFFIX, FORM_SUFFIX],
    },
    "exists": {"va": "0x0090fbc0", "fileSystemOnly": True},
}
EXPECTED_LANGUAGE = {
    "tokenGlobalVa": "0x01356b38",
    "tokens": list(LANGUAGE_TOKENS),
    "outOfRange": "",
    "defaultFunctionVa": "0x004055d0",
    "langIdFunctionVa": "0x00404f70",
    "field": {
        "objectOffset": "0x20",
        "getterVa": "0x004b68a0",
        "setterVa": "0x004b6890",
        "setterCallerVa": "0x00402d63",
    },
}
EXPECTED_ROOTS = {
    "configGroup": 2,
    "configKey": 8,
    "bootValues": [4, 5, 6],
    "bootRoot": "\\client\\sqwt\\boot",
    "gameRoot": "\\client\\sqwt",
}
REQUIRED_CHECKS = {
    "setFilename callback forwards",
    "setFilename stores the string at +0x110",
    "loadForm callback forwards",
    "loadForm appends .form and requests when the id is negative",
    "request loop builder and opener",
    "request loop advance",
    "request success counter",
    "request failure result",
    "builder Desktop.form test",
    "builder stem cut",
    "builder append .form",
    "builder mode switch",
    "builder mode 0 prefix",
    "builder mode 1 prefix",
    "builder mode 2 prefix",
    "opener separator normalization",
    "opener slash rewrite",
    "opener separator call",
    "opener variant then plain",
    "opener loaded gate",
    "opener stores the chosen path",
    "insert length guard",
    "insert tpl end test",
    "insert form end test",
    "insert call",
    "exists reads attributes",
    "exists joins the root",
    "root separator test",
    "root joins install and sqwt",
    "root mode config",
    "token global setter literal",
    "token global setter append",
    "token global getter",
    "token startup call",
    "token selector dispatch",
    "token selector arms",
    "token selector jump table",
    "language field getter",
    "language field setter",
    "language field setter caller",
    "default reads config.lng",
    "default registry call",
    "LANGID map function",
    "texture consumer token call",
    "spk consumer token call",
    "import GetFileAttributesW",
    "import GetUserDefaultLangID",
    "import ReadFile",
    "import CreateFileW",
    "import RegOpenKeyExW",
    "registry key name selection",
}
STRING_ROWS = {
    "string .form loadForm": ".form",
    "string Desktop.form": DESKTOP_NAME,
    "string common Desktop.form": DESKTOP_PATH,
    "string dot": ".",
    "string .form builder": ".form",
    "string widget prefix": "\\widget\\",
    "string debug prefix": "\\debug\\",
    "string root prefix": "\\",
    "string .tpl insert": ".tpl",
    "string .form insert": ".form",
    "string boot root": "\\client\\sqwt\\boot",
    "string sqwt root": "\\client\\sqwt",
    "string token dot": ".",
    "string token empty": "",
    "string config.lng": "\\config.lng",
    "string gtex": ".gtex",
    "string win32": ".win32",
    "string gtex fallback": ".gtex",
}
WSTRING_ROWS = {
    "wstring registry parent": "SOFTWARE\\SquareEnix\\",
    "wstring registry value": "Language",
    "wstring registry latest": "FINAL FANTASY XIV LATEST",
    "wstring registry plain": "FINAL FANTASY XIV",
}
IMPORT_ROWS = {
    "import GetFileAttributesW": "KERNEL32.dll!GetFileAttributesW",
    "import GetUserDefaultLangID": "KERNEL32.dll!GetUserDefaultLangID",
    "import ReadFile": "KERNEL32.dll!ReadFile",
    "import CreateFileW": "KERNEL32.dll!CreateFileW",
    "import RegOpenKeyExW": "ADVAPI32.dll!RegOpenKeyExW",
}
SELECTOR_TABLE = [
    "0x00d34cde",
    "0x00d34ce4",
    "0x00d34cea",
    "0x00d34cf0",
    "0x00d34cf6",
    "0x00d34cfc",
]
STATIC_KINDS = {
    "signature",
    "dwords",
    "string",
    "wstring",
    "import",
    "rangeSha256",
}


def language_token(index: int) -> str:
    """The token string that 0x00d34cd0 selects for a language dword."""
    return LANGUAGE_TOKENS[index] if 0 <= index < len(LANGUAGE_TOKENS) else ""


def insert_language(path: str, index: int) -> str:
    """Model of 0x0096a3b0: insert the token before a trailing .tpl or .form."""
    if len(path) < MINIMUM_LENGTH:
        return path
    for suffix in (TEMPLATE_SUFFIX, FORM_SUFFIX):
        if path.endswith(suffix):
            cut = len(path) - len(suffix)
            return path[:cut] + "." + language_token(index) + path[cut:]
    return path


def open_candidates(path: str, index: int) -> list[str]:
    """Model of 0x0096ba70: the paths tested for one request path, in order."""
    path = path.replace("/", "\\")
    return [insert_language(path, index), path]


def attempt_paths(filename: str) -> list[str]:
    """Model of 0x00533b90 over modes 0 to 2 for the name _loadForm builds."""
    name = filename + FORM_SUFFIX
    if name == DESKTOP_NAME:
        return [DESKTOP_PATH] * len(DIRECTORY_PREFIXES)
    stem = name[: name.rindex(".")]
    return [prefix + stem + FORM_SUFFIX for prefix in DIRECTORY_PREFIXES]


def form_candidates(filename: str, index: int) -> list[str]:
    """Every path tested for a Lua form name, in order."""
    return [
        candidate
        for path in attempt_paths(filename)
        for candidate in open_candidates(path, index)
    ]


def resolve_form(filename: str, index: int, exists) -> str | None:
    for candidate in form_candidates(filename, index):
        if exists(candidate):
            return candidate
    return None


def langid_index(langid: int) -> int:
    """Model of 0x00404f70, the language index of a Windows LANGID."""
    for index, members in EXPECTED_LANGID.items():
        if langid in members:
            return index
    return 0


def default_language(
    config_value: int,
    config_lng: bytes | None,
    registry_value: int | None,
    langid: int,
) -> int:
    """Model of the setter caller 0x00402d42 and 0x004055d0."""
    if config_value:
        return config_value - 1
    if config_lng is not None and len(config_lng) >= 5 and config_lng[4] < 6:
        return config_lng[4]
    if registry_value is not None:
        return registry_value
    return langid_index(langid)


def parse_hex(text: str) -> int:
    return int(text, 16)


def census_errors(census: dict) -> list[str]:
    errors: list[str] = []
    stem_total = {"form": 0, "tpl": 0}
    for root, exts in census.get("roots", {}).items():
        for ext, row in exts.items():
            parts = sum(
                row.get(key, 0)
                for key in ("bareOnly", "languageOnly", "both", "sdOnly")
            )
            if row.get("stems") != parts:
                errors.append(f"census {root} {ext} categories do not sum to stems")
            if row.get("withSd", 0) > row.get("stems", 0):
                errors.append(f"census {root} {ext} withSd exceeds stems")
            stem_total[ext] += row.get("stems", 0)
    localized = census.get("localizedStems", [])
    for ext in ("form", "tpl"):
        rows = [item for item in localized if item["extension"] == ext]
        language_only = sum(
            exts.get(ext, {}).get("languageOnly", 0)
            for exts in census.get("roots", {}).values()
        )
        both = sum(
            exts.get(ext, {}).get("both", 0)
            for exts in census.get("roots", {}).values()
        )
        if sum(1 for item in rows if not item["bare"]) != language_only:
            errors.append(f"census {ext} language-only stems differ from the roots")
        if sum(1 for item in rows if item["bare"]) != both:
            errors.append(f"census {ext} bare and language stems differ from the roots")
        for item in rows:
            if not set(item["languages"]) <= set(LANGUAGE_TOKENS):
                errors.append(f"census {item['stem']} has an unknown language")
    for ext, counts in census.get("shapes", {}).items():
        for shape in counts:
            if any(
                token not in LANGUAGE_TOKENS + ("sd",) for token in shape.split(".")
            ):
                if shape:
                    errors.append(f"census {ext} shape {shape!r} is unknown")
    if census.get("localizedOther") != {"le.spk": 140, "win32.gtex": 140}:
        errors.append("census localized other kinds changed")
    return errors


def flatten(text: str) -> str:
    """Drop code ticks and collapse whitespace so a page may wrap and format a line."""
    return " ".join(text.replace("`", "").split())


def page_rows(document: dict) -> list[str]:
    """Table rows of the page that restate manifest values."""
    rows = [
        f"| {index} | {token} |"
        for index, token in enumerate(document.get("language", {}).get("tokens", []))
    ]
    rows.append("| above 5 | empty |")
    langid = document.get("language", {}).get("langIdMap", {})
    for index, members in langid.items():
        if index != "default":
            names = " ".join(f"0x{int(item, 16):X}" for item in members)
            rows.append(f"| {names} | {index} |")
    census = document.get("census", {})
    for root, kinds in census.get("roots", {}).items():
        for ext, row in kinds.items():
            cells = (
                root,
                ext,
                *(
                    row.get(key, 0)
                    for key in (
                        "stems",
                        "bareOnly",
                        "languageOnly",
                        "both",
                        "sdOnly",
                        "withSd",
                    )
                ),
            )
            rows.append("| " + " | ".join(str(cell) for cell in cells) + " |")
    grouped: dict[tuple[str, str], dict] = {}
    for item in census.get("localizedStems", []):
        entry = grouped.setdefault(
            (item["directory"], item["stem"]), {**item, "extension": []}
        )
        entry["extension"].append(item["extension"])
    for (directory, stem), item in grouped.items():
        rows.append(
            f"| {directory} | {stem} | {', '.join(item['extension'])} | "
            f"{', '.join(item['languages'])} | {'yes' if item['bare'] else 'no'} |"
        )
    return rows


def doc_errors(document: dict, text: str | None = None) -> list[str]:
    if text is None:
        if not DOC.exists():
            return ["page is missing"]
        text = DOC.read_text(encoding="ascii")
    errors: list[str] = []
    flat = flatten(text)
    for line in document.get("contract", []):
        if flatten(line) not in flat:
            errors.append(f"page lacks contract line {line[:40]!r}")
    for row in page_rows(document):
        if row not in flat:
            errors.append(f"page lacks table row {row[:50]!r}")
    for check in ("STATIC", "INFERRED", "UNKNOWN", "CALIBRATION"):
        if check not in text:
            errors.append(f"page lacks the {check} label")
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
    request = document.get("request", {})
    for key, expected in EXPECTED_REQUEST.items():
        found = request.get(key, {})
        for field, value in expected.items():
            if found.get(field) != value:
                errors.append(f"request {key} {field} changed")
    desktop = request.get("builder", {}).get("desktop", {})
    if desktop.get("name") != DESKTOP_NAME or desktop.get("path") != DESKTOP_PATH:
        errors.append("request builder desktop case changed")
    if request.get("builder", {}).get("stem", {}).get("label") != "INFERRED":
        errors.append("request builder stem label changed")
    opener = document.get("opener", {})
    for key, expected in EXPECTED_OPENER.items():
        found = opener.get(key)
        if isinstance(expected, dict):
            for field, value in expected.items():
                if (found or {}).get(field) != value:
                    errors.append(f"opener {key} {field} changed")
        elif found != expected:
            errors.append(f"opener {key} changed")
    if len(opener.get("steps", [])) != 7:
        errors.append("opener steps changed")
    if [item.get("va") for item in opener.get("callers", [])] != [
        "0x005377c0",
        "0x006a52d0",
        "0x004ffcf0",
    ]:
        errors.append("opener callers changed")
    language = document.get("language", {})
    for key, expected in EXPECTED_LANGUAGE.items():
        found = language.get(key)
        if isinstance(expected, dict):
            for field, value in expected.items():
                if (found or {}).get(field) != value:
                    errors.append(f"language {key} {field} changed")
        elif found != expected:
            errors.append(f"language {key} changed")
    if language.get("setter", {}).get("callerVa") != "0x004b3314":
        errors.append("language setter caller changed")
    if language.get("tokenSelector", {}).get("jumpTableVa") != "0x00d34d08":
        errors.append("language token selector changed")
    derivation = language.get("derivation", [])
    if len(derivation) != 4 or any(
        phrase not in line
        for phrase, line in zip(
            ("group 0 key 0x19", "byte 4", "Language", "GetUserDefaultLangID"),
            derivation,
        )
    ):
        errors.append("language derivation changed")
    found_map = language.get("langIdMap", {})
    if (
        found_map.get("default") != 0
        or {
            int(key): [parse_hex(item) for item in value]
            for key, value in found_map.items()
            if key != "default"
        }
        != EXPECTED_LANGID
    ):
        errors.append("language LANGID map changed")
    roots = document.get("roots", {})
    for key, value in EXPECTED_ROOTS.items():
        if roots.get(key) != value:
            errors.append(f"roots {key} changed")
    consumers = document.get("consumers", {})
    if [item.get("va") for item in consumers.get("sameToken", [])] != [
        "0x0096a3b0",
        "0x008e2f40",
        "0x009a1db0",
    ]:
        errors.append("token consumers changed")
    if [item.get("va") for item in consumers.get("noToken", [])] != ["0x00983be0"]:
        errors.append("no-token consumers changed")
    examples = document.get("examples", [])
    if len(examples) != 3:
        errors.append("examples changed")
    for item in examples:
        if form_candidates(item["filename"], item["language"]) != item["candidates"]:
            errors.append(f"example {item['filename']} {item['language']} differs")
    for item in document.get("pathExamples", []):
        if open_candidates(item["path"], item["language"]) != item["candidates"]:
            errors.append(f"path example {item['path']} differs")
    if len(document.get("pathExamples", [])) != 4:
        errors.append("path examples changed")
    errors.extend(census_errors(document.get("census", {})))
    contract = document.get("contract", [])
    if (
        len(contract) != 9
        or "Desktop.form" not in contract[1]
        or "\\widget\\ + N, \\debug\\ + N and \\ + N" not in contract[2]
        or "then test P itself" not in contract[3]
        or "replace every / in P with \\" not in contract[3]
        or "two dots" not in contract[4]
        or "request id -1" not in contract[6]
        or "read as written" not in contract[7]
        or ".sd" not in contract[8]
    ):
        errors.append("contract changed")
    questions = document.get("open", [])
    if len(questions) != 8 or not all(
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
        elif kind == "import" and not re.fullmatch(
            r"[A-Za-z0-9_]+\.dll![A-Za-z0-9_]+", str(item.get("import", ""))
        ):
            errors.append(f"staticChecks {item['name']!r} import invalid")
    for name, text in STRING_ROWS.items():
        if by_name.get(name, {}).get("text") != text:
            errors.append(f"string row {name!r} changed")
    for name, text in WSTRING_ROWS.items():
        if by_name.get(name, {}).get("text") != text:
            errors.append(f"wide string row {name!r} changed")
    for name, ident in IMPORT_ROWS.items():
        if by_name.get(name, {}).get("import") != ident:
            errors.append(f"import row {name!r} changed")
    for token in LANGUAGE_TOKENS:
        if by_name.get(f"string token {token}", {}).get("text") != token:
            errors.append(f"token string {token!r} changed")
    if by_name.get("token selector jump table", {}).get("values") != SELECTOR_TABLE:
        errors.append("token selector jump table changed")
    arms = by_name.get("token selector arms", {})
    if arms.get("va") != "0x00d34cde" or len(arms.get("mask", "")) != 84:
        errors.append("token selector arms changed")
    if by_name.get("LANGID map function", {}).get("length") != 0x110:
        errors.append("LANGID map function length changed")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("request", "setFilename", "field"), "0x114"),
        (("request", "loadForm", "idField"), "0x9c"),
        (("request", "loadForm", "suffix"), ".tpl"),
        (("request", "builder", "modes", 0, "prefix"), "\\widget_c\\"),
        (("request", "builder", "modes", 1, "prefix"), "\\system\\"),
        (("request", "builder", "desktop", "path"), "\\widget\\Desktop.form"),
        (("request", "builder", "stem", "label"), "STATIC"),
        (("request", "loop", "order"), [2, 1, 0]),
        (("request", "loop", "failureId"), 0),
        (("opener", "insert", "extensions"), [".form", ".tpl"]),
        (("opener", "insert", "minimumLength"), 5),
        (("opener", "exists", "fileSystemOnly"), False),
        (("opener", "callers", 1, "va"), "0x006a52d1"),
        (("language", "tokens"), ["ja", "en", "fr", "de", "chs", "cht"]),
        (("language", "outOfRange"), "ja"),
        (("language", "tokenGlobalVa"), "0x01356b3c"),
        (("language", "field", "setterCallerVa"), "0x00402d64"),
        (("language", "langIdMap", "default"), 1),
        (("language", "langIdMap", "5"), ["0x404"]),
        (
            ("language", "derivation", 1),
            "Otherwise byte 0 of config.lng is the language.",
        ),
        (("roots", "bootValues"), [4, 5]),
        (("roots", "configKey"), 9),
        (("roots", "gameRoot"), "\\client\\sqwt\\boot"),
        (("consumers", "noToken", 0, "va"), "0x00983be1"),
        (("examples", 0, "candidates", 0), "\\widget\\ChainBonusEffectWidget.ja.form"),
        (("examples", 2, "candidates", 1), "\\widget\\Desktop.form"),
        (("pathExamples", 2, "candidates", 0), "a.en.tpl"),
        (("census", "roots", "widget", "form", "languageOnly"), 0),
        (("census", "roots", "widget", "form", "bareOnly"), 249),
        (("census", "localizedOther", "le.spk"), 141),
        (("contract", 3), "Test P with the token of L inserted before .form."),
        (("contract", 7), "A Source reference inside a form takes the token."),
        (("open", 7, "label"), "UNKNOWN"),
        (("source", "binarySha256"), "0" * 64),
    ]:
        changed = copy.deepcopy(document)
        cursor = changed
        for key in path[:-1]:
            cursor = cursor[key]
        cursor[path[-1]] = value
        mutations.append(changed)
    census = copy.deepcopy(document)
    census["census"]["localizedStems"].pop(0)
    mutations.append(census)
    shape = copy.deepcopy(document)
    shape["census"]["shapes"]["form"]["ko"] = 1
    mutations.append(shape)
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


def section_rows(data: bytes) -> list[tuple[int, int, int, int]]:
    return image_checks.section_map(data)


def import_slots(data: bytes) -> dict[int, str]:
    """Map every import address table slot VA to DLL!name."""
    sections = section_rows(data)
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    directory = pe_offset + 24 + 96 + 8
    rva, size = struct.unpack_from("<II", data, directory)
    slots: dict[int, str] = {}
    if not rva or not size:
        return slots
    base = image_checks.IMAGE_BASE
    cursor = base + rva
    while True:
        row = image_checks.read_va(data, sections, cursor, 20)
        if row is None:
            break
        lookup, _, _, name_rva, address_rva = struct.unpack("<IIIII", row)
        if not name_rva:
            break
        name = image_checks.read_cstring(data, sections, base + name_rva)
        table = base + (lookup or address_rva)
        for index in range(4096):
            raw = image_checks.read_va(data, sections, table + 4 * index, 4)
            value = struct.unpack("<I", raw)[0] if raw else 0
            if not value:
                break
            if value & 0x80000000:
                label = f"#{value & 0xFFFF}"
            else:
                label = image_checks.read_cstring(data, sections, base + value + 2)
            slots[base + address_rva + 4 * index] = f"{name}!{label}"
        cursor += 20
    return slots


def run_check(
    data: bytes, sections: list, slots: dict[int, str], check: dict
) -> str | None:
    kind = check["kind"]
    va = parse_hex(check["va"])
    if kind == "wstring":
        wanted = check["text"].encode("ascii").decode("ascii").encode("utf-16le")
        wanted += b"\0\0"
        if image_checks.read_va(data, sections, va, len(wanted)) != wanted:
            return f"wide string at {check['va']} is not {check['text']!r}"
        return None
    if kind == "import":
        if slots.get(va) != check["import"]:
            return f"import slot {check['va']} is {slots.get(va)!r}, expected {check['import']!r}"
        return None
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
        sections = section_rows(data)
        slots = import_slots(data)
    except (ValueError, struct.error) as exc:
        return errors + [f"image is not a readable PE32: {exc}"]
    for check in document["staticChecks"]:
        problem = run_check(data, sections, slots, check)
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
        print(f"FAIL: {len(errors)} sqwt form locale error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: sqwt form locale manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
