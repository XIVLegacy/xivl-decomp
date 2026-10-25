#!/usr/bin/env python3
"""Tests for the Sqwt tab, scroll and text part verifier, its models and its census."""

from __future__ import annotations

import copy
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import census_sqwt_tab_scroll_text as census  # noqa: E402
import verify_sqwt_tab_scroll_text as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000
SCRATCH_VA = 0x0A000000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


ZERO = (0.0, 0.0, 0.0, 0.0)


def layout_cases() -> None:
    border = (1.0, 2.0, 3.0, 4.0)
    padding = (10.0, 20.0, 30.0, 40.0)
    check(
        "the inset sum adds border and padding per side",
        verifier.thickness_sum(border, padding) == (44.0, 66.0),
    )
    seen: list[tuple[float, float]] = []

    def child(size):
        seen.append(size)
        return (50.0, 70.0)

    check(
        "control measure deflates for the child and inflates the result",
        verifier.control_measure((200.0, 300.0), border, padding, [child])
        == (94.0, 136.0)
        and seen == [(156.0, 234.0)],
    )
    check(
        "control measure takes the largest child per direction",
        verifier.control_measure(
            (100.0, 100.0),
            ZERO,
            ZERO,
            [lambda size: (5.0, 90.0), lambda size: (60.0, 10.0)],
        )
        == (60.0, 90.0),
    )
    check(
        "control measure of a childless control is the inset sum",
        verifier.control_measure((100.0, 100.0), border, padding, []) == (44.0, 66.0),
    )
    check(
        "control arrange offsets by the left and top insets",
        verifier.control_arrange((200.0, 300.0), border, padding)
        == (11.0, 22.0, 156.0, 234.0),
    )
    seen.clear()
    check(
        "scroll viewer measure counts the inset twice",
        verifier.scrollviewer_measure((200.0, 300.0), border, padding, [child])
        == (138.0, 202.0)
        and seen == [(112.0, 168.0)],
    )
    check(
        "a scroll viewer without insets measures like a control",
        verifier.scrollviewer_measure((80.0, 90.0), ZERO, ZERO, [child])
        == verifier.control_measure((80.0, 90.0), ZERO, ZERO, [child]),
    )
    calls: list[tuple[float, float]] = []

    def big(size):
        calls.append(size)
        return (500.0, 40.0) if len(calls) == 1 else (size[0], size[1])

    desired, extent = verifier.presenter_measure((100.0, 80.0), big)
    check(
        "an overflowing presenter child is measured again at the larger sizes",
        calls == [(100.0, 80.0), (500.0, 80.0)] and desired == (500.0, 80.0) == extent,
    )
    calls.clear()
    desired, extent = verifier.presenter_measure(
        (100.0, 80.0), lambda size: calls.append(size) or (60.0, 30.0)
    )
    check(
        "a fitting presenter child is measured once",
        calls == [(100.0, 80.0)] and desired == (60.0, 30.0) == extent,
    )
    check(
        "presenter arrange never gives the child less than its desired size",
        verifier.presenter_arrange((100.0, 20.0), (60.0, 30.0))
        == ((100.0, 30.0), (100.0, 20.0)),
    )
    check(
        "the item size guard is off at the defaults",
        not verifier.items_guard(10_000_000.0, 0.0),
    )
    check(
        "the item size guard is on for a maximum",
        verifier.items_guard(5.0, 0.0),
    )
    check(
        "the item size guard is on for a minimum",
        verifier.items_guard(10_000_000.0, 1.0),
    )


