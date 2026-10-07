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


def sample_deferred_trace() -> dict:
    trace = sample_trace()
    raw_rows = trace["rows"]
    for row in raw_rows:
        row["engine_generation_known"] = False
        row["engine_generation"] = 0
        row["incomplete"] = True
    admitted = next(
        row
        for row in raw_rows
        if row["kind"] == "pending_event" and row["pending_status"] == "admitted"
    )
    admitted["pending_engine_generation_known"] = False
    admitted["pending_engine_generation"] = 0
    lookup = next(row for row in raw_rows if row["kind"] == "lookup")
    context = next(row for row in raw_rows if row["kind"] == "context_write")
    query = next(row for row in raw_rows if row["kind"] == "query")
    query["sequence"], query["exit_sequence"] = 3, 6
    lookup["sequence"], lookup["exit_sequence"] = 4, 5
    context["sequence"], context["exit_sequence"] = 9, 10
    closed = next(
        row
        for row in raw_rows
        if row["kind"] == "pending_event" and row["pending_status"] == "closed"
    )
    qualified = dict(
        event_complete=True,
        raw_debug_object="0x20",
        event_pid=10,
        event_tid=11,
        raw_generation=1,
        event_index=1,
        engine_generation_known=True,
        engine_generation=2,
    )
    binding = dict(
        identity=qualified,
        kind="engine_binding",
        session_id=1,
        operation_id=6,
        parent_operation_id=0,
        sequence=7,
        exit_sequence=8,
        observer_thread_id=12,
        incomplete=False,
        pre_log_failed=False,
        post_log_failed=False,
        rethrown=False,
        binding_status="bound",
        binding_qualified=True,
        binding_engine_generation=2,
        binding_raw_event_complete=True,
        binding_raw_debug_object="0x20",
        binding_event_pid=10,
        binding_event_tid=11,
        binding_raw_generation=1,
        binding_event_index=1,
        binding_qualified_event_complete=True,
        binding_qualified_engine_generation_known=True,
        binding_qualified_engine_generation=2,
    )
    witness = dict(
        binding_current_thread_known=True,
        binding_current_thread_id=0,
        binding_event_thread_known=True,
        binding_event_thread_id=0,
        binding_cached_thread_known=True,
        binding_cached_thread_id=0,
        binding_current_process_known=True,
        binding_current_process_id=1,
        binding_event_process_known=True,
        binding_event_process_id=1,
        binding_current_system_pid_known=True,
        binding_current_system_pid=10,
        binding_current_system_tid_known=True,
        binding_current_system_tid=11,
    )
    binding.update(witness)
    for key, value in qualified.items():
        binding[key] = value
    for key, value in qualified.items():
        closed[key] = value
        closed["pending_" + key] = value
    closed["sequence"], closed["exit_sequence"] = 11, 12
    context.update(qualified)
    context["incomplete"] = False
    trace["rows"] = [admitted, query, lookup, binding, context, closed]
    return trace


