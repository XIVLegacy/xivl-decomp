# SPDX-License-Identifier: AGPL-3.0-or-later
"""Check complete synthetic forwarding traces; live traces are unsupported."""

import argparse
import json
from pathlib import Path
import re

CALLBACK_UNSET_HRESULT = -2147467259


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
    raw_identity = _raw_identity(row, prefix)
    if row.get(prefix + "engine_generation_known") is not True:
        raise ValueError("unknown raw object or engine generation")
    engine = _integer(row, prefix + "engine_generation", 1)
    return raw_identity + (engine,)


def _raw_identity(row: dict, prefix: str = "") -> tuple:
    if row.get(prefix + "event_complete") is not True:
        raise ValueError("unknown raw event identity")
    raw = _address(row, prefix + "raw_debug_object")
    pid = _integer(row, prefix + "event_pid", 1, 0xFFFFFFFF)
    tid = _integer(row, prefix + "event_tid", 1, 0xFFFFFFFF)
    generation = _integer(row, prefix + "raw_generation", 1)
    index = _integer(row, prefix + "event_index", 0)
    if not raw:
        raise ValueError("unknown raw object")
    if type(row.get(prefix + "engine_generation_known")) is not bool:
        raise ValueError("unknown engine generation state")
    _integer(row, prefix + "engine_generation", 0)
    return raw, pid, tid, generation, index


def _binding_raw_identity(row: dict) -> tuple:
    if row.get("binding_raw_event_complete") is not True:
        raise ValueError("unknown binding raw event")
    raw = _address(row, "binding_raw_debug_object")
    pid = _integer(row, "binding_event_pid", 1, 0xFFFFFFFF)
    tid = _integer(row, "binding_event_tid", 1, 0xFFFFFFFF)
    generation = _integer(row, "binding_raw_generation", 1)
    index = _integer(row, "binding_event_index", 0)
    if not raw:
        raise ValueError("unknown binding raw object")
    return raw, pid, tid, generation, index


def _callback_identity(row: dict, prefix: str, require_complete: bool = True) -> tuple:
    field_prefix = prefix + "_" if prefix else ""
    complete = row.get(field_prefix + "event_complete")
    if type(complete) is not bool:
        raise ValueError(f"unknown callback identity state: {prefix}")
    raw = _address(row, field_prefix + "raw_debug_object")
    pid = _integer(
        row,
        field_prefix + "event_pid",
        1 if complete else 0,
        0xFFFFFFFF,
    )
    tid = _integer(
        row,
        field_prefix + "event_tid",
        1 if complete else 0,
        0xFFFFFFFF,
    )
    generation = _integer(row, field_prefix + "raw_generation", 1 if complete else 0)
    index = _integer(row, field_prefix + "event_index", 0)
    engine_known = row.get(field_prefix + "engine_generation_known")
    if type(engine_known) is not bool:
        raise ValueError(f"unknown callback engine identity state: {prefix}")
    engine_generation = _integer(row, field_prefix + "engine_generation", 0)
    if engine_known is not False or engine_generation != 0:
        raise ValueError(f"callback identity token is not unknown: {prefix}")
    if not complete and not require_complete:
        return raw, pid, tid, generation, index
    if not raw or not complete:
        raise ValueError(f"incomplete callback raw identity: {prefix}")
    return raw, pid, tid, generation, index


def _validate_callback_sdk_read(read: dict, expected_method: str) -> None:
    if not isinstance(read, dict) or read.get("method") != expected_method:
        raise ValueError("callback SDK method order mismatch")
    _integer(read, "hresult", -0x80000000, 0x7FFFFFFF)
    _integer(read, "output", 0, 0xFFFFFFFF)
    if type(read.get("output_known")) is not bool:
        raise ValueError("unknown callback SDK output state")
    if read.get("status") not in {
        "not_attempted",
        "succeeded",
        "failed",
        "invalid_output",
        "exception",
    }:
        raise ValueError("unknown callback SDK read status")
    if read["status"] == "succeeded" and read["output_known"] is not True:
        raise ValueError("successful callback SDK read has unknown output")
    if read["status"] != "succeeded" and read["output_known"] is True:
        raise ValueError("failed callback SDK read claims known output")
    if read["status"] == "succeeded" and read["hresult"] < 0:
        raise ValueError("successful callback SDK read has a failure HRESULT")
    engine_method = expected_method in {
        "GetCurrentThreadId",
        "GetEventThread",
        "GetCurrentProcessId",
        "GetEventProcess",
    }
    invalid_output = 0xFFFFFFFF if engine_method else 0
    if read["status"] == "failed" and read["hresult"] >= 0:
        raise ValueError("failed callback SDK read has a success HRESULT")
    if read["status"] == "invalid_output" and (
        read["hresult"] < 0 or read["output"] != invalid_output
    ):
        raise ValueError("invalid callback SDK output has inconsistent evidence")
    if read["status"] == "not_attempted" and (
        read["hresult"] != CALLBACK_UNSET_HRESULT or read["output"] != invalid_output
    ):
        raise ValueError("unattempted callback SDK read has observed evidence")
    if read["status"] == "exception" and read["hresult"] != CALLBACK_UNSET_HRESULT:
        raise ValueError("exception callback SDK read has a returned HRESULT")
    if read["status"] == "succeeded":
        if engine_method and read["output"] == 0xFFFFFFFF:
            raise ValueError("successful callback engine ID is DEBUG_ANY_ID")
        if not engine_method and read["output"] == 0:
            raise ValueError("successful callback system ID is zero")


def _validate_callback_owner(owner: dict, require_complete: bool) -> None:
    if not isinstance(owner, dict):
        raise ValueError("missing callback owner evidence")
    for key in (
        "serialized_selected_state_access",
        "retained_source_lifetime",
        "cached_raw_lifecycle_associated",
        "cached_engine_id_known",
        "lifecycle_token_known",
    ):
        if type(owner.get(key)) is not bool:
            raise ValueError(f"unknown callback owner state: {key}")
    authority = _integer(owner, "authority_id", 0)
    lifetime = _integer(owner, "lifetime_id", 0)
    cached_raw = _address(owner, "cached_raw_debug_object")
    cached_raw_pid = _integer(owner, "cached_raw_process_id", 0, 0xFFFFFFFF)
    cached_raw_tid = _integer(owner, "cached_raw_thread_id", 0, 0xFFFFFFFF)
    cached_raw_generation = _integer(owner, "cached_raw_generation", 0)
    cached_engine = _integer(owner, "cached_engine_id", 0, 0xFFFFFFFF)
    token = _integer(owner, "lifecycle_token", 0)
    if require_complete and (
        not owner["serialized_selected_state_access"]
        or not owner["retained_source_lifetime"]
        or not authority
        or not lifetime
        or not owner["cached_raw_lifecycle_associated"]
        or not cached_raw
        or not cached_raw_pid
        or not cached_raw_tid
        or not cached_raw_generation
        or not owner["cached_engine_id_known"]
        or cached_engine == 0xFFFFFFFF
        or not owner["lifecycle_token_known"]
        or not token
    ):
        raise ValueError("incomplete callback owner evidence")


