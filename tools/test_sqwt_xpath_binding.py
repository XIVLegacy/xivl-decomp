#!/usr/bin/env python3
"""Tests for the Sqwt XPath data source verifier, its reference model and its census."""

from __future__ import annotations

import copy
import json
import os
import stat
import struct
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_sqwt_xpath_binding as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def tree() -> verifier.Node:
    """Document root over a Main list of three rows and a Root/Marker chain."""
    rows = [
        verifier.Node("Main", {"MainName": "@100", "template": "T1"}, text="one"),
        verifier.Node("Main", {"MainName": "@200", "visibility": "False"}),
        verifier.Node("Main", {"MainName": "@300", "visibility": "Visible"}),
    ]
    main = verifier.Node("Main", children=rows)
    marker = verifier.Node("marker")
    return verifier.Node("", children=[main, verifier.Node("root", children=[marker])])


def select_cases() -> None:
    root = tree()
    found = verifier.select_node("/Main", root)
    check("a one segment path finds the child", found is root.children[0])
    found = verifier.select_node("/root/marker", root)
    check(
        "a two segment path finds the grandchild",
        found is root.children[1].children[0],
    )
    check("a slash alone returns the node", verifier.select_node("/", root) is root)
    check("an unknown name finds nothing", verifier.select_node("/Nope", root) is None)
    check(
        "a path without a leading slash finds nothing",
        verifier.select_node("Main", root) is None,
    )
    check(
        "a missing grandchild finds nothing",
        verifier.select_node("/root/other", root) is None,
    )
    created = verifier.select_node("/a/b", root, create=True)
    check(
        "the create flag appends the chain",
        created is not None
        and created.name == "b"
        and [c.name for c in root.children] == ["Main", "root", "a"],
    )
    check(
        "a second create reuses the chain",
        verifier.select_node("/a/b", root, create=True) is created
        and len(root.children) == 3,
    )


def evaluate_cases() -> None:
    root = tree()
    main = root.children[0]
    check(
        "a matching slash path gives the node handle",
        verifier.evaluate("/Main", "/Main", main) == ("handle", main),
    )
    check(
        "a two segment path matches its own source",
        verifier.evaluate("/root/marker", "/root/marker", main) == ("handle", main),
    )
    check(
        "a slash path outside the source writes nothing",
        verifier.evaluate("/Main", "/System", main) == ("none",),
    )
    check(
        "a source path longer than the binding path writes nothing",
        verifier.evaluate("/root", "/root/marker", main) == ("none",),
    )
    check(
        "a source path inside the binding path but not at its start writes nothing",
        verifier.evaluate("/x/Main", "/Main", main) == ("none",),
    )
    check(
        "an empty source path matches any slash path",
        verifier.evaluate("/Main", "", main) == ("handle", main),
    )
    row = main.children[0]
    check(
        "an at-sign path reads the attribute",
        verifier.evaluate("@MainName", "", row) == ("value", "@100"),
    )
    check(
        "a missing attribute reads as the empty string",
        verifier.evaluate("@Help", "", row) == ("value", ""),
    )
    check(
        "an attribute name is case sensitive",
        verifier.evaluate("@mainname", "", row) == ("value", ""),
    )
    check(
        "text() reads the element text",
        verifier.evaluate("text()", "", row) == ("value", "one"),
    )
    check(
        "text() is an exact match",
        verifier.evaluate("text()x", "", row) == ("handle", row),
    )
    check(
        "an empty path keeps the handle",
        verifier.evaluate("", "/Main", main) == ("handle", main),
    )
    check(
        "an unknown first character keeps the handle",
        verifier.evaluate("Main", "/Main", main) == ("handle", main),
    )
    check(
        "a missing node keeps the empty handle",
        verifier.evaluate("/Main", "/Main", None) == ("handle", None),
    )
    check(
        "the attribute predicate selects the first matching child",
        verifier.evaluate("/Main[@MainName=@300]", "/Main", main)
        == ("handle", main.children[2]),
    )
    twin = verifier.Node(
        "L",
        children=[
            verifier.Node("r", {"k": "v", "n": "first"}),
            verifier.Node("r", {"k": "v", "n": "second"}),
        ],
    )
    picked = verifier.evaluate("/L[@k=v]", "/L", twin)
    check(
        "two matching children give the first",
        picked == ("handle", twin.children[0]),
    )
    check(
        "a predicate without a match keeps the source handle",
        verifier.evaluate("/Main[@MainName=@999]", "/Main", main) == ("handle", main),
    )
    check(
        "a predicate without an equals sign keeps the source handle",
        verifier.evaluate("/Main[@MainName]", "/Main", main) == ("handle", main),
    )
    check(
        "a predicate without a closing bracket keeps the source handle",
        verifier.evaluate("/Main[@MainName=@300", "/Main", main) == ("handle", main),
    )
    prefix = verifier.Node(
        "L",
        children=[verifier.Node("r", {"k": "ab"}), verifier.Node("r", {"k": "xy"})],
    )
    check(
        "a missing bracket does not match a truncated value",
        verifier.evaluate("/L[@k=abc", "/L", prefix) == ("handle", prefix),
    )
    main.range = (1, 2)
    verifier.evaluate("/Main", "/Main", main)
    check("every evaluation resets the child range", main.range == (0, 0xFFFF))
    verifier.evaluate("/Main[position() ge 1 and position() le 1]", "/Main", main)
    check("a position predicate stores the bounds", main.range == (1, 1))
    verifier.evaluate("/Main[position() ge 5]", "/Main", main)
    check("bounds clamp to the last child", main.range == (2, 2))
    verifier.evaluate("/Other[position() ge 1]", "/Main", main)
    check("a mismatched path leaves the range untouched", main.range == (0, 0xFFFF))


