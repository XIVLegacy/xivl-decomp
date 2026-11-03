# Name plate layout and attachment

This note records what the retail 1.23b `NamePlate` class and its callers fix
about where a plate attaches, how the user offset enters, and which tests the
class holds. It answers four layout questions: text size, vertical anchor,
range and count, and overlap handling. Labels: STATIC means read from
instructions or data at the cited addresses, INFERRED means a reading of static
observations, UNKNOWN means not established.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe`, 1.23b `2012.09.19.0001`, 15996808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000` |
| Tools | Ghidra 12.1.4 disassembly, decompilation, string and cross-reference queries over the analyzed program, and raw byte reads of the cited data |
| Addresses | Absolute VAs at the image base |

Decompiler renderings are interpretations. Every claim below states the
instruction or data it rests on. The instruction ranges are `[0x006A28F0,
0x006A43F4)` for the `NamePlate` methods, `[0x005854E0, 0x0058563E)` for the
attachment selector and `[0x007F90BE, 0x007F91D2)` for the offset consumer.

The class is identified by RTTI. The vtable `0x00FCF98C` has 24 slots and the
dword before it, at `0x00FCF988`, points to the complete object locator
`0x0115B780`. Its type descriptor at `0x012BD7C8` names
`.?AVNamePlate@Char@Element@Main`. STATIC.

## Summary

