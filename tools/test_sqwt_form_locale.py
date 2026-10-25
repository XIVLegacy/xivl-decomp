#!/usr/bin/env python3
"""Tests for the Sqwt form locale verifier, its reference model and its census."""

from __future__ import annotations

import copy
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import census_sqwt_form_locale as census  # noqa: E402
import verify_sqwt_form_locale as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000
IMPORT_SECTION_VA = 0x02000000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def raises(function, *args) -> bool:
    try:
        function(*args)
    except ValueError:
        return True
    return False


def insertion_cases() -> None:
    check(
        "a form path takes the token before .form",
        verifier.insert_language("\\widget\\Name.form", 1) == "\\widget\\Name.en.form",
    )
    check(
        "a template path takes the token before .tpl",
        verifier.insert_language("\\widget\\Name.tpl", 3) == "\\widget\\Name.fr.tpl",
    )
    check(
        "each language index selects its token",
        [verifier.insert_language("\\a.form", i) for i in range(6)]
        == [f"\\a.{token}.form" for token in ("ja", "en", "de", "fr", "chs", "cht")],
    )
    check(
        "an index above 5 leaves two dots",
        verifier.insert_language("\\a.form", 6) == "\\a..form",
    )
    check(
        "a negative index leaves two dots",
        verifier.insert_language("\\a.form", -1) == "\\a..form",
    )
    check(
        "a path of five characters is unchanged",
        verifier.insert_language("a.tpl", 1) == "a.tpl",
    )
    check(
        "a path of six characters is changed",
        verifier.insert_language("ab.tpl", 1) == "ab.en.tpl",
    )
    check(
        "the suffix match is case sensitive",
        verifier.insert_language("\\a.FORM", 1) == "\\a.FORM",
    )
    check(
        "only a trailing suffix takes the token",
        verifier.insert_language("\\a.form.xml", 1) == "\\a.form.xml"
        and verifier.insert_language("\\a.form.tpl", 1) == "\\a.form.en.tpl",
    )
    check(
        "another extension is unchanged",
        verifier.insert_language("\\a.skin", 1) == "\\a.skin",
    )
    check(
        "a path already holding a token takes a second one",
        verifier.insert_language("\\a.sd.form", 2) == "\\a.sd.de.form",
    )
    check(
        "an open lists the variant then the plain path",
        verifier.open_candidates("\\a.form", 0) == ["\\a.ja.form", "\\a.form"],
    )
    check(
        "an open rewrites a slash before both tests",
        verifier.open_candidates("/widget/a.form", 1)
        == ["\\widget\\a.en.form", "\\widget\\a.form"],
    )
    check(
        "a slash inside a script name reaches every candidate as a backslash",
        verifier.form_candidates("sub/Foo", 0)[:2]
        == ["\\widget\\sub\\Foo.ja.form", "\\widget\\sub\\Foo.form"],
    )


def directory_cases() -> None:
    check(
        "a script name searches widget, debug and the root",
        verifier.attempt_paths("Foo")
        == ["\\widget\\Foo.form", "\\debug\\Foo.form", "\\Foo.form"],
    )
    check(
        "Desktop maps to the common directory for every attempt",
        verifier.attempt_paths("Desktop") == [verifier.DESKTOP_PATH] * 3,
    )
    check(
        "a name that merely starts with Desktop searches the three directories",
        verifier.attempt_paths("DesktopWidget")[0] == "\\widget\\DesktopWidget.form",
    )
    check(
        "the candidate list has six paths in directory order",
        verifier.form_candidates("Foo", 1)
        == [
            "\\widget\\Foo.en.form",
            "\\widget\\Foo.form",
            "\\debug\\Foo.en.form",
            "\\debug\\Foo.form",
            "\\Foo.en.form",
            "\\Foo.form",
        ],
    )

    def files(*names: str):
        return lambda path: path in names

    check(
        "a language-only widget opens with its own language",
        verifier.resolve_form(
            "ChainBonusEffectWidget",
            2,
            files(
                "\\widget\\ChainBonusEffectWidget.de.form",
                "\\widget\\ChainBonusEffectWidget.en.form",
            ),
        )
        == "\\widget\\ChainBonusEffectWidget.de.form",
    )
    check(
        "a language-only widget fails for a language without a file",
        verifier.resolve_form(
            "ChainBonusEffectWidget",
            4,
            files("\\widget\\ChainBonusEffectWidget.de.form"),
        )
        is None,
    )
    check(
        "a plain-only widget opens in every language",
        all(
            verifier.resolve_form("Foo", index, files("\\widget\\Foo.form"))
            == "\\widget\\Foo.form"
            for index in range(8)
        ),
    )
    check(
        "the language variant wins over the plain file",
        verifier.resolve_form(
            "Foo", 3, files("\\widget\\Foo.form", "\\widget\\Foo.fr.form")
        )
        == "\\widget\\Foo.fr.form",
    )
    check(
        "the widget directory wins over the debug directory",
        verifier.resolve_form(
            "Foo", 1, files("\\debug\\Foo.en.form", "\\widget\\Foo.form")
        )
        == "\\widget\\Foo.form",
    )
    check(
        "the debug directory is tried after widget",
        verifier.resolve_form("Foo", 1, files("\\debug\\Foo.form"))
        == "\\debug\\Foo.form",
    )
    check(
        "a form below widget_c is never reached by name",
        verifier.resolve_form("Foo", 1, files("\\widget_c\\Foo.form")) is None,
    )
    check(
        "a form below the root is reached last",
        verifier.resolve_form("Foo", 1, files("\\Foo.form")) == "\\Foo.form",
    )
    check(
        "an sd file is never a candidate",
        verifier.resolve_form("Foo", 1, files("\\widget\\Foo.sd.form")) is None
        and all(".sd." not in item for item in verifier.form_candidates("Foo", 1)),
    )


