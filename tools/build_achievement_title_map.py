#!/usr/bin/env python3
"""Derive title-linked achievement mappings from explicit pinned retail inputs."""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import struct
from pathlib import Path

EXE_BYTES = 15_996_808
EXE_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
CSV_BYTES = 28_895
CSV_SHA256 = "3aecdc3005619b76c813318b12ba7e9b38588d0e3007811d43668a307a68243f"
# Code and lookup-table bounds, exclusive at the right, in ffxivgame.exe VAs.
FUNCTIONS = {
    "achievement_to_ordinal": (0x0070AB40, 0x0070BCB4, 0x0070DBB0),
    "ordinal_to_achievement": (0x0057ABB0, 0x0057BD28, 0x0057CD28),
    "available_title_whitelist": (0x0070F580, 0x0070F640, 0x0070F910),
    "has_title_parent": (0x0070F910, 0x0070FCA8, 0x007101E0),
}


def va_text(va: int) -> str:
    return f"0x{va:08X}"


def pinned_bytes(path: Path, digest: str, size: int | None = None) -> bytes:
    data = path.read_bytes()
    if (size is not None and len(data) != size) or hashlib.sha256(
        data
    ).hexdigest() != digest:
        raise ValueError(f"{path.name}: input size or SHA-256 mismatch")
    return data


class Image:
    def __init__(self, data: bytes):
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe : pe + 4] != b"PE\0\0":
            raise ValueError("PE signature mismatch")
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        optional = pe + 24
        if struct.unpack_from("<H", data, optional)[0] != 0x10B:
            raise ValueError("PE32 required")
        self.base = struct.unpack_from("<I", data, optional + 28)[0]
        if self.base != 0x00400000:
            raise ValueError("unexpected image base")
        self.sections = []
        for index in range(count):
            row = optional + optional_size + index * 40
            _, rva, raw_size, raw = struct.unpack_from("<IIII", data, row + 8)
            self.sections.append((rva, raw_size, raw))

    def read(self, va: int, size: int) -> bytes:
        rva = va - self.base
        for start, raw_size, raw in self.sections:
            if start <= rva and rva + size <= start + raw_size:
                offset = raw + rva - start
                result = self.data[offset : offset + size]
                if len(result) == size:
                    return result
        raise ValueError(f"{va_text(va)}: range is not file-backed")