def range_cases() -> None:
    cases = [
        ("position() ge 3 and position() le 8]", 20, (3, 8)),
        ("position() le 4]", 20, (0, 4)),
        ("position() ge 25]", 20, (19, 19)),
        ("3 le position() and 8 ge position()]", 20, (3, 8)),
        ("5 ge position()]", 10, (0, 5)),
        ("position() ge 2 position() le 5]", 20, (2, 5)),
        ("ge 3]", 10, (0, 9)),
        ("foo]", 10, (0, 9)),
        ("position() le -5]", 10, (0, 9)),
        ("position() ge 0]", 1, (0, 0)),
        ("position() ge 1]", 0, (-1, -1)),
        ("]", 4, (0, 3)),
        ("position() and ge 3]", 10, (0, 9)),
        ("position() 7 ge 9 ge position()]", 20, (9, 9)),
        ("position() ge 3 and position() le 8 and position() le 2]", 20, (2, 2)),
    ]
    for text, count, bounds in cases:
        check(
            f"range {text!r} over {count} children",
            verifier.range_bounds(text, count) == bounds,
        )
    check(
        "tokens split on brackets and spaces",
        verifier.tokens("position() ge  3]") == ["position()", "ge", "3"],
    )
    check("atol reads a leading number", verifier.atol("12abc") == 12)
    check("atol reads zero for text", verifier.atol("abc") == 0)


def row_cases() -> None:
    check("a single row", verifier.row_spec("7") == [7])
    check("a range", verifier.row_spec("0-33") == [0, -1, 33])
    check("a reversed range", verifier.row_spec("5-3") == [3, -1, 5])
    check("an equal range", verifier.row_spec("6-6") == [6])
    check("an open range", verifier.row_spec("12-0") == [12, -2])
    check("a comma and space list", verifier.row_spec("1, 4-5") == [1, 4, -1, 5])
    check("empty text", verifier.row_spec("") == [])
    check(
        "a range includes both ends",
        verifier.expand_rows([2, -1, 4], 10) == [2, 3, 4],
    )
    check(
        "an open range stops at the sheet end",
        verifier.expand_rows([8, -2], 10) == [8, 9],
    )
    check(
        "rows past the sheet are dropped",
        verifier.expand_rows([1, 20], 10) == [1],
    )
    check(
        "a range then a single row",
        verifier.expand_rows([0, -1, 1, 5], 10) == [0, 1, 5],
    )


