"""Contract tests for verify_npcsay_backdrop_draw.py."""

from __future__ import annotations

import struct
import unittest
from pathlib import Path

import extract_class_registrations as ecr
import verify_npcsay_backdrop_draw as vnb

# The colour, blend and cell models below restate the documented contract by
# hand. Only RetailImageTests check the executable, and they skip when
# orig/ffxivgame.exe is absent. InstallParserTests run on synthetic data.


class PackTests(unittest.TestCase):
    def test_channel_truncates_and_does_not_round(self) -> None:
        self.assertEqual(vnb.pack_channel(0.5), 127)
        self.assertEqual(vnb.pack_channel(0.999), 254)

    def test_channel_clamps_to_the_byte_range(self) -> None:
        self.assertEqual(vnb.pack_channel(-0.25), 0)
        self.assertEqual(vnb.pack_channel(1.0), 255)
        self.assertEqual(vnb.pack_channel(7.0), 255)


class ModulationTests(unittest.TestCase):
    def test_state_and_command_colours_multiply_per_component(self) -> None:
        got = vnb.modulate((1.0, 0.5, 0.25, 0.5), (1.0, 1.0, 0.5, 0.5))
        self.assertEqual(got, (1.0, 0.5, 0.125, 0.25))

    def test_white_command_passes_the_state_colour_through(self) -> None:
        state = (0.2, 0.4, 0.6, 0.8)
        self.assertEqual(vnb.modulate(state, (1.0, 1.0, 1.0, 1.0)), state)

    def test_zero_state_alpha_removes_the_image(self) -> None:
        got = vnb.modulate((1.0, 1.0, 1.0, 0.0), (1.0, 1.0, 1.0, 1.0))
        self.assertEqual(got[3], 0.0)

    def test_pixel_program_multiplies_texel_by_vertex_colour(self) -> None:
        got = vnb.ui_pixel((0.0, 0.0, 0.0, 166 / 255), (1.0, 1.0, 1.0, 0.5))
        self.assertEqual(got, (0.0, 0.0, 0.0, 83 / 255))


class BlendTests(unittest.TestCase):
    def test_black_at_the_plateau_darkens_in_gamma_space(self) -> None:
        got = vnb.blend_over((0.8, 0.8, 0.8), (0.0, 0.0, 0.0, 166 / 255))
        self.assertAlmostEqual(got[0], 0.8 * (1 - 166 / 255))

    def test_zero_alpha_leaves_the_destination(self) -> None:
        got = vnb.blend_over((0.3, 0.6, 0.9), (0.0, 0.0, 0.0, 0.0))
        self.assertEqual(got, (0.3, 0.6, 0.9))

    def test_linear_blend_is_lighter_than_the_gamma_blend(self) -> None:
        source = (0.0, 0.0, 0.0, 166 / 255)
        gamma = vnb.blend_over((0.8, 0.8, 0.8), source)[0]
        linear = vnb.blend_in_linear_space((0.8, 0.8, 0.8), source)[0]
        self.assertGreater(linear, gamma + 0.1)

    def test_blend_spaces_agree_at_the_endpoints(self) -> None:
        for alpha in (0.0, 1.0):
            source = (0.0, 0.0, 0.0, alpha)
            gamma = vnb.blend_over((0.5, 0.5, 0.5), source)[0]
            linear = vnb.blend_in_linear_space((0.5, 0.5, 0.5), source)[0]
            self.assertAlmostEqual(gamma, linear, places=6)


class CellModelTests(unittest.TestCase):
    def test_cell_row_stores_mode_and_far_edges(self) -> None:
        self.assertEqual(
            vnb.cell_row("Tiling", 10, 20, 3, 4), vnb.CellRow(1, 10, 13, 20, 24)
        )

    def test_mode_numbers_follow_the_parser_order(self) -> None:
        self.assertEqual(vnb.cell_row("Stretch", 0, 0, 1, 1).mode, 0)
        self.assertEqual(vnb.cell_row("ConstantSize", 0, 0, 1, 1).mode, 2)

    def test_native_corner_tracks_for_the_body_label(self) -> None:
        self.assertEqual(
            vnb.native_corner_tracks(620.0, 64.0, 64.0), [64.0, 492.0, 64.0]
        )
        self.assertEqual(
            vnb.native_corner_tracks(106.0, 32.0, 32.0), [32.0, 42.0, 32.0]
        )

    def test_native_corner_tracks_shrink_when_the_total_is_small(self) -> None:
        tracks = vnb.native_corner_tracks(32.0, 32.0, 32.0)
        self.assertEqual(tracks, [16.0, 0.0, 16.0])


