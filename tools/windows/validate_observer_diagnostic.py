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
        exit_seq = _integer(row, "exit_sequence", seq + 1)
        if exit_seq <= seq or seq in used_sequences or exit_seq in used_sequences:
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
        }:
            raise ValueError("unsupported row kind")

    kinds = {row["kind"] for row in rows}
    if not {
        "lookup",
        "query",
        "context_write",
        "pending_event",
        "engine_binding",
    }.issubset(kinds):
        raise ValueError("missing delayed binding boundary")
    ordered = sorted(rows, key=lambda row: row["sequence"])
    pending_raw = None
    pending_qualified = None
    admitted_exit = 0
    binding_exit_sequence = None
    event_operations: list[dict] = []
    bound_raw: set[tuple] = set()
    bound_lifecycles: dict[tuple, int] = {}
    bound_generation_lifecycles: dict[int, tuple] = {}

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
                    operation["kind"] != "engine_binding"
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
