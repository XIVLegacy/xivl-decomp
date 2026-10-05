# SPDX-License-Identifier: AGPL-3.0-or-later
"""Inspect one pinned x86 entry profile and plan redirects without installing them."""

import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct


ENGINE_SHA256 = "d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631"
ENGINE_SIZE = 6_097_408
PREFERRED_BASE = 0x10000000


@dataclass(frozen=True)
class Entry:
    rva: int
    original: bytes
    relocation_offsets: tuple[int, ...]
    abi: str


# Whole instruction spans only; QueryService's MOV EAX immediate is relocated.
# Static consumers and ABI: breakpoint-context-path.md, registry and API boundaries.
ENTRIES = {
    "lookup": Entry(
        0x467F13, bytes.fromhex("8bff558bec"), (), "ECX manager; GUID on stack; ret 4"
    ),
    "query": Entry(
        0x468B10,
        bytes.fromhex("6a04b8ef5e4e10"),
        (3,),
        "stdcall manager, GUID, IID, output; ret 16",
    ),
    "context_write": Entry(
        0x3D049D, bytes.fromhex("8bff56578bf9"), (), "fastcall handle ECX, context EDX"
    ),
}


def _unpack(data: bytes, offset: int, fmt: str) -> tuple:
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError("truncated PE field")
    return struct.unpack_from(fmt, data, offset)


def _parse_pe(
    data: bytes,
) -> tuple[int, int, list[tuple[int, int, int, int]], set[int]]:
    if data[:2] != b"MZ":
        raise ValueError("missing DOS signature")
    (pe,) = _unpack(data, 0x3C, "<I")
    if data[pe : pe + 4] != b"PE\0\0":
        raise ValueError("missing PE signature")
    machine, count = _unpack(data, pe + 4, "<HH")
    (optional_size,) = _unpack(data, pe + 20, "<H")
    optional = pe + 24
    (magic,) = _unpack(data, optional, "<H")
    if machine != 0x14C or magic != 0x10B or optional_size < 144:
        raise ValueError("profile requires PE32 I386 and relocation directory")
    _unpack(data, optional, f"<{optional_size}s")
    (base,) = _unpack(data, optional + 28, "<I")
    (image_size,) = _unpack(data, optional + 56, "<I")
    (directory_count,) = _unpack(data, optional + 92, "<I")
    if directory_count < 6 or not image_size:
        raise ValueError("missing relocation directory or image extent")
    sections = []
    for index in range(count):
        section = optional + optional_size + index * 40
        _unpack(data, section, "<40s")
        _, rva, raw_size, raw_offset = _unpack(data, section + 8, "<IIII")
        (flags,) = _unpack(data, section + 36, "<I")
        if raw_size and (
            raw_offset + raw_size > len(data) or rva + raw_size > image_size
        ):
            raise ValueError("section outside file or image")
        sections.append((rva, raw_size, raw_offset, flags))

    def file_offset(rva: int, size: int) -> int:
        matches = [
            offset + rva - start
            for start, extent, offset, _ in sections
            if start <= rva and rva + size <= start + extent
        ]
        if len(matches) != 1:
            raise ValueError("RVA has no unique file-backed section")
        return matches[0]

    relocation_rva, relocation_size = _unpack(data, optional + 136, "<II")
    cursor = file_offset(relocation_rva, relocation_size)
    end = cursor + relocation_size
    highlow = set()
    while cursor < end:
        page, size = _unpack(data, cursor, "<II")
        if size < 8 or size % 2 or cursor + size > end:
            raise ValueError("invalid relocation block")
        for offset in range(cursor + 8, cursor + size, 2):
            (entry,) = _unpack(data, offset, "<H")
            kind, within_page = entry >> 12, entry & 0xFFF
            if kind == 3:
                if page + within_page + 4 > image_size:
                    raise ValueError("relocation outside image")
                highlow.add(page + within_page)
            elif kind != 0:
                raise ValueError("unsupported relocation type")
        cursor += size
    return base, image_size, sections, highlow