def _validate_callback_entry(row: dict) -> tuple[int, tuple | None]:
    operation = _integer(row, "operation_id", 1)
    if _integer(row, "callback_operation_id", 1) != operation:
        raise ValueError("callback operation identifier mismatch")
    kind = row.get("callback_kind")
    if not isinstance(kind, str) or not kind:
        raise ValueError("missing callback kind")
    if type(row.get("entry_raw_identity_known")) is not bool:
        raise ValueError("unknown callback entry raw state")
    if type(row.get("exit_observed")) is not bool:
        raise ValueError("unknown callback exit state")
    if row.get("exit_outcome") not in {
        "not_attempted",
        "completed",
        "incomplete",
        "exception",
    }:
        raise ValueError("unknown callback exit outcome")
    if row["exit_observed"]:
        if row["exit_outcome"] == "not_attempted":
            raise ValueError("callback exit has no outcome")
        if row["exit_sequence"] <= row["sequence"]:
            raise ValueError("callback exit interval mismatch")
    else:
        if row["exit_sequence"] != 0 or row["exit_outcome"] != "not_attempted":
            raise ValueError("missing callback exit was rewritten")
        if row["incomplete"] is not True:
            raise ValueError("missing callback exit is complete")
    if row["entry_raw_identity_known"]:
        raw = _callback_identity(row, "callback_raw")
        if (
            _raw_identity(row) != raw
            or row.get("engine_generation_known") is not False
            or _integer(row, "engine_generation") != 0
        ):
            raise ValueError("callback entry rewrote raw identity")
        _validate_callback_dispatch_fields(row)
        return operation, raw
    _validate_callback_dispatch_fields(row)
    return operation, None


def _validate_callback_dispatch_fields(row: dict) -> None:
    marker = row.get("dispatch_marker", False)
    if type(marker) is not bool:
        raise ValueError("unknown callback dispatch marker")
    if not marker:
        return
    _integer(row, "sequence", 1)
    _integer(row, "exit_sequence", 0)
    for key in ("incomplete", "exit_observed"):
        if type(row.get(key)) is not bool:
            raise ValueError(f"unknown callback dispatch row state: {key}")
    if row.get("dispatch_phase") not in {"before_delegate", "after_delegate"}:
        raise ValueError("unknown callback dispatch phase")
    identity = row.get("delegate_identity")
    if identity not in {
        "IDebugEventCallbacks::Breakpoint",
        "IDebugEventCallbacks::CreateThread",
    }:
        raise ValueError("unknown callback delegate identity")
    if (
        row["dispatch_phase"] == "before_delegate"
        and identity != "IDebugEventCallbacks::Breakpoint"
    ):
        raise ValueError("callback dispatch phase does not match delegate")
    if (
        row["dispatch_phase"] == "after_delegate"
        and identity != "IDebugEventCallbacks::CreateThread"
    ):
        raise ValueError("callback dispatch phase does not match delegate")
    expected_kind = (
        "breakpoint"
        if identity == "IDebugEventCallbacks::Breakpoint"
        else "create_thread"
    )
    if row.get("callback_kind") != expected_kind:
        raise ValueError("callback dispatch delegate does not match callback kind")
    bool_fields = (
        "delegate_completion_known",
        "delegate_hresult_known",
        "delegate_threw",
        "provider_attempted",
        "provider_succeeded",
        "provider_threw",
        "owner_evidence_complete",
        "capture_attempted",
        "capture_completed",
        "binding_attempted",
        "binding_succeeded",
        "foreign_thread_refused",
        "reentry_refused",
        "invalid_configuration_refused",
        "capacity_refused",
        "error_restore_attempted",
        "error_restore_succeeded",
        "error_restore_prerequisite_attempted",
        "error_restore_prerequisite_succeeded",
        "error_restore_late_failure",
        "dispatch_incoming_error_known",
        "dispatch_returned_error_known",
        "callback_arguments_known",
    )
    if any(type(row.get(key)) is not bool for key in bool_fields):
        raise ValueError("unknown callback dispatch state")
    _integer(row, "delegate_begin_sequence", 0)
    _integer(row, "delegate_end_sequence", 0)
    _integer(row, "dispatch_acquisition_operation_id", 0)
    _integer(row, "dispatch_binding_attempt_id", 0)
    _address(row, "breakpoint_pointer")
    for key in (
        "create_thread_handle",
        "create_thread_data_offset",
        "create_thread_start_offset",
        "dispatch_incoming_last_error",
        "dispatch_returned_last_error",
    ):
        _integer(row, key, 0)
    for key in (
        "dispatch_incoming_last_status",
        "dispatch_returned_last_status",
        "delegate_hresult",
    ):
        _integer(row, key, -0x80000000, 0x7FFFFFFF)
    if not row["callback_arguments_known"]:
        raise ValueError("callback dispatch arguments are unknown")
    if (
        identity == "IDebugEventCallbacks::Breakpoint"
        and row["breakpoint_pointer"] == 0
    ):
        raise ValueError("breakpoint dispatch has null callback argument")
    if row["delegate_threw"] and row["delegate_completion_known"]:
        raise ValueError("delegate exception claims completion")
    if row["delegate_completion_known"] and not row["delegate_hresult_known"]:
        raise ValueError("completed delegate lacks HRESULT")
    if row["delegate_hresult_known"] and not row["delegate_completion_known"]:
        raise ValueError("delegate HRESULT lacks completion")
    if row["exit_observed"] and row["exit_outcome"] == "exception":
        if not row["delegate_threw"] or row["delegate_completion_known"]:
            raise ValueError("exception callback exit contradicts delegate state")
    elif row["exit_observed"] and row["exit_outcome"] == "completed":
        if row["delegate_threw"] or not row["delegate_completion_known"]:
            raise ValueError("completed callback exit contradicts delegate state")
    if row["provider_succeeded"] and not row["provider_attempted"]:
        raise ValueError("provider success lacks attempt")
    if row["provider_threw"] and not row["provider_attempted"]:
        raise ValueError("provider exception lacks attempt")
    if row["provider_threw"] and row["provider_succeeded"]:
        raise ValueError("provider exception claims success")
    if row["owner_evidence_complete"] and not row["provider_succeeded"]:
        raise ValueError("owner evidence lacks provider success")
    if row["binding_succeeded"] and not row["binding_attempted"]:
        raise ValueError("binding success lacks attempt")
    if row["binding_attempted"] and not row["dispatch_acquisition_operation_id"]:
        raise ValueError("callback binding lacks acquisition link")
    if row["binding_attempted"] and not row["capture_completed"]:
        raise ValueError("binding attempt lacks capture completion")
    if row["capture_completed"] and not row["capture_attempted"]:
        raise ValueError("capture completion lacks attempt")
    if row["capture_completed"] and not row["dispatch_acquisition_operation_id"]:
        raise ValueError("callback capture lacks acquisition link")
    if row["error_restore_succeeded"] and not row["error_restore_attempted"]:
        raise ValueError("error restore success lacks attempt")
    if (
        row["error_restore_prerequisite_succeeded"]
        and not row["error_restore_prerequisite_attempted"]
    ):
        raise ValueError("error restore prerequisite success lacks attempt")
    if row["error_restore_late_failure"]:
        if (
            not row["error_restore_attempted"]
            or not row["error_restore_prerequisite_attempted"]
            or not row["error_restore_prerequisite_succeeded"]
            or row["error_restore_succeeded"]
            or not row["capture_completed"]
            or not row["dispatch_acquisition_operation_id"]
        ):
            raise ValueError(
                "late error restore failure lacks post-acquisition evidence"
            )
    if (
        not row["dispatch_incoming_error_known"]
        and row["error_restore_attempted"]
        and not row["dispatch_returned_error_known"]
    ):
        raise ValueError("error restore attempted without known error pair")
    if (row["delegate_begin_sequence"] == 0) != (row["delegate_end_sequence"] == 0):
        raise ValueError("callback delegate interval is incomplete")
    if row["delegate_begin_sequence"] == 0:
        if not row["incomplete"]:
            raise ValueError("complete dispatch lacks delegate interval")
    elif row["delegate_end_sequence"] <= row["delegate_begin_sequence"]:
        raise ValueError("callback delegate interval is reversed")
    if row["delegate_begin_sequence"] and row["delegate_end_sequence"]:
        if (
            not row["sequence"]
            < row["delegate_begin_sequence"]
            < row["delegate_end_sequence"]
        ):
            raise ValueError("delegate interval escapes callback entry")
        if (
            row["exit_observed"]
            and not row["delegate_end_sequence"] < row["exit_sequence"]
        ):
            raise ValueError("delegate interval escapes callback exit")
    if not row["incomplete"]:
        if (
            not row["delegate_completion_known"]
            or not row["delegate_hresult_known"]
            or row["delegate_threw"]
            or not row["provider_attempted"]
            or not row["provider_succeeded"]
            or not row["owner_evidence_complete"]
            or not row["capture_attempted"]
            or not row["capture_completed"]
            or not row["binding_attempted"]
            or not row["binding_succeeded"]
            or not row["error_restore_attempted"]
            or not row["error_restore_succeeded"]
            or not row["error_restore_prerequisite_attempted"]
            or not row["error_restore_prerequisite_succeeded"]
            or row["error_restore_late_failure"]
            or not row["dispatch_incoming_error_known"]
            or not row["dispatch_returned_error_known"]
            or row["foreign_thread_refused"]
            or row["reentry_refused"]
            or row["invalid_configuration_refused"]
            or row["capacity_refused"]
        ):
            raise ValueError("complete callback dispatch lacks coverage evidence")


