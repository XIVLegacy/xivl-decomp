#!/usr/bin/env python3
"""Count the language and sd name shapes of the Sqwt form and template files of an install."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path

LANGUAGE_TOKENS = ("ja", "en", "de", "fr", "chs", "cht")
STEM_EXTENSIONS = ("form", "tpl")
SD_TOKEN = "sd"
MANIFEST = Path(__file__).resolve().parent.parent / "config" / "sqwt_form_locale.json"


def split_name(relative: str) -> tuple[str, str, str, tuple[str, ...], str]:
    """Return (root, directory, stem, tokens, extension chain) of a sqwt-relative path.

    The stem is the text before the first dot. The tokens are the dot parts
    between the stem and the last part, which is the final extension.
    """
    parts = relative.split("/")
    pieces = parts[-1].split(".")
    directory = "/".join(parts[:-1])
    root = parts[0] if len(parts) > 1 else ""
    if len(pieces) < 2:
        return root, directory, pieces[0], (), ""
    return root, directory, pieces[0], tuple(pieces[1:-1]), pieces[-1]


def category(variants: set[tuple[str, ...]]) -> str:
    has_bare = () in variants
    has_language = any(item and item[0] in LANGUAGE_TOKENS for item in variants)
    if has_bare and has_language:
        return "both"
    if has_language:
        return "languageOnly"
    if has_bare:
        return "bareOnly"
    return "sdOnly"


def census(paths: list[str]) -> dict:
    """Aggregate the form and template name shapes of sqwt-relative paths.

    A stem is the pair (directory, name before the first dot) per extension.
    Its variants are the token tuples found for it. A language token is the
    first token. The sd token may follow it or stand alone.
    """
    stems: dict[tuple[str, str, str, str], set[tuple[str, ...]]] = {}
    shapes: dict[str, Counter[str]] = {ext: Counter() for ext in STEM_EXTENSIONS}
    localized_other: Counter[str] = Counter()
    other_files = 0
    for relative in sorted(paths):
        root, directory, stem, tokens, ext = split_name(relative)
        if ext in STEM_EXTENSIONS and not (
            len(tokens) > 2
            or any(token not in LANGUAGE_TOKENS + (SD_TOKEN,) for token in tokens)
        ):
            stems.setdefault((root, directory, stem, ext), set()).add(tokens)
            shapes[ext][".".join(tokens)] += 1
        elif ext in STEM_EXTENSIONS:
            raise ValueError(f"unrecognized name shape {relative}")
        else:
            other_files += 1
            if tokens and tokens[0] in LANGUAGE_TOKENS:
                localized_other[".".join(tokens[1:] + (ext,))] += 1
    roots: dict[str, dict[str, Counter[str]]] = {}
    localized: list[dict] = []
    for (root, directory, stem, ext), variants in sorted(stems.items()):
        row = roots.setdefault(root, {}).setdefault(ext, Counter())
        kind = category(variants)
        row["stems"] += 1
        row[kind] += 1
        row["withSd"] += any(SD_TOKEN in item for item in variants)
        languages = sorted(
            {item[0] for item in variants if item and item[0] in LANGUAGE_TOKENS},
            key=LANGUAGE_TOKENS.index,
        )
        if languages:
            localized.append(
                {
                    "directory": directory,
                    "stem": stem,
                    "extension": ext,
                    "languages": languages,
                    "bare": () in variants,
                }
            )
    return {
        "roots": {
            root: {ext: dict(sorted(row.items())) for ext, row in sorted(exts.items())}
            for root, exts in sorted(roots.items())
        },
        "shapes": {
            ext: dict(sorted(counter.items())) for ext, counter in shapes.items()
        },
        "localizedStems": localized,
        "localizedOther": dict(sorted(localized_other.items())),
        "otherFiles": other_files,
    }


def list_install(root: Path) -> list[str]:
    return sorted(
        path.relative_to(root).as_posix() for path in root.rglob("*") if path.is_file()
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--install",
        type=Path,
        required=True,
        help="client/sqwt directory of a 1.23b install",
    )
    parser.add_argument(
        "--print", action="store_true", help="print the census instead of comparing"
    )
    args = parser.parse_args()
    result = census(list_install(args.install))
    if args.print:
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    document = json.loads(MANIFEST.read_text(encoding="ascii"))
    expected = document["census"]
    errors = [
        f"census {key} differs from the manifest"
        for key in result
        if result[key] != expected.get(key)
    ]
    if errors:
        print(f"FAIL: {len(errors)} sqwt form locale census error(s)")
        for error in errors:
            print(f"- {error}")
        return 1
    print("PASS: sqwt form locale census matches the manifest")
    return 0


if __name__ == "__main__":
    sys.exit(main())