def sample_callback_trace() -> dict:
    trace = sample_deferred_trace()
    admitted = next(
        row
        for row in trace["rows"]
        if row["kind"] == "pending_event" and row["pending_status"] == "admitted"
    )
    query = next(row for row in trace["rows"] if row["kind"] == "query")
    lookup = next(row for row in trace["rows"] if row["kind"] == "lookup")
    binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
    closed = next(
        row
        for row in trace["rows"]
        if row["kind"] == "pending_event" and row["pending_status"] == "closed"
    )

    raw = dict(
        event_complete=True,
        raw_debug_object="0x20",
        event_pid=10,
        event_tid=11,
        raw_generation=1,
        event_index=1,
    )
    qualified = dict(raw, engine_generation_known=True, engine_generation=0x44)

    def header(
        kind: str, operation: int, parent: int, sequence: int, exit_sequence: int
    ) -> dict:
        return dict(
            kind=kind,
            session_id=1,
            operation_id=operation,
            parent_operation_id=parent,
            sequence=sequence,
            exit_sequence=exit_sequence,
            observer_thread_id=12,
            incomplete=True,
            pre_log_failed=False,
            post_log_failed=False,
            rethrown=False,
            incoming_error_known=False,
            returned_error_known=False,
            incoming_last_error=0,
            returned_last_error=0,
            incoming_last_status=0,
            returned_last_status=0,
            engine_generation_known=False,
            engine_generation=0,
            **raw,
        )

    entry = header("callback_entry", 7, 0, 3, 18)
    entry.update(
        callback_operation_id=7,
        callback_kind="breakpoint",
        entry_raw_identity_known=True,
        callback_raw_event_complete=True,
        callback_raw_raw_debug_object="0x20",
        callback_raw_event_pid=10,
        callback_raw_event_tid=11,
        callback_raw_raw_generation=1,
        callback_raw_event_index=1,
        callback_raw_engine_generation_known=False,
        callback_raw_engine_generation=0,
        exit_outcome="completed",
        exit_observed=True,
    )

    acquisition = header("callback_acquisition", 8, 7, 4, 5)
    acquisition.update(
        incoming_error_known=True,
        returned_error_known=True,
        callback_operation_id=7,
        acquisition_operation_id=8,
        callback_kind="breakpoint",
        acquisition_begin_sequence=4,
        acquisition_end_sequence=5,
        acquisition_raw_event_complete=True,
        acquisition_raw_raw_debug_object="0x20",
        acquisition_raw_event_pid=10,
        acquisition_raw_event_tid=11,
        acquisition_raw_raw_generation=1,
        acquisition_raw_event_index=1,
        acquisition_raw_engine_generation_known=False,
        acquisition_raw_engine_generation=0,
        acquisition_rechecked_event_complete=True,
        acquisition_rechecked_raw_debug_object="0x20",
        acquisition_rechecked_event_pid=10,
        acquisition_rechecked_event_tid=11,
        acquisition_rechecked_raw_generation=1,
        acquisition_rechecked_event_index=1,
        acquisition_rechecked_engine_generation_known=False,
        acquisition_rechecked_engine_generation=0,
        query_interface=dict(
            hresult=0,
            output="0x200",
            output_known=True,
            status="succeeded",
            release_attempted=True,
            release_succeeded=True,
            release_threw=False,
            release_result=1,
        ),
        sdk_reads=[
            dict(
                method=method,
                hresult=0,
                output=output,
                output_known=True,
                status="succeeded",
            )
            for method, output in (
                ("GetCurrentThreadId", 7),
                ("GetEventThread", 7),
                ("GetCurrentProcessId", 2),
                ("GetEventProcess", 2),
                ("GetCurrentThreadSystemId", 11),
                ("GetCurrentProcessSystemId", 10),
            )
        ],
        owner=dict(
            serialized_selected_state_access=True,
            retained_source_lifetime=True,
            authority_id=0xA1,
            lifetime_id=0xB2,
            cached_raw_lifecycle_associated=True,
            cached_raw_debug_object="0x20",
            cached_raw_process_id=10,
            cached_raw_thread_id=11,
            cached_raw_generation=1,
            cached_engine_id_known=True,
            cached_engine_id=7,
            lifecycle_token_known=True,
            lifecycle_token=0x44,
        ),
        outcome="accepted",
        binding_eligible=True,
        binding_status="bound",
        binding_attempt_id=9,
        error_restore_attempted=True,
        error_restore_succeeded=True,
        binding_current_thread_known=True,
        binding_current_thread_id=7,
        binding_event_thread_known=True,
        binding_event_thread_id=7,
        binding_cached_thread_known=True,
        binding_cached_thread_id=7,
        binding_current_process_known=True,
        binding_current_process_id=2,
        binding_event_process_known=True,
        binding_event_process_id=2,
        binding_current_system_pid_known=True,
        binding_current_system_pid=10,
        binding_current_system_tid_known=True,
        binding_current_system_tid=11,
    )

    for row in (query, lookup):
        row.update(
            engine_generation_known=True, engine_generation=0x44, incomplete=False
        )
    query["sequence"], query["exit_sequence"] = 8, 11
    lookup["sequence"], lookup["exit_sequence"] = 9, 10
    binding["sequence"], binding["exit_sequence"] = 6, 7
    binding.update(
        callback_operation_id=7,
        callback_acquisition_operation_id=8,
        binding_attempt_id=9,
        engine_generation=0x44,
        binding_engine_generation=0x44,
        binding_qualified_engine_generation=0x44,
        binding_current_thread_id=7,
        binding_event_thread_id=7,
        binding_cached_thread_id=7,
        binding_current_process_id=2,
        binding_event_process_id=2,
        binding_current_system_pid=10,
        binding_current_system_tid=11,
    )
    closed.update(
        engine_generation_known=True, engine_generation=0x44, incomplete=False
    )
    closed["pending_engine_generation_known"] = True
    closed["pending_engine_generation"] = 0x44
    closed["sequence"], closed["exit_sequence"] = 19, 20
    context = next(row for row in trace["rows"] if row["kind"] == "context_write")
    context.update(qualified)
    context["incomplete"] = False
    context["sequence"], context["exit_sequence"] = 13, 14
    trace["rows"] = [
        admitted,
        entry,
        acquisition,
        binding,
        query,
        lookup,
        context,
        closed,
    ]
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

    def test_deferred_binding_keeps_early_query_unqualified(self) -> None:
        trace = sample_deferred_trace()
        self.assertEqual(validator.validate_trace(trace), len(trace["rows"]))
        early = next(row for row in trace["rows"] if row["kind"] == "query")
        self.assertTrue(early["incomplete"])
        self.assertFalse(early["engine_generation_known"])

    def test_callback_trace_accepts_shared_clock_and_full_sdk_evidence(self) -> None:
        trace = sample_callback_trace()
        self.assertEqual(validator.validate_trace(trace), len(trace["rows"]))

    def test_callback_sdk_method_type_is_rejected(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["sdk_reads"][0]["method"] = "GetEventProcess"
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_successful_id_cannot_use_invalid_sentinel(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["sdk_reads"][5]["output"] = 0
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_successful_sdk_read_cannot_have_failure_hresult(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["sdk_reads"][0]["hresult"] = -1
        self.assertRaises(ValueError, validator.validate_trace, trace)

        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["query_interface"]["hresult"] = -1
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_witness_must_match_all_sdk_outputs(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["binding_event_process_id"] = 3
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_owner_lifecycle_tuple_must_match_raw_key(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["owner"]["cached_raw_generation"] = 2
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_kind_and_binding_generation_links_are_strict(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["callback_kind"] = "create_thread"
        self.assertRaises(ValueError, validator.validate_trace, trace)

        trace = sample_callback_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding.update(
            binding_engine_generation=0x45,
            binding_qualified_engine_generation=0x45,
            engine_generation=0x45,
        )
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_bound_acquisition_requires_one_reciprocal_binding_link(
        self,
    ) -> None:
        trace = sample_callback_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding.update(
            callback_operation_id=0,
            callback_acquisition_operation_id=0,
            binding_attempt_id=999,
        )
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_refused_acquisition_cannot_link_to_bound_row(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["outcome"] = "query_interface_refused"
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_accepted_acquisition_cannot_use_foreign_observer_thread(
        self,
    ) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["observer_thread_id"] = 99
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_trace_keeps_context_write_boundary(self) -> None:
        trace = sample_callback_trace()
        trace["rows"] = [row for row in trace["rows"] if row["kind"] != "context_write"]
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_owner_token_must_be_nonzero(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["owner"]["lifecycle_token"] = 0
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_error_pair_restoration_is_required(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["error_restore_succeeded"] = False
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_qi_cleanup_must_release_owned_reference(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["query_interface"]["release_succeeded"] = False
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_callback_refusal_preserves_unqualified_rows(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["outcome"] = "query_interface_refused"
        acquisition["binding_eligible"] = False
        acquisition["binding_status"] = "refused"
        acquisition["query_interface"].update(
            hresult=-2147467262,
            output="0x0",
            output_known=False,
            status="failed",
            release_attempted=False,
            release_succeeded=False,
            release_result=0,
        )
        for read in acquisition["sdk_reads"]:
            read.update(
                output=(
                    0xFFFFFFFF
                    if read["method"]
                    in {
                        "GetCurrentThreadId",
                        "GetEventThread",
                        "GetCurrentProcessId",
                        "GetEventProcess",
                    }
                    else 0
                ),
                hresult=-2147467259,
                output_known=False,
                status="not_attempted",
            )
        for key in (
            "serialized_selected_state_access",
            "retained_source_lifetime",
            "cached_raw_lifecycle_associated",
            "cached_engine_id_known",
            "lifecycle_token_known",
        ):
            acquisition["owner"][key] = False
        acquisition["owner"].update(
            authority_id=0,
            lifetime_id=0,
            cached_engine_id=0xFFFFFFFF,
            lifecycle_token=0,
        )
        for key in (
            "binding_current_thread_known",
            "binding_event_thread_known",
            "binding_cached_thread_known",
            "binding_current_process_known",
            "binding_event_process_known",
            "binding_current_system_pid_known",
            "binding_current_system_tid_known",
        ):
            acquisition[key] = False
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding.update(
            binding_status="refused",
            binding_qualified=False,
            binding_engine_generation=0,
            incomplete=True,
        )
        for row in trace["rows"]:
            if row["kind"] in {"query", "lookup", "context_write", "pending_event"}:
                row["engine_generation_known"] = False
                row["engine_generation"] = 0
                row["incomplete"] = True
                if row["kind"] == "pending_event":
                    row["pending_engine_generation_known"] = False
                    row["pending_engine_generation"] = 0
        self.assertEqual(validator.validate_trace(trace), len(trace["rows"]))

        foreign_refusal = copy.deepcopy(trace)
        foreign_refusal["rows"][2]["observer_thread_id"] = 99
        self.assertEqual(
            validator.validate_trace(foreign_refusal), len(foreign_refusal["rows"])
        )

        malformed = copy.deepcopy(trace)
        malformed_read = malformed["rows"][2]["sdk_reads"][0]
        malformed_read.update(status="failed", hresult=0)
        self.assertRaises(ValueError, validator.validate_trace, malformed)

        malformed = copy.deepcopy(trace)
        malformed_read = malformed["rows"][2]["sdk_reads"][0]
        malformed_read.update(status="invalid_output", hresult=-1)
        self.assertRaises(ValueError, validator.validate_trace, malformed)

        malformed = copy.deepcopy(trace)
        malformed_read = malformed["rows"][2]["sdk_reads"][0]
        malformed_read.update(status="not_attempted", hresult=0)
        self.assertRaises(ValueError, validator.validate_trace, malformed)

        for prefix in ("acquisition_raw", "acquisition_rechecked", ""):
            malformed = copy.deepcopy(trace)
            malformed_acquisition = malformed["rows"][2]
            complete_key = f"{prefix}_event_complete" if prefix else "event_complete"
            known_key = (
                f"{prefix}_engine_generation_known"
                if prefix
                else "engine_generation_known"
            )
            generation_key = (
                f"{prefix}_engine_generation" if prefix else "engine_generation"
            )
            malformed_acquisition[complete_key] = False
            malformed_acquisition[known_key] = True
            malformed_acquisition[generation_key] = 999
            self.assertRaises(ValueError, validator.validate_trace, malformed)

    def test_callback_acquisition_interval_must_match_shared_clock(self) -> None:
        trace = sample_callback_trace()
        acquisition = next(
            row for row in trace["rows"] if row["kind"] == "callback_acquisition"
        )
        acquisition["acquisition_end_sequence"] = 3
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_query_crossing_binding_is_rejected(self) -> None:
        trace = sample_deferred_trace()
        early = next(row for row in trace["rows"] if row["kind"] == "query")
        context = next(row for row in trace["rows"] if row["kind"] == "context_write")
        closed = next(
            row
            for row in trace["rows"]
            if row["kind"] == "pending_event" and row["pending_status"] == "closed"
        )
        early["exit_sequence"] = 9
        context["sequence"], context["exit_sequence"] = 10, 11
        closed["sequence"], closed["exit_sequence"] = 12, 13
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_binding_must_match_raw_tuple(self) -> None:
        trace = sample_deferred_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding["binding_event_index"] = 2
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_binding_rejects_inconsistent_witness(self) -> None:
        trace = sample_deferred_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding["binding_event_process_id"] = 2
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_binding_generation_must_match_qualified_identity(self) -> None:
        trace = sample_deferred_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding["binding_qualified_engine_generation"] = 3
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_close_must_match_bound_identity(self) -> None:
        trace = sample_deferred_trace()
        closed = next(
            row
            for row in trace["rows"]
            if row["kind"] == "pending_event" and row["pending_status"] == "closed"
        )
        closed["pending_engine_generation"] = 3
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_forwarding_keeps_strict_query_fields(self) -> None:
        trace = sample_deferred_trace()
        query = next(row for row in trace["rows"] if row["kind"] == "query")
        query["service_guid_status"] = "null"
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_complete_forwarding_rejects_logging_failure(self) -> None:
        for key in ("pre_log_failed", "post_log_failed", "rethrown"):
            with self.subTest(key=key):
                trace = sample_deferred_trace()
                context = next(
                    row for row in trace["rows"] if row["kind"] == "context_write"
                )
                context[key] = True
                self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_binding_exit_overlap_is_rejected(self) -> None:
        trace = sample_deferred_trace()
        early = next(row for row in trace["rows"] if row["kind"] == "query")
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        context = next(row for row in trace["rows"] if row["kind"] == "context_write")
        closed = next(
            row
            for row in trace["rows"]
            if row["kind"] == "pending_event" and row["pending_status"] == "closed"
        )
        binding["exit_sequence"] = 9
        early["exit_sequence"] = 10
        context["sequence"], context["exit_sequence"] = 11, 12
        closed["sequence"], closed["exit_sequence"] = 13, 14
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_raw_only_identity_fields_are_typed(self) -> None:
        for key, value in (
            ("engine_generation_known", "bogus"),
            ("engine_generation", "invalid"),
        ):
            with self.subTest(key=key):
                trace = sample_deferred_trace()
                early = next(row for row in trace["rows"] if row["kind"] == "query")
                early[key] = value
                self.assertRaises(ValueError, validator.validate_trace, trace)
        trace = sample_deferred_trace()
        admitted = next(
            row
            for row in trace["rows"]
            if row["kind"] == "pending_event" and row["pending_status"] == "admitted"
        )
        admitted["pending_engine_generation"] = "invalid"
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_bound_qualification_flag_is_boolean(self) -> None:
        trace = sample_deferred_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        binding["binding_qualified"] = "yes"
        self.assertRaises(ValueError, validator.validate_trace, trace)

    def test_deferred_nonbound_binding_cannot_claim_qualification(self) -> None:
        trace = sample_deferred_trace()
        binding = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        context = next(row for row in trace["rows"] if row["kind"] == "context_write")
        closed = next(
            row
            for row in trace["rows"]
            if row["kind"] == "pending_event" and row["pending_status"] == "closed"
        )
        binding["binding_status"] = "duplicate"
        binding["binding_qualified"] = True
        binding["incomplete"] = True
        binding["sequence"], binding["exit_sequence"] = 9, 10
        context["sequence"], context["exit_sequence"] = 11, 12
        closed["sequence"], closed["exit_sequence"] = 13, 14
        with self.assertRaisesRegex(
            ValueError, "non-bound binding claims qualification"
        ):
            validator.validate_trace(trace)

        trace = sample_deferred_trace()
        bound = next(row for row in trace["rows"] if row["kind"] == "engine_binding")
        stale = bound.copy()
        stale["binding_status"] = "stale"
        stale["binding_qualified"] = True
        stale["incomplete"] = True
        stale["operation_id"] = 7
        stale["sequence"], stale["exit_sequence"] = 15, 16
        trace["rows"].append(stale)
        with self.assertRaisesRegex(
            ValueError, "non-bound binding claims qualification"
        ):
            validator.validate_trace(trace)

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
