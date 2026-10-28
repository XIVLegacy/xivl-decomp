"""Contract tests for verify_npcsay_log_wait.py."""

from __future__ import annotations

import os
import unittest
from pathlib import Path
from unittest import mock

import extract_class_registrations as ecr
import verify_npcsay_log_wait as vnl

CONTROLS = ["LogControl_NpcSay", "LogControl_Main_1"]

# Fan-out, release and name-request tests below exercise hand-written reference
# models that restate the documented contract. Only RetailImageTests check the
# executable, and they skip when orig/ffxivgame.exe is absent.


class ContainerFanoutTests(unittest.TestCase):
    def test_key_and_mouse_press_reach_every_registered_control(self) -> None:
        want = [(vnl.CONTROL_PRESS_KEY, c) for c in CONTROLS]
        self.assertEqual(vnl.container_fanout(vnl.PRESS_KEY, CONTROLS), want)
        self.assertEqual(vnl.container_fanout(vnl.PRESS_MOUSE, CONTROLS), want)

    def test_cancel_reaches_every_registered_control_as_cancel(self) -> None:
        want = [(vnl.CONTROL_PRESS_CANCEL, c) for c in CONTROLS]
        self.assertEqual(vnl.container_fanout(vnl.PRESS_CANCEL, CONTROLS), want)

    def test_wait0_sets_the_message_flag_and_touches_no_control(self) -> None:
        self.assertEqual(
            vnl.container_fanout(vnl.WAIT0, CONTROLS),
            [("DesktopWidget.messageWaitFlag", "set")],
        )

    def test_press_with_no_registered_control_does_nothing(self) -> None:
        self.assertEqual(vnl.container_fanout(vnl.PRESS_KEY, []), [])

    def test_unrelated_command_does_nothing(self) -> None:
        self.assertEqual(
            vnl.container_fanout("RaptureCommands.ScreenShot", CONTROLS), []
        )


class KeyReleaseTests(unittest.TestCase):
    def test_press_with_nothing_pending_is_not_consumed(self) -> None:
        got = vnl.release_wait(0, 0, 0, False)
        self.assertEqual(got, vnl.Release(False, 0, 0, False, False))

    def test_last_wait_token_raises_wait0(self) -> None:
        got = vnl.release_wait(1, 1, 1, False)
        self.assertEqual(got, vnl.Release(True, 0, 0, False, True))

    def test_earlier_wait_token_leaves_the_message_parked(self) -> None:
        got = vnl.release_wait(2, 2, 1, False)
        self.assertEqual(got, vnl.Release(True, 1, 1, False, False))

    def test_page_break_alone_never_raises(self) -> None:
        got = vnl.release_wait(1, 0, 0, True)
        self.assertEqual(got, vnl.Release(True, 0, 0, False, False))

    def test_wait_token_on_a_page_break_line_releases_both(self) -> None:
        got = vnl.release_wait(2, 1, 1, True)
        self.assertEqual(got, vnl.Release(True, 0, 0, False, True))

    def test_row_past_the_count_wipes_the_wait_without_a_release(self) -> None:
        got = vnl.release_wait(2, 2, 1, False, row_in_range=False)
        self.assertEqual(got, vnl.Release(False, 0, 0, False, False))

    def test_null_row_lookup_wipes_the_wait_without_a_release(self) -> None:
        got = vnl.release_wait(1, 1, 1, False, row_found=False)
        self.assertEqual(got, vnl.Release(False, 0, 0, False, False))

    def test_page_break_with_a_wait_still_pending_keeps_wait0_back(self) -> None:
        got = vnl.release_wait(3, 2, 1, True)
        self.assertEqual(got, vnl.Release(True, 1, 1, False, False))


class ArmWaitTests(unittest.TestCase):
    def test_plain_line_inside_one_page_arms_nothing(self) -> None:
        got = vnl.arm_wait([0], 3)
        self.assertEqual(got, vnl.Armed(0, 0, -1, (), 1))

    def test_plain_message_that_exactly_fills_a_page_arms_nothing(self) -> None:
        got = vnl.arm_wait([0, 0, 0], 3)
        self.assertEqual(got, vnl.Armed(0, 0, -1, (), 3))

    def test_plain_message_one_row_past_a_page_breaks_once(self) -> None:
        got = vnl.arm_wait([0, 0, 0, 0], 3)
        self.assertEqual(got, vnl.Armed(1, 0, -1, (2,), 2))

    def test_plain_message_flags_every_page_boundary_but_the_last_row(self) -> None:
        got = vnl.arm_wait([0] * 7, 3)
        self.assertEqual(got.page_breaks, (2, 5))
        self.assertEqual(got.pending, 2)

    def test_weighted_last_row_holds_without_a_page_break(self) -> None:
        got = vnl.arm_wait([0, 0, 1], 3)
        self.assertEqual(got, vnl.Armed(1, 1, 2, (), 2))

    def test_weighted_row_on_the_boundary_takes_no_page_break(self) -> None:
        got = vnl.arm_wait([0, 0, 2, 0], 3)
        self.assertEqual(got, vnl.Armed(2, 2, 2, (), 2))

    def test_boundary_starts_at_the_current_row(self) -> None:
        got = vnl.arm_wait([0] * 6, 2, first_row=2)
        self.assertEqual(got, vnl.Armed(1, 0, -1, (3,), 3))


