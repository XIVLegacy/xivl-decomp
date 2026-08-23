#!/usr/bin/env python3
"""Build the row-level external-source dependency ledger.

The ledger is deliberately self-contained. Cross-repository consumers are
recorded as audited facts, but this generator never opens a sibling checkout.
"""

from __future__ import annotations

import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ROOT / "config"
OUTPUT = CONFIG / "ffxivgame.external_dependencies.json"
VTABLE_GENERATED_PATH = "config/ffxivgame.vtable_method_names.json"

SOURCE_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
UNNAMED_SYMBOL = re.compile(r"^(FUN_|thunk_FUN_|SUB_|LAB_)")

DISPOSITIONS = {
    "independently-rederived": (
        "The complete row fact is reproduced by the self-sourced RTTI base "
        "with a binary locator, producing tool, and observation."
    ),
    "keep-with-citation": (
        "A live consumer still uses an imported fact that the self-sourced "
        "base does not completely establish. The row and attribution stay."
    ),
    "delete-with-source": (
        "No surviving consumer requires the row and the self-sourced base "
        "does not establish its imported semantic content."
    ),
}

CONSUMERS = [
    {
        "id": "decomp.apply-known-names",
        "repository": "xivl-decomp",
        "path": "tools/ghidra_scripts/ApplyKnownNames.java",
        "use": "Applies reviewed name rows to the local Ghidra program.",
    },
    {
        "id": "decomp.vtable-name-builder",
        "repository": "xivl-decomp",
        "path": "tools/build_vtable_method_names.py",
        "use": "Builds neutral vfunc names from the self-sourced RTTI and slot catalogs.",
    },
    {
        "id": "decomp.imported-prose",
        "repository": "xivl-decomp",
        "path": "docs/",
        "use": "Contains a surviving imported function locator or semantic name.",
    },
    {
        "id": "opcodes.decomp-anchors",
        "repository": "xivl-opcodes",
        "path": "opcodes.json",
        "use": "Retains 0x0135 as a blocked semantic candidate while citing the bare Ghidra sender anchor.",
        "audit_sha256": "5616b391e07ef2841c75696f451786d7be372a32cd289e2e9ae156d664361d04",
        "audit_path": "data/client_opcode_semantics.json",
        "audit_result": "The 37-row ledger closes 36 imported semantics; 0x0135 remains open only for binding-id and subscription-type meanings, while all 45 decompAnchor values are bare Ghidra symbols.",
    },
    {
        "id": "client-structs.promotions",
        "repository": "xivl-client-structs",
        "path": "manifests/",
        "use": "Contains promoted rows citing selected tracked xivl-decomp artifacts, excluding the imported CharaActor layout.",
        "audit_sha256": "205fce1dd956c0726538ee3c0c0eba2b405140bc263a2cc683fa57e5c882ba32",
        "audit_path": "manifests/actor_vtable5_drill.json",
        "audit_result": "The promotion re-derives CharaActor identity, constructor, vftable, and slot count while narrowing out size, fields, and subobject relationships.",
    },
]


