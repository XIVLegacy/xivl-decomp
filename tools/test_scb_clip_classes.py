#!/usr/bin/env python3
"""Tests for the cut scheduler parser, the clip class manifest verifier and the install census."""

from __future__ import annotations

import copy
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import scb_clip_scheduler as scb  # noqa: E402
import verify_scb_clip_classes as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000
ACTORS = ["ProxyActor", "CommonActor"]
CLIPS = [
    "SetPosClip",
    "ShowHideClip",
    "MotionClip",
    "RapturePhysicsLookAtClip",
    "ClipSyncClip",
    "LocalCameraClip",
    "BindActorClip",
    "RaptureNqMotionBlendOnOffClip",
    "RaptureFacialPoseForSchClip",
    "RaptureFacialLipSyncClip",
    "RaptureMesClip",
    "DepthOfFieldClip",
    "KillClip",
]


# @CCPT entry byte +2 per class, in CLIPS order
KEY_BYTES = [1, 1, 2, 0, 1, 4, 0, 1, 0, 0, 3, 3, 1]


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def pad16(data: bytes) -> bytes:
    return data + bytes(-len(data) % 16)


def chunk(tag: bytes, count: int, body: bytes) -> bytes:
    units = (0x10 + len(body) + 15) // 16
    head = bytearray(0x10)
    head[0 : len(tag)] = tag
    struct.pack_into("<H", head, 6, units)
    struct.pack_into("<h", head, 0x0A, count)
    return pad16(bytes(head) + body)


