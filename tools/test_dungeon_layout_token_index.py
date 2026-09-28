import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools import build_dungeon_layout_token_index as index


class DungeonLayoutTokenIndexTests(unittest.TestCase):
    def test_token_classification(self):
        cases = {
            "time_door_a1_open": "timeline",
            "sdef_door_a1_open": "scheduler_definition",
            "open": "short_command",
            "vfx_door_spark": "vfx_resource",
            "sgrp_door_a1": "control_group",
            "LayCollisionOnOffClip": "clip_class",
            "door_a1_collision": "semantic_resource",
        }
        for token, expected in cases.items():
            with self.subTest(token=token):
                self.assertEqual(index.token_class(token), expected)

    def test_scan_keeps_first_offset_and_occurrence_count(self):
        data = b"open\0open\0time_door_a1_open\0door_a1_collision\0"
        digest = hashlib.sha256(data).hexdigest()
        rows = index.scan_dat("0x29B00008", data, digest)
        by_token = {str(row["token"]): row for row in rows}

        self.assertEqual(by_token["open"]["first_offset_hex"], "0x0")
        self.assertEqual(by_token["open"]["occurrences"], 2)
        self.assertEqual(by_token["time_door_a1_open"]["first_offset_hex"], "0xA")
        self.assertEqual(
            by_token["door_a1_collision"]["token_class"], "semantic_resource"
        )
        self.assertEqual(
            by_token["door_a1_collision"]["relative_path"], "data/29/B0/00/08.DAT"
        )

    def test_build_rejects_a_hash_mismatch(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            client_root = Path(temp_dir)
            first_key = "0x00000000"
            first_path = client_root / index.dat_relative_path(first_key)
            first_path.parent.mkdir(parents=True)
            first_path.write_bytes(b"open\0")
            fake_pins = {f"0x{value:08X}": "0" * 64 for value in range(24)}

            with patch.dict(index.DATS, fake_pins, clear=True):
                with self.assertRaisesRegex(ValueError, "hash mismatch"):
                    index.build_rows(client_root)


if __name__ == "__main__":
    unittest.main()
