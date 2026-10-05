# SPDX-License-Identifier: AGPL-3.0-or-later
"""Check complete synthetic forwarding traces; live traces are unsupported."""

import argparse
import json
from pathlib import Path
import re


def unique_object(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _integer(
    row: dict, key: str, minimum: int = 0, maximum: int = 0xFFFFFFFFFFFFFFFF
) -> int:
    value = row.get(key)
    if type(value) is not int or not minimum <= value <= maximum:
        raise ValueError(f"invalid integer: {key}")
    return value


def _address(row: dict, key: str) -> int:
    value = row.get(key)
    if not isinstance(value, str) or not re.fullmatch(r"0x[0-9a-fA-F]{1,8}", value):
        raise ValueError(f"invalid x86 address: {key}")
    return int(value, 16)


def _identity(row: dict, prefix: str = "") -> tuple:
    if row.get(prefix + "event_complete") is not True:
        raise ValueError("unknown event identity")
    raw = _address(row, prefix + "raw_debug_object")
    pid = _integer(row, prefix + "event_pid", 1, 0xFFFFFFFF)
    tid = _integer(row, prefix + "event_tid", 1, 0xFFFFFFFF)
    generation = _integer(row, prefix + "raw_generation", 1)
    index = _integer(row, prefix + "event_index", 1)
    if not raw or row.get(prefix + "engine_generation_known") is not True:
        raise ValueError("unknown raw object or engine generation")
    engine = _integer(row, prefix + "engine_generation")
    return raw, pid, tid, generation, index, engine


def _guid(row: dict, key: str) -> str:
    value = row.get(key)
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-fA-F]{32}", value):
        raise ValueError(f"invalid GUID bytes: {key}")
    return value


