#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Build neutral vtable method labels from the self-sourced RTTI base.

The only inputs are ``config/<binary>.rtti.json`` and
``config/<binary>.vtable_slots.jsonl``. A row is emitted when its class occurs
in the RTTI catalog, its clean auto-analysis name starts with ``FUN_``,
``SUB_``, ``LAB_``, or ``thunk_FUN_``, its target belongs to exactly one
distinct class, and its RVA occurs exactly once among those candidates.
"""

from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ROOT / "config"
UNNAMED_SYMBOL = re.compile(r"^(?:FUN_|SUB_|LAB_|thunk_FUN_)")


def load_slots(path: Path) -> list[dict]:
    slots = []
    for line in path.read_text(encoding="utf-8").splitlines():
        row = json.loads(line)
        if row.get("record_type") == "vtable_slot":
            slots.append(row)
    return slots


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", nargs="?", default="ffxivgame")
    args = parser.parse_args()
    binary = args.binary

    rtti_path = CONFIG / f"{binary}.rtti.json"
    slots_path = CONFIG / f"{binary}.vtable_slots.jsonl"
    output_path = CONFIG / f"{binary}.vtable_method_names.json"

    rtti = json.loads(rtti_path.read_text(encoding="utf-8"))
    classes = {row["class"] for row in rtti["classes"]}
    slots = load_slots(slots_path)

    fn_classes: dict[int, set[str]] = defaultdict(set)
    for row in slots:
        fn_classes[row["fn_rva"]].add(row["class"])

    class_owner_candidates = [
        row for row in slots
        if row["class"] in classes and len(fn_classes[row["fn_rva"]]) == 1
    ]
    candidate_rvas = Counter(row["fn_rva"] for row in class_owner_candidates)
    unique_rva_candidates = [
        row for row in class_owner_candidates
        if candidate_rvas[row["fn_rva"]] == 1
    ]
    named_candidates = [
        row for row in unique_rva_candidates
        if UNNAMED_SYMBOL.match(row.get("fn_name", ""))
    ]

    output = [
        {
            "rva": row["fn_rva"],
            "rva_hex": f"0x{row['fn_rva']:08x}",
            "name": f"{row['class']}::vfunc{row['slot']}",
            "source": "vtable-slot",
            "current_symbol": row["fn_name"],
        }
        for row in named_candidates
    ]
    output.sort(key=lambda row: row["rva"])
    output_path.write_text(
        json.dumps(output, indent=2) + "\n", encoding="utf-8", newline="\n"
    )

    print(f"vtable-method-names [{binary}]:")
    print(f"  slots: {len(slots)}")
    print(f"  RTTI classes: {len(classes)}")
    print(f"  unique-RVA candidates before naming: {len(unique_rva_candidates)}")
    print(f"  unnamed candidates emitted: {len(output)}")
    print(f"  wrote {output_path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
