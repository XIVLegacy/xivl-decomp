# 0x017C block normalization and allocation paths

The inbound route through `0x006CC620` to `0x006CC070` is documented in the
[group queue trace](../event/group-shared-work-system.md#1-type-tagged-queue-and-conditional-work-creation)
and [wire-protocol notes](wire-protocol.md#type-tagged-high-opcode-route). This
page records the narrower instruction-level behavior at `0x006CC5B0` and
`0x006CC070`: the first copies a `0x78`-byte block, changes the copied word at
`+0x74` to `1`, and passes the copy onward. The second has a guarded
`+0x10 == 0x0E` branch that requests `0x50` bytes and a general branch that
requests `0x40` bytes. The observations below name fields by offset and callees
by address; they do not identify higher-level field semantics.

## Copy and replay

At VA `0x006CC5B0`, the function reads the input word at `+0x74`. If the signed
word is greater than `1`, it stores `1` at offset `+0x0A` relative to entry `ECX`. It
then copies `0x1E` dwords (`0x78` bytes) into a stack buffer, writes `1` to the copy's
word at `+0x74`, and calls `0x006CC070` with the copy as an argument. The call
sets `ECX` to the dword at offset `+0x00` relative to the entry `ECX` value of
`0x006CC5B0`.

## Branches in `0x006CC070`

The function compares the input dword at `+0x10` with `0x0E`. For that value,
it passes the address at `+0x28` and the word at `+0x74` to `0x006C2640`. If
that call returns nonzero, it requests `0x50` bytes from `0x009D1B35`; when the
allocation is non-null, it calls `0x006C4440` with the address at `+0x28`. It
then calls `0x007238B0` with `ECX` set to the context pointer plus `0x08`.

The other path is also taken when the `0x006C2640` call returns zero. Its
observed input accesses are:

| Input offset | Direct observation |
|---|---|
| `+0x10` | Dword compared with `0x0E` and later tested against zero. |
| `+0x18`, `+0x1C` | Dwords compared with the pair at `+0x28`, `+0x2C`; one branch passes this pair to `0x006BF420`. |
| `+0x20`, `+0x24` | Alternate dword pair passed to `0x006BF420` on another branch. |
| `+0x28`, `+0x2C` | Dwords used in the pair comparison; the address at `+0x28` is also passed onward. |
| `+0x30` | Dword loaded for the call to `0x006CBEE0`. |
| `+0x38` | Address passed to `0x006CBEE0`. |
| `+0x40`, `+0x44` | The dword at `+0x40` selects a helper path; the address at `+0x44` is passed to `0x00447260`. |
| `+0x64`, `+0x74` | Addresses passed to `0x006D79D0`; the word at `+0x74` is also loaded for the `0x006CBEE0` call. |

This path requests `0x40` bytes from `0x009D1B35`. On the non-null allocation
arm it calls `0x006CBEE0` with values and addresses derived from the input and
keeps that call's return value. The null arm sets the current value to zero.
Both arms reach the later `0x006C4330` call. The function then calls
`0x007238B0` with the context pointer plus `0x08`, and `0x006D75F0` with the
context pointer plus `0x30`. For that latter call, it assembles a three-dword
argument from input offsets `+0x00`, `+0x08`, and `+0x0C`, then writes the
current `EDI` value through the returned slot. The `0x50` path also
calls `0x006D75F0` with the context pointer plus `0x30` and writes the current
`EDI` value through the returned slot.

## Child-record setup

After earlier gates, including a zero result and an owner `+0x0C` zero check,
`0x006CC620` requests `0x0C` bytes at VA `0x006CC697`. If non-null, that
allocation is passed to `0x006C6CA0` by the direct call at VA `0x006CC6B4`;
the returned pointer (or zero on allocation failure) is then passed to
`0x006D1DC0` with `ECX` set to owner `+0x0C`. `0x006C6CA0` requests `0xFC`
bytes from `0x009D1B35`. On success, it calls `0x006C5E80` with the allocation
in `ECX` and its stack argument; otherwise it uses zero. It stores that value
at record `+0x00`, writes zero at `+0x04`, sets byte `+0x08` to `1`, and sets
bytes `+0x09` and `+0x0A` to zero. It returns the outer record pointer in
`EAX` and does not explicitly write byte `+0x0B`. These instructions establish
allocation and initialized offsets, not a higher-level type or helper purpose.

## Evidence

Addresses in this note are virtual addresses (VA). The observations come from
Ghidra instruction listings exported with the repository's
[`DumpFunctions.java`](../../tools/ghidra_scripts/DumpFunctions.java) script
from `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
instruction bytes at function ranges beginning at VAs `0x006CC5B0`,
`0x006CC070`, `0x006CC620`, and `0x006C6CA0` were checked against that
executable.
