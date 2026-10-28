# SetActorName display-name id 0 and the speaker name

The map-server packet `0x013D` `SetActorName` carries a display-name id, a
32-byte string and one byte. This page traces how the client decodes those
fields, what it does with an id of `0`, `0xFFFFFFFF` or any other value, and
what name the NpcSay speaker strip ends up holding. The strip itself is in
[LogControl Talker and the speaker display name](logcontrol-talker-name.md).
The window chain is in the [NpcSay window chain](../event/npcsay-window-chain.md).

## Summary

| Question | Answer |
|---|---|
| Where are the fields? | Payload `+0` is a `u32` id, `+4` is a 32-byte string buffer, `+0x24` is one flag byte. The payload starts 0x10 bytes into the game packet. OBSERVED |
| Is there a custom string on the wire? | Yes, the 32-byte buffer. Every retail sample holds zeros there. OBSERVED |
| What does a wire id of `0` do? | The decoder replaces it with `0xFFFFFFFF` before anything else reads it. The constructor of the receiver replaces it a second time. OBSERVED |
| So are `0` and `0xFFFFFFFF` different? | No. Both take the custom-string path. OBSERVED |
| What does a custom-string packet with an empty string and a zero flag byte do? | Nothing. The decoder makes no receiver and the stored name stays as it was. OBSERVED |
| What does a nonzero id do? | It is stored with `+0x59 = 0`. The wire string and the wire flag byte are not stored. The name is looked up through row 222 of `xtx/_text_ui` into `xtx/displayName`. OBSERVED |
| Does row 222 treat id 0 as a custom string? | Yes. The `08` condition tests integer context element 6 for nonzero. The else branch reads string context element 1. Element 6 is zero for the custom path. OBSERVED, with the link between the pushed context and the decoder vector INFERRED |
| Does the client fall back to the `0x00CC` instance name? | No path was found. UNKNOWN only for writers that were not traced |
| What does a retail client receive? | Of 55 retained samples, 22 carry id `0`, 5 carry `0xFFFFFFFF` and 28 carry one of 20 nonzero ids. All 27 id `0` or `0xFFFFFFFF` samples have an all-zero string and byte, so the client ignores them. OBSERVED |

## Input and method

Observed against FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All locators are VAs. Ghidra CLI navigated the code and Capstone
5.0.7 listings of the identified PE checked every cited instruction and data
word. Decompiler output is navigation only. OBSERVED marks instructions, data
and decoded assets. INFERRED marks an interpretation of observed code. UNKNOWN
marks what was not traced. `tools/verify_display_name_id_zero.py` re-checks the
listings, tables, caller sets, the row 222 cells and the capture census below,
and `tools/test_display_name_id_zero.py` covers it.

