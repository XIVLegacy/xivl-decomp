# Combat result element path

This page traces how a Map clientbound combat result (opcodes `0x0139`,
`0x013A`, `0x013B` and `0x013C`) reaches the element that owns it, what that
element stages and drains, which fields the traced bodies consume and where the
numbers go. The action queue half of the path is in the
[action queue page](../actor/action-queue.md), the animation bank half in the
[bank routing page](../actor/animation-bank-routing.md) and the sound half in the
[sound effect page](../resource/sound-effect-triggers.md#server-driven-sound).
Wire layouts belong to the opcode and capture catalogs. This page records what
the client does with the fields.

A result goes to the element whose key equals the source actor ID of the
sub-event. The element stages it at once into a ring. A per-frame drain takes
one record per pass, copies it to a per-class bucket ring and posts it to the
scene. The log lines, the sound filter and the damage plate run in a separate
row pass that only element event `0x59` starts, and no publisher of that event
was found.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| Before the range test the dispatcher exits for every opcode except 0x0004 while container byte +0x54B is nonzero or 0x00575740 on the object at container +0x510 returns true | STATIC | [Element resolution](#element-resolution) |
| The receive dispatcher sends every opcode from 0x012E to 0x013D to one shared lookup path | STATIC | [Element resolution](#element-resolution) |
| The lookup key is the sub-event header dword at +4 (the source actor ID), with 0x5FF8xxxx values remapped to 0x5FF80002 minus the low bit | STATIC | [Element resolution](#element-resolution) |
| A key with no registered element ends the packet and stages nothing | STATIC | [Element resolution](#element-resolution) |
| A found element receives the payload view through virtual slot 9, 0x0058CCA0 | STATIC | [Element resolution](#element-resolution) |
| Slot 9 first calls 0x004D8830, which returns 0 only when element byte +0x92, pointer +0x80 and the readiness slot of that pointer are all nonzero | STATIC | [Admission](#admission) |
| A zero result drops opcodes 0x0139 to 0x013C, and above 0x0134 only 0x0137, 0x0144, 0x0145, 0x0197 and 0x01A0 still pass | STATIC | [Admission](#admission) |
| Opcodes 0x0139, 0x013A, 0x013B and 0x013C call the wrappers 0x0058C880, 0x0058C930, 0x0058C990 and 0x0058C7D0 | STATIC | [Opcode routes](#opcode-routes) |
| Each wrapper classifies the effect ID with fallback 6, passes route flag 1 to its stager and queues the record at element +0xA80 | STATIC | [Opcode routes](#opcode-routes) |
| The stagers read one row (0x0139), up to ten (0x013A), up to eighteen (0x013B) or none (0x013C) | STATIC | [Staging](#staging) |
| Every staged record takes a serial from the shared counter at 0x01338D2C, skipping 0, and publishes it as bus command 0x39 | STATIC | [Staging](#staging) |
| The base stager copies the wire header fields to the record offsets of the field map | STATIC | [Staging](#staging) |
| The row stagers copy target, numeric value, effect ID, text ID, text parameter and ordinal from the wire offsets of the row map | STATIC | [Staging](#staging) |
| The ten-row and eighteen-row loops are bounded by the wire row count alone, and the single-row stager ignores it | STATIC | [Staging](#staging) |
| A nonzero control halfword or row text ID makes the stager call 0x004D7470 and then 0x00578390 | STATIC | [Staging](#staging) |
| What 0x00578390 does after it skips effect IDs of type 0x10, type 0x14 and the ID 0x7C000062 | UNPROVED | [Staging](#staging) |
| The class comes from the effect ID type lane, four reserved types and a route-kind table | STATIC | [Classifier](#classifier) |
| The table at 0x00FE32D8 has 34 valid rows of 12 bytes, and the classifier maps route kinds 8, 1, 2, 3, 4 and 5 | STATIC | [Classifier](#classifier) |
| The element ring at +0xA80 stores private 0x1A0-byte copies at head plus count and grows through 0x00590070 when the new count would reach the capacity | STATIC | [Drain](#drain) |
| Element slot 6 runs the drain once per call and the drain takes one record per pass | STATIC | [Drain](#drain) |
| Classes 0 to 25 are copied to a bucket ring at +0x878 plus 0x14 times the class and larger classes are dropped | STATIC | [Drain](#drain) |
| Class 0 posts bus command 3 and classes 1 to 25 post command 4, each with the record and 0x38 plus 0x14 times the row count bytes | STATIC | [Drain](#drain) |
| Classes 3 to 11 then call 0x0058CA80, which runs the header for class 5 through 0x00585AF0 and for classes 6 to 10 through 0x0058C220, and does nothing for classes 3, 4 and 11 | STATIC | [Drain](#drain) |
| The header runs only when the container-selected element casts to a character element and lies closer than the radius at element +0xAA8 | STATIC | [Drain](#drain) |
| The range gate reads the radius dword at element +0xAA8 as an unsigned 32-bit value (0x00588E9D loads a signed integer and adds 2 to the 32nd power when the sign is set), and 0x005875A0 passes only when that radius is strictly greater than the three-component distance between the two elements | STATIC | [Drain](#drain) |
| The radius setter 0x00585930 stores the low dword of the toward-zero truncated float as a 64-bit integer when its mode byte is 0, and stores 50 for five element kinds when the mode byte is 1 | STATIC | [Drain](#drain) |
| A nonzero control halfword is sent to the logger as a text ID with zero target, numeric value, effect ID and parameter, then cleared | STATIC | [Header line](#header-line) |
| The drain calls 0x0058CA80 with the arguments 1 and 0, which stops 0x0058C220 after the header line, so no row is processed at drain time | STATIC | [Header line](#header-line) |
| Rows are processed by element event 0x59, which finds the record by serial in its bucket ring and calls 0x0058CA80 with 0 and the record route flag, so the ordinal filter is on only for route flag 0 | STATIC | [Row trigger](#row-trigger) |
| No code was found that publishes event 0x59, because the only command 0x44 publisher has no caller of its own | NOT FOUND | [Row trigger](#row-trigger) |
| Event 0x59 searches only the bucket rings of classes 3 to 11 and 0x0058CA80 runs the row pass only for classes 6 to 10, so rows of every other class reach no row processing in the traced element code | STATIC | [Row trigger](#row-trigger) |
| Element event 0x64 stages a record through the same classifier and the single-row stager with route flag 0 | STATIC | [Row trigger](#row-trigger) |
| The routine at 0x006AACE0 builds result views with numeric value 9999 and calls the wrappers 0x0058C880 and 0x0058C930 directly | STATIC | [Row trigger](#row-trigger) |
| Row processing runs the inert test, the ordinal filter, the plate, the sound filter, the special text ID switch, the default log, the post call and the selector update in that order | STATIC | [Row processing](#row-processing) |
| The sound filter 0x005859F0 compares the text ID with 30 u16 values at 0x01270BA4, which include 0x847F, 0x848E to 0x8495 and 0x8499 to 0x84A3 and exclude 0x75B1, 0x75B4, 0x846E and 0x8475 | STATIC | [Row processing](#row-processing) |
| A table hit returns false unless the row target equals the actor ID returned by 0x004D7490, which skips the rest of the row (special routes, default log line, post call and selector update), while a miss or a matching target goes on to the 0x8475 and 0x846E sound tests and returns true | STATIC | [Row processing](#row-processing) |
| The default log call passes 14 arguments to 0x005759A0 in the listed order | STATIC | [Row processing](#row-processing) |
| Text IDs 0x75B1, 0x75B4, 0x847F, 0x848E to 0x8492, 0x8494, 0x8495 and 0x8499 to 0x84A3 take extra routes before or instead of the default log, and 0x8493 and 0x8496 to 0x8498 get the default line only | STATIC | [Row processing](#row-processing) |
| What the plate list at +0xE24 and the group list at +0xE4C display | UNPROVED | [Row processing](#row-processing) |
| A plate is created only when the container override pointer at +0x17838 is nonzero, configuration option (2, 5) is nonzero, the global pointer at 0x01356E10 is set and the global byte at 0x01356E14 is clear | STATIC | [Damage plate](#damage-plate) |
| Effect ID bits 27 to 30 give the kind, and kinds 7 to 10 anchor the plate on the record source and every other kind on the row target | STATIC | [Damage plate](#damage-plate) |
| The plate routine 0x00795630 reloads the row effect ID from record +0x40 plus 0x14 times the row at 0x007956D4 and, when it is 0, branches at 0x007956E5 to the epilogue 0x007967A2 before its inline kind decode at 0x007956ED and the layout jump table, so a row with effect ID 0 gets no plate and the row pass goes on to the sound filter | STATIC | [Damage plate](#damage-plate) |
| Each element keeps a ring of five plate slots, and a slot object is 0x48 bytes | STATIC | [Damage plate](#damage-plate) |
| The plate list initializer loads DamagePlateWidget.form, and a plate fills five digit blocks, an icon, an additional text and a damage type text | STATIC | [Damage plate](#damage-plate) |
| The numeric value is written least significant digit first into Number_5 through Number_1, so only the low five decimal digits show | STATIC | [Damage plate](#damage-plate) |
| A jump table with ten entries selects the plate layout by kind 1 to 10 | STATIC | [Damage plate](#damage-plate) |
| The visual meaning of each kind, the animation duration and the widget layout | UNPROVED | [Damage plate](#damage-plate) |
| No staging or row-processing body reads a status-change field | NOT FOUND | [Damage plate](#damage-plate) |
| No delay or timer constant is read between receive, drain and row processing | NOT FOUND | [Damage plate](#damage-plate) |

STATIC means read from instructions or data in the pinned executable. INFERRED
means the evidence is consistent with the claim and does not close it. UNPROVED
means no evidence addresses it. NOT FOUND means the named bodies, call sites or
data were searched by the stated method and hold no such behavior.

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Disassembly listings from ghidra-cli 12.1.4
and Capstone 5.0.7 over the pinned bytes were treated as leads. The sequences,
tables and strings cited here are the `staticChecks` rows of
[`config/combat_result_element.json`](../../config/combat_result_element.json),
and `tools/verify_combat_result_element.py --exe` compares each one with the
pinned bytes. Three scans in the same manifest are recomputed from the image.
The call site scan lists every `E8` and `E9` opcode in the code section whose
rel32 reaches a target. The push scan lists the call sites of a target with a
`6A imm8` push of a listed value in the 14 bytes before the call. The dword scan
lists every occurrence of a little-endian dword in the raw bytes of all
sections. Offsets written `+0x..` are bytes from the start of the named object.
Wire offsets count from the first byte after the 16-byte game-message header,
which is the payload view `P+0x10` that the slot 9 cases pass on.

The ghidra-cli export has no function bounds for some of this code, so a list
built from its cross-references alone can miss callers. The scans do not follow
indirect calls and cannot see a command number that a caller computes at run
time.

## Element resolution

The dispatcher `0x004DC690` first applies two pre-gates (`0x004DC6D5`). While
container byte `+0x54B` is nonzero, or while `0x00575740` on the object at
container `+0x510` returns true, it exits for every opcode except `0x0004`. What
sets that byte and what the predicate tests are open. It then compares the opcode
word at view `+2` with `0x013D` and `0x012E` at `0x004DC71E`. Every opcode from
`0x012E` to `0x013D` takes the direct branch to `0x004DCFFF`, which the
[receive dispatcher page](receive-dispatch-ranges.md) lists as the shared object
lookup. The shared path calls `0x004D9910` with the key, which searches the
ordered map at container `+0x17804` and returns the pointer stored in the node or
null. A null result jumps to the common exit and nothing is staged. A found
element receives the view through virtual slot 9 (offset `+0x24`) of its table
`0x00FA7C50`, and the cell holds `0x0058CCA0`. Slots 6 and 8 of the same table
hold `0x0058DF90` and `0x0058E0C0`, which the drain and the event handler below
use.

The key is the source actor ID of the sub-event. The caller `0x004E20A0` loads
the dword at packet record `+0x14` at `0x004E21E8`. A key whose bits under
`0x5FF80000` are all set becomes `0x5FF80002` minus its low bit, and every other
key passes unchanged. The call into the dispatcher is at `0x004E221D`. The
record gets that dword from the dequeue parser `0x00DAF5B0`, which copies the
wrapped sub-event header dwords to staging, so header `+4` lands at staging
`+0xC` (`0x00DAF612`) and the record store at `0x00DAF789` writes it to record
`+0x14` with header `+8`, the target, to `+0x18`. The
[scene binding page](../resource/attribute-resource-and-scene-binding.md#case-2-appearance-ordering)
joins the same field to a captured source ID. An element stores its key at
`+0x88`, and `0x004D6750` reads it.

Two actor IDs therefore matter. The element key is the sub-event source, and the
record keeps the first payload dword separately as its own source field. The
two are not forced equal.

## Admission

Slot 9 calls `0x004D8830` at `0x0058CCE5` before it reads the opcode. The
predicate returns 0 only when element byte `+0x92`, element pointer `+0x80` and
the readiness slot of that pointer (`0x00574AA0`) are all nonzero. It returns 1
in every other state. The
[scene binding page](../resource/attribute-resource-and-scene-binding.md#appearance-admission-before-binding)
owns what sets those fields: a `0xCA` packet sets `+0x92`, a `0x00CC` packet
clears it and a bound script implementation answers the readiness slot.

A nonzero result sends the opcode through a table of cases (`0x58D930` and
`0x58D994`), and `0x0139` to `0x013C` reach their staging cases. A zero result
sends it through a stricter filter. The filter lets `0x0134` through and exits
for `0x0135` and `0x0136`. From `0x0137` on, byte table `0x0058D8C4` decides, and
a zero byte lets the opcode continue to the same case table. Above `0x0134` the
table passes `0x0137`, `0x0144`, `0x0145`, `0x0197` and `0x01A0` and exits for
every other opcode, so the four combat opcodes are dropped. A result for an
element in that state ends at the jump to the common exit.

## Opcode routes

| Opcode | Case | Wrapper | Stager | Rows staged | Count bound |
| --- | --- | --- | --- | --- | --- |
| `0x0139` | `0x0058D553` | `0x0058C880` | `0x00587370` | one | none, one row is always staged (record count still holds the wire u16) |
| `0x013A` | `0x0058D565` | `0x0058C930` | `0x005873E0` | up to ten | wire u16 only |
| `0x013B` | `0x0058D577` | `0x0058C990` | `0x005874B0` | up to eighteen | wire u16 only |
| `0x013C` | `0x0058D541` | `0x0058C7D0` | `0x00587210` | none | not bounded (record count still holds the wire u16) |

Each case pushes the view (`P+0x10`) and the literal 1 and calls its wrapper.
The wrapper reads the effect ID at view `+4`, calls the classifier `0x00585800`
with the fallback 6, builds the record in a `0x1A0`-byte stack buffer through
its stager with route flag 1 and calls the ring push `0x005901D0` on element
`+0xA80`. One local routine reaches the wrappers without a packet. The routine at
`0x006AACE0` builds a view whose numeric values are the constant 9999 and calls
`0x0058C880` and `0x0058C930` directly, and its own callers were not traced.

## Staging

The base stager `0x00587210` takes the output record, the view, the class and
the route flag. It adds 1 to the shared counter at `0x01338D2C` (a result of 0
is replaced by 1) and publishes the new value as bus command `0x39` with a
4-byte payload, which becomes event `0x4E`. It then copies the fields below. The
other stagers call it first and then fill the rows.

| Record offset | Width | Field | Source |
| --- | --- | --- | --- |
| `+0x00` | 4 | sequence ID | counter |
| `+0x04` | 4 | visual class | caller |
| `+0x08` | 4 | route flag | caller |
| `+0x0C` | 4 | source actor ID | view `+0x00` |
| `+0x10` | 4 | effect or animation ID | view `+0x04` |
| `+0x14` | 4 | pre-action dword 0 | view `+0x08` |
| `+0x18` | 4 | pre-action dword 1 | view `+0x0C` |
| `+0x1C` | 4 | pre-action float 0 | view `+0x10` |
| `+0x20` | 4 | pre-action float 1 | view `+0x14` |
| `+0x24` | 4 | pre-action float 2 | view `+0x18` |
| `+0x28` | 4 | constant 1.0 | `0x00F54F70` |
| `+0x2C` | 4 | pre-action float 3 | view `+0x1C` |
| `+0x30` | 2 | row count | view `+0x20` |
| `+0x32` | 2 | control value | view `+0x22` |
| `+0x34` | 2 | command ID | view `+0x24` |
| `+0x36` | 2 | presentation flags | view `+0x26` |

The final store of the flags is a bitfield merge that ends with all 16 bits of
the wire halfword. Eighteen rows of `0x14` bytes follow the header at `+0x38`.

The word at view `+0x26` is shared by the `0x013B` eighteen-row and
`0x013C` no-row staging routes. It is separate from row count at view
`+0x20` and from the parallel row arrays. Copying this word does not
establish a required constant for either opcode. On the
[default row-log path](#row-processing), its bits 4-10, 0-1, 2-3 and 11-15
become logger arguments 3, 6, 7 and 9 respectively. These partitions are
established carriers; their semantic names and opcode-specific values
remain unestablished. A no-row staging route does not by itself prove a
zero wire count or a particular presentation-flags word.

| Row offset | Width | Field |
| --- | --- | --- |
| `+0x00` | 4 | target actor ID |
| `+0x04` | 4 | numeric value, zero-extended from a wire u16 |
| `+0x08` | 4 | effect ID |
| `+0x0C` | 2 | worldMaster text ID |
| `+0x0E` | 1 | text parameter |
| `+0x0F` | 1 | row ordinal |
| `+0x10` | 4 | reserved, not written |

The wire rows are parallel arrays. The single-row form reads each field once,
and the ten-row and eighteen-row forms read element `i` at the offset plus `i`
times the stride.

| Form | Target (4) | Numeric (2) | Text ID (2) | Effect (4) | Text param (1) | Ordinal (1) | Last byte read |
| --- | --- | --- | --- | --- | --- | --- | --- |
| single | `0x28` | `0x2C` | `0x2E` | `0x30` | `0x34` | `0x35` | 53 |
| ten | `0x28` | `0x50` | `0x64` | `0x78` | `0xA0` | `0xAA` | 179 |
| eighteen | `0x28` | `0x70` | `0x94` | `0xB8` | `0x100` | `0x112` | 291 |
| none | | | | | | | 39 |

The ten-row and eighteen-row loops run from 0 up to the wire count, and the
count is the record's unsigned halfword. Neither loop compares it with 10 or
18. A count past the form capacity writes past the 18 rows of the stack buffer
and reads past the end of the view. The single-row stager stages one row
whatever the count says.

The base stager copies the wire u16 at view `+0x20` to record `+0x30` at
`0x005872CA` for all four forms, and the wrapper stack buffers are not cleared.
The drain post size at `0x0058DA88` (`0x38 + 0x14 * count`) and the row walks at
`0x0058C303` and `0x0058C636` read that count. A `0x013C` record, or a `0x0139`
record, with a nonzero wire count therefore posts and walks rows that were never
staged, and those rows hold whatever the stack held.

When the control halfword is nonzero, or when a staged row has a nonzero text ID,
the stager calls `0x004D7470`, which returns the logger object at container
`+0x510`, and then `0x00578390` with the staged row pointer and the record effect ID. The routine returns at once
for effect IDs whose top byte is `0x10` or `0x14` and for `0x7C000062`. What it
does after that guard was not read.

## Classifier

The wrapper passes the effect ID to `0x00585800` with the fallback class 6. The
function splits the ID through `0x00798370` into a type lane (`ID >> 24`), a
middle lane (bits 12 to 23) and a low lane (bits 0 to 11), and type `0x0B` keeps
the low 24 bits as one value with a zero middle lane. Four types are classes by
themselves and every other type reads a row of the table at `0x00FE32D8`
through `0x007982D0`.

| Rule | Class |
| --- | --- |
| type `0x7C` | `0x07` |
| type `0x6F` | `0x08` |
| type `0x70` | `0x09` |
| type `0x71` | `0x0A` |
| route kind 8 (row 10) | `0x0B` |
| route kind 1 (row 4) with middle lane `0x00D` to `0x013` or `0xFE9` | `0x0D` |
| route kind 1 (row 4) with any other middle lane | `0x0C` |
| route kind 2 (row 21) | `0x0E` |
| route kind 3 (row 22) | `0x0F` |
| route kind 4 (row 23) | `0x10` |
| route kind 5 (row 24) | `0x11` |
| every other row, and a type past the table | the caller fallback |

A row is 12 bytes: a name pointer, a character bank byte, an effect bank byte,
two zero bytes and a route kind dword. The type indexes the table directly, and
the guard accepts indexes up to `0x22`. Rows 0 to 33 are structured. Index 34 is
read from text bytes after the table and gives the fallback. The names of the
rows that carry a route kind from the rules above are `lib` (row 4), `kao`
(row 10), `liu`, `lin`, `lif` and `lil` (rows 21 to 24). The
[bank routing page](../actor/animation-bank-routing.md) reads the same rows
for bank selection. With the fallback 6 a result classifies to a class from 6 to
`0x0A` or from `0x0B` to `0x11`. Row names do not say what a class means.

## Drain

Element slot 6, `0x0058DF90`, calls the drain `0x0058DA10` at `0x0058DFC8` once
per call, and the container calls slot 6 of every element each frame (see the
[scene binding page](../resource/attribute-resource-and-scene-binding.md)). The
drain does nothing when the ring at `+0xA80` is empty. Otherwise it reads the
head record and routes it by class, then pops that one record. The ring stores a
private `0x1A0`-byte copy per slot (the copy routine is `0x0058ECF0`), starts a
push at head plus count, and grows through `0x00590070` when the new count would
reach the capacity.

| Class | Bucket ring | Bus command | Header call |
| --- | --- | --- | --- |
| 0 | `+0x878` | 3 | none |
| 1 | `+0x88C` | 4 | none |
| 2 | `+0x8A0` | 4 | none |
| 3 to 11 | `+0x878` plus `0x14` times the class | 4 | `0x0058CA80` |
| 12 to 25 | `+0x878` plus `0x14` times the class | 4 | none |
| above 25 | none | none | none |

The bucket rings are 26 rings of `0x14` bytes that end at the main ring. The
post passes the record pointer and `0x38 + 0x14 * rowCount` bytes to
`0x004D7980`. That routine adds `0x15` to the command to form the event code
(`0x004D79D2`) and delivers it through slot 3 of the object at element `+0x84`
(`0x004D79EA`). The actor side consumes commands 3 and 4 in the path of the
[action queue page](../actor/action-queue.md#boundary-with-wire-and-vfx-systems).

For classes 3 to 11 the drain then calls the header routine `0x0058CA80` with the
arguments 1 and 0, and `0x0058A010` on the same record. The header routine
returns unless `0x00588E70` agrees. That gate casts the container-selected
element (the override pointer at `+0x17838`, otherwise `+0x17834`) to the
character element type and passes only when the distance between the two
elements is smaller than the radius at element `+0xAA8`.

The radius is an unsigned 32-bit integer. `0x00588E9D` loads it with a signed
integer load and the code after the load adds the constant `4294967296.0` at
`0x00F54A54` when the sign bit is set, so a stored value from `0x80000000`
upward is a large positive radius and never a negative one. The float goes to
`0x005875A0` as its last argument. That routine takes the difference of the
two position vectors that `0x004CFE30` returns, sums the squares of the first
three components, takes the square root and compares with `comisd`
(`0x005876AA`). It returns true only when the radius is strictly greater than
the distance, so a distance equal to the radius fails.

The radius setter `0x00585930` has two modes. Mode 0 loads the float argument,
sets the rounding to truncation, stores it as a 64-bit integer and writes the
low dword to `+0xAA8` (`0x005859D2`). Mode 1 writes 50 for five element kinds.
The helper `0x00587580` passes 35.0. Writers beyond the four setter call sites,
and the unit of the radius, were not traced.

When the gate passes, `0x0058CA80` runs class 5 through `0x00585AF0` and classes
6 to 10 through `0x0058C220`. Classes 3, 4 and 11 run nothing. The
`0x0058A010` call looks for a record in a ring at element `+0xA94`. A sweep of
`0x00580000` to `0x005A0000` for displacements in that ring found no push site,
so the call is expected to find nothing.

## Header line

`0x0058C220` takes the record and two flags. When the control halfword at record
`+0x32` is nonzero it first sends a log line whose text ID is the control value
and whose target, numeric value, effect ID and text parameter are zero, then
clears the control halfword and calls the post routine `0x005785D0` with the
element key and the effect ID. The line uses the same 14-argument call as a row.
The drain calls the routine with the flags 1 and 0, and that pair stops the
routine right after the header, so no row text, plate or sound is produced at
drain time. The routine runs the rows unless the second argument is nonzero and
the third is zero, and the third argument also turns the ordinal filter off.

Because only classes 6 to 10 reach `0x0058C220` at drain time, a result whose
effect ID classifies to `0x0B` through `0x11` gets no header line from the drain.

## Row trigger

Element slot 8, `0x0058E0C0`, receives bus events. It subtracts `0x4A` from the
event code and jumps through a 27-entry table at `0x0058E6B0`. The cases that
matter here are:

| Event | Command | Case | Action |
| --- | --- | --- | --- |
| `0x59` | `0x44` | `0x0058E480` | calls `0x0058DE90` |
| `0x58` | `0x43` | `0x0058E463` | calls `0x0058DBE0` |
| `0x64` | `0x4F` | `0x0058E663` | stages a record |

`0x0058DE90` reads the serial, the class and the route flag from the payload and
returns unless the class is 3 to 11. It searches the bucket ring of that class
for the serial. For the first match it calls `0x0058CA80` with the arguments 0 and
the route flag bit. That routine runs the row pass only for classes 6 to 10 and
only when the range gate passes. For network records the route flag is 1, so every
row runs and the ordinal filter is off. A record with route flag 0 runs one
ordinal group per call. Rows of every other class reach no row processing in the
traced element code. Event `0x58` releases the record from its bucket ring and,
for classes 1 to 11, first runs the header with the arguments 1 and 0.

Event `0x64` classifies the view with the fallback 6, builds a record through the
single-row stager with route flag 0 and pushes it onto the `+0xA80` ring. It is a
second door into the same ring for results that do not come from a packet.

The searches for a publisher of event `0x59` found none:

- The only calls to `0x004D7980` with an immediate push of `0x43`, `0x44` or
  `0x45` before them are at `0x00585722` and `0x00585749`.
- `0x00585722` is inside `0x005856C0`, which posts command `0x43` or `0x44` with
  a 40-byte payload. Its only call or jump site is `0x005871EC`, inside
  `0x00587130`.
- `0x00587130` has no call or jump site, and the addresses of `0x00587130`,
  `0x005856C0` and `0x00585740` occur nowhere as a dword in the raw bytes of any
  section.
- The actor-side poster `0x0054B0D0`, which uses the same command mapping, has no
  call with those immediates.
- `0x00585749` is inside `0x00585740`, which posts command `0x45` and has one
  caller, `0x006ACA52`, inside a routine that creates an element through
  `0x004D90C0`.

A caller that computes the command in a register is outside these scans.

## Row processing

`0x0058C220` walks the rows from 0 to the row count. A selector starts at -1.
Each row runs these steps in order:

1. The row is skipped when the text ID is zero.
2. With the ordinal filter on, the row is skipped when a selector is set and the
   row ordinal differs from it.
3. The plate step runs, described in [Damage plate](#damage-plate).
4. The sound filter `0x005859F0` takes the target and the text ID. A false result
   skips the rest of the row, including the log line and the selector update. The
   [sound effect page](../resource/sound-effect-triggers.md#server-driven-sound)
   owns that routine.
5. The special text ID switch runs.
6. The default log line goes out unless text ID `0x75B4` took its special route.
7. The post routine `0x005785D0` runs with the row target and the record effect ID.
8. The selector becomes the row ordinal and the row text ID is cleared.

The sound filter `0x005859F0` can end a row after its plate. It scans 30 `u16` text IDs at `0x01270BA4` (stride 2, bound `0x3C` bytes).

| Table IDs | Count |
| --- | --- |
| `0x846D`, `0x8471`, `0x8472`, `0x8473`, `0x8479`, `0x847B`, `0x8481`, `0x8482`, `0x84D1`, `0x84D2` | 10 |
| `0x847F` | 1 |
| `0x848E` to `0x8495` | 8 |
| `0x8499` to `0x84A3` | 11 |

On a hit it compares the row target with the dword that `0x004D7490` returns
(container `+0x1783C`, the local actor in the
[sound effect page](../resource/sound-effect-triggers.md#server-driven-sound))
and returns false when they differ, which ends the row before the special
switch, the default line, the post call and the selector update. A text ID that
is not in the table, or a hit whose target matches, goes on to the `0x8475` and
`0x846E` tests and the function returns true. The IDs `0x75B1`, `0x75B4`,
`0x846E`, `0x8475` and `0x8496` to `0x8498` are not in the table. Every ID of the
group list route and `0x847F` are, so those extra routes run only for a row
whose target is that actor, and `0x8493` is in the table as well.

The default log line is one call of `0x005759A0` on the logger object, with these
14 arguments in order:

| Position | Value |
| --- | --- |
| 1 | source element ID (`0x004D6750` on the element) |
| 2 | command ID |
| 3 | presentation flags bits 4 to 10 |
| 4 | pre-action dword 0 |
| 5 | pre-action dword 1 |
| 6 | presentation flags bits 0 to 1 |
| 7 | presentation flags bits 2 to 3 |
| 8 | pre-action float 3 |
| 9 | presentation flags bits 11 to 15 |
| 10 | text ID |
| 11 | target actor ID |
| 12 | numeric value, low 16 bits |
| 13 | effect ID |
| 14 | text parameter |

The logger returns without output when the text ID is zero. The earlier
[battle part-name page](../actor/battle-npc-body-parts.md) follows the same call
for text IDs 30112, 30113 and 30303 into the name resolver and the text delivery.

Four groups of text IDs take extra routes before or instead of that line. The
gate `0x0058C010` passes at once when the record source equals the ID of the
container-selected element, and otherwise runs a longer check that was not read.

| Text IDs | Extra route |
| --- | --- |
| `0x75B1` | plate list at the target element `+0xE24` with the text parameter, gated, then the default line |
| `0x75B4` | plate list at the target element `+0xE24` with 0, gated, no default line |
| `0x847F` | when the target is the container-selected element, `Window_PlayerProfileWidget` is looked up and a value built from the numeric value and the text parameter goes to a global object with a field of that widget, then `Window_ChainBonusEffectWidget` is looked up and receives the numeric value through `0x005391D0`, then the default line |
| `0x848E` to `0x8492`, `0x8494`, `0x8495`, `0x8499` to `0x84A3` | group list at the target element `+0xE4C` with a flag (text parameter nonzero) and the numeric value, then the default line |

The group IDs come from two tests. Text IDs `0x848E` to `0x8492` pass a range
compare at `0x0058C3A2` straight to the group call at `0x0058C582`. Above
`0x8492` the code at `0x0058C56A` subtracts `0x8494`, drops results above 15 to
the default line and indexes the byte table at `0x0058C680` into the two-entry
jump table at `0x0058C678`. Table value 0 selects `0x0058C582` (the group call)
and value 1 selects `0x0058C5A8` (the default line). The table is 0, 0, 1, 1, 1
and eleven zeros, so `0x8494`, `0x8495` and `0x8499` to `0x84A3` take the group
call, while `0x8493` (below the subtracted base) and `0x8496` to `0x8498` get
the default line only.

What those lists and widgets display was not traced.

## Damage plate

The plate step is the block at `0x0058C34A`. It runs for a row that passed the
first two steps, and it runs before the sound filter, so a row the filter drops
still gets its plate. It needs all of these:

- the container override pointer at `+0x17838` is nonzero (`0x004D7430`)
- configuration option (2, 5) is nonzero (`0x004B4E90` over `0x00443E40`, which
  reads a dword of the table at container `+0x17430`)
- the global pointer at `0x01356E10` is set and the global byte at `0x01356E14` is
  clear (the first checks of `0x00795630`)
- the anchor element exists

The anchor comes from `0x00794EE0`, which has no zero test. It reads the effect
ID dword of the row (record `+0x40` plus `0x14` times the row index) and decodes
the kind from bits 27 to 30. Kinds 7 to 10 look up the record source actor and
every other kind, a zero effect ID included, looks up the row target, both
through `0x004D9970` on the container. The call at `0x0058C382` is then
`0x00795630` on the plate list at anchor `+0xD3C` with the record and the row
index.

The plate routine `0x00795630` reloads the row effect ID at `0x007956D4` and,
when it is 0, branches at `0x007956E5` to its epilogue `0x007967A2`. That branch
comes after its calls to `0x00794F30`, `0x00794EE0` (at `0x007956AE`),
`0x004D86B0` and `0x004D9970` and before its inline kind decode at `0x007956ED`
and the layout jump table. A row with effect ID 0 therefore gets no plate of any
kind, and the row pass goes on with the sound filter at `0x0058C387`.

Each list keeps a ring of five slots, advanced by a low-nibble counter that
wraps after 4. A slot object is `0x48` bytes. The list initializer `0x007950A0`,
called from the element constructor at `0x0058B768`, loads
`\system\ingame\DamagePlateWidget.form`. A plate fills the digit blocks
`Label_DamageDisplay:TextBlock_Number_5` through `Number_1`, `IconControl_Icon`,
`TextBlock_Additional` and `TextBlock_DamageType`, and looks up the animation
`DamagePlateAnime.Popup` or `MissPlateAnime.Popup`.

The digit routine `0x00795280` divides the numeric value by 10 five times. The
remainder goes into `Number_5` first, then `Number_4` and so on, and the loop ends
after five blocks. Blocks past the last significant digit take the path used for
a negative value. A value of 100000 or more shows only its low five decimal
digits.

A ten-entry jump table at `0x007967CC` selects the layout by kind 1 to 10.

| Kind | Layout target |
| --- | --- |
| 1, 2, 7 | `0x00795A27` |
| 3 | `0x00795B74` |
| 4, 10 | `0x00795C50` |
| 5 | `0x007963AE` |
| 6 | `0x00795CA6` |
| 8 | `0x00795C0C` |
| 9 | `0x0079609B` |
| 0, 11 to 15 (bounds fallthrough) | `0x007963AE` |

The visual meaning of each kind, the animation durations and the layout inside
the form were not read. No staging or row body reads a status-change field, and
no delay or timer constant is read between receive, drain and row processing.

## Order and timing

1. The packet arrives, passes the pre-gates and the dispatcher resolves the element
   by source actor ID.
2. Slot 9 applies the admission predicate and, if admitted, the wrapper stages the
   record at once. Staging publishes the serial and may call the name request.
3. On the next call of slot 6 the drain takes the head record, copies it to its
   bucket ring and posts it to the scene.
4. In the same pass the header routine runs for classes 3 to 11 inside the range
   gate, and a class 6 to 10 record with a nonzero control halfword sends its
   header line.
5. The rows, with their log lines, sound filter and plates, run when event `0x59`
   arrives for the serial. The publisher is unknown, so the frame is unknown.

## Open questions

- CALIBRATION: the frame on which row processing runs while event `0x59` has no
  identified publisher.
- CALIBRATION: the row cap for a wire count above the form capacity, which the
  client leaves unchecked.
- CALIBRATION: the unit of the range radius at element `+0xAA8`.
- UNPROVED: the display state values written for blocks past the last significant
  digit.
- INFERRED: the ring at element `+0xA94` is never pushed, because a Capstone
  displacement sweep of `0x00580000` to `0x005A0000` found no push site, so the
  drain call `0x0058A010` finds no entry.

## Consumer contract

What a consumer of the staged combat ring can implement from this page, with
the tier of the underlying claims above:

- Drop a result without staging it when the element resolved from the sub-event source ID is in the rejecting state of the admission predicate (the client drops silently, so a drop counter is AUTHORED)
- Stage one row for 0x0139 and the wire row count for 0x013A and 0x013B, with a caller-chosen cap of 10 and 18, and keep the wire count for 0x0139 and 0x013C records as the record count that the post size and row walk read, with a caller-chosen treatment of count against staged rows (AUTHORED)
- Treat a row with text ID 0 as inert: no log line, no plate and no selector change
- Route each drained record by class: copy it to the bucket ring, post scene command 3 for class 0 and 4 otherwise with 0x38 + 0x14 * rows bytes, and drop classes above 25
- At drain time, for classes 6 to 10 inside the range gate, emit the header log line from the control halfword and clear the halfword
- Skip the rest of a row when its text ID is in the 30-ID sound table of the message sound filter and its target is not the actor returned by 0x004D7490
- Emit one log line per row as the 14-argument tuple, in row order, after the message sound filter of the sound effect page
- Create one plate per row whose own effect ID is nonzero, with the kind in its bits 27 to 30, anchored on the source for kinds 7 to 10 and on the row target otherwise, showing the low five decimal digits of the numeric value in a ring of five slots per element
- Take the plate gate as four host booleans: container override pointer set, configuration option (2, 5), global pointer set and global byte clear
- Run the row pass only for classes 6 to 10 inside the range gate, because 0x0058CA80 runs 0x0058C220 for no other class
- Pass the range gate only when the radius, read as an unsigned 32-bit integer and converted to a float, is strictly greater than the three-component distance (the unit stays CALIBRATION)

What stays blocked, and why:

- The publisher of event 0x59, so the frame on which rows run is an AUTHORED choice until a publisher is found
- worldMaster text resolution for the logged message IDs
- Per-kind plate visuals, animation durations and the layout of DamagePlateWidget.form
- The display meaning of the +0xE24 plate list, the +0xE4C group list and the two named widgets
- The initial value of the range radius at element +0xAA8 and any writer beyond its four setter call sites
- The effect of the stage-time request 0x00578390 and of the post routine 0x005785D0 beyond their guards

## Reproduction

```sh
python3 tools/verify_combat_result_element.py
python3 tools/verify_combat_result_element.py --exe orig/ffxivgame.exe
python3 tools/test_combat_result_element.py
```

The first command checks the manifest, this page and the mutations. The second
adds the instruction, table and string checks and recomputes the scans against
the pinned executable.
