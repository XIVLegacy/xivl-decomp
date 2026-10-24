#!/usr/bin/env python3
"""Tests for the sound-effect trigger verifier."""

from __future__ import annotations

import copy
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_sound_effect_triggers as verifier  # noqa: E402


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


def image_fixture() -> tuple[bytes, dict]:
    """Four NOPs, an ADD EAX with the interface global, then a 16-byte row table."""
    text = bytes.fromhex("90 90 90 90 05 AC C2 35 01 C3") + struct.pack(
        "<4I", 7, 0, 0, 0
    )
    document = {
        "immediateScans": [{"value": "0x0135c2ac", "sites": [f"{SITE + 1:#010x}"]}],
        "creationVoice": {
            "listMenu": {
                "rowTableVa": f"{0x0040100A:#010x}",
                "rowStride": 4,
                "rowGroupDwords": [7, 0, 0, 0],
            }
        },
        "staticChecks": [
            {
                "name": "add",
                "kind": "signature",
                "va": f"{SITE:#010x}",
                "pattern": "05 ac c2 35 01",
                "mask": "ff ff ff ff ff",
            }
        ],
    }
    return build_image(text), document


def run(image: bytes, document: dict) -> list[str]:
    return verifier.verify_image(image, document, require_identity=False)


def real_manifest() -> dict:
    return json.loads(verifier.MANIFEST.read_text(encoding="ascii"))


def put(root: Path, resource: int, data: bytes) -> None:
    path = root / verifier.resource_path(resource)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


XML = (
    chr(0xFEFF) + "<Root><Style>"
    '<EventTrigger RoutedEvent="MouseEnter"><SoundPlayerAction FileNumber="3" />'
    "</EventTrigger>"
    '<CommandTrigger Command="UILuaCommands.SoundPlayMyOperate">'
    '<SoundPlayerAction FileNumber="4" Volume="100" /></CommandTrigger>'
    '<CommandTrigger Command="ApplicationCommands.Operate">'
    '<SoundPlayerAction FileNumber="4" Priority="1" /></CommandTrigger>'
    "</Style></Root>"
)
BROKEN = '<Root><SoundPlayerAction FileNumber="9" /><Unclosed>'


