#!/usr/bin/env python3
"""Mutation tests for the retail protocol-caller contract."""

from __future__ import annotations

import copy
import json
import subprocess
import sys
import tempfile
from unittest import mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import _schema_check  # noqa: E402
import verify_retail_protocol_caller as verifier  # noqa: E402


REPO = Path(__file__).resolve().parents[1]
FIXTURE = REPO / "tools" / "fixtures" / "retail_protocol_caller_observations.json"
CHECK = REPO / "config" / "retail_protocol_caller_check.json"
RETAIL_INPUTS = REPO / "config" / "retail_inputs.json"
PROTOCOL = REPO / "config" / "ffxivgame.protocol_evidence.json"
SCHEMA = REPO / "schemas" / "retail-evidence-attestation.schema.json"
VERIFY = REPO / "tools" / "verify_retail_protocol_caller.py"
EXPORTER = REPO / "tools" / "ghidra_scripts" / "FindCallers.java"
WORKFLOW = REPO / ".github" / "workflows" / "retail-checks.yml"
PASSED: list[str] = []
FAILED: list[str] = []


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def _load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _write(path: Path, document: object) -> Path:
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return path


def _fails(
    directory: Path,
    observation: dict | None = None,
    expected: dict | None = None,
    retail_inputs: dict | None = None,
    protocol: dict | None = None,
) -> bool:
    observation_path = _write(
        directory / "observations.json",
        observation if observation is not None else _load(FIXTURE),
    )
    expected_path = _write(
        directory / "expected.json", expected if expected is not None else _load(CHECK)
    )
    retail_path = _write(
        directory / "retail-inputs.json",
        retail_inputs if retail_inputs is not None else _load(RETAIL_INPUTS),
    )
    protocol_path = _write(
        directory / "protocol.json",
        protocol if protocol is not None else _load(PROTOCOL),
    )
    try:
        return bool(verifier.verify(
            observation_path, expected_path, retail_path, protocol_path
        ))
    except (OSError, KeyError, TypeError, ValueError, verifier.VerificationError):
        return True


def _run_cli(path: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(VERIFY), "--input", str(path)],
        cwd=REPO, capture_output=True, text=True, check=False,
    )


def _source_row(protocol: dict) -> dict:
    return next(
        row for row in protocol["observations"]
        if row.get("sender_va_hex") == verifier.TARGET_VA
    )