def _validate_callback_observation(
    row: dict, raw: tuple, require_complete: bool = True
) -> None:
    keys = (
        "binding_current_thread_known",
        "binding_event_thread_known",
        "binding_cached_thread_known",
        "binding_current_process_known",
        "binding_event_process_known",
        "binding_current_system_pid_known",
        "binding_current_system_tid_known",
    )
    if any(type(row.get(key)) is not bool for key in keys):
        raise ValueError("unknown callback SDK witness state")
    if not require_complete:
        for key in (
            "binding_current_thread_id",
            "binding_event_thread_id",
            "binding_cached_thread_id",
            "binding_current_process_id",
            "binding_event_process_id",
            "binding_current_system_pid",
            "binding_current_system_tid",
        ):
            _integer(row, key, 0, 0xFFFFFFFF)
        return
    if any(row.get(key) is not True for key in keys):
        raise ValueError("incomplete callback SDK witness")
    current_thread = _integer(row, "binding_current_thread_id", 0, 0xFFFFFFFF)
    event_thread = _integer(row, "binding_event_thread_id", 0, 0xFFFFFFFF)
    cached_thread = _integer(row, "binding_cached_thread_id", 0, 0xFFFFFFFF)
    current_process = _integer(row, "binding_current_process_id", 0, 0xFFFFFFFF)
    event_process = _integer(row, "binding_event_process_id", 0, 0xFFFFFFFF)
    if (
        0xFFFFFFFF
        in {current_thread, event_thread, cached_thread, current_process, event_process}
        or current_thread != event_thread
        or current_thread != cached_thread
        or current_process != event_process
    ):
        raise ValueError("inconsistent callback SDK witness")
    if _integer(row, "binding_current_system_pid", 1, 0xFFFFFFFF) != raw[1]:
        raise ValueError("callback SDK system PID mismatch")
    if _integer(row, "binding_current_system_tid", 1, 0xFFFFFFFF) != raw[2]:
        raise ValueError("callback SDK system TID mismatch")


