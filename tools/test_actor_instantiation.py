#!/usr/bin/env python3
"""Tests for the actor instantiation verifier and its reference models."""

from __future__ import annotations

import copy
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import census_actor_instantiation as census  # noqa: E402
import verify_actor_instantiation as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000
SELF_ID = 0x029B2941


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def raises(function, *args) -> bool:
    try:
        function(*args)
    except verifier.RecordError:
        return True
    return False


def word(value: int, tag: int = 0) -> bytes:
    return bytes([tag]) + value.to_bytes(4, "big")


def text(value: str) -> bytes:
    return b"\x02" + value.encode("latin-1") + b"\0"


def integer(value: int) -> bytes:
    return b"\x00" + struct.pack(">i", value)


def single(value: float) -> bytes:
    return b"\x01" + struct.pack(">f", value)


TRUE, FALSE, NIL, END = b"\x03", b"\x04", b"\x05", b"\x0f"


def stream(
    path: str,
    values: bytes,
    a: bool = False,
    bundle: bytes | None = None,
    c: int | None = None,
    d: int | None = None,
    e: int | None = None,
    self_words: list[int] | None = None,
) -> bytes:
    """Writes the reads of 0x00774ad0 in order, then pads to the 192-byte span."""
    out = text(path) + (TRUE if a else FALSE)
    out += TRUE + bundle if bundle is not None else FALSE
    out += TRUE + word(c) if c is not None else FALSE
    out += TRUE + word(d) if d is not None else FALSE
    out += TRUE + word(e) if e is not None else FALSE
    if self_words is not None:
        out += b"".join(word(item) for item in self_words)
    out += values + END
    return out.ljust(verifier.STREAM_BYTES, b"\0")


def application(name: str, cls: str, body: bytes, first: int = 0x3D) -> bytes:
    def field(value: str) -> bytes:
        return value.encode("latin-1").ljust(verifier.TEXT_BYTES, b"\0")

    return (
        first.to_bytes(4, "big") + field(name) + field(cls) + body + b"\0\0\0\0"
    ).ljust(verifier.APPLICATION_BYTES, b"\0")


def bundle_bytes() -> bytes:
    return text("prv0Inn01") + text("") + word(0xFFFFFFFF) + b"\x00\x00" + FALSE + TRUE


def candidate(key: int, cls: str, path: str = "") -> dict:
    return {"key": key, "cls": cls, "path": path or f"/Chara/{cls}"}