def language_cases() -> None:
    check(
        "a nonzero config value minus one is the language",
        verifier.default_language(3, b"\0\0\0\0\2", 1, 0x409) == 2,
    )
    check(
        "a config value of one selects Japanese",
        verifier.default_language(1, None, None, 0x409) == 0,
    )
    check(
        "config.lng byte 4 is used when below 6",
        verifier.default_language(0, b"\0\0\0\0\3\0\0\0", 2, 0x409) == 3,
    )
    check(
        "config.lng byte 4 of 6 is ignored",
        verifier.default_language(0, b"\0\0\0\0\6", 2, 0x409) == 2,
    )
    check(
        "a config.lng shorter than 5 bytes is ignored",
        verifier.default_language(0, b"\0\0\0\2", None, 0x40C) == 3,
    )
    check(
        "the registry value is returned unchecked",
        verifier.default_language(0, None, 77, 0x409) == 77,
    )
    check(
        "the LANGID map is the last source",
        verifier.default_language(0, None, None, 0x407) == 2,
    )
    members = [item for values in verifier.EXPECTED_LANGID.values() for item in values]
    check("no LANGID maps to two languages", len(members) == len(set(members)))
    check(
        "Japanese and unmapped LANGIDs give index 0",
        verifier.langid_index(0x411) == 0 and verifier.langid_index(0x1234) == 0,
    )
    check(
        "the Chinese LANGIDs give 4 and 5",
        verifier.langid_index(0x804) == 4 and verifier.langid_index(0x404) == 5,
    )


def census_cases() -> None:
    paths = [
        "widget/A.form",
        "widget/A.sd.form",
        "widget/B.en.form",
        "widget/B.de.form",
        "widget/B.en.tpl",
        "widget/C.form",
        "widget/C.fr.form",
        "widget/C.fr.sd.form",
        "widget/D.sd.tpl",
        "common/X.en.le.spk",
        "common/X.en.win32.gtex",
        "common/X.win32.gtex",
        "common/default.skin",
    ]
    result = census.census(paths)
    form = result["roots"]["widget"]["form"]
    check("a plain stem is bareOnly", form["bareOnly"] == 1)
    check(
        "a language stem without a plain file is languageOnly",
        form["languageOnly"] == 1,
    )
    check("a stem with both is counted as both", form["both"] == 1)
    check("stems are the sum of the categories", form["stems"] == 3)
    check("sd variants are counted per stem", form["withSd"] == 2)
    check(
        "a stem with only an sd variant is sdOnly",
        result["roots"]["widget"]["tpl"]["sdOnly"] == 1,
    )
    check(
        "shapes count files",
        result["shapes"]["form"]
        == {"": 2, "sd": 1, "en": 1, "de": 1, "fr": 1, "fr.sd": 1},
    )
    check(
        "localized stems list the languages in index order",
        [
            (item["stem"], item["languages"], item["bare"])
            for item in result["localizedStems"]
            if item["extension"] == "form"
        ]
        == [("B", ["en", "de"], False), ("C", ["fr"], True)],
    )
    check(
        "localized other kinds count the language-token files",
        result["localizedOther"] == {"le.spk": 1, "win32.gtex": 1},
    )
    check("other files are counted", result["otherFiles"] == 4)
    check(
        "an unknown token in a form name is rejected",
        raises(census.census, ["widget/A.ko.form"]),
    )
    check(
        "a name with three tokens is rejected",
        raises(census.census, ["widget/A.en.sd.x.form"]),
    )
    check(
        "the same stem in two directories is two stems",
        census.census(["a/Z.form", "b/Z.form"])["roots"]["a"]["form"]["stems"] == 1
        and census.census(["a/Z.form", "b/Z.form"])["roots"]["b"]["form"]["stems"] == 1,
    )
    check(
        "the same stem in two extensions is two stems",
        len(census.census(["a/Z.form", "a/Z.tpl"])["roots"]["a"]) == 2,
    )
    check(
        "a language token that is not first is not a language variant",
        census.census(["a/Z.sd.en.form"])["roots"]["a"]["form"].get("sdOnly") == 1,
    )


