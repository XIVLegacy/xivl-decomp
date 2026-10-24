#!/usr/bin/env python3
"""Parse Qix effect resource banks and their SEDB payload blobs.

The layouts are the ones recorded in docs/resource/vfx-qix-resource-container.md
and docs/resource/vfx-effect-family.md. Every function works on caller-supplied
bytes. Nothing here reads a client install on its own.
"""

from __future__ import annotations

import struct

QIX_TAGS = ("veff", "vins", "leaf", "vmdl", "vtex")
SEDB = b"SEDB"
RES_TAG = b"RES "
RES_HEADER = 0x40
RES_ENTRY = 16
BLOB_HEADER_SIZE = 0x30
PAYLOAD_MODE_BLOCKS = 2
REF_STRIDE = 0x18
NAME_SIZE = 16

# Root record layout per tag: where the 16-byte resource name starts.
ROOT_NAME_OFFSET = {"veff": 4, "vins": 0x20, "leaf": 0x20, "vmdl": 0, "vtex": 0}


class ParseError(ValueError):
    """Raised for bytes that cannot be walked without reading out of range."""


def u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise ParseError(f"u16 at {offset:#x} is outside the input")
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise ParseError(f"u32 at {offset:#x} is outside the input")
    return struct.unpack_from("<I", data, offset)[0]


def parse_res(data: bytes, start: int, end: int) -> dict:
    """Directory of the SEDB RES container spanning data[start:end]."""
    if data[start : start + 4] != SEDB or data[start + 4 : start + 8] != RES_TAG:
        raise ParseError(f"no SEDB RES at {start:#x}")
    declared = u16(data, start + 0x0E)
    if declared not in (0x30, RES_HEADER):
        raise ParseError(f"RES header size {declared:#x} is not 0x30 or 0x40")
    count = u32(data, start + 0x30)
    if u32(data, start + 0x38) != count:
        raise ParseError("RES entry count words disagree")
    base = start + RES_HEADER + RES_ENTRY * count
    if base > end:
        raise ParseError("RES directory runs past its container")
    entries = []
    for index in range(count):
        at = start + RES_HEADER + RES_ENTRY * index
        number, offset, size, kind = struct.unpack_from("<IIII", data, at)
        entries.append(
            {
                "index": number,
                "start": base + offset,
                "size": size,
                "kind": kind,
            }
        )
    return {
        "start": start,
        "end": end,
        "declaredHeader": declared,
        "base": base,
        "nameTable": base + u32(data, start + 0x34),
        "nameCount": u32(data, start + 0x38),
        "typeName": bytes(data[start + 0x3C : start + 0x40]),
        "entries": entries,
    }


def bank_root(data: bytes) -> tuple[str, int]:
    """Wrapper kind and the offset of the bank's SEDB RES container."""
    if data[:4] == b"PWIB":
        total = struct.unpack_from(">I", data, 4)[0]
        first = struct.unpack_from(">I", data, 8)[0]
        second = struct.unpack_from(">I", data, 12)[0]
        if not 16 <= first <= second <= total <= len(data):
            raise ParseError("PWIB boundaries are out of order")
        return "PWIB", first
    return "plain", 0


