#!/usr/bin/env python3
"""Tests for the Qix effect body parser, runtime manifest verifier and install census."""

from __future__ import annotations

import copy
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_sqwt_command_parameter_consumer as image_checks  # noqa: E402
import verify_vfx_effect_runtime as verifier  # noqa: E402
import vfx_effect_body as body  # noqa: E402
import vfx_emitter_math as emath  # noqa: E402
import vfx_qix_resource as qix  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000
MANIFEST = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def name16(text: str) -> bytes:
    return text.encode("ascii").ljust(16, b"\0")


def make_blob(tag: str, record: bytes, tail: bytes = b"") -> bytes:
    """A mode 2 blob with one empty group, one root at 0x2c and a record then tail."""
    root = 0x2C
    payload = bytearray(root)
    struct.pack_into("<HH", payload, 0x08, 1, 1)
    payload[0x0C] = 4
    payload[0x0E] = 2
    struct.pack_into("<II", payload, 0x10, 0x1C, 0x18)
    struct.pack_into("<I", payload, 0x18, root)
    struct.pack_into("<IIiH", payload, 0x1C, root, root, 0, 0)
    body_bytes = bytearray(payload) + record + tail
    total = 0x30 + len(body_bytes)
    struct.pack_into("<I", body_bytes, 4, total)
    head = bytearray(0x30)
    head[0:4] = b"SEDB"
    head[4:8] = tag.encode()
    head[0x0D] = 4
    struct.pack_into("<HI", head, 0x0E, 0x30, total)
    return bytes(head) + bytes(body_bytes)


def res(entries: list, names: list) -> bytes:
    directory = bytearray()
    payload = bytearray()
    for index, (kind, data) in enumerate(entries):
        directory += struct.pack("<IIII", index, len(payload), len(data), kind)
        payload += data
    name_at = len(payload)
    payload += b"".join(name + b"\0" for name in names)
    count = len(entries)
    total = 0x40 + 16 * count + len(payload)
    head = bytearray(0x40)
    head[0:8] = b"SEDBRES "
    head[0x0D] = 4
    struct.pack_into("<HI", head, 0x0E, 0x40, total)
    struct.pack_into("<IIII", head, 0x30, count, name_at, count, 0)
    head[0x3C:0x40] = b"nib\0"
    return bytes(head) + bytes(directory) + bytes(payload)


def bank_of(blobs: list[tuple[str, bytes]]) -> bytes:
    inner = res(
        [(4, blob) for _, blob in blobs], [b"bin\\n%d" % i for i in range(len(blobs))]
    )
    outer = res([(4, inner)], [b"bin\\pack"])
    total = 16 + len(outer)
    return b"PWIB" + struct.pack(">III", total, 16, total) + outer


def module_row(name: str, table: str = "rapture") -> dict:
    for row in MANIFEST["modules"]:
        if row["table"] == table and row["name"] == name:
            return row
    raise KeyError(name)


def decl_bytes(row: dict, name_at: int) -> bytes:
    entry = bytearray(24)
    entry[0:6] = bytes.fromhex(row["signature"])
    struct.pack_into("<H", entry, 6, row["kind"])
    struct.pack_into("<I", entry, 0x10, name_at)
    struct.pack_into("<I", entry, 0x14, row["revision"])
    return bytes(entry)


def synthetic_veff(
    version: int = 0x1D, bad_unit: bool = False, flaw: str = ""
) -> bytes:
    """Two units and two instances in a tree. The second unit holds a color node, a
    matrix node and a DrawResource node that links to both."""
    modules = [
        ("cdev", "ColorRGBARoot:CoordRoot"),
        ("cdev", "MatrixRoot:MatrixRoot"),
        ("rapture", "DrawResource"),
    ]
    layout = {}
    tail = bytearray()

    def put(label: str, data: bytes, align: int = 4) -> int:
        while len(tail) % align:
            tail.append(0)
        layout[label] = 0x2C + 0x100 + len(tail)
        tail.extend(data)
        return layout[label]

    decls = bytearray()
    for index, (table, name) in enumerate(modules):
        prefix = MANIFEST["registry"]["namePrefix"][table]
        put(f"name{index}", (prefix + name).encode() + b"\0")
        decls += decl_bytes(module_row(name, table), layout[f"name{index}"])
    put("decl", bytes(decls))
    put("model", name16("mdl") + b"ldmv" + bytes(4))
    tag = b"xxxx" if flaw == "tag" else b"ldmv"
    put("record", bytes(8) + name16("mdl") + tag + bytes(8))
    put("block", struct.pack("<II", layout["record"], 1) + bytes(8))
    slot = bytearray(16)
    slot[9], slot[10], slot[11] = 0, (5 if flaw == "slot" else 0), 0xFF
    put("slot", bytes(slot))
    controls = bytearray()
    for index in range(3):
        control = bytearray(0x24)
        if index == 0:
            struct.pack_into("<II", control, 0, layout["slot"], 1)
        if index == 2:
            struct.pack_into("<I", control, 0x18, layout["block"])
        struct.pack_into("<H", control, 0x1C, index)
        controls += control
    put("control", bytes(controls))
    put(
        "codes",
        struct.pack("<II", 0x10010000 if flaw == "code" else 0x00010000, 0x00010001),
    )
    links = bytearray()
    for index in range(3):
        link = bytearray(0x1C)
        if index == 2:
            struct.pack_into("<II", link, 0, layout["codes"], 2)
        struct.pack_into(
            "<H", link, 0x14, 0 if flaw == "link" and index == 1 else index
        )
        links += link
    put("link", bytes(links))
    put("kids0", struct.pack("<HH", 1, 1) if flaw == "child" else struct.pack("<H", 1))
    schedule = struct.pack("<iHH", 0, 0, 0)
    if flaw == "sched":
        schedule = struct.pack("<iHH", 10, 0, 0) + struct.pack("<iHH", 5, 0, 0)
    put("sched0", schedule)
    put("nodes1", struct.pack("<3H", 0, 1, 2))
    first = struct.pack("<BBH", 1, 2 if flaw == "segment" else 3, 0) + struct.pack(
        "<6f", 1, 2, 3, 0.5, 0.5, 0.5
    )
    put("segments", first + struct.pack("<BBH", 0, 3, 0) + struct.pack("<3f", 4, 5, 6))
    put("cell", struct.pack("<I", layout["segments"]))
    put("starts", struct.pack("<II", 5 if flaw == "start" else 0, 5000))
    put("binding", struct.pack("<III", layout["cell"], layout["starts"], 2))
    put("graph", struct.pack("<HBBIHH", 51, 0, 0, layout["binding"], 1, 0))
    unit0 = struct.pack(
        "<10I",
        0,
        0,
        layout["kids0"],
        2 if flaw == "child" else 1,
        layout["sched0"],
        2 if flaw == "sched" else 1,
        0,
        0,
        0,
        0,
    )
    unit1 = struct.pack(
        "<10I", layout["nodes1"], 3, 0, 0, 0, 0, layout["graph"], 1, 0, 0
    )
    units = bytearray()
    for unit in (unit0, unit1):
        entry = bytearray(0x38)
        entry[0 : len(unit)] = unit
        entry[0x34] = 0 if flaw == "end" else 0xFF
        units += entry
    put("units", bytes(units))
    instances = bytearray(0x48 * 2)
    struct.pack_into("<H", instances, 0x48, 9 if bad_unit else 1)
    put("instances", bytes(instances))
    record = bytearray(0x100)
    record[0:2] = struct.pack("<H", version)
    record[4:20] = name16("eff")
    record[0x14:0x18] = b"ffev"
    for at, key, count in (
        (0x20, "model", 1),
        (0x30, "control", 3),
        (0x38, "link", 3),
        (0x40, "units", 2),
        (0x48, "instances", 1 if flaw == "count" else 2),
        (0x58, "decl", 3),
    ):
        if flaw == "bounds" and key == "model":
            count = 100
        struct.pack_into("<II", record, at, layout[key], count)
    struct.pack_into("<H", record, 0x1C, 1)
    struct.pack_into("<I", record, 0x50, 600000)
    struct.pack_into("<3f", record, 0x70, -1.0, -1.0, -1.0)
    struct.pack_into("<3f", record, 0x80, 1.0, 1.0, 1.0)
    return make_blob("veff", bytes(record), bytes(tail))


