#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C)  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify the LogControl key-wait, paging and speaker-name facts in ffxivgame.exe.

Eight check groups over the pinned executable, plus an optional install check:

1. Instruction listings at the command fan-out, the LogControl command handler,
   the LogDocument wait release and update producers, the property setters and
   the display-name lookup recorded in docs/resource/logcontrol-key-wait.md,
   docs/resource/logcontrol-talker-name.md and docs/event/npcsay-window-chain.md.
2. The command name literals, their command objects and their initializers.
3. The property and event name literals.
4. The direct E8/E9 caller sets of the entries those pages rely on.
5. The LogDocument interface, container handler and SetDisplayNameReceiver
   table slots.
6. The `xtx/_text_ui` literal used by the display-name request.
7. The data-reference sets and constants behind the wait-weight, page-row and
   scroll-pacing pages: command object, RTTI descriptor and vtable references,
   float and double operands, the rich-string decoder handler vector, and the
   store sites of the FontAttribute family.
8. The event source gate that decides whether the Wait0 callback runs.

With --install and --blowfish the form attributes of NpcSayWidget and
LogWidget in docs/resource/logcontrol-key-wait.md are checked as well.

The module also holds reference models of the container fan-out, the key-wait
arming and release, the page row count, the scroll pacing and the boolean
property byte, so the tests state their expected results without running the
code they check.
"""

from __future__ import annotations

import argparse
import importlib.util
import re
import struct
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import NamedTuple, Sequence

import extract_class_registrations as ecr

PRESS_KEY = "RaptureCommands.LogControlPressKey"
PRESS_MOUSE = "RaptureCommands.LogControlPressMouse"
PRESS_CANCEL = "RaptureCommands.LogControlPressKeyCancel"
WAIT0 = "RaptureCommands.LogControlWait0"
CONTROL_PRESS_KEY = "LogControl.PressKey"
CONTROL_PRESS_CANCEL = "LogControl.PressKeyCancel"
CONTROL_PRESSED_KEY = "LogControl.PressedKey"

NO_DISPLAY_NAME = 0xFFFFFFFF
TEXT_UI_SHEET = "xtx/_text_ui"
TEXT_UI_ROW = 0xDE


def container_fanout(command: str, controls: Sequence[str]) -> list[tuple[str, str]]:
    """Reference model of the container handler 0x004D9D70.

    Returns (command, target) pairs. Wait0 acts on the DesktopWidget flag and
    has no control target. Other commands do nothing here.
    """
    if command in (PRESS_KEY, PRESS_MOUSE):
        return [(CONTROL_PRESS_KEY, control) for control in controls]
    if command == PRESS_CANCEL:
        return [(CONTROL_PRESS_CANCEL, control) for control in controls]
    if command == WAIT0:
        return [("DesktopWidget.messageWaitFlag", "set")]
    return []


class Release(NamedTuple):
    consumed: bool
    pending: int
    weight: int
    page_flag: bool
    raises: bool


def release_wait(
    pending: int,
    weight: int,
    line_weight: int,
    page_flag: bool,
    row_in_range: bool = True,
    row_found: bool = True,
) -> Release:
    """Reference model of one LogDocument key release (0x00590BB0).

    `pending` is the counter at I+0x134, `weight` the wait weight at I+0x130,
    and `line_weight` and `page_flag` the LogAttribute fields +0x4C and +0x90
    of the first line that carries either. Wait0 and PressedKey are raised only
    when a wait weight was released and no wait weight remains. When the
    current row is not below the row count (0x00590C12) or a row lookup is null
    (0x00590DA3), the wait fields are zeroed and nothing is raised or consumed.
    """
    if pending == 0:
        return Release(False, pending, weight, page_flag, False)
    if not row_in_range or not row_found:
        return Release(False, 0, 0, page_flag, False)
    released = 0
    if line_weight:
        weight -= line_weight
        pending -= line_weight
        released = line_weight
    if page_flag:
        pending -= 1
        page_flag = False
    return Release(True, pending, weight, page_flag, released != 0 and weight == 0)


class Armed(NamedTuple):
    pending: int
    weight: int
    last_weight_row: int
    page_breaks: tuple[int, ...]
    next_row: int


def arm_wait(weights: Sequence[int], page_rows: int, first_row: int = 0) -> Armed:
    """Reference model of the arming loop in update slot 2 (0x00595070..0x00595106).

    `weights` are the LogAttribute+0x4C values of the rows, and the loop starts
    at first_row with a zero counter. A weighted row adds its weight to the
    pending counter. A zero-weight row other than the last gets a page break
    when it sits on the page boundary, which starts at first_row + page_rows - 1
    and advances by page_rows. A row that arms nothing while the counter is
    zero moves the current row on.
    """
    pending = weight = 0
    last = -1
    breaks: list[int] = []
    boundary = first_row + page_rows - 1
    current = first_row
    for row in range(first_row, len(weights)):
        if weights[row]:
            pending += weights[row]
            last = row
            weight += weights[row]
        elif row + 1 != len(weights) and row == boundary:
            boundary += page_rows
            breaks.append(row)
            pending += 1
        elif pending == 0:
            current += 1
    return Armed(pending, weight, last, tuple(breaks), current)


def visible_lines(spacing: int, height: float, text_height: float) -> int:
    """VisibleLines (control +0x179C) as computed at 0x00959BF4.

    The line height at +0x1964 is the measured text height plus the spacing
    integer reached through +0x17F0 (0x0095BEE2). The count truncates and is
    at least 1.
    """
    return max(1, int((spacing + height) / (text_height + spacing)))


def scroll_speed(
    per_sec: float,
    per_sec_hi: float,
    begin_hi: int,
    backlog_rows: int,
    hi_active: bool,
) -> float:
    """Speed operand of the per-frame update (0x00595704, 0x0059576B).

    While the high-speed marker at +0x1F04 is set and the view has not reached
    it, the speed is LinesPerSecHi times 3.0. Otherwise a backlog of at least
    LinesBeginHi rows selects LinesPerSecHi times 5.0, and a shorter backlog
    keeps LinesPerSec.
    """
    if hi_active:
        return per_sec_hi * 3.0
    if backlog_rows >= begin_hi:
        return per_sec_hi * 5.0
    return per_sec


def scroll_step(line_height: float, speed: float, display_last_one: bool) -> float:
    """Offset added per update call at 0x005957C3..0x00595800.

    The value returned by the timer helper is not an operand of the step.
    IsDisplayLastOne halves it.
    """
    step = line_height * 100.0 * speed / 5000.0
    return step * 0.5 if display_last_one else step


def property_bool(byte: int) -> bool:
    """0x0093E220: the boolean value of a property byte is bit 5."""
    return (byte >> 5) & 1 == 1


def display_name_request(display_name_id: int) -> tuple[str, int, int] | None:
    """The text request 0x0076FD80 makes for a name id, or None for the custom
    string sentinel, which is stored without a lookup."""
    if display_name_id == NO_DISPLAY_NAME:
        return None
    return (TEXT_UI_SHEET, TEXT_UI_ROW, display_name_id)


# label, VA, expected listing
CHECKS: tuple[tuple[str, int, str], ...] = (
    (
        "container_wait0",
        0x004D9E8F,
        "cmp eax, 0x133a438; jne 0x4d9eb2; cmp dword ptr [edi + 8], 0; je 0x4d9ea9; push 0; lea ecx, [esi + 0x45c]; call 0x575900; mov byte ptr [edi + 0x19], 1",
    ),
    (
        "container_press_fanout",
        0x004D9EB2,
        "cmp eax, 0x133a4d8; je 0x4da015; cmp eax, 0x133a578; je 0x4da015; cmp eax, 0x133a618; jne 0x4d9f90",
    ),
    (
        "container_cancel_exec",
        0x004D9F3B,
        "push 0; push eax; lea edx, [esp + 0x30]; push edx; mov ecx, 0x133c1f8; call 0x927fa0",
    ),
    (
        "container_press_exec",
        0x004DA0AA,
        "push 0; push eax; lea ecx, [esp + 0x30]; push ecx; mov ecx, 0x133c158; call 0x927fa0",
    ),
    (
        "container_registry_read",
        0x004DA03D,
        "lea ecx, [esi + 0x17794]; push eax; call 0x6cfbb0; mov ebx, dword ptr [esp + 0x18]",
    ),
    (
        "container_register",
        0x004DBAD0,
        "mov eax, dword ptr [esp + 4]; lea edx, [esp + 4]; push edx; add ecx, 0x17848; mov dword ptr [esp + 8], eax; call 0x813050; ret 4",
    ),
    (
        "logcontrol_register",
        0x00595FA9,
        "mov ecx, dword ptr [esi + 0x1b08]; push esi; call 0x4dbad0; push edi",
    ),
    (
        "logcontrol_unregister",
        0x00591D6E,
        "mov ecx, dword ptr [esi + 0x1b08]; push esi; call 0x4d9b30; mov al, 0xd",
    ),
    (
        "logcontrol_cancel_branch",
        0x0059593B,
        "mov eax, dword ptr [ebp + 0x1c]; cmp eax, 0x133c1f8; jne 0x595996; mov edx, dword ptr [edi + 0x1a50]",
    ),
    (
        "logcontrol_press_branch",
        0x00595996,
        "cmp eax, 0x133c158; jne 0x5959a9; push ebp; lea ecx, [edi - 0xb4]; call 0x591180; cmp dword ptr [ebp + 0x1c], 0x1357c88",
    ),
    (
        "logcontrol_operate_branch",
        0x005959A9,
        "cmp dword ptr [ebp + 0x1c], 0x1357c88; jne 0x5959f7; mov edi, dword ptr [edi + 0x1a54]; cmp edi, esi",
    ),
    (
        "logcontrol_operate_exec",
        0x005959D6,
        "push esi; push edi; push eax; mov ecx, 0x133a4d8",
    ),
    (
        "press_gate",
        0x00591180,
        "push esi; mov esi, ecx; cmp dword ptr [esi + 0x1b0c], 0; push edi; je 0x591217; mov edi, dword ptr [esp + 0xc]",
    ),
    (
        "press_release_call",
        0x005911A2,
        "push edx; mov edx, dword ptr [esi + 0x179c]; lea ecx, [eax + 0x28]; mov eax, dword ptr [ecx]; mov eax, dword ptr [eax + 0xc]; push edx; mov edx, dword ptr [esi + 0x1f00]; push edx",
    ),
    (
        "press_result",
        0x005911B9,
        "call eax; cmp al, 1; jne 0x591217; mov ecx, esi; call 0x5910f0",
    ),
    (
        "key_gate",
        0x00590BFA,
        "cmp dword ptr [esi + 0x134], eax; je 0x590c2a; mov edi, dword ptr [esi + 0x140]",
    ),
    (
        "key_row_range_wipe",
        0x00590C08,
        "cmp edi, dword ptr [esi + 0x138]; mov dword ptr [esp + 0x1c], edi; jb 0x590c31; mov dword ptr [esi + 0x130], eax; mov dword ptr [esi + 0x12c], 0xffffffff; mov dword ptr [esi + 0x134], eax; xor al, al; jmp 0x590f0b",
    ),
    (
        "key_null_row_wipe",
        0x00590DA3,
        "mov dword ptr [esi + 0x130], 0; mov dword ptr [esi + 0x12c], 0xffffffff; mov dword ptr [esi + 0x134], 0; xor al, al; jmp 0x590f0b",
    ),
    (
        "key_wait_weight",
        0x00590CC3,
        "mov ecx, dword ptr [ebp + 0x4c]; xor dl, dl; test ecx, ecx; je 0x590d10; sub dword ptr [esi + 0x130], ecx; jne 0x590cde; mov dword ptr [esi + 0x12c], 0xffffffff; mov ecx, dword ptr [ebp + 0x4c]; sub dword ptr [esi + 0x134], ecx",
    ),
    (
        "key_page_flag",
        0x00590D10,
        "cmp byte ptr [ebp + 0x90], 1; je 0x590d2f",
    ),
    (
        "key_page_release",
        0x00590D2F,
        "add dword ptr [esi + 0x134], -1; mov byte ptr [ebp + 0x90], 0; mov ebx, dword ptr [esp + 0x14]",
    ),
    (
        "key_raise_guard",
        0x00590E4C,
        "cmp dword ptr [esp + 0x18], 0; mov dword ptr [esi + 0x128], eax; je 0x590f09; cmp dword ptr [esi + 0x130], 0; jne 0x590f09; mov edi, dword ptr [esp + 0x20]; test edi, edi; je 0x590e7a",
    ),
    ("key_wait0", 0x00590E9A, "mov ecx, 0x133a438; call 0x927fa0"),
    ("key_pressedkey", 0x00590EEC, "mov ecx, 0x133c298; call 0x927fa0"),
    (
        "update_overflow_guard",
        0x00594AE3,
        "cmp dword ptr [esi + 0x12c], -1; mov byte ptr [esp + 0x14], 1; je 0x594b9c; mov eax, dword ptr [esi + 0x48]",
    ),
    ("update_overflow_wait0", 0x00594B1E, "mov ecx, 0x133a438; call 0x927fa0"),
    ("update_overflow_pressed", 0x00594B6D, "mov ecx, 0x133c298; call 0x927fa0"),
    (
        "update_overflow_reset",
        0x00594B8A,
        "mov dword ptr [esi + 0x12c], edi; mov dword ptr [esi + 0x130], ebp; mov dword ptr [esi + 0x134], ebp; cmp dword ptr [esi + 0xe0], ebp",
    ),
    ("update_fit_wait0", 0x00594C86, "mov ecx, 0x133a438; call 0x927fa0"),
    ("update_fit_pressed", 0x00594CD3, "mov ecx, 0x133c298; call 0x927fa0"),
    (
        "update_lastone_remove",
        0x00594EAC,
        "test byte ptr [ecx + 0x19e3], 0x20; je 0x594f3e; mov ebx, dword ptr [esi + 0x64]",
    ),
    (
        "update_talker_write",
        0x00594FF6,
        "mov eax, dword ptr [esi + 0x48]; push 1; xor edx, edx; push edx; push edx; push 0x133c4cc; push edx; mov ecx, 0x593f90; push ecx; push eax",
    ),
    (
        "update_talker_compare_discard",
        0x00594FA7,
        "push eax; lea ecx, [esp + 0x24]; call 0x445db0; mov edi, dword ptr [esi + 0x48]; test byte ptr [edi + 0x19e1], 0x10",
    ),
    (
        "update_talker_last_category",
        0x00594FD6,
        "mov al, byte ptr [edi + 0x19e1]; test al, 3; je 0x594ff6; cmp byte ptr [edi + 0x19e0], bl; je 0x594ff6; or al, 4; mov byte ptr [edi + 0x19e1], al; mov byte ptr [edi + 0x19e0], bl",
    ),
    (
        "event_constructor_field_8",
        0x00929132,
        "xor eax, eax; lea ecx, [ebx + 0x19]; mov dword ptr [ebx + 8], eax",
    ),
    (
        "update_talker_notify",
        0x0059501C,
        "mov eax, dword ptr [esi + 0x48]; push 1; push 0x133c4cc; xor edi, edi; push edi; mov edx, 0x593f90; push edx",
    ),
    (
        "update_key_wait_bit",
        0x00595070,
        "mov ecx, dword ptr [esi + 0x48]; test byte ptr [ecx + 0x19e2], 0x20; je 0x5950ed",
    ),
    (
        "update_wait_weight",
        0x0059509B,
        "push eax; call 0x9da6cc; add esp, 0x14; test eax, eax; je 0x5950fd; mov ecx, dword ptr [eax + 0x4c]",
    ),
    (
        "update_page_break",
        0x005950CC,
        "lea ecx, [edi + 1]; cmp ecx, dword ptr [esi + 0x138]; je 0x5950ed; cmp edi, ebx; jne 0x5950ed; add ebx, ebp; mov byte ptr [eax + 0x90], 1; add dword ptr [esi + 0x134], 1",
    ),
    (
        "textchanged_raise",
        0x0059112B,
        "mov dword ptr [esp + 0xc], 0x135dbec; mov dword ptr [esp + 0x14], esi; je 0x59113f; lea eax, [esi + 0xb4]; push eax; lea eax, [esp + 0xc]; push eax; call 0x91f1e0",
    ),
    (
        "logchanged_raise",
        0x005910A5,
        "mov dword ptr [esp + 0x1c], 0x133c334; call 0x91f1e0",
    ),
    (
        "textchanged_event_init",
        0x00F253A0,
        "push 0x1074b90; push ecx; mov eax, esp; push ecx; mov byte ptr [eax], 0x3f; mov eax, esp; mov ecx, 0x135dbec",
    ),
    (
        "setprop_dispatch",
        0x00591620,
        "mov eax, dword ptr [esp + 4]; cmp eax, 0xe; ja 0x59176e; jmp dword ptr [eax*4 + 0x591774]",
    ),
    (
        "setprop_talker",
        0x00591716,
        "mov eax, dword ptr [esp + 8]; push eax; add ecx, 0x19d8; call 0x5965b0; mov al, 1",
    ),
    (
        "setprop_keywait",
        0x00591744,
        "mov eax, dword ptr [esp + 8]; push eax; add ecx, 0x19e2; call 0x93e220; mov al, 1",
    ),
    (
        "setprop_lastone",
        0x00591759,
        "mov edx, dword ptr [esp + 8]; push edx; add ecx, 0x19e3; call 0x93e220; mov al, 1",
    ),
    (
        "bool_setter",
        0x0093E220,
        "push ecx; mov al, byte ptr [ecx]; mov ecx, dword ptr [esp + 8]; shr al, 5; push ecx; lea edx, [esp + 7]",
    ),
    (
        "talker_info",
        0x00F0FC50,
        "push 0xfa8e80; mov ecx, 0x133c4cc; call 0x9108e0; mov dword ptr [0x133c4e0], 0xb; mov dword ptr [0x133c4e4], 0x100",
    ),
    (
        "keywait_info",
        0x00F0FCB0,
        "push 0xfa8e98; mov ecx, 0x133c514; call 0x9108e0; mov dword ptr [0x133c528], 0xd",
    ),
    (
        "lastone_info",
        0x00F0FCD0,
        "push 0xfa8ea8; mov ecx, 0x133c538; call 0x9108e0; mov dword ptr [0x133c54c], 0xe",
    ),
    (
        "setname_custom",
        0x006FB0A1,
        "mov esi, dword ptr [edi + 0x60]; mov edx, dword ptr [esp + 0x18]; push edx; lea ecx, [esi + 4]; mov dword ptr [esi], 0xffffffff; call 0x447450; cmp dword ptr [esi], -1; mov al, byte ptr [esp + 0xe4]; sete cl; cmp byte ptr [esp + 0x1c], 0; mov byte ptr [esi + 0x58], al; mov byte ptr [esi + 0x59], cl; je 0x6fb24c",
    ),
    (
        "setname_looked_up",
        0x006FB1F3,
        "mov esi, dword ptr [edi + 0x60]; movzx ecx, byte ptr [esi + 0x58]; mov edx, dword ptr [esp + 0xdc]; lea eax, [esp + 0x70]; mov byte ptr [esp + 0x17], cl; push eax; lea ecx, [esi + 4]; mov dword ptr [esi], edx; call 0x447450; cmp dword ptr [esi], -1; movzx ecx, byte ptr [esp + 0x17]; mov eax, dword ptr [esp + 0x1c]; sete dl",
    ),
    (
        "source_name_cast",
        0x007899A4,
        "call 0x9da6cc; mov edi, dword ptr [eax + 0x60]; add esp, 0x14; cmp byte ptr [edi + 0x59], 0; jne 0x789a60",
    ),
    (
        "name_lookup_call",
        0x0076FDFE,
        "push 0xde; push 0x134bbc0; mov ecx, ebx; call 0x76fb10; test al, al; je 0x76fe66",
    ),
    (
        "name_lookup_slot",
        0x0076FBE2,
        "mov ecx, dword ptr [esi]; mov eax, dword ptr [ecx]; mov eax, dword ptr [eax + 0xc]; push ebp; lea edx, [esp + 0x1c]; push edx; mov edx, dword ptr [esp + 0xa8]; push 0xff",
    ),
    (
        "element_raise_gate",
        0x0091E1FC,
        "mov eax, dword ptr [ebp + 0x64]; cmp eax, edi; je 0x91e33d; cmp dword ptr [edx + 0xc], edi; jne 0x91e20f; mov dword ptr [edx + 0xc], eax; cmp dword ptr [edx + 8], edi; jne 0x91e21a; mov eax, dword ptr [ebp + 0x68]; mov dword ptr [edx + 8], eax",
    ),
    (
        "element_ctor_handle_store",
        0x0091E11A,
        "mov dword ptr [esi + 0x64], eax; mov dword ptr [esi + 0x68], eax; mov dword ptr [esi + 0x6c], edi; mov byte ptr [esi + 0x70], cl; jne 0x91e15c",
    ),
    (
        "element_ctor_default_object",
        0x0091E12F,
        "mov edi, 0x1357070; jne 0x91e156",
    ),
    (
        "element_ctor_default_handle",
        0x0091E156,
        "mov dword ptr [esi + 0x64], edi; mov dword ptr [esi + 0x68], edi",
    ),
    (
        "fontattr_ctor_tail",
        0x00557893,
        "mov dword ptr [eax + 0x48], ecx; movss dword ptr [eax + 0x24], xmm0; movss xmm0, dword ptr [esp + 0x24]; xor ecx, ecx; movss dword ptr [eax + 0x28], xmm0; mov dword ptr [eax + 0x44], edx; mov dword ptr [eax + 0x4c], ecx; mov dword ptr [eax + 0x54], ecx; mov dword ptr [eax + 0x58], ecx; mov dword ptr [eax + 0x5c], ecx; mov dword ptr [eax + 0x60], ecx; mov dword ptr [esp + 0x10], ecx; mov dword ptr [eax + 0x68], ecx",
    ),
    (
        "logattr_ctor_zero_fields",
        0x005980A3,
        "mov dword ptr [esi], 0xfa9164; mov dword ptr [esi + 0x80], eax; mov dword ptr [esi + 0x84], eax; mov dword ptr [esi + 0x88], eax; mov dword ptr [esi + 0x8c], eax; mov byte ptr [esi + 0x90], al",
    ),
    (
        "logattr_base_ctor_call",
        0x00598032,
        "push 1; sub esp, 8; movq qword ptr [ecx + 8], xmm0; fst dword ptr [esp + 4]; fstp dword ptr [esp]; push ebx; push ebx; push ebx; lea ecx, [esi + 4]; call 0x557810",
    ),
    (
        "logdoc_row_attr_copy",
        0x00593278,
        "push 0; push 0x126ed78; push 0x126e5dc; push 0; push eax; call 0x9da6cc; add esp, 0x14; push eax; mov ecx, ebx; call 0x599720",
    ),
    (
        "logdoc_row_attr_store",
        0x00593364,
        "mov edx, dword ptr [ecx]; push eax; mov eax, dword ptr [edx + 0x10]; call eax",
    ),
    (
        "page_rows_calc",
        0x00959BF4,
        "mov eax, dword ptr [esi + 0x17f0]; fild dword ptr [eax]; fadd dword ptr [esi + 0x35c]; fdiv dword ptr [esi + 0x1964]; call 0x9d6600; cmp eax, 1; mov dword ptr [esp + 0x20], eax; jge 0x959c1e; mov dword ptr [esp + 0x20], 1",
    ),
    (
        "page_rows_property_store",
        0x00959C1E,
        "lea edi, [esi + 0x179c]; xor ebx, ebx; test byte ptr [edi + 4], 0x10",
    ),
    (
        "page_rows_registered",
        0x0095C591,
        "push ebx; push ebx; push 0x135a5ac; push ebx; mov eax, 0x958ed0; push eax; lea ecx, [esi + 0x179c]; push esi; call 0x9bd240",
    ),
    (
        "line_height_calc",
        0x0095BEE2,
        "call 0x988090; fstp dword ptr [esp + 0x14]; mov ecx, dword ptr [esp + 0x100]; fild dword ptr [ecx]; mov edx, dword ptr [edi + 0x17f0]; fadd dword ptr [esp + 0x14]; fstp dword ptr [esp + 0x14]; fld dword ptr [esp + 0x14]; fst dword ptr [edi + 0x1964]; fstp dword ptr [esp + 0x14]",
    ),
    (
        "visible_lines_info",
        0x00F23A60,
        "push 0x106eb70; mov ecx, 0x135a5ac; call 0x9108e0",
    ),
    (
        "frame_slot2_call",
        0x00595562,
        "mov ecx, dword ptr [esi + 0x1b04]; mov eax, dword ptr [esi + 0x179c]; mov edx, dword ptr [ecx + 0x28]; mov edx, dword ptr [edx + 8]; add ecx, 0x28; push eax; mov eax, dword ptr [esi + 0x1f00]; push eax; call edx; cmp al, 1",
    ),
    (
        "pace_gate",
        0x00595683,
        "test byte ptr [esi + 0x1f0c], 1; je 0x595874; cmp dword ptr [esi + 0x1740], ebp; jge 0x595874; cmp dword ptr [esi + 0x1f04], ebp; movss xmm0, dword ptr [esi + 0x1980]; movss dword ptr [esp + 0x1c], xmm0; jbe 0x595729",
    ),
    (
        "pace_hi_active",
        0x00595704,
        "movss xmm0, dword ptr [esi + 0x1988]; cvtps2pd xmm0, xmm0; mulsd xmm0, qword ptr [0xf62ff0]; cvtpd2ps xmm0, xmm0; movss dword ptr [esp + 0x1c], xmm0",
    ),
    (
        "pace_hi_enter",
        0x0059576B,
        "cmp ecx, dword ptr [esi + 0x1990]; jl 0x595796; movss xmm0, dword ptr [esi + 0x1988]; cvtps2pd xmm0, xmm0; mulsd xmm0, qword ptr [0xf886d8]; cvtpd2ps xmm0, xmm0; mov dword ptr [esi + 0x1f04], eax; movss dword ptr [esp + 0x1c], xmm0",
    ),
    (
        "pace_step",
        0x005957C3,
        "test byte ptr [esi + 0x19e3], bl; movss xmm0, dword ptr [esi + 0x1964]; cvtss2sd xmm1, dword ptr [esp + 0x1c]; cvtps2pd xmm0, xmm0; mulsd xmm0, qword ptr [0xf55ad0]; mulsd xmm0, xmm1; divsd xmm0, qword ptr [0xfa9598]; cvtpd2ps xmm0, xmm0; je 0x595804; cvtss2sd xmm0, xmm0",
    ),
    (
        "scroll_object_getter",
        0x00595652,
        "mov edx, dword ptr [esi + 0x12b4]; mov eax, dword ptr [edx + 0x28]; lea edi, [esi + 0x12b4]; mov ecx, edi; call eax",
    ),
    (
        "pace_offset_set",
        0x00595850,
        "mov edx, dword ptr [edi]; fld dword ptr [esp + 0x1c]; mov eax, dword ptr [edx + 0x6c]; push ecx; mov ecx, edi; fstp dword ptr [esp]; call eax",
    ),
    (
        "default_lines_per_sec",
        0x00595AE7,
        "fld dword ptr [0xf62f64]; push ebx; lea ecx, [esi + 0x1980]; push ecx",
    ),
    (
        "default_lines_per_sec_hi",
        0x00595B26,
        "fld dword ptr [0xfa95a0]; push ebx; lea ecx, [esi + 0x1988]; push ecx",
    ),
    (
        "default_lines_begin_hi",
        0x00595B4B,
        "push ebx; push 0xa; push 0x133c388; xor edx, edx; push edx; mov eax, 0x593f90; push eax",
    ),
    (
        "lines_per_sec_info",
        0x00F0FAF0,
        "push 0xfa8dec; mov ecx, 0x133c340; call 0x9108e0; mov dword ptr [0x133c354], 0",
    ),
    (
        "lines_per_sec_hi_info",
        0x00F0FB10,
        "push 0xfa8df8; mov ecx, 0x133c364; call 0x9108e0; mov dword ptr [0x133c378], 1",
    ),
    (
        "lines_begin_hi_info",
        0x00F0FB30,
        "push 0xfa8e08; mov ecx, 0x133c388; call 0x9108e0; mov dword ptr [0x133c39c], 2",
    ),
    (
        "desktopwidget_ctor_flag",
        0x006E0698,
        "mov al, 1; mov byte ptr [esp + 0x18], al; xorps xmm0, xmm0; movss dword ptr [esi + 0x74], xmm0; mov byte ptr [esi + 0x78], bl; mov byte ptr [esi + 0x79], al",
    ),
    (
        "decoder_passthrough_0x11",
        0x00C9F1C0,
        "mov eax, dword ptr [esp + 0x10]; mov edx, dword ptr [esp + 0xc]; push 0x11; push eax; mov eax, dword ptr [esp + 0xc]; push edx; push eax; call 0xcb3250",
    ),
    (
        "decoder_passthrough_0x12",
        0x00C9F1E0,
        "mov eax, dword ptr [esp + 0x10]; mov edx, dword ptr [esp + 0xc]; push 0x12; push eax; mov eax, dword ptr [esp + 0xc]; push edx; push eax; call 0xcb3250",
    ),
    (
        "decoder_passthrough_0x15",
        0x00C9F310,
        "mov eax, dword ptr [esp + 0x10]; mov edx, dword ptr [esp + 0xc]; push 0x15; push eax; mov eax, dword ptr [esp + 0xc]; push edx; push eax; call 0xcb3250",
    ),
    (
        "decoder_passthrough_0x18",
        0x00C9F330,
        "mov eax, dword ptr [esp + 0x10]; mov edx, dword ptr [esp + 0xc]; push 0x18; push eax; mov eax, dword ptr [esp + 0xc]; push edx; push eax; call 0xcb3250",
    ),
    (
        "decoder_default_0x13",
        0x00C9F270,
        "push esi; push edi; mov edi, dword ptr [esp + 0xc]; push edi; mov esi, ecx; call 0xcb2590; test al, al; je 0xc9f29e; mov eax, dword ptr [esp + 0x18]; push 0xec; push 0x13; push eax; mov ecx, esi; call 0xcb25d0",
    ),
    (
        "decoder_default_0x14",
        0x00C9F2C0,
        "push esi; push edi; mov edi, dword ptr [esp + 0xc]; push edi; mov esi, ecx; call 0xcb2590; test al, al; je 0xc9f2ee; mov eax, dword ptr [esp + 0x18]; push 0xec; push 0x14; push eax; mov ecx, esi; call 0xcb25d0",
    ),
    (
        "decoder_default_probe",
        0x00CB2590,
        "mov eax, dword ptr [esp + 4]; cmp byte ptr [eax], 0xec; sete al; ret 4",
    ),
)

# object VA, initializer VA, name literal VA, name
COMMANDS: tuple[tuple[int, int, int, str], ...] = (
    (0x0133A438, 0x00F0E2D0, 0x00FA8230, WAIT0),
    (0x0133A4D8, 0x00F0E350, 0x00FA8250, PRESS_KEY),
    (0x0133A578, 0x00F0E3D0, 0x00FA8274, PRESS_MOUSE),
    (0x0133A618, 0x00F0E450, 0x00FA829C, PRESS_CANCEL),
    (0x0133C158, 0x00F0F940, 0x00FA8A40, CONTROL_PRESS_KEY),
    (0x0133C1F8, 0x00F0F9C0, 0x00FA8A54, CONTROL_PRESS_CANCEL),
    (0x0133C298, 0x00F0FA40, 0x00FA8A70, CONTROL_PRESSED_KEY),
    (0x01357C88, 0x00F21460, 0x0106A4BC, "ApplicationCommands.Operate"),
)
COMMAND_CONSTRUCTOR = 0x00928C90

LITERALS: tuple[tuple[int, str], ...] = (
    (0x00FA8E80, "Talker"),
    (0x00FA8E88, "LastCategory"),
    (0x00FA8E98, "IsEnableKeyWait"),
    (0x00FA8EA8, "IsDisplayLastOne"),
    (0x00FA8DE0, "LogChanged"),
    (0x01074B90, "TextChanged"),
    (0x00FD87C4, TEXT_UI_SHEET),
    (0x00FA8DEC, "LinesPerSec"),
    (0x00FA8DF8, "LinesPerSecHi"),
    (0x00FA8E08, "LinesBeginHi"),
    (0x0106EB58, "FirstVisibleLineIndex"),
    (0x0106EB70, "VisibleLines"),
)

# target -> direct callers (E8 and E9)
CALLERS: dict[int, list[int]] = {
    0x00575900: [0x004D9EA4],
    0x005910F0: [0x005911C1, 0x00594068, 0x0059419A, 0x00594972, 0x005955EB],
    0x004DBAD0: [0x00595FB0],
    0x004D9B30: [0x00591D75],
    0x0076FB10: [0x0076FE0A, 0x00770198],
    0x0076FD80: [0x0076FFAE, 0x0077006C, 0x00770F64],
    0x007700B0: [0x005789F0],
    0x006FAFF0: [0x007083B0, 0x0089D649],
    0x00772050: [0x006E8599, 0x0089FF75],
    0x00591180: [0x00595963, 0x005959A4],
    0x00557810: [
        0x00557A27,
        0x00557BB8,
        0x0055B192,
        0x0055B5E6,
        0x0055B81E,
        0x00598049,
        0x006970AC,
        0x008CFA38,
    ],
    0x00597FB0: [0x00598098],
    0x00598070: [0x00595E49],
    0x00599690: [0x0059974D],
    0x00599720: [0x00593292],
    0x0091CDD0: [0x0097BCC7],
    0x0091E1C0: [
        0x0066DCD1,
        0x0091E3B9,
        0x0091E688,
        0x0091E7B4,
        0x0091E7F4,
        0x0091E825,
        0x0091E880,
        0x0091EBF2,
        0x0091F199,
        0x0091F1F0,
        0x0091F5A4,
    ],
}

# value -> every dword-aligned or unaligned occurrence in the image sections
DWORD_REFS: dict[int, list[int]] = {
    # Wait0 command object: handler, release, two update raises, initializer, atexit
    0x0133A438: [
        0x004D9E90,
        0x00590E9B,
        0x00594B1F,
        0x00594C87,
        0x00F0E315,
        0x00F2FDC1,
    ],
    # LogAttribute and RaptureFontAttribute RTTI descriptors
    0x0126ED38: [
        0x0054D9F8,
        0x00590C66,
        0x00590CA7,
        0x00590D81,
        0x00590DDF,
        0x0059269D,
        0x005930A0,
        0x005935A8,
        0x00593DBA,
        0x00595090,
        0x01148E64,
        0x01148E8C,
    ],
    0x0126ED78: [
        0x0054D90A,
        0x0055B109,
        0x0055B4BC,
        0x0059327B,
        0x01146A74,
        0x01146A98,
    ],
    # LogAttribute, RaptureFontAttribute and FontAttribute vtable stores
    0x00FA9164: [0x005980A5, 0x0059975A],
    0x00FA2F8C: [0x0055B2D0, 0x00597FEF, 0x005996D3],
    0x00FA2B88: [
        0x00554652,
        0x0055466A,
        0x0055B2BA,
        0x0055B342,
        0x00597FDE,
        0x005996BA,
        0x00959ED9,
    ],
}

# Rich-string decoder handler vector stores in 0x00CA0390 (lead -> thunk).
DECODER_HANDLERS: dict[int, int] = {
    0x07: 0x00B24200,
    0x08: 0x00A53E30,
    0x09: 0x00B24230,
    0x11: 0x00A53E20,
    0x12: 0x00CA1600,
    0x13: 0x00B241D0,
    0x14: 0x00B24210,
    0x15: 0x0059BD10,
    0x18: 0x00B24220,
    0x1E: 0x00B24240,
    0x20: 0x00A4B0F0,
    0x21: 0x008F70A0,
    0x22: 0x008F70C0,
    0x23: 0x008F7080,
    0x24: 0x00A4B100,
    0x25: 0x008E6320,
    0x26: 0x008F70D0,
    0x28: 0x00CA1590,
    0x29: 0x00CA15B0,
    0x2A: 0x00CA15D0,
    0x2B: 0x00A4B120,
    0x2C: 0x00A4B0D0,
    0x2D: 0x00A4B110,
    0x2E: 0x00CA15E0,
    0x2F: 0x00A4B130,
    0x30: 0x00A4B0E0,
    0x31: 0x00CA15C0,
    0x32: 0x00CA15F0,
    0x33: 0x00CA1610,
    0x34: 0x00CA15A0,
}

# VA, struct format, value
OPERANDS: tuple[tuple[int, str, float], ...] = (
    (0x00F62F64, "<f", 2.0),
    (0x00FA95A0, "<f", 10.0),
    (0x00F62FF0, "<d", 3.0),
    (0x00F886D8, "<d", 5.0),
    (0x00F55AD0, "<d", 100.0),
    (0x00FA9598, "<d", 5000.0),
    (0x00F59898, "<d", 0.5),
)

# Functions that construct, copy or cast the FontAttribute family, as (first, end).
FONT_ATTRIBUTE_RANGES: tuple[tuple[int, int], ...] = (
    (0x0054D7B0, 0x0054DC00),
    (0x00557810, 0x005578F0),
    (0x00557950, 0x00557AD0),
    (0x00557AD0, 0x00557C30),
    (0x0055AF50, 0x0055AFF0),
    (0x0055AFF0, 0x0055B0D0),
    (0x0055B0D0, 0x0055B200),
    (0x0055B290, 0x0055B300),
    (0x0055B300, 0x0055B360),
    (0x0055B380, 0x0055BB50),
    (0x00590BB0, 0x00590F40),
    (0x00592600, 0x00592760),
    (0x00592F30, 0x00593740),
    (0x00593CF0, 0x00593F90),
    (0x00594A20, 0x005951A0),
    (0x00595A20, 0x005961E0),
    (0x00597FB0, 0x00598070),
    (0x00598070, 0x005980F0),
    (0x00599690, 0x00599700),
    (0x00599720, 0x005997A0),
    (0x005997A0, 0x00599800),
    (0x00697000, 0x00697170),
    (0x00697170, 0x00697580),
    (0x00697A90, 0x00697AF0),
    (0x00697AF0, 0x00697B20),
    (0x00697B40, 0x00697BA0),
    (0x00697BA0, 0x00697C40),
    (0x00697C40, 0x00697CE0),
    (0x00697D80, 0x006980B0),
    (0x008CE530, 0x008CE7D0),
    (0x008CF960, 0x008CF9B0),
    (0x008CF9B0, 0x008CFB00),
)
# Stores to [reg + 0x48] or [reg + 0x4C] in those ranges, esp-relative locals
# excluded: the constructor, two copies and the draw-record writer 0x00697170.
FONT_ATTRIBUTE_STORES = frozenset(
    {
        0x00557893,
        0x005578AB,
        0x0055AFC0,
        0x0055AFCA,
        0x0055B085,
        0x0055B08F,
        0x00697313,
        0x006973F7,
        0x00697435,
    }
)

# table VA, slot -> target
LOG_DOCUMENT_INTERFACE = 0x00FA9504
LOG_DOCUMENT_SLOTS = {
    0: 0x00599500,
    1: 0x00593950,
    2: 0x00594A20,
    3: 0x00590BB0,
    4: 0x00590960,
}
CONTAINER_HANDLER_WORD = (0x00F912C0, 0x004D9D70)
SET_DISPLAY_NAME = (
    0x01057388,
    ".?AVSetDisplayNameReceiver@Network@Command@Client@Script@Lua@Application@@",
)
SET_DISPLAY_NAME_SLOT = (1, 0x0089D610)


def disassemble(image: ecr.Image, va: int, count: int) -> list[str]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    code = image.read(va, 16 * count)
    rows = [f"{i.mnemonic} {i.op_str}".strip() for i in md.disasm(code, va)]
    return rows[:count]


def check_listings(image: ecr.Image) -> list[str]:
    errors = []
    for label, va, text in CHECKS:
        want = text.split("; ")
        got = disassemble(image, va, len(want))
        if got != want:
            errors.append(f"listing {label} at {va:#010x} differs: {got}")
    return errors


def check_commands(image: ecr.Image) -> list[str]:
    errors = []
    for obj, init, literal, name in COMMANDS:
        if image.cstr(literal) != name:
            errors.append(f"literal at {literal:#010x} is not {name!r}")
        rows = disassemble(image, init, 26)
        if f"push {literal:#x}" not in rows:
            errors.append(f"initializer {init:#010x} does not push {name}")
        index = next((i for i, r in enumerate(rows) if r == f"mov ecx, {obj:#x}"), None)
        want_call = f"call {COMMAND_CONSTRUCTOR:#x}"
        if index is None or want_call not in rows[index + 1 : index + 4]:
            errors.append(f"initializer {init:#010x} does not construct {obj:#010x}")
    return errors


def check_literals(image: ecr.Image) -> list[str]:
    return [
        f"literal at {va:#010x} is not {text!r}"
        for va, text in LITERALS
        if image.cstr(va) != text
    ]


def check_callers(image: ecr.Image) -> list[str]:
    found = ecr.find_callers(image, list(CALLERS))
    return [
        f"callers of {target:#010x} are {[hex(s) for s in found[target]]}"
        for target, want in CALLERS.items()
        if found[target] != want
    ]


def read_dword(image: ecr.Image, va: int) -> int:
    return struct.unpack("<I", image.read(va, 4))[0]


def check_tables(image: ecr.Image) -> list[str]:
    errors = []
    for slot, want in LOG_DOCUMENT_SLOTS.items():
        got = read_dword(image, LOG_DOCUMENT_INTERFACE + 4 * slot)
        if got != want:
            errors.append(f"LogDocument interface slot {slot} is {got:#010x}")
    word, want = CONTAINER_HANDLER_WORD
    if read_dword(image, word) != want:
        errors.append(f"container handler word {word:#010x} is not {want:#010x}")
    vtable, rtti = SET_DISPLAY_NAME
    locator = read_dword(image, vtable - 4)
    descriptor = read_dword(image, locator + 12)
    if image.cstr(descriptor + 8) != rtti:
        errors.append(f"vtable {vtable:#010x} is not {rtti}")
    slot, want = SET_DISPLAY_NAME_SLOT
    if read_dword(image, vtable + 4 * slot) != want:
        errors.append(f"SetDisplayNameReceiver slot {slot} is not {want:#010x}")
    return errors


def find_dwords(image: ecr.Image, value: int) -> list[int]:
    pattern = struct.pack("<I", value)
    found = []
    for _, vsize, address, rsize, raw in image.sections:
        block = image.data[raw : raw + min(vsize, rsize)]
        start = block.find(pattern)
        while start >= 0:
            found.append(image.base + address + start)
            start = block.find(pattern, start + 1)
    return found


def check_refs(image: ecr.Image) -> list[str]:
    errors = []
    for value, want in DWORD_REFS.items():
        got = find_dwords(image, value)
        if got != want:
            errors.append(f"references to {value:#010x} are {[hex(v) for v in got]}")
    return errors


def check_operands(image: ecr.Image) -> list[str]:
    errors = []
    for va, fmt, want in OPERANDS:
        got = struct.unpack(fmt, image.read(va, struct.calcsize(fmt)))[0]
        if got != want:
            errors.append(f"operand at {va:#010x} is {got}")
    return errors


def check_decoder_handlers(image: ecr.Image) -> list[str]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    store = re.compile(r"^mov dword ptr \[e\w\w \+ (0x[0-9a-f]+)\], (0x[0-9a-f]{6})$")
    found = {}
    for ins in md.disasm(image.read(0x00CA0390, 0x570), 0x00CA0390):
        match = store.match(f"{ins.mnemonic} {ins.op_str}")
        if match:
            found[int(match.group(1), 16) // 4] = int(match.group(2), 16)
    if found == DECODER_HANDLERS:
        return []
    return [
        f"decoder handler vector is { ({hex(k): hex(v) for k, v in found.items()}) }"
    ]


def check_attribute_stores(image: ecr.Image) -> list[str]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    pattern = re.compile(
        r"^(mov|add|or|sub|inc|and|xor|xchg) (dword ptr )?\[e(?!sp)\w\w \+ 0x4[8c]\]"
    )
    found = set()
    for first, end in FONT_ATTRIBUTE_RANGES:
        for ins in md.disasm(image.read(first, end - first), first):
            if pattern.match(f"{ins.mnemonic} {ins.op_str}"):
                found.add(ins.address)
    if found == FONT_ATTRIBUTE_STORES:
        return []
    return [
        "FontAttribute-family stores differ: "
        f"extra {sorted(hex(a) for a in found - FONT_ATTRIBUTE_STORES)} "
        f"missing {sorted(hex(a) for a in FONT_ATTRIBUTE_STORES - found)}"
    ]


def verify(image: ecr.Image) -> list[str]:
    return (
        check_listings(image)
        + check_commands(image)
        + check_literals(image)
        + check_callers(image)
        + check_tables(image)
        + check_refs(image)
        + check_operands(image)
        + check_decoder_handlers(image)
        + check_attribute_stores(image)
    )


# ---- optional install check ----

SIGNATURE = bytes.fromhex("5351455800000000")

NPC_SAY_LOG = {
    "Name": "LogControl_NpcSay",
    "LinesPerSec": "10",
    "LogFormat": "2600",
    "LogFormat2": "2600",
    "IsEnableKeyWait": "True",
    "IsDisplayLastOne": "True",
    "FontSize": "16",
    "FontHeightSpacing": "2",
    "Margin": "10,0,10,4",
}
NPC_SAY_BODY_LABEL = {"Width": "620", "Height": "106"}
LOG_WIDGET_LOGS = {
    "LogControl_Main_1": {
        "LinesPerSec": "3",
        "LinesPerSecHi": "30",
        "IsEnableKeyWait": "False",
    },
    "LogControl_Main_2": {
        "LinesPerSec": "3",
        "LinesPerSecHi": "30",
        "IsEnableKeyWait": "False",
    },
}
# Property names that no shipped Lua script mentions.
UNSET_PROPERTY_NAMES = (
    b"IsEnableKeyWait",
    b"IsDisplayLastOne",
    b"LinesPerSec",
    b"LinesBeginHi",
    b"FontHeightSpacing",
    b"VisibleLines",
)


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_form(path: Path, blowfish_module) -> ET.Element:
    data = path.read_bytes()
    if data[:8] != SIGNATURE:
        raise SystemExit(f"{path.name} has no SQEX signature")
    body = blowfish_module.Blowfish(path.name.encode("utf-8")).decrypt(data[8:])
    return ET.fromstring(body.decode("utf-8").lstrip("\ufeff"))


def log_controls(root: ET.Element) -> dict[str, dict[str, str]]:
    return {
        node.get("Name", ""): dict(node.attrib)
        for node in root.iter()
        if node.tag.rsplit("}", 1)[-1] == "LogControl"
    }


def check_install(client: Path, blowfish_path: Path) -> list[str]:
    errors = []
    module = load_module(blowfish_path, "sqex_blowfish")
    widgets = client / "sqwt" / "widget"
    npc = decode_form(widgets / "NpcSayWidget.form", module)
    controls = log_controls(npc)
    if list(controls) != ["LogControl_NpcSay"]:
        errors.append(f"NpcSay LogControl names are {list(controls)}")
    for key, want in NPC_SAY_LOG.items():
        got = controls.get("LogControl_NpcSay", {}).get(key)
        if got != want:
            errors.append(f"NpcSay LogControl {key} is {got!r}")
    labels = [
        node.attrib
        for node in npc.iter()
        if node.tag.rsplit("}", 1)[-1] == "Label" and node.get("Height")
    ]
    if [{k: v for k, v in a.items() if k in NPC_SAY_BODY_LABEL} for a in labels] != [
        NPC_SAY_BODY_LABEL
    ]:
        errors.append(f"NpcSay body label size is {labels}")
    log = log_controls(decode_form(widgets / "LogWidget.form", module))
    for name, want in LOG_WIDGET_LOGS.items():
        for key, value in want.items():
            if log.get(name, {}).get(key) != value:
                errors.append(
                    f"LogWidget {name} {key} is {log.get(name, {}).get(key)!r}"
                )
    lpb = load_module(Path(__file__).with_name("decode_lpb.py"), "decode_lpb")
    hits: dict[bytes, int] = {name: 0 for name in UNSET_PROPERTY_NAMES}
    scripts = 0
    for path in sorted((client / "script").rglob("*.lpb")):
        decoded = lpb.decode_lpb(path.read_bytes())
        if decoded is None:
            continue
        scripts += 1
        for name in hits:
            hits[name] += name in decoded
    if scripts == 0:
        errors.append("no shipped script decoded")
    errors += [
        f"{name.decode()} occurs in {n} scripts" for name, n in hits.items() if n
    ]
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("orig/ffxivgame.exe"))
    parser.add_argument("--install", type=Path, help="client directory of an install")
    parser.add_argument("--blowfish", type=Path, help="xivl-tools tools/blowfish.py")
    args = parser.parse_args(argv)
    data = args.exe.read_bytes()
    ecr.verify_identity(data)
    try:
        errors = verify(ecr.Image(data))
    except ImportError:
        raise SystemExit("Capstone is required: python -m pip install capstone")
    install = "not run"
    if args.install and args.blowfish:
        extra = check_install(args.install, args.blowfish)
        errors += extra
        install = "FAIL" if extra else "ok"
    for error in errors:
        print(f"FAIL {error}")
    print(
        f"{len(CHECKS)} listings, {len(COMMANDS)} commands, {len(LITERALS)} literals, "
        f"{len(CALLERS)} caller sets, {len(LOG_DOCUMENT_SLOTS) + 3} table slots, "
        f"{len(DWORD_REFS)} reference sets, {len(OPERANDS)} operands, "
        f"{len(FONT_ATTRIBUTE_RANGES)} attribute ranges, install {install}: "
        f"{'FAIL' if errors else 'ok'}"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