def scheduler(
    blocks: list[list[bytes]],
    actors: list[tuple[int, int]],
    subtype=b"SCB\0",
    key_bytes=KEY_BYTES,
    index_words=True,
) -> bytes:
    """A SEDBSCB span with the directory, string table and four child chunks."""
    names = ACTORS + CLIPS
    deltas = b""
    text = b""
    start = 2 * len(names)
    for name in names:
        deltas += struct.pack("<h", start + len(text))
        text += name.encode() + b"\0"
    strings = bytearray(0x20) + deltas + text
    strings[0x10:0x16] = b"String"
    strings = pad16(bytes(strings))
    names_body = lambda base, n: b"".join(struct.pack("<I", base + i) for i in range(n))  # noqa: E731
    catt = chunk(b"@CATT\0", len(ACTORS), names_body(0, len(ACTORS)))
    ccpt = chunk(
        b"@CCPT\0",
        len(CLIPS),
        b"".join(
            struct.pack("<I", len(ACTORS) + i | key << 16)
            for i, key in enumerate(key_bytes)
        ),
    )
    cact = chunk(
        b"@CACT\0",
        len(actors),
        b"".join(
            struct.pack("<hBB", link, cls, 0) + bytes(link - 4) for link, cls in actors
        ),
    )
    parts = b""
    index = 0
    for clips in blocks:
        if index_words:
            clips = [
                raw
                if struct.unpack_from("<h", raw, 0x0C)[0]
                else raw[:0x0C] + struct.pack("<h", index + k) + raw[0x0E:]
                for k, raw in enumerate(clips)
            ]
        index += len(clips)
        body = bytearray(0x30)
        struct.pack_into("<H", body, 0x18, len(clips))
        raw = b"".join(clips)
        block = bytearray(0x10) + body + raw
        block = bytearray(pad16(bytes(block)))
        struct.pack_into("<H", block, 6, len(block) // 16)
        parts += bytes(block)
    cbkt = chunk(b"@CBKT\0", len(blocks), parts)
    root = bytearray(0x20)
    struct.pack_into("<H", root, 6, 2)
    root[0x1C] = 4
    children = catt + ccpt + cact + cbkt
    directory = bytearray(0x20)
    directory[0:6] = b"@CRST\0"
    struct.pack_into("<H", directory, 6, 2)
    directory[8] = 1
    struct.pack_into("<h", directory, 0x0A, 2)
    struct.pack_into("<II", directory, 0x10, 0, len(root) + len(children))
    head = bytearray(0x30)
    head[0:4] = b"SEDB"
    head[4:8] = subtype
    struct.pack_into("<I", head, 8, 2)
    struct.pack_into("<H", head, 0x0E, 0x30)
    out = bytes(head) + bytes(directory) + bytes(root) + children + strings
    return out[:0x10] + struct.pack("<I", len(out)) + out[0x14:]


def record(cls: int, actor: int, link: int, **fields) -> bytes:
    """A clip record. fields maps 'offset:format' to a value."""
    out = bytearray(link)
    struct.pack_into("<hBB", out, 0, link, cls, actor)
    for key, value in fields.items():
        offset, fmt = key.split("_")
        struct.pack_into("<" + fmt, out, int(offset, 16), value)
    return bytes(out)


def setpos(**over) -> bytes:
    fields = {
        f"{o:x}_B": 1 for o in (0x34, 0x35, 0x36, 0x38, 0x39, 0x3A, 0x3C, 0x3D, 0x3E)
    }
    fields.update({"10_f": -5.5, "14_f": 3.0, "18_f": 100.0})
    fields.update({"28_f": 0.5, "2c_f": 1.0, "30_f": 8.0})
    fields.update(over)
    return record(0, 0, 0x40, **fields)


def showhide(state=1, **over) -> bytes:
    return record(1, 0, 0x14, **{"10_B": state, **over})


def motion(**over) -> bytes:
    return record(
        2,
        0,
        0x2C,
        **{"10_i": 5, "18_i": 20000, "1c_i": 10000, "20_i": 30000, "24_H": 100, **over},
    )


def lookat(mode=0, **over) -> bytes:
    return record(3, 0, 0x34, **{"14_B": 1, "15_B": 1, "16_B": 1, "1c_I": mode, **over})


def clipsync(refs=(1,), all_mode=True, link=None, **over) -> bytes:
    n = len(refs)
    size = link or (0x14 + 2 * n + 3) & ~3
    fields = {"10_H": n | (0x8000 if all_mode else 0)}
    for k, ref in enumerate(refs):
        fields[f"{0x14 + 2 * k:x}_h"] = ref
    fields.update(over)
    return record(4, 0, size, **fields)


def localcamera(ext=False, **over) -> bytes:
    fields = {"1a_B": 3 if ext else 2}
    if ext:
        fields.update({"20_h": 1, "22_h": 40})
    fields.update(over)
    return record(5, 1, 0x40 if ext else 0x20, **fields)


def nqblend(flag=1, value=15, **over) -> bytes:
    return record(7, 0, 0x18, **{"10_I": flag, "14_I": value, **over})


def facialpose(name=b"cfxf_smile", **over) -> bytes:
    out = bytearray(record(8, 0, 0x38, **{"30_i": 2, "34_i": 10, **over}))
    out[0x10:0x30] = bytes(32)
    out[0x10 : 0x10 + len(name)] = name
    return bytes(out)


def lipsync(mode=0, set_word=0, **over) -> bytes:
    return record(9, 0, 0x18, **{"10_I": mode, "14_I": set_word, **over})


def mes(**over) -> bytes:
    fields = {"10_I": 1, "14_I": 22, "18_I": 100, "1c_B": 1, "1d_B": 1, "20_I": 0x1E}
    return record(10, 0, 0x24, **{**fields, **over})


def dof(preset=False, **over) -> bytes:
    if preset:
        fields = {"10_I": 20000, "14_I": 0x7F3, "18_H": 3}
        return record(11, 0, 0x1C, **{**fields, **over})
    return record(11, 0, 0x34, **{"14_I": 0x7F1, "18_f": 2.0, **over})


def kill(refs=(0,), link=None, **over) -> bytes:
    n = len(refs)
    fields = {"10_h": n}
    for k, ref in enumerate(refs):
        fields[f"{0x14 + 2 * k:x}_h"] = ref
    fields.update(over)
    return record(12, 0, link or (0x14 + 2 * n + 3) & ~3, **fields)


def good_blocks() -> list[list[bytes]]:
    return [
        [
            setpos(),
            showhide(),
            motion(),
            lookat(0),
            lookat(1, **{"20_f": 45.0}),
            clipsync(),
        ],
        [
            localcamera(),
            localcamera(ext=True),
            record(6, 1, 0x14),
            clipsync((0, 2), False),
        ],
        [
            nqblend(),
            nqblend(0, 500),
            facialpose(),
            lipsync(2, 1),
            mes(),
            dof(),
            dof(True),
            kill((0, -1)),
        ],
    ]


def run(blocks: list[list[bytes]]) -> dict:
    out = verifier.blank_census()
    data = b"junk" + scheduler(blocks, [(8, 0), (8, 0)]) + b"tail"
    verifier.census_data(data, out)
    return out


def violations(blocks: list[list[bytes]]) -> dict:
    return {key: count for key, count in run(blocks)["violations"].items() if count}


def with_clip(index: int, new: bytes) -> list[list[bytes]]:
    blocks = copy.deepcopy(good_blocks())
    blocks[0][index] = new
    return blocks


def with_block_clip(block: int, index: int, new: bytes) -> list[list[bytes]]:
    blocks = copy.deepcopy(good_blocks())
    blocks[block][index] = new
    return blocks


def check_bytes(item: dict) -> bytes:
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "dwords":
        return b"".join(int(v, 16).to_bytes(4, "little") for v in item["values"])
    pattern, mask = bytes.fromhex(item["pattern"]), bytes.fromhex(item["mask"])
    return bytes(p if m else 0 for p, m in zip(pattern, mask))


def build_image(rows: list[tuple[int, bytes]]) -> bytes:
    """A PE32 image with one section per row, each placed at its own VA."""
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(rows) + 0xFFF) & ~0xFFF
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(rows))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    body = bytearray()
    for index, (va, data) in enumerate(rows):
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII",
            header,
            base + 8,
            len(data),
            va - IMAGE_BASE,
            len(data),
            first_raw + len(body),
        )
        body += data
    return bytes(header) + bytes(body)


