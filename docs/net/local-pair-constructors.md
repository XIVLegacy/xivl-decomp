# Local pair constructor variants

This page records instruction-level initialization at two related bodies, a
tail-jump body, and two shared child helpers. Their object identity and
application role remain unresolved.

## Binary and method

Input: retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is `0x00400000`. The VAs below were mapped through the PE
section table with `pefile` 2024.8.26 and decoded as x86-32 instructions with
Capstone 5.0.7.

## Bodies at `0x00DC1C60` and `0x00DC1CF0`

Both bodies use the incoming `ECX` pointer as their destination and return it
in `EAX`. They first write `0x01129AD4` at destination `+0`, call
`0x00DC1E00` with `ECX` set to destination `+8`, write `0x01129ACC` at `+4`,
and zero `+0x0C` (`0x00DC1C89`-`0x00DC1CA0` and
`0x00DC1D19`-`0x00DC1D30`). Each then replaces the dword at `+0` with
`0x01129AE8` and calls `0x00DC1EA0` with `ECX` set to destination `+0x10`
(`0x00DC1CB0`-`0x00DC1CBC` and `0x00DC1D40`-`0x00DC1D4C`).

After that call, both bodies write a dword read from a stack slot to
destination `+0x1C`: the stores are at `0x00DC1CC9` and `0x00DC1D55`, after
the loads at `0x00DC1CC1` and `0x00DC1D51`. The body at `0x00DC1C60`
additionally loads a pointer from a stack slot at `0x00DC1CC5` and copies its
first two dwords to destination `+0x14` and `+0x18` at `0x00DC1CCE` and
`0x00DC1CD4`. The body at `0x00DC1CF0` has no subsequent stores to `+0x14`
or `+0x18`, so those two dwords retain the zeros written by `0x00DC1EA0`.

## Shared child helpers

At `0x00DC1E00`, the body moves `ECX` to `EAX`, writes zero to the dword
pointed to by `EAX`, and returns (`0x00DC1E00`-`0x00DC1E08`). In the two
callers above, this zeroes only the dword at destination `+8`.

At `0x00DC1EA0`, the body reads two 16-bit values from stack offsets `+4`
and `+8`, stores them at the object pointed to by `ECX` at offsets `+0` and
`+2`, zeroes dwords at `+4` and `+8`, then returns while removing eight bytes
of stack arguments (`0x00DC1EA0`-`0x00DC1EBB`). Its callers pass
`destination+0x10`, so the observed writes are at destination `+0x10`,
`+0x12`, `+0x14`, and `+0x18`. The `0x00DC1C60` body later overwrites the
last two dwords; the `0x00DC1CF0` body does not.

The separate body at `0x00DC1C40` writes `0x01129AD4` at `[ecx]` and
`0x01129ACC` at `[ecx+4]`, advances `ECX` by 8, then jumps to
`0x00DC1DF0`, whose sole instruction is `ret`. Its body is 21 bytes and the
tail target is one byte. This records the emitted stores and control flow
without assigning a destructor or cleanup role. See
`asm/ffxivgame/009c1c40_FUN_00dc1c40.s:6-9` and
`asm/ffxivgame/009c1df0_FUN_00dc1df0.s:6`; source note
`FF14-Memory/tools/outputs/lpb/native_retainer_dispatch_children_20260617/child_notes/child_00DC1C40_packet_pair_cleanup.md`
has SHA-256 `005f3ff4685169385f10fdff4ccd33a35dc94c90b97dec9bb7ab438fe5a290cd`.

## Limits

The constants and field writes above do not establish a C++ type, vtable
identity, or application behavior. Labels that associate these bodies with a
packet pair, state value, or particular workflow remain hypotheses; this
instruction evidence does not establish those meanings.
