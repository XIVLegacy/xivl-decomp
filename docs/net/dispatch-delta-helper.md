# Signed delta helper

This page records the signed-delta branches of a helper that updates a dword
at offset `+0x20`. The identity and role of the receiver and updated value
remain unknown.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for RVAs. All 40 listed instructions in
`asm/ffxivgame/009ae710_FUN_00dae710.s` were byte-compared with the pinned
PE. The function starts at VA `0x00DAE710` (RVA `0x009AE710`).

## Direct branch behavior

The helper reads a signed dword argument and takes the pointer at receiver
`+0x08` as its working object. A negative argument is negated and compared
unsigned with the dword at working object `+0x20`. If the stored value is
smaller than the negated argument, the helper writes zero to working object
`+0x20` and receiver `+0x08`, then returns the working pointer
(`0x00DAE719`-`0x00DAE736`). Otherwise the negative value continues to the
update path.

For a positive argument, the helper adds it to working object `+0x20`. If
the resulting dword is unsigned-greater than `0x898`, it takes the same two
zero writes and returns (`0x00DAE73B`-`0x00DAE748`, `0x00DAE72B`-
`0x00DAE736`). Zero skips the update callback. A nonzero accepted delta
is added to working object `+0x20`, then passed to the working object's
vtable slot at byte offset `+8` (`0x00DAE74A`-`0x00DAE759`). The helper
then clears receiver `+0x08` and returns the working pointer
(`0x00DAE75B`-`0x00DAE763`).

These branches establish the signed negative-delta and unsigned `0x898`
threshold checks in this body. They do not establish a queue, cursor,
packet, or commit meaning for either field or the virtual call.
