# Map 0x00E2 selector and state writes

Map clientbound game-message `0x00E2` reaches CharaElement's synchronous
apply case. The local-actor path reads a u32 selector at application `+0`.
Selectors `0x15` and `0x16` call the published pre/post-warp bridges and
leave the case before its state writes. Other local selectors share one
state path. A nonlocal actor skips the selector read and takes only the
final parent-owned state write. The packet's semantic name remains unproved.

## Input and routing

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA 6.4.0 with Ghidra 12.1.3 supplied bounded byte reads and function extents.
Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 authenticated original
raw backing and decoded the selected instruction union.

The existing Element route and CharaElement slot-9 owner are recorded in
`xivl-client-structs:manifests/symbols.json#BCS-Y-0540` and the
[receive-dispatch ranges](receive-dispatch-ranges.md). In `0x0058CCA0`,
EDI retains entry ECX and ESI receives the packet argument. The opcode is
the u16 at game-message `+2`; application offsets subtract its 16-byte
header. The entry predicate `0x004D8830` either bypasses the filter or
selects its whitelist. Opcode `0x00E2` passes both routes:

| Selected lookup | Index and value | Destination |
| --- | --- | --- |
| Entry filter byte | `[0x0058D6B0+(0xE2-0x0F)] = 0` | Dword `[0x0058D6A4] = 0x0058CD21` |
| Main low-switch byte | `[0x0058D7E4+(0xE2-0x0F)] = 0x12` | Dword `[0x0058D788+4*0x12] = 0x0058CD81` |

Only these two byte cells and two dword cells were read, not complete tables.
The selected case compares the Element actor ID with the parent container's
self actor ID, using the published `0x004D6750/0x004D7490` getters
(`BCS-Y-1837/1834`). The parent pointer comes from CharaElement `+0x7C`.
Those getter bodies and the entry-predicate body were not reopened.

## Conditional effects

Let C be the CharaElement, P be `[C+0x7C]` and S be the full u32 at
application `+0`. Under valid storage and normal call returns:

| Actor and selector | Ordered case behavior |
| --- | --- |
| Local, S=`0x15` | Call `0x00574ED0` with receiver `[C+0x80]` and argument `0x15`; jump to the shared exit. |
| Local, S=`0x16` | Call `0x00574EE0` with receiver `[C+0x80]` and argument `0x16`; jump to the shared exit. |
| Local, any other S | OR bit 0 into byte `[C+0x26D]`; supply P and tag `0x4FE` to `0x00586F40`; after its return, get `[P+0x4E0]` through `0x004D7390` and store byte 1 at that returned pointer `+0xBC`; exit. |
| Nonlocal | Do not read S. Get `[P+0x4E0]` and store byte 1 at the returned pointer `+0xBC`; exit. |

The full selector is loaded at `0x0058CD96` with no byte mask, byte swap or
length check. For example, `0x00000115` takes the other-selector path,
rather than the `0x15` bridge. The local path requires at least four
application bytes. No selected instruction consumes application `+4..+7`.
The final `+0xBC` store is conditional on the branch and helper returns;
it is not unconditional across the `0x15/0x16` cases and is not a field
of CharaElement itself. The selected code does not null-check its result.

The existing bridge contracts are
`xivl-client-structs:manifests/symbols.json#BCS-Y-1816/1817`, which bind
`0x00574ED0/0x00574EE0` to LuaActorImpl slots 41/42. Their dynamic targets,
effects and complete warp sequence are separate from this call-site proof.
The reused `0x00586F40` contract is `BCS-Y-0330/0664`: a gated Map
serverbound `0x0007` record emitter. Its body and downstream send gates were
not inspected here; a call does not prove an observed response or delivery.

## Retained framing

The pinned products in
`xivl-opcodes:data/vendor/captures/payload_layouts.json#layouts.s2c.0x00e2`
and `payload_samples.json#samples.s2c.0x00e2` retain eight records with
40-byte subevents, 24-byte game messages and eight-byte applications.
The application u32 values are `0x02` or `0x0F`; application `+4..+7`
is zero in those samples. These values use the other-selector branch
when delivered to the local CharaElement. They do not witness selectors
`0x15/0x16` or identify a fade, transfer or server allocation policy.
This is a promoted-product comparison; no original capture was decoded.

## Reproduction and limits

Read these half-open original instruction unions:

| Range | Bytes | Purpose |
| --- | ---: | --- |
| `[0x0058CCA0,0x0058CE18)` | 376 | Entry receiver/packet binding, selected filter/main lookups and selected case. |
| `[0x0058D67D,0x0058D6A3)` | 38 | Shared exit, SEH/cookie cleanup and `RET 4`. |
| `[0x004D7390,0x004D7397)` | 7 | Parent-owned pointer getter at `+0x4E0`. |

The union is 421 code bytes and 111 instructions. The initial overlapping
requests attempted 428 bytes; their incomplete edge instructions were joined
before decoding conclusions. Read only the four selected table cells above,
10 typed-data bytes. Map each `VA-0x00400000` through the PE sections and
require complete original raw backing before comparing REA bytes and decoding.

The root was not read as a full body. The two ID getters, entry predicate,
pre/post-warp bridges, emitter, cookie helper and downstream consumers were
not read. The state-byte meanings, actual dynamic implementations, packet
ordering, successful warp, historical delivery and server policy remain
unproved. Keep the neutral catalog name and distinguish conditional native
effects from client acceptance.
