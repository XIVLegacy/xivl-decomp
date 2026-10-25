#!/usr/bin/env python3
"""Tests for the Sqwt Anime property verifier, its reference model and image checks."""

from __future__ import annotations

import copy
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_sqwt_anime_property as verifier  # noqa: E402


PASSED: list[str] = []
FAILED: list[str] = []
IMAGE_BASE = 0x00400000


def check(name: str, condition: bool) -> None:
    (PASSED if condition else FAILED).append(name)


def crc32_bitwise(data: bytes) -> int:
    """Independent CRC-32 so the model is not checked against itself."""
    state = 0xFFFFFFFF
    for byte in data:
        state ^= byte
        for _ in range(8):
            state = (state >> 1) ^ 0xEDB88320 if state & 1 else state >> 1
    return state ^ 0xFFFFFFFF


def name_cases() -> None:
    check("Anime id is the CRC-32 of ANIME", verifier.name_id("Anime") == 0xE4509BF2)
    check(
        "the id folds case",
        verifier.name_id("anime") == verifier.name_id("aNiMe") == 0xE4509BF2,
    )
    check(
        "the id matches an independent CRC-32",
        verifier.name_id("UIElement.Execute") == crc32_bitwise(b"UIELEMENT.EXECUTE"),
    )
    check("a longer name has another id", verifier.name_id("Animes") != 0xE4509BF2)
    check(
        "non-ASCII bytes pass through unfolded and keep the length",
        verifier.name_id("\xff") == crc32_bitwise(b"\xff")
        and verifier.name_id("stra\xdfe") == crc32_bitwise(b"STRA\xdfE")
        and verifier.name_id("stra\xdfe") != verifier.name_id("strasse"),
    )
    check(
        "Show becomes ShowInactive",
        verifier.rewrite_value("Show") == "ShowInactive",
    )
    check(
        "only the exact text is rewritten",
        [
            verifier.rewrite_value(text)
            for text in ("show", "SHOW", "Show ", " Show", "ShowInactive", "Hide")
        ]
        == ["show", "SHOW", "Show ", " Show", "ShowInactive", "Hide"],
    )
    check(
        "the gate passes for byte 1 with any value",
        verifier.gate_passes(1, "ShowInactive") and verifier.gate_passes(1, "Hide"),
    )
    check(
        "the gate blocks ShowInactive when the byte is not 1",
        not verifier.gate_passes(0, "ShowInactive")
        and not verifier.gate_passes(2, "ShowInactive"),
    )
    check(
        "the gate passes other values when the byte is not 1",
        verifier.gate_passes(0, "Hide") and verifier.gate_passes(0, ""),
    )
    check(
        "the registry holds 23 names",
        len(verifier.REGISTERED) == 23 and len(set(verifier.REGISTERED)) == 23,
    )
    check(
        "lookup is case sensitive and exact",
        verifier.command_exists("ShowInactive")
        and verifier.command_exists("Null")
        and not verifier.command_exists("show")
        and not verifier.command_exists("Fade")
        and not verifier.command_exists("ShowInactive "),
    )


def effect_cases() -> None:
    effect = verifier.window_effect
    check(
        "Show on a hidden window raises Shown",
        effect("Show", "Hidden") == ("Visible", ["Shown"]),
    )
    check(
        "Show on a visible window raises nothing",
        effect("Show", "Visible") == ("Visible", []),
    )
    check(
        "Show under a hidden ancestor raises nothing",
        effect("Show", "Hidden", ancestors_visible=False) == ("Visible", []),
    )
    check(
        "ShowInactive on a visible window still raises Shown",
        effect("ShowInactive", "Visible") == ("Visible", ["Shown"]),
    )
    check(
        "ShowInactive under a hidden ancestor still raises Shown",
        effect("ShowInactive", "Hidden", ancestors_visible=False)
        == ("Visible", ["Shown"]),
    )
    check(
        "the log widget global of 1 removes Shown",
        effect("ShowInactive", "Hidden", log_widget=True, log_global=1)
        == ("Visible", []),
    )
    check(
        "the log widget global of 0 keeps Shown",
        effect("ShowInactive", "Hidden", log_widget=True, log_global=0)
        == ("Visible", ["Shown"]),
    )
    check(
        "the global needs the log widget name",
        effect("ShowInactive", "Hidden", log_widget=False, log_global=1)
        == ("Visible", ["Shown"]),
    )
    check(
        "Hide raises Hiding then hides",
        effect("Hide", "Visible") == ("Hidden", ["Hiding"]),
    )
    check(
        "a cancelled Hide keeps the window visible",
        effect("Hide", "Visible", cancel_hiding=True) == ("Visible", ["Hiding"]),
    )
    check(
        "Hide without the allow flag does nothing",
        effect("Hide", "Visible", hide_allowed=False) == ("Visible", []),
    )
    check(
        "the log widget Hide skips Hiding",
        effect("Hide", "Visible", log_widget=True, log_global=1) == ("Hidden", []),
    )
    check(
        "InvisibleWindow hides without an event",
        effect("InvisibleWindow", "Visible") == ("Hidden", []),
    )
    check(
        "an untraced command leaves the state alone",
        effect("Close", "Visible") == ("Visible", [])
        and effect("Copy", "Hidden") == ("Hidden", []),
    )


