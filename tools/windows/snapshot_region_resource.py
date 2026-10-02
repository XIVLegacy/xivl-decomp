#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Read a pinned-build region resource object graph without controlling a process.

The native addresses in this module are VAs from the pinned 32-bit image.  The
reader reports object configuration and configured dispatch targets only.  It
does not attach a debugger, call a target function, or write target memory.
"""

from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import time
from typing import Callable, Iterable


IMAGE_BASE = 0x00400000
EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
EXPECTED_MACHINE = 0x014C
EXPECTED_OPTIONAL_MAGIC = 0x010B
SCENE_GLOBAL = 0x0133DEF4
FORMATTER_MODE = 0x01266B64
ASYNC_GATE = 0x01376038
EXPECTED_PRIMARY_VTABLE = 0x00FB7E14
EXPECTED_SECONDARY_VTABLE = 0x00FB7DF4
EXPECTED_COMMON_ACTOR_VTABLE = 0x00FB7E4C
EXPECTED_MANAGER_VTABLE = 0x00FBFA88
EXPECTED_RESOURCE_VTABLE = 0x0110C4C0
EXPECTED_REGION_MODE = 3
WINDOWS_EPOCH_FILETIME_OFFSET = 116444736000000000
MAX_SAMPLES = 10_000
MAX_READ_SIZE = 0x1000

# These instruction sites are the fixed locators used by this tool.  Their
# loaded bytes are compared with the pinned disk image before object reads.
LOCATOR_SIZES = {
    # Scene global getters and manager construction/field stores.
    0x00623C60: 6,
    0x00626E57: 6,
    0x0060B360: 3,
    0x0060B370: 3,
    0x0064F92F: 6,
    0x0064F935: 7,
    0x0064F93C: 3,
    0x0064F955: 6,
    0x0064F960: 3,
    0x0064F963: 2,
    0x0064F966: 3,
    0x0064E87C: 3,
    0x0064E9A7: 3,
    # Current formatter/completion selectors and alternate-selector getter.
    0x0044B3CB: 7,
    0x00631D0D: 7,
    0x0061FEA0: 3,
    # Event identity, mode, and the fixed event payload fields.
    0x006113DC: 6,
    0x006113E2: 7,
    0x006113E9: 3,
    0x006113EC: 3,
    0x006113EF: 3,
    0x006113F2: 3,
    0x0061F582: 3,
    0x006114B0: 3,
    # Decoder and actor dispatch slots.
    0x00631DD2: 3,
    0x00631956: 3,
    0x00620905: 3,
    0x00620914: 3,
    # RegionInfo root-key definition and lookup consumers.
    0x0079CDC4: 6,
    0x0079B35E: 6,
    0x0079B3CE: 6,
    # Resource constructor/accessors and state/buffer checks.
    0x00CAEE0B: 6,
    0x00CAEE22: 3,
    0x00CAEE29: 3,
    0x00CAEE30: 3,
    0x00CAEE8E: 6,
    0x00CAE660: 3,
    0x00CAE675: 6,
    0x00CAE682: 3,
    0x00CAE68B: 4,
    0x00CAE69C: 3,
}

ReadBytes = Callable[[int, int], bytes]


class SnapshotError(RuntimeError):
    """A read or pinned-profile check failed."""


class SnapshotReadError(SnapshotError):
    """A requested address could not be read completely."""


def _check_range(address: int, size: int) -> None:
    if not isinstance(address, int) or not isinstance(size, int):
        raise SnapshotReadError("address and size must be integers")
    if address < 0 or address > 0xFFFFFFFF:
        raise SnapshotReadError(f"address outside 32-bit range: 0x{address:x}")
    if size < 0 or size > MAX_READ_SIZE:
        raise SnapshotReadError(f"read size outside bound: {size}")
    if address + size > 0x1_0000_0000:
        raise SnapshotReadError("read crosses 32-bit address space")


def _read(read_bytes: ReadBytes, address: int, size: int) -> bytes:
    _check_range(address, size)
    try:
        value = read_bytes(address, size)
    except SnapshotReadError:
        raise
    except Exception as exc:  # A test reader may expose OSError or ValueError.
        raise SnapshotReadError(f"read failed at 0x{address:08x}: {exc}") from exc
    if not isinstance(value, (bytes, bytearray, memoryview)):
        raise SnapshotReadError("reader returned a non-bytes value")
    result = bytes(value)
    if len(result) != size:
        raise SnapshotReadError(
            f"short read at 0x{address:08x}: {len(result)} of {size} bytes"
        )
    return result


def _u32(read_bytes: ReadBytes, address: int) -> int:
    return struct.unpack("<I", _read(read_bytes, address, 4))[0]


def _u8(read_bytes: ReadBytes, address: int) -> int:
    return _read(read_bytes, address, 1)[0]


def _nullable(value: int | None) -> int | None:
    if value is None or value == 0:
        return None
    return value


def _safe_u32(
    read_bytes: ReadBytes,
    address: int,
    name: str,
    errors: list[dict[str, str]],
) -> int | None:
    try:
        return _u32(read_bytes, address)
    except SnapshotReadError as exc:
        errors.append({"field": name, "message": str(exc)})
        return None


def _safe_u8(
    read_bytes: ReadBytes,
    address: int,
    name: str,
    errors: list[dict[str, str]],
) -> int | None:
    try:
        return _u8(read_bytes, address)
    except SnapshotReadError as exc:
        errors.append({"field": name, "message": str(exc)})
        return None


def _pointer_fields(
    read_bytes: ReadBytes,
    base: int,
    fields: Iterable[tuple[str, int]],
    errors: list[dict[str, str]],
) -> dict[str, int | None]:
    values: dict[str, int | None] = {}
    for name, offset in fields:
        raw = _safe_u32(read_bytes, base + offset, name, errors)
        values[name] = _nullable(raw)
    return values


def _read_target(
    read_bytes: ReadBytes,
    vtable: int | None,
    offset: int,
    name: str,
    errors: list[dict[str, str]],
) -> int | None:
    if not vtable:
        return None
    return _nullable(_safe_u32(read_bytes, vtable + offset, name, errors))


def collect_snapshot(
    read_bytes: ReadBytes,
    expected_region: int | None = None,
) -> dict[str, object]:
    """Collect one non-atomic object graph through an injected byte reader.

    The callable receives a 32-bit VA and an exact byte count.  It must return
    exactly that many bytes or raise.  This function does no Windows API work,
    which keeps synthetic verification independent from a live process.
    """

    errors: list[dict[str, str]] = []
    result: dict[str, object] = {
        "format_version": 1,
        "status": "snapshot",
        "scene": None,
        "context": None,
        "context_detail": None,
        "manager": None,
        "event": None,
        "region": None,
        "resource": None,
        "decoded_object": None,
        "alternate_selector": None,
        "decoder": None,
        "actor": None,
        "formatter_mode": None,
        "async_gate": None,
    }

    result["formatter_mode"] = _safe_u8(
        read_bytes, FORMATTER_MODE, "formatter_mode", errors
    )
    result["async_gate"] = _safe_u32(read_bytes, ASYNC_GATE, "async_gate", errors)
    scene = _safe_u32(read_bytes, SCENE_GLOBAL, "scene_global", errors)
    result["scene"] = _nullable(scene)
    if scene is None:
        result["status"] = "read_error"
        result["errors"] = errors or [
            {"field": "scene_global", "message": "scene global was unreadable"}
        ]
        return result
    if scene == 0:
        result["status"] = "uninitialized"
        if errors:
            result["status"] = "read_error"
            result["errors"] = errors
        return result

    context = _nullable(_safe_u32(read_bytes, scene + 0x114, "scene.context", errors))
    manager = _nullable(_safe_u32(read_bytes, scene + 0x17C, "scene.manager", errors))
    result["context"] = _nullable(context)
    result["manager"] = _nullable(manager)
    if manager is None:
        result["status"] = "read_error" if errors else "uninitialized"
        if errors:
            result["errors"] = errors
        return result

    manager_vtable = _nullable(_safe_u32(read_bytes, manager, "manager.vtable", errors))
    result["manager_vtable"] = manager_vtable
    result["validation"] = {"manager_vtable": manager_vtable == EXPECTED_MANAGER_VTABLE}
    if manager_vtable != EXPECTED_MANAGER_VTABLE:
        result["status"] = "read_error" if errors else "rejected"
        result["validation_errors"] = ["manager_vtable"]
        if errors:
            result["errors"] = errors
        return result

    event_address = manager + 0x3C
    event = _pointer_fields(
        read_bytes,
        event_address,
        (("primary_vtable", 0), ("secondary_vtable", 4), ("actor", 8)),
        errors,
    )
    event["address"] = event_address
    event["mode"] = _safe_u32(read_bytes, event_address + 0x14, "event.mode", errors)
    result["event"] = event

    primary_ok = event["primary_vtable"] == EXPECTED_PRIMARY_VTABLE
    secondary_ok = event["secondary_vtable"] == EXPECTED_SECONDARY_VTABLE
    mode_ok = event["mode"] == EXPECTED_REGION_MODE
    validation = {
        "manager_vtable": True,
        "primary_vtable": primary_ok,
        "secondary_vtable": secondary_ok,
        "mode": mode_ok,
    }
    result["validation"] = validation
    if not (primary_ok and secondary_ok and mode_ok):
        result["status"] = "rejected"
        result["validation_errors"] = [
            name for name, valid in validation.items() if not valid
        ]
        if errors:
            result["status"] = "read_error"
            result["errors"] = errors
        return result

    # These are manager fields and are read only after the required event
    # identity and mode checks, avoiding layout-dependent dereferences.
    manager_fields = _pointer_fields(
        read_bytes,
        manager,
        (
            ("region_info", 0x10),
            ("resource", 0x54),
            ("decoded_object", 0x58),
            ("alternate_selector", 0x5C),
        ),
        errors,
    )
    manager_fields["region"] = _safe_u32(
        read_bytes, manager + 0x14, "manager.region", errors
    )
    result["decoded_object"] = manager_fields["decoded_object"]
    result["alternate_selector"] = manager_fields["alternate_selector"]
    event["resource"] = manager_fields["resource"]
    event["decoded_object"] = manager_fields["decoded_object"]
    event["alternate_selector"] = manager_fields["alternate_selector"]
    region_info = manager_fields["region_info"]
    region_record: dict[str, object] = {
        "pointer": region_info,
        "manager_region": manager_fields["region"],
        "root_key": None,
    }
    if region_info:
        region_record["root_key"] = _safe_u32(
            read_bytes, region_info + 0xB0, "region_info.root_key", errors
        )
    if expected_region is not None:
        actual_region = region_record["manager_region"]
        region_record["expected"] = expected_region
        region_record["expected_match"] = actual_region == expected_region
        if not region_record["expected_match"]:
            result.setdefault("validation_errors", []).append("expected_region")
    result["region"] = region_record

    resource = _nullable(manager_fields["resource"])
    if resource:
        resource_vtable = _nullable(
            _safe_u32(read_bytes, resource, "resource.vtable", errors)
        )
        resource_record: dict[str, object] = {
            "pointer": resource,
            "vtable": resource_vtable,
            "vtable_expected": EXPECTED_RESOURCE_VTABLE,
            "vtable_matches": resource_vtable == EXPECTED_RESOURCE_VTABLE,
        }
        if resource_vtable == EXPECTED_RESOURCE_VTABLE:
            resource_record.update(
                {
                    "key": _safe_u32(
                        read_bytes, resource + 0x58, "resource.key", errors
                    ),
                    "raw_decode_arg": _safe_u32(
                        read_bytes, resource + 0x5C, "resource.raw_decode_arg", errors
                    ),
                    "buffer": _nullable(
                        _safe_u32(
                            read_bytes, resource + 0x64, "resource.buffer", errors
                        )
                    ),
                    "state": _safe_u32(
                        read_bytes, resource + 0xB0, "resource.state", errors
                    ),
                }
            )
        else:
            result.setdefault("validation_errors", []).append("resource_vtable")
        result["resource"] = resource_record

    context_record: dict[str, object] = {"pointer": context}
    if context:
        decoder = _nullable(
            _safe_u32(read_bytes, context + 0x08, "context.decoder", errors)
        )
        context_object = _nullable(
            _safe_u32(read_bytes, context + 0x10, "context.object", errors)
        )
        context_record["decoder"] = decoder
        context_record["object"] = context_object
        decoder_record: dict[str, object] = {"pointer": decoder}
        if decoder:
            decoder_vtable = _nullable(
                _safe_u32(read_bytes, decoder, "decoder.vtable", errors)
            )
            decoder_record["vtable"] = decoder_vtable
            decoder_record["sync_target"] = _read_target(
                read_bytes, decoder_vtable, 0x58, "decoder.vtable.sync", errors
            )
            decoder_record["async_target"] = _read_target(
                read_bytes, decoder_vtable, 0x5C, "decoder.vtable.async", errors
            )
        context_record["decoder_detail"] = decoder_record

        common_actor = None
        if context_object:
            common_actor = _nullable(
                _safe_u32(
                    read_bytes,
                    context_object + 0x7C,
                    "context.object.common_actor",
                    errors,
                )
            )
        context_actor = {
            "pointer": common_actor,
            "event_actor": _nullable(event["actor"]),
            "matches_event_actor": common_actor == _nullable(event["actor"]),
        }
        context_record["common_actor"] = context_actor
        actor_record: dict[str, object] = {
            "pointer": common_actor,
            "event_actor": _nullable(event["actor"]),
            "matches_event_actor": context_actor["matches_event_actor"],
            "layout_read": False,
        }
        if common_actor and context_actor["matches_event_actor"]:
            common_vtable = _nullable(
                _safe_u32(read_bytes, common_actor, "common_actor.vtable", errors)
            )
            actor_record["layout_read"] = True
            actor_record["vtable"] = common_vtable
            actor_record["vtable_expected"] = EXPECTED_COMMON_ACTOR_VTABLE
            actor_record["vtable_matches"] = (
                common_vtable == EXPECTED_COMMON_ACTOR_VTABLE
            )
            if actor_record["vtable_matches"]:
                forwarded = _nullable(
                    _safe_u32(
                        read_bytes,
                        common_actor + 0x08,
                        "common_actor.forwarded",
                        errors,
                    )
                )
                actor_record["forwarded"] = forwarded
                forwarded_vtable = None
                if forwarded:
                    forwarded_vtable = _nullable(
                        _safe_u32(read_bytes, forwarded, "forwarded.vtable", errors)
                    )
                actor_record["forwarded_vtable"] = forwarded_vtable
                actor_record["forwarded_target"] = _read_target(
                    read_bytes,
                    forwarded_vtable,
                    0x18,
                    "forwarded.vtable.slot_18",
                    errors,
                )
            else:
                actor_record["layout_skipped"] = "common_actor_vtable_mismatch"
                result.setdefault("validation_errors", []).append("common_actor_vtable")
        elif common_actor:
            actor_record["layout_skipped"] = "actor_mismatch"
        result["actor"] = actor_record
        result["decoder"] = decoder_record
        result["context_detail"] = context_record

    if errors:
        result["status"] = "read_error"
        result["errors"] = errors
    elif result.get("validation_errors"):
        result["status"] = "rejected"
    return result


def collect_stable_sample(
    read_bytes: ReadBytes,
    expected_region: int | None = None,
) -> dict[str, object]:
    """Collect two consecutive graphs and retain both on mismatch or error."""

    first = collect_snapshot(read_bytes, expected_region)
    second = collect_snapshot(read_bytes, expected_region)
    if first != second:
        return {
            "kind": "error",
            "error_code": "snapshot_mismatch",
            "stable": False,
            "first": first,
            "second": second,
        }
    if first.get("status") == "read_error":
        return {
            "kind": "error",
            "error_code": "read_failure",
            "status": "unusable",
            "stable": False,
            "snapshot": first,
        }
    if first.get("status") == "rejected":
        return {
            "kind": "error",
            "error_code": "rejected_snapshot",
            "status": "unusable",
            "observation_status": "rejected",
            "stable": True,
            "snapshot": first,
        }
    if first.get("status") == "uninitialized":
        return {
            "kind": "error",
            "error_code": "uninitialized_scene",
            "status": "unusable",
            "observation_status": "uninitialized",
            "stable": True,
            "snapshot": first,
        }
    return {"kind": "snapshot", "stable": True, "snapshot": first}


@dataclass
class _PeImage:
    data: bytes
    machine: int
    optional_magic: int
    image_base: int
    section_table: int
    sections: tuple[tuple[int, int, int, int], ...]

    def file_offset_for_rva(self, rva: int, size: int) -> int:
        for virtual_address, virtual_size, raw_offset, raw_size in self.sections:
            span = max(virtual_size, raw_size)
            if virtual_address <= rva and rva + size <= virtual_address + span:
                offset = raw_offset + (rva - virtual_address)
                if offset + size <= len(self.data):
                    return offset
        if rva + size <= self.section_table:
            return rva
        raise SnapshotError(f"locator RVA 0x{rva:x} is not in the PE image")


def _parse_pe(data: bytes) -> _PeImage:
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise SnapshotError("resident image is not a PE32 image")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if pe_offset + 24 > len(data) or data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise SnapshotError("PE signature is missing")
    machine, sections_count, _, _, _, optional_size, _ = struct.unpack_from(
        "<HHIIIHH", data, pe_offset + 4
    )
    optional = pe_offset + 24
    if optional + optional_size > len(data) or optional_size < 32:
        raise SnapshotError("PE optional header is truncated")
    optional_magic = struct.unpack_from("<H", data, optional)[0]
    image_base = struct.unpack_from("<I", data, optional + 28)[0]
    section_table = optional + optional_size
    sections: list[tuple[int, int, int, int]] = []
    for index in range(sections_count):
        offset = section_table + index * 40
        if offset + 40 > len(data):
            raise SnapshotError("PE section table is truncated")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, offset + 8
        )
        sections.append((virtual_address, virtual_size, raw_offset, raw_size))
    return _PeImage(
        data=data,
        machine=machine,
        optional_magic=optional_magic,
        image_base=image_base,
        section_table=section_table,
        sections=tuple(sections),
    )


def _profile_identity(reader: "WindowsProcessReader") -> dict[str, object]:
    image_path = reader.image_path()
    try:
        disk_data = Path(image_path).read_bytes()
    except OSError as exc:
        raise SnapshotError(f"cannot read target executable: {exc}") from exc
    digest = hashlib.sha256(disk_data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise SnapshotError("target executable SHA-256 rejected")
    disk_image = _parse_pe(disk_data)
    if (
        disk_image.machine != EXPECTED_MACHINE
        or disk_image.optional_magic != EXPECTED_OPTIONAL_MAGIC
        or disk_image.image_base != IMAGE_BASE
    ):
        raise SnapshotError("disk executable is not the pinned PE32 image")

    loaded_header = _read(reader.read, IMAGE_BASE, 0x1000)
    loaded_image = _parse_pe(loaded_header)
    if (
        loaded_image.machine != EXPECTED_MACHINE
        or loaded_image.optional_magic != EXPECTED_OPTIONAL_MAGIC
        or loaded_image.image_base != IMAGE_BASE
    ):
        raise SnapshotError("resident image is non-PE32 or rebased")

    locator_checks: list[dict[str, object]] = []
    for va, size in LOCATOR_SIZES.items():
        disk_offset = disk_image.file_offset_for_rva(va - IMAGE_BASE, size)
        disk_bytes = disk_data[disk_offset : disk_offset + size]
        resident_bytes = _read(reader.read, va, size)
        check = {
            "va": va,
            "size": size,
            "disk": disk_bytes.hex(),
            "resident": resident_bytes.hex(),
            "match": resident_bytes == disk_bytes,
        }
        locator_checks.append(check)
        if not check["match"]:
            raise SnapshotError(f"loaded locator bytes rejected at 0x{va:08x}")
    return {
        "executable_sha256": digest,
        "image_base": IMAGE_BASE,
        "machine": disk_image.machine,
        "optional_magic": disk_image.optional_magic,
        "locators": locator_checks,
    }


class WindowsProcessReader:
    """A process handle with only query and virtual-memory read rights."""

    PROCESS_QUERY_INFORMATION = 0x0400
    PROCESS_VM_READ = 0x0010

    def __init__(self, pid: int) -> None:
        if os.name != "nt":
            raise SnapshotError("the Windows process backend requires Windows")
        if not isinstance(pid, int) or pid <= 0 or pid > 0xFFFFFFFF:
            raise SnapshotError("PID must be a positive 32-bit integer")
        self.pid = pid
        self._kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self._kernel.OpenProcess.argtypes = [
            wintypes.DWORD,
            wintypes.BOOL,
            wintypes.DWORD,
        ]
        self._kernel.OpenProcess.restype = wintypes.HANDLE
        self._kernel.ReadProcessMemory.argtypes = [
            wintypes.HANDLE,
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ]
        self._kernel.ReadProcessMemory.restype = wintypes.BOOL
        self._kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        self._kernel.CloseHandle.restype = wintypes.BOOL
        self._kernel.QueryFullProcessImageNameW.argtypes = [
            wintypes.HANDLE,
            wintypes.DWORD,
            wintypes.LPWSTR,
            ctypes.POINTER(wintypes.DWORD),
        ]
        self._kernel.QueryFullProcessImageNameW.restype = wintypes.BOOL
        self.handle = self._kernel.OpenProcess(
            self.PROCESS_QUERY_INFORMATION | self.PROCESS_VM_READ,
            False,
            pid,
        )
        if not self.handle:
            raise SnapshotError(f"OpenProcess failed: {ctypes.get_last_error()}")

    def image_path(self) -> str:
        size = wintypes.DWORD(32768)
        buffer = ctypes.create_unicode_buffer(size.value)
        if not self._kernel.QueryFullProcessImageNameW(
            self.handle, 0, buffer, ctypes.byref(size)
        ):
            raise SnapshotError(
                f"QueryFullProcessImageNameW failed: {ctypes.get_last_error()}"
            )
        return buffer.value

    def read(self, address: int, size: int) -> bytes:
        if not isinstance(address, int) or not isinstance(size, int):
            raise SnapshotReadError("address and size must be integers")
        if address < 0 or address > ctypes.c_void_p(-1).value or size < 0:
            raise SnapshotReadError("read is outside native address range")
        if size > MAX_READ_SIZE or address + size > ctypes.c_void_p(-1).value + 1:
            raise SnapshotReadError("read is outside native address range")
        buffer = ctypes.create_string_buffer(size)
        count = ctypes.c_size_t(0)
        success = self._kernel.ReadProcessMemory(
            self.handle,
            ctypes.c_void_p(address),
            ctypes.cast(buffer, ctypes.c_void_p),
            size,
            ctypes.byref(count),
        )
        if not success or count.value != size:
            raise SnapshotReadError(
                f"ReadProcessMemory failed at 0x{address:08x}: "
                f"{ctypes.get_last_error()} ({count.value}/{size})"
            )
        return buffer.raw[:size]

    def close(self) -> None:
        handle = getattr(self, "handle", None)
        if handle:
            self._kernel.CloseHandle(handle)
            self.handle = None

    def __enter__(self) -> "WindowsProcessReader":
        return self

    def __exit__(self, _exc_type, _exc, _tb) -> None:
        self.close()


def _duration(value: str) -> int:
    try:
        parsed = int(value, 10)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "seconds must be an integer from 1 to 30"
        ) from exc
    if parsed < 1 or parsed > 30:
        raise argparse.ArgumentTypeError("seconds must be an integer from 1 to 30")
    return parsed


def _region(value: str) -> int:
    try:
        parsed = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "expected-region must be a nonzero 16-bit integer"
        ) from exc
    if parsed < 1 or parsed > 0xFFFF:
        raise argparse.ArgumentTypeError(
            "expected-region must be a nonzero 16-bit integer"
        )
    return parsed


def _pid(value: str) -> int:
    try:
        parsed = int(value, 10)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("pid must be a positive integer") from exc
    if parsed <= 0 or parsed > 0xFFFFFFFF:
        raise argparse.ArgumentTypeError("pid must be a positive 32-bit integer")
    return parsed


def _record(sequence: int, kind: str, body: dict[str, object]) -> dict[str, object]:
    record = {
        "sequence": sequence,
        "monotonic_ns": time.monotonic_ns(),
        "utc_filetime": (time.time_ns() // 100) + WINDOWS_EPOCH_FILETIME_OFFSET,
        "kind": kind,
    }
    record.update(body)
    return record


def _write_records(path: Path, records: list[dict[str, object]]) -> None:
    try:
        with path.open("x", encoding="ascii", newline="\n") as output:
            for record in records:
                output.write(json.dumps(record, sort_keys=True, separators=(",", ":")))
                output.write("\n")
    except FileExistsError as exc:
        raise SnapshotError("Cannot create new output: file already exists") from exc
    except OSError as exc:
        raise SnapshotError(f"Cannot create output: {exc}") from exc


def run_capture(pid: int, expected_region: int, output: Path, seconds: int) -> None:
    """Run a bounded capture while retaining the process handle."""

    if output.exists() or output.is_symlink():
        raise SnapshotError("Cannot create new output: file already exists")
    if output.parent and not output.parent.exists():
        raise SnapshotError("output parent directory does not exist")

    records: list[dict[str, object]] = []
    started = time.monotonic()
    with WindowsProcessReader(pid) as reader:
        identity = _profile_identity(reader)
        records.append(
            _record(
                0,
                "identity",
                {
                    "format_version": 1,
                    "pid": pid,
                    "expected_region": expected_region,
                    **identity,
                },
            )
        )
        sequence = 1
        stop_reason = "duration"
        stable_samples = 0
        stable_error_samples = 0
        error_samples = 0
        try:
            while time.monotonic() - started < seconds:
                if sequence - 1 >= MAX_SAMPLES:
                    stop_reason = "record_limit"
                    break
                observation = collect_stable_sample(reader.read, expected_region)
                kind = observation.pop("kind")
                if kind == "snapshot":
                    stable_samples += 1
                elif observation.get("stable"):
                    stable_error_samples += 1
                else:
                    error_samples += 1
                records.append(_record(sequence, kind, observation))
                sequence += 1
                remaining = seconds - (time.monotonic() - started)
                if remaining > 0:
                    time.sleep(min(0.25, remaining))
        except KeyboardInterrupt:
            stop_reason = "keyboard_interrupt"
        records.append(
            _record(
                sequence,
                "summary",
                {
                    "samples": sequence - 1,
                    "stable_samples": stable_samples,
                    "stable_error_samples": stable_error_samples,
                    "error_samples": error_samples,
                    "stop_reason": stop_reason,
                    "record_limit": MAX_SAMPLES,
                },
            )
        )
        _write_records(output, records)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", required=True, type=_pid)
    parser.add_argument("--expected-region", required=True, type=_region)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--seconds", default="12", type=_duration)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        run_capture(args.pid, args.expected_region, args.output, args.seconds)
    except KeyboardInterrupt:
        print("capture interrupted", file=sys.stderr)
        return 130
    except SnapshotError as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
