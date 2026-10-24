#!/usr/bin/env python3
"""Validate the Qix effect resource manifest, optional exe bytes and install census."""

from __future__ import annotations

import argparse
import collections
import copy
import json
import re
import struct
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks
import vfx_qix_resource as qix


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "vfx_qix_resource.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
TAGS = qix.QIX_TAGS
REVERSED_VTEX = qix.REVERSED_TAG["vtex"]
TAG_DWORD = {
    "veff": "0x76656666",
    "vins": "0x76696e73",
    "leaf": "0x6c656166",
    "vmdl": "0x766d646c",
    "vtex": "0x76746578",
}
# tag: (CDev factory vtable, Rapture factory vtable, node vtable, node size)
FACTORIES = {
    "veff": ("0x00fe942c", "0x00fe944c", "0x010b7434", 0x80),
    "vins": ("0x00fe93ac", "0x00fe946c", "0x010b6c84", 0xB0),
    "leaf": ("0x00fe93cc", "0x00fe948c", "0x010b945c", 0xB0),
    "vmdl": ("0x00fe93ec", "0x00fe94ac", "0x010b7c3c", 0x80),
    "vtex": ("0x00fe940c", "0x00fe94cc", "0x010b7f4c", 0x90),
}
VTABLE_SLOTS = {
    "0x00fe93ac": [
        0x7CAF80,
        0xB97690,
        0x60D980,
        0xB975A0,
        0x5B32F0,
        0xB73290,
        0xB73290,
    ],
    "0x00fe93cc": [
        0x7CAF80,
        0xB97880,
        0x60D980,
        0xB977B0,
        0x5B32F0,
        0xB73290,
        0xB73290,
    ],
    "0x00fe93ec": [
        0x7CAF80,
        0xB97A50,
        0x60D980,
        0xB97980,
        0xBC3680,
        0xB73290,
        0xB73290,
    ],
    "0x00fe940c": [
        0x7CAF80,
        0xB97C20,
        0x60D980,
        0xB97B60,
        0x7CAF00,
        0xB73290,
        0xB73290,
    ],
    "0x00fe942c": [
        0x7CAF80,
        0xB974A0,
        0x60D980,
        0xB973D0,
        0xBC3680,
        0xB73290,
        0xB73290,
    ],
    "0x00fe944c": [
        0x7CAFF0,
        0x7CA2F0,
        0x60D980,
        0x7C9F60,
        0xBC3680,
        0xB73290,
        0xB73290,
    ],
    "0x00fe946c": [
        0x7CAFF0,
        0x7CA420,
        0x60D980,
        0x7C9FF0,
        0x5B32F0,
        0xB73290,
        0xB73290,
    ],
    "0x00fe948c": [
        0x7CAFF0,
        0x7CA550,
        0x60D980,
        0x7CA080,
        0x5B32F0,
        0xB73290,
        0xB73290,
    ],
    "0x00fe94ac": [
        0x7CAFF0,
        0x7CA680,
        0x60D980,
        0x7CA110,
        0xBC3680,
        0xB73290,
        0xB73290,
    ],
    "0x00fe94cc": [
        0x7CAFF0,
        0x7CA7B0,
        0x60D980,
        0x7CA1A0,
        0x7CAF00,
        0xB73290,
        0xB73290,
    ],
}
REQUIRED_CHECKS = (
    {
        "blob tag word read at blob plus 4 then byte swap",
        "ResourceSceneGraph constructor vtable store",
        "RaptureWorld constructor vtable store",
        "node size getter 0x80",
        "node size getter 0xb0",
        "node size getter 0x90",
        "clone reads header size word and endian byte",
        "clone tests relocated flag",
        "clone relocated path rebases",
        "clone fresh path relocates",
        "relocated flag getter",
        "relocator tests flag then reads mode byte",
        "relocator dispatches on mode",
        "relocator sets relocated flag",
        "rebase dispatches on mode",
        "mode 2 reads group count and rebases group table",
        "mode 2 group entry reads",
        "mode 2 block entry reads",
        "mode 2 slot loop",
        "mode 2 root table loop",
        "mode 0 group entry reads",
        "mode 0 slot loop",
        "mode 1 slot loop",
        "veff loader requires nonzero root count",
        "veff loader reads root 0 and name words",
        "vtex loader reads root 0 and name words",
        "vmdl loader reads root 0 and name words",
        "leaf and vins loader reads root 0",
        "leaf and vins loader copies name at root plus 0x20",
        "leaf and vins loader copies name at root plus 0x28",
        "leaf and vins loader version word gate",
        "leaf and vins loader version gate failure path",
        "leaf and vins loader version error call",
        "version error format string",
        "leaf and vins loader control entry rejection branch",
        "leaf and vins loader control version error call",
        "binary control version error format string",
        "leaf and vins loader control table count",
        "veff build version word gate",
        "veff build reads sub-record words",
        "veff build passes sub-record address",
        "veff build calls the sub-record constructor",
        "veff reference counts",
        "veff reference array stride",
    }
    | {f"tag dispatch compares {tag}" for tag in TAGS}
    | {f"CDev registers {tag}" for tag in TAGS}
    | {f"Rapture registers {tag}" for tag in TAGS}
    | {f"Rapture factory constructor vtable store {tag}" for tag in TAGS}
    | {f"CDev create allocates node {tag}" for tag in TAGS}
    | {f"CDev create calls node constructor {tag}" for tag in TAGS}
    | {f"Rapture create delegates to CDev create {tag}" for tag in TAGS}
    | {f"node constructor vtable store {tag}" for tag in TAGS}
    | {f"node initializer stores tag {tag}" for tag in TAGS}
    | {
        f"Rapture factory constructor asserts the find resource manager {tag}"
        for tag in TAGS
    }
    | {
        f"Rapture factory constructor pushes the find resource manager string {tag}"
        for tag in TAGS
    }
    | {f"{vtable} factory vtable" for vtable in VTABLE_SLOTS}
)
EXPECTED_OUTCOMES = {
    "five-tag-registry": "STATIC",
    "factory-to-node-class": "STATIC",
    "blob-relocation-in-place": "STATIC",
    "relocation-mode-2": "STATIC",
    "relocation-modes-0-and-1": "STATIC",
    "root-name-offsets": "STATIC",
    "veff-reference-arrays": "STATIC",
    "leaf-version-gate": "STATIC",
    "reference-name-and-tag-meaning": "INFERRED",
    "leaf-to-veff-link": "INFERRED",
    "vins-to-leaf-link": "INFERRED",
    "veff-effect-body": "UNKNOWN",
    "leaf-control-table": "UNKNOWN",
    "payload-words-00-04-0c": "UNKNOWN",
    "mode-1-use": "UNKNOWN",
    "non-sedb-directory-entries": "UNKNOWN",
}
FIXED_TEXT = {
    ("registry", "mapInsert"): "0x0060dac0",
    ("payload", "headerSizeSource"): "blob +0x0e",
    ("payload", "relocatedFlag"): "payload +0x0d bit 0",
    ("payload", "modeSource"): "payload +0x0e byte",
}
HEX_RE = re.compile(r"^0x[0-9a-f]{8}$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
REF_KEYS = (
    "arrays",
    "emptyArrays",
    "entries",
    "arrayAEntries",
    "arrayBEntries",
    "arrayAModelTags",
    "arrayBTextureTags",
    "trailingZero",
    "resolved",
)
SCOPE = (
    "owner installation of build 2012.09.19.0001, client/vfx tree,"
    " counts and structure only"
)


def tag_row(document: dict, tag: str) -> dict:
    for row in document.get("tags", []):
        if row.get("tag") == tag:
            return row
    return {}


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-vfx-qix-resource-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binary") != "ffxivgame.exe retail 1.23b"
        or source.get("build") != "2012.09.19.0001"
        or source.get("size") != EXPECTED_SIZE
        or source.get("binarySha256") != EXPECTED_SHA256
        or source.get("imageBase") != "0x00400000"
    ):
        errors.append("evidence identity changed")
    tags = document.get("tags")
    if not isinstance(tags, list) or [row.get("tag") for row in tags] != list(TAGS):
        errors.append("tags are not the five Qix tags in order")
    for tag in TAGS:
        row = tag_row(document, tag)
        cdev, rapture, node, size = FACTORIES[tag]
        want = {
            "dword": TAG_DWORD[tag],
            "reversedTag": qix.REVERSED_TAG[tag].decode(),
            "cdevFactoryVtable": cdev,
            "raptureFactoryVtable": rapture,
            "nodeVtable": node,
            "nodeSize": size,
            "rootNameOffset": qix.ROOT_NAME_OFFSET[tag],
            "rootTagOffset": qix.ROOT_TAG_OFFSET[tag],
        }
        for key, value in want.items():
            if row.get(key) != value:
                errors.append(f"tag {tag} field {key} changed")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    missing = REQUIRED_CHECKS - set(names)
    if missing:
        errors.append(f"staticChecks lacks {sorted(missing)}")
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not HEX_RE.fullmatch(str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            pattern, mask = item.get("pattern", ""), item.get("mask", "")
            try:
                same = len(bytes.fromhex(pattern)) == len(bytes.fromhex(mask)) > 0
            except ValueError:
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
        elif item["kind"] == "dwords":
            expected = VTABLE_SLOTS.get(item.get("va"))
            if expected is not None and item.get("values") != [
                f"{value:#010x}" for value in expected
            ]:
                errors.append(f"staticChecks {item['name']!r} slot values changed")
    for path, expected in FIXED_TEXT.items():
        node = document
        for key in path:
            node = node.get(key, {}) if isinstance(node, dict) else {}
        if node != expected:
            errors.append(f"{'.'.join(path)} changed")
    payload = document.get("payload", {})
    if (
        payload.get("groupTable") != "0x1c"
        or payload.get("rootTable") != "0x18"
        or payload.get("blockStride") != 12
        or payload.get("groupEntryStride") != 16
        or payload.get("referenceStride") != qix.REF_STRIDE
    ):
        errors.append("payload layout constants changed")
    modes = payload.get("modes", {})
    if sorted(modes) != ["0", "1", "2"]:
        errors.append("payload modes are not 0, 1 and 2")
    outcomes = document.get("outcomes")
    if not isinstance(outcomes, list):
        errors.append("outcomes is not a list")
    else:
        found = {item.get("id"): item.get("tier") for item in outcomes}
        if len(found) != len(outcomes) or found != EXPECTED_OUTCOMES:
            errors.append("outcome ids or tiers changed")
        if any(not str(item.get("claim", "")).strip() for item in outcomes):
            errors.append("an outcome has no claim")
    errors += install_errors(document.get("install", {}))
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def install_errors(install: dict) -> list[str]:
    """Internal consistency of the recorded install census."""
    errors = []
    if install.get("scope") != SCOPE:
        errors.append("install.scope changed")
    banks = install.get("banks", {})
    containers = install.get("containers", {})
    if banks.get("files") != banks.get("pwib", 0) + banks.get("plain", 0):
        errors.append("install.banks wrapper counts disagree with the file count")
    if max(banks.get("pwibFirst16", 0), banks.get("pwibTotalIsLength", 0)) > banks.get(
        "pwib", 0
    ):
        errors.append("install.banks PWIB field counts exceed the PWIB count")
    if containers.get("depth0") != banks.get("files"):
        errors.append("install.containers.depth0 differs from the bank count")
    total = sum(containers.get(f"depth{n}", 0) for n in range(3))
    if total != containers.get("header0x30", 0) + containers.get("header0x40", 0):
        errors.append("install.containers header counts disagree with the depths")
    blobs = install.get("blobs", {})
    if sorted(blobs) != sorted(TAGS):
        return errors + ["install.blobs is not keyed by the five tags"]
    for tag, row in blobs.items():
        count = row.get("count")
        if count != row.get("depth1", 0) + row.get("depth2", 0):
            errors.append(f"install {tag} depth counts disagree with the count")
        if count != row.get("kind1", 0) + row.get("kind4", 0):
            errors.append(f"install {tag} kind counts disagree with the count")
        if count != sum(row.get(f"mode{n}", 0) for n in range(3)):
            errors.append(f"install {tag} mode counts disagree with the count")
        if count != sum(row.get("byte0C", {}).values()):
            errors.append(f"install {tag} byte0C counts disagree with the count")
        if count != sum(row.get("declaredSize", {}).values()):
            errors.append(f"install {tag} declaredSize counts disagree with the count")
        for key in (
            "headerUniform",
            "relocationValid",
            "rootNameValid",
            "rootTagMatches",
            "nameTableLastComponent",
        ):
            value = row.get(key)
            value = value.get("equal") if isinstance(value, dict) else value
            if value is None or value > count:
                errors.append(f"install {tag} {key} is missing or exceeds the count")
        if row.get("groupCountMin", 0) > row.get("groupCountMax", 0):
            errors.append(f"install {tag} group count range is inverted")
    others = install.get("otherSubtypes", {})
    if others.get("SCB") is None:
        errors.append("install.otherSubtypes lacks SCB")
    for subtype, row in others.items():
        if not isinstance(row, dict):
            errors.append(f"install otherSubtypes {subtype} is not a count row")
        elif row.get("count") != sum(row.get(f"depth{n}", 0) for n in range(3)):
            errors.append(f"install otherSubtypes {subtype} depth counts disagree")
    errors += non_sedb_errors(install.get("nonSedb", {}))
    refs = install.get("veffReferences", {})
    if (
        refs.get("entries")
        != refs.get("arrayAEntries", 0) + refs.get("arrayBEntries", 0)
        or refs.get("arrayAModelTags", 0) > refs.get("arrayAEntries", 0)
        or refs.get("arrayBTextureTags", 0) > refs.get("arrayBEntries", 0)
        or refs.get("resolved", 0) > refs.get("entries", 0)
        or refs.get("trailingZero", 0) > refs.get("entries", 0)
    ):
        errors.append("install.veffReferences counts disagree")
    if sum(install.get("veffPerPack", {}).values()) != install.get("packs"):
        errors.append("install.veffPerPack does not cover the packs")
    return errors


def non_sedb_errors(non_sedb: dict) -> list[str]:
    """Internal consistency of the recorded non-SEDB directory entry census."""
    if sorted(non_sedb) != ["all", "leadXetv"]:
        return ["install.nonSedb is not keyed by all and leadXetv"]
    errors = []
    for label, row in non_sedb.items():
        count = row.get("count")
        if count != sum(row.get(f"depth{n}", 0) for n in range(3)):
            errors.append(f"install nonSedb {label} depth counts disagree")
        if count != row.get("kind0", 0) + row.get("kind1", 0):
            errors.append(f"install nonSedb {label} kind counts disagree")
        if count != row.get("leadPrintableAscii", 0) + row.get("leadOther", 0):
            errors.append(f"install nonSedb {label} lead classes disagree")
        if row.get("extendsPastBank", 0) > count:
            errors.append(f"install nonSedb {label} past-end count exceeds the count")
        if row.get("entryBytesMin", 0) > row.get("entryBytesMax", 0):
            errors.append(f"install nonSedb {label} size range is inverted")
    xetv, everything = non_sedb["leadXetv"], non_sedb["all"]
    if xetv.get("count", 0) > everything.get("count", 0) or xetv.get(
        "entryBytes", 0
    ) > everything.get("entryBytes", 0):
        errors.append("install nonSedb leadXetv exceeds all")
    if xetv.get("leadOther", 0):
        errors.append("install nonSedb leadXetv holds a non-printable lead")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("tags", 0, "dword"), "0x76656667"),
        (("tags", 1, "nodeSize"), 0x80),
        (("tags", 2, "rootNameOffset"), 0),
        (("tags", 3, "reversedTag"), "vmdl"),
        (("tags", 4, "nodeVtable"), "0x010b7f4d"),
        (("registry", "mapInsert"), "0x0060dac1"),
        (("payload", "groupTable"), "0x18"),
        (("payload", "blockStride"), 16),
        (("payload", "modeSource"), "payload +0x0f byte"),
        (("payload", "relocatedFlag"), "payload +0x0c bit 0"),
        (("outcomes", 0, "tier"), "INFERRED"),
        (("outcomes", 8, "tier"), "STATIC"),
        (("outcomes", 11, "tier"), "STATIC"),
        (("outcomes", 3, "claim"), " "),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0xb936d9"),
        (("staticChecks", 2, "mask"), "ff"),
        (("install", "scope"), "owner installation"),
        (("install", "banks", "files"), 827),
        (("install", "containers", "depth0"), 827),
        (("install", "blobs", "veff", "count"), 1080),
        (("install", "blobs", "vins", "mode2"), 1043),
        (("install", "blobs", "leaf", "kind4"), 1013),
        (("install", "blobs", "vmdl", "byte0C", "0x02"), 4992),
        (("install", "blobs", "vtex", "relocationValid"), 4643),
        (("install", "blobs", "veff", "groupCountMin"), 99),
        (("install", "veffReferences", "entries"), 10931),
        (("install", "veffPerPack", "1"), 500),
        (("install", "veffReferences", "arrayAModelTags"), 5494),
        (("install", "otherSubtypes", "SCB"), None),
        (("install", "otherSubtypes", "SCB", "depth1"), 22),
        (("install", "nonSedb", "all", "count"), 3199),
        (("install", "nonSedb", "all", "depth1"), 1499),
        (("install", "nonSedb", "all", "kind1"), 23),
        (("install", "nonSedb", "leadXetv", "count"), 746),
        (("install", "nonSedb", "leadXetv", "leadOther"), 1),
        (("install", "nonSedb", "leadXetv", "entryBytesMin"), 2000),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    for name in (
        "relocator dispatches on mode",
        "mode 2 slot loop",
        "veff reference array stride",
        "CDev registers veff",
        "Rapture registers vtex",
        "node initializer stores tag vmdl",
        "0x00fe942c factory vtable",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    changed_slot = copy.deepcopy(document)
    for item in changed_slot["staticChecks"]:
        if item["name"] == "0x00fe942c factory vtable":
            item["values"][1] = "0x00b974a1"
    mutations.append(changed_slot)
    short = copy.deepcopy(document)
    short["outcomes"].pop()
    mutations.append(short)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    return image_checks.verify_image(data, document, require_identity)


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def bank_files(root: Path) -> list[Path]:
    return sorted(
        path
        for path in (root / "client" / "vfx").rglob("*")
        if path.is_file() and path.name != ".DS_Store"
    )


def blank_tag_row() -> dict:
    return {
        "count": 0,
        "entryBytes": 0,
        "depth1": 0,
        "depth2": 0,
        "kind1": 0,
        "kind4": 0,
        "mode0": 0,
        "mode1": 0,
        "mode2": 0,
        "groupCountMin": 0,
        "groupCountMax": 0,
        "byte0C": {},
        "declaredSize": {"equal": 0, "below": 0, "above": 0},
        "headerUniform": 0,
        "relocationValid": 0,
        "rootNameValid": 0,
        "rootTagMatches": 0,
        "nameTableLastComponent": {"equal": 0, "different": 0},
    }


def blank_subtype_row() -> dict:
    return {"count": 0, "entryBytes": 0, "depth0": 0, "depth1": 0, "depth2": 0}


def blank_unwrapped_row() -> dict:
    return {
        "count": 0,
        "entryBytes": 0,
        "entryBytesMin": 0,
        "entryBytesMax": 0,
        "depth0": 0,
        "depth1": 0,
        "depth2": 0,
        "kind0": 0,
        "kind1": 0,
        "leadPrintableAscii": 0,
        "leadOther": 0,
        "extendsPastBank": 0,
    }


def add_unwrapped(row: dict, entry: dict) -> None:
    size = entry["entrySize"]
    row["entryBytesMin"] = size if not row["count"] else min(row["entryBytesMin"], size)
    row["entryBytesMax"] = max(row["entryBytesMax"], size)
    row["count"] += 1
    row["entryBytes"] += size
    row[f"depth{entry['depth']}"] += 1
    row[f"kind{entry['kind']}"] = row.get(f"kind{entry['kind']}", 0) + 1
    printable = len(entry["lead"]) == 4 and all(
        0x20 <= byte < 0x7F for byte in entry["lead"]
    )
    row["leadPrintableAscii" if printable else "leadOther"] += 1
    row["extendsPastBank"] += entry["pastEnd"]


def census_unwrapped(entries: list, out: dict) -> None:
    """Directory entries that do not start with SEDB, plus the xetv-led subset."""
    for entry in entries:
        add_unwrapped(out["all"], entry)
        if entry["lead"] == REVERSED_VTEX:
            add_unwrapped(out["leadXetv"], entry)


def header_uniform(info: dict) -> bool:
    """The SEDB and payload header values every blob in the install shares."""
    return (
        info["unknown08"] == 0
        and info["endianByte"] == 0
        and info["flagsByte"] == 4
        and info["headerSize"] == qix.BLOB_HEADER_SIZE
        and info["word00"] == 0
        and info["word04"] == info["declaredSize"]
        and info["relocatedFlag"] == 0
        and info["rootCount"] == 1
        and info["byte0F"] == 0
        and info["groupTable"] == 0x1C
        and info["rootTable"] == 0x18
    )


def census_bank(data: bytes, out: dict) -> None:
    """Add one bank's counts to the census dictionary."""
    walk = qix.walk_bank(data)
    out["banks"][walk["wrapper"].lower()] += 1
    if walk["wrapper"] == "PWIB":
        first, total = (
            struct.unpack_from(">II", data, 8)[0],
            struct.unpack_from(">I", data, 4)[0],
        )
        out["banks"]["pwibFirst16"] += first == 16
        out["banks"]["pwibTotalIsLength"] += total == len(data)
    for res in walk["containers"]:
        out["containers"][f"depth{res['depth']}"] += 1
        out["containers"][f"header{res['declaredHeader']:#04x}"] += 1
        if res["typeName"] == b"nib\0":
            out["containers"]["typeNameBin"] += 1
    for subtype, entries in walk["other"].items():
        row = out["otherSubtypes"].setdefault(subtype, blank_subtype_row())
        for entry in entries:
            row["count"] += 1
            row["entryBytes"] += entry["entrySize"]
            row[f"depth{entry['depth']}"] += 1
    census_unwrapped(walk["unwrapped"], out["nonSedb"])
    by_container = collections.defaultdict(list)
    for blob in walk["blobs"]:
        by_container[blob["container"]].append(blob)
    for number, blobs in by_container.items():
        res = walk["containers"][number]
        try:
            names = qix.name_table(data, res)
        except qix.ParseError:
            names = []
        starts = [entry["start"] for entry in res["entries"]]
        rows = {}
        for blob in blobs:
            info = qix.parse_blob(data, blob)
            root = qix.root_record(data, info)
            name = qix.cstring(
                data, root + qix.ROOT_NAME_OFFSET[blob["tag"]], qix.NAME_SIZE
            )
            rows[blob["start"]] = (blob, info, root, name)
        veff_names = {n for b, _, _, n in rows.values() if b["tag"] == "veff"}
        out["packs"] += 1
        veff_count = sum(1 for b, _, _, _ in rows.values() if b["tag"] == "veff")
        out["veffPerPack"][str(veff_count)] += 1
        leaf_names = {n for b, _, _, n in rows.values() if b["tag"] == "leaf"}
        sibling = {(n, b["tag"]) for b, _, _, n in rows.values()}
        for blob, info, root, name in rows.values():
            tag = blob["tag"]
            row = out["blobs"][tag]
            row["count"] += 1
            row["entryBytes"] += blob["entrySize"]
            row[f"depth{blob['depth']}"] += 1
            row[f"kind{blob['kind']}"] = row.get(f"kind{blob['kind']}", 0) + 1
            row[f"mode{info['mode']}"] += 1
            groups = info["groupCount"]
            row["groupCountMin"] = (
                groups if row["count"] == 1 else min(row["groupCountMin"], groups)
            )
            row["groupCountMax"] = max(row["groupCountMax"], groups)
            key = f"{info['byte0C']:#04x}"
            row["byte0C"][key] = row["byte0C"].get(key, 0) + 1
            size = blob["entrySize"]
            order = (info["declaredSize"] > size) - (info["declaredSize"] < size)
            row["declaredSize"][{0: "equal", -1: "below", 1: "above"}[order]] += 1
            row["headerUniform"] += header_uniform(info)
            row["relocationValid"] += not qix.relocation_errors(data, info)
            overruns = qix.block_overruns(data, info)
            out["blockOverrun"]["blocks"] += len(overruns)
            out["blockOverrun"]["maxBytes"] = max(
                [out["blockOverrun"]["maxBytes"], *overruns]
            )
            row["rootNameValid"] += name is not None
            tag_at = root + qix.ROOT_TAG_OFFSET[tag]
            row["rootTagMatches"] += data[tag_at : tag_at + 4] == qix.REVERSED_TAG[tag]
            index = starts.index(blob["start"])
            full = names[index] if index < len(names) else None
            same = full is not None and full.split(b"\\")[-1] == name
            row["nameTableLastComponent"]["equal" if same else "different"] += 1
            blob_bytes = data[blob["start"] : blob["start"] + size]
            if tag == "veff":
                version = f"{qix.u16(data, root):#06x}"
                versions = row.setdefault("versionWord", {})
                versions[version] = versions.get(version, 0) + 1
                for number, (pointer, count, want) in enumerate(qix.VEFF_REF_ARRAYS):
                    census_references(
                        data, info, root, (pointer, count, want), number, sibling, out
                    )
            elif tag in ("vins", "leaf"):
                row["versionWord8"] = row.get("versionWord8", 0) + (
                    qix.u16(data, root + 0x1C) == 8
                )
                row["controlTableEntries"] = row.get("controlTableEntries", 0) + (
                    qix.u32(data, root + 4)
                )
                targets = veff_names if tag == "leaf" else leaf_names
                key = "namesSiblingVeff" if tag == "leaf" else "namesSiblingLeaf"
                row[key] = row.get(key, 0) + any(
                    target + b"\0" in blob_bytes for target in targets
                )


def census_references(
    data: bytes,
    info: dict,
    root: int,
    array: tuple,
    number: int,
    sibling: set,
    out: dict,
) -> None:
    pointer, count, want = array
    refs = out["veffReferences"]
    total = qix.u32(data, root + count)
    refs["arrays"] += 1
    if total == 0:
        refs["emptyArrays"] += 1
    start = info["payload"] + qix.u32(data, root + pointer)
    reverse = {value: key for key, value in qix.REVERSED_TAG.items()}
    side = "arrayA" if number == 0 else "arrayB"
    for index in range(total):
        entry = start + qix.REF_STRIDE * index
        name = qix.cstring(data, entry, qix.NAME_SIZE)
        tag = reverse.get(bytes(data[entry + 16 : entry + 20]))
        refs["entries"] += 1
        refs[f"{side}Entries"] += 1
        refs[f"{side}{'ModelTags' if side == 'arrayA' else 'TextureTags'}"] += (
            tag == want
        )
        refs["trailingZero"] += data[entry + 20 : entry + 24] == bytes(4)
        refs["resolved"] += name is not None and (name, tag) in sibling


def census_install(root: Path) -> dict:
    """Counts and structure of every bank below client/vfx. No bytes are kept."""
    out = {
        "scope": SCOPE,
        "banks": collections.Counter(),
        "containers": collections.Counter(),
        "otherSubtypes": {},
        "nonSedb": {"all": blank_unwrapped_row(), "leadXetv": blank_unwrapped_row()},
        "blobs": {tag: blank_tag_row() for tag in TAGS},
        "blockOverrun": {"blocks": 0, "maxBytes": 0},
        "veffReferences": collections.Counter(dict.fromkeys(REF_KEYS, 0)),
        "veffPerPack": collections.Counter(),
        "packs": 0,
    }
    for path in bank_files(root):
        data = path.read_bytes()
        out["banks"]["files"] += 1
        out["banks"]["bytes"] += len(data)
        census_bank(data, out)
    for key in (
        "banks",
        "containers",
        "otherSubtypes",
        "veffReferences",
        "veffPerPack",
    ):
        out[key] = dict(sorted(out[key].items()))
    for key in ("pwib", "plain", "pwibFirst16", "pwibTotalIsLength"):
        out["banks"].setdefault(key, 0)
    return out


def flatten(node, prefix: str = "") -> dict:
    if isinstance(node, dict):
        flat = {}
        for key, value in node.items():
            flat.update(flatten(value, f"{prefix}{key}."))
        return flat
    return {prefix.rstrip("."): node}


def verify_install(root: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    try:
        found = flatten(census_install(root))
    except (qix.ParseError, OSError, struct.error) as exc:
        return [f"install cannot be walked: {exc}"]
    wanted = flatten(document["install"])
    errors = []
    for key in sorted(set(found) | set(wanted)):
        if found.get(key) != wanted.get(key):
            errors.append(
                f"install {key} is {found.get(key)}, manifest has {wanted.get(key)}"
            )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install",
        type=Path,
        help="game root whose client/vfx banks are compared with the census",
    )
    parser.add_argument(
        "--print-install",
        type=Path,
        metavar="ROOT",
        help="print the census of ROOT as JSON and exit",
    )
    args = parser.parse_args()
    if args.print_install:
        print(json.dumps(census_install(args.print_install), indent=2, sort_keys=True))
        return 0
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
    if errors:
        print(f"FAIL: {len(errors)} Qix effect resource error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install census"
    print(f"PASS: Qix effect resource manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
