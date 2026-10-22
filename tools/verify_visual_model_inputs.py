"""Check pinned visual resource inputs and native Model slot-1 forwarding.

Requires pefile, Capstone and Unicorn 2.1.4. All objects are synthetic.
Separate selector, publication, forwarding and enable fragments do not load
a scene, identify its resources or admit a draw. Output must be a new file.
"""

import argparse
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
    UC_X86_REG_ECX,
    UC_X86_REG_ESI,
    UC_X86_REG_ESP,
    UC_X86_REG_EIP,
)

from verify_body_shader_join import PIN

BASE = 0x400000


def native_anchors(pe):
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    anchors = [
        (0x665EF0, "mov", "ecx, 0x1d"),
        (0x665F0D, "call", "eax"),
        (0x6B7898, "mov", "dword ptr [esi + 0x5c], ecx"),
        (0x6B78F8, "mov", "ecx, dword ptr [eax + 0x38]"),
        (0x6B78FB, "mov", "dword ptr [esi + 0x9c], ecx"),
        (0x6B5E88, "jbe", "0x6b624f"),
        (0x6B5ECA, "call", "0x6306a0"),
        (0x6B5F1E, "call", "0x9d4f83"),
        (0x6B5F54, "call", "0x9d4f83"),
        (0x6B5FC1, "call", "0xd39290"),
        (0x6B5FEF, "je", "0x6b6238"),
        (0x6B6013, "call", "0x6320c0"),
        (0xD392A9, "call", "0xd38e50"),
        (0xD38EC6, "call", "0x447450"),
        (0xD38EE6, "call", "0x4531c0"),
        (0xD39022, "call", "dword ptr [0xf3e348]"),
        (0xD39152, "call", "dword ptr [0xf3e114]"),
        (0xD391B0, "call", "0x447450"),
        (0x6320CD, "imul", "esi, esi, 0x6c"),
        (0x6320D3, "add", "esi, dword ptr [edi + 0x18]"),
        (0x6320E1, "je", "0x6321d7"),
        (0x6320F1, "call", "0x447450"),
        (0x63211B, "mov", "dword ptr [esi + 0x58], 0"),
        (0x632126, "mov", "dword ptr [esi + 0x68], eax"),
        (0x631F4A, "mov", "dword ptr [esi + 0x14], edi"),
        (0x631FA1, "mov", "dword ptr [esi + 0x18], ebp"),
        (0x6B7636, "push", "0x39"),
        (0x631D65, "mov", "eax, dword ptr [edi]"),
        (0x632511, "mov", "eax, dword ptr [edx + 0x19c]"),
        (0x631DEB, "call", "edx"),
        (0x631DED, "test", "eax, eax"),
        (0x631DEF, "je", "0x631e00"),
        (0x631DFE, "call", "edx"),
        (0x631C84, "cmp", "byte ptr [esp + 0x210], 0"),
        (0x631C9F, "jne", "0x631cd4"),
        (0x631CAE, "call", "edx"),
        (0x631CBE, "call", "edx"),
        (0x63137A, "mov", "dword ptr [eax + ecx + 0x58], 0"),
        (0x632453, "test", "ebx, ebx"),
        (0x632455, "je", "0x63249b"),
        (0x632471, "call", "edx"),
        (0x632489, "call", "eax"),
        (0x632497, "add", "dword ptr [edi + 0x1c], 1"),
        (0x63249B, "mov", "dword ptr [esi + 0x5c], ebx"),
        (0x631330, "mov", "eax, dword ptr [ecx + 0x18]"),
        (0x631351, "mov", "eax, dword ptr [ecx + 0x1c]"),
        (0x631362, "call", "edx"),
        (0x632400, "mov", "eax, dword ptr [ecx + 0x18]"),
        (0x632420, "mov", "eax, dword ptr [ecx + 0x1c]"),
        (0x632431, "call", "edx"),
        (0x632436, "mov", "byte ptr [ecx + 0x20], 1"),
        (0x6315D3, "mov", "eax, dword ptr [esi]"),
        (0x6315E6, "jne", "0x63160f"),
        (0x631603, "call", "edx"),
        (0x6BE42B, "cmp", "eax, 3"),
        (0x6BE445, "call", "0x6bb080"),
        (0x6BB125, "mov", "ecx, dword ptr [ebx + 8]"),
        (0x6BB135, "call", "eax"),
        (0x7CD5BB, "push", "0"),
        (0xA5F6D1, "call", "edx"),
        (0xA5F6D7, "mov", "ecx, dword ptr [esi + 0xc]"),
        (0xA5F6DF, "push", "0"),
        (0xA5F6F3, "call", "eax"),
        (0xC52532, "mov", "edx, dword ptr [ebp + edi*4]"),
        (0xC52539, "call", "0xc52260"),
        (0xC522A8, "call", "0xa44bf0"),
        (0xC52315, "call", "0xa44bf0"),
        (0xA61FA5, "call", "0xc510f0"),
        (0xA62207, "or", "byte ptr [esi + 0x29c], 6"),
        (0xA62211, "cmp", "byte ptr [esp + 0x458], bl"),
        (0xA62218, "jne", "0xa6222b"),
        (0xA62222, "call", "eax"),
        (0xA62224, "and", "byte ptr [esi + 0x29c], 0xfb"),
        (0xA62234, "call", "eax"),
        (0xA62236, "mov", "eax, dword ptr [esi + 0xd0]"),
        (0x8DE860, "push", "ebx"),
        (0x6576B0, "mov", "eax, dword ptr [ecx + 0xd8]"),
    ]
    for va, mnemonic, operands in anchors:
        instruction = next(decoder.disasm(pe.get_data(va - BASE, 15), va))
        assert (instruction.mnemonic, instruction.op_str) == (mnemonic, operands), hex(
            va
        )
    tables = {
        0xFD3ED4: {
            3: 0x631370,
            6: 0x632440,
            7: 0x631440,
            12: 0x6315C0,
            13: 0x6315D0,
            14: 0x630930,
            16: 0x6BE3D0,
            25: 0x6B7840,
        },
        0xFC0D34: {
            20: 0xA5F730,
            103: 0x61FF60,
            104: 0x65B950,
            108: 0x65B9F0,
            170: 0x669490,
        },
        0x109CE2C: {1: 0xA61DF0, 3: 0xA62480, 27: 0x8DE860, 82: 0x6576B0},
        0x10653A4: {1: 0xA61DF0, 27: 0x8DE860, 82: 0x6576B0},
    }
    for table, slots in tables.items():
        for slot, target in slots.items():
            actual = struct.unpack("<I", pe.get_data(table + slot * 4 - BASE, 4))[0]
            assert actual == target, (hex(table), slot)

    def text_at(va):
        return pe.get_data(va - BASE, 128).split(b"\0", 1)[0].decode("ascii")

    assert text_at(0xFBB3F8) == "/client/chara/%s/%s%03d/"
    assert text_at(0xFD2C6C) == "%s%s%03d/%s"
    assert text_at(0xFD2C78) == "%s0001"
    return dict(
        instruction_anchors=len(anchors),
        table_slots=sum(map(len, tables.values())),
        formats=3,
    )