def synthetic_emitter_veff(flaw: str = "") -> bytes:
    """A GenerateMaster that holds a Genesis slot in the root unit and a child unit
    with a position receiver and a DrawResource."""
    modules = [
        ("rapture", "GenerateMaster"),
        ("cdev", "Position3DGenerated:GendNormal:CoordRoot"),
        ("rapture", "DrawResource"),
    ]
    layout = {}
    tail = bytearray()

    def put(label: str, data: bytes) -> int:
        while len(tail) % 4:
            tail.append(0)
        layout[label] = 0x2C + 0x100 + len(tail)
        tail.extend(data)
        return layout[label]

    decls = bytearray()
    for index, (table, name) in enumerate(modules):
        prefix = MANIFEST["registry"]["namePrefix"][table]
        put(f"name{index}", (prefix + name).encode() + b"\0")
        decls += decl_bytes(module_row(name, table), layout[f"name{index}"])
    put("decl", bytes(decls))
    put("cell", bytes(4))
    area = bytearray(0x40)
    area[0x1E] = 0x7F
    put("area", bytes(area))
    record = bytearray(0x30)
    struct.pack_into("<f", record, 0x0C, 1.0)
    struct.pack_into("<I", record, 0x1C, layout["cell"])
    record[0x20], record[0x21] = 1, 1
    struct.pack_into("<I", record, 0x24, layout["area"])
    record[0x28] = 1
    struct.pack_into("<f", record, 0x2C, 0.5)
    put("record", bytes(record))
    put("block", struct.pack("<II", layout["record"], 2 if flaw == "kind" else 1))
    slot = bytearray(16)
    slot[9], slot[10], slot[11] = (5 if flaw == "owner" else 0), 0, 0xFF
    put("slot", bytes(slot))
    put("members", struct.pack("<H", 0))
    put("groups", struct.pack("<II", layout["members"], 1) + bytes(8))
    controls = bytearray()
    for index in range(3):
        control = bytearray(0x24)
        if index == 0:
            struct.pack_into("<II", control, 0, layout["slot"], 1)
            struct.pack_into("<II", control, 8, layout["groups"], 2)
            struct.pack_into("<I", control, 0x18, layout["block"])
        struct.pack_into("<H", control, 0x1C, index)
        controls += control
    put("control", bytes(controls))
    links = bytearray()
    for index in range(3):
        link = bytearray(0x1C)
        struct.pack_into("<H", link, 0x14, index)
        links += link
    put("link", bytes(links))
    put("nodes0", struct.pack("<H", 0))
    put("nodes1", struct.pack("<2H", 1, 2))
    put("kids0", struct.pack("<H", 1))
    put("sched0", struct.pack("<iHH", 0, 0, 0))
    if flaw == "rising":
        segments = struct.pack("<BBH", 2, 1, 0) + struct.pack("<3f", 0.0, 1.0, 2.0)
        segments += struct.pack("<BBH", 2, 1, 0) + struct.pack("<3f", 0.0, 1.0, -2.0)
    else:
        segments = struct.pack("<BBH", 0, 1, 0) + struct.pack("<f", 3.0)
        segments += struct.pack("<BBH", 0, 1, 0) + struct.pack("<f", 4.0)
    put("segments", segments)
    put("cellseg", struct.pack("<I", layout["segments"]))
    put("starts", struct.pack("<II", 0, 5000))
    put("binding", struct.pack("<III", layout["cellseg"], layout["starts"], 2))
    graph_index = next(
        row["index"]
        for row in MANIFEST["graphModules"]
        if row["name"] == "StandardPolynomialSureGenesis"
    )
    put("graph", struct.pack("<HBBIHH", graph_index, 0, 0, layout["binding"], 1, 0))
    unit0 = struct.pack(
        "<10I",
        layout["nodes0"],
        1,
        layout["kids0"],
        1,
        layout["sched0"],
        1,
        layout["graph"],
        1,
        0,
        0,
    )
    unit1 = struct.pack("<10I", layout["nodes1"], 2, 0, 0, 0, 0, 0, 0, 0, 0)
    units = bytearray()
    for unit in (unit0, unit1):
        entry = bytearray(0x38)
        entry[0 : len(unit)] = unit
        entry[0x34] = 0xFF
        struct.pack_into("<I", entry, 0x28, 230000)
        units += entry
    put("units", bytes(units))
    child = layout["units"] + (3 if flaw == "child" else 0x38)
    struct.pack_into("<I", tail, layout["cell"] - 0x2C - 0x100, child)
    instances = bytearray(0x48 * 2)
    struct.pack_into("<H", instances, 0x48, 1)
    instances[0x48 + 0x0F] = 2
    put("instances", bytes(instances))
    root = bytearray(0x100)
    root[0:2] = struct.pack("<H", 0x1D)
    root[4:20] = name16("emit")
    root[0x14:0x18] = b"ffev"
    for at, key, count in (
        (0x30, "control", 3),
        (0x38, "link", 3),
        (0x40, "units", 2),
        (0x48, "instances", 2),
        (0x58, "decl", 3),
    ):
        struct.pack_into("<II", root, at, layout[key], count)
    struct.pack_into("<I", root, 0x50, 600000)
    return make_blob("veff", bytes(root), bytes(tail))


