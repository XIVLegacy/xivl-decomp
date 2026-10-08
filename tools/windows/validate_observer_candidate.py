#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Check an offline candidate receipt against its production trace evidence."""

import argparse
import json
from collections import Counter
from pathlib import Path

from validate_observer_diagnostic import (
    TRANSLATION_IID,
    TRANSLATION_SERVICE_GUID,
    _binding_raw_identity,
    _integer,
    _raw_identity,
    _validate_binding_witness,
    _validate_callback_acquisition,
    _validate_callback_dispatch_fields,
    _validate_callback_entry,
    _validate_deferred_operation,
    unique_object,
    validate_provenance,
    validate_trace,
)

LIMITS = (
    "hold_ticks",
    "known_cleanup_ticks",
    "responsiveness_ticks",
    "owner_exit_ticks",
    "termination_ticks",
    "acknowledgement_ticks",
    "exit_confirmation_ticks",
)
FAULTS = (
    "publication_original_mismatch",
    "publication_owner_mismatch",
    "unread_provenance",
    "row_cap_overflow",
    "held_cleanup_retained",
)


def read_artifact(value: object, label: str) -> object:
    if not isinstance(value, str) or not value or not Path(value).is_absolute():
        raise ValueError(f"invalid absolute {label} artifact path")
    try:
        return json.loads(
            Path(value).read_text(encoding="ascii"), object_pairs_hook=unique_object
        )
    except (OSError, UnicodeError, ValueError) as error:
        raise ValueError(f"unreadable {label} artifact: {error}") from error


def validate_failure_trace(trace: object, name: str) -> dict:
    """Check persisted partial evidence without requiring a successful collection."""
    trace = require_fields(
        trace,
        {"provenance": "synthetic-forwarding-profile", "live_coverage": "incomplete"},
        f"{name} trace",
    )
    for field in ("overflow_count", "unlogged_calls", "active_calls"):
        value = trace.get(field)
        if type(value) is not int or value < 0:
            raise ValueError(f"invalid {name} trace {field}")
    positive_integer(
        trace.get("passthrough_generation"), "publication generation", 2**64 - 1
    )
    if type(trace.get("passthrough_published")) is not bool:
        raise ValueError(f"invalid {name} trace publication state")
    rows = trace.get("rows")
    if not isinstance(rows, list) or not rows:
        raise ValueError(f"missing {name} trace rows")
    required = {
        "pending_event",
        "callback_entry",
        "callback_acquisition",
        "engine_binding",
    }
    if name == "row_cap_overflow":
        required = {"pending_event"}
    elif name in ("publication_original_mismatch", "unread_provenance"):
        required.update(("query", "lookup"))
        if name == "publication_original_mismatch":
            required.add("context_write")
    kinds = set()
    sequences = set()
    operations = set()
    session = None
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError(f"invalid {name} trace row")
        kind = row.get("kind")
        if kind not in required | {"query", "lookup", "context_write"}:
            raise ValueError(f"invalid {name} trace row kind")
        kinds.add(kind)
        current = positive_integer(row.get("session_id"), "failure session", 2**64 - 1)
        if session is not None and session != current:
            raise ValueError(f"mixed {name} trace sessions")
        session = current
        operation = positive_integer(
            row.get("operation_id"), "failure operation", 2**64 - 1
        )
        begin = positive_integer(row.get("sequence"), "failure entry", 2**64 - 1)
        end = positive_integer(row.get("exit_sequence"), "failure exit", 2**64 - 1)
        if operation in operations or begin >= end or {begin, end} & sequences:
            raise ValueError(f"invalid {name} trace interval")
        operations.add(operation)
        sequences.update((begin, end))
        positive_integer(row.get("observer_thread_id"), "failure thread", 2**32 - 1)
        for field in ("incomplete", "pre_log_failed", "post_log_failed", "rethrown"):
            if type(row.get(field)) is not bool:
                raise ValueError(f"invalid {name} trace {field}")
        provenance = row.get("provenance")
        if isinstance(provenance, dict) and provenance.get("status") != "not_attempted":
            acquired = positive_integer(
                provenance.get("acquisition_begin_sequence"),
                "failure acquisition",
                2**64 - 1,
            )
            released = positive_integer(
                provenance.get("acquisition_end_sequence"), "failure release", 2**64 - 1
            )
            if (
                not begin < acquired < released < end
                or {acquired, released} & sequences
            ):
                raise ValueError(f"invalid {name} provenance interval")
            sequences.update((acquired, released))
    if not required <= kinds:
        raise ValueError(f"missing {name} causal trace boundaries")
    operations_by_id = {row["operation_id"]: row for row in rows}
    entries = {}
    for row in rows:
        if row["kind"] == "callback_entry":
            callback_operation, _ = _validate_callback_entry(row)
            _validate_callback_dispatch_fields(row)
            entries[callback_operation] = row
    for row in rows:
        kind = row["kind"]
        if kind == "pending_event":
            if _raw_identity(row) != _raw_identity(row, "pending_") or row.get(
                "pending_status"
            ) not in {"admitted", "closed"}:
                raise ValueError(f"invalid {name} pending raw key")
        elif kind == "callback_acquisition":
            _validate_callback_acquisition(row, entries)
        elif kind == "engine_binding":
            raw = _binding_raw_identity(row)
            _integer(row, "binding_engine_generation")
            if row.get("binding_status") == "bound":
                if row.get("binding_qualified") is not True:
                    raise ValueError(f"invalid {name} qualified binding")
                _validate_binding_witness(row, raw)
            elif row.get("binding_qualified") is not False:
                raise ValueError(f"invalid {name} refused binding")
        elif kind in {"query", "lookup", "context_write"}:
            _validate_deferred_operation(
                row,
                _integer(row, "parent_operation_id"),
                operations_by_id,
                _raw_identity(row),
            )
    return trace


