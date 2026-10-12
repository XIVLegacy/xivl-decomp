#!/usr/bin/env python3
"""Extract exact-build controller tables and check their instruction anchors."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


IDENTITIES = {
    "ffxivconfig.exe": (
        3_471_240,
        "e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977",
    ),
    "ffxivgame.exe": (
        15_996_808,
        "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9",
    ),
}


class Image:
    def __init__(self, path: Path, name: str):
        self.data = path.read_bytes()
        size, digest = IDENTITIES[name]
        if len(self.data) != size or hashlib.sha256(self.data).hexdigest() != digest:
            raise ValueError(f"{name}: input size or SHA-256 mismatch")
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe : pe + 4] != b"PE\0\0":
            raise ValueError("PE signature mismatch")
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        optional = pe + 24
        if struct.unpack_from("<H", self.data, optional)[0] != 0x10B:
            raise ValueError("PE32 required")
        self.base = struct.unpack_from("<I", self.data, optional + 28)[0]
        self.sections = []
        for i in range(count):
            row = optional + optional_size + i * 40
            _, rva, raw_size, raw = struct.unpack_from("<IIII", self.data, row + 8)
            self.sections.append((rva, raw_size, raw))

    def read(self, va: int, size: int) -> bytes:
        rva = va - self.base
        for start, raw_size, raw in self.sections:
            if start <= rva and rva + size <= start + raw_size:
                offset = raw + rva - start
                result = self.data[offset : offset + size]
                if len(result) == size:
                    return result
        raise ValueError(f"VA 0x{va:08x}: range is not file-backed")

    def check(self, va: int, signature: str) -> None:
        expected = bytes.fromhex(signature)
        if self.read(va, len(expected)) != expected:
            raise ValueError(f"instruction anchor differs at VA 0x{va:08x}")


def extract(config: Image, game: Image) -> dict:
    # Short instructions qualify the table-to-file and file-to-consumer joins.
    for va, signature in {
        0x00430DFB: "89 46 2c",
        0x00430E0D: "89 46 40",
        0x004317BB: "3c 5c",
        0x00431815: "80 f9 18",
        0x00431844: "88 98 bc c2 45 00",
    }.items():
        config.check(va, signature)
    for va, signature in {
        0x00D34421: "8d bb 80 00 00 00 b9 52 00 00 00 f3 a5",
        0x00D3448A: "f3 0f 2a 83 a8 00 00 00",
        0x00D32C70: "0f be 94 06 ac 00 00 00",
        0x00D32CF0: "83 f8 18",
        0x00D32469: "83 bf c4 00 00 00 00",
        0x00D323F0: "8b 41 1c",
        0x00D32433: "d9 44 b0 30",
    }.items():
        game.check(va, signature)
    pairs = struct.unpack("<48I", config.read(0x0044D6C0, 24 * 8))
    controls = [
        {"map_index": i, "control_id": pairs[i * 2], "next_index": pairs[i * 2 + 1]}
        for i in range(24)
    ]
    if any(row["next_index"] >= 24 for row in controls):
        raise ValueError("focus table index exceeds the meaningful control table")
    labels = struct.unpack("<3I", config.read(0x00457450, 12))
    presets = [
        {
            "index": i,
            "label_id": labels[i],
            "va": f"0x{0x00457408 + i * 24:08x}",
            "sources": list(config.read(0x00457408 + i * 24, 24)),
        }
        for i in range(3)
    ]
    masks = []
    for i in range(24):
        if 4 <= i < 12:
            masks.append(0)
            continue
        va = 0x00D32BBA + i * 8 if i < 4 else 0x00D32BFF + (i - 12) * 8
        instruction = game.read(va, 8)
        if instruction[:3] != b"\xc7\x44\x24" or instruction[3] != 0x1C + i * 4:
            raise ValueError(f"logical mask store differs at VA 0x{va:08x}")
        masks.append(struct.unpack_from("<I", instruction, 4)[0])
    zero_stores = (
        0x00D32BDA,
        0x00D32BDE,
        0x00D32BE7,
        0x00D32BEB,
        0x00D32BEF,
        0x00D32BF3,
        0x00D32BF7,
        0x00D32BFB,
    )
    for i, va in enumerate(zero_stores, start=4):
        game.check(va, f"89 4c 24 {0x1C + i * 4:02x}")
    game.check(0x00D32BAA, "33 c9")
    return {
        "format": "xivl-controller-mapping-v1",
        "inputs": {
            name: {"bytes": size, "sha256": digest, "image_base": "0x00400000"}
            for name, (size, digest) in IDENTITIES.items()
        },
        "producer": "tools/extract_controller_mapping.py",
        "control_table_va": "0x0044d6c0",
        "controls": controls,
        "preset_label_table_va": "0x00457450",
        "presets": presets,
        "game_logical_masks": masks,
        "constants": {
            "config_assignment_threshold": struct.unpack(
                "<f", config.read(0x0044F390, 4)
            )[0],
            "config_selector_step": struct.unpack("<d", config.read(0x0044F3A8, 8))[0],
            "game_selector_step": struct.unpack("<d", game.read(0x00FBAD50, 8))[0],
            "game_button_threshold": struct.unpack("<d", game.read(0x01110608, 8))[0],
            "game_axis_denominator_margin": struct.unpack(
                "<d", game.read(0x00FC3368, 8)
            )[0],
            "game_axis_output_scale": struct.unpack("<d", game.read(0x00F55AD0, 8))[0],
            "game_xinput_trigger_divisor": struct.unpack(
                "<d", game.read(0x00F598B0, 8)
            )[0],
            "game_xinput_stick_divisor": struct.unpack("<d", game.read(0x010997F0, 8))[
                0
            ],
        },
        "limits": "Static tables and checked instructions only. No runtime result, device identity, availability or hardware qualification.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    destination = parser.add_mutually_exclusive_group(required=True)
    destination.add_argument("--output", type=Path)
    destination.add_argument("--check", type=Path)
    args = parser.parse_args()
    try:
        document = extract(
            Image(args.config, "ffxivconfig.exe"), Image(args.game, "ffxivgame.exe")
        )
        if args.check:
            if json.loads(args.check.read_text(encoding="ascii")) != document:
                raise ValueError("catalog differs from exact-build extraction")
            print("Controller tables and instruction anchors: PASS")
        else:
            if args.output.exists() and any(
                args.output.samefile(source) for source in (args.config, args.game)
            ):
                raise ValueError("output must not replace an input image")
            args.output.write_text(
                json.dumps(document, indent=2) + "\n", encoding="ascii"
            )
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f"{error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
