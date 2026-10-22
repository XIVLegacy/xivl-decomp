"""Contract tests for extract_class_registrations.py."""

from __future__ import annotations

import unittest
from pathlib import Path

import extract_class_registrations as ecr
from extract_class_registrations import Ins

LITERALS = {0x100: "", 0x110: "ParentClass", 0x120: "ChildClass", 0x130: "LeafClass"}
BASE = 0x00401000


def listing(rows: list[str]) -> list[Ins]:
    out = []
    for index, text in enumerate(rows):
        mnemonic, _, operands = text.partition(" ")
        out.append(Ins(BASE + index, mnemonic, operands))
    return out


def construct(parent: int, name: int) -> list[str]:
    return [
        "push 0",
        f"push {parent:#x}",
        "lea ecx, [esp + 0x20]",
        f"call {ecr.STRING_CTOR:#x}",
        "push 0",
        f"push {name:#x}",
        "lea ecx, [esp + 0x74]",
        f"call {ecr.STRING_CTOR:#x}",
    ]


def with_object(parent: int, name: int, size: int, ctor: int) -> list[str]:
    return [
        *construct(parent, name),
        f"push {size:#x}",
        f"call {ecr.ALLOCATOR:#x}",
        "add esp, 4",
        f"call {ctor:#x}",
        "lea edx, [esp + 0x18]",
        "push edx",
        "lea ecx, [esp + 0x70]",
        "push ecx",
        "push ecx",
        f"call {ecr.REGISTER_WITH_OBJECT:#x}",
    ]


def script_only(parent: int, name: int, pushes: tuple[str, str]) -> list[str]:
    return [
        *construct(parent, name),
        f"lea ecx, [esp + {pushes[0]}]",
        "push ecx",
        f"lea edx, [esp + {pushes[1]}]",
        "push edx",
        f"call {ecr.REGISTER_SCRIPT_ONLY:#x}",
    ]


def trace(rows: list[str]) -> list[ecr.Registration]:
    return ecr.trace(listing(rows), LITERALS.__getitem__, lambda _: 0, BASE)


class ClassRegistrationTests(unittest.TestCase):
    def test_object_row_reads_name_second_and_parent_third(self) -> None:
        rows = trace(with_object(0x110, 0x120, 0x68, 0x6E0080))
        self.assertEqual(len(rows), 1)
        self.assertEqual((rows[0].name, rows[0].parent), ("ChildClass", "ParentClass"))
        self.assertEqual((rows[0].alloc_size, rows[0].constructor_va), (0x68, 0x6E0080))

    def test_script_only_row_reads_last_push_as_name(self) -> None:
        rows = trace(script_only(0x120, 0x130, ("0x18", "0x70")))
        self.assertEqual((rows[0].name, rows[0].parent), ("LeafClass", "ChildClass"))
        self.assertIsNone(rows[0].alloc_size)

    def test_empty_parent_literal_is_preserved(self) -> None:
        rows = trace(with_object(0x100, 0x110, 0x60, 0x6DBB70))
        self.assertEqual((rows[0].name, rows[0].parent), ("ParentClass", ""))

    def test_rows_keep_address_order_across_registrations(self) -> None:
        rows = trace(
            with_object(0x100, 0x110, 0x60, 0x6DBB70)
            + script_only(0x110, 0x120, ("0x18", "0x70"))
        )
        self.assertEqual(
            [(r.name, r.parent) for r in rows],
            [("ParentClass", ""), ("ChildClass", "ParentClass")],
        )
        self.assertLess(rows[0].call_va, rows[1].call_va)

    def test_swapped_argument_slots_fail_the_cross_check(self) -> None:
        with self.assertRaises(ValueError):
            trace(script_only(0x120, 0x130, ("0x6c", "0x1c")))

    def test_find_callers_matches_call_and_jmp_rel32_only(self) -> None:
        text = bytearray(32)
        text[0:5] = b"\xe8" + (0x20 - 5).to_bytes(4, "little")  # call 0x20
        text[8:13] = b"\xe9" + (0x20 - 13).to_bytes(4, "little")  # jmp 0x20
        text[16:21] = b"\xe8" + (0x40 - 21).to_bytes(4, "little")  # other target
        image = object.__new__(ecr.Image)
        image.base = 0x400000
        image.data = bytes(text)
        image.sections = [(b".text\0\0\0", 32, 0, 32, 0)]
        found = ecr.find_callers(image, (0x400020,))
        self.assertEqual(found, {0x400020: [0x400000, 0x400008]})

    @unittest.skipUnless(Path("orig/ffxivgame.exe").is_file(), "no local image")
    def test_retail_image_table(self) -> None:
        try:
            import capstone  # noqa: F401
        except ImportError:
            self.skipTest("Capstone is not installed")
        data = Path("orig/ffxivgame.exe").read_bytes()
        ecr.verify_identity(data)
        image = ecr.Image(data)
        cleanup = ecr.callee_cleanup(image)
        found = []
        for start, end, base in ecr.WINDOWS:
            found += ecr.trace(
                ecr.load_instructions(image, start, end), image.cstr, cleanup, base
            )
        by_name = {r.name: r for r in found}
        self.assertEqual(len(found), 41)
        self.assertEqual(by_name["NpcBaseClass"].parent, "CharaBaseClass")
        self.assertEqual(by_name["PlayerBaseClass"].parent, "CharaBaseClass")
        self.assertEqual(by_name["DesktopWidgetBaseClass"].parent, "WidgetBaseClass")
        self.assertEqual(by_name["Player"].parent, "PlayerBaseClass")
        callers = ecr.find_callers(
            image, (ecr.REGISTER_WITH_OBJECT, ecr.REGISTER_SCRIPT_ONLY)
        )
        self.assertEqual(len(callers[ecr.REGISTER_WITH_OBJECT]), 26)
        self.assertEqual(len(callers[ecr.REGISTER_SCRIPT_ONLY]), 15)
        self.assertEqual(
            sorted(c for sites in callers.values() for c in sites),
            sorted(r.call_va for r in found),
        )


if __name__ == "__main__":
    unittest.main()
