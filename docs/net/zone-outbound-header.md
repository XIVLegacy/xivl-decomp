# Zone outbound opcode header binding

The retail Zone ClientPacketBuilder copies the low 16 bits of its value at
builder `+0x1C` to opcode offset `+2` of the 16-byte game-message header.
Its construction hook writes header tag `0x0014`; the selected finalization
hook preserves these bytes through submission to `FUN_00DAF920`. This binds
the native record value 2 to Map serverbound opcode `0x0002` on the published
forwarding path. It does not establish a login name, delivery or server reply.

## Input and exact reads

The input was retail 1.23b `ffxivgame.exe`, size 15996808 bytes, PE32 image
base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA with Ghidra 12.1.3 supplied metadata and exact reads, analysis-profile
digest `44bc792e0da2f944576805bbd496f5de8b85f040449b1daa16ebf6d298518d1b`.
Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked original
file backing, byte equality and complete instruction decoding. All addresses
are VAs. No client was executed.

| Complete body | Bytes | Instructions | SHA-256 |
| --- | ---: | ---: | --- |
| `[0x00DB06A0,0x00DB06F5)` | 85 | 37 | `7402418e268cd39aaf8340245d7a13f46406c52314493dc5e2ef015ccfa5fcc1` |
| `[0x00DC1BD0,0x00DC1BD4)` | 4 | 2 | `108eb05660dfe1a041240deb6da273082575a75017fa45a5f13e0086c713bc3d` |
| `[0x00DC1BE0,0x00DC1C0E)` | 46 | 15 | `8ce7c4678c7ad3348658bb295498478af63d369f977ad66aa755e62a5d2d58fc` |
| `[0x00DC1C10,0x00DC1C3E)` | 46 | 18 | `67165bc05379c30683d73ee92697865610554f4e3e26d4d6fc378c47836418c0` |
| `[0x00DBFC60,0x00DBFC65)` | 5 | 2 | `4ff0ad8d4237bea4f5c26fb637e6d0ce45eb12afba29cd0d28f48a2a147aae34` |
| `[0x00DBFCE0,0x00DBFCE5)` | 5 | 2 | `99712f5745d56904d51e658eeac81bb39a7a2acb4c8834e9af91b4ef58557a0d` |

The published final Zone builder vtable is `0x01129AE8`,
`xivl-client-structs:manifests/symbols.json#BCS-Y-0299`. Two exact four-byte
slot reads were checked against the original PE:

| Slot | Cell VA | Target | Cell SHA-256 |
| ---: | --- | --- | --- |
| 2 | `0x01129AF0` | `0x00DC1BE0` | `5677afd9d7401168c722e7a296ced78d894711bbd79f6bf50e3676d86a5bb24b` |
| 3 | `0x01129AF4` | `0x00DC1C10` | `f6687015940a19a2fa3e781e29fd88759ee5bd202125d4202538d10792cc0dae` |

## Construction and finalization

The [constructor write contract](local-pair-constructors.md) places the input
value at builder `+0x1C` in both `0x00DC1CF0` and `0x00DC1C60`. The published
`xivl-client-structs:structs/network/net-buffer-up.md` contract for
`0x00DAF850` zeroes the backing span for the assignment length and passes
its data pointer D to builder virtual slot 2. The same call edge is recorded
in [record construction sites](record-construction-sites.md#related-dispatch-helpers).
These published constructor and allocation bodies were reused, not reread.

Slot 2 target `0x00DC1BE0` receives the builder in ECX and D as its explicit
argument. Its writes are:

| Header location | Write | Locator |
| --- | --- | --- |
| D `+0` first byte | Forces `0x14` by mask `0xFFFFFF14` and OR `0x14` | `0x00DC1BE5..0x00DC1BEF` |
| D `+1` | Zero byte, completing tag `0x0014` | `0x00DC1BFB` |
| D `+2` | Low 16 bits of builder `+0x1C` | Load `0x00DC1BF1`, store `0x00DC1BF7` |
| D `+8` | Low 32 bits returned by `_time64(NULL)` | Call `0x00DC1BFF`, store `0x00DC1C07` |

The `_time64` target `0x009D5725` is independently identified in the
[wire finding](wire-protocol.md#server_utc-and-lobby-client-number-paths).
Its body and import bytes were not reopened. This header value is separate
from the application clock in [Map serverbound `0x0001`](periodic-map-0001.md).
The construction hook does not write D `+4..+7` or `+0x0C..+0x0F`; the
published preceding clear supplies their zeros on that path.

At `0x00DB06A0`, ESI holds the builder. The wrapper obtains its buffer through
`0x00DAE710`; the published forwarding path supplies zero adjustment and
reuses the [Fix contract](../arr-debug-comparison.md#zoneclient-outbound-packet-forwarding).
It calls builder virtual slot 1 at `0x00DB06BF`. The published
[slot catalog](../../config/ffxivgame.vtable_slots.jsonl) identifies the Zone
table's slot 1 as RVA `0x009C1BD0`, VA `0x00DC1BD0`. This complete four-byte
body returns builder `+0x10` without stores or calls; its slot cell was reused
from the catalog, not reread. The wrapper advances that result by four and
copies two metadata dwords into the NetBuffer object at `+0x0C/+0x10`.
These object metadata stores are not writes into D.

The wrapper then calls builder virtual slot 3 at `0x00DB06E4`, passing the
buffer's data pointer and length. It submits that buffer to `0x00DAF920` at
`0x00DB06EA`.

Slot 3 target `0x00DC1C10` passes the header's opcode halfword, D `+0x10`,
length minus `0x10` and zero to `0x00DBFC60`. It then passes D, `0x10` and
zero to `0x00DBFCE0`. The first helper only returns its third explicit stack
argument; the second only returns its second. Neither helper stores memory
or calls another function. The hook ignores both return values and has no
header stores, so this finalization preserves the constructed opcode.
The submission callee's queue, transport and delivery behavior is outside
this finding.

## Value 2 and remaining boundaries

The [Zone forwarding contract](../arr-debug-comparison.md#zoneclient-outbound-packet-forwarding)
passes the source record's value to the selected constructor: states 1 and 2
require value 2 and use `0x00DC1CF0`; state 3 uses `0x00DC1C60`. Both store
that value at builder `+0x1C`, and both install the vtable checked above.
Combined with these header writes, the value-2 record is a native producer
for the Map serverbound `0x0002` route, conditional on that forwarding path.

The [record builder](zone-value-two-record.md) has length `0x28` and a
24-byte application copy: one caller u32, 16 supplied bytes and four bytes
not written by that builder. With the 16-byte game-message header and the
separate 16-byte actor wrapper, this implies a 56-byte subpacket. This is
static geometry, not a newly observed capture witness.

The caller's source values and initialization of the final application u32
remain unresolved. The wire binding does not authenticate a login/handshake
name, a final-u32 selector meaning, a server response, admission policy,
successful transmission or client acceptance. A different channel's value 2
requires its own binding; the ARR enum is not a retail semantic authority.
