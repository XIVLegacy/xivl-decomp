"""Contract tests for verify_run_event_function.py."""

from __future__ import annotations

import unittest
from pathlib import Path

import extract_class_registrations as ecr
import verify_run_event_function as vrf
from verify_run_event_function import Result

GROUP_F = {0xF: 0xA0F00000}


class BindingPredicateTests(unittest.TestCase):
    def test_group_f_key_selects_the_trigger_once_registered(self) -> None:
        self.assertTrue(vrf.selects_trigger(0xA0F055F1, GROUP_F))

    def test_unregistered_group_f_selects_the_owner(self) -> None:
        self.assertFalse(vrf.selects_trigger(0xA0F055F1, {}))

    def test_bits_24_to_27_are_not_tested(self) -> None:
        self.assertTrue(vrf.selects_trigger(0xA1F00001, GROUP_F))
        self.assertTrue(vrf.selects_trigger(0xAEF00001, GROUP_F))

    def test_top_nibble_must_match_the_stored_value(self) -> None:
        self.assertFalse(vrf.selects_trigger(0xB0F00001, GROUP_F))

    def test_registered_group_other_than_f_selects_the_owner(self) -> None:
        registered = {0x9: 0xA0900000, 0xF: 0xA0F00000}
        self.assertFalse(vrf.selects_trigger(0xA0900123, registered))

    def test_witness_trigger_key_selects_the_owner(self) -> None:
        self.assertFalse(vrf.selects_trigger(0x029B2941, GROUP_F))


class RefusalReplyTests(unittest.TestCase):
    def test_window_is_end_tag_over_zeros(self) -> None:
        self.assertEqual(vrf.refusal_window(), b"\x0f" + bytes(63))

    def test_application_layout_and_checksum(self) -> None:
        got = vrf.refusal_application(0x029B2941, 0x11223344, 0x05)
        self.assertEqual(len(got), 0x51)
        self.assertEqual(got[0x00:0x04], bytes.fromhex("41299b02"))
        self.assertEqual(got[0x04:0x08], bytes.fromhex("44332211"))
        self.assertEqual(got[0x08:0x0C], bytes.fromhex("01000000"))
        self.assertEqual(got[0x0C:0x10], bytes.fromhex("71d66bcc"))
        self.assertEqual(got[0x10], 0x05)
        self.assertEqual(got[0x11:0x12], b"\x0f")
        self.assertEqual(got[0x12:], bytes(63))


class ResultSerializerTests(unittest.TestCase):
    def test_integer_is_big_endian_with_an_end_tag(self) -> None:
        self.assertEqual(
            vrf.encode_results([Result("int", 258)]), bytes.fromhex("00000001020f")
        )

    def test_float_is_a_single_precision_value(self) -> None:
        self.assertEqual(
            vrf.encode_results([Result("float", 1.5)]), bytes.fromhex("013fc000000f")
        )

    def test_booleans_nil_and_string(self) -> None:
        self.assertEqual(vrf.encode_results([Result("bool", True)]), b"\x03\x0f")
        self.assertEqual(vrf.encode_results([Result("bool", False)]), b"\x04\x0f")
        self.assertEqual(vrf.encode_results([Result("nil")]), b"\x05\x0f")
        self.assertEqual(
            vrf.encode_results([Result("string", "ab")]), b"\x02ab\x00\x0f"
        )

    def test_table_and_tentative_results_write_nothing(self) -> None:
        got = vrf.encode_results(
            [Result("int", 1), Result("table"), Result("tentative"), Result("int", 2)]
        )
        self.assertEqual(got, bytes.fromhex("00000000010000000002") + b"\x0f")

    def test_empty_list_leaves_only_the_end_tag(self) -> None:
        self.assertEqual(vrf.encode_results([]), b"\x0f")

    def test_long_string_is_cut_to_the_window_and_overruns_by_three(self) -> None:
        stream = vrf.encode_results([Result("string", "a" * 70)])
        self.assertEqual(len(stream), 67)
        self.assertEqual(stream[:2], b"\x02a")
        self.assertEqual(stream[-3:], b"a\x00\x0f")
        sent = vrf.wire_window(stream)
        self.assertEqual(sent, b"\x02" + b"a" * 63)

    def test_integers_past_the_window_are_written_and_cut_on_the_wire(self) -> None:
        stream = vrf.encode_results([Result("int", 1)] * 13)
        self.assertEqual(len(stream), 66)
        sent = vrf.wire_window(stream)
        self.assertEqual(sent[60:], bytes.fromhex("00000000"))
        self.assertNotIn(0x0F, sent[1:])

    def test_short_stream_is_padded_with_zeros(self) -> None:
        sent = vrf.wire_window(vrf.encode_results([Result("bool", True)]))
        self.assertEqual(sent, b"\x03\x0f" + bytes(62))


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

    def test_pinned_image_satisfies_every_check(self) -> None:
        self.assertEqual(vrf.verify(ecr.Image(self.data)), [])

    def test_trigger_group_byte_is_pinned(self) -> None:
        errors = vrf.check_data(self.patched(0x0130D426, 0, 0x0E))
        self.assertEqual(len(errors), 1)
        self.assertIn("trigger_group_byte", errors[0])

    def test_refusal_kind_byte_is_pinned(self) -> None:
        errors = vrf.check_data(self.patched(0x012C3F72, 0, 0x00))
        self.assertEqual(len(errors), 1)
        self.assertIn("refusal_kind_byte", errors[0])

    def test_membership_mask_is_pinned(self) -> None:
        errors = vrf.check_listings(self.patched(0x00D035AD, 5, 0xF1))
        self.assertEqual(len(errors), 1)
        self.assertIn("membership_mask_compare", errors[0])

    def test_pcall_status_values_are_pinned(self) -> None:
        errors = vrf.check_listings(self.patched(0x00CF40A5, 2, 0x03))
        self.assertTrue(any("pcall_status_switch" in e for e in errors))

    def test_refusal_send_arguments_are_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00897223) + 1] = 0x7A
        errors = vrf.check_listings(ecr.Image(bytes(patched)))
        self.assertTrue(any("refusal_b_send" in e for e in errors))

    def test_result_type_codes_are_pinned(self) -> None:
        errors = vrf.check_listings(self.patched(0x00583FD9, 3, 0x05))
        self.assertTrue(any("variant_nil" in e for e in errors))

    def test_client_handler_slot_four_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00FE0184 + 16)] ^= 0x01
        errors = vrf.check_vtables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("slot 4", errors[0])

    def test_player_manager_slot_one_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x0105706C + 4)] ^= 0x01
        errors = vrf.check_vtables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("PlayerManager slot 1", errors[0])

    def test_error_path_finish_call_is_pinned(self) -> None:
        errors = vrf.check_listings(self.patched(0x00CCF1BB, 1, 0x00))
        self.assertTrue(any("error_path_finish_call" in e for e in errors))

    def test_error_branch_polarity_is_pinned(self) -> None:
        errors = vrf.check_listings(self.patched(0x00CCF0A1, 0, 0x74))
        self.assertTrue(any("error_result_branch" in e for e in errors))

    def test_listener_target_is_pinned(self) -> None:
        errors = vrf.check_listings(self.patched(0x00896161, 1, 0x00))
        self.assertTrue(any("listener_second_target" in e for e in errors))

    def test_caller_sets_are_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00894B65) + 1] ^= 0x01
        errors = vrf.check_callers(ecr.Image(bytes(patched)))
        self.assertTrue(errors)


if __name__ == "__main__":
    unittest.main()
