#!/usr/bin/env python3
# xivl-decomp - clean-room decompilation of FINAL FANTASY XIV 1.x client binaries
# Copyright (C) 2026  XIVLegacy Dev Team
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Decode `LobbyCryptEngine`'s 9 vtable slots and validate the embedded
Blowfish P/S init tables against the canonical OpenSSL pi-derived
constants.

Outputs:
- `build/wire/<binary>.crypt_engine.md` - slot-by-slot decode + key
  schedule walkthrough and client-derived findings.

The 9 slots were identified from `build/wire/<binary>.net_handlers.md`
(the section `Application::Network::LobbyProtoChannel::ServiceConsumerConnectionManager::LobbyCryptEngine`).
The Blowfish init constants live at fixed virtual addresses in `.data`:
- `0x01267278..0x012672BF` - initial P[18] (72 bytes)
- `0x012672C0..0x012682BF` - initial S[4][256] (4096 bytes)
Total 4168 bytes = `sizeof(BF_KEY)` in OpenSSL.

The lobby's per-block primitives are statically-linked OpenSSL:
- `FUN_0045aac0` = `BF_encrypt(BF_LONG[2], BF_KEY*)` (forward through P[0..17])
- `FUN_0045aa30` = `BF_decrypt(BF_LONG[2], BF_KEY*)` (backward from P[17])
- `FUN_0045abf0` = `BF_set_key(BF_KEY*, int keylen, const unsigned char*)`

The slot-level wrappers add lobby-specific framing:
- 32-byte chunk alignment (encrypt/decrypt round length DOWN to a
  multiple of 32 = 4 Blowfish blocks, NOT 8).
- A non-canonical sign-extension quirk in the key-schedule's byte-
  cycling step (uses `MOVSX byte` not `MOVZX byte`), so keys with
  high-bit bytes produce a different schedule than stock OpenSSL.

"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

# Paths in the repo
ROOT = Path(__file__).resolve().parent.parent
ORIG = ROOT / "orig"
BUILD_WIRE = ROOT / "build" / "wire"

# ---------------------------------------------------------------------
# Slot table (from build/wire/ffxivgame.net_handlers.md, the
# LobbyProtoChannel::ServiceConsumerConnectionManager::LobbyCryptEngine
# section). 9 slots, all overrides of the abstract
# CryptEngineInterface (whose slots 1..8 are __purecall in the parent
# vtable).
# ---------------------------------------------------------------------

@dataclass(frozen=True)
class Slot:
    idx: int
    rva: int
    semantic: str
    summary: str

LOBBY_SLOTS: list[Slot] = [
    Slot(0, 0x009a1e40, "~LobbyCryptEngine (dtor)",
         "Sets parent vtable, frees [this+0x30] (= BF_KEY*) via _free."),
    Slot(1, 0x009a1590, "PrepareHandshake / SeedRequest",
         "Copies 32-byte seed (\"Test Ticket Data\\0\\0\\0\\0clientNumber\") "
         "from .data 0x011274F0 to this+0x10. Calls __time64(NULL) and stores "
         "low 32 bits of result at this+0x8 + req+0x74. Memcpys 64 bytes from "
         "this+0x10 to req+0x34 (the cipher-init payload sent in the lobby "
         "handshake). Returns true."),
    Slot(2, 0x009a1640, "GetExtendedFlag (3-arg, returns 0)",
         "Logger noise + XOR EAX, EAX; RET 0xc. Always returns 0/null. Stub "
         "override of an interface method that real subclasses might use; "
         "lobby has no extended payload."),
    Slot(3, 0x009a0f10, "Verify-A (2-arg, returns false)",
         "5-byte stub: XOR AL, AL; RET 8. Always returns false. The lobby "
         "doesn't implement this verification slot."),
    Slot(4, 0x009a1670, "SetSessionKey (2 args)",
         "~600 bytes. Frees old [this+0x30] and clears it. Builds a 16-byte "
         "key on stack from arg1+arg2 (the SqexId session token + handshake "
         "response). Allocates 0x1048 (4168) bytes via _malloc -> new BF_KEY*. "
         "Calls FUN_0045abf0 = BF_set_key(BF_KEY*, &key_data, 16). Stores "
         "result at [this+0x30]. Logs progress at each step. Returns true."),
    Slot(5, 0x009a0f20, "Verify-B (2-arg, returns false)",
         "5-byte stub: XOR AL, AL; RET 8. Always returns false. Same shape "
         "as slot 3; the two are kept separate rather than COMDAT-folded."),
    Slot(6, 0x009a18d0, "Encrypt(_, buf, len)",
         "Reads len = (uint16) [ESP+0xc], rounds DOWN to multiple of 32 "
         "(via AND ~0x1F). If [this+0x30] != null, calls FUN_0045ab60 with "
         "(buf, buf, len_aligned) - in-place ECB Blowfish encrypt of "
         "len/8 blocks via BF_encrypt per-block. Returns true."),
    Slot(7, 0x009a0f30, "Decrypt(_, buf, len)",
         "Same shape as slot 6 but with no logging; calls FUN_0045abb0 -> "
         "in-place ECB Blowfish decrypt via BF_decrypt per-block. The two "
         "32-byte alignment + in-place semantics are identical."),
    Slot(8, 0x009a1920, "GetCompatibility (1-arg, returns true)",
         "Logs the arg, returns AL=1. A capability-probe stub the lobby "
         "always answers \"yes\" to."),
]

