"""Check one pinned body PRAM/program join and finite native submission paths.

Requires pefile and Unicorn 2.1.4. Wrappers, tables and contexts are synthetic.
No client process, device, GPU, queue or complete mesh draw is executed.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX,
    UC_X86_REG_ECX,
    UC_X86_REG_ESP,
    UC_X86_REG_EIP,
    UC_X86_REG_ESI,
)

PIN = "9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9"
ASSET_PIN = "21570ADE19172245ED9BB541FA3C2C7D4A74D5CD8F19500468ABCE51F788D093"
SHADER_PIN = "DD98D6ECD1F274E91E0B36151AD06735A84F0070E84C7F819748BABFA302446A"


def h(data):
    return hashlib.sha256(data).hexdigest().upper()


def fnv(name):
    value = 0x811C9DC5
    for b in name.encode("ascii"):
        value = ((value * 0x1000193) & 0xFFFFFFFF) ^ b
    return value


def parse_ctab(data):
    assert data[504:508] == b"CTAB"
    base = 508
    size, creator, version, count, infos, flags, target = struct.unpack_from(
        "<7I", data, base
    )
    assert size == 28 and version == 0xFFFE0300

    def cstr(off):
        return data[off : data.index(b"\0", off)].decode("ascii")

    out = []
    for i in range(count):
        name, bank, reg, nreg, res, typ, default = struct.unpack_from(
            "<IHHHHII", data, base + infos + 20 * i
        )
        clas, kind, rows, cols, elements, members, memberinfo = struct.unpack_from(
            "<6HI", data, base + typ
        )
        out.append(
            dict(
                name=cstr(base + name),
                bank=bank,
                register=reg,
                count=nreg,
                elements=elements,
            )
        )
    return out


def table_records(ctab):
    # Native 419F80: omit zero RegisterCount; strip leading '$'; expand each
    # multi-element constant into name[index], register=base+index; sort FNV1.
    records = []
    for c in ctab:
        if not c["count"]:
            continue
        name = c["name"].removeprefix("$")
        for i in range(c["elements"]):
            n = name if c["elements"] == 1 else f"{name}[{i}]"
            records.append(
                dict(name=n, hash=fnv(n), register=c["register"] + i, bank=c["bank"])
            )
    records.sort(key=lambda c: c["hash"])
    assert len({r["hash"] for r in records}) == len(records)
    for i, r in enumerate(records):
        r["handle"] = i + 1
    return records


def vertex_parameters(asset, records):
    chunk, body = 13792, 13808
    assert asset[chunk : chunk + 4] == b"PRAM"
    count, data_offsets, name_offsets = struct.unpack_from("<3I", asset, body + 8)
    assert count == 27
    names = {r["name"]: r for r in records}
    rows = []
    for index in range(count):
        data = (
            body + struct.unpack_from("<I", asset, body + data_offsets + 4 * index)[0]
        )
        name = (
            body + struct.unpack_from("<I", asset, body + name_offsets + 4 * index)[0]
        )
        text = asset[name : asset.index(b"\0", name)].decode("ascii")
        if asset[data] == 0:
            record = names[text]
            assert record["bank"] == 2
            rows.append(dict(index=index, data=data - body, **record))
    assert [(r["index"], r["name"], r["register"]) for r in rows] == [
        (0, "EnableShadowFlag", 177),
        (2, "uvofs0", 178),
        (9, "fogParam", 179),
    ]
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--exe", type=Path, required=True)
    p.add_argument("--asset", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    if not __debug__:
        p.error("run without Python optimization so verification assertions execute")
    if a.output.exists():
        p.error("--output must name a new file")
    raw = a.exe.read_bytes()
    asset = a.asset.read_bytes()
    assert h(raw) == PIN and len(raw) == 15996808
    assert h(asset) == ASSET_PIN and h(asset[496 : 496 + 2952]) == SHADER_PIN
    pe = pefile.PE(data=raw, fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    assert base == 0x400000 and pe.FILE_HEADER.Machine == 0x14C
    ctab = parse_ctab(asset)
    records = table_records(ctab)
    parameters = vertex_parameters(asset, records)
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    image = pe.get_memory_mapped_image()
    machine.mem_map(base, (len(image) + 0xFFF) & ~0xFFF)
    machine.mem_write(base, image)
    heap = 0x2000000
    stack = 0x3000000
    stop = 0x4000000
    machine.mem_map(heap, 0x100000)
    machine.mem_map(stack, 0x100000)
    machine.mem_map(stop, 0x1000)

    def read32(addr):
        return struct.unpack("<I", machine.mem_read(addr, 4))[0]

    def write32(addr, value):
        machine.mem_write(addr, struct.pack("<I", value))

    def cstr(addr):
        data = bytearray()
        while True:
            b = machine.mem_read(addr, 1)[0]
            addr += 1
            if not b:
                return data.decode("ascii")
            data.append(b)

    def ret(value, pop=0):
        esp = machine.reg_read(UC_X86_REG_ESP)
        dest = read32(esp)
        machine.reg_write(UC_X86_REG_EAX, value)
        machine.reg_write(UC_X86_REG_ESP, esp + 4 + pop)
        machine.reg_write(UC_X86_REG_EIP, dest)

    queries = []
    appends = []
    destroyed = []
    writes = []
    source_calls = []
    visited = []
    bindings = []

    def hook(uc, addr, size, user):
        esp = uc.reg_read(UC_X86_REG_ESP)
        if addr == 0x9D20F4:  # Explicit security-cookie boundary only.
            ret(uc.reg_read(UC_X86_REG_EAX))
        elif addr == 0x9D4F08:  # sprintf boundary; all init formats are one %d.
            dst, fmt, value = (read32(esp + i) for i in (4, 8, 12))
            fmt = cstr(fmt)
            assert fmt.count("%d") == 1
            text = (fmt % value).encode("ascii")
            uc.mem_write(dst, text + b"\0")
            ret(len(text))
        elif addr == 0x420900:
            queries.append(
                dict(name=cstr(read32(esp + 8)), callsite=f"{read32(esp) - 5:08X}")
            )
        elif addr == 0xC2EEF0:
            appends.append(
                dict(
                    header=uc.reg_read(UC_X86_REG_ECX) - container,
                    driver=read32(read32(esp + 4)) - driver,
                )
            )
            ret(0, 4)
        elif addr in (0xC2D130, 0xC2D160, 0xC23A60):
            destroyed.append(f"{addr:08X}")
            ret(0, 4)
        elif addr == 0x422F90:
            reg, src, count = (read32(esp + i) for i in (4, 8, 12))
            writes.append(
                dict(callsite=f"{read32(esp) - 5:08X}", register=reg, count=count)
            )
            ret(0, 12)  # No renderer/cache/device executes.
        elif addr == 0x41BF10:
            ret(0)  # cdecl bool-bank submission boundary.
        elif addr in (0x9FE250, 0x9FE770, 0x9FC750):
            ret(0)  # Profiling boundaries, cdecl or tail return.
        elif addr == 0x9FC74A:
            ret(0, 8)  # Profiling scope entry.
        elif addr == 0x4180B0:
            ret(0)  # Select the ordinary shader-device mode.
        elif addr == 0x423210:
            bindings.append(read32(esp + 4))
            ret(0, 4)  # Renderer shader-binding boundary; no device executes.
        elif addr == 0x41C460:
            ret(0)  # Pixel shader binding is outside the vertex join.
        elif addr == 0xC3D790:
            destination = read32(esp + 4)
            write32(destination, container)
            write32(destination + 4, container)
            ret(destination, 16)  # Explicit upstream selection-pair boundary.
        elif addr in (0xC20A70, 0xC20B80):
            dst = read32(esp + 4)
            source_calls.append(f"{addr:08X}")
            uc.mem_write(dst, struct.pack("<12f", *([1.0] * 12)))
            ret(dst)
        elif addr in (0xC216B0, 0xC21A60):
            visited.append(f"{addr:08X}")
        elif addr == stop:
            uc.emu_stop()

    machine.hook_add(UC_HOOK_CODE, hook)
    wrapper = heap + 0x1000
    table = heap + 0x2000
    container = heap + 0x4000
    driver = heap + 0x5000
    write32(wrapper + 8, table)
    write32(wrapper + 12, len(records))
    write32(container + 0x1C, wrapper)
    machine.mem_write(
        table, b"".join(struct.pack("<II", r["hash"], r["register"]) for r in records)
    )
    results = []
    for name, entry, vtable, ctor, size in [
        ("WaveAnim", 0xC65410, 0x10D4604, None, 0xA0),
        ("VertexLight", 0xC20EA0, 0x10D3BB0, 0xC209D0, 0xF4),
        ("PRT", 0xC66240, 0x10D4618, None, 0x10C),
    ]:
        machine.mem_write(driver, b"\0" * 0x1000)
        write32(driver, vtable)

        def call(entry, args, this=driver):
            esp = stack + 0x80000
            for i, v in enumerate([stop] + args):
                write32(esp + 4 * i, v)
            machine.reg_write(UC_X86_REG_ESP, esp)
            machine.reg_write(UC_X86_REG_ECX, this)
            try:
                machine.emu_start(entry, stop + 1, count=2000000)
            except Exception as exc:
                raise AssertionError(
                    f"native failure EIP={machine.reg_read(UC_X86_REG_EIP):08X}, ESP={machine.reg_read(UC_X86_REG_ESP):08X}, ECX={machine.reg_read(UC_X86_REG_ECX):08X}, EAX={machine.reg_read(UC_X86_REG_EAX):08X}, ESI={machine.reg_read(UC_X86_REG_ESI):08X}"
                ) from exc
            assert machine.reg_read(UC_X86_REG_EIP) == stop
            assert machine.reg_read(UC_X86_REG_ESP) == esp + 4 + 4 * len(args)
            return machine.reg_read(UC_X86_REG_EAX)

        if ctor:
            call(ctor, [])
        queries.clear()
        result = call(entry, [container]) & 0xFF
        lookup = {r["name"]: r for r in records}
        fields = []
        for off in range(4, size, 4):
            value = read32(driver + off)
            if value:
                r = records[value - 1]
                fields.append(dict(offset=f"{off:X}", **r))
        for q in queries:
            q.update(
                {k: lookup.get(q["name"], {}).get(k, 0) for k in ["handle", "register"]}
            )
        initial_queries = [q.copy() for q in queries]
        queries.clear()
        appends.clear()
        destroyed.clear()
        call(0xC57470 if name == "VertexLight" else 0xC57430, [driver], container)
        assert bool(appends) == bool(result) and bool(destroyed) != bool(result)
        assert [q["name"] for q in queries] == [q["name"] for q in initial_queries]
        results.append(
            dict(
                name=name,
                entry=f"{entry:08X}",
                accepted=bool(result),
                queries=initial_queries,
                nonzero_fields=fields,
                append=list(appends),
                destroyed=list(destroyed),
            )
        )
        if name == "VertexLight":
            light_payload = bytes(machine.mem_read(driver, 0xF4))
    assert [r["accepted"] for r in results] == [False, True, False]
    light = results[1]
    assert [
        (f["offset"], f["name"], f["register"]) for f in light["nonzero_fields"]
    ] == [
        ("8C", "PointLightPositions[0]", 171),
        ("90", "PointLightColors[0]", 173),
        ("94", "PointLightParams[0]", 175),
        ("98", "PointLightPositions[1]", 172),
        ("9C", "PointLightColors[1]", 174),
        ("A0", "PointLightParams[1]", 176),
        ("C4", "fogParam", 179),
    ], light["nonzero_fields"]
    for r in records:
        machine.mem_write(heap + 0x8000, r["name"].encode() + b"\0")
        assert call(0x420900, [0, heap + 0x8000], wrapper + 8) == r["handle"]

    # The same native candidate getter is used by selection and default records.
    write32(container, 0x10F5B78)
    machine.mem_write(container + 0x20, asset[13728:13744])
    entry_wrapper = heap + 0x30000
    write32(entry_wrapper + 0x14, container)
    selector, entries, request = heap + 0x31000, heap + 0x32000, heap + 0x33000
    write32(selector + 4, entries)
    write32(selector + 8, entries + 8)
    write32(entries, entry_wrapper)
    write32(entries + 4, container)
    selector_cases = []
    for masks in ((0, 0), (0x30, 0x100), (0xFFFFFFFF, 0xFFFFFFFF), (1, 0)):
        machine.mem_write(request, struct.pack("<2I", *masks))
        assert call(0xC66F50, [request, heap + 0x34000, 0], selector) == container
        assert read32(heap + 0x34000) == entry_wrapper
        selector_cases.append(list(masks))

    # Execute post-override publication, then use its actual selected wrapper
    # for bbox lookup and the native final binding, including the cache hit.
    mesh, geometry = heap + 0x35000, heap + 0x36000
    material, resource, definition = heap + 0x37000, heap + 0x38000, heap + 0x39000
    context = heap + 0x10000
    write32(mesh + 0x1C, geometry)
    write32(mesh + 0xB0, material)
    write32(mesh + 0x118, geometry)
    write32(mesh + 0x30, 0xFFFFFFFF)
    write32(material + 0x38, resource)
    write32(resource + 0x44, definition)
    write32(wrapper + 4, 0x12345678)  # A numeric mock handle, not a D3D object.
    override_container, override_wrapper = heap + 0x3A000, heap + 0x3B000
    write32(override_container + 0x1C, override_wrapper)
    machine.mem_write(override_wrapper, bytes(machine.mem_read(wrapper, 16)))
    write32(override_wrapper + 4, 0x23456789)
    binding_cases = []
    for override in (0, override_container):
        machine.mem_write(context, b"\0" * 0x2000)
        write32(context + 0xBA4, override)
        write32(0x1328DAC, 0)
        write32(0x1328DB0, wrapper)
        bindings.clear()
        call(0xC41990, [context], mesh)
        selected = override or container
        assert read32(0x1374CB0) == selected
        writes.clear()
        call(0xC43340, [], mesh)
        assert [(w["register"], w["count"]) for w in writes] == [(169, 1), (170, 1)]
        call(0xC41AA0, [context], mesh)
        call(0xC41AA0, [context], mesh)
        expected_wrapper = read32(selected + 0x1C)
        assert bindings == [read32(expected_wrapper + 4)]
        assert read32(0x1328DAC) == expected_wrapper
        binding_cases.append(
            dict(
                override=bool(override),
                in_profile=not bool(override),
                bbox=list(writes),
                binding_count=1,
            )
        )

    # Selected vertex mutable entries use synthetic values and actual PRAM names.
    pram, mutable = heap + 0x40000, heap + 0x44000
    machine.mem_write(pram, asset[13808:15993])
    write32(definition + 0x30, pram)
    write32(material + 8, mutable)
    write32(material + 12, mutable + 0x30 * len(parameters))
    write32(0x1374CB0, container)
    for i, parameter in enumerate(parameters):
        write32(mutable + 0x30 * i, parameter["index"])
        machine.mem_write(mutable + 0x30 * i + 4, struct.pack("<4f", *([i + 1.0] * 4)))
    writes.clear()
    call(0xC3EA90, [context], material)
    assert [(w["register"], w["count"]) for w in writes] == [
        (177, 1),
        (178, 1),
        (179, 1),
    ]
    mutable_writes = list(writes)

    # Default-record arrays are populated from the statically checked initializer
    # rule; the hot writer and candidate getter then execute without a stub.
    default_record, handles, indices = heap + 0x45000, heap + 0x46000, heap + 0x47000
    write32(default_record + 4, entry_wrapper)
    write32(default_record + 8, len(parameters))
    write32(default_record + 12, handles)
    write32(default_record + 16, indices)
    for i, parameter in enumerate(parameters):
        write32(handles + 4 * i, parameter["handle"])
        machine.mem_write(indices + 2 * i, struct.pack("<h", parameter["index"]))
    writes.clear()
    call(0xC3E8B0, [pram], default_record)
    assert [(w["register"], w["count"]) for w in writes] == [
        (177, 1),
        (178, 1),
        (179, 1),
    ]
    default_writes = list(writes)
    # Finite native writer dispatch/helper replay, varying fog and light inputs.
    # Source derivation helpers are explicit stubs, since only destinations/counts
    # are tested. Native writer selection, tests, loops and named setter execute.
    writer_cases = []
    context = heap + 0x10000
    lighting = heap + 0x20000
    for fog in range(3):
        for kinds in [[], [3], [3, 3], [3, 3, 3, 3], [1], [1, 3], [1, 1, 3, 3], [4, 3]]:
            machine.mem_write(driver, light_payload)
            machine.mem_write(context, b"\0" * 0x2000)
            machine.mem_write(lighting, b"\0" * 0x1000)
            write32(context + 0x308, lighting)
            machine.mem_write(container + 0x58, b"\1")
            machine.mem_write(context + 0x773, b"\1")
            machine.mem_write(lighting + 0x150, bytes([len(kinds)]))
            for i, kind in enumerate(kinds):
                light_obj = lighting + 0x200 + 0x100 * i
                write32(light_obj + 0xDC, kind)
                write32(lighting + 0x30 + 0x24 * i, light_obj)
            if fog:
                machine.mem_write(context + 0x4E3 + fog, b"\1")
            machine.mem_write(context + 0x31C, struct.pack("<4f", 10, 1, 1, 1))
            writes.clear()
            source_calls.clear()
            visited.clear()
            call(0xC22A90, [context, container])
            assert visited == ["00C21A60"], visited
            assert all(
                w["count"] == 1 and w["register"] in [171, 172, 173, 174, 175, 176, 179]
                for w in writes
            ), writes
            point_count = sum(k == 3 for k in kinds)
            expected = (
                (3 * min(point_count, 2))
                + (max(0, 2 - point_count))
                + (1 if fog else 0)
            )
            assert len(writes) == expected, (fog, kinds, writes)
            assert sum(w["register"] == 179 for w in writes) == bool(fog)
            writer_cases.append(
                dict(
                    fog=fog,
                    light_types=kinds,
                    writes=list(writes),
                    source_stubs=list(source_calls),
                )
            )
    output = dict(
        exe_sha256=PIN,
        asset_sha256=ASSET_PIN,
        shader_sha256=SHADER_PIN,
        ctab=ctab,
        records=records,
        vertex_parameters=parameters,
        results=results,
        writer_cases=writer_cases,
        selector_cases=selector_cases,
        binding_cases=binding_cases,
        mutable_writes=mutable_writes,
        default_writes=default_writes,
        boundary="Synthetic objects and a modeled CTAB-to-hash table. Native name lookup, driver initializers/append decisions, VertexLight destinations, single-entry selector, bbox lookup, post-override publication, ordinary final binding/cache hit, mutable and default vertex writers execute. Explicit stubs: sprintf, cookie/profiling, append allocation/destruction, light source conversion, shader mode, upstream selection pair, float/bool submission and renderer/pixel binding. Default record arrays are manually populated by the checked initialization rule. No full loader/factory, complete mesh path, draw, renderer cache/queue or GPU executes.",
    )
    with a.output.open("x", encoding="ascii") as stream:
        stream.write(json.dumps(output, indent=2) + "\n")
    print(
        f"PASS: identity, CTAB, native init/append acceptance and fields, {len(writer_cases)} native writer destination/count cases"
    )


if __name__ == "__main__":
    main()
