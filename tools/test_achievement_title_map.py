"""Asset-free contracts for the complete achievement-title mapping artifact."""

from __future__ import annotations

import copy
import io
import json
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

try:
    from . import build_achievement_title_map as builder
except ImportError:
    import build_achievement_title_map as builder

ARTIFACT = Path(__file__).resolve().parents[1] / "config/achievement_title_map.json"


class AchievementTitleMapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = json.loads(ARTIFACT.read_text(encoding="ascii"))

    def test_complete_published_map(self) -> None:
        self.assertEqual(builder.validate_artifact(self.document), [])
        rows = {row["achievement_id"]: row for row in self.document["rows"]}
        self.assertEqual(len(rows), 208)
        self.assertEqual(
            sum(row["eligible_for_available_title"] for row in rows.values()), 105
        )
        for achievement, title, ordinal in (
            (197, 200, 18),
            (198, 201, 19),
            (390, 400, 146),
            (391, 401, 147),
            (392, 402, 148),
        ):
            self.assertEqual(
                (
                    rows[achievement]["title_id"],
                    rows[achievement]["direct_completion"]["ordinal"],
                ),
                (title, ordinal),
            )
        self.assertEqual(rows[106]["direct_completion"]["ordinal"], 6)
        self.assertEqual(
            rows[106]["has_achievement_title_parent"]["completion"]["ordinal"], 18
        )
        self.assertEqual(rows[279]["direct_completion"]["ordinal"], 129)
        self.assertEqual(
            rows[279]["has_achievement_title_parent"]["completion"]["ordinal"], 148
        )

    def test_incomplete_duplicate_and_unpinned_artifacts_fail(self) -> None:
        for mutation in ("missing", "duplicate", "executable", "csv", "summary"):
            document = copy.deepcopy(self.document)
            with self.subTest(mutation=mutation):
                if mutation == "missing":
                    document["rows"].pop()
                elif mutation == "duplicate":
                    document["rows"][1] = copy.deepcopy(document["rows"][0])
                elif mutation == "executable":
                    document["inputs"]["ffxivgame.exe"]["sha256"] = "0" * 64
                elif mutation == "csv":
                    document["inputs"]["achievement.csv"]["sha256"] = "0" * 64
                else:
                    document["summary"]["title_linked_rows"] = 207
                self.assertTrue(builder.validate_artifact(document))

    def test_direct_parent_conflation_and_loader_rejection_fail(self) -> None:
        for mutation in ("direct", "parent", "loader", "admission", "sentinel"):
            document = copy.deepcopy(self.document)
            rows = {row["achievement_id"]: row for row in document["rows"]}
            with self.subTest(mutation=mutation):
                if mutation == "direct":
                    rows[106]["direct_completion"] = rows[106][
                        "has_achievement_title_parent"
                    ]["completion"]
                elif mutation == "parent":
                    rows[106]["has_achievement_title_parent"]["completion"] = rows[106][
                        "direct_completion"
                    ]
                elif mutation == "loader":
                    rows[106]["title_map_store"] = "rejected"
                elif mutation == "admission":
                    rows[106]["eligible_for_available_title"] = True
                else:
                    rows[197]["has_achievement_title_parent"]["completion"] = rows[197][
                        "direct_completion"
                    ]
                self.assertTrue(builder.validate_artifact(document))

    def test_wrong_known_ordinal_and_inverse_fail(self) -> None:
        for mutation in ("ordinal", "inverse", "unknown", "out_of_range"):
            document = copy.deepcopy(self.document)
            record = next(
                row for row in document["rows"] if row["achievement_id"] == 197
            )["direct_completion"]
            with self.subTest(mutation=mutation):
                if mutation == "ordinal":
                    record["ordinal"] = record["native_result"] = 20
                elif mutation == "inverse":
                    record["inverse_achievement_id"] = 198
                elif mutation == "unknown":
                    record["status"] = "unproven_inverse_mismatch"
                else:
                    record["ordinal"] = record["native_result"] = 1024
                self.assertTrue(builder.validate_artifact(document))

    def test_zero_native_result_needs_an_inverse_join(self) -> None:
        class ZeroMapping:
            def evaluate(self, name: str, argument: int) -> tuple[int, str]:
                if name == "achievement_to_ordinal":
                    return 0, "0x0070BCB3"
                if name == "ordinal_to_achievement" and argument == 0:
                    return 100, "0x0057ABCB"
                raise AssertionError((name, argument))

        mapping = ZeroMapping()
        self.assertEqual(builder.completion(mapping, 100)["ordinal"], 0)
        self.assertEqual(builder.completion(mapping, 100)["status"], "proven")
        unknown = builder.completion(mapping, 0)
        self.assertEqual(unknown["status"], "unproven_inverse_mismatch")
        self.assertIsNone(unknown["ordinal"])
        self.assertEqual(unknown["native_result"], 0)
        self.assertEqual(unknown["inverse_achievement_id"], 100)

    def test_column_labels_and_row_ids_own_the_join(self) -> None:
        header = " ," + ",".join(str(index) for index in range(20)) + "\n"
        types = " ,s32,s32,s32,s32,s32,s32,,,,,,,,,,,s8,s8,s32,s32\n"

        def row(achievement: int, title: int) -> str:
            return ",".join(map(str, [achievement, 0, 1, 999, title] + [0] * 16)) + "\n"

        data = (header + types + row(198, 201) + row(197, 200) + row(100, 0)).encode(
            "ascii"
        )
        self.assertEqual(builder.title_links(data), {198: 201, 197: 200})
        with self.assertRaisesRegex(ValueError, "duplicate"):
            builder.title_links(data + row(198, 201).encode("ascii"))
        with self.assertRaisesRegex(ValueError, "layout"):
            builder.title_links(data.replace(b" ,0,1,2,3,4", b" ,0,1,2,4,3", 1))
        with self.assertRaisesRegex(ValueError, "width"):
            builder.title_links((header + types + "197,200\n").encode("ascii"))

    def test_wrong_input_identity_is_rejected_before_pe_parsing(self) -> None:
        with patch.object(Path, "read_bytes", return_value=b"not a retail input"):
            for name, digest, size in (
                ("ffxivgame.exe", builder.EXE_SHA256, builder.EXE_BYTES),
                ("achievement.csv", builder.CSV_SHA256, builder.CSV_BYTES),
            ):
                with (
                    self.subTest(name=name),
                    self.assertRaisesRegex(ValueError, "SHA-256 mismatch"),
                ):
                    builder.pinned_bytes(Path(name), digest, size)

    def test_check_rejects_byte_drift_without_writing(self) -> None:
        argv = [
            "build_achievement_title_map.py",
            "--exe",
            "input.exe",
            "--achievement-csv",
            "input.csv",
            "--check",
            "map.json",
        ]
        rendered = builder.serialize(self.document).encode("ascii")
        for supplied, expected in ((rendered, 0), (rendered + b" ", 1)):
            with (
                self.subTest(expected=expected),
                patch.object(sys, "argv", argv),
                patch.object(builder, "pinned_bytes", return_value=b""),
                patch.object(builder, "Image"),
                patch.object(builder, "title_links", return_value={}),
                patch.object(builder, "derive", return_value=self.document),
                patch.object(Path, "read_bytes", return_value=supplied),
                patch.object(Path, "write_bytes") as write,
                patch("sys.stdout", new_callable=io.StringIO),
                patch("sys.stderr", new_callable=io.StringIO),
            ):
                if expected:
                    with self.assertRaises(SystemExit) as error:
                        builder.main()
                    self.assertEqual(error.exception.code, 1)
                else:
                    self.assertEqual(builder.main(), 0)
                write.assert_not_called()


if __name__ == "__main__":
    unittest.main()
