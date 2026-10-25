#!/usr/bin/env python3
"""Validate the Sqwt TabControl, ScrollViewer and TextBox part contract, its models and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import functools
import hashlib
import json
import re
import struct
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "sqwt_tab_scroll_text.json"
DOC = ROOT / "docs" / "resource" / "sqwt-tab-scroll-text-parts.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
HEX8_RE = re.compile(r"^0x[0-9a-f]{8}$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
LABELS = {"STATIC", "INFERRED", "UNKNOWN", "CALIBRATION"}
TYPES = ("TabControl", "TabItem", "TextBox", "ScrollViewer", "LogControl")
CONDITIONS = (
    "SkinNormal",
    "SkinActive",
    "SkinDisabled",
    "SkinFocused",
    "SkinHover",
    "SkinReserved",
    "SkinAnchored",
)
UNLIMITED_ITEMS = 10_000_000.0
UNLIMITED_LINES = 0x7FFFFFFF
VISIBILITY = {"Collapsed": 0, "Hidden": 1, "Visible": 2}
SCROLLBAR_VISIBILITY = {
    "Auto": 0,
    "Disabled": 1,
    "Hidden": 2,
    "Visible": 3,
    "AutoHidden": 4,
}

# (key, mangled name, name string VA, offset-0 vftable, base chain after the class)
CLASSES = (
    (
        "TabControl",
        ".?AVTabControl@Controls@Sqwt@@",
        0x012E8D18,
        0x01081654,
        ("Selector", "ItemsControl", "Control"),
    ),
    (
        "TabPanel",
        ".?AVTabPanel@Primitives@Controls@Sqwt@@",
        0x012E8D40,
        0x01081A7C,
        ("ListBox", "Selector", "ItemsControl", "Control"),
    ),
    (
        "TabItem",
        ".?AVTabItem@Controls@Sqwt@@",
        0x012E8BC8,
        0x01080E6C,
        ("HeaderedContentControl", "ContentControl", "Control"),
    ),
    (
        "TabItemBorder",
        ".?AVTabItemBorder@Controls@Sqwt@@",
        0x012E8B9C,
        0x01080C24,
        ("ContentPresenter",),
    ),
    (
        "SelectedContentPanel",
        ".?AVSelectedContentPanel@TabControl@Controls@Sqwt@@",
        0x012E8C90,
        0x010813E4,
        ("Panel",),
    ),
    (
        "ScrollViewer",
        ".?AVScrollViewer@Controls@Sqwt@@",
        0x012E45AC,
        0x010733D4,
        ("ContentControl", "Control"),
    ),
    (
        "ScrollContentPresenter",
        ".?AVScrollContentPresenter@Controls@Sqwt@@",
        0x012E90A4,
        0x010829A4,
        ("ContentPresenter",),
    ),
    (
        "TextBoxBase",
        ".?AVTextBoxBase@Primitives@Controls@Sqwt@@",
        0x012712B8,
        0x0107573C,
        ("Control",),
    ),
    (
        "TextBoxScrollViewer",
        ".?AVTextBoxScrollViewer@TextBoxBase@Primitives@Controls@Sqwt@@",
        0x012E4C40,
        0x01074F74,
        ("ScrollViewer", "ContentControl", "Control"),
    ),
    (
        "TextBox",
        ".?AVTextBox@Controls@Sqwt@@",
        0x0126D2C8,
        0x0106EDB4,
        ("TextBoxBase", "Control"),
    ),
    (
        "LogControl",
        ".?AVLogControl@CustomControl@SqwtInterface@Main@Application@@",
        0x01271270,
        0x00FA935C,
        ("TextBox", "TextBoxBase", "Control"),
    ),
    (
        "ContentControl",
        ".?AVContentControl@Controls@Sqwt@@",
        0x01269E44,
        0x0106B634,
        ("Control",),
    ),
    (
        "ItemsControl",
        ".?AVItemsControl@Controls@Sqwt@@",
        0x0126D6F4,
        0x0106FADC,
        ("Control",),
    ),
    (
        "Selector",
        ".?AVSelector@Primitives@Controls@Sqwt@@",
        0x012BBC78,
        0x01072874,
        ("ItemsControl", "Control"),
    ),
    (
        "ListBox",
        ".?AVListBox@Controls@Sqwt@@",
        0x0126D314,
        0x01072214,
        ("Selector", "ItemsControl", "Control"),
    ),
    ("Control", ".?AVControl@Controls@Sqwt@@", 0x01269E70, 0x0106BB54, ()),
)
# class -> (arrange, measure): virtual slots 54 and 55 of the offset-0 vftable
LAYOUT = {
    "TabControl": (0x00960BF0, 0x00960A40),
    "TabPanel": (0x00960BF0, 0x00960A40),
    "TabItem": (0x00937480, 0x00937310),
    "TabItemBorder": (0x0099CA00, 0x0092A250),
    "SelectedContentPanel": (0x0092A300, 0x0092A250),
    "ScrollViewer": (0x00970DF0, 0x00970070),
    "ScrollContentPresenter": (0x009C2EC0, 0x009C2C90),
    "TextBoxBase": (0x00937480, 0x00937310),
    "TextBoxScrollViewer": (0x00970DF0, 0x00970070),
    "TextBox": (0x00957B80, 0x00937310),
    "LogControl": (0x00957B80, 0x00937310),
    "ContentControl": (0x00937480, 0x00937310),
    "ItemsControl": (0x00960BF0, 0x00960A40),
    "Selector": (0x00960BF0, 0x00960A40),
    "ListBox": (0x00960BF0, 0x00960A40),
    "Control": (0x00937480, 0x00937310),
}
ARRANGE_SLOT = 54
MEASURE_SLOT = 55
# name -> (VA, size) of the pinned functions and tables
PINS = {
    "Control measure": (0x937310, 361),
    "Control arrange": (0x937480, 317),
    "Control border thickness read": (0x937260, 164),
    "Control padding read": (0x9371B0, 164),
    "ScrollViewer measure": (0x970070, 241),
    "ScrollViewer arrange": (0x970DF0, 86),
    "ScrollContentPresenter measure": (0x9C2C90, 545),
    "ScrollContentPresenter arrange": (0x9C2EC0, 309),
    "ItemsControl measure": (0x960A40, 423),
    "ItemsControl arrange": (0x960BF0, 382),
    "ItemsControl item count": (0x960760, 77),
    "ItemsControl item at index": (0x961420, 90),
    "TabItem content accessor": (0x9BAA00, 7),
    "TextBox arrange": (0x957B80, 527),
    "TabControl scroll changed": (0x9BB320, 160),
    "TabControl items added": (0x9BBD10, 288),
    "TabControl selection changed": (0x9BBB00, 520),
    "TabControl constructor": (0x9BC270, 1703),
    "TabItem constructor": (0x9BB0F0, 419),
    "ScrollViewer constructor": (0x973190, 1229),
    "ItemsControl constructor": (0x9638C0, 835),
    "Selector constructor": (0x96F7B0, 310),
    "TextBox constructor": (0x95C300, 1489),
    "style part resolver": (0x933E70, 920),
    "ScrollBarVisibility parser": (0x914460, 146),
    "ScrollBarVisibility name": (0x914500, 37),
    "Visibility parser": (0x914580, 107),
    "Visibility name": (0x9145F0, 54),
    "Grid column setter": (0x976C80, 52),
    "Grid row setter": (0x976C40, 53),
    "Grid column span setter": (0x976D10, 80),
    "SelectedContentPanel slot 5": (0x9BB510, 3),
    "ScrollViewer scroll info update": (0x973660, 1594),
    "string compare entry": (0x9D244E, 80),
    "Visibility setter": (0x93C2A0, 46),
    "part table lookup": (0x8E0EB0, 131),
    "default part table": (0x12DCDE0, 3168),
    "ASCII case-folding compare": (0x9D2346, 53),
}
# (name, VA of the push, target VA, owner, role)
REFS = (
    (
        "TabControl ctor TabPanel key",
        0x9BC430,
        0x01081150,
        "TabControl",
        "Frame key of the TabControl",
    ),
    (
        "TabControl ctor LeftButton key",
        0x9BC681,
        0x0108118C,
        "TabControl",
        "Key of the RepeatButton at +0x4AC",
    ),
    (
        "TabControl ctor RightButton key",
        0x9BC701,
        0x010811A4,
        "TabControl",
        "Key of the RepeatButton at +0x860",
    ),
    (
        "TabControl ctor SelectedContentPanel name",
        0x9BC810,
        0x010811BC,
        "TabControl",
        "Name of the panel at +0x25A0",
    ),
    (
        "TabControl ctor SelectedContent key",
        0x9BC853,
        0x010811D8,
        "TabControl",
        "Frame key of the panel at +0x25A0",
    ),
    ("TabControl type name", 0x9BC3E8, 0x01081144, "TabControl", "Type name"),
    (
        "TabItem ctor Header key",
        0x9BB230,
        0x01080A7C,
        "TabItem",
        "Frame key of the TabItem",
    ),
    (
        "TabItemBorder ctor type name",
        0x9BB04D,
        0x01080A98,
        "TabItem",
        "Type name of the header border",
    ),
    ("ScrollViewer ctor type name", 0x973589, 0x0107302C, "ScrollViewer", "Type name"),
    (
        "ScrollViewer ctor Background key",
        0x9735D2,
        0x0107303C,
        "ScrollViewer",
        "Background key",
    ),
    (
        "ScrollViewer horizontal bar part name",
        0x970973,
        0x01073054,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer horizontal left part name",
        0x9709C8,
        0x01073070,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer horizontal right part name",
        0x970A21,
        0x01073080,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer horizontal track part name",
        0x970A7E,
        0x01073094,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer horizontal thumb part name",
        0x970AC8,
        0x010730A0,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer vertical bar part name",
        0x970B63,
        0x010730AC,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer vertical up part name",
        0x970BB8,
        0x010730C4,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer vertical down part name",
        0x970C11,
        0x010730D4,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer vertical track part name",
        0x970C6E,
        0x010730E4,
        "ScrollViewer",
        "Template part",
    ),
    (
        "ScrollViewer vertical thumb part name",
        0x970CB8,
        0x010730F0,
        "ScrollViewer",
        "Template part",
    ),
    ("TextBox ctor type name", 0x95C76E, 0x0106EBC4, "TextBox", "Type name"),
    ("TextBox ctor Background key", 0x95C7B6, 0x0106EBCC, "TextBox", "Background key"),
    (
        "TextBoxBase ctor content host name",
        0x97D59B,
        0x01074D90,
        "TextBoxBase",
        "Template part",
    ),
    (
        "TextBoxBase ctor text area name",
        0x97D5D5,
        0x01074DA4,
        "TextBoxBase",
        "Template part",
    ),
    (
        "TextBoxBase scroll viewer type name",
        0x97BB0A,
        0x01074D7C,
        "TextBoxBase",
        "Type name of the content host",
    ),
    (
        "TextBoxBase scroll info type name",
        0x97BC87,
        0x01074DB8,
        "TextBoxBase",
        "Type name of the scroll info",
    ),
    ("Caret type name", 0x97B988, 0x01074DCC, "TextBoxBase", "Type name of the caret"),
    (
        "Caret Background key",
        0x97B9BA,
        0x01074DD4,
        "TextBoxBase",
        "Background key of the caret",
    ),
    ("Caret part name", 0x97BA2E, 0x01074DE8, "TextBoxBase", "Template part"),
    ("LogControl ctor type name", 0x595EDB, 0x00FA8EBC, "LogControl", "Type name"),
    (
        "LogControl ctor Background key",
        0x595F25,
        0x00FA8EC8,
        "LogControl",
        "Background key",
    ),
    (
        "LogControl ctor wait cursor name first",
        0x59601E,
        0x00FA8EE0,
        "LogControl",
        "Template part",
    ),
    (
        "LogControl ctor wait cursor name second",
        0x59608D,
        0x00FA8EF0,
        "LogControl",
        "Template part",
    ),
    (
        "LogControl ctor WaitCursor key",
        0x5960AC,
        0x00FA8F00,
        "LogControl",
        "Key of the wait cursor",
    ),
    (
        "LogControl content host accessor",
        0x591410,
        0x00FA8F30,
        "LogControl",
        "Template part",
    ),
    (
        "LogControl horizontal bar accessor",
        0x59149E,
        0x00FA8F44,
        "LogControl",
        "Template part",
    ),
    (
        "LogControl vertical bar accessor",
        0x59153E,
        0x00FA8F60,
        "LogControl",
        "Template part",
    ),
)
PROPS = {
    "TabControl": ((0x01081120, "TabStripPlacement"), (0x01081134, "SelectedContent")),
    "TabItem": ((0x01080A54, "IsSelected"), (0x01080A60, "TabStripPlacement")),
    "Selector": ((0x0107263C, "SelectedIndex"), (0x0107264C, "SelectedItem")),
    "ItemsControl": ((0x0106F808, "MaxVisibleItems"), (0x0106F818, "MinVisibleItems")),
    "ScrollViewer": (
        (0x01072F0C, "CanContentScroll"),
        (0x01072F20, "ComputedHorizontalScrollBarVisibility"),
        (0x01072F48, "ComputedVerticalScrollBarVisibility"),
        (0x01072F6C, "ExtentHeight"),
        (0x01072F7C, "ExtentWidth"),
        (0x01072F88, "HorizontalOffset"),
        (0x01072F9C, "VerticalOffset"),
        (0x01072FAC, "HorizontalScrollBarVisibility"),
        (0x01072FCC, "VerticalScrollBarVisibility"),
        (0x01072FE8, "ScrollableHeight"),
        (0x01072FFC, "ScrollableWidth"),
        (0x0107300C, "ViewportHeight"),
        (0x0107301C, "ViewportWidth"),
    ),
    "ScrollContentPresenter": (
        (0x010827B0, "ViewportHeight"),
        (0x010827C0, "ViewportWidth"),
        (0x010827D0, "ExtentHeight"),
        (0x010827E0, "ExtentWidth"),
        (0x010827EC, "HorizontalOffset"),
        (0x01082800, "VerticalOffset"),
    ),
    "TextBoxBase": (
        (0x01074AD8, "IsReadOnly"),
        (0x01074AE4, "AcceptsReturn"),
        (0x01074AF4, "HorizontalScrollBarVisibility"),
        (0x01074B14, "VerticalScrollBarVisibility"),
        (0x01074B90, "TextChanged"),
    ),
    "TextBox": (
        (0x0106EAD4, "SelectionStart"),
        (0x0106EAE4, "SelectionLength"),
        (0x0106EAF4, "CaretIndex"),
        (0x0106EB00, "TextWrapping"),
        (0x0106EB10, "MaxLength"),
        (0x0106EB1C, "MinVisibleLines"),
        (0x0106EB2C, "MaxVisibleLines"),
        (0x0106EB3C, "MaxLines"),
        (0x0106EB48, "TextAlignment"),
        (0x0106EB58, "FirstVisibleLineIndex"),
        (0x0106EB70, "VisibleLines"),
        (0x0106EB80, "VerticalTextAlignment"),
        (0x0106EB98, "TextUI"),
        (0x0106EBA0, "IsReplaceUnaccept"),
        (0x0106EBB4, "IsValidFirst"),
    ),
    "LogControl": (
        (0x00FA8DE0, "LogChanged"),
        (0x00FA8DEC, "LinesPerSec"),
        (0x00FA8DF8, "LinesPerSecHi"),
        (0x00FA8E08, "LinesBeginHi"),
        (0x00FA8E18, "Categories"),
        (0x00FA8E24, "LogFormat"),
        (0x00FA8E30, "LogFormat2"),
        (0x00FA8E3C, "IndentHasName"),
        (0x00FA8E4C, "IndentNoName"),
        (0x00FA8E5C, "SignalText"),
        (0x00FA8E68, "SignalCommand"),
        (0x00FA8E78, "Action"),
        (0x00FA8E80, "Talker"),
        (0x00FA8E88, "LastCategory"),
        (0x00FA8E98, "IsEnableKeyWait"),
        (0x00FA8EA8, "IsDisplayLastOne"),
    ),
}
ENUM_STRINGS = (
    (0x010686D0, "Disabled"),
    (0x010686DC, "Hidden"),
    (0x010686E4, "Visible"),
    (0x010686EC, "AutoHidden"),
    (0x010686F8, "Auto"),
    (0x01068728, "Collapsed"),
    (0x01068734, "Hidden"),
    (0x0106873C, "False"),
    (0x01068744, "Visible"),
)
HOST_PATHS = (
    (0x00FA8A88, "PART_ContentHost:PART_HorizontalScrollBar:PART_RightButton"),
    (0x00FA8AF0, "PART_ContentHost:PART_HorizontalScrollBar:PART_LeftButton"),
    (0x00FA8B58, "PART_ContentHost:PART_HorizontalScrollBar:PART_Thumb"),
    (0x00FA8BC8, "PART_ContentHost:PART_HorizontalScrollBar:PART_Track"),
    (0x00FA8C34, "PART_ContentHost:PART_VerticalScrollBar:PART_DownButton"),
    (0x00FA8C94, "PART_ContentHost:PART_VerticalScrollBar:PART_UpButton"),
    (0x00FA8CF4, "PART_ContentHost:PART_VerticalScrollBar:PART_Thumb"),
    (0x00FA8D5C, "PART_ContentHost:PART_VerticalScrollBar:PART_Track"),
)
PART_TABLE = {"baseVa": 0x012DCDE0, "stride": 0x30, "keyOffset": 0x20, "count": 66}
PART_TABLE_KEYS = (
    "",
    "Window",
    "ScrollBar:LeftButton",
    "ScrollBar:RightButton",
    "ScrollBar:VerticalSlidingThumb",
    "ScrollBar:HorizontalSlidingThumb",
    "ScrollBar:UpButton",
    "ScrollBar:DownButton",
    "ScrollBar:VerticalBackground",
    "ScrollBar:HorizontalBackground",
    "TextBox:HorizontalScrollBar:LeftButton",
    "TextBox:HorizontalScrollBar:RightButton",
    "TextBox:HorizontalScrollBar:HorizontalSlidingThumb",
    "TextBox:HorizontalScrollBar:HorizontalBackground",
    "TextBox:VerticalScrollBar:VerticalSlidingThumb",
    "TextBox:VerticalScrollBar:UpButton",
    "TextBox:VerticalScrollBar:DownButton",
    "TextBox:VerticalScrollBar:VerticalBackground",
    "ListBox:HorizontalScrollBar:LeftButton",
    "ListBox:HorizontalScrollBar:RightButton",
    "ListBox:HorizontalScrollBar:HorizontalSlidingThumb",
    "ListBox:HorizontalScrollBar:HorizontalBackground",
    "ListBox:VerticalScrollBar:VerticalSlidingThumb",
    "ListBox:VerticalScrollBar:UpButton",
    "ListBox:VerticalScrollBar:DownButton",
    "ListBox:VerticalScrollBar:VerticalBackground",
    "ScrollViewer:HorizontalScrollBar:LeftButton",
    "ScrollViewer:HorizontalScrollBar:RightButton",
    "ScrollViewer:HorizontalScrollBar:HorizontalSlidingThumb",
    "ScrollViewer:HorizontalScrollBar:HorizontalBackground",
    "ScrollViewer:VerticalScrollBar:VerticalSlidingThumb",
    "ScrollViewer:VerticalScrollBar:UpButton",
    "ScrollViewer:VerticalScrollBar:DownButton",
    "ScrollViewer:VerticalScrollBar:VerticalBackground",
    "Button",
    "ComboBox:Background",
    "ComboBox:DownButton",
    "ListBox:Background",
    "RadioButton:UncheckedButton:Frame,CheckedButton:Frame",
    "CheckBox:UncheckedButton:Frame,CheckedButton:Frame",
    "ToggleButton:UncheckedButton:Frame,CheckedButton:Frame",
    "ComboBox:DownButton:UncheckedButton:Frame,DownButton:CheckedButton:Frame",
    "TextBox:Background",
    "ProgressBar:HorizontalBackground",
    "ProgressBar:VerticalBackground",
    "ProgressBar:HorizontalIndicator",
    "ProgressBar:VerticalIndicator",
    "ListBoxItem",
    "Slider:HorizontalBackground",
    "Slider:VerticalBackground",
    "Slider:HorizontalSlidingThumb",
    "Slider:VerticalSlidingThumb",
    "TabItem:Background_Header",
    "TabControl:Background_SelectedContent",
    "TabControl:Background_TabPanel",
    "RepeatButton",
    "LogControl:Background",
    "LogControl:WaitCursor",
    "LogControl:HorizontalScrollBar:LeftButton",
    "LogControl:HorizontalScrollBar:RightButton",
    "LogControl:HorizontalScrollBar:HorizontalSlidingThumb",
    "LogControl:HorizontalScrollBar:HorizontalBackground",
    "LogControl:VerticalScrollBar:UpButton",
    "LogControl:VerticalScrollBar:DownButton",
    "LogControl:VerticalScrollBar:VerticalSlidingThumb",
    "LogControl:VerticalScrollBar:VerticalBackground",
)
UNSET_ATTRIBUTES = {
    "TabControl": (
        "TabStripPlacement",
        "SelectedIndex",
        "MaxVisibleItems",
        "MinVisibleItems",
    ),
    "TabItem": ("TabStripPlacement", "IsSelected"),
    "TextBox": ("MinVisibleLines", "MaxVisibleLines"),
    "LogControl": ("MinVisibleLines", "MaxVisibleLines"),
}
PAGE_KEY_PREFIXES = (
    "TabControl:",
    "TabItem:",
    "TextBox:",
    "ScrollViewer:",
    "LogControl:",
    "ScrollBar:",
)
# name -> (VA, dwords)
DWORD_ROWS = {
    "MaxVisibleItems default constant": (0x0106F68C, (0x4B189680,)),
    "integer sentinel": (0x012E33AC, (0x7FFFFFFF,)),
    "half constant": (0x00F59898, (0x00000000, 0x3FE00000)),
}
# name -> (VA, length, pattern hex)
SIGNATURE_ROWS = {
    "MeasureCore calls virtual slot 55": (0x92BAD6, "8b92dc000000"),
    "arrange helper calls virtual slot 54": (0x92FFFC, "8b92d8000000"),
    "Selector SelectedIndex default is -1": (0x96F7E0, "33d2526aff6880b63501"),
    "TabStripPlacement default byte is 1": (0x9BC2E4, "c644243701"),
    "MaxVisibleItems stored at ItemsControl +0x370": (0x9652FA, "8d8e70030000"),
    "MinVisibleItems stored at ItemsControl +0x378": (0x965324, "8d8e78030000"),
    "Padding stored at Control +0x2a4": (0x93829D, "8d8ea4020000"),
    "BorderThickness stored at Control +0x2b8": (0x9382D8, "8d8eb8020000"),
    "Visibility stored at FrameworkElement +0x158": (
        0x93C897,
        "68bc96350152b800b79300508d8e5801000056",
    ),
    "ScrollViewer HorizontalScrollBarVisibility default is 1": (
        0x973341,
        "68a4bc350153b860269700508d8eb003000056c644242e01",
    ),
    "ScrollViewer VerticalScrollBarVisibility default is 3": (
        0x973364,
        "686cbe350153b860269700508d8eb203000056c644242e03",
    ),
}
STATIC_KINDS = {"signature", "dwords", "string", "rangeSha256", "rtti", "recordKeys"}
FRAME_COLUMNS = ("SkinNormal", "SkinActive", "SkinHover", "SkinReserved")


def class_names() -> list[str]:
    return [item[0] for item in CLASSES]


def expected_check_names() -> set[str]:
    names = set(SIGNATURE_ROWS) | set(DWORD_ROWS) | {"default part table keys"}
    for key, *_ in CLASSES:
        names |= {f"rtti name {key}", f"rtti bases {key}", f"layout slots {key}"}
    names |= {f"range {name}" for name in PINS}
    for name, _, target, *_ in REFS:
        names |= {name, f"string {name}"}
    for owner, items in PROPS.items():
        names |= {f"property {owner} {text}" for _, text in items}
    names |= {f"enum string 0x{va:08x} {text}" for va, text in ENUM_STRINGS}
    names |= {
        "LogControl content host path " + text.split(":", 1)[1]
        for _, text in HOST_PATHS
    }
    return names


# ---- reference models of the rules on the page


def thickness_sum(border, padding) -> tuple[float, float]:
    """Horizontal and vertical inset: (left, top, right, bottom) of border plus padding."""
    total = [b + p for b, p in zip(border, padding)]
    return total[0] + total[2], total[1] + total[3]


def control_measure(available, border, padding, children) -> tuple[float, float]:
    """Model of 0x00937310: each child sees the available size less the inset sum."""
    inset_x, inset_y = thickness_sum(border, padding)
    inner = (available[0] - inset_x, available[1] - inset_y)
    width = height = 0.0
    for measure in children:
        child = measure(inner)
        width = max(width, child[0])
        height = max(height, child[1])
    return width + inset_x, height + inset_y


def control_arrange(final, border, padding) -> tuple[float, float, float, float]:
    """Model of 0x00937480: the rectangle every child receives, as x, y, width, height."""
    total = [b + p for b, p in zip(border, padding)]
    return (
        total[0],
        total[1],
        final[0] - (total[2] + total[0]),
        final[1] - (total[3] + total[1]),
    )


def scrollviewer_measure(available, border, padding, children) -> tuple[float, float]:
    """Model of 0x00970070: the inset sum is applied around the Control measure as well."""
    inset_x, inset_y = thickness_sum(border, padding)
    inner = (available[0] - inset_x, available[1] - inset_y)
    width, height = control_measure(inner, border, padding, children)
    return width + inset_x, height + inset_y


def presenter_measure(available, child_measure):
    """Model of 0x009C2C90: returns (desired, extent) for the content presenter."""
    child = child_measure(available)
    if child[0] <= available[0] and child[1] <= available[1]:
        return child, child
    again = (max(child[0], available[0]), max(child[1], available[1]))
    second = child_measure(again)
    return second, second


def presenter_arrange(final, desired):
    """Model of 0x009C2EC0: returns (child size, viewport)."""
    return (max(final[0], desired[0]), max(final[1], desired[1])), final


def items_guard(max_items: float, min_items: float) -> bool:
    """The ItemsControl size adjustment runs only off the default limits."""
    return max_items != UNLIMITED_ITEMS or min_items != 0.0


def tab_arrows(
    extent: float, viewport: float, offset: float
) -> tuple[bool, bool, bool]:
    """Model of 0x009BB320: (buttons visible, left enabled, right enabled)."""
    return viewport < extent, offset != 0.0, offset + viewport < extent


def items_added(
    selected_index: int, visibilities: list[int], in_panel: bool = False
) -> tuple[int, bool, bool]:
    """Model of 0x009BBD10 for an add of the last item: (selection left, content appended, content Hidden).

    visibilities are the Visibility bytes of the TabItems, last one just added. in_panel
    says the content element already has the SelectedContentPanel as its parent.
    """
    if not visibilities:
        return selected_index, False, False
    if selected_index == -1:
        if visibilities[-1] == VISIBILITY["Visible"]:
            return len(visibilities) - 1, False, False
        return selected_index, False, False
    if in_panel:
        return selected_index, False, False
    return selected_index, True, True


def textbox_rect(
    final,
    border,
    padding,
    min_lines,
    max_lines,
    line_height,
    extra,
    stretch,
    host_a,
    host_b,
):
    """Model of 0x00957B80: the child rectangle as x, y, width, height."""
    height = final[1]
    if stretch:
        cap = (
            UNLIMITED_ITEMS
            if max_lines == UNLIMITED_LINES
            else max_lines * line_height - extra
        )
        if final[1] > cap:
            height = cap - host_a + host_b
    floor = min_lines * line_height - extra
    if height < floor:
        height = floor - host_a + host_b
    total = [b + p for b, p in zip(border, padding)]
    return (
        total[0],
        total[1] + (final[1] - height) * 0.5,
        final[0] - (total[2] + total[0]),
        height - (total[3] + total[1]),
    )


ASCII_FOLD = {code: code + 0x20 for code in range(0x41, 0x5B)}


def ascii_lower(text: str) -> str:
    """The fold of 0x009D2346: only A to Z change."""
    return text.translate(ASCII_FOLD)


def scrollbar_visibility(text: str) -> int:
    """Model of 0x00914460: ASCII case-insensitive names, anything else is 0 (Auto)."""
    for name, value in SCROLLBAR_VISIBILITY.items():
        if name != "Auto" and ascii_lower(text) == ascii_lower(name):
            return value
    return 0


def scrollbar_visibility_name(value: int) -> str:
    """Model of 0x00914500."""
    for name, number in SCROLLBAR_VISIBILITY.items():
        if number == value and name != "Auto":
            return name
    return "Auto"


def visibility(text: str) -> int:
    """Model of 0x00914580: Collapsed and False give 0, Hidden gives 1, the rest 2."""
    lowered = ascii_lower(text)
    if lowered in ("collapsed", "false"):
        return 0
    if lowered == "hidden":
        return 1
    return 2


def computed_visibility(mode: int, extent: float, viewport: float) -> int:
    """Model of the bar visibility in 0x00973660: 0 Collapsed, 1 Hidden, 2 Visible."""
    if mode == 0:
        return 2 if extent > viewport else 0
    if mode == 1:
        return 0
    if mode == 2:
        return 1
    if mode == 4:
        return 2 if extent > viewport else 1
    return 2


def scrollable(extent: float, viewport: float) -> float:
    """ScrollableWidth and ScrollableHeight as 0x00973660 stores them."""
    return extent - viewport if extent > viewport else 0.0


def frame_slots(frame: dict[str, str]) -> list[str | None]:
    """Model of 0x00933E70: the seven slots with the Reserved and Anchored fallbacks."""
    slots: list[str | None] = [frame.get(name) for name in CONDITIONS]
    if slots[5] is None:
        slots[5] = slots[1]
    if slots[6] is None:
        slots[6] = slots[3]
    return slots


# ---- page rows


def upper_hex(text: str) -> str:
    return "0x" + text[2:].upper()


def chain_text(key: str) -> str:
    for name, _, _, _, chain in CLASSES:
        if name == key:
            return ", ".join(chain) or "none"
    return ""


def table_rows(document: dict) -> dict[str, list[str]]:
    """Table rows of the page that restate manifest values, by table name."""
    tables: dict[str, list[str]] = {}
    tables["classes"] = [
        "| {0} | {1} | {2} | {3} | {4} |".format(
            item["class"],
            chain_text(item["class"]),
            upper_hex(item["vftable"]),
            upper_hex(item["arrange"]),
            upper_hex(item["measure"]),
        )
        for item in document.get("classes", [])
    ]
    texts = {
        row.get("name"): row.get("text", "") for row in document.get("staticChecks", [])
    }
    tables["references"] = [
        f"| {owner} | {texts.get('string ' + name, '')} | {role} "
        f"| {upper_hex('0x%08x' % target)} | {upper_hex('0x%08x' % va)} |"
        for name, va, target, owner, role in REFS
    ]
    tables["properties"] = [
        f"| {owner} | " + ", ".join(text for _, text in items) + " |"
        for owner, items in PROPS.items()
    ]
    tables["scrollbarVisibility"] = [
        f"| {text} | {value} |" for text, value in SCROLLBAR_VISIBILITY.items()
    ]
    tables["visibility"] = [
        f"| {text} | {value} |" for text, value in VISIBILITY.items()
    ]
    census = document.get("census", {})
    tables["census"] = []
    for kind in TYPES:
        entry = census.get("elements", {}).get(kind)
        if entry:
            resolution = census["styleResolution"][kind]
            tables["census"].append(
                f"| {kind} | {entry['elements']} | {entry['files']} "
                f"| {len(entry['attributes'])} | {resolution['distinct']} "
                f"| {resolution['defined']} |"
            )
    log = census.get("logWidget", {})
    tables["logElements"] = [
        f"| {kind} | {log['elements'][kind]} | {', '.join(log['styles'][kind])} "
        f"| {', '.join(log['attributes'][kind])} |"
        for kind in TYPES
        if kind in log.get("elements", {})
    ]
    tables["logTargets"] = [
        f"| {path} | {count} |" for path, count in log.get("targets", {}).items()
    ]
    tables["logParts"] = [
        "| " + " | ".join(part) + " |" for part in log.get("parts", [])
    ]
    tables["logFrames"] = [
        f"| {frame[0]} | {frame[1]} | "
        + " | ".join(dict(zip(CONDITIONS, frame[3:]))[name] for name in FRAME_COLUMNS)
        + " |"
        for frame in log.get("frames", [])
    ]
    shadow = log.get("shadowing", {})
    tables["logShadowing"] = (
        [
            f"| {shadow['leafStyles']} | {shadow['fewestFiles']} "
            f"| {shadow['mostFiles']} | {shadow['differing']} |"
        ]
        if shadow
        else []
    )
    tables["partRecords"] = (
        [
            f"| {index} | {key} |"
            for index, key in enumerate(PART_TABLE_KEYS)
            if key.startswith(PAGE_KEY_PREFIXES)
        ]
        if document.get("partTable")
        else []
    )
    return tables


def page_rows(document: dict) -> list[str]:
    return [row for rows in table_rows(document).values() for row in rows]


def flatten(text: str) -> str:
    """Drop code ticks and collapse whitespace so a page may wrap and format a line."""
    return " ".join(text.replace("`", "").split())


@functools.lru_cache(maxsize=4)
def flat_lower(text: str) -> str:
    return flatten(text).lower()


def doc_errors(document: dict, text: str | None = None) -> list[str]:
    if text is None:
        if not DOC.exists():
            return ["page is missing"]
        text = DOC.read_text(encoding="ascii")
    errors: list[str] = []
    flat = flat_lower(text)
    for line in document.get("contract", []):
        if flatten(line).lower() not in flat:
            errors.append(f"page lacks contract line {line[:40]!r}")
    for row in page_rows(document):
        if row.lower() not in flat:
            errors.append(f"page lacks table row {row[:60]!r}")
    for label in sorted(LABELS):
        if label not in text:
            errors.append(f"page lacks the {label} label")
    return errors


# ---- manifest verification


def parse_hex(text: str) -> int:
    return int(text, 16)


def static_check_errors(document: dict) -> list[str]:
    errors: list[str] = []
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        return ["staticChecks is not a list"]
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    expected = expected_check_names()
    if expected - set(names):
        errors.append(f"staticChecks lacks {sorted(expected - set(names))[:3]}")
    if set(names) - expected:
        errors.append(
            f"staticChecks holds unexpected {sorted(set(names) - expected)[:3]}"
        )
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
    for key, mangled, name_va, vftable, chain in CLASSES:
        row = by_name.get(f"rtti name {key}", {})
        if row.get("text") != mangled or row.get("va") != f"0x{name_va:08x}":
            errors.append(f"rtti name {key} changed")
        row = by_name.get(f"rtti bases {key}", {})
        bases = row.get("bases", [])
        wanted = (
            [mangled] + [f".?AV{name}@" for name in chain] + [".?AVFrameworkElement@"]
        )
        if (
            row.get("va") != f"0x{vftable:08x}"
            or row.get("nameVa") != f"0x{name_va:08x}"
            or len(bases) < len(wanted)
            or bases[0] != mangled
            or any(not bases[i].startswith(wanted[i]) for i in range(1, len(wanted)))
        ):
            errors.append(f"rtti bases {key} changed")
        arrange, measure = LAYOUT[key]
        row = by_name.get(f"layout slots {key}", {})
        if row.get("va") != f"0x{vftable + 4 * ARRANGE_SLOT:08x}" or row.get(
            "values"
        ) != [f"0x{arrange:08x}", f"0x{measure:08x}"]:
            errors.append(f"layout slots {key} changed")
    for name, (va, size) in PINS.items():
        row = by_name.get(f"range {name}", {})
        if row.get("va") != f"0x{va:08x}" or row.get("length") != size:
            errors.append(f"range {name} changed")
    for name, va, target, *_ in REFS:
        row = by_name.get(name, {})
        wanted = "68" + target.to_bytes(4, "little").hex()
        if (
            row.get("va") != f"0x{va:08x}"
            or row.get("pattern") != wanted
            or row.get("mask") != "ff" * 5
        ):
            errors.append(f"reference {name} changed")
        row = by_name.get(f"string {name}", {})
        if row.get("va") != f"0x{target:08x}" or not row.get("text"):
            errors.append(f"string {name} changed")
    for owner, items in PROPS.items():
        for va, text in items:
            row = by_name.get(f"property {owner} {text}", {})
            if row.get("va") != f"0x{va:08x}" or row.get("text") != text:
                errors.append(f"property {owner} {text} changed")
    for va, text in ENUM_STRINGS:
        row = by_name.get(f"enum string 0x{va:08x} {text}", {})
        if row.get("va") != f"0x{va:08x}" or row.get("text") != text:
            errors.append(f"enum string {text} changed")
    for va, text in HOST_PATHS:
        row = by_name.get("LogControl content host path " + text.split(":", 1)[1], {})
        if row.get("va") != f"0x{va:08x}" or row.get("text") != text:
            errors.append(f"content host path {text} changed")
    for name, (va, values) in DWORD_ROWS.items():
        row = by_name.get(name, {})
        if row.get("va") != f"0x{va:08x}" or row.get("values") != [
            f"0x{v:08x}" for v in values
        ]:
            errors.append(f"dword row {name} changed")
    for name, (va, pattern) in SIGNATURE_ROWS.items():
        row = by_name.get(name, {})
        if (
            row.get("va") != f"0x{va:08x}"
            or row.get("pattern") != pattern
            or row.get("mask") != "ff" * (len(pattern) // 2)
        ):
            errors.append(f"signature row {name} changed")
    row = by_name.get("default part table keys", {})
    if (
        row.get("va") != f"0x{PART_TABLE['baseVa']:08x}"
        or row.get("stride") != PART_TABLE["stride"]
        or row.get("keyOffset") != PART_TABLE["keyOffset"]
        or row.get("count") != PART_TABLE["count"]
        or row.get("keys") != list(PART_TABLE_KEYS)
    ):
        errors.append("default part table keys changed")
    return errors


def classes_errors(document: dict) -> list[str]:
    found = document.get("classes", [])
    expected = [
        {
            "class": key,
            "typeName": mangled,
            "vftable": f"0x{vftable:08x}",
            "arrange": f"0x{LAYOUT[key][0]:08x}",
            "measure": f"0x{LAYOUT[key][1]:08x}",
        }
        for key, mangled, _, vftable, _ in CLASSES
    ]
    return [] if found == expected else ["classes changed"]


def census_errors(census: dict) -> list[str]:
    errors: list[str] = []
    elements = census.get("elements", {})
    if list(elements) != list(TYPES):
        return ["census types changed"]
    for kind, entry in elements.items():
        if entry["files"] > entry["elements"] or not entry["elements"]:
            errors.append(f"census {kind} counts are inconsistent")
        for name in entry["styles"]:
            if not name:
                errors.append(f"census {kind} has an empty style")
        if sum(entry["styles"].values()) > entry["elements"]:
            errors.append(f"census {kind} styles exceed the elements")
        if any(count > entry["elements"] for count in entry["attributes"].values()):
            errors.append(f"census {kind} attribute counts exceed the elements")
        for name in UNSET_ATTRIBUTES.get(kind, ()):
            if name in entry["attributes"]:
                errors.append(f"census {kind} sets {name}")
        resolution = census["styleResolution"][kind]
        if (
            resolution["distinct"] != len(entry["styles"])
            or resolution["defined"] > resolution["distinct"]
        ):
            errors.append(f"census {kind} style resolution is inconsistent")
    corpus = census.get("corpus", {})
    if corpus.get("unparsed", 0) > corpus.get("forms", 0) + corpus.get("templates", 0):
        errors.append("census unparsed count is inconsistent")
    log = census.get("logWidget", {})
    for kind in TYPES:
        if log.get("elements", {}).get(kind, 0) < 1:
            errors.append(f"LogWidget census lacks {kind}")
        if len(log.get("styles", {}).get(kind, [])) != 1:
            errors.append(f"LogWidget census style count for {kind} changed")
    shadow = log.get("shadowing", {})
    if (
        not 1 <= shadow.get("fewestFiles", 0) <= shadow.get("mostFiles", 0)
        or shadow.get("leafStyles", 0) < 1
        or not 0 <= shadow.get("differing", -1) <= shadow.get("leafStyles", 0)
    ):
        errors.append("LogWidget shadowing counts changed")
    targets = log.get("targets", {})
    if not targets or any(
        ":" not in path or count < 1 for path, count in targets.items()
    ):
        errors.append("LogWidget targets changed")
    for frame in log.get("frames", []):
        if len(frame) != 3 + len(CONDITIONS):
            errors.append("LogWidget frame row width changed")
    distinct = {
        s for row in log.get("frames", []) for s in row[3:] if s and s != "NULL"
    }
    if log.get("skinNames") != len(distinct):
        errors.append("LogWidget skin name count disagrees with the frames")
    if sum(log.get("skinKinds", {}).values()) != len(distinct) or "missing" in log.get(
        "skinKinds", {}
    ):
        errors.append("LogWidget skin kinds disagree with the frames")
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
        or source.get("build") != "2012.09.19.0001"
        or source.get("gameVerSha256")
        != "3dbeed87ae1f2805bec69d63137e3deca9dd444ad5caa3ea5dedb68b8a50bfef"
    ):
        errors.append("source identity changed")
    errors.extend(classes_errors(document))
    table = document.get("partTable", {})
    if table != {
        "baseVa": f"0x{PART_TABLE['baseVa']:08x}",
        "stride": hex(PART_TABLE["stride"]),
        "keyOffset": hex(PART_TABLE["keyOffset"]),
        "count": PART_TABLE["count"],
    }:
        errors.append("partTable changed")
    errors.extend(census_errors(document.get("census", {})))
    contract = document.get("contract", [])
    if len(contract) != len(set(contract)) or not contract:
        errors.append("contract lines are repeated or missing")
    if not all(any(label in line for label in LABELS) for line in contract):
        errors.append("a contract line lacks a label")
    questions = document.get("open", [])
    if not questions or not all(
        item.get("label") in LABELS - {"STATIC"} for item in questions
    ):
        errors.append("open questions changed")
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


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    mutations: list[dict] = []
    for path, value in [
        (("source", "binarySha256"), "0" * 64),
        (("source", "size"), 1),
        (("classes", 0, "arrange"), "0x00960bf1"),
        (("classes", 5, "measure"), "0x00970071"),
        (("classes", 9, "vftable"), "0x00fa935d"),
        (("partTable", "count"), 65),
        (("census", "elements", "TabControl", "elements"), 26),
        (("census", "elements", "TextBox", "files"), 53),
        (("census", "styleResolution", "TabItem", "defined"), 3),
        (("census", "logWidget", "elements", "TabItem"), 1),
        (("census", "logWidget", "skinNames"), 37),
        (("census", "logWidget", "skinKinds", "CellSkins"), 36),
        (("census", "logWidget", "frames", 3, 3), "TBI_basis_header_A"),
        (("census", "logWidget", "shadowing", "differing"), 8),
        (("census", "logWidget", "shadowing", "fewestFiles"), 9),
        (("census", "logWidget", "parts", 0, 4), "TBC_chat"),
        (("contract", 0), "A part key is never resolved through a style file."),
        (("open", 0, "label"), "STATIC"),
    ]:
        changed = copy.deepcopy(document)
        cursor = changed
        for key in path[:-1]:
            cursor = cursor[key]
        cursor[path[-1]] = value
        mutations.append(changed)
    for index, item in enumerate(document["staticChecks"]):
        changed = copy.deepcopy(document)
        changed["staticChecks"].pop(index)
        mutations.append(changed)
    for index, item in enumerate(document["staticChecks"]):
        if item["kind"] in {"string", "dwords", "signature"}:
            changed = copy.deepcopy(document)
            row = changed["staticChecks"][index]
            if item["kind"] == "string":
                row["va"] = "0x%08x" % (parse_hex(item["va"]) + 1)
            elif item["kind"] == "dwords":
                row["values"][0] = "0x%08x" % (parse_hex(item["values"][0]) ^ 1)
            else:
                row["va"] = "0x%08x" % (parse_hex(item["va"]) + 1)
            mutations.append(changed)
    for index, item in enumerate(document["staticChecks"]):
        if item["kind"] == "rtti":
            changed = copy.deepcopy(document)
            changed["staticChecks"][index]["bases"][1] = ".?AVWrong@Sqwt@@"
            mutations.append(changed)
    leaked = copy.deepcopy(document)
    leaked["source"]["install"] = "agent-islands/private-evidence"
    mutations.append(leaked)
    page = DOC.read_text(encoding="ascii") if DOC.exists() else ""
    results = [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item, page)
    ]
    if verify(document, "") == []:
        results.append("an empty page was accepted")
    return results


# ---- executable verification


def read_cstring(data: bytes, sections: list, va: int) -> str | None:
    out = bytearray()
    while len(out) < 512:
        byte = image_checks.read_va(data, sections, va + len(out), 1)
        if byte is None:
            return None
        if byte == b"\0":
            return out.decode("latin-1")
        out += byte
    return None


def read_u32(data: bytes, sections: list, va: int) -> int | None:
    raw = image_checks.read_va(data, sections, va, 4)
    return None if raw is None else struct.unpack("<I", raw)[0]


def rtti_names(data: bytes, sections: list, vftable: int) -> list[str] | None:
    """Mangled names of the base class array reached from the vftable's locator."""
    locator = read_u32(data, sections, vftable - 4)
    if locator is None:
        return None
    hierarchy = read_u32(data, sections, locator + 16)
    if hierarchy is None:
        return None
    count = read_u32(data, sections, hierarchy + 8)
    array = read_u32(data, sections, hierarchy + 12)
    if count is None or array is None or count > 64:
        return None
    names: list[str] = []
    for index in range(count):
        entry = read_u32(data, sections, array + 4 * index)
        descriptor = None if entry is None else read_u32(data, sections, entry)
        name = (
            None if descriptor is None else read_cstring(data, sections, descriptor + 8)
        )
        if name is None:
            return None
        names.append(name)
    return names


def run_check(data: bytes, sections: list, check: dict) -> str | None:
    kind = check["kind"]
    va = parse_hex(check["va"])
    if kind == "rangeSha256":
        raw = image_checks.read_va(data, sections, va, check["length"])
        if raw is None:
            return f"{check['va']} is outside the image"
        if hashlib.sha256(raw).hexdigest() != check["sha256"]:
            return f"range {check['name']!r} at {check['va']} differs"
        return None
    if kind == "rtti":
        if rtti_names(data, sections, va) != check["bases"]:
            return f"rtti bases at {check['va']} differ"
        return None
    if kind == "recordKeys":
        for index, key in enumerate(check["keys"]):
            pointer = read_u32(
                data, sections, va + check["stride"] * index + check["keyOffset"]
            )
            found = None if pointer is None else read_cstring(data, sections, pointer)
            if found != key:
                return f"part table record {index} is {found!r}, expected {key!r}"
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
        sections = image_checks.section_map(data)
    except (ValueError, struct.error) as exc:
        return errors + [f"image is not a readable PE32: {exc}"]
    for check in document["staticChecks"]:
        problem = run_check(data, sections, check)
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
        print(f"FAIL: {len(errors)} sqwt tab scroll text error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: sqwt tab scroll text manifest and mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
