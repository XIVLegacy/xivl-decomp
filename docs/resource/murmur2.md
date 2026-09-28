# MurmurHash2

This page documents the client's backward-walking MurmurHash2 variant and its
wire-id use. The implementation was validated bit-for-bit.

The 1.x client uses a custom **backward-walking** MurmurHash2 variant
to derive the 32-bit wire-id from a property's `/`-path string for
opcode 0x0137. As an inert validation input, `"vector"` hashes to
`0x1294324A`.

This is the *only* hash function in the SetActorProperty path. The
client does not normalise the string, lowercase it, or apply any
length suffix. Just MurmurHash2 with seed=0, reading bytes backward
from the end of the string.

## Source-of-truth function

- Binary: `ffxivgame.exe`, `.text` RVA `0x00931490`,
  `FUN_00d31490`, 170 bytes (0xAA).
- Magic constant `0x5BD1E995` appears 5 times in the function body
  (one per multiply site). This is the standard MurmurHash2 `M`.

## client's port

## Step-by-step correspondence

| binary asm (FUN_00d31490) | rust (murmur_hash2) | description |
|---|---|---|
| `MOV EAX, [ESP+0x4]` | `let data = key.as_bytes()` | data pointer |
| `PUSH ESI; MOV ESI, [ESP+0xc]` | `let mut len = key.len()` | length |
| `MOV ECX, ESI; XOR ECX, [ESP+0x10]` | `let mut h = seed ^ key.len() as u32` | initial hash state |
| `ADD EAX, ESI` | (implicit: `data_index = len - 4`) | walk-end pointer |
| `CMP ESI, 0x4; JC tail` | `while len >= 4` | guard |
| **Main loop:** | | |
| `IMUL ECX, ECX, 0x5bd1e995` | `h = h.wrapping_mul(M)` | h *= M |
| `MOVZX EBX, [EAX-0x2]` ... `OR EDX, EBX` | byte-swap of `i32::from_le_bytes(data[di..di+4])` | k = (b0<<24)\|(b1<<16)\|(b2<<8)\|b3 |
| `IMUL EDX, EDX, 0x5bd1e995` | `k = k.wrapping_mul(M)` | k *= M |
| `MOV EBX, EDX; SHR EBX, 0x18; XOR EBX, EDX` | `k ^= k >> R` (R=24) | k ^= k >> 24 |
| `IMUL EBX, EBX, 0x5bd1e995` | `k = k.wrapping_mul(M)` | k *= M |
| `XOR ECX, EBX` | `h ^= k` | h ^= k |
| `SUB EAX, 0x4; SUB ESI, 0x4` | `data_index -= 4; len -= 4` | walk backward |
| `SUB EDI, 1; JNZ` | `while`-loop | iterate |
| **Tail** (cascading fall-through 3->2->1): | `match tail` | |
| tail=3: `h ^= [EAX] << 16` | `h ^= (data[0] as u32) << 16` | byte 0 |
| tail=3 then 2: `h ^= [EAX+ESI-2] << 8` | `h ^= (data[1] as u32) << 8` | byte 1 |
| tail=3 then 2 then 1: `h ^= [EAX+ESI-1]` | `h ^= data[2] as u32` | byte 2 |
| `IMUL EAX, EAX, 0x5bd1e995` | `h = h.wrapping_mul(M)` | h *= M |
| **Finalizer:** | | |
| `MOV EDX, ECX; SHR EDX, 0xd; XOR EDX, ECX` | `h ^= h >> 13` | |
| `IMUL EDX, EDX, 0x5bd1e995` | `h = h.wrapping_mul(M)` | |
| `MOV EAX, EDX; SHR EAX, 0xf; XOR EAX, EDX` | `h ^= h >> 15` | |
| `RET` | (return h) | |

The two layouts produce bit-identical hash values for any input.

## Test vectors (validated)

All 6 match:

| string | hash (seed=0) |
|---|---:|
| `""` | `0x00000000` |
| `"a"` | `0x92685f5e` |
| `"hello"` | `0x08c5daa9` |
| `"vector"` | `0x1294324a` |
| `"vector2"` | `0x9c7a9994` |
| `"vector02"` | `0xc35bfd82` |

To re-run the validation:

```sh
# Compute Python expected values:
python3 tools/validate_murmur2.py

# The validator exits nonzero if any fixed client-derived vector changes.
```

## Why the backward walk?

Standard MurmurHash2 walks forward through the input. The 1.x client's
variant walks backward. Two plausible reasons:

1. **Cache locality**: at the time of the function call, the string's
   *end* is the most recently-touched byte (the C-string terminator
   was just confirmed by the caller's `strlen` or equivalent). Walking
   backward means processing already-warm cache lines first.

2. **Source artifact**: the original SE engineer might have ported the
   reference MurmurHash2 with a typo and locked in the result via
   shipped test data. By the time anyone noticed, the wire ids were
   baked into server-side dispatch tables and couldn't change.

## What this validates

## What this does NOT validate

- The binary may *also* hash strings in other places using a different
  algorithm (e.g. for sqpack file lookup, opcode dispatch, or
  Lua-script identifier lookup). This validation only covers
  `FUN_00d31490`. Other hash functions need their own independent
  check.

## Related

- `tools/validate_murmur2.py` - Python port + test-vector generator.
- `asm/ffxivgame/00931490_FUN_00d31490.s` - the disassembly.
