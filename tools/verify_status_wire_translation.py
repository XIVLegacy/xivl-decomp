#!/usr/bin/env python3
"""Validate the status wire translation contract and optional exe bytes."""

from __future__ import annotations

import argparse
import copy
import json
import re
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "status_wire_translation.json"
DOC = ROOT / "docs" / "actor" / "status-wire-translation.md"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
SECTIONS = ("translation", "receiver", "producers", "sheet", "corrections")

KEY_BASE = 200_000
HIGH_FLOOR = 0x8000
HIGH_ADJUSTMENT = 0x4350
LOW_KEYS = (KEY_BASE + 1, KEY_BASE + HIGH_FLOOR)
HIGH_KEYS = (
    KEY_BASE + HIGH_FLOOR + 1 + HIGH_ADJUSTMENT,
    KEY_BASE + 0xFFFF + HIGH_ADJUSTMENT,
)

# xivl-client-data derived/status_names.csv at 68990f1f3f4f2c4018f351ff21497e2305a16c43.
SHEET_ROW_COUNT = 399
SHEET_LOW_ROWS = {"count": 394, "min": 221000, "max": 232015}
SHEET_HIGH_ROWS = (251001, 253001, 253002, 253003)

EDGES = [
    (0x0000, None),
    (0x0001, 200001),
    (0x5A59, 223129),
    (0x5ADF, 223263),
    (0x5DBF, 223999),
    (0x8000, 232768),
    (0x8001, 250001),
    (0x83E9, 251001),
    (0x8BB9, 253001),
    (0x8BBA, 253002),
    (0x8BBB, 253003),
    (0xFFFF, 282767),
]


def status_key(wire: int) -> int | None:
    """The key 0x008A3350 hands to the handle lookup, or None for the zero id."""
    if not 0 <= wire <= 0xFFFF:
        raise ValueError("wire status id must fit u16")
    if wire == 0:
        return None
    return KEY_BASE + wire + (HIGH_ADJUSTMENT if wire > HIGH_FLOOR else 0)


def wire_for_key(key: int) -> int | None:
    """The only wire id that produces a key, or None when no wire id does."""
    if LOW_KEYS[0] <= key <= LOW_KEYS[1]:
        return key - KEY_BASE
    if HIGH_KEYS[0] <= key <= HIGH_KEYS[1]:
        return key - KEY_BASE - HIGH_ADJUSTMENT
    return None


def run_translation(code: bytes, wire: int) -> int:
    """Execute the translation bytes with bp holding the wire id; eax is the key.

    Only the seven encodings the retail sequence uses are decoded.
    """
    eax = ecx = edx = 0
    bp = wire & 0xFFFF
    carry = 0
    index = 0
    while index < len(code):
        head = code[index : index + 3]
        if head[:2] == b"\x66\xba":
            edx = int.from_bytes(code[index + 2 : index + 4], "little")
            index += 4
        elif head == b"\x66\x3b\xd5":
            carry = 1 if (edx & 0xFFFF) < bp else 0
            index += 3
        elif head[:2] == b"\x1b\xc0":
            eax = (-carry) & 0xFFFFFFFF
            index += 2
        elif head[:1] == b"\x25":
            eax &= int.from_bytes(code[index + 1 : index + 5], "little")
            index += 5
        elif head == b"\x0f\xb7\xcd":
            ecx = bp
            index += 3
        elif head[:1] == b"\x05":
            eax = (eax + int.from_bytes(code[index + 1 : index + 5], "little")) & (
                0xFFFFFFFF
            )
            index += 5
        elif head[:2] == b"\x03\xc1":
            eax = (eax + ecx) & 0xFFFFFFFF
            index += 2
        elif head[:2] == b"\x2b\xc1":
            eax = (eax - ecx) & 0xFFFFFFFF
            index += 2
        else:
            raise ValueError(f"unsupported encoding at offset {index}: {head.hex()}")
    return eax