def main() -> int:
    check("manifest verifies", verifier.verify() == [])
    check("mutations are rejected", verifier.mutation_test() == [])
    real = real_manifest()
    check(
        "claim tiers are pinned",
        {c["id"]: c["tier"] for c in real["claims"]} == verifier.EXPECTED_CLAIMS,
    )
    check(
        "resource path formatting",
        verifier.resource_path(0x035B0007) == "data/03/5B/00/07.DAT"
        and verifier.resource_path(0x5B3E0036) == "data/5B/3E/00/36.DAT",
    )
    check(
        "category numbering of voice ids",
        verifier.category_of(0x5B2D001B) == 3
        and verifier.category_of(0x5B3E0017) == 20,
    )
    check(
        "category 0 holds the markup file numbers",
        real["markup"]["maxFileNumber"]
        <= real["install"]["categories"][0]["lastIndex"],
    )

    image, document = image_fixture()
    check("matching image passes", run(image, document) == [])
    flipped = bytearray(image)
    flipped[0x200 + 5] ^= 1
    check(
        "a changed instruction byte is reported",
        len(run(bytes(flipped), document)) >= 1,
    )
    extra = build_image(image[0x200:] + bytes.fromhex("AC C2 35 01"))
    check("an unlisted immediate is reported", len(run(extra, document)) == 1)
    table = copy.deepcopy(document)
    table["creationVoice"]["listMenu"]["rowGroupDwords"] = [7, 0, 0, 1]
    check("a row table difference is reported", len(run(image, table)) == 1)
    check("identity is enforced", len(verifier.verify_image(image, document)) >= 2)

    calls = build_image(
        bytes.fromhex("E8 0B 00 00 00 E9 06 00 00 00 90 90 90 90 90 C3")
    )
    reached = verifier.call_sites(calls, {0x00401010, 0x00401099})
    check(
        "rel32 call and jump targets are collected",
        reached == {0x00401010: [0x00401000, 0x00401005], 0x00401099: []},
    )
    scanned = {
        "staticChecks": [],
        "entries": [
            {
                "va": "0x00401010",
                "sites": [{"site": "0x00401005"}, {"site": "0x00401000"}],
            }
        ],
        "directCallScans": [{"target": "0x00401099", "sites": []}],
    }
    check("listed call sites match the scan", run(calls, scanned) == [])
    scanned["entries"][0]["sites"].pop()
    check("an unlisted call site is reported", len(run(calls, scanned)) == 1)

    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)
        put(root, 0x035B0000, b"SEDBSSCF" + b"\0" * 8)
        put(root, 0x035B0003, b"SEDBSSCF")
        put(root, 0x035B0004, b"plain")
        put(root, 0x035B2710, b"outside the category span")
        got = verifier.census_category(root, 0x035B0000)
        check(
            "category census counts files, extremes and containers",
            got == {"files": 3, "firstIndex": 0, "lastIndex": 4, "sscfFiles": 2},
        )
        check(
            "an empty category reports no extremes",
            verifier.census_category(root, 0x07FD0000)
            == {"files": 0, "firstIndex": None, "lastIndex": None, "sscfFiles": 0},
        )
        put(root, 0x08070000, b"x")
        put(root, 0x08070101, b"y")
        check(
            "directory counts walk subdirectories",
            verifier.count_files(root, "03", "5B") == 4
            and verifier.count_files(root, "08", "07") == 2,
        )
        (root / "data" / "08" / "07" / ".DS_Store").write_bytes(b"z")
        check(
            "finder metadata is not counted",
            verifier.count_files(root, "08", "07") == 2,
        )
        synthetic = {
            "install": {
                "categories": [
                    {
                        "category": 0,
                        "base": "0x035b0000",
                        "files": 3,
                        "firstIndex": 0,
                        "lastIndex": 4,
                        "sscfFiles": 2,
                    }
                ],
                "directoryFileCounts": {
                    "03/5B": 4,
                    "07/FD": 0,
                    "07/FE": 0,
                    "5B/2D..5B/3E": 0,
                    "8A/0A": 0,
                },
                "footstepBanks": {"08/07": 2, "1A/46": 0},
            }
        }
        check(
            "install table matches itself",
            verifier.verify_install(root, synthetic) == [],
        )
        put(root, 0x035B0005, b"SEDBSSCF")
        check(
            "an extra bank file is reported",
            len(verifier.verify_install(root, synthetic)) >= 2,
        )

    counts = verifier.markup_counts({"a.tpl": XML, "b.tpl": "<Root />"})
    check(
        "markup census counts actions, values and attributes",
        counts["actionElements"] == 3
        and counts["filesWithAction"] == 1
        and counts["fileNumberCounts"] == [[3, 1], [4, 2]]
        and counts["distinctFileNumbers"] == 2
        and counts["minFileNumber"] == 3
        and counts["maxFileNumber"] == 4
        and counts["volumeAttributeCount"] == 1
        and counts["priorityAttributeCount"] == 1
        and counts["indexAttributeCount"] == 0,
    )
    check(
        "markup census classifies the nearest trigger",
        counts["triggerKinds"] == {"EventTrigger": 1, "CommandTrigger": 2}
        and counts["luaSoundCommandTriggerElements"] == 1,
    )
    broken = verifier.markup_counts({"c.tpl": BROKEN})
    check(
        "an unparsable file still counts its actions",
        broken["actionElements"] == 1
        and broken["triggerKinds"] == {"unparsed": 1}
        and broken["fileNumberCounts"] == [[9, 1]],
    )
    expected = dict(counts)
    expected["sqexFiles"] = 2
    expected["templateFilesDecoded"] = 2
    wrong = {"markup": dict(expected, actionElements=4)}
    right = {"markup": expected}
    texts = {"a.tpl": XML, "b.tpl": "<Root />"}
    check(
        "markup comparison accepts equal counts",
        verifier.compare_markup(texts, 2, right) == [],
    )
    check(
        "markup comparison reports a difference",
        len(verifier.compare_markup(texts, 2, wrong)) == 1,
    )

    late = copy.deepcopy(real)
    late["markup"]["fileNumberCounts"][-1][0] = 83
    late["markup"]["maxFileNumber"] = 83
    check(
        "a file number past category 0 is reported",
        any("past the category 0" in e for e in verifier.verify_markup_table(late)),
    )
    swapped = copy.deepcopy(real)
    for item in swapped["staticChecks"]:
        if item["name"] == "creation voice id table":
            item["values"][7], item["values"][8] = item["values"][8], item["values"][7]
    check(
        "swapped voice ids disagree with the group categories",
        any("disagrees" in e for e in verifier.verify_voice(swapped)),
    )

    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} sound effect trigger tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
