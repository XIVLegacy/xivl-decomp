#!/usr/bin/env python3
"""Validate the music song-id resource contract, optional exe bytes and install table."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
from pathlib import Path

import verify_sqwt_command_parameter_consumer as image_checks


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "music_song_resources.json"
EXPECTED_SHA256 = image_checks.EXPECTED_SHA256
EXPECTED_SIZE = image_checks.EXPECTED_SIZE
RESOURCE_BASE = 0x07FE0000
SONG_ID_LIMIT = 0x10000
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
EMPTY_SHA256 = hashlib.sha256(b"").hexdigest()
REQUIRED_CHECKS = {
    "receiver add site, mode 1",
    "receiver add site, deferred internal mode",
    "numeric mode compare",
    "numeric mode byte-group push order",
    "numeric format string",
    "read service null stream test",
    "read service no-stream branch",
    "read service publishes state 3",
    "read service publishes state 2",
    "first pass state read",
    "second pass state dispatch",
    "second pass read service call",
    "namespace class test",
    "request slot module call",
    "receiver slot 4 cell",
    "receiver packet read",
    "lua wrapper sign extension",
    "request same-id early return",
    "request releases with its duration",
    "request module mode byte test",
    "request slot 4 or 5 call",
    "module mode byte constructor store",
    "dispatcher zeroes ebx",
    "dispatcher passes ebx as mode byte",
    "sound driver null gate",
    "sound driver global getter",
    "requested id stored before the load",
    "existing entry skips the load",
    "new request object and map insert",
    "request returns true",
    "resource module callback state table",
    "state 2 publishes 7 and calls back with success",
    "state 3 publishes 8 and calls back with failure",
    "callback dispatch virtual offset 4",
    "SoundCacheSound table slot 1",
    "callback failure branch",
    "callback failure return",
    "callback length gate",
    "callback empty return",
    "Resource length slot",
    "Resource length getter",
    "callback bank creation",
    "callback invalid-id retry",
    "callback no bank id return",
    "callback no-id tail",
    "invalid bank id constant",
    "driver wrapper dispatch",
    "SoundDriver slot 4 cell",
    "SoundDriver RTTI name",
    "driver global construction",
    "bank magic SEDB compare",
    "bank magic SSCF compare",
    "bank magic bytes",
    "bank magic bytes, second dword",
    "bank version gate",
    "bank creation",
    "bank constructor",
    "bank base constructor",
    "ActualBank table head",
    "bank loader count words",
    "bank entry accessor clamps",
    "bank creator calls the loader",
    "ActualBank loader slot cell",
    "bank loader entry flag loop",
    "reaper skips unstamped entries",
    "time stamp store",
    "command queue when no sound id",
    "release live handle",
    "release pending request",
    "pending cancel match",
    "deferred cell sentinel",
    "mode 4 deferred store",
    "Sleep import name",
    "InterlockedExchange import name",
    "InterlockedExchangeAdd import name",
}
EXPECTED_IMMEDIATE_SITES = [
    0x004BBB82,
    0x004BBBB7,
    0x004BBC18,
    0x004BBC54,
    0x004BBC75,
    0x004BBCB3,
    0x004BBCEA,
    0x004BBD25,
    0x004BBD58,
    0x004BBE3B,
    0x007CF362,
]
FIXED_TEXT = {
    ("formatter", "modeByteStatus"): "INFERRED",
    ("missingFile", "musicSoundObjectReaction"): "STATIC",
    ("policy", "nativeZonePolicy"): "none found",
    (
        "install",
        "scope",
    ): "owner installation of build 2012.09.19.0001, existence and size and SHA-256 only",
}


EXPECTED_OUTCOMES = {
    "driver-absent": "STATIC",
    "requested-id-before-load": "STATIC",
    "previous-song-released-first": "STATIC",
    "same-id-no-retry": "STATIC",
    "failed-load-callback": "STATIC",
    "failed-load-silent": "STATIC",
    "failed-entry-persists": "STATIC",
    "no-log-in-traced-path": "NOT FOUND",
    "empty-file": "STATIC",
    "bank-created-for-streamless": "STATIC",
    "streamless-audible": "UNPROVED",
    "invalid-bank-retry": "STATIC",
    "module-byte-zero": "INFERRED",
    "lua-sign-extension": "STATIC",
    "mode-4-sentinel": "STATIC",
    "bank-loader-at-creation": "STATIC",
    "callback-ready-byte-and-return": "STATIC",
}
TABLE_OFFSET_WORD = 0x0E


def song_path(song_id: int) -> str:
    resource = RESOURCE_BASE + song_id
    return (
        "data/"
        + "/".join(f"{(resource >> shift) & 0xFF:02X}" for shift in (24, 16, 8, 0))
        + ".DAT"
    )


def verify(document: dict | None = None) -> list[str]:
    errors: list[str] = []
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    if document.get("format") != "xivl-music-song-resources-v1":
        errors.append("format changed")
    source = document.get("source", {})
    if (
        source.get("binary") != "ffxivgame.exe retail 1.23b"
        or source.get("build") != "2012.09.19.0001"
        or source.get("size") != EXPECTED_SIZE
        or source.get("binarySha256") != EXPECTED_SHA256
        or source.get("imageBase") != "0x00400000"
    ):
        errors.append("evidence identity changed")
    request = document.get("request", {})
    if (
        request.get("receiverVa") != "0x004bbb30"
        or request.get("resourceBase") != "0x07fe0000"
        or request.get("songIdWidth") != 16
        or request.get("immediateSites")
        != [f"{site:#010x}" for site in EXPECTED_IMMEDIATE_SITES]
    ):
        errors.append("request derivation changed")
    checks = document.get("staticChecks")
    if not isinstance(checks, list):
        errors.append("staticChecks is not a list")
        checks = []
    names = [item.get("name") for item in checks if isinstance(item, dict)]
    if len(names) != len(set(names)):
        errors.append("staticChecks names are not unique")
    missing = REQUIRED_CHECKS - set(names)
    if missing:
        errors.append(f"staticChecks lacks {sorted(missing)}")
    for item in checks:
        if not isinstance(item, dict) or item.get("kind") not in {
            "signature",
            "dwords",
            "string",
        }:
            errors.append("staticChecks entry has an unknown kind")
        elif not re.fullmatch(r"0x[0-9a-f]{8}", str(item.get("va", ""))):
            errors.append(f"staticChecks {item.get('name')!r} has a malformed va")
        elif item["kind"] == "signature":
            pattern, mask = item.get("pattern", ""), item.get("mask", "")
            try:
                same = len(bytes.fromhex(pattern)) == len(bytes.fromhex(mask))
            except ValueError:
                same = False
            if not same:
                errors.append(f"staticChecks {item['name']!r} pattern or mask invalid")
    for path, expected in FIXED_TEXT.items():
        node = document
        for key in path:
            node = node.get(key, {}) if isinstance(node, dict) else {}
        if node != expected:
            errors.append(f"{'.'.join(path)} changed")
    install = document.get("install", {})
    songs = install.get("songs")
    if not isinstance(songs, list):
        return errors + ["install.songs is not a list"]
    ids = [song.get("songId") for song in songs]
    if ids != list(range(len(songs))):
        errors.append("install.songs ids are not contiguous from zero")
    last = len(songs) - 1
    if (
        install.get("firstSongId") != 0
        or install.get("lastSongId") != last
        or install.get("absentSongIds") != f"{last + 1}..{SONG_ID_LIMIT - 1}"
        or install.get("absentCount") != SONG_ID_LIMIT - len(songs)
    ):
        errors.append("install span fields disagree with the table")
    total = 0
    paths = set()
    for song in songs:
        song_id = song.get("songId")
        size, digest = song.get("size"), song.get("sha256")
        if not isinstance(song_id, int) or not isinstance(size, int):
            errors.append("install.songs row is malformed")
            continue
        total += size
        paths.add(song_path(song_id))
        if not isinstance(digest, str) or not SHA_RE.match(digest):
            errors.append(f"song {song_id} sha256 is malformed")
        elif (size == 0) != (digest == EMPTY_SHA256):
            errors.append(f"song {song_id} size and sha256 disagree about emptiness")
        if (size == 0) != (song_id == 0):
            errors.append(f"song {song_id} emptiness is unexpected")
        if not isinstance(song.get("streams"), int):
            errors.append(f"song {song_id} stream count is malformed")
    if len(paths) != len(songs):
        errors.append("install.songs paths are not unique")
    if install.get("nonEmptyCount") != sum(1 for s in songs if s.get("size")):
        errors.append("install.nonEmptyCount disagrees with the table")
    streamless = [
        s.get("songId") for s in songs if s.get("size") and not s.get("streams")
    ]
    if install.get("noStreamSongIds") != streamless:
        errors.append("install.noStreamSongIds disagrees with the table")
    if install.get("totalBytes") != total:
        errors.append("install.totalBytes disagrees with the table")
    outcomes = document.get("downstream", {}).get("outcomes")
    if not isinstance(outcomes, list):
        errors.append("downstream.outcomes is not a list")
    else:
        found = {item.get("id"): item.get("tier") for item in outcomes}
        if len(found) != len(outcomes) or found != EXPECTED_OUTCOMES:
            errors.append("downstream outcome ids or tiers changed")
        if any(not str(item.get("claim", "")).strip() for item in outcomes):
            errors.append("a downstream outcome has no claim")
    table = install.get("noStreamTable", {})
    if (
        table.get("songIds") != streamless
        or table.get("tableOffsetWord") != "0x0e"
        or table.get("tableOffset") != "0x30"
        or table.get("words") != [1, 0, 0]
        or table.get("otherWords") != [1, 1, 1]
    ):
        errors.append("install.noStreamTable changed")
    text = json.dumps(document, sort_keys=True, ensure_ascii=True)
    forbidden = (r"[A-Za-z]" + r":\\\\", "/" + "Users/", "/" + "home/")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        errors.append("manifest contains a machine path")
    return errors


def mutation_test() -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    cases = [
        (("source", "binarySha256"), "0" * 64),
        (("request", "resourceBase"), "0x07fd0000"),
        (("request", "songIdWidth"), 8),
        (("request", "immediateSites", 3), "0x004bbc55"),
        (("formatter", "modeByteStatus"), "STATIC"),
        (("missingFile", "musicSoundObjectReaction"), "silence"),
        (("missingFile", "musicSoundObjectReaction"), "UNPROVED"),
        (("downstream", "outcomes", 0, "tier"), "UNPROVED"),
        (("downstream", "outcomes", 10, "tier"), "STATIC"),
        (("downstream", "outcomes", 5, "id"), "failed-load-audible"),
        (("downstream", "outcomes", 8, "claim"), " "),
        (("install", "noStreamTable", "words"), [1, 1, 1]),
        (("install", "noStreamTable", "songIds"), [7, 30, 31]),
        (("policy", "nativeZonePolicy"), "per zone"),
        (("install", "lastSongId"), 108),
        (("install", "absentCount"), 1),
        (("install", "totalBytes"), 1),
        (("install", "noStreamSongIds"), [7]),
        (("install", "songs", 7, "streams"), 1),
        (("install", "nonEmptyCount"), 110),
        (("install", "songs", 5, "songId"), 6),
        (("install", "songs", 7, "sha256"), "0" * 63),
        (("install", "songs", 0, "size"), 1),
        (("install", "songs", 9, "size"), 0),
        (("staticChecks", 0, "kind"), "other"),
        (("staticChecks", 1, "va"), "0x4bbb80"),
    ]
    mutations = []
    for path, value in cases:
        changed = copy.deepcopy(document)
        node = changed
        for key in path[:-1]:
            node = node[key]
        node[path[-1]] = value
        mutations.append(changed)
    for name in (
        "read service publishes state 3",
        "requested id stored before the load",
        "state 3 publishes 8 and calls back with failure",
        "callback failure return",
        "bank constructor",
        "bank creator calls the loader",
        "ActualBank loader slot cell",
    ):
        dropped = copy.deepcopy(document)
        dropped["staticChecks"] = [
            item for item in dropped["staticChecks"] if item["name"] != name
        ]
        mutations.append(dropped)
    short = copy.deepcopy(document)
    short["downstream"]["outcomes"].pop()
    mutations.append(short)
    shorter = copy.deepcopy(document)
    shorter["install"]["songs"].pop()
    mutations.append(shorter)
    leaked = copy.deepcopy(document)
    leaked["source"]["localPath"] = "/" + "Users/maintainer/install"
    mutations.append(leaked)
    return [
        f"mutation {index} was accepted"
        for index, item in enumerate(mutations, 1)
        if not verify(item)
    ]


def immediate_sites(data: bytes) -> list[int]:
    """VAs of the little-endian 0x07FE0000 immediate inside executable sections."""
    needle = RESOURCE_BASE.to_bytes(4, "little")
    sites = []
    for start, virtual_size, raw_offset, raw_size in image_checks.section_map(data):
        if not (start <= 0x00401000 < start + max(virtual_size, raw_size)):
            continue
        end = raw_offset + raw_size
        at = data.find(needle, raw_offset, end)
        while at != -1:
            sites.append(start + at - raw_offset)
            at = data.find(needle, at + 1, end)
    return sites


def verify_image(
    data: bytes, document: dict, require_identity: bool = True
) -> list[str]:
    errors = image_checks.verify_image(data, document, require_identity)
    if any("not a readable PE32" in error for error in errors):
        return errors
    found = immediate_sites(data)
    expected = [int(site, 16) for site in document["request"]["immediateSites"]]
    if found != expected:
        errors.append(f"0x07FE0000 immediates in .text are {[hex(v) for v in found]}")
    return errors


def verify_executable(path: Path) -> list[str]:
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    return verify_image(path.read_bytes(), document)


def table_words(data: bytes) -> list[int]:
    """The first three u16 words of the table that header u16 +0x0e names."""
    start = int.from_bytes(data[TABLE_OFFSET_WORD : TABLE_OFFSET_WORD + 2], "little")
    return [
        int.from_bytes(data[start + 2 * i : start + 2 * i + 2], "little")
        for i in range(3)
    ]


def census_install(root: Path) -> dict:
    """Existence, size and SHA-256 of every music resource the install holds."""
    songs = []
    for song_id in range(SONG_ID_LIMIT):
        path = root / song_path(song_id)
        if not path.is_file():
            continue
        data = path.read_bytes()
        sscf = data[:8] == b"SEDBSSCF"
        songs.append(
            {
                "songId": song_id,
                "size": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "sscf": sscf,
                "streams": int.from_bytes(data[0x34:0x36], "little") if sscf else 0,
                "words": table_words(data) if sscf else [],
            }
        )
    return {"songs": songs}


NUMERIC_PATH_RE = re.compile(
    r"^data/[0-9A-F]{2}/[0-9A-F]{2}/[0-9A-F]{2}/[0-9A-F]{2}\.DAT$"
)


def tree_counts(root: Path) -> dict:
    """File counts below data: all files, the .DAT ones, and those in the numeric form."""
    dat = other = numeric = 0
    for path in (root / "data").rglob("*"):
        if not path.is_file() or path.name == ".DS_Store":
            continue
        relative = path.relative_to(root).as_posix()
        if relative.endswith(".DAT"):
            dat += 1
            numeric += bool(NUMERIC_PATH_RE.match(relative))
        else:
            other += 1
    return {"datFiles": dat, "otherFiles": other, "numericPathFiles": numeric}


def verify_install(root: Path, document: dict | None = None) -> list[str]:
    if document is None:
        document = json.loads(MANIFEST.read_text(encoding="ascii"))
    errors = []
    counts = tree_counts(root)
    if counts != document["install"]["dataTree"]:
        errors.append(f"data tree counts are {counts}")
    found = census_install(root)["songs"]
    expected = document["install"]["songs"]
    if [song["songId"] for song in found] != [song["songId"] for song in expected]:
        errors.append(
            f"install holds song ids {[s['songId'] for s in found][:3]}.. "
            f"count {len(found)}, manifest lists {len(expected)}"
        )
        return errors
    for have, want in zip(found, expected):
        if have["size"] != want["size"] or have["sha256"] != want["sha256"]:
            errors.append(f"song {want['songId']} size or SHA-256 differs")
        if have["streams"] != want["streams"]:
            errors.append(f"song {want['songId']} stream count differs")
        if have["sscf"] != (want["size"] != 0):
            errors.append(f"song {want['songId']} SSCF magic differs from the table")
    table = document["install"].get("noStreamTable")
    if table:
        for have, want in zip(found, expected):
            if not have["sscf"]:
                continue
            words = table[
                "words" if want["songId"] in table["songIds"] else "otherWords"
            ]
            if have["words"] != words:
                errors.append(f"song {want['songId']} table words are {have['words']}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exe", type=Path, help="pinned retail executable for byte checks"
    )
    parser.add_argument(
        "--install",
        type=Path,
        help="game root whose data tree is compared with the song table",
    )
    parser.add_argument(
        "--print-install",
        type=Path,
        metavar="ROOT",
        help="print the install table of ROOT as JSON rows and exit",
    )
    args = parser.parse_args()
    if args.print_install:
        songs = [
            {key: song[key] for key in ("songId", "size", "streams", "sha256")}
            for song in census_install(args.print_install)["songs"]
        ]
        print(json.dumps(songs, indent=2))
        return 0
    errors = verify() + mutation_test()
    if args.exe:
        errors += verify_executable(args.exe)
    if args.install:
        errors += verify_install(args.install)
    if errors:
        print(f"FAIL: {len(errors)} music song resource error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    suffix = ""
    if args.exe:
        suffix += " plus executable checks"
    if args.install:
        suffix += " plus install table"
    print(f"PASS: music song resource manifest, mutations{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
