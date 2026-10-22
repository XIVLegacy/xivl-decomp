#!/usr/bin/env python3
"""Validate the skeleton hierarchy composition manifest and its executable evidence.

Without options the tool checks the tracked manifest and its mutations. The
other forms read caller-supplied inputs and never copy them:

  --exe PATH      instruction bytes, call targets and function-range digests
  --install ROOT  skeleton selector and root counts under ROOT/client/chara
  --emulate PATH  retail function bytes under Unicorn on synthetic inputs
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import random
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "skeleton_hierarchy_composition.json"
EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
EXPECTED_SIZE = 15_996_808
IMAGE_BASE = 0x00400000
RECORD_SIZE = 176
SELECTORS = ("0x00000012", "0x00000002")
GROUPS = ("bgobj", "mon", "pc", "wep")
DISPATCH = [
    {
        "routineVa": "0x00a40990",
        "testVa": "0x00a409ab",
        "separateCallVa": "0x00a40a2a",
        "matrixCallVa": "0x00a40aba",
    },
    {
        "routineVa": "0x00a404c0",
        "testVa": "0x00a40535",
        "separateCallVa": "0x00a4053e",
        "matrixCallVa": "0x00a4054b",
    },
]
EXPECTED_CALLS = {
    "0x00a40a2a": "0x00a3f960",
    "0x00a40aba": "0x00a3ff70",
    "0x00a4053e": "0x00a3f960",
    "0x00a4054b": "0x00a3ff70",
    "0x00a412e3": "0x00a40990",
    "0x00a3ffc8": "0x00a41ca0",
    "0x00a3ffcf": "0x0061f0f0",
    "0x00a402ba": "0x0061d050",
    "0x00a40442": "0x0061f0f0",
    "0x00a40463": "0x0042edb0",
    "0x00a4046a": "0x00a41650",
    "0x00a40477": "0x00545670",
}
EXPECTED_BYTES = {
    "0x00a40b01": "806752f9",
    "0x00a40dcf": "8b4268",
    "0x00a40dd6": "84c07408",
    "0x00a40dde": "80485202",
    "0x00a40deb": "83f8ff",
    "0x00a409ab": "f6475202",
    "0x00a409b9": "0f8498000000",
    "0x00a40535": "f6415202",
    "0x00a4053c": "740d",
    "0x00a412d5": "6683baf000000000",
    "0x00a412dd": "7d09",
}
EXPECTED_RANGES = {
    "matrix composition routine": ("0x00a3ff70", "0x00a404b4"),
    "hierarchy routine for one joint": ("0x00a404c0", "0x00a40558"),
    "hierarchy routine over every joint": ("0x00a40990", "0x00a40ae7"),
    "matrix multiply": ("0x0061d050", "0x0061d169"),
    "selector bit 4 getter": ("0x00e11d40", "0x00e11d4c"),
    "selector bit 1 getter": ("0x00e11d60", "0x00e11d6c"),
}


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-skeleton-hierarchy-composition-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if source != {
        "binary": "ffxivgame.exe retail 1.23b",
        "build": "2012.09.19.0001",
        "size": EXPECTED_SIZE,
        "binarySha256": EXPECTED_SHA256,
        "imageBase": "0x00400000",
        "ghidraCli": "0.2.2",
        "ghidraVersion": "12.1.4",
        "runId": "skeleton-hierarchy-composition",
    }:
        errors.append("evidence identity changed")
    flag = document.get("flag", {})
    if (
        flag.get("skeletonByte") != "0x52"
        or flag.get("mask") != "0x02"
        or flag.get("clearVa") != "0x00a40b01"
        or flag.get("setVa") != "0x00a40dde"
        or flag.get("selectorBit4GetterSlot") != "0x68"
        or flag.get("selectorBit4GetterVa") != "0x00e11d40"
        or flag.get("selectorBit1GetterSlot") != "0x70"
        or flag.get("selectorBit1GetterVa") != "0x00e11d60"
        or flag.get("rule")
        != "Set when any record of any merged container has selector bit 4."
    ):
        errors.append("flag contract changed")
    routines = document.get("routines", {})
    if (
        routines.get("separateScaleVa") != "0x00a3f960"
        or routines.get("matrixCompositionVa") != "0x00a3ff70"
        or routines.get("initialHierarchyVa") != "0x00a40990"
        or routines.get("perJointVa") != "0x00a404c0"
        or routines.get("perJointCallers")
        != [
            "0x007cd17f",
            "0x00a6d5a6",
            "0x00a6d5e1",
            "0x00a6d680",
            "0x00a6db28",
            "0x00a6db66",
        ]
        or routines.get("dispatch") != DISPATCH
    ):
        errors.append("dispatch contract changed")
    composition = document.get("matrixComposition", {})
    if (
        composition.get("stackArguments")
        != {
            "pose": "[EBP+0x08]",
            "output": "[EBP+0x0c]",
            "parentOutput": "[EBP+0x10]",
        }
        or composition.get("poseScaleOffsets") != ["0xc0", "0xc4", "0xc8", "0xcc"]
        or composition.get("rowScaleMulVas")
        != ["0x00a4007f", "0x00a400fd", "0x00a40186"]
        or composition.get("rowScaleShuffleVas")
        != ["0x00a40012", "0x00a4008a", "0x00a4010b"]
        or composition.get("rowScaleShuffleImmediates") != ["0xaa", "0x55", "0x00"]
        or composition.get("localMatrixHelperVa") != "0x00a41ca0"
        or composition.get("copyHelperVa") != "0x0061f0f0"
        or composition.get("multiplyVa") != "0x0061d050"
        or composition.get("multiplyCallVa") != "0x00a402ba"
        or composition.get("outputMatrixOffset") != "0x00"
        or composition.get("inheritedScaleOffset") != "0x80"
        or composition.get("tailOutputOffset") != "0x40"
        or composition.get("writesOutputQuaternion") is not False
        or composition.get("equation")
        != "G_child = L * G_parent with L rows 0..2 = R(q) rows scaled by s_i and L row 3 = (tx, ty, tz, 1)"
    ):
        errors.append("matrix composition contract changed")
    parentless = document.get("parentless", {})
    if parentless != {
        "parentGetterSlot": "0x54",
        "parentGetterVa": "0x00e11cf0",
        "compareVa": "0x00a40deb",
        "runtimeParentOffset": "0xf0",
        "outputStride": "0xa0",
        "comparesRootCount": False,
        "linkBuilderVa": "0x00e0e1b0",
        "firstJointCheckVa": "0x00a412d5",
    }:
        errors.append("parentless contract changed")
    ranges = document.get("ranges", [])
    if {
        item.get("name"): (item.get("startVa"), item.get("endVa")) for item in ranges
    } != EXPECTED_RANGES or any(
        len(item.get("sha256", "")) != 64
        or set(item.get("sha256", "")) - set("0123456789abcdef")
        for item in ranges
    ):
        errors.append("function ranges changed")
    if {
        item.get("va"): item.get("hex") for item in document.get("bytes", [])
    } != EXPECTED_BYTES:
        errors.append("instruction bytes changed")
    if {
        item.get("va"): item.get("target") for item in document.get("calls", [])
    } != EXPECTED_CALLS:
        errors.append("call targets changed")
    errors += verify_measured(document.get("measured", {}))
    emulation = document.get("emulation", {})
    if emulation != {
        "emulator": "unicorn 2.1.4",
        "trials": {"matrixComposition": 200, "tail": 100, "chains": 5},
        "seeds": {"matrixComposition": 7, "tail": 3, "chains": 11},
        "bounds": {
            "matrixCompositionAbs": 5e-6,
            "tailAbs": 5e-6,
            "unitAndUniformChainAbs": 5e-6,
            "nonuniformChainMin": 0.5,
        },
    }:
        errors.append("emulation contract changed")
    return errors


def verify_measured(measured: dict) -> list[str]:
    errors: list[str] = []
    files = measured.get("skeletonFiles")
    selectors = measured.get("selectorFiles", {})
    groups = measured.get("groups", {})
    if (
        files != 316
        or set(selectors) != set(SELECTORS)
        or sum(selectors.values()) != files
        or set(groups) != set(GROUPS)
        or any(
            group.get("files") != sum(group.get(name, -1) for name in SELECTORS)
            for group in groups.values()
        )
        or sum(group.get("files", 0) for group in groups.values()) != files
        or any(
            sum(group.get(name, 0) for group in groups.values()) != selectors[name]
            for name in SELECTORS
            if name in selectors
        )
    ):
        errors.append("selector tallies are inconsistent")
    if (
        measured.get("filesWithOneRoot") != 315
        or measured.get("filesWithTwoRoots") != ["mon/m039"]
        or measured.get("filesWithOneRoot", 0) + 1 != files
    ):
        errors.append("root tallies changed")
    if measured.get("selector2Monsters") != [
        "m091",
        "m509",
        "m515",
        "m524",
        "m525",
        "m526",
        "m527",
        "m980",
        "m998",
        "m999",
    ]:
        errors.append("selector 0x02 monster list changed")
    if measured.get("identities") != {
        "mon/m509/skl/0001": {
            "size": 13344,
            "sha256": "ad4e96a86df2ce112a04dd00c9a4ac0827473b245ecf7992a4ccf8e584f7cae2",
        },
        "mon/m039/skl/0001": {
            "size": 25120,
            "sha256": "0b0d4a890facaff187eade45174651a197a827991e3338a5a77a835756549756",
        },
    }:
        errors.append("resource identities changed")
    return errors


def mutations() -> list[dict]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("flag", "mask"), "0x01"),
        (("flag", "selectorBit4GetterVa"), "0x00e11d60"),
        (("routines", "matrixCompositionVa"), "0x00a3f960"),
        (("routines", "dispatch"), list(reversed(DISPATCH))),
        (("matrixComposition", "rowScaleShuffleImmediates"), ["0x00", "0x55", "0xaa"]),
        (("matrixComposition", "writesOutputQuaternion"), True),
        (("matrixComposition", "multiplyVa"), "0x0061f0f0"),
        (("parentless", "comparesRootCount"), True),
        (("parentless", "compareVa"), "0x00a40df0"),
        (("emulation", "bounds"), {"matrixCompositionAbs": 1.0}),
    ]
    changed_documents: list[dict] = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        changed[path[0]][path[1]] = value
        changed_documents.append(changed)
    for key in ("ranges", "bytes", "calls"):
        changed = copy.deepcopy(document)
        changed[key] = changed[key][:-1]
        changed_documents.append(changed)
    for name in ("filesWithTwoRoots", "selector2Monsters"):
        changed = copy.deepcopy(document)
        changed["measured"][name] = []
        changed_documents.append(changed)
    changed = copy.deepcopy(document)
    changed["measured"]["selectorFiles"]["0x00000002"] += 1
    changed_documents.append(changed)
    return changed_documents


def mutation_test() -> list[str]:
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations(), 1)
        if not verify(item)
    ]


class Image:
    """A PE32 file read through its section table."""

    def __init__(self, data: bytes) -> None:
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        self.sections = []
        for index in range(count):
            at = pe + 24 + optional + index * 40
            virtual_size, rva, raw_size, raw = struct.unpack_from("<IIII", data, at + 8)
            self.sections.append((rva, max(virtual_size, raw_size), raw))

    def offset(self, va: int) -> int:
        rva = va - IMAGE_BASE
        for start, size, raw in self.sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise ValueError(f"{va:#010x} is not in a section")

    def read(self, va: int, size: int) -> bytes:
        at = self.offset(va)
        return self.data[at : at + size]

    def call_target(self, va: int) -> int | None:
        if self.read(va, 1) != b"\xe8":
            return None
        displacement = struct.unpack("<i", self.read(va + 1, 4))[0]
        return va + 5 + displacement


def verify_executable(path: Path) -> list[str]:
    errors: list[str] = []
    data = path.read_bytes()
    if len(data) != EXPECTED_SIZE:
        errors.append(f"executable size is {len(data)}, expected {EXPECTED_SIZE}")
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        errors.append(f"executable SHA-256 is {digest}, expected {EXPECTED_SHA256}")
        return errors
    image = Image(data)
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    for item in document["bytes"]:
        found = image.read(int(item["va"], 16), len(item["hex"]) // 2).hex()
        if found != item["hex"]:
            errors.append(f"bytes at {item['va']} are {found}")
    for item in document["calls"]:
        target = image.call_target(int(item["va"], 16))
        if target is None or f"{target:#010x}" != item["target"]:
            errors.append(f"call at {item['va']} does not reach {item['target']}")
    for item in document["ranges"]:
        start, end = int(item["startVa"], 16), int(item["endVa"], 16)
        if hashlib.sha256(image.read(start, end - start)).hexdigest() != item["sha256"]:
            errors.append(f"range {item['name']} digest changed")
    return errors


def tally(install: Path) -> dict:
    groups: dict[str, dict] = {}
    one_root, two_roots, selector2 = 0, [], []
    total = 0
    for group in GROUPS:
        for path in sorted((install / "client" / "chara" / group).glob("*/skl/0001")):
            data = path.read_bytes()
            base = data.find(b"SEDBSKL ")
            if base < 0:
                continue
            records = struct.unpack_from("<I", data, base + 0x48)[0] // RECORD_SIZE
            selectors, roots = set(), 0
            for index in range(records):
                at = base + 0x50 + index * RECORD_SIZE
                selectors.add(struct.unpack_from("<I", data, at + 0x48)[0])
                roots += struct.unpack_from("<I", data, at + 0x38)[0] == 0xFFFFFFFF
            if len(selectors) != 1:
                raise ValueError(f"{path} mixes selectors {sorted(selectors)}")
            name = f"{next(iter(selectors)):#010x}"
            entry = groups.setdefault(group, {"files": 0})
            entry["files"] += 1
            entry[name] = entry.get(name, 0) + 1
            total += 1
            if roots == 1:
                one_root += 1
            elif roots == 2:
                two_roots.append(f"{group}/{path.parent.parent.name}")
            else:
                raise ValueError(f"{path} has {roots} roots")
            if group == "mon" and name == "0x00000002":
                selector2.append(path.parent.parent.name)
    return {
        "skeletonFiles": total,
        "groups": groups,
        "filesWithOneRoot": one_root,
        "filesWithTwoRoots": two_roots,
        "selector2Monsters": selector2,
    }


def verify_install(install: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    measured = document["measured"]
    found = tally(install)
    errors = []
    if found["skeletonFiles"] != measured["skeletonFiles"]:
        errors.append(f"skeleton files are {found['skeletonFiles']}")
    for group, expected in measured["groups"].items():
        entry = {name: found["groups"].get(group, {}).get(name, 0) for name in expected}
        if entry != expected:
            errors.append(f"group {group} is {entry}, expected {expected}")
    for key in ("filesWithOneRoot", "filesWithTwoRoots", "selector2Monsters"):
        if found[key] != measured[key]:
            errors.append(f"{key} is {found[key]}")
    for name, expected in measured["identities"].items():
        data = (install / "client" / "chara" / name).read_bytes()
        if (
            len(data) != expected["size"]
            or hashlib.sha256(data).hexdigest() != expected["sha256"]
        ):
            errors.append(f"identity of {name} changed")
    return errors


def emulate(path: Path) -> list[str]:
    from unicorn import UC_ARCH_X86, UC_MODE_32, Uc
    from unicorn.x86_const import (
        UC_X86_REG_CR0,
        UC_X86_REG_CR4,
        UC_X86_REG_ECX,
        UC_X86_REG_ESP,
    )

    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA256:
        return ["executable SHA-256 changed"]
    image = Image(data)
    bounds = json.loads(MANIFEST.read_text(encoding="ascii"))["emulation"]
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(IMAGE_BASE, 0x1400000)
    for rva, size, raw in image.sections:
        uc.mem_write(IMAGE_BASE + rva, data[raw : raw + size])
    for base in (0x20000000, 0x30000000):
        uc.mem_map(base, 0x100000)
    uc.reg_write(UC_X86_REG_CR0, uc.reg_read(UC_X86_REG_CR0) & ~0xC)
    uc.reg_write(UC_X86_REG_CR4, uc.reg_read(UC_X86_REG_CR4) | 0x600)

    pose_at, out_at, parent_at = 0x20000000, 0x20001000, 0x20002000
    chain_at = 0x20010000

    def call(entry: int, args: list[int]) -> None:
        sp = 0x30080000
        for value in reversed(args):
            sp -= 4
            uc.mem_write(sp, struct.pack("<I", value))
        sp -= 4
        uc.mem_write(sp, struct.pack("<I", 0x20080000))
        uc.mem_write(0x20080000, b"\x90")
        uc.reg_write(UC_X86_REG_ESP, sp)
        uc.reg_write(UC_X86_REG_ECX, 0x20003000)
        uc.emu_start(entry, 0x20080000, count=200000)

    def floats(at: int, count: int) -> tuple[float, ...]:
        return struct.unpack(f"<{count}f", bytes(uc.mem_read(at, 4 * count)))

    def quaternion_matrix(q: list[float]) -> list[list[float]]:
        x, y, z, w = q
        return [
            [
                1 - 2 * y * y - 2 * z * z,
                2 * x * y + 2 * z * w,
                2 * x * z - 2 * y * w,
                0,
            ],
            [
                2 * x * y - 2 * z * w,
                1 - 2 * x * x - 2 * z * z,
                2 * y * z + 2 * x * w,
                0,
            ],
            [
                2 * x * z + 2 * y * w,
                2 * y * z - 2 * x * w,
                1 - 2 * x * x - 2 * y * y,
                0,
            ],
            [0, 0, 0, 1],
        ]

    def unit(rng: random.Random) -> list[float]:
        q = [rng.uniform(-1, 1) for _ in range(4)]
        length = math.sqrt(sum(c * c for c in q))
        return [c / length for c in q]

    def set_pose(at: int, t: list[float], q: list[float], s: list[float]) -> None:
        uc.mem_write(at, bytes(0x100))
        uc.mem_write(at + 0x80, struct.pack("<4f", *t, 1.0))
        uc.mem_write(at + 0xA0, struct.pack("<4f", *q))
        uc.mem_write(at + 0xC0, struct.pack("<4f", *s, 1.0))
        uc.mem_write(at + 0xE0, struct.pack("<4f", 0, 0, 0, 1))
        uc.mem_write(at + 0xF0, struct.pack("<hBB", -1, 0, 1))

    def random_parent(rng: random.Random):
        q, s = unit(rng), [rng.uniform(0.5, 2) for _ in range(3)]
        t = [rng.uniform(-3, 3) for _ in range(3)]
        r = quaternion_matrix(q)
        rows = [[r[i][j] * s[i] for j in range(4)] for i in range(3)]
        rows.append([t[0], t[1], t[2], 1.0])
        blob = b"".join(struct.pack("<4f", *row) for row in rows) + bytes(0x40)
        blob += struct.pack("<4f", *s, 1.0) + struct.pack("<4f", *q)
        return rows, s, blob

    errors: list[str] = []
    rng = random.Random(7)
    worst_matrix = 0.0
    for _ in range(bounds["trials"]["matrixComposition"]):
        t = [rng.uniform(-3, 3) for _ in range(3)]
        q = unit(rng)
        s = [rng.uniform(0.5, 2) for _ in range(3)]
        set_pose(pose_at, t, q, s)
        rows, parent_scale, blob = random_parent(rng)
        uc.mem_write(parent_at, blob)
        uc.mem_write(out_at, bytes(0xA0))
        call(0x00A3FF70, [pose_at, out_at, parent_at])
        r = quaternion_matrix(q)
        local = [[r[i][j] * s[i] for j in range(4)] for i in range(3)] + [[*t, 1.0]]
        for i in range(4):
            got = floats(out_at + 16 * i, 4)
            for j in range(4):
                want = sum(local[i][k] * rows[k][j] for k in range(4))
                worst_matrix = max(worst_matrix, abs(got[j] - want))
        scale = floats(out_at + 0x80, 3)
        for i in range(3):
            worst_matrix = max(worst_matrix, abs(scale[i] - parent_scale[i] * s[i]))
    limit = bounds["bounds"]["matrixCompositionAbs"]
    print(f"matrix composition largest absolute difference {worst_matrix:.3e}")
    if worst_matrix > limit:
        errors.append("matrix composition left the bound")

    rng = random.Random(3)
    for entry, name in ((0x00A3F960, "separate-scale"), (0x00A3FF70, "matrix")):
        worst = 0.0
        for _ in range(bounds["trials"]["tail"]):
            t = [rng.uniform(-3, 3) for _ in range(3)]
            q = unit(rng)
            s = [rng.uniform(0.5, 2) for _ in range(3)]
            set_pose(pose_at, t, q, s)
            inverse = [[rng.uniform(-2, 2) for _ in range(4)] for _ in range(4)]
            uc.mem_write(
                pose_at + 0x40, b"".join(struct.pack("<4f", *r) for r in inverse)
            )
            _, _, blob = random_parent(rng)
            uc.mem_write(parent_at, blob)
            uc.mem_write(out_at, bytes(0xA0))
            call(entry, [pose_at, out_at, parent_at])
            g = [floats(out_at + 16 * i, 4) for i in range(4)]
            q_rows = [floats(out_at + 0x40 + 16 * i, 4) for i in range(4)]
            for j in range(4):
                for r in range(4):
                    want = sum(g[k][j] * inverse[r][k] for k in range(4))
                    worst = max(worst, abs(q_rows[j][r] - want))
        print(f"{name} tail largest absolute difference {worst:.3e}")
        if worst > bounds["bounds"]["tailAbs"]:
            errors.append(f"{name} tail left the bound")

    rng = random.Random(11)

    def chain(entry: int, joints: list) -> list:
        uc.mem_write(
            chain_at,
            struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
            + bytes(0x40)
            + struct.pack("<4f", 1, 1, 1, 1)
            + struct.pack("<4f", 0, 0, 0, 1),
        )
        previous, matrices = chain_at, []
        for index, (t, q, s) in enumerate(joints):
            pose = 0x20020000 + index * 0x100
            out = chain_at + 0xA0 * (index + 1)
            set_pose(pose, t, q, s)
            uc.mem_write(out, bytes(0xA0))
            call(entry, [pose, out, previous])
            matrices.append([floats(out + 16 * i, 4) for i in range(4)])
            previous = out
        return matrices

    for name, draw, check in (
        ("unit", lambda: [1.0] * 3, "unitAndUniformChainAbs"),
        ("uniform", lambda: [0.7] * 3, "unitAndUniformChainAbs"),
        (
            "nonuniform",
            lambda: [rng.uniform(0.5, 2) for _ in range(3)],
            "nonuniformChainMin",
        ),
    ):
        joints = [
            ([rng.uniform(-3, 3) for _ in range(3)], unit(rng), draw())
            for _ in range(bounds["trials"]["chains"])
        ]
        a, b = chain(0x00A3F960, joints), chain(0x00A3FF70, joints)
        gap = max(
            abs(a[i][r][c] - b[i][r][c])
            for i in range(len(joints))
            for r in range(4)
            for c in range(4)
        )
        print(f"{name} chains differ by {gap:.3e}")
        if check == "nonuniformChainMin":
            if gap < bounds["bounds"][check]:
                errors.append("routines no longer differ at nonuniform scale")
        elif gap > bounds["bounds"][check]:
            errors.append(f"routines differ at {name} scale")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, help="pinned retail executable")
    parser.add_argument("--install", type=Path, help="retail install root")
    parser.add_argument("--emulate", type=Path, help="pinned executable, needs unicorn")
    args = parser.parse_args()
    errors = verify() + mutation_test()
    suffix = ""
    if args.exe:
        errors += verify_executable(args.exe)
        suffix += " plus executable bytes"
    if args.install:
        errors += verify_install(args.install)
        suffix += " plus install tallies"
    if args.emulate:
        errors += emulate(args.emulate)
        suffix += " plus emulated routines"
    if errors:
        print(f"FAIL: {len(errors)} skeleton hierarchy composition error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print(
        f"PASS: skeleton hierarchy composition manifest, {len(mutations())} mutations{suffix}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
