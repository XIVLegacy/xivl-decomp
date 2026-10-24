#!/usr/bin/env python3
"""Derive completion ordinals for every row of the pinned achievement sheet."""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path

try:
    from . import build_achievement_title_map as native
except ImportError:
    import build_achievement_title_map as native

SHEET_IDS_SHA256 = "5a4ae783e64738156ac19bd6a6c7106104c391edb2161416dd0f9258eb91a193"
TITLE_SHA256 = "d5c658fce09240b30ca35e3545309a8571414292ffc09b8545a88682426a5b1d"
UNPROVEN_IDS = (1107, 1108, 1109, 1110, 1221)
FUNCTION_NAMES = ("achievement_to_ordinal", "ordinal_to_achievement")
TITLE_ARTIFACT = (
    Path(__file__).resolve().parents[1] / "config/achievement_title_map.json"
)


def sheet_ids(data: bytes) -> list[int]:
    native.title_links(data)
    rows = list(csv.reader(io.StringIO(data.decode("utf-8-sig"))))
    return sorted(int(row[0]) for row in rows[2:])


def ids_digest(ids: list[int]) -> str:
    return hashlib.sha256(
        json.dumps(ids, separators=(",", ":")).encode("ascii")
    ).hexdigest()


def input_metadata() -> dict:
    return {
        "ffxivgame.exe": {
            "bytes": native.EXE_BYTES,
            "sha256": native.EXE_SHA256,
            "image_base": "0x00400000",
        },
        "achievement.csv": {
            "bytes": native.CSV_BYTES,
            "sha256": native.CSV_SHA256,
            "source": "xivl-client-data:csv/achievement.csv",
            "manifest": "xivl-client-data:manifests/tables.json",
            "revision": "492c0e2701747bf63e91cb7ce419e9743f63c3f5",
        },
        "achievement_title_map.json": {
            "source": "config/achievement_title_map.json",
            "sha256": TITLE_SHA256,
            "revision": "9c319448e84c1bfd2ee3aa3509d35a61161f89ed",
            "use": "Independent agreement check for all 208 title-linked rows.",
        },
    }


def method_metadata() -> dict:
    return {
        "decoder": "Capstone 5.0.7, x86-32",
        "evaluation": "File-backed instruction and lookup-table evaluation for every sheet achievement ID; no target execution.",
        "functions": {
            name: {
                "entry_va": native.va_text(native.FUNCTIONS[name][0]),
                "code_end_exclusive_va": native.va_text(native.FUNCTIONS[name][1]),
                "lookup_end_exclusive_va": native.va_text(native.FUNCTIONS[name][2]),
            }
            for name in FUNCTION_NAMES
        },
        "completion": "An ordinal is proven only when <1024 and inverse(ordinal) equals that achievement. Otherwise ordinal is null and native evidence is retained.",
        "default_zero": {
            "entry_va": "0x0070BCB1",
            "result_va": "0x0070BCB3",
            "meaning": "Native zero is ambiguous: inverse(0) returns achievement 100, which owns ordinal 0.",
        },
        "coverage": "All 746 unique row IDs, sorted numerically. Neither sheet order nor reward values supply an ordinal.",
        "sheet_ids_sha256": SHEET_IDS_SHA256,
        "sheet_ids_encoding": "ASCII JSON array of sorted integer IDs with separators comma and colon, without whitespace or newline.",
    }


def derive(image: native.Image, achievements: list[int]) -> dict:
    mapping = native.NativeMappings(image)
    rows = [
        {"achievement_id": achievement, **native.completion(mapping, achievement)}
        for achievement in sorted(achievements)
    ]
    return {
        "format": "xivl-achievement-completion-ordinals-v1",
        "producer": "tools/build_achievement_completion_ordinals.py",
        "inputs": input_metadata(),
        "method": method_metadata(),
        "summary": {
            "sheet_rows": len(rows),
            "proven_ordinals": sum(row["status"] == "proven" for row in rows),
            "unproven_achievement_ids": [
                row["achievement_id"] for row in rows if row["status"] != "proven"
            ],
            "title_linked_rows": 208,
        },
        "rows": rows,
    }