def ctab_tokens(version: int, constants: list[tuple[str, int, int]]) -> bytes:
    """A token stream holding a version token and a CTAB comment."""
    strings = bytearray(b"cr\0")
    rows = bytearray()
    for name, register_set, index in constants:
        rows += struct.pack(
            "<IHHHHII",
            28 + 20 * len(constants) + len(strings),
            register_set,
            index,
            4,
            0,
            0,
            0,
        )
        strings += name.encode() + b"\0"
    head = struct.pack(
        "<7I", 28, 28 + 20 * len(constants), version, len(constants), 28, 0, 0
    )
    ctab = b"CTAB" + head + bytes(rows) + bytes(strings)
    ctab += bytes(-len(ctab) % 4)
    comment = struct.pack("<HH", 0xFFFE, len(ctab) // 4) + ctab
    return struct.pack("<I", version) + comment + struct.pack("<I", 0x0000FFFF)


def shader_record(version: int, constants: list, key: bytes = b"K" * 16) -> bytes:
    tokens = ctab_tokens(version, constants)
    return key + struct.pack(">I", len(tokens)) + bytes(12) + tokens


def synthetic_vmdl(vs: bytes, ps: bytes, textures: int = 1, sets: int = 1) -> bytes:
    layout = {}
    tail = bytearray()

    def put(label: str, data: bytes) -> int:
        while len(tail) % 4:
            tail.append(0)
        layout[label] = 0x2C + 0x80 + len(tail)
        tail.extend(data)
        return layout[label]

    put("vs", vs)
    put("ps", ps)
    put(
        "set",
        struct.pack("<5I", layout["vs"], len(vs), layout["ps"], len(ps), 0) * sets,
    )
    refs = b"".join(name16(f"tex{i}") + b"xetv" + bytes(4) for i in range(textures))
    put("refs", refs)
    put("model", struct.pack("<I", 1) + b"mxfv" + bytes(8))
    record = bytearray(0x80)
    record[0:16] = name16("mdl")
    record[0x10:0x14] = b"ldmv"
    struct.pack_into("<II", record, 0x18, layout["set"], sets)
    struct.pack_into("<II", record, 0x20, layout["refs"], textures)
    struct.pack_into("<I", record, 0x28, 1)
    struct.pack_into("<II", record, 0x2C, layout["model"], 16)
    struct.pack_into("<II", record, 0x48, layout["vs"], len(vs))
    struct.pack_into("<II", record, 0x50, layout["ps"], len(ps))
    return make_blob("vmdl", bytes(record), bytes(tail))


def census(files: dict[str, bytes], document: dict | None = None) -> dict:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        for relative, data in files.items():
            path = root / "client" / "vfx" / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        return verifier.census_install(root, document or MANIFEST)


def check_bytes(item: dict) -> bytes:
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "dwords":
        return b"".join(int(v, 16).to_bytes(4, "little") for v in item["values"])
    pattern, mask = bytes.fromhex(item["pattern"]), bytes.fromhex(item["mask"])
    return bytes(p if m else 0 for p, m in zip(pattern, mask))


def build_image(rows: list[tuple[int, bytes]]) -> bytes:
    """A PE32 image with one section per row, each placed at its own VA."""
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(rows) + 0xFFF) & ~0xFFF
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(rows))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    payload = bytearray()
    for index, (va, data) in enumerate(rows):
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII",
            header,
            base + 8,
            len(data),
            va - IMAGE_BASE,
            len(data),
            first_raw + len(payload),
        )
        payload += data
    return bytes(header) + bytes(payload)