def _validate_callback_acquisition(
    row: dict, entries: dict[int, dict]
) -> tuple[int, tuple, tuple]:
    operation = _integer(row, "operation_id", 1)
    callback_operation = _integer(row, "callback_operation_id", 1)
    if _integer(row, "acquisition_operation_id", 1) != operation:
        raise ValueError("callback acquisition identifier mismatch")
    if _integer(row, "parent_operation_id", 1) != callback_operation:
        raise ValueError("callback acquisition parent mismatch")
    if callback_operation not in entries:
        raise ValueError("callback acquisition has no entry")
    kind = row.get("callback_kind")
    if not isinstance(kind, str) or not kind:
        raise ValueError("missing callback acquisition kind")
    outcome = row.get("outcome")
    allow_incomplete_identity = outcome != "accepted" and row.get("incomplete") is True
    begin = _integer(row, "acquisition_begin_sequence", 1)
    end = _integer(row, "acquisition_end_sequence", begin + 1)
    if row["sequence"] != begin or row["exit_sequence"] != end or not begin < end:
        raise ValueError("callback acquisition interval mismatch")
    raw = _callback_identity(
        row, "acquisition_raw", require_complete=not allow_incomplete_identity
    )
    rechecked = _callback_identity(
        row, "acquisition_rechecked", require_complete=not allow_incomplete_identity
    )
    header_raw = _callback_identity(
        row, "", require_complete=not allow_incomplete_identity
    )
    if (raw != rechecked and outcome != "changed_raw_key") or header_raw != raw:
        raise ValueError("callback acquisition raw key mismatch")
    entry = entries[callback_operation]
    if row.get("callback_kind") != entry.get("callback_kind"):
        raise ValueError("callback acquisition kind mismatch")
    if row.get("outcome") == "accepted" and row.get("observer_thread_id") != entry.get(
        "observer_thread_id"
    ):
        raise ValueError("accepted callback acquisition has foreign observer thread")
    if entry.get("entry_raw_identity_known"):
        entry_raw = _callback_identity(entry, "callback_raw")
        if raw != entry_raw:
            raise ValueError("callback acquisition entry raw mismatch")
    query = row.get("query_interface")
    if not isinstance(query, dict):
        raise ValueError("missing callback QueryInterface result")
    _integer(query, "hresult", -0x80000000, 0x7FFFFFFF)
    query_output = _address(query, "output")
    for key in (
        "output_known",
        "release_attempted",
        "release_succeeded",
        "release_threw",
    ):
        if type(query.get(key)) is not bool:
            raise ValueError(f"unknown callback QueryInterface state: {key}")
    if query.get("status") not in {
        "not_attempted",
        "succeeded",
        "failed",
        "invalid_output",
        "exception",
    }:
        raise ValueError("unknown callback QueryInterface status")
    _integer(query, "release_result", 0, 0xFFFFFFFF)
    if query["release_succeeded"] and query["release_threw"]:
        raise ValueError("callback QueryInterface cleanup has conflicting state")
    if (query["release_succeeded"] or query["release_threw"]) and not query[
        "release_attempted"
    ]:
        raise ValueError("callback QueryInterface cleanup was not attempted")
    if query["status"] == "succeeded" and (
        query["output_known"] is not True
        or query_output == 0
        or query["release_attempted"] is not True
    ):
        raise ValueError("successful QueryInterface lacks owned reference evidence")
    if query["status"] == "succeeded" and query["hresult"] < 0:
        raise ValueError("successful QueryInterface has a failure HRESULT")
    if query["status"] != "succeeded" and query["output_known"] is True:
        raise ValueError("failed QueryInterface claims known interface")
    if query["status"] == "failed" and query["hresult"] >= 0:
        raise ValueError("failed QueryInterface has a success HRESULT")
    if query["status"] == "invalid_output" and (
        query["hresult"] < 0 or query_output != 0
    ):
        raise ValueError("invalid QueryInterface output has inconsistent evidence")
    if query["status"] == "not_attempted" and (
        query["hresult"] != CALLBACK_UNSET_HRESULT
        or query_output != 0
        or query["release_attempted"]
    ):
        raise ValueError("unattempted QueryInterface has observed evidence")
    if query["status"] == "exception" and (
        query["hresult"] != CALLBACK_UNSET_HRESULT or query["release_attempted"]
    ):
        raise ValueError("exception QueryInterface has cleanup or HRESULT evidence")
    methods = (
        "GetCurrentThreadId",
        "GetEventThread",
        "GetCurrentProcessId",
        "GetEventProcess",
        "GetCurrentThreadSystemId",
        "GetCurrentProcessSystemId",
    )
    reads = row.get("sdk_reads")
    if not isinstance(reads, list) or len(reads) != len(methods):
        raise ValueError("callback SDK read set is incomplete")
    for read, method in zip(reads, methods):
        _validate_callback_sdk_read(read, method)
    _validate_callback_owner(row.get("owner"), row.get("outcome") == "accepted")
    if row.get("outcome") not in {
        "not_attempted",
        "accepted",
        "missing_callback",
        "missing_raw_key",
        "changed_raw_key",
        "missing_owner_evidence",
        "changed_owner_evidence",
        "query_interface_refused",
        "getter_refused",
        "invalid_output",
        "exception",
        "reference_cleanup_failed",
        "binding_refused",
        "overflow",
    }:
        raise ValueError("unknown callback acquisition outcome")
    if type(row.get("binding_eligible")) is not bool:
        raise ValueError("unknown callback binding eligibility")
    for key in ("error_restore_attempted", "error_restore_succeeded"):
        if type(row.get(key)) is not bool:
            raise ValueError(f"unknown callback error restoration state: {key}")
    if row.get("binding_status") not in {
        "bound",
        "missing",
        "changed",
        "stale",
        "already_bound",
        "conflict",
        "duplicate",
        "incomplete_evidence",
        "continuation_closed",
        "refused",
        "overflow",
    }:
        raise ValueError("unknown callback binding status")
    _integer(row, "binding_attempt_id", 0)
    _validate_callback_observation(
        row,
        raw,
        row.get("binding_eligible") is True
        or row.get("outcome") == "accepted"
        or row.get("binding_status") == "bound",
    )
    owner = row["owner"]
    if (
        row.get("outcome") in {"accepted", "binding_refused"}
        or row.get("binding_status") == "bound"
    ):
        if (
            _address(owner, "cached_raw_debug_object"),
            _integer(owner, "cached_raw_process_id"),
            _integer(owner, "cached_raw_thread_id"),
            _integer(owner, "cached_raw_generation"),
        ) != raw[:4]:
            raise ValueError(
                "callback owner lifecycle does not match acquisition raw key"
            )
    if row.get("outcome") == "accepted":
        sdk_outputs = [read["output"] for read in reads]
        witness_outputs = [
            _integer(row, "binding_current_thread_id"),
            _integer(row, "binding_event_thread_id"),
            _integer(row, "binding_current_process_id"),
            _integer(row, "binding_event_process_id"),
            _integer(row, "binding_current_system_tid"),
            _integer(row, "binding_current_system_pid"),
        ]
        if sdk_outputs != witness_outputs:
            raise ValueError("callback SDK reads do not match binding witness")
        if _integer(row, "binding_cached_thread_id") != _integer(
            owner, "cached_engine_id"
        ):
            raise ValueError("callback owner engine ID does not match binding witness")
        if row.get("binding_status") == "bound":
            if not _integer(row, "binding_attempt_id"):
                raise ValueError("bound callback acquisition lacks binding receipt")
        elif row.get("binding_status") != "refused" or _integer(
            row, "binding_attempt_id"
        ):
            raise ValueError(
                "accepted callback acquisition has inconsistent binding state"
            )
    if row.get("outcome") != "accepted" and row.get("binding_status") == "bound":
        raise ValueError("refused callback acquisition links to bound state")
    if row.get("outcome") == "accepted":
        if row.get("binding_eligible") is not True:
            raise ValueError("accepted callback acquisition is ineligible")
        if (
            query["status"] != "succeeded"
            or query["release_succeeded"] is not True
            or not all(
                read["status"] == "succeeded" and read["output_known"] for read in reads
            )
        ):
            raise ValueError("accepted callback acquisition has failed SDK evidence")
        for key in ("incoming_error_known", "returned_error_known"):
            if row.get(key) is not True:
                raise ValueError("accepted callback acquisition has unknown error pair")
        if (
            row.get("error_restore_attempted") is not True
            or row.get("error_restore_succeeded") is not True
        ):
            raise ValueError("accepted callback acquisition lacks error restoration")
    return callback_operation, raw, rechecked


def _guid(row: dict, key: str) -> str:
    value = row.get(key)
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-fA-F]{32}", value):
        raise ValueError(f"invalid GUID bytes: {key}")
    return value


TRANSLATION_SERVICE_GUID = "c07d98ae247d334ca5a6312d96192c8e"
TRANSLATION_IID = "2c235ebe4b1d8349a520383da865da1c"
X86_ADDRESS_LIMIT = 1 << 32


def _provenance_identity(row: dict) -> tuple:
    return _identity(row)


def _mapping_geometry(mapping: dict, label: str) -> tuple[int, int]:
    base = _address(mapping, "base")
    extent = _integer(mapping, "extent", 1)
    if base == 0 or base + extent > X86_ADDRESS_LIMIT:
        raise ValueError(f"non x86 provenance mapping: {label}")
    return base, extent


def _mapping_contains(mapping: dict, address: int, label: str) -> None:
    if not isinstance(mapping, dict):
        raise ValueError(f"missing provenance mapping: {label}")
    if mapping.get("status") != "read":
        raise ValueError(f"unread provenance mapping: {label}")
    if mapping.get("kind") not in {"allocation", "image"}:
        raise ValueError(f"unknown claimed provenance mapping: {label}")
    base, extent = _mapping_geometry(mapping, label)
    if address < base or address - base >= extent:
        raise ValueError(f"provenance address outside mapping: {label}")


def _object_mapping(mapping: dict, address: int, label: str) -> None:
    if not isinstance(mapping, dict):
        raise ValueError(f"missing provenance mapping: {label}")
    if (
        mapping.get("status") == "not_attempted"
        and mapping.get("kind") == "unknown"
        and mapping.get("base") == "0x0"
        and mapping.get("extent") == 0
        and mapping.get("executable") is False
    ):
        return
    _mapping_contains(mapping, address, label)