def selector_replay(image):
    results = []
    for packed in [0, 0xC00, 0xC01, 0xC0000FFF, 0x1000]:
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(0, 0x1000)
        uc.mem_map(0x400000, (len(image) + 0xFFF) & ~0xFFF)
        uc.mem_write(0x400000, image)
        uc.mem_map(0x20000000, 0x20000)
        uc.mem_map(0x30000000, 0x20000)

        def w(a, v):
            uc.mem_write(a, struct.pack("<I", v & 0xFFFFFFFF))

        def r(a):
            return struct.unpack("<I", uc.mem_read(a, 4))[0]

        def cstr(a):
            buf = bytearray()
            while uc.mem_read(a + len(buf), 1) != b"\0":
                buf += uc.mem_read(a + len(buf), 1)
                assert len(buf) < 1024
            return bytes(buf).decode("ascii")

        nextmem = [0x30008000]

        def string_obj(a, text):
            at = nextmem[0]
            nextmem[0] += 0x200
            uc.mem_write(at, text.encode("ascii") + b"\0")
            w(a, at)
            w(a + 8, len(text) + 1)

        def ret(pop=0, eax=None):
            sp = uc.reg_read(UC_X86_REG_ESP)
            target = r(sp)
            if eax is not None:
                uc.reg_write(UC_X86_REG_EAX, eax)
            uc.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
            uc.reg_write(UC_X86_REG_EIP, target)

        visual = 0x30000000
        vtable = 0x30001000
        end = 0x30002000
        prefixstub = 0x30002100
        w(visual, vtable)
        w(visual + 0x54, 0)
        w(visual + 0x58, 1)
        w(vtable + 0x58, prefixstub)
        requests = []
        format_calls = []
        visited = set()

        def hook(u, a, size, _):
            visited.add(a)
            sp = u.reg_read(UC_X86_REG_ESP)
            ecx = u.reg_read(UC_X86_REG_ECX)
            if a == end:
                u.emu_stop()
            elif a == prefixstub:
                out, cat, num = [r(sp + i) for i in [4, 8, 12]]
                assert cat == 0 and num == 1
                string_obj(out, "/client/chara/pc/c001/")
                ret(12, out)
            elif a == 0x445210:
                ret(0, r(ecx))
            elif a == 0x445CF0:
                string_obj(ecx, "")
                ret(0, ecx)
            elif a == 0x447260:
                string_obj(ecx, cstr(r(sp + 4)))
                ret(8, ecx)
            elif a == 0x446F50:
                ret()
            elif a == 0x9D4F83:
                dest, limit, fmt = [r(sp + i) for i in [4, 8, 12]]
                fmt = cstr(fmt)
                if fmt == "%s%s%03d/%s":
                    value = fmt % (
                        cstr(r(sp + 16)),
                        cstr(r(sp + 20)),
                        r(sp + 24),
                        cstr(r(sp + 28)),
                    )
                else:
                    assert fmt in ["%s0001", "%s9999"]
                    value = fmt % cstr(r(sp + 16))
                assert len(value) < limit
                u.mem_write(dest, value.encode("ascii") + b"\0")
                format_calls.append(value)
                ret(0, len(value))
            elif a == 0xD39290:
                out, path, suffix, flag = [r(sp + i) for i in [4, 8, 12, 16]]
                assert cstr(r(suffix)) == ".bin" and flag == 0
                string_obj(out, cstr(r(path)))
                ret(0, 1)
            elif a == 0x6320C0:
                bank, path, flag = [r(sp + i) for i in [4, 8, 12]]
                requests.append({"bank": bank, "flag": flag, "path": cstr(r(path))})
                u.emu_stop()
            elif a == 0x9D20F4:
                ret()

        uc.hook_add(UC_HOOK_CODE, hook)
        sp = 0x20010000
        for i, val in enumerate([end, 3, packed]):
            w(sp + 4 * i, val)
        uc.reg_write(UC_X86_REG_ESP, sp)
        uc.reg_write(UC_X86_REG_ECX, visual)
        uc.emu_start(0x6B5E40, end, count=20000)
        if packed == 0:
            assert uc.reg_read(UC_X86_REG_EIP) == end
            assert not requests
        else:
            assert uc.reg_read(UC_X86_REG_EIP) == 0x6320C0
            assert {0x6306A0, prefixstub, 0xD39290} <= visited
            gear = (packed >> 10) & 0x3FF
            assert requests == [
                {
                    "bank": 5,
                    "flag": 2,
                    "path": f"/client/chara/pc/c001/equ/e{gear:03d}/top_mdl/0001",
                }
            ]
        results.append(
            {
                "packed_word": hex(packed),
                "requests": requests,
                "formatted_strings": format_calls,
            }
        )
    return results


