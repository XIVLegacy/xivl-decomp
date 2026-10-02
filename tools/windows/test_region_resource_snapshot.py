#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Asset-free tests for the read-only region resource snapshot reader."""

from __future__ import annotations

import ctypes
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parent))
import snapshot_region_resource as snapshot  # noqa: E402


class Memory:
    def __init__(self) -> None:
        self._bytes: dict[int, int] = {}

    def set_bytes(self, address: int, value: bytes) -> None:
        self._bytes.update({address + index: byte for index, byte in enumerate(value)})

    def set_u8(self, address: int, value: int) -> None:
        self.set_bytes(address, bytes([value]))

    def set_u32(self, address: int, value: int) -> None:
        self.set_bytes(address, value.to_bytes(4, "little"))

    def read(self, address: int, size: int) -> bytes:
        try:
            return bytes(self._bytes[address + index] for index in range(size))
        except KeyError as exc:
            missing = exc.args[0]
            raise OSError(f"unmapped byte at 0x{missing:x}") from exc


class ProfileReader:
    def __init__(self, disk_image: bytes) -> None:
        self._resident_header = bytearray(disk_image[:0x1000])
        self._locators = {
            address: disk_image[
                0x400 + address - snapshot.IMAGE_BASE : 0x400
                + address
                - snapshot.IMAGE_BASE
                + size
            ]
            for address, size in snapshot.LOCATOR_SIZES.items()
        }
        self.short_header = False

    def image_path(self) -> str:
        return r"C:\synthetic\pinned.exe"

    def read(self, address: int, size: int) -> bytes:
        if address == snapshot.IMAGE_BASE:
            if self.short_header:
                return bytes(self._resident_header[: size - 1])
            return bytes(self._resident_header[:size])
        try:
            return self._locators[address][:size]
        except KeyError as exc:
            raise OSError(f"unmapped synthetic address 0x{address:x}") from exc


class PinnedDigest:
    def hexdigest(self) -> str:
        return snapshot.EXPECTED_SHA256


def synthetic_profile_image(
    *,
    machine: int = snapshot.EXPECTED_MACHINE,
    optional_magic: int = snapshot.EXPECTED_OPTIONAL_MAGIC,
    image_base: int = snapshot.IMAGE_BASE,
) -> bytes:
    max_rva = max(
        address - snapshot.IMAGE_BASE + size
        for address, size in snapshot.LOCATOR_SIZES.items()
    )
    raw_size = (max_rva + 0xFFF) & ~0xFFF
    image = bytearray(0x400 + raw_size)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 0x3C, 0x80)
    image[0x80:0x84] = b"PE\0\0"
    struct.pack_into(
        "<HHIIIHH",
        image,
        0x84,
        machine,
        1,
        0,
        0,
        0,
        0xE0,
        0,
    )
    optional = 0x98
    struct.pack_into("<H", image, optional, optional_magic)
    struct.pack_into("<I", image, optional + 28, image_base)
    section = 0x178
    image[section : section + 8] = b".text\0\0\0"
    struct.pack_into("<IIII", image, section + 8, raw_size, 0, raw_size, 0x400)
    for index, (address, size) in enumerate(snapshot.LOCATOR_SIZES.items()):
        offset = 0x400 + address - snapshot.IMAGE_BASE
        image[offset : offset + size] = bytes(
            ((index + value) & 0xFF) for value in range(size)
        )
    return bytes(image)


SCENE = 0x02000000
MANAGER = 0x02100000
CONTEXT = 0x02200000
REGION_INFO = 0x02300000
DECODER = 0x02400000
DECODER_VTABLE = 0x02500000
CONTEXT_OBJECT = 0x02600000
CONTEXT_OBJECT_TWO = 0x02610000
ACTOR = 0x02700000
FORWARDED = 0x02800000
FORWARDED_VTABLE = 0x02900000
RESOURCE = 0x02A00000