def validate_trace(trace: object) -> int:
    """Return row count after structural checks, never runtime qualification."""
    if (
        not isinstance(trace, dict)
        or trace.get("provenance") != "synthetic-forwarding-profile"
        or trace.get("live_coverage") != "incomplete"
    ):
        raise ValueError("unsupported provenance or live coverage claim")
    if _integer(trace, "overflow_count"):
        raise ValueError("trace overflow")
    rows = trace.get("rows")
    if not isinstance(rows, list) or not rows:
        raise ValueError("missing trace rows")
    operations = {}
    used_sequences = set()
    session = None
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("row must be an object")
        seq = _integer(row, "sequence", 1)
        exit_seq = _integer(row, "exit_sequence", seq + 1)
        if seq in used_sequences or exit_seq in used_sequences:
            raise ValueError("duplicate entry/exit sequence")
        used_sequences.update((seq, exit_seq))
        operation = _integer(row, "operation_id", 1)
        if operation in operations:
            raise ValueError("duplicate operation")
        operations[operation] = row
        _integer(row, "observer_thread_id", 1, 0xFFFFFFFF)
        current_session = _integer(row, "session_id", 1)
        if session is not None and current_session != session:
            raise ValueError("mixed sessions")
        session = current_session
        for key in ("incomplete", "pre_log_failed", "post_log_failed", "rethrown"):
            if row.get(key) is not False:
                raise ValueError(f"incomplete trace: {key}")
        if row.get("kind") not in {"lookup", "query", "context_write", "pending_event"}:
            raise ValueError("unsupported row kind")
    ordered = sorted(rows, key=lambda row: row["sequence"])
    if {row["kind"] for row in rows} != {
        "lookup",
        "query",
        "context_write",
        "pending_event",
    }:
        raise ValueError("missing forwarding boundary")
    pending = None
    seen_events = set()
    event_operations = []
    admitted_exit = 0
    for row in ordered:
        parent_id = _integer(row, "parent_operation_id")
        if parent_id:
            parent = operations.get(parent_id)
            if (
                parent is None
                or not parent["sequence"]
                < row["sequence"]
                < row["exit_sequence"]
                < parent["exit_sequence"]
                or parent["observer_thread_id"] != row["observer_thread_id"]
            ):
                raise ValueError("invalid parent interval or thread")
        if row["kind"] == "pending_event":
            attempted = _identity(row, "pending_")
            status = row.get("pending_status")
            if (
                status == "admitted"
                and pending is None
                and attempted not in seen_events
            ):
                seen_events.add(attempted)
                if _identity(row) != attempted:
                    raise ValueError("admission header mismatch")
                pending = attempted
                admitted_exit = row["exit_sequence"]
                event_operations = []
            elif status == "closed" and pending == attempted:
                if _identity(row) != pending:
                    raise ValueError("close header mismatch")
                if any(
                    operation["exit_sequence"] >= row["sequence"]
                    for operation in event_operations
                ):
                    raise ValueError("operation crosses event closure")
                pending = None
            else:
                raise ValueError("duplicate, changed or missing pending event")
            continue
        if pending is None or _identity(row) != pending:
            raise ValueError("operation has no exact pending event join")
        if row["sequence"] <= admitted_exit:
            raise ValueError("operation crosses event admission")
        event_operations.append(row)
        if row.get("call_completed") is not True or not _address(
            row, "original_target"
        ):
            raise ValueError("missing original completion")
        if (
            row.get("incoming_error_known") is not True
            or row.get("returned_error_known") is not True
        ):
            raise ValueError("unknown error fields")
        for key in ("incoming_last_error", "returned_last_error"):
            _integer(row, key, 0, 0xFFFFFFFF)
        for key in ("incoming_c_error", "returned_c_error"):
            _integer(row, key, -0x80000000, 0x7FFFFFFF)
        if row["kind"] == "lookup":
            for key in ("manager", "guid_pointer"):
                if not _address(row, key):
                    raise ValueError(f"null lookup argument: {key}")
            _address(row, "ignored_edx")
            _guid(row, "service_guid")
            if (
                row.get("guid_status") != "read"
                or row.get("record_status") != "read"
                or row.get("record_service_status") != "read"
                or not _address(row, "returned_record")
                or not _address(row, "record_service")
            ):
                raise ValueError("unread selected service")
            if (
                _address(row, "record_service_address")
                != _address(row, "returned_record") + 0x10
            ):
                raise ValueError("wrong service record field")
            if parent_id:
                parent = operations[parent_id]
                if (
                    parent["kind"] != "query"
                    or any(
                        row.get(key) != parent.get(key)
                        for key in ("manager", "service_guid")
                    )
                    or _identity(parent) != pending
                ):
                    raise ValueError("lookup/query binding mismatch")
        elif row["kind"] == "query":
            for key in ("manager", "service_guid_pointer", "iid_pointer"):
                if not _address(row, key):
                    raise ValueError(f"null query argument: {key}")
            _guid(row, "service_guid")
            _guid(row, "iid")
            if (
                _integer(row, "result", -0x80000000, 0x7FFFFFFF) < 0
                or row.get("successful_interface_qualified") is not True
                or any(
                    row.get(key) != "read"
                    for key in (
                        "service_guid_status",
                        "iid_status",
                        "returned_interface_status",
                        "vtable_status",
                        "slot_plus_10_status",
                    )
                )
            ):
                raise ValueError("unsuccessful or unread query")
            for key in (
                "returned_interface",
                "vtable",
                "slot_plus_10_target",
                "output_slot",
            ):
                if not _address(row, key):
                    raise ValueError(f"null query field: {key}")
            if _address(row, "slot_plus_10_address") != _address(row, "vtable") + 0x10:
                raise ValueError("wrong interface slot")
            if not any(
                child["kind"] == "lookup"
                and child["parent_operation_id"] == row["operation_id"]
                for child in rows
            ):
                raise ValueError("missing selected lookup for query")
        else:
            _integer(row, "result", -0x80000000, 0x7FFFFFFF)
            if (
                row.get("context_status") != "read"
                or row.get("target_identity_status") != "read"
            ):
                raise ValueError("unread context or handle identity")
            if (
                not _address(row, "context_pointer")
                or not _address(row, "handle")
                or _integer(row, "target_handle_value") != _address(row, "handle")
            ):
                raise ValueError("context pointer or handle binding mismatch")
            _integer(row, "target_pid", 1, 0xFFFFFFFF)
            _integer(row, "target_tid", 1, 0xFFFFFFFF)
            for key in (
                "context_flags",
                "eip",
                "eflags",
                "dr0",
                "dr1",
                "dr2",
                "dr3",
                "dr6",
                "dr7",
            ):
                _integer(row, key, 0, 0xFFFFFFFF)
            if not row["context_flags"]:
                raise ValueError("malformed context flags")
    if pending is not None:
        raise ValueError("pending event not closed")
    return len(rows)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True)
    args = parser.parse_args()
    if not args.trace.is_absolute():
        parser.error("--trace must be an absolute path")
    try:
        value = json.loads(
            args.trace.read_text(encoding="ascii"), object_pairs_hook=unique_object
        )
        count = validate_trace(value)
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"observer diagnostic: {error}\n")
    print(f"validated {count} synthetic rows; live coverage remains incomplete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