def verify_image(data: bytes) -> dict:
    """Admit only the exact file profile; no DLL loading or resident inspection."""
    if len(data) != ENGINE_SIZE or hashlib.sha256(data).hexdigest() != ENGINE_SHA256:
        raise ValueError("unsupported engine size or SHA-256")
    base, image_size, sections, relocations = _parse_pe(data)
    if base != PREFERRED_BASE:
        raise ValueError("unsupported preferred image base")
    for name, entry in ENTRIES.items():
        matches = [
            (start, offset, flags)
            for start, extent, offset, flags in sections
            if start <= entry.rva and entry.rva + len(entry.original) <= start + extent
        ]
        if len(matches) != 1:
            raise ValueError(f"{name}: ambiguous file span")
        start, offset, flags = matches[0]
        if not flags & 0x20000000:
            raise ValueError(f"{name}: entry is not executable")
        if (
            data[
                offset + entry.rva - start : offset
                + entry.rva
                - start
                + len(entry.original)
            ]
            != entry.original
        ):
            raise ValueError(f"{name}: unexpected instruction span")
        overlapping = {
            rva - entry.rva
            for rva in relocations
            if rva < entry.rva + len(entry.original) and rva + 4 > entry.rva
        }
        if overlapping != set(entry.relocation_offsets):
            raise ValueError(f"{name}: unexpected relocation coverage")
    return {
        "profile": "pinned-x86-entry-plan-v1",
        "sha256": ENGINE_SHA256,
        "size": len(data),
        "preferred_base": base,
        "image_size": image_size,
        "installed": False,
        "live_coverage": False,
        "entries": {
            name: {
                "rva": entry.rva,
                "span": len(entry.original),
                "file_bytes": entry.original.hex(),
                "relocation_offsets": entry.relocation_offsets,
                "abi": entry.abi,
            }
            for name, entry in ENTRIES.items()
        },
    }


def _range(address: int, size: int) -> tuple[int, int]:
    if (
        type(address) is not int
        or type(size) is not int
        or address <= 0
        or size <= 0
        or address + size > 0x1_0000_0000
    ):
        raise ValueError("allocation requires a nonzero, nonwrapping x86 range")
    return address, address + size


def _jump(source: int, destination: int) -> bytes:
    return b"\xe9" + struct.pack("<I", (destination - (source + 5)) & 0xFFFFFFFF)


def plan_redirect(
    name: str,
    loaded_base: int,
    image_size: int,
    wrapper: int,
    wrapper_size: int,
    trampoline: int,
) -> dict:
    """Compute bytes for caller-supplied locations; this is not an install API."""
    if name not in ENTRIES:
        raise ValueError("unsupported observation boundary")
    entry = ENTRIES[name]
    image_range = _range(loaded_base, image_size)
    wrapper_range = _range(wrapper, wrapper_size)
    trampoline_range = _range(trampoline, len(entry.original) + 5)
    ranges = [image_range, wrapper_range, trampoline_range]
    if any(
        max(a[0], b[0]) < min(a[1], b[1])
        for index, a in enumerate(ranges)
        for b in ranges[index + 1 :]
    ):
        raise ValueError("image, wrapper and trampoline ranges must not overlap")
    if entry.rva + len(entry.original) >= image_size:
        raise ValueError("entry outside assigned image")
    resident = bytearray(entry.original)
    for offset in entry.relocation_offsets:
        value = struct.unpack_from("<I", resident, offset)[0]
        struct.pack_into(
            "<I", resident, offset, (value + loaded_base - PREFERRED_BASE) & 0xFFFFFFFF
        )
    site = loaded_base + entry.rva
    redirect = _jump(site, wrapper) + b"\x90" * (len(resident) - 5)
    copied = bytes(resident) + _jump(trampoline + len(resident), site + len(resident))
    return {
        "name": name,
        "site": site,
        "wrapper": wrapper,
        "trampoline": trampoline,
        "expected_resident_bytes": resident.hex(),
        "redirect_bytes": redirect.hex(),
        "trampoline_bytes": copied.hex(),
        "addresses_source": "caller-assigned; not observed",
        "installed": False,
        "live_coverage": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not args.engine.is_absolute() or not args.output.is_absolute():
        parser.error("--engine and --output must be absolute paths")
    try:
        result = verify_image(args.engine.read_bytes())
        with args.output.open("x", encoding="ascii", newline="\n") as stream:
            json.dump(result, stream, indent=2)
            stream.write("\n")
    except (OSError, ValueError) as error:
        parser.exit(1, f"observer_hook_plan: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
