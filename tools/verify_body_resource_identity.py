"""Check pinned body appearance, request and resource identity boundaries.

Requires pefile, Capstone and Unicorn 2.1.4. All native objects are synthetic;
separate fragments do not identify an actual actor or load an actual scene.
Output must name a new JSON file.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct

import capstone
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX,
    UC_X86_REG_EBX,
    UC_X86_REG_EBP,
    UC_X86_REG_ECX,
    UC_X86_REG_ESP,
    UC_X86_REG_EIP,
)

from verify_body_shader_join import PIN, ASSET_PIN
from verify_body_mesh_admission import selected_asset

BASE = 0x400000

CAPTURE_SHA256 = "5A24B2E2FD1A85CB6615588A8322A56323EB9A99BC7680E606B4A0DB07BDD3E7"
HEADER = struct.Struct("<23I")
CORE = struct.Struct("<295I")
PLACEMENT = struct.Struct("<125I")


def native_anchors(pe):
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    anchors = [
        (0x6623F8, "lea", "edi, [eax + 0x13c8]"),
        (0x6623FE, "mov", "ecx, 0x1d"),
        (0x662403, "rep movsd", "dword ptr es:[edi], dword ptr [esi]"),
        (0x662407, "call", "0x65d730"),
        (0x65D856, "cmp", "dword ptr [ecx + 0x138c], 0"),
        (0x65D85F, "mov", "eax, dword ptr [ecx + 0x1400]"),
        (0x65D865, "mov", "dword ptr [ecx + 0x138c], eax"),
        (0x65D9C2, "mov", "dword ptr [ecx + 0x2b70], eax"),
        (0x665EF0, "mov", "ecx, 0x1d"),
        (0x665EFB, "rep movsd", "dword ptr es:[edi], dword ptr [esi]"),
        (0x665EFD, "mov", "ecx, dword ptr [ebx + 0x2b5c]"),
        (0x665F0D, "call", "eax"),
        (0x6B7898, "mov", "dword ptr [esi + 0x5c], ecx"),
        (0x6B78FB, "mov", "dword ptr [esi + 0x9c], ecx"),
        (0x6B79D1, "mov", "ecx, dword ptr [esi + 0x58]"),
        (0x6320DA, "call", "0x445db0"),
        (0x6320E1, "je", "0x6321d7"),
        (0x6320F1, "call", "0x447450"),
        (0x6320F6, "mov", "eax, dword ptr [esi + 0x58]"),
        (0x632126, "mov", "dword ptr [esi + 0x68], eax"),
        (0x6321D2, "call", "eax"),
        (0x6321D4, "mov", "dword ptr [esi + 0x58], eax"),
        (0x60B250, "mov", "eax, dword ptr [ecx + 0x50]"),
        (0x631DEB, "call", "edx"),
        (0x631DED, "test", "eax, eax"),
        (0x7CB90C, "call", "edx"),
        (0x7CBCE4, "call", "edx"),
        (0x7CBD22, "call", "0xa63760"),
        (0x7CBD27, "test", "eax, eax"),
        (0x7CBD3E, "mov", "eax, edi"),
        (0x7CC9CB, "mov", "dword ptr [esi], 0xfe981c"),
        (0x7CC9D1, "mov", "dword ptr [esi + 0xc], 0xfe980c"),
        (0xA63797, "mov", "byte ptr [ebx + 0x14], 1"),
        (0xA637BE, "call", "0x9d4600"),
        (0xA637E0, "call", "edx"),
        (0xA637F8, "call", "edx"),
        (0xC9933A, "call", "0x44ab90"),
        (0xC993DE, "call", "0xcaedd0"),
        (0xCAEE11, "call", "0x447200"),
        (0xC99437, "call", "0x8edda0"),
        (0xC962E7, "call", "0x453030"),
        (0xC962EC, "cmp", "dword ptr [esi + 0x5c], eax"),
        (0xC962F5, "call", "0x453000"),
        (0xC96307, "call", "dword ptr [0xf3e148]"),
        (0xA445BE, "call", "eax"),
        (0xC49FA2, "call", "0xc771b0"),
        (0xC4A03B, "call", "0xc771b0"),
        (0xC77187, "call", "0x9d4600"),
        (0xC79844, "call", "0x9d4600"),
        (0x7CF090, "mov", "dword ptr [esi + 0x114], ecx"),
    ]
    for va, mnemonic, operands in anchors:
        instruction = next(decoder.disasm(pe.get_data(va - BASE, 15), va))
        assert (instruction.mnemonic, instruction.op_str) == (mnemonic, operands), hex(
            va
        )
    assert struct.unpack("<I", pe.get_data(0xFD3ED4 + 25 * 4 - BASE, 4))[0] == 0x6B7840
    tables = {
        0xFE96FC: {2: 0xA642F0, 19: 0x7CC660, 21: 0x7CBCC0, 22: 0x7CB8D0},
        0x1108C3C: {2: 0xC992D0},
        0x110C4C0: {6: 0xCAE670, 10: 0xCAE660},
        0x10F2BC8: {1: 0xC49F30, 3: 0xC49E20},
        0x10F2C64: {1: 0xC4A160},
    }
    for table, slots in tables.items():
        for slot, target in slots.items():
            assert (
                struct.unpack("<I", pe.get_data(table + slot * 4 - BASE, 4))[0]
                == target
            )
    return dict(
        instruction_anchors=len(anchors),
        table_slots=sum(map(len, tables.values())) + 1,
        visual_slot25="0x006B7840",
    )


def machine(image):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(BASE, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(BASE, image)
    uc.mem_map(0, 0x1000)
    uc.mem_map(0x2000000, 0x10000)
    uc.mem_map(0x3000000, 0x10000)
    return uc


def appearance_replay(image):
    results = []
    for old, requested in ((0, 0xC00), (0xC00, 0x1000), (0x1000, 0xC00)):
        uc = machine(image)
        actor, payload, visual, stop, stack = (
            0x2000000,
            0x2004000,
            0x2005000,
            0x200F000,
            0x3008000,
        )

        def put(address, *values):
            uc.mem_write(address, struct.pack("<" + "I" * len(values), *values))

        def get(address):
            return struct.unpack("<I", uc.mem_read(address, 4))[0]

        words = [0x12000000 + i for i in range(29)]
        words[0], words[14], words[28] = 1, requested, 4
        put(payload, *words)
        put(actor + 0x1354, int(bool(old)))
        put(actor + 0x138C, old)
        put(actor + 0x2B5C, visual)
        put(visual, 0xFD3ED4)
        put(visual + 0x5C, 1)
        put(visual + 0x9C, old)
        put(stack, stop, payload)
        uc.reg_write(UC_X86_REG_ECX, actor)
        uc.reg_write(UC_X86_REG_ESP, stack)
        uc.emu_start(0x6623F0, stop, count=2000)
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 8
        assert uc.mem_read(actor + 0x13C8, 116) == uc.mem_read(payload, 116)
        base = old or requested
        assert get(actor + 0x1354) == 1 and get(actor + 0x138C) == base
        assert get(actor + 0x2B70) & 0xF == 1
        assert get(visual + 0x9C) == old
        # Separately supply a ready record at the real apply-copy interior.
        uc.reg_write(UC_X86_REG_EBX, actor)
        uc.reg_write(UC_X86_REG_EBP, payload)
        uc.reg_write(UC_X86_REG_ESP, stack)
        visited = set()

        def hook(_uc, address, _size, _user):
            visited.add(address)
            if address == 0x6B79D1:
                uc.emu_stop()

        uc.hook_add(UC_HOOK_CODE, hook)
        uc.emu_start(0x665EF0, 0, count=2000)
        assert uc.reg_read(UC_X86_REG_EIP) == 0x6B79D1
        assert 0x6B7840 in visited
        assert get(visual + 0x5C) == 1 and get(visual + 0x9C) == requested
        assert get(visual + 0x54) == 0 and get(visual + 0x58) == 1
        assert get(visual + 0x40) & 0x6000 == 0x4000
        results.append(
            dict(
                old_word=old,
                requested_word=requested,
                staged_base_word=base,
                active_before_apply=old,
                active_after_apply=requested,
                base_category=0,
                base_number=1,
            )
        )
    return results


def request_replay(image):
    results = []
    path = "/client/chara/pc/c001/equ/e003/top_mdl/0001"
    for equal in (False, True):
        uc = machine(image)
        visual, records, owner, world, module, table = (
            0x2000000,
            0x2001000,
            0x2003000,
            0x2004000,
            0x2005000,
            0x2006000,
        )
        incoming, incoming_bytes, old_bytes = 0x2007000, 0x2008000, 0x2009000
        stop, request_stub, handle, old_root = (
            0x200F000,
            0x200F010,
            0x200A000,
            0x200B000,
        )
        record, stack = records + 5 * 0x6C, 0x3008000

        def put(address, *values):
            uc.mem_write(address, struct.pack("<" + "I" * len(values), *values))

        def get(address):
            return struct.unpack("<I", uc.mem_read(address, 4))[0]

        def string(obj, address, value):
            uc.mem_write(address, value.encode("ascii") + b"\0")
            put(obj, address)
            put(obj + 8, len(value) + 1)

        string(incoming, incoming_bytes, path)
        string(record + 4, old_bytes, path if equal else "/previous/resource")
        put(visual + 8, owner)
        put(visual + 0x18, records)
        put(visual + 0x1C, 1)
        put(owner + 0x114, world)
        put(world + 0x50, module)
        put(module, table)
        put(table + 8, request_stub)
        put(record + 0x5C, old_root)
        put(0x1376038, 0)
        calls = []

        def ret(value, pop):
            esp = uc.reg_read(UC_X86_REG_ESP)
            target = get(esp)
            uc.reg_write(UC_X86_REG_EAX, value)
            uc.reg_write(UC_X86_REG_ESP, esp + 4 + pop)
            uc.reg_write(UC_X86_REG_EIP, target)

        def hook(_uc, address, _size, _user):
            esp = uc.reg_read(UC_X86_REG_ESP)
            if address == 0x447450:
                assert uc.reg_read(UC_X86_REG_ECX) == record + 4
                assert get(esp + 4) == incoming
                # String copying is represented by a stable borrowed fixture buffer.
                put(record + 4, get(incoming))
                put(record + 12, get(incoming + 8))
                ret(record + 4, 4)
            elif address == request_stub:
                assert uc.reg_read(UC_X86_REG_ECX) == module
                args = [get(esp + 4 * i) for i in range(1, 10)]
                assert args == [incoming, 0, 0, 0, visual, 5, 0, 0, 1]
                assert get(record + 0x5C) == old_root
                calls.append(args)
                ret(handle, 36)

        uc.hook_add(UC_HOOK_CODE, hook)
        put(stack, stop, 5, incoming, 2)
        uc.reg_write(UC_X86_REG_ECX, visual)
        uc.reg_write(UC_X86_REG_ESP, stack)
        uc.emu_start(0x6320C0, stop, count=2000)
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 16
        assert len(calls) == int(not equal)
        assert get(record + 0x58) == (0 if equal else handle)
        assert get(record + 0x5C) == old_root and get(visual + 0x1C) == 1
        assert get(record + 4) == (old_bytes if equal else incoming_bytes)
        if not equal:
            assert get(record + 0x68) == 2 and get(record) == 0xFFFFFFFF
        results.append(
            dict(
                paths_equal=equal,
                requests=len(calls),
                old_root_preserved=True,
                pending_handle_nonnull=not equal,
                maintained_count=1,
            )
        )
    return results


def inspect_accepted_v5(data):
    if not __debug__:
        raise ValueError("optimized Python is unsupported")
    if hashlib.sha256(data).hexdigest().upper() != CAPTURE_SHA256:
        raise ValueError("accepted capture SHA-256 mismatch")
    h = HEADER.unpack_from(data)
    assert h[0:3] == (0x434C5050, 5, CORE.size)
    assert h[3] == 131072 and h[4] == h[5] == 43166
    assert h[6] == h[7] == 0 and h[10] == 1
    assert h[12] == 155 and h[17] == 23884
    assert h[19] == PLACEMENT.size and h[20] == h[22] == 1199 and h[21] == 4096
    tail = HEADER.size + h[5] * CORE.size
    assert len(data) == tail + h[20] * PLACEMENT.size
    records = [CORE.unpack_from(data, HEADER.size + i * CORE.size) for i in range(h[5])]
    identities, models, drawables, events = Counter(), Counter(), Counter(), Counter()
    for i, r in enumerate(records):
        assert r[0] == 1 and r[2] == i
        identities[(r[8], r[9], r[10])] += 1
        events[r[1]] += 1
        if r[27]:
            models[r[27]] += 1
        if r[34]:
            drawables[r[34]] += 1
    assert identities == {(1000476, 0x2EE90048, 155): 43166}
    assert models == {0x297FF450: 1199} and drawables == {0x2D016200: 1199}
    assert events[10] == events[11] == h[20]
    keys, owners = Counter(), Counter()
    last_entry = -1
    for i in range(h[20]):
        p = PLACEMENT.unpack_from(data, tail + i * PLACEMENT.size)
        assert p[0] == 1 and p[1] == i
        assert last_entry < p[7] < p[8] < h[5]
        last_entry = p[7]
        e, t = records[p[7]], records[p[8]]
        assert e[1] == 10 and t[1] == 11
        for r in (e, t):
            assert (r[3], r[4], r[9], r[59], r[18]) == (p[2], p[3], p[4], p[5], p[6])
        assert p[4] == 0x2EE90048 and p[14] == p[15] == 14 and p[16] == 0
        # Producer fields occupy placement words 22:47 (100 bytes).
        assert p[25] == 0x007F91D2
        assert p[26:31] == (0x29800340, 0x00FF1CC4, 0x29805AB0, 1000476, 0xFFFFFFFF)
        assert p[29] == e[8]
        assert p[22] == p[50] == p[17] != 0
        assert p[24] == p[2] and p[23] <= p[3]
        assert 0 < e[22] <= 16 and p[28] in e[279 : 279 + e[22]]
        # First glyph fields occupy placement words 47:125 (312 bytes).
        assert p[47:50] == (0x00CA565E, 1, 6)
        assert p[51] == p[28] and p[52] == p[26]
        assert p[53] == 0x0130BBC4
        assert p[39:43] == p[43:47] == p[65:69] == p[18:22] == p[35:39]
        assert p[62:65] == p[65:68]
        keys[p[29:31]] += 1
        owners[p[26:29]] += 1
    return {
        "sha256": CAPTURE_SHA256,
        "bytes": len(data),
        "core_records": h[5],
        "placements": h[20],
        "main_actor_address": "0x2EE90048",
        "snapshot_actor_id": 1000476,
        "primary_key": 1000476,
        "secondary_key": "0xFFFFFFFF",
        "window_actor": "0x29800340",
        "window_actor_vtable": "0x00FF1CC4",
        "ui_context_root": "0x29805AB0",
        "key_join_count": sum(keys.values()),
        "owner_join_count": sum(owners.values()),
        "model_candidate": "0x297FF450",
        "model_candidate_records": 1199,
        "drawable_candidate": "0x2D016200",
        "drawable_candidate_records": 1199,
        "recorded_glyph_calls": 1199 * 14,
        "limit": "No Scene lookup result/type, actual appearance, bank-5 request/root or loaded-byte identity is established by these fields.",
    }


def factory_replay(image, asset, mode):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    im = image
    u.mem_map(BASE, (len(im) + 4095) & ~4095)
    u.mem_write(BASE, im)
    u.mem_map(0, 0x1000)
    u.mem_map(0x2000000, 0x100000)
    u.mem_map(0x3000000, 0x1000000)

    def put(p, v):
        u.mem_write(p, struct.pack("<I", v & 0xFFFFFFFF))

    def get(p):
        return struct.unpack("<I", u.mem_read(p, 4))[0]

    def ret(v=0, n=0):
        sp = u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EAX, v)
        u.reg_write(UC_X86_REG_EIP, get(sp))
        u.reg_write(UC_X86_REG_ESP, sp + 4 + n)

    def args(n):
        return [get(u.reg_read(UC_X86_REG_ESP) + 4 + 4 * i) for i in range(n)]

    mod = 0x3000000
    parent = 0x3000100
    lock = 0x3000200
    lockvt = 0x3000300
    alloc = 0x3000400
    allocvt = 0x3000500
    root = 0x3000800
    handle = 0x3001000
    memvt = 0x3002000
    data = 0x3100000
    src = 0x3200000
    path = 0x3003000
    name = 0x3003100
    stop = 0x3FFF000
    stublock = 0x3FFF010
    stuballoc = 0x3FFF020
    stubrelease = 0x3FFF030
    stubptr = 0x3FFF040
    stubview = 0x3FFF050
    put(mod, 0xFE96FC)
    put(mod + 8, parent)
    put(mod + 0x44, lock)
    put(lock, lockvt)
    put(lockvt + 0x10, stublock)
    put(0x1327FC0, alloc)
    put(alloc, allocvt)
    put(allocvt + 0xC, stuballoc)
    put(allocvt + 0x10, stubrelease)
    put(0x1327C24, alloc)
    put(memvt + 4, stubptr)
    put(memvt + 0x24, stubview)
    put(handle, memvt)
    put(handle + 4, data)
    put(handle + 8, len(asset))
    put(0x137B8E8, 0)
    put(0x13654E4, 0)
    u.mem_write(src, asset)
    u.mem_write(path, b"/client/chara/pc/c001/equ/e003/top_mdl/0001\0")
    u.mem_write(name, b"0001\0")
    sp = 0x20FF000
    u.mem_write(sp, struct.pack("<6I", stop, path, name, src, len(asset), parent))
    u.reg_write(UC_X86_REG_ESP, sp)
    u.reg_write(UC_X86_REG_ECX, mod)
    result = {
        "mode": mode,
        "stubbed_services": [
            "root allocation0040E110",
            "root base initialization007CC920",
            "root display-name setter00A3DC20",
            "critical-section slot+10",
            "memory allocation/release and memory-view pointer/slicing interface",
        ],
        "native_byte_copy": False,
    }
    views = []
    steps = 0

    def hook(uc, at, size, user):
        nonlocal steps
        steps += 1
        if steps > 100000:
            raise AssertionError("instruction budget")
        if at == stop:
            u.emu_stop()
            return
        if at == stublock:
            ret()
            return
        if at == 0x40E110:
            assert args(2)[0] == 0x40
            ret(root, 8)
            return
        if at == 0x7CC920:
            ret(root)
            return
        if at == 0xA3DC20:
            ret(0, 4)
            return
        if at == stuballoc:
            assert args(3) == [len(asset), 4, 0]
            ret(handle, 12)
            return
        if at == stubrelease:
            result["temporary_released"] = args(1)[0] == handle
            ret(0, 4)
            return
        if at == stubptr:
            ret(get(u.reg_read(UC_X86_REG_ECX) + 4))
            return
        if at == stubview:
            off, L, align, last = args(4)
            h = 0x3004000 + len(views) * 0x20
            p = get(u.reg_read(UC_X86_REG_ECX) + 4)
            assert off + L <= len(asset)
            put(h, memvt)
            put(h + 4, p + off)
            put(h + 8, L)
            views.append({"offset": off, "length": L, "alignment": align})
            ret(h, 16)
            return
        if at == 0xA642F0:
            ar = args(6)
            assert ar[2:5] == [handle, len(asset), root]
            ctx = ar[5]
            assert bytes(u.mem_read(ctx + 0x14, 1)) == b"\1"
            assert bytes(u.mem_read(data, len(asset))) == asset
            assert (
                get(root) == 0xFE981C
                and get(root + 0x30) == 0x66697274
                and get(root + 0x14) == parent
            )
            result["native_byte_copy"] = True
            result["root_type"] = "Application::Scene::RaptureResourceFileRoot"
            result["root_native_vtable"] = "00FE981C"
            result["context_raw_flag"] = 1
            if mode == "wrapper_return":
                result["parser_stubbed"] = True
                ret(root + 0x100, 24)
                return
        if at == 0xE0D4C0:
            view, ctx = args(2)
            p = get(view + 4)
            L = get(view + 8)
            assert p == data + 16 and L == len(asset) - 16
            assert bytes(u.mem_read(p, L)) == asset[16:]
            result["first_span_native_selected"] = True
            result["first_span_sha256"] = hashlib.sha256(asset[16:]).hexdigest()
            result["stopped_before_reader_constructor"] = True
            u.emu_stop()

    u.hook_add(UC_HOOK_CODE, hook)
    u.emu_start(0x7CB8D0, stop + 1, count=100000)
    result["steps"] = steps
    result["views"] = views
    if mode == "wrapper_return":
        assert u.reg_read(UC_X86_REG_EAX) == root and result.get("temporary_released")
        result["returned_same_outer_root"] = True
    else:
        assert result.get("first_span_native_selected")
    return result


def body_frames(data):
    """Check the selected two RES directories and consumed metadata prefixes."""
    assert data[:4] == b"PWIB"
    total, first, second = struct.unpack_from(">3I", data, 4)
    assert (total, first, second) == (len(data), 16, len(data))

    def frame(start, length):
        end = start + length
        assert 0 <= start < end <= len(data) and data[start : start + 8] == b"SEDBRES "
        assert data[start + 12] == 0
        count, name_offset, name_count, _ = struct.unpack_from(
            "<4I", data, start + 0x30
        )
        payload = start + 0x40 + 16 * count
        assert payload <= end
        cursor = payload + name_offset
        names = []
        for _ in range(name_count):
            terminator = data.index(0, cursor, end)
            names.append(data[cursor:terminator].decode("ascii"))
            cursor = terminator + 1
        rows = []
        for i in range(count):
            name_index, offset, size, _ = struct.unpack_from(
                "<4I", data, start + 0x40 + 16 * i
            )
            assert name_index < len(names)
            name = names[name_index]
            at = payload + offset
            if name not in ("RESOURCE_TYPE", "RESOURCE_ID"):
                assert start <= at <= at + size <= end
            rows.append(
                dict(
                    name=name, start=at, length=size, declared_in_frame=at + size <= end
                )
            )
        for name, width in (("RESOURCE_TYPE", 4), ("RESOURCE_ID", 16)):
            row = next(row for row in rows if row["name"] == name)
            assert start <= row["start"] and row["start"] + count * width <= end
        type_start = next(
            row["start"] for row in rows if row["name"] == "RESOURCE_TYPE"
        )
        for i, row in enumerate(rows):
            row["type"] = struct.unpack_from("<I", data, type_start + 4 * i)[0]
        return rows

    outer = frame(first, second - first)
    assert outer[0]["start"] == 144 and outer[0]["type"] == 0x747262
    nested = frame(outer[0]["start"], outer[0]["length"])
    assert [(row["start"], row["length"], row["type"]) for row in nested[:4]] == [
        (320, 15673, 0x73647262),
        (15996, 15673, 0x73647262),
        (31672, 15673, 0x73647262),
        (47348, 58928, 0x777262),
    ]
    assert not outer[3]["declared_in_frame"]
    return dict(
        pwib_first=[first, second],
        pwib_second=[second, total],
        outer=outer,
        nested=nested,
        boundary="Selected normal entries and consumed type/ID/name prefixes fit; full metadata declared extents are not validated.",
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "asset", "capture", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if not __debug__:
        parser.error("run without Python optimization")
    if args.output.exists():
        parser.error("--output must name a new file")
    raw, asset = args.exe.read_bytes(), args.asset.read_bytes()
    assert len(raw) == 15996808 and hashlib.sha256(raw).hexdigest().upper() == PIN
    assert (
        len(asset) == 107152 and hashlib.sha256(asset).hexdigest().upper() == ASSET_PIN
    )
    pe = pefile.PE(data=raw, fast_load=True)
    assert pe.FILE_HEADER.Machine == 0x14C and pe.OPTIONAL_HEADER.ImageBase == BASE
    image = pe.get_memory_mapped_image()
    result = dict(
        exe_sha256=PIN,
        asset_sha256=ASSET_PIN,
        native=native_anchors(pe),
        selected_body=selected_asset(asset),
        framing=body_frames(asset),
        capture=inspect_accepted_v5(args.capture.read_bytes()),
        factory=[
            factory_replay(image, asset, mode)
            for mode in ("wrapper_return", "first_span")
        ],
        appearance=appearance_replay(image),
        request=request_replay(image),
        boundary="Separate synthetic native fragments. Appearance staging executes6623F0/65D730, then a supplied ready record enters665EF0->6B7840 and stops6B79D1; asynchronous ready-record selection is not executed. Request replay uses a String-copy stub with borrowed fixture storage and a module request stub, real445DB0/60B250 and6320C0; pending begins null, global1376038 is modeled zero. Factory cases supply FE96FC and pinned bytes, stub services/root base/name initialization, and separately stub A642F0 or stop before E0D4C0. No concrete RES child factory/loader executes. Accepted capture uses its exact v5 layout; it does not supply Scene result/type, appearance or a bank-5 root. No actual bytes-to-live-root association, draw, queue, GPU or lifetime proof.",
    )
    with args.output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    print(
        "PASS: body pins, capture key coverage, native appearance/request controls and conditional factory/PWIB copy"
    )


if __name__ == "__main__":
    main()