EXPECTED = {
    "translation": {
        "functionVa": "0x008a3350",
        "convention": "thiscall, four stack arguments, ret 0x10",
        "arguments": [
            {"name": "this", "register": "ecx", "use": "receiver storage"},
            {"name": "handleTable", "stack": "0x2c", "use": "key lookup context"},
            {
                "name": "ownerHandle",
                "stack": "0x30",
                "use": "pointer, dword copied to +0x08",
            },
            {"name": "slot", "stack": "0x34", "use": "byte stored at +0x14"},
            {"name": "wire", "stack": "0x38", "use": "u16 held in bp"},
        ],
        "zeroWire": {
            "testVa": "0x008a33a5",
            "handleSourceVa": "0x0130c778",
            "sentinel": "0xe0000000",
            "lookupCalled": False,
            "doneFlag": 1,
        },
        "nonzero": {
            "base": 200000,
            "highFloor": "0x8000",
            "compare": "cmp dx,bp sets carry when 0x8000 < wire, unsigned",
            "mask": "sbb eax,eax gives 0xffffffff on carry, and eax,0x4350 keeps 0x4350",
            "adjustment": "0x4350",
            "adjustmentDecimal": 17232,
            "adjustmentSign": "added",
            "form": "key = 200000 + wire + (0x4350 if wire > 0x8000 else 0)",
            "wireWidth": "zero-extended from 16 bits",
            "keyRangeTest": "none",
            "lookupCallVa": "0x008a33d3",
            "lookupVa": "0x00cc70e0",
            "handleCallVa": "0x00cc7160",
        },
        "images": {
            "low": {"wires": [1, 32768], "keys": [200001, 232768]},
            "high": {"wires": [32769, 65535], "keys": [250001, 282767]},
            "unreachableKeys": [232769, 250000],
            "overlap": False,
        },
        "edges": [{"wire": f"0x{wire:04x}", "key": key} for wire, key in EDGES],
        "tier": "STATIC",
    },
    "receiver": {
        "class": "Application::Lua::Script::Client::Command::System::ChangeActorSubStatStatusReceiver",
        "vtableVa": "0x010575c8",
        "slots": ["0x008a4f40", "0x008a34d0", "0x008a3550", "0x008a3630", "0x008a36a0"],
        "size": "0x18",
        "fields": {
            "0x08": "owner actor handle",
            "0x0c": "status handle from the key",
            "0x10": "cached status object, zero after construction",
            "0x14": "slot byte",
            "0x15": "done flag, preset to wire == 0",
        },
        "slot2": {
            "doneShortCircuitVa": "0x008a3553",
            "registryMissesVa": "0x008a3584",
            "doneClearExitVa": "0x008a3603",
            "successExitVa": "0x008a3615",
            "statusMissOutcome": "done-clear exit, done flag stays clear",
            "statusCastType": "0x012c31f8",
            "ownerCastType": "0x012709a4",
            "sourceCastType": "0x01270b4c",
            "ownerGateOffset": "0x5c",
            "statusReadyOffset": "0x7d",
            "successByteVa": "0x012c41af",
            "exitByteVa": "0x0134c560",
        },
        "slot3": {
            "va": "0x008a3630",
            "callVa": "0x008a3687",
            "targetVa": "0x007084b0",
            "arguments": "manager, slot byte, cached status object, 1",
            "ownerMissOutcome": "passes the status handle to 0x00cc7170 unless it is the sentinel",
        },
        "tier": "STATIC",
    },
    "producers": {
        "callSites": [
            {"va": "0x0076c7b1", "function": "0x0076c720", "branch": "stack temporary"},
            {"va": "0x0076c83c", "function": "0x0076c720", "branch": "heap"},
            {"va": "0x0076d4d4", "function": "0x0076d450", "branch": "heap"},
        ],
        "slot88": [
            {
                "class": "Application::Lua::Script::Client::LuaActorImpl",
                "vtableVa": "0x00fdfb2c",
                "slotVa": "0x00fdfc8c",
                "function": "0x0076c720",
            },
            {
                "class": "Application::Lua::Script::Client::NullActorImpl",
                "vtableVa": "0x00fe02ac",
                "slotVa": "0x00fe040c",
                "function": "0x0076d450",
            },
        ],
        "slot88Thunk": {
            "va": "0x005752a0",
            "vtableOffset": "0x160",
            "callerVa": "0x00588d80",
        },
        "arguments": "slot index, wire id, unchanged from the packet",
        "apply": {
            "va": "0x00588d20",
            "loop": 20,
            "dirtyShift": 3,
            "recordOffset": "0x100",
            "appliesZeroWire": True,
        },
        "stage": {
            "va": "0x0058eab0",
            "wordOffset": "0x18",
            "maskOffset": "0x08",
            "maskKeeps": "0x007ffff8",
        },
        "dispatcher": {
            "highBlockVa": "0x0058d330",
            "opcodeBase": "0x135",
            "byteTableVa": "0x0058d994",
            "dwordTableVa": "0x0058d930",
            "opcodes": [
                {
                    "opcode": "0x0177",
                    "byteEntryVa": "0x0058d9d6",
                    "caseVa": "0x0058d45b",
                    "indexOffset": "0x10",
                    "indexWidth": 1,
                    "wireOffset": "0x12",
                },
                {
                    "opcode": "0x0179",
                    "byteEntryVa": "0x0058d9d8",
                    "caseVa": "0x0058d488",
                    "arrayOffset": "0x10",
                    "count": 20,
                },
            ],
        },
        "tier": "STATIC",
    },
    "sheet": {
        "table": "xivl-client-data:derived/status_names.csv",
        "commit": "68990f1f3f4f2c4018f351ff21497e2305a16c43",
        "rowCount": SHEET_ROW_COUNT,
        "lowRows": SHEET_LOW_ROWS,
        "rowsBetween": {"from": 232769, "to": 250000, "count": 0},
        "highRows": [
            {"row": row, "wire": f"0x{wire_for_key(row):04x}"}
            for row in SHEET_HIGH_ROWS
        ],
        "tier": "INFERRED",
        "assumption": "the sheet row ids are the keys the client resolves",
    },
    "corrections": [
        {
            "repository": "xivl-captures",
            "path": "studies/status-wire-projection-census/README.md",
            "section": "Source material",
            "replace": "The native translation is `row = 200000 + wire - adjustment`, where adjustment is `0x4350` only when `wire > 0x8000`. A row can therefore have overlapping low and high reverse encodings.",
            "with": "The native translation is `row = 200000 + wire + adjustment`, where adjustment is `0x4350` only when `wire > 0x8000`. The low branch reaches rows 200001 to 232768 and the high branch rows 250001 to 282767, so a row has at most one wire id.",
        },
        {
            "repository": "xivl-captures",
            "path": "studies/status-wire-projection-census/derived/verdicts.md",
            "section": "Projection witnesses and Remaining boundary",
            "replace": "Each observed row has both a low and high reverse wire encoding under the native transform.",
            "with": "Each observed row has one wire encoding, the low one. The high branch cannot reach rows 223129, 223263 or 223999.",
        },
        {
            "repository": "xivl-captures",
            "path": "studies/status-wire-projection-census/derived/status-projections.csv",
            "section": "all_wire_ids_for_row_hex",
            "replace": "0x5a59 0x9da9, 0x5adf 0x9e2f, 0x5dbf 0xa10f",
            "with": "0x5a59, 0x5adf, 0x5dbf",
        },
        {
            "repository": "xivl-captures",
            "path": "tools/extractors/extract_status_wire_census.py",
            "section": "decode_wire_id",
            "replace": "200000 + wire_id - (0x4350 if wire_id > 0x8000 else 0)",
            "with": "200000 + wire_id + (0x4350 if wire_id > 0x8000 else 0)",
        },
        {
            "repository": "xivl-client-data",
            "path": "docs/substat-status-join.md",
            "section": "Lookup chain",
            "replace": "wire >  0x8000: 200000 + wire - 0x4350",
            "with": "wire >  0x8000: 200000 + wire + 0x4350",
        },
        {
            "repository": "xivl-client-data",
            "path": "derived/substat_status_crosswalk.csv",
            "section": "high_wire_id_hex",
            "replace": "a high wire id for every row from 221000 to 232015",
            "with": "no high wire id for those rows, and 0x83E9, 0x8BB9, 0x8BBA and 0x8BBB for rows 251001, 253001, 253002 and 253003",
        },
        {
            "repository": "xivl-client-structs",
            "path": "manifests/lua_bridge_verification.json and manifests/symbols.json",
            "section": "ChangeActorSubStatStatusReceiver constructor notes",
            "replace": "+200000 for status_value < 0x8000, +217232 for status_value >= 0x8000",
            "with": "+200000 for status_value <= 0x8000, +217232 for status_value > 0x8000",
        },
    ],
}

