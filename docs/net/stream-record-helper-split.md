# 1.23b stream-record helper split

This finding complements the [stream-record parser pair](stream-record-parser-pair.md) with instruction-level observations for two helper bodies. It does not restate the record layouts or assign application-level names.

## Binary and decode

The input was retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Its PE32 image base is `0x00400000`. Capstone 5.0.7 decoded each body in
x86-32 mode after mapping the VA through the PE section table. The spans stop
after the final `ret` and exclude the following `int3` byte.

| Body VA (end exclusive) | RVA | File offset | Bytes | Instructions | Body SHA-256 |
|---|---:|---:|---:|---:|---|
| `0x00DB4E30..0x00DB4E97` | `0x009B4E30` | `0x009B4E30` | 103 | 39 | `fed05d28c3161a984782cf34a582c04518f0c70f413379f8e372570fe8925c16` |
| `0x00DAF430..0x00DAF5A1` | `0x009AF430` | `0x009AF430` | 369 | 134 | `05a2a8409a886e1a673acb4bbbc65f90e2c604d8de81ce9b7ba6fb6968ae54f8` |

## VA 0x00DB4E30

The body reads the dword at entry `[esp+8]` into EAX, then adds the dword
at entry `[esp+4]` to EAX (`0x00DB4E30`, `0x00DB4E3A`,
`0x00DB4E3E`). It compares the resulting 32-bit value with `0x230` and
uses unsigned `JBE` at `0x00DB4E47`. The add has no separate carry check.

For a result at or below `0x230`, the branch reaches the direct call to
`0x00DB4DA0` at `0x00DB4E8B`. A larger result falls through to the direct
call to `0x00DB4830` at `0x00DB4E4B`. These bytes establish the arithmetic,
threshold, and call destinations only. The meanings of the inputs and
destination functions remain unresolved; this finding assigns neither an
allocator contract nor retainer behavior.

## VA 0x00DAF430

The body saves its incoming owner pointer in ESI and loads its second stack
argument, the output pointer, into EDI (`0x00DAF457`, `0x00DAF459`). When
the output pointer's dword at `+4` is zero, it reads a pointer at owner offset
`+8`, follows its first dword as a vtable, loads slot `+8`, and calls
through it (`0x00DAF45D` through `0x00DAF474`). A nonzero return is stored
at output `+4` (`0x00DAF476` through `0x00DAF47E`). Later, if owner
`+8` is nonzero, the body calls that object's vtable slot `+0x10` with the
dword at output `+4` (`0x00DAF536` through `0x00DAF548`). These are
observed offsets and indirect calls; the object and slot identities are
unresolved.

Near the end, the body loads the owner's pointer at `+0x78`
(`0x00DAF54A`). If it is nonzero, the body sets ECX to output `+8` and
directly calls `0x00DB2E00` at `0x00DAF55E`. A nonzero AL return skips the
fallback. If the `+0x78` pointer is null, or the call returns zero, the body
sets ECX to output `+8`, pushes the dword at output `+4`, and directly
calls `0x00DB2D30` at `0x00DAF56E`. The branch and both call destinations
are explicit in the decoded instructions; the callee contracts remain
unresolved.

## Factory threshold callers

At VA `0x00DAF210`, the body adds its two stack dwords and compares the 32-bit
result with `0x1C10` (`0x00DAF21E` through `0x00DAF220`). Unsigned `JBE` at
`0x00DAF227` selects a direct call to `0x00DAF110` at `0x00DAF26B` for results
at or below the threshold. Above it, the body calls `0x00DAEC70` at
`0x00DAF22B`, then calls `0x004E5CA0` at `0x00DAF253` with `ECX` set to owner
`+0x14`.

VA `0x00DB3430` has the same instruction shape with threshold `0x238`. Unsigned
`JBE` at `0x00DB3447` selects `0x00DB33A0` at `0x00DB348B`. For larger results,
the body calls `0x00DB31D0` at `0x00DB344B`, then `0x004E5CA0` at
`0x00DB3473` with `ECX` set to owner `+0x14`. In both bodies, the compare
overwrites the addition flags; no separate carry branch follows the addition.
On the larger-result path in `0x00DB3430`, calls through slots `+4` and `+8`
of the vtable pointer at `[ESI]` also bracket the `0x004E5CA0` call, with
`ECX=ESI` at both indirect calls (`0x00DB3450`-`0x00DB3459`,
`0x00DB3478`-`0x00DB347F`). Their contracts remain unresolved.

## Limits

These observations do not identify a packet opcode, application meaning, C++
type, or retainer behavior. They record the two function bodies, their memory
offsets, branch condition, and direct call destinations from the pinned
executable.