def textbox_cases() -> None:
    base = dict(
        border=ZERO,
        padding=(2.0, 0.0, 0.0, 0.0),
        min_lines=1,
        max_lines=verifier.UNLIMITED_LINES,
        line_height=14.0,
        extra=0,
        stretch=True,
        host_a=0.0,
        host_b=0.0,
    )
    check(
        "the default limits leave the control arrange rectangle",
        verifier.textbox_rect((100.0, 40.0), **base) == (2.0, 0.0, 98.0, 40.0)
        and verifier.control_arrange((100.0, 40.0), ZERO, base["padding"])
        == (2.0, 0.0, 98.0, 40.0),
    )
    capped = dict(base, max_lines=2)
    check(
        "a stretch alignment caps the height at the visible lines",
        verifier.textbox_rect((100.0, 40.0), **capped) == (2.0, 6.0, 98.0, 28.0),
    )
    check(
        "a non-stretch alignment ignores the maximum",
        verifier.textbox_rect((100.0, 40.0), **dict(capped, stretch=False))
        == (2.0, 0.0, 98.0, 40.0),
    )
    check(
        "the height is raised to the minimum lines and centred",
        verifier.textbox_rect((100.0, 10.0), **dict(base, min_lines=2))
        == (2.0, -9.0, 98.0, 28.0),
    )
    check(
        "the host fields enter the capped and raised heights",
        verifier.textbox_rect((100.0, 40.0), **dict(capped, host_a=3.0, host_b=5.0))
        == (2.0, 5.0, 98.0, 30.0),
    )
    check(
        "the extra term lowers the cap and the floor",
        verifier.textbox_rect((100.0, 40.0), **dict(capped, extra=4))
        == (2.0, 8.0, 98.0, 24.0),
    )


def enum_cases() -> None:
    check(
        "scroll bar visibility names give 1 to 4",
        [
            verifier.scrollbar_visibility(name)
            for name in ("Disabled", "Hidden", "Visible", "AutoHidden")
        ]
        == [1, 2, 3, 4],
    )
    check(
        "Auto and unknown text give 0",
        [verifier.scrollbar_visibility(name) for name in ("Auto", "", "True", "auto1")]
        == [0, 0, 0, 0],
    )
    check(
        "the scroll bar visibility match ignores ASCII case",
        verifier.scrollbar_visibility("autohidden") == 4
        and verifier.scrollbar_visibility("VISIBLE") == 3,
    )
    check(
        "scroll bar visibility values write back to names",
        [verifier.scrollbar_visibility_name(value) for value in range(6)]
        == ["Auto", "Disabled", "Hidden", "Visible", "AutoHidden", "Auto"],
    )
    check(
        "the case fold changes only ASCII A to Z",
        verifier.ascii_lower("AbZ\u212a\u0130\u00c9") == "abz\u212a\u0130\u00c9"
        and verifier.scrollbar_visibility("\u212aidden") == 0
        and verifier.visibility("F\u00c0LSE") == 2,
    )
    check(
        "visibility text maps Collapsed and False to 0",
        verifier.visibility("Collapsed") == 0 and verifier.visibility("False") == 0,
    )
    check(
        "visibility text maps Hidden to 1 and the rest to 2",
        verifier.visibility("Hidden") == 1
        and [verifier.visibility(x) for x in ("Visible", "True", "", "x")] == [2] * 4,
    )
    check(
        "Auto shows a bar only on overflow",
        [verifier.computed_visibility(0, e, 10.0) for e in (5.0, 10.0, 11.0)]
        == [0, 0, 2],
    )
    check(
        "AutoHidden hides a bar that fits",
        [verifier.computed_visibility(4, e, 10.0) for e in (5.0, 10.0, 11.0)]
        == [1, 1, 2],
    )
    check(
        "Disabled, Hidden and Visible ignore the sizes",
        [verifier.computed_visibility(m, 99.0, 1.0) for m in (1, 2, 3)] == [0, 1, 2]
        and [verifier.computed_visibility(m, 1.0, 99.0) for m in (1, 2, 3)]
        == [0, 1, 2],
    )
    check(
        "scrollable sizes floor at zero",
        verifier.scrollable(30.0, 10.0) == 20.0
        and verifier.scrollable(5.0, 10.0) == 0.0,
    )


