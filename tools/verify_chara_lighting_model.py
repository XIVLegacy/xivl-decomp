#!/usr/bin/env python3
"""Validate the character lighting model contract, optional exe bytes and install shaders.

The reference functions are a closed-form restatement of the decoded cloth
pixel program and of the present blit. The golden vectors in
test_chara_lighting_model.py come from an untracked executor of the decoded token
stream and are a lead.
"""

from __future__ import annotations

import argparse
import copy
import json
import math
import re
import struct
import sys
from pathlib import Path

import d3d9_shader_decode as shader_decode
import verify_sqwt_command_parameter_consumer as image_checks

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "chara_lighting_model.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
IMAGE_BASE = 0x00400000
RENDER_STATE_MANAGER = 0x004236E0
SAMPLER_STATE_MANAGER = 0x004236A0
SRGB_WRITE_STATE = 194
SRGB_TEXTURE_SAMPLER = 11
INSTALL_TREE = Path("client/chara/pc/c001")

SPEC_GAIN = 1.2
SPEC_OFFSET = 2.0
SHININESS_BIAS = 0.26
NORMAL_Z_BASE = 0.25
SPEC_FALLOFF_POWER = 8
POINT_FLOOR = 0.001
POINT_FAR_SCALE = 1000.0
DITHER_ROWS = (
    (25, 17, 29, 21),
    (9, 1, 13, 5),
    (31, 23, 27, 19),
    (15, 7, 11, 3),
)
DITHER_DENOMINATOR = 32