def record_replay(mapped):
    def make():
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(0x400000, (len(mapped) + 0xFFF) & ~0xFFF)
        uc.mem_write(0x400000, mapped)
        uc.mem_map(0x20000000, 0x10000)
        uc.mem_map(0x30000000, 0x10000)
        return uc

    def w(u, a, v):
        u.mem_write(a, struct.pack("<I", v))

    def r(u, a):
        return struct.unpack("<I", u.mem_read(a, 4))[0]

    end = 0x3000F000
    stack = 0x20008000
    compare = []
    for old, new in [("", ""), ("", "a"), ("a", "a"), ("a", "b"), ("abc", "abd")]:
        u = make()
        for obj, buf, value in [
            (0x30000000, 0x30001000, old),
            (0x30000100, 0x30001100, new),
        ]:
            u.mem_write(buf, value.encode() + b"\0")
            w(u, obj, buf)
            w(u, obj + 8, len(value) + 1)
        w(u, stack, end)
        w(u, stack + 4, 0x30000100)
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_ECX, 0x30000000)
        u.emu_start(0x445DB0, end, count=1000)
        assert u.reg_read(UC_X86_REG_EIP) == end
        assert u.reg_read(UC_X86_REG_ESP) == stack + 8
        got = u.reg_read(UC_X86_REG_EAX) & 0xFF
        assert got == int(old != new)
        compare.append({"existing": old, "incoming": new, "AL": got})
    publication = []
    for previous, incoming in [
        (0, 0x30004000),
        (0x30005000, 0x30004000),
        (0x30005000, 0),
    ]:
        u = make()
        visual = 0x30000000
        record = 0x30001000
        owner = 0x30002000
        table = 0x30003000
        retain = 0x3000E000
        release = 0x3000E100
        w(u, visual + 8, owner)
        w(u, visual + 0x18, record)
        w(u, owner, table)
        w(u, table + 0x18, retain)
        w(u, table + 0x1C, release)
        w(u, record + 5 * 0x6C + 0x68, 2)
        w(u, record + 5 * 0x6C + 0x5C, previous)
        w(u, visual + 0x1C, int(bool(previous)))
        events = []

        def hook(uc, a, size, _):
            if a not in [retain, release]:
                return
            sp = uc.reg_read(UC_X86_REG_ESP)
            assert uc.reg_read(UC_X86_REG_ECX) == owner
            ptr = r(uc, sp + 4)
            events.append(
                {
                    "operation": "retain" if a == retain else "release",
                    "pointer": hex(ptr),
                }
            )
            if a == retain:
                assert ptr == incoming and r(uc, sp + 8) == 0
            else:
                assert ptr == previous
            uc.reg_write(UC_X86_REG_EIP, r(uc, sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 4 + (8 if a == retain else 4))

        u.hook_add(UC_HOOK_CODE, hook)
        for i, val in enumerate([end, owner, incoming, 5]):
            w(u, stack + 4 * i, val)
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_ECX, visual)
        u.emu_start(0x632440, end, count=1000)
        assert u.reg_read(UC_X86_REG_EIP) == end
        assert u.reg_read(UC_X86_REG_ESP) == stack + 16
        assert r(u, record + 5 * 0x6C + 0x5C) == incoming
        assert r(u, visual + 0x1C) == int(bool(previous) or bool(incoming))
        assert [event["operation"] for event in events] == (
            ["retain"] + (["release"] if previous else []) if incoming else []
        )
        publication.append(
            {
                "previous_nonnull": bool(previous),
                "incoming_nonnull": bool(incoming),
                "published_pointer_unchanged": True,
                "events": events,
                "count": r(u, visual + 0x1C),
            }
        )
    return dict(string_comparisons=compare, publication_cases=publication)


