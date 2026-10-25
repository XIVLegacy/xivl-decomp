# Chara turn, visibility, floating offset and marker consumers

The presentation natives `_turnBack`, `_waitForTurning`, `_setVisible`,
`_setFloatingOffset` and `_setMapMarker` write chara fields and send scene
records. This page traces what reads those fields and records: the turn
controller of the CharaActor and the producer of the event that ends a turn
wait, the visibility byte, the floating offset easing, the map marker readers
and the code that applies the pending marker of an NpcBase. It also records the
writers of the two target fields that `_getLookAtCharacter` reads. The Lua
marshalling and the field writers are in the
[presentation native contract](https://github.com/XIVLegacy/xivl-client-structs/blob/d25367a570b2bf5a83db52eac35463b9b99dd351/docs/presentation-native-contract.md).
The record route from an element to a CharaActor, and the look-at records, are
on the [look-at page](chara-look-at-consumers.md#record-path).

## Verdict

| Claim | Tier | Section |
|---|---|---|
| Record 0x33 from the turn back native reaches CharaActor case 0x00662FBC, which calls the turn back routine 0x007B0690 on the object at actor +0xBF0 | STATIC | [turn-request](#turn-request) |
| 0x007B0690 posts event 0x4A at once when no heading was saved or when the heading is within 0x38E39 units of the saved one, and otherwise starts a turn to the saved heading and clears the saved flag | STATIC | [turn-request](#turn-request) |
| The heading setter 0x00587D40 sets bit 1 of byte +0x26D and posts record 0x32, which CharaActor case 0x00662F94 hands to 0x0065C220 and the turn start 0x007B0530 | STATIC | [turn-request](#turn-request) |
| 0x007B0530 posts event 0x4A for a turn still pending, stores the target or saved heading, classifies the angle into six classes and starts the rotation and the timer | STATIC | [turn-request](#turn-request) |
| The angle classes are -1, -2, -3, 3, 2 and 1 for differences up to 0x1C71C7, 0x4E38E3, 0x800000, 0xB1C71C, 0xE38E39 and above in 24 bit units of 360 over 2 to the 24 degrees | STATIC | [turn-request](#turn-request) |
| 0x007A4E70 subtracts its second argument from its first and masks the result to 24 bits, and 0x007B0530 pushes the current angle at +0x1AC first and the target angle second, so the difference is the target minus the current angle | STATIC | [turn-request](#turn-request) |
| After the class call, a negative class adds one circle to the target when the current angle is greater than it and a positive class subtracts one circle when the current angle is smaller, and 0x007A4CD0 stores that target at +0x1B4 and the current angle at +0x1B0 | STATIC | [turn-request](#turn-request) |
| The angle at +0x1AC is not shown to stay inside 0 to 0xFFFFFF between turns, because the chase near 0x007AB040 stores an unmasked sum, difference or target at +0x1AC (0x007AB0AA, 0x007AB0C1 and 0x007AB0D4) while 0x007A4B31 and 0x007A7DE3 store masked values | UNPROVED | [turn-request](#turn-request) |
| The turn timer at controller +0x38 starts at 10.0 or at the float of the low 6 bits of the dword at controller +0x258 if that is larger, and mode 2 turns store 0 in +0x1C4 | STATIC | [turn-completion](#turn-completion) |
| Each CharaActor slot 142 call lowers the timer by the delta times the controller scale at +0xF8, and when it reaches 0 the controller posts event 0x4A | STATIC | [turn-completion](#turn-completion) |
| The only calls of the event post 0x0065BEF0 are the timer expiry 0x007A4C20, the pending turn replacement 0x007B0547 and the turn back shortcut 0x007B06F6 | STATIC | [turn-completion](#turn-completion) |
| 0x007CE7B0 queues the event with its code plus 3 in the ring at container +0x17928, 0x004DAA10 subtracts 3 and calls element slot 8, and case 0x4A of 0x0058E0C0 clears bit 1 of byte +0x26D | STATIC | [turn-completion](#turn-completion) |
| Bit 1 of byte +0x26D is set at 0x00585EC0 and 0x00587DF2 and cleared by a pattern at 0x0058E373 only | STATIC | [turn-completion](#turn-completion) |
| The unit of the frame delta, the origin of the low 6 bits at controller +0x258 and the angle chase that follows 0x007A4CD0 were not traced | UNPROVED | [turn-completion](#turn-completion) |
| Commands 0x10 and 0x11 reach 0x007CCCC0 with 1 and 0 through the command table entries 0x007C9696 and 0x007C96B4 | STATIC | [visibility](#visibility) |
| 0x007CCCC0 sets bit 1 of the actor byte at +0xE8 when the argument is 0 and clears it otherwise, and CharaActor slot 43 reports visible when that byte is 0 | STATIC | [visibility](#visibility) |
| When the visibility answer changes, 0x007CCCC0 calls 0x00A5FA10, which forwards to the object at actor +0x0C, and virtual offset 0x250 of the actor, which is the empty routine 0x00712B40 for CharaActor | STATIC | [visibility](#visibility) |
| Command 0x3B is record 0x26, whose CharaActor case 0x00663074 calls 0x0065CC60 with the float at payload +4 and 0x007A42B0 stores it as the target at +0x588 of the object at actor +0x590 | STATIC | [floating-offset](#floating-offset) |
| 0x007A7280 adds to the current value at +0x58C the smaller of 0.02 and the difference between target and current times 0.02 times the delta in state 0, so an upward step is capped at 0.02 and a downward step is not | STATIC | [floating-offset](#floating-offset) |
| The placement routine 0x00660F70 adds the current value, read by 0x007A42F0, to the probed ground height when the probe reports a hit and its gates pass | STATIC | [floating-offset](#floating-offset) |
| The probed value is the actor height, from the addition into the second position component and the native name | INFERRED | [floating-offset](#floating-offset) |
| The marker number is the low 6 bits of the dword at chara +0x270, read by 0x00585300 at 0x006808BC in 0x00680810 and at 0x006A2EA9 in 0x006A2E90 | STATIC | [map-marker](#map-marker) |
| 0x00680810 uses 0 for the chara stored at +0x9E0 of its context and -1 for another chara when bit 2 of byte +0x26D is clear or the number is 0 or above 0x40, passes -1 to the untraced 0x00680730, and clears the entity only for 0x3F | STATIC | [map-marker](#map-marker) |
| Bits 6 to 10 of the same dword are read by 0x00585310 at 0x006809E9 and only 0x00680810 calls it | STATIC | [map-marker](#map-marker) |
| The two marker strings are the 0x20 byte buffers at +0x34 of the two 0x54 byte records at chara +0x3B0 and +0x404, and 0x005881C0 clears them with the record flags | STATIC | [map-marker](#map-marker) |
| No instruction reads the string buffers at chara +0x3E4 or +0x438 by displacement, and the readers behind the record base were not traced | NOT FOUND | [map-marker](#map-marker) |
| NpcBase slot 12, 0x006F8660, calls the base routine 0x006E18B0 and, when it returns the byte at +0x5C as nonzero, calls 0x0076D920 with the pending number and vector and then releases the pending object at +0x128 | STATIC | [map-marker](#map-marker) |
| Slot 12 of CharaBase, PlayerBase and MyPlayer is 0x006E18B0 and slot 12 of NpcBase is 0x006F8660 | STATIC | [map-marker](#map-marker) |
| Chara +0x12C starts at 0xC0000000 and the case for opcode 0x00D3 stores payload dword 0 in it | STATIC | [target-fields](#target-fields) |
| NpcBase +0x154 is set to 0 by the NpcBase constructor | STATIC | [target-fields](#target-fields) |
| No function in the NpcBase vtable stores to +0x154 | NOT FOUND | [target-fields](#target-fields) |

STATIC means read from instructions or data in the pinned executable. INFERRED
means the evidence is consistent with the claim and does not close it. UNPROVED
means no evidence addresses it. NOT FOUND means the named bodies, call sites or
data were searched by the stated method and hold no such behavior.

## Binary and method

The executable, the tools and the scans are those of the
[look-at page](chara-look-at-consumers.md#binary-and-method), and the rows cited
here are in the same manifest,
[`config/chara_presentation_consumers.json`](../../config/chara_presentation_consumers.json).
A record is the number an element passes to `0x004D7980` and a command is the
record plus `0x15`. The byte `+0x26D` and the dwords at `+0x270`, `+0x12C` and
`+0xB0` are CharaElement fields. Fields written `actor +0x..` belong to the
CharaActor, vtable `0x00FC0D34`.

## Turn request

`_turnBack` sets bit 1 of the element byte `+0x26D` and sends record `0x33`,
command `0x48`. The CharaActor case `0x00662FBC` for that record computes the
object at actor `+0xBF0` and calls `0x007B0690` on it. That object is the turn
controller. None of its methods is a virtual method, so the vtable catalog
names no class for it. The fields below are the ones the traced routines use.

| Controller offset | Use |
|---|---|
| `+0x04` | Flag dword, bit 1 holds a saved heading |
| `+0x38` | Turn timer, float |
| `+0x70` | Heading in 24 bit units, `0x1000000` per circle |
| `+0xE8` | Clock, advanced by the scaled delta |
| `+0xF8` | Time scale, float |
| `+0x1AC` | Current angle in the same units |
| `+0x1B0` to `+0x1B8` | Start angle, target angle, state |
| `+0x1BC` | Saved heading in radians |
| `+0x1C4` | Turn duration, float |
| `+0x258` | Dword whose low 6 bits feed the duration |

`0x007B0690` has two outcomes.

1. When bit 1 of `+0x04` is clear, it posts event `0x4A` at once by a tail jump
   to `0x0065BEF0`.
2. Otherwise it clears the bit and converts the saved radians at `+0x1BC` to
   units. It takes the difference from the heading at `+0x70` modulo
   `0x1000000`. A difference from `0x38E39` to `0xFC71C7`, which is more than 5
   degrees either way, calls `0x007B0530` with the saved radians and two zero
   arguments. Any other difference posts event `0x4A`.

The heading setter `0x00587D40` also takes part. When the new heading differs
from the current one, or when one of the element bytes `+0x264` and `+0x265` is
set, it sets bit 1 of `+0x26D` at `0x00587DF2`, stores the heading at `+0xAC`
and posts record `0x32` with a float angle and two zero words (`0x00587E5D`).
The CharaActor case `0x00662F94` reads the word at payload `+6`, maps it
through the table `0x00FC0B78` (0, 0, 1, 2, 3 for the words 0 to 4, and 0 above)
and calls `0x0065C220` with the angle, that value and 1. `0x0065C220` calls the
turn start `0x007B0530`. Record `0x30` takes the same path with a zero last
argument, and record `0x31` goes through `0x0065C250` to `0x007B1520`, which was
not traced.

`0x007B0530` takes an angle in radians, a mode and a flag.

1. When the timer at `+0x38` is above 0, it sets the timer to 0 and posts
   event `0x4A`.
2. With a zero flag it stores the angle as the heading at `+0x70`. With a
   nonzero flag it sets bit 1 of `+0x04` and stores the current heading, in
   radians, at `+0x1BC`.
3. It classifies the target angle minus the current angle at `+0x1AC` through
   `0x007A4E70`, calls the object at `+0x284` with the class and the mode when
   that object exists, and moves the target by one circle so that it lies on the
   side the class names (see below).
4. It calls `0x007A4CD0`, which stores the start and target angles, the state 1
   and zero in `+0x1C0` and `+0x1C8`, and puts the float of the low 6 bits of
   the dword at `+0x258` in `+0x1C4`. Mode 2 sets flag `0x80` in `+0x04` and
   zeroes `+0x1C4`. Any other mode raises `+0x1C4` to 10.0 when it is lower.
5. It stores the timer: `+0x1C4` when that is above 10.0 and 10.0 otherwise.

The angle classes of `0x007A4E70` use the 24 bit difference `u`. The routine
returns its first argument minus its second, masked with `0xFFFFFF`, and
`0x007B0530` pushes the current angle at `+0x1AC` first and the target angle
second (the call at `0x007B05EB`), so `u` is the target minus the current angle.
The bounds are exact in units of 360 over 2 to the 24 degrees, and the last
column rounds them to whole degrees.

| `u` up to | Class | Degrees |
|---|---:|---|
| `0x1C71C7` | -1 | 40 |
| `0x4E38E3` | -2 | 110 |
| `0x800000` | -3 | 180 |
| `0xB1C71C` | 3 | 250 |
| `0xE38E39` | 2 | 320 |
| above | 1 | 360 |

After the call the target is unwrapped against the current angle (`0x007B0609`
to `0x007B062D`). A negative class adds `0x1000000` to the target when the
current angle is greater than it, and a positive class subtracts `0x1000000`
when the current angle is smaller. The unwrapped target therefore lies above the
current angle by `u` for the classes -1 to -3 (`u` up to `0x800000`) and below
it by `0x1000000` minus `u` for the classes 3, 2 and 1, when the current angle
lies in 0 to `0xFFFFFF`. The two compares are signed (`jle` at `0x007B0613` and
`jge` at `0x007B0625`), so the add and subtract rule above holds for any current
angle, while the sides stated hold only inside that range. `0x007A4CD0` then
stores the current angle at `+0x1B0` and the unwrapped target at `+0x1B4`.

Whether the angle at `+0x1AC` stays inside 0 to `0xFFFFFF` between turns is not
established. `0x007A4B31` and `0x007A7DE3` store it after `and 0xFFFFFF`. The
chase near `0x007AB040` masks only the copy it compares, and stores the sum
(`0x007AB0C1`), the difference (`0x007AB0AA`) or the target loaded from `+0x1B4`
(`0x007AB0D4`) at `+0x1AC` unmasked. The unwrapped target at `+0x1B4` can lie
below 0 or above `0xFFFFFF`, and whether the chase stops before such a value
reaches `+0x1AC` was not traced. Those are the writers that were read, not a
search of every writer. Which way the heading value increases on screen, and
what runs the chase, were not traced.

## Turn completion

The CharaActor update, slot 142, `0x006679C0`, calls `0x007A4B60` on the
controller with its frame delta (`0x00667F88`). The routine adds the product of
the delta and the scale at `+0xF8` to the clock at `+0xE8`. When the timer is
above 0 it lowers it by the same product. When the result is 0 or below it
stores 0 and posts event `0x4A` through `0x0065BEF0`.

The event post `0x0065BEF0` calls the actor queue `0x007CE7B0` with the code
`0x4A`. That routine builds a packet with the code plus 3 and the key at actor
`+0xDC`, and queues it in the ring at container `+0x17928`. The container
drain `0x004DAA10` pops packets, subtracts 3 from the code, looks the element up
by key and calls its virtual slot 8 with the code, the data and the size. Slot
8 is `0x0058E0C0`. Its case for code `0x4A` (`0x0058E373`) clears bit 1 of
`+0x26D`. The dispatcher indexes its table with the code minus `0x4A`. A queued
code of 2 takes another branch of the drain.

Exactly three calls of `0x0065BEF0` exist in the code section: the timer expiry
`0x007A4C20`, the replacement of a pending turn `0x007B0547` and the shortcut of
`0x007B0690` at `0x007B06F6`. No call posts the event when the angle is
reached, and none of the three sites tests the rotation. A turn wait therefore
ends when the timer runs out, when a new turn replaces the pending one, or at
once for a turn back with no saved heading or with the heading inside 5
degrees. The byte pattern for setting bit 1 of `+0x26D` occurs at `0x00585EC0`
and `0x00587DF2` only, and the clearing pattern at `0x0058E373` only. The
element constructor also resets the byte with a different instruction.

The unit of the frame delta, the origin of the dword at controller `+0x258` and
the motion that turns the actor after `0x007A4CD0` were not traced. The timer
is a bound on the wait and not a model of the rotation.

## Visibility

`_setVisible` posts the commands `0x10` and `0x11`, which carry no record
offset. The command table entry `0x007C9696` calls `0x007CCCC0` on the actor
with 1 and `0x007C96B4` with 0. `0x007CCCC0` asks the actor's visibility
virtual (offset `0xAC`) for the old answer. It then sets bit 1 of the actor byte
at `+0xE8` when the argument is 0 and clears it when the argument is 1, and asks
again. CharaActor slot 43, `0x0061FF40`, answers true when the byte is 0.

When the answer changed, `0x007CCCC0` calls `0x00A5FA10`, which forwards the new
value to virtual offset `0x6C` of the object at actor `+0x0C`, and then calls
virtual offset `0x250` of the actor. For CharaActor that slot is `0x00712B40`,
which returns at once. The object at `+0x0C` was not identified. What other
code reads the byte at `+0xE8` was not traced.

## Floating offset

`_setFloatingOffset` sends record `0x26`, command `0x3B`, with the 16 bytes at
element `+0xB0`. The CharaActor case `0x00663074` calls `0x0065CC60` with the
payload. The routine reads the float at payload `+4`, which is the value of
element `+0xB4`, and calls `0x007A42B0` on the object at actor `+0x590` with
that float and a byte. The byte is `0x80`, or the result of `0x00850EA0` when
the dword at actor `+0x2B10` is nonzero. `0x007A42B0` stores the byte at `+0x5B4`
of the object and the float at `+0x588` as the target. When bit 9 of the dword
at `+0x5B4` is set it clears the bit and stores the float at `+0x58C` as the
current value as well.

The per-frame step of that object, `0x007A7280`, runs in state 0 and for a
positive delta. It adds to the current value the smaller of 0.02 and the
difference between target and current times 0.02 times the delta. The delta
unit was not traced. `0x007A42F0` returns the current value.

The ground placement routine `0x00660F70` takes a position and probes the
ground through virtual offset `0x29C`. When the probe reports a hit and the
gates at `0x00661029` to `0x0066103F` pass, it adds the current value to the
probed height and stores the sum as the second component of the position
(`0x00661053` to `0x00661070`). The second component is the one the routine
takes from the old actor position when the probe reports no hit. It then sets
the actor position through `0x007CE080`. That the value is a height is
INFERRED from this addition and from the native name.

## Map marker

The marker number is the low 6 bits of the dword at element `+0x270`. The
setter `0x00586720` replaces those bits and the element constructor leaves them
at `0x3F`. The getter `0x00585300` has two callers. `0x00680810` calls it at
`0x006808BC` and `0x006A2E90` at `0x006A2EA9`.

`0x00680810` handles one map entity per call and takes the marker branch only
when the entity flag byte at `+0x3D` has bit 1 set. For the chara stored at
`+0x9E0` of the map context it uses 0. The routine treats that chara
separately, and it is INFERRED to be the local player. For any other chara it
uses the marker number when bit 2 of the chara byte `+0x26D` is set, and -1
otherwise. A number of 0 or above `0x40` becomes -1 for those charas. It stores
the low byte at entity `+0x3C`. A value of `0x3F` clears bits 0 and 2 of the
entity flags and ends the routine. Any other value goes through `0x00680730`
and updates the entity. Bit 2 of `+0x26D` is the bit that element event `0x55`
sets. Bits 6 to 10 of the same dword are read by `0x00585310` and only
`0x00680810` calls it, at `0x006809E9`. The routine tests that value for
2 to 5 together with bit 1 of the chara byte `+0x274`. The icon table behind
`0x00680730` was not traced.

The two marker strings are not read by displacement. `0x00588490` copies up to
two strings of 0x54 bytes each from a vector into the 0x20 byte buffers at
`+0x34` of two records of 0x54 bytes from element `+0x3B0`, so the buffers are
at `+0x3E4` and `+0x438`. `0x005881C0` clears bits 0, 2 and 3 of both record
flag dwords and the first byte of the buffers at `+0x3B4`, `+0x3E4`, `+0x408`
and `+0x438`. The per-frame step `0x0058B1B0` reads the record flags and tests a
distance against fields at `+0x24` and `+0x28` of each record. No instruction
reads `+0x3E4` or `+0x438` directly, so the readers behind the record base were
not found.

The pending marker is applied by NpcBase virtual slot 12. The vtable cells at
offset `0x30` hold `0x006E18B0` for CharaBase, PlayerBase and MyPlayer and
`0x006F8660` for NpcBase (`0x00FD5CDC`, `0x00FD5E34`, `0x00FD788C` and
`0x00FD64AC`). The NpcBase routine first calls the base routine, which returns
the byte at `+0x5C`, and stops when that byte is 0. Otherwise it runs a list
build of its own that sets the byte at `+0x15A` under a condition, which was not
traced. At `0x006F8830` it calls `0x0076D920` with the receiver identity, the
pending number at the object's `+0` and its vector at `+4`. Then it releases
the pending object at `+0x128` through `0x007218E0`. `0x0076D920` sets the
number through `0x00586720` and copies the strings through `0x00588490`. The
caller `0x006F0020` of the contract stores the pending object only while the
byte at `+0x5C` is 0, so a marker set before the byte becomes nonzero is
applied when slot 12 next runs.

## Target fields

`_getLookAtCharacter` reads the dword at element `+0x12C` last. The
constructor stores `0xC0000000` there (`0x0058B56F`). The element receive case
for opcode `0x00D3` stores the first payload dword there (`0x0058D047`), after
`0x00587E70` has written the same target into look-at slot 1, as the
[look-at page](chara-look-at-consumers.md#server-feed) describes. The opcode
catalog names `0x00D3` SetActorTargetAnimatedPacket.

The NpcBase dword at `+0x154` is set to 0 by the NpcBase constructor
`0x006F3650` through the handle initializer `0x00CC9320` (`0x006F373F` to
`0x006F3769`). The NpcBase getter in slot 40, `0x0072D000`, returns it. No
function in the NpcBase vtable stores to `+0x154` by displacement. A writer
outside the vtable functions, or one that takes the field address, was not
searched for.

## Open questions

- CALIBRATION: the unit of the frame delta behind the 10.0 turn timer
- UNPROVED: the origin of the low 6 bits of the controller dword at +0x258 and the rotation that follows 0x007A4CD0
- UNPROVED: the object at actor +0x0C and any other reader of the visibility byte at actor +0xE8
- UNPROVED: the icon table behind 0x00680730 and the display of marker numbers 1 to 0x40
- NOT FOUND: a reader of the marker strings at chara +0x3E4 and +0x438
- NOT FOUND: a store to NpcBase +0x154 by any function of the NpcBase vtable

## Consumer contract

What a consumer of these fields can implement from this page, with the tier of
the underlying claims above:

- Raise bit 1 of the turning byte for _turnBack and for a heading change and clear it on event 0x4A
- Raise event 0x4A for the turn back shortcut when no heading is saved or the heading is within 0x38E39 units of the saved one, and for a pending turn that a new turn replaces
- Classify a turn from the 24 bit target minus current angle difference, then lift the target by one circle for a negative class when the current angle is above it and lower it by one circle for a positive class when the current angle is below it
- Raise event 0x4A when the turn timer runs out, with a timer of the larger of 10.0 and the 6 bit value of the controller, lowered by delta times the controller scale (AUTHORED unit and scale)
- Hold the hidden bit as bit 1 of a visibility byte that _setVisible sets for false and clears for true, with the chara visible when the byte is 0
- Ease the floating offset as the current value plus the smaller of 0.02 and the difference times 0.02 times the delta, and add it to the ground height of the chara
- Use the marker number only while bit 2 of +0x26D (set by element event 0x55, meaning not traced) is set, map 0 and numbers above 0x40 to -1 for a chara other than the local player, clear the map entity only for 0x3F, and apply a pending marker when the NpcBase byte at +0x5C becomes nonzero (what -1 shows is CALIBRATION)

What stays blocked, and why:

- The unit of the frame delta behind the turn timer, the origin of the controller dword at +0x258 and the default controller scale at +0xF8
- The marker icon table behind 0x00680730 and any reader of the marker strings at chara +0x3E4 and +0x438
- The writers of NpcBase +0x154 and the role of the ready byte at +0x5C
- What the object at actor +0x0C does with the visibility argument

## Reproduction

```sh
python3 tools/verify_chara_presentation_consumers.py
python3 tools/verify_chara_presentation_consumers.py --exe orig/ffxivgame.exe
python3 tools/test_chara_presentation_consumers.py
```

The first command checks the manifest, both pages and the mutations. The second
adds the instruction, table and string checks and recomputes the scans against
the pinned executable.