def tab_cases() -> None:
    check(
        "the arrows hide when the strip fits",
        verifier.tab_arrows(100.0, 100.0, 0.0) == (False, False, False),
    )
    check(
        "an overflowing strip shows the arrows and enables the right one",
        verifier.tab_arrows(200.0, 100.0, 0.0) == (True, False, True),
    )
    check(
        "a scrolled strip enables the left arrow",
        verifier.tab_arrows(200.0, 100.0, 40.0) == (True, True, True),
    )
    check(
        "the right arrow disables at the end of the strip",
        verifier.tab_arrows(200.0, 100.0, 100.0) == (True, True, False),
    )
    visible = verifier.VISIBILITY["Visible"]
    hidden = verifier.VISIBILITY["Hidden"]
    check(
        "the first visible item is selected and its content is left alone",
        verifier.items_added(-1, [visible]) == (0, False, False),
    )
    check(
        "with a selection the added content is appended and hidden",
        verifier.items_added(0, [visible, visible]) == (0, True, True),
    )
    check(
        "a content element already in the panel is not appended again",
        verifier.items_added(0, [visible, visible], in_panel=True) == (0, False, False),
    )
    check(
        "a hidden last item returns early with no selection and no append",
        verifier.items_added(-1, [hidden]) == (-1, False, False),
    )
    check(
        "the last added item is the one selected",
        verifier.items_added(-1, [hidden, visible]) == (1, False, False),
    )
    check("no items change nothing", verifier.items_added(-1, []) == (-1, False, False))
    check(
        "the panel parent does not matter while nothing is selected",
        verifier.items_added(-1, [visible], in_panel=True) == (0, False, False),
    )
    full = {
        "SkinNormal": "n",
        "SkinActive": "a",
        "SkinDisabled": "d",
        "SkinFocused": "f",
        "SkinHover": "h",
        "SkinReserved": "r",
        "SkinAnchored": "c",
    }
    check(
        "frame slots follow the executable order",
        verifier.frame_slots(full) == ["n", "a", "d", "f", "h", "r", "c"],
    )
    partial = {name: full[name] for name in ("SkinNormal", "SkinActive", "SkinFocused")}
    check(
        "an absent Reserved takes Active and an absent Anchored takes Focused",
        verifier.frame_slots(partial) == ["n", "a", None, "f", None, "a", "f"],
    )
    check(
        "an explicit NULL Reserved is kept",
        verifier.frame_slots(dict(full, SkinReserved="NULL"))[5] == "NULL",
    )


