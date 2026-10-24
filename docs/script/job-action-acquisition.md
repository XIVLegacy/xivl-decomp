# Seven-job actions and acquisition state

The retail 1.23b client lists five job-specific actions for each of PLD, WAR,
MNK, DRG, BRD, WHM and BLM. The job-action grid reads
`charaWork.additionalCommandAcquired`, independently of the larger
`charaWork.commandAcquired` array. These lists describe the job-specific
actions, not the inherited actions of the corresponding battle class.

The client evidence establishes the list, display predicates, boolean-array
index conversion and relative packed work layout. A captured server record
sets the Vengeance acquisition flag true. Its quest causality and persistence,
and a fixed PlayerBase displacement to the work-pool bytes remain **UNPROVEN**.

## Exact action groups

`xivl-client-scripts:lua/scripts/chara/charabaseclass_ffxivbattle.lua:317-395`
returns the ordered action list. `setJobAction` at
`xivl-client-scripts:lua/scripts/widget/actionsettingwidget.lua:2943-3053`
selects five entries at these one-based starts:

| Job | Active job ID | First Lua entry |
| --- | ---: | ---: |
| PLD | 16 | 1 |
| WAR | 17 | 6 |
| MNK | 15 | 11 |
| DRG | 19 | 16 |
| BRD | 18 | 21 |
| WHM | 27 | 26 |
| BLM | 26 | 31 |

Names below are the exact English strings in
`xivl-client-data:csv/xtx_command.csv`, column `2`, joined by command-ID row
key. The byte/bit column is relative to the start of the `charaWork` sync
declaration, as derived below. It is not a PlayerBase object displacement
or a packet offset. Bit 0 is the least significant bit.

| Job | Lua additional index | Command ID | English name | Relative byte:bit |
| --- | ---: | ---: | --- | --- |
| PLD | 1 | 27146 | Cover | `+0x04:0` |
| PLD | 2 | 27147 | Divine Veil | `+0x04:1` |
| PLD | 3 | 27148 | Hallowed Ground | `+0x04:2` |
| PLD | 4 | 27149 | Holy Succor | `+0x04:3` |
| PLD | 5 | 27159 | Spirits Within | `+0x04:4` |
| WAR | 6 | 27186 | Vengeance | `+0x04:5` |
| WAR | 7 | 27187 | Antagonize | `+0x04:6` |
| WAR | 8 | 27188 | Collusion | `+0x04:7` |
| WAR | 9 | 27189 | Mighty Strikes | `+0x05:0` |
| WAR | 10 | 27192 | Steel Cyclone | `+0x05:1` |
| MNK | 11 | 27106 | Hundred Fists | `+0x05:2` |
| MNK | 12 | 27107 | Spinning Heel | `+0x05:3` |
| MNK | 13 | 27108 | Shoulder Tackle | `+0x05:4` |
| MNK | 14 | 27109 | Fists of Wind | `+0x05:5` |
| MNK | 15 | 27118 | Dragon Kick | `+0x05:6` |
| DRG | 16 | 27266 | Jump | `+0x05:7` |
| DRG | 17 | 27267 | Elusive Jump | `+0x06:0` |
| DRG | 18 | 27268 | Dragonfire Dive | `+0x06:1` |
| DRG | 19 | 27272 | Disembowel | `+0x06:2` |
| DRG | 20 | 27277 | Ring of Talons | `+0x06:3` |
| BRD | 21 | 27227 | Battle Voice | `+0x06:4` |
| BRD | 22 | 27232 | Rain of Death | `+0x06:5` |
| BRD | 23 | 27237 | Ballad of Magi | `+0x06:6` |
| BRD | 24 | 27238 | Paeon of War | `+0x06:7` |
| BRD | 25 | 27239 | Minuet of Rigor | `+0x07:0` |
| WHM | 26 | 27344 | Presence of Mind | `+0x07:1` |
| WHM | 27 | 27345 | Benediction | `+0x07:2` |
| WHM | 28 | 27357 | Esuna | `+0x07:3` |
| WHM | 29 | 27358 | Regen | `+0x07:4` |
| WHM | 30 | 27359 | Holy | `+0x07:5` |
| BLM | 31 | 27305 | Convert | `+0x07:6` |
| BLM | 32 | 27316 | Burst | `+0x07:7` |
| BLM | 33 | 27317 | Sleepga | `+0x08:0` |
| BLM | 34 | 27318 | Flare | `+0x08:1` |
| BLM | 35 | 27319 | Freeze | `+0x08:2` |