SECTIONS = (
    "constants",
    "lightDrivers",
    "gammaBlit",
    "stateIds",
    "shaders",
    "pramCensus",
    "shaderCensus",
)
EXPECTED = {
    "constants": {
        "specularCurve": {"gain": SPEC_GAIN, "offset": SPEC_OFFSET},
        "shininessDivisor": {"bias": SHININESS_BIAS},
        "normalMap": {"center": 0.5, "zSquaredBase": NORMAL_Z_BASE},
        "specularFalloff": {"power": SPEC_FALLOFF_POWER},
        "pointLight": {
            "attenuationFloor": POINT_FLOOR,
            "farScale": POINT_FAR_SCALE,
            "count": {"000": 4, "001": 2},
        },
        "directionalLightCount": 2,
        "shadowTapCount": 4,
        "luminanceLengthKind": "euclidean",
    },
    "lightDrivers": {
        "ambientFinalize": "(block16 + running / N) * 0.5 when N > 0, else block16",
        "directionalScale": 1.0,
        "pointScale": 1.0,
        "modulateColor": "node colour rgb (w forced 1.0) * mesh colour rgba",
        "enableShadowFlagEffect": "none: shader floor max(1, flag) is 1 for flag <= 1",
    },
    "gammaBlit": {
        "presentCallVa": ["0x00a52f5d", "0x00a53136"],
        "blitVa": "0x0041f020",
        "mode": 3,
        "gammaFieldOffset": "0x17c",
        "gammaDefault": 1.0,
        "gammaSetterVa": "0x0041bae0",
        "gammaGetterVa": "0x0041bb10",
        "formula": "out = (floor(255 * |c|^gamma) + (frac >= T(x, y))) / 255",
        "ditherRows": [list(row) for row in DITHER_ROWS],
        "ditherDenominator": DITHER_DENOMINATOR,
        "ditherIndex": "T = rows[y & 3][x & 3] / 32 with x, y the pixel position",
    },
    "stateIds": {
        "srgbWriteState": SRGB_WRITE_STATE,
        "srgbTextureSampler": SRGB_TEXTURE_SAMPLER,
        "renderStateManagerVa": "0x004236e0",
        "samplerStateManagerVa": "0x004236a0",
        "renderStateImmediates": [
            7,
            8,
            14,
            15,
            19,
            20,
            22,
            23,
            24,
            25,
            27,
            52,
            57,
            58,
            59,
            154,
            156,
            168,
            171,
            174,
            175,
            181,
            185,
            190,
            191,
            192,
            193,
            195,
            206,
            207,
            208,
            209,
        ],
        "samplerStateImmediates": [1, 2, 3, 5, 6, 7, 8, 9, 10],
        "registerFedCallSites": {"renderState": 12, "samplerState": 5},
    },
}
INSTALL_PINS = [
    ("equ/e216/pch_mdl/0001", 35280, 464, "vs_3_0", 136, "511577889b120f87"),
    ("equ/e216/pch_mdl/0001", 35280, 3520, "ps_3_0", 216, "587dd3b49e7ea1bc"),
    ("equ/e216/pch_mdl/0001", 35280, 9312, "ps_3_0", 163, "9b2d4c31e37c46fc"),
    ("equ/e001/top_mdl/0001", 71042, 3536, "ps_3_0", 218, "6fcea3b529d7c066"),
    ("equ/e001/top_mdl/0001", 71042, 9360, "ps_3_0", 165, "ff5c6a16fdf86fc7"),
    ("equ/e001/top_mdl/0001", 71042, 19276, "ps_3_0", 314, "551cf91208c9688a"),
    ("equ/e001/top_mdl/0001", 71042, 26716, "ps_3_0", 210, "6ed757ea039d8580"),
    ("kao/f001/fac_mdl/0001", 150256, 512, "vs_3_0", 139, "a223e42399fb3190"),
    ("kao/f001/fac_mdl/0001", 150256, 3664, "ps_3_0", 230, "cb4f097f39cb89db"),
    ("kao/f001/fac_mdl/0001", 150256, 9712, "ps_3_0", 177, "49995906d05e0881"),
    ("kao/f001/fac_mdl/0001", 150256, 19820, "ps_3_0", 202, "1b12a628ca9b9346"),
    ("kao/f001/fac_mdl/0001", 150256, 25292, "ps_3_0", 149, "0150765eb75ca15a"),
    ("kao/f001/fac_mdl/0001", 150256, 34676, "ps_3_0", 302, "e496d448dd9b11f0"),
    ("kao/f001/fac_mdl/0001", 150256, 41780, "ps_3_0", 197, "1e07191fee1e10f0"),
]
EXE_PINS = [
    ("0x00f58528", 0x140, "vs_3_0", 13, "fe940ffba7f0ccf2"),
    ("0x00f58668", 0x390, "ps_3_0", 42, "1a347d4fad42d2e9"),
]
SHADER_ROLES = {
    "clothVertex",
    "clothPixel000",
    "clothPixel001",
    "clothCubeAlphaPixel000",
    "clothCubeAlphaPixel001",
    "skinPixel000",
    "skinPixel001",
    "faceHairVertex",
    "hairPixel000",
    "hairPixel001",
    "irisPixel000",
    "irisPixel001",
    "faceSkinPixel000",
    "faceSkinPixel001",
    "gammaBlitVertex",
    "gammaBlitPixel",
}
REQUIRED_CHECKS = {
    "present blit mode 3 first call",
    "present blit mode 3 second call",
    "blit reads gamma and copies it to four lanes",
    "blit submits the gamma constant",
    "gamma setter",
    "gamma getter",
    "device create resets gamma to one",
    "device create resets gamma store",
    "gamma constant name",
    "gamma sampler constant name",
    "constant 1.0 float",
    "constant 0.5 float",
    "directional light scale 1.0",
    "point light scale 1.0",
    "ambient block seed copy",
    "ambient divide by light count",
    "ambient scale by one half",
    "modulateColor reads node colour",
    "modulateColor reads mesh colour",
    "glareLdrScale reads the context scale",
    "context stores node colour",
    "context stores mesh colour",
    "pixel permutation flag setter",
    "pixel permutation flag getter",
}


def sat(value: float) -> float:
    return min(1.0, max(0.0, value))


def dot(a: list[float], b: list[float]) -> float:
    return sum(x * y for x, y in zip(a, b))


def normalize(a: list[float]) -> list[float]:
    length = math.sqrt(dot(a, a))
    return [x / length for x in a] if length > 0 else [0.0] * len(a)