CENSUS_FORM = """<Window SqwtStyleFile="common/default.style" Name="W">
  <TabControl SqwtStyle="TBC_a" Name="T"><TabItem SqwtStyle="TBI_a" Header="@1"/>
    <TabItem SqwtStyle="TBI_a"/></TabControl>
  <Grid><TextBox SqwtStyle="TEB_a" CaretIndex="0"/></Grid>
  <ScrollViewer SqwtStyle="SCV_missing"/>
  <LogControl SqwtStyle="LOG_a" Name="L"><Border TargetName="T:PART_X"/></LogControl>
  <KeyBinding CommandTarget="{Binding ElementName=L:PART_ContentHost,Path=p}" Name="K"/>
</Window>"""
CENSUS_TEMPLATE = """<ResourceDictionary>
  <Setter TargetName="T:TabItem_1" Property="IsHitTestVisible" Value="False"/>
  <DoubleAnimation Storyboard.TargetName="T:PART_Y" To="0"/>
  <Style TargetType="TextBox"><Setter Property="Name" Value="x"/></Style>
</ResourceDictionary>"""
CENSUS_STYLE = """<StyleResources Source="common/default.skin">
  <Widget Type="TabControl"><SqwtStyle Name="TBC_a">
    <Parts Name="Background_SelectedContent" Reference="Window" Style="WIN_a"/>
    <Parts Name="Background_TabPanel" Reference="Window" Style="NULL"/></SqwtStyle></Widget>
  <Widget Type="TabItem"><SqwtStyle Name="TBI_a">
    <Parts Name="Background_Header" Reference="Button" Style="BTN_a"/></SqwtStyle></Widget>
  <Widget Type="TextBox"><SqwtStyle Name="TEB_a">
    <Parts Name="Background" Reference="Window" Style="WIN_a"/></SqwtStyle></Widget>
  <Widget Type="LogControl"><SqwtStyle Name="LOG_a">
    <Parts Name="Background" Reference="Window" Style="WIN_a"/></SqwtStyle></Widget>
  <Widget Type="Window"><SqwtStyle Name="WIN_a"><Parts Name="Frame">
    <Property Condition="SkinNormal" Skin="S_N"/><Property Condition="SkinActive" Skin="S_A"/>
    <Property Condition="SkinHover" Skin="NULL"/></Parts></SqwtStyle></Widget>
  <Widget Type="Button"><SqwtStyle Name="BTN_a"><Parts Name="Frame">
    <Property Condition="SkinNormal" Skin="S_B"/><Property Condition="SkinReserved" Skin="S_R"/>
    </Parts></SqwtStyle></Widget>
</StyleResources>"""
CENSUS_SHADOW = """<StyleResources Source="common/default.skin">
  <Widget Type="Window"><SqwtStyle Name="WIN_a"><Parts Name="Frame">
    <Property Condition="SkinNormal" Skin="S_X"/><Property Condition="SkinReserved" Skin="S_X"/>
    </Parts></SqwtStyle></Widget>
  <Widget Type="Button"><SqwtStyle Name="BTN_a"><Parts Name="Frame">
    <Property Condition="SkinNormal" Skin="S_B"/><Property Condition="SkinReserved" Skin="S_R"/>
    </Parts></SqwtStyle></Widget>
</StyleResources>"""
CENSUS_SKIN = """<SkinResources><CellSkins><CellSkin Name="S_N"/><CellSkin Name="S_A"/>
  <CellSkin Name="S_B"/></CellSkins><AnimationSkins><AnimationSkin Name="S_R"/>
  </AnimationSkins></SkinResources>"""


