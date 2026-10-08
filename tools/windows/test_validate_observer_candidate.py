# SPDX-License-Identifier: AGPL-3.0-or-later
"""Reject altered candidate receipts using a preserved CPU qualification run."""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
from uuid import uuid4

import validate_observer_candidate as validator

RECEIPT_PATH = None


class CandidateReceiptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if RECEIPT_PATH is None:
            raise unittest.SkipTest("supply --receipt from an offline candidate run")
        cls.receipt = json.loads(
            RECEIPT_PATH.read_text(encoding="ascii"),
            object_pairs_hook=validator.unique_object,
        )
        validator.validate_candidate(cls.receipt)

    def altered(self) -> dict:
        return copy.deepcopy(self.receipt)

    def test_preserved_receipt(self) -> None:
        self.assertGreater(validator.validate_candidate(self.receipt), 0)

    def test_success_summary_cannot_replace_trace(self) -> None:
        value = self.altered()
        value["trace"]["rows"] = []
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_total_row_cap_must_cover_all_rows(self) -> None:
        value = self.altered()
        value["row_cap"] = 1
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_every_causal_fault_result_is_required(self) -> None:
        for name in validator.FAULTS:
            with self.subTest(name=name):
                value = self.altered()
                value["fail_closed_cases"][name] = False
                with self.assertRaises(ValueError):
                    validator.validate_candidate(value)

    def test_output_artifacts_must_exist(self) -> None:
        for field in ("trace_artifact", "failure_artifact"):
            with self.subTest(field=field):
                value = self.altered()
                value[field] = str(RECEIPT_PATH.parent / (uuid4().hex + ".json"))
                with self.assertRaises(ValueError):
                    validator.validate_candidate(value)

    def test_variant_artifacts_must_exist(self) -> None:
        reader = validator.read_artifact
        failures = reader(self.receipt["failure_artifact"], "failure")
        for name in failures["variants"]:
            with self.subTest(name=name):
                altered = copy.deepcopy(failures)
                altered["variants"][name]["state_artifact"] = str(
                    RECEIPT_PATH.parent / (uuid4().hex + ".json")
                )

                def read_altered(value: object, label: str) -> object:
                    if value == self.receipt["failure_artifact"]:
                        return altered
                    return reader(value, label)

                with patch.object(validator, "read_artifact", side_effect=read_altered):
                    with self.assertRaises(ValueError):
                        validator.validate_candidate(self.receipt)

    def test_negative_artifacts_require_causal_trace_structure(self) -> None:
        reader = validator.read_artifact
        failures = reader(self.receipt["failure_artifact"], "failure")
        for name, variant in failures["variants"].items():
            original = reader(variant["trace_artifact"], name)
            for replacement in (
                {"rows": []},
                {**original, "rows": []},
                {**original, "rows": [0]},
            ):
                with self.subTest(name=name, replacement=replacement["rows"]):

                    def read_altered(value: object, label: str) -> object:
                        if value == variant["trace_artifact"]:
                            return replacement
                        return reader(value, label)

                    with patch.object(
                        validator, "read_artifact", side_effect=read_altered
                    ):
                        with self.assertRaises(ValueError):
                            validator.validate_candidate(self.receipt)

    def test_positive_trace_cannot_replace_a_causal_failure(self) -> None:
        reader = validator.read_artifact
        failures = reader(self.receipt["failure_artifact"], "failure")
        for name, variant in failures["variants"].items():
            with self.subTest(name=name):

                def read_altered(value: object, label: str) -> object:
                    if value == variant["trace_artifact"]:
                        return self.receipt["trace"]
                    return reader(value, label)

                with patch.object(validator, "read_artifact", side_effect=read_altered):
                    with self.assertRaises(ValueError):
                        validator.validate_candidate(self.receipt)

    def test_native_claim_refuses(self) -> None:
        for section, field, replacement in (
            ("identity", "native_identity", "qualified"),
            ("fixture_outcome", "native_survival", "confirmed"),
            (None, "mode", "native"),
        ):
            with self.subTest(field=field):
                value = self.altered()
                target = value if section is None else value[section]
                target[field] = replacement
                with self.assertRaises(ValueError):
                    validator.validate_candidate(value)

    def test_missing_or_nonfinite_limit_refuses(self) -> None:
        for key in validator.LIMITS:
            for replacement in (None, 0, True, -1, 2**32):
                with self.subTest(key=key, replacement=replacement):
                    value = self.altered()
                    value["limits"][key] = replacement
                    with self.assertRaises(ValueError):
                        validator.validate_candidate(value)

    def test_cleanup_outcomes_remain_separate(self) -> None:
        for section, field in (
            ("installation", "restored"),
            ("publication", "cleared"),
            ("publication", "owner_released"),
            ("recovery", "succeeded"),
        ):
            with self.subTest(section=section, field=field):
                value = self.altered()
                value[section][field] = False
                with self.assertRaises(ValueError):
                    validator.validate_candidate(value)

    def test_consumed_limit_cannot_exceed_supplied_budget(self) -> None:
        for key in validator.LIMITS:
            with self.subTest(key=key):
                value = self.altered()
                value["clock"]["consumed"][key] = value["limits"][key] + 1
                with self.assertRaises(ValueError):
                    validator.validate_candidate(value)

    def test_clock_accounting_must_agree(self) -> None:
        value = self.altered()
        value["clock"]["ticks"] += 1
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_summary_count_must_match_evidence(self) -> None:
        value = self.altered()
        value["identity"]["query_rows"] += 1
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_receipt_session_must_match_evidence(self) -> None:
        value = self.altered()
        value["session_id"] += 1
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_forwarding_gap_refuses(self) -> None:
        value = self.altered()
        value["trace"]["unlogged_calls"] = 1
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_released_publication_cannot_leave_forwarding_active(self) -> None:
        for field, replacement in (
            ("active_calls", 1),
            ("passthrough_published", True),
        ):
            with self.subTest(field=field):
                value = self.altered()
                value["trace"][field] = replacement
                with self.assertRaises(ValueError):
                    validator.validate_candidate(value)

    def test_query_target_must_match_query_time_provenance(self) -> None:
        value = self.altered()
        query = next(row for row in value["trace"]["rows"] if row["kind"] == "query")
        query["slot_plus_10_target"] = "0x1234"
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_production_binding_cannot_be_dropped(self) -> None:
        value = self.altered()
        value["trace"]["rows"] = [
            row for row in value["trace"]["rows"] if row["kind"] != "engine_binding"
        ]
        with self.assertRaises(ValueError):
            validator.validate_candidate(value)

    def test_duplicate_json_fields_refuse(self) -> None:
        with self.assertRaises(ValueError):
            json.loads(
                '{"success":false,"success":true}',
                object_pairs_hook=validator.unique_object,
            )


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--receipt":
        RECEIPT_PATH = Path(sys.argv[2])
        del sys.argv[1:3]
    unittest.main()