def parse_cases() -> None:
    npc = application(
        "pplStd_a",
        "PopulaceStandard",
        stream(
            "/Chara/Npc/Populace/PopulaceStandard",
            integer(1200335) + FALSE + FALSE + integer(0) + integer(1) + text("TEST"),
        ),
    )
    got = verifier.parse_application(npc, False)
    check(
        "an ordinary record yields the value list and no controls",
        got["values"] == [1200335, False, False, 0, 1, "TEST"]
        and got["path"] == "/Chara/Npc/Populace/PopulaceStandard"
        and got["name"] == "pplStd_a"
        and got["class"] == "PopulaceStandard"
        and got["self"] is None
        and got["bundle"] is None,
    )
    check(
        "the first application dword is kept apart from the arguments",
        got["first"] == (0x3D).to_bytes(4, "big"),
    )

    words = [1351666115] + [0] * 15 + [1356861720, 0, 1356867785, 0]
    player = application(
        "_pc43723073",
        "Player",
        stream("/Chara/Player/Player_work", TRUE, d=0, self_words=words),
    )
    got = verifier.parse_application(player, True)
    check(
        "the self block is read before the values and is not an argument",
        got["self"] == words and got["values"] == [True],
    )
    check(
        "a zero word D clears control D",
        got["controlD"] is False and got["wordD"] == 0,
    )
    got = verifier.parse_application(
        application("p", "Player", stream("/p", TRUE, d=4, self_words=words)), True
    )
    check("a nonzero word D keeps control D", got["controlD"] is True)
    check(
        "a record that is not the self player reads no self block",
        verifier.parse_application(
            application("p", "Player", stream("/p", TRUE)), False
        )["values"]
        == [True],
    )
    wrong = application("p", "Player", stream("/p", TRUE, self_words=words))
    try:
        shifted = verifier.parse_application(wrong, False)["values"] != [True]
    except verifier.RecordError:
        shifted = True
    check("a self block read as values changes the argument list", shifted)

    debug = verifier.parse_application(
        application(
            "debug", "Debug", stream("/System/Debug.prog", TRUE + TRUE, e=50497)
        ),
        False,
    )
    check(
        "control E consumes a word that is not an argument",
        debug["wordE"] == 50497 and debug["values"] == [True, True],
    )
    area = verifier.parse_application(
        application(
            "_areaMaster@0F400",
            "ZoneMasterPrvI0",
            stream(
                "/Area/Zone/ZoneMasterPrvI0",
                TRUE + FALSE + FALSE,
                bundle=bundle_bytes(),
            ),
        ),
        False,
    )
    check(
        "control B reads the setup bundle ahead of the values",
        area["bundle"]["texts"] == ["prv0Inn01", ""]
        and area["bundle"]["word"] == 0xFFFFFFFF
        and area["bundle"]["bools"] == [False, True]
        and area["values"] == [True, False, False],
    )
    both = verifier.parse_application(
        application("x", "X", stream("/x", TRUE, c=7, d=3, e=9)), False
    )
    check(
        "controls C, D and E each read their own word in order",
        both["wordC"] == 7 and both["wordD"] == 3 and both["wordE"] == 9,
    )
    mixed = verifier.parse_application(
        application(
            "x",
            "X",
            stream(
                "/x",
                integer(-7)
                + single(1.5)
                + NIL
                + text("abc")
                + b"\x06"
                + (5).to_bytes(4, "big")
                + b"\x0c\x01"
                + b"\x0b"
                + bytes(8)
                + b"\x0a"
                + bytes(9)
                + integer(2),
            ),
        ),
        False,
    )
    check(
        "value tags decode as signed integer, float, nil, string and actor",
        mixed["values"] == [-7, 1.5, None, "abc", ("actor", 5), 2],
    )
    for tag in (0x07, 0x08, 0x09):
        check(
            f"tag {tag:#04x} is not decoded by the model",
            raises(
                verifier.parse_application,
                application("x", "X", stream("/x", bytes([tag]) + bytes(8))),
                False,
            ),
        )
    for tag in (0x0D, 0x0E, 0x10, 0x7F, 0x80, 0xFF):
        got = verifier.parse_application(
            application("x", "X", stream("/x", integer(1) + bytes([tag]) + integer(2))),
            False,
        )
        check(
            f"tag {tag:#04x} stops the values without raising or a value",
            got["values"] == [1] and not got["ended"] and not got["tailClear"],
        )
    ended = verifier.parse_application(
        application("x", "X", stream("/x", integer(1))), False
    )
    check("the END tag ends the values with a clear tail", ended["tailClear"])
    head = text("/x") + FALSE * 5
    tail = integer(1) + b"\x02\xff\0"
    got = verifier.parse_stream(
        head + FALSE * (verifier.STREAM_BYTES - len(head) - len(tail)) + tail, False
    )
    check(
        "an exhausted length stops the values",
        got["values"][-2:] == [1, "\xff"] and not got["ended"],
    )
    unterminated = application("x", "X", b"\x02/x\0" + FALSE * 5 + integer(1))
    check(
        "a stream with no END or 0xff byte is refused",
        raises(verifier.parse_application, unterminated, False),
    )
    nearly = application("x", "X", text("/x") + FALSE * 5 + integer(1) + b"\x0e")
    check(
        "a final byte of 0x0e is refused",
        raises(verifier.parse_application, nearly, False)
        and not verifier.terminator_ok(verifier.split_application(nearly)[3]),
    )
    check(
        "a stream of zero bytes is refused",
        raises(verifier.parse_application, application("x", "X", b""), False),
    )
    escape = application("x", "X", text("/x") + FALSE * 5 + integer(1) + b"\xff")
    check(
        "an 0xff final byte passes the guard",
        verifier.terminator_ok(escape[verifier.STREAM_OFFSET :][:192]),
    )
    check(
        "a short application is rejected",
        raises(verifier.parse_application, bytes(100), False),
    )
    check(
        "text fields stop at the first NUL",
        verifier.split_application(application("ab", "cd", stream("/x", TRUE)))[1:3]
        == ("ab", "cd"),
    )


