#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C)  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Verify the generic RunEventFunction executor facts in ffxivgame.exe.

Five check groups over the pinned executable:

1. Instruction listings, data bytes and literals at the binding predicate, the
   two refusal branches, the Lua error path and the result serializer recorded
   in docs/script/delegate-command-producer.md and
   docs/event/event-base-callback-arguments.md.
2. The zero-fill cells the pages record as having no image value.
3. The direct E8/E9 caller sets of the entries those pages rely on.
4. The Client::ErrorHandler and Event::PlayerManager vtable slots.
5. The empty base handler slot.

The module also holds reference models of the binding predicate, the refusal
reply and the result serializer, so the tests state their expected bytes
without running the code they check.
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path
from typing import Mapping, NamedTuple, Sequence

import extract_class_registrations as ecr

TRIGGER_GROUP = 0x0F
WINDOW = 0x40
END_TAG = 0x0F
REFUSAL_KIND_BYTE = 1


def group_of(key: int) -> int:
    return (key >> 20) & 0xF


def selects_trigger(key: int, registered: Mapping[int, int]) -> bool:
    """0x00D035D0: the group is registered, the masked key equals its stored
    value and the group equals the byte at 0x0130D426."""
    stored = registered.get(group_of(key))
    return (
        stored is not None
        and key & 0xF0F00000 == stored
        and group_of(key) == TRIGGER_GROUP
    )


def refusal_window() -> bytes:
    """The 64-byte buffer both failure branches of 0x00896F70 send."""
    return bytes([END_TAG]) + bytes(WINDOW - 1)


def refusal_application(trigger_key: int, context_word: int, event_type: int) -> bytes:
    """The first 0x51 application bytes of the 0x012E reply; 0x51-0x57 are an
    unwritten stack tail."""
    block = refusal_window()
    kind = struct.pack("<i", REFUSAL_KIND_BYTE)
    crc = struct.pack("<I", zlib.crc32(block))
    return (
        struct.pack("<II", trigger_key, context_word)
        + kind
        + crc
        + bytes([event_type])
        + block
    )


class Result(NamedTuple):
    kind: str
    value: object = None


def encode_results(
    results: Sequence[Result], window: int = WINDOW, big_endian: bool = True
) -> bytes:
    """Reference model of 0x00790CE0 over the writer built by 0x0078F790.

    Returns every byte written, including bytes past the window. Only the
    first `window` bytes are transmitted. The model does not cover the item
    branch of control results or the closed flag at writer +0x22.
    """
    order = ">" if big_endian else "<"
    out = bytearray([END_TAG])
    remaining = window - 1

    def put(chunk: bytes) -> None:
        nonlocal remaining
        del out[-1]
        out.extend(chunk)
        out.append(END_TAG)
        remaining -= len(chunk)

    for result in results:
        kind, value = result
        if kind == "int":
            put(b"\x00" + struct.pack(order + "I", value & 0xFFFFFFFF))
        elif kind == "float":
            put(b"\x01" + struct.pack(order + "f", value))
        elif kind == "bool":
            put(b"\x03" if value else b"\x04")
        elif kind == "string":
            text = value.encode("ascii")[: remaining + 1]
            put(b"\x02" + text + b"\x00")
        elif kind == "control":
            put(b"\x06" + struct.pack(order + "I", value))
        elif kind == "nil":
            put(b"\x05")
        # tentative and table results, and any other kind, write nothing
    return bytes(out)


def wire_window(stream: bytes, window: int = WINDOW) -> bytes:
    return (stream + bytes(window))[:window]