def table_image() -> tuple[bytes, dict]:
    """An image holding one tiny module table, family table, graph table and constants."""
    prefix = MANIFEST["registry"]["namePrefix"]["cdev"]
    strings_at = 0x00900000
    strings = bytearray()
    where = {}

    def string(text: str) -> int:
        if text not in where:
            where[text] = strings_at + len(strings)
            strings.extend(text.encode("ascii") + b"\0")
        return where[text]

    modules = bytearray()
    names = ["Alpha", "Beta"]
    for index, name in enumerate(names):
        row = bytearray(0xE8)
        row[0:6] = bytes([3, index, 0, 0, 0, 0])
        struct.pack_into("<H", row, 6, 0x10 + index)
        struct.pack_into("<I", row, 0x18, 3)
        struct.pack_into("<I", row, 0x1C, 0x20)
        struct.pack_into("<I", row, 0x20, 0x10)
        struct.pack_into("<I", row, 0x2C, 0x30)
        struct.pack_into("<I", row, 0x34, 0x24)
        struct.pack_into("<I", row, 0x3C, 0x28)
        struct.pack_into("<I", row, 0x5C, string(prefix + name))
        struct.pack_into("<I", row, 0x60, 100 + index)
        if index:
            struct.pack_into("<I", row, 0x64, string(prefix + "Alpha"))
        struct.pack_into("<I", row, 0x68, 0x00B00000 + index)
        struct.pack_into("<I", row, 0x78, 0x00B00100)
        modules += row
    families = struct.pack(
        "<6I", string(prefix + "AlphaFamily"), string(prefix + "Alpha"), 100, 0, 0, 0
    )
    graphs = bytearray(0x30)
    struct.pack_into(
        "<12I",
        graphs,
        0,
        0x40000,
        0x80008,
        0x40004,
        4,
        string("StandardImmediate"),
        1,
        2,
        3,
        4,
        5,
        6,
        7,
    )
    engine = bytearray()
    for index in range(verifier.ENGINE_COUNT):
        row = bytearray(0x20)
        struct.pack_into("<II", row, 0, string(f"constant{index}"), 16)
        struct.pack_into("<4f", row, 0x10, 1.0, 0.5, 0.25, float(index))
        engine += row
    rows = [
        (0x00A00000, bytes(modules)),
        (0x00A10000, families),
        (0x00A20000, bytes(graphs)),
        (0x00A30000, bytes(engine)),
        (strings_at, bytes(strings)),
    ]
    document = {
        "registry": {
            "namePrefix": MANIFEST["registry"]["namePrefix"],
            "tables": [
                {
                    "id": "cdev",
                    "moduleTable": "0x00a00000",
                    "moduleCount": 2,
                    "familyTable": "0x00a10000",
                    "familyCount": 1,
                }
            ],
            "graphTables": [{"id": "standard", "va": "0x00a20000", "count": 1}],
            "engineTable": "0x00a30000",
        },
        "staticChecks": [],
    }
    image = build_image(rows)
    (
        document["modules"],
        document["families"],
        document["graphModules"],
        document["engineConstants"],
    ) = verifier.image_rows(image, document)
    return image, document