class PageRowTests(unittest.TestCase):
    # The text height is an illustrative value, not a measured retail metric.
    def test_count_truncates_the_height_over_line_height(self) -> None:
        self.assertEqual(vnl.visible_lines(2, 102, 18), 5)

    def test_count_is_never_below_one(self) -> None:
        self.assertEqual(vnl.visible_lines(2, 5, 18), 1)


class ScrollPaceTests(unittest.TestCase):
    def test_short_backlog_keeps_lines_per_sec(self) -> None:
        self.assertEqual(vnl.scroll_speed(10.0, 10.0, 10, 9, False), 10.0)

    def test_backlog_at_lines_begin_hi_selects_five_times_the_hi_speed(self) -> None:
        self.assertEqual(vnl.scroll_speed(10.0, 10.0, 10, 10, False), 50.0)

    def test_active_marker_selects_three_times_the_hi_speed(self) -> None:
        self.assertEqual(vnl.scroll_speed(10.0, 10.0, 10, 99, True), 30.0)

    def test_step_is_per_call_and_scales_with_line_height(self) -> None:
        self.assertEqual(vnl.scroll_step(20.0, 10.0, False), 4.0)

    def test_display_last_one_halves_the_step(self) -> None:
        self.assertEqual(vnl.scroll_step(20.0, 10.0, True), 2.0)


class PropertyByteTests(unittest.TestCase):
    def test_boolean_is_bit_five(self) -> None:
        self.assertTrue(vnl.property_bool(0x20))
        self.assertTrue(vnl.property_bool(0xFF))
        self.assertFalse(vnl.property_bool(0xDF))
        self.assertFalse(vnl.property_bool(0x00))