def cross(a: list[float], b: list[float]) -> list[float]:
    return [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]


def lerp(a: list[float], b: list[float], t: float) -> list[float]:
    return [x + (y - x) * t for x, y in zip(a, b)]


def cloth_lit_color(
    params: dict,
    inputs: dict,
    textures: dict,
    points: int = 4,
    cube_alpha_lerp: bool = False,
) -> list[float]:
    """Lit rgb of the cloth pixel program before shadow, fog and the tone tail.

    `inputs` carries v0 (lit vertex term and alpha), v1 (world position), v3
    (world normal), v4 (packed tangent), v6 (view vector) and v7 (vertex colour).
    `textures` carries d, s, n texels and the two cube samples.
    """
    d, s, n = textures["d"], textures["s"], textures["n"]
    mask = n[2]
    albedo = lerp(
        [d[k] * params["diffuseColor"][k] for k in range(3)],
        [d[k] * params["multiDiffuseColor"][k] for k in range(3)],
        mask,
    )
    curve = [SPEC_GAIN * s[k] * (SPEC_OFFSET - s[k]) ** 2 for k in range(3)]
    spec_color = lerp(
        [curve[k] * params["specularColor"][k] for k in range(3)],
        [curve[k] * params["multiSpecularColor"][k] for k in range(3)],
        mask,
    )
    green = s[1]
    inverse = 1.0 / (green * green - green + SHININESS_BIAS)
    exponent = (inverse * params["shininess"]) * (1 - mask) + (
        inverse * params["multiShininess"]
    ) * mask
    reflectivity = lerp(
        [s[k] * params["reflectivity"][k] for k in range(3)],
        [s[k] * params["multiReflectivity"][k] for k in range(3)],
        mask,
    )
    view = normalize(inputs["v6"][:3])
    normal0 = normalize(inputs["v3"])
    packed = [x * 2 - 1 for x in inputs["v4"]]
    tangent = normalize(packed[:3])
    binormal = [x * packed[3] for x in cross(tangent, normal0)]
    nx, ny = n[0] - 0.5, n[1] - 0.5
    nz = math.sqrt(NORMAL_Z_BASE - nx * nx - ny * ny)
    q = normalize([nx * params["normalPower"], ny * params["normalPower"], nz])
    normal = [
        q[0] * binormal[k] + q[1] * tangent[k] + q[2] * normal0[k] for k in range(3)
    ]
    reflect = [view[k] - 2 * dot(view, normal) * normal[k] for k in range(3)]
    fresnel = (1 - sat(abs(dot(view, normal)))) ** params["fresnelExp"]
    bias = params["fresnelLightDiffBias"]
    fresnel_spec = sat(green + (1 - green) * fresnel)
    fresnel_diff = sat(bias + (1 - bias) * fresnel)
    ambient = [
        params["ambientColor"][k] + params["ambientLightColor"][k] for k in range(3)
    ]
    diffuse = [0.0] * 3
    specular = [0.0] * 3

    def accumulate(
        color: list[float], attenuation: float, light_dir: list[float]
    ) -> None:
        toward = [-x for x in light_dir]
        n_dot_l = sat(dot(normal, toward))
        r_dot_l = sat(dot(reflect, toward))
        lobe = (r_dot_l**exponent if r_dot_l > 0 else 0.0) * (
            1 - (1 - n_dot_l) ** SPEC_FALLOFF_POWER
        )
        for k in range(3):
            diffuse[k] += color[k] * attenuation * n_dot_l
            specular[k] += color[k] * attenuation * lobe

    for i in range(2):
        accumulate(params["DirLightColors"][i], 1.0, params["DirLightDirections"][i])
    for i in range(points):
        position = params["PointLightPositions"][i]
        decay, radius = params["PointLightParams"][i][2:4]
        delta = [inputs["v1"][k] - position[k] for k in range(3)]
        distance = math.sqrt(dot(delta, delta))
        denominator = radius * distance**decay
        falloff = (
            1.0 / denominator if denominator - POINT_FLOOR >= 0 else POINT_FAR_SCALE
        )
        attenuation = max(radius - distance, POINT_FLOOR) * falloff
        accumulate(
            params["PointLightColors"][i], attenuation, [x / distance for x in delta]
        )
    lit = [
        (ambient[k] * albedo[k] + albedo[k] * diffuse[k]) * inputs["v0"][k]
        for k in range(3)
    ]
    lat = params["latitudeParam"]
    length = math.sqrt(
        sum(((lit[k] * lat[0] + lat[1]) * lat[2]) ** 2 for k in range(3))
    )
    cube_d, cube_r = textures["cubeD"], textures["cubeR"]
    scale_d = fresnel_diff * params["lightDiffusePower"]
    keep_d = 1 - sat(length * params["lightDiffuseInfluence"])
    out = [
        lit[k] + albedo[k] * inputs["v7"][k] * cube_d[k] * scale_d * keep_d
        for k in range(3)
    ]
    restrain = params["refAlphaRestrain"]
    if cube_alpha_lerp:
        restrain = cube_r[3] + restrain * (1 - cube_r[3])
    keep_r = 1 - sat(length * params["reflectMapInfluence"])
    out = [
        out[k] + cube_r[k] * reflectivity[k] * fresnel_spec * restrain * keep_r
        for k in range(3)
    ]
    keep_s = 1 - sat(length * params["specularInfluence"])
    return [
        out[k] + spec_color[k] * fresnel_spec * specular[k] * keep_s for k in range(3)
    ]


