#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Validate fixed vectors for FUN_00d31490 at RVA 0x00931490."""

from __future__ import annotations

M = 0x5BD1E995
R = 24
MASK32 = 0xFFFFFFFF


def murmur_hash2_backward(key: bytes, seed: int) -> int:
    """Port of the client's backward-walking MurmurHash2 variant."""
    h = (seed ^ len(key)) & MASK32
    for chunk in range(len(key) // 4):
        i = len(key) - 4 - chunk * 4
        h = (h * M) & MASK32
        k = (key[i] << 24) | (key[i + 1] << 16) | (key[i + 2] << 8) | key[i + 3]
        k = (k * M) & MASK32
        k ^= k >> R
        k = (k * M) & MASK32
        h = (h ^ k) & MASK32
    tail = len(key) % 4
    if tail == 3:
        h ^= key[0] << 16
        h ^= key[1] << 8
        h ^= key[2]
        h = (h * M) & MASK32
    elif tail == 2:
        h ^= key[0] << 8
        h ^= key[1]
        h = (h * M) & MASK32
    elif tail == 1:
        h ^= key[0]
        h = (h * M) & MASK32
    h ^= h >> 13
    h = (h * M) & MASK32
    h ^= h >> 15
    return h & MASK32


VECTORS = {
    "": 0x00000000,
    "a": 0x92685F5E,
    "hello": 0x08C5DAA9,
    "vector": 0x1294324A,
    "vector2": 0x9C7A9994,
    "vector02": 0xC35BFD82,
}


def main() -> int:
    failed = []
    for value, expected in VECTORS.items():
        actual = murmur_hash2_backward(value.encode("utf-8"), 0)
        if actual != expected:
            failed.append((value, expected, actual))
    if failed:
        for value, expected, actual in failed:
            print(f"FAIL {value!r}: expected 0x{expected:08x}, got 0x{actual:08x}")
        return 1
    print(f"PASS: {len(VECTORS)} Murmur2 vectors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