def validate_failure_artifacts(receipt: dict) -> None:
    failures = require_fields(
        read_artifact(receipt.get("failure_artifact"), "failure"),
        {
            "schema": "observer-candidate-failure-cases-v2",
            "persistence_confirmed": True,
            "variant_artifacts_written": True,
            **dict.fromkeys(FAULTS, True),
        },
        "failure evidence",
    )
    variants = failures.get("variants")
    if not isinstance(variants, dict):
        raise ValueError("missing failure variants")
    for name in (
        "publication_original_mismatch",
        "unread_provenance",
        "row_cap_overflow",
        "held_cleanup_retained",
    ):
        variant = require_fields(
            variants.get(name), {"refused": True, "artifact_persisted": True}, name
        )
        trace = read_artifact(variant.get("trace_artifact"), f"{name} trace")
        state = read_artifact(variant.get("state_artifact"), f"{name} state")
        trace = validate_failure_trace(trace, name)
        if not isinstance(state, dict) or not state:
            raise ValueError(f"missing {name} state")
        try:
            validate_provenance(trace)
        except ValueError:
            pass
        else:
            raise ValueError(f"{name} failure trace unexpectedly qualifies")
        if name == "publication_original_mismatch":
            require_fields(
                state,
                {
                    "active_graph": True,
                    "hook_installed": True,
                    "transport_mutation_write": True,
                    "bridge_called_while_installed": True,
                    "original_mismatch_refused": True,
                    "owner_mismatch_refused": True,
                },
                name,
            )
            before = state.get("bridge_unlogged_before")
            after = state.get("bridge_unlogged_after")
            if (
                type(before) is not int
                or type(after) is not int
                or not 0 <= before < after
                or trace.get("unlogged_calls") != after
            ):
                raise ValueError("missing publication mismatch forwarding gap")
        elif name == "unread_provenance":
            require_fields(
                state,
                {"collector_read_failed": True, "provenance_refused": True},
                name,
            )
            queries = [row for row in trace["rows"] if row["kind"] == "query"]
            if len(queries) != state.get("query_rows") or not queries:
                raise ValueError("unread provenance query count disagrees with trace")
            for query in queries:
                require_fields(
                    query,
                    {
                        "service_guid": TRANSLATION_SERVICE_GUID,
                        "iid": TRANSLATION_IID,
                        "incomplete": True,
                        "call_completed": True,
                        "result": 0,
                    },
                    name,
                )
                require_fields(
                    query.get("provenance"),
                    {
                        "status": "refused",
                        "lifetime": "unknown",
                        "coherence": "unknown",
                        "output_complete": False,
                    },
                    name,
                )
                raw_fields = (
                    "raw_debug_object",
                    "event_pid",
                    "event_tid",
                    "raw_generation",
                    "event_index",
                    "engine_generation",
                )
                bindings = [
                    row
                    for row in trace["rows"]
                    if row["kind"] == "engine_binding"
                    and row.get("engine_generation_known") is True
                    and all(row.get(field) == query.get(field) for field in raw_fields)
                    and row["exit_sequence"] < query["sequence"]
                ]
                if not bindings or query.get("engine_generation_known") is not True:
                    raise ValueError("unread provenance lacks prior event binding")
        elif name == "row_cap_overflow":
            overflow = positive_integer(
                state.get("overflow_count"), "overflow", 2**64 - 1
            )
            cap = positive_integer(state.get("row_cap"), "failure row cap", 2**32 - 1)
            if trace.get("overflow_count") != overflow or len(trace["rows"]) > cap:
                raise ValueError("row-cap failure state disagrees with trace")
        else:
            require_fields(
                state,
                {
                    "unknown_side_effects": True,
                    "restoration_outcome": "retained-unknown",
                    "publication_outcome": "owner-retained",
                    "backing_graph_retained": True,
                    "transaction_unknown": True,
                },
                name,
            )
            if trace["passthrough_published"] is not True:
                raise ValueError("held cleanup trace lost retained publication")


def positive_integer(value: object, label: str, maximum: int) -> int:
    if type(value) is not int or not 0 < value <= maximum:
        raise ValueError(f"invalid positive finite {label}")
    return value


