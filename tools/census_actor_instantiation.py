#!/usr/bin/env python3
"""Parse every retail s2c 0x00CC record with the reference model and compare the census."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from collections import Counter
from pathlib import Path

import verify_actor_instantiation as verifier


OPCODE = 0x00CC
HEADER_BYTES = 32


def pattern_key(controls: list[bool]) -> str:
    return ",".join("3" if flag else "4" for flag in controls)


def summarize(records: list[dict]) -> dict:
    """Aggregate parsed records into the manifest census.

    Each record carries capture, sha256, source, destination and application.
    A record is the self record when its source equals its destination. A record
    parses to its end when the values stop at the END tag and only zero bytes
    follow it.
    """
    patterns: Counter[str] = Counter()
    to_end = 0
    captures: dict[str, dict] = {}
    for record in records:
        is_self = record["source"] == record["destination"]
        parsed = verifier.parse_application(record["application"], is_self)
        patterns[pattern_key(parsed["controls"])] += 1
        to_end += parsed["tailClear"]
        row = captures.setdefault(
            record["capture"],
            {
                "capture": record["capture"],
                "sha256": record["sha256"],
                "records": 0,
                "selfRecords": 0,
            },
        )
        row["records"] += 1
        row["selfRecords"] += is_self
    rows = [captures[name] for name in sorted(captures)]
    return {
        "records": sum(row["records"] for row in rows),
        "selfRecords": sum(row["selfRecords"] for row in rows),
        "parsedToEnd": to_end,
        "controlPatterns": dict(patterns),
        "captures": rows,
    }


def compare(found: dict, observed: dict) -> list[str]:
    errors = []
    for key in ("records", "selfRecords", "parsedToEnd", "controlPatterns", "captures"):
        if found[key] != observed[key]:
            errors.append(f"census {key} differs from the manifest")
    return errors


def collect(directory: Path, extractors: Path) -> list[dict]:
    sys.path.insert(0, str(extractors))
    from extract_observations import parse_sub_events  # noqa: PLC0415
    from extract_streams import (  # noqa: PLC0415
        maybe_inflate,
        parse_outer_frames,
        reconstruct_lanes,
    )

    records = []
    for path in sorted(directory.glob("*.pcapng")):
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        for connection in reconstruct_lanes(path):
            if connection["lane"] != "main":
                continue
            blob = connection["streams"].get("s2c", b"")
            for frame in parse_outer_frames(blob):
                inflated = maybe_inflate(frame["body"])
                body = inflated if inflated is not None else frame["body"]
                for event in parse_sub_events(body):
                    if event["type"] != 3 or event.get("inner_opcode") != OPCODE:
                        continue
                    start = event["offset"]
                    raw = body[start : start + event["size"]]
                    records.append(
                        {
                            "capture": path.name,
                            "sha256": digest,
                            "source": event["src_actor"],
                            "destination": event["dst_actor"],
                            "application": raw[HEADER_BYTES:],
                        }
                    )
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--captures",
        type=Path,
        default=os.environ.get("XIVL_PCAP_OBJECTS_DIR"),
        help="directory of the pcap-1.23b objects",
    )
    parser.add_argument(
        "--extractors",
        type=Path,
        default=os.environ.get("XIVL_CAPTURES_EXTRACTORS"),
        help="tools/extractors of an xivl-captures checkout",
    )
    args = parser.parse_args()
    if not args.captures or not args.extractors:
        parser.error("--captures and --extractors are required")
    found = summarize(collect(Path(args.captures), Path(args.extractors)))
    document = json.loads(verifier.MANIFEST.read_text(encoding="ascii"))
    errors = compare(found, document["observed"])
    if errors:
        print(f"FAIL: {len(errors)} census difference(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"PASS: {found['records']} records, {found['selfRecords']} self records")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
