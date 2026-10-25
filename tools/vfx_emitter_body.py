#!/usr/bin/env python3
"""Parse the emitter records of Qix veff bodies.

The layouts are the ones recorded in docs/resource/vfx-generate-emitters.md. Every
function works on caller-supplied bytes and on the arrays that
vfx_effect_body.parse_veff returned. Nothing here reads a client install on its own.
"""

from __future__ import annotations

import struct

import vfx_qix_resource as qix

NAME_PREFIX = {
    "cdev": "SQEX/CDev/Engine/Vfx/QixControl/Controls/",
    "rapture": "Application/Scene/Vfx/RaptureQixControl/QixControl/Controls/",
}
CONTROL_STRIDE = 0x24
LINK_STRIDE = 0x1C
UNIT_STRIDE = 0x38
GRAPH_STRIDE = 12
BINDING_STRIDE = 12
SLOT_STRIDE = 16
GROUP_STRIDE = 8
INSTANCE_STRIDE = 0x48
INSTANCE_FLAGS = 0x0F
UNIT_END_TICK = 0x28
POLYNOMIAL_WIDTHS = {"Float32": 1, "Float32X2": 2, "Float32X3": 3, "Float32X4": 4}

# GenerateMaster block record, relative to the record the block points at.
MASTER_CHILD_REF = 0x1C
MASTER_CHILD_KIND = 0x20
MASTER_GATE_FLAGS = 0x21
MASTER_AREA_REF = 0x24
MASTER_AREA_KIND = 0x28
# Area record, relative to the record the area reference points at.
AREA_FLAGS = 0x1E
# GenerateTime and GenerateTimeArea blocks are the record itself.
TIME_CHILD_POINTER = 0x0C
TIME_GATE_FLAGS = 0x10


def short_name(name: str | None, prefixes: dict = NAME_PREFIX) -> str:
    """Module name without its table prefix. Rapture names carry the R: marker."""
    if not name:
        return ""
    for table, prefix in prefixes.items():
        if name.startswith(prefix):
            return ("R:" if table == "rapture" else "") + name[len(prefix) :]
    return name


def control_at(arrays: dict, base: int, index: int) -> int:
    return base + arrays["controls"]["pointer"] + CONTROL_STRIDE * index


def control_module(data: bytes, base: int, arrays: dict, decls: list, index: int):
    """The declaration row that control index names."""
    declaration = qix.u16(data, control_at(arrays, base, index) + 0x1C)
    return decls[declaration] if declaration < len(decls) else None


def unit_control_indices(data: bytes, base: int, arrays: dict, unit: int) -> list[int]:
    """Control indices of a unit in node order, through the link array."""
    at = base + arrays["units"]["pointer"] + UNIT_STRIDE * unit
    pointer, count = qix.u32(data, at), qix.u32(data, at + 4)
    links = base + arrays["links"]["pointer"]
    out = []
    for number in range(count):
        link = qix.u16(data, base + pointer + 2 * number)
        out.append(qix.u16(data, links + LINK_STRIDE * link + 0x14))
    return out


def unit_graphs(data: bytes, base: int, arrays: dict, unit: int, names: dict, table):
    """Graph records of one list of a unit, table is standard or random.

    Each record is (module name, [bindings]) and a polynomial binding is
    (start ticks, [(degree, components, coefficients)]). Other modules keep an
    empty binding list.
    """
    list_at = 0x18 if table == "standard" else 0x20
    at = base + arrays["units"]["pointer"] + UNIT_STRIDE * unit
    pointer, count = qix.u32(data, at + list_at), qix.u32(data, at + list_at + 4)
    out = []
    for number in range(count):
        module, _, _, bindings_at, bindings = struct.unpack_from(
            "<HBBIH", data, base + pointer + GRAPH_STRIDE * number
        )
        name = names.get((table, module), "")
        rows = []
        for slot in range(bindings):
            first_at, starts_at, segments = struct.unpack_from(
                "<III", data, base + bindings_at + BINDING_STRIDE * slot
            )
            cursor = qix.u32(data, base + first_at)
            starts = [qix.u32(data, base + starts_at + 4 * k) for k in range(segments)]
            parsed = []
            for _ in range(segments):
                degree, components = data[base + cursor], data[base + cursor + 1]
                count_floats = components * (degree + 1)
                coefficients = struct.unpack_from(
                    f"<{count_floats}f", data, base + cursor + 4
                )
                parsed.append((degree, components, coefficients))
                cursor += 4 + 4 * count_floats
            rows.append((starts, parsed))
        out.append((name, rows))
    return out


def genesis_owners(
    data: bytes, base: int, arrays: dict, decls: list, unit: int, graphs: list
) -> list[str]:
    """Module names of the controls that hold a slot on a Genesis graph of the unit."""
    owners = []
    for control in unit_control_indices(data, base, arrays, unit):
        at = control_at(arrays, base, control)
        pointer, count = qix.u32(data, at), qix.u32(data, at + 4)
        module = control_module(data, base, arrays, decls, control)
        for number in range(count):
            record = base + pointer + SLOT_STRIDE * number
            graph = data[record + 9]
            if graph < len(graphs) and "Genesis" in graphs[graph][0]:
                owners.append(module)
    return owners


def block_record(data: bytes, base: int, arrays: dict, span: int, control: int):
    """Record pointer and kind byte of an indirect parameter block, else None."""
    block = qix.u32(data, control_at(arrays, base, control) + 0x18)
    if block == 0 or block + 8 > span:
        return None
    return qix.u32(data, base + block), data[base + block + 4]


def child_unit(data: bytes, base: int, arrays: dict, pointer: int) -> int | None:
    """Unit index that a child pointer names, or None when it is not a unit entry."""
    units = arrays["units"]
    offset = pointer - units["pointer"]
    if offset < 0 or offset % UNIT_STRIDE:
        return None
    index = offset // UNIT_STRIDE
    return index if index < units["count"] else None


def listener_groups(data: bytes, base: int, arrays: dict, control: int):
    """Per group lists of child control node indices that a control sends to."""
    at = control_at(arrays, base, control)
    pointer, count = qix.u32(data, at + 8), qix.u32(data, at + 12)
    groups = []
    for group in range(count):
        list_at, size = struct.unpack_from(
            "<II", data, base + pointer + GROUP_STRIDE * group
        )
        groups.append([qix.u16(data, base + list_at + 2 * k) for k in range(size)])
    return groups