REQUIRED_CHECKS = {
    "wire load and zero test",
    "translation sequence",
    "key lookup call",
    "owner handle copy and vtable store",
    "slot byte and done flag stores",
    "handle store",
    "sentinel compare and register",
    "sentinel handle value",
    "receiver vtable",
    "call site LuaActorImpl stack temporary",
    "call site LuaActorImpl heap",
    "call site NullActorImpl",
    "LuaActorImpl slot 88",
    "NullActorImpl slot 88",
    "slot 88 thunk",
    "stage helper",
    "apply loop",
    "dispatcher high block",
    "dispatcher byte entry 0x0177",
    "dispatcher byte entry 0x0179",
    "dispatcher dword entry 0x0177",
    "dispatcher dword entry 0x0179",
    "case 0x0177",
    "case 0x0179",
    "slot 2 done short circuit",
    "slot 2 registry misses",
    "slot 2 done-clear exit",
    "slot 2 success exit",
    "slot 3 apply",
    "success byte",
}

CITED_ADDRESSES = [
    "0x008a3350",
    "0x008a33a5",
    "0x008a33b1",
    "0x008a33d3",
    "0x00cc70e0",
    "0x00cc7160",
    "0x0130c778",
    "0x010575c8",
    "0x008a3550",
    "0x008a3603",
    "0x008a3615",
    "0x008a3630",
    "0x007084b0",
    "0x0076c720",
    "0x0076d450",
    "0x00fdfc8c",
    "0x00fe040c",
    "0x005752a0",
    "0x00588d20",
    "0x0058eab0",
    "0x0058d330",
    "0x0058d45b",
    "0x0058d488",
]