def step_cases() -> None:
    step = verifier.anime_step
    start = {"latch": "", "visibility": "Hidden"}
    state, events = step(start, "Show")
    check(
        "the first Show latches ShowInactive, shows and raises Shown",
        state == {"latch": "ShowInactive", "visibility": "Visible"}
        and events == ["Shown"],
    )
    state, events = step(state, "Show")
    check(
        "the second Show is suppressed by the latch",
        state == {"latch": "ShowInactive", "visibility": "Visible"} and events == [],
    )
    state, events = step(state, "ShowInactive")
    check("a typed ShowInactive hits the same latch", events == [])
    state["visibility"] = "Hidden"
    state, events = step(state, "Show")
    check(
        "a window hidden by other means stays hidden under the latch",
        state["visibility"] == "Hidden" and events == [],
    )
    state, events = step(state, "Hide")
    check(
        "Hide changes the latch and hides",
        state == {"latch": "Hide", "visibility": "Hidden"} and events == ["Hiding"],
    )
    state, events = step(state, "Show")
    check(
        "Show after Hide fires again",
        state == {"latch": "ShowInactive", "visibility": "Visible"}
        and events == ["Shown"],
    )
    state, events = step(start, "Fade")
    check(
        "an unregistered value latches and does nothing",
        state == {"latch": "Fade", "visibility": "Hidden"} and events == [],
    )
    state, events = step(start, "show")
    check(
        "lower case show is not rewritten and finds no command",
        state == {"latch": "show", "visibility": "Hidden"} and events == [],
    )
    state, events = step(start, "Show", receiver_byte=0)
    check(
        "a clear receiver byte drops ShowInactive before the latch",
        state == start and events == [],
    )
    state, events = step(start, "Hide", receiver_byte=0)
    check("a clear receiver byte keeps Hide", state["latch"] == "Hide")
    state, events = step(start, "")
    check(
        "an empty value equals the empty latch",
        state == start and events == [],
    )
    state, events = step(start, "Show", store_refused=True)
    check(
        "a refused store keeps the latch and raises nothing",
        state == start and events == [],
    )
    state, events = step(start, "Show", preview_handled=True)
    check(
        "a handled PreviewExecuted latches but skips the Executed effect",
        state == {"latch": "ShowInactive", "visibility": "Hidden"} and events == [],
    )
    state, events = step(start, "Show", preview_handled=True, store_refused=True)
    check("a refused store wins over a handled preview", state == start)
    original = dict(start)
    step(start, "Show")
    check("a step does not change its input", start == original)


def example_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    names = [item["name"] for item in document["examples"]]
    check("the examples are unique", len(names) == len(set(names)) == 16)
    for item in document["examples"]:
        state, events = verifier.run_example(item)
        check(
            f"example {item['name']} matches its literals",
            (state["latch"], state["visibility"], events)
            == (item["latch"], item["visibility"], item["events"]),
        )
    sequence = next(i for i in document["examples"] if i["name"] == "Hide then Show")
    check(
        "the Hide then Show example has two steps and two events",
        len(sequence["steps"]) == 2 and sequence["events"] == ["Hiding", "Shown"],
    )


def image_bytes(rows: list[tuple[int, bytes, int | None]]) -> bytes:
    """Build a PE32 image with one section per row. A row may add virtual-only bytes."""
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(rows) + 0xFFF) & ~0xFFF
    header = bytearray(first_raw)
    header[0:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x80 + 6, len(rows))
    struct.pack_into("<H", header, 0x80 + 20, 0xE0)
    body = bytearray()
    for index, (va, data, virtual_size) in enumerate(rows):
        raw_offset = first_raw + len(body)
        base = table + 40 * index
        header[base : base + 8] = f".s{index}".encode().ljust(8, b"\0")
        struct.pack_into(
            "<IIII",
            header,
            base + 8,
            virtual_size or len(data),
            va - IMAGE_BASE,
            len(data),
            raw_offset,
        )
        body += data
    return bytes(header) + bytes(body)


