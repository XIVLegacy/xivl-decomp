#!/usr/bin/env python3
"""Parse embedded SEDBSCB cut schedulers into actor records and clip records.

The layout is the one recorded in docs/resource/cut-scheduler-clip-layout.md.
Every function works on caller-supplied bytes. Nothing here reads a client
install on its own.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field

SEDB = b"SEDB"
CHUNK_UNIT = 16
MIN_ACTOR_LINK = 4
MIN_CLIP_LINK = 8
BLOCK_FIRST_CLIP = 0x40


class ParseError(ValueError):
    """Raised for bytes that cannot be walked without reading out of range."""


@dataclass
class Clip:
    offset: int
    link: int
    cls: int
    actor: int
    raw: int
    class_name: str
    record: bytes
    block: int = 0


@dataclass
class Scheduler:
    start: int
    size: int
    actor_classes: list[str]
    clip_classes: list[str]
    actors: list[tuple[int, str, bytes]] = field(default_factory=list)
    clips: list[Clip] = field(default_factory=list)
    clip_class_bytes: list[int] = field(default_factory=list)


def is_scb_subtype(raw: bytes) -> bool:
    """The generic reader's fold: skip NUL and space, lowercase, compare to scb."""
    kept = bytes(
        b | 0x20 if 0x41 <= b <= 0x5A else b for b in raw if b not in (0, 0x20)
    )
    return kept == b"scb"


def count_sedb_headers(data: bytes) -> int:
    total = 0
    at = data.find(SEDB)
    while at >= 0:
        total += 1
        at = data.find(SEDB, at + 1)
    return total


def find_scheduler_starts(data: bytes) -> list[int]:
    """Offsets of every SEDB signature whose subtype folds to scb."""
    found = []
    at = data.find(SEDB)
    while at >= 0:
        if is_scb_subtype(data[at + 4 : at + 8]):
            found.append(at)
        at = data.find(SEDB, at + 1)
    return found


def _need(data: bytes, offset: int, width: int, what: str) -> None:
    if offset < 0 or offset + width > len(data):
        raise ParseError(f"{what} at {offset:#x} is outside the scheduler")


def _read(fmt: str, data: bytes, offset: int, what: str) -> int:
    _need(data, offset, struct.calcsize(fmt), what)
    return struct.unpack_from(fmt, data, offset)[0]


def _cstr(data: bytes, offset: int, what: str) -> str:
    _need(data, offset, 1, what)
    end = data.find(b"\0", offset)
    if end < 0:
        raise ParseError(f"{what} at {offset:#x} has no terminator")
    return data[offset:end].decode("ascii", "replace")


def _names(data: bytes, chunk: int, table: int, what: str) -> list[str]:
    count = _read("<h", data, chunk + 0x0A, what)
    out = []
    for k in range(count):
        index = _read("<H", data, chunk + 0x10 + 4 * k, what)
        delta = _read("<h", data, table + 2 * index, what)
        out.append(_cstr(data, table + delta, what))
    return out


def _entry_bytes(data: bytes, chunk: int, what: str) -> list[int]:
    """Byte +2 of each four byte name entry, which the client keeps at key entry +4."""
    count = _read("<h", data, chunk + 0x0A, what)
    return [_read("<B", data, chunk + 0x10 + 4 * k + 2, what) for k in range(count)]


def parse_scheduler(span: bytes, start: int = 0) -> Scheduler:
    """Parse the scheduler at span[0:]. The span may run past the scheduler."""
    if span[:4] != SEDB or not is_scb_subtype(span[4:8]):
        raise ParseError("not a SEDB scb header")
    size = _read("<I", span, 0x10, "declared size")
    if size > len(span):
        raise ParseError(f"declared size {size:#x} runs past the input")
    data = span[:size]
    if (
        _read("<I", data, 0x08, "version") < 2
        or _read("<B", data, 0x0C, "byte 0x0C") != 0
    ):
        raise ParseError("header version or byte 0x0C check failed")
    q = _read("<H", data, 0x0E, "directory offset")
    if data[q : q + 6] != b"@CRST\0" or _read("<B", data, q + 8, "crst byte 8") != 1:
        raise ParseError("no @CRST directory")
    count = _read("<h", data, q + 0x0A, "directory count")
    if count < 1:
        raise ParseError("empty directory")
    end = q + CHUNK_UNIT * _read("<H", data, q + 6, "crst size")
    keys = [
        end + _read("<I", data, q + 0x10 + 4 * j, "directory entry")
        for j in range(count)
    ]
    table = next(
        (k + 0x20 for k in keys if data[k + 0x10 : k + 0x17] == b"String\0"), None
    )
    if table is None:
        raise ParseError("no String resource")
    root = keys[0]
    children = _read("<B", data, root + 0x1C, "root child count")
    if children > 127:
        raise ParseError("negative root child count")
    child = root + CHUNK_UNIT * _read("<H", data, root + 6, "root size")
    found: dict[bytes, int] = {}
    for _ in range(children):
        _need(data, child, 8, "child chunk")
        found.setdefault(bytes(data[child : child + 6]), child)
        step = CHUNK_UNIT * _read("<H", data, child + 6, "child size")
        if step == 0:
            raise ParseError("zero-length child chunk")
        child += step
    for tag in (b"@CATT\0", b"@CCPT\0", b"@CACT\0", b"@CBKT\0"):
        if tag not in found:
            raise ParseError(f"missing {tag[:5].decode()} chunk")
    sched = Scheduler(
        start,
        size,
        _names(data, found[b"@CATT\0"], table, "@CATT"),
        _names(data, found[b"@CCPT\0"], table, "@CCPT"),
    )
    sched.clip_class_bytes = _entry_bytes(data, found[b"@CCPT\0"], "@CCPT")
    cact = found[b"@CACT\0"]
    at = cact + 0x10
    for _ in range(_read("<h", data, cact + 0x0A, "@CACT count")):
        link = _read("<h", data, at, "actor link")
        cls = _read("<B", data, at + 2, "actor class")
        if link < MIN_ACTOR_LINK or at + link > size or cls >= len(sched.actor_classes):
            raise ParseError(f"bad actor record at {at:#x}")
        sched.actors.append(
            (cls, sched.actor_classes[cls], bytes(data[at : at + link]))
        )
        at += link
    cbkt = found[b"@CBKT\0"]
    header = cbkt + 0x10
    for block_index in range(_read("<h", data, cbkt + 0x0A, "@CBKT count")):
        block = CHUNK_UNIT * _read("<H", data, header + 6, "block size")
        if block < BLOCK_FIRST_CLIP or header + block > size:
            raise ParseError(f"bad block at {header:#x}")
        record = header + BLOCK_FIRST_CLIP
        for _ in range(_read("<H", data, header + 0x10 + 0x18, "clip count")):
            link = _read("<h", data, record, "clip link")
            cls = _read("<B", data, record + 2, "clip class")
            if (
                link < MIN_CLIP_LINK
                or record + link > size
                or cls >= len(sched.clip_classes)
            ):
                raise ParseError(f"bad clip record at {record:#x}")
            sched.clips.append(
                Clip(
                    start + record,
                    link,
                    cls,
                    _read("<B", data, record + 3, "clip actor"),
                    _read("<I", data, record + 4, "clip raw"),
                    sched.clip_classes[cls],
                    bytes(data[record : record + link]),
                    block_index,
                )
            )
            record += link
        header += block
    return sched