def shadow_factor(params: dict, inputs: dict, depth_at) -> list[float]:
    """Per-channel shadow multiplier of the `_000` cloth pass."""
    matrix = [params[f"shadowProjMatrix{i}"] for i in range(4)]
    x, y, z = inputs["v1"]
    clip = [
        x * matrix[0][k] + y * matrix[1][k] + z * matrix[2][k] + matrix[3][k]
        for k in range(4)
    ]
    px, py, pz = clip[0] / clip[3], clip[1] / clip[3], clip[2]
    lit = 0
    for offset in params["shadowOffset0"]:
        lit += (
            1 if depth_at(px + offset[0], py + offset[1]) - (pz + offset[2]) >= 0 else 0
        )
    fraction = lit / 4.0
    fade_param = params["shadowFadeParam"]
    fade = sat((inputs["v2z"] - fade_param[0]) / fade_param[1])
    color = params["shadowColor"]
    amount = color[3] * fade_param[2] * (1 - fraction) * (1 - fade)
    return [1 + amount * (color[k] - 1) for k in range(3)]


def tone_tail(
    params: dict, inputs: dict, textures: dict, rgb: list[float], shadow: list[float]
) -> list[float]:
    """Shadow, fog, modulate, latitude and glare scale; returns rgba."""
    fog = min(params["fogColor"][3], inputs["v6"][3])
    modulate = params["modulateColor"]
    lat = params["latitudeParam"]
    out = []
    for k in range(3):
        color = rgb[k] * shadow[k]
        color = color * (1 - fog) + params["fogColor"][k] * fog
        color = (color * modulate[k] * lat[0] + lat[1]) * lat[2]
        out.append(color * params["glareLdrScale"][0])
    alpha = textures["s"][3] * inputs["v0"][3] * modulate[3]
    return out + [alpha]


def dither_threshold(x: int, y: int) -> float:
    return DITHER_ROWS[y & 3][x & 3] / DITHER_DENOMINATOR


def present_channel(value: float, gamma: float, x: int, y: int) -> int:
    """8-bit result of the gamma and ordered-dither present blit for one channel."""
    magnitude = abs(value)
    scaled = (magnitude**gamma if magnitude > 0 else 0.0) * 255.0
    whole = math.floor(scaled)
    bump = 1 if scaled - whole - dither_threshold(x, y) >= 0 else 0
    return whole + bump