def row_bytes(item: dict, strings: dict[str, int]) -> bytes:
    kind = item["kind"]
    if kind == "string":
        return item["text"].encode("ascii") + b"\0"
    if kind == "dwords":
        return b"".join(
            int(value, 16).to_bytes(4, "little") for value in item["values"]
        )
    if kind == "rangeSha256":
        return bytes(index % 251 for index in range(item["length"]))
    if kind == "routedEvent":
        name_va = strings[item["eventName"]]
        return (
            b"\x68"
            + name_va.to_bytes(4, "little")
            + b"\x51\x8b\xc4\x51\xc6\x00"
            + int(item["id"], 16).to_bytes(1, "little")
            + b"\x8b\xc4\xb9"
            + int(item["static"], 16).to_bytes(4, "little")
            + b"\xc6\x00"
            + item["strategy"].to_bytes(1, "little")
            + b"\xe8\0\0\0\0"
        )
    return bytes.fromhex(item["pattern"])


def build_image(document: dict) -> tuple[bytes, list[dict], list[int]]:
    """Return the image, the byte-carrying rows in section order and their offsets."""
    for item in document["staticChecks"]:
        if item["kind"] == "rangeSha256":
            filler = bytes(index % 251 for index in range(item["length"]))
            item["sha256"] = hashlib.sha256(filler).hexdigest()
    names_va = 0x02000000
    strings: dict[str, int] = {}
    name_rows = []
    for item in document["staticChecks"]:
        if item["kind"] == "routedEvent":
            strings[item["eventName"]] = names_va
            name_rows.append((names_va, item["eventName"].encode() + b"\0", None))
            names_va += 0x100
    carrying = [
        item
        for item in document["staticChecks"]
        if item["kind"] not in {"absoluteRefs", "zeroFill"}
    ]
    rows = [(int(item["va"], 16), row_bytes(item, strings), None) for item in carrying]
    rows += name_rows
    refs = next(i for i in document["staticChecks"] if i["kind"] == "absoluteRefs")
    needle = int(refs["value"], 16).to_bytes(4, "little")
    rows += [(int(site, 16), needle, None) for site in refs["sites"]]
    zero = next(i for i in document["staticChecks"] if i["kind"] == "zeroFill")
    zero_va = int(zero["va"], 16)
    rows.append((zero_va & ~0xFFFF, bytes(0x40), 0x20000))
    return image_bytes(rows), carrying, [len(row[1]) for row in rows]


def image_cases() -> None:
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    for item in document["staticChecks"]:
        if item["kind"] == "absoluteRefs":
            item["sites"] = [f"0x{int(s, 16) + 0x10000000:08x}" for s in item["sites"]]
    image, carrying, sizes = build_image(document)
    check(
        "an image built from the rows passes",
        verifier.verify_image(image, document, require_identity=False) == [],
    )
    check(
        "identity is enforced",
        len(verifier.verify_image(image, document)) >= 2,
    )
    table = 0x80 + 24 + 0xE0
    first_raw = (table + 40 * len(sizes) + 0xFFF) & ~0xFFF
    missed = []
    offset = first_raw
    for index, item in enumerate(carrying):
        flipped = bytearray(image)
        flipped[offset] ^= 0x01
        if not verifier.verify_image(bytes(flipped), document, require_identity=False):
            missed.append(item["name"])
        offset += sizes[index]
    check(f"every row reports a flipped byte (missed {missed})", not missed)
    ref_start = first_raw + sum(sizes[: len(carrying)])
    names_size = sum(sizes[len(carrying) : -3])
    ref_start += names_size
    removed = bytearray(image)
    removed[ref_start : ref_start + 4] = b"\0\0\0\0"
    check(
        "a missing reference site is reported",
        verifier.verify_image(bytes(removed), document, require_identity=False) != [],
    )
    extra = copy.deepcopy(document)
    for item in extra["staticChecks"]:
        if item["kind"] == "absoluteRefs":
            item["sites"] = item["sites"][:1]
    check(
        "an unlisted reference site is reported",
        verifier.verify_image(image, extra, require_identity=False) != [],
    )
    backed = copy.deepcopy(document)
    for item in backed["staticChecks"]:
        if item["kind"] == "zeroFill":
            item["va"] = "0x01350010"
    check(
        "a zero fill address backed by file bytes is reported",
        verifier.verify_image(image, backed, require_identity=False) != [],
    )
    outside = copy.deepcopy(document)
    for item in outside["staticChecks"]:
        if item["kind"] == "zeroFill":
            item["va"] = "0x7f000000"
    check(
        "a zero fill address outside the image is reported",
        verifier.verify_image(image, outside, require_identity=False) != [],
    )
    check(
        "an image that is not a PE is reported",
        verifier.verify_image(b"not a pe", document, require_identity=False) != [],
    )