def failure_replay(image):
    """Run the actual callback failure prefix through pending/root publication."""
    results = []
    for previous in (0, 0x2006000):
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(BASE, (len(image) + 0xFFF) & ~0xFFF)
        uc.mem_write(BASE, image)
        uc.mem_map(0x2000000, 0x10000)
        uc.mem_map(0x3000000, 0x10000)
        visual, owner, records, request = 0x2000000, 0x2001000, 0x2002000, 0x2005000
        record = records + 5 * 0x6C

        def put(address, value):
            uc.mem_write(address, struct.pack("<I", value))

        def get(address):
            return struct.unpack("<I", uc.mem_read(address, 4))[0]

        put(visual, 0xFD3ED4)
        put(visual + 8, owner)
        put(visual + 0x18, records)
        put(visual + 0x1C, int(bool(previous)))
        put(record + 0x58, request)
        put(record + 0x5C, previous)
        put(record + 0x68, 2)
        stack = 0x3008000
        for i, value in enumerate((0, 0, request, 5)):
            put(stack + 4 * i, value)
        uc.reg_write(UC_X86_REG_ESP, stack)
        uc.reg_write(UC_X86_REG_ECX, visual)
        visited = set()

        def hook(_uc, address, _size, _user):
            visited.add(address)
            if address == 0x631CC0:
                uc.emu_stop()

        uc.hook_add(UC_HOOK_CODE, hook)
        uc.emu_start(0x631C70, 0, count=1000)
        assert uc.reg_read(UC_X86_REG_EIP) == 0x631CC0
        assert uc.reg_read(UC_X86_REG_ESP) == stack - 0x218
        assert {0x631370, 0x632440} <= visited
        assert get(record + 0x58) == get(record + 0x5C) == 0
        assert get(visual + 0x1C) == int(bool(previous))
        results.append(
            dict(
                previous_root_nonnull=bool(previous),
                pending_cleared=True,
                published_root=0,
                maintained_count=get(visual + 0x1C),
                nonnull_root_count=0,
                boundary="Actual callback arg1=0 prefix and visual slots3/6; V+20=0, no stubs, stop before request follow-up at631CC0.",
            )
        )
    return results