def parse_pram(data: bytes, start: int) -> dict[str, tuple[float, ...]]:
    """Parameter rows of the PRAM block whose tag is at `start`."""
    body = start + 16
    count, table, names = struct.unpack_from("<3I", data, body + 8)
    params = {}
    for index in range(count):
        (record,) = struct.unpack_from("<I", data, body + table + 4 * index)
        (name_at,) = struct.unpack_from("<I", data, body + names + 4 * index)
        end = data.index(b"\0", body + name_at)
        name = data[body + name_at : end].decode("ascii")
        params[name] = struct.unpack_from("<4f", data, body + record + 0x10)
    return params


def pram_census(root: Path) -> dict:
    """Block counts and value sets of PRAM rows over the c001 equ, kao and acc models."""
    blocks = 0
    seen: dict[str, dict[tuple[float, ...], int]] = {}
    for model in sorted((root / INSTALL_TREE).rglob("0001")):
        if not model.parent.name.endswith("_mdl"):
            continue
        data = model.read_bytes()
        for match in re.finditer(b"PRAM", data):
            try:
                params = parse_pram(data, match.start())
            except (struct.error, ValueError):
                continue
            blocks += 1
            for name, value in params.items():
                rounded = tuple(round(x, 3) for x in value)
                counts = seen.setdefault(name, {})
                counts[rounded] = counts.get(rounded, 0) + 1
    return {"blocks": blocks, "values": seen}


def shader_census(root: Path) -> dict:
    """Distinct programs and `def` float constants over the c001 equ, kao and acc models."""
    programs: dict[str, shader_decode.Shader] = {}
    for model in sorted((root / INSTALL_TREE).rglob("0001")):
        if not model.parent.name.endswith("_mdl"):
            continue
        data = model.read_bytes()
        for match in re.finditer(rb"\x00\x03[\xfe\xff]\xff", data):
            try:
                decoded = shader_decode.decode(data, match.start())
            except (ValueError, struct.error, IndexError):
                continue
            if len(decoded.lines) > 1:
                programs.setdefault(decoded.sha256(), decoded)
    floats: set[float] = set()
    for decoded in programs.values():
        for line in decoded.lines:
            if line.startswith("def "):
                for word in line.split(",")[1:]:
                    floats.add(float(word) + 0.0)
    return {
        "vertex": sum(p.version == "vs_3_0" for p in programs.values()),
        "pixel": sum(p.version == "ps_3_0" for p in programs.values()),
        "defFloats": sorted(floats),
    }