def list_cases() -> None:
    main = tree().children[0]
    shown = verifier.visible_rows(main.children)
    check(
        "a row with visibility False is hidden",
        [row.attrs["MainName"] for row in shown] == ["@100", "@300"],
    )
    hidden = verifier.Node("r", {"visibility": "false"})
    check(
        "the hidden value is case sensitive",
        verifier.visible_rows([hidden]) == [hidden],
    )
    with_template = main.children[0]
    without = main.children[2]
    check(
        "a selector lets the row name its template",
        verifier.row_template(with_template, "Default", "Sel") == "T1",
    )
    check(
        "a row without a template name uses ItemTemplate",
        verifier.row_template(without, "Default", "Sel") == "Default",
    )
    check(
        "no selector keeps ItemTemplate for every row",
        verifier.row_template(with_template, "Default", "") == "Default",
    )


FORM = """<Window xmlns="http://example.invalid/sqwt" xmlns:x="http://example.invalid/x">
  <Window.Resources>
    <ResourceDictionary>
      <ResourceDictionary.MergedDictionaries>
        <SqwtXmlDataMaker x:Key="XmlData" XPath="/Main" Name="MainList" />
        <RaptureSsdDataReader x:Key="Rd" XPath="/root/m" Name="Rd" SheetName="S" XmlFormat=",A" Row="0-3" />
      </ResourceDictionary.MergedDictionaries>
    </ResourceDictionary>
  </Window.Resources>
  <Grid>
    <ListBox Name="List1" ItemTemplate="Tpl1" ItemTemplateSelector="{StaticResource Sel}" ItemsSource="{Binding Source={StaticResource XmlData}, XPath=/Main, Mode=OneWay}" />
    <CustomControl Name="CC" GlSource="{Binding Source={StaticResource Rd}, XPath=/root/m, Mode=OneWay}" />
  </Grid>
</Window>"""
TEMPLATE = """<ResourceDictionary xmlns="http://example.invalid/sqwt" xmlns:x="http://example.invalid/x">
  <DataTemplate x:Key="Tpl1">
    <Grid>
      <TextBlock Text="{Binding XPath=@Name1, Mode=OneWay}" Visibility="{Binding XPath=@Vis, Mode=OneWay}" />
      <TextBlock Text="plain" />
    </Grid>
  </DataTemplate>
  <ControlTemplate x:Key="Ct">
    <Border Tag="{Binding XPath=@Tag1, Mode=OneWay}" />
  </ControlTemplate>
  <DataTemplate x:Key="NoRows"><Grid /></DataTemplate>
</ResourceDictionary>"""


def census_cases() -> None:
    texts = {"a.form": FORM, "b.tpl": TEMPLATE}
    got = verifier.markup_census(texts)
    check("the census counts files", got["files"] == 2)
    check(
        "the census groups declaration attribute sets",
        got["declarationAttributeSets"]
        == {
            "RaptureSsdDataReader": {"Name,Row,SheetName,XPath,XmlFormat,x:Key": 1},
            "SqwtXmlDataMaker": {"Name,XPath,x:Key": 1},
        },
    )
    check(
        "the census shapes declared paths",
        got["declarationXPathShapes"] == {"/N": 1, "/N/N": 1},
    )
    check(
        "the census shapes binding paths over every attribute of a tag",
        got["bindingXPathShapes"] == {"/N": 1, "/N/N": 1, "@N": 3},
    )
    check(
        "the census counts source and mode",
        got["bindings"] == {"total": 5, "withSource": 2, "oneWay": 5},
    )
    check(
        "the census counts items sources by element",
        got["itemsSourceBindings"] == {"ListBox": 1},
    )
    unterminated = verifier.markup_census({"c.form": '<SqwtXmlDataMaker XPath="/A"'})
    check(
        "an unterminated tag adds no declaration",
        unterminated["declarationAttributeSets"] == {},
    )
    odd = verifier.markup_census(
        {"d.form": '<X A="{Binding XPath=@N, Mode=TwoWay}" B="{Binding Path=Z}" />'}
    )
    check(
        "the census counts a mode other than OneWay apart",
        odd["bindings"] == {"total": 1, "withSource": 0, "oneWay": 0},
    )