class NativeMappings:
    """Evaluate only the four read-only integer helpers, without executing x86."""

    def __init__(self, image: Image):
        import capstone

        if capstone.__version__ != "5.0.7":
            raise ValueError("Capstone 5.0.7 required")
        self.cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.cs.detail = True
        self.image = image
        self.instructions = {}

    def evaluate(self, name: str, argument: int) -> tuple[int, str]:
        from capstone.x86_const import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

        if not 0 <= argument <= 0xFFFFFFFF:
            raise ValueError("native argument must be a u32")
        start, code_end, table_end = FUNCTIONS[name]
        registers = {"eax": 0, "ecx": 0, "edx": 0, "esp": 0x02000000}
        flags = {"z": False, "s": False, "o": False, "c": False}
        pc = start

        def register(register_id: int) -> int:
            if register_id == 0:
                return 0
            reg = self.cs.reg_name(register_id)
            if reg in ("al", "cl", "dl"):
                return registers["e" + reg[0] + "x"] & 255
            return registers[reg]

        def get(operand) -> int:
            if operand.type == X86_OP_IMM:
                return operand.imm & 0xFFFFFFFF
            if operand.type == X86_OP_REG:
                return register(operand.reg)
            if operand.type != X86_OP_MEM:
                raise ValueError("unsupported native operand")
            mem = operand.mem
            if mem.segment:
                raise ValueError("segmented memory operand")
            address = (
                mem.disp + register(mem.base) + register(mem.index) * mem.scale
            ) & 0xFFFFFFFF
            if address == 0x02000004 and operand.size == 4:
                return argument
            if not code_end <= address < address + operand.size <= table_end:
                raise ValueError(f"{va_text(pc)}: read outside helper lookup tables")
            return int.from_bytes(self.image.read(address, operand.size), "little")

        def put(operand, value: int) -> None:
            if operand.type != X86_OP_REG:
                raise ValueError(
                    "native memory write is outside the evaluator contract"
                )
            reg = self.cs.reg_name(operand.reg)
            if reg in ("al", "cl", "dl"):
                full = "e" + reg[0] + "x"
                registers[full] = (registers[full] & 0xFFFFFF00) | (value & 255)
            elif reg in ("eax", "ecx", "edx"):
                registers[reg] = value & 0xFFFFFFFF
            else:
                raise ValueError(f"unsupported native destination {reg}")

        for _ in range(200):
            if not start <= pc < code_end:
                raise ValueError(f"{va_text(pc)}: branch outside helper code")
            if pc not in self.instructions:
                instruction = next(
                    self.cs.disasm(self.image.read(pc, 15), pc, count=1), None
                )
                if (
                    instruction is None
                    or instruction.address + instruction.size > code_end
                ):
                    raise ValueError(f"{va_text(pc)}: invalid instruction")
                self.instructions[pc] = instruction
            instruction = self.instructions[pc]
            operands = instruction.operands
            mnemonic = instruction.mnemonic
            next_pc = pc + instruction.size
            if mnemonic == "ret" and not operands:
                return registers["eax"], va_text(pc)
            if mnemonic in ("mov", "movzx"):
                put(operands[0], get(operands[1]))
            elif mnemonic in ("add", "sub", "xor", "cmp"):
                left, right = get(operands[0]), get(operands[1])
                bits = operands[0].size * 8
                mask, sign = (1 << bits) - 1, 1 << (bits - 1)
                left, right = left & mask, right & mask
                if mnemonic == "add":
                    result = (left + right) & mask
                    carry = left + right > mask
                    overflow = bool((~(left ^ right) & (left ^ result)) & sign)
                elif mnemonic in ("sub", "cmp"):
                    result = (left - right) & mask
                    carry = left < right
                    overflow = bool(((left ^ right) & (left ^ result)) & sign)
                else:
                    result, carry, overflow = left ^ right, False, False
                flags = {
                    "z": result == 0,
                    "s": bool(result & sign),
                    "o": overflow,
                    "c": carry,
                }
                if mnemonic != "cmp":
                    put(operands[0], result)
            elif mnemonic == "jmp":
                next_pc = get(operands[0])
            elif mnemonic in (
                "je",
                "jne",
                "jg",
                "jl",
                "jge",
                "jle",
                "ja",
                "jb",
                "jae",
                "jbe",
            ):
                z, s, o, c = (flags[key] for key in ("z", "s", "o", "c"))
                taken = {
                    "je": z,
                    "jne": not z,
                    "jg": not z and s == o,
                    "jl": s != o,
                    "jge": s == o,
                    "jle": z or s != o,
                    "ja": not c and not z,
                    "jb": c,
                    "jae": not c,
                    "jbe": c or z,
                }[mnemonic]
                if taken:
                    next_pc = get(operands[0])
            else:
                raise ValueError(f"{va_text(pc)}: unsupported instruction {mnemonic}")
            pc = next_pc
        raise ValueError("native mapping exceeded 200-instruction bound")


def title_links(data: bytes) -> dict[int, int]:
    rows = list(csv.reader(io.StringIO(data.decode("utf-8-sig"))))
    if (
        len(rows) < 3
        or rows[0][1:] != [str(index) for index in range(20)]
        or rows[1][4] != "s32"
    ):
        raise ValueError("achievement.csv logical column layout differs")
    links = {}
    seen = set()
    for row in rows[2:]:
        if len(row) != 21:
            raise ValueError("achievement.csv row width differs")
        achievement, title = int(row[0]), int(row[4] or 0)
        if achievement in seen:
            raise ValueError("duplicate achievement row ID")
        seen.add(achievement)
        if not 0 <= achievement <= 0xFFFFFFFF or not 0 <= title <= 0x7FFFFFFF:
            raise ValueError("achievement or title ID out of range")
        if title:
            links[achievement] = title
    return links


