"""Check one pinned body mesh/material binding and bounded admission fragments.

Requires pefile, Capstone and Unicorn 2.1.4. Objects are synthetic; no client,
renderer, queue or GPU runs. Output must name a new JSON file.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct

import capstone
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_EAX,
    UC_X86_REG_ECX,
    UC_X86_REG_EIP,
    UC_X86_REG_ESP,
)

from verify_body_shader_join import ASSET_PIN, PIN


def native_anchors(pe):
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    anchors = [
        (0xE0D12F, "mov", "dword ptr [esi + 0x18], ecx"),
        (0xE0D135, "mov", "dword ptr [esi + 0x1c], ecx"),
        (0xE0D279, "shl", "eax, 4"),
        (0xE0D27F, "mov", "dword ptr [esi + 0x10], eax"),
        (0xE0D2D2, "mov", "ecx, dword ptr [eax + 4]"),
        (0xE0D2E5, "mov", "dword ptr [edx + edi*4], ecx"),
        (0xE0CCEC, "mov", "edx, dword ptr [eax + edx]"),
        (0xE0CCF2, "mov", "eax, dword ptr [eax + edx*4]"),
        (0xC7966A, "call", "0xc79310"),
        (0xC79689, "call", "eax"),
        (0xC51A71, "call", "0xc3e4e0"),
        (0xC3E512, "mov", "dword ptr [esi + 0x38], edi"),
        (0xC51A82, "call", "0xc3dbc0"),
        (0xC8DD22, "call", "0xc8f520"),
        (0xC8DD74, "lea", "ecx, [esi + 0x58]"),
        (0xC8DDB5, "call", "0xc8f990"),
        (0xC8DDBC, "mov", "dword ptr [esi + 0x30], ecx"),
        (0xC34752, "mov", "eax, dword ptr [ebp + 0x30]"),
        (0xC34760, "je", "0xc34847"),
        (0xC348AF, "call", "0x9d244e"),
        (0xC348CE, "call", "0xc41440"),
        (0xC4145C, "mov", "dword ptr [esi + 0xb0], ecx"),
        (0xC41474, "mov", "dword ptr [ecx + 0x74], edi"),
        (0xC41479, "call", "0xc3cdd0"),
        (0xC4148D, "xor", "byte ptr [esi + 0x1a], dl"),
        (0xC418DC, "mov", "dword ptr [eax + 0x114], ecx"),
        (0xC58091, "call", "0xc59ad0"),
        (0xC580E1, "call", "0xc57dd0"),
        (0xC57E4B, "mov", "dword ptr [edi + 8], esi"),
    ]
    anchors.extend(
        [
            (0xC58091, "call", "0xc59ad0"),
            (0xC580E1, "call", "0xc57dd0"),
            (0xC57E4B, "mov", "dword ptr [edi + 8], esi"),
            (0xC2829C, "lea", "ecx, [esi + 0x30]"),
            (0xC282A4, "call", "0xc48c40"),
            (0xC4900C, "mov", "dword ptr [esi + 0xba4], ebx"),
            (0xC2919D, "lea", "esi, [ebx + 0x30]"),
            (0xC2936E, "push", "esi"),
            (0xC2936F, "call", "0xc59970"),
            (0xC5998A, "mov", "dword ptr [esi + 4], ecx"),
            (0xC29395, "call", "0xc38920"),
            (0xC293A3, "call", "0xc38ad0"),
            (0xC2954F, "call", "0xc57fd0"),
            (0xC29556, "call", "0xc59280"),
            (0xC57EBD, "call", "edx"),
            (0xC57EB7, "mov", "edx, dword ptr [eax + 0x2c]"),
            (0xC39ADC, "test", "byte ptr [eax + 0x1a], 2"),
            (0xC39AF9, "call", "0xc598b0"),
            (0xC598C3, "mov", "dword ptr [edi + 0x10], eax"),
            (0xC59903, "mov", "eax, dword ptr [edx + 0x28]"),
            (0xC5994E, "call", "0xc59480"),
            (0xC5949A, "test", "byte ptr [edi + 0x1a], 2"),
            (0xC59533, "add", "ecx, 0x14"),
            (0xC5953A, "mov", "dword ptr [esp + 0x18], edi"),
            (0xC5953E, "call", "0xc6bc90"),
            (0xC59283, "call", "0xc591d0"),
            (0xC59263, "call", "0xc583a0"),
            (0xC589C6, "mov", "ecx, dword ptr [ebx + 0x18]"),
            (0xC58A13, "mov", "ebp, dword ptr [eax + ebx*8 + 4]"),
            (0xC58C43, "mov", "ecx, dword ptr [esi + 4]"),
            (0xC58C49, "mov", "edx, dword ptr [eax + 4]"),
            (0xC58C4F, "call", "edx"),
            (0xC39677, "test", "byte ptr [ecx + 0x1a], 2"),
            (0xC396A0, "test", "byte ptr [eax + 0x1a], 1"),
            (0xC396CF, "call", "eax"),
            (0xC41479, "call", "0xc3cdd0"),
            (0xC4148D, "xor", "byte ptr [esi + 0x1a], dl"),
            (0xC3CDE0, "and", "eax, 1"),
            (0xC43B1E, "call", "0xc41990"),
            (0xC43D6C, "call", "0xc421f0"),
            (0xC421F6, "call", "0xc44c50"),
            (0xC44F07, "call", "0x41bfc0"),
            (0xC43EC3, "call", "0xc3eca0"),
            (0xC43ECF, "call", "0xc336c0"),
            (0xC43EDE, "call", "0xc41aa0"),
            (0xC43EFF, "cmp", "dword ptr [esi + 0x114], ebx"),
            (0xC43F2D, "call", "0xc423b0"),
            (0xC419D2, "mov", "eax, dword ptr [edi + 0xba4]"),
            (0xC419DC, "mov", "dword ptr [esp + 0xc], eax"),
        ]
    )
    anchors.extend(
        [
            (0xA62236, "mov", "eax, dword ptr [esi + 0xd0]"),
            (0xA62240, "mov", "ecx, dword ptr [esi + 0xd8]"),
            (0xA62246, "mov", "dword ptr [ecx + 0x10c], eax"),
            (0xA62260, "mov", "eax, dword ptr [esp + 0x454]"),
            (0xA6226E, "call", "0xc2dbd0"),
            (0xC2DBDA, "and", "eax, 0x7f"),
            (0xC2DBDD, "xor", "dword ptr [ecx + 0x2ac], eax"),
            (0xA6229D, "mov", "eax, dword ptr [esi + 0xd4]"),
            (0xA622A3, "mov", "ecx, dword ptr [eax + 0x20]"),
            (0xA622A6, "call", "0xc27a10"),
            (0xC2DBF0, "mov", "eax, dword ptr [ecx + 0x2ac]"),
            (0xC2DBF6, "shl", "eax, 0x19"),
            (0xC2DBF9, "sar", "eax, 0x19"),
            (0xC27A4C, "call", "0xc2dbf0"),
            (0xC27A51, "mov", "ecx, dword ptr [esi + 0x18]"),
            (0xC27A66, "cmp", "edi, eax"),
            (0xC27A68, "jl", "0xc27a6c"),
            (0xC27A6A, "xor", "edi, edi"),
            (0xC27A88, "mov", "ecx, dword ptr [ecx + edi*4]"),
            (0xC27A90, "add", "ecx, 0x1c"),
            (0xC27A93, "call", "0xc38d80"),
            (0xC38DB0, "mov", "edx, dword ptr [ecx]"),
            (0xC38DB2, "mov", "dword ptr [eax], edx"),
            (0xC38DB7, "mov", "dword ptr [esi + 8], eax"),
            (0xC27AF3, "mov", "byte ptr [0x136eb11], al"),
            (0xC27B1D, "add", "ecx, 0x30"),
            (0xC27B20, "call", "0xc38bb0"),
            (0xC38BCA, "cmp", "byte ptr [0x136eb11], 0"),
            (0xC38BE8, "call", "0xc6f660"),
            (0xC38C15, "test", "al, 7"),
            (0xC38C17, "je", "0xc38c77"),
            (0xC38C7F, "lea", "ebx, [ecx + 0x130]"),
            (0xC38D24, "call", "0xc38df0"),
            (0xC38D37, "call", "0xc6e170"),
            (0xC38FD0, "mov", "ecx, dword ptr [esp + 0x20]"),
            (0xC38FD9, "call", "0xc2eef0"),
            (0xC38B16, "mov", "eax, dword ptr [edi + 0x134]"),
            (0xC38B1C, "add", "edi, 0x130"),
            (0xC38B6D, "mov", "eax, dword ptr [esi]"),
            (0xC38B6F, "mov", "edx, dword ptr [eax + 0xa0]"),
            (0xC38B75, "and", "edx, ebx"),
            (0xC38B77, "cmp", "edx, ebx"),
            (0xC38B88, "call", "0xc38d80"),
            (0xC2939F, "push", "1"),
            (0xC293A3, "call", "0xc38ad0"),
        ]
    )
    ownership = {
        0xA61F6E: "cmp byte ptr [esp + 0x450], bl",
        0xA61F81: "je 0xa61ff2",
        0xA61FA5: "call 0xc510f0",
        0xA61FAD: "mov edi, dword ptr [eax]",
        0xA61FCC: "mov dword ptr [esi + 0xd8], edi",
        0xA61FD4: "call 0xa3e680",
        0xA62014: "call 0xc50fe0",
        0xA61FF8: "push edx",
        0xA61FFE: "mov ecx, dword ptr [esp + 0x44c]",
        0xA62006: "mov eax, dword ptr [esp + 0x454]",
        0xA6201C: "mov edi, dword ptr [eax]",
        0xA6203B: "mov dword ptr [esi + 0xd8], edi",
        0xA62043: "call 0xa3e680",
        0xA62073: "mov ecx, dword ptr [esi + 0xd8]",
        0xA62096: "cmp dword ptr [eax + 0x368], ebx",
        0xA62236: "mov eax, dword ptr [esi + 0xd0]",
        0xA62240: "mov ecx, dword ptr [esi + 0xd8]",
        0xA62246: "mov dword ptr [ecx + 0x10c], eax",
        0xA622A6: "call 0xc27a10",
        0xA62D50: "mov dword ptr [esi + 0xd0], eax",
        0xA60CB2: "mov dword ptr [esi + 0xc], eax",
        0x61F582: "mov dword ptr [eax + 8], ecx",
        0xC341FD: "mov dword ptr [esi + 0x78], eax",
        0xC343BA: "call 0xc33430",
        0xC33516: "mov dword ptr [ebx + 0x80], edi",
        0xC335F8: "mov dword ptr [ecx + esi*4], eax",
        0xC33645: "call 0xc42500",
        0xC42524: "mov dword ptr [esi + 0x118], edi",
        0xC4252A: "mov dword ptr [esi + 0x11c], ecx",
        0xC50147: "call 0xc341d0",
        0xC5025F: "call 0xc4f6f0",
        0xC50D71: "call 0xc341d0",
        0xC50E0E: "call 0xc4f6f0",
        0xC393CD: "test byte ptr [ecx + 0x1a], 1",
        0x62E3A9: "mov byte ptr [eax + 0x1a], cl",
        0x84637D: "mov dword ptr [edi + 8], eax",
        0x846388: "mov dword ptr [eax + 0x238], edx",
    }
    anchors.extend(
        (address, *expected.split(" ", 1)) for address, expected in ownership.items()
    )
    tables = [
        (0xFC0D34, 71, 0xCB1C80),
        (0x10653A4, 82, 0x6576B0),
        (0x10653A4, 1, 0xA61DF0),
        (0x10D6414, 7, 0xC39370),
        (0x10D7A04, 1, 0xC439C0),
        (0xFC0D34, 13, 0xA602A0),
        (0x109CE2C, 1, 0xA61DF0),
        (0x109CE2C, 82, 0x6576B0),
        (0x10D6414, 9, 0xC39550),
        (0x10D6414, 10, 0xC39620),
        (0x10D6414, 11, 0xC39A20),
    ]
    for table, slot, target in tables:
        value = struct.unpack("<I", pe.get_data(table - 0x400000 + 4 * slot, 4))[0]
        assert value == target, (hex(table), slot, hex(value))
    checked = []
    for address, mnemonic, operands in anchors:
        instruction = next(decoder.disasm(pe.get_data(address - 0x400000, 16), address))
        assert (instruction.mnemonic, instruction.op_str) == (mnemonic, operands), (
            hex(address),
            instruction.mnemonic,
            instruction.op_str,
        )
        checked.append(f"{address:08X}")
    assert pe.get_data(0x10FAE5C - 0x400000, 2) == b".\0"
    assert pe.get_data(0x10D4030 - 0x400000, 18).split(b"\0")[0] == b"DrawManager::Draw"
    return dict(instructions=checked, vtable_entries=len(tables))


def prefix_order(pe):
    # Intraprocedural CFG only: calls return normally; descendants are not expanded.
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    code = {
        i.address: i for i in decoder.disasm(pe.get_data(0x8439C0, 0x60A), 0xC439C0)
    }

    def reachable(block=None):
        todo, seen = [0xC439C0], set()
        while todo:
            address = todo.pop()
            if address == block or address in seen:
                continue
            seen.add(address)
            instruction = code[address]
            following = address + instruction.size
            if instruction.mnemonic.startswith("ret"):
                continue
            if instruction.mnemonic == "jmp":
                todo.append(instruction.operands[0].imm)
            elif instruction.mnemonic.startswith("j"):
                todo.extend((following, instruction.operands[0].imm))
            else:
                todo.append(following)
        return seen

    all_sites = reachable()
    before_builder = reachable(0xC43D6C)
    late = (0xC43EC3, 0xC43ECF, 0xC43EDE, 0xC43EFF, 0xC43F2D)
    assert all(site in all_sites and site not in before_builder for site in late)
    assert all(
        not any(
            op.type == capstone.CS_OP_MEM and op.mem.disp == 0x114
            for op in code[address].operands
        )
        for address in before_builder
    )
    return dict(
        late_sites_require_builder=True,
        mesh114_unread_before_builder=True,
        boundary="Intraprocedural C439C0 CFG with ordinary call returns; no descendant or alias proof.",
    )


def selected_asset(raw):
    def u32(p):
        return struct.unpack_from("<I", raw, p)[0]

    assert len(raw) == 107152 and raw[:4] == b"PWIB"

    def directory(root):
        assert raw[root : root + 8] == b"SEDBRES "
        count = u32(root + 0x30)
        assert count == u32(root + 0x38) and 2 <= count < 64
        content = root + 0x40 + 16 * count
        cursor = content + u32(root + 0x34)
        names = []
        for _ in range(count):
            end = raw.index(b"\0", cursor)
            names.append(raw[cursor:end].decode("ascii"))
            cursor = end + 1
        rows = []
        for i in range(count):
            index, relative, size, kind = struct.unpack_from(
                "<4I", raw, root + 0x40 + 16 * i
            )
            rows.append(
                dict(
                    index=index,
                    name=names[index],
                    start=content + relative,
                    size=size,
                    kind=kind,
                )
            )
        return rows

    outer = directory(16)
    assert outer[0]["start"] == 144
    rows = directory(144)
    shader, wrb = rows[0], rows[3]
    assert shader["start"] == 320 and shader["size"] == 15673
    assert shader["name"] == "F03\\mtc001e003tc_a.win32"
    assert wrb["start"] == 47348 and wrb["size"] == 58928
    assert wrb["name"] == "F06\\c001e003_top.opt16.win32"
    types = next(r for r in rows if r["name"] == "RESOURCE_TYPE")
    assert raw[types["start"] : types["start"] + 4] == b"brds"
    chunks = []

    def walk(start, end):
        while start + 16 <= end:
            tag = raw[start : start + 4]
            size, advance = struct.unpack_from(">2I", raw, start + 8)
            assert size >= 16 and start + size <= end
            advance = advance if advance >= size else (size + 15) & ~15
            assert start + advance <= end or start + size == end
            chunks.append((tag, start, size))
            if tag in (b"WRB\0", b"MDLC", b"MDL\0", b"MESH"):
                walk(start + 32, start + size)
            start += advance
        assert start >= end or not raw[start:end].strip(b"\0")

    start = wrb["start"]
    assert raw[start : start + 8] == b"SEDBwrb\0"
    header = struct.unpack_from("<H", raw, start + 14)[0]
    walk(start + header, start + u32(start + 16))
    meshes = [p for tag, p, _ in chunks if tag == b"MESH"]
    assert meshes == [47620, 77396, 99860]
    selected = meshes[1]
    assert raw[77460:77464] == b"STR\0" and raw[77492:77496] == b"RSID"
    mesh_name = raw[77476 : raw.index(b"\0", 77476)].decode("ascii")
    material_name = shader["name"].rsplit("\\", 1)[1].split(".", 1)[0]
    assert mesh_name == material_name == "mtc001e003tc_a"
    assert raw[77508:77524] == b"\0" * 16
    return dict(
        shader=shader,
        wrb=wrb,
        mesh=selected,
        mesh_ordinal=1,
        mesh_name=mesh_name,
        material_name=material_name,
        rsid_zero=True,
        pram=13808,
    )


RANGES = (
    (0xC439C0, 0xC43D71),
    (0xC41990, 0xC41A95),
    (0xC3DB90, 0xC3DB9C),
    (0xC421F0, 0xC42219),
    (0xC48760, 0xC4876B),
    (0xC44C50, 0xC44F25),
    (0xC32BD0, 0xC32BD7),
    (0xC33C10, 0xC33CFE),
    (0x61F0F0, 0x61F201),
    (0x545670, 0x54574E),
    (0x42EDB0, 0x42EE11),
    (0x42EC50, 0x42EDAA),
    (0x4201A0, 0x420200),
    (0x419B60, 0x419BBA),
    (0x6522D0, 0x652449),
    (0xC47910, 0xC47960),
)

# Target: (callee stack cleanup, native RET VA). Each anchor is checked below.
STUB_RETURNS = {
    0x41BF10: (0, 0x41BF2A),
    0x60D260: (4, 0x60D28C),
    0x9FE250: (0, 0x9FE301),
    0x9FE770: (0, 0x9FE781),
    0xC37540: (4, 0xC37609),
    0xC3D790: (16, 0xC3D8B4),
    0xC44F80: (4, 0xC45033),
    0xC450F0: (4, 0xC45154),
    0xC47870: (4, 0xC478A9),
    0xC47A70: (0, 0xC47A76),
    0xC47BF0: (0, 0xC47BF6),
}
STUBS = {target: cleanup for target, (cleanup, _) in STUB_RETURNS.items()}
IDENT = struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
DATA = 0x9000000
MESH = DATA
NODE = DATA + 0x1000
CTX = DATA + 0x2000
DRAW = DATA + 0x4000
ASSET = DATA + 0x5000
MAT = DATA + 0x6000
RES = DATA + 0x7000
DEF = DATA + 0x8000
V = DATA + 0x8100
OV = DATA + 0x8200
PIX = DATA + 0x8300


def prefix_replay(image):
    """Return four synthetic case records, asserting fresh native RET anchors."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    for target, (cleanup, va) in STUB_RETURNS.items():
        instruction = next(md.disasm(image[va - 0x400000 : va - 0x400000 + 3], va))
        assert instruction.mnemonic == "ret", hex(target)
        actual = instruction.operands[0].imm if instruction.operands else 0
        assert actual == cleanup, (hex(target), actual, cleanup)

    def run(override, instance):
        u = Uc(UC_ARCH_X86, UC_MODE_32)
        u.mem_map(0x400000, (len(image) + 4095) & ~4095)
        u.mem_write(0x400000, image)
        u.mem_map(0x8000000, 0x10000)
        u.mem_map(DATA, 0x10000)

        def p(a, v):
            u.mem_write(a, struct.pack("<I", v))

        def g(a):
            return struct.unpack("<I", u.mem_read(a, 4))[0]

        p(MESH + 0x118, NODE)
        p(MESH + 0x114, instance)
        p(MESH + 0x1C, ASSET)
        p(MESH + 0xB0, MAT)
        u.mem_write(MESH + 0x131, b"\1\1")
        p(NODE + 0x78, DRAW)
        p(MAT + 0x38, RES)
        p(RES + 0x44, DEF)
        for x in (V, OV, PIX):
            p(x + 0x1C, x + 0x40)
        p(CTX + 0xBA4, OV if override else 0)
        p(CTX + 0xBA8, PIX)
        for off in (0x20, 0xA0, 0xE0, 0x160):
            u.mem_write(CTX + off, IDENT)
        u.mem_write(DRAW + 0x30, IDENT)
        sp = 0x800FF00
        p(sp, 0x7000000)
        p(sp + 4, CTX)
        u.reg_write(UC_X86_REG_ESP, sp)
        u.reg_write(UC_X86_REG_ECX, MESH)
        seen = []
        stub_hits = []
        ba4writes = []
        submission = []

        def ret(pop=0):
            s = u.reg_read(UC_X86_REG_ESP)
            u.reg_write(UC_X86_REG_EIP, g(s))
            u.reg_write(UC_X86_REG_ESP, s + 4 + pop)

        def hook(uc, a, size, data):
            seen.append(a)
            if a == 0x41BFC0:
                s = u.reg_read(UC_X86_REG_ESP)
                assert (g(s + 4), g(s + 12)) == (0, 16)
                assert 0x8000000 <= g(s + 8) < 0x8010000
                assert g(CTX + 0xBA4) == (OV if override else 0)
                assert g(0x1374CB0) == (OV if override else V)
                submission.append(
                    {
                        "start": g(s + 4),
                        "count": g(s + 12),
                        "ba4_nonzero": bool(g(CTX + 0xBA4)),
                        "selected_override": g(0x1374CB0) == OV,
                    }
                )
                u.emu_stop()
                return
            if a in STUBS:
                stub_hits.append(a)
                s = u.reg_read(UC_X86_REG_ESP)
                value = 0
                if a == 0xC3D790:
                    out = g(s + 4)
                    p(out, V)
                    p(out + 4, PIX)
                    value = out
                elif a == 0xC37540:
                    value = g(s + 4)
                    u.mem_write(value, IDENT)
                elif a == 0x60D260:
                    value = u.reg_read(UC_X86_REG_ECX)
                    u.mem_write(value, IDENT)
                u.reg_write(UC_X86_REG_EAX, value)
                ret(STUBS[a])
                return
            assert any(lo <= a < hi for lo, hi in RANGES), f"unexpected native {a:08X}"

        def write(uc, access, a, size, value, data):
            if a < CTX + 0xBA8 and a + size > CTX + 0xBA4:
                ba4writes.append(a)

        u.hook_add(UC_HOOK_CODE, hook)
        u.hook_add(UC_HOOK_MEM_WRITE, write)
        try:
            u.emu_start(0xC439C0, 0, count=30000)
        except Exception as ex:
            raise RuntimeError([hex(x) for x in seen[-12:]]) from ex
        assert len(submission) == 1 and not ba4writes
        for a in (0xC43B1E, 0xC419D2, 0xC43D6C, 0xC421F6, 0xC44F07):
            assert a in seen, hex(a)
        assert 0xC43EFF not in seen and 0xC43EC3 not in seen and 0xC43EDE not in seen
        return dict(
            override=override,
            mesh114=instance,
            submission=submission[0],
            native_instructions=len(seen) - len(stub_hits),
            stub_targets=[f"{x:08X}" for x in sorted(set(stub_hits))],
            ba4_writes=len(ba4writes),
        )

    return [
        run(override, instance) for override in (False, True) for instance in (0, 1)
    ]