def main() -> int:
    baseline = _load(FIXTURE)
    with tempfile.TemporaryDirectory(prefix="retail-protocol-caller-test-") as raw:
        directory = Path(raw)
        check("canonical fixture passes", not _fails(directory, baseline))

        for field, replacement in (
            ("schema_version", 2),
            ("check_id", "wrong-check"),
            ("input_id", "wrong-input"),
            ("target_va", "0xdeadbeef"),
        ):
            mutated = copy.deepcopy(baseline)
            mutated[field] = replacement
            check(f"observation {field} drift fails", _fails(directory, mutated))

        for callers, label in (
            ([], "missing caller"),
            (["0x00705eb0", "0x00705eb0"], "duplicate caller"),
            (["0x00705eb0", "0x00600000"], "unsorted callers"),
            (["0x00705EB0"], "mixed-case caller"),
            (["0x705eb0"], "non-padded caller"),
            (["0xdeadbeef"], "wrong caller"),
            ("0x00705eb0", "non-array caller"),
        ):
            mutated = copy.deepcopy(baseline)
            mutated["direct_caller_entry_vas"] = callers
            check(f"{label} fails", _fails(directory, mutated))

        mutated = copy.deepcopy(baseline)
        mutated["unexpected"] = True
        check("extra observation field fails", _fails(directory, mutated))
        mutated = copy.deepcopy(baseline)
        del mutated["target_va"]
        check("missing observation field fails", _fails(directory, mutated))

        expected = _load(CHECK)
        expected["expected"]["direct_caller_entry_vas"] = ["0xdeadbeef"]
        check("expected caller drift fails", _fails(directory, expected=expected))
        expected = _load(CHECK)
        expected["locator"]["target_va"] = "0xdeadbeef"
        check("expected target drift fails", _fails(directory, expected=expected))

        for mutation, label in (
            (("size", verifier.INPUT_SIZE + 1), "input size"),
            (("sha256", "0" * 64), "input hash"),
            (("filename", "other.exe"), "input filename"),
            (("id", "other-input"), "input id"),
        ):
            retail = _load(RETAIL_INPUTS)
            retail["inputs"][0][mutation[0]] = mutation[1]
            check(f"{label} drift fails", _fails(directory, retail_inputs=retail))
        retail = _load(RETAIL_INPUTS)
        retail["inputs"][0]["source"]["commit"] = "0" * 40
        check("private commit drift fails", _fails(directory, retail_inputs=retail))
        retail = _load(RETAIL_INPUTS)
        retail["inputs"][0]["allowed_checks"].append("other-check")
        check("input grant expansion fails", _fails(directory, retail_inputs=retail))

        protocol = _load(PROTOCOL)
        _source_row(protocol)["caller_va_hex"] = "0xdeadbeef"
        check("tracked caller drift fails", _fails(directory, protocol=protocol))
        protocol = _load(PROTOCOL)
        _source_row(protocol)["producer"] = "other-tool"
        check("tracked producer drift fails", _fails(directory, protocol=protocol))
        protocol = _load(PROTOCOL)
        protocol["observations"].append(copy.deepcopy(_source_row(protocol)))
        check("duplicate tracked row fails", _fails(directory, protocol=protocol))

        exporter = EXPORTER.read_text(encoding="utf-8").lower()
        check("exporter omits expected caller literal", "0x00705eb0" not in exporter)

        workflow = WORKFLOW.read_text(encoding="utf-8")
        check("workflow is manual only",
              "  workflow_dispatch:\n" in workflow
              and "pull_request:" not in workflow and "push:" not in workflow)
        check("workflow guards protected main",
              "python tools/verify_retail_protocol_caller.py --check-dispatch"
              in workflow
              and "if: github.event_name == 'workflow_dispatch'" not in workflow)
        check("workflow uses protected environment",
              "environment:\n      name: retail-evidence" in workflow)
        shared_actions = [
            line.strip().removeprefix("uses: ")
            for line in workflow.splitlines()
            if line.strip().startswith(
                "uses: XIVLegacy/xivl-tools/.github/actions/"
            )
        ]
        shared_revisions = {
            action.rsplit("@", 1)[-1] for action in shared_actions
        }
        shared_revision = next(iter(shared_revisions), "")
        check("shared retail actions use one immutable pin",
              len(shared_actions) == 3 and len(shared_revisions) == 1
              and len(shared_revision) == 40
              and all(char in "0123456789abcdef" for char in shared_revision)
              and sum("/fetch-retail-input@" in action
                      for action in shared_actions) == 1
              and sum("/setup-retail-toolchain@" in action
                      for action in shared_actions) == 1
              and sum("/finalize-retail-attestation@" in action
                      for action in shared_actions) == 1)
        check("local grant passes the token to the shared fetch action",
              "token: ${{ secrets.RETAIL_INPUTS_TOKEN }}" in workflow
              and "commit: aeb52f6dbde95a793ee6d52be28de9f28a885b15" in workflow
              and "path: ffxivgame.exe" in workflow
              and 'size: "15996808"' in workflow
              and "sha256: 9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9" in workflow)
        check("shared fetch action owns input identity",
              "RETAIL_INPUTS_REPOSITORY" not in workflow
              and "name: Verify private input identity" not in workflow)
        check("toolchain explicitly enables Ghidra",
              'include-ghidra: "true"' in workflow)
        check("analysis requires fetch and toolchain",
              "if: steps.fetch.outcome == 'success' && steps.toolchain.outcome == 'success'" in workflow)
        check("finalize precedes the local retained verifier",
              workflow.index("id: finalize") < workflow.index("id: retained")
              and "if: always() && !cancelled() && steps.finalize.outcome == 'success'" in workflow
              and "hashFiles" not in workflow)
        check("artifact upload follows finalize and retained validation",
              "always() && !cancelled() && steps.finalize.outcome == 'success' && steps.retained.outcome == 'success'" in workflow)
        check("failed evidence result requires every evidence stage",
              "steps.fetch.outcome != 'success' || steps.toolchain.outcome != 'success' || steps.analysis.outcome != 'success' || steps.finalize.outcome != 'success' || steps.retained.outcome != 'success'" in workflow)
        check("upload keeps only name and path",
              "if-no-files-found: error" in workflow
              and "retention-days: 30" in workflow
              and "compression-level:" not in workflow
              and "overwrite:" not in workflow
              and "include-hidden-files:" not in workflow)
        check("lane verifier remains local",
              "python tools/verify_retail_protocol_caller.py --input" in workflow
              and "python tools/verify_retail_protocol_caller.py --validate-retained-output _retail-staging" in workflow)

        schema = _schema_check.load_schema(SCHEMA)
        attestation = verifier.build_attestation("pass", "1" * 40)
        check("passing attestation satisfies schema",
              not _schema_check.validate(attestation, schema))
        attestation["observations"] = []
        check("unexpected attestation field fails",
              bool(_schema_check.validate(attestation, schema)))

        safe = directory / "safe"
        safe.mkdir()
        _write(safe / verifier.ATTESTATION_FILENAME,
               verifier.build_attestation("pass", "1" * 40))
        check("single sanitized retained file passes",
              not verifier.retained_output_errors(safe))
        (safe / "extra.log").write_text("unsafe\n", encoding="ascii")
        check("extra retained file fails", bool(verifier.retained_output_errors(safe)))
        (safe / "extra.log").unlink()
        (safe / "extra").mkdir()
        check("retained directory fails", bool(verifier.retained_output_errors(safe)))
        (safe / "extra").rmdir()

        target = directory / "symlink-target.json"
        _write(target, verifier.build_attestation("pass", "1" * 40))
        symlink_root = directory / "symlink-safe"
        symlink_root.mkdir()
        link = symlink_root / verifier.ATTESTATION_FILENAME
        try:
            link.symlink_to(target)
        except OSError:
            symlink_calls = [0]
            def fake_is_symlink():
                symlink_calls[0] += 1
                return symlink_calls[0] == 2
            with mock.patch.object(Path, "is_symlink", side_effect=fake_is_symlink):
                check("retained symlink fails",
                      bool(verifier.retained_output_errors(symlink_root)))
        else:
            check("retained symlink fails", bool(verifier.retained_output_errors(symlink_root)))
            link.unlink()

        sha = "1" * 40
        check("main dispatch passes", not verifier.dispatch_errors(
            "workflow_dispatch", "refs/heads/main", sha, sha
        ))
        check("feature ref fails", bool(verifier.dispatch_errors(
            "workflow_dispatch", "refs/heads/feature", sha, sha
        )))
        check("pull request event fails", bool(verifier.dispatch_errors(
            "pull_request", "refs/heads/main", sha, sha
        )))
        check("revision mismatch fails", bool(verifier.dispatch_errors(
            "workflow_dispatch", "refs/heads/main", sha, "2" * 40
        )))

        failed = copy.deepcopy(baseline)
        failed["direct_caller_entry_vas"] = ["0xdeadbeef"]
        failed_path = _write(directory / "failed.json", failed)
        result = _run_cli(failed_path)
        try:
            output = json.loads(result.stdout)
        except json.JSONDecodeError:
            output = {}
        check("failure invocation exits nonzero", result.returncode != 0)
        check("failure output is sanitized", set(output) == {
            "schemaVersion", "publicRepositoryCommit", "approvedInputSha256",
            "toolVersions", "check", "result",
        } and output.get("result", {}).get("status") == "fail"
              and "direct_caller_entry_vas" not in result.stdout)

        first = _run_cli(FIXTURE)
        second = _run_cli(FIXTURE)
        check("repeated passing output is byte-identical",
              first.returncode == second.returncode == 0
              and first.stdout.encode() == second.stdout.encode())

    if FAILED:
        print("FAIL: " + "; ".join(FAILED))
        return 1
    print(f"PASS: {len(PASSED)} protocol-caller verification checks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