def fixture(*, resource: bool = False, region: int = 202) -> Memory:
    memory = Memory()
    memory.set_u8(snapshot.FORMATTER_MODE, 1)
    memory.set_u32(snapshot.ASYNC_GATE, 0)
    memory.set_u32(snapshot.SCENE_GLOBAL, SCENE)
    memory.set_u32(SCENE + 0x114, CONTEXT)
    memory.set_u32(SCENE + 0x17C, MANAGER)
    memory.set_u32(MANAGER, snapshot.EXPECTED_MANAGER_VTABLE)
    memory.set_u32(MANAGER + 0x14, region)
    memory.set_u32(MANAGER + 0x10, REGION_INFO)
    memory.set_u32(REGION_INFO + 0xB0, 0x12345678)
    memory.set_u32(MANAGER + 0x3C, snapshot.EXPECTED_PRIMARY_VTABLE)
    memory.set_u32(MANAGER + 0x40, snapshot.EXPECTED_SECONDARY_VTABLE)
    memory.set_u32(MANAGER + 0x44, ACTOR)
    memory.set_u32(MANAGER + 0x50, snapshot.EXPECTED_REGION_MODE)
    memory.set_u32(MANAGER + 0x54, RESOURCE if resource else 0)
    memory.set_u32(MANAGER + 0x58, 0)
    memory.set_u32(MANAGER + 0x5C, 0)
    memory.set_u32(CONTEXT + 0x08, DECODER)
    memory.set_u32(CONTEXT + 0x10, CONTEXT_OBJECT)
    memory.set_u32(DECODER, DECODER_VTABLE)
    memory.set_u32(DECODER_VTABLE + 0x58, 0x03000000)
    memory.set_u32(DECODER_VTABLE + 0x5C, 0x03000004)
    memory.set_u32(CONTEXT_OBJECT + 0x7C, ACTOR)
    memory.set_u32(ACTOR, snapshot.EXPECTED_COMMON_ACTOR_VTABLE)
    memory.set_u32(ACTOR + 0x08, FORWARDED)
    memory.set_u32(FORWARDED, FORWARDED_VTABLE)
    memory.set_u32(FORWARDED_VTABLE + 0x18, 0x03000008)
    if resource:
        memory.set_u32(RESOURCE, snapshot.EXPECTED_RESOURCE_VTABLE)
        memory.set_u32(RESOURCE + 0x58, 0x10203040)
        memory.set_u32(RESOURCE + 0x5C, 0x50607080)
        memory.set_u32(RESOURCE + 0x64, 0x03100000)
        memory.set_u32(RESOURCE + 0xB0, 7)
    return memory


