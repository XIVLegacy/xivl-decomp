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
            incoming_last_status=0,
            returned_last_status=0,
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
        passthrough_generation=0,
        passthrough_published=False,
        unlogged_calls=0,
        rows=rows,
    )


def sample_provenance_trace() -> dict:
    trace = sample_trace()
    query = next(row for row in trace["rows"] if row["kind"] == "query")
    lookup = next(row for row in trace["rows"] if row["kind"] == "lookup")
    query["service_guid"] = validator.TRANSLATION_SERVICE_GUID
    query["iid"] = validator.TRANSLATION_IID
    lookup["service_guid"] = validator.TRANSLATION_SERVICE_GUID
    query["exit_sequence"] = 8
    next(row for row in trace["rows"] if row["kind"] == "context_write")["sequence"] = 9
    next(row for row in trace["rows"] if row["kind"] == "context_write")[
        "exit_sequence"
    ] = 10
    next(
        row
        for row in trace["rows"]
        if row["kind"] == "pending_event" and row["pending_status"] == "closed"
    )["sequence"] = 11
    next(
        row
        for row in trace["rows"]
        if row["kind"] == "pending_event" and row["pending_status"] == "closed"
    )["exit_sequence"] = 12
    unknown_mapping = dict(
        status="not_attempted",
        kind="unknown",
        base="0x0",
        extent=0,
        executable=False,
        module=dict(
            mapping_status="not_attempted",
            resident_base="0x0",
            resident_extent=0,
            resident_path="",
            architecture="",
            backing_file_size=0,
            backing_sha256="0" * 64,
            binding_resident_base="0x0",
            binding_resident_extent=0,
            binding_file_size=0,
            binding_file_sha256="0" * 64,
            binding_lifetime_id=0,
            binding_authority_id="",
            binding_mechanism="",
            binding_status="unknown",
            binding_evidence=False,
        ),
    )
    target_mapping = copy.deepcopy(unknown_mapping)
    target_mapping.update(
        status="read", kind="image", base="0x300", extent=0x1000, executable=True
    )
    target_mapping["module"].update(
        mapping_status="read",
        resident_base="0x300",
        resident_extent=0x1000,
        resident_path="C:\\synthetic\\provider.dll",
        architecture="PE32",
        backing_file_size=0x1000,
        backing_sha256="5a" * 32,
        binding_resident_base="0x300",
        binding_resident_extent=0x1000,
        binding_file_size=0x1000,
        binding_file_sha256="5a" * 32,
        binding_lifetime_id=0xA1,
        binding_authority_id='synthetic"witness\n1',
        binding_mechanism="injected-witness",
        binding_status="bound",
        binding_evidence=True,
    )
    query["provenance"] = dict(
        status="accepted",
        acquisition_begin_sequence=6,
        acquisition_end_sequence=7,
        lifetime_id=0xA1,
        lifetime="retained",
        coherence="coherent",
        output_complete=True,
        session_id=query["session_id"],
        operation_id=query["operation_id"],
        event_complete=query["event_complete"],
        raw_debug_object=query["raw_debug_object"],
        event_pid=query["event_pid"],
        event_tid=query["event_tid"],
        raw_generation=query["raw_generation"],
        event_index=query["event_index"],
        engine_generation_known=query["engine_generation_known"],
        engine_generation=query["engine_generation"],
        returned_interface=query["returned_interface"],
        vtable=query["vtable"],
        slot_plus_10_address=query["slot_plus_10_address"],
        slot_plus_10_target=query["slot_plus_10_target"],
        interface_mapping=unknown_mapping,
        vtable_mapping=copy.deepcopy(unknown_mapping),
        target_mapping=target_mapping,
    )
    return trace


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

    def test_zero_based_raw_event_index_is_preserved(self) -> None:
        for row in self.trace["rows"]:
            row["event_index"] = 0
            if row["kind"] == "pending_event":
                row["pending_event_index"] = 0
        self.assertEqual(validator.validate_trace(self.trace), len(self.trace["rows"]))

    def test_duplicate_keys_are_not_last_writer_wins(self) -> None:
        with self.assertRaisesRegex(ValueError, "duplicate JSON key"):
            json.loads(
                '{"result":0,"result":-1}', object_pairs_hook=validator.unique_object
            )

    def test_known_zero_engine_generation_is_refused(self) -> None:
        for row in self.trace["rows"]:
            row["engine_generation"] = 0
            if row["kind"] == "pending_event":
                row["pending_engine_generation"] = 0
        self.reject()

    def test_no_live_or_overflow_claim(self) -> None:
        for key, value in [
            ("live_coverage", "complete"),
            ("provenance", "live"),
            ("overflow_count", 1),
            ("unlogged_calls", 1),
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

    def test_strict_provenance_requires_selected_query(self) -> None:
        with self.assertRaisesRegex(ValueError, "no qualified translation query"):
            validator.validate_provenance(sample_trace())

    def test_strict_provenance_accepts_synthetic_output(self) -> None:
        trace = self.trace if TRACE_PATH is not None else sample_provenance_trace()
        self.assertGreaterEqual(validator.validate_provenance(trace), 1)

    def test_strict_provenance_rejects_target_binding_mutation(self) -> None:
        trace = self.trace if TRACE_PATH is not None else sample_provenance_trace()
        target = next(row for row in trace["rows"] if row["kind"] == "query")[
            "provenance"
        ]["target_mapping"]
        target["module"]["binding_status"] = "unbound"
        with self.assertRaisesRegex(ValueError, "backing file"):
            validator.validate_provenance(trace)

    def test_strict_provenance_rejects_association_mutation(self) -> None:
        trace = self.trace if TRACE_PATH is not None else sample_provenance_trace()
        query = next(row for row in trace["rows"] if row["kind"] == "query")
        query["provenance"]["operation_id"] += 1
        with self.assertRaisesRegex(ValueError, "operation mismatch"):
            validator.validate_provenance(trace)

    def test_strict_provenance_rejects_address_and_session_mutations(self) -> None:
        trace = sample_provenance_trace()
        query = next(row for row in trace["rows"] if row["kind"] == "query")
        query["provenance"]["slot_plus_10_target"] = "0x401"
        with self.assertRaisesRegex(ValueError, "address mismatch"):
            validator.validate_provenance(trace)

        trace = sample_provenance_trace()
        query = next(row for row in trace["rows"] if row["kind"] == "query")
        query["session_id"] += 1
        with self.assertRaisesRegex(ValueError, "mixed sessions"):
            validator.validate_provenance(trace)

    def test_strict_provenance_rejects_unknown_or_unread_target(self) -> None:
        for field, value, message in [
            ("status", "not_attempted", "unread provenance mapping"),
            ("kind", "unknown", "unknown claimed provenance mapping"),
        ]:
            with self.subTest(field=field):
                trace = sample_provenance_trace()
                target = next(row for row in trace["rows"] if row["kind"] == "query")[
                    "provenance"
                ]["target_mapping"]
                target[field] = value
                with self.assertRaisesRegex(ValueError, message):
                    validator.validate_provenance(trace)

    def test_strict_provenance_rejects_changed_or_incomplete_evidence(self) -> None:
        for field, value, message in [
            ("coherence", "changed", "not coherent"),
            ("output_complete", False, "incomplete query output"),
            ("event_complete", False, "unknown event identity"),
        ]:
            with self.subTest(field=field):
                trace = sample_provenance_trace()
                query = next(row for row in trace["rows"] if row["kind"] == "query")
                if field == "event_complete":
                    query["provenance"][field] = value
                else:
                    query["provenance"][field] = value
                with self.assertRaisesRegex(ValueError, message):
                    validator.validate_provenance(trace)

    def test_strict_provenance_rejects_matching_incomplete_events(self) -> None:
        trace = sample_provenance_trace()
        query = next(row for row in trace["rows"] if row["kind"] == "query")
        query["event_complete"] = False
        query["provenance"]["event_complete"] = False
        query["incomplete"] = False
        with self.assertRaisesRegex(ValueError, "unknown event identity"):
            validator.validate_provenance(trace)

    def test_strict_provenance_rejects_hash_only_or_zero_hash_binding(self) -> None:
        trace = sample_provenance_trace()
        target = next(row for row in trace["rows"] if row["kind"] == "query")[
            "provenance"
        ]["target_mapping"]
        target["module"]["binding_status"] = "unknown"
        target["module"]["binding_evidence"] = False
        with self.assertRaisesRegex(ValueError, "backing file"):
            validator.validate_provenance(trace)

        trace = sample_provenance_trace()
        target = next(row for row in trace["rows"] if row["kind"] == "query")[
            "provenance"
        ]["target_mapping"]
        target["module"]["backing_sha256"] = "0" * 64
        target["module"]["binding_file_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "invalid resident backing hash"):
            validator.validate_provenance(trace)

    def test_strict_provenance_rejects_non_x86_target_extent(self) -> None:
        trace = sample_provenance_trace()
        target = next(row for row in trace["rows"] if row["kind"] == "query")[
            "provenance"
        ]["target_mapping"]
        target["extent"] = 0xFFFFFFFFFFFFFFFF
        target["module"]["resident_extent"] = 0xFFFFFFFFFFFFFFFF
        target["module"]["binding_resident_extent"] = 0xFFFFFFFFFFFFFFFF
        with self.assertRaisesRegex(ValueError, "non x86"):
            validator.validate_provenance(trace)

    def test_strict_provenance_rejects_duplicate_acquisition_sequence(self) -> None:
        trace = sample_provenance_trace()
        query = next(row for row in trace["rows"] if row["kind"] == "query")
        query["provenance"]["acquisition_begin_sequence"] = query["sequence"]
        with self.assertRaisesRegex(ValueError, "duplicate provenance acquisition"):
            validator.validate_provenance(trace)


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--trace":
        TRACE_PATH = Path(sys.argv[2])
        del sys.argv[1:3]
    unittest.main()
