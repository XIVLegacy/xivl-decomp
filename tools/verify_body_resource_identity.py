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
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ
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
KEY = 1000476
RTTI_SHA256 = "DE199A40B0BC723FC9CDC9E9F6FFEC087C91277AB01110A525C8D3376571A5D6"
PROBE_WRITERS = {
    "D1EE2C7719686656749CE8F98CCB00B987E919C5107E4744AA164F8637DE3C7B": (
        5,
        0x1004EC80,
        607,
        0x1004EDF9,
        0x1004EE05,
        0x1004EE16,
        0x10051E00,
        166,
    ),
    "55D0BEE4F8690E589B91CCC0E9B13FF9C56BFFA51D22B5A0F0D44138FBF58B8A": (
        5,
        0x10050CE0,
        607,
        0x10050E59,
        0x10050E65,
        0x10050E76,
        0x10052AD0,
        166,
    ),
    "964151C767A834D7A4506C4D8D705411B8A78F6828AC2CB5AA05ED1A1C5E85C5": (
        4,
        0x100515A0,
        445,
        0x100516E7,
        0x100516F3,
        0x10051704,
        0x1004FC60,
        147,
    ),
    "A15C89BC2C169298B88985DDB9C66C962AA6093F01EF30AB8A54241C28E2D07E": (
        4,
        0x1004EE20,
        445,
        0x1004EF67,
        0x1004EF73,
        0x1004EF84,
        0x1003C6C0,
        147,
    ),
}
PROBE_KEYS = {
    "FD0CBDD4E63C3C5E4EF895FF65CA3D95804EC8DF9F92C500D56A2BA7B985259A": (
        0x10040470,
        "115197FCFA85BA04D6FBA62A4247027C8D79018C8138605B6EBDCD644C5587F3",
    ),
    "1A40DF663FD3C82E0E4EDCD2FF4E28A51942EC6F8C20827B89A8C7FFB4FBBB1D": (
        0x100490A0,
        "7E9218D723D418E467A9CAA597D6CD721873DD8E4F3D6373BC4F1303B72F2D65",
    ),
}
RETAINED_INPUTS = {
    "48B2DDAC9520A3C1F723914BFCFF3AE58A3D4BB53B79F28A74175E8C1FC81377": (
        "primary-label-context-probe-stationary-no-draw.bin",
        30527624,
    ),
    "88AD2E82A96E567386C7E37F8623F9ECAEED3674CAE77FF7927A9712BCD1424A": (
        "camera-cadence.bin",
        960024,
    ),
    "6BED727FE0F74230013EF4939251766D410EA59827158EFDACE6664344BD26A7": (
        "player-render-adjustment-probe.bin",
        1696264,
    ),
    "20776FCDC349314EEB0424D7455668118B10C2FE34C7956ED0F26942931D3984": (
        "nameplate-ui-identity.bin",
        64,
    ),
    "7EA4366A6482695900DF7AB34CC17ACBE43A1B7DB6BFD85CC7E9E1E292D158AA": (
        "player-render-probe.bin",
        128,
    ),
}