def doc_errors() -> list[str]:
    if not DOC.is_file():
        return ["status wire translation page is missing"]
    text = DOC.read_text(encoding="utf-8").lower()
    errors = [
        f"page does not cite {address}"
        for address in CITED_ADDRESSES
        if address.lower() not in text
    ]
    for tier in ("static", "inferred"):
        if tier not in text:
            errors.append(f"page does not use the {tier.upper()} tier label")
    for needle in (
        "200000 + wire + 0x4350",
        "what bahamut-client can change",
        "xivl-captures",
    ):
        if needle not in text:
            errors.append(f"page lacks {needle!r}")
    return errors


def translation_errors(document: dict) -> list[str]:
    """Execute the manifest's translation bytes against the numeric model."""
    errors: list[str] = []
    rows = {
        item.get("name"): item
        for item in document.get("staticChecks", [])
        if isinstance(item, dict)
    }
    row = rows.get("translation sequence")
    if row is None:
        return ["translation sequence row is missing"]
    try:
        code = bytes.fromhex(row["pattern"])
        for wire in range(1, 0x10000):
            got = run_translation(code, wire)
            if got != status_key(wire):
                errors.append(
                    f"translation bytes map {wire:#06x} to {got}, model says "
                    f"{status_key(wire)}"
                )
                break
    except (KeyError, ValueError) as exc:
        errors.append(f"translation sequence is not executable: {exc}")
    return errors