class SnapshotTests(unittest.TestCase):
    def test_valid_default_null_auxiliary(self) -> None:
        row = snapshot.collect_snapshot(fixture().read, expected_region=202)
        self.assertEqual(row["status"], "snapshot")
        self.assertEqual(row["region"]["root_key"], 0x12345678)
        self.assertIsNone(row["resource"])
        self.assertEqual(row["decoder"]["sync_target"], 0x03000000)
        self.assertTrue(row["actor"]["vtable_matches"])

    def test_valid_nonnull_resource(self) -> None:
        row = snapshot.collect_snapshot(
            fixture(resource=True).read, expected_region=202
        )
        self.assertEqual(row["status"], "snapshot")
        self.assertEqual(row["resource"]["key"], 0x10203040)
        self.assertEqual(row["resource"]["raw_decode_arg"], 0x50607080)
        self.assertEqual(row["resource"]["state"], 7)

    def test_wrong_resource_vtable_skips_resource_fields(self) -> None:
        memory = fixture(resource=True)
        memory.set_u32(RESOURCE, 0x0BADF00D)
        row = snapshot.collect_snapshot(memory.read, expected_region=202)
        self.assertEqual(row["status"], "rejected")
        self.assertFalse(row["resource"]["vtable_matches"])
        self.assertNotIn("key", row["resource"])

    def test_wrong_region_is_read_check_failure(self) -> None:
        row = snapshot.collect_snapshot(fixture(region=203).read, expected_region=202)
        self.assertEqual(row["status"], "rejected")
        self.assertIn("expected_region", row["validation_errors"])
        self.assertFalse(row["region"]["expected_match"])

    def test_stable_rejected_sample_is_unusable_error(self) -> None:
        row = snapshot.collect_stable_sample(
            fixture(region=203).read, expected_region=202
        )
        self.assertEqual(row["kind"], "error")
        self.assertEqual(row["status"], "unusable")
        self.assertEqual(row["error_code"], "rejected_snapshot")
        self.assertEqual(row["observation_status"], "rejected")
        self.assertTrue(row["stable"])

    def test_stable_uninitialized_sample_is_unusable_error(self) -> None:
        memory = fixture()
        memory.set_u32(snapshot.SCENE_GLOBAL, 0)
        row = snapshot.collect_stable_sample(memory.read, expected_region=202)
        self.assertEqual(row["kind"], "error")
        self.assertEqual(row["status"], "unusable")
        self.assertEqual(row["error_code"], "uninitialized_scene")
        self.assertEqual(row["observation_status"], "uninitialized")
        self.assertTrue(row["stable"])

    def test_wrong_event_and_mode_stop_derived_reads(self) -> None:
        memory = fixture()
        memory.set_u32(MANAGER + 0x3C, 0x0BADF00D)
        memory.set_u32(MANAGER + 0x50, 2)
        row = snapshot.collect_snapshot(memory.read, expected_region=202)
        self.assertEqual(row["status"], "rejected")
        self.assertIsNone(row["region"])
        self.assertIn("primary_vtable", row["validation_errors"])
        self.assertIn("mode", row["validation_errors"])

    def test_null_uninitialized_scene(self) -> None:
        memory = fixture()
        memory.set_u32(snapshot.SCENE_GLOBAL, 0)
        row = snapshot.collect_snapshot(memory.read, expected_region=202)
        self.assertEqual(row["status"], "uninitialized")
        self.assertIsNone(row["scene"])

    def test_unreadable_pointer_is_explicit_error(self) -> None:
        memory = fixture()
        for index in range(4):
            del memory._bytes[REGION_INFO + 0xB0 + index]
        row = snapshot.collect_snapshot(memory.read, expected_region=202)
        self.assertEqual(row["status"], "read_error")
        self.assertTrue(
            any(error["field"] == "region_info.root_key" for error in row["errors"])
        )

    def test_actor_mismatch_skips_foreign_layout(self) -> None:
        memory = fixture()
        memory.set_u32(CONTEXT_OBJECT + 0x7C, 0x02F00000)
        row = snapshot.collect_snapshot(memory.read, expected_region=202)
        self.assertEqual(row["status"], "snapshot")
        self.assertFalse(row["actor"]["matches_event_actor"])
        self.assertEqual(row["actor"]["layout_skipped"], "actor_mismatch")

    def test_snapshot_race_retains_both_collections(self) -> None:
        memory = fixture()
        reads = 0

        def changing_read(address: int, size: int) -> bytes:
            nonlocal reads
            reads += 1
            if reads > 17 and address == MANAGER + 0x50:
                return (2).to_bytes(4, "little")
            return memory.read(address, size)

        row = snapshot.collect_stable_sample(changing_read, expected_region=202)
        self.assertEqual(row["kind"], "error")
        self.assertEqual(row["error_code"], "snapshot_mismatch")
        self.assertIn("first", row)
        self.assertIn("second", row)

    def test_context_identity_change_is_not_stable(self) -> None:
        memory = fixture()
        memory.set_u32(CONTEXT_OBJECT_TWO + 0x7C, ACTOR)
        object_reads = 0

        def changing_read(address: int, size: int) -> bytes:
            nonlocal object_reads
            if address == CONTEXT + 0x10:
                object_reads += 1
                if object_reads == 2:
                    return CONTEXT_OBJECT_TWO.to_bytes(4, "little")
            return memory.read(address, size)

        row = snapshot.collect_stable_sample(changing_read, expected_region=202)
        self.assertEqual(row["kind"], "error")
        self.assertEqual(row["error_code"], "snapshot_mismatch")
        self.assertNotEqual(
            row["first"]["context_detail"]["object"],
            row["second"]["context_detail"]["object"],
        )
        self.assertEqual(row["first"]["context"], row["second"]["context"])

    def test_existing_output_is_refused_before_open_process(self) -> None:
        with tempfile.TemporaryDirectory(prefix="region-resource-") as directory:
            output = Path(directory) / "existing.jsonl"
            output.write_text("keep\n", encoding="ascii")
            with mock.patch.object(
                snapshot,
                "WindowsProcessReader",
                side_effect=AssertionError("target was touched"),
            ):
                with self.assertRaises(snapshot.SnapshotError):
                    snapshot.run_capture(1, 202, output, 1)

    def test_invalid_duration_is_rejected(self) -> None:
        with self.assertRaises(SystemExit):
            snapshot.build_parser().parse_args(
                [
                    "--pid",
                    "1",
                    "--expected-region",
                    "202",
                    "--seconds",
                    "31",
                    "--output",
                    "x",
                ]
            )

    def test_oversized_pid_is_rejected(self) -> None:
        with self.assertRaises(SystemExit):
            snapshot.build_parser().parse_args(
                [
                    "--pid",
                    "4294967296",
                    "--expected-region",
                    "202",
                    "--seconds",
                    "1",
                    "--output",
                    "x",
                ]
            )

    def test_oversized_pid_is_rejected_by_backend_before_open(self) -> None:
        with mock.patch.object(snapshot.os, "name", "nt"):
            with mock.patch.object(
                snapshot.ctypes,
                "WinDLL",
                side_effect=AssertionError("OpenProcess path was reached"),
                create=True,
            ):
                with self.assertRaises(snapshot.SnapshotError):
                    snapshot.WindowsProcessReader(0x1_0000_0000)

    def test_profile_accepts_synthetic_pinned_image(self) -> None:
        image = synthetic_profile_image()
        reader = ProfileReader(image)
        with mock.patch.object(Path, "read_bytes", return_value=image):
            with mock.patch.object(
                snapshot.hashlib, "sha256", return_value=PinnedDigest()
            ):
                result = snapshot._profile_identity(reader)
        self.assertEqual(result["executable_sha256"], snapshot.EXPECTED_SHA256)
        self.assertEqual(len(result["locators"]), len(snapshot.LOCATOR_SIZES))

    def test_profile_rejects_mutated_consumer_site(self) -> None:
        image = synthetic_profile_image()
        cases = (
            (0x00631DD2, 3),
            (0x0044B3CB, 7),
            (0x00631D0D, 7),
            (0x0061FEA0, 3),
        )
        for address, size in cases:
            with self.subTest(address=hex(address)):
                reader = ProfileReader(image)
                reader._locators[address] = b"\xff" * size
                with mock.patch.object(Path, "read_bytes", return_value=image):
                    with mock.patch.object(
                        snapshot.hashlib, "sha256", return_value=PinnedDigest()
                    ):
                        with self.assertRaisesRegex(
                            snapshot.SnapshotError, "loaded locator bytes rejected"
                        ):
                            snapshot._profile_identity(reader)

    def test_profile_rejects_wrong_sha(self) -> None:
        image = synthetic_profile_image()
        reader = ProfileReader(image)
        with mock.patch.object(Path, "read_bytes", return_value=image):
            with self.assertRaisesRegex(
                snapshot.SnapshotError, "target executable SHA-256 rejected"
            ):
                snapshot._profile_identity(reader)

    def test_profile_rejects_non_pinned_disk_headers(self) -> None:
        cases = (
            {"machine": 0x8664},
            {"optional_magic": 0x20B},
            {"image_base": 0x00500000},
        )
        for options in cases:
            with self.subTest(options=options):
                image = synthetic_profile_image(**options)
                reader = ProfileReader(image)
                with mock.patch.object(Path, "read_bytes", return_value=image):
                    with mock.patch.object(
                        snapshot.hashlib, "sha256", return_value=PinnedDigest()
                    ):
                        with self.assertRaisesRegex(
                            snapshot.SnapshotError,
                            "disk executable is not the pinned PE32 image",
                        ):
                            snapshot._profile_identity(reader)

    def test_profile_rejects_bad_or_short_resident_header(self) -> None:
        image = synthetic_profile_image()
        for description, mutate in (
            ("machine", lambda header: struct.pack_into("<H", header, 0x84, 0x8664)),
            (
                "optional magic",
                lambda header: struct.pack_into("<H", header, 0x98, 0x20B),
            ),
            (
                "image base",
                lambda header: struct.pack_into("<I", header, 0xB4, 0x00500000),
            ),
        ):
            with self.subTest(description=description):
                reader = ProfileReader(image)
                mutate(reader._resident_header)
                with mock.patch.object(Path, "read_bytes", return_value=image):
                    with mock.patch.object(
                        snapshot.hashlib, "sha256", return_value=PinnedDigest()
                    ):
                        with self.assertRaisesRegex(
                            snapshot.SnapshotError,
                            "resident image is non-PE32 or rebased",
                        ):
                            snapshot._profile_identity(reader)

        reader = ProfileReader(image)
        reader.short_header = True
        with mock.patch.object(Path, "read_bytes", return_value=image):
            with mock.patch.object(
                snapshot.hashlib, "sha256", return_value=PinnedDigest()
            ):
                with self.assertRaisesRegex(snapshot.SnapshotError, "short read"):
                    snapshot._profile_identity(reader)

    def test_summary_counts_stable_invalid_samples(self) -> None:
        class FakeReader:
            def __enter__(self):
                return self

            def __exit__(self, _exc_type, _exc, _tb):
                return None

            def read(self, _address, _size):
                raise AssertionError("collector should be mocked")

        observations = iter(
            (
                {
                    "kind": "error",
                    "error_code": "rejected_snapshot",
                    "observation_status": "rejected",
                    "status": "unusable",
                    "stable": True,
                    "snapshot": {},
                },
                {
                    "kind": "error",
                    "error_code": "uninitialized_scene",
                    "observation_status": "uninitialized",
                    "status": "unusable",
                    "stable": True,
                    "snapshot": {},
                },
            )
        )
        with tempfile.TemporaryDirectory(prefix="region-resource-") as directory:
            output = Path(directory) / "summary.jsonl"
            with mock.patch.object(
                snapshot, "WindowsProcessReader", return_value=FakeReader()
            ):
                with mock.patch.object(
                    snapshot,
                    "_profile_identity",
                    return_value={"executable_sha256": snapshot.EXPECTED_SHA256},
                ):
                    with mock.patch.object(
                        snapshot, "collect_stable_sample", side_effect=observations
                    ):
                        with mock.patch.object(
                            snapshot.time,
                            "monotonic",
                            side_effect=(0, 0, 0, 0, 0, 2),
                        ):
                            with mock.patch.object(snapshot.time, "sleep"):
                                snapshot.run_capture(1, 202, output, 1)
            summary = json.loads(output.read_text(encoding="ascii").splitlines()[-1])
        self.assertEqual(summary["stable_samples"], 0)
        self.assertEqual(summary["stable_error_samples"], 2)
        self.assertEqual(summary["error_samples"], 0)

    def test_record_uses_windows_filetime_epoch(self) -> None:
        with mock.patch.object(snapshot.time, "monotonic_ns", return_value=17):
            with mock.patch.object(snapshot.time, "time_ns", return_value=0):
                epoch = snapshot._record(1, "test", {})
            with mock.patch.object(
                snapshot.time, "time_ns", return_value=1_000_000_000
            ):
                one_second = snapshot._record(2, "test", {})
        self.assertEqual(epoch["utc_filetime"], 116444736000000000)
        self.assertEqual(one_second["utc_filetime"], 116444736010000000)
        self.assertEqual(epoch["monotonic_ns"], 17)
        self.assertEqual(one_second["monotonic_ns"], 17)


@unittest.skipUnless(os.name == "nt", "Windows backend smoke test")
class WindowsBackendTests(unittest.TestCase):
    def test_self_process_read_and_close(self) -> None:
        buffer = ctypes.create_string_buffer(b"region-rpm-smoke")
        address = ctypes.addressof(buffer)
        with snapshot.WindowsProcessReader(os.getpid()) as reader:
            self.assertEqual(reader.read(address, len(buffer.raw)), buffer.raw)
            self.assertIsNotNone(reader.handle)
        self.assertIsNone(reader.handle)


if __name__ == "__main__":
    unittest.main()
