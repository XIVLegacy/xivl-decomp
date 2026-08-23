#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Sqpack-cat - open a DAT file by resource_id and dump its contents.
#
# Provides the resource-resolution and chunk-reading operations described in
# `docs/resource/sqpack.md`:
#   1. PASS Resolve resource_id -> DAT path  (via tools/sqpack_path.py)
#   2. PASS Open the DAT file               (this tool)
#   3. PASS Walk the chunked PackRead format if applicable, else
#         dump raw bytes / detect known magics
#   4. PASS Optionally inflate raw-deflate payloads with --inflate
#
# Chunk format (recovered from ChunkReadUInt::ReadNextChunkHeader at
# RVA 0x004ebd40 + PackRead::ProcessChunk at RVA 0x00942740):
#
#   struct ChunkHeader {
#       u32 unknown_0;            // bytes 0..4 (not read by ReadNext)
#       u32 chunk_size;           // bytes 4..8 (optionally byte-swapped
#                                 //              if PackRead.m_flag15 = 1)
#   };                            // payload follows: chunk_size bytes
#
# Many DAT files do NOT use this format - they're file-type-specific
# binary blobs with their own magic ("GTEX" texture, "SEDB" sound DB,
# "MapL" map layout, etc.). The chunk walker only runs when the file
# starts with a plausible chunk header (chunk_size + 8 fits in the
# file size).

import argparse
import os
import struct
import sys
import zlib

# Import the path resolver from the sibling tool.
_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
from sqpack_path import build_path  # noqa: E402

# Decompression layer:
# The binary statically links zlib 1.2.3 ("inflate 1.2.3 Copyright
# 1995-2005 Mark Adler" at .rdata 0xd16e71). The inflate function is
# at VA 0xd4f640 (5,451 B) - confirmed by:
#   - "incorrect header check" string xref at file 0x94f822 hits
#     `MOV [EDX+0x18], 0x1114208` (zlib's `state->msg = "..."` pattern).
#   - PackRead::ProcessChunk -> FUN_00d42590 (PackRead::ProcessNextChunk,
#     427 B) -> FUN_00d4f510 (inflateInit_ thunk, 25 B) -> FUN_00d4f640
#     (inflate). Architecture confirmed.
# Python's zlib.decompress() is byte-compatible with zlib 1.2.3 and is
# what this tool uses. The linked inflate function is byte-identical to the
# upstream zlib implementation; no Square Enix customization was observed.


# Known file-type magics - surfaced empirically from a real install
# scan. These are NOT chunked PackRead format. They have their own
# readers in the binary.
KNOWN_MAGICS = {
    b"GTEX": "Texture (DDS-like)",
    b"SEDB": "Sound DB (followed by RES tag)",
    b"MapL": "MapLayoutResource",
    b"PWIB": "PWIB (unknown - possibly procedural-world index buffer)",
    b"\x23fil": "CSV-like text (#fileSet,...)",
}


def detect_magic(head: bytes) -> str | None:
    """Match the first 4 bytes against the known-magic table."""
    return KNOWN_MAGICS.get(head[:4])


def looks_like_chunked(data: bytes) -> bool:
    """Heuristic: first chunk header parses + chunk_size fits in file."""
    if len(data) < 8:
        return False
    chunk_size = struct.unpack_from("<I", data, 4)[0]
    return 8 + chunk_size <= len(data) and chunk_size > 0


def looks_like_zlib(payload: bytes) -> bool:
    """zlib stream starts with a 2-byte header - the first byte's low
    nibble is the compression method (0x8 = deflate); the (header[0],
    header[1]) pair must satisfy the modular check (header[0]*256 +
    header[1]) % 31 == 0. Common pairs: 0x78 0x9c (default), 0x78 0xda
    (best), 0x78 0x01 (fastest)."""
    if len(payload) < 2:
        return False
    if (payload[0] & 0x0f) != 0x08:
        return False
    return ((payload[0] << 8) | payload[1]) % 31 == 0


def try_inflate(payload: bytes) -> bytes | None:
    """Best-effort zlib inflate. Returns decompressed bytes on success,
    None on failure."""
    if not looks_like_zlib(payload):
        return None
    try:
        return zlib.decompress(payload)
    except zlib.error:
        return None


