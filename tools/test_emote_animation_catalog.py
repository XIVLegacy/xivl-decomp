"""Asset-free contracts for distinct emote, category and selector namespaces."""

from __future__ import annotations

import copy
import io
import json
import unittest
from contextlib import redirect_stdout

try:
    from . import verify_emote_animation_catalog as verifier
except ImportError:
    import verify_emote_animation_catalog as verifier


class EmoteAnimationCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.document = json.loads(verifier.ARTIFACT.read_text(encoding="ascii"))

    def test_curated_domains_and_bounded_capture_joins(self) -> None:
        self.assertEqual(verifier.validate_artifact(self.document), [])
        self.assertEqual(len(self.document["selectors"]), 243)
        rows = {r["id"]: r for r in self.document["emotes"]}
        self.assertNotIn(150, rows)
        self.assertEqual(
            [rows[i]["text_command_id"] for i in (146, 147, 148)], [314, 314, 314]
        )
        self.assertEqual(rows[111]["observed_pair"], "dance")
        self.assertEqual(rows[119]["observed_pair"], "kneel")
        self.assertEqual(sum(r["observed_pair"] is None for r in rows.values()), 53)
        self.assertTrue(
            all(r["native_selector_join"]["selector"] is None for r in rows.values())
        )
        selectors = {r["value"]: r for r in self.document["selectors"]}
        self.assertEqual(selectors[0x0500B000]["enum_identifier"], "EmtDance")
        self.assertEqual(selectors[0x05013000]["enum_identifier"], "EmtKneel")
        self.assertEqual(selectors[0x05FA3065]["effect_field"], 101)
        self.assertEqual(selectors[0x05FA2066]["effect_field"], 102)
        self.assertEqual(len(self.document["dynamic_standard_calls"]), 18)
        self.assertEqual(len(self.document["excluded_debug_standard_calls"]), 3)

    def test_namespace_coverage_and_proof_mutations_are_rejected(self) -> None:
        for mutation in (
            "missing_emote",
            "duplicate_emote",
            "absent_150",
            "system_command_as_emote",
            "text_command_as_selector",
            "false_universal_join",
            "extra_observed_pair",
            "fabricated_category34",
            "duplicate_selector",
            "duplicate_name",
            "source_visual_name",
            "false_resource_visual_proof",
            "wrong_api",
            "wrong_source_pin",
            "proof_tier",
            "source_value",
            "missing_dynamic_call",
            "coverage_complete",
            "text_runtime_claim",
            "source_hash",
            "capture_pin",
            "dynamic_status",
            "category_hex",
        ):
            document = copy.deepcopy(self.document)
            with self.subTest(mutation=mutation):
                unknown = next(
                    r for r in document["selectors"] if r["name_tier"] == "UNKNOWN"
                )
                resource = next(
                    r
                    for r in document["selectors"]
                    if r["name_tier"] == "RESOURCE_LABEL"
                )
                source_evidence = next(
                    e
                    for e in document["evidence"].values()
                    if e["origin"] in ("script_literal", "branch_may_value")
                )
                if mutation == "missing_emote":
                    document["emotes"].pop()
                elif mutation == "duplicate_emote":
                    document["emotes"][1] = copy.deepcopy(document["emotes"][0])
                elif mutation == "absent_150":
                    document["emotes"][-1]["id"] = 150
                elif mutation == "system_command_as_emote":
                    document["emotes"][0]["id"] = 24102
                elif mutation == "text_command_as_selector":
                    document["selectors"][0]["value"] = 243
                elif mutation == "false_universal_join":
                    for row in document["emotes"]:
                        row["native_selector_join"] = {
                            "status": "STATIC",
                            "selector": 0x05000000 | ((row["id"] - 100) << 12),
                        }
                elif mutation == "extra_observed_pair":
                    document["emotes"][0]["observed_pair"] = "dance"
                elif mutation == "fabricated_category34":
                    row = copy.deepcopy(document["categories"][-1])
                    row["category"] = 34
                    document["categories"].append(row)
                elif mutation == "duplicate_selector":
                    document["selectors"][1]["value"] = document["selectors"][0][
                        "value"
                    ]
                elif mutation == "duplicate_name":
                    document["selectors"][1]["enum_identifier"] = document["selectors"][
                        0
                    ]["enum_identifier"]
                elif mutation == "source_visual_name":
                    unknown["enum_identifier"] = "SnowmanGrowth"
                elif mutation == "false_resource_visual_proof":
                    resource["name_tier"] = "STATIC_VISUAL"
                elif mutation == "wrong_api":
                    source_evidence["api"] = "_runCharaSchedulerTutorial"
                elif mutation == "wrong_source_pin":
                    source_evidence["script"] = "command/system/emotesitcommand.lua"
                elif mutation == "proof_tier":
                    unknown["proof_tiers"] = ["OBSERVED"]
                elif mutation == "source_value":
                    unknown["value"] += 1
                elif mutation == "missing_dynamic_call":
                    document["dynamic_standard_calls"].pop()
                elif mutation == "coverage_complete":
                    document["coverage"]["selector_domain"] = "COMPLETE"
                elif mutation == "source_hash":
                    document["inputs"]["scripts"][source_evidence["script"]][
                        "sha256"
                    ] = "0" * 64
                elif mutation == "capture_pin":
                    document["capture_pairs"][0]["capture_sha256"] = "0" * 64
                elif mutation == "dynamic_status":
                    document["dynamic_standard_calls"][0]["qualification"] = (
                        "unproven but all branches feasible"
                    )
                elif mutation == "category_hex":
                    document["categories"][0]["category_hex"] = "0x22"
                else:
                    document["emotes"][0]["text_command_status"] = "RUNTIME_PROVEN"
                self.assertTrue(verifier.validate_artifact(document))

    def test_source_callers_cannot_be_rebound_to_another_selector(self) -> None:
        for fields in (
            ("evidence",),
            ("source_callers",),
            ("evidence", "source_callers"),
        ):
            document = copy.deepcopy(self.document)
            rows = {row["value"]: row for row in document["selectors"]}
            destination, unrelated = rows[0x0400002E], rows[0x04000037]
            self.assertEqual(destination["evidence"], ["E002"])
            self.assertEqual(unrelated["evidence"], ["E004"])
            with self.subTest(fields=fields):
                for field in fields:
                    destination[field] = copy.deepcopy(unrelated[field])
                self.assertIn(
                    "source selector/caller binding drift",
                    verifier.validate_artifact(document),
                )

    def test_native_layout_and_exception_do_not_truncate(self) -> None:
        self.assertEqual(verifier.decode_selector(0x0500B000), (5, 11, 0))
        self.assertEqual(verifier.decode_selector(0x05FA3065), (5, 4003, 101))
        self.assertEqual(verifier.decode_selector(0x0BABCDEF), (11, 0, 0xABCDEF))
        self.assertEqual(verifier.encode_selector(11, 0, 0xABCDEF), 0x0BABCDEF)
        self.assertEqual(verifier.encode_selector(255, 4095, 4095), 0xFFFFFFFF)
        for args in (
            (11, 1, 0),
            (11, 0, 0x1000000),
            (4, 4096, 0),
            (4, 0, 4096),
            (256, 0, 0),
            (-1, 0, 0),
            (True, 0, 0),
        ):
            with self.subTest(args=args), self.assertRaises(ValueError):
                verifier.encode_selector(*args)
        for value in (-1, 0x100000000, True):
            with self.subTest(value=value), self.assertRaises(ValueError):
                verifier.decode_selector(value)
        document = copy.deepcopy(self.document)
        document["selector_layout"]["exception_effect_width_bits"] = 12
        self.assertTrue(verifier.validate_artifact(document))

    def test_csharp_export_is_clean_and_has_separate_types(self) -> None:
        output = io.StringIO()
        with redirect_stdout(output):
            self.assertEqual(verifier.main(["--emit-csharp"]), 0)
        text = output.getvalue()
        self.assertIn("public enum EmoteId : ushort", text)
        self.assertIn("public enum AnimationCategory : byte", text)
        self.assertIn("public enum AnimationSelector : uint", text)
        self.assertIn("Dance = 0x006F,", text)
        self.assertIn("EmtDance = 0x0500B000u,", text)
        self.assertIn("Wss21 = 0x21,", text)
        self.assertNotIn("0x22,", text)
        self.assertNotIn("validated", text)
        self.assertNotIn("SystemCommand", text)

    def test_malformed_or_private_catalogs_return_errors(self) -> None:
        for value in (None, [], {}, {"schema_version": 1}):
            with self.subTest(value=value):
                self.assertTrue(verifier.validate_artifact(value))
        document = copy.deepcopy(self.document)
        document["scope"] = "C:/private/client"
        self.assertTrue(verifier.validate_artifact(document))


if __name__ == "__main__":
    unittest.main()
