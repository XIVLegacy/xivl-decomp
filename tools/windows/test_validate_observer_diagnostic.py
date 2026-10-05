# SPDX-License-Identifier: AGPL-3.0-or-later
"""Reject malformed joins and incomplete forwarding evidence without native APIs."""

import copy
import json
from pathlib import Path
import sys
import unittest

import validate_observer_diagnostic as validator


TRACE_PATH = None


def sample_trace() -> dict:
    identity = dict(
        event_complete=True,
        raw_debug_object="0x20",
        event_pid=10,
        event_tid=11,
        raw_generation=1,
        event_index=1,
        engine_generation_known=True,
        engine_generation=2,
    )

    def row(
        kind: str, operation: int, sequence: int, exit_sequence: int, **fields: object
    ) -> dict:
        return dict(
            identity,
            kind=kind,
            session_id=1,
            operation_id=operation,
            parent_operation_id=0,
            sequence=sequence,
            exit_sequence=exit_sequence,
            observer_thread_id=12,
            incomplete=False,
            pre_log_failed=False,
            post_log_failed=False,
            rethrown=False,
            call_completed=True,
            original_target="0x400000",
            incoming_error_known=True,
            returned_error_known=True,
            incoming_last_error=0,
            returned_last_error=0,
            incoming_c_error=0,
            returned_c_error=0,
            **fields,
        )

    pending = {"pending_" + key: value for key, value in identity.items()}
    query = row(
        "query",
        2,
        3,
        6,
        manager="0x100",
        service_guid_pointer="0x110",
        iid_pointer="0x120",
        service_guid="11" * 16,
        iid="22" * 16,
        service_guid_status="read",
        iid_status="read",
        result=0,
        returned_interface="0x200",
        returned_interface_status="read",
        vtable="0x300",
        vtable_status="read",
        slot_plus_10_address="0x310",
        slot_plus_10_target="0x400",
        slot_plus_10_status="read",
        output_slot="0x500",
        successful_interface_qualified=True,
    )
    lookup = row(
        "lookup",
        3,
        4,
        5,
        manager="0x100",
        guid_pointer="0x110",
        ignored_edx="0x0",
        record_status="read",
        service_guid="11" * 16,
        guid_status="read",
        returned_record="0x600",
        record_service_address="0x610",
        record_service="0x700",
        record_service_status="read",
    )
    lookup["parent_operation_id"] = 2
    context = row(
        "context_write",
        4,
        7,
        8,
        context_pointer="0x800",
        handle="0x900",
        context_status="read",
        target_identity_status="read",
        target_handle_value=0x900,
        target_pid=10,
        target_tid=11,
        context_flags=0x10011,
        eip=0x1234,
        eflags=0x202,
        dr0=0,
        dr1=0,
        dr2=0,
        dr3=0,
        dr6=0,
        dr7=0,
        result=1,
    )
    rows = [
        row("pending_event", 1, 1, 2, pending_status="admitted", **pending),
        query,
        lookup,
        context,
        row("pending_event", 5, 9, 10, pending_status="closed", **pending),
    ]
    return dict(
        provenance="synthetic-forwarding-profile",
        live_coverage="incomplete",
        overflow_count=0,
        rows=rows,
    )


class TraceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.trace = (
            sample_trace()
            if TRACE_PATH is None
            else json.loads(
                TRACE_PATH.read_text(encoding="ascii"),
                object_pairs_hook=validator.unique_object,
            )
        )

    def selected(self, kind: str) -> dict:
        return next(row for row in self.trace["rows"] if row["kind"] == kind)

    def reject(self) -> None:
        with self.assertRaises(ValueError):
            validator.validate_trace(self.trace)

    def test_complete_synthetic_trace(self) -> None:
        self.assertEqual(validator.validate_trace(self.trace), len(self.trace["rows"]))

    def test_duplicate_keys_are_not_last_writer_wins(self) -> None:
        with self.assertRaisesRegex(ValueError, "duplicate JSON key"):
            json.loads(
                '{"result":0,"result":-1}', object_pairs_hook=validator.unique_object
            )

    def test_no_live_or_overflow_claim(self) -> None:
        for key, value in [
            ("live_coverage", "complete"),
            ("provenance", "live"),
            ("overflow_count", 1),
        ]:
            with self.subTest(key=key):
                trace = copy.deepcopy(self.trace)
                trace[key] = value
                with self.assertRaises(ValueError):
                    validator.validate_trace(trace)

    def test_missing_or_duplicate_exit(self) -> None:
        self.selected("query")["exit_sequence"] = 0
        self.reject()

    def test_event_mismatch_is_not_joined_by_adjacency(self) -> None:
        self.selected("query")["event_index"] += 1
        self.reject()

    def test_lookup_cannot_borrow_other_query(self) -> None:
        nested = next(
            row
            for row in self.trace["rows"]
            if row["kind"] == "lookup" and row["parent_operation_id"]
        )
        nested["service_guid"] = "00" * 16
        self.reject()

    def test_failed_or_null_query_remains_unqualified(self) -> None:
        self.selected("query")["returned_interface"] = "0x0"
        self.reject()

    def test_incomplete_errors_or_handle_identity(self) -> None:
        self.selected("context_write")["target_handle_value"] += 1
        self.reject()

    def test_native_result_is_required_and_typed(self) -> None:
        del self.selected("context_write")["result"]
        self.reject()

    def test_raw_arguments_and_observation_status_must_agree(self) -> None:
        cases = [
            ("lookup", "manager", {}),
            ("query", "manager", None),
            ("lookup", "guid_pointer", "0x0"),
            ("query", "iid_pointer", "0x0"),
            ("query", "service_guid_pointer", "0x0"),
            ("lookup", "record_status", "null"),
            ("context_write", "context_flags", 0),
        ]
        for kind, key, value in cases:
            with self.subTest(kind=kind, key=key):
                trace = copy.deepcopy(self.trace)
                next(row for row in trace["rows"] if row["kind"] == kind)[key] = value
                with self.assertRaises(ValueError):
                    validator.validate_trace(trace)

    def test_admission_header_must_match_attempt(self) -> None:
        self.selected("pending_event")["event_index"] += 1
        self.reject()

    def test_guid_bytes_cannot_be_json_numbers(self) -> None:
        value = 11111111111111111111111111111111
        for row in self.trace["rows"]:
            if row["kind"] in {"lookup", "query"}:
                row["service_guid"] = value
            if row["kind"] == "query":
                row["iid"] = value
        self.reject()

    def test_event_must_close_after_operations_return(self) -> None:
        self.selected("context_write")["exit_sequence"] = (
            max(row["exit_sequence"] for row in self.trace["rows"]) + 1
        )
        self.reject()

    def test_missing_boundary_or_pending_tail(self) -> None:
        self.trace["rows"] = [
            row
            for row in self.trace["rows"]
            if not (
                row["kind"] == "pending_event" and row["pending_status"] == "closed"
            )
        ]
        self.reject()


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--trace":
        TRACE_PATH = Path(sys.argv[2])
        del sys.argv[1:3]
    unittest.main()