def image_bytes(rows: list[tuple[int, bytes]], imports: list[tuple[int, str]]) -> bytes:
    """Build a PE32 image with one section per row and an import directory."""
    extra: list[tuple[int, bytes]] = []
    names = bytearray()
    descriptors = bytearray()
    layout: list[tuple[int, int, int, int]] = []
    name_base = IMPORT_SECTION_VA + 0x1000
    for slot, ident in imports:
        dll, symbol = ident.split("!")
        dll_va = name_base + len(names)
        names += dll.encode() + b"\0"
        hint_va = name_base + len(names)
        names += b"\0\0" + symbol.encode() + b"\0"
        layout.append((slot, dll_va, hint_va, len(layout)))
        extra.append((slot, struct.pack("<II", hint_va - IMAGE_BASE, 0)))
    for slot, dll_va, _, _ in layout:
        descriptors += struct.pack(
            "<IIIII", 0, 0, 0, dll_va - IMAGE_BASE, slot - IMAGE_BASE
        )
    descriptors += bytes(20)
    sections = rows + extra
    sections.append((IMPORT_SECTION_VA, bytes(descriptors).ljust(0x1000, b"\0")))
    sections.append((name_base, bytes(names)))
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(sections) + 0xFFF) & ~0xFFF
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(sections))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    struct.pack_into(
        "<II", header, 0x80 + 24 + 96 + 8, IMPORT_SECTION_VA - IMAGE_BASE, 0x1000
    )
    body = bytearray()
    for index, (va, data) in enumerate(sections):
        raw_offset = first_raw + len(body)
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII", header, base + 8, len(data), va - IMAGE_BASE, len(data), raw_offset
        )
        body += data
    return bytes(header) + bytes(body)


def row_bytes(item: dict) -> bytes:
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "wstring":
        return item["text"].encode("utf-16le") + b"\0\0"
    if item["kind"] == "dwords":
        return b"".join(
            int(value, 16).to_bytes(4, "little") for value in item["values"]
        )
    if item["kind"] == "rangeSha256":
        return bytes(index % 251 for index in range(item["length"]))
    return bytes.fromhex(item["pattern"])


def image_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    for item in document["staticChecks"]:
        if item["kind"] == "rangeSha256":
            item["sha256"] = hashlib.sha256(row_bytes(item)).hexdigest()
    plain = [i for i in document["staticChecks"] if i["kind"] != "import"]
    import_rows = [
        (int(i["va"], 16), i) for i in document["staticChecks"] if i["kind"] == "import"
    ]
    imports = [(slot, item["import"]) for slot, item in import_rows]
    rows = [(int(i["va"], 16), row_bytes(i)) for i in plain]
    image = image_bytes(rows, imports)
    check(
        "an image built from the rows passes",
        verifier.verify_image(image, document, require_identity=False) == [],
    )
    check(
        "identity is enforced",
        len(verifier.verify_image(image, document)) >= 2,
    )
    check(
        "the import table parser maps every slot",
        verifier.import_slots(image) == {slot: ident for slot, ident in imports},
    )
    first_raw = (
        0x80 + 24 + 0xE0 + 40 * (len(rows) + len(imports) + 2) + 0xFFF
    ) & ~0xFFF
    missed = []
    offset = first_raw
    for index, (_, data) in enumerate(rows):
        flipped = bytearray(image)
        flipped[offset] ^= 0x01
        if not verifier.verify_image(bytes(flipped), document, require_identity=False):
            missed.append(plain[index]["name"])
        offset += len(data)
    check(f"every row reports a flipped byte (missed {missed})", not missed)
    moved = image_bytes([(va + 1, data) for va, data in rows], imports)
    check(
        "rows at shifted addresses are reported",
        len(verifier.verify_image(moved, document, require_identity=False)) >= 1,
    )
    for name, replacement in (
        ("import GetUserDefaultLangID", "KERNEL32.dll!GetUserDefaultLCID"),
        ("import ReadFile", "KERNEL32.dll!WriteFile"),
        ("import RegOpenKeyExW", "KERNEL32.dll!RegOpenKeyExW"),
    ):
        changed = [
            (slot, replacement if item["name"] == name else item["import"])
            for slot, item in import_rows
        ]
        check(
            f"a wrong import name for {name} is reported",
            verifier.verify_image(
                image_bytes(rows, changed), document, require_identity=False
            )
            != [],
        )
    check(
        "an image that is not a PE is reported",
        verifier.verify_image(b"not a pe", document, require_identity=False) != [],
    )