def order_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    group = [
        candidate(SELF_ID, "Player", "/Chara/Player/Player_work"),
        candidate(0x5FF80002, "Debug", "/System/Debug.prog"),
        candidate(0x5FF80001, "WorldMaster", "/World/WorldMaster_event"),
        candidate(
            0x47A00011, "AfterQuestWarpDirector", "/Director/AfterQuestWarpDirector"
        ),
        candidate(0x47A00001, "ZoneMasterPrvI0", "/Area/Zone/ZoneMasterPrvI0"),
        candidate(0x47A0000C, "ObjectBed", "/Chara/Npc/Object/ObjectBed"),
        candidate(0x47A00006, "PopulaceCutScenePlayer", "/Chara/Npc/P"),
    ]
    order = verifier.creation_order(document, group, SELF_ID)
    check(
        "the login group binds Debug, WorldMaster, the player, the area, then the rest",
        order
        == [
            0x5FF80002,
            0x5FF80001,
            SELF_ID,
            0x47A00001,
            verifier.LOCAL_DESKTOP_KEY,
            0x47A00011,
            0x47A0000C,
            0x47A00006,
        ],
    )
    waits = dict(verifier.creation_waits(document, group, SELF_ID))
    check(
        "each bind below state 10 waits for the previous instance to finish init",
        waits[0x5FF80002] is None
        and waits[0x5FF80001] == 0x5FF80002
        and waits[SELF_ID] == 0x5FF80001
        and waits[0x47A00001] == SELF_ID
        and waits[verifier.LOCAL_DESKTOP_KEY] == 0x47A00001
        and waits[0x47A00011] == verifier.LOCAL_DESKTOP_KEY,
    )
    check(
        "binds made in state 10 wait for nothing",
        waits[0x47A0000C] is None and waits[0x47A00006] is None,
    )
    check(
        "a privileged class waits for the player to finish init",
        dict(
            verifier.creation_waits(
                document,
                group + [candidate(7, "CommandDebuggerGM")],
                SELF_ID,
                gm=True,
            )
        )[7]
        == SELF_ID,
    )
    check(
        "the player arrives first and binds third",
        group[0]["key"] == SELF_ID and order.index(SELF_ID) == 2,
    )
    check(
        "worldMaster binds before the player",
        order.index(0x5FF80001) < order.index(SELF_ID),
    )
    check(
        "without a Debug record nothing binds",
        verifier.creation_order(
            document, [c for c in group if c["cls"] != "Debug"], SELF_ID
        )
        == [],
    )
    check(
        "without a WorldMaster record only Debug binds",
        verifier.creation_order(
            document, [c for c in group if c["cls"] != "WorldMaster"], SELF_ID
        )
        == [0x5FF80002],
    )
    check(
        "without an area record the queue stops after the player",
        verifier.creation_order(
            document, [c for c in group if c["cls"] != "ZoneMasterPrvI0"], SELF_ID
        )
        == [0x5FF80002, 0x5FF80001, SELF_ID],
    )
    extra = group + [
        candidate(7, "CommandDebuggerGM"),
        candidate(8, "CommandDebuggerDEV"),
        candidate(9, "CommandDebuggerTEST"),
    ]
    gm = verifier.creation_order(document, extra, SELF_ID, gm=True)
    check(
        "the privileged classes bind between the player and the area when flagged",
        gm[2:7] == [SELF_ID, 7, 8, 9, 0x47A00001],
    )
    check(
        "the privileged classes wait for state 10 when no flag is set",
        verifier.creation_order(document, extra, SELF_ID).index(7)
        > verifier.creation_order(document, extra, SELF_ID).index(
            verifier.LOCAL_DESKTOP_KEY
        ),
    )
    dev = verifier.creation_order(document, extra, SELF_ID, dev=True)
    check(
        "flag 0x175 alone still expects the DEV and TEST classes",
        dev[2:6] == [SELF_ID, 8, 9, 0x47A00001],
    )
    decoy = [candidate(0x77, "Decoy", "/AreaMaster/Decoy")] + group
    check(
        "a path that only starts with /Area is not the area record",
        verifier.creation_order(document, decoy, SELF_ID)[3:5]
        == [0x47A00001, verifier.LOCAL_DESKTOP_KEY],
    )
    other = [candidate(0x1234, "Player", "/Chara/Player/Player_work")] + group[1:]
    check(
        "a Player record that is not the self id is not admitted by state 2",
        verifier.creation_order(document, other, SELF_ID)[:2]
        == [0x5FF80002, 0x5FF80001]
        and 0x1234 not in verifier.creation_order(document, other, SELF_ID)[2:3],
    )
    document2 = copy.deepcopy(document)
    document2["queue"]["states"][1]["next"] = 3
    check(
        "the order follows the manifest transitions",
        verifier.creation_order(document2, group, SELF_ID)
        != verifier.creation_order(document, group, SELF_ID),
    )


