#!/usr/bin/env python3
"""Tests for the character lighting model verifier, shader decoder and reference model."""

from __future__ import annotations

import copy
import json
import math
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import d3d9_shader_decode as decoder  # noqa: E402
import test_promoted_static_checks as promoted  # noqa: E402
import verify_chara_lighting_model as model  # noqa: E402

PASSED: list[str] = []
FAILED: list[str] = []


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def near(a: float, b: float, tolerance: float = 2e-5) -> bool:
    return abs(a - b) <= tolerance * max(1.0, abs(a), abs(b))


def base_params() -> dict:
    return {
        "diffuseColor": [1.0, 1.0, 1.0, 1.0],
        "multiDiffuseColor": [0.9, 0.5, 0.3, 1.0],
        "specularColor": [1.0, 1.0, 1.0, 0.0],
        "multiSpecularColor": [0.8, 0.8, 0.8, 0.0],
        "shininess": 0.3,
        "multiShininess": 0.5,
        "reflectivity": [1.0, 1.0, 1.0, 0.0],
        "multiReflectivity": [0.7, 0.7, 0.7, 0.0],
        "normalPower": 1.0,
        "fresnelExp": 2.0,
        "fresnelLightDiffBias": 0.6,
        "ambientColor": [0.0, 0.0, 0.0, 0.0],
        "ambientLightColor": [0.3, 0.3, 0.35, 0.0],
        "lightDiffusePower": 2.5,
        "lightDiffuseInfluence": 0.58,
        "reflectMapInfluence": 0.58,
        "specularInfluence": 0.58,
        "refAlphaRestrain": 1.0,
        "lightDiffuseMapLod": 6.3,
        "reflectMapLod": 4.0,
        "latitudeParam": [1.0, 0.0, 1.0, 1.0],
        "DirLightDirections": [[0.0, -0.8, -0.6, 0.0], [0.6, -0.6, 0.529150262, 0.0]],
        "DirLightColors": [[1.0, 0.95, 0.9, 0.0], [0.2, 0.25, 0.35, 0.0]],
        "PointLightPositions": [
            [1.0, 1.5, 0.5, 1.0],
            [-2.0, 1.0, 0.0, 1.0],
            [0.0, 0.0, 0.0, 1.0],
            [0.0, 0.0, 0.0, 1.0],
        ],
        "PointLightColors": [
            [0.9, 0.6, 0.3, 0.0],
            [0.1, 0.2, 0.4, 0.0],
            [0.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0, 0.0],
        ],
        "PointLightParams": [
            [-1.0, -1.0, 1.5, 6.0],
            [-1.0, -1.0, 2.0, 4.0],
            [-1.0, -1.0, 1.0, 1.0],
            [-1.0, -1.0, 1.0, 1.0],
        ],
        "EnableShadowFlag": [1.0, 1.0, 1.0, 1.0],
        "shadowColor": [0.35, 0.4, 0.5, 0.0],
        "shadowFadeParam": [0.0, 1.0, 1.0, 0.0],
        "shadowOffset0": [[0.0, 0.0, 0.0, 0.0]] * 4,
        "shadowProjMatrix0": [1.0, 0.0, 0.0, 0.0],
        "shadowProjMatrix1": [0.0, 1.0, 0.0, 0.0],
        "shadowProjMatrix2": [0.0, 0.0, 1.0, 0.0],
        "shadowProjMatrix3": [0.0, 0.0, 0.0, 1.0],
        "fogColor": [1.0, 1.0, 1.0, 0.0],
        "modulateColor": [1.0, 1.0, 1.0, 1.0],
        "glareLdrScale": [1.0, 0.0, 0.0, 0.0],
    }


def base_inputs() -> dict:
    return {
        "v0": [1.0, 1.0, 1.0, 1.0],
        "v1": [0.2, 1.0, 0.4],
        "v2z": 0.3,
        "v3": [0.0, 1.0, 0.0],
        "v4": [1.0, 0.5, 0.5, 1.0],
        "v6": [0.0, -0.6, -0.8, 0.0],
        "v7": [1.0, 1.0, 1.0],
    }


def base_textures() -> dict:
    return {
        "d": [0.8, 0.6, 0.4, 1.0],
        "s": [0.5, 0.5, 0.5, 0.9],
        "n": [0.5, 0.5, 0.0, 1.0],
        "cubeD": [0.5, 0.55, 0.6, 1.0],
        "cubeR": [0.4, 0.5, 0.6, 0.5],
    }


