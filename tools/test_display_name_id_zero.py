"""Contract tests for verify_display_name_id_zero.py."""

from __future__ import annotations

import json
import struct
import tempfile
import unittest
from pathlib import Path

import extract_class_registrations as ecr
import verify_display_name_id_zero as vdn

# Reference-model, macro and census tests below use hand-written inputs that
# restate the documented contract. Only RetailImageTests check the executable,
# and they skip when orig/ffxivgame.exe is absent.

NAMES = {1000036: b"Anselm", 3104216: b"tree slug", 0: b"???"}

# Column 1 of row 222 of xtx/_text_ui as exported by xivl extract.
ENGLISH_CELL_HEX = (
    "022d25ff2302081fe908ff19022815ff107874782f646973706c61794e616d65e9080203eb030303"
)


def payload(ident: int, text: bytes = b"", flag: int = 0) -> bytes:
    return struct.pack("<I", ident) + text.ljust(32, b"\0") + bytes([flag]) + b"\0" * 3


def cell(hex_text: str, name: str = "head-all") -> str:
    return f"[@{name}:{hex_text}]"


class PayloadTests(unittest.TestCase):
    def test_fields_sit_at_0_4_and_0x24(self) -> None:
        got = vdn.parse_payload(payload(0x2F5DD8, b"Baderon", 1))
        self.assertEqual(got, vdn.WireName(0x2F5DD8, b"Baderon", 1))

    def test_string_stops_at_the_first_nul(self) -> None:
        raw = bytearray(payload(0, b"abc"))
        raw[8:10] = b"zz"
        self.assertEqual(vdn.parse_payload(bytes(raw)).text, b"abc")

    def test_a_full_32_byte_string_is_kept_whole(self) -> None:
        text = b"x" * 32
        self.assertEqual(vdn.parse_payload(payload(0, text)).text, text)

    def test_short_payload_is_refused(self) -> None:
        with self.assertRaises(ValueError):
            vdn.parse_payload(b"\0" * 0x24)


class ApplyTests(unittest.TestCase):
    def kind(self, ident: int, text: bytes = b"", flag: int = 0) -> str:
        return vdn.apply_wire(vdn.WireName(ident, text, flag)).kind

    def test_id_zero_with_a_string_is_a_custom_name(self) -> None:
        got = vdn.apply_wire(vdn.WireName(0, b"Baderon", 0))
        self.assertEqual(got, vdn.Outcome("custom", 0xFFFFFFFF, b"Baderon", 0))

    def test_id_ffffffff_with_a_string_is_a_custom_name(self) -> None:
        self.assertEqual(self.kind(0xFFFFFFFF, b"Baderon"), "custom")

    def test_id_zero_and_ffffffff_are_the_same_outcome(self) -> None:
        for text, flag in ((b"", 0), (b"A", 0), (b"", 1)):
            self.assertEqual(
                vdn.apply_wire(vdn.WireName(0, text, flag)),
                vdn.apply_wire(vdn.WireName(0xFFFFFFFF, text, flag)),
            )

    def test_empty_string_and_zero_flag_makes_no_receiver(self) -> None:
        self.assertEqual(self.kind(0), "ignored")
        self.assertEqual(self.kind(0xFFFFFFFF), "ignored")

    def test_a_flag_byte_makes_the_empty_name_apply(self) -> None:
        self.assertEqual(self.kind(0, b"", 1), "custom")

    def test_a_nonzero_id_is_a_lookup_even_with_an_empty_string(self) -> None:
        self.assertEqual(self.kind(1000036), "lookup")
        self.assertEqual(self.kind(1000036, b"ignored", 1), "lookup")


class NameStateTests(unittest.TestCase):
    def test_default_is_an_empty_custom_name(self) -> None:
        self.assertEqual(vdn.DEFAULT_NAME, vdn.NameState(0xFFFFFFFF, b"", 0, True))

    def test_resolved_byte_is_one_only_for_the_custom_id(self) -> None:
        custom = vdn.store_name(vdn.apply_wire(vdn.WireName(0, b"A", 0)))
        lookup = vdn.store_name(vdn.apply_wire(vdn.WireName(7, b"A", 0)))
        self.assertTrue(custom.resolved)
        self.assertFalse(lookup.resolved)

    def test_context_for_a_custom_name_carries_the_string_and_a_zero_id(self) -> None:
        got = vdn.name_context(vdn.NameState(0xFFFFFFFF, b"Baderon", 3, True))
        self.assertEqual(got, vdn.Context(b"Baderon", 3, 0))

    def test_context_for_an_id_carries_the_id_and_an_empty_string(self) -> None:
        got = vdn.name_context(vdn.NameState(1000036, b"ignored", 3, False))
        self.assertEqual(got, vdn.Context(b"", 0, 1000036))


