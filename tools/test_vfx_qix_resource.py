#!/usr/bin/env python3
"""Tests for the Qix effect resource parser, manifest verifier and install census."""

from __future__ import annotations

import copy
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_vfx_qix_resource as verifier  # noqa: E402
import vfx_qix_resource as qix  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def name16(text: str) -> bytes:
    return text.encode("ascii").ljust(16, b"\0")


def blob(tag: str, mode: int = 2, name: str = "node01", refs: list | None = None):
    """A synthetic SEDB blob with one group, one block and one pointer cell."""
    root = 0x40
    name_at = {"veff": 4, "vins": 0x20, "leaf": 0x20, "vmdl": 0, "vtex": 0}[tag]
    tag_at = {"veff": 0x14, "vins": 0x30, "leaf": 0x30, "vmdl": 0x10, "vtex": 0x10}[tag]
    record = bytearray(0x40)
    record[name_at : name_at + 16] = name16(name)
    record[tag_at : tag_at + 4] = tag[::-1].encode()
    struct.pack_into("<I", record, 0x38, 0x20)
    tail = b""
    if tag == "veff" and refs is not None:
        for number, (ref, ref_tag) in enumerate(refs):
            tail += name16(ref) + ref_tag[::-1].encode() + bytes(4)
        struct.pack_into("<II", record, 0x20, 0x80, len(refs))
        struct.pack_into("<II", record, 0x28, 0x80, 0)
    payload = bytearray(0x40)
    struct.pack_into("<I", payload, 0x04, 0)
    struct.pack_into("<HH", payload, 0x08, 1, 1)
    payload[0x0C] = 4
    payload[0x0E] = mode
    struct.pack_into("<II", payload, 0x10, 0x1C, 0x18)
    struct.pack_into("<I", payload, 0x18, root)
    if mode == 2:
        struct.pack_into("<IIiH", payload, 0x1C, 0x2C, 0x3C, 1, 1)
        struct.pack_into("<IiH", payload, 0x2C, root, 1, 0x40)
        struct.pack_into("<I", payload, 0x3C, 0x38)
    else:
        struct.pack_into("<iIHHI", payload, 0x1C, 1, root, 0x40, 1, 0x2C)
        struct.pack_into("<I", payload, 0x2C, 0x38)
    body = bytearray(payload) + record + tail
    total = 0x30 + len(body)
    struct.pack_into("<I", body, 4, total)
    head = bytearray(0x30)
    head[0:4] = b"SEDB"
    head[4:8] = tag.encode()
    head[0x0D] = 4
    struct.pack_into("<HI", head, 0x0E, 0x30, total)
    return bytes(head) + bytes(body)


def res(entries: list, names: list, header: int = 0x40) -> bytes:
    """A SEDB RES container. entries are (kind, bytes) pairs."""
    count = len(entries)
    directory = bytearray()
    body = bytearray()
    for index, (kind, data) in enumerate(entries):
        directory += struct.pack("<IIII", index, len(body), len(data), kind)
        body += data
    name_at = len(body)
    body += b"".join(name + b"\0" for name in names)
    total = 0x40 + 16 * count + len(body)
    head = bytearray(0x40)
    head[0:8] = b"SEDBRES "
    head[0x0D] = 4
    struct.pack_into("<HI", head, 0x0E, header, total)
    struct.pack_into("<IIII", head, 0x30, count, name_at, count, 0)
    head[0x3C:0x40] = b"nib\0"
    return bytes(head) + bytes(directory) + bytes(body)


def pwib(inner: bytes) -> bytes:
    total = 16 + len(inner)
    return b"PWIB" + struct.pack(">III", total, 16, total) + inner