# Per-block + key-schedule helper RVAs (file offsets, .text)
HELPERS: dict[str, tuple[int, str]] = {
    "BF_encrypt":  (0x0005aac0, "Forward Blowfish round (XOR P[0..17] in order)."),
    "BF_decrypt":  (0x0005aa30, "Reverse Blowfish round (XOR P[17..0])."),
    "BF_set_key":  (0x0005abf0, "OpenSSL key schedule: copies P+S init from .data, "
                                "XORs key bytes (sign-extended via MOVSX!), "
                                "then encrypts (0,0)->P[0..1] cascade."),
    "encrypt_buf": (0x0005ab60, "Slot-6 helper: optional memcpy(dst,src,len) + "
                                "loop calling BF_encrypt for each 8-byte block."),
    "decrypt_buf": (0x0005abb0, "Slot-7 helper: same shape but BF_decrypt + "
                                "different loop guard (`JZ` vs `JLE`)."),
}

# Where the canonical pi-derived BF init tables live.
P_INIT_VA = 0x01267278       # 72 bytes = 18 u32
S_INIT_VA = 0x012672C0       # 4096 bytes = 4 * 256 u32
IMAGE_BASE = 0x00400000

# OpenSSL canonical pi-derived first 4 P entries, for sanity check.
EXPECTED_P0 = [0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344]
# OpenSSL canonical pi-derived first 4 S[0] entries.
EXPECTED_S00 = [0xD1310BA6, 0x98DFB5AC, 0x2FFD72DB, 0xD01ADFB7]


def parse_pe(path: Path) -> tuple[bytes, list[tuple[str, int, int, int, int]]]:
    """Return (raw_bytes, [(name, va, vsize, raw_off, raw_sz), ...])."""
    data = path.read_bytes()
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, e_lfanew + 6)[0]
    opt_size = struct.unpack_from("<H", data, e_lfanew + 0x14)[0]
    sec_off = e_lfanew + 0x18 + opt_size
    sections = []
    for i in range(nsec):
        s = data[sec_off + i * 0x28: sec_off + (i + 1) * 0x28]
        name = s[:8].rstrip(b"\x00").decode("ascii", errors="replace")
        vsize, vaddr, rsize, raddr = struct.unpack("<IIII", s[8:0x18])
        sections.append((name, vaddr, vsize, raddr, rsize))
    return data, sections


def va_to_off(va: int, sections: list[tuple[str, int, int, int, int]]) -> int | None:
    rva = va - IMAGE_BASE
    for _, vaddr, vsize, raddr, _ in sections:
        if vaddr <= rva < vaddr + vsize:
            return raddr + (rva - vaddr)
    return None


def read_u32_le(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binary", nargs="?", default="ffxivgame",
                    choices=("ffxivgame", "ffxivgame.exe"),
                    help="fixed supported client binary")
    args = ap.parse_args()
    stem = args.binary.replace(".exe", "")
    exe = ORIG / f"{stem}.exe"
    if not exe.exists():
        print(f"error: {exe} not found", file=sys.stderr)
        return 1

    data, sections = parse_pe(exe)
    p_off = va_to_off(P_INIT_VA, sections)
    s_off = va_to_off(S_INIT_VA, sections)
    if p_off is None or s_off is None:
        print("error: P/S init VAs not in any section", file=sys.stderr)
        return 1
    bin_p = data[p_off:p_off + 72]
    bin_s = data[s_off:s_off + 4096]
    p_ok = all(read_u32_le(bin_p, i * 4) == EXPECTED_P0[i] for i in range(4))
    s_ok = all(read_u32_le(bin_s, i * 4) == EXPECTED_S00[i] for i in range(4))

    BUILD_WIRE.mkdir(parents=True, exist_ok=True)
    out = BUILD_WIRE / f"{stem}.crypt_engine.md"
    lines = [
        f"# LobbyCryptEngine decode - {stem}.exe", "",
        "Generated by `tools/extract_crypt_engine.py` from the client executable.", "",
        "## Client findings", "",
        "- Cipher: statically linked OpenSSL Blowfish.",
        "- Block size: 8 bytes.",
        "- Session key size: 16 bytes.",
        "- Slots 6 and 7 round the processed length down to a multiple of 32 bytes.",
        "- The key schedule sign-extends key bytes with MOVSX.",
        "- P and S initialization tables begin at VAs `0x01267278` and `0x012672c0`.", "",
        "## Canonical table prefix check", "",
        f"- P prefix: {'PASS' if p_ok else 'FAIL'}",
        f"- S prefix: {'PASS' if s_ok else 'FAIL'}", "",
        "## Vtable slots", "",
        "| slot | RVA | semantic | summary |", "|---:|---:|---|---|",
    ]
    for slot in LOBBY_SLOTS:
        lines.append(f"| {slot.idx} | `0x{slot.rva:08x}` | {slot.semantic} | {slot.summary} |")
    lines.extend(["", "## Helper functions", "", "| RVA | name | role |", "|---:|---|---|"])
    for name, (rva, role) in HELPERS.items():
        lines.append(f"| `0x{rva:08x}` | `{name}` | {role} |")
    lines.extend(["", "The table-prefix checks validate the client bytes against fixed canonical Blowfish constants.", ""])
    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {out.relative_to(ROOT)}")
    print(f"  P-init prefix: {'PASS' if p_ok else 'FAIL'}")
    print(f"  S-init prefix: {'PASS' if s_ok else 'FAIL'}")
    return 0 if p_ok and s_ok else 1


if __name__ == "__main__":
    sys.exit(main())