The sheet facts use the exports `xtx__text_ui.csv` (1284564 bytes, SHA-256
`23762a422118d99c4144a48b3495c52511b1b1b865e4fc50fdc61a973e3c6ece`) and
`xtx_displayName.csv` (838262 bytes, SHA-256
`36c5dfba312695d9f4ee090dba123dfd7792feba55d88b0062c070d98abc0505`). Both
match the entries in `xivl-client-data:manifests/tables.json` and the exports
that the [Talker page](logcontrol-talker-name.md#input-and-method) used. The
census uses `xivl-opcodes:data/vendor/captures/payload_samples.json`, key
`s2c` `0x013d`, 55 samples.

## Packet dispatch

OBSERVED. The packet dispatcher `0x004D8860` reads the opcode word at packet
`+2`, subtracts `0xCC` and bounds the result by `0xDC`
(`0x004D88C0..0x004D88D4`). A byte table at `0x004D8C24` picks one of the cases
of the jump table at `0x004D8BC0`. The opcode `0x013D` is table index `0x71`,
which holds class `11`. No other opcode holds class `11`, and its case starts
at `0x004D8A36`. The case adds `0x10` to the packet pointer and calls `0x00575100`
(`0x004D8A44..0x004D8A48`). That thunk jumps through slot 62 (offset `0xF8`) of
the implementation object reached through dispatcher `+0x80`.

| Implementation | Vtable | Slot 62 |
|---|---|---|
| `LuaActorImpl` | `0x00FDFB2C` | `0x0076C4D0` |
| `NullActorImpl` | `0x00FE02AC` | `0x00771980` |

UNKNOWN. Which implementation sits behind dispatcher `+0x80` for a given actor
was not traced. The target actor is not a field of the payload.

## Payload layout

OBSERVED. Both slot 62 routines decode the same payload, with `edi` set to
packet `+0x10`.

| Payload offset | Size | Use | Locator |
|---|---|---|---|
| `+0` | `u32` | Display-name id. `0` becomes `0xFFFFFFFF` | `0x0076C512..0x0076C526`, `0x007719CB..0x007719DD` |
| `+4` | 32 bytes | String buffer. Copied into a zeroed 33-byte buffer and read as a NUL-terminated string | `0x0076C55D..0x0076C58B` |
| `+0x24` | `u8` | Flag byte, stored beside the string | `0x0076C52A`, `0x007719DD` |

The string constructor `0x00447260` receives the codepage word at `0x00F67298`,
which holds `0xFFFFFFFF` in the image. With that value the constructor measures
the length of the text up to the first NUL (`0x00447272..0x004472A9`), so the
string is at most 32 bytes. The wire payload is 40 bytes in all 55 retail
samples, which leaves 3 bytes after the flag.

OBSERVED. The flag byte becomes the name structure byte at `+0x58`
(`0x0089D633..0x0089D649`, `0x006FB0BA..0x006FB0C9`). The client
copies it to the listener (`+0x88`), to the record (`+0xE4`) and to context
element 4 for a custom name. No traced reader branches on it. Its meaning is
UNKNOWN.

## What the decoder does

OBSERVED. `LuaActorImpl` slot 62 (`0x0076C4D0`) runs these steps.

1. It sets a local id to `0xFFFFFFFF` when the wire id is `0` and to the wire
   id otherwise (`0x0076C516..0x0076C526`).
2. It builds the string from the 32 bytes as above.
3. When the local id is `0xFFFFFFFF`, the flag byte is zero and the first
   character of the string is NUL, it releases its temporaries and returns
   without making a receiver (`0x0076C5A4..0x0076C5DB`).
4. Otherwise it builds a `SetDisplayNameReceiver` on the stack with `0x0089D530`
   from the raw wire id, the string and the flag byte (`0x0076C5E0..0x0076C5F4`).
   The constructor maps `0` to `0xFFFFFFFF` again (`0x0089D566..0x0089D574`) and
   stores id `+4`, string `+8` and flag `+0x5C`.
5. It calls the receiver's slot 1 `0x0089D610` directly (`0x0076C614`).
   That routine casts its second argument to `CharaBase` (type descriptor
   `0x012709A4`) and, when the cast succeeds, calls `0x006FAFF0` with the id,
   the string, the flag byte and a force byte of 1 (`0x0089D633..0x0089D649`).

OBSERVED. `NullActorImpl` slot 62 (`0x00771980`) has the same id map, flag read
and drop rule (`0x007719CB..0x007719DD`, `0x00771A57..0x00771A6C`). Three
branches differ from the Lua path.

| Impl byte | Effect |
|---|---|
| `+0x52` nonzero | The packet has no effect (`0x007719BE`) |
| `+0x50` nonzero | The receiver is built with `0x0089D530` and queued (`0x00771AC3`, `0x00771B00`) |
| both zero | The packet routine calls `0x00771230` (`0x00771B1B`), which puts the id, string and flag byte into the impl's own name state at `+0x54`, `+0x58` and `+0xAC`. Byte `+0xAD` becomes `id == -1`. A lookup starts through `0x00770F10` unless the id is `0xFFFFFFFF` (`0x00771269..0x0077129D`) |

UNKNOWN. The writers of impl bytes `+0x50` and `+0x52` were not traced to
`NullActorImpl`.

| Wire id | Wire string | Wire flag | Outcome |
|---|---|---|---|
| `0` | empty | `0` | Ignored |
| `0xFFFFFFFF` | empty | `0` | Ignored |
| `0` or `0xFFFFFFFF` | empty | nonzero | Custom name, empty string |
| `0` or `0xFFFFFFFF` | not empty | any | Custom name, the string |
| other | any | any | Id lookup, the string and flag are not stored |

## The name structure

OBSERVED. The structure at `CharaBase +0x60` is created by the `CharaBase`
vtable slot 3 `0x006E03B0` (vtable `0x00FD5CAC`). The routine passes its two
arguments to virtual `+0x90`, then allocates `0x5C` bytes and runs the default
constructor `0x006BF420` (`0x006E03E9..0x006E0417`). The default constructor
stores id `0xFFFFFFFF`, an empty string, byte `+0x58 = 0` and byte `+0x59 = 1`
(`0x006BF44B..0x006BF45F`). A fresh actor therefore holds an empty custom name.

OBSERVED. `0x006FAFF0` takes five stack arguments after the `CharaBase`
(`ret 0x14`). They are a handle used to reach the core work object (`0x006FB03A`
reads `[esp+0xD8]`), the id (`[esp+0xDC]`), the string address (`[esp+0xE0]`),
the flag byte (`[esp+0xE4]`) and the force byte (`[esp+0xE8]`). The routine
does not special-case `0`. It branches on `-1` (`0x006FB08E`).

| Id argument | Stores | Locator |
|---|---|---|
| `0xFFFFFFFF` | id `-1`, the supplied string at `+4`, the flag byte at `+0x58`, `+0x59 = 1` | `0x006FB0A1..0x006FB0CC` |
| other | the id at `+0`, the string and flag byte it already held, `+0x59 = 0`, then `0x0076FFF0` starts the lookup | `0x006FB1D8..0x006FB233` |

OBSERVED. The lookup branch copies the current string and byte out of the
structure before it stores the id, then writes them back, so the string and the
flag byte of the packet do not reach the structure. This corrects the shared
description of both branches in the
[Talker page](logcontrol-talker-name.md#source-name-of-an-npc-record).

OBSERVED. When the lookup result arrives, `0x006FB280` stores the resolved
string and sets `+0x59 = 1` (`0x006FB2C1..0x006FB2D8`). It is called from
`0x00758F30` (`0x00759027`).

INFERRED. The custom branch ends by calling the object at core work `+0xE4`
through `0x006C3E40` (`0x006FB1AC`), and the resolved-name callback ends with
the same call (`0x006FB3DA..0x006FB3E1`). The object is a display-name sink,
possibly the nameplate, and its type was not traced.

## Consumers of the structure

OBSERVED. The listener fill `0x007898E0` copies the structure into the listener
(`0x00789927..0x00789944`) and then branches on byte `+0x59` (`0x007899AF`).

| Byte `+0x59` | Action |
|---|---|
| nonzero | `0x00777FE0` copies id, string, byte and flag into the record fields at `+0x8C`, `+0x90`, `+0xE4` and `+0xE5` |
| zero | `0x00770F10` requests the name. The id passed is `0` when the structure id is `-1`, otherwise the id (`0x00789A26..0x00789A33`) |

OBSERVED. The sink `0x00772050` stores the structure id into record `+0x108`
with `-1` stored as `0` (`0x007720D8..0x007720EA`). The record therefore cannot
tell `0` from `-1`. No instruction between the decoder and these consumers
treats `0` as a name. The request routines `0x00770F10` and `0x0076FD80` pass the
id through (`0x00770F59..0x00770F64`, `0x0076FDFE`). The result handler
`0x00777370` receives the request (`0x007899E0`), and its body was not traced.

## Row 222 and the text context

OBSERVED. `0x0076FB10` resets the text context with `0x00769CC0`
(`0x0076FB58`), pushes the requested id with `0x00769AE0` (`0x0076FBC5`) and
calls TextModule slot `+0x0C` for row `222` of `xtx/_text_ui` (see the
[Talker page](logcontrol-talker-name.md#display-name-lookup)). The context is a
vector of variants. The accessors check the variant tag: `0x00778D50` returns an
integer only for tag 0 and `0x00779BC0` returns a string only for tag 1.

| Routine | Element 1 (string) | Element 4 (integer) | Element 6 (integer) |
|---|---|---|---|
| `0x00769CC0` reset | empty | `0` | `0` |
| `0x00769AE0`, id `0xFFFFFFFF` | the structure string | the structure byte `+0x58` | `0` |
| `0x00769AE0`, other id | empty | `0` | the id |

The reset is at `0x00769CC0..0x00769D09` and the two push branches at
`0x00769AE6`, `0x00769B02` and `0x00769B37`. The routine `0x00785120` pushes the
three name structures of a message record through the same routine
(`0x0078512E`), so the message templates can read the same elements.

OBSERVED. The operand decoders read these classes.

| Lead | Decoder | Meaning |
|---|---|---|
| `01`..`D7` | `0x00CB2D50` (`0x00CB314B..0x00CB3155`) | Literal, value is the lead minus 1 |
| `E9 n` | `0x00CB2DFD` | Integer element, index is `value(n) - 1`, read from the vector at decoder `+0x14` |
| `EB n` | `0x00CB2BAF` | String element, index is `value(n) - 1`, same vector |
| `EA n` | `0x00CB2C0E` | String from the caller's argument vector |
| `E8 n` | `0x00CB2E46` | Integer from the caller's argument vector |

Leads `D8`..`DF` take a jump table at `0x00CB3230` (`0x00CB30A4..0x00CB30BD`) that was not decoded, and row 222 uses none of them. So `E9 08` reads literal 7, minus 1 gives index 6, and `EB 03` reads literal 2,
minus 1 gives index 1. The `08` tag evaluates the first alternative when its
condition is nonzero and the second when it is zero
([rich-string entry counts](rich-string-entry-counts.md)).

INFERRED. The vector the context routines fill is the vector at decoder
`+0x14`. The evidence is that the integer id is written to element 6 and the
condition reads element 6 as an integer, and the custom string is written to
element 1 and the else branch reads element 1 as a string. A type mismatch
would reach the accessors' error path. The pointer chain from the adapter that
`0x0076FB10` uses to the decoder was not traced.

OBSERVED. All five cells of row 222 test `E9 08` in an `08` tag and read
`EB 03` in the else branch, inside tags `2C` and `29` for column 4. The then branch is a code `28` sheet reference to
`xtx/displayName` with row `E9 08`, or for column 2 a code `32` reference whose
row operand is also `E9 08`.

| Column | Wrapper | Then branch | Else branch |
|---|---|---|---|
| 0 | none | `28`, column literal 0 | `EB 03` |
| 1 | `2D` | `28`, column literal 1 | `EB 03` |
| 2 | `2B` | `32`, literals 5, row, 1, 1, 1 | `EB 03` |
| 3 | `2B` | `28`, column literal 13 | `EB 03` |
| 4 | none | `28`, column literal 19 | `2C` with `29` around `EB 03`, a space and literal 1 |

The wrapper and code `32`, `2C` and `29` meanings are UNKNOWN. The exporter
names `2D` and `2B` `head-all` and `head`.

## What the strip shows

INFERRED from the observed chain. The sink copies the structure into the record
and the record's source name becomes `Talker`
([Talker page](logcontrol-talker-name.md)).

| Packet | Stored state | Speaker text |
|---|---|---|
| id `0`, string `Name` | id `-1`, string `Name`, `+0x59 = 1` | `Name` |
| id `0xFFFFFFFF`, string `Name` | the same | `Name` |
| id `0` or `0xFFFFFFFF`, empty string, flag `0` | unchanged | the earlier name, and the empty default name on a fresh actor, so the label is hidden |
| id `0` or `0xFFFFFFFF`, empty string, flag nonzero | id `-1`, empty string | empty, so the label is hidden |
| nonzero id | the id, `+0x59 = 0` | the `xtx/displayName` row of the id, selected by row 222 |
| nonzero id absent from `xtx/displayName` | the id | UNKNOWN |

OBSERVED. A row `0` exists in `xtx/displayName` and holds `???` in column 1. The
`08` condition is false for `0`, so no path through row 222 reads that row. A
lookup context for id `0` (no traced caller builds one from the wire) would
evaluate to the empty string held in element 1.

OBSERVED for the traced writers, UNKNOWN beyond them. The writers `0x006FAFF0`,
`0x006FB280`, `0x006BF420` and `0x006E03B0` do not read the `0x00CC` instance
name. That record holds the Lua global name, such as `_pc43723073`
([actor instantiation](../actor/actor-instantiation-and-self-player.md)). A full
census of every store to the name structure was not made.

## Capture census

OBSERVED. The 55 retained `0x013D` samples are 56 bytes each. The payload starts
at sample offset 16 and is 40 bytes. Every byte after the id is zero in every
sample, so no sample holds a string or a flag byte.

| Id | Samples | Distinct ids |
|---|---|---|
| `0` | 22 | 1 |
| `0xFFFFFFFF` | 5 | 1 |
| `1000036`..`2200228` | 12 | 11 |
| `3100502`..`3104305` | 14 | 8 |
| `4000602` | 2 | 1 |

OBSERVED. All 20 nonzero ids are keys of `xtx/displayName`. Column 1 holds
names such as `Anselm` (1000036), `Penelope` (1100404), `canopy galago`
(3100502), `tree slug` (3104216) and `task board` (4000602).

INFERRED. The first four bytes of the sample's game-packet header at offset 8
are not an actor id. The values run from 1356917211 to 1356946277, which is
2012-12-31 01:26:51 to 09:31:17 UTC, and they never decrease within a capture.
The leading `0x50` byte that the generated struct header labels as a constant is
the high byte of that clock. The samples do not carry the target actor, so the
census cannot say which kind of actor held id `0`. The zero-id packets arrive in
bursts of one `0xFFFFFFFF` followed by three `0` at one second in
`move_out_of_room`, `return_to_inn`, `teleport_to_camp_nine_ivies` and
`teleport_to_camp_tranquil`, and one `0xFFFFFFFF` and two `0` in
`teleport_to_gridania`. `war_quest_update2` holds four `0` at one second.

## Open items

- Which `xtx/displayName` column the English client reads. Selector `0xFF` uses
  the default logical column of the sheet and that column was not read.
- The meaning of the flag byte at payload `+0x24`.
- What the client shows for a nonzero id with no `xtx/displayName` row.
- The wrappers `2D` and `2B` and the codes `32`, `2C` and `29` in the row 222
  cells.
- Which actors use `NullActorImpl` and what sets its bytes `+0x50` and `+0x52`.
- The pointer chain between the adapter at core work `+0xDC` and the decoder
  vector at `+0x14`.
- The object at core work `+0xE4` and the body of `0x00777370`.
- Whether the retail server sends a custom string at all. None of the 55
  samples holds one.

## Reproduction

Read the cited ranges in the identified executable, the vtables at
`0x00FDFB2C`, `0x00FE02AC`, `0x00FD5CAC` and `0x01057388` with their RTTI, and
the dispatch tables at `0x004D8C24` and `0x004D8BC0`. Run
`python tools/verify_display_name_id_zero.py --exe <ffxivgame.exe>` for the
pinned listings. Add `--text-ui-csv` and `--display-name-csv` for the row 222
cells and `--samples <payload_samples.json>` for the census. Without `--exe` the
command prints a skip line. No instruction body, byte dump or decompiled body is
needed in a published artifact.

## Consumer notes for bahamut-client

A client may implement these parts of the contract.

- Decode the payload as `u32` id, 32-byte NUL-terminated string and one flag
  byte. Treat id `0` as `0xFFFFFFFF`.
- Make no change for id `0xFFFFFFFF` with an empty string and a zero flag byte.
- For id `0xFFFFFFFF` store the string as the whole name. Show it as the
  speaker with no sheet lookup.
- For any other id keep the previous string and byte, resolve the name from the
  `xtx/displayName` row of the id, and ignore the wire string.
- Start a fresh actor with an empty custom name. Do not use the `0x00CC`
  instance name as a display name.

A server may rely on these parts. An id of `0` beside a non-empty custom string
shows that string. A custom string longer than 32 bytes does not fit. An id of
`0` with no string does nothing, so it cannot clear an earlier name unless the
flag byte is nonzero. A nonzero id replaces any string with the sheet name.

These stay CALIBRATION: the language column, the flag byte meaning and the
behavior for an id with no sheet row.