def scenarios() -> dict:
    """Named scenarios: parameters, inputs, textures, program choice and shadow map."""
    shadowed = base_params()
    shadowed["shadowColor"] = [0.35, 0.4, 0.5, 1.0]
    shadowed["shadowFadeParam"] = [0.1, 0.5, 1.0, 0.0]
    shadowed["fogColor"] = [0.6, 0.65, 0.8, 0.8]
    shadowed["modulateColor"] = [0.9, 0.95, 1.0, 0.75]
    shadowed["latitudeParam"] = [1.25, -0.05, 0.75, 1.0]
    shadowed["glareLdrScale"] = [0.9, 0.0, 0.0, 0.0]
    inputs = base_inputs()
    inputs["v6"] = [0.0, -0.6, -0.8, 0.4]
    inputs["v0"] = [1.2, 1.1, 1.0, 0.8]
    tinted = base_textures()
    tinted["n"] = [0.55, 0.45, 0.7, 1.0]
    tinted["s"] = [0.9, 0.3, 0.6, 0.9]
    return {
        "daylight": {
            "params": base_params(),
            "inputs": base_inputs(),
            "textures": base_textures(),
            "pass": "000",
            "alphaLerp": False,
        },
        "shadow fog tint": {
            "params": shadowed,
            "inputs": inputs,
            "textures": tinted,
            "pass": "000",
            "alphaLerp": False,
        },
        "cube alpha lerp": {
            "params": base_params() | {"refAlphaRestrain": 0.4},
            "inputs": base_inputs(),
            "textures": tinted,
            "pass": "000",
            "alphaLerp": True,
        },
        "lighter pass": {
            "params": base_params()
            | {
                "PointLightParams": [
                    [-1.0, -1.0, 1.5, 6.0],
                    [-1.0, -1.0, 2.0, 2.0],
                    [-1.0, -1.0, 1.0, 1.0],
                    [-1.0, -1.0, 1.0, 1.0],
                ]
            },
            "inputs": inputs,
            "textures": tinted,
            "pass": "001",
            "alphaLerp": False,
        },
    }


def shadow_depth(x: float, y: float) -> float:
    return 0.5 + 0.5 * math.sin(7 * x + 3 * y)


def evaluate(case: dict) -> list[float]:
    points = 2 if case["pass"] == "001" else 4
    rgb = model.cloth_lit_color(
        case["params"], case["inputs"], case["textures"], points, case["alphaLerp"]
    )
    shadow = [1.0, 1.0, 1.0]
    if case["pass"] == "000":
        shadow = model.shadow_factor(case["params"], case["inputs"], shadow_depth)
    return model.tone_tail(
        case["params"], case["inputs"], case["textures"], rgb, shadow
    )


# Outputs of executing the decoded programs on the scenarios above, rgba.
GOLDEN = {
    "daylight": [1.33407, 0.909821, 0.578672, 0.9],
    "shadow fog tint": [0.585274, 0.392238, 0.359537, 0.54],
    "cube alpha lerp": [1.35701, 0.665895, 0.358892, 0.9],
    "lighter pass": [1.504923, 0.668822, 0.315548, 0.72],
}


def register_bits(kind: int, number: int) -> int:
    return 0x80000000 | (kind & 7) << 28 | (kind & 0x18) << 8 | number


def dest(kind: int, number: int, mask: int = 0xF) -> int:
    return register_bits(kind, number) | mask << 16


def src(kind: int, number: int, swizzle: int = 0xE4) -> int:
    return register_bits(kind, number) | swizzle << 16