def main() -> int:
    sched = scb.parse_scheduler(scheduler(good_blocks(), [(8, 0), (12, 1)]))
    check("actor classes are read", sched.actor_classes == ACTORS)
    check("clip classes are read", sched.clip_classes == CLIPS)
    check(
        "actor records keep their link and class",
        [(a[0], len(a[2])) for a in sched.actors] == [(0, 8), (1, 12)],
    )
    check(
        "all blocks are walked in order",
        [c.class_name for c in sched.clips][:2] == ["SetPosClip", "ShowHideClip"]
        and len(sched.clips) == 18
        and [c.block for c in sched.clips] == [0] * 6 + [1] * 4 + [2] * 8,
    )
    check(
        "clip links are the record sizes",
        [c.link for c in sched.clips]
        == [0x40, 0x14, 0x2C, 0x34, 0x34, 24, 0x20, 0x40, 0x14, 24]
        + [0x18, 0x18, 0x38, 0x18, 0x24, 0x34, 0x1C, 24],
    )
    check("key entry bytes are read", sched.clip_class_bytes == KEY_BYTES)
    check(
        "clip actor and class fields",
        (sched.clips[6].actor, sched.clips[6].cls) == (1, 5),
    )

    data = b"xx" + scheduler(good_blocks(), [(8, 0)]) + b"yy"
    check("scheduler is found past a prefix", scb.find_scheduler_starts(data) == [2])
    check(
        "header count includes every SEDB",
        scb.count_sedb_headers(data + b"SEDBRES ") == 2,
    )
    check(
        "subtype fold skips NUL and space and lowercases",
        all(scb.is_scb_subtype(t) for t in (b"SCB\0", b"scb ", b"Sc b", b"ScB ")),
    )
    check(
        "other subtypes are not scb",
        not scb.is_scb_subtype(b"RES ") and not scb.is_scb_subtype(b"SCBX"),
    )
    check(
        "a scheduler parses with a lowercase subtype",
        scb.parse_scheduler(scheduler([[]], [(8, 0)], b"scb ")).clips == [],
    )

    whole = scheduler(good_blocks(), [(8, 0)])
    for name, broken in (
        ("a truncated span", whole[: len(whole) - 20]),
        ("a missing signature", b"SEDX" + whole[4:]),
        ("a wrong subtype", whole[:4] + b"RES " + whole[8:]),
        ("a bad directory tag", whole[:0x30] + b"@XRST\0" + whole[0x36:]),
    ):
        try:
            scb.parse_scheduler(broken)
            ok = False
        except scb.ParseError:
            ok = True
        check(f"{name} is rejected", ok)
    bad_class = bytearray(scheduler([[record(13, 0, 0x14)]], [(8, 0)]))
    try:
        scb.parse_scheduler(bytes(bad_class))
        rejected = False
    except scb.ParseError:
        rejected = True
    check("a clip class ordinal past the class list is rejected", rejected)

    census = run(good_blocks())
    check(
        "census counts files, headers, schedulers and clips",
        (census["files"], census["sedbHeaders"], census["schedulers"], census["clips"])
        == (1, 1, 1, 18),
    )
    check(
        "census counts clips per class",
        dict(census["classClips"])
        == {
            "SetPosClip": 1,
            "ShowHideClip": 1,
            "MotionClip": 1,
            "RapturePhysicsLookAtClip": 2,
            "ClipSyncClip": 2,
            "LocalCameraClip": 2,
            "BindActorClip": 1,
            "RaptureNqMotionBlendOnOffClip": 2,
            "RaptureFacialPoseForSchClip": 1,
            "RaptureFacialLipSyncClip": 1,
            "RaptureMesClip": 1,
            "DepthOfFieldClip": 2,
            "KillClip": 1,
        },
    )
    check(
        "census counts schedulers once per class",
        dict(census["classSchedulers"]) == {name: 1 for name in CLIPS},
    )
    check(
        "census keeps record sizes",
        dict(census["classLinks"]["ClipSyncClip"]) == {24: 2}
        and dict(census["classLinks"]["LocalCameraClip"]) == {32: 1, 64: 1},
    )
    check(
        "a well formed set has no violations", sum(census["violations"].values()) == 0
    )
    check(
        "lookat angle check only sees mode 1",
        census["checked"]["lookat-angles"] == 1
        and census["checked"]["lookat-link"] == 2,
    )
    check(
        "extension check only sees long cameras",
        census["checked"]["localcamera-ext"] == 1,
    )
    check(
        "census csv rows are ranked by clips then name",
        verifier.census_csv(census) == "rank,class,clips,schedulers,recordBytes\n"
        "1,ClipSyncClip,2,1,24:2\n"
        "2,DepthOfFieldClip,2,1,28:1;52:1\n"
        "3,LocalCameraClip,2,1,32:1;64:1\n"
        "4,RaptureNqMotionBlendOnOffClip,2,1,24:2\n"
        "5,RapturePhysicsLookAtClip,2,1,52:2\n"
        "6,BindActorClip,1,1,20:1\n"
        "7,KillClip,1,1,24:1\n"
        "8,MotionClip,1,1,44:1\n"
        "9,RaptureFacialLipSyncClip,1,1,24:1\n"
        "10,RaptureFacialPoseForSchClip,1,1,56:1\n"
        "11,RaptureMesClip,1,1,36:1\n"
        "12,SetPosClip,1,1,64:1\n"
        "13,ShowHideClip,1,1,20:1\n",
    )

    check(
        "census buckets the distributions",
        {k: dict(v) for k, v in census["distributions"].items()}
        == {
            "showhide-state": {"1": 1},
            "motion-speed": {"100": 1},
            "lookat-mode": {"0": 1, "1": 1},
            "clipsync-count-parity": {"odd": 1, "even": 1},
            "motion-end-vs-start": {"after": 1},
            "motion-end-unit": {"unit": 1},
            "localcamera-drift-bit": {"clear": 2},
            "nqblend-flag": {"0": 1, "1": 1},
            "lipsync-set": {"1": 1},
            "dof-mode": {"parameters": 1, "preset": 1},
            "kill-count": {"2": 1},
            "kill-missing-index": {"minus-one": 1},
            "nqblend-value-15": {"15": 1, "other": 1},
            "lipsync-mode": {"2": 1},
            "dof-flags": {"0x7f1": 1, "0x7f3": 1},
            "mes-route": {"11": 1},
        },
    )
    skewed = copy.deepcopy(good_blocks())
    skewed[1].append(clipsync((0,)))
    check(
        "census counts clipsync count parity",
        dict(run(skewed)["distributions"]["clipsync-count-parity"])
        == {"odd": 2, "even": 1},
    )
    check(
        "census tracks float ranges",
        census["ranges"]
        == {"setpos-position": [-5.5, 100.0], "setpos-scale": [0.5, 8.0]},
    )
    check(
        "census checks the key byte of every clip",
        census["checked"]["localcamera-key-byte"] == 2
        and census["checked"]["lookat-key-byte"] == 2,
    )
    for name in verifier.CLASS_ORDER:
        index = CLIPS.index(name)
        wrong = list(KEY_BYTES)
        wrong[index] ^= 1
        out = verifier.blank_census()
        verifier.census_data(
            scheduler(good_blocks(), [(8, 0), (8, 0)], key_bytes=wrong), out
        )
        key = next(
            k
            for k, v in verifier.INVARIANTS.items()
            if v[0] == name and k.endswith("-key-byte")
        )
        check(
            f"census reports a changed key byte for {name}",
            {k: v for k, v in out["violations"].items() if v}
            == {key: out["classClips"][name]},
        )

    cases = [
        ("setpos flag byte of 2", 0, setpos(**{"3c_B": 2}), {"setpos-flags": 1}),
        ("setpos unread byte set", 0, setpos(**{"3b_B": 1}), {"setpos-unread": 1}),
        ("setpos rotation past pi", 0, setpos(**{"20_f": 3.2}), {"setpos-rotation": 1}),
        (
            "setpos rotation nan",
            0,
            setpos(**{"24_I": 0x7FC00000}),
            {"setpos-rotation": 1},
        ),
        (
            "setpos short record",
            0,
            record(0, 0, 0x3C),
            {"setpos-link": 1, "setpos-flags": 1, "setpos-unread": 1},
        ),
        ("showhide state of 2", 1, showhide(2), {"showhide-state": 1}),
        ("showhide padding", 1, showhide(1, **{"12_B": 1}), {"showhide-pad": 1}),
        (
            "showhide long record",
            1,
            record(1, 0, 0x18, **{"10_B": 1}),
            {"showhide-link": 1},
        ),
        (
            "motion negative name index",
            2,
            motion(**{"10_i": -1}),
            {"motion-name-index": 1},
        ),
        (
            "motion scaled word off the unit",
            2,
            motion(**{"1c_i": 11000}),
            {"motion-time-units": 1},
        ),
        ("motion flag of 2", 2, motion(**{"26_B": 2}), {"motion-flag": 1}),
        ("motion unread byte", 2, motion(**{"27_B": 1}), {"motion-unread": 1}),
        ("lookat mode 3", 3, lookat(3), {"lookat-mode": 1}),
        ("lookat flag of 2", 3, lookat(0, **{"17_B": 2}), {"lookat-flags": 1}),
        (
            "lookat angle of 91 in mode 1",
            4,
            lookat(1, **{"24_f": 91.0}),
            {"lookat-angles": 1},
        ),
        (
            "clipsync link not following the count",
            5,
            clipsync((1,), link=28),
            {"clipsync-link": 1},
        ),
        ("clipsync index past the clip list", 5, clipsync((99,)), {"clipsync-refs": 1}),
        ("clipsync negative index", 5, clipsync((-1,)), {"clipsync-refs": 1}),
        ("clipsync index of itself", 5, clipsync((5,)), {"clipsync-refs": 1}),
        ("clipsync padding", 5, clipsync((1,), **{"12_H": 1}), {"clipsync-pad": 1}),
    ]
    for name, index, new, expected in cases:
        check(f"census reports {name}", violations(with_clip(index, new)) == expected)

    bad_pose = bytearray(facialpose())
    bad_pose[0x2F] = 1
    last = 7
    new_cases = [
        ("nqblend switch of 2", 2, 0, nqblend(2), {"nqblend-flag": 1}),
        ("nqblend value of 501", 2, 0, nqblend(1, 501), {"nqblend-value": 1}),
        (
            "nqblend short record",
            2,
            0,
            record(7, 0, 0x14),
            {"nqblend-link": 1, "nqblend-value": 1},
        ),
        (
            "facial pose name without the prefix",
            2,
            2,
            facialpose(b"cfxl_smile"),
            {"facialpose-name": 1},
        ),
        (
            "facial pose name of 16 characters",
            2,
            2,
            facialpose(b"cfxf_0123456789a"),
            {"facialpose-name": 1},
        ),
        (
            "facial pose bytes after the name",
            2,
            2,
            bytes(bad_pose),
            {"facialpose-name": 1},
        ),
        (
            "facial pose first word of 10",
            2,
            2,
            facialpose(**{"30_i": 10}),
            {"facialpose-words": 1},
        ),
        (
            "facial pose negative second word",
            2,
            2,
            facialpose(**{"34_i": -1}),
            {"facialpose-words": 1},
        ),
        ("lipsync mode of 8", 2, 3, lipsync(8), {"lipsync-mode": 1}),
        ("lipsync set of 3", 2, 3, lipsync(0, 3), {"lipsync-set": 1}),
        ("mes lead word of 2", 2, 4, mes(**{"10_I": 2}), {"mes-lead": 1}),
        ("mes route byte of 2", 2, 4, mes(**{"1c_B": 2}), {"mes-flags": 1}),
        ("mes byte 0x1f set", 2, 4, mes(**{"1f_B": 1}), {"mes-flags": 1}),
        ("mes mask bit 0", 2, 4, mes(**{"20_I": 1}), {"mes-mask": 1}),
        ("mes mask bit 25", 2, 4, mes(**{"20_I": 1 << 25}), {"mes-mask": 1}),
        ("dof flag bit 2", 2, 5, dof(**{"14_I": 0x7F5}), {"dof-flags": 1}),
        (
            "dof preset padding",
            2,
            6,
            dof(True, **{"1a_H": 1}),
            {"dof-preset-pad": 1},
        ),
        (
            "dof preset record of parameter length",
            2,
            6,
            record(11, 0, 0x34, **{"10_I": 20000, "14_I": 0x7F3}),
            {"dof-link": 1},
        ),
        ("dof end before the start", 2, last, dof(**{"04_I": 10000}), {"dof-end": 1}),
        (
            "dof parameter end after the start",
            2,
            5,
            dof(**{"10_I": 10000}),
            {"dof-end": 1},
        ),
        ("kill index of itself", 2, last, kill((17,)), {"kill-refs": 1}),
        ("kill index past the list", 2, last, kill((99,)), {"kill-refs": 1}),
        ("kill index of -2", 2, last, kill((-2,)), {"kill-refs": 1}),
        ("kill link off the count", 2, last, kill((0,), link=28), {"kill-link": 1}),
        ("kill high count word", 2, last, kill((0,), **{"12_h": 1}), {"kill-pad": 1}),
        (
            "kill byte after the indexes",
            2,
            last,
            kill((0,), **{"16_h": 5}),
            {"kill-pad": 1},
        ),
        (
            "kill negative count",
            2,
            last,
            record(12, 0, 0x14, **{"10_h": -1}),
            {"kill-count": 1, "kill-refs": 1},
        ),
        (
            "camera end before the start",
            1,
            3,
            localcamera(**{"04_I": 10000, "10_I": 0}),
            {"localcamera-end": 1},
        ),
        (
            "start tick off the unit",
            0,
            5,
            clipsync(**{"04_I": 10001}),
            {"clip-start-unit": 1},
        ),
        (
            "index word of another clip",
            2,
            2,
            facialpose(**{"0c_h": 7}),
            {"clip-index-word": 1},
        ),
        (
            "start tick lower than the previous clip",
            0,
            5,
            clipsync(**{"04_I": 10000}),
            {"clip-start-order": 1},
        ),
    ]
    for name, block, index, new, expected in new_cases:
        blocks = with_block_clip(block, index, new)
        if name == "start tick lower than the previous clip":
            blocks[0][4] = lookat(1, **{"20_f": 45.0, "04_I": 20000})
        check(f"census reports {name}", violations(blocks) == expected)
    blocks = with_block_clip(0, 5, clipsync(**{"04_I": 50000}))
    check("a start tick may drop at a block boundary", violations(blocks) == {})
    ordered = copy.deepcopy(good_blocks())
    ordered[2] += [
        motion(**{"04_I": 30000, "18_i": 10000}),
        motion(**{"04_I": 30000, "18_i": 30000}),
    ]
    check(
        "census orders the motion end against the start",
        dict(run(ordered)["distributions"]["motion-end-vs-start"])
        == {"after": 1, "before": 1, "equal": 1},
    )
    check(
        "census tracks integer ranges",
        run(good_blocks())["intRanges"]
        == {
            "nqblend-value": [15, 500],
            "facialpose-words": [2, 10],
            "lipsync-mode": [2, 2],
        },
    )
    check(
        "census counts the timing checks over every clip",
        run(good_blocks())["checked"]["clip-start-unit"] == 18
        and run(good_blocks())["checked"]["clip-start-order"] == 15,
    )

    blocks = copy.deepcopy(good_blocks())
    blocks[1][0] = record(5, 1, 0x40, **{"1a_B": 2})
    check(
        "camera long record without the extension bit",
        violations(blocks) == {"localcamera-link": 1},
    )
    blocks[1][0] = record(5, 1, 0x20, **{"1a_B": 3})
    check(
        "camera short record with the extension bit",
        violations(blocks) == {"localcamera-link": 1},
    )
    blocks[1][0] = localcamera(True, **{"20_h": 2})
    check(
        "camera extension actor past the actor list",
        violations(blocks) == {"localcamera-ext": 1},
    )
    blocks[1][0] = localcamera(True, **{"22_h": -1})
    check(
        "camera extension negative part", violations(blocks) == {"localcamera-ext": 1}
    )
    blocks[1][0] = localcamera(False, **{"16_H": 1})
    check("camera padding word", violations(blocks) == {"localcamera-pad": 1})

    manifest = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    csv_text = verifier.CENSUS_CSV.read_text(encoding="ascii")
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    check(
        "outcome tiers are pinned",
        {o["id"]: o["tier"] for o in manifest["outcomes"]}
        == verifier.EXPECTED_OUTCOMES,
    )
    check(
        "every invariant has a manifest entry",
        {i["id"] for i in manifest["invariants"]} == set(verifier.INVARIANTS),
    )
    check(
        "the census csv has one row per class", len(verifier.csv_rows(csv_text)) == 102
    )
    check(
        "the documented classes sit at their ranks in the csv",
        {r["class"]: int(r["rank"]) for r in verifier.csv_rows(csv_text)}[
            "RaptureMesClip"
        ]
        == 11
        and [
            int(r["rank"])
            for r in verifier.csv_rows(csv_text)
            if r["class"] in verifier.CLASS_ORDER
        ]
        == [v[0] for v in verifier.OBJECTS.values()]
        and list(verifier.OBJECTS) == verifier.CLASS_ORDER,
    )

    check(
        "invariant owners are documented classes or every clip",
        {owner for owner, _, _ in verifier.INVARIANTS.values()}
        <= set(verifier.CLASS_ORDER) | {"*"},
    )
    check("the timing section verifies", verifier.timing_errors(manifest) == [])
    for name, path, value in (
        ("the tick rate", ("unit", "ticksPerSecond"), 30000),
        ("the image ticks per frame", ("unit", "ticksPerFrameImageValue"), 10000),
        ("the tick entry", ("scheduler", "tick"), "0x00a1d331"),
        ("the check list", ("checks",), []),
    ):
        broken = copy.deepcopy(manifest)
        node = broken["timing"]
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        check(
            f"a changed timing {name} is reported",
            len(verifier.timing_errors(broken)) >= 1,
        )
    for name, key, value in (
        ("start", "start", "u32(D+8)"),
        ("end", "end", "u32(D+0x1c)"),
        ("end method", "endVa", "0x00a21081"),
    ):
        broken = copy.deepcopy(manifest)
        broken["timing"]["classes"][1][key] = value
        check(
            f"a changed timing class {name} is reported",
            len(verifier.timing_errors(broken)) >= 1,
        )
    broken = copy.deepcopy(manifest)
    broken["timing"]["classes"].reverse()
    check(
        "a reordered timing class list is reported",
        any("rank order" in e for e in verifier.timing_errors(broken)),
    )
    check(
        "every documented class has an end method that matches its object",
        all(
            row["object"]["slot8"]
            == f"0x{verifier.SLOT8.get(row['name'], verifier.BASE_SLOT8):08x}"
            for row in manifest["classes"]
        ),
    )
    late = copy.deepcopy(manifest)
    late["classes"][8]["registration"]["secondInsertSite"] = "0x6366"
    check(
        "a malformed second insert site is reported",
        any("secondInsertSite" in e for e in verifier.class_errors(late)),
    )
    wrong_total = csv_text.replace("1,SetPosClip,33078", "1,SetPosClip,33079", 1)
    check(
        "a csv count change is reported",
        any("clip total" in e for e in verifier.csv_errors(wrong_total, manifest)),
    )
    unsorted = csv_text.replace("2,MotionClip,22085", "2,MotionClip,2208", 1)
    check(
        "a csv ordering change is reported",
        len(verifier.csv_errors(unsorted, manifest)) >= 1,
    )
    shrunk = copy.deepcopy(manifest)
    shrunk["install"]["perClass"]["LocalCameraClip"]["recordBytes"]["64"] -= 1
    check(
        "an inconsistent install block is reported",
        any("recordBytes" in e for e in verifier.install_errors(shrunk["install"])),
    )
    shifted = copy.deepcopy(manifest)
    shifted["classes"][2]["fields"][0]["offset"] = "0x30"
    check(
        "a field outside the record span is reported",
        any("outside the record" in e for e in verifier.class_errors(shifted)),
    )
    overlap = copy.deepcopy(manifest)
    overlap["classes"][2]["fields"][0]["width"] = 4
    overlap["classes"][2]["fields"][0]["offset"] = "0x08"
    overlap["classes"][2]["fields"].append(
        dict(overlap["classes"][2]["fields"][0], field="second")
    )
    check(
        "overlapping fields are reported",
        any("overlap" in e for e in verifier.class_errors(overlap)),
    )

    rows = [(int(i["va"], 16), check_bytes(i)) for i in manifest["staticChecks"]]
    image = build_image(rows)
    check(
        "an image built from the rows passes",
        verifier.verify_image(image, manifest, require_identity=False) == [],
    )
    check("identity is enforced", len(verifier.verify_image(image, manifest)) == 2)
    flipped = bytearray(image)
    flipped[len(image) - 1] ^= 0xFF
    check(
        "a changed byte is reported",
        len(verifier.verify_image(bytes(flipped), manifest, require_identity=False))
        == 1,
    )

    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        path = root / "client" / "cut" / "abc10000" / "abc10000"
        path.parent.mkdir(parents=True)
        path.write_bytes(b"PWIB" + scheduler(good_blocks(), [(8, 0)]))
        (path.parent / ".DS_Store").write_bytes(b"x")
        found = verifier.census_install(root)
        check(
            "install walk skips .DS_Store and counts the file",
            (found["files"], found["schedulers"], found["clips"]) == (1, 1, 18),
        )
        document = {"install": verifier.census_manifest(found)}
        text = verifier.census_csv(found)
        check(
            "install census matches itself",
            verifier.compare_census(found, document, text) == [],
        )
        for section, key, value in (
            ("distributions", "showhide-state", {"1": 2}),
            ("distributions", "lookat-mode", {"0": 1, "1": 1, "2": 1}),
            ("ranges", "setpos-scale", [0.5, 8.1]),
            ("ranges", "setpos-position", [-5.5, 99.9]),
        ):
            drifted = copy.deepcopy(document)
            drifted["install"][section][key] = value
            check(
                f"install census notices a changed {key}",
                any(
                    f"install {section}.{key}" in e
                    for e in verifier.compare_census(found, drifted, text)
                ),
            )
        changed = copy.deepcopy(document)
        changed["install"]["clips"] += 1
        check(
            "install census notices a changed total",
            any(
                "install clips" in e
                for e in verifier.compare_census(found, changed, text)
            ),
        )
        check(
            "install census notices a changed csv",
            any(
                "census csv" in e
                for e in verifier.compare_census(found, document, text + "x")
            ),
        )
        path.write_bytes(b"PWIB" + scheduler(good_blocks(), [(8, 0)])[:-40])
        check(
            "an unparsable scheduler stops the walk",
            verifier.verify_install(root, document)[0].startswith(
                "install cannot be walked"
            ),
        )

    print(f"{len(PASSED)} passed, {len(FAILED)} failed")
    for name in FAILED:
        print(f"FAIL: {name}")
    return 1 if FAILED else 0


if __name__ == "__main__":
    raise SystemExit(main())