def facts_cases() -> None:
    verifier.FORM_FILES["TestForm"] = ["a.form", "b.tpl"]
    try:
        facts = verifier.form_facts({"a.form": FORM, "b.tpl": TEMPLATE}, "TestForm")
    finally:
        del verifier.FORM_FILES["TestForm"]
    check(
        "the form sources keep their attributes",
        facts["sources"]
        == [
            {
                "tag": "SqwtXmlDataMaker",
                "x:Key": "XmlData",
                "XPath": "/Main",
                "Name": "MainList",
            },
            {
                "tag": "RaptureSsdDataReader",
                "x:Key": "Rd",
                "XPath": "/root/m",
                "Name": "Rd",
                "SheetName": "S",
                "XmlFormat": ",A",
                "Row": "0-3",
            },
        ],
    )
    check(
        "the form bindings keep their source key and template names",
        facts["bindings"]
        == [
            {
                "element": "ListBox",
                "name": "List1",
                "property": "ItemsSource",
                "source": "XmlData",
                "xpath": "/Main",
                "itemTemplate": "Tpl1",
                "selector": True,
            },
            {
                "element": "CustomControl",
                "name": "CC",
                "property": "GlSource",
                "source": "Rd",
                "xpath": "/root/m",
                "itemTemplate": "",
                "selector": False,
            },
        ],
    )
    check(
        "template row attributes list only keyed templates with at-sign bindings",
        facts["templateRowAttributes"] == {"Tpl1": ["Name1", "Vis"], "Ct": ["Tag1"]},
    )
    rows = verifier.attribute_names_in_templates(ET.fromstring(TEMPLATE))
    check("the helper agrees on the same tree", rows == facts["templateRowAttributes"])
    check(
        "a binding with a slash path is not a row attribute",
        verifier.attribute_names_in_templates(
            ET.fromstring(
                '<R><DataTemplate Key="K"><A V="{Binding XPath=/X}" /></DataTemplate></R>'
            )
        )
        == {},
    )


def decode_cases() -> None:
    if os.name == "nt":
        return
    with tempfile.TemporaryDirectory() as raw:
        base = Path(raw)
        sqwt = base / "install" / "client" / "sqwt" / "widget"
        sqwt.mkdir(parents=True)
        (sqwt / "a.form").write_bytes(b"SQEX" + bytes(8))
        (sqwt / "plain.txt").write_bytes(b"nope")
        fake = base / "xivl"
        fake.write_text(
            f"#!{sys.executable}\n"
            "import sys, pathlib\n"
            "out = pathlib.Path(sys.argv[sys.argv.index('--output') + 1])\n"
            "(out / 'payloads').mkdir(parents=True)\n"
            "(out / 'payloads' / 'decoded.xml').write_text('<A XPath=\"/Z\"/>')\n",
            encoding="ascii",
        )
        fake.chmod(fake.stat().st_mode | stat.S_IXUSR)
        texts = verifier.decode_markup(base / "install", fake)
        check(
            "decode keeps only SQEX files under their sqwt path",
            texts == {"widget/a.form": '<A XPath="/Z"/>'},
        )
        only = verifier.decode_markup(base / "install", fake, ["widget/a.form"])
        check("decode honors an explicit file list", list(only) == ["widget/a.form"])


def image_bytes(rows: list[tuple[int, bytes]]) -> bytes:
    """Build a PE32 image with one section per row."""
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(rows) + 0xFFF) & ~0xFFF
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(rows))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    body = bytearray()
    for index, (va, data) in enumerate(rows):
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII",
            header,
            base + 8,
            len(data),
            va - IMAGE_BASE,
            len(data),
            first_raw + len(body),
        )
        body += data
    return bytes(header) + bytes(body)


def row_bytes(item: dict) -> bytes:
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "dwords":
        return b"".join(
            int(value, 16).to_bytes(4, "little") for value in item["values"]
        )
    return bytes.fromhex(item["pattern"])