def load(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def row_hash(row: dict) -> str:
    raw = json.dumps(row, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    return hashlib.sha256(raw.encode("ascii")).hexdigest()


def sanitise_class_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_")


def load_base():
    document = load("config/ffxivgame.rtti.json")
    classes = document["classes"]
    by_rva = {row["rva"]: row for row in classes}
    by_class = defaultdict(list)
    for row in classes:
        by_class[row["class"]].append(row)

    slots = []
    for line in (CONFIG / "ffxivgame.vtable_slots.jsonl").read_text(
            encoding="utf-8").splitlines():
        row = json.loads(line)
        if row.get("record_type") == "vtable_slot":
            slots.append(row)
    by_class_slot = defaultdict(list)
    fn_classes = defaultdict(set)
    for row in slots:
        by_class_slot[(row["class"], row["slot"])].append(row)
        fn_classes[row["fn_rva"]].add(row["class"])
    return document, by_rva, by_class, by_class_slot, fn_classes


def base_record(status: str, locator: dict | None, observation: str,
                independent_fields: list[str], agreement_fields: list[str],
                uncovered_fields: list[str]) -> dict:
    record = {
        "status": status,
        "binary": "ffxivgame.exe retail 1.23b",
        "source_sha256": SOURCE_SHA256,
        "producer": "tools/ghidra_scripts/DumpRtti.java under Ghidra 12.1",
        "observation": observation,
        "independently_rederived_fields": independent_fields,
        "agreement_only_fields": agreement_fields,
        "uncovered_imported_fields": uncovered_fields,
        "confidence": "direct structural observation for base fields only",
        "ambiguity": "Semantic names or roles outside RTTI class and slot structure remain imported.",
    }
    if locator:
        record["locator"] = locator
    return record


def make_row(catalog: str, ordinal: int, source: dict, disposition: str,
             consumers: list[str], blocking_consumers: list[str],
             evidence: dict, basis: str) -> dict:
    digest = row_hash(source)
    return {
        "row_id": f"{catalog}#sha256:{digest}",
        "source_ordinal": ordinal,
        "source_row_sha256": digest,
        "locator": {
            key: source[key]
            for key in ("rva_hex", "va_hex", "name", "class", "slot")
            if key in source
        },
        "disposition": disposition,
        "consumer_ids": sorted(set(consumers)),
        "blocking_consumer_ids": sorted(set(blocking_consumers)),
        "base_evidence": evidence,
        "basis": basis,
    }


def summarize(rows: list[dict]) -> dict:
    dispositions = Counter(row["disposition"] for row in rows)
    base_status = Counter(row["base_evidence"]["status"] for row in rows)
    eligibility = Counter(
        row["eligibility"]["classification"]
        for row in rows
        if "eligibility" in row
    )
    summary = {
        "rows": len(rows),
        "dispositions": dict(sorted(dispositions.items())),
        "base_status": dict(sorted(base_status.items())),
        "rows_with_direct_consumers": sum(bool(row["consumer_ids"]) for row in rows),
        "rows_blocked_from_deletion": sum(
            row["disposition"] == "keep-with-citation" and bool(row["blocking_consumer_ids"])
            for row in rows
        ),
    }
    if eligibility:
        summary["eligibility"] = dict(sorted(eligibility.items()))
    return summary


def catalog_record(path: str, source_project: str, rows: list[dict]) -> dict:
    return {
        "path": path,
        "source_project": source_project,
        "summary": summarize(rows),
        "rows": rows,
    }


def build_catalogs():
    _, _, base_by_class, base_by_class_slot, fn_classes = load_base()
    vtable_names = load(VTABLE_GENERATED_PATH)

    catalogs = []

    rows = []
    vtable_name_rvas = Counter(source["rva"] for source in vtable_names)
    base_candidate_rvas = Counter(
        slot["fn_rva"]
        for slot_rows in base_by_class_slot.values()
        for slot in slot_rows
        if (slot["class"] in base_by_class and
            len(fn_classes[slot["fn_rva"]]) == 1)
    )
    for index, source in enumerate(vtable_names):
        match = re.fullmatch(r"(.+)::vfunc(\d+)", source["name"])
        candidates = []
        class_name = ""
        slot_index = -1
        if match:
            class_name = match.group(1)
            slot_index = int(match.group(2))
            candidates = [
                slot for slot in base_by_class_slot.get((class_name, slot_index), [])
                if slot["fn_rva"] == source["rva"]
            ]
        base_selected = bool(candidates) and class_name in base_by_class
        base_symbol = candidates[0].get("fn_name", "") if len(candidates) == 1 else ""
        unnamed_target = bool(UNNAMED_SYMBOL.match(base_symbol))
        symbol_matches = source.get("current_symbol") == base_symbol
        unique_owner = len(fn_classes[source["rva"]]) == 1
        unique_rva = base_candidate_rvas[source["rva"]] == 1
        unique_output_rva = vtable_name_rvas[source["rva"]] == 1
        derived_shape = (
            source.get("rva_hex") == f"0x{source['rva']:08x}" and
            source.get("name") == f"{class_name}::vfunc{slot_index}" and
            source.get("source") == "vtable-slot"
        )
        full = all((
            base_selected, unnamed_target, symbol_matches, unique_owner,
            unique_rva, unique_output_rva, derived_shape,
        ))
        if not full:
            raise SystemExit(
                f"generated vtable row fails its own eligibility checks: {source}"
            )
        evidence = base_record(
            "full-independent-derivation",
            {
                "vtable_rvas": sorted({slot["vtable_rva_hex"] for slot in candidates}),
                "slot": slot_index,
                "fn_rva": source["rva_hex"],
                "current_symbol": base_symbol,
            },
            "The local base yields the class, slot, function RVA, unnamed target, unique owning class, and unique emitted RVA used to form the neutral Class::vfuncN label.",
            ["rva", "rva_hex", "name", "source", "current_symbol"], [], [],
        )
        disposition = "independently-rederived"
        basis = "The complete neutral vfunc fact is reproducible from the tracked RTTI and slot base without imported vocabulary."
        row = make_row(
            "ffxivgame.vtable_method_names", index, source, disposition,
            ["decomp.apply-known-names"],
            ["decomp.apply-known-names"],
            evidence, basis,
        )
        row["eligibility"] = {
            "classification": "selected-by-generator",
            "rejection_reasons": [],
            "deciding_inputs": [
                "config/ffxivgame.rtti.json",
                "config/ffxivgame.vtable_slots.jsonl",
            ],
        }
        rows.append(row)
    vtable_catalog = catalog_record(
        VTABLE_GENERATED_PATH, "xivl-decomp", rows
    )
    catalogs.append(vtable_catalog)

    return catalogs


def main() -> int:
    catalogs = build_catalogs()

    all_rows = [row for catalog in catalogs for row in catalog["rows"]]
    catalog_dispositions = Counter(row["disposition"] for row in all_rows)
    base_statuses = Counter(row["base_evidence"]["status"] for row in all_rows)
    vtable_eligibility = Counter(
        row["eligibility"]["classification"]
        for row in all_rows
        if "eligibility" in row
    )

    document = {
        "schema_version": 1,
        "binary": "ffxivgame.exe retail 1.23b",
        "source_sha256": SOURCE_SHA256,
        "producer": "tools/build_external_dependency_ledger.py",
        "policy": {
            "row_granularity": "A disposition covers the complete source row, not selected matching fields.",
            "cross_check_rule": "Address, RTTI-name, vtable, size, or offset agreement alone never adopts an imported semantic claim.",
            "consumer_rule": "Only a surviving semantic consumer blocks deletion; a producer, index entry, or artifact already marked delete-with-source does not.",
            "dispositions": DISPOSITIONS,
        },
        "inventory_audit": {
            "plan_data_files": 45,
            "public_data_files": 13,
            "private_maintainer_records_verified_read_only": 6,
            "private_record_paths_in_tracked_ledger": 0,
            "missing_plan_files": [],
            "unexpected_public_data_files": [],
            "catalog_files": 1,
            "generated_layout_artifacts": 0,
            "imported_prose_files": 5,
            "imported_header_sections": 0,
            "downstream_constant_files": 0,
        },
        "consumer_registry": CONSUMERS,
        "summary": {
            "catalog_rows": len(all_rows),
            "catalog_dispositions": dict(sorted(catalog_dispositions.items())),
            "base_status": dict(sorted(base_statuses.items())),
            "vtable_name_eligibility": dict(sorted(vtable_eligibility.items())),
            "rows_blocked_from_deletion": sum(
                row["disposition"] == "keep-with-citation" and bool(row["blocking_consumer_ids"])
                for row in all_rows
            ),
        },
        "catalogs": catalogs,
    }
    OUTPUT.write_text(
        json.dumps(document, indent=1, ensure_ascii=True) + "\n",
        encoding="ascii", newline="\n",
    )
    print(f"wrote {OUTPUT.relative_to(ROOT)}: {len(all_rows)} catalog rows, "
          f"{OUTPUT.stat().st_size} bytes")
    for catalog in catalogs:
        counts = catalog["summary"]["dispositions"]
        print(f"  {catalog['path']}: {counts}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