def completion(mapping: NativeMappings, achievement: int) -> dict:
    ordinal, result_va = mapping.evaluate("achievement_to_ordinal", achievement)
    if ordinal >= 1024:
        return {
            "status": "out_of_range",
            "ordinal": None,
            "native_result": ordinal,
            "result_va": result_va,
        }
    inverse, inverse_va = mapping.evaluate("ordinal_to_achievement", ordinal)
    return {
        "status": "proven" if inverse == achievement else "unproven_inverse_mismatch",
        "ordinal": ordinal if inverse == achievement else None,
        "native_result": ordinal,
        "result_va": result_va,
        "inverse_achievement_id": inverse,
        "inverse_result_va": inverse_va,
    }


def derive(image: Image, links: dict[int, int]) -> dict:
    mapping = NativeMappings(image)
    rows = []
    for achievement, title in sorted(links.items()):
        whitelist, whitelist_va = mapping.evaluate(
            "available_title_whitelist", achievement
        )
        admitted = whitelist & 255
        if admitted not in (0, 1):
            raise ValueError("whitelist returned a non-Boolean AL")
        parent, parent_va = mapping.evaluate("has_title_parent", achievement)
        parent_record = {
            "status": "mapped" if parent else "no_parent",
            "native_result": parent,
            "result_va": parent_va,
            "achievement_id": parent or None,
            "completion": completion(mapping, parent) if parent else None,
        }
        if parent and (parent not in links or links[parent] != title):
            raise ValueError(f"achievement {achievement}: parent title join differs")
        if (
            parent
            and mapping.evaluate("available_title_whitelist", parent)[0] & 255 != 1
        ):
            raise ValueError(f"achievement {achievement}: parent is not admitted")
        rows.append(
            {
                "achievement_id": achievement,
                "title_id": title,
                "title_map_store": "stores_nonzero_when_row_loaded",
                "eligible_for_available_title": bool(admitted),
                "whitelist_status": "proven",
                "whitelist_result_va": whitelist_va,
                "direct_completion": completion(mapping, achievement),
                "has_achievement_title_parent": parent_record,
            }
        )
    return {
        "format": "xivl-achievement-title-map-v1",
        "producer": "tools/build_achievement_title_map.py",
        "inputs": {
            "ffxivgame.exe": {
                "bytes": EXE_BYTES,
                "sha256": EXE_SHA256,
                "image_base": "0x00400000",
            },
            "achievement.csv": {
                "bytes": CSV_BYTES,
                "sha256": CSV_SHA256,
                "source": "xivl-client-data:manifests/tables.json",
                "column": "3 (zero-based logical column; row ID precedes column 0)",
            },
        },
        "method": {
            "decoder": "Capstone 5.0.7, x86-32",
            "evaluation": "File-backed instruction and lookup-table evaluation for every title-linked row; no target execution.",
            "functions": {
                name: {
                    "entry_va": va_text(start),
                    "code_end_exclusive_va": va_text(code_end),
                    "lookup_end_exclusive_va": va_text(table_end),
                }
                for name, (start, code_end, table_end) in FUNCTIONS.items()
            },
            "title_map_store": {
                "loader_column_read_va": "0x0076AF0F",
                "loader_call_va": "0x0076AF95",
                "helper_nonzero_store_va": "0x006F0F75-0x006F0F88",
                "map_va": "0x0134B578",
                "meaning": "Static nonzero store when the loader visits the row, not observed runtime membership.",
            },
            "completion": "A direct or parent ordinal is proven only when <1024 and inverse(ordinal) equals that achievement. Native result 0 alone is ambiguous.",
            "parent_zero": "No parent returned by 0x0070F910; null completion is not a direct-bit rejection.",
        },
        "summary": {
            "title_linked_rows": len(rows),
            "eligible_for_available_title": sum(
                row["eligible_for_available_title"] for row in rows
            ),
            "not_eligible_for_available_title": sum(
                not row["eligible_for_available_title"] for row in rows
            ),
            "proven_direct_ordinals": sum(
                row["direct_completion"]["status"] == "proven" for row in rows
            ),
            "unresolved_direct_ordinals": [
                row["achievement_id"]
                for row in rows
                if row["direct_completion"]["status"] != "proven"
            ],
            "mapped_predicate_parents": sum(
                row["has_achievement_title_parent"]["status"] == "mapped"
                for row in rows
            ),
        },
        "rows": rows,
    }


