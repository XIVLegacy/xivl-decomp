# Cut begin viewport flag and time-of-day switch

The cut begin routine `0x00611C30` sets a viewport flag that the per-frame
present reads to letterbox the frame to 16:9, and it switches the world clock
readers to the alternate clock. The cut end routine `0x00611D60` clears the
flag. This page records those two stores, their readers and the selector-0
consumer that runs the begin. Neither store reaches an actor visibility field
in the traced code.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. Addresses are VAs. Observations come from ghidra-cli
disassembly listings and cross-reference queries on that program. Tiers:
STATIC = read from the cited instructions, INFERRED = interpretation, UNKNOWN
= not traced.

## Viewport flag

STATIC. `0x00A47130` stores its stack argument at `[ECX+0x38]` and returns
with `RET 4`. `0x00A47140` copies `[ECX+0x38]` to its out pointer and returns
that pointer. Both run on the object at global `0x013654FC`.

| Caller | Callsite | Value |
| --- | --- | --- |
| cut begin `0x00611C30` | `0x00611C56` | literal 1, written at `0x00611C4A` |
| cut end `0x00611D60` | `0x00611FB9` | EBX, zeroed at `0x00611D65` and not written again in the routine |

The only caller of the getter `0x00A47140` is the present `0x00A52C50` at
`0x00A52ECD`. Before the call it reads the back buffer width and height from
`u32(u32(0x01365568)+0x3C)+0x14/+0x18` and forms `EDI = (9 * width) / 16`
with signed rounding toward zero (`0x00A52EA4..0x00A52ECA`). The draw takes
the centred rectangle branch only when the getter value is nonzero and `EDI`
differs from the height (`0x00A52ED2..0x00A52EDE`). The branch at
`0x00A52EE7` selects bars at top and bottom or at the sides by comparing
`EDI` with the height. The rectangle path and the full-size blit are in
[Present blit gamma and dither](present-gamma-blit.md#call-path-v).

INFERRED. The word at `+0x38` is a cinema 16:9 viewport flag that the cut
begin sets and the cut end clears. A back buffer whose 16:9 height equals its
own height draws full size with the flag set.

## Time-of-day switch

STATIC. After the flag store the cut begin calls `0x0060B360` on the object
at `+0x114` and passes `u32(result+0x78)` to `0x0060FFC0` with ECX set to
`this+0x340` (`0x00611C89..0x00611CB4`). Let `A` be that argument and
`W = u32(A+0x158)`. `0x0060FFC0` saves `u8(W+0x1A7)`, `u32(W+0x1A8)` and the
16 bytes that `0x007E03C0` returns into its own record, then calls
`0x007E0410` on W with a pointer to the literal 1 (`0x00610023..0x0061002B`).

`0x007E0410` stores the pointed dword at `[ECX+0xF4]`. Its three callers are
`0x0061002B` (the begin path above), `0x00610094` and `0x0062CE9A`. The call
at `0x00610094` is in `0x00610040`, which the cut end calls at `0x00611E3C`.
On the arm where bit 2 of its argument is set (`0x0061004F`), that routine
writes 0 through `0x007E0410` (`0x00610089..0x00610094`). The cut end passes
0 as that argument when its `+0x1E8` word equals 4 (`0x00611E2B..0x00611E3B`).

`0x007E0430` returns `W+0x120` when `u32(W+0xF4)` is nonzero and `W+0x108`
otherwise. `0x007E0450` returns `W+0x108` and `0x007E0460` returns `W+0x120`.
The callers of `0x007E0430` are `0x007FCBF0`, `0x00809D50`, `0x0080AF10`,
`0x00824270`, `0x008D1D60` (two callsites) and `0x008D3400`. `0x00824270`
divides the selected 64-bit value by 60, 60 and 24 through `0x009D5880` and
returns one of four fields by its selector argument (`0x00824280..0x00824300`).

INFERRED. W is the weather and clock owner of
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#time-of-day),
`+0x108` the field clock and `+0x120` the alternate clock. The cut begin
switches the clock readers to the alternate clock. The cut end returns them to
the field clock only on its masked arm.

## Selector-0 consumer

STATIC. `0x0061C4D0` copies the play record into the owner and calls the cut
begin at `0x0061C587`. When the begin returns nonzero it copies 32 bytes from
the record start to M+`0x181` and to M+`0x1A1`, zeroes M+`0x1C1..0x1E0`, and
calls `0x0061BE00` with M+`0x181` and ECX set to `u32(M+0x4F0)` at
`0x0061C625`. `0x0061BE00` builds a path from the name through `0x0061B020`
and two literals and, when `0x00D39290` succeeds, calls `0x006320C0(1, path,
0)` (`0x0061BEBA..0x0061BEF8`). It then clears `+0x28` and calls `0x006323D0`.
The begin returns zero without these copies when byte `+0x180` is already set
(`0x00611C37..0x00611C46`), and sets that byte at `0x00611C66`.

UNKNOWN. Which load state of the `0x006320C0` request the scheduler start
waits for, and what keeps the map's own characters out of the drawn cut. The
traced begin stores reach the viewport flag, the clock selector and the owner
fields listed here.

## Reproduction

Run `ghidra disasm` on `0x00A47130`, `0x00A47140`, `0x00611C30`,
`0x00611D60`, `0x00A52E9A`, `0x0060FFC0`, `0x00610040`, `0x007E0410`,
`0x007E0430`, `0x00824270`, `0x0061C4D0` and `0x0061BE00`. Run
`ghidra x-ref to` on `0x00A47130`, `0x00A47140`, `0x007E0410` and
`0x007E0430` for the caller sets above.
