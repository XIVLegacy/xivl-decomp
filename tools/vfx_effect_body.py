#!/usr/bin/env python3
"""Parse the body of Qix veff and vmdl blobs.

The layouts are the ones recorded in docs/resource/vfx-effect-data-body.md,
docs/resource/vfx-control-module-registry.md and
docs/resource/vfx-drawresource-and-shaders.md. Every function works on
caller-supplied bytes and offsets taken from vfx_qix_resource.parse_blob. Nothing
here reads a client install on its own.
"""

from __future__ import annotations

import struct

import vfx_qix_resource as qix

VEFF_VERSION = 0x1D
DECL_STRIDE = 24
SHADER_SET_STRIDE = 20
SHADER_HEADER = 0x20
TEXTURE_REF_STRIDE = 24

# Veff root arrays: (name, pointer offset, count offset, element stride).
VEFF_ARRAYS = (
    ("models", 0x20, 0x24, 24),
    ("textures", 0x28, 0x2C, 24),
    ("controls", 0x30, 0x34, 0x24),
    ("links", 0x38, 0x3C, 0x1C),
    ("units", 0x40, 0x44, 0x38),
    ("instances", 0x48, 0x4C, 0x48),
    ("decls", 0x58, 0x5C, DECL_STRIDE),
)
# Leaf and vins roots hold one declaration table at the start of the root.
LEAF_DECL = (0x00, 0x04)
# The veff root embeds a leaf shaped sub-record that starts here.
VEFF_LEAF_SUB = 0x94

D3D_VERTEX_VERSION = 0xFFFE0300
D3D_PIXEL_VERSION = 0xFFFF0300
CTAB_TAG = b"CTAB"


def cstring(data: bytes, offset: int) -> str | None:
    """The NUL-terminated printable ASCII text at offset, else None."""
    end = data.find(b"\0", offset)
    if end < 0 or offset < 0:
        return None
    text = data[offset:end]
    if not text or any(not 0x20 <= byte < 0x7F for byte in text):
        return None
    return text.decode("ascii")


def parse_decls(data: bytes, base: int, pointer: int, count: int) -> list[dict]:
    """Declaration entries: six signature bytes, kind word, name, tail dword."""
    rows = []
    for index in range(count):
        at = base + pointer + DECL_STRIDE * index
        if at + DECL_STRIDE > len(data):
            raise qix.ParseError("declaration table runs past the input")
        name = cstring(data, base + qix.u32(data, at + 0x10))
        rows.append(
            {
                "signature": bytes(data[at : at + 6]),
                "kind": qix.u16(data, at + 6),
                "name": name,
                "tail": qix.u32(data, at + 0x14),
            }
        )
    return rows


def parse_veff(data: bytes, info: dict, root: int) -> dict:
    """Top level directory of a veff root record."""
    base = info["payload"]
    out = {
        "version": qix.u16(data, root),
        "nameA": qix.u16(data, root + 0x1C),
        "nameB": qix.u16(data, root + 0x1E),
        "lifetimeTicks": qix.u32(data, root + 0x50),
        "sourcePath": cstring(data, base + qix.u32(data, root + 0x54)),
        "boundsMin": struct.unpack_from("<3f", data, root + 0x70),
        "boundsMax": struct.unpack_from("<3f", data, root + 0x80),
        "cullingType": data[root + 0x90],
        "leafSubPointer": qix.u32(data, root + VEFF_LEAF_SUB),
        "leafSubCount": qix.u32(data, root + VEFF_LEAF_SUB + 4),
        "leafSubVersion": qix.u16(data, root + VEFF_LEAF_SUB + 0x1C),
        "arrays": {},
    }
    span = info["payloadSpan"]
    for name, pointer_at, count_at, stride in VEFF_ARRAYS:
        pointer, count = (
            qix.u32(data, root + pointer_at),
            qix.u32(data, root + count_at),
        )
        out["arrays"][name] = {
            "pointer": pointer,
            "count": count,
            "stride": stride,
            "inside": pointer + count * stride <= span,
        }
    models = out["arrays"]["models"]
    out["modelNames"] = [
        qix.cstring(data, base + models["pointer"] + 24 * index, 16)
        for index in range(models["count"])
        if base + models["pointer"] + 24 * (index + 1) <= len(data)
    ]
    out["decls"] = parse_decls(
        data, base, out["arrays"]["decls"]["pointer"], out["arrays"]["decls"]["count"]
    )
    return out