def census_cases() -> None:
    files = {
        "widget/LogWidget.form": CENSUS_FORM,
        "widget/LogWidget.tpl": CENSUS_TEMPLATE,
        "common/default.style": CENSUS_STYLE,
        "common/default.skin": CENSUS_SKIN,
        "widget/Broken.form": "<Window><Unclosed></Window>",
        "widget/Bomb.form": "<!DOCTYPE x [<!ENTITY a 'b'>]><Window/>",
        "widget/readme.txt": "<TabControl/>",
    }
    result = census.census_texts(files)
    check(
        "the corpus counts files by kind and counts unparsed documents",
        result["corpus"]
        == {"forms": 3, "templates": 1, "styles": 1, "skins": 1, "unparsed": 2},
    )
    tab = result["elements"]["TabControl"]
    check(
        "elements, files and attributes are counted",
        tab["elements"] == 1
        and tab["files"] == 1
        and tab["attributes"] == {"Name": 1, "SqwtStyle": 1}
        and result["elements"]["TabItem"]["elements"] == 2
        and result["elements"]["TabItem"]["styles"] == {"TBI_a": 2},
    )
    check(
        "a template style target is not an element",
        result["elements"]["TextBox"]["elements"] == 1,
    )
    check(
        "style resolution counts defined styles",
        result["styleResolution"]["ScrollViewer"] == {"distinct": 1, "defined": 0}
        and result["styleResolution"]["TabControl"] == {"distinct": 1, "defined": 1},
    )
    log = result["logWidget"]
    check(
        "the LogWidget closure reads the style and skin files named by the form",
        log["styleFile"] == "common/default.style"
        and log["skinFile"] == "common/default.skin",
    )
    check(
        "the LogWidget element counts and styles are kept",
        log["elements"]["TabItem"] == 2
        and log["styles"]["ScrollViewer"] == ["SCV_missing"]
        and log["styles"]["TabControl"] == ["TBC_a"],
    )
    check(
        "the part rows are depth first",
        log["parts"][:3]
        == [
            ["TabControl", "TBC_a", "Background_SelectedContent", "Window", "WIN_a"],
            ["TabControl", "TBC_a", "Background_TabPanel", "Window", "NULL"],
            ["TabItem", "TBI_a", "Background_Header", "Button", "BTN_a"],
        ],
    )
    check(
        "a shared style is listed once",
        sum(1 for row in log["parts"] if row[:3] == ["TextBox", "TEB_a", "Background"])
        == 1
        and [row[:3] for row in log["frames"]].count(["Window", "WIN_a", "Frame"]) == 1,
    )
    check(
        "frame rows keep the seven slots in executable order",
        ["Window", "WIN_a", "Frame", "S_N", "S_A", "", "", "NULL", "", ""]
        in log["frames"]
        and ["Button", "BTN_a", "Frame", "S_B", "", "", "", "", "S_R", ""]
        in log["frames"],
    )
    check(
        "skin names are counted without NULL and typed from the skin file",
        log["skinNames"] == 4
        and log["skinKinds"] == {"AnimationSkins": 1, "CellSkins": 3},
    )
    check(
        "target paths come from target attributes and element name bindings",
        log["targets"]
        == {"L:PART_ContentHost": 1, "T:PART_X": 1, "T:PART_Y": 1, "T:TabItem_1": 1},
    )
    for path in ("system/other.style", "aaa/other.style"):
        shadowed = census.census_texts({**files, path: CENSUS_SHADOW})["logWidget"]
        check(
            f"a style file that sorts after or before the named one ({path}) "
            "does not change the closure",
            shadowed["frames"] == log["frames"]
            and shadowed["parts"] == log["parts"]
            and shadowed["skinNames"] == log["skinNames"],
        )
        check(
            f"shadowing counts the other files and the differing rows ({path})",
            shadowed["shadowing"]
            == {"leafStyles": 2, "fewestFiles": 2, "mostFiles": 2, "differing": 1},
        )
    check(
        "a lone style file has no shadowing",
        log["shadowing"]
        == {"leafStyles": 2, "fewestFiles": 1, "mostFiles": 1, "differing": 0},
    )
    check(
        "an other file does not replace the definition counts of a missing style",
        census.census_texts({**files, "system/other.style": CENSUS_SHADOW})[
            "styleResolution"
        ]["ScrollViewer"]
        == {"distinct": 1, "defined": 0},
    )
    check(
        "a form with no LogWidget gives an empty closure",
        census.census_texts({"widget/A.form": "<Window/>"})["logWidget"] == {},
    )


def row_bytes(item: dict) -> bytes:
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "dwords":
        return b"".join(int(v, 16).to_bytes(4, "little") for v in item["values"])
    if item["kind"] == "rangeSha256":
        return bytes(index % 251 for index in range(item["length"]))
    return bytes.fromhex(item["pattern"])


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


class Scratch:
    """Allocator for synthetic structures that rows point at."""

    def __init__(self) -> None:
        self.next = SCRATCH_VA
        self.rows: list[tuple[int, bytes]] = []

    def put(self, data: bytes) -> int:
        va = self.next
        self.rows.append((va, data))
        self.next += (len(data) + 0xFFF) & ~0xFFF
        return va

    def text(self, text: str) -> int:
        return self.put(text.encode("ascii") + b"\0")


