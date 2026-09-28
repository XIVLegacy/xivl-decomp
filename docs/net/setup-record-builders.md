# Setup record builders and constructor context

This page records the byte writes and direct calls for two setup-labeled record builders. The names "plain" and "secure" below follow the caller's log strings; they do not identify a retainer operation or a server-side record type.

## Binary and decode

The input is retail 1.23b `ffxivgame.exe`, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The PE32 image base is `0x00400000`. `pefile` 2024.8.26 mapped virtual addresses through the PE section table; Capstone 5.0.7 decoded the x86-32 instructions. VA equals image base plus RVA.

## Shared output-buffer checks

The builders receive a state pointer in `ECX`. Both require the word at state `+0x0E` to be zero. Each zero-extends the cursor word at `+0x0C`, adds its literal headroom, and rejects only when the sum is above the capacity dword at `+0`:

| Builder VA (RVA) | Capacity comparison | Copied record | Record words at `+0` and `+2` |
|---|---:|---:|---|
| `0x00DB2A90` (`0x009B2A90`) | cursor `+ 0x48` | `0x38` bytes (`0x0E` dwords) | `0x38`, `1` |
| `0x00DB2BD0` (`0x009B2BD0`) | cursor `+ 0x288` | `0x278` bytes (`0x9E` dwords) | `0x278`, `9` |

The headroom terms are the instructions' literal comparison operands. On an empty-buffer path, each builder initializes the output header, sets its length and cursor to `0x10`, then appends the record. Thus `0x48` and `0x288` equal the initialized `0x10` header plus the respective record lengths in that path. The code still compares cursor plus the full headroom; the record copy itself is only `0x38` or `0x278` bytes.

When the cursor is zero, both bodies clear four dwords at the output pointer, write byte `1` at offset `+0`, set the length word at `+4` to `0x10`, clear the count word at `+6`, and set the state cursor to `0x10`. Both then write a caller-supplied low word to output offset `+2`: the plain body loads it from `[ebp+0x0C]`, while the secure body loads it from `[ebp+0x10]`. After a successful append, each advances the state cursor and output length by the copied record length, increments the output count word at `+6`, and returns `1`. A failed gate, capacity check, or callback returns `0`.

## Plain builder at `0x00DB2A90`

The staging record starts at local stack offset `+0x10`. The body writes `0x38` at record `+0`, type word `1` at `+2`, zeroes dwords at `+4` and `+8`, writes the first argument at `+0x10`, and zeroes dwords at `+0x14` through `+0x34`. It does not write record `+0x0C`; that local dword is included in the append copy without being initialized by this body. A conditional path reads dwords at `+4` and `+8` through another argument and calls `0x006CE260` and `0x009D4600` using a length derived from those values. Those helper contracts and the copied data's meaning are not identified here.

On the append path, `rep movsd` copies exactly `0x0E` dwords from the staging record to the output pointer plus the cursor. The cursor and output length each advance by `0x38`, and the output count increases by one. These writes establish the local record bytes and append mechanics, not the meaning of its type word or body.

## Secure-labeled builder at `0x00DB2BD0`

The staging record starts at local stack offset `+0x10`. The body writes `0x278` at record `+0`, type word `9` at `+2`, and zeroes `0x268` bytes beginning at record `+0x10`. It then writes the data argument from `[ebp+0x0C]` at record `+0x10`. A conditional path reads dwords at `+4` and `+8` through another argument and calls `0x006CE260` and `0x009D4600`; the helpers' span and payload contracts remain unresolved.

Before copying, the body rejects a null first argument. Otherwise it loads the vtable pointer from the object at offset `+0`, loads the function pointer from vtable offset `+4`, and calls it with the staging-record address. A zero `AL` result aborts; a nonzero result permits `rep movsd` to copy exactly `0x9E` dwords to the output pointer plus the cursor. The cursor and output length each advance by `0x278`, and the output count increases by one. This is a local approval gate in the observed code; it does not establish what the argument represents.

## Constructor and caller context

The constructor at VA `0x00DB8330` (RVA `0x009B8330`) installs vtable VA `0x01129754`, stores its argument at `+0x08`, calls `0x0095E590` with `ECX=this+0x0C`, clears state at `+0x18` through `+0x3C`, calls `0x004588E0` with argument `5` for subobjects at `+0x40` and `+0x5C`, and clears `+0x78`. Its direct constructor callsite is VA `0x00DB7C7F` (RVA `0x009B7C7F`).

The constructor at VA `0x00DA28C0` (RVA `0x009A28C0`) performs the same observed field initialization but installs vtable VA `0x01127700`. Its direct constructor callsite is VA `0x00DA1990` (RVA `0x009A1990`). The distinct vtable values establish two constructor variants; these bodies alone do not identify their C++ types or relationship.

At VA `0x00DB3CB0` (RVA `0x009B3CB0`), the caller references `=== tryToAddSecSetupPacket` at VA `0x01129130` (RVA `0x00D29130`) and `=== tryToAddSetupPacket` at VA `0x0112914C` (RVA `0x00D2914C`). When state `+0x78` is nonzero it logs the secure-labeled string and calls `0x00DB2BD0`; if that call fails, it proceeds to log the plain-labeled string and call `0x00DB2A90`. If `+0x78` is zero, it goes directly to the plain-labeled call. Other direct pairs appear at callsites `0x00DB847A` / `0x00DB84BB` and `0x00DA2A0A` / `0x00DA2A4B`.

At `0x00DB3D42`-`0x00DB3D58`, the body calls through slot `+4` of the vtable pointer at `[EDI]` only when byte `[ESP+0x1C]` and word `[EDI+0x14]` are both nonzero. The call sets `ECX=EDI` and pushes EBP; at `0x00DB3D5D`, AL is restored from BL, the saved earlier builder result. The indirect call contract remains unresolved.

## Limits

The log strings and append behavior support describing these as setup-record builders, while the stored words and copied lengths remain raw byte observations. The constructor layouts and callsites do not establish retainer-specific meaning, a wire schema, a server contract, or a receive-side response layout.