def visual_replay(
    image,
    entry,
    resources,
    layer_result,
    pending=False,
    count_override=None,
    callback_failure=False,
):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(BASE, (len(image) + 4095) & ~4095)
    uc.mem_write(BASE, image)
    uc.mem_map(0, 0x1000)
    uc.mem_map(0x2000000, 0x20000)
    uc.mem_map(0x3000000, 0x20000)
    V, S, M, R, OUT = 0x3000000, 0x3001000, 0x3005000, 0x3006000, 0x3008000
    CONTEXT, CTABLE, LAYERS = 0x3009000, 0x300A000, 0x300B000
    STOP, CONTEXT_GET = 0x300F000, 0x300F010

    def w(addr, *values):
        uc.mem_write(addr, struct.pack("<" + "I" * len(values), *values))

    def rd(addr, n=1):
        return struct.unpack("<" + "I" * n, uc.mem_read(addr, 4 * n))

    w(V, 0xFD3ED4)
    w(V + 8, S)
    count = (
        sum(bool(x) for x in resources) if count_override is None else count_override
    )
    w(V + 0x14, len(resources), R, count)
    w(V + 0x24, OUT)
    w(V + 0x30, 0x300C000)
    uc.mem_write(V + 0x34, b"\x01\x00\x00\x00")
    w(V + 0x38, V + 0x178)
    w(V + 0x178, 3)
    w(S, 0xFC0D34)
    w(S + 0xC, M)
    w(S + 0x114, CONTEXT)
    w(M, 0x109CE2C)
    w(CONTEXT, CTABLE)
    w(CTABLE + 0x18, CONTEXT_GET)
    for i, resource in enumerate(resources):
        w(R + i * 0x6C + 0x5C, resource)
    if pending:
        w(R + 0x58, 0x300D000)
    sp = 0x201F000
    w(sp, STOP)
    if callback_failure:
        assert entry == 0x631C70 and not pending and count == 1
        assert resources[5] and sum(bool(value) for value in resources) == 1
        w(R + 5 * 0x6C + 0x58, 0x300D000)
        uc.mem_write(V + 0x20, b"\x01")
        w(sp, STOP, 0, 0x300D000, 5)
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_ECX, V)
    captured = []
    visited = set()
    trace = []

    def return_stub(value, pop):
        esp = uc.reg_read(UC_X86_REG_ESP)
        target = rd(esp)[0]
        uc.reg_write(UC_X86_REG_EAX, value)
        uc.reg_write(UC_X86_REG_ESP, esp + 4 + pop)
        uc.reg_write(UC_X86_REG_EIP, target)

    def hook(_uc, address, _size, _user):
        visited.add(address)
        trace.append(address)
        if address == STOP:
            uc.emu_stop()
        elif address == CONTEXT_GET:
            assert uc.reg_read(UC_X86_REG_ECX) == CONTEXT
            assert rd(uc.reg_read(UC_X86_REG_ESP) + 4)[0] == S + 0x2B79
            return_stub(LAYERS, 0)
        elif address == 0xC269F0:
            assert uc.reg_read(UC_X86_REG_ECX) == LAYERS
            return_stub(layer_result, 4)
        elif (
            address
            == struct.unpack(
                "<I", image[0x109CE2C - BASE + 12 : 0x109CE2C - BASE + 16]
            )[0]
        ):
            assert uc.reg_read(UC_X86_REG_ECX) == M
            return_stub(0, 0)
        elif address == 0xA61DF0:
            assert uc.reg_read(UC_X86_REG_ECX) == M
            captured.extend(rd(uc.reg_read(UC_X86_REG_ESP) + 4, 6))
            uc.emu_stop()

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(entry, 0, count=30000)
    if pending:
        assert not captured
        assert uc.mem_read(V + 0x20, 1) == b"\x01"
        assert STOP in visited
        return dict(entry=hex(entry), pending_deferred=True)
    selected = [value for value in resources if value]
    if callback_failure:
        assert {0x631370, 0x632440, 0x631320} <= visited
        assert rd(R + 5 * 0x6C + 0x58, 2) == (0, 0)
        assert uc.mem_read(V + 0x20, 1) == b"\x00"
        selected = []
    expected_layer = 0 if layer_result == 0xFFFFFFFF else layer_result
    assert captured == [OUT, count, 1, expected_layer, 0, 0], (
        captured,
        hex(entry),
        resources,
        [hex(x) for x in trace[-30:]],
    )
    assert list(rd(OUT, len(selected))) == selected
    assert rd(V + 0x1C)[0] == count
    assert {
        0x6315C0,
        0x6315D0,
        0x6BE3D0,
        0x6BB080,
        0x65B9F0,
        0x7CD5B0,
        0xA5F6B0,
        0x65B950,
        0xA61DF0,
    } <= visited
    return dict(
        entry=hex(entry),
        input_records=len(resources),
        packed_count=len(selected),
        forwarded_count=count,
        model_arguments=captured,
        layer_lookup=layer_result,
        boundary="Stop at native A61DF0 entry; context getter, layer lookup and Model teardown are stubs.",
    )