| Question | Result | Tier |
|---|---|---|
| Text size | The form declares FontSize 24 on the plate label `TextBlock_PrimaryLabel`, in the [form attribute record](https://github.com/XIVLegacy/xivl-client-data/blob/758cf9ebf4edf6bbbf622cfe799b51c7eb99eabd/docs/sqwt-nameplate-primary-label.md). That page bounds its values as declared metadata with no runtime meaning, and the same element carries a `SqwtStyle` of `TBL_namePlate` that was not resolved and could override the size. No `NamePlate` method scales the label | Declared metadata (size), STATIC (no method scales it) |
| Vertical anchor | The anchor is the model attachment point `EID_NAME`, or one of five state variants, selected by the state value | STATIC |
| User offset entry | The value `name_plate_offset / 100.0` enters the `+0x158` lane of a record that the producer combines with camera-derived vectors. Its units, direction and sign are not established, and it is not shown to be a plain addition to the attachment height | STATIC (value and entry), INFERRED (lane role), UNKNOWN (units, direction, sign) |
| Default offset | `name_plate_offset` defaults to 0 (`xivl-client-structs:docs/user-config-store-contract.md`, group 6 key 3) | STATIC |
| Attachment height in yalms | Not recoverable from the executable. The point lives in model data | UNKNOWN |
| Range, count cap | No distance, depth or count test exists in the `NamePlate` methods | STATIC |
| Overlap handling | No overlap, collision or stacking test exists in the `NamePlate` methods | STATIC |
| Range, cap and overlap elsewhere | Not traced in the helpers the methods call | UNKNOWN |
| Depth scaling of the glyph quads | The vertex shader was not read | UNKNOWN |

## Attachment selector

The `CharaElement` state setter `0x00588BA0` stores the incoming state value at
`CharaElement+0xF4`. The first time `+0xF0` is `-1` it also stores the value
there and calls `0x005854E0`, and `0x0058E395` calls the same function from a
second site. `0x005854E0` bounds its argument by `cmp eax, 0x5B` at `0x00585508`,
reads a byte class from the 92-byte table at `0x00585658`, and jumps through the
six-entry table at `0x00585640`. Each live arm loads one string, builds a
selector record on the stack and calls `NamePlate` vtable slot 9 through
`[CharaElement+0xBA0]` and `[vtable+0x24]` at `0x00585608`.

| Class | Arm | String and VA | State values |
|---:|---|---|---|
| 0 | `0x00585546` | `EID_NAME` (`0x00FA71BC`) | 0, 2, 14, 50, 51, 70, 81 |
| 1 | `0x0058551F` | `EID_NAME_GRD` (`0x00FA71AC`) | 1, 3, 12, 13 |
| 2 | `0x0058556D` | `EID_NAME_SIT` (`0x00FA71C8`) | 11 |
| 3 | `0x005855B5` | `EID_NAME_GB` (`0x00FA71E8`) when byte `CharaElement+0x269` is nonzero, otherwise `EID_NAME_CHO` (`0x00FA71F4`) | 15, 91 |
| 4 | `0x00585591` | `EID_NAME_CFT` (`0x00FA71D8`) | 30, 31, 32 |
| 5 | `0x0058560D` | no string and no slot 9 call, the selector is left unchanged | every other value up to `0x5B`, and every value above it |

Every arm continues into the tail at `0x0058560D`, which class 5 enters
directly. The tail calls `0x004D7310` on `[CharaElement+0x7C]` and, when that
returns an object, calls `0x004D6750` and then `0x00561DC0`. That is not a name
plate call and its role was not traced. UNKNOWN.

The table and the values come from reading the 92 bytes at `0x00585658` and the
24 bytes at `0x00585640`. STATIC. The suffixes are labels in the data and their
meanings are INFERRED: `GRD` ground, `SIT` sitting, `CFT` crafting and `CHO`
chocobo. The `GB` meaning is UNKNOWN. The mapping from a state value to its
gameplay name is not established here, and state value 91 has no local support
for either of its two arms.

`NamePlate` slot 9 at `0x006A2930` copies the string into `NamePlate+0x13C`
and forwards it through `0x00548450` to `DrawingToolContext` slot 47
(`0x0055A5A0`). That slot copies at most 15 bytes into record `+0x388`, sets
record byte `+0x5A` bit `0x08`, and the cached-record publisher `0x0054E890`
submits it as operation `0x1E` with length `0x10`. The `WindowActor` receiver
of that operation is owned by
[the anchor producer](window-actor-anchor-producer.md#construction-and-lifecycle),
which resolves the selector against the actor's attachment helper, so the
anchor is the model's own node. STATIC for the path. The point's position is
not in the executable.

Damage plates substitute the same `EID_NAME` default for an empty string
([damage-plate-numeric-presentation.md](../resource/damage-plate-numeric-presentation.md), kind 1 anchor).

## Offset setting

`NamePlate` slot 13 at `0x006A3560` ends with a cache test. At `0x006A39F0` it
calls `0x004B5230`, which pushes `(0, 3, 6)` into `0x00443E40`. That is user
config group 6 key 3, `name_plate_offset`, range -30 to 30, default 0
(`xivl-client-structs:docs/user-config-store-contract.md`). When the signed
byte differs from the cache at `NamePlate+0xF1` the slot stores the new value
and builds a four-float vector at `0x006A3A1F..0x006A3A4B`. Its x and z lanes are
zero, its y lane is the signed byte divided by 100.0 and narrowed to a float
(`cvtsd2ss` at `0x006A3A40`), and its w lane is 1.0f loaded at `0x006A3A38`.

The divisor is the double at `0x00F55AD0`, which reads `0x4059000000000000`,
that is 100.0. The `w` lane is the float at `0x00F54F70`, 1.0f. The vector goes
through `0x005484D0` to `DrawingToolContext` slot 48 (`0x0055A5F0`), which
copies 16 bytes into record `+0x398` and sets byte `+0x5A` bit `0x10`. The
publisher `0x0054E890` submits the 16 bytes at record `+0x398` as operation
`0x1F`, through the helper `0x0054B0D0`. The `WindowActor` receiver slot 157 (`0x007FA2B0`)
stores operation `0x1F` into `WindowActor+0x154..+0x163`.

In the producer `0x007F8D20`, the instructions at `0x007F90BE..0x007F90DC`
multiply the `+0x158` lane by one normalized four-lane vector, multiply the
`+0x154` lane by another, subtract the first product from the second and store
the result at `+0x164`. `0x007F9136..0x007F917A` then adds `+0x164` to the
filtered anchor at `+0x190`, as the anchor producer page records. Thus the value
`setting / 100.0` enters the `+0x158` lane before the camera-derived vectors are
applied. STATIC for that entry only. The lane is multiplied, not added, so the
setting is not shown to be a plain vertical addition. Both multiplied vectors are derived inside
the producer from per-frame state. Their directions and the sign of the vertical
displacement are not established here. INFERRED for the lane roles, UNKNOWN for
the directions. The setting spans -0.30 to +0.30 in that lane's units, and the
anchor producer page's capture saw `+0x164` equal to zero, which matches the
default 0.

## What the NamePlate class does not test

The decoded range `[0x006A28F0, 0x006A43F4)` holds the 24 methods of the
vtable `0x00FCF98C` (slot 11 is `0x006A28F0`, the lowest) and the functions
around them, among them `0x006A2D50`,
`0x006A2E90`, `0x006A3380`, `0x006A3430`, `0x006A34B0`, `0x006A3C00` and
`0x006A4270`. In it:

- The floating-point arithmetic on geometry is one `divsd` at `0x006A3A23`, the
  offset division above. There is no `sqrtss`, `rsqrtss`, `mulss`, `addss`,
  `subss` or `divss`. No instruction forms a distance, a depth, an overlap or a
  screen extent. Three integer-to-float conversions remain. Two `cvtsi2ss` at
  `0x006A36A8` and `0x006A36C1` convert integers into the float argument of the
  setters `0x00967500` and `0x00966C00`, called on the object at `[ESI+0xC8]`,
  and a `cvttss2si` at `0x006A2E74` reads the float at `[owner+0x1AC]` back as
  an integer. They look like size or position setters of the label control and
  were not traced, so a layout role is UNKNOWN.
- The only float compares are the four `ucomisd` instructions at `0x006A393E`,
  `0x006A3968`, `0x006A3980` and `0x006A3998` between the four colour lanes
  at `NamePlate+0x78..+0x84` and their cache at `+0x98..+0xA4`.
- The float constants loaded from data are 36.0, 64.0 and 96.0 at `0x00FCF500`,
  `0x00FCF504` and `0x00FCF508` for the control-size slots `0x006A2C50` and
  `0x006A2E10`, the colour defaults 1.0f and 0.5f at `0x00F54F70` and
  `0x00F62F60` stored by `0x006A4150..0x006A4178`, the 100.0 above, and the
  1.0f at `0x00F54F70` loaded again at `0x006A3A38` (the vector w lane) and
  `0x006A3C3F` (the constructor `0x006A3C00`).
- The visibility selector `0x006A2E90` reads the per-kind switches through
  `0x004B5100..0x004B5360`. Those are config reads, not geometry. The kinds are
  the player, party, other, npc and enemy name and HP settings of the
  [user config store](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/user-config-store-contract.md).
- Slot 13 compares the owner `CharaElement` pointer with a lookup at
  `0x006A3823..0x006A3830` and compares the global at `0x012BD77C` with a
  value of the owner at `0x006A35CF..0x006A35E0`. These are identity tests of
  one actor. No method walks a list of plates or actors.
- Each plate is inline in its own `CharaElement` at `+0xBA0`, so the class has
  no pooled plate array that could carry a count cap.

The claim is limited to this range. A distance cull, a count limit or a
declutter pass could sit in the scene visibility of the actor, in the
`DrawingToolContext` consumer or in the draw pass, none of which was traced.
Those stay UNKNOWN.

## Glyph submission

The accepted label consumer `0x00CA55A0` calls the quad builder `0x00CB9280`.
The decompiled body copies one three-float point, its first argument, into all
six vertices, derives the corner offsets of a rectangle from a second four-float
rect widened by the last float argument on the left and right, and derives
texture coordinates from a third rect divided by the texture extent. INFERRED
from the decompiler rendering: a glyph is a screen-sized rectangle around a
world point, which fits the anchor global `0x0130BBC4` of the anchor producer
page. Whether the vertex shader scales the rectangle with depth is UNKNOWN.

## Boundaries

- The form attribute values are declared metadata of the extraction record and
  are not re-derived here.
- Nothing here fixes the height of `EID_NAME` for any model, the effective unit
  of `name_plate_offset` in screen space, the retail visible range, a count cap
  or an overlap rule.
- A server cannot supply any of these values. They are client constants and
  model data.

## Reproduction

Open `ffxivgame.exe` and verify the size and hash above. Disassemble from
`0x005854E0`, `0x006A2930` and `0x006A3560` and follow the cited calls. Read the
byte tables `[0x00585640, 0x00585658)` as six little-endian dwords and
`[0x00585658, 0x005856B4)` as bytes, and read the strings at the listed VAs.
Read the double at `0x00F55AD0` and the float at `0x00F54F70`. Do not publish
executable bytes, decoded forms or instruction listings.
