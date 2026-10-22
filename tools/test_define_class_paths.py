"""Contract tests for verify_define_class_paths.py."""

from __future__ import annotations

import struct
import unittest
from pathlib import Path

import extract_class_registrations as ecr
import verify_define_class_paths as vdc
from verify_define_class_paths import Site

HEADER = bytes.fromhex("1b4c75615100010404040800")


def word(op: int, a: int, bx: int) -> int:
    return op | a << 6 | bx << 14


def call(a: int) -> int:
    return 28 | a << 6 | 3 << 23 | 1 << 14


def chunk(code: list[int], names: list[bytes], children: bytes = b"") -> bytes:
    """One main function with the given code and string constants."""
    out = HEADER + struct.pack("<I", 0) + struct.pack("<II", 0, 0) + bytes(4)
    out += struct.pack("<I", len(code)) + b"".join(struct.pack("<I", w) for w in code)
    out += struct.pack("<I", len(names))
    for name in names:
        out += b"\x04" + struct.pack("<I", len(name) + 1) + name + b"\0"
    out += struct.pack("<I", 1 if children else 0) + children
    return out + struct.pack("<IIII", 0, 0, 0, 0)


def define(api: int, child: int, parent: int, reg: int = 0) -> list[int]:
    return [
        word(5, reg, api),
        word(1, reg + 1, child),
        word(1, reg + 2, parent),
        call(reg),
    ]


NAMES = [b"_defineClass", b"_defineBaseClass", b"Child", b"Parent", b"Other"]


class DefineSiteTests(unittest.TestCase):
    def test_literal_site_reads_child_first_then_parent(self) -> None:
        sites = vdc.define_sites(chunk(define(0, 2, 3), NAMES))
        self.assertEqual(sites, [Site("_defineClass", "Child", "Parent")])

    def test_base_native_is_distinguished(self) -> None:
        sites = vdc.define_sites(chunk(define(1, 2, 3), NAMES))
        self.assertEqual(sites[0].api, "_defineBaseClass")

    def test_registers_must_follow_the_callee(self) -> None:
        code = define(0, 2, 3)
        code[1] = word(1, 5, 2)
        self.assertEqual(
            vdc.define_sites(chunk(code, NAMES)), [Site("_defineClass", None, None)]
        )

    def test_call_must_pass_two_arguments_and_keep_one_result(self) -> None:
        for b, c in ((4, 1), (3, 2), (3, 0)):
            code = define(0, 2, 3)
            code[3] = 28 | b << 23 | c << 14
            self.assertEqual(
                vdc.define_sites(chunk(code, NAMES)),
                [Site("_defineClass", None, None)],
            )

    def test_nested_function_sites_are_found(self) -> None:
        inner = chunk(define(0, 4, 3), NAMES)[len(HEADER) :]
        sites = vdc.define_sites(chunk(define(0, 2, 3), NAMES, inner))
        self.assertEqual([s.child for s in sites], ["Child", "Other"])

    def test_unrelated_global_is_ignored(self) -> None:
        self.assertEqual(vdc.define_sites(chunk([word(5, 0, 2)], NAMES)), [])

    def test_unexpected_layout_is_refused(self) -> None:
        with self.assertRaises(ValueError):
            vdc.define_sites(b"\x1bLuaQ\x00\x01\x08\x08\x04\x08\x00")


class SummaryTests(unittest.TestCase):
    def test_repeat_counts_follow_the_parent_set(self) -> None:
        sites = [
            Site("_defineBaseClass", "Base", "Root"),
            Site("_defineClass", "A", "Base"),
            Site("_defineClass", "A", "Base"),
            Site("_defineClass", "B", "Base"),
            Site("_defineClass", "B", "Root"),
        ]
        got = vdc.summarize(sites, ["Root"])
        self.assertEqual(got["names at more than one site"], 2)
        self.assertEqual(got["most sites for one name"], 2)
        self.assertEqual(got["repeated with the same parent"], 1)
        self.assertEqual(got["repeated with different parents"], 1)
        self.assertEqual(got["parents neither native nor _defineBaseClass"], 0)

    def test_concrete_class_used_as_parent_is_counted(self) -> None:
        sites = [Site("_defineClass", "A", "Root"), Site("_defineClass", "B", "A")]
        self.assertEqual(
            vdc.summarize(sites, ["Root"])["parents defined by _defineClass"], 1
        )

    def test_native_name_redefined_by_a_script_is_counted(self) -> None:
        sites = [Site("_defineClass", "Root", "Base")]
        self.assertEqual(
            vdc.summarize(sites, ["Root", "Base"])[
                "child names that are native registrations"
            ],
            1,
        )

    def test_parent_closed_through_the_class_shim_is_counted(self) -> None:
        sites = [Site("_defineClass", "A", "Math")]
        got = vdc.summarize(sites, ["Math"], ["Math"])
        self.assertEqual(
            got["parents closed through 0x00CC71F0 by the registry function"], 1
        )

    def test_non_literal_sites_stay_out_of_name_counts(self) -> None:
        got = vdc.summarize([Site("_defineClass", None, None)], [])
        self.assertEqual((got["sites"], got["distinct child names"]), (1, 0))


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

    def test_pinned_image_satisfies_every_check(self) -> None:
        self.assertEqual(vdc.verify(ecr.Image(self.data)), [])

    def test_parent_flag_test_is_pinned_to_offset_7c(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00CD923B) + 2] = 0x7B
        errors = vdc.check_listings(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("parent_flag ", errors[0])

    def test_close_flag_writes_are_pinned_to_their_offsets(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00CD9C20) + 2] = 0x7B
        errors = vdc.check_listings(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("close_class_flag", errors[0])

    def test_registry_closes_twelve_and_twenty_four_names(self) -> None:
        closed = vdc.registry_closures(ecr.Image(self.data))
        self.assertEqual(len(closed[vdc.CLOSE_CLASS]), 12)
        self.assertEqual(len(closed[vdc.CLOSE_BASE]), 24)
        self.assertIn("Global", closed[vdc.CLOSE_CLASS])
        self.assertIn("QuestBaseClass", closed[vdc.CLOSE_BASE])

    def test_native_registrations_cover_forty_one_names(self) -> None:
        self.assertEqual(len(vdc.native_names(ecr.Image(self.data))), 41)


if __name__ == "__main__":
    unittest.main()