def scan_state_ids(data: bytes) -> dict:
    """Immediate state ids pushed to the two device state managers."""
    sections = image_checks.section_map(data)
    start, _, raw_offset, raw_size = sections[0]
    code = data[raw_offset : raw_offset + raw_size]
    render: set[int] = set()
    sampler: set[int] = set()
    register_fed = {"renderState": 0, "samplerState": 0}
    index = 0
    while True:
        index = code.find(b"\xe8", index)
        if index < 0 or index + 5 > len(code):
            break
        (rel,) = struct.unpack_from("<i", code, index + 1)
        target = (start + index + 5 + rel) & 0xFFFFFFFF
        before = code[max(0, index - 8) : index]
        if target == RENDER_STATE_MANAGER:
            if len(before) >= 2 and before[-2] == 0x6A:
                render.add(before[-1])
            elif len(before) >= 5 and before[-5] == 0x68:
                render.add(struct.unpack_from("<I", before, len(before) - 4)[0])
            else:
                register_fed["renderState"] += 1
        elif target == SAMPLER_STATE_MANAGER:
            found = re.search(rb"\x6a(.)(?:[\x50-\x57]|\x6a.)$", before, re.S)
            if found:
                sampler.add(found.group(1)[0])
            else:
                register_fed["samplerState"] += 1
        index += 1
    return {
        "render": sorted(render),
        "sampler": sorted(sampler),
        "registerFed": register_fed,
    }


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-chara-lighting-model-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != EXPECTED_SHA256
        or source.get("size") != EXPECTED_SIZE
        or source.get("imageBase") != "0x00400000"
        or source.get("build") != "2012.09.19.0001"
    ):
        errors.append("evidence identity changed")
    for section in ("constants", "lightDrivers", "gammaBlit", "stateIds"):
        if document.get(section) != EXPECTED[section]:
            errors.append(f"{section} contract changed")
    shaders = document.get("shaders", {})
    roles = {
        item.get("role") for item in shaders.get("install", []) + shaders.get("exe", [])
    }
    if roles != SHADER_ROLES:
        errors.append(
            f"shader roles are {sorted(roles ^ SHADER_ROLES)} apart from the contract"
        )
    pins = [
        (
            i["path"],
            i["fileSize"],
            i["offset"],
            i["version"],
            i["lines"],
            i["decodedSha256"][:16],
        )
        for i in shaders.get("install", [])
    ]
    if pins != [(f"{INSTALL_TREE}/{p[0]}", *p[1:]) for p in INSTALL_PINS]:
        errors.append("install shader pins changed")
    exe_pins = [
        (i["va"], i["size"], i["version"], i["lines"], i["decodedSha256"][:16])
        for i in shaders.get("exe", [])
    ]
    if exe_pins != EXE_PINS:
        errors.append("exe shader pins changed")
    census = document.get("pramCensus", {})
    if census.get("blocks") != 871 or census.get("lightRows") != 805:
        errors.append("PRAM census counts changed")
    programs = document.get("shaderCensus", {})
    if (programs.get("vertex"), programs.get("pixel")) != (4, 40):
        errors.append("shader census counts changed")
    if not programs.get("defFloats") or not programs.get("gammaCandidates"):
        errors.append("shader census lists are empty")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    if REQUIRED_CHECKS - set(names):
        errors.append(f"staticChecks lacks {sorted(REQUIRED_CHECKS - set(names))}")
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not re.fullmatch(r"0x[0-9a-f]{8}", str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            try:
                same = len(bytes.fromhex(item["pattern"])) == len(
                    bytes.fromhex(item["mask"])
                )
            except (KeyError, ValueError):
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
    if not document.get("unresolved"):
        errors.append("unresolved list is empty")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]:\\\\", "/" + "Users/", "/" + "home/", "agent-islands")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("constants", "specularCurve", "gain"), 1.0),
        (("constants", "shininessDivisor", "bias"), 0.25),
        (("constants", "normalMap", "zSquaredBase"), 1.0),
        (("constants", "specularFalloff", "power"), 4),
        (("constants", "pointLight", "attenuationFloor"), 0.01),
        (("constants", "pointLight", "count", "001"), 4),
        (("constants", "directionalLightCount"), 1),
        (("lightDrivers", "directionalScale"), 0.5),
        (("lightDrivers", "ambientFinalize"), "block16 + running"),
        (("gammaBlit", "mode"), 2),
        (("gammaBlit", "gammaDefault"), 2.2),
        (("gammaBlit", "gammaFieldOffset"), "0x178"),
        (("gammaBlit", "ditherRows", 0, 0), 24),
        (("gammaBlit", "presentCallVa", 1), "0x00a53137"),
        (("stateIds", "srgbWriteState"), 193),
        (("stateIds", "renderStateImmediates", 0), 194),
        (("stateIds", "samplerStateImmediates", 0), 11),
        (("shaders", "install", 1, "decodedSha256"), "0" * 64),
        (("shaders", "install", 1, "offset"), 3521),
        (("shaders", "exe", 1, "lines"), 41),
        (("shaders", "exe", 0, "role"), "gammaBlitPixel"),
        (("pramCensus", "blocks"), 870),
        (("shaderCensus", "pixel"), 39),
        (("source", "binarySha256"), "0" * 64),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0xa53124"),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    dropped = copy.deepcopy(document)
    dropped["staticChecks"] = [
        item for item in dropped["staticChecks"] if item["name"] != "gamma setter"
    ]
    mutations.append(dropped)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def decoded_row(decoded: shader_decode.Shader) -> tuple:
    return (
        decoded.version,
        len(decoded.lines) - 1,
        decoded.sha256(),
        decoded.table_sha256(),
    )