class InstallParserTests(unittest.TestCase):
    FORM = (
        '<Window xmlns="http://namespace.square-enix.com/sqwt" SqwtStyle="WIN_null" '
        'Name="Window_NpcSayWidget"><Grid><Label SqwtStyle="LAB_basis_npcLinesBody" '
        'Width="620" Height="106" Grid.Row="1"/></Grid></Window>'
    )

    def test_form_shape_reads_names_styles_and_sizes(self) -> None:
        shape = vnb.form_shape(self.FORM)
        self.assertEqual(
            shape["rows"][0],
            ("Window", "Window_NpcSayWidget", "WIN_null", None, None, None),
        )
        self.assertEqual(
            shape["rows"][1],
            ("Label", None, "LAB_basis_npcLinesBody", "620", "106", "1"),
        )
        self.assertEqual(shape["forbidden"], [])

    def test_form_shape_reports_an_opacity_attribute(self) -> None:
        text = self.FORM.replace('Width="620"', 'Width="620" VisualOpacity="0.5"')
        self.assertEqual(
            vnb.form_shape(text)["forbidden"], [("Label", "VisualOpacity")]
        )

    STYLE = (
        '<StyleResources><Widget Type="Label"><SqwtStyle Name="S"><Parts Name="Frame">'
        '<Property Condition="SkinNormal" Skin="A"/>'
        '<Property Condition="SkinReserved" Skin="NULL"/></Parts></SqwtStyle></Widget>'
        '<Widget Type="LogControl"><SqwtStyle Name="L">'
        '<Parts Name="Background" Reference="Window" Style="WIN_null"/>'
        "</SqwtStyle></Widget></StyleResources>"
    )

    def test_style_rows_maps_condition_to_skin(self) -> None:
        rows = vnb.style_rows(self.STYLE, "Label", "S")
        self.assertEqual(rows, {"Frame": {"SkinNormal": "A", "SkinReserved": "NULL"}})

    def test_style_rows_reads_a_part_reference(self) -> None:
        rows = vnb.style_rows(self.STYLE, "LogControl", "L")
        self.assertEqual(rows["Background"]["reference"], "Window:WIN_null")

    SKIN = (
        '<SkinResources><CellSkins Source="common/default.dds">'
        '<CellSkin Cells="1,1" Name="X" HitMargin="1,2,3,4" SkinBorderThickness="6">'
        '<Cell DrawMode="Stretch" Rectangle="5,6,7,8"/></CellSkin></CellSkins></SkinResources>'
    )

    def test_cell_skin_reads_rectangles_as_parser_rows(self) -> None:
        got = vnb.cell_skin(self.SKIN, "X")
        self.assertEqual(got["rows"], [vnb.CellRow(0, 5, 12, 6, 14)])
        self.assertEqual((got["hit"], got["border"]), ("1,2,3,4", "6"))

    @staticmethod
    def gtex(width: int, height: int, pixels: bytes) -> bytes:
        header = bytearray(32)
        header[0:4] = b"GTEX"
        header[6] = 4
        header[7] = 1
        struct.pack_into(">HH", header, 0x0A, width, height)
        struct.pack_into(">II", header, 0x10, 0x18, 0x20)
        struct.pack_into(">II", header, 0x18, 0, len(pixels))
        return bytes(header) + pixels

    def test_gtex_reader_returns_bgra_surface(self) -> None:
        data = self.gtex(2, 1, bytes([1, 2, 3, 4, 5, 6, 7, 8]))
        texture = vnb.read_gtex_a8r8g8b8(data)
        self.assertEqual((texture.width, texture.height), (2, 1))
        self.assertEqual(vnb.texel(texture, 0, 0), (3, 2, 1, 4))
        self.assertEqual(vnb.texel(texture, 1, 0), (7, 6, 5, 8))

    def test_gtex_reader_rejects_other_formats(self) -> None:
        data = bytearray(self.gtex(1, 1, bytes(4)))
        data[6] = 22
        with self.assertRaises(SystemExit):
            vnb.read_gtex_a8r8g8b8(bytes(data))

    def test_half_point_finds_the_first_texel_at_half_the_plateau(self) -> None:
        self.assertEqual(vnb.half_point([0, 10, 82, 83, 166]), 3)

    def test_profiles_read_the_edge_texel_lines(self) -> None:
        pixels = bytearray()
        for y in range(2):
            for x in range(3):
                pixels += bytes([0, 0, 0, 10 * y + x])
        texture = vnb.read_gtex_a8r8g8b8(self.gtex(3, 2, bytes(pixels)))
        self.assertEqual(vnb.row_profile(texture, (0, 0, 3, 2)), [10, 11, 12])
        self.assertEqual(vnb.column_profile(texture, (0, 0, 3, 2)), [2, 12])


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
        self.assertEqual(vnb.verify(ecr.Image(self.data)), [])

    def test_state_multiply_offset_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x0069679D, 0, 0x91))
        self.assertTrue(any("image_execute_modulate" in e for e in errors))

    def test_flag_gate_polarity_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x00696782, 0, 0x74))
        self.assertTrue(any("image_execute_flag_gate" in e for e in errors))

    def test_source_blend_index_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x00CA38FF, 0, 0x6A))
        self.assertTrue(any("ui_state_blend_factors" in e for e in errors))

    def test_destination_blend_index_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x00CA38FF, 1, 0x05))
        self.assertTrue(any("ui_state_blend_factors" in e for e in errors))

    def test_blend_state_id_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x0041C1FC, 1, 0x1C))
        self.assertTrue(any("blend_enable_wrapper" in e for e in errors))

    def test_alpha_test_enable_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x00CA34AE, 1, 0x00))
        self.assertTrue(any("flush_alpha_test" in e for e in errors))

    def test_opacity_default_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x00940647, 1, 0xEE))
        self.assertTrue(any("visual_opacity_default" in e for e in errors))

    def test_opacity_context_slot_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x0093F923, 2, 0x48))
        self.assertTrue(any("element_context_visual_opacity" in e for e in errors))

    def test_opacity_record_offset_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x0055A05F, 4, 0x90))
        self.assertTrue(any("context_slot17_store" in e for e in errors))

    def test_scene_packet_offset_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x0054B11F, 2, 0x16))
        self.assertTrue(any("scene_packet_index_offset" in e for e in errors))

    def test_cell_stride_is_pinned(self) -> None:
        errors = vnb.check_listings(self.patched(0x009A8506, 0, 0x00))
        self.assertTrue(any("cell_stride" in e for e in errors))

    def test_literal_is_pinned(self) -> None:
        errors = vnb.check_literals(self.patched(0x0107C4F4, 0, 0x73))
        self.assertEqual(len(errors), 1)

    def test_blend_factor_table_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00F596E0 + 4 * 6)] ^= 0x01
        errors = vnb.check_tables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("blend factor", errors[0])

    def test_pixel_program_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        pointer = vnb.read_dword(image, vnb.PIXEL_PROGRAMS[0] + 4)
        patched = bytearray(self.data)
        # flip the opcode of the last mul in the pixel program
        index = bytes(patched).find(b"\x05\x00\x00\x03", image.offset(pointer))
        patched[index] = 0x02
        errors = vnb.check_programs(ecr.Image(bytes(patched)))
        self.assertTrue(errors)

    def test_vtable_slot_is_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x00FA2A00 + 4)] ^= 0x01
        errors = vnb.check_vtables(ecr.Image(bytes(patched)))
        self.assertEqual(len(errors), 1)
        self.assertIn("DrawImage slot 1", errors[0])

    def test_caller_sets_are_pinned(self) -> None:
        image = ecr.Image(self.data)
        patched = bytearray(self.data)
        patched[image.offset(0x0069681A) + 1] ^= 0x01
        errors = vnb.check_callers(ecr.Image(bytes(patched)))
        self.assertTrue(errors)

    def test_srgb_write_state_scan_detects_a_set(self) -> None:
        image = ecr.Image(self.data)
        site = 0x0041D022
        patched = bytearray(self.data)
        patched[image.offset(site) + 1] = vnb.RS_SRGBWRITEENABLE
        errors = vnb.check_state_ids(ecr.Image(bytes(patched)))
        self.assertTrue(any("0xC2" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