def manifest_cases() -> None:
    check("the manifest verifies", verifier.verify() == [])
    check("every manifest mutation is rejected", verifier.mutation_test() == [])
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    check(
        "the page holds every contract line",
        verifier.doc_errors(document) == [],
    )
    check(
        "a page without a contract line is rejected",
        verifier.doc_errors(document, "STATIC INFERRED UNKNOWN CALIBRATION") != [],
    )
    page = verifier.DOC.read_text(encoding="ascii")
    for label, old, new in (
        (
            "a changed census count",
            "| `widget` | form | 249 | 248 |",
            "| `widget` | form | 249 | 247 |",
        ),
        ("a changed token", "| 4 | `chs` |", "| 4 | `cn` |"),
        ("a changed LANGID", "`0x804 0x1004` | 4", "`0x804 0x1005` | 4"),
        (
            "a changed localized stem",
            "| form, tpl | ja, en, de, fr | no |",
            "| form, tpl | ja, en, de, fr | yes |",
        ),
    ):
        check(
            f"a page with {label} is rejected",
            old in page and verifier.doc_errors(document, page.replace(old, new)) != [],
        )
    check(
        "a wrapped and ticked contract line is accepted",
        verifier.flatten("`a`\n  `b` c") == "a b c"
        and verifier.doc_errors(document, page) == [],
    )
    broken = copy.deepcopy(document)
    broken["staticChecks"][0]["kind"] = "bytes"
    check("an unknown row kind is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["examples"][0]["candidates"].reverse()
    check(
        "an example that disagrees with the model is rejected",
        verifier.verify(broken) != [],
    )
    broken = copy.deepcopy(document)
    broken["examples"][0]["candidates"][0], broken["examples"][0]["candidates"][1] = (
        broken["examples"][0]["candidates"][1],
        broken["examples"][0]["candidates"][0],
    )
    check("a plain-first order is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    for item in broken["staticChecks"]:
        if item["name"] == "token selector jump table":
            item["values"][4], item["values"][5] = item["values"][5], item["values"][4]
    check("a swapped token arm is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["language"]["tokens"][4] = "cn"
    check("a changed token is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["census"]["roots"]["widget"]["form"]["both"] = 1
    check("a census that misses the sum is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["census"]["localizedStems"][-1]["bare"] = True
    check(
        "a localized stem that disagrees with the roots is rejected",
        verifier.verify(broken) != [],
    )
    broken = copy.deepcopy(document)
    for item in broken["staticChecks"]:
        if item["name"] == "LANGID map function":
            item["sha256"] = "z" * 64
    check("a malformed range hash is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    for item in broken["staticChecks"]:
        if item["name"] == "import ReadFile":
            item["import"] = "ReadFile"
    check("a malformed import is rejected", verifier.verify(broken) != [])
    widget_form = document["census"]["roots"]["widget"]["form"]
    check(
        "the census holds one language-only widget form",
        widget_form["languageOnly"] == 1
        and [
            item["stem"]
            for item in document["census"]["localizedStems"]
            if not item["bare"] and item["extension"] == "form"
        ]
        == ["ChainBonusEffectWidget"],
    )
    check(
        "the manifest census holds every installed form",
        sum(document["census"]["shapes"]["form"].values()) == 620
        and sum(document["census"]["shapes"]["tpl"].values()) == 482,
    )


def main() -> int:
    insertion_cases()
    directory_cases()
    language_cases()
    census_cases()
    image_cases()
    manifest_cases()
    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} sqwt form locale tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