def walk_bank(data: bytes) -> dict:
    """Walk one bank. Returns its wrapper kind, RES containers and Qix blobs.

    Other SEDB subtypes are listed per subtype in other. Entries that do not
    start with SEDB are listed in unwrapped.
    """
    wrapper, root = bank_root(data)
    containers = []
    blobs = []
    other = {}
    unwrapped = []

    def visit(start: int, end: int, depth: int) -> None:
        res = parse_res(data, start, end)
        res["depth"] = depth
        containers.append(res)
        for entry in res["entries"]:
            at, size = entry["start"], entry["size"]
            if data[at : at + 4] != SEDB:
                unwrapped.append(
                    {
                        "start": at,
                        "entrySize": size,
                        "kind": entry["kind"],
                        "depth": depth,
                        "lead": bytes(data[at : at + 4]),
                        "pastEnd": at + size > len(data),
                    }
                )
                continue
            subtype = bytes(data[at + 4 : at + 8])
            if subtype == RES_TAG:
                visit(at, min(at + size, len(data)), depth + 1)
                continue
            text = subtype.rstrip(b"\0 ").decode("latin-1")
            if text in QIX_TAGS:
                blobs.append(
                    {
                        "tag": text,
                        "start": at,
                        "entrySize": size,
                        "kind": entry["kind"],
                        "depth": depth,
                        "container": len(containers) - 1,
                    }
                )
            else:
                other.setdefault(text, []).append(
                    {"entrySize": size, "depth": depth, "kind": entry["kind"]}
                )

    visit(root, len(data), 0)
    return {
        "wrapper": wrapper,
        "containers": containers,
        "blobs": blobs,
        "other": other,
        "unwrapped": unwrapped,
    }


def parse_blob(data: bytes, blob: dict) -> dict:
    """SEDB header, payload header, relocation directory and root records of one blob."""
    at, tag = blob["start"], blob["tag"]
    if data[at : at + 4] != SEDB or data[at + 4 : at + 8].rstrip(b"\0") != tag.encode():
        raise ParseError("blob signature or tag differs from its directory entry")
    header = u16(data, at + 0x0E)
    declared = u32(data, at + 0x10)
    payload = at + header
    limit = min(at + blob["entrySize"], len(data)) - payload
    info = {
        "tag": tag,
        "unknown08": u32(data, at + 0x08),
        "endianByte": data[at + 0x0C],
        "flagsByte": data[at + 0x0D],
        "headerSize": header,
        "declaredSize": declared,
        "entrySize": blob["entrySize"],
        "payload": payload,
        "payloadSpan": limit,
    }
    if limit < 0x18:
        raise ParseError("payload is shorter than its fixed header")
    info.update(
        {
            "word00": u32(data, payload),
            "word04": u32(data, payload + 4),
            "groupCount": u16(data, payload + 8),
            "rootCount": u16(data, payload + 0x0A),
            "byte0C": data[payload + 0x0C],
            "relocatedFlag": data[payload + 0x0D] & 1,
            "mode": data[payload + 0x0E],
            "byte0F": data[payload + 0x0F],
            "groupTable": u32(data, payload + 0x10),
            "rootTable": u32(data, payload + 0x14),
        }
    )
    return info


def relocation_errors(data: bytes, info: dict) -> list[str]:
    """Check the relocation directory against the payload extent.

    Offsets are payload-relative in the file. A pointer cell is zero or an
    offset up to and including the payload end. Returns every failure found.
    """
    errors = []
    base, span = info["payload"], info["payloadSpan"]
    mode = info["mode"]

    def inside(offset: int, length: int) -> bool:
        return 0 <= offset and length >= 0 and offset + length <= span

    if mode not in (0, 1, PAYLOAD_MODE_BLOCKS):
        return [f"mode {mode} has no relocation routine"]
    groups = info["groupCount"]
    table = info["groupTable"]
    if not inside(table, 16 * groups):
        return ["group table is outside the payload"]
    roots = info["rootCount"]
    if not inside(info["rootTable"], 4 * roots):
        errors.append("root table is outside the payload")
    else:
        for number in range(roots):
            value = u32(data, base + info["rootTable"] + 4 * number)
            if not inside(value, 1):
                errors.append(f"root {number} offset {value:#x} is outside the payload")

    def slot_errors(label: str, slots: int, count: int, step: int) -> list | None:
        if not inside(slots, step * count):
            errors.append(f"{label} slot list is outside the payload")
            return None
        width = "<H" if step == 2 else "<I"
        return [
            struct.unpack_from(width, data, base + slots + step * n)[0] & 0xFFFF
            for n in range(count)
        ]

    def cells(label: str, start: int, count: int, stride: int, offsets: list) -> None:
        reach = max(offsets, default=-4) + 4
        if count < 0 or start > span or (count and not inside(start, 0)):
            errors.append(f"{label} is outside the payload")
        elif count and not inside(start + (count - 1) * stride, reach):
            errors.append(f"{label} cells are outside the payload")
        elif count and any(slot + 4 > stride for slot in offsets):
            errors.append(f"{label} slot exceeds its stride")
        else:
            for record in range(count):
                for slot in offsets:
                    value = u32(data, base + start + record * stride + slot)
                    if value and not inside(value, 0):
                        errors.append(f"{label} pointer {value:#x} is past the payload")

    for group in range(groups):
        at = base + table + 16 * group
        if mode == PAYLOAD_MODE_BLOCKS:
            blocks, slots, block_count, slot_count = struct.unpack_from(
                "<IIiH", data, at
            )
            if block_count < 0 or not inside(blocks, 12 * block_count):
                errors.append(f"group {group} block list is outside the payload")
                continue
            offsets = slot_errors(f"group {group}", slots, slot_count, 4)
            if offsets is None:
                continue
            for number in range(block_count):
                start, count, stride = struct.unpack_from(
                    "<IiH", data, base + blocks + 12 * number
                )
                cells(f"group {group} block {number}", start, count, stride, offsets)
        else:
            count, start, stride, slot_count, slots = struct.unpack_from(
                "<iIHHI", data, at
            )
            offsets = slot_errors(
                f"group {group}", slots, slot_count, 2 if mode == 0 else 4
            )
            if offsets is not None:
                cells(f"group {group}", start, count, stride, offsets)
    return errors