def native_anchors(pe):
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    anchors = [
        (0x7F8D38, "mov", "eax, dword ptr [esi + 0x1ac]"),
        (0x7F8D3E, "mov", "ecx, dword ptr [esi + 0x118]"),
        (0x7F8D46, "add", "ecx, 0x54"),
        (0x7F8D49, "call", "0x7a0150"),
        (0x7A0156, "lea", "esi, [ecx + 0x30]"),
        (0x7A0165, "call", "0x71d420"),
        (0x7A0199, "mov", "eax, dword ptr [esi + 0x10]"),
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


def registry_replay(image):
    results = []
    for node_key, tag in ((KEY - 1, 0xFC0D34), (KEY, 0xFC0D34), (KEY, 0x1057EE4)):
        uc = machine(image)
        scene, head, node, value, stop, stack = (
            0x2000000,
            0x2001000,
            0x2001100,
            0x2002000,
            0x200F000,
            0x3008000,
        )

        def put(address, *values):
            uc.mem_write(address, struct.pack("<" + "I" * len(values), *values))

        # Minimal one-node tree; first-word tags are not constructed actors.
        put(scene + 0x84 + 4, head)
        put(head, node, node, node)
        uc.mem_write(head + 0x15, b"\1")
        put(node, head, head, head, node_key, value)
        put(value, tag)
        put(stack, stop, KEY)
        uc.reg_write(UC_X86_REG_ECX, scene + 0x54)
        uc.reg_write(UC_X86_REG_ESP, stack)
        visited, object_reads = set(), []

        def code(_uc, address, _size, _user):
            visited.add(address)
            assert (
                address == stop
                or 0x7A0150 <= address < 0x7A01B0
                or 0x71D420 <= address < 0x71D489
            ), hex(address)
            if address == stop:
                uc.emu_stop()

        def read(_uc, _access, address, size, _value, _user):
            if address < value + 0x100 and address + size > value:
                object_reads.append((address, size))

        uc.hook_add(UC_HOOK_CODE, code)
        uc.hook_add(UC_HOOK_MEM_READ, read)
        uc.emu_start(0x7A0150, 0, count=1000)
        expected = value if node_key == KEY else 0
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 8
        assert uc.reg_read(UC_X86_REG_EAX) == expected
        assert {0x7A0150, 0x71D420} <= visited and not object_reads
        results.append(
            dict(
                query=KEY,
                node_key=node_key,
                returned_value=hex(expected),
                synthetic_first_word=hex(tag),
                pointed_object_reads=0,
                boundary="Native wrapper/tree search, no stubs. Minimal synthetic map and first-word tags; no actor constructor or legal real-registry binding is established.",
            )
        )
    return results


def v3_vtable_literals(pe, data):
    catalog_bytes = (
        Path(__file__).resolve().parent.parent / "config/ffxivgame.rtti.json"
    ).read_bytes()
    if hashlib.sha256(catalog_bytes).hexdigest().upper() != RTTI_SHA256:
        raise ValueError("RTTI catalog SHA-256 mismatch")
    catalog = json.loads(catalog_bytes)
    assert catalog["metadata"]["source_sha256"].upper() == PIN
    tables = {BASE + row["rva"]: row for row in catalog["classes"]}
    assert len(tables) == len(catalog["classes"]) == 5719
    # Validate the cataloged entry -> COL pointers against the pinned PE.
    for table, row in tables.items():
        assert struct.unpack("<I", pe.get_data(table - 4 - BASE, 4))[0] == (
            BASE + row["locator_rva"]
        )
    hits = {}
    for record in struct.iter_unpack("<206I", data[72:]):
        for word, value in enumerate(record):
            if value in tables:
                assert record[1] == 2
                hits.setdefault(value, Counter())[word * 4] += 1
    assert hits == {
        0x106E844: {252: 1196},
        0x1074554: {284: 1196, 348: 1196, 380: 1196},
        0x107F304: {316: 1196},
        0x107A9B4: {412: 1196},
        0x106A0BC: {444: 1196},
        0xF912E4: {476: 1196},
    }
    matches = []
    for table, offsets in hits.items():
        locator = BASE + tables[table]["locator_rva"]
        descriptor = struct.unpack("<I", pe.get_data(locator + 12 - BASE, 4))[0]
        name = pe.get_data(descriptor + 8 - BASE, 256).split(b"\0", 1)[0]
        assert name.startswith(b".?AV") and name.endswith(b"@@")
        matches.append(
            dict(
                table_va=hex(table),
                col_va=hex(locator),
                type_descriptor_va=hex(descriptor),
                original_rtti=name.decode("ascii"),
                raw_record_offsets=dict(offsets),
            )
        )
    return dict(
        catalog_sha256=RTTI_SHA256,
        cataloged_entry_vas=5719,
        matches=matches,
        boundary="Aligned words of the exact v3 record bodies only. Literal matches to cataloged fixed-image entry VAs do not type any captured pointee or assign a field schema. No other cataloged entry VA occurs there; encoded, indirect, unaligned, rebased or uncataloged type evidence remains outside this check.",
    )


def probe_key_replay(pe, digest):
    entry, body_pin = PROBE_KEYS[digest]
    base = pe.OPTIONAL_HEADER.ImageBase
    assert pe.FILE_HEADER.Machine == 0x14C and base == 0x10000000
    code = pe.get_data(entry - base, 62)
    assert hashlib.sha256(code).hexdigest().upper() == body_pin
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    page = entry & ~0xFFF
    uc.mem_map(page, 0x1000)
    uc.mem_write(page, pe.get_data(page - base, 0x1000))
    uc.mem_map(0x30000000, 0x10000)
    stack, stop = 0x30008000, 0x3000F000
    uc.mem_write(stack, struct.pack("<2I", stop, 0x79))
    uc.reg_write(UC_X86_REG_ESP, stack)

    def code_hook(_uc, address, _size, _user):
        assert address == stop or entry <= address < entry + 62

    def read_hook(_uc, _access, address, size, _value, _user):
        assert stack - 0x1000 <= address and address + size <= stack + 8

    uc.hook_add(UC_HOOK_CODE, code_hook)
    uc.hook_add(UC_HOOK_MEM_READ, read_hook)
    uc.emu_start(entry, stop, count=1000)
    assert uc.reg_read(UC_X86_REG_EIP) == stop
    assert uc.reg_read(UC_X86_REG_ESP) == stack + 4
    assert uc.reg_read(UC_X86_REG_EAX) == 0
    return dict(
        kind="built-in key predicate",
        sha256=digest,
        predicate_va=hex(entry),
        predicate_bytes=62,
        predicate_sha256=body_pin,
        key=0x79,
        result=0,
        boundary="Native F10 predicate control with no stubs or enable-flag reads. Only predicate code and synthetic stack/stop are mapped. This excludes its built-in F10 arming path; other initialization, indirect/plugin, writer and historical load paths remain unqualified.",
    )


def inspect_probe_dll(data):
    if not __debug__:
        raise ValueError("optimized Python is unsupported")
    digest = hashlib.sha256(data).hexdigest().upper()
    if digest not in PROBE_WRITERS and digest not in PROBE_KEYS:
        raise ValueError("probe DLL SHA-256 mismatch")
    pe = pefile.PE(data=data, fast_load=True)
    if digest in PROBE_KEYS:
        result = probe_key_replay(pe, digest)
        if digest == "1A40DF663FD3C82E0E4EDCD2FF4E28A51942EC6F8C20827B89A8C7FFB4FBBB1D":
            result["callback_sink"] = probe_sink_replay(pe)
        return result
    version, flush, flush_size, multiply, push_size, call, publish, publish_size = (
        PROBE_WRITERS[digest]
    )
    base = pe.OPTIONAL_HEADER.ImageBase
    assert pe.FILE_HEADER.Machine == 0x14C and base == 0x10000000
    header_size, header = (92, 0x1034E0F8) if version == 5 else (76, 0x101570F8)
    assert struct.unpack("<4I", pe.get_data(header - base, 16)) == (
        0x434C5050,
        version,
        1180,
        131072,
    )
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    anchors = (
        (multiply, "imul", "eax, edi, 0x49c" if version == 5 else "eax, esi, 0x49c"),
        (multiply + 6, "push", "eax"),
        (push_size, "push", hex(header_size)),
        (push_size + 2, "push", hex(header)),
        (call, "call", hex(publish)),
    )
    for va, mnemonic, operands in anchors:
        instruction = next(decoder.disasm(pe.get_data(va - base, 15), va))
        assert (instruction.mnemonic, instruction.op_str) == (mnemonic, operands)
    for va, size in ((flush, flush_size), (publish, publish_size)):
        code = pe.get_data(va - base, size)
        assert len(code) == size
        assert sum(i.size for i in decoder.disasm(code, va)) == size
    return dict(
        kind="direct-writer argument setup",
        sha256=digest,
        version_initializer=version,
        flush_va=hex(flush),
        flush_body_bytes=flush_size,
        record_multiply_va=hex(multiply),
        header_size_push_va=hex(push_size),
        publish_call_va=hex(call),
        publish_va=hex(publish),
        publish_body_bytes=publish_size,
        serialized_header_bytes=header_size,
        native_record_byte_multiplier=1180,
        boundary="Pinned native direct-writer argument setup and contiguous PDB-sized body decoding only. Its header/record sizes differ from v3 72/824. Validation, OS file I/O and alternate or dormant writer paths are not executed or exhaustively excluded; no capture-to-loaded-DLL attribution or v3 schema is established.",
    )


def probe_sink_replay(pe):
    base = pe.OPTIONAL_HEADER.ImageBase
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    anchors = (
        (0x1004086A, "mov", "dword ptr [ebp - 0x44], 0x10040bb0"),
        (0x10037432, "mov", "dword ptr [0x10248a28], edx"),
        (0x10037438, "mov", "dword ptr [0x10248a2c], eax"),
        (0x10037459, "call", "0x100031fc"),
        (0x10037A28, "mov", "edx, dword ptr [0x10248a28]"),
        (0x10037A2E, "mov", "eax, dword ptr [0x10248a2c]"),
        (0x10037B0B, "call", "dword ptr [ebp - 0x9c]"),
        (0x10037B62, "call", "dword ptr [ebp - 0xa0]"),
        (0x10037B70, "push", "edx"),
        (0x10037B73, "push", "eax"),
        (0x10037B7F, "pop", "eax"),
        (0x10037B80, "pop", "edx"),
        (0x10001127, "jmp", "0x10010ad0"),
        (0x10040BDA, "call", "0x10001127"),
        (0x1003764F, "mov", "edx, dword ptr [ecx]"),
        (0x10037657, "mov", "ecx, dword ptr [eax + 8]"),
        (0x10037669, "cmp", "dword ptr [ebp - 0x24], 0x2001"),
        (0x10010B59, "cmp", "esi, 0x2000"),
        (0x10010BCC, "cmp", "eax, 0x100"),
        (0x10010BD1, "jne", "0x10010c2d"),
        (0x10035AFC, "mov", "ecx, 0x1c"),
        (0x10010D78, "add", "eax, 0x1c"),
        (0x10010DCD, "movzx", "ecx, byte ptr [eax + 0x94]"),
        (0x10010DD6, "jne", "0x10010e1c"),
        (0x10010DE0, "cmp", "dword ptr [eax + 0x7c], 0"),
        (0x10010E14, "call", "0x10005bb4"),
        (0x10005BB4, "jmp", "0x10013770"),
        (0x1001378B, "push", "0x101e9be8"),
        (0x100137B5, "cmp", "eax, 6"),
        (0x10013802, "push", "0"),
        (0x10013804, "push", "0"),
        (0x10013806, "push", "1"),
        (0x1001380F, "call", "0x10001ec9"),
        (0x10001EC9, "jmp", "0x10164be0"),
    )
    for va, mnemonic, operands in anchors:
        instruction = next(decoder.disasm(pe.get_data(va - base, 15), va))
        assert (instruction.mnemonic, instruction.op_str) == (mnemonic, operands)
    assert pe.get_data(0x101E9A9C - base, 3) == b": \0"
    assert pe.get_data(0x101E9BE8 - base, 5) == b"chat\0"
    count, table = struct.unpack("<2I", pe.get_data(0x10037BA4 - base, 8))
    assert count == 3
    locals_seen = set()
    for i in range(count):
        offset, size, name = struct.unpack(
            "<iII", pe.get_data(table + 12 * i - base, 12)
        )
        label = pe.get_data(name - base, 32).split(b"\0", 1)[0].decode("ascii")
        locals_seen.add((offset, size, label))
    assert locals_seen == {
        (-40, 8, "sink"),
        (-76, 28, "sourceText"),
        (-112, 28, "messageText"),
    }
    results = []
    for context, source_size, message_size in (
        (0x20004000, 5, 7),
        (0x20005000, 0, 8192),
    ):
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        for page in (0x10001000, 0x10002000, 0x10010000, 0x10037000, 0x10040000):
            uc.mem_map(page, 0x1000)
            uc.mem_write(page, pe.get_data(page - base, 0x1000))
        uc.mem_map(0x20000000, 0x10000)
        frame, stack, stop = 0x20001000, 0x20000F00, 0x10010AD0
        source, message = (0x20006000, source_size), (0x20007000, message_size)

        def put(address, *words):
            uc.mem_write(address, struct.pack("<" + "I" * len(words), *words))

        def get(address):
            return struct.unpack("<I", uc.mem_read(address, 4))[0]

        put(frame - 0x28, context, 0x10040BB0)
        views = {frame - 0x4C: source, frame - 0x70: message}
        uc.reg_write(UC_X86_REG_EBP, frame)
        uc.reg_write(UC_X86_REG_ESP, stack)
        receipts = []

        def hook(_uc, at, _size, _user):
            esp = uc.reg_read(UC_X86_REG_ESP)
            if at == 0x10002CF2:
                view = views[uc.reg_read(UC_X86_REG_ECX)]
                out = get(esp + 4)
                put(out, *view)
                uc.reg_write(UC_X86_REG_EAX, out)
                uc.reg_write(UC_X86_REG_ESP, esp + 8)
                uc.reg_write(UC_X86_REG_EIP, get(esp))
            elif at == 0x10040BB0:
                receipt = [get(esp + 4 * i) for i in range(1, 6)]
                assert receipt == [context, *source, *message]
                receipts.append(receipt)
            else:
                assert (
                    0x10037AA3 <= at <= 0x10037B0B
                    or 0x10040BB0 <= at <= 0x10040BDA
                    or at in (0x10001127, stop)
                ), hex(at)

        uc.hook_add(UC_HOOK_CODE, hook)
        uc.emu_start(0x10037AA3, stop, count=500)
        assert uc.reg_read(UC_X86_REG_EIP) == stop and len(receipts) == 1
        assert uc.reg_read(UC_X86_REG_ECX) == context
        esp = uc.reg_read(UC_X86_REG_ESP)
        assert [get(esp + 4 * i) for i in range(1, 5)] == [*source, *message]
        results.append(
            dict(
                source_bytes=source_size,
                message_bytes=message_size,
                forwarded_text_dwords=4,
            )
        )
    return dict(
        callback_va="0x100379A0",
        sink_va="0x10010AD0",
        consumer_va="0x10010CD0",
        vm_dispatch_va="0x10013770",
        instruction_anchors=len(anchors),
        replays=results,
        boundary="Exact pinned probe and selected binding anchors. Native replay assumes successful earlier text decoding, supplies two synthetic pointer/length views through an explicit constructor stub, follows actual argument packing/wrapper/thunk, and stops at sink entry. Queue and VM observations use static anchors, not execution. Fresh successful binding installation and no intervening clear/replacement are conditions; no historical capture/module binding, Lua chat closure, v3 schema or actual Scene result/type is established.",
    )


def inspect_retained_capture(data, pe):
    if not __debug__:
        raise ValueError("optimized Python is unsupported")
    digest = hashlib.sha256(data).hexdigest().upper()
    if digest not in RETAINED_INPUTS:
        raise ValueError("retained capture SHA-256 mismatch")
    name, size = RETAINED_INPUTS[digest]
    assert len(data) == size
    needle, hits, start = struct.pack("<I", KEY), [], 0
    while (offset := data.find(needle, start)) != -1:
        hits.append(offset)
        start = offset + 1
    result = dict(name=name, sha256=digest, bytes=size, literal_key_hits=len(hits))
    if name != "primary-label-context-probe-stationary-no-draw.bin":
        assert not hits
        result["boundary"] = (
            "Exhaustive literal little-endian u32 key scan only; no semantic schema or absence of an indirect/encoded identity is proved."
        )
        return result
    header = struct.unpack_from("<18I", data)
    assert header[:3] == (0x434C5050, 3, 824)
    assert header[3] == 131072 and header[4] == header[5] == 37048
    assert header[6] == header[7] == 0 and header[17] == 20104
    assert len(data) == 72 + header[5] * 824
    tuples, zero_indices, raw_events = Counter(), [], Counter()
    for i in range(header[5]):
        base = 72 + i * 824
        prefix = struct.unpack_from("<3I", data, base)
        assert prefix[0] == 1 and prefix[2] == i
        raw_events[prefix[1]] += 1
        triple = struct.unpack_from("<3I", data, base + 0x20)
        tuples[triple] += 1
        if triple == (0, 0, 0):
            zero_indices.append(i)
    assert tuples == {(KEY, 0x2D780048, 155): 37047, (0, 0, 0): 1}
    assert zero_indices == [11304]
    assert hits == [72 + i * 824 + 0x20 for i in range(37048) if i != 11304]
    result.update(
        header_words=header,
        record_bytes=824,
        records=37048,
        raw_word4_counts=dict(raw_events),
        key_record_offset="0x20",
        raw_tuple_at_20=[dict(words=words, count=n) for words, n in tuples.items()],
        zero_tuple_indices=zero_indices,
        vtable_literals=v3_vtable_literals(pe, data),
        boundary="Pinned framing and raw offsets only. Full v3 producer/schema is unqualified; no opaque word is labeled as a Scene result/type. Other record bytes may contain unqualified data. This is a different instance from the accepted v5 capture.",
    )
    return result


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
    parser.add_argument("--retained-capture", type=Path, action="append", default=[])
    parser.add_argument("--probe-dll", type=Path, action="append", default=[])
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
        retained=[
            inspect_retained_capture(path.read_bytes(), pe)
            for path in args.retained_capture
        ],
        registry=registry_replay(image),
        probe_controls=[
            inspect_probe_dll(path.read_bytes()) for path in args.probe_dll
        ],
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
        "PASS: body pins, capture key coverage, native registry/appearance/request controls and conditional factory/PWIB copy"
    )


if __name__ == "__main__":
    main()
