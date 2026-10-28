#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C)  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify the LogControl key-wait and speaker-name facts in ffxivgame.exe.

Six check groups over the pinned executable:

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

The module also holds reference models of the container fan-out, the key-wait
release and the boolean property byte, so the tests state their expected
results without running the code they check.
"""

from __future__ import annotations

import argparse
import struct
import sys
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
}

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


def verify(image: ecr.Image) -> list[str]:
    return (
        check_listings(image)
        + check_commands(image)
        + check_literals(image)
        + check_callers(image)
        + check_tables(image)
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("orig/ffxivgame.exe"))
    args = parser.parse_args(argv)
    data = args.exe.read_bytes()
    ecr.verify_identity(data)
    try:
        errors = verify(ecr.Image(data))
    except ImportError:
        raise SystemExit("Capstone is required: python -m pip install capstone")
    for error in errors:
        print(f"FAIL {error}")
    print(
        f"{len(CHECKS)} listings, {len(COMMANDS)} commands, {len(LITERALS)} literals, "
        f"{len(CALLERS)} caller sets, {len(LOG_DOCUMENT_SLOTS) + 3} table slots: "
        f"{'FAIL' if errors else 'ok'}"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
