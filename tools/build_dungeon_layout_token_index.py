#!/usr/bin/env python3
"""Build a hash-guarded raw-string index for the 24 dungeon layout DATs."""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CLIENT_ROOT = Path(r"C:\Program Files (x86)\SquareEnix\FINAL FANTASY XIV")
DEFAULT_OUTPUT = REPO_ROOT / "docs/resource/dungeon-layout-token-index.csv"

# The 24 installed layout resources covered by the typed dungeon timeline corpus.
DATS = {
    "0x29B00008": "263c2a90566037253c69b4c32c99f1cc8b9aae1e7bd23247b9234b4c41ae7b4d",
    "0x29B00009": "360bef1ab5917e3d2e87e4a946c3fc848a7d76ced5159e21a5bb01af1cd4f111",
    "0x29B0000A": "4cc89b551f12551e6b320297fc4b22e28c7b551216f0fdfbd4ebf2322ddc4d62",
    "0x29B0000B": "3262d8053b77ec1ccfd73d9fc0337385ec021c80db5fb6d4f292d5a5ebb5c4c6",
    "0x29B0000C": "c8c0d9455d45b44ce5218d9a7a0d776303fe9d90334bb8a5661e01106efa1ef9",
    "0x29B0000D": "dee1bd760dcdfe1a46c24b5c8bc79314ef8866acd19b59187e2f93f40d6fd51e",
    "0x28D90006": "98bc9d3a0111de81a95b01a3e1c9a94453f51f11169634eea0c80f97d329b368",
    "0x28D90007": "2d18ff1f1e3c0ee2e8ee41b035e7a3f8e43f69341f4cf7d9c7585d1b1bc93529",
    "0x28D90008": "91383d4dbdddbdf297e854a56590b40b077b7faffcb5e60defad68307708d2b0",
    "0x28D90009": "a4fdee1c2f4767aefd204a118173155cb89ef03713fb35a93776e64b23d7fe2c",
    "0x28D9000A": "078958f6995371e44c36127de0ed2c2e6144cc667e2d43e8a6608d4c264c0929",
    "0x28D9000B": "1a1ab2c65759d5b8c7a969cfa4fad57162b48cc09f750978e1ace644d72b636d",
    "0x29D90008": "4574985fb2d1e4b6068411350375777dd9d257fa2c5111f1de53a775172e67ec",
    "0x29D90009": "0fdba99375f3282d58286d98ecb2af734170ff3cfb3369967b569488c8df46c2",
    "0x29D9000A": "55661f6823d8647131fbaeba0770a3c581fb72cf94528f60a98ffdcf2c234d10",
    "0x29D9000B": "24f04e719bba037d2f3303864e43139a8094e777acbcf2bf0f0eb81a95975922",
    "0x29D9000C": "8e290d6c5ccf037621ed5cc092927caa46e12d8c38fb429a04e03424d24c9d16",
    "0x29D9000D": "d8ff9874f392367fa111a75997915257a465bf8fc4c613cafa035cd12f6e733a",
    "0x615A0009": "5a945e6936d565547fc7bb81bbda35c385fa2564506ffd4670819fb52eaf69c5",
    "0x615A000A": "019c70f30dc49d9c26b86d7c60033eac075c44222a95c2063048df8e089a3892",
    "0x615A000B": "c669f65d3f0897f8c328ebe51f2dbc8417dcfb9ec327af49eba14de9320afa06",
    "0x615A000C": "ffbab10a565160d2dd06c8fb36f0f9e136db9eb81ad0f1a8ab6b817519d4ab49",
    "0x615A000D": "aaa14a56cb812cc82f91ab62b3df01c61a2fe6d9e95b74d7ca0df3a4c3fd3df4",
    "0x615A000E": "541b6742c55921f3ba3152da94c83a0057dfea7662bb01d89adea9f260f98790",
}

ASCII_RUN = re.compile(rb"[ -~]{4,}")
TOKEN_PATTERN = re.compile(
    r"^(?:"
    r"time_[A-Za-z0-9_.-]+|"
    r"sdef_[A-Za-z0-9_.-]+|"
    r"(?:sgrp|grp|tgbx|ipomk|cbind)_[A-Za-z0-9_.-]+|"
    r"vfx_[A-Za-z0-9_.-]+|"
    r"Lay[A-Za-z0-9]+Clip|ShowHideClip|"
    r"open|clos|close|hide|show|stt0|end0|start|loop|idle|initf_idle"
    r")$",
    re.IGNORECASE,
)
SEMANTIC_KEYWORDS = re.compile(
    r"(?:door|gate|barrier|warp|tele|exit|treasure|chest|coffer|sand|quicksand|"
    r"magitek|magic|terminal|lift|elev|lever|switch|bridge|seal|boss|fire|flame|"
    r"crystal|photocell|kama|wall|port|circle)",
    re.IGNORECASE,
)
SEMANTIC_TOKEN = re.compile(r"[A-Za-z0-9_./\\:-]+")
SHORT_COMMANDS = {
    "open",
    "clos",
    "close",
    "hide",
    "show",
    "stt0",
    "end0",
    "start",
    "loop",
    "idle",
    "initf_idle",
}
FIELDS = (
    "dat_key_hex",
    "relative_path",
    "dat_sha256",
    "token_class",
    "token",
    "first_offset_hex",
    "occurrences",
)
EXPECTED_ROWS = 8373
EXPECTED_OCCURRENCES = 9849