# label, VA, expected listing
CHECKS: tuple[tuple[str, int, str], ...] = (
    ("owner_predicate", 0x00CC7190, "mov ecx, dword ptr [ecx]; jmp 0xcd80f0"),
    ("classifier_load", 0x00CD80F0, "mov ecx, dword ptr [ecx + 0x1c4]; jmp 0xd035d0"),
    (
        "group_extract",
        0x00D03340,
        "mov ecx, dword ptr [esp + 8]; mov eax, dword ptr [esp + 4]; shr ecx, 0x14; and cl, 0xf",
    ),
    (
        "predicate_membership_first",
        0x00D035D0,
        "push esi; mov esi, dword ptr [esp + 8]; push esi; call 0xd03540; test al, al; jne 0xd035e3",
    ),
    (
        "predicate_group_compare",
        0x00D035E3,
        "mov eax, dword ptr [esi]; push eax; lea ecx, [esp + 0xc]; push ecx; call 0xd03340; mov dl, byte ptr [eax]; add esp, 8; cmp dl, byte ptr [0x130d426]",
    ),
    (
        "membership_lookup",
        0x00D03555,
        "call 0xd03340; add esp, 8; lea edx, [esp + 0x1c]; push edx; lea eax, [esp + 0x14]; push eax; lea ecx, [esi + 4]; call 0xd03dc0",
    ),
    (
        "membership_sentinel",
        0x00D0356F,
        "mov ebx, dword ptr [esi + 0xc]; lea eax, [esi + 8]; mov esi, dword ptr [esp + 0x10]",
    ),
    (
        "membership_absent",
        0x00D0358A,
        "cmp ebp, ebx; jne 0xd0359a; pop edi",
    ),
    (
        "membership_mask_compare",
        0x00D035AD,
        "and edi, 0xf0f00000; cmp edi, dword ptr [ebp + 0xc]; pop edi",
    ),
    ("membership_result", 0x00D035B9, "sete al"),
    (
        "hash_scramble",
        0x00D03DCC,
        "movsx ecx, byte ptr [eax]; push edi; xor ecx, 0xdeadbeef",
    ),
    ("hash_divisor", 0x00D03DD6, "push 0x1f31d; push ecx; call 0x9d6e98"),
    (
        "hash_multipliers",
        0x00D03DE1,
        "imul edx, edx, 0x41a7; imul eax, eax, 0xb14; add esp, 8",
    ),
    (
        "hash_wrap",
        0x00D03DF2,
        "jns 0xd03dfa; add edx, 0x7fffffff; mov eax, dword ptr [esi + 0x20]",
    ),
    (
        "node_key_compare",
        0x00D03E7D,
        "mov dl, byte ptr [ebx + 8]; mov eax, dword ptr [esp + 0x28]; cmp dl, byte ptr [eax]",
    ),
    (
        "executor_call",
        0x0089E2B3,
        "lea edx, [esi + 0x64]; push edx; add esi, 0x10; push esi; push ebx; mov ecx, ebp",
    ),
    (
        "executor_thunk",
        0x006E1140,
        "mov dword ptr [esp + 4], ecx; mov ecx, dword ptr [ecx + 0xf8]; jmp 0x896f70",
    ),
    (
        "binding_test",
        0x00896FA6,
        "push ebx; mov ecx, edi; call 0xcc7190; test al, al",
    ),
    (
        "binding_owner_lookup",
        0x00896FBD,
        "push ebx; mov ecx, edi; call 0xcc7a50; mov ebx, dword ptr [esp + 0xa0]; mov ebp, eax",
    ),
    (
        "binding_gates",
        0x00896FCE,
        "test ebp, ebp; je 0x8971b4; cmp byte ptr [ebp + 0x5c], 0; je 0x8971b4; mov ecx, dword ptr [esi + 8]; test ecx, ecx; je 0x8971b4; mov eax, dword ptr [ecx]; mov edx, dword ptr [eax + 0x10]; call edx; test al, al; je 0x8971b4",
    ),
    (
        "conversion_branch",
        0x0089706F,
        "call 0x790aa0; test al, al; jne 0x897152",
    ),
    (
        "refusal_a_buffer",
        0x0089707C,
        "push 0; push 0x40; lea ecx, [esp + 0x30]; call 0x785b90; mov byte ptr [esp + 0x98], 7",
    ),
    (
        "refusal_a_length_slot",
        0x008970D1,
        "push eax; push 0; lea ecx, [esp + 0x30]; call 0x6ce260",
    ),
    (
        "refusal_a_send",
        0x008970DD,
        "mov edx, dword ptr [esp + 0xb4]; push eax; push 0x12c3f72; push edx; push ebx; push edi; call 0x894090; add esp, 0x18",
    ),
    (
        "refusal_b_buffer",
        0x008971B4,
        "push 0; push 0x40; lea ecx, [esp + 0x30]; call 0x785b90",
    ),
    (
        "refusal_b_writer",
        0x008971E9,
        "push 0; push ebp; push esi; lea ecx, [esp + 0x44]",
    ),
    (
        "refusal_b_send",
        0x00897213,
        "mov eax, dword ptr [esp + 0xb0]; mov ecx, dword ptr [esp + 0xa0]; push ebp; push esi; push 0x12c3f72; push eax; push ecx; push edi; call 0x894090; add esp, 0x18",
    ),
    ("index_returns_pointer", 0x006CE284, "ret 4"),
    (
        "writer_end_tag",
        0x0078F7C8,
        "call 0x9d2110; mov eax, dword ptr [esi + 4]; mov ecx, dword ptr [edi]; mov byte ptr [eax], 0xf; add esp, 0xc; add eax, 1",
    ),
    (
        "wrapper_to_builder",
        0x008940EC,
        "mov edx, dword ptr [esp + 0x3c]; mov eax, dword ptr [esp + 0x38]; mov ecx, dword ptr [esp + 0x30]; push edx; mov edx, dword ptr [esp + 0x38]; push eax; push ecx; push edx; lea eax, [esp + 0x18]; push eax; push esi; lea ecx, [esp + 0x24]",
    ),
    (
        "builder_opcode",
        0x0075E700,
        "mov dword ptr [esp + 0x20], 0x12e; mov dword ptr [esp + 0x24], 0x68",
    ),
    (
        "builder_kind_byte",
        0x0075E6D7,
        "mov edx, dword ptr [ebp + 0x10]; mov edi, dword ptr [ebp + 0xc]; mov edi, dword ptr [edi]; mov ecx, dword ptr [ebp + 0x14]; mov cl, byte ptr [ecx]; mov dl, byte ptr [edx]",
    ),
    (
        "completion_serializer",
        0x00894B3C,
        "lea edi, [esi + 4]; push edi; lea ecx, [esp + 0x26]; call 0x790ce0; mov ecx, dword ptr [esi + 8]",
    ),
    (
        "request_latch",
        0x00CCF9B3,
        "mov eax, dword ptr [esi + 0xc]; mov ecx, dword ptr [eax + 0x1cc]; cmp byte ptr [ecx + 8], 0; jne 0xccfa04; push ebx",
    ),
    (
        "pcall_status_switch",
        0x00CF409D,
        "call 0xdce900; add esp, 0x10; sub eax, 2; je 0xcf418b; sub eax, 2; je 0xcf412a; sub eax, 1",
    ),
    (
        "pcall_run_text",
        0x00CF418B,
        "mov ecx, dword ptr [0xf67298]; push ecx; push 0x110eacc",
    ),
    (
        "pcall_memory_text",
        0x00CF412A,
        "mov edx, dword ptr [0xf67298]; push edx; push 0x110eadc",
    ),
    (
        "pcall_other_status_exit",
        0x00CF40AE,
        "sub eax, 2; je 0xcf412a; sub eax, 1; jne 0xcf4205",
    ),
    (
        "report_run_error",
        0x00CD00BA,
        "call 0xcf3d80; test al, al; je 0xcd00de",
    ),
    (
        "report_run_call",
        0x00CD00D0,
        "mov ecx, dword ptr [ebp + 0x1cc]; push eax; call 0xcd3e70",
    ),
    ("report_memory_call", 0x00CD00FF, "call 0xcd3990"),
    (
        "report_double_error",
        0x00CD0106,
        "lea ecx, [esp + 0x18]; call 0xcf3d90; test al, al; je 0xcd0132; mov eax, dword ptr [esp + 0x1c]",
    ),
    ("latch_set_run", 0x00CD3EBC, "mov byte ptr [esi + 8], 1"),
    ("latch_set_double", 0x00CD40D2, "mov byte ptr [esi + 8], 1"),
    (
        "latch_clear_handled",
        0x00CD3FC3,
        "cmp al, bl; je 0xcd3fcc; mov byte ptr [esi + 8], bl; jmp 0xcd4033; mov edx, dword ptr [esi + 4]",
    ),
    (
        "sink_slot4",
        0x00CD401C,
        "mov edx, dword ptr [esi]; mov edx, dword ptr [edx + 0x10]; lea eax, [esp + 0x118]; push eax; mov ecx, esi; call edx; mov byte ptr [edi + 0x13b], bl",
    ),
    (
        "sink_slot1",
        0x00CD3AA0,
        "mov eax, dword ptr [edx + 4]; mov ecx, esi; mov byte ptr [esp + 0x180], 4; call eax; lea ecx, [esp + 0x74]",
    ),
    (
        "sink_slot5",
        0x00CD4189,
        "mov edx, dword ptr [esi]; push eax; mov eax, dword ptr [edx + 0x14]; mov ecx, esi; mov byte ptr [esp + 0x180], 4",
    ),
    (
        "sink_one_shot",
        0x0076F149,
        "cmp byte ptr [ebp + 0x24], bl; je 0x76f1f5",
    ),
    ("sink_raise", 0x0076F1EB, "push 0x9c40; call 0x4da380"),
    (
        "sink_generic_emit",
        0x0076F52E,
        "call 0x76e270; lea eax, [esp + 0x80]",
    ),
    (
        "sink_servererror_state",
        0x0076F4FA,
        "je 0x76f511; lea edx, [esp + 0x80]; mov dword ptr [esp + 0x80], 6",
    ),
    (
        "diagnostic_record",
        0x0076E4B4,
        "mov dword ptr [esp + 0x194], 0x12d; mov dword ptr [esp + 0x198], 0xc8",
    ),
    ("result_type", 0x00790D8F, "movsx ecx, byte ptr [ecx + 0xc]"),
    ("result_int_tag", 0x00790DC1, "mov byte ptr [eax], 0"),
    ("result_end_tag", 0x00790E0C, "mov byte ptr [eax], 0xf"),
    ("result_float_call", 0x00790E59, "call 0x791640"),
    (
        "result_other_types",
        0x00790E7C,
        "lea edx, [esp + 0x24]; push edx; push ecx; lea ecx, [esp + 0x48]",
    ),
    (
        "result_dispatch",
        0x00792B70,
        "mov eax, dword ptr [esp + 4]; cmp eax, 2; jne 0x792b91; mov eax, dword ptr [esp + 8]; mov ecx, dword ptr [eax + 4]; movzx edx, byte ptr [ecx]; mov eax, dword ptr [eax]; mov ecx, dword ptr [eax + 4]; push edx; call 0x7900b0; ret 8; cmp eax, 3; jne 0x792ba2; mov ecx, dword ptr [esp + 8]; call 0x7924c0; ret 8; cmp eax, 4; jne 0x792bbb; mov eax, dword ptr [esp + 8]; mov ecx, dword ptr [eax + 4]",
    ),
    (
        "result_tentative_nil",
        0x00792BBB,
        "cmp eax, 5; je 0x792bce; cmp eax, 6; jne 0x792bce; mov ecx, dword ptr [esp + 8]",
    ),
    ("result_bool_true", 0x007900D0, "mov byte ptr [ecx], 3"),
    ("result_bool_false", 0x007900D5, "mov byte ptr [ecx], 4"),
    ("result_nil_tag", 0x0079248C, "mov byte ptr [edx], 5"),
    (
        "result_float_tag",
        0x00791640,
        "mov eax, dword ptr [esp + 4]; mov byte ptr [eax], 1",
    ),
    (
        "result_actor_tag",
        0x00791800,
        "mov eax, dword ptr [esp + 4]; mov byte ptr [eax], 6",
    ),
    ("result_string_call", 0x007924E7, "call 0x7916a0"),
    ("result_string_tag", 0x00791708, "mov byte ptr [esi], 2"),
    (
        "result_string_truncate",
        0x00791762,
        "call 0x445e50; sub eax, edi; push eax; lea ecx, [esp + 0x1c]; call 0x448b70; mov byte ptr [esp + 0xc8], 0",
    ),
    ("variant_int", 0x00583E09, "mov byte ptr [esi + 0xc], 0"),
    ("variant_bool", 0x00583EA9, "mov byte ptr [esi + 0xc], 2"),
    ("variant_string", 0x00583F4D, "mov byte ptr [esi + 0xc], 3"),
    ("variant_float", 0x0073E089, "mov byte ptr [esi + 0xc], 1"),
    ("variant_control", 0x0073DE49, "mov byte ptr [esi + 0xc], 4"),
    ("variant_table", 0x00CFCABD, "mov byte ptr [edi + 0xc], 7"),
    ("variant_nil", 0x00583FD9, "mov byte ptr [esi + 0xc], 6"),
    ("variant_next", 0x00584069, "mov byte ptr [esi + 0xc], 7"),
    ("float_operator_value_path", 0x00CFAE92, "call 0x73e010"),
    ("control_operator_value_path", 0x00CFB4A3, "call 0x73ddd0"),
    ("table_operator_value_path", 0x00CFB701, "call 0xcfca70"),
    (
        "int_operator_truncation",
        0x00CFACB6,
        "mov eax, dword ptr [esi]; push eax; mov ecx, edi; call 0xcf35c0; call 0x9d6600",
    ),
    ("int_predicate_type", 0x00CF367F, "call 0xdcdce0; add esp, 8; cmp eax, 3"),
    (
        "int_predicate_integral",
        0x00CF369A,
        "call 0xdcded0; fld st(0); add esp, 8; call 0x9d6636; mov dword ptr [esp + 8], eax; mov dword ptr [esp + 0xc], edx; fild qword ptr [esp + 8]; fucompp; fnstsw ax; test ah, 0x44",
    ),
    (
        "float_predicate",
        0x00CF3840,
        "mov eax, dword ptr [esp + 4]; mov ecx, dword ptr [ecx]; push eax; push ecx; call 0xdcdce0; xor edx, edx; add esp, 8; cmp eax, 3; sete dl",
    ),
    (
        "chooser_candidate_loop",
        0x00CFC5A9,
        "mov eax, dword ptr [esi]; mov ecx, dword ptr [eax + 4]; mov eax, dword ptr [ebp]; mov edx, dword ptr [ecx]; mov edx, dword ptr [edx + 0x14]; push eax; push ebx; call edx; test al, al; jne 0xcfc5cd; cmp esi, dword ptr [edi + 8]",
    ),
    (
        "chooser_nil_first",
        0x00CFC514,
        "push eax; mov ecx, ebx; call 0xcf3760; test al, al",
    ),
    ("int_operator_nil_path", 0x00CFAC99, "call 0x583f70"),
    ("int_operator_value_path", 0x00CFACE7, "call 0x583d90"),
    (
        "error_result_branch",
        0x00CCF08E,
        "mov ecx, edi; call 0xcf39c0; test al, al; jne 0xccf1c4; cmp byte ptr [esp + 0x16], al; jne 0xccf1b9",
    ),
    ("error_result_store", 0x00CCF04D, "mov byte ptr [esp + 0x2e], al"),
    (
        "status_is_yield",
        0x00CF39C0,
        "mov eax, dword ptr [ecx]; push eax; call 0xdceab0; sub eax, 1; add esp, 4; neg eax; sbb eax, eax; add eax, 1",
    ),
    ("error_path_finish_call", 0x00CCF1B9, "mov ecx, esi; call 0xcce8e0"),
    (
        "finish_phase0_dispatch",
        0x00CCE9B1,
        "push 0; mov ecx, esi; call 0xccd910",
    ),
    (
        "dispatch_phase0_slot1",
        0x00CCDA53,
        "cmp esi, dword ptr [edi + 4]; jne 0xccda5d; call 0x9d22b4; mov ecx, dword ptr [esi + 0xc]; mov edx, dword ptr [ecx]; mov edx, dword ptr [edx + 4]",
    ),
    (
        "dispatch_phase2_slot3",
        0x00CCDA03,
        "cmp esi, dword ptr [edi + 4]; jne 0xccda0d; call 0x9d22b4; mov ecx, dword ptr [esi + 0xc]; mov edx, dword ptr [ecx]; mov edx, dword ptr [edx + 0xc]",
    ),
    ("dispatch_listener_call", 0x00CCDA75, "call edx"),
    ("phase2_dispatch", 0x00CCDBF7, "push 2; mov ecx, esi; call 0xccd910"),
    (
        "error_path_phase2_call",
        0x00CCF23B,
        "mov bl, byte ptr [esp + 0x16]; test bl, bl; je 0xccf259; mov ecx, esi; call 0xccdbc0",
    ),
    ("count_compare", 0x00CCF173, "cmp dword ptr [esp + 0x18], eax; jg 0xccf1a5"),
    (
        "handler_report_sets_byte",
        0x00CCF199,
        "mov ecx, ebx; call 0xcd4970; mov byte ptr [esp + 0x16], 1",
    ),
    (
        "handler_report_slot_0x30",
        0x00CD49D8,
        "mov edx, dword ptr [esi]; mov edx, dword ptr [edx + 0x30]; push ecx; push edi; push eax; mov ecx, esi; mov byte ptr [esp + 0xc8], 1; call edx",
    ),
    (
        "listener_register",
        0x00CD0940,
        "mov eax, dword ptr [esp + 0x10]; push esi; mov esi, ecx; mov ecx, dword ptr [esi]; push eax; call 0xccdda0",
    ),
    (
        "owner_registered",
        0x0089718D,
        "push esi; lea ecx, [esp + 0x64]; push ecx; push edx; push ebp; lea ecx, [esp + 0x30]; call 0xcd0940",
    ),
    (
        "player_manager_vtable",
        0x00895F85,
        "mov eax, dword ptr [esp + 0x20]; mov dword ptr [esi], 0x105706c",
    ),
    (
        "listener_first_word",
        0x008960DA,
        "mov cx, word ptr [eax]; cmp cx, word ptr [0x134bc18]",
    ),
    (
        "listener_first_target",
        0x008960FB,
        "mov edx, dword ptr [esp + 0x30]; mov ecx, dword ptr [esp + 0x18]; push edx; call 0x893ab0",
    ),
    (
        "listener_second_word",
        0x00896132,
        "mov cx, word ptr [eax]; cmp cx, word ptr [0x134bc1c]",
    ),
    (
        "listener_second_target",
        0x00896157,
        "mov edx, dword ptr [esp + 0x30]; mov ecx, dword ptr [esp + 0x18]; push eax; push edx; call 0x894ab0",
    ),
    ("listener_slot3_noop", 0x00892870, "ret 0x10"),
    (
        "wrapper_selector",
        0x00CD0087,
        "cmp byte ptr [esp + 0x58], bl; mov byte ptr [esp + 0x40], 1; je 0xcd00a3",
    ),
    (
        "pcall_error_function_zero",
        0x00CF4096,
        "mov edx, dword ptr [esi]; push 0; push ecx; push edi; push edx; call 0xdce900",
    ),
    (
        "resume_call",
        0x00CF42AF,
        "mov ecx, dword ptr [esi]; push edi; push ecx; call 0xdcfd30; add esp, 8; sub eax, 2; je 0xcf43a2; sub eax, 2; je 0xcf4341; sub eax, 1; jne 0xcf441b",
    ),
    (
        "resume_err_text",
        0x00CF42D2,
        "mov edx, dword ptr [0xf67298]; push edx; push 0x110eb18",
    ),
    (
        "resume_run_text",
        0x00CF43A2,
        "mov eax, dword ptr [0xf67298]; push eax; push 0x110eaf8",
    ),
    (
        "resume_memory_text",
        0x00CF4341,
        "mov ecx, dword ptr [0xf67298]; push ecx; push 0x110eb08",
    ),
    (
        "actor_result_item_branch",
        0x0075AFEB,
        "test al, al; je 0x75b015; push 0; push 0x12c0864; push 0x1270b4c; push 0; push edi; call 0x9da6cc",
    ),
)

