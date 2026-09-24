# Wrapper output attachment

This page records two paths through a native wrapper-output helper. The
function's object names and application role remain unknown; the addresses
and pointer relationships below are direct instruction observations.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for RVAs. All 68 listed instructions in
`asm/ffxivgame/009b5300_FUN_00db5300.s` were byte-compared with the pinned
PE. The helper starts at VA `0x00DB5300` (RVA `0x009B5300`).

## Direct paths

The helper reads a pointer from argument `+0x08`. If nonzero, it reads
that object's dwords at `+0x14` and `+0x18`; otherwise both are treated as
zero (`0x00DB5305`-`0x00DB5324`).

When both dwords are nonzero, it passes the address of the `+0x18` value
to `0x004E4B40`. A zero return ends the helper. A nonzero return is used
as ECX for `0x004E4BA0`, with the `+0x14` value as argument. If the dword
at the `0x004E4B40` result `+0x24` is nonzero, the helper calls that dword's
vtable slot at byte offset `+4` with the `0x004E4BA0` return and original
argument (`0x00DB532A`-`0x00DB5359`). A zero `+0x24` ends the helper.

When either extracted dword is zero, it calls vtable slot `+8` on the
object at receiver `+0x34`, passing receiver and the value at receiver
`+0x28` twice. It then calls receiver vtable slot `+0x60` with that result
and the original argument. If the first call returned a nonzero pointer,
it calls that pointer's vtable slot `+0` with literal argument `1`
(`0x00DB535C`-`0x00DB5385`).

The `+0x14` and `+0x18` values, lookup functions, and two vtable calls
do not establish a retail result schema or a network opcode. The fallback
path is selected by missing extracted values, not by failure of
`0x004E4B40` or a zero target at `+0x24`.