def binding_replay(image, asset, selection):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x400000, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(0x400000, image)
    data, stack, stop = 0x02000000, 0x03000000, 0x04000000
    uc.mem_map(data, 0x100000)
    uc.mem_map(stack, 0x10000)
    uc.mem_map(stop, 0x1000)
    node, drawable, tag, mesh = (data + n for n in (0, 0x1000, 0x2000, 0x3000))
    slots, resources, definitions = (data + n for n in (0x4000, 0x5000, 0x6000))
    pram, children, collected = (data + n for n in (0x8000, 0x9000, 0xA000))

    def put32(address, value):
        uc.mem_write(address, struct.pack("<I", value))

    def get32(address):
        return struct.unpack("<I", uc.mem_read(address, 4))[0]

    def ret(value, pop=0):
        esp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EAX, value)
        uc.reg_write(UC_X86_REG_EIP, get32(esp))
        uc.reg_write(UC_X86_REG_ESP, esp + 4 + pop)

    dispatches = []

    def hook(_uc, address, _size, _user):
        if address == 0xC439C0:
            dispatches.append(uc.reg_read(UC_X86_REG_ECX))
            ret(0, 4)  # Only test list partition/dispatch, not the mesh draw.
        elif address == stop:
            uc.emu_stop()

    uc.hook_add(UC_HOOK_CODE, hook)

    def call(entry, this, arguments):
        esp = stack + 0xF000
        for i, value in enumerate([stop] + arguments):
            put32(esp + 4 * i, value)
        uc.reg_write(UC_X86_REG_ESP, esp)
        uc.reg_write(UC_X86_REG_ECX, this)
        uc.emu_start(entry, stop + 1, count=100000)
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == esp + 4 + 4 * len(arguments)

    put32(node + 0x80, 1)
    put32(node + 0x84, children)
    put32(children, tag)
    put32(tag, 0x10D7A04)
    put32(tag + 0x1C, mesh)
    uc.mem_write(tag + 0x1A, b"\x51")
    uc.mem_write(mesh + 0x5C, selection["mesh_name"].encode("ascii") + b"\0")
    put32(mesh + 0x70, 15)  # Native small-string representation.
    put32(drawable + 0x360, 3)
    put32(drawable + 0x364, slots)
    uc.mem_write(pram, asset[13808:15993])
    # The selected material is deliberately not first in the synthetic array.
    for i, name in enumerate(("unrelated", selection["material_name"], "later")):
        slot, resource, definition = (
            slots + 0x54 * i,
            resources + 0x100 * i,
            definitions + 0x100 * i,
        )
        put32(slot, 0x10D646C)
        put32(slot + 0x38, resource)
        uc.mem_write(slot + 0x40, name.encode("ascii") + b"\0")
        put32(resource + 0x44, definition)
        put32(definition + 0x30, pram)
    call(0xC34700, node, [drawable])
    assert get32(tag + 0xB0) == slots + 0x54
    assert get32(mesh + 0x74) == definitions + 0x100
    assert not uc.mem_read(tag + 0x1A, 1)[0] & 2
    assert uc.mem_read(tag + 0x133, 1)[0] == asset[13811]
    put32(drawable + 0x39C, collected)
    put32(drawable + 0x3A0, collected + 4)
    put32(collected, tag)
    for entry, expected in ((0xC39550, 0), (0xC39620, 1)):
        dispatches.clear()
        call(entry, drawable, [data + 0xC000])
        assert dispatches == ([tag] if expected else [])
    uc.mem_write(slots + 0x40, selection["material_name"].encode("ascii") + b"\0")
    call(0xC34700, node, [drawable])
    assert get32(tag + 0xB0) == slots
    assert get32(mesh + 0x74) == definitions
    # A nonmatching name leaves the reset tag unbound; it is not auto-admitted.
    uc.mem_write(mesh + 0x5C, b"not_selected\0")
    put32(tag + 0xB0, 0)
    call(0xC34700, node, [drawable])
    assert get32(tag + 0xB0) == 0
    return dict(
        matching_slot=1,
        duplicate_name_first_slot=0,
        slot9_dispatches=0,
        slot10_dispatches=1,
        unmatched_binding=0,
        boundary="Synthetic slots, resource associations, runtime mesh and tag; native binder, strcmp, material/name/PRAM getters and Drawable slot9/10 partition execute. MeshDrawTag draw entry is a stub. Resource loader, actor, pass context and draw are not executed by this fragment.",
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "asset", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    if not __debug__:
        parser.error("run without Python optimization")
    if args.output.exists():
        parser.error("--output must name a new file")
    raw, asset = args.exe.read_bytes(), args.asset.read_bytes()
    assert hashlib.sha256(raw).hexdigest().upper() == PIN
    assert hashlib.sha256(asset).hexdigest().upper() == ASSET_PIN
    pe = pefile.PE(data=raw, fast_load=True)
    assert pe.OPTIONAL_HEADER.ImageBase == 0x400000 and pe.FILE_HEADER.Machine == 0x14C
    selection = selected_asset(asset)
    result = dict(
        exe_sha256=PIN,
        asset_sha256=ASSET_PIN,
        anchors=native_anchors(pe),
        order=prefix_order(pe),
        selection=selection,
        binding=binding_replay(pe.get_memory_mapped_image(), asset, selection),
        prefix=prefix_replay(pe.get_memory_mapped_image()),
        boundary="Separate static, binder and mesh-prefix checks on synthetic objects. The prefix executes real selection/override publication and matrix builder/helpers, with the fixed STUB_RETURNS boundaries, and stops before the float submission gateway body. No actor loading, complete DrawManager pass, constant cache/queue, draw or lifetime proof.",
    )
    with args.output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    print(
        "PASS: body mesh/material binding, list partition and four native pre-submission cases"
    )


if __name__ == "__main__":
    main()