def validate_artifact(document: dict) -> list[str]:
    """Check the published complete map without the private retail inputs."""
    errors = []
    if not isinstance(document, dict):
        return ["artifact must be an object"]
    if document.get("format") != "xivl-achievement-title-map-v1":
        errors.append("artifact format differs")
    inputs = document.get("inputs", {})
    if not isinstance(inputs, dict) or inputs.get("ffxivgame.exe") != {
        "bytes": EXE_BYTES,
        "sha256": EXE_SHA256,
        "image_base": "0x00400000",
    }:
        errors.append("executable identity differs")
    csv_input = inputs.get("achievement.csv", {}) if isinstance(inputs, dict) else {}
    if (
        not isinstance(csv_input, dict)
        or csv_input.get("sha256") != CSV_SHA256
        or csv_input.get("bytes") != CSV_BYTES
    ):
        errors.append("achievement CSV identity differs")
    if document.get("producer") != "tools/build_achievement_title_map.py":
        errors.append("artifact producer differs")
    method = document.get("method", {})
    expected_functions = {
        name: {
            "entry_va": va_text(start),
            "code_end_exclusive_va": va_text(code_end),
            "lookup_end_exclusive_va": va_text(table_end),
        }
        for name, (start, code_end, table_end) in FUNCTIONS.items()
    }
    if (
        not isinstance(method, dict)
        or method.get("decoder") != "Capstone 5.0.7, x86-32"
        or method.get("functions") != expected_functions
    ):
        errors.append("native derivation locators differ")

    def valid_locator(value, name: str) -> bool:
        if not isinstance(value, str):
            return False
        try:
            address = int(value, 16)
        except ValueError:
            return False
        start, end, _ = FUNCTIONS[name]
        return value == va_text(address) and start <= address < end

    rows = document.get("rows")
    if not isinstance(rows, list) or len(rows) != 208:
        return errors + ["expected exactly 208 title-linked rows"]
    by_id = {}
    ordinals = set()
    admitted = 0
    parent_count = 0
    for row in rows:
        if not isinstance(row, dict):
            errors.append("row must be an object")
            continue
        achievement = row.get("achievement_id")
        if type(achievement) is not int or achievement <= 0 or achievement in by_id:
            errors.append("invalid or duplicate achievement ID")
            continue
        by_id[achievement] = row
        if type(row.get("title_id")) is not int or row["title_id"] <= 0:
            errors.append(f"{achievement}: expected a nonzero title ID")
        if row.get("title_map_store") != "stores_nonzero_when_row_loaded":
            errors.append(f"{achievement}: static loader-store qualification differs")
        eligible = row.get("eligible_for_available_title")
        if type(eligible) is not bool or row.get("whitelist_status") != "proven":
            errors.append(f"{achievement}: whitelist admission is not proven")
        admitted += eligible is True
        direct = row.get("direct_completion", {})
        if not isinstance(direct, dict):
            errors.append(f"{achievement}: missing direct completion")
            continue
        if (
            not valid_locator(
                row.get("whitelist_result_va"), "available_title_whitelist"
            )
            or not valid_locator(direct.get("result_va"), "achievement_to_ordinal")
            or not valid_locator(
                direct.get("inverse_result_va"), "ordinal_to_achievement"
            )
        ):
            errors.append(f"{achievement}: native result locator differs")
        ordinal = direct.get("ordinal")
        if (
            type(ordinal) is not int
            or not 0 <= ordinal < 1024
            or direct.get("status") != "proven"
            or direct.get("native_result") != ordinal
            or direct.get("inverse_achievement_id") != achievement
        ):
            errors.append(
                f"{achievement}: direct completion is not independently proven"
            )
        elif ordinal in ordinals:
            errors.append(f"{achievement}: duplicate direct ordinal")
        else:
            ordinals.add(ordinal)
    for achievement, row in by_id.items():
        parent = row.get("has_achievement_title_parent", {})
        if not isinstance(parent, dict):
            errors.append(f"{achievement}: missing predicate-parent result")
            continue
        if not valid_locator(parent.get("result_va"), "has_title_parent"):
            errors.append(f"{achievement}: predicate result locator differs")
        parent_id = parent.get("achievement_id")
        if row.get("eligible_for_available_title") is True:
            if any(
                (
                    parent.get("status") != "no_parent",
                    parent.get("native_result") != 0,
                    parent_id is not None,
                    parent.get("completion") is not None,
                )
            ):
                errors.append(f"{achievement}: no-parent sentinel differs")
            continue
        parent_count += 1
        target = by_id.get(parent_id) if type(parent_id) is int else None
        completion_record = parent.get("completion")
        if (
            parent.get("status") != "mapped"
            or parent.get("native_result") != parent_id
            or target is None
            or target.get("title_id") != row.get("title_id")
            or target.get("eligible_for_available_title") is not True
            or completion_record != target.get("direct_completion")
        ):
            errors.append(
                f"{achievement}: predicate parent/title/completion join differs"
            )
    if list(by_id) != sorted(by_id) or len(by_id) != 208:
        errors.append("achievement IDs must be unique and sorted")
    if admitted != 105 or parent_count != 103:
        errors.append("expected 105 whitelist rows and 103 predicate-parent rows")
    for achievement, expected in {
        197: 18,
        198: 19,
        390: 146,
        391: 147,
        392: 148,
    }.items():
        direct = by_id.get(achievement, {}).get("direct_completion", {})
        if not isinstance(direct, dict) or direct.get("ordinal") != expected:
            errors.append(f"{achievement}: known ordinal {expected} differs")
    for achievement, ordinal, parent_id, parent_ordinal in (
        (106, 6, 197, 18),
        (279, 129, 392, 148),
    ):
        row = by_id.get(achievement, {})
        direct = row.get("direct_completion", {})
        parent = row.get("has_achievement_title_parent", {})
        if (
            not isinstance(direct, dict)
            or direct.get("ordinal") != ordinal
            or not isinstance(parent, dict)
            or parent.get("achievement_id") != parent_id
            or not isinstance(parent.get("completion"), dict)
            or parent["completion"].get("ordinal") != parent_ordinal
        ):
            errors.append(f"{achievement}: direct/parent distinction differs")
    if document.get("summary") != {
        "title_linked_rows": 208,
        "eligible_for_available_title": 105,
        "not_eligible_for_available_title": 103,
        "proven_direct_ordinals": 208,
        "unresolved_direct_ordinals": [],
        "mapped_predicate_parents": 103,
    }:
        errors.append("complete-map summary differs")
    return errors