The list has one further entry, Lua index 36 / native element 35, command
29742. The seven selectors above stop at index 35. This page does not assign
29742 to an eighth job or to any of the seven five-action groups.

## Acquisition namespaces and indices

`charabaseclass.lua:558-565` declares `commandAcquired` as
`array, 4096, boolean`; `:736-743` independently declares
`additionalCommandAcquired` as `array, 36, boolean`.
`chara/player/playerbaseclass.lua:1037-1041` binds work ID 3001 to the first
array. That ID does not identify the second array.

| Consumer | Lua access | Native element index |
| --- | --- | --- |
| `isCommandAcquired(c)` for `c < 30000` | `commandAcquired[c - 26000]` | `c - 26001`, when the Lua index is within 1..4096 |
| `isCommandAcquired(c)` for `c >= 30000` | Returns true without reading an acquisition array | none |
| `isAcquiredAdditionalCommand(i)` | `additionalCommandAcquired[i]` | `i - 1`, when `i` is within 1..36 |
| `setJobAction` | Tests the five additional indices of the active job | the five indices in the table minus 1 |

The getter sources are
`xivl-client-scripts:lua/scripts/chara/charabaseclass_cliprog.lua:205-229`
and `xivl-client-scripts:lua/scripts/chara/player/playerbaseclass.lua:1112-1120`.
The former accepts a numeric command ID or obtains it with `getCommandId`.
The inspected decoded corpus contains no call to `isCommandAcquired` beyond
its definition. This bounded search does not establish the absence of a
native caller or an unextracted script.

The two arrays are separate learned-state namespaces. A job action such as
Cover has additional Lua index 1 and command-acquired Lua index 1146. Its
native indices are respectively 0 and 1145. The inspected job grid uses the
first of these and does not consult the command-acquired bit.

### Native boolean layout

All VAs in this page refer to the pinned `ffxivgame.exe` below. Fresh
Capstone decoding and PE-byte comparison established these instructions:

| Native VA | Observation |
| --- | --- |
| `0x00D243E0` | The array factory reads declaration operand 3 as the count and operand 4 as the element type; it constructs ArrayInformation through `0x00D21D40`. |
| `0x00D21D40` | Stores count at descriptor `+0x10` and child descriptor at `+0x14`. If child width is byte aligned it aligns the start through `0x00D1F4F0`; a one-bit boolean keeps the unrounded start. |
| `0x00D259E0 -> 0x00D258E0` | ArrayInformation width slot 3 multiplies the stored count by the child's slot-3 bit width. This is direct instruction evidence for array width, not an inference from neighboring fields. |
| `0x00D28390 -> 0x00D27450` | BooleanInformation preserves the supplied bit start at descriptor `+0x08`. Its vtable `0x0110FBC4`, slot 3, points to `0x00D2DDE0`, which returns one bit. |
| `0x00D1E59F..0x00D1E5CD` | The declaration walker aligns before adding byte-aligned widths and adds the descriptor's width to its running bit position. |
| `0x00D2F220 -> 0x00D2E0A0` | The array read bridge accepts Lua index 1 through the stored count. Outside that interval it pushes nil. |
| `0x00D2E101..0x00D2E12B` | The read bridge calls `0x00D1A620`, then dispatches the child descriptor's slot 7. `0x00D1A620` subtracts one from a positive index. |
| `0x00D28890 -> 0x00D25900` | The BooleanInformation reader adds the normalized element index times the one-bit width to the descriptor start. |
| `0x00D11C00` | Splits that bit start into byte offset `start >> 3` and bit `start & 7`. |
| `0x00CE45D0 -> 0x00CCB4D0` | Adds the selected class-level base from the pool-kind layout vector at work record `+0x2C`, then calls slot 4 of the selected pool object for the bit read. The pool-object vector is at container `+0x0C`. |

The array vtable is `0x0110F804`, whose slot 3 is `0x00D259E0`.

For the local-player declaration in `charabaseclass.lua:531-781`, the prefix
is:

