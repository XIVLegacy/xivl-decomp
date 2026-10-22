#!/usr/bin/env python3
"""Tests for the music song-id resource verifier."""

from __future__ import annotations

import copy
import hashlib
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_music_song_resources as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
SITE = 0x00401004


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def build_image(text: bytes) -> bytes:
    """A one-section PE32 image with raw .text at 0x401000."""
    header = bytearray(0x200)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, 1)
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    base = 0x80 + 24 + 0xE0
    header[base : base + 5] = b".text"
    struct.pack_into("<IIII", header, base + 8, len(text), 0x1000, len(text), 0x200)
    return bytes(header) + text


def fixture() -> tuple[bytes, dict]:
    text = bytes.fromhex("90 90 90 90 05 00 00 FE 07 C3")
    document = {
        "request": {"immediateSites": [f"{SITE + 1:#010x}"]},
        "staticChecks": [
            {
                "name": "add",
                "kind": "signature",
                "va": f"{SITE:#010x}",
                "pattern": "05 00 00 fe 07",
                "mask": "ff ff ff ff ff",
            }
        ],
    }
    return build_image(text), document


def real_manifest() -> dict:
    return json.loads(verifier.MANIFEST.read_text(encoding="ascii"))


def run(image: bytes, document: dict) -> list[str]:
    return verifier.verify_image(image, document, require_identity=False)


def put(root: Path, song_id: int, data: bytes) -> None:
    path = root / verifier.song_path(song_id)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def sscf(streams: int, tracks: int | None = None) -> bytes:
    body = bytearray(0x60)
    body[0:8] = b"SEDBSSCF"
    struct.pack_into("<H", body, 0x0E, 0x30)
    struct.pack_into(
        "<HHH", body, 0x30, 1, streams if tracks is None else tracks, streams
    )
    return bytes(body)


def install_document(root: Path) -> dict:
    rows = verifier.census_install(root)["songs"]
    return {
        "install": {
            "songs": [
                {k: row[k] for k in ("songId", "size", "streams", "sha256")}
                for row in rows
            ],
            "dataTree": verifier.tree_counts(root),
            "noStreamTable": {
                "songIds": [
                    row["songId"] for row in rows if row["size"] and not row["streams"]
                ],
                "words": [1, 0, 0],
                "otherWords": [1, 1, 1],
            },
        }
    }


def main() -> int:
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    check(
        "outcome tiers are pinned",
        {o["id"]: o["tier"] for o in real_manifest()["downstream"]["outcomes"]}
        == verifier.EXPECTED_OUTCOMES,
    )
    check(
        "song path formatting",
        verifier.song_path(13) == "data/07/FE/00/0D.DAT"
        and verifier.song_path(0xFFFF) == "data/07/FE/FF/FF.DAT"
        and verifier.song_path(109) == "data/07/FE/00/6D.DAT",
    )
    real = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    rows = {row["songId"]: row for row in real["install"]["songs"]}
    check(
        "the retained 13 and 57 identities agree with the tracked page",
        rows[13]["size"] == 3_244_271
        and rows[13]["sha256"]
        == "1bfec9b74401244af9c1a3cd32b3f0a72729643b1695d0181916550a582ca4b6"
        and rows[57]["size"] == 2_803_090
        and rows[57]["sha256"]
        == "3d36a533f009c5c4ad8dc3bb2ae8aab2414fb9ffc4da11c68abfcc21c72ca817",
    )

    image, document = fixture()
    check("matching image passes", run(image, document) == [])
    flipped = bytearray(image)
    flipped[0x200 + 5] ^= 1
    check(
        "a changed instruction byte is reported",
        len(run(bytes(flipped), document)) >= 1,
    )
    unlisted = build_image(bytes.fromhex("90 90 90 90 05 00 00 FE 07 C3 00 00 FE 07"))
    check("an unlisted immediate is reported", len(run(unlisted, document)) == 1)
    check("identity is enforced", len(verifier.verify_image(image, document)) >= 2)

    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)
        put(root, 0, b"")
        put(root, 1, sscf(1) + b"audio")
        put(root, 2, sscf(0))
        document = install_document(root)
        check(
            "install table matches itself",
            verifier.verify_install(root, document) == [],
        )
        check(
            "a stream-less container is counted without streams",
            [row["streams"] for row in document["install"]["songs"]] == [0, 1, 0],
        )
        put(root, 1, sscf(1) + b"audiO")
        check(
            "a changed file is reported",
            len(verifier.verify_install(root, document)) == 1,
        )
        put(root, 1, sscf(1) + b"audio")
        put(root, 3, sscf(1))
        check(
            "an extra song is reported",
            len(verifier.verify_install(root, document)) >= 1,
        )
        (root / verifier.song_path(3)).unlink()
        (root / "data" / "07" / "table.txt").write_text("x")
        check(
            "a non-DAT file is reported",
            len(verifier.verify_install(root, document)) == 1,
        )
        wrong = copy.deepcopy(document)
        wrong["install"]["songs"][2]["streams"] = 1
        (root / "data" / "07" / "table.txt").unlink()
        check(
            "a stream count difference is reported",
            len(verifier.verify_install(root, wrong)) == 1,
        )
        words = copy.deepcopy(document)
        words["install"]["noStreamTable"]["otherWords"] = [1, 1, 0]
        check(
            "a table word difference alone is reported",
            len(verifier.verify_install(root, words)) == 1,
        )
        check(
            "the stream-less table words come from the header offset",
            [row["words"] for row in verifier.census_install(root)["songs"]]
            == [[], [1, 1, 1], [1, 0, 0]],
        )
        digest = hashlib.sha256(sscf(0)).hexdigest()
        check(
            "the table digest is a sha256",
            document["install"]["songs"][2]["sha256"] == digest,
        )

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} music song resource tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