def serialize(document: dict) -> str:
    return json.dumps(document, indent=2, ensure_ascii=True) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--achievement-csv", type=Path, required=True)
    destination = parser.add_mutually_exclusive_group(required=True)
    destination.add_argument("--output", type=Path)
    destination.add_argument("--check", type=Path)
    args = parser.parse_args()
    try:
        image = Image(pinned_bytes(args.exe, EXE_SHA256, EXE_BYTES))
        links = title_links(pinned_bytes(args.achievement_csv, CSV_SHA256, CSV_BYTES))
        document = derive(image, links)
        errors = validate_artifact(document)
        if errors:
            raise ValueError("; ".join(errors))
        rendered = serialize(document)
        if args.check:
            if args.check.read_bytes() != rendered.encode("ascii"):
                raise ValueError(
                    "achievement title map differs from exact-build derivation"
                )
            print("Achievement title map: PASS (208 title-linked rows)")
        else:
            target = args.output.resolve()
            if target in (args.exe.resolve(), args.achievement_csv.resolve()) or (
                target.exists()
                and any(
                    target.samefile(source)
                    for source in (args.exe, args.achievement_csv)
                )
            ):
                raise ValueError("output must not replace an input")
            args.output.write_bytes(rendered.encode("ascii"))
    except (OSError, ValueError, KeyError, struct.error, ImportError) as error:
        parser.exit(1, f"ERROR: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
