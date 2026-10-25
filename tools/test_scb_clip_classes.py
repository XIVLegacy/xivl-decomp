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
]


# @CCPT entry byte +2 per class, in CLIPS order
KEY_BYTES = [1, 1, 2, 0, 1, 4, 0]


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
    for clips in blocks:
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
        "both blocks are walked in order",
        [c.class_name for c in sched.clips][:2] == ["SetPosClip", "ShowHideClip"]
        and len(sched.clips) == 10,
    )
    check(
        "clip links are the record sizes",
        [c.link for c in sched.clips]
        == [0x40, 0x14, 0x2C, 0x34, 0x34, 24, 0x20, 0x40, 0x14, 24],
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
    bad_class = bytearray(scheduler([[record(9, 0, 0x14)]], [(8, 0)]))
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
        == (1, 1, 1, 10),
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
        "2,LocalCameraClip,2,1,32:1;64:1\n"
        "3,RapturePhysicsLookAtClip,2,1,52:2\n"
        "4,BindActorClip,1,1,20:1\n"
        "5,MotionClip,1,1,44:1\n"
        "6,SetPosClip,1,1,64:1\n"
        "7,ShowHideClip,1,1,20:1\n",
    )

    check(
        "census buckets the distributions",
        {k: dict(v) for k, v in census["distributions"].items()}
        == {
            "showhide-state": {"1": 1},
            "motion-speed": {"100": 1},
            "lookat-mode": {"0": 1, "1": 1},
            "clipsync-count-parity": {"odd": 1, "even": 1},
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
    for index, name in enumerate(CLIPS[:6]):
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
        ("clipsync index past the clip list", 5, clipsync((10,)), {"clipsync-refs": 1}),
        ("clipsync negative index", 5, clipsync((-1,)), {"clipsync-refs": 1}),
        ("clipsync index of itself", 5, clipsync((5,)), {"clipsync-refs": 1}),
        ("clipsync padding", 5, clipsync((1,), **{"12_H": 1}), {"clipsync-pad": 1}),
    ]
    for name, index, new, expected in cases:
        check(f"census reports {name}", violations(with_clip(index, new)) == expected)

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
        "the six documented ranks lead the csv",
        [r["class"] for r in verifier.csv_rows(csv_text)[:6]] == verifier.CLASS_ORDER,
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
            (found["files"], found["schedulers"], found["clips"]) == (1, 1, 10),
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