# VA, expected bytes
DATA: tuple[tuple[str, int, bytes], ...] = (
    ("trigger_group_byte", 0x0130D426, b"\x0f"),
    ("refusal_kind_byte", 0x012C3F72, b"\x01"),
    ("talk_kind_byte", 0x012C3F7A, b"\x01"),
)

LITERALS: tuple[tuple[int, str], ...] = (
    (0x00FD87D4, "[clientversionerror]"),
    (0x00FD87EC, "[servererror]"),
    (0x00FDF528, " ***** Client Script ERROR *****"),
    (0x0110E510, "stack trace:\n"),
    (0x0110EACC, "runtime error"),
    (0x0110EADC, "memory error"),
    (0x0110EAEC, "err error"),
    (0x0110EAF8, "runtime error"),
    (0x0110EB08, "memory error"),
    (0x0110EB18, "err error"),
)

# target -> direct callers (E8 and E9)
CALLERS: dict[int, list[int]] = {
    0x00894090: [0x00894B65, 0x00894C7C, 0x008970ED, 0x0089722B],
    0x00790CE0: [0x00894B44],
    0x0075E670: [0x0089410A],
    0x006E1140: [0x0089E2BE],
    0x00896F70: [0x006E114A],
    0x0076E270: [0x0076EA44, 0x0076F239, 0x0076F299, 0x0076F329, 0x0076F52E],
    0x00CD3E70: [
        0x00CCEBF9,
        0x00CCF4F4,
        0x00CD00D7,
        0x00CD7F27,
        0x00CDA7BE,
        0x00D0C2B1,
        0x00D0CD8B,
        0x00D2AC85,
        0x00D2ACFC,
    ],
    0x00CD3990: [
        0x00CCEC24,
        0x00CCF51F,
        0x00CD00FF,
        0x00CD7EFF,
        0x00CDA7E6,
        0x00D0C287,
        0x00D0CBD7,
        0x00D0CD61,
    ],
    0x00CD4080: [
        0x00CCEC4F,
        0x00CCF54A,
        0x00CD0127,
        0x00CD7F4F,
        0x00CDA80E,
        0x00D0C2DB,
        0x00D0CDB5,
    ],
    0x00CCFFE0: [0x00CCED4D, 0x00CCF042],
    0x00CC9830: [0x00763623],
    0x00894AB0: [0x00896161],
    0x0076F120: [
        0x0076F256,
        0x0076F2B6,
        0x0076F2F0,
        0x0076F346,
        0x0076F551,
        0x0076F590,
    ],
}

