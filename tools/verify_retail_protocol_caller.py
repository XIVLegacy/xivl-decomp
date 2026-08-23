#!/usr/bin/env python3
"""Verify the fixed retail protocol-caller observation contract."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

import _schema_check  # noqa: E402


REPO = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = (
    REPO / "tools" / "fixtures" / "retail_protocol_caller_observations.json"
)
DEFAULT_CHECK = REPO / "config" / "retail_protocol_caller_check.json"
DEFAULT_RETAIL_INPUTS = REPO / "config" / "retail_inputs.json"
DEFAULT_PROTOCOL_EVIDENCE = REPO / "config" / "ffxivgame.protocol_evidence.json"
DEFAULT_SCHEMA = REPO / "schemas" / "retail-evidence-attestation.schema.json"

CHECK_ID = "protocol-0x0135-single-direct-caller-v1"
INPUT_ID = "ffxivgame-1.23b"
INPUT_FILENAME = "ffxivgame.exe"
INPUT_SIZE = 15996808
INPUT_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
PRIVATE_REPOSITORY = "XIVLegacy/xivl-private-assets"
PRIVATE_COMMIT = "aeb52f6dbde95a793ee6d52be28de9f28a885b15"
PRIVATE_PATH = "ffxivgame.exe"
TARGET_VA = "0x0075ecd0"
EXPECTED_CALLERS = ("0x00705eb0",)
SCHEMA_VERSION = 1
ATTESTATION_FILENAME = "retail-evidence-attestation.json"
TOOL_VERSIONS = {
    "ghidra": "12.1.3",
    "jdk": "21.0.12.1+1",
    "verifier": "1.0",
}

ADDRESS_RE = re.compile(r"^0x[0-9a-f]{8}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
OBSERVATION_KEYS = frozenset({
    "schema_version", "check_id", "input_id", "target_va",
    "direct_caller_entry_vas",
})
CHECK_KEYS = frozenset({
    "schema_version", "check", "input_id", "locator", "expected",
})


class VerificationError(Exception):
    """Malformed input that is safe to report without its contents."""


def _read_json(path: Path) -> Any:
    try:
        with path.open(encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise VerificationError("JSON input could not be read") from exc


def _caller_list_errors(value: Any) -> list[str]:
    if not isinstance(value, list):
        return ["caller entries are not an array"]
    if any(not isinstance(entry, str) or not ADDRESS_RE.fullmatch(entry)
           for entry in value):
        return ["caller entry is malformed"]
    if len(value) != len(set(value)):
        return ["caller entry is duplicated"]
    if value != sorted(value, key=lambda entry: int(entry[2:], 16)):
        return ["caller entries are not sorted"]
    return []


def _observation_errors(document: Any) -> list[str]:
    if not isinstance(document, dict) or frozenset(document) != OBSERVATION_KEYS:
        return ["observation document shape is invalid"]
    errors = _caller_list_errors(document.get("direct_caller_entry_vas"))
    if (
        document.get("schema_version") != SCHEMA_VERSION
        or document.get("check_id") != CHECK_ID
        or document.get("input_id") != INPUT_ID
        or document.get("target_va") != TARGET_VA
    ):
        errors.append("observation identity is invalid")
    if document.get("direct_caller_entry_vas") != list(EXPECTED_CALLERS):
        errors.append("direct caller set differs")
    return errors


def _check_errors(document: Any) -> list[str]:
    if not isinstance(document, dict) or frozenset(document) != CHECK_KEYS:
        return ["check document shape is invalid"]
    callers = document.get("expected", {}).get("direct_caller_entry_vas") \
        if isinstance(document.get("expected"), dict) else None
    errors = _caller_list_errors(callers)
    if (
        document.get("schema_version") != SCHEMA_VERSION
        or document.get("check") != {"id": CHECK_ID, "version": 1}
        or document.get("input_id") != INPUT_ID
        or document.get("locator") != {"target_va": TARGET_VA}
        or document.get("expected") != {
            "direct_caller_entry_vas": list(EXPECTED_CALLERS)
        }
    ):
        errors.append("check document drifted")
    return errors


def _retail_input_errors(document: Any) -> list[str]:
    expected = {
        "schema_version": 1,
        "inputs": [{
            "id": INPUT_ID,
            "filename": INPUT_FILENAME,
            "size": INPUT_SIZE,
            "sha256": INPUT_SHA256,
            "source": {
                "repository": PRIVATE_REPOSITORY,
                "commit": PRIVATE_COMMIT,
                "path": PRIVATE_PATH,
            },
            "allowed_checks": [CHECK_ID],
        }],
    }
    return [] if document == expected else ["retail input grant drifted"]


def _protocol_evidence_errors(document: Any) -> list[str]:
    if not isinstance(document, dict) or document.get("schema_version") != 1:
        return ["protocol evidence is malformed"]
    observations = document.get("observations")
    if not isinstance(observations, list):
        return ["protocol evidence observations are malformed"]
    rows = [row for row in observations if isinstance(row, dict)
            and row.get("sender_va_hex") == TARGET_VA]
    if len(rows) != 1:
        return ["tracked protocol source row is not unique"]
    row = rows[0]
    if (
        row.get("caller_va_hex") != EXPECTED_CALLERS[0]
        or row.get("direction") != "client-to-server"
        or "FindCallers.java" not in str(row.get("producer", ""))
    ):
        return ["tracked protocol source row drifted"]
    return []


def _git_commit() -> str:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=REPO, check=True,
            capture_output=True, text=True,
        )
        commit = result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        commit = "0" * 40
    return commit if COMMIT_RE.fullmatch(commit) else "0" * 40


def build_attestation(status: str, public_commit: str | None = None) -> dict[str, Any]:
    commit = public_commit if public_commit is not None else _git_commit()
    if not COMMIT_RE.fullmatch(commit):
        raise ValueError("public commit is malformed")
    return {
        "schemaVersion": SCHEMA_VERSION,
        "publicRepositoryCommit": commit,
        "approvedInputSha256": INPUT_SHA256,
        "toolVersions": dict(TOOL_VERSIONS),
        "check": {"id": CHECK_ID, "version": 1},
        "result": {"status": status},
    }


def verify(
    input_path: Path = DEFAULT_INPUT,
    check_path: Path = DEFAULT_CHECK,
    retail_inputs_path: Path = DEFAULT_RETAIL_INPUTS,
    protocol_evidence_path: Path = DEFAULT_PROTOCOL_EVIDENCE,
) -> list[str]:
    observations = _read_json(input_path)
    check = _read_json(check_path)
    retail_inputs = _read_json(retail_inputs_path)
    protocol_evidence = _read_json(protocol_evidence_path)
    errors = _retail_input_errors(retail_inputs)
    errors.extend(_check_errors(check))
    errors.extend(_protocol_evidence_errors(protocol_evidence))
    errors.extend(_observation_errors(observations))
    return errors


def dispatch_errors(event_name: str, ref: str, sha: str, head: str) -> list[str]:
    if event_name != "workflow_dispatch":
        return ["dispatch event is unauthorized"]
    if ref != "refs/heads/main":
        return ["dispatch ref is unauthorized"]
    if not COMMIT_RE.fullmatch(sha) or sha != head:
        return ["dispatch revision is unauthorized"]
    return []


def retained_output_errors(directory: Path) -> list[str]:
    if not directory.is_dir() or directory.is_symlink():
        return ["retained output root is invalid"]
    entries = list(directory.iterdir())
    if len(entries) != 1 or entries[0].name != ATTESTATION_FILENAME:
        return ["retained output allowlist differs"]
    attestation_path = entries[0]
    if attestation_path.is_symlink() or not attestation_path.is_file():
        return ["retained attestation is not a regular file"]
    if attestation_path.stat().st_size > 4096:
        return ["retained attestation is too large"]
    try:
        attestation = _read_json(attestation_path)
        schema = _schema_check.load_schema(DEFAULT_SCHEMA)
        schema_errors = _schema_check.validate(attestation, schema)
    except (OSError, ValueError, VerificationError, _schema_check.SchemaError):
        return ["retained attestation could not be validated"]
    return ["retained attestation schema rejected output"] if schema_errors else []


def _parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT, dest="input_path")
    parser.add_argument("--check-dispatch", action="store_true")
    parser.add_argument("--validate-retained-output", type=Path)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(argv)
    if args.check_dispatch:
        errors = dispatch_errors(
            os.environ.get("GITHUB_EVENT_NAME", ""),
            os.environ.get("GITHUB_REF", ""),
            os.environ.get("GITHUB_SHA", ""),
            _git_commit(),
        )
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1 if errors else 0
    if args.validate_retained_output is not None:
        errors = retained_output_errors(args.validate_retained_output)
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1 if errors else 0

    try:
        errors = verify(
            args.input_path,
        )
    except (VerificationError, OSError, KeyError, TypeError, ValueError):
        errors = ["verification input is malformed"]
    attestation = build_attestation("pass" if not errors else "fail")
    try:
        schema = _schema_check.load_schema(DEFAULT_SCHEMA)
        schema_errors = _schema_check.validate(attestation, schema)
    except (OSError, ValueError, _schema_check.SchemaError):
        schema_errors = ["schema unavailable"]
    if schema_errors:
        errors.append("attestation schema rejected output")
        attestation = build_attestation("fail")
    print(json.dumps(
        attestation, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    ))
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