def main() -> int:
    # veff body parser and census
    bank = bank_of([("veff", synthetic_veff())])
    walk = qix.walk_bank(bank)
    info = qix.parse_blob(bank, walk["blobs"][0])
    root = qix.root_record(bank, info)
    veff = body.parse_veff(bank, info, root)
    check(
        "veff root fields",
        veff["version"] == 0x1D
        and veff["lifetimeTicks"] == 600000
        and veff["cullingType"] == 0
        and veff["boundsMin"] == (-1.0, -1.0, -1.0),
    )
    check(
        "veff arrays read their pointers and counts",
        [
            veff["arrays"][k]["count"]
            for k in ("controls", "links", "units", "instances", "decls")
        ]
        == [3, 3, 2, 2, 3]
        and all(item["inside"] for item in veff["arrays"].values()),
    )
    prefix = MANIFEST["registry"]["namePrefix"]["rapture"]
    check(
        "declaration entry fields",
        veff["decls"][2]["name"] == prefix + "DrawResource"
        and veff["decls"][2]["signature"]
        == bytes.fromhex(module_row("DrawResource")["signature"]),
    )
    rel = body.veff_relations(bank, info, root, veff)
    check("relations hold for a tree", all(rel.values()))
    bad = bank_of([("veff", synthetic_veff(bad_unit=True))])
    bad_info = qix.parse_blob(bad, qix.walk_bank(bad)["blobs"][0])
    bad_veff = body.parse_veff(bad, bad_info, qix.root_record(bad, bad_info))
    check(
        "an instance naming a missing unit is flagged",
        not body.veff_relations(
            bad, bad_info, qix.root_record(bad, bad_info), bad_veff
        )["instanceUnitInRange"],
    )
    counts = census({"abl/0001": bank})
    veff_row = counts["veff"]
    check(
        "census counts the synthetic effect",
        veff_row["count"] == 1
        and veff_row["versions"] == {"0x1d": 1}
        and veff_row["totals"]
        == {"controls": 3, "links": 3, "units": 2, "instances": 2, "decls": 3}
        and veff_row["relations"]["childrenReferencedOnce"] == 1
        and veff_row["relations"]["rootInstanceIsUnitZero"] == 1
        and veff_row["relations"]["linkControlPermutation"] == 1
        and veff_row["relations"]["scheduleSorted"] == 1
        and veff_row["relations"]["graphTotalsMatch"] == 1
        and veff_row["relations"]["linkTotalMatchesUnitLists"] == 1
        and veff_row["relations"]["unitEndByteFF"] == 1
        and veff_row["relations"]["unitsEqualInstances"] == 1
        and veff_row["relations"]["arraysInside"] == 1,
    )
    check(
        "census resolves the declaration against the catalog",
        counts["decls"]["veff"]["signatureMatch"] == 3
        and counts["decls"]["veff"]["revisionMatch"] == 3
        and counts["decls"]["veff"]["kindMatch"] == 3
        and counts["decls"]["veff"]["unresolved"] == 0
        and counts["drawResource"]
        == {
            "blocks": 1,
            "kinds": {"1": 1},
            "modelRecords": 1,
            "modelTag": 1,
            "modelFlagsClear": 1,
            "modelInReferences": 1,
            "links": {
                "nodes": 1,
                "twoLinks": 1,
                "bothLocal": 1,
                "color": {"ColorRGBARoot:CoordRoot": 1},
                "matrix": {"MatrixRoot:MatrixRoot": 1},
            },
        },
    )
    check(
        "census walks the graph and slot records",
        counts["graphs"]
        == {
            "records": 1,
            "overflowIdValid": 1,
            "recordPadZero": 1,
            "modules": {"StandardPolynomialFloat32X3": 1},
            "bindings": 1,
            "segments": 2,
            "segmentsContiguous": 1,
            "startsAtZero": 1,
            "startsNondecreasing": 1,
            "degrees": {"0": 1, "1": 1},
        }
        and counts["slots"]
        == {
            "records": 1,
            "graphInRange": 1,
            "bindingInRange": 1,
            "randomNone": 1,
            "randomInRange": 0,
        }
        and veff_row["relations"]["graphTotalsMatch"] == 1,
    )
    for flaw, key in (
        ("link", "linkControlPermutation"),
        ("end", "unitEndByteFF"),
        ("count", "unitsEqualInstances"),
        ("sched", "scheduleSorted"),
        ("child", "childrenReferencedOnce"),
        ("bounds", "arraysInside"),
    ):
        flawed = census({"abl/0001": bank_of([("veff", synthetic_veff(flaw=flaw))])})
        relations = flawed["veff"]["relations"]
        check(
            f"a {flaw} flaw clears its relation",
            relations[key] == 0 and relations["graphTotalsMatch"] == 1,
        )
    for flaw, key in (("start", "startsAtZero"), ("segment", "segmentsContiguous")):
        flawed = census({"abl/0001": bank_of([("veff", synthetic_veff(flaw=flaw))])})
        check(
            f"a {flaw} flaw clears its graph count",
            flawed["graphs"][key] == 0 and flawed["graphs"]["bindings"] == 1,
        )
    badcode = census({"abl/0001": bank_of([("veff", synthetic_veff(flaw="code"))])})
    check(
        "a link code with another top nibble is not local",
        badcode["drawResource"]["links"]["twoLinks"] == 1
        and badcode["drawResource"]["links"]["bothLocal"] == 0
        and badcode["drawResource"]["links"]["color"] == {},
    )
    badslot = census({"abl/0001": bank_of([("veff", synthetic_veff(flaw="slot"))])})
    check(
        "a slot whose binding index is out of range is counted apart",
        badslot["slots"]["graphInRange"] == 1
        and badslot["slots"]["bindingInRange"] == 0,
    )
    badtag = census({"abl/0001": bank_of([("veff", synthetic_veff(flaw="tag"))])})
    check(
        "a model record with another tag is counted apart",
        badtag["drawResource"]["modelRecords"] == 1
        and badtag["drawResource"]["modelTag"] == 0,
    )
    older = census({"abl/0001": bank_of([("veff", synthetic_veff(version=0x1C))])})
    check(
        "an older effect is counted but skips the relations",
        older["veff"]["versions"] == {"0x1c": 1}
        and older["veff"]["relations"] == {}
        and older["decls"]["veffOlder"]["entries"] == 3
        and older["decls"]["veff"]["entries"] == 0,
    )
    wrong = copy.deepcopy(MANIFEST)
    for row in wrong["modules"]:
        if row["name"] == "DrawResource":
            row["signature"] = "030200002000"
    skewed = census({"abl/0001": bank}, wrong)["decls"]["veff"]
    check(
        "a catalog signature that differs is counted as a mismatch",
        skewed["signatureMatch"] == 2 and skewed["signatureMismatch"] == 1,
    )
    # vmdl parser and census
    vs = shader_record(0xFFFE0300, [("controlColor", 2, 4), ("worldMatrix", 2, 0)])
    ps = shader_record(0xFFFF0300, [("_sampler_00", 3, 0)])
    vbank = bank_of([("vmdl", synthetic_vmdl(vs, ps))])
    vinfo = qix.parse_blob(vbank, qix.walk_bank(vbank)["blobs"][0])
    vroot = qix.root_record(vbank, vinfo)
    vmdl = body.parse_vmdl(vbank, vinfo, vroot)
    check(
        "vmdl root fields",
        vmdl["type"] == 1
        and vmdl["shaderSetCount"] == 1
        and vmdl["shaderSets"][0]["flags"] == 0
        and vmdl["vertexShader"] == vmdl["shaderSets"][0]["vertex"]
        and vmdl["textureRefs"] == [{"name": b"tex0", "tag": b"xetv"}],
    )
    record = body.parse_shader(vs)
    check(
        "shader record fields",
        record["lengthMatches"]
        and record["padZero"]
        and record["versionToken"] == 0xFFFE0300
        and record["constants"]
        == [("controlColor", 2, 4, 4), ("worldMatrix", 2, 0, 4)],
    )
    check(
        "a record whose length word is wrong is flagged",
        not body.parse_shader(vs[:16] + struct.pack(">I", 3) + vs[20:])[
            "lengthMatches"
        ],
    )
    vcounts = census({"abl/0002": vbank})["vmdl"]
    check(
        "census counts the synthetic model",
        vcounts["count"] == 1
        and vcounts["shaderSets"] == {"1": 1}
        and vcounts["shaderRecords"] == 2
        and vcounts["uniqueRecords"] == 2
        and vcounts["vertexTokens"] == {"0xfffe0300": 1}
        and vcounts["pixelTokens"] == {"0xffff0300": 1}
        and vcounts["vertexConstants"] == {"controlColor:2": 1, "worldMatrix:2": 1}
        and vcounts["pixelConstants"] == {"_sampler_00:3": 1}
        and vcounts["rootMirrorsFirstSet"] == 1
        and vcounts["modelChunk"] == {"mxfv": 1}
        and vcounts["textureRefs"] == {"1": 1},
    )
    pair = census({"abl/0003": bank_of([("vmdl", synthetic_vmdl(vs, ps, sets=2))])})[
        "vmdl"
    ]
    check(
        "a second shader set is read at the 20 byte stride",
        pair["shaderSets"] == {"2": 1}
        and pair["shaderRecords"] == 4
        and pair["uniqueRecords"] == 2
        and pair["setFlagsZero"] == 2,
    )
    padded = bytearray(vs)
    padded[20] = 7
    odd = census({"abl/0004": bank_of([("vmdl", synthetic_vmdl(bytes(padded), ps))])})
    check(
        "a shader record with a nonzero pad is counted apart",
        odd["vmdl"]["padZero"] == 1 and odd["vmdl"]["lengthMatches"] == 2,
    )
    twice = census({"a/1": vbank, "a/2": vbank})["vmdl"]
    check(
        "identical shader records are counted once",
        twice["count"] == 2
        and twice["shaderRecords"] == 4
        and twice["uniqueRecords"] == 2,
    )
    # image tables
    image, tiny = table_image()
    check(
        "image tables read back",
        tiny["modules"][1]
        == {
            "table": "cdev",
            "index": 1,
            "name": "Beta",
            "signature": "030100000000",
            "kind": 0x11,
            "unitFlags": 3,
            "workSize": 0x20,
            "workAlign": 0x10,
            "slotArrayOffset": 0x30,
            "staticLinkOffset": 0x24,
            "exprLinkOffset": 0x28,
            "revision": 101,
            "slots": {"0x68": "0x00b00001", "0x78": "0x00b00100"},
            "parent": "Alpha",
        }
        and tiny["modules"][0]["signature"] == "030000000000"
        and "parent" not in tiny["modules"][0]
        and tiny["families"][0]
        == {
            "table": "cdev",
            "index": 0,
            "family": "AlphaFamily",
            "current": "Alpha",
            "revision": 100,
            "former": None,
            "formerRevision": 0,
        }
        and tiny["graphModules"][0]
        == {
            "table": "standard",
            "index": 0,
            "name": "StandardImmediate",
            "idWord": "0x00040000",
            "sizeWord": "0x00080008",
            "dataWord": "0x00040004",
            "dataBytes": 4,
            "slots": [f"0x{value:08x}" for value in range(1, 8)],
        }
        and tiny["engineConstants"][20]
        == {"name": "constant20", "bytes": 16, "defaults": [1.0, 0.5, 0.25, 20.0]},
    )
    check("an image matching its rows passes", verifier.table_errors(image, tiny) == [])
    corrupt = copy.deepcopy(tiny)
    corrupt["modules"][1]["signature"] = "030100000001"
    check(
        "a changed row is reported",
        len(verifier.table_errors(image, corrupt)) == 1
        and "modules differ" in verifier.table_errors(image, corrupt)[0],
    )
    broken = copy.deepcopy(tiny)
    broken["registry"]["tables"][0]["moduleTable"] = "0x0ee00000"
    check(
        "a table outside the image is reported",
        "cannot be read" in verifier.table_errors(image, broken)[0],
    )
    # emitter census
    ebank = bank_of([("veff", synthetic_emitter_veff())])
    found = census({"abl/0005": ebank})["emitters"]
    check(
        "census reads the master record and its child unit",
        found["effects"] == 1
        and found["modules"]
        == {
            "R:GenerateMaster": {"nodes": 1, "effects": 1},
            "Position3DGenerated:GendNormal:CoordRoot": {"nodes": 1, "effects": 1},
        }
        and found["master"]
        == {
            "controls": 1,
            "blockKinds": {"1": 1},
            "childRefKinds": {"1": 1},
            "childUnitResolved": 1,
            "childUnitHasDrawResource": 1,
            "childEndTicks": {"min": 230000, "median": 230000, "max": 230000},
            "gated": 1,
            "baseProbability": {"1.000": 1},
            "probabilityRange": {"0.500": 1},
            "areaKinds": {"1": 1},
            "areaSurface": {"1": 1},
            "areaAxesFree": {"1": 0},
            "listenerGroups": {"0:Position3DGenerated:GendNormal:CoordRoot": 1},
        },
    )
    check(
        "census attributes a Genesis graph to the control that holds its slot",
        found["genesis"]
        == {
            "modules": {
                "StandardPolynomialSureGenesis": {
                    "bindings": 1,
                    "segments": 2,
                    "degrees": {"0": 2},
                }
            },
            "owners": {"R:GenerateMaster": 1},
            "risingQuadraticSegments": 0,
        },
    )
    check(
        "census counts the instances that set the loop bit", found["loopInstances"] == 1
    )
    rising = census({"abl/0005": bank_of([("veff", synthetic_emitter_veff("rising"))])})
    check(
        "census counts the Genesis segments with a rising quadratic term",
        rising["emitters"]["genesis"]["risingQuadraticSegments"] == 1
        and rising["emitters"]["genesis"]["modules"]["StandardPolynomialSureGenesis"][
            "degrees"
        ]
        == {"2": 2},
    )
    flawed = census({"abl/0005": bank_of([("veff", synthetic_emitter_veff("child"))])})
    check(
        "a child pointer that is not a unit entry is not resolved",
        flawed["emitters"]["master"]["childUnitResolved"] == 0
        and flawed["emitters"]["master"]["listenerGroups"] == {},
    )
    flawed = census({"abl/0005": bank_of([("veff", synthetic_emitter_veff("kind"))])})
    check(
        "a block of another kind is counted and not read further",
        flawed["emitters"]["master"]["blockKinds"] == {"2": 1}
        and flawed["emitters"]["master"]["areaKinds"] == {},
    )
    flawed = census({"abl/0005": bank_of([("veff", synthetic_emitter_veff("owner"))])})
    check(
        "a slot on a missing graph owns no Genesis value",
        flawed["emitters"]["genesis"]["owners"] == {},
    )
    check(
        "the emitter census agrees with itself",
        verifier.emitter_errors(found, {"0x1d": 1}) == [],
    )
    # descriptor words of the generated receivers
    descriptor = bytearray(0xE8)
    struct.pack_into("<I", descriptor, 0x50, 1)
    struct.pack_into("<I", descriptor, 0x58, 1)
    probe = {
        "registry": {"tables": [{"id": "cdev", "moduleTable": "0x00a00000"}]},
        "modules": [{"table": "cdev", "name": "Gen", "index": 0}],
        "generate": {
            "coordinateModes": [{"table": "cdev", "module": "Gen", "coordinate": 1}]
        },
    }
    probe_image = build_image([(0x00A00000, bytes(descriptor))])
    check(
        "a receiver descriptor with the listed coordinate id passes",
        verifier.generate_image_errors(probe_image, probe) == [],
    )
    probe["generate"]["coordinateModes"][0]["coordinate"] = 2
    check(
        "a receiver whose coordinate id differs is reported",
        len(verifier.generate_image_errors(probe_image, probe)) == 1,
    )
    probe["generate"]["coordinateModes"][0]["coordinate"] = 1
    struct.pack_into("<I", descriptor, 0x50, 0)
    check(
        "a receiver that is not generated is reported",
        len(
            verifier.generate_image_errors(
                build_image([(0x00A00000, bytes(descriptor))]), probe
            )
        )
        == 1,
    )
    # reference math
    mt = emath.ClientMt(5489, pointer_word=0xDEADBEEF)
    out = [mt.next_u32() for _ in range(1250)]
    check(
        "the client generator starts at word 1 of the seeded state",
        out[:3] == [581869302, 3890346734, 3586334585] and out[622] == 4020325887,
    )
    check(
        "the client generator emits the counter cell and refills at word 0",
        out[623] == 3759560541
        and out[624:626] == [4178893912, 610818241]
        and out[1247] == 2538210759
        and out[1248] == 3759560541
        and out[1249] == 358555951,
    )
    check(
        "draw conversions",
        emath.unit_draw(0xFFFFFFFF) == 1.0
        and emath.unit_draw(0) == 0.0
        and emath.unit_draw(0x80000000) == 0.5
        and emath.signed_draw(0x80000000) == -1.0
        and emath.signed_draw(0xC0000000) == -0.5
        and emath.signed_draw(0x40000000) == 0.5,
    )
    sure = (
        [0, 40000],
        [
            (2, 1, (1.0, 1.447808861732483, -0.08070230484008789)),
            (2, 1, (10.0, 0.0, 0.0)),
        ],
    )
    check(
        "polynomial segments evaluate at ticks over 5000",
        abs(emath.graph_value(*sure, 20000) - 6.145617) < 1e-5
        and emath.graph_value(*sure, 40000) == 10.0
        and abs(emath.graph_value(*sure, 39999) - 9.99986) < 1e-4
        and emath.graph_value([0], [(1, 2, (1.0, 0.5, 2.0, 0.25))], 10000, 1) == 2.5,
    )
    check(
        "segments are entered one start at a time",
        emath.graph_segment([0, 30000, 10000], 15000) == 0
        and emath.graph_segment([0, 30000, 10000], 35000) == 2
        and emath.graph_segment([0, 20000, 40000], 20000) == 1,
    )
    counter = emath.Genesis(*sure)
    steps = [counter.step(0, 0)] + [counter.step(5000, 5000 * k) for k in range(1, 11)]
    check(
        "a degree two Genesis binding emits the whole counts it crosses",
        steps == [1, 1, 1, 1, 2, 1, 1, 1, 1, 0, 0],
    )
    line = ([0, 310000], [(1, 1, (1.0, 0.064516)), (1, 1, (5.0, 0.0))])
    counter = emath.Genesis(*line)
    total = counter.step(0, 0)
    for k in range(1, 62):
        total += counter.step(5000, 5000 * k)
    early = total
    for k in range(62, 67):
        total += counter.step(5000, 5000 * k)
    check(
        "a degree one Genesis binding reaches its next constant term",
        early == 4 and total == 5,
    )
    rising = emath.Genesis([0], [(2, 1, (0.0, 1.0, 2.0))])
    check(
        "a rising quadratic Genesis segment emits its peak count once",
        [rising.step(0, 0), rising.step(5000, 5000)] == [0, 2]
        and rising.emitted == 2.0,
    )
    falling = emath.Genesis([0, 5000], [(0, 1, (5.0,)), (1, 1, (1.0, -0.5))])
    check(
        "a falling count never pushes the emitted count back",
        [falling.step(0, 0), falling.step(5000, 5000), falling.step(5000, 10000)]
        == [5, 0, 0]
        and falling.emitted == 5.0,
    )
    gated = emath.Genesis(*sure, gated=True)
    check(
        "a gated Genesis binding emits nothing",
        [gated.step(0, 0)] + [gated.step(5000, 5000 * k) for k in range(1, 11)]
        == [0] * 11
        and gated.emitted == 0.0
        and emath.Genesis([0], [(0, 1, (3.0,))], gated=True).step(0, 0) == 0,
    )
    burst = emath.Genesis(
        [0, 50000, 100000], [(0, 1, (3.0,)), (0, 1, (3.0,)), (0, 1, (5.0,))]
    )
    check(
        "a degree zero segment emits the step of its constant term",
        [burst.step(0, 0), burst.step(5000, 5000), burst.step(5000, 50000)] == [3, 0, 0]
        and burst.step(5000, 100000) == 2,
    )
    check(
        "spawn chance and the camera gate",
        emath.spawn_probability(0.5, 0.5, 0x80000000) == 0.0
        and emath.spawn_probability(0.5, 0.5, 0x40000000) == 0.75
        and emath.distance_pass(2.0, 10.0, 0.2, 6.0, 0x80000000)
        and not emath.distance_pass(2.0, 10.0, 0.2, 6.0, 0xC0000000)
        and not emath.distance_pass(2.0, 10.0, 0.2, 12.0, 0x40000000)
        and emath.distance_pass(2.0, 10.0, 0.2, 2.0, 0xFFFFFFFF)
        and emath.distance_pass(2.0, 10.0, 1.0, 50.0, 0xFFFFFFFF),
    )
    check(
        "axis values follow the flag bit pairs",
        emath.axis_value(1, 1, 0xC0000000) == 0.5
        and emath.axis_value(1, 0, 0xC0000000) == 0.75
        and emath.axis_value(0, 1, 0xC0000000) == -0.75
        and emath.axis_value(0, 0, 0xC0000000) == 0.0
        and emath.axis_vector(0x1E, (0xC0000000, 0x40000000, 0xC0000000))
        == (0.5, -0.5, 0.0),
    )
    draws = (0xC0000000,) * 3
    check(
        "each axis reads its enable bit at 1 + 2i and its sign bit at 2 + 2i",
        emath.axis_vector(0x02, draws) == (0.75, 0.0, 0.0)
        and emath.axis_vector(0x04, draws) == (-0.75, 0.0, 0.0)
        and emath.axis_vector(0x08, draws) == (0.0, 0.75, 0.0)
        and emath.axis_vector(0x10, draws) == (0.0, -0.75, 0.0)
        and emath.axis_vector(0x20, draws) == (0.0, 0.0, 0.75)
        and emath.axis_vector(0x40, draws) == (0.0, 0.0, -0.75),
    )
    axes = (0xC0000000, 0x40000000, 0x80000000)
    half = 0.5**0.5
    position, direction = emath.sphere_sample(
        0x7E, 2.0, 0.5, axes + (0x80000000, 0x80000000)
    )
    shell, _ = emath.sphere_sample(0x7F, 2.0, 0.5, axes + (0x80000000, 0))
    check(
        "a sphere sample scales the unit direction",
        all(
            abs(a - b) < 1e-6
            for a, b in zip(position, (half * 1.125, -half * 1.125, 0.0))
        )
        and all(abs(a - b) < 1e-6 for a, b in zip(direction, (half, -half, 0.0)))
        and all(
            abs(a - b) < 1e-5 for a, b in zip(shell, (half * 2.25, -half * 2.25, 0.0))
        ),
    )
    inner, _ = emath.box_sample(
        0x7E, (1.0, 2.0, 3.0), (0.0, 0.0, 0.0), axes + (0, 0, 0)
    )
    outer, _ = emath.box_sample(
        0x7F, (1.0, 2.0, 3.0), (0.0, 0.0, 0.0), axes + (0, 0, 0)
    )
    check(
        "a box sample snaps the axis chosen by the signed compare",
        inner == (0.5, -1.0, 0.0) and outer == (0.5, -1.0, -3.0),
    )
    wall, wall_direction = emath.cylinder_sample(
        0x7F, 2.0, 3.0, (0.0, 0.5), axes + (0x80000000,) * 3
    )
    cap, _ = emath.cylinder_sample(
        0x7F,
        2.0,
        3.0,
        (0.0, 0.0),
        (0x90000000, 0xF0000000, 0x90000000, 0x80000000, 0, 0),
    )
    check(
        "a cylinder sample picks the wall or the cap",
        abs(wall[0]) < 1e-6
        and abs(wall[1] + 1.625) < 1e-6
        and abs(wall[2] - 2.0) < 1e-6
        and abs(wall_direction[2] - 1.0) < 1e-6
        and abs(cap[0] - half) < 1e-5
        and cap[1] == 3.0
        and abs(cap[2] - half) < 1e-5,
    )
    check(
        "generated motion follows the elapsed frames",
        emath.generated_position((1, 2, 3), (0, 1, 0), 0.035, -0.00025, 500000)
        == (1.0, 4.25, 3.0)
        and emath.generated_position((0, 0, 0), (0, 1, 0), 0.0, 0.0, 5000, (10, 0, 0))
        == (10.0, 0.0, 0.0)
        and emath.generated_rotation(
            (0.1, 0, 0), (0.2, 0, 0), (0.02, 0, 0), 50000, (1, 0, 0)
        )
        == (4.1, 0.0, 0.0)
        and emath.generated_scale((1, 1, 1), (0.1, 0, 0), (0, 0, 0), 50000, (2, 2, 2))
        == (4.0, 2.0, 2.0),
    )
    drag = emath.DragPosition((0, 0, 0), (1, 0, 0), 1.0, 0.5)
    check(
        "the drag mode damps the step once per update",
        [drag.update(5000)[0] for _ in range(3)] == [1.0, 1.5, 1.75],
    )
    check(
        "particle steps and line lifetimes",
        emath.particle_step((0, 0, 0), (1, 0, 0), (2, 0, 0), 10000)
        == ((6.0, 0.0, 0.0), (5.0, 0.0, 0.0))
        and emath.line_lifetime_ticks(16, 8, 8) == 120000,
    )
    record = {
        "speed": (0.075, 0.025),
        "drag": (1.0, 0.0),
        "velocity": ((0, 0, 0), (1, 1, 1)),
        "accel": ((0, 0, 0), (0, 0, 0)),
    }
    plain = emath.emission_init(record, (0, 1, 0), [0] * 14)
    jittered = emath.emission_init(
        record, (0, 0, 0), [0x40000000, 0, 0x40000000, 0x40000000] + [0] * 10
    )
    flipped = emath.emission_init(record, (0, 0, 0), [0, 0, 0x40000000, 0] + [0] * 10)
    check(
        "emission start state",
        plain[0] == [0.0, 0.075, 0.0]
        and plain[2] == 1.0
        and jittered[0][0] == 0.25
        and flipped[0][0] == -0.25,
    )
    # manifest
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    check(
        "outcome tiers are pinned",
        {o["id"]: o["tier"] for o in MANIFEST["outcomes"]}
        == verifier.EXPECTED_OUTCOMES,
    )
    rows = [(int(i["va"], 16), check_bytes(i)) for i in MANIFEST["staticChecks"]]
    static_image = build_image(rows)
    check(
        "an image built from the check rows passes the byte checks",
        image_checks.verify_image(static_image, MANIFEST, require_identity=False) == [],
    )
    check(
        "identity is enforced",
        len(image_checks.verify_image(static_image, MANIFEST)) == 2,
    )
    moved = copy.deepcopy(MANIFEST)
    for item in moved["staticChecks"]:
        if item["kind"] == "string":
            item["text"] = item["text"][:-1]
            break
    check(
        "a check whose expected bytes differ is reported",
        len(image_checks.verify_image(static_image, moved, require_identity=False))
        == 1,
    )
    shrunk = copy.deepcopy(MANIFEST)
    shrunk["install"]["decls"]["veff"]["signatureMatch"] -= 1
    check(
        "an inconsistent install block is reported",
        any(
            "signature counts" in e for e in verifier.install_errors(shrunk["install"])
        ),
    )
    check(
        "the recorded install agrees with itself",
        verifier.install_errors(MANIFEST["install"]) == [],
    )
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "client" / "vfx" / "abl" / "0001"
        path.parent.mkdir(parents=True)
        path.write_bytes(bank)
        document = {**MANIFEST, "install": counts}
        check(
            "install census matches itself",
            verifier.verify_install(Path(directory), document) == [],
        )
        path.write_bytes(bad)
        found = verifier.verify_install(Path(directory), document)
        check(
            "an install whose structure changed is reported",
            any("instanceUnitInRange" in error for error in found),
        )
    print(f"{len(PASSED)} passed, {len(FAILED)} failed")
    for name in FAILED:
        print(f"FAIL: {name}")
    return 1 if FAILED else 0


if __name__ == "__main__":
    raise SystemExit(main())
