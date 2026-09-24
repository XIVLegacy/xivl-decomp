# Stream receive entry paths

This page records the direct gates and calls in two stream dispatcher callers,
plus selected call mechanics in helper `0x00DB3300`. Their timing policies,
owner types, and application roles remain unknown.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for RVAs. All 127 listed instructions in
`asm/ffxivgame/009b28e0_FUN_00db28e0.s` were byte-compared with the pinned
PE. The function starts at VA `0x00DB28E0` (RVA `0x009B28E0`). All 82
listed instructions at VA `0x00DA1AB0` (RVA `0x009A1AB0`) were checked in
the same way against `asm/ffxivgame/009a1ab0_FUN_00da1ab0.s`.

## Caller at `0x00DA1AB0`

Its setup call first requires a nonzero owner `+0x110` pointer with a zero
dword at that pointer `+4`. When owner `+0x130` is nonzero, it calls
`0x009D5725` with literal zero, subtracts owner `+0x118` from the return,
and skips setup if the unsigned result is below `+0x130`
(`0x00DA1AD6`-`0x00DA1B05`). It calls `0x00DA29C0` with the `+0x110`
pointer and literal arguments `0`, `1`, `3`, and `0`. A nonzero byte
return causes a call to `0x004D0760` with ECX at owner `+0x118` and a
write of `5` to owner `+0x130` (`0x00DA1B07`-`0x00DA1B2C`).

The function then calls `0x00DB3300`. If owner `+0x110` is nonzero and
that object's `+4` is zero, it initializes a local wrapper and calls
`0x00DA25D0` at `0x00DA1B7E`. A nonzero dispatcher byte result and
nonzero output pointer lead to `0x00DA2230`, with ECX loaded from output
object `+8` and the output object passed on the stack (`0x00DA1B83`-
`0x00DA1B98`). It then clears the local pointer and cleans up the
wrapper. If the `+0x110` pointer exists, it calls `0x00D3D690` with ECX
at that pointer `+0x4C` (`0x00DA1BB9`-`0x00DA1BC6`). The normal return
sets `AL=1` even if setup or dispatch did not succeed.

## Caller at `0x00DB28E0`: setup gate and stored pair

When either owner `+0xF8` or `+0xFC` is nonzero, the function calls
`0x009D5725` and compares its returned EDX:EAX pair with those stored
dwords (`0x00DB2908`-`0x00DB2941`). The high-dword comparison uses signed
branches; an equal high dword uses an unsigned low-dword comparison. The
path at `0x00DB2947` continues only when the returned pair is at least the
stored pair under those comparisons. When both stored dwords are zero, it
reaches that same path directly.

The setup call requires a nonzero owner `+0xF4` pointer, zero at that
pointer `+4`, and a nonzero byte return from its vtable slot `+0x10`
(`0x00DB2947`-`0x00DB295F`). It calls `0x00DB3CB0` with the pointer at
`+0xF4`, literals `0`, `1`, and `2`, and the address of owner `+0xE4`
(`0x00DB2961`-`0x00DB297D`). A zero byte return skips the stored-pair
update.

On a nonzero byte return, the function obtains a pair through
`0x009D5725` if its earlier local pair is still zero. It uses the word at
owner `+0xE0` in a multiply and shift sequence, adds that result to the
pair, writes the two dwords to owner `+0xF8/+0xFC`, and writes `1` to
owner `+0x80` (`0x00DB297F`-`0x00DB29C3`). The instruction sequence does
not by itself establish a clock unit for `+0xE0`.

## Caller at `0x00DB28E0`: dispatch and follow-on call

The function calls `0x00DB3300` even when the setup gate is skipped
(`0x00DB29CD`). If owner `+0xF4` is nonzero and that object's `+4` is zero,
it initializes a local wrapper and calls `0x00DB3880` at `0x00DB2A0C`.
Only a nonzero dispatcher byte result together with a nonzero pointer in
the local wrapper reaches `0x004E6080` (`0x00DB2A11`-`0x00DB2A26`).
The call uses ECX loaded from output object `+8` and passes the output
object as its stack argument. The local pointer is then zeroed and the
wrapper cleaned up through `0x00DC1DF0`.

A separate final path checks the `+0xF4` pointer and its dword `+0x84`.
When both tests permit it, the function enters a lock at that pointer
`+0x10A0`, calls `0x00D3D690` with ECX at that pointer `+0x4C`, and
releases the lock (`0x00DB2A3F`-`0x00DB2A67`). The normal return writes
`AL=1` at `0x00DB2A6D`; that return does not prove the setup call or
dispatcher succeeded.

No direct incoming reference to either function was decoded in the
screened output. Their exact callers and any retail workflow association
remain open.

## Helper body at `0x00DB3300`

The body prepares pointers derived from `ESI+0x5C`/`ESI+0x60` and
`ESI+0x40`/`ESI+0x44`,
then calls through the pointer at `0x00F3E16C` for each pair
(`0x00DB332F`-`0x00DB3350`). It calls `0x00DB3280` at `0x00DB336A`, then calls
through the pointer at `0x00F3E168` with the `+0x44` and `+0x60` pointers
(`0x00DB336F`-`0x00DB337C`). The indirect-call contracts and the roles of
these owner offsets remain unknown.
