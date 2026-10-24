"""Asset-free checks for sheet-wide completion ordinal evidence."""

from __future__ import annotations

import copy
import io
import json
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

try:
    from . import build_achievement_completion_ordinals as builder
except ImportError:
    import build_achievement_completion_ordinals as builder

ARTIFACT = (
    Path(__file__).resolve().parents[1] / "config/achievement_completion_ordinals.json"
)


class AchievementCompletionOrdinalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = json.loads(ARTIFACT.read_text(encoding="ascii"))
        cls.title_map = json.loads(builder.TITLE_ARTIFACT.read_text(encoding="ascii"))

    def test_complete_sheet_and_title_subset(self) -> None:
        self.assertEqual(builder.validate_artifact(self.document, self.title_map), [])
        rows = {row["achievement_id"]: row for row in self.document["rows"]}
        self.assertEqual(len(rows), 746)
        ordinals = [
            row["ordinal"] for row in rows.values() if row["status"] == "proven"
        ]
        self.assertEqual(len(ordinals), 741)
        self.assertEqual(len(set(ordinals)), 741)
        self.assertTrue(all(0 <= ordinal < 1024 for ordinal in ordinals))
        self.assertEqual(rows[100]["ordinal"], 0)
        self.assertEqual(
            (
                rows[2082]["ordinal"],
                rows[2082]["result_va"],
                rows[2082]["inverse_result_va"],
            ),
            (1023, "0x0070B88A", "0x0057B8FD"),
        )
        self.assertEqual(
            [
                achievement
                for achievement, row in rows.items()
                if row["ordinal"] is None
            ],
            [1107, 1108, 1109, 1110, 1221],
        )
        for achievement, ordinal in (
            (197, 18),
            (198, 19),
            (390, 146),
            (391, 147),
            (392, 148),
        ):
            self.assertEqual(rows[achievement]["ordinal"], ordinal)
        for title_row in self.title_map["rows"]:
            record = dict(rows[title_row["achievement_id"]])
            record.pop("achievement_id")
            self.assertEqual(record, title_row["direct_completion"])

    def test_default_zero_is_not_assigned_to_unproven_rows(self) -> None:
        class DefaultMapping:
            def evaluate(self, name: str, argument: int) -> tuple[int, str]:
                if name == "achievement_to_ordinal":
                    return 0, "0x0070BCB3"
                if name == "ordinal_to_achievement" and argument == 0:
                    return 100, "0x0057ABCB"
                raise AssertionError((name, argument))

        with patch.object(
            builder.native, "NativeMappings", return_value=DefaultMapping()
        ):
            document = builder.derive(None, [1107, 100])
        legitimate, unknown = document["rows"]
        self.assertEqual(legitimate["achievement_id"], 100)
        self.assertEqual(legitimate["ordinal"], 0)
        self.assertEqual(legitimate["status"], "proven")
        self.assertEqual(unknown["status"], "unproven_inverse_mismatch")
        self.assertIsNone(unknown["ordinal"])
        self.assertEqual(unknown["native_result"], 0)
        self.assertEqual(unknown["inverse_achievement_id"], 100)
        document = copy.deepcopy(self.document)
        row = next(row for row in document["rows"] if row["achievement_id"] == 1107)
        row["ordinal"] = 0
        row["status"] = "proven"
        self.assertTrue(builder.validate_artifact(document))

    def test_missing_duplicate_or_substituted_sheet_ids_fail(self) -> None:
        for mutation in ("missing", "duplicate", "substituted"):
            document = copy.deepcopy(self.document)
            with self.subTest(mutation=mutation):
                if mutation == "missing":
                    document["rows"].pop()
                elif mutation == "duplicate":
                    document["rows"][1] = copy.deepcopy(document["rows"][0])
                else:
                    document["rows"][-1]["achievement_id"] = 999999
                    document["rows"][-1]["inverse_achievement_id"] = 999999
                self.assertTrue(builder.validate_artifact(document))

    def test_duplicate_out_of_range_or_unproven_ordinals_fail(self) -> None:
        for mutation in ("duplicate", "range", "inverse", "status", "locator"):
            document = copy.deepcopy(self.document)
            row = document["rows"][1]
            with self.subTest(mutation=mutation):
                if mutation == "duplicate":
                    row["ordinal"] = row["native_result"] = 0
                elif mutation == "range":
                    row["ordinal"] = row["native_result"] = 1024
                elif mutation == "inverse":
                    row["inverse_achievement_id"] = 100
                elif mutation == "status":
                    row["status"] = "unproven_inverse_mismatch"
                    row["ordinal"] = None
                else:
                    row["result_va"] = "0x0070BCB4"
                self.assertTrue(builder.validate_artifact(document))

    def test_changed_source_or_coverage_metadata_fails(self) -> None:
        for mutation in ("executable", "csv", "method", "summary"):
            document = copy.deepcopy(self.document)
            with self.subTest(mutation=mutation):
                if mutation == "executable":
                    document["inputs"]["ffxivgame.exe"]["sha256"] = "0" * 64
                elif mutation == "csv":
                    document["inputs"]["achievement.csv"]["sha256"] = "0" * 64
                elif mutation == "method":
                    document["method"]["sheet_ids_sha256"] = "0" * 64
                else:
                    document["summary"]["proven_ordinals"] = 746
                self.assertTrue(builder.validate_artifact(document))

    def test_title_subset_disagreement_fails(self) -> None:
        document = copy.deepcopy(self.document)
        row = next(row for row in document["rows"] if row["achievement_id"] == 106)
        row["result_va"] = "0x0070AB7D"
        self.assertEqual(builder.validate_artifact(document), [])
        self.assertIn(
            "title-linked completion subset differs",
            builder.validate_artifact(document, self.title_map),
        )

    def test_sheet_parser_includes_rows_without_titles(self) -> None:
        header = " ," + ",".join(str(index) for index in range(20)) + "\n"
        types = " ,s32,s32,s32,s32,s32,s32,,,,,,,,,,,s8,s8,s32,s32\n"
        no_title = ",".join(map(str, [100] + [0] * 20)) + "\n"
        title = ",".join(map(str, [197, 0, 1, 999, 200] + [0] * 16)) + "\n"
        data = (header + types + title + no_title).encode("ascii")
        self.assertEqual(builder.sheet_ids(data), [100, 197])
        with self.assertRaisesRegex(ValueError, "duplicate"):
            builder.sheet_ids(data + no_title.encode("ascii"))

    def test_check_refuses_byte_drift_and_never_writes(self) -> None:
        argv = [
            "builder.py",
            "--exe",
            "input.exe",
            "--achievement-csv",
            "input.csv",
            "--check",
            "map.json",
        ]
        rendered = builder.native.serialize(self.document).encode("ascii")
        for supplied, success in ((rendered, True), (rendered + b" ", False)):
            with (
                self.subTest(success=success),
                patch.object(sys, "argv", argv),
                patch.object(
                    builder.native,
                    "pinned_bytes",
                    return_value=json.dumps(self.title_map).encode("ascii"),
                ),
                patch.object(builder.native, "Image"),
                patch.object(builder, "sheet_ids", return_value=[]),
                patch.object(builder, "derive", return_value=self.document),
                patch.object(Path, "read_bytes", return_value=supplied),
                patch.object(Path, "write_bytes") as write,
                patch("sys.stdout", new_callable=io.StringIO),
                patch("sys.stderr", new_callable=io.StringIO),
            ):
                if success:
                    self.assertEqual(builder.main(), 0)
                else:
                    with self.assertRaises(SystemExit) as error:
                        builder.main()
                    self.assertEqual(error.exception.code, 1)
                write.assert_not_called()


if __name__ == "__main__":
    unittest.main()