def _validate_target_mapping(mapping: dict, address: int, lifetime_id: int) -> None:
    _mapping_contains(mapping, address, "target")
    if mapping.get("kind") != "image" or mapping.get("executable") is not True:
        raise ValueError("target is not an executable image mapping")
    module = mapping.get("module")
    if not isinstance(module, dict) or module.get("mapping_status") != "read":
        raise ValueError("target resident mapping is unread")
    resident_base = _address(module, "resident_base")
    resident_extent = _integer(module, "resident_extent", 1)
    if resident_base == 0 or resident_base + resident_extent > X86_ADDRESS_LIMIT:
        raise ValueError("non x86 target resident mapping")
    if resident_base != _address(mapping, "base") or resident_extent != _integer(
        mapping, "extent", 1
    ):
        raise ValueError("target resident mapping mismatch")
    path = module.get("resident_path")
    architecture = module.get("architecture")
    if not isinstance(path, str) or not path or architecture not in {"PE32", "I386"}:
        raise ValueError("incomplete resident module identity")
    _integer(module, "backing_file_size", 1)
    digest = module.get("backing_sha256")
    if (
        not isinstance(digest, str)
        or not re.fullmatch(r"[0-9a-fA-F]{64}", digest)
        or not any(character != "0" for character in digest)
    ):
        raise ValueError("invalid resident backing hash")
    if _address(module, "binding_resident_base") != resident_base:
        raise ValueError("resident binding base mismatch")
    if _integer(module, "binding_resident_extent", 1) != resident_extent:
        raise ValueError("resident binding extent mismatch")
    if _integer(module, "binding_file_size", 1) != module["backing_file_size"]:
        raise ValueError("resident binding size mismatch")
    binding_digest = module.get("binding_file_sha256")
    if (
        not isinstance(binding_digest, str)
        or not re.fullmatch(r"[0-9a-fA-F]{64}", binding_digest)
        or not any(character != "0" for character in binding_digest)
        or binding_digest.lower() != digest.lower()
    ):
        raise ValueError("resident binding hash mismatch")
    if (
        not isinstance(module.get("binding_authority_id"), str)
        or not module["binding_authority_id"]
    ):
        raise ValueError("missing resident binding authority")
    if (
        not isinstance(module.get("binding_mechanism"), str)
        or not module["binding_mechanism"]
    ):
        raise ValueError("missing resident binding mechanism")
    if (
        module.get("binding_status") != "bound"
        or module.get("binding_evidence") is not True
    ):
        raise ValueError("resident image is not bound to its backing file")
    if _integer(module, "binding_lifetime_id", 1) != lifetime_id:
        raise ValueError("resident binding lifetime mismatch")


def _validate_query_provenance(row: dict) -> None:
    evidence = row.get("provenance")
    if not isinstance(evidence, dict) or evidence.get("status") != "accepted":
        raise ValueError("missing accepted query provenance")
    if evidence.get("output_complete") is not True:
        raise ValueError("incomplete query output provenance")
    if _integer(evidence, "session_id", 1) != _integer(row, "session_id", 1):
        raise ValueError("query provenance session mismatch")
    if _integer(evidence, "operation_id", 1) != _integer(row, "operation_id", 1):
        raise ValueError("query provenance operation mismatch")
    if _provenance_identity(evidence) != _provenance_identity(row):
        raise ValueError("query provenance event mismatch")
    for key in (
        "returned_interface",
        "vtable",
        "slot_plus_10_address",
        "slot_plus_10_target",
    ):
        if _address(evidence, key) != _address(row, key):
            raise ValueError(f"query provenance address mismatch: {key}")
    begin = _integer(evidence, "acquisition_begin_sequence", 1)
    end = _integer(evidence, "acquisition_end_sequence", begin + 1)
    if not row["sequence"] < begin < end < row["exit_sequence"]:
        raise ValueError("query provenance acquisition interval mismatch")
    if _integer(evidence, "lifetime_id", 1) and evidence.get("lifetime") == "retained":
        pass
    else:
        raise ValueError("query provenance lifetime is not retained")
    if evidence.get("coherence") != "coherent":
        raise ValueError("query provenance is not coherent")
    _object_mapping(
        evidence.get("interface_mapping"),
        _address(row, "returned_interface"),
        "interface",
    )
    _object_mapping(evidence.get("vtable_mapping"), _address(row, "vtable"), "vtable")
    _validate_target_mapping(
        evidence.get("target_mapping"),
        _address(row, "slot_plus_10_target"),
        _integer(evidence, "lifetime_id", 1),
    )


def validate_provenance(trace: object) -> int:
    """Validate selected query provenance while keeping native qualification unsupported."""
    validate_trace(trace)
    rows = trace["rows"]
    selected = []
    for row in rows:
        if row.get("kind") != "query":
            continue
        if (
            row.get("service_guid", "").lower() != TRANSLATION_SERVICE_GUID
            or row.get("iid", "").lower() != TRANSLATION_IID
        ):
            continue
        _validate_query_provenance(row)
        selected.append(row)
    if not selected:
        raise ValueError("no qualified translation query provenance")
    return len(selected)


def _validate_deferred_operation(
    row: dict, parent_id: int, operations: dict[int, dict], pending_raw: tuple
) -> None:
    """Validate forwarding payloads even when raw identity is still unbound."""
    if row.get("call_completed") is not True or not _address(row, "original_target"):
        raise ValueError("missing original completion")
    if (
        row.get("incoming_error_known") is not True
        or row.get("returned_error_known") is not True
    ):
        raise ValueError("unknown error fields")
    for key in ("incoming_last_error", "returned_last_error"):
        _integer(row, key, 0, 0xFFFFFFFF)
    for key in ("incoming_last_status", "returned_last_status"):
        _integer(row, key, -0x80000000, 0x7FFFFFFF)
    for key in ("pre_log_failed", "post_log_failed", "rethrown"):
        if row.get(key) is not False:
            raise ValueError(f"forwarding row has failed logging state: {key}")

    kind = row["kind"]
    if kind == "lookup":
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
                or _raw_identity(parent) != pending_raw
            ):
                raise ValueError("lookup/query binding mismatch")
    elif kind == "query":
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
            for child in operations.values()
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


def _validate_binding_witness(row: dict, raw: tuple) -> None:
    witness_keys = (
        "binding_current_thread_known",
        "binding_event_thread_known",
        "binding_cached_thread_known",
        "binding_current_process_known",
        "binding_event_process_known",
        "binding_current_system_pid_known",
        "binding_current_system_tid_known",
    )
    if any(row.get(key) is not True for key in witness_keys):
        raise ValueError("incomplete binding witness")
    current_thread = _integer(row, "binding_current_thread_id", 0, 0xFFFFFFFF)
    event_thread = _integer(row, "binding_event_thread_id", 0, 0xFFFFFFFF)
    cached_thread = _integer(row, "binding_cached_thread_id", 0, 0xFFFFFFFF)
    current_process = _integer(row, "binding_current_process_id", 0, 0xFFFFFFFF)
    event_process = _integer(row, "binding_event_process_id", 0, 0xFFFFFFFF)
    if (
        0xFFFFFFFF
        in {current_thread, event_thread, cached_thread, current_process, event_process}
        or not current_thread == event_thread == cached_thread
    ):
        raise ValueError("inconsistent binding thread witness")
    if current_process != event_process:
        raise ValueError("inconsistent binding process witness")
    if _integer(row, "binding_current_system_pid", 1, 0xFFFFFFFF) != raw[1]:
        raise ValueError("binding system PID mismatch")
    if _integer(row, "binding_current_system_tid", 1, 0xFFFFFFFF) != raw[2]:
        raise ValueError("binding system TID mismatch")