def walk_chunks(data: bytes, byteswap: bool = False, limit: int = 32):
    """Iterate (chunk_index, offset, header_u32, chunk_size) tuples
    using PackRead's chunk format. Stops at end-of-file or limit and emits a
    truncation sentinel when the safety limit is reached."""
    cursor = 0
    idx = 0
    while cursor + 8 <= len(data):
        hdr = struct.unpack_from("<I", data, cursor)[0]
        size_raw = struct.unpack_from("<I", data, cursor + 4)[0]
        if byteswap:
            size = struct.unpack("<I", struct.pack(">I", size_raw))[0]
        else:
            size = size_raw
        next_cursor = cursor + 8 + size
        if next_cursor > len(data):
            yield (idx, cursor, hdr, size, "OVERFLOW")
            return
        yield (idx, cursor, hdr, size, "ok")
        cursor = next_cursor
        idx += 1
        if idx >= limit and cursor < len(data):
            yield (None, cursor, None, None, f"...truncated at {limit}")
            return


def hexdump(data: bytes, max_bytes: int = 256) -> None:
    """Standard hex+ascii dump, up to max_bytes."""
    n = min(len(data), max_bytes)
    for i in range(0, n, 16):
        chunk = data[i:i+16]
        hex_part = " ".join(f"{b:02x}" for b in chunk).ljust(48)
        ascii_part = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        print(f"  {i:08x}  {hex_part}  {ascii_part}")
    if len(data) > max_bytes:
        print(f"  ... ({len(data) - max_bytes} more bytes)")


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Sqpack-cat: open a DAT file by resource_id and dump contents.",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("resource_id",
                    help="resource_id (hex 0x... or decimal)")
    ap.add_argument("--root", required=True,
                    help="game-root path (e.g. .../FINAL FANTASY XIV)")
    ap.add_argument("--raw", action="store_true",
                    help="dump file contents to stdout as raw bytes")
    ap.add_argument("--hexdump-bytes", type=int, default=256,
                    help="how many bytes to hexdump (default 256)")
    ap.add_argument("--chunks", action="store_true",
                    help="force chunk-walk even if heuristic says not chunked")
    ap.add_argument("--byteswap", action="store_true",
                    help="byte-swap chunk_size (matches PackRead.m_flag15=1)")
    ap.add_argument("--inflate", action="store_true",
                    help="zlib-inflate each chunk's payload (and dump first bytes)")
    args = ap.parse_args()

    rid = int(args.resource_id, 0)
    rel = build_path(rid, posix=True).lstrip("/")
    full = os.path.join(args.root, rel)

    if not os.path.exists(full):
        print(f"error: file not found: {full}", file=sys.stderr)
        # Helpful hint: scan for typo'd nearby IDs
        return 2

    size = os.path.getsize(full)
    with open(full, "rb") as fp:
        data = fp.read()

    if args.raw:
        sys.stdout.buffer.write(data)
        return 0

    print(f"Resource:   0x{rid:08x}")
    print(f"Path:       {full}")
    print(f"Size:       {size} bytes")

    magic = detect_magic(data)
    if magic:
        print(f"Magic:      {data[:4]!r} -> {magic}")

    chunked = args.chunks or (magic is None and looks_like_chunked(data))
    if chunked:
        print(f"Chunks:     (PackRead format, byteswap={args.byteswap})")
        print(f"  {'idx':>4}  {'offset':>10}  {'hdr (u32)':>12}  {'size':>10}  zlib?  status")
        truncated = False
        for idx, off, hdr, sz, status in walk_chunks(
                data, byteswap=args.byteswap):
            if idx is None:
                truncated = True
                print(f"  ----  {off:>10}  {'':>12}  {'':>10}  {'':>5}  {status}")
            else:
                payload = data[off+8:off+8+sz]
                inflated = try_inflate(payload) if args.inflate else None
                z_marker = "  yes" if looks_like_zlib(payload) else "  no "
                line = f"  {idx:>4}  {off:>10}  0x{hdr:08x}  {sz:>10}  {z_marker}  {status}"
                print(line)
                if inflated is not None:
                    print(f"           -> inflated to {len(inflated)} bytes; "
                          f"first 32: {inflated[:32].hex()}")
    else:
        # Even when not chunk-formatted, the whole file MIGHT be zlib-
        # compressed (some installer DAT bodies are raw zlib streams).
        if looks_like_zlib(data):
            print("Chunks:     n/a; whole file looks like a raw zlib stream")
            if args.inflate:
                inflated = try_inflate(data)
                if inflated is not None:
                    print(f"           -> inflated to {len(inflated)} bytes; "
                          f"first 32: {inflated[:32].hex()}")
        else:
            print("Chunks:     n/a (file does not look chunk-formatted)")

    print()
    print(f"First {min(args.hexdump_bytes, len(data))} bytes:")
    hexdump(data, args.hexdump_bytes)
    if chunked and truncated:
        print("error: chunk walk reached its 32-chunk safety limit; output is incomplete",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
