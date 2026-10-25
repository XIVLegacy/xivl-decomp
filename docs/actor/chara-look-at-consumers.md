# Chara look-at records and their consumers

A CharaElement keeps six look-at records at `+0x290`. The look-at natives and
six server opcodes write them. This page traces what the client does with
them: the per-frame reader, the scene record it posts, the CharaActor
controller that receives the record and the constraint objects that hold the
target and the weight. The Lua marshalling and the three native writers are in
the [presentation native contract](https://github.com/XIVLegacy/xivl-client-structs/blob/d25367a570b2bf5a83db52eac35463b9b99dd351/docs/presentation-native-contract.md#look-at-natives).
The route of a record from an element to a CharaActor is in the
[container command section](../resource/attribute-resource-and-scene-binding.md#container-command-39-and-non-cut-activation)
and the [item appearance boundary](item-appearance-boundary.md), and this page
restates only what it needs.

An arbiter picks the record with the highest priority in each bank and posts
one scene record that names the bank, the target and a float. The CharaActor
writes those into a slot of a constraint controller. The float is the weight of
an IK object. The solver that turns target and weight into head and eye
rotation was not traced.

## Verdict

| Claim | Tier | Section |
|---|---|---|
| The object at chara +0x84 is the SceneModule with vtable 0x00F91DE8. Its slot 3, 0x004E8390, queues the ScenePacket that 0x004D7980 builds from a record number plus 0x15 | STATIC | [record-path](#record-path) |
| The scene thread pump 0x004E9700 handles commands 3 to 6 itself and passes the others to 0x0060C140, which accepts commands 8 to 0x116 and tail-jumps to 0x007C93C0 | STATIC | [record-path](#record-path) |
| 0x007C93C0 sends commands 0x15 to 0x115 to virtual offset 0x274 of the actor found for the key, with the command minus 0x15, and sends commands 0x0E to 0x14 through a seven entry table | STATIC | [record-path](#record-path) |
| CharaActor slot 157 is 0x00662D30. It ignores every record while bit 6 of the actor dword at +0x2B70 is set and every record above 0x63, and it dispatches through a byte table and a 72 entry jump table | STATIC | [record-path](#record-path) |
| Records 0x0A, 0x0B, 0x0C, 0x0D and 0x0E reach the cases 0x006633CC, 0x0066340C, 0x00663497, 0x00663520 and 0x00663569 | STATIC | [record-path](#record-path) |
| 0x0058F130 initializes each look-at record to a flag word whose low five bits are 11010 (kind 5, bit 4 set, bit 0 clear) with bits 5 to 15 kept, priority 0, decay 0, target 0xC0000000 and weight 1.0 | STATIC | [look-at-records](#look-at-records) |
| The arbiter reads bit 0 of the flag word as a sent marker, bits 1 to 3 as the message kind and bit 4 as a message flag, and it never reads bits 5 to 15 | STATIC | [look-at-records](#look-at-records) |
| The native look-at routes 0x00589260, 0x005892B0, 0x00589310 and 0x00589380 write slot 0 of both banks at priority 0xC8 with decay 0 and skip a record whose priority is above the new one | STATIC | [look-at-records](#look-at-records) |
| The cancel worker stores priority 0 and kind 5, so the arbiter posts nothing for it, because a record without priority never wins and kind 5 selects no message | STATIC | [look-at-records](#look-at-records) |
| No call of the record writer 0x00585F60 passes a nonzero kind flag, so no traced writer produces kind 4 | NOT FOUND | [look-at-records](#look-at-records) |
| No traced writer produces kind 0, so the arbiter never posts record 0x0A | NOT FOUND | [look-at-records](#look-at-records) |
| The named worker 0x00587FD0 is the only caller that gives the record writer a pointer, which the writer copies as 16 bytes to record +0x08, and it is reached from the server case for opcode 0x00DF and from 0x005892B0, which the callback of _lookAtCharacterEid calls through 0x0075C5B0 | STATIC | [look-at-records](#look-at-records) |
| The cases for opcodes 0x00D3 and 0x00DB to 0x00DF sit on the full route of slot 9 that 0x004D8830 selects with a nonzero result, and the stricter route sends all six opcodes to the common exit | STATIC | [server-feed](#server-feed) |
| Opcode 0x00D3 writes the first payload dword as the target of slot 1 in both banks at priority 0x64 through 0x00587E70 and then stores it at chara +0x12C | STATIC | [server-feed](#server-feed) |
| Opcode 0x00DB writes slot 1 at priority 0x64 with the target from payload dword 0 and the weight from dword 1, and it ignores a target whose top three bits are 110 | STATIC | [server-feed](#server-feed) |
| Opcode 0x00DC writes slot 1 at priority 0x64 as a position from payload dwords 0 to 2 with the weight from dword 3 | STATIC | [server-feed](#server-feed) |
| Opcode 0x00DD writes slot 1 at priority 0x64 as kind 3 with two angle floats from payload dwords 0 and 1 and the weight from dword 2 | STATIC | [server-feed](#server-feed) |
| Opcode 0x00DE calls the cancel worker for slot 1, and opcode 0x00DF writes slot 1 at priority 0x64 with a target, a weight and a 16 byte bone name | STATIC | [server-feed](#server-feed) |
| Slot 6 of CharaElement calls the arbiter 0x005861B0 once per call, after 0x00585D70 and 0x0058A090 and before the drain 0x0058DA10 | STATIC | [arbiter](#arbiter) |
| For each bank the arbiter picks the record with the highest priority and the lowest slot on a tie, then lowers every nonzero priority by its decay byte down to 0 | STATIC | [arbiter](#arbiter) |
| The arbiter posts nothing for a bank when no record has priority or when the winner has bit 0 set, and it leaves bit 0 of the three records unchanged in both cases | STATIC | [arbiter](#arbiter) |
| Kind 0 posts record 0x0A, kinds 1 and 4 post record 0x0B or, with a bone name, record 0x0C, kind 2 posts 0x0D, kind 3 posts 0x0E and kinds 5 to 7 post nothing | STATIC | [arbiter](#arbiter) |
| The payloads carry the bank in byte 0 and the weight and the target or vector, and byte 1 holds bit 0 set for kind 4 in record 0x0B, flag bit 4 in bit 0 and kind 4 in bit 1 in record 0x0C and flag bit 4 in bit 0 in records 0x0D and 0x0E, with the sizes 4, 12, 40, 20 and 16 for the records 0x0A, 0x0B, 0x0C, 0x0D and 0x0E | STATIC | [arbiter](#arbiter) |
| The payload priority byte of records 0x0B and 0x0C is the constant 0x64 for kind 1 and 0xC8 for kind 4 and does not come from the record | STATIC | [arbiter](#arbiter) |
| After a message or a kind 5 to 7 decision the winner gets bit 0 set and the other two records of the bank get it cleared | STATIC | [arbiter](#arbiter) |
| Slot 6 calls 0x00586150, which clears bit 0 of all six records, when bits 6 and 2 of byte +0x26D are set, and then clears bit 6 | STATIC | [arbiter](#arbiter) |
| Bit 6 is set by the appearance post 0x00585D70 and bit 2 by element event 0x55, which CharaActor routine 0x0065FDA0 posts | STATIC | [arbiter](#arbiter) |
| CharaActor +0x2620 holds a controller with 2 groups of 3 slots of 0x40 bytes from +0x10, and records 0x0A to 0x0E write the slot whose index is the bank in group 0 or, for the group variant of records 0x0B and 0x0C, in group 1 | STATIC | [actor-controller](#actor-controller) |
| The slot writers 0x00853B50, 0x00853C50 and 0x00853D30 store the weight at +0x18, a node name at +0x20, a vector at +0x30 and the setter flag 0x200, 0x80 or 0x100, and only 0x00853B50 stores a caller target at +0x1C | STATIC | [actor-controller](#actor-controller) |
| A weight equal to 0.0 sets bit 0 of slot byte +8, and the state machine answers it with one call of 0x0065BB50 with argument 1 and clears the bit | STATIC | [actor-controller](#actor-controller) |
| A slot written without a node name takes the name EID_LOOK_AT, and the target resolver asks the target actor for that node and falls back to the target position when the actor has no such node | STATIC | [actor-controller](#actor-controller) |
| EID_LOOK_AT names an attachment point of the target model, from the EID_ prefix and from the node matrix translation the resolver reads | INFERRED | [actor-controller](#actor-controller) |
| 0x00854600 steps each slot through the states 0 to 3 held in the low 3 flag bits and calls the constraint routines 0x00AC95D0, 0x00AC9980, 0x00ACA250 and 0x00AC99B0 | STATIC | [actor-controller](#actor-controller) |
| The binder 0x008540A0 finds the constraints named k0, s0 and t0 for the three slot positions of each group and stores their indexes at slot +0 | STATIC | [actor-controller](#actor-controller) |
| Record 0x0A resets a slot of group 0 to the target 0xC0000000 with weight 1.0 | STATIC | [actor-controller](#actor-controller) |
| The record float travels as the slot weight to 0x00AC99B0 and 0x00ADD760, which store it in the weight field of the IK objects of type 2 and 3 | STATIC | [ik-constraints](#ik-constraints) |
| The resolved target position travels to 0x00AC9980 and 0x00ADD6B0, which store it in the target field of the IK objects of type 2, 3 and 8 | STATIC | [ik-constraints](#ik-constraints) |
| The IK object type code is 2 for LookAtIKObject, 3 for ConstraintDirectionIKObject and 8 for TwoJointIKObject, and LookAtIKObject slot 12 is 0x00AEAB00 | STATIC | [ik-constraints](#ik-constraints) |
| LookAtIKObject slot 2, 0x00AED860, installs the step 0x00AED730, which runs the solve step 0x00AEA570 only while the float at +0xD4 is at least 0.00001 and otherwise clears the state at +0x60, +0xCC and +0xD0 and the active byte at +0x31 | STATIC | [ik-constraints](#ik-constraints) |
| The record float is a blend weight of the IK objects, from the setter pair that writes it to one float of two member types and from the step gate that switches a LookAtIKObject off when the float is nearly 0 | INFERRED | [ik-constraints](#ik-constraints) |
| The solver 0x00AEAB00 of LookAtIKObject, its angle limits, rotation order and blend speed, was not traced | UNPROVED | [ik-constraints](#ik-constraints) |

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
[`config/chara_presentation_consumers.json`](../../config/chara_presentation_consumers.json),
and `tools/verify_chara_presentation_consumers.py --exe` compares each one with
the pinned bytes. The call site scan lists every `E8` and `E9` opcode in the
code section whose rel32 reaches a target. The pattern scan lists every match
of a byte regex in the code section. The dword scan lists every occurrence of
a little-endian dword in the raw bytes of all sections.

Class names are the RTTI names of
[`config/ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
Offsets written `+0x..` are bytes from the start of the named object. A record
is the number that an element passes to `0x004D7980`, and a command is the
number the scene thread sees, which is the record plus `0x15`. A bank is 0 or 1
and a slot is 0 to 2. The record index is `slot + 3 * bank`.

The scans do not follow indirect calls. The ghidra-cli export has no function
bounds for some of this code, so a caller list built from its cross-references
alone can miss callers.

## Record path

An element sends a record through `0x004D7980`. The routine adds `0x15` to the
record number, keys the packet with the element identity at `+0x88` and calls
virtual slot 3 of the object at `+0x84` (`0x004D79EA` to `0x004D79F6`). That
object is the SceneModule, RTTI `Application::Scene::SceneModule`, vtable
`0x00F91DE8`. Its slot 3, `0x004E8390`, queues the packet in the scene thread
ring. The element constructor stores the object at `+0x84` (`0x004DABC4`), and
the [scene binding page](../resource/attribute-resource-and-scene-binding.md#container-command-39-and-non-cut-activation)
traces where the constructor argument comes from.

The scene thread pump `0x004E9700` jumps through a four entry table for the
commands 3 to 6 (`0x004E9A14`) and sends every other command, with key, data
and size, to `0x0060C140`. That routine accepts the commands 8 to `0x116` and
tail-jumps to the container dispatcher `0x007C93C0`. The dispatcher looks the
key up. For the commands `0x0E` to `0x115` an unknown key ends with no action.
For the commands `0x15` to `0x115` it loads virtual offset `0x274` of the actor
and calls it with the command minus `0x15`, which is the record number again.
The commands `0x0E` to `0x14` go through the seven entry table at `0x007C9720`.
The entries for `0x10` and `0x11` are the visibility cases of the
[turn and field page](chara-turn-and-field-consumers.md#visibility).

| Record | Command | Case | Calls |
|---|---|---|---|
| `0x0A` | `0x1F` | `0x006633CC` | `0x0065CEB0` |
| `0x0B` | `0x20` | `0x0066340C` | `0x0065CEB0` or `0x0065CF00`, then `0x0065D020` |
| `0x0C` | `0x21` | `0x00663497` | `0x0065CEB0` or `0x0065CF00` |
| `0x0D` | `0x22` | `0x00663520` | `0x0065CFA0` |
| `0x0E` | `0x23` | `0x00663569` | `0x006617F0` |

CharaActor, vtable `0x00FC0D34`, has the record handler in slot 157,
`0x00662D30`. It first calls its own slot 154, `0x00669650`, which returns bit 6
of the actor dword at `+0x2B70`, and returns without acting when that bit is
set. It ignores every record above `0x63`. The remaining records go through the
byte table `0x00663C08` and the jump table `0x00663AE8`, which has 72 entries.
Thirty records, among them `0x02`, `0x11`, `0x42` and `0x4A` to `0x60`, share the
empty case `0x00663AD4`. The same handler serves the turn, floating offset and
heading records of the [turn and field page](chara-turn-and-field-consumers.md).

## Look-at records

The six records are 0x30 bytes each from `+0x290`.

| Offset | Width | Field |
|---|---:|---|
| `+0x00` | 2 | Flag word |
| `+0x02` | 1 | Priority |
| `+0x03` | 1 | Decay |
| `+0x04` | 4 | Target identity |
| `+0x08` | 16 | Bone name, first byte 0 when absent |
| `+0x18` | 4 | Weight, float |
| `+0x20` | 12 | Three floats |

The element constructor initializes the array through `0x0058F130` (`0x0058B5D9`
to `0x0058B619`). Each record starts with low flag bits 11010, priority 0,
decay 0, target `0xC0000000` and weight 1.0, the float at `0x00F54F70`. The
flag word is not stored whole: the initializer keeps bits 5 to 15 of the old
memory and fixes the low five bits to 11010 (kind 5, bit 4 set, bit 0 clear).
The arbiter never reads bits 5 to 15. A host that keeps all-zero records
differs in the low flag bits, the target and the weight.

The arbiter reads three parts of the flag word. Bit 0 marks the record that won
the last arbitration. Bits 1 to 3 hold the message kind, so `(flags >> 1) & 7`
selects the case. Bit 4 is copied into the message. Bits 5 to 15 are not read.

| Kind | Bits set among 1 to 3 | Written by |
|---|---|---|
| 0 | none | No traced writer |
| 1 | 1 | `0x00585F60` with a zero kind flag, and `0x00587E70` |
| 2 | 2 | `0x00588020`, and `0x00586030` which no call reaches |
| 3 | 1, 2 | `0x005880F0` |
| 4 | 3 | `0x00585F60` with a nonzero kind flag |
| 5 | 1, 3 | The cancel `0x00587EF0` and the initializer |

The record writer `0x00585F60` is the common path of kinds 1 and 4. It skips a
record whose priority is above the new one. Otherwise it clears bit 0, sets the
kind bits, sets bit 4, stores the priority, the decay argument, the weight and
the target, and replaces a target equal to the own element key by
`0xC0000000`. A nonzero name pointer is copied as 16 bytes to `+0x08` and a
null pointer clears the byte at `+0x08`. The two callers of the writer,
`0x00587F80` and `0x00587FD0`, write both banks and pass decay 0, so a
priority never falls in the traced code. No call of the writer passes a
nonzero kind flag, so kind 4 is not produced.

The native routes write slot 0 at priority `0xC8`. They are `0x00589260` for
`_lookAtCharacter`, `0x00589310` for `_lookAtPosition` and `0x00589240` for the
cancel, all in the contract. Two more routes reach the same workers.
`0x005892B0` is called through `0x0075C5B0` by the callback `0x006E7890`, which
[`manifests/control_class_napi_field_access.json`](https://github.com/XIVLegacy/xivl-client-structs/blob/d25367a570b2bf5a83db52eac35463b9b99dd351/manifests/control_class_napi_field_access.json)
lists as `_lookAtCharacterEid`. It calls `0x00587FD0` with a bone name from the
Lua string. `0x00589380` is called through `0x0075C630` and reaches the angle
worker `0x005880F0` with kind 3. The server routes below write slot 1 at
priority `0x64`. A script target at `0xC8` therefore outranks a server target.
The routine at `0x00562D30` also calls the server character entry
`0x00589290`, at `0x00562DEE`, with weight 1.0.

The cancel worker `0x00587EF0` stores priority 0, decay 0, target `0xC0000000`
and weight 1.0, and sets the flag word to kind 5 with bit 0 clear. The arbiter
ignores a record without priority and posts nothing for kind 5, so the cancel
posts no message by itself. The next lower priority record wins and posts its
message, or no message goes out when no record has priority. No traced code
posts record `0x0A`, because no writer produces kind 0.

## Server feed

The element receive handler, slot 9, `0x0058CCA0`, calls `0x004D8830` first. A
nonzero result takes the full route at `0x0058CD21`, which indexes the opcode
minus `0x0F` through the byte table `0x0058D7E4` and the jump table
`0x0058D788`. A zero result takes the stricter route at `0x0058CCEE`, which
indexes `0x0058D6B0` and `0x0058D6A4` and sends all six opcodes below to the
common exit `0x0058D67D`. The [combat result page](../net/combat-result-element.md#admission)
describes the predicate. The opcode catalog names 0x00D3 as
SetActorTargetAnimatedPacket and 0x00DB as SetActorTargetPacket and leaves
0x00DC to 0x00DF unnamed. Payload dwords count from the first dword after the
16 byte game-message header.

| Opcode | Case | Entry | Worker | Payload |
|---|---|---|---|---|
| `0x00D3` | `0x0058D039` | none | `0x00587E70` | Target in dword 0 |
| `0x00DB` | `0x0058D1A1` | `0x00589290` | `0x00587F80` | Target in dword 0, weight in dword 1 |
| `0x00DC` | `0x0058D1CE` | `0x00589350` | `0x00588020` | Position in dwords 0 to 2, weight in dword 3 |
| `0x00DD` | `0x0058D1F8` | `0x005893B0` | `0x005880F0` | Two angles in dwords 0 and 1, weight in dword 2 |
| `0x00DE` | `0x0058D21B` | none | `0x00587EF0` | None |
| `0x00DF` | `0x0058D229` | `0x005892E0` | `0x00587FD0` | Target in dword 0, weight in dword 1, bone name from `+0x18` |

Every case except `0x00DE` writes slot 1 at priority `0x64`, and `0x00DE`
calls the cancel worker for slot 1. `0x00DB` returns without writing
when the target has `110` in its top three bits (`0x0058D1A6` to `0x0058D1B2`).
`0x00D3` calls `0x00587E70`, which writes kind 1 with weight 1.0 into slot 1 of
both banks, skips a bank whose slot 1 priority is above `0x64` and replaces a
target equal to the own key by `0xC0000000`. It then stores the dword at
element `+0x12C` (`0x0058D047`). That field is the third source of
`_getLookAtCharacter` in the contract.

## Arbiter

Slot 6 of CharaElement, `0x0058DF90`, runs once per call. It calls
`0x00585D70`, `0x0058A090`, the arbiter `0x005861B0` and the drain `0x0058DA10`
in that order (`0x0058DFB3` to `0x0058DFC8`). The arbiter walks the banks 0 and
1 and handles each bank alone.

1. Visit the three records in slot order. A record whose priority is above the
   best so far becomes the best, so the lowest slot wins a tie.
2. In the same visit lower each nonzero priority by its decay byte, not below 0.
   The best value is the one before the decay.
3. Stop for the bank when the best priority is 0 or when the winner has bit 0
   set. Bit 0 stays unchanged in both cases.
4. Read the kind from the winner and post the message of the next table.
5. Set bit 0 on the winner and clear it on the other two records. This also
   runs for the kinds 5 to 7, which post nothing.

| Kind | Record | Size | Payload priority byte |
|---|---|---:|---|
| 0 | `0x0A` | 4 | none |
| 1 | `0x0B`, or `0x0C` with a bone name | 12, or 40 | `0x64` |
| 2 | `0x0D` | 20 | none |
| 3 | `0x0E` | 16 | none |
| 4 | `0x0B`, or `0x0C` with a bone name | 12, or 40 | `0xC8` |

The payloads are built on the stack and the bytes that the builder does not
write hold stack contents. The handlers read only the fields below.

| Record | Offset | Field |
|---|---|---|
| `0x0A` | `+0x00` | Bank |
| `0x0B` | `+0x00`, `+0x01`, `+0x02` | Bank, bit 0 set for kind 4 and clear for kind 1, priority byte |
| `0x0B` | `+0x04`, `+0x08` | Target, weight |
| `0x0C` | `+0x00`, `+0x01`, `+0x02` | Bank, bit 0 the flag bit and bit 1 set for kind 4, priority byte |
| `0x0C` | `+0x04`, `+0x08`, `+0x18`, `+0x1C` | Target, bone name with the last byte forced to 0, weight, three floats |
| `0x0D` | `+0x00`, `+0x01`, `+0x04`, `+0x08` | Bank, bit 0 the flag bit, weight, three floats |
| `0x0E` | `+0x00`, `+0x01`, `+0x04`, `+0x08` | Bank, bit 0 the flag bit, weight, two angle floats |

The priority byte is a constant of the kind and does not come from the record.
Kind 2 sends the record floats at `+0x20` to `+0x28`, and kind 3 sends the
floats at `+0x20` and `+0x24`.

The resend routine `0x00586150` clears bit 0 of all six records. Slot 6 calls
it at `0x0058DFA5` when bits 6 and 2 of the element byte `+0x26D` are both set,
and then clears bit 6. `0x00585D70` sets bit 6 at `0x00585E09` after it posts
record 8, the appearance record of the scene binding page. Element event `0x55`
sets bit 2 (`0x0058E407`). The event is posted by `0x0065FDA0` at `0x0065FF37`,
and what it signals was not traced. Once both bits are set after an appearance
post, every record is sent again.

## Actor controller

CharaActor `+0x2620` holds the controller. Its slots are 0x40 bytes each from
`+0x10`, in two groups of three, so slot `s` of group `g` is at
`+0x10 + (s + 3 * g) * 0x40`. The record handlers write the slot whose index is
the bank.

| Slot offset | Field |
|---|---|
| `+0x00` | Constraint index, negative when the constraint is missing |
| `+0x04` | Flag dword, low 3 bits are the state |
| `+0x08` | Byte, bit 0 set for a zero weight |
| `+0x10` to `+0x14` | Three words |
| `+0x18` | Weight |
| `+0x1C` | Target identity |
| `+0x20` | Node name, 16 bytes |
| `+0x30` | Four floats |

Three writers fill a slot. All keep the bits of the old flag dword under
`0xFFFC0037`, set `0x20`, set `0x08` for a nonzero presence argument, set `0x40`
for a nonzero flag argument and set one setter flag. The `0x200` writer sets
`0x08` only for a target other than `0xC0000000`. The `0x200` writer stores the
default node name `EID_LOOK_AT` when the name pointer is null and the other two
always store it.

| Writer | Setter flag | Wrappers | Stores a target |
|---|---|---|---|
| `0x00853B50` | `0x200` | `0x00854830` for group 0, `0x00854880` for group 1 | Yes |
| `0x00853C50` | `0x80` | `0x008548E0` | No, `0xC0000000` |
| `0x00853D30` | `0x100` | `0x00854D20` | No, `0xC0000000` |

The group 0 wrapper stores the words `100`, 0 and 0. The group 1 wrapper keeps
the words it is given. Record `0x0A` calls `0x0065CEB0` with the target
`0xC0000000` and weight 1.0. Records `0x0B` and `0x0C` call `0x0065CEB0` for
group 0 and `0x0065CF00` for group 1. Group 1 is chosen by bit 0 of byte 1 for
`0x0B` and by bit 1 of byte 1 for `0x0C`, so only kind 4 reaches it. Record
`0x0D` calls the `0x80` writer through `0x0065CFA0` and record `0x0E` the
`0x100` writer through `0x006617F0`. `0x0065D020` sets or clears flag `0x10` of
a slot through `0x00853630`. Flag `0x10` blocks the activation and ends the
active state in the state machine below.

The binder `0x008540A0` asks the constraint set at controller `+0x00` for the
index of the names `k0`, `s0` and `t0` (pointer table `0x012D135C`) and stores
the three indexes at the slot positions of both groups. A name the set lacks
leaves a negative index. The constraint lookup `0x00AC93A0` compares the name
with each constraint.

The state machine `0x00854600` handles one slot per call and acts only when its
second argument equals whether the state is 0. `0x00854B20` runs every slot once
with 1, which reaches the state 0 slots, and once with 0, which reaches the
others.

| State | Action | Next |
|---|---|---|
| 0 | `0x00AC95D0` with the index, 0.0 and 1, weight to `0x00AC99B0`, zero weight call | 1 |
| 1 | None | 2 when flag `0x10` is clear and flags `0x08`, `0x20000` and `0x40000` are set |
| 2 | Resolve the target, then `0x00AC9980` with the position, `0x00ACA250` with 0.0 and 1, weight, zero weight call | 3 |
| 3 | When flag `0x20` or `0x40` is set clear `0x20`, resolve, set position and weight | 0 when flag `0x10` is set, flag `0x08` or `0x40000` is clear or `0x20000` is clear |

A slot with a zero weight carries bit 0 of byte `+0x08`. The state machine
answers it by calling `0x0065BB50` with 1 on the object at actor `+0x2830` and
clearing the bit. That call changes a mode field of that object and its effect
was not traced.

The target resolver `0x00853720` returns a world position. With flag `0x200` it
looks the target identity up in the registry reached through actor `+0x118`,
asks the target for the node of the slot name and reads the translation of the
node matrix. When the target has no such node it uses the target position. It
adds the vector at `+0x30`. With flag `0x80` the vector is used as a position. With
flag `0x100` the vector holds two angles and the position follows from them
and the node `EID_LOOK_AT1` of the own actor. With flag `0x400`, which
`0x008549E0` sets for a caller in `0x00664460`, the position follows the own
actor. The angle and `0x400` branches were read less closely than the other
two.

## IK constraints

The constraint calls take the constraint set at controller `+0x00` and an index.
`0x00AC9980` and `0x00AC99B0` act only on a constraint whose field at `+4` is 7.
They pass the group at constraint `+0x40` to the setters, which walk the members
of the group by the virtual type code at offset `+0x14`.
`0x00ADD6B0` stores the position vector in the member of type 2 at `+0xF0`, of
type 3 at `+0x90` and of type 8 at `+0x60`. `0x00ADD760` stores the weight in
the member of type 2 at `+0xD4` and of type 3 at `+0x5C`.

The RTTI names of the members come from the vtable catalog. The type codes are
the constants returned by slot 5 of each vtable.

| Type | Class | Vtable |
|---:|---|---|
| 2 | `SQEX::CDev::Engine::Phieg::IKDynamics::LookAtIKObject` | `0x010A92E4` |
| 3 | `SQEX::CDev::Engine::Phieg::IKDynamics::ConstraintDirectionIKObject` | `0x010A91F4` |
| 8 | `SQEX::CDev::Engine::Phieg::IKDynamics::TwoJointIKObject` | `0x010A9140` |

The record float therefore reaches the IK objects unchanged. LookAtIKObject slot
2, `0x00AED860`, installs the step `0x00AED730` at object `+0x28`. While the
active byte at `+0x31` is set, the step runs the solve step `0x00AEA570` only if
the float at `+0xD4` is at least 0.00001 (`0x00FEE2B4`). Below that it clears
`+0x60`, `+0xCC` and `+0xD0` and the active byte. A float near 0 therefore
switches the look-at off, which fits the name weight that this page uses for the
float. That reading is INFERRED. The zero weight flag of the controller slot is
the controller side of the same case.

Slot 12 of `LookAtIKObject` is `0x00AEAB00`, a routine of 8,364 bytes that is
the likely solver. Its angle limits, rotation order and blend speed were not
traced.

## Open questions

- CALIBRATION: the head and eye rotation limits, rotation order and blend speed of the LookAtIKObject solver
- UNPROVED: the role of the group 1 slots, which only kind 4 reaches, and of the call 0x0065BB50 for a zero weight
- UNPROVED: the Lua natives that reach the angle worker through 0x0075C630 and the purpose of the call at 0x00562DEE
- UNPROVED: the target resolver branches for the setter flags 0x100 and 0x400

## Consumer contract

What a consumer of the look-at records can implement from this page, with the
tier of the underlying claims above:

- Seed every look-at record with low flag bits 11010 (bits 5 to 15 are never read), priority 0, decay 0, target 0xC0000000 and weight 1.0, where the native host kept all zero
- Keep the native writers on slot 0 at priority 0xC8 and add the server writers on slot 1 at priority 0x64 for opcodes 0x00D3 and 0x00DB to 0x00DF, so a script target outranks a server target
- Run the arbiter once per CharaElement slot 6 call (a per frame cadence is AUTHORED) and post the five records with the payload layouts of this page to a host sink, because a pose layer needs the winning target and weight and not the retail IK objects
- Let the native cancel only clear slot 0 and post nothing, and let the server cancel clear slot 1
- Take the target of _getLookAtCharacter from the value that opcode 0x00D3 stored, starting from 0xC0000000
- Treat a look-at weight below 0.00001 as inactive

What stays blocked, and why:

- The LookAtIKObject solver 0x00AEAB00, so the head and eye rotation, its angle limits and its blend speed are a host choice (CALIBRATION)
- The effect of the zero weight flag call 0x0065BB50 and of the group 1 slots that only kind 4 reaches
- Which Lua natives reach the angle and position workers beyond the contract rows, and the user of the server character entry at 0x00562DEE

## Reproduction

```sh
python3 tools/verify_chara_presentation_consumers.py
python3 tools/verify_chara_presentation_consumers.py --exe orig/ffxivgame.exe
python3 tools/test_chara_presentation_consumers.py
```

The first command checks the manifest, both pages and the mutations. The second
adds the instruction, table and string checks and recomputes the scans against
the pinned executable.
