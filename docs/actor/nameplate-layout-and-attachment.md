# Name plate layout and attachment

This note records what the retail 1.23b `NamePlate` class and its callers fix
about where a plate attaches, how the user offset enters, and which tests the
class holds. It answers four layout questions: text size, vertical anchor,
range and count, and overlap handling. Labels: STATIC means read from
instructions or data at the cited addresses. MEASURED means decoded from
identified installed carriers. INFERRED means a reading of static observations.
UNKNOWN marks unresolved questions.

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
| Attachment height in yalms | Two installed c001 providers give different local coordinates. The selected live position depends on the provider, current pose and skeleton placement | MEASURED (local coordinates), UNKNOWN (live height) |
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
which resolves the selector against the actor's attachment helper. That helper
can select an eligible auxiliary actor, resolve the model path or take the
fallback described below. STATIC for the path.

Damage plates substitute the same `EID_NAME` default for an empty string
([damage-plate-numeric-presentation.md](../resource/damage-plate-numeric-presentation.md), kind 1 anchor).

## EID_NAME model record and world position

STATIC. The nameplate's ordinary CharaActor path can resolve a named
ElementId record. The admitted model lookup reaches Drawable
`+0x334`, its CharacterSkeletonNode. Drawable name lookup
`0x008DD880` tries the node's ElementId provider at `+0x0C` first
(`0x008DD8B7 -> 0x00A3EC50`). A hit returns the provider index
tagged with `0x01000000` at `0x008DD8F6`. On a provider miss it
tries the ordinary skeleton name lookup at `0x008DD8CE`, then the
provider key `CDevDefault` at `0x008DD8EC` when that provider exists.
The CharaActor helper tries its eligible auxiliary actors first
(`0x00854FB2 -> 0x0065BD70 -> 0x00846E00`), so this model path
requires the auxiliary search to miss.

The factory registration at `0x00A3C732..0x00A3C75D` associates
RES type `0x00656C62` with ElementIdResourceNodeFactory table
`0x01099F8C`. The native ElementIdResourceNode initialization at
`0x00A4686B..0x00A468A3` passes its backing payload to
ElementIdBinaryParser `0x00A46E30` and retains the parser at
node `+0x50`. For the selected `SEDBelb` version-3, endian-byte-zero shape,
the count is a little-endian u16 at member `+0x30`. The fixed array
starts at member `+0x40` with stride `0x20`. Record `+0x00`
and `+0x04` are member-relative offsets to the NUL-terminated
joint and attachment names respectively (`0x00A46D90`,
`0x00A46DD0`). Slot 0 `0x00A46CF0` scans attachment names in
ordinal order for exact equality. Record `+0x08..+0x13` is the
local XYZ float32 triple (`0x00A46F90`). The separate rotation
triples start immediately after the fixed array and have stride 12
(`0x00A46F5E..0x00A46F6A`, `0x00A47000`). These reads apply no
unit conversion to the translation.

### Installed c001 example