def rtti_rows(item: dict, scratch: Scratch) -> list[tuple[int, bytes]]:
    """A vftable whose locator walks to the named base classes."""
    descriptors = [
        scratch.put(bytes(8) + name.encode("ascii") + b"\0") for name in item["bases"]
    ]
    entries = [scratch.put(struct.pack("<I", descriptor)) for descriptor in descriptors]
    array = scratch.put(b"".join(struct.pack("<I", entry) for entry in entries))
    hierarchy = scratch.put(struct.pack("<IIII", 0, 0, len(entries), array))
    locator = scratch.put(struct.pack("<IIIII", 0, 0, 0, 0, hierarchy))
    return [(int(item["va"], 16) - 4, struct.pack("<I", locator))]


def table_rows(item: dict, scratch: Scratch) -> list[tuple[int, bytes]]:
    base = int(item["va"], 16)
    body = bytearray(item["stride"] * item["count"])
    for index, key in enumerate(item["keys"]):
        struct.pack_into(
            "<I", body, index * item["stride"] + item["keyOffset"], scratch.text(key)
        )
    return [(base, bytes(body))]


def merged_sections(memory: dict[int, int]) -> list[tuple[int, bytes]]:
    """Group a sparse byte map into contiguous sections."""
    sections: list[tuple[int, bytes]] = []
    for address in sorted(memory):
        if sections and sections[-1][0] + len(sections[-1][1]) == address:
            sections[-1] = (sections[-1][0], sections[-1][1] + bytes([memory[address]]))
        else:
            sections.append((address, bytes([memory[address]])))
    return sections


def file_offset(sections: list[tuple[int, bytes]], address: int) -> int:
    first_raw = (0x80 + 24 + 0xE0 + 40 * len(sections) + 0xFFF) & ~0xFFF
    offset = first_raw
    for va, data in sections:
        if va <= address < va + len(data):
            return offset + address - va
        offset += len(data)
    raise ValueError(hex(address))


def image_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    scratch = Scratch()
    memory: dict[int, int] = {}
    for item in document["staticChecks"]:
        if item["kind"] == "rangeSha256":
            va = int(item["va"], 16)
            for index in range(item["length"]):
                memory[va + index] = index % 251
    structure_rows: list[tuple[int, bytes]] = []
    plain: list[dict] = []
    for item in document["staticChecks"]:
        if item["kind"] == "rtti":
            structure_rows += rtti_rows(item, scratch)
        elif item["kind"] == "recordKeys":
            structure_rows += table_rows(item, scratch)
        elif item["kind"] != "rangeSha256":
            for index, value in enumerate(row_bytes(item)):
                memory[int(item["va"], 16) + index] = value
            plain.append(item)
    for va, data in structure_rows + scratch.rows:
        for index, value in enumerate(data):
            memory[va + index] = value
    for item in document["staticChecks"]:
        if item["kind"] == "rangeSha256":
            va = int(item["va"], 16)
            raw = bytes(memory[va + i] for i in range(item["length"]))
            item["sha256"] = hashlib.sha256(raw).hexdigest()
    sections = merged_sections(memory)
    image = image_bytes(sections)
    check(
        "an image built from the rows passes",
        verifier.verify_image(image, document, require_identity=False) == [],
    )
    check("identity is enforced", len(verifier.verify_image(image, document)) >= 2)
    missed = []
    for item in document["staticChecks"]:
        if item["kind"] in {"rtti", "recordKeys"}:
            continue
        flipped = bytearray(image)
        flipped[file_offset(sections, int(item["va"], 16))] ^= 0x01
        table = verifier.image_checks.section_map(bytes(flipped))
        if verifier.run_check(bytes(flipped), table, item) is None:
            missed.append(item["name"])
    check(
        f"every plain and range row reports a flipped byte (missed {missed})",
        not missed,
    )
    rtti = next(i for i in document["staticChecks"] if i["kind"] == "rtti")
    flipped = bytearray(image)
    flipped[file_offset(sections, int(rtti["va"], 16) - 4)] ^= 0x01
    check(
        "a moved locator is reported",
        verifier.verify_image(bytes(flipped), document, require_identity=False) != [],
    )
    wrong = copy.deepcopy(document)
    next(i for i in wrong["staticChecks"] if i["kind"] == "rtti")["bases"][1] = (
        ".?AVx@@"
    )
    check(
        "a wrong base class name is reported",
        verifier.verify_image(image, wrong, require_identity=False) != [],
    )
    wrong = copy.deepcopy(document)
    next(i for i in wrong["staticChecks"] if i["kind"] == "recordKeys")["keys"][5] = "x"
    check(
        "a wrong part table key is reported",
        verifier.verify_image(image, wrong, require_identity=False) != [],
    )
    moved = image_bytes([(va + 1, data) for va, data in sections])
    check(
        "rows at shifted addresses are reported",
        len(verifier.verify_image(moved, document, require_identity=False)) >= 1,
    )
    check(
        "an image that is not a PE is reported",
        verifier.verify_image(b"not a pe", document, require_identity=False) != [],
    )