# vtable VA, RTTI name, slot index -> target
CLIENT_HANDLER = (0x00FE0184, ".?AVErrorHandler@Client@Script@Lua@Application@@")
CLIENT_HANDLER_SLOTS = {
    0: 0x0076F210,
    1: 0x0076F270,
    2: 0x0076F2D0,
    3: 0x0076F300,
    4: 0x0076F360,
    5: 0x0076F570,
}
PLAYER_MANAGER = (0x0105706C, ".?AVPlayerManager@Event@Client@Script@Lua@Application@@")
PLAYER_MANAGER_SLOTS = {1: 0x00896090, 3: 0x00892870}
BASE_HANDLER = (0x0110E564, ".?AVErrorHandler@GameEngine@Lua@Component@@")
BASE_HANDLER_SLOT = 4
RET_4 = bytes.fromhex("C20400")


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


def check_data(image: ecr.Image) -> list[str]:
    errors = []
    for label, va, want in DATA:
        if image.read(va, len(want)) != want:
            errors.append(f"data {label} at {va:#010x} is not {want.hex()}")
    for va, text in LITERALS:
        if image.cstr(va) != text:
            errors.append(f"literal at {va:#010x} is not {text!r}")
    return errors


ZERO_FILL_CELLS = (0x0134C3FC, 0x0134BC18, 0x0134BC1C)


