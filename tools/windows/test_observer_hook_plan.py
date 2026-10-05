# SPDX-License-Identifier: AGPL-3.0-or-later
"""Asset-free checks for copied instructions and x86 redirect arithmetic."""

import struct
import unittest

import observer_hook_plan as plan


def destination(source: int, jump: bytes) -> int:
    assert jump[0] == 0xE9
    return (source + 5 + struct.unpack_from("<i", jump, 1)[0]) & 0xFFFFFFFF


class HookPlanTests(unittest.TestCase):
    def build(self, name: str, **overrides: int) -> dict:
        args = dict(
            loaded_base=0x20000000,
            image_size=0x600000,
            wrapper=0x30000000,
            wrapper_size=0x100,
            trampoline=0x40000000,
        )
        args.update(overrides)
        return plan.plan_redirect(name, **args)

    def test_full_instructions_and_return_target(self) -> None:
        # The native write entry's sixth byte completes MOV EDI, ECX.
        expected = {
            "lookup": "8bff558bec",
            "query": "6a04b8ef5e4e20",
            "context_write": "8bff56578bf9",
        }
        for name, original in expected.items():
            with self.subTest(name=name):
                result = self.build(name)
                copied = bytes.fromhex(result["trampoline_bytes"])
                redirect = bytes.fromhex(result["redirect_bytes"])
                span = len(bytes.fromhex(original))
                self.assertEqual(copied[:span].hex(), original)
                self.assertEqual(len(redirect), span)
                self.assertEqual(redirect[5:], b"\x90" * (span - 5))
                self.assertEqual(
                    destination(result["site"], redirect), result["wrapper"]
                )
                self.assertEqual(
                    destination(result["trampoline"] + span, copied[span:]),
                    result["site"] + span,
                )
                self.assertFalse(result["installed"])
                self.assertFalse(result["live_coverage"])

    def test_high_x86_address_uses_modulo_relative_jump(self) -> None:
        result = self.build("lookup", wrapper=0xF0000000)
        self.assertEqual(
            destination(result["site"], bytes.fromhex(result["redirect_bytes"])),
            0xF0000000,
        )

    def test_overlap_extent_and_wrap_are_rejected(self) -> None:
        for overrides in [
            dict(wrapper=0x20001000),
            dict(trampoline=0x30000080),
            dict(wrapper_size=0x10000001),
            dict(loaded_base=0xFFFFFF00),
            dict(trampoline=0xFFFFFFF8),
            dict(wrapper=0),
            dict(image_size=0x1000),
            dict(wrapper_size=True),
        ]:
            with self.subTest(overrides=overrides), self.assertRaises(ValueError):
                self.build("lookup", **overrides)

    def test_unsupported_boundary_and_image_fail_closed(self) -> None:
        with self.assertRaises(ValueError):
            self.build("locate")
        with self.assertRaisesRegex(ValueError, "unsupported engine"):
            plan.verify_image(b"MZ" + bytes(2048))

    def test_resume_must_remain_inside_image(self) -> None:
        extent = plan.ENTRIES["lookup"].rva + len(plan.ENTRIES["lookup"].original)
        with self.assertRaises(ValueError):
            self.build(
                "lookup",
                loaded_base=0x100000000 - extent,
                image_size=extent,
                wrapper=0x10000,
                trampoline=0x20000,
            )

    def test_truncated_pe_is_rejected_without_unpack_exception(self) -> None:
        with self.assertRaisesRegex(ValueError, "truncated PE"):
            plan._parse_pe(b"MZ")


if __name__ == "__main__":
    unittest.main()