def root_record(data: bytes, info: dict) -> int:
    """File offset of root record 0. A zero root count raises ParseError."""
    if info["rootCount"] == 0:
        raise ParseError("blob has no root cell")
    base = info["payload"]
    value = u32(data, base + info["rootTable"])
    return base + value


def cstring(data: bytes, offset: int, size: int) -> bytes | None:
    """The NUL-terminated ASCII text in data[offset:offset+size], else None."""
    field = data[offset : offset + size]
    if len(field) != size or b"\0" not in field:
        return None
    text = field[: field.index(b"\0")]
    if not text or any(not 0x20 <= byte < 0x7F for byte in text):
        return None
    if any(byte for byte in field[len(text) :]):
        return None
    return bytes(text)


REVERSED_TAG = {
    "veff": b"ffev",
    "vins": b"sniv",
    "leaf": b"fael",
    "vmdl": b"ldmv",
    "vtex": b"xetv",
}
# Offset of the reversed tag dword inside root record 0, per tag.
ROOT_TAG_OFFSET = {"veff": 0x14, "vins": 0x30, "leaf": 0x30, "vmdl": 0x10, "vtex": 0x10}
# (pointer offset, count offset, reference tag) inside veff root record 0.
VEFF_REF_ARRAYS = ((0x20, 0x24, "vmdl"), (0x28, 0x2C, "vtex"))


def name_table(data: bytes, res: dict) -> list[bytes]:
    """The NUL-terminated strings of a RES name table, in entry order."""
    names = []
    at = res["nameTable"]
    for _ in range(res["nameCount"]):
        end = data.find(b"\0", at)
        if end < 0:
            raise ParseError("name table string runs past the input")
        names.append(bytes(data[at:end]))
        at = end + 1
    return names


def block_overruns(data: bytes, info: dict) -> list[int]:
    """Bytes by which each mode 2 block extent passes the payload end."""
    if info["mode"] != PAYLOAD_MODE_BLOCKS:
        return []
    base, span = info["payload"], info["payloadSpan"]
    found = []
    for group in range(info["groupCount"]):
        blocks, _, count, _ = struct.unpack_from(
            "<IIiH", data, base + info["groupTable"] + 16 * group
        )
        for number in range(max(count, 0)):
            start, items, stride = struct.unpack_from(
                "<IiH", data, base + blocks + 12 * number
            )
            if items > 0 and start + items * stride > span:
                found.append(start + items * stride - span)
    return found