def check_zero_fill(image: ecr.Image) -> list[str]:
    """The listed cells lie past the raw data of their section, so they have no
    image value."""
    errors = []
    for cell in ZERO_FILL_CELLS:
        rva = cell - image.base
        for _, vsize, address, rsize, _ in image.sections:
            if address <= rva < address + vsize:
                if rva < address + rsize:
                    errors.append(f"{cell:#010x} has an image value")
                break
        else:
            errors.append(f"{cell:#010x} is outside every section")
    return errors


def check_callers(image: ecr.Image) -> list[str]:
    found = ecr.find_callers(image, list(CALLERS))
    return [
        f"callers of {target:#010x} are {[hex(s) for s in found[target]]}"
        for target, want in CALLERS.items()
        if found[target] != want
    ]


def check_vtables(image: ecr.Image) -> list[str]:
    errors = []
    for vtable, rtti in (CLIENT_HANDLER, PLAYER_MANAGER, BASE_HANDLER):
        locator = struct.unpack("<I", image.read(vtable - 4, 4))[0]
        descriptor = struct.unpack("<I", image.read(locator + 12, 4))[0]
        if image.cstr(descriptor + 8) != rtti:
            errors.append(f"vtable {vtable:#010x} is not {rtti}")
    for slot, want in CLIENT_HANDLER_SLOTS.items():
        got = struct.unpack("<I", image.read(CLIENT_HANDLER[0] + 4 * slot, 4))[0]
        if got != want:
            errors.append(f"Client handler slot {slot} is {got:#010x}")
    for slot, want in PLAYER_MANAGER_SLOTS.items():
        got = struct.unpack("<I", image.read(PLAYER_MANAGER[0] + 4 * slot, 4))[0]
        if got != want:
            errors.append(f"PlayerManager slot {slot} is {got:#010x}")
    base = struct.unpack("<I", image.read(BASE_HANDLER[0] + 4 * BASE_HANDLER_SLOT, 4))[
        0
    ]
    if image.read(base, 3) != RET_4:
        errors.append("base handler slot 4 is not ret 4")
    return errors


def verify(image: ecr.Image) -> list[str]:
    return (
        check_listings(image)
        + check_data(image)
        + check_zero_fill(image)
        + check_callers(image)
        + check_vtables(image)
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
        f"{len(CHECKS)} listings, {len(DATA) + len(LITERALS)} data rows, "
        f"{len(CALLERS)} caller sets, {len(CLIENT_HANDLER_SLOTS) + len(PLAYER_MANAGER_SLOTS) + 1} handler slots: "
        f"{'FAIL' if errors else 'ok'}"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
