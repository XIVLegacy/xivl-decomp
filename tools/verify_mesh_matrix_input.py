#!/usr/bin/env python3
"""Replay the pinned mesh matrix builder on synthetic objects, without a process.

Requires numpy, pefile and unicorn 2.1.4. The submission gateway and security
cookie check are stubs. Both native matrix providers and arithmetic execute.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

EXPECTED_SHA256 = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
STACK, STACK_SIZE = 0x08000000, 0x10000
DATA, DATA_SIZE = 0x09000000, 0x10000
MESH, NODE, CONTEXT, DRAWABLE = DATA, DATA + 0x1000, DATA + 0x2000, DATA + 0x4000
STOP = 0x07000000
RANGES = (
    (0xC44C50, 0xC44F25),
    (0xC32BD0, 0xC32BD7),
    (0xC33C10, 0xC33CFE),
    (0x61F0F0, 0x61F201),
    (0x545670, 0x54574E),
    (0x42EDB0, 0x42EE11),
    (0x42EC50, 0x42EDAA),
    (0x4201A0, 0x420200),
    (0x419B60, 0x419BBA),
    (0x6522D0, 0x652449),
    (0xC47910, 0xC47960),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def execute(image: bytes, matrices: dict, alternate: bool) -> tuple[np.ndarray, int]:
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x400000, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(0x400000, image)
    uc.mem_map(STACK, STACK_SIZE)
    uc.mem_map(DATA, DATA_SIZE)
    uc.mem_map(STOP, 0x1000)

    def put32(address: int, value: int) -> None:
        uc.mem_write(address, struct.pack("<I", value))

    def get32(address: int) -> int:
        return struct.unpack("<I", uc.mem_read(address, 4))[0]

    put32(MESH + 0x118, NODE)
    put32(NODE + 0x78, DRAWABLE)
    put32(NODE + 0x98, int(alternate))
    uc.mem_write(DRAWABLE + 0x30, matrices["N"].tobytes())
    for name, offset in (("A", 0xA0), ("E", 0xE0), ("P", 0x160), ("V", 0x20)):
        uc.mem_write(CONTEXT + offset, matrices[name].tobytes())
    before = bytes(uc.mem_read(DATA, DATA_SIZE))
    entry_esp = STACK + STACK_SIZE - 0x100
    put32(entry_esp, STOP)
    put32(entry_esp + 4, CONTEXT)
    uc.reg_write(UC_X86_REG_ESP, entry_esp)
    uc.reg_write(UC_X86_REG_ECX, MESH)
    submissions, writes, visits = [], [], set()

    def stub_return() -> None:
        sp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EIP, get32(sp))
        uc.reg_write(UC_X86_REG_ESP, sp + 4)

    def code_hook(_uc, address, _size, _data) -> None:
        visits.add(address)
        if address == 0x41BFC0:
            sp = uc.reg_read(UC_X86_REG_ESP)
            start, source, count = (get32(sp + k) for k in (4, 8, 12))
            require((start, count) == (0, 16), "unexpected constant range")
            # Q is the aligned ESP before SUB ESP,0x218 in the native prologue.
            q = (entry_esp - 4) & ~0xF
            require(source == q - 0x110, "unexpected stack buffer base")
            require(source + 256 == q - 0x10, "unexpected stack buffer extent")
            submissions.append(bytes(uc.mem_read(source, 256)))
            stub_return()
        elif address == 0x9D20F4:
            stub_return()
        else:
            require(
                any(lo <= address < hi for lo, hi in RANGES),
                f"unexpected native instruction {address:08X}",
            )

    def write_hook(_uc, _access, address, size, _value, _data) -> None:
        writes.append((address, size))
        require(
            STACK <= address and address + size <= STACK + STACK_SIZE,
            f"non-stack write {address:08X}",
        )

    uc.hook_add(UC_HOOK_CODE, code_hook)
    uc.hook_add(UC_HOOK_MEM_WRITE, write_hook)
    uc.emu_start(0xC44C50, STOP, count=20000)
    require(uc.reg_read(UC_X86_REG_EIP) == STOP, "instruction limit reached")
    require(len(submissions) == 1, "expected one submission")
    require(uc.reg_read(UC_X86_REG_ESP) == entry_esp + 8, "RET 4 mismatch")
    require(bytes(uc.mem_read(DATA, DATA_SIZE)) == before, "input objects changed")
    require((0xC33C10 in visits) == alternate, "provider selection mismatch")
    return np.frombuffer(submissions[0], dtype="<f4").reshape(4, 4, 4), len(writes)


def affine(rng) -> np.ndarray:
    matrix = np.eye(4, dtype=np.float32)
    matrix[:3, :3] += rng.uniform(-0.3, 0.3, (3, 3)).astype(np.float32)
    matrix[3, :3] = rng.uniform(-20, 20, 3)
    return matrix


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    args = parser.parse_args()
    raw = args.exe.read_bytes()
    require(len(raw) == 15996808, "executable size mismatch")
    require(hashlib.sha256(raw).hexdigest() == EXPECTED_SHA256, "SHA-256 mismatch")
    pe = pefile.PE(data=raw)
    require(pe.FILE_HEADER.Machine == 0x14C, "expected x86")
    require(pe.OPTIONAL_HEADER.ImageBase == 0x400000, "image base mismatch")
    image = pe.get_memory_mapped_image()
    rng = np.random.default_rng(123)
    case_count, write_count, largest_error = 0, 0, 0.0
    for alternate in (False, True):
        for _ in range(16):
            matrices = {name: affine(rng) for name in ("N", "A", "E", "P", "V")}
            actual, writes = execute(image, matrices, alternate)
            selected = matrices["N"].copy()
            if alternate:
                selected[:3] = np.eye(4, dtype=np.float32)[:3]
            composite = matrices["E"] @ matrices["A"] @ selected
            expected = np.stack(
                (
                    composite,
                    np.linalg.inv(composite).T,
                    composite @ matrices["P"],
                    composite @ matrices["V"],
                )
            )
            largest_error = max(largest_error, float(np.max(np.abs(actual - expected))))
            np.testing.assert_allclose(actual, expected, rtol=2e-5, atol=5e-5)
            case_count += 1
            write_count += writes
    print(
        json.dumps(
            {
                "executable_sha256": EXPECTED_SHA256,
                "cases": case_count,
                "native_alternate_provider": True,
                "stack_write_instructions": write_count,
                "non_stack_writes": 0,
                "gateway_start": 0,
                "gateway_float4_count": 16,
                "maximum_absolute_float_error": largest_error,
                "node_context_unchanged": True,
                "limits": "synthetic affine objects; gateway and cookie check stubbed; "
                "no material, cache, draw, queue, GPU, lifetime or actor admission execution",
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