def enable_replay(image):
    cases = []
    for enable in (0, 1):
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(0x400000, (len(image) + 0xFFF) & ~0xFFF)
        uc.mem_write(0x400000, image)
        data, stack = 0x2000000, 0x3000000
        uc.mem_map(data, 0x10000)
        uc.mem_map(stack, 0x10000)
        model, drawable = data, data + 0x1000

        def p32(address, value):
            uc.mem_write(address, struct.pack("<I", value))

        p32(model, 0x10653A4)
        p32(model + 0xD8, drawable)
        uc.mem_write(model + 0x29C, b"\x06")
        p32(drawable + 0xA0, 0xBD)
        esp = stack + 0x8000
        p32(esp + 0x458, enable)
        uc.reg_write(UC_X86_REG_ESI, model)
        uc.reg_write(UC_X86_REG_EBX, 0)
        uc.reg_write(UC_X86_REG_ESP, esp)
        calls = []

        def hook(_uc, address, _size, _user):
            assert (
                0xA62211 <= address < 0xA62236
                or 0x8DE860 <= address < 0x8DE8CF
                or 0x6576B0 <= address < 0x6576B7
                or address == 0xA62236
            ), hex(address)
            if address == 0x8DE860:
                sp = uc.reg_read(UC_X86_REG_ESP)
                calls.append(struct.unpack("<I", uc.mem_read(sp + 4, 4))[0])
            if address == 0xA62236:
                uc.emu_stop()

        uc.hook_add(UC_HOOK_CODE, hook)
        uc.emu_start(0xA62211, 0, count=1000)
        assert uc.reg_read(UC_X86_REG_EIP) == 0xA62236
        assert uc.reg_read(UC_X86_REG_ESP) == esp
        flags = struct.unpack("<I", uc.mem_read(drawable + 0xA0, 4))[0]
        model_flags = uc.mem_read(model + 0x29C, 1)[0]
        assert bool(flags & 1) == bool(enable)
        assert bool(model_flags & 4) == bool(enable)
        assert calls == ([1] if enable else [0, 1])
        cases.append(
            dict(
                enable_argument=enable,
                drawable_mask1=bool(flags & 1),
                model_bit2=bool(model_flags & 4),
                native_slot27_calls=calls,
            )
        )
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if not __debug__:
        parser.error("run without Python optimization")
    if args.output.exists():
        parser.error("--output must name a new file")
    raw = args.exe.read_bytes()
    assert len(raw) == 15996808 and hashlib.sha256(raw).hexdigest().upper() == PIN
    pe = pefile.PE(data=raw, fast_load=True)
    assert pe.OPTIONAL_HEADER.ImageBase == BASE and pe.FILE_HEADER.Machine == 0x14C
    anchors = native_anchors(pe)
    image = pe.get_memory_mapped_image()
    forwarded = []
    for entry in (0x631320, 0x6323D0):
        for resources in (
            [],
            [0, 0, 0],
            [0x3100010],
            [0, 0x3100020, 0, 0x3100030],
            [0x3100040] * 57,
        ):
            for layer in (0xFFFFFFFF, 5):
                forwarded.append(visual_replay(image, entry, resources, layer))
    result = dict(
        exe_sha256=PIN,
        native=anchors,
        selector=selector_replay(image),
        records=record_replay(image),
        callback_failure=failure_replay(image),
        failure_to_model=visual_replay(
            image,
            0x631C70,
            [0] * 5 + [0x3100020] + [0] * 51,
            5,
            callback_failure=True,
        ),
        forwarding=forwarded,
        pending=visual_replay(image, 0x6323D0, [0x3100010, 0], 5, pending=True),
        inconsistent_count=visual_replay(
            image, 0x631320, [0, 0x3100020], 5, count_override=7
        ),
        enable=enable_replay(image),
        boundary="Separate native fragments with synthetic objects. Selector stubs prefix, String runtime, formatter, successful identity-copy resolver and cookie check. Publication stubs owner retain/release with V+20 zero. Callback failure prefix executes real slots3/6 with V+20 zero and stops at631CC0. Forwarding stubs context getter, layer lookup and Model teardown and stops at slot1 entry. Enable starts at A62211 with native prior OR6 modeled and stops before ownership/registration. No scene, resolver, resource factory/loading, asset association, complete draw, queue or GPU executes.",
    )
    with args.output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    print(
        "PASS: selector, publication/count, callback failure, 20 slot1 forwards, pending/count controls and enable pair"
    )


if __name__ == "__main__":
    main()