def pinned_row(item: dict) -> tuple:
    return (
        item["version"],
        item["lines"],
        item["decodedSha256"],
        item["constantTableSha256"],
    )


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    data = path.read_bytes()
    errors = image_checks.verify_image(data, document)
    sections = image_checks.section_map(data)
    for item in document["shaders"]["exe"]:
        va = int(item["va"], 16)
        blob = image_checks.read_va(data, sections, va, item["size"])
        if blob is None:
            errors.append(f"{item['role']} is outside the image")
            continue
        try:
            decoded = shader_decode.decode(blob, 0)
        except (ValueError, struct.error, IndexError) as exc:
            errors.append(f"{item['role']} does not decode: {exc}")
            continue
        if decoded_row(decoded) != pinned_row(item):
            errors.append(f"{item['role']} decodes differently")
    ids = scan_state_ids(data)
    state = document["stateIds"]
    if ids["render"] != state["renderStateImmediates"]:
        errors.append(f"render state ids are {ids['render']}")
    if ids["sampler"] != state["samplerStateImmediates"]:
        errors.append(f"sampler state ids are {ids['sampler']}")
    if ids["registerFed"] != state["registerFedCallSites"]:
        errors.append(f"register-fed call sites are {ids['registerFed']}")
    if SRGB_WRITE_STATE in ids["render"] or SRGB_TEXTURE_SAMPLER in ids["sampler"]:
        errors.append("an sRGB state id is passed to a device state manager")
    return errors


def verify_install(root: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors: list[str] = []
    for item in document["shaders"]["install"]:
        path = root / item["path"]
        if not path.is_file():
            errors.append(f"{item['path']} is missing")
            continue
        data = path.read_bytes()
        if len(data) != item["fileSize"]:
            errors.append(f"{item['path']} is {len(data)} bytes")
            continue
        try:
            decoded = shader_decode.decode(data, item["offset"])
        except (ValueError, struct.error, IndexError) as exc:
            errors.append(f"{item['role']} does not decode: {exc}")
            continue
        if decoded_row(decoded) != pinned_row(item):
            errors.append(f"{item['role']} decodes differently")
    census = pram_census(root)
    expected = document["pramCensus"]
    if census["blocks"] != expected["blocks"]:
        errors.append(f"PRAM blocks are {census['blocks']}")
    for name, row in expected["uniform"].items():
        values = census["values"].get(name, {})
        if (
            list(values) != [tuple(row["value"])]
            or sum(values.values()) != row["blocks"]
        ):
            errors.append(f"PRAM row {name} differs: {dict(list(values.items())[:3])}")
    for name, row in expected["ranges"].items():
        values = census["values"].get(name, {})
        low = min((min(v[:3]) if row["lanes"] == 3 else v[0]) for v in values)
        high = max((max(v[:3]) if row["lanes"] == 3 else v[0]) for v in values)
        if (low, high, sum(values.values())) != (row["min"], row["max"], row["blocks"]):
            errors.append(f"PRAM range {name} is {low} to {high}")
    errors += verify_shader_census(root, document["shaderCensus"])
    return errors


def verify_shader_census(root: Path, expected: dict) -> list[str]:
    errors: list[str] = []
    census = shader_census(root)
    for key in ("vertex", "pixel", "defFloats"):
        if census[key] != expected[key]:
            errors.append(f"shader census {key} is {census[key]}")
    tolerance = expected["gammaCandidateTolerance"]
    for value in census["defFloats"]:
        for candidate in expected["gammaCandidates"]:
            if abs(value - candidate) <= tolerance:
                errors.append(f"a def constant {value} is near gamma value {candidate}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install", type=Path, help="retail install root for shader checks"
    )
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
    if errors:
        print(f"FAIL: {len(errors)} character lighting model error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install checks"
    print(f"PASS: character lighting model manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