def model_errors() -> list[str]:
    errors: list[str] = []
    seen: dict[int, int] = {}
    for wire in range(1, 0x10000):
        key = status_key(wire)
        if key in seen:
            errors.append(f"wire ids {seen[key]:#06x} and {wire:#06x} share a key")
            break
        seen[key] = wire
        if wire_for_key(key) != wire:
            errors.append(f"wire {wire:#06x} does not round trip")
            break
    for wire, key in EDGES:
        if (status_key(wire) if wire else None) != key:
            errors.append(f"edge {wire:#06x} disagrees with the model")
    if min(seen) != LOW_KEYS[0] or max(seen) != HIGH_KEYS[1]:
        errors.append("key range disagrees with the images")
    if any(LOW_KEYS[1] < key < HIGH_KEYS[0] for key in seen):
        errors.append("a key falls in the unreachable gap")
    return errors


def verify(document: dict | None = None, check_doc: bool = True) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-status-wire-translation-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binarySha256") != EXPECTED_SHA256
        or source.get("size") != EXPECTED_SIZE
        or source.get("imageBase") != "0x00400000"
        or source.get("build") != "2012.09.19.0001"
    ):
        errors.append("evidence identity changed")
    for section in SECTIONS:
        if document.get(section) != EXPECTED[section]:
            errors.append(f"{section} contract changed")
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
    errors += translation_errors(document)
    errors += model_errors()
    if check_doc:
        errors += doc_errors()
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("translation", "nonzero", "adjustmentSign"), "subtracted"),
        (("translation", "nonzero", "adjustment"), "0x4530"),
        (("translation", "nonzero", "highFloor"), "0x7fff"),
        (("translation", "nonzero", "base"), 20000),
        (("translation", "nonzero", "keyRangeTest"), "present"),
        (("translation", "zeroWire", "lookupCalled"), True),
        (("translation", "zeroWire", "sentinel"), "0x00000000"),
        (("translation", "images", "overlap"), True),
        (("translation", "images", "high", "keys", 0), 215537),
        (("translation", "edges", 5, "key"), 232767),
        (("translation", "edges", 6, "key"), 215537),
        (("receiver", "vtableVa"), "0x010575d4"),
        (("receiver", "fields", "0x0c"), "owner actor handle"),
        (("receiver", "slot2", "statusMissOutcome"), "success exit"),
        (("receiver", "slot2", "doneClearExitVa"), "0x008a3615"),
        (("receiver", "slot3", "targetVa"), "0x007084c0"),
        (("producers", "slot88", 0, "slotVa"), "0x00fdfc90"),
        (("producers", "slot88Thunk", "vtableOffset"), "0x164"),
        (("producers", "apply", "appliesZeroWire"), False),
        (("producers", "dispatcher", "opcodes", 0, "wireOffset"), "0x10"),
        (("producers", "dispatcher", "opcodes", 1, "count"), 19),
        (("sheet", "highRows", 0, "wire"), "0x9559"),
        (("sheet", "lowRows", "count"), 399),
        (("corrections", 4, "with"), "wire >  0x8000: 200000 + wire - 0x4350"),
        (("source", "binarySha256"), "0" * 64),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x8a33b1"),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    # sub eax,ecx in place of add eax,ecx
    flipped = copy.deepcopy(document)
    for item in flipped["staticChecks"]:
        if item["name"] == "translation sequence":
            item["pattern"] = item["pattern"][:-5] + "2b c1"
    mutations.append(flipped)
    dropped = copy.deepcopy(document)
    dropped["staticChecks"] = [
        item
        for item in dropped["staticChecks"]
        if item["name"] != "translation sequence"
    ]
    mutations.append(dropped)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item, check_doc=False)
    ]


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return image_checks.verify_image(path.read_bytes(), document)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    args = parser.parse_args()
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if errors:
        print(f"FAIL: {len(errors)} status wire translation error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = " plus executable checks" if args.exe else ""
    print(f"PASS: status wire translation manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