def synthetic_shader() -> bytes:
    """ps_3_0 with a def, a dcl, one mul and a CTAB naming one constant."""
    words: list[int] = [0xFFFF0300]

    def instruction(opcode: int, *operands: int) -> None:
        words.append(opcode | len(operands) << 24)
        words.extend(operands)

    name = b"ambientColor\0"
    table = bytearray(struct.pack("<7I", 28, 0, 0x300, 1, 28, 0, 0))
    table += struct.pack("<IHHHHII", 28 + 20, 2, 7, 1, 0, 0, 0)
    table += name
    table += b"\0" * (-len(table) % 4)
    body = b"CTAB" + bytes(table)
    words.append(0xFFFE | (len(body) // 4) << 16)
    words.extend(struct.unpack(f"<{len(body) // 4}I", body))
    one = struct.unpack("<I", struct.pack("<f", 1.5))[0]
    instruction(81, dest(2, 3), one, one, 0, 0)
    instruction(31, 5, dest(3, 0))
    instruction(5, dest(0, 1, 0x7), src(2, 3, 0x00), src(0, 0))
    instruction(66, dest(0, 2), src(3, 0), src(10, 1))
    words.append(0xFFFF)
    return struct.pack(f"<{len(words)}I", *words)


def decoder_checks() -> None:
    blob = synthetic_shader()
    shader = decoder.decode(b"\x00\x00" + blob, 2)
    check("decoder reads the version", shader.version == "ps_3_0")
    check(
        "decoder prints def, dcl, mul and texld",
        list(shader.lines[1:])
        == [
            "def c3, 1.5, 1.5, 0, 0",
            "dcl_texcoord0 t0",
            "mul r1.xyz, c3.xxxx, r0",
            "texld r2, t0, s1",
        ],
    )
    check(
        "decoder reads the constant table",
        shader.constant("ambientColor").index == 7
        and shader.constant("ambientColor").register_set == "c",
    )
    check("decoder reports the stream length", shader.length == len(blob))
    check(
        "the text hash covers the instructions",
        shader.sha256()
        != decoder.decode(
            blob.replace(b"\x00\x00\xc0\x3f", b"\x00\x00\x00\x40"), 0
        ).sha256(),
    )
    check(
        "a bad version token is rejected",
        raises(ValueError, lambda: decoder.decode(b"\x01\x02\x03\x04", 0)),
    )


def raises(kind: type[Exception], call) -> bool:
    try:
        call()
    except kind:
        return True
    return False


def reference_checks() -> None:
    for name, case in scenarios().items():
        got = evaluate(case)
        want = GOLDEN[name]
        check(
            f"reference model matches the executed program: {name}",
            all(near(got[k], want[k]) for k in range(4)),
        )
    daylight = scenarios()["daylight"]
    base = evaluate(daylight)
    dark = copy.deepcopy(daylight)
    dark["params"]["DirLightColors"] = [[0.0] * 4, [0.0] * 4]
    dark["params"]["ambientLightColor"] = [0.0] * 4
    dark["params"]["PointLightColors"] = [[0.0] * 4] * 4
    check(
        "unlit cloth keeps only the cube and reflection terms",
        evaluate(dark)[0] < base[0],
    )
    plain = copy.deepcopy(daylight)
    plain["params"]["lightDiffusePower"] = 0.0
    plain["params"]["refAlphaRestrain"] = 0.0
    plain["params"]["specularColor"] = [0.0] * 4
    plain["params"]["multiSpecularColor"] = [0.0] * 4
    flat = evaluate(plain)
    check(
        "without cube and specular terms the output equals ambient plus diffuse",
        all(0.0 < flat[k] < base[k] for k in range(3)),
    )
    check(
        "the point light count of the lighter pass is two",
        model.EXPECTED["constants"]["pointLight"]["count"]["001"] == 2,
    )

    check(
        "the specular curve peaks near two thirds",
        max(range(101), key=lambda i: i * (2 - i / 100) ** 2) in range(60, 70),
    )
    check(
        "the shininess divisor ranges 3.85 to 100",
        near(1 / (0.0 - 0.0 + model.SHININESS_BIAS), 3.846154, 1e-5)
        and near(1 / (0.25 - 0.5 + model.SHININESS_BIAS), 100.0, 1e-5),
    )
    check(
        "rim falloff term is 1 - (1 - d)^8",
        near(1 - (1 - 0.5) ** model.SPEC_FALLOFF_POWER, 0.99609375, 1e-9),
    )


def present_checks() -> None:
    check(
        "dither rows are the odd numerators 1 to 31 once each",
        sorted(v for row in model.DITHER_ROWS for v in row) == list(range(1, 32, 2)),
    )
    check("dither threshold corner values", model.dither_threshold(0, 0) == 25 / 32)
    check("dither index wraps at four", model.dither_threshold(5, 6) == 23 / 32)
    check(
        "gamma one keeps whole levels",
        model.present_channel(100.25 / 255, 1.0, 0, 0) == 100,
    )
    check("alpha one is 255", model.present_channel(1.0, 1.0, 3, 3) == 255)
    check("zero stays zero for any gamma", model.present_channel(0.0, 2.2, 1, 1) == 0)
    low = [model.present_channel(0.5, 1.0, x, y) for y in range(4) for x in range(4)]
    check("half level dithers between 127 and 128", set(low) == {127, 128})
    check("exactly 8 of 16 cells round up at 0.5", low.count(128) == 8)
    check(
        "gamma two darkens a half level",
        model.present_channel(0.5, 2.0, 0, 0) in (63, 64)
        and model.present_channel(0.5, 0.5, 0, 0) in (180, 181),
    )
    check(
        "negative input uses the magnitude",
        model.present_channel(-0.5, 1.0, 1, 1) == model.present_channel(0.5, 1.0, 1, 1),
    )


def scanner_checks() -> None:
    def call(target: int, at: int) -> bytes:
        return b"\xe8" + struct.pack("<i", target - (at + 5))

    base = 0x00401000
    code = bytearray()
    code += b"\x6a\x0f" + call(model.RENDER_STATE_MANAGER, base + len(code) + 2)
    code += (
        b"\x68"
        + struct.pack("<I", 0xC2)
        + call(model.RENDER_STATE_MANAGER, base + len(code) + 5)
    )
    code += b"\x6a\x0b\x56" + call(model.SAMPLER_STATE_MANAGER, base + len(code) + 3)
    code += b"\x6a\x03\x6a\x00" + call(
        model.SAMPLER_STATE_MANAGER, base + len(code) + 4
    )
    code += b"\x52" + call(model.RENDER_STATE_MANAGER, base + len(code) + 1)
    image = promoted.build_image([(base, bytes(code))])
    found = model.scan_state_ids(image)
    check("scanner reads immediate and wide render ids", found["render"] == [15, 194])
    check("scanner reads sampler types", found["sampler"] == [3, 11])
    check(
        "scanner counts register-fed calls",
        found["registerFed"] == {"renderState": 1, "samplerState": 0},
    )
    flagged = [
        item
        for item in (found["render"], found["sampler"])
        if model.SRGB_WRITE_STATE in item or model.SRGB_TEXTURE_SAMPLER in item
    ]
    check("the sRGB ids are reported when present", len(flagged) == 2)


def manifest_checks() -> None:
    document = json.loads(model.MANIFEST.read_text(encoding="ascii"))
    check("the manifest verifies", model.verify(document) == [])
    check("every mutation is rejected", model.mutation_test() == [])
    promoted.exercise("character lighting", model, model.MANIFEST)
    for name in promoted.FAILED:
        FAILED.append(name)
    constants = document["constants"]
    check(
        "constants agree with the reference module",
        constants["specularCurve"]
        == {"gain": model.SPEC_GAIN, "offset": model.SPEC_OFFSET}
        and constants["shininessDivisor"]["bias"] == model.SHININESS_BIAS
        and constants["pointLight"]["attenuationFloor"] == model.POINT_FLOOR
        and constants["pointLight"]["farScale"] == model.POINT_FAR_SCALE
        and document["gammaBlit"]["ditherRows"] == [list(r) for r in model.DITHER_ROWS],
    )
    pins = {item["role"]: item for item in document["shaders"]["install"]}
    check(
        "the lighter pass is shorter than the main pass for each family",
        pins["clothPixel001"]["lines"] < pins["clothPixel000"]["lines"]
        and pins["skinPixel001"]["lines"] < pins["skinPixel000"]["lines"]
        and pins["hairPixel001"]["lines"] < pins["hairPixel000"]["lines"],
    )
    for role, item in pins.items():
        if not role.endswith(("000", "001")):
            continue
        rows = item["constants"]
        shadow = any(row.startswith("shadowMap0 ") for row in rows)
        count = next(r for r in rows if r.startswith("PointLightPositions")).split("x")[
            -1
        ]
        want = (True, "4") if role.endswith("000") else (False, "2")
        check(f"{role} shadow sampler and point light count", (shadow, count) == want)
    missing = copy.deepcopy(document)
    del missing["shaders"]["install"][0]
    check("a missing install pin is reported", model.verify(missing) != [])
    swapped = copy.deepcopy(document)
    swapped["shaders"]["install"][1], swapped["shaders"]["install"][2] = (
        swapped["shaders"]["install"][2],
        swapped["shaders"]["install"][1],
    )
    check("swapped pins are reported", model.verify(swapped) != [])


def census_checks() -> None:
    blob = synthetic_shader()
    gamma = blob.replace(struct.pack("<f", 1.5), struct.pack("<f", 2.2))
    expected = {
        "gammaCandidates": [2.2],
        "gammaCandidateTolerance": 0.005,
        "vertex": 0,
        "pixel": 1,
        "defFloats": [0.0, 1.5],
    }
    with tempfile.TemporaryDirectory() as scratch:
        root = Path(scratch)
        leaf = root / model.INSTALL_TREE / "equ" / "e000" / "top_mdl"
        leaf.mkdir(parents=True)
        (leaf / "0001").write_bytes(b"\x00\x00" + blob + b"\x00" * 8 + blob)
        found = model.shader_census(root)
        check(
            "census dedupes and reads def floats",
            (found["vertex"], found["pixel"], found["defFloats"]) == (0, 1, [0.0, 1.5]),
        )
        check("a clean census passes", model.verify_shader_census(root, expected) == [])
        (leaf / "0001").write_bytes(gamma)
        flagged = model.verify_shader_census(root, expected)
        check(
            "a gamma-like def constant is reported",
            any("near gamma value" in item for item in flagged)
            and any("defFloats" in item for item in flagged),
        )


def main() -> int:
    decoder_checks()
    census_checks()
    reference_checks()
    present_checks()
    scanner_checks()
    manifest_checks()
    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} character lighting model tests")
    return 0


if __name__ == "__main__":
    sys.exit(main())
