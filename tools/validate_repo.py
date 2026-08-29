#!/usr/bin/env python3
"""Validate the public repository boundary and initial import manifest."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from pathlib import Path
from urllib.parse import unquote

import _schema_check
import verify_lobby_assigned_connection_u32 as lobby_assigned_u32_verifier
import verify_lobby_acknowledgement_consumer as lobby_ack_verifier
import verify_lobby_clear_0007_0008_consumers as lobby_clear_verifier
import verify_retail_protocol_caller as retail_verifier
import verify_s2c_018d_client_consumer as s2c_018d_consumer_verifier
import verify_s2c_0190_persistent_consumer as s2c_0190_consumer_verifier
import verify_s2c_0193_native_state as s2c_0193_state_verifier

ROOT = Path(__file__).resolve().parent.parent
PERMITTED_TOP_LEVEL_GROUPS = {
    "root",
    ".github",
    "config",
    "docs",
    "include",
    "schemas",
    "tools",
}
EXPECTED_BLOBS = {
    "LICENSE": "29ebfa545f5580919a4e884d7014d7a3eb2df762",
}
REQUIRED_AGENT_TOOLING_IGNORE_LINES = {
    "# Agent / AI tooling",
    ".claude/",
    ".agents/",
    "AGENTS.md",
    "CLAUDE.md",
    "docs/ai_agents/local/",
}
EXPECTED_RTTI_SOURCE_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
EXPECTED_RTTI_CLASSES = 5719
EXPECTED_LARGE_ARTIFACT_SHA256 = {
    "config/ffxivgame.vtable_slots.jsonl": "b776f19827f3002b6fc7fd522812f23d851b9a6065d47620e54f01bd0ae5732f",
    "config/ffxivgame.vtable_method_names.json": "bc009641d6a5debdc4cabfce8acc9d1e74f0445b14cf7596b81abc3513c7a0f5",
    "config/ffxivgame.external_dependencies.json": "c7b9f65e54abeb98eaa5a52eb1cf26405405f98ad1294f757a92c25e6dc8d3ef",
}
FORBIDDEN_SUFFIXES = {
    ".exe", ".dll", ".pdb", ".obj", ".o", ".lib", ".exp", ".ilk",
    ".map", ".dat", ".idx", ".index", ".index2",
}
FORBIDDEN_PREFIXES = (
    "src/", "asm/", ".claude/", ".agents/", "docs/ai_agents/local/",
)
TEXT_SUFFIXES = {".md", ".py", ".ps1", ".sh", ".java", ".h", ".txt", ".yml", ".yaml", ".jsonl"}
LINK_RE = re.compile(r"(?<!!)\[[^]]*\]\(([^)]+)\)")
INLINE_CODE_RE = re.compile(r"(?<!`)`([^`\r\n]+)`(?!`)")
def git_paths() -> list[str]:
    command = ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"]
    result = subprocess.run(command, cwd=ROOT, check=True, capture_output=True)
    return sorted(p for p in result.stdout.decode("utf-8").split("\0") if p)


def git_tracked_paths() -> list[str]:
    command = ["git", "ls-files", "-z", "--cached"]
    result = subprocess.run(command, cwd=ROOT, check=True, capture_output=True)
    return sorted(p for p in result.stdout.decode("utf-8").split("\0") if p)


def git_blob(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def canonical_row_sha256(row: dict) -> str:
    raw = json.dumps(
        row, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("ascii")
    return hashlib.sha256(raw).hexdigest()


def validate_precontract_derivation(
    document: dict, path: str, expected_scripts: list[str], errors: list[str]
) -> None:
    run = document.get("derivation_run", {})
    required_run_fields = {
        "contract_version", "contract_status", "scope", "binary_sha256",
        "ghidra_version", "scripts", "historical_arguments",
        "historical_output_sha256", "unknown", "regeneration",
    }
    if (
        not required_run_fields <= set(run)
        or run.get("contract_version") != 1
        or run.get("contract_status") != "predates-contract"
        or run.get("binary_sha256") != EXPECTED_RTTI_SOURCE_SHA256
        or run.get("ghidra_version") != "12.1"
        or run["historical_arguments"] is not None
        or run["historical_output_sha256"] is not None
    ):
        errors.append(f"pre-contract derivation metadata mismatch: {path}")
    scripts = run.get("scripts", [])
    clean_scripts = (
        isinstance(scripts, list)
        and all(isinstance(row, dict) for row in scripts)
    )
    if not clean_scripts or [row.get("path") for row in scripts] != expected_scripts:
        errors.append(f"pre-contract script inventory mismatch: {path}")
    if clean_scripts and any(
            set(row) != {"path", "historical_sha256"}
            or row["historical_sha256"] is not None
            for row in scripts):
        errors.append(f"pre-contract script hash was invented: {path}")
    unknown = run.get("unknown", [])
    regeneration = run.get("regeneration", [])
    if (
        not isinstance(run.get("scope"), str)
        or not run["scope"]
        or not isinstance(unknown, list)
        or not unknown
        or any(not isinstance(item, str) or not item for item in unknown)
        or not isinstance(regeneration, list)
        or not regeneration
        or any(
            not isinstance(item, dict)
            or not item.get("script")
            or not item.get("argument_source")
            or not ({"environment", "arguments"} & set(item))
            for item in regeneration
        )
    ):
        errors.append(f"incomplete pre-contract regeneration record: {path}")


def is_forbidden_asset(path: str) -> bool:
    lower = path.lower()
    suffix = Path(lower).suffix
    return suffix in FORBIDDEN_SUFFIXES or bool(re.search(r"\.dat[0-9]$", lower)) or ".win32.dat" in lower


def markdown_prose_lines(text: str):
    fenced = False
    for line_number, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith(("```", "~~~")):
            fenced = not fenced
            continue
        if not fenced:
            yield line_number, line


def inline_repository_path(raw: str, prefixes: set[str]) -> str | None:
    candidate = unquote(raw.strip()).replace("\\", "/")
    if (
        not candidate
        or re.match(r"^(?:[a-z]+:|/|~)", candidate, re.I)
        or any(char in candidate for char in " <>*{}$|")
    ):
        return None
    candidate = candidate.split("#", 1)[0].rstrip("/")
    if not candidate or "/" not in candidate:
        return None
    if candidate.split("/", 1)[0] not in prefixes:
        return None
    return candidate


def main() -> int:
    errors: list[str] = []
    paths = git_paths()
    tracked_paths = git_tracked_paths()

    for path in paths:
        group = path.split("/", 1)[0] if "/" in path else "root"
        if group not in PERMITTED_TOP_LEVEL_GROUPS:
            errors.append(f"unexpected top-level tracked group: {path}")

    for path in paths:
        lower = path.lower()
        if lower.startswith(FORBIDDEN_PREFIXES) or path in {"AGENTS.md", "CLAUDE.md"}:
            errors.append(f"forbidden tracked path: {path}")
        full = ROOT / path
        if is_forbidden_asset(path):
            errors.append(f"forbidden binary or asset extension: {path}")
        if full.read_bytes()[:2] == b"MZ":
            errors.append(f"PE MZ magic in tracked file: {path}")
        if path.endswith(".json"):
            try:
                json.loads(full.read_text(encoding="utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                errors.append(f"invalid tracked JSON {path}: {exc}")

    docs_tree = {
        p
        for p in paths
        if p.startswith("docs/") and p.endswith(".md") and p != "docs/README.md"
    }
    index_text = (ROOT / "docs/README.md").read_text(encoding="utf-8")
    index_paths = set()
    for target in LINK_RE.findall(index_text):
        target = target.split()[0].strip("<>").split("#", 1)[0]
        if target and not re.match(r"^[a-z]+://", target, re.I):
            index_paths.add("docs/" + unquote(target).replace("\\", "/"))
    if docs_tree != index_paths:
        for missing in sorted(docs_tree - index_paths):
            errors.append(f"docs index missing: {missing}")
        for extra in sorted(index_paths - docs_tree):
            errors.append(f"docs index extra: {extra}")

    published_prefixes = {
        path.split("/", 1)[0] for path in tracked_paths if "/" in path
    }
    for path in tracked_paths:
        if not path.endswith(".md"):
            continue
        full = ROOT / path
        text = full.read_text(encoding="utf-8")
        for line_number, line in markdown_prose_lines(text):
            link_line = INLINE_CODE_RE.sub("", line)
            for match in LINK_RE.finditer(link_line):
                raw = match.group(1)
                target = raw.strip().strip("<>")
                if re.match(r"^(?:[a-z]+:|#)", target, re.I):
                    continue
                target = target.split()[0].split("#", 1)[0]
                if not target:
                    continue
                resolved = (full.parent / unquote(target)).resolve()
                try:
                    resolved.relative_to(ROOT)
                except ValueError:
                    errors.append(
                        f"relative link escapes repository: {path}:{line_number} -> {raw}"
                    )
                    continue
                if not resolved.exists():
                    errors.append(
                        f"unresolved relative link: {path}:{line_number} -> {raw}"
                    )
            for match in INLINE_CODE_RE.finditer(line):
                raw = match.group(1)
                target = inline_repository_path(raw, published_prefixes)
                if target is not None and not (ROOT / target).exists():
                    errors.append(
                        f"unresolved repository path: {path}:{line_number} -> {raw}"
                    )

    for path, expected in EXPECTED_BLOBS.items():
        actual = git_blob((ROOT / path).read_bytes())
        if actual != expected:
            errors.append(f"immutable blob {path}: expected {expected}, got {actual}")

    for path, expected in EXPECTED_LARGE_ARTIFACT_SHA256.items():
        full = ROOT / path
        if not full.is_file():
            errors.append(f"missing large artifact: {path}")
            continue
        actual = hashlib.sha256(full.read_bytes()).hexdigest()
        if actual != expected:
            errors.append(
                f"large artifact hash mismatch: {path}: expected {expected}, got {actual}"
            )

    ignore_text = (ROOT / ".gitignore").read_text(encoding="utf-8").replace("\r\n", "\n")
    ignore_lines = set(ignore_text.split("\n"))
    for required in sorted(REQUIRED_AGENT_TOOLING_IGNORE_LINES):
        if required not in ignore_lines:
            errors.append(f".gitignore missing required line: {required}")

    symbol_evidence = json.loads(
        (ROOT / "config/ffxivgame.symbol_evidence.json").read_text(encoding="ascii")
    )
    symbol_observations = symbol_evidence.get("observations", [])
    symbol_rvas = [row.get("rva") for row in symbol_observations]
    if (symbol_evidence.get("schema_version") != 1 or
            symbol_evidence.get("source_sha256") != EXPECTED_RTTI_SOURCE_SHA256 or
            symbol_evidence.get("ghidra_version") != "12.1"):
        errors.append("symbol-evidence metadata mismatch")
    if symbol_rvas != sorted(set(symbol_rvas)):
        errors.append("symbol-evidence rows are not uniquely ordered by RVA")
    required_symbol_fields = {
        "rva", "rva_hex", "va_hex", "name", "kind", "evidence_class",
        "producer", "observation", "supported_imported_fields",
        "confidence", "ambiguity",
    }
    if any(not required_symbol_fields <= set(row) for row in symbol_observations):
        errors.append("symbol-evidence row lacks required fields")
    validate_precontract_derivation(
        symbol_evidence,
        "config/ffxivgame.symbol_evidence.json",
        [
            "tools/ghidra_scripts/DumpRtti.java",
            "tools/ghidra_scripts/DecompileToText.java",
            "tools/ghidra_scripts/FindBytes.java",
        ],
        errors,
    )

    protocol_evidence = json.loads(
        (ROOT / "config/ffxivgame.protocol_evidence.json").read_text(encoding="ascii")
    )
    if (
        protocol_evidence.get("schema_version") != 1
        or protocol_evidence.get("source_sha256") != EXPECTED_RTTI_SOURCE_SHA256
        or protocol_evidence.get("ghidra_version") != "12.1"
        or not protocol_evidence.get("observations")
        or not protocol_evidence.get("lua_binding_name_review")
    ):
        errors.append("protocol-evidence metadata mismatch")
    validate_precontract_derivation(
        protocol_evidence,
        "config/ffxivgame.protocol_evidence.json",
        [
            "tools/ghidra_scripts/DecompileToText.java",
            "tools/ghidra_scripts/FindCallers.java",
            "tools/ghidra_scripts/DumpStrings.java",
            "tools/decode_lpb.py",
            "tools/extract_cpp_bindings.py",
        ],
        errors,
    )

    lobby_ack_errors = lobby_ack_verifier.verify()
    if lobby_ack_errors:
        errors.extend(
            f"lobby acknowledgement contract: {error}"
            for error in lobby_ack_errors
        )

    lobby_assigned_u32_errors = lobby_assigned_u32_verifier.verify()
    if lobby_assigned_u32_errors:
        errors.extend(
            f"lobby assigned-u32 contract: {error}"
            for error in lobby_assigned_u32_errors
        )

    lobby_clear_errors = lobby_clear_verifier.verify()
    if lobby_clear_errors:
        errors.extend(
            f"lobby clear type-7/8 contract: {error}"
            for error in lobby_clear_errors
        )

    retail_errors = retail_verifier.verify()
    if retail_errors:
        errors.extend(f"retail protocol-caller contract: {error}"
                      for error in retail_errors)
    s2c_018d_errors = s2c_018d_consumer_verifier.verify()
    if s2c_018d_errors:
        errors.extend(
            f"s2c 0x018D client-consumer manifest: {error}"
            for error in s2c_018d_errors
        )

    s2c_0190_errors = s2c_0190_consumer_verifier.verify()
    if s2c_0190_errors:
        errors.extend(
            f"s2c 0x0190 persistent-consumer contract: {error}"
            for error in s2c_0190_errors
        )

    s2c_0193_errors = s2c_0193_state_verifier.verify()
    if s2c_0193_errors:
        errors.extend(
            f"s2c 0x0193 native-state contract: {error}"
            for error in s2c_0193_errors
        )
    try:
        attestation_schema = _schema_check.load_schema(
            ROOT / "schemas/retail-evidence-attestation.schema.json"
        )
    except (OSError, ValueError, _schema_check.SchemaError) as exc:
        errors.append(f"retail attestation schema is invalid: {exc}")
        attestation_schema = None
    if attestation_schema is not None:
        for status in ("pass", "fail"):
            sample = retail_verifier.build_attestation(status, "0" * 40)
            for problem in _schema_check.validate(sample, attestation_schema):
                errors.append(f"retail {status} attestation: {problem}")
        evidence_root = ROOT / "config/retail_evidence"
        if evidence_root.exists():
            expected_name = "protocol-0x0135-single-direct-caller.json"
            evidence_files = sorted(
                path for path in evidence_root.iterdir() if path.is_file()
            )
            if [path.name for path in evidence_files] != [expected_name]:
                errors.append("retail evidence file allowlist differs")
            for path in evidence_files:
                try:
                    document = json.loads(path.read_text(encoding="ascii"))
                except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                    errors.append(f"invalid retail evidence {path.name}: {exc}")
                    continue
                for problem in _schema_check.validate(document, attestation_schema):
                    errors.append(f"retail evidence {path.name}: {problem}")
                if document.get("result") != {"status": "pass"}:
                    errors.append(f"retail evidence {path.name} is not a pass")

    struct_evidence = json.loads(
        (ROOT / "config/ffxivgame.struct_evidence.json").read_text(encoding="ascii")
    )
    if (
        struct_evidence.get("schema_version") != 1
        or struct_evidence.get("source_sha256") != EXPECTED_RTTI_SOURCE_SHA256
        or "Ghidra 12.1" not in struct_evidence.get("producer", "")
        or not struct_evidence.get("class_write_sites")
        or not struct_evidence.get("imported_constructor_candidates")
    ):
        errors.append("struct-evidence metadata mismatch")
    validate_precontract_derivation(
        struct_evidence,
        "config/ffxivgame.struct_evidence.json",
        [
            "tools/ghidra_scripts/DecompileToText.java",
            "tools/ghidra_scripts/FindCallers.java",
        ],
        errors,
    )

    rtti = json.loads((ROOT / "config/ffxivgame.rtti.json").read_text(encoding="utf-8"))
    classes = rtti.get("classes", [])
    metadata = rtti.get("metadata", {})
    stats = rtti.get("stats", {})
    rvas = [row.get("rva") for row in classes]
    if metadata.get("source_sha256") != EXPECTED_RTTI_SOURCE_SHA256:
        errors.append("RTTI catalog source SHA-256 mismatch")
    if metadata.get("ghidra_version") != "12.1":
        errors.append("RTTI catalog Ghidra version mismatch")
    if rvas != sorted(set(rvas)):
        errors.append("RTTI catalog is not uniquely ordered by vtable RVA")
    if len(classes) != EXPECTED_RTTI_CLASSES:
        errors.append("pinned RTTI class count mismatch")
    if stats.get("vtable_records") != len(classes):
        errors.append("RTTI catalog vtable count mismatch")

    slot_lines = (ROOT / "config/ffxivgame.vtable_slots.jsonl").read_text(
        encoding="utf-8"
    ).splitlines()
    try:
        slot_rows = [json.loads(line) for line in slot_lines if line]
    except json.JSONDecodeError as exc:
        errors.append(f"invalid tracked JSONL config/ffxivgame.vtable_slots.jsonl: {exc}")
        slot_rows = []
    if slot_rows:
        slot_metadata = slot_rows[0]
        slots = slot_rows[1:]
        slot_keys = [(row.get("vtable_rva"), row.get("slot")) for row in slots]
        if (slot_metadata.get("record_type") != "metadata" or
                slot_metadata.get("source_sha256") != EXPECTED_RTTI_SOURCE_SHA256 or
                slot_metadata.get("ghidra_version") != "12.1"):
            errors.append("vtable-slot metadata mismatch")
        if any(row.get("record_type") != "vtable_slot" for row in slots):
            errors.append("unexpected vtable-slot record type")
        if slot_keys != sorted(slot_keys):
            errors.append("vtable-slot catalog is not ordered by vtable RVA and slot")
        if stats.get("vtable_slots") != len(slots):
            errors.append("RTTI catalog slot count mismatch")
        if any(not isinstance(row.get("fn_name"), str) for row in slots):
            errors.append("vtable-slot row lacks clean function name")
    else:
        errors.append("empty vtable-slot catalog")

    ledger_path = ROOT / "config/ffxivgame.external_dependencies.json"
    ledger_raw = ledger_path.read_bytes()
    try:
        ledger_text = ledger_raw.decode("ascii")
    except UnicodeDecodeError as exc:
        errors.append(f"dependency ledger is not ASCII: {exc}")
        ledger_text = ""
        ledger = {}
    else:
        ledger = json.loads(ledger_text)
    expected_catalogs = [
        "config/ffxivgame.vtable_method_names.json",
    ]
    ledger_catalogs = ledger.get("catalogs", [])
    if ledger.get("source_sha256") != EXPECTED_RTTI_SOURCE_SHA256:
        errors.append("dependency ledger source SHA-256 mismatch")
    if ledger.get("producer") != "tools/build_external_dependency_ledger.py":
        errors.append("dependency ledger producer mismatch")
    if [catalog.get("path") for catalog in ledger_catalogs] != expected_catalogs:
        errors.append("dependency ledger catalog inventory mismatch")
    valid_dispositions = {
        "independently-rederived", "keep-with-citation", "delete-with-source"
    }
    known_consumers = {
        consumer.get("id") for consumer in ledger.get("consumer_registry", [])
    }
    ledger_row_count = 0
    ledger_dispositions: dict[str, int] = {}
    blocked_rows = 0
    for catalog in ledger_catalogs:
        source_path = ROOT / catalog["path"]
        source_document = json.loads(source_path.read_text(encoding="utf-8"))
        source_rows = (
            source_document["classes"]
            if isinstance(source_document, dict) and "classes" in source_document
            else source_document
        )
        rows = catalog.get("rows", [])
        ledger_row_count += len(rows)
        if len(rows) != len(source_rows):
            errors.append(f"dependency ledger row count mismatch: {catalog['path']}")
            continue
        row_ids = [row.get("row_id") for row in rows]
        if len(row_ids) != len(set(row_ids)):
            errors.append(f"duplicate dependency ledger row id: {catalog['path']}")
        for ordinal, (source_row, row) in enumerate(zip(source_rows, rows)):
            digest = canonical_row_sha256(source_row)
            if row.get("source_ordinal") != ordinal or row.get("source_row_sha256") != digest:
                errors.append(f"stale dependency ledger row: {catalog['path']}#{ordinal}")
                break
            disposition = row.get("disposition")
            if catalog["path"] == "config/ffxivgame.vtable_method_names.json":
                eligibility = row.get("eligibility", {})
                if (eligibility.get("classification") != "selected-by-generator" or
                        eligibility.get("rejection_reasons")):
                    errors.append(
                        f"generated vtable row has invalid eligibility: {catalog['path']}#{ordinal}"
                    )
                    break
                if disposition != "independently-rederived":
                    errors.append(
                        f"generated vtable row lacks independent disposition: {catalog['path']}#{ordinal}"
                    )
                    break
            consumers = row.get("consumer_ids", [])
            blocking = row.get("blocking_consumer_ids", [])
            base_status = row.get("base_evidence", {}).get("status")
            ledger_dispositions[disposition] = ledger_dispositions.get(disposition, 0) + 1
            blocked_rows += disposition == "keep-with-citation" and bool(blocking)
            if disposition not in valid_dispositions:
                errors.append(f"invalid dependency disposition: {catalog['path']}#{ordinal}")
                break
            if not set(blocking) <= set(consumers):
                errors.append(f"blocking consumer is not a direct consumer: {catalog['path']}#{ordinal}")
                break
            if not set(consumers) <= known_consumers:
                errors.append(f"unknown dependency consumer: {catalog['path']}#{ordinal}")
                break
            if disposition == "delete-with-source" and blocking:
                errors.append(f"delete disposition has blocking consumer: {catalog['path']}#{ordinal}")
                break
            if disposition == "keep-with-citation" and not blocking:
                errors.append(f"keep disposition lacks blocking consumer: {catalog['path']}#{ordinal}")
                break
            if disposition == "independently-rederived" and base_status != "full-independent-derivation":
                errors.append(f"independent disposition lacks full derivation: {catalog['path']}#{ordinal}")
                break
    if ledger.get("summary", {}).get("catalog_rows") != ledger_row_count:
        errors.append("dependency ledger total row count mismatch")
    if ledger.get("summary", {}).get("catalog_dispositions") != dict(
            sorted(ledger_dispositions.items())):
        errors.append("dependency ledger disposition summary mismatch")
    if ledger.get("summary", {}).get("rows_blocked_from_deletion") != blocked_rows:
        errors.append("dependency ledger blocked-row summary mismatch")
    if re.search(r"(?:[A-Za-z]:\\|/Users/|/home/|agent-islands)", ledger_text, re.I):
        errors.append("dependency ledger contains a machine or private-island path")
    for path in paths:
        full = ROOT / path
        if full.suffix.lower() not in TEXT_SUFFIXES:
            continue
        try:
            text = full.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        if path != "tools/validate_repo.py":
            lower = text.lower()
            for token in ("garlemald", "server-workspace", "memory.md", ".claude"):
                if token in lower:
                    errors.append(f"forbidden private or consumer reference in {path}: {token}")
            if re.search(r"(?:[A-Za-z]:\\Users\\|/Users/|/home/)", text, re.I):
                errors.append(f"absolute maintainer path in {path}")
        if path != "tools/validate_repo.py":
            for line_number, line in enumerate(text.splitlines(), 1):
                lower_line = line.lower()
                for token in ("agents.md", "claude.md", "docs/ai_agents/local/"):
                    if token in lower_line:
                        errors.append(
                            f"forbidden local-contract reference: "
                            f"{path}:{line_number} -> {token}"
                        )

    for path in tracked_paths:
        data = (ROOT / path).read_bytes()
        if b"\0" in data:
            continue
        for line_number, raw_line in enumerate(data.splitlines(), 1):
            if not any(byte > 127 for byte in raw_line):
                continue
            try:
                decoded = raw_line.decode("utf-8")
                detail = ", ".join(
                    f"U+{ord(char):04X}"
                    for char in sorted({char for char in decoded if ord(char) > 127})
                )
            except UnicodeDecodeError:
                detail = ", ".join(
                    f"0x{byte:02X}" for byte in sorted({b for b in raw_line if b > 127})
                )
            errors.append(f"non-ASCII byte: {path}:{line_number} -> {detail}")

    if errors:
        print(f"FAIL: {len(errors)} repository validation error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print(
        f"PASS: repository boundary, provenance, links, JSON, and "
        f"{len(paths)} tracked files"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
