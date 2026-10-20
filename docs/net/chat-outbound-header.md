# Chat outbound header and fixed value

The retail Chat ClientPacketBuilder writes header tag `0x0014`, copies the
low 16 bits of builder `+0x1C` to header opcode `+2`, and writes literal
u32 `10` at header `+8`. This constructor has no clock call. Its accessor
and finalization hooks do not change those header bytes. The class-specific
binding supports the published World serverbound record values `0x0002`
and `0x012C`; their narrower server behavior remains unresolved.

## Input and exact functions

The input was retail 1.23b `ffxivgame.exe`, 15996808 bytes, PE32 image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA/Ghidra 12.1.3 supplied exact metadata and reads, analysis profile
`44bc792e0da2f944576805bbd496f5de8b85f040449b1daa16ebf6d298518d1b`.
Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 independently
checked original PE backing, byte equality and complete decoding. These
are static observations; no client was run. All addresses below are VAs.

| Complete body | Bytes | Instructions | SHA-256 |
| --- | ---: | ---: | --- |
| `[0x00E409D0,0x00E409D4)` | 4 | 2 | `108eb05660dfe1a041240deb6da273082575a75017fa45a5f13e0086c713bc3d` |
| `[0x00E409E0,0x00E40A07)` | 39 | 10 | `d8add7a55442a02d62dea4aaeedca50b72727acaf4abfbe09d222e5a93f6506b` |
| `[0x00E40A10,0x00E40A3E)` | 46 | 18 | `5f7cb1a9d139134499f8cc65a2412f0dd9e543cd914cac068d1d1589ce85cb27` |
| `[0x00E40580,0x00E40585)` | 5 | 2 | `4ff0ad8d4237bea4f5c26fb637e6d0ce45eb12afba29cd0d28f48a2a147aae34` |
| `[0x00E40600,0x00E40605)` | 5 | 2 | `99712f5745d56904d51e658eeac81bb39a7a2acb4c8834e9af91b4ef58557a0d` |

The [published slot catalog](../../config/ffxivgame.vtable_slots.jsonl)
identifies `Application::Network::ChatProtoChannel::ClientPacketBuilder`
table RVA `0x00D3E8D0`, VA `0x0113E8D0`. Slots 1, 2 and 3 point to
`0x00E409D0`, `0x00E409E0` and `0x00E40A10`, respectively. These exact
catalog rows and the constructor's final vtable identity in
`xivl-client-structs:manifests/symbols.json#BCS-Y-0302/BCS-Y-0303`
were reused; no original vtable cells were read here.

## Construction and hook effects

The published constructor `0x00E40A60` stores its supplied value at builder
`+0x1C`. The [buffer-building contract](stream-pump-and-state.md#additional-state-and-object-paths)
for `0x00DB5010` clears the supplied extent and invokes builder slot 2 with
the data pointer D. The header function `0x00E409E0` receives the builder in
ECX and D as its explicit stack argument.

| Header location | Write | Locator |
| --- | --- | --- |
| D `+0` first byte | Forces `0x14` with mask `0xFFFFFF14` and OR `0x14` | `0x00E409E4..0x00E409EF` |
| D `+1` | Zero byte, completing tag `0x0014` | `0x00E409F9` |
| D `+2` | Low 16 bits of builder `+0x1C` | `0x00E409F1..0x00E409F5` |
| D `+8` | Literal u32 `10` | `0x00E409FD` |

The function contains no call. In particular, the value at `+8` is supplied
by the literal store, not a clock reading. The separately established
[Zone header](zone-outbound-header.md) has its own clock-producing path;
one channel's field must not be inferred from another channel's capture or
from a generic timestamp-named API member.

Slot 1 target `0x00E409D0` returns builder `+0x10` with no stores or calls.
The published wrapper `0x00DB5A90` uses that result plus four to copy two
dwords into NetBuffer object metadata `+0x0C/+0x10`; those are distinct from
data-header writes. It also invokes slot 3 with D and the buffer length.
The slot 3 function has no memory stores and calls only `0x00E40580` and
`0x00E40600`. The first only returns its third explicit stack argument; the
second only returns its second. Neither writes memory or calls another
function. Thus these virtual hooks preserve the constructed opcode and
fixed `+8` value. The published wrapper's buffer-replacement helper
`0x00DB4680`, subsequent submission `0x00DB50E0`, allocation and transport
were not reopened; this finding does not prove successful transmission.

## Published producers and field boundaries

`xivl-client-structs:manifests/structs.json#BCS-S-0373` and its C475 method
record own the complete 40-byte application for World serverbound `0x0002`.
The callback producer `0x004DF9A0` writes a zero leading dword and 36 supplied
bytes. The dedicated producer `0x00DB4020` writes leading `0x00003C6B`.
They use the same Chat constructor value 2 and size `0x38`; the header store
above supplies its numeric opcode binding. This statically implies a
56-byte game message and 72-byte actor-wrapped subpacket, not a new captured
witness. The supplied block does not authenticate individual identity fields
or a universal leading value.

`xivl-client-structs:manifests/structs.json#BCS-S-0374` owns the eight-byte
application for World serverbound `0x012C`. The same callback's global-state-1
branch supplies value `0x012C`, record size `0x18`, and leading application
u32 `1`; its closing four bytes have no documented writer. The published
`0x00DB3E30` path passes that value to the Chat constructor. Its static
game-message size is 24 and actor-wrapped subpacket size is 40; neither is a
capture observation. The record size must not be used as a subpacket length.

These producer layouts and caller branches were reused, not reread. The
callback's narrower operation, static-block meanings, closing-word
initialization, complete server lifecycle, response policy, delivery and
client acceptance remain unresolved. The fixed header word is not evidence
of any of those behaviors.