def veff_relations(data: bytes, info: dict, root: int, body: dict) -> dict:
    """Index relations between the veff arrays, as booleans."""
    base = info["payload"]
    arrays = body["arrays"]
    control_at = base + arrays["controls"]["pointer"]
    decl_count = arrays["decls"]["count"]
    link_at = base + arrays["links"]["pointer"]
    instance_at = base + arrays["instances"]["pointer"]
    unit_count = arrays["units"]["count"]
    control_count = arrays["controls"]["count"]
    return {
        "controlDeclInRange": all(
            qix.u16(data, control_at + 0x24 * index + 0x1C) < decl_count
            for index in range(control_count)
        ),
        "linkControlInRange": all(
            qix.u16(data, link_at + 0x1C * index + 0x14) < control_count
            for index in range(arrays["links"]["count"])
        ),
        "instanceUnitInRange": all(
            qix.u16(data, instance_at + 0x48 * index) < unit_count
            for index in range(arrays["instances"]["count"])
        ),
    }


def parse_leaf_decls(data: bytes, info: dict, root: int) -> list[dict]:
    """Declaration table of a leaf or vins root."""
    return parse_decls(
        data,
        info["payload"],
        qix.u32(data, root + LEAF_DECL[0]),
        qix.u32(data, root + LEAF_DECL[1]),
    )


def parse_vmdl(data: bytes, info: dict, root: int) -> dict:
    """Shader sets, texture references and chunk pointers of a vmdl root."""
    base = info["payload"]
    span = info["payloadSpan"]
    u32 = lambda offset: qix.u32(data, root + offset)  # noqa: E731
    out = {
        "shaderSetPointer": u32(0x18),
        "shaderSetCount": u32(0x1C),
        "textureRefPointer": u32(0x20),
        "textureRefCount": u32(0x24),
        "type": u32(0x28),
        "modelChunk": (u32(0x2C), u32(0x30)),
        "variant": u32(0x34),
        "animationChunk": (u32(0x38), u32(0x3C)),
        "instanceChunk": (u32(0x40), u32(0x44)),
        "vertexShader": (u32(0x48), u32(0x4C)),
        "pixelShader": (u32(0x50), u32(0x54)),
        "shaderSets": [],
        "textureRefs": [],
    }
    for index in range(out["shaderSetCount"]):
        at = base + out["shaderSetPointer"] + SHADER_SET_STRIDE * index
        if at + SHADER_SET_STRIDE > base + span:
            raise qix.ParseError("shader set runs past the payload")
        vs_at, vs_size, ps_at, ps_size, flags = struct.unpack_from("<5I", data, at)
        out["shaderSets"].append(
            {
                "vertex": (vs_at, vs_size),
                "pixel": (ps_at, ps_size),
                "flags": flags,
            }
        )
    for index in range(out["textureRefCount"]):
        at = base + out["textureRefPointer"] + TEXTURE_REF_STRIDE * index
        if at + TEXTURE_REF_STRIDE > base + span:
            raise qix.ParseError("texture reference runs past the payload")
        out["textureRefs"].append(
            {"name": qix.cstring(data, at, 16), "tag": bytes(data[at + 16 : at + 20])}
        )
    return out


def parse_shader(blob: bytes) -> dict:
    """A shader record: 16 byte key, big endian length, 12 pad bytes, token stream."""
    if len(blob) < SHADER_HEADER + 4:
        raise qix.ParseError("shader record is shorter than its header")
    length = struct.unpack_from(">I", blob, 16)[0]
    token = struct.unpack_from("<I", blob, SHADER_HEADER)[0]
    return {
        "key": bytes(blob[:16]),
        "length": length,
        "lengthMatches": length + SHADER_HEADER == len(blob),
        "padZero": blob[20:SHADER_HEADER] == bytes(12),
        "versionToken": token,
        "constants": ctab_constants(blob[SHADER_HEADER:]),
    }


def ctab_constants(code: bytes) -> list[tuple[str, int, int, int]]:
    """(name, register set, register index, register count) from the CTAB comment."""
    at = code.find(CTAB_TAG)
    if at < 0:
        return []
    start = at + 4
    size, creator, version, count, info_at, flags, target = struct.unpack_from(
        "<7I", code, start
    )
    rows = []
    for index in range(count):
        name_at, register_set, register_index, register_count, _, _, _ = (
            struct.unpack_from("<IHHHHII", code, start + info_at + 20 * index)
        )
        end = code.find(b"\0", start + name_at)
        if end < 0:
            raise qix.ParseError("constant name runs past the shader")
        rows.append(
            (
                code[start + name_at : end].decode("ascii"),
                register_set,
                register_index,
                register_count,
            )
        )
    return rows