def bank(mode: int = 2) -> bytes:
    """A PWIB bank: one nested RES holding a vmdl, a vtex, a veff and a leaf."""
    parts = [
        ("vmdl", "modelA", b"bin\\modelA"),
        ("vtex", "texA", b"bin\\texA"),
        ("veff", "effA", b"bin\\effA"),
        ("leaf", "leafA", b"bin\\leafA"),
    ]
    blobs = []
    for tag, name, _ in parts:
        if tag == "veff":
            blobs.append(
                (4, blob(tag, mode, name, [("modelA", "vmdl"), ("texA", "vtex")]))
            )
        else:
            blobs.append((4, blob(tag, mode, name)))
    inner = res(blobs, [path for _, _, path in parts])
    return pwib(res([(4, inner)], [b"bin\\pack"]))


def census(files: dict[str, bytes]) -> dict:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        for relative, data in files.items():
            path = root / "client" / "vfx" / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        return verifier.census_install(root)


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
    good = bank()
    walk = qix.walk_bank(good)
    check("PWIB wrapper detected", walk["wrapper"] == "PWIB")
    check(
        "two containers are walked", [c["depth"] for c in walk["containers"]] == [0, 1]
    )
    check(
        "four Qix blobs are found in entry order",
        [b["tag"] for b in walk["blobs"]] == ["vmdl", "vtex", "veff", "leaf"],
    )
    info = qix.parse_blob(good, walk["blobs"][2])
    check(
        "payload header fields",
        (info["headerSize"], info["groupCount"], info["rootCount"], info["mode"])
        == (0x30, 1, 1, 2)
        and (info["groupTable"], info["rootTable"], info["byte0C"]) == (0x1C, 0x18, 4),
    )
    check("relocation directory is clean", qix.relocation_errors(good, info) == [])
    root = qix.root_record(good, info)
    check(
        "veff root name sits at root plus 4",
        qix.cstring(good, root + 4, 16) == b"effA"
        and good[root + 0x14 : root + 0x18] == b"ffev",
    )
    for mode in (0, 1):
        other = bank(mode)
        found = qix.walk_bank(other)
        errors = [
            qix.relocation_errors(other, qix.parse_blob(other, b))
            for b in found["blobs"]
        ]
        check(f"mode {mode} relocation directory is clean", errors == [[]] * 4)
    check(
        "an unknown mode has no routine",
        qix.relocation_errors(good, {**info, "mode": 7})
        == ["mode 7 has no relocation routine"],
    )

    def corrupt(offset: int, value: int, fmt: str = "<I") -> list[str]:
        data = bytearray(good)
        struct.pack_into(fmt, data, info["payload"] + offset, value)
        again = qix.parse_blob(bytes(data), walk["blobs"][2])
        return qix.relocation_errors(bytes(data), again)

    check(
        "a pointer cell past the payload is rejected",
        any("past the payload" in e for e in corrupt(0x40 + 0x38, 0x4000)),
    )
    check(
        "a pointer cell at the payload end is accepted",
        corrupt(0x40 + 0x38, info["payloadSpan"]) == [],
    )
    check(
        "a root offset outside the payload is rejected",
        any("root 0" in e for e in corrupt(0x18, 0x9000)),
    )
    check(
        "a block that starts outside the payload is rejected",
        any("block" in e for e in corrupt(0x2C, 0x9000)),
    )
    check(
        "a slot beyond the stride is rejected",
        any("stride" in e for e in corrupt(0x3C, 0x3E)),
    )
    check(
        "a group table outside the payload is rejected",
        qix.relocation_errors(good, {**info, "groupTable": 0x8000})
        == ["group table is outside the payload"],
    )
    check(
        "a negative block count is rejected",
        any("block list" in e for e in corrupt(0x24, 0xFFFFFFFF)),
    )
    leaf_info = qix.parse_blob(good, walk["blobs"][3])
    stretched = bytearray(good)
    struct.pack_into("<H", stretched, leaf_info["payload"] + 0x34, 0x48)
    stretched_info = qix.parse_blob(bytes(stretched), walk["blobs"][3])
    check(
        "a last record that overhangs the payload end is accepted and measured",
        qix.relocation_errors(bytes(stretched), stretched_info) == []
        and qix.block_overruns(bytes(stretched), stretched_info) == [8]
        and qix.block_overruns(good, leaf_info) == [],
    )
    check(
        "names must be terminated ASCII with zero padding",
        qix.cstring(name16("ab"), 0, 16) == b"ab"
        and qix.cstring(b"abcdefghijklmnop", 0, 16) is None
        and qix.cstring(b"ab\0x" + bytes(12), 0, 16) is None
        and qix.cstring(b"\0" * 16, 0, 16) is None
        and qix.cstring(b"a\xffb" + bytes(13), 0, 16) is None,
    )
    for label, data, fragment in (
        (
            "a PWIB with first past second",
            b"PWIB" + struct.pack(">III", 64, 40, 20) + bytes(64),
            "boundaries",
        ),
        ("a bank that is not a RES", pwib(b"SEDBtxb\0" + bytes(0x40)), "no SEDB RES"),
    ):
        try:
            qix.walk_bank(data)
            check(label + " is rejected", False)
        except qix.ParseError as exc:
            check(label + " is rejected", fragment in str(exc))
    plain = res([(1, blob("vtex", name="texP"))], [b"bin\\texP"], header=0x30)
    plain_walk = qix.walk_bank(plain)
    check(
        "an unwrapped bank with a 0x30 RES header is walked",
        plain_walk["wrapper"] == "plain"
        and plain_walk["containers"][0]["declaredHeader"] == 0x30
        and [b["kind"] for b in plain_walk["blobs"]] == [1],
    )
    wrong = bytearray(good)
    wrong[walk["blobs"][0]["start"] + 4 : walk["blobs"][0]["start"] + 8] = b"vtex"
    try:
        qix.parse_blob(bytes(wrong), walk["blobs"][0])
        check("a blob tag that differs from its entry is rejected", False)
    except qix.ParseError:
        check("a blob tag that differs from its entry is rejected", True)

    counts = census({"abl/0001": good})
    check(
        "census counts the synthetic bank",
        counts["banks"]
        == {
            "files": 1,
            "bytes": len(good),
            "pwib": 1,
            "plain": 0,
            "pwibFirst16": 1,
            "pwibTotalIsLength": 1,
        }
        and counts["containers"]["depth0"] == 1
        and counts["containers"]["depth1"] == 1
        and counts["containers"]["typeNameBin"] == 2
        and counts["blobs"]["veff"]["count"] == 1
        and counts["blobs"]["vmdl"]["mode2"] == 1
        and counts["blobs"]["leaf"]["groupCountMax"] == 1,
    )
    check(
        "census resolves both veff references",
        counts["veffReferences"]["entries"] == 2
        and counts["veffReferences"]["resolved"] == 2
        and counts["veffReferences"]["arrayAModelTags"] == 1
        and counts["veffReferences"]["arrayBEntries"] == 0
        and counts["veffReferences"]["trailingZero"] == 2
        and counts["packs"] == 1
        and counts["veffPerPack"] == {"1": 1}
        and counts["veffReferences"]["emptyArrays"] == 1,
    )
    check(
        "census checks the name table last component",
        counts["blobs"]["veff"]["nameTableLastComponent"]
        == {"equal": 1, "different": 0}
        and counts["blobs"]["vmdl"]["rootNameValid"] == 1
        and counts["blobs"]["vtex"]["rootTagMatches"] == 1,
    )
    mode0 = census({"abl/0001": bank(0)})
    check(
        "census separates mode 0 from mode 2",
        mode0["blobs"]["leaf"]["mode0"] == 1 and mode0["blobs"]["leaf"]["mode2"] == 0,
    )
    inner = res(
        [
            (4, blob("vmdl", name="modelA")),
            (0, b"xetv" + bytes(28)),
            (1, b"SEDBSCB\0" + bytes(24)),
        ],
        [b"bin\\modelA", b"x", b"y"],
    )
    extra = pwib(
        res(
            [(4, inner), (0, bytes([1, 2, 3, 4]) + bytes(8)), (0, b"main" + bytes(10))],
            [b"bin\\pack", b"a", b"b"],
        )
    )
    shaped = census({"abl/0002": extra})["nonSedb"]
    check(
        "census counts directory entries that do not start with SEDB",
        shaped["all"]
        == {
            "count": 3,
            "entryBytes": 58,
            "entryBytesMin": 12,
            "entryBytesMax": 32,
            "depth0": 2,
            "depth1": 1,
            "depth2": 0,
            "kind0": 3,
            "kind1": 0,
            "leadPrintableAscii": 2,
            "leadOther": 1,
            "extendsPastBank": 0,
        },
    )
    check(
        "census isolates the xetv led entries",
        shaped["leadXetv"]["count"] == 1
        and shaped["leadXetv"]["entryBytes"] == 32
        and shaped["leadXetv"]["depth1"] == 1
        and shaped["leadXetv"]["leadOther"] == 0,
    )
    check(
        "census records other SEDB subtypes with bytes and depth",
        census({"abl/0002": extra})["otherSubtypes"]
        == {
            "SCB": {
                "count": 1,
                "entryBytes": 32,
                "depth0": 0,
                "depth1": 1,
                "depth2": 0,
            }
        },
    )
    short = bytearray(res([(0, b"main" + bytes(12))], [b"x"]))
    walked = qix.walk_bank(bytes(short))["unwrapped"][0]
    struct.pack_into("<I", short, 0x40 + 8, 0x7000)
    stretched_entry = qix.walk_bank(bytes(short))["unwrapped"][0]
    check(
        "an entry that runs past the bank is flagged",
        walked["pastEnd"] is False and stretched_entry["pastEnd"] is True,
    )
    no_root = bytearray(good)
    struct.pack_into("<H", no_root, walk["blobs"][0]["start"] + 0x30 + 0x0A, 0)
    try:
        qix.root_record(
            bytes(no_root), qix.parse_blob(bytes(no_root), walk["blobs"][0])
        )
        check("a blob with no root cell is rejected", False)
    except qix.ParseError as exc:
        check("a blob with no root cell is rejected", "no root cell" in str(exc))
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "client" / "vfx" / "abl" / "0003"
        path.parent.mkdir(parents=True)
        path.write_bytes(bytes(no_root))
        found = verifier.verify_install(Path(directory), {"install": counts})
        check(
            "an install holding a blob with no root cell fails closed",
            len(found) == 1 and "cannot be walked" in found[0],
        )
    broken = bytearray(good)
    broken[
        walk["blobs"][1]["start"] + 0x30 + 0x40 + 0x38 : walk["blobs"][1]["start"]
        + 0x30
        + 0x40
        + 0x3C
    ] = struct.pack("<I", 0x7000)
    bad = census({"abl/0001": bytes(broken)})
    check(
        "census counts a blob whose relocation directory fails",
        bad["blobs"]["vtex"]["relocationValid"] == 0
        and bad["blobs"]["vmdl"]["relocationValid"] == 1,
    )
    document = {"install": counts}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "client" / "vfx" / "abl" / "0001"
        path.parent.mkdir(parents=True)
        path.write_bytes(good)
        check(
            "install census matches itself",
            verifier.verify_install(Path(directory), document) == [],
        )
        path.write_bytes(good + b"\0")
        check(
            "install census notices a changed byte count",
            any(
                "banks.bytes" in e
                for e in verifier.verify_install(Path(directory), document)
            ),
        )

    manifest = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    check(
        "outcome tiers are pinned",
        {o["id"]: o["tier"] for o in manifest["outcomes"]}
        == verifier.EXPECTED_OUTCOMES,
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
    shrunk = copy.deepcopy(manifest)
    shrunk["install"]["blobs"]["leaf"]["kind4"] -= 1
    check(
        "an inconsistent install block is reported",
        any("kind counts" in e for e in verifier.install_errors(shrunk["install"])),
    )

    print(f"{len(PASSED)} passed, {len(FAILED)} failed")
    for name in FAILED:
        print(f"FAIL: {name}")
    return 1 if FAILED else 0


if __name__ == "__main__":
    raise SystemExit(main())
