#!/usr/bin/env python3
"""Validate curated emote/animation evidence and optionally authenticate inputs.

The optional checks do not repeat the original LPB CFG reaching-definition
analysis, capture reconstruction, or native instruction review. No client code
is executed. Normal imports and validation require only the standard library.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import re
import struct
import sys
from pathlib import Path

ARTIFACT = Path(__file__).resolve().parents[1] / "config/emote_animation_catalog.json"
EMOTE_IDS = set(range(101, 150)) | set(range(151, 157))
SOURCE_VALUES_SHA256 = (
    "a3a1339d97b70ea257615c558df2f2892e6bbf337cbbcaf3db31c7397f402f62"
)
SOURCE_BINDINGS_SHA256 = (
    "cae6a341825e3f88dc848c212432c5be63401304caa941ee3be9d236ed0f6f34"
)
EMOTE_ROWS_SHA256 = "1b6a747cc28d920023c17df85beb0f224142d4693bcddbbefaa89e43b39c26b4"
# These are curated evidence projections, not regenerated runtime domains.
# Updating a section requires reviewing its pins, locators and qualifications.
REVIEWED_SECTIONS = {
    "inputs": "e2fc68639443d89676c602bf7787c31748b890156c73e1bcb1186e9fd263f13d",
    "evidence": "5d58e22fab6b19b9fd0ef7e502170c71a10982c4eacf58759cb1a29eab2ec152",
    "resources": "188aec2205fd1eb31af1d7b57e0937061d5923153983eb01368f2781be570c39",
    "capture_pairs": "9af81191b4e2b8df4db88fffc960322f975a0dfb4fb00d3df92c3d26b471cec3",
    "dynamic_standard_calls": "f23a599af3cb97db1a44e7668c96cb95c76bc53e0bf1f29b0e1cbc2eb3648880",
    "excluded_debug_standard_calls": "1187b563c948347e010eee938beccf882d33c50aad4e2d1eb6cb59a041c8ce0a",
}
EXPECTED_API_COUNTS = {
    "_runCharaScheduler": 5230,
    "_runCharaSchedulerAgainstTarget": 9,
    "_runCharaSchedulerTutorial": 44,
    "_runBgScheduler": 25,
    "_runBgSchedulerFromMidstream": 18,
    "_waitForCharaSchedulerFinished": 159,
    "_waitForCharaSchedulerTutorialFinished": 2,
    "_executeEmote": 1,
    "_cancelEmote": 8,
}
TIERS = {
    "script_literal": "STATIC_SOURCE_LITERAL",
    "branch_may_value": "STATIC_BRANCH_MAY_VALUE",
    "native_resource_route": "STATIC_RESOURCE_ROUTE",
    "capture": "OBSERVED_CAPTURE_PAIR",
    "resource_composition": "STATIC_RESOURCE_COMPOSITION",
}
CATEGORY_NAMES = (
    "cmn mgc sys etc lib emt em1 em2 em3 em4 kao glx gly cbi mgc pop cft btl "
    "wsc wss pic liu lin lif lil atk cmn cmn cmn cmn cmn cmn cmn wss"
).split()
CATEGORY_CHARACTER_FLAGS = (
    0,
    1,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
)
CATEGORY_EFFECT_FLAGS = (
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
)
CATEGORY_ROUTES = (
    0,
    9,
    0,
    0,
    1,
    7,
    6,
    6,
    6,
    6,
    8,
    10,
    11,
    0,
    12,
    13,
    0,
    0,
    0,
    0,
    0,
    2,
    3,
    4,
    5,
    17,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    16,
)


def digest(value: object) -> str:
    return hashlib.sha256(
        json.dumps(
            value, sort_keys=True, separators=(",", ":"), ensure_ascii=True
        ).encode()
    ).hexdigest()


def decode_selector(value: int) -> tuple[int, int, int]:
    """Decode the native 8/12/12 representation, including category 0x0B."""
    if type(value) is not int or not 0 <= value <= 0xFFFFFFFF:
        raise ValueError("selector must be a uint32")
    category = value >> 24
    if category == 0x0B:
        return category, 0, value & 0xFFFFFF
    return category, (value >> 12) & 0xFFF, value & 0xFFF


def encode_selector(category: int, character: int, effect: int) -> int:
    """Range-check fields; representability does not establish native validity."""
    if any(type(v) is not int for v in (category, character, effect)):
        raise ValueError("selector fields must be integers")
    if not 0 <= category <= 255:
        raise ValueError("category must fit 8 bits")
    if category == 0x0B:
        if character != 0 or not 0 <= effect <= 0xFFFFFF:
            raise ValueError("category 0x0B requires character 0 and a 24-bit effect")
        return (category << 24) | effect
    if not 0 <= character <= 0xFFF or not 0 <= effect <= 0xFFF:
        raise ValueError("ordinary banks must fit 12 bits")
    return (category << 24) | (character << 12) | effect


def _validate(document: dict, errors: list[str]) -> None:
    def require(condition: bool, message: str) -> None:
        if not condition:
            errors.append(message)

    require(document["schema_version"] == 1, "unsupported catalog schema")
    for key, expected in REVIEWED_SECTIONS.items():
        require(
            digest(document[key]) == expected,
            f"reviewed evidence projection drift: {key}",
        )
    coverage = document["coverage"]
    for key, expected in {
        "selector_domain": "PARTIAL",
        "nondebug_standard_calls": 5227,
        "direct_literal_standard_calls": 5209,
        "dynamic_standard_calls": 18,
        "selected_unique_selector_values": 228,
        "script_literal_unique_values": 211,
        "branch_added_unique_values": 17,
        "standing_bank_candidates": 55,
        "posture_witness_files": 10,
        "capture_pairs": 2,
        "native_emote_selector_joins": 0,
        "resource_composition_candidates": 2,
        "merged_selector_values": 243,
        "excluded_debug_standard_calls": 3,
    }.items():
        require(coverage[key] == expected, f"coverage drift: {key}")
    require(
        coverage["all_api_reference_counts"] == EXPECTED_API_COUNTS, "API census drift"
    )
    require(
        coverage["excluded_namespaces"]
        == {k: v for k, v in EXPECTED_API_COUNTS.items() if k != "_runCharaScheduler"},
        "excluded API namespace drift",
    )
    require(
        document["native_emote_boundary"]["status"] == "UNPROVEN"
        and document["native_emote_boundary"]["selector"] is None,
        "native emote join must remain unproven",
    )
    layout = document["selector_layout"]
    for key, value in {
        "width_bits": 32,
        "category_shift": 24,
        "category_width_bits": 8,
        "ordinary_character_shift": 12,
        "ordinary_character_width_bits": 12,
        "ordinary_effect_width_bits": 12,
        "exception_category": 11,
        "exception_character_value": 0,
        "exception_effect_width_bits": 24,
        "parser_va": "0x00798370",
        "consumer_va": "0x00798470",
    }.items():
        require(layout[key] == value, f"selector layout drift: {key}")

    emotes = document["emotes"]
    require(
        len(emotes) == 55 and {r["id"] for r in emotes} == EMOTE_IDS,
        "emote domain must equal 101..149,151..156; 150 is absent",
    )
    crosswalk = [
        {
            k: r[k]
            for k in (
                "id",
                "hex",
                "english_name",
                "enum_identifier",
                "text_command_id",
                "text_command_hex",
                "english_commands",
                "sources",
            )
        }
        for r in emotes
    ]
    require(
        digest(crosswalk) == EMOTE_ROWS_SHA256,
        "authenticated emote/name/text-command crosswalk drift",
    )
    require(
        len({r["text_command_id"] for r in emotes}) == 53,
        "text-command namespace must retain the shared GC salute key",
    )
    for row in emotes:
        require(
            row["native_selector_join"] == {"status": "UNPROVEN", "selector": None},
            f"emote {row['id']}: fabricated native selector join",
        )
        require(
            row["text_command_status"] == "STATIC_JOIN_RUNTIME_UNPROVEN",
            "text-command c3 join is static only",
        )
        require(
            row["observed_pair"] == {111: "dance", 119: "kneel"}.get(row["id"]),
            "only two bounded observed emote joins are supported",
        )
    require(
        [(r["id"], r["class_path"]) for r in document["system_commands"]]
        == [
            (24102, "/Command/System/EmoteStandardCommand"),
            (24312, "/Command/System/EmoteSitCommand"),
        ],
        "system commands are a separate namespace",
    )
    require(
        document["sit_variations"]
        == [
            {"id": 10001, "english_name": "Sit", "main_state": 13},
            {"id": 10002, "english_name": "Sit", "main_state": 11},
        ],
        "sit variation namespace drift",
    )
    require(
        document["sit_contract"]["map"] == {"10001": 13, "10002": 11, "nil": 0},
        "sit states are not selectors",
    )
    require(
        [r["category"] for r in document["text_categories"]] == [1120, 1040, 1041],
        "text category namespace drift",
    )
    require(
        all("unresolved" in r["sheet_resolution"] for r in document["text_categories"]),
        "native text resolver remains unresolved",
    )

    categories = document["categories"]
    require(
        len(categories) == 34 and {r["category"] for r in categories} == set(range(34)),
        "only 34 pointer-backed categories are valid",
    )
    for row in categories:
        index = row["category"]
        if type(index) is not int or not 0 <= index < 34:
            continue
        require(
            row["category_hex"] == f"0x{index:02X}", "category decimal/hex mismatch"
        )
        require(
            (
                row["name"],
                row["character_enabled"],
                row["effect_enabled"],
                row["route_kind"],
            )
            == (
                CATEGORY_NAMES[index],
                CATEGORY_CHARACTER_FLAGS[index],
                CATEGORY_EFFECT_FLAGS[index],
                CATEGORY_ROUTES[index],
            ),
            f"category {index}: native record drift",
        )
        require(
            row["enum_identifier"]
            == CATEGORY_NAMES[index].capitalize() + f"{index:02X}",
            "retain literal category labels with numeric disambiguation",
        )
        require(
            row["status"] == "pointer_backed_record" and row["reserved_halfword"] == 0,
            "category status drift",
        )
        require(
            row["record_va"] == f"0x{0x00FE32D8 + index * 12:08X}",
            "category record VA drift",
        )
    anomaly = document["category_accessor_anomaly"]
    require(
        anomaly["category"] == 34
        and anomaly["name_va"] == "0x74736163"
        and anomaly["name"] is None
        and anomaly["valid_category"] is False
        and anomaly["accepted_by_accessor"] is True,
        "accepted category 0x22 is an adjacent-literal anomaly",
    )
    require(
        (
            anomaly["record_va"],
            anomaly["character_enabled"],
            anomaly["effect_enabled"],
            anomaly["reserved_halfword"],
            anomaly["route_kind"],
            anomaly["status"],
        )
        == (
            "0x00FE3470",
            95,
            109,
            28271,
            3748191,
            "accepted_index_reads_adjacent_literal",
        ),
        "category 0x22 anomaly must retain the raw accessor interpretation",
    )

    pairs = document["capture_pairs"]
    require(
        [(p["emote_id"], p["selector"], p["status"]) for p in pairs]
        == [(111, 0x0500B000, "OBSERVED"), (119, 0x05013000, "OBSERVED")],
        "capture joins must remain the two observed pairs",
    )
    for pair in pairs:
        require(
            pair["request"]["direction"] == "c2s"
            and pair["playback"]["direction"] == "s2c",
            "capture direction-local locators required",
        )
        require(
            pair["request"]["emote_argument_offset"] == "0x41"
            and pair["playback"]["selector_application_offset"] == 0,
            "capture field namespace drift",
        )
        require(
            bool(pair["scope"]) and pair["selector_hex"] == f"0x{pair['selector']:08X}",
            "capture qualification/hex required",
        )

    resources = {r["id"]: r for r in document["resources"]}
    require(
        len(resources) == len(document["resources"]) == 65,
        "65 unique bounded resource witnesses required",
    )
    standing = {r["bank"] for r in resources.values() if r["posture_bank"] == "em1"}
    require(
        standing == set(range(1, 56)), "standing bank witness domain must equal 1..55"
    )
    evidence = document["evidence"]
    scripts = document["inputs"]["scripts"]
    for eid, item in evidence.items():
        require(item["origin"] in TIERS, f"{eid}: unknown evidence origin")
        if item["origin"] in ("script_literal", "branch_may_value"):
            require(
                item["api"] == "_runCharaScheduler",
                f"{eid}: separate API namespace cannot supply standard selectors",
            )
            require(
                item["script"] in scripts
                and item["source"].startswith(
                    f"xivl-client-scripts:lua/scripts/{item['script']}:"
                ),
                f"{eid}: caller source/pin mismatch",
            )
            if item["origin"] == "branch_may_value":
                require(
                    all(
                        k in item["original_bytecode"]
                        for k in (
                            "prototype",
                            "pc",
                            "offset",
                            "resource",
                            "sha256",
                            "decoded_sha256",
                        )
                    ),
                    f"{eid}: reviewed branch requires original LPB locator and pins",
                )

    selectors = document["selectors"]
    require(len(selectors) == 243, "merged selector coverage drift")
    require(
        len({r["value"] for r in selectors}) == len(selectors),
        "duplicate selector uint",
    )
    require(
        len({r["enum_identifier"] for r in selectors}) == len(selectors),
        "duplicate selector enum name",
    )
    source_values = sorted(
        r["value"]
        for r in selectors
        if set(r["origins"]) & {"script_literal", "branch_may_value"}
    )
    require(
        digest(source_values) == SOURCE_VALUES_SHA256,
        "228 authenticated source selector values changed",
    )
    # Evidence-pool pins alone do not bind a caller to its selector. Preserve the
    # reviewed per-value locator and caller-summary associations as one projection.
    source_bindings = []
    for row in sorted(selectors, key=lambda item: item["value"]):
        refs = sorted(
            ref
            for ref in row["evidence"]
            if evidence[ref]["origin"] in ("script_literal", "branch_may_value")
        )
        if refs:
            source_bindings.append(
                {
                    "value": row["value"],
                    "evidence": refs,
                    "source_callers": row["source_callers"],
                }
            )
    require(
        digest(source_bindings) == SOURCE_BINDINGS_SHA256,
        "source selector/caller binding drift",
    )
    for origin, count in [
        ("script_literal", 211),
        ("native_resource_route", 55),
        ("capture", 2),
        ("resource_composition", 2),
    ]:
        require(
            sum(origin in r["origins"] for r in selectors) == count,
            f"selector origin coverage drift: {origin}",
        )
    for row in selectors:
        value = row["value"]
        require(row["hex"] == f"0x{value:08X}", "selector decimal/hex mismatch")
        require(
            decode_selector(value)
            == (row["category"], row["character_field"], row["effect_field"]),
            "selector field decoding mismatch",
        )
        require(
            re.fullmatch(r"[A-Z][A-Za-z0-9]*", row["enum_identifier"]) is not None,
            "enum identifiers must be ASCII PascalCase",
        )
        origins = row["origins"]
        require(
            origins == sorted(set(origins)) and set(origins) <= TIERS.keys(),
            "selector origins must be unique and supported",
        )
        require(
            row["proof_tiers"] == [TIERS[o] for o in origins],
            "selector proof/status drift",
        )
        refs = row["evidence"]
        require(
            bool(refs)
            and len(set(refs)) == len(refs)
            and all(e in evidence for e in refs),
            "selector evidence must resolve uniquely",
        )
        require(
            {evidence[e]["origin"] for e in refs} == set(origins),
            "selector origin/evidence mismatch",
        )
        if "native_resource_route" in origins:
            require(
                row["category"] == 5 and row["effect_field"] == 0,
                "emote bank candidate requires category 5 and no inferred VFX",
            )
            witness = resources[f"em1-{row['character_field']:04}"]
            label = witness["entries"][0]["name"].rsplit("\\", 1)[-1]
            expected = "Emt" + "".join(
                t[:1].upper() + t[1:] for t in label[5:].split("_")
            )
            require(
                row["name_tier"] == "RESOURCE_LABEL"
                and row["enum_identifier"] == expected,
                "resource name is a qualified literal label",
            )
            require(
                "unproven" in row["meaning"],
                "resource label cannot prove visual semantics",
            )
            require(
                any(evidence[e].get("resource") == witness["id"] for e in refs),
                "resource witness does not support this selector",
            )
        elif "resource_composition" in origins:
            require(
                (value, row["enum_identifier"])
                in [
                    (0x05FA3065, "EmtEatWithBreadParent"),
                    (0x05FA2066, "EmtDrinkWithDrinkParent"),
                ],
                "only supported food/drink compositions admitted",
            )
            require(
                row["name_tier"] == "RESOURCE_COMPOSITION",
                "composition name qualification required",
            )
        else:
            require(
                row["name_tier"] == "UNKNOWN"
                and row["enum_identifier"] == f"Selector{value:08X}"
                and row["meaning"] == "Unknown presentation meaning",
                "source caller alone cannot prove a visual name",
            )
        if set(origins) & {"script_literal", "branch_may_value"}:
            require(
                row["source_callers"]["count"] > 0
                and re.fullmatch(r"[0-9a-f]{64}", row["source_callers"]["sha256"])
                is not None,
                "source caller count/digest required",
            )

    for key, count in [
        ("dynamic_standard_calls", 18),
        ("excluded_debug_standard_calls", 3),
    ]:
        calls = document[key]
        require(
            len(calls) == count and len({c["source"] for c in calls}) == count,
            f"{key}: retain every distinct call",
        )
        for call in calls:
            require(
                call["api"] == "_runCharaScheduler" and call["script"] in scripts,
                f"{key}: standard first-argument namespace required",
            )
            require(
                "unproven" in call["qualification"], f"{key}: may-value limits required"
            )
            require(
                all(
                    v == 0 or v in source_values
                    for v in call["numeric_reaching_candidates"]
                ),
                "dynamic may-value must belong to the source candidates; zero is excluded",
            )


def validate_artifact(document: object) -> list[str]:
    """Return catalog contract errors without retail inputs or optional packages."""
    errors: list[str] = []
    try:
        if not isinstance(document, dict):
            return ["catalog must be an object"]
        text = json.dumps(document, ensure_ascii=False)
        if not text.isascii() or re.search(
            r"[A-Za-z]:[\\/]|(?:^|[\\/])\.tmp[\\/]|xivl-agents", text
        ):
            errors.append(
                "catalog must contain only ASCII public evidence, without machine/private paths"
            )
        _validate(document, errors)
    except (KeyError, TypeError, ValueError, IndexError, AttributeError) as exc:
        errors.append(f"malformed catalog: {exc}")
    return errors


def emit_csharp(document: dict) -> str:
    """Emit three distinct enum namespaces; validate before calling."""
    lines = ["// Curated retail 1.23b evidence; AnimationSelector coverage is partial."]
    for name, underlying, key, value_key in [
        ("EmoteId", "ushort", "emotes", "hex"),
        ("AnimationCategory", "byte", "categories", "category_hex"),
        ("AnimationSelector", "uint", "selectors", "hex"),
    ]:
        lines.extend(["", f"public enum {name} : {underlying}", "{"])
        for row in document[key]:
            qualification = row.get("name_tier", "STATIC")
            lines.append(f"    // {qualification}")
            suffix = "u" if underlying == "uint" else ""
            lines.append(f"    {row['enum_identifier']} = {row[value_key]}{suffix},")
        lines.append("}")
    return "\n".join(lines) + "\n"


def authenticate(path: Path, pin: dict, canonical_lua: bool = False) -> bytes:
    data = path.read_bytes()
    if canonical_lua:
        data = data.replace(b"\r\n", b"\n")
    if ("bytes" in pin and len(data) != pin["bytes"]) or hashlib.sha256(
        data
    ).hexdigest() != pin["sha256"]:
        raise ValueError(f"{path.name}: input size or SHA-256 mismatch")
    return data


def verify_exe(document: dict, path: Path) -> None:
    try:
        from .build_achievement_title_map import Image
    except ImportError:
        from build_achievement_title_map import Image
    pin = document["inputs"]["binary"]
    image = Image(authenticate(path, {"bytes": pin["size"], "sha256": pin["sha256"]}))
    table = document["category_table"]
    raw = image.read(0x00FE32D8, 35 * 12)
    if hashlib.sha256(raw).hexdigest() != table["accepted_read_sha256"]:
        raise ValueError("accepted native table-read identity mismatch")
    for row in document["categories"]:
        name, character, effect, reserved, route = struct.unpack_from(
            "<IBBHI", raw, row["category"] * 12
        )
        expected = (
            int(row["name_va"], 16),
            row["character_enabled"],
            row["effect_enabled"],
            row["reserved_halfword"],
            row["route_kind"],
        )
        if (name, character, effect, reserved, route) != expected or image.read(
            name, len(row["name"]) + 1
        ) != row["name"].encode() + b"\0":
            raise ValueError("native pointer-backed category record mismatch")
    if raw[34 * 12 :] != b"cast_mon_19\0":
        raise ValueError("category 0x22 adjacent-literal mismatch")


def verify_csv(document: dict, root: Path) -> None:
    sheets = {}
    for name, pin in document["inputs"]["csv"].items():
        data = authenticate(root / name, pin)
        sheets[name] = {
            int(row[0]): row
            for row in list(csv.reader(io.StringIO(data.decode("utf-8-sig"))))[2:]
            if row
        }
    if (
        set(sheets["emote.csv"]) != EMOTE_IDS
        or set(sheets["xtx_emote.csv"]) != EMOTE_IDS
    ):
        raise ValueError("CSV emote domain mismatch")
    for row in document["emotes"]:
        key = row["id"]
        command = int(sheets["emote.csv"][key][4])
        aliases = list(
            dict.fromkeys(
                v for v in sheets["xtx__textCommand.csv"][command][10:13] if v
            )
        )
        if (command, sheets["xtx_emote.csv"][key][6], aliases) != (
            row["text_command_id"],
            row["english_name"],
            row["english_commands"],
        ):
            raise ValueError(f"emote {key}: CSV name/command crosswalk mismatch")
    for row in document["sit_variations"]:
        if sheets["xtx_command_variableemote.csv"][row["id"]][2] != row["english_name"]:
            raise ValueError("sit variation CSV mismatch")


def verify_scripts(document: dict, root: Path) -> None:
    for path, pin in document["inputs"]["scripts"].items():
        authenticate(root / path, pin, canonical_lua=True)


def verify_client(document: dict, root: Path) -> None:
    if (root / "game.ver").read_text(encoding="ascii").strip() != document[
        "extraction"
    ]:
        raise ValueError("client game.ver mismatch")
    verify_exe(document, root / "ffxivgame.exe")
    for row in document["resources"]:
        data = authenticate(root / row["path"], row)
        if data[:4] != b"PWIB":
            raise ValueError("bank witness is not PWIB")
        segment = struct.unpack_from(">I", data, 8)[0]
        if data[segment : segment + 8] != b"SEDBRES ":
            raise ValueError("bank witness has no RES segment")
        for entry in row["entries"]:
            payload = data[entry["offset"] : entry["offset"] + entry["bytes"]]
            if (
                len(payload) != entry["bytes"]
                or payload[:4] != b"SEDB"
                or payload[4:8].strip(b"\0 ") != entry["kind"].encode()
                or entry["name"].encode() + b"\0" not in data
            ):
                raise ValueError("typed resource witness mismatch")
    for item in document["evidence"].values():
        if item["origin"] == "resource_composition":
            for key in ("character", "effect"):
                authenticate(root / item[key]["path"], item[key])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", type=Path, default=ARTIFACT)
    parser.add_argument("--exe", type=Path, help="Explicit retail ffxivgame.exe")
    parser.add_argument("--csv-dir", type=Path, help="Explicit decoded CSV directory")
    parser.add_argument(
        "--scripts-root",
        type=Path,
        help="Explicit decoded Lua root; authenticates selected members, no CFG re-analysis",
    )
    parser.add_argument(
        "--client-root",
        type=Path,
        help="Explicit retail install root containing game.ver, ffxivgame.exe and client/",
    )
    parser.add_argument(
        "--emit-csharp",
        action="store_true",
        help="Write only C# enums to stdout on success",
    )
    args = parser.parse_args(argv)
    try:
        document = json.loads(args.artifact.read_text(encoding="ascii"))
        errors = validate_artifact(document)
        if errors:
            raise ValueError("; ".join(errors))
        checks = []
        for option, check, description in [
            (args.exe, verify_exe, "pinned executable and raw native table"),
            (args.csv_dir, verify_csv, "six pinned CSVs and emote crosswalk"),
            (args.scripts_root, verify_scripts, "selected canonical Lua identities"),
            (
                args.client_root,
                verify_client,
                "retail build and bounded resource witnesses",
            ),
        ]:
            if option is not None:
                check(document, option)
                checks.append(description)
    except (OSError, ValueError, KeyError, IndexError, struct.error) as exc:
        print(f"emote/animation catalog: {exc}", file=sys.stderr)
        return 1
    if args.emit_csharp:
        print(emit_csharp(document), end="")
    else:
        print(
            "emote/animation catalog: 55 emotes, 34 categories, 243 partial selectors validated"
        )
        if checks:
            print("Authenticated: " + "; ".join(checks))
            print(
                "Limits: no CFG re-analysis, capture reconstruction, native instruction review or client execution"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