class Row222Tests(unittest.TestCase):
    def test_nonzero_id_selects_the_sheet_row(self) -> None:
        self.assertEqual(vdn.row_222(vdn.Context(b"", 0, 1000036), NAMES), b"Anselm")

    def test_zero_id_selects_the_string_not_sheet_row_zero(self) -> None:
        self.assertEqual(vdn.row_222(vdn.Context(b"Baderon", 0, 0), NAMES), b"Baderon")
        self.assertEqual(vdn.row_222(vdn.Context(b"", 0, 0), NAMES), b"")

    def test_missing_row_is_none(self) -> None:
        self.assertIsNone(vdn.row_222(vdn.Context(b"", 0, 99), NAMES))


class SpeakerTests(unittest.TestCase):
    def speaker(self, ident: int, text: bytes = b"", flag: int = 0, **kw):
        return vdn.speaker_name(vdn.WireName(ident, text, flag), NAMES, **kw)

    def test_id_zero_shows_the_wire_string(self) -> None:
        self.assertEqual(self.speaker(0, b"Baderon"), b"Baderon")

    def test_id_zero_without_a_string_leaves_the_default_empty_name(self) -> None:
        self.assertEqual(self.speaker(0), b"")

    def test_id_zero_without_a_string_keeps_a_name_set_earlier(self) -> None:
        old = vdn.NameState(0xFFFFFFFF, b"Old", 0, True)
        self.assertEqual(self.speaker(0, current=old), b"Old")

    def test_nonzero_id_shows_the_sheet_name_and_drops_the_string(self) -> None:
        self.assertEqual(self.speaker(1000036, b"Wire Name"), b"Anselm")

    def test_unknown_nonzero_id_has_no_name(self) -> None:
        self.assertIsNone(self.speaker(555, b"Wire Name"))


class MacroTests(unittest.TestCase):
    def test_english_cell_tests_element_6_and_falls_back_to_element_1(self) -> None:
        self.assertEqual(vdn.check_cell(cell(ENGLISH_CELL_HEX)), [])

    def test_literal_decodes_as_lead_minus_one(self) -> None:
        got, end = vdn.read_expr(bytes([0xE9, 0x08]), 0)
        self.assertEqual((got, end), (("E9", ("lit", 7)), 2))

    def test_condition_on_another_element_is_reported(self) -> None:
        bad = ENGLISH_CELL_HEX.replace("e908ff19", "e907ff19")
        self.assertTrue(vdn.check_cell(cell(bad)))

    def test_else_branch_on_another_element_is_reported(self) -> None:
        bad = ENGLISH_CELL_HEX.replace("eb03", "eb04")
        self.assertTrue(vdn.check_cell(cell(bad)))

    def test_non_macro_cell_is_refused(self) -> None:
        with self.assertRaises(ValueError):
            vdn.check_cell("Bardolf")


class CensusTests(unittest.TestCase):
    def sample(self, ident: int, word8: int = 1356917211, tail: bytes = b"") -> dict:
        raw = struct.pack("<HHIII", 0x14, 0x13D, 0, word8, 0) + struct.pack("<I", ident)
        raw += tail.ljust(36, b"\0")
        return {"bytes": raw.hex()}

    def wrap(self, *entries: dict) -> dict:
        return {"samples": {"s2c": {"0x013d": {"samples": list(entries)}}}}

    def test_counts_split_zero_ffffffff_and_other_ids(self) -> None:
        got = vdn.census(
            self.wrap(
                self.sample(0),
                self.sample(0),
                self.sample(0xFFFFFFFF),
                self.sample(1000036),
                self.sample(1000036),
            ),
            {1000036: b"Anselm"},
        )
        self.assertEqual(
            (got["id_zero"], got["id_ffffffff"], got["id_other"]), (2, 1, 2)
        )
        self.assertEqual((got["distinct_ids"], got["distinct_other"]), (3, 1))
        self.assertEqual(got["other_ids_without_row"], [])
        self.assertEqual(got["payload_bytes"], 40)

    def test_a_string_byte_breaks_the_all_zero_tail(self) -> None:
        got = vdn.census(self.wrap(self.sample(0, tail=b"A")))
        self.assertFalse(got["tail_after_id_all_zero"])

    def test_missing_sheet_row_is_listed(self) -> None:
        got = vdn.census(self.wrap(self.sample(77)), {1: b"x"})
        self.assertEqual(got["other_ids_without_row"], [77])

    def test_check_census_reports_a_changed_count(self) -> None:
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "samples.json"
            path.write_text(json.dumps(self.wrap(self.sample(0))))
            errors = vdn.check_census(path, None)
        self.assertTrue(any("samples" in e for e in errors))