MEASURED. The selected files are under `client/chara/`. Validate
PWIB framing and select the physical RES directory rows below, then
read their type dwords from the containing view's RESOURCE_TYPE
member using the [PWIB directory framing](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/gtex-pwib.md#directory-and-type-selection).
Keep each nested view within its parent extent and check the fixed and
rotation arrays and both name scans against the typed member's extent.
Offsets are absolute in the named carrier. The two members have the
same typed layout with one exact `EID_NAME` row each.

| Carrier | Bytes | Carrier SHA-256 | Physical path | Member | Type cell | Payload range |
| --- | ---: | --- | --- | --- | --- | --- |
| `pc/c001/skl/0001` | 54672 | `d90a756fc1cba6f03bb8dc102b367feb905b5e57a00d209ff4fccddaa17cd031` | `[3,1,0]` | `F00\elementId` | `0xD2C0` | `[0x5B10,0x7D56)` |
| `pc/cmn/0001` | 113264 | `3f71c608e46d616e78883acd262190366085ef2bc587b8ae48cc4538f573c929` | `[3]` | `template\elementid\template_pc` | `0x19C48` | `[0xD20,0x2AE7)` |

The c001 model member has 139 records. Row 42 at file `0x6090`
selects joint `n_root` and translation
`(0, 2.119999885559082, 0)` in local coordinate units, with zero
rotation components at `0x6EA8`. The common template has 121
records. Row 42 at file `0x12A0` selects `n_root` and translation
`(0, 1.850000023841858, 0)`, with zero rotation components at
`0x1E78`. The c001 model's paired SKL member is physical path
`[3,0]`. Its joint 0 is `n_root`, with parent `-1` and identity
constructor-pose inputs. This pins the authored starting pose only.

STATIC. The [appearance slots](appearance-resource-slots.md#bank-numbers)
request `cmn/0001` as bank 0 and model `skl/0001` as bank 1.
The [ordinary root pack](body-resource-inputs.md#packing-count-and-exact-model-call)
passes retained roots in bank order. The ResourceFinder walks that
array forward at `0x00C52532..0x00C52543` and appends accepted
ElementId nodes to its `+0x64` vector at `0x00C52688..0x00C52697`,
through metadata test `0x00C515F0` and append `0x00858140`.
The ordinary merge loader visits this vector backwards
(`0x00C50920..0x00C509DC`) and places each parser from
`0x00A468F0` in the first free compositor slot. The search at
`0x00C51C20` walks the contiguous provider slots forward and returns
the first nonnegative match, adding the row counts of preceding
providers. The compositor has 16 provider slots. Thus when both named providers are admitted into the
compositor with valid backing, the c001 model's row takes precedence
over the common row. An auxiliary actor or a later-collected provider
can supply an earlier match. Actual retained roots and the live winner
remain unresolved.

### From joint space to the nameplate input

STATIC. CharacterSkeletonNode construction retains the compositor
at `+0x0C` and stores the signed-short remap allocation at `+0x14`
(`0x00A3EAD0`, store and null check at `0x00A3EB34..0x00A3EB39`).
When allocation succeeds, it looks up each provider row's joint name
in the constructed skeleton at `0x00A3EB4A..0x00A3EB5F`, storing
joint zero on a name miss. The tagged matrix path at
`0x006693C9..0x006693DE` clears the tag. `0x00669340` checks the
node, provider and index against the provider count before calling
`0x00A3EC70`. The remap read at `0x00A3EC83..0x00A3EC86` requires
the populated allocation. With an initialized skeleton and populated
remap, this path performs three steps.

1. The remap selects the current joint output at
   `[skeleton+0x44] + 0xA0 * joint`. Call `0x00A3EC9F` composes
   that output with the skeleton matrix at its first four rows.
   The checked Drawable update paths `0x00C39B20` and `0x00C39B70`
   supply these rows from Drawable `+0x30` through `0x007DD5B0` at
   `0x00C39B46` and `0x00C39B96`.
2. Provider slots `+0x10` and `+0x14` at `0x00A3ECB2` and
   `0x00A3ECF7` supply the local translation and rotation. The
   rotation builder at `0x00A3ED38` and fourth-row replacement at
   `0x00A3ED6B` build the local matrix. Call `0x00A3ED70`
   composes it with the first result.
3. Scene actor method `0x007CCFF0` returns the resulting fourth
   row at `0x007CD03B..0x007CD043`. This is the raw primary target
   supplied to the [WindowActor producer](window-actor-anchor-producer.md#target-production).

INFERRED from the row-vector multiplication at `0x0061D050`.
The matrix product is `M_attachment * M_joint * M_skeleton`.
The raw target is its translation row. For an admitted c001 model
row with its authored identity root pose, a populated remap
and an initialized skeleton matrix, the corresponding point is
`(0, 2.119999885559082, 0, 1) * M_skeleton`. Live animation,
actor placement and scale can affect it through the matrix inputs.
A failed tagged matrix check returns the Drawable matrix through
`0x00669357`. A negative attachment lookup instead takes actor
slot `+0x88` at `0x007CD018`. The optional second-actor target,
retention filters and configured offset follow the raw primary target.
These observations establish the selected local coordinates and
native transform path. They leave the live selected model, pose,
filter and projected plate position unresolved.

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
- The selected local coordinates fix two installed model records. Live height,
  the effective unit of `name_plate_offset` in screen space, retail visible
  range, a count cap and an overlap rule remain unresolved.
- A server cannot supply any of these values. They are client constants and
  model data.

## Reproduction

Open `ffxivgame.exe` and verify the size and hash above. Disassemble from
`0x005854E0`, `0x006A2930` and `0x006A3560` and follow the cited calls. Read the
byte tables `[0x00585640, 0x00585658)` as six little-endian dwords and
`[0x00585658, 0x005856B4)` as bytes, and read the strings at the listed VAs.
Read the double at `0x00F55AD0` and the float at `0x00F54F70`. Do not publish
executable bytes, decoded forms or instruction listings.
