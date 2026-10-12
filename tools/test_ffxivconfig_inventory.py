import os
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import extract_ffxivconfig_inventory as inventory  # noqa: E402


CONFIG_ENV = "XIVL_CONFIG_EXE"
CONFIG_PATH = (
    Path(config_value) if (config_value := os.environ.get(CONFIG_ENV)) else None
)


class FfxivconfigInventoryTests(unittest.TestCase):
    def test_extended_empty_dialog_is_consumed(self):
        payload = (
            struct.pack(
                "<HHIIIHhhhh",
                1,
                0xFFFF,
                0,
                0,
                0,
                0,
                0,
                0,
                10,
                10,
            )
            + b"\0\0\0\0\0\0"
        )
        parsed = inventory.parse_dialog(payload)
        self.assertTrue(parsed["extended_template"])
        self.assertEqual(parsed["control_count"], 0)
        self.assertEqual(parsed["controls"], [])

    def test_classic_empty_dialog_is_consumed(self):
        payload = struct.pack("<IIHhhhh", 0, 0, 0, 0, 0, 10, 10) + b"\0" * 6
        parsed = inventory.parse_dialog(payload)
        self.assertFalse(parsed["extended_template"])
        self.assertEqual(parsed["control_count"], 0)
        self.assertEqual(parsed["controls"], [])

    def test_classic_control_creation_data_count_includes_size_word(self):
        payload = (
            struct.pack("<IIHhhhh", 0, 0, 1, 0, 0, 10, 10)
            + b"\0" * 6
            + struct.pack("<IIhhhhH", 0, 0, 0, 0, 1, 1, 1)
            + b"\0\0\0\0"
            + struct.pack("<H", 4)
            + b"\xaa\xbb"
        )
        parsed = inventory.parse_dialog(payload)
        self.assertFalse(parsed["extended_template"])
        self.assertEqual(parsed["control_count"], 1)
        self.assertEqual(parsed["controls"][0]["extra_hex"], "aabb")

    def test_identity_gate_rejects_non_target(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "ffxivconfig.exe"
            path.write_bytes(b"not the pinned image")
            with self.assertRaisesRegex(ValueError, "expected 3471240 bytes"):
                inventory.Image(path)

    def test_output_path_rejects_input_binary_alias(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "ffxivconfig.exe"
            path.write_bytes(b"not the pinned image")
            with self.assertRaisesRegex(
                ValueError, "output path must differ from input binary"
            ):
                inventory.ensure_distinct_output(path, path)

    def test_rva_to_offset_rejects_virtual_zero_tail_and_negative_inputs(self):
        image = object.__new__(inventory.Image)
        image.data = b"\0" * 0x400
        image.sections = [(0x1000, 0x1000, 0x20, 0x200, ".test")]
        self.assertEqual(image.rva_to_offset(0x101F, 1), 0x21F)
        with self.assertRaisesRegex(ValueError, "not file-backed"):
            image.rva_to_offset(0x1020, 1)
        with self.assertRaisesRegex(ValueError, "non-negative"):
            image.rva_to_offset(-1, 1)
        with self.assertRaisesRegex(ValueError, "non-negative"):
            image.rva_to_offset(0x1000, -1)

    def test_optional_pinned_binary_inventory(self):
        if CONFIG_PATH is None:
            self.skipTest(f"set {CONFIG_ENV} for the optional retail fixture check")
        if not CONFIG_PATH.is_file():
            self.skipTest(f"{CONFIG_ENV} does not name a file: {CONFIG_PATH}")
        document = inventory.extract(CONFIG_PATH)
        self.assertEqual(document["coverage"]["dialog_count"], 17)
        self.assertEqual(document["coverage"]["control_count"], 143)
        self.assertEqual(
            [dialog["resource_id"] for dialog in document["dialogs"]],
            [
                101,
                102,
                103,
                110,
                111,
                112,
                117,
                118,
                119,
                122,
                123,
                124,
                125,
                126,
                127,
                128,
                132,
            ],
        )
        self.assertEqual(document["config_sys"]["current_size"], "0x2ac")
        locator_cases = [
            (
                document["localization"]["language_file"],
                "suffix_va",
                "suffix_rva",
                0x0044D81C,
            ),
            (
                document["localization"]["binary_resource_loader"],
                "type_locator_va",
                "type_locator_rva",
                0x0044B4B0,
            ),
            (
                document["config_sys"],
                "suffix_va",
                "suffix_rva",
                0x0044D0E8,
            ),
            (
                document["config_sys"],
                "current_block_va",
                "current_block_rva",
                0x0045C600,
            ),
            (
                document["config_sys"],
                "snapshot_block_va",
                "snapshot_block_rva",
                0x0045C8B0,
            ),
            (
                document["config_sys"],
                "read_buffer_va",
                "read_buffer_rva",
                0x0045CC78,
            ),
            (
                document["controller_mapping_helper"],
                "table_va",
                "table_rva",
                0x0044D6C0,
            ),
        ]
        for locator, va_key, rva_key, va in locator_cases:
            self.assertEqual(int(locator[va_key], 16), va)
            self.assertEqual(int(locator[rva_key], 16), va - inventory.IMAGE_BASE)
        expected_resource_languages = {
            ("BINARY", 113): 0,
            ("BINARY", 114): 0,
            ("BINARY", 115): 0,
            ("BINARY", 116): 0,
            ("BINARY", 133): 0,
            ("RT_BITMAP", 120): 1041,
            ("RT_BITMAP", 121): 1041,
            ("RT_BITMAP", 130): 0,
            ("RT_BITMAP", 131): 1041,
            ("RT_ICON", 1): 1041,
            ("RT_ICON", 2): 1041,
            ("RT_ICON", 3): 1041,
            ("RT_ICON", 4): 1041,
            ("RT_ICON", 5): 1041,
            ("RT_ICON", 6): 1041,
            ("RT_ICON", 7): 1041,
            ("RT_DIALOG", 101): 0,
            ("RT_DIALOG", 102): 0,
            ("RT_DIALOG", 103): 0,
            ("RT_DIALOG", 110): 0,
            ("RT_DIALOG", 111): 0,
            ("RT_DIALOG", 112): 0,
            ("RT_DIALOG", 117): 0,
            ("RT_DIALOG", 118): 0,
            ("RT_DIALOG", 119): 0,
            ("RT_DIALOG", 122): 0,
            ("RT_DIALOG", 123): 0,
            ("RT_DIALOG", 124): 0,
            ("RT_DIALOG", 125): 0,
            ("RT_DIALOG", 126): 0,
            ("RT_DIALOG", 127): 0,
            ("RT_DIALOG", 128): 0,
            ("RT_DIALOG", 132): 0,
            ("RT_GROUP_ICON", 100): 1041,
            ("RT_VERSION", 1): 0,
            ("RT_MANIFEST", 1): 1033,
        }
        actual_resource_languages = {
            (row["type_name"], row["name"]): row["language"]
            for row in document["resource_directory"]["entries"]
        }
        self.assertEqual(actual_resource_languages, expected_resource_languages)
        defaults = {
            item["offset"]: item["value"]
            for item in document["config_sys"]["default_dwords"]
        }
        self.assertEqual(defaults["0x50"], 1)
        self.assertEqual(
            document["config_sys"]["field_joins"]["unjoined_current_dword_offsets"],
            [],
        )
        legacy_conversion = document["config_sys"]["field_joins"]["conversion_maps"][
            "20100706"
        ]
        self.assertIn("legacy_value*4", legacy_conversion["producer"])
        self.assertIn(
            "indexed by the legacy dword value",
            legacy_conversion["legacy_dword_to_current"][6]["conversion"],
        )
        save_paths = document["config_sys"]["field_joins"]["save_cancel_paths"]
        self.assertEqual(
            save_paths["ok_save"]["page_capture_order"],
            ["network", "display", "graphics", "sound", "font"],
        )
        self.assertEqual(
            save_paths["ok_save"]["global_gate"]["accepted_unsigned_min"],
            "0xd800",
        )
        self.assertEqual(
            save_paths["ok_save"]["global_gate"]["accepted_unsigned_max"],
            "0xd8ff",
        )
        self.assertEqual(
            save_paths["cancel_discard"]["page_capture_order"], ["network"]
        )
        window_close = save_paths["window_close"]
        self.assertEqual(window_close["status"], "observed")
        self.assertEqual(window_close["callback"], "LAB_00442570")
        self.assertTrue(window_close["function_start_unresolved"])
        self.assertEqual(
            window_close["page_capture_order"],
            ["network", "display", "graphics", "sound", "font"],
        )
        self.assertIn("0x0044284D", window_close["changed_action"])
        self.assertIn("0x0044285B..0x00442862", window_close["unchanged_action"])
        display = next(
            dialog for dialog in document["dialogs"] if dialog["resource_id"] == 126
        )
        display_controls = {control["id"]: control for control in display["controls"]}
        self.assertEqual(
            display_controls[1100]["join"]["file_offsets"], ["0x14", "0x18"]
        )
        self.assertEqual(
            display_controls[1102]["join"]["file_offsets"], ["0x24", "0x28"]
        )
        input_test = next(
            dialog for dialog in document["dialogs"] if dialog["resource_id"] == 102
        )
        self.assertEqual(
            input_test["runtime"]["caption_source"],
            "runtime_localized_lookup_FUN_0040d350",
        )
        self.assertEqual(input_test["localized_titles"]["1"], "Test Gamepad")
        input_test_controls = {
            control["id"]: control for control in input_test["controls"]
        }
        self.assertEqual(input_test_controls[1]["join"]["kind"], "close_command")
        for control_id in (0x3FD, 0x3FE, 0x3FF):
            self.assertEqual(
                input_test_controls[control_id]["join"]["kind"],
                "input_test_visual_control",
            )
        for control_id in (1045, 1046, 1047):
            self.assertEqual(
                input_test_controls[control_id]["join"]["status"], "unknown"
            )
        self.assertEqual(document["localization"]["language_file"]["size"], 8)
        self.assertEqual(len(document["controller_mapping_helper"]["pairs"]), 24)
        tables = document["localization"]["binary_resource_loader"]["decoded_tables"][
            "tables"
        ]
        self.assertEqual([table["record_count"] for table in tables], [190] * 5)
        english = next(table for table in tables if table["language_index"] == 1)
        self.assertEqual(
            next(record for record in english["records"] if record["key"] == 0xF0)[
                "text"
            ],
            "Activate gamepad vibration",
        )
        controller_vibration = next(
            control
            for control in document["dialogs"][0]["controls"]
            if control["id"] == 1038
        )
        self.assertEqual(
            controller_vibration["localized_captions"]["1"],
            "Activate gamepad vibration",
        )
        for dialog_id in (101, 102, 103):
            dialog = next(
                item for item in document["dialogs"] if item["resource_id"] == dialog_id
            )
            for control in dialog["controls"]:
                if "caption_key" not in control:
                    self.assertNotEqual(
                        control["caption_source"],
                        "runtime_localized_lookup_FUN_0040d350",
                    )
        controller_controls = {
            control["id"]: control
            for control in next(
                dialog for dialog in document["dialogs"] if dialog["resource_id"] == 101
            )["controls"]
        }
        for control_id in (1044, 1046, 1047, 1068):
            self.assertEqual(
                controller_controls[control_id]["caption_source"],
                "generated_mapping_text_or_literal_dash",
            )
            self.assertIn(
                "DAT_00459140",
                controller_controls[control_id]["caption_source_evidence"],
            )
        self.assertEqual(
            controller_controls[1018]["caption_source"], "device_record_name"
        )
        self.assertIn("+0x3cc", controller_controls[1018]["caption_source_evidence"])
        self.assertEqual(
            controller_controls[1001]["caption_source"], "other_runtime_text"
        )
        self.assertEqual(
            next(
                control
                for control in next(
                    dialog
                    for dialog in document["dialogs"]
                    if dialog["resource_id"] == 127
                )["controls"]
                if control["id"] == 1141
            )["join"]["file_offset"],
            "0xa0",
        )
        self.assertEqual(
            document["dialogs"][0]["controls"][0]["join"]["status"], "unknown"
        )


if __name__ == "__main__":
    unittest.main()