def require_fields(value: object, expected: dict, label: str) -> dict:
    if not isinstance(value, dict):
        raise ValueError(f"missing {label}")
    for key, required in expected.items():
        if type(value.get(key)) is not type(required) or value.get(key) != required:
            raise ValueError(f"invalid {label}.{key}")
    return value


def validate_candidate(value: object) -> int:
    """Accept CPU evidence only; this cannot qualify a native observation."""
    receipt = require_fields(
        value,
        {
            "schema": "observer-candidate-qualification-v1",
            "profile": "query-output-identity-v1",
            "mode": "offline_cpu",
            "success": True,
        },
        "receipt",
    )
    session = positive_integer(receipt.get("session_id"), "session", 2**64 - 1)
    cap = positive_integer(receipt.get("row_cap"), "row cap", 2**32 - 1)
    limits = receipt.get("limits")
    if not isinstance(limits, dict):
        raise ValueError("missing explicit limits")
    for key in LIMITS:
        positive_integer(limits.get(key), key, 2**32 - 1)
    clock = require_fields(
        receipt.get("clock"), {"kind": "deterministic-injected"}, "clock"
    )
    ticks = positive_integer(clock.get("ticks"), "clock ticks", 2**64 - 1)
    consumed = clock.get("consumed")
    if not isinstance(consumed, dict):
        raise ValueError("missing consumed clock limits")
    for key in LIMITS:
        positive_integer(consumed.get(key), f"consumed {key}", limits[key])
    if sum(consumed[key] for key in LIMITS) != ticks:
        raise ValueError("clock ticks disagree with consumed limits")
    require_fields(
        receipt.get("installation"),
        {"installed": True, "restored": True, "five_attestations": True},
        "installation",
    )
    require_fields(
        receipt.get("publication"),
        {
            "published": True,
            "cleared": True,
            "owner_released": True,
            "bridge_consumed": True,
            "bridge_unlogged_calls": 0,
        },
        "publication",
    )
    require_fields(
        receipt.get("binding"),
        {
            "raw_admission": True,
            "production_callback": True,
            "delayed_binding": True,
            "continuation_closed": True,
        },
        "binding",
    )
    identity = require_fields(
        receipt.get("identity"),
        {
            "query_provenance_qualified": True,
            "native_identity": "unknown",
            "witness": "injected-cpu-only",
        },
        "identity",
    )
    require_fields(
        receipt.get("recovery"),
        {
            "classification": "known_empty",
            "action_adapter": True,
            "succeeded": True,
            "observer_outcome": "orderly-offline",
        },
        "recovery",
    )
    require_fields(
        receipt.get("fixture_outcome"),
        {
            "qualification": "cpu-only",
            "exit_confirmed": False,
            "native_survival": "unknown",
        },
        "fixture_outcome",
    )
    require_fields(
        receipt.get("fail_closed_cases"),
        {
            "invalid_limits": True,
            "native_authority_refused": True,
            **dict.fromkeys(FAULTS, True),
        },
        "fail_closed_cases",
    )
    blockers = receipt.get("residual_blockers")
    if (
        not isinstance(blockers, list)
        or not blockers
        or any(not isinstance(item, str) or not item for item in blockers)
    ):
        raise ValueError("missing residual blockers")
    trace = receipt.get("trace")
    count = validate_trace(trace)
    validate_provenance(trace)
    if count > cap:
        raise ValueError("total trace rows exceed supplied row cap")
    if (
        type(trace.get("active_calls")) is not int
        or trace.get("active_calls") != 0
        or trace.get("passthrough_published") is not False
    ):
        raise ValueError("forwarding remains active after reported publication release")
    counts = Counter(row["kind"] for row in trace["rows"])
    for kind in (
        "pending_event",
        "callback_entry",
        "callback_acquisition",
        "engine_binding",
    ):
        if not counts[kind]:
            raise ValueError(f"missing production composition evidence: {kind}")
    for kind in ("lookup", "query", "context_write"):
        summary_key = "context_rows" if kind == "context_write" else f"{kind}_rows"
        reported = positive_integer(identity.get(summary_key), summary_key, cap)
        if reported != counts[kind]:
            raise ValueError(f"{kind} summary disagrees with trace or row cap")
    for row in trace["rows"]:
        if row.get("session_id") != session:
            raise ValueError("receipt session disagrees with trace")
    if read_artifact(receipt.get("trace_artifact"), "trace") != trace:
        raise ValueError("trace artifact disagrees with embedded evidence")
    validate_failure_artifacts(receipt)
    return count


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    if not args.receipt.is_absolute():
        parser.error("--receipt must be an absolute path")
    try:
        value = json.loads(
            args.receipt.read_text(encoding="ascii"), object_pairs_hook=unique_object
        )
        count = validate_candidate(value)
    except (OSError, UnicodeError, ValueError, TypeError, KeyError) as error:
        parser.exit(1, f"observer candidate: {error}\n")
    print(
        f"validated offline candidate with {count} trace rows; native state remains unknown"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
