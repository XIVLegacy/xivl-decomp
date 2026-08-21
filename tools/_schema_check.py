"""Strict stdlib-only validator for the in-repo attestation schema."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any, Iterator


SUPPORTED = frozenset({
    "$schema", "$id", "title", "description", "type", "properties",
    "required", "additionalProperties", "enum", "const", "pattern",
})
NAME_MAPS = frozenset({"properties"})
JSON_TYPES: dict[str, type | tuple[type, ...]] = {
    "object": dict,
    "array": list,
    "string": str,
    "integer": int,
    "number": (int, float),
    "boolean": bool,
    "null": type(None),
}


class SchemaError(Exception):
    """The schema is malformed or uses an unsupported keyword."""


def load_schema(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as handle:
        schema = json.load(handle)
    if not isinstance(schema, dict):
        raise SchemaError("schema root must be an object")
    _assert_supported(schema, "#", in_name_map=False)
    return schema


def _assert_supported(node: Any, location: str, in_name_map: bool) -> None:
    if isinstance(node, dict):
        if not in_name_map:
            for key in node:
                if key not in SUPPORTED:
                    raise SchemaError(
                        f"{location}: unsupported schema keyword {key!r}"
                    )
            names = node.get("type")
            if names is not None:
                for name in ([names] if isinstance(names, str) else names):
                    if name not in JSON_TYPES:
                        raise SchemaError(
                            f"{location}/type: unknown type {name!r}"
                        )
        for key, value in node.items():
            _assert_supported(
                value,
                f"{location}/{key}",
                in_name_map=not in_name_map and key in NAME_MAPS,
            )
    elif isinstance(node, list):
        for index, value in enumerate(node):
            _assert_supported(value, f"{location}/{index}", in_name_map=False)


def _is_type(value: Any, name: str) -> bool:
    expected = JSON_TYPES[name]
    if name in {"integer", "number"}:
        return isinstance(value, expected) and not isinstance(value, bool)
    return isinstance(value, expected)


def validate(document: Any, schema: dict[str, Any]) -> list[str]:
    return list(_validate(document, schema, "$"))


def _validate(value: Any, schema: dict[str, Any], location: str) -> Iterator[str]:
    names = schema.get("type")
    if names is not None:
        names = [names] if isinstance(names, str) else names
        if not any(_is_type(value, name) for name in names):
            yield (
                f"{location}: expected type {'|'.join(names)}, "
                f"got {type(value).__name__}"
            )
            return

    if "const" in schema and value != schema["const"]:
        yield f"{location}: expected const {schema['const']!r}, got {value!r}"
    if "enum" in schema and value not in schema["enum"]:
        yield f"{location}: {value!r} not in enum {schema['enum']!r}"

    if isinstance(value, str) and "pattern" in schema:
        if not re.search(schema["pattern"], value):
            yield f"{location}: {value!r} does not match schema pattern"

    if not isinstance(value, dict):
        return

    for name in schema.get("required", []):
        if name not in value:
            yield f"{location}: missing required property {name!r}"

    properties = schema.get("properties", {})
    for name, child in value.items():
        if name in properties:
            yield from _validate(child, properties[name], f"{location}.{name}")
        elif schema.get("additionalProperties", True) is False:
            yield f"{location}: unexpected property {name!r}"