class CommandTests(unittest.TestCase):
    def test_missing_executable_skips_with_exit_zero(self) -> None:
        self.assertEqual(vdn.main(["--exe", "does/not/exist.exe"]), 0)


@unittest.skipUnless(Path("orig/ffxivgame.exe").is_file(), "no local image")
class RetailImageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        try:
            import capstone  # noqa: F401
        except ImportError:
            raise unittest.SkipTest("Capstone is not installed")
        cls.data = Path("orig/ffxivgame.exe").read_bytes()
        ecr.verify_identity(cls.data)

    def patched(self, va: int, delta: int, value: int) -> ecr.Image:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(va) + delta] = value
        return ecr.Image(bytes(patched))

    def flagged(self, errors: list[str], label: str) -> bool:
        return any(label in e for e in errors)

    def test_pinned_image_satisfies_every_check(self) -> None:
        self.assertEqual(vdn.verify(ecr.Image(self.data)), [])

    def test_lua_id_zero_map_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x0076C524, 0, 0x75))
        self.assertTrue(self.flagged(errors, "lua_wire_id_map"))

    def test_lua_drop_rule_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x0076C5AB, 0, 0x74))
        self.assertTrue(self.flagged(errors, "lua_wire_drop"))

    def test_null_impl_id_zero_map_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x007719D7, 0, 0x75))
        self.assertTrue(self.flagged(errors, "null_wire_id_map"))

    def test_constructor_zero_map_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x0089D572, 0, 0x74))
        self.assertTrue(self.flagged(errors, "receiver_ctor_zero_map"))

    def test_flag_byte_offset_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x0076C5E0, 3, 0x25))
        self.assertTrue(self.flagged(errors, "lua_construct_apply"))

    def test_custom_branch_selector_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x006FB09B, 0, 0x74))
        self.assertTrue(self.flagged(errors, "apply_branch"))

    def test_resolved_byte_for_the_custom_id_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x006FB0C1, 1, 0x95))
        self.assertTrue(self.flagged(errors, "set_name_custom"))

    def test_default_name_state_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x006BF45F, 3, 0x00))
        self.assertTrue(self.flagged(errors, "default_name_struct"))

    def test_context_branch_polarity_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x00769AEB, 0, 0x75))
        self.assertTrue(self.flagged(errors, "context_push_branch"))

    def test_context_element_indices_are_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x00769B1A, 1, 0x05))
        self.assertTrue(self.flagged(errors, "context_push_lookup"))

    def test_variant_tags_are_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x00778D56, 0, 0x01))
        self.assertTrue(self.flagged(errors, "variant_int_accessor"))
        errors = vdn.check_listings(self.patched(0x00779BC6, 0, 0x00))
        self.assertTrue(self.flagged(errors, "variant_string_accessor"))

    def test_operand_decrement_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x00CB2E0F, 2, 0xFE))
        self.assertTrue(self.flagged(errors, "num_expr_e9"))

    def test_row_222_request_is_pinned(self) -> None:
        errors = vdn.check_listings(self.patched(0x0076FDFE, 1, 0xDF))
        self.assertTrue(self.flagged(errors, "request_row"))

    def test_lua_actor_impl_slot_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00FDFB2C + 4 * 62)] ^= 0x01
        errors = vdn.check_tables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("LuaActorImpl", errors[0])
        self.assertIn("slot 62", errors[0])

    def test_dispatch_class_of_the_opcode_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(vdn.DISPATCH_CLASS_TABLE + 0x71)] = 10
        errors = vdn.check_tables(ecr.Image(bytes(patched)))
        self.assertTrue(any("dispatch class of 0x013D" in e for e in errors))

    def test_string_length_mode_word_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(vdn.STRING_CODEPAGE_WORD)] = 0x00
        errors = vdn.check_data(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)

    def test_caller_sets_are_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x0076C5F4) + 1] ^= 0x01
        errors = vdn.check_callers(ecr.Image(bytes(patched)))
        self.assertTrue(errors)


if __name__ == "__main__":
    unittest.main()