def dat_relative_path(dat_key_hex: str) -> str:
    value = int(dat_key_hex, 16)
    return "data/{:02X}/{:02X}/{:02X}/{:02X}.DAT".format(
        (value >> 24) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 8) & 0xFF,
        value & 0xFF,
    )


def token_class(token: str) -> str:
    lowered = token.lower()
    if lowered.startswith("time_"):
        return "timeline"
    if lowered.startswith("sdef_"):
        return "scheduler_definition"
    if lowered in SHORT_COMMANDS:
        return "short_command"
    if lowered.startswith("vfx_"):
        return "vfx_resource"
    if lowered.startswith(("sgrp_", "grp_", "tgbx_", "ipomk_", "cbind_")):
        return "control_group"
    if lowered.endswith("clip"):
        return "clip_class"
    return "semantic_resource"


def scan_dat(dat_key_hex: str, data: bytes, digest: str) -> list[dict[str, object]]:
    relative_path = dat_relative_path(dat_key_hex)
    found: dict[tuple[str, str], dict[str, object]] = {}
    for match in ASCII_RUN.finditer(data):
        token = match.group().decode("ascii")
        selected = TOKEN_PATTERN.fullmatch(token)
        semantic = (
            not selected
            and len(token) <= 128
            and SEMANTIC_KEYWORDS.search(token)
            and SEMANTIC_TOKEN.fullmatch(token)
        )
        if not selected and not semantic:
            continue
        kind = token_class(token) if selected else "semantic_resource"
        key = (kind, token)
        if key in found:
            found[key]["occurrences"] = int(found[key]["occurrences"]) + 1
            continue
        found[key] = {
            "dat_key_hex": dat_key_hex,
            "relative_path": relative_path,
            "dat_sha256": digest,
            "token_class": kind,
            "token": token,
            "first_offset_hex": f"0x{match.start():X}",
            "occurrences": 1,
        }
    return sorted(
        found.values(), key=lambda row: (str(row["token_class"]), str(row["token"]))
    )


def build_rows(client_root: Path) -> list[dict[str, object]]:
    if len(DATS) != 24:
        raise ValueError(f"expected 24 pinned DATs, found {len(DATS)}")
    rows: list[dict[str, object]] = []
    for dat_key_hex, expected_digest in sorted(
        DATS.items(), key=lambda item: int(item[0], 16)
    ):
        path = client_root / dat_relative_path(dat_key_hex)
        data = path.read_bytes()
        actual_digest = hashlib.sha256(data).hexdigest()
        if actual_digest != expected_digest:
            raise ValueError(
                f"{dat_key_hex} hash mismatch: expected {expected_digest}, found {actual_digest}"
            )
        rows.extend(scan_dat(dat_key_hex, data, actual_digest))
    if len(rows) != EXPECTED_ROWS:
        raise ValueError(f"expected {EXPECTED_ROWS} token rows, found {len(rows)}")
    occurrences = sum(int(row["occurrences"]) for row in rows)
    if occurrences != EXPECTED_OCCURRENCES:
        raise ValueError(
            f"expected {EXPECTED_OCCURRENCES} occurrences, found {occurrences}"
        )
    return rows


def render_csv(rows: list[dict[str, object]]) -> bytes:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue().encode("utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client-root", type=Path, default=DEFAULT_CLIENT_ROOT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--check", action="store_true", help="compare with the existing output"
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        rendered = render_csv(build_rows(args.client_root))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    if args.check:
        try:
            existing = args.output.read_bytes()
        except OSError as error:
            print(f"error: {error}", file=sys.stderr)
            return 1
        if existing != rendered:
            print(f"mismatch: {args.output}", file=sys.stderr)
            return 1
        print(
            f"PASS: {len(DATS)} pinned DATs, {EXPECTED_ROWS} rows, {EXPECTED_OCCURRENCES} occurrences"
        )
        return 0
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(rendered)
    print(
        f"wrote {len(DATS)} DATs, {EXPECTED_ROWS} rows, {EXPECTED_OCCURRENCES} occurrences to {args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
