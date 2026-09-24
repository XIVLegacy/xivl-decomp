# 1.23b native receive pump and state helper

This finding records two instruction paths in retail 1.23b `ffxivgame.exe`.
It describes calls, offsets, and dispatch targets without assigning application
or wire meanings.

## Binary and method

The executable SHA-256 is
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. Its PE32
image base is `0x00400000`. Ghidra 12.1 disassembly was checked at the VAs below;
the vtable entries are also recorded in the
[vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl) at RVA
`0x00D29360` (rows 84009-84018).

## VA `0x004E2670`

The body follows the pointer at `this+0x238` to offset `+0x70` and, when that
pointer is nonzero, calls `0x00DB41E0` (`0x004E26AB`). It initializes a local
record through `0x004E3F60`, then repeatedly calls `0x00DB4460` with that
record (`0x004E26D5`). A false return exits the loop. On a true return, the
body reads the record's pointer at `+8`. If it is nonzero, the body reads
that object's pointer at `+0x24`; otherwise it uses zero. It passes the
selected pointer to `0x004D8D10` with ECX set to the pointer stored at
`this+8`, advanced by `0x10` (`0x004E26DE`-`0x004E26F4`). It then calls
`0x004E6080` for the fetched object
and returns to the loop (`0x004E26F9`-`0x004E270E`).

`0x00DB4460` first calls `0x00DB6D20` at `0x00DB448B`. A zero AL result
returns zero; the nonzero branch calls `0x00E40620`, then writes vtable
`0x01129360` into its temporary object at `0x00DB449D` before calling
`0x00E40630` at `0x00DB44B4`. The latter reads a 16-bit word
at offset `+2` of a nested record and uses it to select an indirect call
through a vtable slot. At the active vtable VA `0x01129360`, the catalog
records offsets
`+0x08` through `+0x24` in four-byte slot steps, targeting
`0x00E3FE10` through `0x00E3FE80` in `0x10`-byte target steps. Each
target's disassembly is a three-byte `ret 0x0C` stub.
These observations establish a dispatch table and its stub targets, not the
word's meaning or the callback contract.

After that dispatch, `0x00DB4460` follows the fetched object's pointer at
`+8` and then the pointer stored at its `+0x24`. When the word at that
payload pointer `+2` equals `2`, it writes `3` to owner `+0x8C`
(`0x00DB44B9`-`0x00DB44D8`). The success path returns one even when the
word differs; no application meaning is assigned to either value.

A separate caller at `0x00DB4300` also calls `0x00DB6D20`
(`0x00DB4353`). On its nonzero return path it conditionally resolves
owner `+0x88` through `0x004E4B40` and `0x004E4BA0`
(`0x00DB4395`-`0x00DB43BE`), and later passes its fetched object to
`0x004E6080` (`0x00DB440F`-`0x00DB4417`). These are caller and
lifetime edges; they do not identify the payload or consumer role.

## VA `0x00DB4020`

This body returns false unless the dword at owner offset `+0x8C` equals `1`
(`0x00DB4048`-`0x00DB4053`). It checks owner `+0x88`, resolves the nonzero
value through calls to `0x004E4B40` and `0x004E4BA0`, then pushes `0`,
`0x38`, and `2` before calling `0x00E40A60`
(`0x00DB409D`-`0x00DB40A7`). It next calls
`0x00DB5010`; only a nonzero return reaches `0x00DB5A90` and the later success
path. That path references the `Send Packet :` literal, performs additional
calls, and stores `2` at owner `+0x8C` (`0x00DB40E2`-`0x00DB4113`). The return
value reports whether owner `+0x8C` equals `2` (`0x00DB4130`-`0x00DB4137`).

## Limits

These are static observations from the pinned executable. The source labels
"receive pump" and "state helper" describe the paths only. The catalog
identifies the active temporary vtable as
`Application::Network::ChatClient::ChatProtoDownDummyCallback`; this
does not identify the fetched object's application role, wire packet
meaning, or runtime behavior.