def manifest_cases() -> None:
    check("the manifest verifies", verifier.verify() == [])
    check("every manifest mutation is rejected", verifier.mutation_test() == [])
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    check("the page holds every contract line", verifier.doc_errors(document) == [])
    check(
        "a page without a contract line is rejected",
        verifier.doc_errors(document, "STATIC INFERRED UNKNOWN CALIBRATION") != [],
    )
    page = verifier.DOC.read_text(encoding="ascii")
    for label, old, new in (
        (
            "a changed contract line",
            "3. Let W be ShowInactive when V is exactly Show",
            "3. Let W be Show when V is exactly ShowInactive",
        ),
        (
            "a changed handler row",
            "| ShowInactive | 0x013576E8 | 0x0092638E |",
            "| ShowInactive | 0x013576E8 | 0x0092638F |",
        ),
        (
            "a changed command object",
            "| Hide | 0x01357788 | 0x0106A3BC |",
            "| Hide | 0x01357789 | 0x0106A3BC |",
        ),
        ("a changed Visibility value", "| Visible | 2 |", "| Visible | 1 |"),
        (
            "a changed pickup slot",
            "| 3 | 0x00542710 | string |",
            "| 3 | 0x00542711 | string |",
        ),
    ):
        check(
            f"a page with {label} is rejected",
            old in page and verifier.doc_errors(document, page.replace(old, new)) != [],
        )
    check(
        "a wrapped and ticked contract line is accepted",
        verifier.flatten("`a`\n  `b` c") == "a b c"
        and verifier.doc_errors(document, page) == [],
    )
    for label in ("STATIC", "INFERRED", "UNKNOWN", "CALIBRATION"):
        check(
            f"a page without the {label} label is rejected",
            verifier.doc_errors(document, page.replace(label, "X")) != [],
        )
    broken = copy.deepcopy(document)
    broken["staticChecks"][0]["kind"] = "bytes"
    check("an unknown row kind is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["examples"][0]["events"] = []
    check(
        "an example that disagrees with the model is rejected",
        verifier.verify(broken) != [],
    )
    broken = copy.deepcopy(document)
    broken["examples"].pop()
    check("a dropped example is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    for item in broken["staticChecks"]:
        if item["name"] == "setter function":
            item["sha256"] = "z" * 64
    check("a malformed range hash is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    for item in broken["staticChecks"]:
        if item["name"] == "absolute references to log widget global":
            item["sites"] = ["0x009263bc"]
    check("a shortened reference list is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["commands"].pop()
    check("a dropped command is rejected", verifier.verify(broken) != [])
    broken = copy.deepcopy(document)
    broken["handlers"].append(
        {
            "command": "Fade",
            "objectVa": "0x01357000",
            "compareVa": "0x00926000",
            "effect": "x",
        }
    )
    check(
        "a handler for an unregistered command is rejected",
        verifier.verify(broken) != [],
    )
    broken = copy.deepcopy(document)
    broken["open"][0]["label"] = "STATIC"
    check("a STATIC open question is rejected", verifier.verify(broken) != [])
    check(
        "the Visibility values order Collapsed, Hidden, Visible",
        document["visibility"]["values"] == {"Collapsed": 0, "Hidden": 1, "Visible": 2},
    )
    check(
        "the show routine passes 1, 0, 1 for Show and 1, 1, 1 for ShowInactive",
        document["showRoutine"]["show"] == [1, 0, 1]
        and document["showRoutine"]["showInactive"] == [1, 1, 1],
    )
    check(
        "the Window delivery chain ends at the generic dispatcher",
        document["window"]["chain"][0] == document["window"]["forwardVa"]
        and document["window"]["chain"][-1] == document["window"]["dispatchVa"],
    )
    check(
        "the Window Executed slot is 65 at offset 0x104",
        document["window"]["executedSlot"] == 65
        and document["window"]["executedOffset"] == "0x104"
        and 65 * 4 == 0x104,
    )


def main() -> int:
    name_cases()
    effect_cases()
    step_cases()
    example_cases()
    image_cases()
    manifest_cases()
    if FAILED:
        print(f"FAIL: {len(FAILED)} of {len(FAILED) + len(PASSED)}")
        for name in FAILED:
            print(f"- {name}")
        return 1
    print(f"PASS: {len(PASSED)} sqwt anime property tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