def _validate_deferred_trace(trace: dict) -> int:
    """Validate raw admission, delayed binding, and ordered forwarding rows."""
    if _integer(trace, "overflow_count"):
        raise ValueError("trace overflow")
    _integer(trace, "passthrough_generation")
    if type(trace.get("passthrough_published")) is not bool:
        raise ValueError("unknown publication state")
    if _integer(trace, "unlogged_calls"):
        raise ValueError("unlogged passthrough calls")
    rows = trace.get("rows")
    if not isinstance(rows, list) or not rows:
        raise ValueError("missing trace rows")

    operations: dict[int, dict] = {}
    used_sequences = set()
    session = None
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("row must be an object")
        seq = _integer(row, "sequence", 1)
        is_missing_callback_exit = (
            row.get("kind") == "callback_entry" and row.get("exit_sequence") == 0
        )
        exit_seq = (
            0 if is_missing_callback_exit else _integer(row, "exit_sequence", seq + 1)
        )
        if (
            (not is_missing_callback_exit and exit_seq <= seq)
            or seq in used_sequences
            or (exit_seq and exit_seq in used_sequences)
        ):
            raise ValueError("duplicate entry/exit sequence")
        used_sequences.add(seq)
        if exit_seq:
            used_sequences.add(exit_seq)
        if row.get("kind") == "callback_entry" and row.get("dispatch_marker", False):
            _validate_callback_dispatch_fields(row)
            for key in ("delegate_begin_sequence", "delegate_end_sequence"):
                stamp = _integer(row, key, 0)
                if stamp and stamp in used_sequences:
                    raise ValueError("duplicate callback delegate sequence")
                if stamp:
                    used_sequences.add(stamp)
        provenance = row.get("provenance")
        if isinstance(provenance, dict) and provenance.get("status") != "not_attempted":
            begin = _integer(provenance, "acquisition_begin_sequence", 1)
            end = _integer(provenance, "acquisition_end_sequence", begin + 1)
            if end <= begin or begin in used_sequences or end in used_sequences:
                raise ValueError("duplicate provenance acquisition sequence")
            used_sequences.update((begin, end))
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
            if type(row.get(key)) is not bool:
                raise ValueError(f"unknown row state: {key}")
        if not row["incomplete"] and any(
            row.get(key) is True
            for key in ("pre_log_failed", "post_log_failed", "rethrown")
        ):
            raise ValueError("complete row has failed logging state")
        if row.get("kind") not in {
            "lookup",
            "query",
            "context_write",
            "pending_event",
            "engine_binding",
            "callback_entry",
            "callback_acquisition",
        }:
            raise ValueError("unsupported row kind")

    kinds = {row["kind"] for row in rows}
    if "callback_entry" in kinds or "callback_acquisition" in kinds:
        required_kinds = {
            "lookup",
            "query",
            "context_write",
            "pending_event",
            "engine_binding",
            "callback_entry",
            "callback_acquisition",
        }
    else:
        required_kinds = {
            "lookup",
            "query",
            "context_write",
            "pending_event",
            "engine_binding",
        }
    if not required_kinds.issubset(kinds):
        raise ValueError("missing delayed binding boundary")
    callback_entries: dict[int, dict] = {}
    callback_acquisitions: dict[int, dict] = {}
    for row in rows:
        if row["kind"] == "callback_entry":
            callback_operation, _ = _validate_callback_entry(row)
            if callback_operation in callback_entries:
                raise ValueError("duplicate callback entry")
            callback_entries[callback_operation] = row
    for row in rows:
        if row["kind"] == "callback_acquisition":
            callback_operation, _, _ = _validate_callback_acquisition(
                row, callback_entries
            )
            if _integer(row, "acquisition_operation_id", 1) in callback_acquisitions:
                raise ValueError("duplicate callback acquisition")
            callback_acquisitions[_integer(row, "acquisition_operation_id", 1)] = row
    for callback_operation, entry in callback_entries.items():
        if not entry.get("dispatch_marker", False):
            continue
        acquisition_id = _integer(entry, "dispatch_acquisition_operation_id", 0)
        if acquisition_id == 0:
            if not entry["incomplete"]:
                raise ValueError("complete callback dispatch has no acquisition link")
            continue
        acquisition = callback_acquisitions.get(acquisition_id)
        if (
            acquisition is None
            or _integer(acquisition, "callback_operation_id", 1) != callback_operation
        ):
            raise ValueError("callback dispatch acquisition link mismatch")
        accepted = acquisition.get("outcome") == "accepted"
        acquisition_binding_attempted = (
            acquisition.get("binding_status") != "refused"
            or _integer(acquisition, "binding_attempt_id", 0) != 0
        )
        dispatch_binding_attempted = entry.get("binding_attempted") is True
        dispatch_binding_succeeded = entry.get("binding_succeeded") is True
        dispatch_binding_receipt = (
            _integer(entry, "dispatch_binding_attempt_id", 0) != 0
        )
        binding_evidence_present = (
            acquisition_binding_attempted
            or dispatch_binding_attempted
            or dispatch_binding_succeeded
            or dispatch_binding_receipt
        )
        if (
            accepted
            or acquisition.get("binding_status") == "bound"
            or dispatch_binding_succeeded
        ):
            late_restore_failure = entry.get("error_restore_late_failure") is True
            if (
                entry.get("foreign_thread_refused")
                or entry.get("reentry_refused")
                or entry.get("invalid_configuration_refused")
                or entry.get("provider_attempted") is not True
                or entry.get("provider_succeeded") is not True
                or entry.get("provider_threw")
                or entry.get("owner_evidence_complete") is not True
                or entry.get("capture_attempted") is not True
                or entry.get("capture_completed") is not True
                or entry.get("error_restore_attempted") is not True
                or entry.get("error_restore_prerequisite_attempted") is not True
                or entry.get("error_restore_prerequisite_succeeded") is not True
                or (
                    entry.get("error_restore_succeeded") is not True
                    and not late_restore_failure
                )
                or (late_restore_failure and entry.get("incomplete") is not True)
                or entry.get("dispatch_incoming_error_known") is not True
                or entry.get("dispatch_returned_error_known") is not True
            ):
                raise ValueError(
                    "accepted callback acquisition contradicts dispatch evidence"
                )
            if binding_evidence_present:
                if (
                    entry.get("binding_attempted") is not True
                    or entry.get("binding_succeeded") is not True
                    or acquisition.get("binding_status") != "bound"
                    or not _integer(acquisition, "binding_attempt_id", 1)
                ):
                    raise ValueError(
                        "callback dispatch binding evidence contradicts acquisition"
                    )
            elif (
                not accepted
                or acquisition.get("binding_status") != "refused"
                or _integer(acquisition, "binding_attempt_id", 0) != 0
                or entry.get("binding_attempted") is not False
                or entry.get("binding_succeeded") is not False
                or dispatch_binding_receipt
            ):
                raise ValueError(
                    "accepted callback acquisition has inconsistent binding evidence"
                )
        entry_begin = _integer(entry, "delegate_begin_sequence", 0)
        entry_end = _integer(entry, "delegate_end_sequence", 0)
        acquisition_begin = _integer(acquisition, "acquisition_begin_sequence", 1)
        acquisition_end = _integer(
            acquisition, "acquisition_end_sequence", acquisition_begin + 1
        )
        if entry_begin == 0 or entry_end == 0:
            if not entry["incomplete"]:
                raise ValueError("complete callback dispatch has no delegate interval")
        elif entry["dispatch_phase"] == "before_delegate":
            if not acquisition_end < entry_begin:
                raise ValueError("breakpoint acquisition does not precede delegate")
        elif not entry_end < acquisition_begin:
            raise ValueError("create-thread delegate does not precede acquisition")
        if entry.get("binding_succeeded"):
            if (
                acquisition.get("outcome") != "accepted"
                or acquisition.get("binding_status") != "bound"
                or not _integer(acquisition, "binding_attempt_id", 1)
                or _integer(entry, "dispatch_binding_attempt_id", 0)
                != _integer(acquisition, "binding_attempt_id", 1)
            ):
                raise ValueError("callback dispatch binding link mismatch")
    ordered = sorted(rows, key=lambda row: row["sequence"])
    pending_raw = None
    pending_qualified = None
    admitted_exit = 0
    binding_exit_sequence = None
    event_operations: list[dict] = []
    callback_bound_links: dict[int, list[dict]] = {}
    bound_raw: set[tuple] = set()
    bound_lifecycles: dict[tuple, int] = {}
    bound_generation_lifecycles: dict[int, tuple] = {}

    for row in ordered:
        parent_id = _integer(row, "parent_operation_id")
        if parent_id:
            parent = operations.get(parent_id)
            closed_callback_refusal = (
                row.get("kind") == "callback_acquisition"
                and row.get("outcome") != "accepted"
                and row.get("incomplete") is True
                and isinstance(parent, dict)
                and parent.get("kind") == "callback_entry"
                and parent.get("exit_observed") is True
                and parent.get("exit_sequence", 0) <= row["sequence"]
            )
            callback_refusal = (
                row.get("kind") == "callback_acquisition"
                and row.get("outcome") != "accepted"
                and row.get("incomplete") is True
                and isinstance(parent, dict)
                and parent.get("kind") == "callback_entry"
            )
            parent_interval_valid = (
                parent is not None
                and parent["sequence"]
                < row["sequence"]
                < row["exit_sequence"]
                < parent["exit_sequence"]
                and (
                    parent["observer_thread_id"] == row["observer_thread_id"]
                    or callback_refusal
                )
            )
            if not parent_interval_valid and not closed_callback_refusal:
                raise ValueError("invalid parent interval or thread")

        kind = row["kind"]
        if kind == "pending_event":
            attempted = _raw_identity(row, "pending_")
            header_raw = _raw_identity(row)
            status = row.get("pending_status")
            if status == "admitted" and pending_raw is None:
                if attempted != header_raw or pending_qualified is not None:
                    raise ValueError("admission header mismatch")
                pending_raw = attempted
                pending_qualified = (
                    _identity(row)
                    if row.get("engine_generation_known") is True
                    else None
                )
                admitted_exit = row["exit_sequence"]
                binding_exit_sequence = None
                event_operations = []
            elif status == "closed" and pending_raw == attempted:
                if pending_qualified is None:
                    if (
                        header_raw != pending_raw
                        or row.get("pending_engine_generation_known") is True
                    ):
                        raise ValueError("close header mismatch")
                elif (
                    _identity(row) != pending_qualified
                    or _identity(row, "pending_") != pending_qualified
                ):
                    raise ValueError("close header mismatch")
                if any(
                    operation["exit_sequence"] >= row["sequence"]
                    for operation in event_operations
                ):
                    raise ValueError("operation crosses event closure")
                pending_raw = None
                pending_qualified = None
                binding_exit_sequence = None
            else:
                raise ValueError("duplicate, changed or missing pending event")
            continue

        if kind == "callback_entry":
            callback_operation = _integer(row, "callback_operation_id", 1)
            raw = (
                _callback_identity(row, "callback_raw")
                if row.get("entry_raw_identity_known")
                else None
            )
            if pending_raw is None:
                if raw is not None or row["incomplete"] is not True:
                    raise ValueError("callback entry has no pending raw key")
            else:
                if raw is not None and raw != pending_raw:
                    raise ValueError("callback entry raw identity mismatch")
                if row["sequence"] <= admitted_exit:
                    raise ValueError("callback entry crosses event admission")
                event_operations.append(row)
            continue

        if kind == "callback_acquisition":
            callback_operation = _integer(row, "callback_operation_id", 1)
            raw = _callback_identity(
                row,
                "acquisition_raw",
                require_complete=row.get("outcome") == "accepted"
                or row.get("incomplete") is not True,
            )
            refusal = row.get("outcome") != "accepted" and row["incomplete"] is True
            if not raw[0] and refusal:
                continue
            if pending_raw is None or raw != pending_raw:
                if refusal:
                    continue
                raise ValueError("callback acquisition has no exact pending event")
            if row["sequence"] <= admitted_exit:
                raise ValueError("callback acquisition crosses event admission")
            if pending_qualified is None:
                if (
                    row.get("engine_generation_known") is True
                    or row["incomplete"] is not True
                ):
                    raise ValueError(
                        "unbound callback acquisition claims qualification"
                    )
            elif (
                binding_exit_sequence is not None
                and row["sequence"] <= binding_exit_sequence
            ):
                if (
                    row.get("engine_generation_known") is True
                    or row["incomplete"] is not True
                ):
                    raise ValueError("early callback acquisition claims qualification")
            event_operations.append(row)
            continue

        if kind == "engine_binding":
            status = row.get("binding_status")
            if status not in {
                "bound",
                "missing",
                "changed",
                "stale",
                "already_bound",
                "conflict",
                "duplicate",
                "incomplete_evidence",
                "continuation_closed",
                "refused",
                "overflow",
            }:
                raise ValueError("unknown binding status")
            binding_qualified = row.get("binding_qualified")
            if status == "bound":
                if binding_qualified is not True:
                    raise ValueError("bound binding is not qualified")
            elif binding_qualified is not False:
                raise ValueError("non-bound binding claims qualification")
            raw = _binding_raw_identity(row)
            binding_generation_value = _integer(row, "binding_engine_generation")
            callback_operation = (
                _integer(row, "callback_operation_id")
                if "callback_operation_id" in row
                else 0
            )
            callback_acquisition = (
                _integer(row, "callback_acquisition_operation_id")
                if "callback_acquisition_operation_id" in row
                else 0
            )
            binding_attempt = (
                _integer(row, "binding_attempt_id")
                if "binding_attempt_id" in row
                else 0
            )
            if callback_operation or callback_acquisition:
                if not callback_operation or not callback_acquisition:
                    raise ValueError("partial callback binding link")
                acquisition = callback_acquisitions.get(callback_acquisition)
                if (
                    acquisition is None
                    or _integer(acquisition, "callback_operation_id", 1)
                    != callback_operation
                ):
                    raise ValueError("callback binding link mismatch")
                if _callback_identity(acquisition, "acquisition_raw") != raw:
                    raise ValueError("callback binding raw key mismatch")
                if binding_attempt == 0 or binding_attempt != _integer(
                    acquisition, "binding_attempt_id"
                ):
                    raise ValueError("callback binding attempt mismatch")
                if acquisition.get("binding_status") != status:
                    raise ValueError("callback binding status mismatch")
                if status == "bound":
                    if (
                        acquisition.get("outcome") != "accepted"
                        or acquisition.get("binding_eligible") is not True
                        or acquisition.get("query_interface", {}).get("status")
                        != "succeeded"
                        or acquisition.get("query_interface", {}).get(
                            "release_succeeded"
                        )
                        is not True
                        or acquisition.get("error_restore_attempted") is not True
                        or acquisition.get("error_restore_succeeded") is not True
                        or not all(
                            read.get("status") == "succeeded"
                            and read.get("output_known") is True
                            for read in acquisition.get("sdk_reads", [])
                        )
                    ):
                        raise ValueError("bound callback links to refused acquisition")
                    if _integer(row, "binding_engine_generation") != _integer(
                        acquisition.get("owner", {}), "lifecycle_token"
                    ):
                        raise ValueError(
                            "callback binding generation does not match owner token"
                        )
                    for key in (
                        "binding_current_thread_id",
                        "binding_event_thread_id",
                        "binding_cached_thread_id",
                        "binding_current_process_id",
                        "binding_event_process_id",
                        "binding_current_system_pid",
                        "binding_current_system_tid",
                    ):
                        if row.get(key) != acquisition.get(key):
                            raise ValueError(
                                "callback binding witness differs from acquisition"
                            )
                    callback_bound_links.setdefault(callback_acquisition, []).append(
                        row
                    )
                if row["sequence"] <= acquisition["exit_sequence"]:
                    raise ValueError("callback binding precedes acquisition end")
            if status == "bound" and binding_generation_value == 0:
                raise ValueError("zero binding generation")
            if pending_raw is None:
                if (
                    status not in {"missing", "stale", "continuation_closed"}
                    or not row["incomplete"]
                ):
                    raise ValueError("binding without pending event")
                continue
            if raw != pending_raw:
                raise ValueError("binding identity mismatch")
            if row["sequence"] <= admitted_exit:
                raise ValueError("binding crosses event admission")
            event_operations.append(row)
            if status == "bound":
                if pending_qualified is not None or binding_exit_sequence is not None:
                    raise ValueError("duplicate binding transition")
                pending_qualified = _identity(row)
                if row["incomplete"]:
                    raise ValueError("bound binding is incomplete")
                if not pending_qualified[:5] == raw:
                    raise ValueError("qualified binding raw identity mismatch")
                if (
                    row.get("binding_qualified_event_complete") is not True
                    or row.get("binding_qualified_engine_generation_known") is not True
                ):
                    raise ValueError("missing qualified binding identity")
                if pending_qualified[-1] != binding_generation_value:
                    raise ValueError("binding generation mismatch")
                if (
                    _integer(row, "binding_qualified_engine_generation", 1)
                    != binding_generation_value
                ):
                    raise ValueError("qualified binding generation mismatch")
                lifecycle = raw[:4]
                if raw in bound_raw:
                    raise ValueError("duplicate binding lifecycle")
                prior_generation = bound_lifecycles.get(lifecycle)
                if (
                    prior_generation is not None
                    and prior_generation != binding_generation_value
                ):
                    raise ValueError("binding token changed for lifecycle")
                prior_lifecycle = bound_generation_lifecycles.get(
                    binding_generation_value
                )
                if prior_lifecycle is not None and prior_lifecycle != lifecycle:
                    raise ValueError("binding lifecycle token reused")
                _validate_binding_witness(row, raw)
                bound_raw.add(raw)
                bound_lifecycles[lifecycle] = binding_generation_value
                bound_generation_lifecycles[binding_generation_value] = lifecycle
                binding_exit_sequence = row["exit_sequence"]
                if any(
                    operation["kind"] not in {"engine_binding", "callback_entry"}
                    and operation["sequence"]
                    <= binding_exit_sequence
                    <= operation["exit_sequence"]
                    for operation in event_operations
                ):
                    raise ValueError("operation crosses binding transition")
            elif not row["incomplete"]:
                raise ValueError("refused binding marked complete")
            continue

        if pending_raw is None:
            raise ValueError("operation has no pending event")
        if _raw_identity(row) != pending_raw:
            raise ValueError("operation has no exact pending event join")
        if row["sequence"] <= admitted_exit:
            raise ValueError("operation crosses event admission")
        if pending_qualified is None:
            if row.get("engine_generation_known") is True or not row["incomplete"]:
                raise ValueError("unbound operation claims qualification")
        else:
            if binding_exit_sequence is None:
                raise ValueError("qualified event has no binding boundary")
            if row["sequence"] <= binding_exit_sequence <= row["exit_sequence"]:
                raise ValueError("operation crosses binding transition")
            if row["sequence"] <= binding_exit_sequence:
                if row.get("engine_generation_known") is True or not row["incomplete"]:
                    raise ValueError("early operation claims qualification")
            elif (
                row.get("engine_generation_known") is not True
                or _identity(row) != pending_qualified
                or row["incomplete"]
            ):
                raise ValueError("post-binding operation lost qualified identity")
        event_operations.append(row)
        _validate_deferred_operation(row, parent_id, operations, pending_raw)

    for acquisition_operation, acquisition in callback_acquisitions.items():
        if acquisition.get("outcome") != "accepted":
            continue
        bound_links = callback_bound_links.get(acquisition_operation, [])
        binding_attempted = (
            acquisition.get("binding_status") != "refused"
            or _integer(acquisition, "binding_attempt_id", 0) != 0
        )
        if binding_attempted and len(bound_links) != 1:
            raise ValueError("accepted callback acquisition lacks one bound link")
        if not binding_attempted and bound_links:
            raise ValueError("unbound callback acquisition has a bound link")

    if pending_raw is not None:
        raise ValueError("pending event not closed")
    return len(rows)