| Member | Relative bit start | Width |
| --- | ---: | ---: |
| `property` | 0 | 32 bits |
| `additionalCommandAcquired` | 32 | 36 bits |
| alignment before `currentContentGroup` | 68 | 4 bits |
| `currentContentGroup` | 72 | 32 bits |
| `depictionJudge` | 104 | 32 bits |
| `statusShownTime` | 136 | 20 * 32 bits |
| `parameterSave` | 776 | 512 bytes |
| `parameterTemp` | 4872 | 256 bytes |
| `eventSave` | 6920 | 3 bytes |
| `eventTemp` | 6944 | 9 bytes |
| `commandAcquired` | 7016 | 4096 bits |

Thus additional Lua entry `i` uses relative bit `32 + i - 1`.
Command-acquired Lua entry `j` uses relative bit `7016 + j - 1`.
For command ID `c`, the latter is `7016 + c - 26001`. Cover therefore uses
`additionalCommandAcquired` at relative byte `+0x04`, bit 0, and the other
namespace at relative byte `+0x3FC`, bit 1. This does not show that both bits
must be true for Cover to be displayed or used.

The 512/256-byte nesting capacities depend on both `isPlayer()` and
`isMyPlayer()` in the declaration. `commandAcquired` and the command-slot
arrays are appended only on that local-player branch. The additional array
is declared independently before those nested members. Do not apply the
local-player `+0x36D` command-acquired start to another actor's layout.

**UNPROVEN:** a constant `PlayerBase + offset` for either array, the concrete
pool allocation reached for this player, and the complete actor-to-pool
pointer chain. The proven offsets above are relative to a work declaration
and its selected pool/class-level base. The active job's native byte is a
different field: `_getJob` at `0x00706D90` reads `PlayerBase+0xF1`, as owned
by [Active job and soul-stone state](../actor/job-soulstone-state.md).

## Availability, active job, level and stone

`setJobAction` obtains `getMainClassOrJob()`, hides the grid when `isJob`
rejects that value, selects its five entries, and includes only entries for
which `isAcquiredAdditionalCommand` is true. If none pass, it hides the
grid. There is no `hasJobStone` call and no current-level comparison in
this method. Each included command is passed to `setIconList` with
availability value 1 and comparison level 1.

The shared `setIconList` (`actionsettingwidget.lua:3893-4023`) treats a
negative availability value as an additional-acquisition index and changes
it to 1 when that bit is true. Positive values show the entry; values above
the supplied comparison level draw it at 0.5 color intensity. Nonpositive
values hide it. This is a display rule. The job path supplies the constant
pair 1/1, so it does not derive job availability from a level table.

The help panel independently displays the `gameCommandBasic` best-level
field, column 39, when positive (`actionsettingwidget.lua:551-783`). All 35
rows above contain 0 in that column in the pinned table. Neither that zero
nor the other static columns establish a server acquisition level.