def census_cases() -> None:
    words = [1351666115] + [0] * 19
    area_stream = stream("/Area/Zone/Z", TRUE + FALSE + FALSE, bundle=bundle_bytes())
    apps = [
        (
            SELF_ID,
            SELF_ID,
            application("_pc1", "Player", stream("/p", TRUE, d=4, self_words=words)),
        ),
        (7, SELF_ID, application("_pc2", "Player", stream("/p", TRUE))),
        (8, SELF_ID, application("debug", "Debug", stream("/d", TRUE + TRUE, e=1))),
        (9, SELF_ID, application("_areaMaster", "Zone", area_stream)),
        (10, SELF_ID, application("n", "Npc", stream("/n", integer(1)))),
        (11, SELF_ID, application("debug", "Debug", stream("/d", TRUE, e=2))),
    ]
    records = [
        {
            "capture": "a.pcapng" if index < 3 else "b.pcapng",
            "sha256": "0" * 64,
            "source": src,
            "destination": dst,
            "application": app,
        }
        for index, (src, dst, app) in enumerate(apps)
    ]
    found = census.summarize(records)
    check(
        "the census counts records and self records",
        found["records"] == 6
        and found["selfRecords"] == 1
        and found["parsedToEnd"] == 6
        and [row["records"] for row in found["captures"]] == [3, 3]
        and [row["selfRecords"] for row in found["captures"]] == [1, 0],
    )
    check(
        "the census keys the control patterns by which controls are true",
        found["controlPatterns"]
        == {
            "4,4,4,3,4": 1,
            "4,4,4,4,4": 2,
            "4,4,4,4,3": 2,
            "4,3,4,4,4": 1,
        },
    )
    check(
        "a census equal to the manifest numbers passes",
        census.compare(found, found) == [],
    )
    shifted = copy.deepcopy(found)
    shifted["controlPatterns"]["4,4,4,4,4"] += 1
    check(
        "a census that differs is reported",
        len(census.compare(found, shifted)) == 1,
    )
    fewer = copy.deepcopy(found)
    fewer["captures"][0]["records"] -= 1
    check(
        "a per-capture count that differs is reported",
        census.compare(found, fewer) == ["census captures differs from the manifest"],
    )
    body = bytearray(stream("/x", integer(1)))
    body[body.index(END) + 1] = 0x07
    body[-1] = END[0]
    loose = application("x", "X", stream("/x", integer(1)))
    tail = application("x", "X", bytes(body))
    trailing = census.summarize(
        [
            {
                "capture": "c.pcapng",
                "sha256": "0" * 64,
                "source": 1,
                "destination": 2,
                "application": app,
            }
            for app in (loose, tail)
        ]
    )
    check(
        "a byte left after END is not counted as parsed to the end",
        trailing["records"] == 2 and trailing["parsedToEnd"] == 1,
    )
    foreign = copy.deepcopy(records)
    foreign[1]["destination"] = foreign[1]["source"]
    check(
        "a record whose source equals its destination is read as the self record",
        raises(census.summarize, foreign),
    )