def image_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    checks = document["staticChecks"]
    rows = [(int(item["va"], 16), row_bytes(item)) for item in checks]
    image = image_bytes(rows)
    check(
        "an image built from the rows passes",
        verifier.verify_image(image, document, require_identity=False) == [],
    )
    check("identity is enforced", len(verifier.verify_image(image, document)) >= 2)
    first_raw = (0x80 + 24 + 0xE0 + 40 * len(rows) + 0xFFF) & ~0xFFF
    missed = []
    offset = first_raw
    for index, (_, data) in enumerate(rows):
        flipped = bytearray(image)
        flipped[offset] ^= 0x01
        if not verifier.verify_image(bytes(flipped), document, require_identity=False):
            missed.append(checks[index]["name"])
        offset += len(data)
    check(f"every row reports a flipped first byte (missed {missed})", not missed)
    moved = image_bytes([(va + 1, data) for va, data in rows])
    check(
        "rows at shifted addresses are reported",
        verifier.verify_image(moved, document, require_identity=False) != [],
    )
    check(
        "an image that is not a PE is reported",
        verifier.verify_image(b"not a pe", document, require_identity=False) != [],
    )
    masked = [
        i for i in checks if i["kind"] == "signature" and "00" in i["mask"].split()
    ]
    check("signature rows mask call displacements", len(masked) >= 20)
    for item in masked[:3]:
        parts = item["mask"].split()
        position = parts.index("00")
        data = bytearray(row_bytes(item))
        data[position] ^= 0xFF
        patched = [
            (int(c["va"], 16), bytes(data) if c is item else row_bytes(c))
            for c in checks
        ]
        check(
            f"a masked byte of {item['name']} is ignored",
            verifier.verify_image(
                image_bytes(patched), document, require_identity=False
            )
            == [],
        )


def manifest_cases() -> None:
    check("the manifest verifies", verifier.verify() == [])
    check("every manifest mutation is rejected", verifier.mutation_test() == [])
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    check("the page holds every line and row", verifier.doc_errors(document) == [])
    check(
        "a page without the lines is rejected",
        verifier.doc_errors(document, "STATIC INFERRED UNKNOWN CALIBRATION MEASURED")
        != [],
    )
    page = verifier.DOC.read_text(encoding="ascii")
    for label, old, new in (
        ("a changed census count", "| `@N` | 642 |", "| `@N` | 641 |"),
        ("a changed offset", "| 2 | `XPath` | `0x54` |", "| 2 | `XPath` | `0x58` |"),
        ("a changed shape label", "| STATIC |\n| `/N/N` |", "| INFERRED |\n| `/N/N` |"),
        (
            "a changed source key",
            "| `XmlData_Main` | `/Main` |",
            "| `XmlData_Main` | `/Main2` |",
        ),
        ("a changed item property", "| 6 | `ItemsSource` |", "| 6 | `ItemSource` |"),
        (
            "a changed binding field",
            "| B+0x54 | xpath |",
            "| B+0x56 | xpath |",
        ),
    ):
        check(
            f"a page with {label} is rejected",
            old in page and verifier.doc_errors(document, page.replace(old, new)) != [],
        )
    broken = copy.deepcopy(document)
    broken["staticChecks"][0]["kind"] = "bytes"
    check("an unknown row kind is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["staticChecks"][0]["mask"] = "FF"
    check("a mask of the wrong length is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["xpathForms"].reverse()
    check("a reordered shape table is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["forms"].pop("MiniMapWidget")
    check("a missing form is rejected", verifier.verify(broken) != [])
    check(
        "the three forms hold only slash and at-sign bindings",
        all(
            row["xpath"].startswith(("/", "@")) and "[" not in row["xpath"]
            for name in verifier.FORM_NAMES
            for row in document["forms"][name]["bindings"]
        ),
    )
    check(
        "the census holds no position predicate",
        verifier.shape("/Main[position() ge 1 and position() le 2]")
        not in document["census"]["bindingXPathShapes"],
    )
    check(
        "the reader rows of the minimap are all declared",
        [source["x:Key"] for source in document["forms"]["MiniMapWidget"]["sources"]]
        == ["type_data", "gl_marker_data", "ssd_marker_data", "map_marker_data"],
    )


def main() -> int:
    select_cases()
    evaluate_cases()
    range_cases()
    row_cases()
    list_cases()
    census_cases()
    facts_cases()
    decode_cases()
    image_cases()
    manifest_cases()
    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} sqwt xpath binding tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