def manifest_cases() -> None:
    check("the manifest verifies", verifier.verify() == [])
    check("every manifest mutation is rejected", verifier.mutation_test() == [])
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    check("the page holds every row and line", verifier.doc_errors(document) == [])
    check(
        "a page without a contract line is rejected",
        verifier.doc_errors(document, "STATIC INFERRED UNKNOWN CALIBRATION") != [],
    )
    page = verifier.DOC.read_text(encoding="ascii")
    for label, old, new in (
        (
            "a changed class slot",
            "| TabControl | Selector, ItemsControl, Control | 0x01081654 | 0x00960BF0 |",
            "| TabControl | Selector, ItemsControl, Control | 0x01081654 | 0x00960BF1 |",
        ),
        (
            "a changed census count",
            "| TabControl | 27 | 25 |",
            "| TabControl | 27 | 24 |",
        ),
        ("a changed enum value", "| AutoHidden | 4 |", "| AutoHidden | 3 |"),
        (
            "a changed skin name",
            "| Button | TBI_basis_header | TBI_basis_header_N |",
            "| Button | TBI_basis_header | TBI_basis_header_H |",
        ),
        (
            "a changed target count",
            "| TabControl_Log:PART_SelectedContentPanel | 3 |",
            "| TabControl_Log:PART_SelectedContentPanel | 2 |",
        ),
        (
            "a changed part record",
            "| 52 | TabItem:Background_Header |",
            "| 52 | TabItem:Background_Footer |",
        ),
    ):
        check(
            f"a page with {label} is rejected",
            old in page and verifier.doc_errors(document, page.replace(old, new)) != [],
        )
    check(
        "a wrapped and ticked line is accepted",
        verifier.flatten("`a`\n  `b` c") == "a b c"
        and verifier.doc_errors(document, page) == [],
    )
    broken = copy.deepcopy(document)
    broken["staticChecks"][0]["kind"] = "bytes"
    check("an unknown row kind is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["census"]["elements"]["TabControl"]["attributes"]["TabStripPlacement"] = 1
    check(
        "an install that sets an unset attribute is rejected",
        verifier.verify(broken) != [],
    )
    broken = copy.deepcopy(document)
    broken["census"]["logWidget"]["skinKinds"] = {"CellSkins": 37}
    check("skin kinds that miss the frames are rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["open"] = [{"label": "STATIC", "text": "x"}]
    check("a STATIC open question is rejected", verifier.verify(broken) != [])
    check(
        "the manifest layout rows match the offset-0 slot pair",
        all(
            item["arrange"] == f"0x{verifier.LAYOUT[item['class']][0]:08x}"
            for item in document["classes"]
        ),
    )
    check(
        "the census holds every LogWidget control",
        all(
            document["census"]["logWidget"]["elements"][kind] >= 1
            for kind in census.TYPES
        ),
    )


def main() -> int:
    layout_cases()
    textbox_cases()
    enum_cases()
    tab_cases()
    census_cases()
    image_cases()
    manifest_cases()
    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} sqwt tab scroll text tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