def image_bytes(rows: list[tuple[int, bytes]]) -> bytes:
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
        raw_offset = first_raw + len(body)
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII", header, base + 8, len(data), va - IMAGE_BASE, len(data), raw_offset
        )
        body += data
    return bytes(header) + bytes(body)


def row_bytes(item: dict) -> bytes:
    if item["kind"] == "string":
        return item["text"].encode("ascii") + b"\0"
    if item["kind"] == "dwords":
        return b"".join(
            int(value, 16).to_bytes(item.get("width", 4), "little")
            for value in item["values"]
        )
    return bytes.fromhex(item["pattern"])


def image_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    rows = [(int(i["va"], 16), row_bytes(i)) for i in document["staticChecks"]]
    image = image_bytes(rows)
    check(
        "an image built from the rows passes",
        verifier.image_checks.verify_image(image, document, require_identity=False)
        == [],
    )
    check(
        "identity is enforced",
        len(verifier.image_checks.verify_image(image, document)) >= 2,
    )
    first_raw = (0x80 + 24 + 0xE0 + 40 * len(rows) + 0xFFF) & ~0xFFF
    missed = []
    offset = first_raw
    for index, (_, data) in enumerate(rows):
        flipped = bytearray(image)
        flipped[offset] ^= 0x01
        if not verifier.image_checks.verify_image(
            bytes(flipped), document, require_identity=False
        ):
            missed.append(document["staticChecks"][index]["name"])
        offset += len(data)
    check(f"every row reports a flipped byte (missed {missed})", not missed)
    moved = image_bytes([(va + 1, data) for va, data in rows])
    check(
        "rows at shifted addresses are reported",
        len(verifier.image_checks.verify_image(moved, document, require_identity=False))
        >= 1,
    )


def manifest_cases() -> None:
    check("the manifest verifies", verifier.verify() == [])
    check("every manifest mutation is rejected", verifier.mutation_test() == [])
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    broken = copy.deepcopy(document)
    broken["staticChecks"][0]["kind"] = "bytes"
    check("an unknown row kind is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    for item in broken["staticChecks"]:
        if item["name"] == "queue state 2":
            item["va"] = "0x00766558"
    check(
        "a state row away from its table entry is rejected",
        verifier.verify(broken) != [],
    )
    broken = copy.deepcopy(document)
    broken["observed"]["captures"][0]["sha256"] = "z" * 64
    check("a malformed capture hash is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["observed"]["controlPatterns"]["4,4,4,4,4"] = 792
    check(
        "a control census that misses the total is rejected",
        verifier.verify(broken) != [],
    )
    check(
        "every self record in the census sits in the Player class",
        sum(row["selfRecords"] for row in document["observed"]["classes"])
        == document["observed"]["selfRecords"],
    )


def main() -> int:
    parse_cases()
    order_cases()
    census_cases()
    image_cases()
    manifest_cases()
    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} actor instantiation tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