class DisplayNameTests(unittest.TestCase):
    def test_numeric_id_requests_the_text_ui_row(self) -> None:
        self.assertEqual(
            vnl.display_name_request(1000001), ("xtx/_text_ui", 222, 1000001)
        )

    def test_custom_string_sentinel_makes_no_request(self) -> None:
        self.assertIsNone(vnl.display_name_request(0xFFFFFFFF))


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
        self.assertEqual(vnl.verify(ecr.Image(self.data)), [])

    def test_wait0_compare_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x004D9E8F, 1, 0x37))
        self.assertTrue(any("container_wait0" in e for e in errors))

    def test_press_fanout_commands_are_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x004D9EBD, 1, 0x79))
        self.assertTrue(any("container_press_fanout" in e for e in errors))

    def test_release_raise_guard_polarity_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00590E64, 1, 0x84))
        self.assertTrue(any("key_raise_guard" in e for e in errors))

    def test_key_gate_polarity_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00590C00, 0, 0x75))
        self.assertTrue(any("key_gate" in e for e in errors))

    def test_row_range_wipe_exit_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00590C12, 0, 0x73))
        self.assertTrue(any("key_row_range_wipe" in e for e in errors))

    def test_null_row_wipe_exit_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00590DAD, 0, 0x01))
        self.assertTrue(any("key_null_row_wipe" in e for e in errors))

    def test_talker_compare_result_is_unused_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00594FAC, 1, 0x00))
        self.assertTrue(any("update_talker_compare_discard" in e for e in errors))

    def test_last_category_condition_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00594FDC, 1, 0x07))
        self.assertTrue(any("update_talker_last_category" in e for e in errors))

    def test_event_field_8_constructor_store_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00929137, 1, 0x4B))
        self.assertTrue(any("event_constructor_field_8" in e for e in errors))

    def test_event_source_gate_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x0091E212, 0, 0x74))
        self.assertTrue(any("element_raise_gate" in e for e in errors))

    def test_element_handle_store_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x0091E11D, 2, 0x6C))
        self.assertTrue(any("element_ctor_handle_store" in e for e in errors))

    def test_wait_weight_field_default_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x005578AB, 2, 0x50))
        self.assertTrue(any("fontattr_ctor_tail" in e for e in errors))

    def test_page_row_floor_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00959C0D, 2, 2))
        self.assertTrue(any("page_rows_calc" in e for e in errors))

    def test_scroll_step_operand_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x005957DA, 4, 0xD1))
        self.assertTrue(any("pace_step" in e for e in errors))

    def test_lines_begin_hi_default_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00595B4C, 1, 0x0B))
        self.assertTrue(any("default_lines_begin_hi" in e for e in errors))

    def test_pacing_constant_is_pinned(self) -> None:
        errors = vnl.check_operands(self.patched(0x00F55AD0, 0, 0x01))
        self.assertEqual(len(errors), 1)

    def test_wait0_reference_set_is_pinned(self) -> None:
        errors = vnl.check_refs(self.patched(0x00594B1F, 0, 0x39))
        self.assertTrue(any("0x0133a438" in e for e in errors))

    def test_attribute_store_set_is_pinned(self) -> None:
        errors = vnl.check_attribute_stores(self.patched(0x0055AFCA, 2, 0x50))
        self.assertEqual(len(errors), 1)
        self.assertIn("0x55afca", errors[0])

    def test_attribute_constructor_callers_are_pinned(self) -> None:
        errors = vnl.check_callers(self.patched(0x00557A27, 0, 0x90))
        self.assertTrue(any("0x00557810" in e for e in errors))

    def test_decoder_handler_vector_is_pinned(self) -> None:
        errors = vnl.check_decoder_handlers(self.patched(0x00CA043D, 3, 0x01))
        self.assertEqual(len(errors), 1)

    def test_decoder_passthrough_lead_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x00C9F310, 3, 0x16))
        self.assertTrue(any("decoder_passthrough_0x15" in e for e in errors))

    def test_desktop_widget_initial_flag_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x006E0698, 1, 0x00))
        self.assertTrue(any("desktopwidget_ctor_flag" in e for e in errors))

    def test_boolean_bit_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x0093E227, 2, 0x04))
        self.assertTrue(any("bool_setter" in e for e in errors))

    def test_name_lookup_row_is_pinned(self) -> None:
        errors = vnl.check_listings(self.patched(0x0076FDFE, 1, 0xDF))
        self.assertTrue(any("name_lookup_call" in e for e in errors))

    def test_command_object_is_pinned(self) -> None:
        errors = vnl.check_commands(self.patched(0x00F0E314, 1, 0x39))
        self.assertTrue(any("initializer" in e for e in errors))

    def test_property_literal_is_pinned(self) -> None:
        errors = vnl.check_literals(self.patched(0x00FA8E80, 0, 0x74))
        self.assertEqual(len(errors), 1)

    def test_caller_sets_are_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x004D9EA4) + 1] ^= 0x01
        errors = vnl.check_callers(ecr.Image(bytes(patched)))
        self.assertTrue(errors)

    def test_document_interface_slot_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(vnl.LOG_DOCUMENT_INTERFACE + 12)] ^= 0x01
        errors = vnl.check_tables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("slot 3", errors[0])

    def test_set_display_name_slot_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(vnl.SET_DISPLAY_NAME[0] + 4)] ^= 0x01
        errors = vnl.check_tables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("SetDisplayNameReceiver", errors[0])


CLIENT = os.environ.get("XIVL_CLIENT_ROOT")
BLOWFISH = os.environ.get("XIVL_BLOWFISH")


@unittest.skipUnless(CLIENT and BLOWFISH, "no client install or Blowfish module")
class InstallTests(unittest.TestCase):
    def test_install_satisfies_every_form_and_script_check(self) -> None:
        self.assertEqual(vnl.check_install(Path(CLIENT), Path(BLOWFISH)), [])

    def test_form_attribute_mismatch_is_reported(self) -> None:
        with mock.patch.dict(vnl.NPC_SAY_LOG, {"IsEnableKeyWait": "False"}):
            errors = vnl.check_install(Path(CLIENT), Path(BLOWFISH))
        self.assertEqual(len(errors), 1)
        self.assertIn("IsEnableKeyWait", errors[0])

    def test_script_mention_of_a_property_name_is_reported(self) -> None:
        names = (*vnl.UNSET_PROPERTY_NAMES, b"cancelLogWait")
        with mock.patch.object(vnl, "UNSET_PROPERTY_NAMES", names):
            errors = vnl.check_install(Path(CLIENT), Path(BLOWFISH))
        self.assertEqual(len(errors), 1)
        self.assertIn("cancelLogWait occurs in 2 scripts", errors[0])


if __name__ == "__main__":
    unittest.main()