def validate_artifact(document: dict, title_map: dict | None = None) -> list[str]:
    """Check complete coverage and native evidence without retail inputs or Capstone."""
    if not isinstance(document, dict):
        return ["artifact must be an object"]
    errors = []
    if document.get("format") != "xivl-achievement-completion-ordinals-v1":
        errors.append("artifact format differs")
    if document.get("producer") != "tools/build_achievement_completion_ordinals.py":
        errors.append("artifact producer differs")
    if document.get("inputs") != input_metadata():
        errors.append("input identities differ")
    if document.get("method") != method_metadata():
        errors.append("native derivation method or locators differ")
    if document.get("summary") != {
        "sheet_rows": 746,
        "proven_ordinals": 741,
        "unproven_achievement_ids": list(UNPROVEN_IDS),
        "title_linked_rows": 208,
    }:
        errors.append("complete-sheet summary differs")
    rows = document.get("rows")
    if not isinstance(rows, list) or len(rows) != 746:
        return errors + ["expected exactly 746 sheet rows"]

    def valid_locator(value, name: str) -> bool:
        if not isinstance(value, str):
            return False
        try:
            address = int(value, 16)
        except ValueError:
            return False
        start, end, _ = native.FUNCTIONS[name]
        return value == native.va_text(address) and start <= address < end

    by_id = {}
    ordinals = set()
    for row in rows:
        if not isinstance(row, dict):
            errors.append("row must be an object")
            continue
        achievement = row.get("achievement_id")
        if (
            type(achievement) is not int
            or not 0 <= achievement <= 0xFFFFFFFF
            or achievement in by_id
        ):
            errors.append("invalid or duplicate achievement ID")
            continue
        by_id[achievement] = row
        if not valid_locator(
            row.get("result_va"), "achievement_to_ordinal"
        ) or not valid_locator(row.get("inverse_result_va"), "ordinal_to_achievement"):
            errors.append(f"{achievement}: native result locator differs")
        if achievement in UNPROVEN_IDS:
            if row != {
                "achievement_id": achievement,
                "status": "unproven_inverse_mismatch",
                "ordinal": None,
                "native_result": 0,
                "result_va": "0x0070BCB3",
                "inverse_achievement_id": 100,
                "inverse_result_va": "0x0057ABCB",
            }:
                errors.append(f"{achievement}: unproven default-zero evidence differs")
            continue
        ordinal = row.get("ordinal")
        if (
            type(ordinal) is not int
            or not 0 <= ordinal < 1024
            or row.get("status") != "proven"
            or type(row.get("native_result")) is not int
            or row.get("native_result") != ordinal
            or type(row.get("inverse_achievement_id")) is not int
            or row.get("inverse_achievement_id") != achievement
        ):
            errors.append(f"{achievement}: completion is not independently proven")
        elif ordinal in ordinals:
            errors.append(f"{achievement}: duplicate completion ordinal")
        else:
            ordinals.add(ordinal)
    ids = list(by_id)
    if ids != sorted(ids) or ids_digest(sorted(ids)) != SHEET_IDS_SHA256:
        errors.append("sheet achievement ID coverage differs")
    if len(ordinals) != 741:
        errors.append("expected 741 unique proven ordinals")
    for achievement, expected in {
        100: 0,
        197: 18,
        198: 19,
        390: 146,
        391: 147,
        392: 148,
    }.items():
        if by_id.get(achievement, {}).get("ordinal") != expected:
            errors.append(f"{achievement}: known ordinal {expected} differs")
    if title_map is not None:
        if native.validate_artifact(title_map):
            errors.append("title map integrity differs")
        elif any(
            {
                key: value
                for key, value in by_id.get(row["achievement_id"], {}).items()
                if key != "achievement_id"
            }
            != row["direct_completion"]
            for row in title_map["rows"]
        ):
            errors.append("title-linked completion subset differs")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--achievement-csv", type=Path, required=True)
    destination = parser.add_mutually_exclusive_group(required=True)
    destination.add_argument("--output", type=Path)
    destination.add_argument("--check", type=Path)
    args = parser.parse_args()
    try:
        image = native.Image(
            native.pinned_bytes(args.exe, native.EXE_SHA256, native.EXE_BYTES)
        )
        achievements = sheet_ids(
            native.pinned_bytes(
                args.achievement_csv, native.CSV_SHA256, native.CSV_BYTES
            )
        )
        title_map = json.loads(native.pinned_bytes(TITLE_ARTIFACT, TITLE_SHA256))
        document = derive(image, achievements)
        errors = validate_artifact(document, title_map)
        if errors:
            raise ValueError("; ".join(errors))
        rendered = native.serialize(document).encode("ascii")
        if args.check:
            if args.check.read_bytes() != rendered:
                raise ValueError(
                    "achievement completion ordinals differ from exact-build derivation"
                )
            print(
                "Achievement completion ordinals: PASS (746 rows, 741 proven, 5 unproven)"
            )
        else:
            target = args.output.resolve()
            sources = (args.exe, args.achievement_csv, TITLE_ARTIFACT)
            if any(
                target == source.resolve()
                or (target.exists() and target.samefile(source))
                for source in sources
            ):
                raise ValueError("output must not replace an input")
            args.output.write_bytes(rendered)
    except (OSError, ValueError, KeyError, struct.error, ImportError) as error:
        parser.exit(1, f"ERROR: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