def validate_trace(trace: object) -> int:
    """Return row count after structural checks, never runtime qualification."""
    if (
        not isinstance(trace, dict)
        or trace.get("provenance") != "synthetic-forwarding-profile"
        or trace.get("live_coverage") != "incomplete"
    ):
        raise ValueError("unsupported provenance or live coverage claim")
    if isinstance(trace.get("rows"), list) and any(
        isinstance(row, dict) and row.get("kind") == "engine_binding"
        for row in trace["rows"]
    ):
        return _validate_deferred_trace(trace)
    if _integer(trace, "overflow_count"):
        raise ValueError("trace overflow")
    _integer(trace, "passthrough_generation")
    if type(trace.get("passthrough_published")) is not bool:
        raise ValueError("unknown publication state")
    if _integer(trace, "unlogged_calls"):
        raise ValueError("unlogged passthrough calls")
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
        provenance = row.get("provenance")
        if isinstance(provenance, dict) and provenance.get("status") != "not_attempted":
            begin = _integer(provenance, "acquisition_begin_sequence", 1)
            end = _integer(provenance, "acquisition_end_sequence", begin + 1)
            if end <= begin or begin in used_sequences or end in used_sequences:
                raise ValueError("duplicate provenance acquisition sequence")
            used_sequences.update((begin, end))
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
        for key in ("incoming_last_status", "returned_last_status"):
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
    parser.add_argument("--strict-provenance", action="store_true")
    args = parser.parse_args()
    if not args.trace.is_absolute():
        parser.error("--trace must be an absolute path")
    try:
        value = json.loads(
            args.trace.read_text(encoding="ascii"), object_pairs_hook=unique_object
        )
        count = (
            validate_provenance(value)
            if args.strict_provenance
            else validate_trace(value)
        )
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"observer diagnostic: {error}\n")
    if args.strict_provenance:
        print(
            f"validated {count} selected query provenance rows; native qualification remains unsupported"
        )
    else:
        print(f"validated {count} synthetic rows; live coverage remains incomplete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