The Additional Action class chooser calls `checkClassCommandPermission`
(`actionsettingwidget.lua:1181-1195`), whose active-job class whitelist is
documented in [Active job and soul-stone state](../actor/job-soulstone-state.md#actions-and-acquisition-boundaries).
Its consumer is the class chooser. It is not an individual job action's
learned-state predicate or a server equip/execution authorization check.
Stone possession and job-change command admission have their own consumers
in that same page. Possession, an active job byte, and an acquired-action
bit must not be substituted for one another.

The [placement finding](actions-traits-menu.md) and
`xivl-client-scripts:docs/action-bar-placement.md` distinguish icon
selection, placement bindings, equipped slots and learned state.
`showGetJobAbilityWidget` presents a reward but writes neither acquisition
array. **UNPROVEN:** the authoritative level, quest, stone-possession and
active-job conditions the retail server enforced for granting, equipping
or executing each action.

## Incoming state and outgoing work requests

The server-to-client property carrier is opcode `0x0137`. Its existing
[SyncWriter finding](../net/sync-writer.md) owns the generic handler,
callback and storage boundaries. Fresh byte checks reproduced the receive
route `0x00759ED0 -> 0x0089E550 -> 0x00775A30 -> 0x00775180`.

Offsets below start at the application body pointer passed to
`0x00759ED0`, after its enclosing actor/event framing. They are not network
frame offsets:

| Application offset | Native observation |
| --- | --- |
| `+0x00` | `0x00759EFA` reads one unsigned byte as stream length `N`. |
| `+0x01` through `+N` | `0x00759EFE` advances the pointer by one; `0x0089F4C0` copies exactly the supplied `N` bytes into its internal vector. |

For a property record beginning at position `r` in that stream:

| Stream offset | Field |
| --- | --- |
| `r+0` | One-byte value width `w` on the parser's property branch |
| `r+1..r+4` | 32-bit property hash, little endian on the retained Windows path |
| `r+5..r+4+w` | `w` raw value bytes |

`0x0077527C..0x00775282` reads the width and advances one byte.
`0x007755AA..0x007755CF` reads the hash through `0x006BF0B0` and advances
four. `0x00775621..0x00775652` obtains the registered writer at map-node
`+0x10` and invokes slot 1 with the raw value pointer and width.
`0x00775674..0x00775677` advances by the value width. Separate parser
branches handle target/control entries, so an arbitrary stream byte is not
necessarily a property width. The common scalar slot-1 thunk `0x00D30C70`
increments writer `+0x0C` before typed slot 6.

The native property-name builder `0x00D278D0` joins member names using `.`
and formats supplied native array indices inside `[...]`; its literals are
at `0x0110FA54..0x0110FA70`. It hashes the resulting bytes through
`0x00D31540 -> 0x00D31490`, backward MurmurHash2 with seed zero. Native
array index 0 therefore names the first element, for example
`charaWork.additionalCommandAcquired[0]`, while Lua reads it with `[1]`.
These are hashed property paths, not work ID 3001 or byte offsets.

The BooleanInformation sync registration branch at
`0x00D2BCD4..0x00D2BD43` constructs `SyncWriterBoolean` through
`0x00D2FB80`, preserving `start & 7` in writer `+0x10`. Its vtable
`0x011101CC` binds slot 6 to `0x00D2F8B0`, which reads the first value byte,
normalizes nonzero to true, and stores it at writer `+0x11`. Thus a scalar
boolean property value is a byte even though the destination field is a
packed bit. The callback `0x00D2F8C0` forwards the bit selector and boolean
value through handler slot 2. The [SyncWriter finding](../net/sync-writer.md#boolean-callback-path)
continues that callback trace and distinguishes the writer staging bytes
from backing work storage.

### Named acquisition registrations

The two declarations select different handlers. In
`charabaseclass.lua:975-1000`, the tag entries are structurally
`{"commandAcquired", {"commandAcquired", "."}}` and
`{"additionalCommand", 1, self, {"additionalCommandAcquired"}}`.
`0x00D26B10` parses the outer tag and calls `0x00D26590` for each member.
The latter finds the declared sync member and dispatches its descriptor's
slot 8 at `0x00D26A50..0x00D26A7E`. For these arrays that slot is
`0x00D22030` in vtable `0x0110F804`.

| Named input | Selected native branch | Registered property/value |
| --- | --- | --- |
| `additionalCommand`, numeric tag operand 1 | The outer parser takes `0x00D27250` or `0x00D27290`; both set the descriptor flag at `+0xAD` to 1. ArrayInformation tests that flag at `0x00D22077` and expands indices 0..35 through child slot 8 at `0x00D22090..0x00D220CB`. | Individual `SyncWriterBoolean` records for `charaWork.additionalCommandAcquired[n]`, `n=0..35`. A one-byte value is sufficient; value 0 is false and any nonzero value is true. |
| `commandAcquired`, member selector `.` | `0x00D26590` compares the selector with literal `.` at `0x0110F9F4` and stores sentinel -3 from `0x0130D794`. ArrayInformation takes `0x00D223DF`, builds a raw array writer, and wraps it through `0x00D309C0` at `0x00D22621`. | `SyncWriterArrayPartial`, vtable `0x01110310`, around `SyncWriterArray`, vtable `0x011102C8`, for the base property `charaWork.commandAcquired`. |

The other literal `[*]`, at `0x0110F9F0`, maps to -2 and takes a
different per-element branch. It is not the `.` selector used here.
The outer `commandAcquired` entry has no numeric tag operand, so its
descriptor starts with `+0xAD=0` through `0x0070AA10`. The Boolean child
does not match the Actor/Integer16/Integer32/Float specializations tested
in `0x00D2248B..0x00D225AE`; the fall-through constructor at
`0x00D225C6` is `0x00D305A0`, the raw array writer. Its size is
4096 bits / 8 = 512 bytes. The wrapper retains element width 1 bit.

For the scalar additional records, the property hash is exactly
`Murmur2("charaWork.additionalCommandAcquired[" + decimal(n) + "]", 0)`.
For example index 0 hashes to `0x0CCA47C4`, and WAR's first action,
Vengeance 27186, uses native index 5 / Lua index 6 and hash `0xFCA1FFCD`.
A true-value property fragment for that entry is
`01 CD FF A1 FC 01`: width, little-endian hash, boolean byte. This is the
registered format; a historical transaction requires its own witness.

The partial-array property hash is `0x54EDB926`, from the base name
`charaWork.commandAcquired`, without `[index]`. Its slot 1 is
`0x00D30A70`. For a raw value of width `w`, it appends the first `w-1`
bytes to its accumulation vector and excludes the final marker byte.
ActorWorkSync's interface at object `+0x08` is vtable `0x00FE0438`
(`0x0076DC8B`). Its slot 1 is `0x00758B60`, which tests that last byte
against the file-backed constant 1 at `0x00FE05A2`. A marker of 1 clears
the previous accumulation before appending. Markers 2 and 3 do not clear
it on this slot-1 path. The adjacent interface methods `0x00758B80` and
`0x00758BA0` test 2 and 3 respectively; this page does not assign a
higher-level lifecycle name to those two values.

A range descriptor carries `09`, little-endian u16 first and last native
indices, then the literal `charaWork/commandAcquired` path. The parser
`0x00CCAC60` reads that prefix into descriptor `+0xA8/+0xAA` and sets
`+0xAC=1`; `0x006BF310` preserves those fields in the notification
descriptor. `0x00774220 -> 0x00762D20` passes the two endpoints from
updater `+0xBC/+0xBE` to the first registered writer's slot 2.
For the named partial writer that slot is `0x00D30AF0`.

The partial slot-2 body places the accumulated bytes at byte offset
`first >> 3` in its zero-filled temporary vector and supplies that vector
to the inner array writer. There is no shift by `first & 7` in this
operation. The second endpoint is passed but is not read by this body.
The temporary is sized from the accumulation vector's capacity, with an
extra byte when capacity is at least 94 (`0x0110E1A4`). The inner
512-byte writer's `0x00D30640` excludes that final byte in its large-array
branch. Consequently chunk marker bytes are transport control, not
acquisition bits. These instructions do not establish preservation of
bits outside a requested range or arbitrary non-byte-aligned merge
behavior. The exact observed range and chunks must be retained when
reproducing a transaction.

Registration passes the relative byte offset through
`0x00D126F0` for a Boolean, or `0x00D12670` for the array, into
`0x00CE4720`. That function adds the selected sync class-level base;
`0x00CCB810` selects the sync pool and invokes its slot 18. The common
SyncContainer registration `0x00CFD610` installs its secondary callback
interface in writer `+0x04` and the computed offset in writer `+0x08`.
The partial wrapper copies those fields to the inner writer through slot
5, `0x00D2FDE0`. Boolean callback `0x00D2F8C0 -> 0x00CFD5B0`
dispatches to the access object's slot 5. Array callback
`0x00D302A0 -> 0x00CFD580` dispatches its byte vector to access slot 3.
The concrete access object is held at `SyncContainer+0x08`. The
[generic storage finding](../net/sync-writer.md#shared-sync-staging)
documents the access-interface boundary and SharedSyncAccess behavior. The
complete player allocation/owner chain to that access object remains unproven
here; the named registrations and offset calculation do not require
inventing a fixed acquisition-array displacement in PlayerBase.

The conditional native notification reaches
`0x00774220 -> 0x00773F10` and Lua `_onUpdateWork`.
`charabaseclass.lua:1182-1188,1569-1579` specially forwards changed
`commandAcquired` indices, adding 26000 for the desktop callback. This
callback does not grant an action. Other work uses the generic parameter
notification path.

### Outgoing range request

In the opposite direction, `updateCommandAcquired` at
`charabaseclass.lua:1532-1566` requests
`_updateWork("charaWork", "commandAcquired", first-26000, last-26000)`
behind the request-information guard. Native `0x006E7670` subtracts one
from each supplied range endpoint (`0x006E77CD..0x006E77E0`). Its request
route reaches the client-to-server builder `0x0075E770`, which writes
message `0x012F` and declared length `0x38` at
`0x0075E822..0x0075E82A`. This is a request for work, not an acquisition
grant. Do not confuse this client-to-server opcode with the unrelated
server-to-client event opcode of the same number.

`0x0070AAA0` stores the two strings at request-object `+0x00/+0x54`,
the normalized u16 endpoints at `+0xA8/+0xAA`, and range flag 1 at
`+0xAC`. `0x00767FC0` can defer its input under the recorded work-context
guard; its direct path calls `0x00767C00`. That function serializes the
range descriptor through `0x00CCAAF0`, copies the result into its byte
vector and calls `0x0075E770` at `0x00767E34`.

The serializer writes byte `09` at encoded offset 0, endpoints at 1 and
3, the first string at 5, `/` after that string, and the second string
after `/` (`0x00CCAB90..0x00CCAC4C`). It copies exactly the string lengths,
without a terminator. For these strings the encoded span is 30 bytes.
The builder first clears its 32-byte encoded buffer and copies the
serialized vector into it. The actor/target dword and four qword copies
then yield this application layout:

| Application offset | Exact source/value |
| --- | --- |
| `+0x00..+0x03` | Request target dword, passed from N-API ExecuteParameters `+0x0C` through `0x00767E19..0x00767E34` |
| `+0x04` | `09`, range prefix |
| `+0x05..+0x06` | `(firstCommandId - 26001) & 0xFFFF`, little endian |
| `+0x07..+0x08` | `(lastCommandId - 26001) & 0xFFFF`, little endian |
| `+0x09..+0x21` | 25 ASCII bytes `charaWork/commandAcquired` |
| `+0x22..+0x23` | Zero padding from the cleared 32-byte encoded buffer |
| `+0x24..+0x27` | Four trailing bytes not written by this builder; actual values UNPROVEN |

These are application offsets, not offsets from the internal descriptor.
The builder puts the target at descriptor `+0x18` and the 32-byte encoded
buffer at `+0x1C`. The forwarding chain is
`0x004D6D30 -> 0x004E0240 -> 0x00DAE010`. On its state-3 branch,
`0x00DAE0A9..0x00DAE0B8` copies `declaredLength - 0x10` bytes from
descriptor `+0x18` to game-message `+0x10`. For declared length `0x38`,
that is 40 application bytes, including the trailing unwritten four.
States 1 and 2 accept only record value 2 on this forwarding path, so
they do not send the `0x012F` record. The
[Zone header finding](../net/zone-outbound-header.md) owns the final
16-byte game-message header and submission path. Static serialization
does not establish successful transmission or a reply.

The halfword serializer's mode at `0x0134B368` (RVA `0x00F4B368`) lies
in the zero-initialized PE virtual tail, not in file-backed bytes. The
`.data` section starts at RVA `0x00E65000`, has raw size `0x000BF000`
(raw-backed end `0x00F24000`) and virtual size `0x00117940`.
At `0x00CCAAF3..0x00CCAB20`, mode zero writes -1 and takes the direct
little-endian stores; a negative mode also takes that path, while a
positive mode swaps the halfwords. These are static branch observations,
not an executed-mode measurement. No range validation beyond the observed
u16 conversion is attributed to this request path.

### Observed Vengeance setting record

**OBSERVED:** the retained server-to-client `0x0137` record in
`xivl-captures:studies/property-stream-hash-catalog/derived/property-records.csv`,
`record_index=9303`, has width 1, property hash `0xFCA1FFCD` and value `01`.
The native registration above hashes that name as
`charaWork.additionalCommandAcquired[5]`: native element 5, Lua index 6,
command 27186, Vengeance. The Boolean writer consumes `01` as true. This
binds a captured setting record to this particular job-action flag.

The source member `war_quest_update3.pcapng` is 103,520 bytes with SHA-256
`52279E8CBC13D3F4751B5EE3CB9C91DF1895ED5C41D30672629DB3EC1C9EFD05`,
pinned by `xivl-captures:sources/pcap-1.23b/manifest.yaml`. The witness is
main lane 0, server-to-client frame 140 at reconstructed stream offset
17,381, subevent 3 at inflated-frame-body offset 504, completed by capture
packet 421. These are decoder indices, not capture-wide chronology.
The inner-message SHA-256 is
`96A4A577A825C94039C90FE97BBC25CCB6D213E23BB6EBA9AB0EB8091B2F4583`.
Its 136-byte application has declared property-stream length 100. The
first record starts at application offset 1 (inner-message offset 17):
`01 CD FF A1 FC 01`, meaning width 1, little-endian hash and one value byte.
The canonical product identifies it as property packet 9, record 0,
stream offset 1 within that capture.

The retained capture decoder and property parser reproduce the numeric
witness; the native hash/Boolean writer supply its acquisition meaning.
Run `python tools/validate_framing.py sources/pcap-1.23b/objects/war_quest_update3.pcapng`
in `xivl-captures` for framing and
`python tools/extractors/extract_property_stream_catalog.py --check` for
the canonical property projection. The latter checks the retained corpus,
not only this record. Source/destination actor header fields alone do not
prove the property's subject.

**UNPROVEN:** whether this record changed a previously false bit, exact
quest eligibility or causality, the other acquisition namespace in that
transaction, initial values, persistence, all 35 grant records, and ordering
relative to job or inventory updates. A level table, quest reward popup,
or hotbar placement reply does not fill those edges.

## Evidence identity and reproduction

The binary is retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.
The observations use retained Ghidra instruction listings checked against
the PE and freshly decoded with Capstone 5.0.7 and pefile 2024.8.26.
No target process was executed. The validation compares each listed
instruction at its exact VA; it does not fill listing gaps with assumed
instructions or promote decompiler-inferred types.

To reproduce any cited instruction window with a caller-supplied binary,
save this as `inspect_window.py` outside the tracked tree and run
`python inspect_window.py <ffxivgame.exe> 0x00D2E0A0 0xD0`:

```python
import hashlib, sys
from pathlib import Path
import capstone, pefile
data = Path(sys.argv[1]).read_bytes()
assert len(data) == 15996808
assert hashlib.sha256(data).hexdigest() == (
    "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9")
pe = pefile.PE(data=data)
assert pe.OPTIONAL_HEADER.ImageBase == 0x400000
va, size = int(sys.argv[2], 0), int(sys.argv[3], 0)
offset = pe.get_offset_from_rva(va - 0x400000)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for insn in md.disasm(data[offset:offset + size], va):
    print(f"{insn.address:08X}: {insn.mnemonic} {insn.op_str}")
```

Select a bounded window at each cited function or instruction address.
Read vtable cells as little-endian dwords at the cited table VA plus
`4 * slot`. The two native indices and bit formulas above then reproduce
directly from the declaration order and width/normalization instructions.
`python tools/validate_murmur2.py` reproduces the hash primitive's fixed
vectors; [MurmurHash2](../resource/murmur2.md) owns that primitive.

Script source citations in this page use the prefix
`xivl-client-scripts:lua/scripts/` unless written in full. Inputs belong to
extraction `2012.09.19.0001`; selected identities were checked against
`xivl-client-scripts:manifests/scripts.json` after its declared LF
canonicalization, and raw CSV bytes against
`xivl-client-data:manifests/tables.json`.

| Selected source | SHA-256 |
| --- | --- |
| `lua/scripts/chara/charabaseclass.lua` | `F17B3E62A137D8F524BA88D55296E1FB064512DC803F7A85DC980DD4BCBC9625` |
| `lua/scripts/chara/charabaseclass_ffxivbattle.lua` | `D1DB23D11F911EA3205CA72868CC9FCA22CEFE0D3F88A6706CBB035D92130BDA` |
| `lua/scripts/chara/charabaseclass_cliprog.lua` | `EA04844921F8820562AFD172845CAB7BAF2DF235725D20AD179BB713754C87FD` |
| `lua/scripts/chara/player/playerbaseclass.lua` | `6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4` |
| `lua/scripts/widget/actionsettingwidget.lua` | `875CA1651F984A6135D8C2439FFE1DD9DD997898720D9C2C5559A9FA57F3CABF` |
| `csv/xtx_command.csv` | `2AA0B20822DF0B2270918D9AC5525C86A34A941D7C66D17A106ADD1B2D6567D3` |
| `csv/gameCommandBasic.csv` | `4D2D75D88E96D56D1B4812B27D1D8CBA68919EC9A4D4E2527647F4AE38CF7424` |
