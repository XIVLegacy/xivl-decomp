# Zone lighting source and time of day

Where the retail client gets directional, ambient and point lights, fog and
environment cubes for a draw, and how Eorzea time selects among authored keys.
Static read of `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`, all locators VA) plus a structure-only read of the installed
region, weather and zone layout DATs. No retail bytes recorded, counts and
shapes only. Tiers: V = verified from cited instructions or file bytes,
I = inferred, U = unknown.

This page owns the clock, the runtime light node, the draw-time light list and
the zone layout lights. Related owners:

- [Weather transition runtime](../net/weather-transition-runtime.md) for the
  packet path, the two-entry weather queue and the weather fraction.
- [Weather layout resource shape](weather-drawenv-resource-shape.md) for the
  DAT inventory and object names.
- [Weather, sky and fog environment path](weather-sky-fog-environment-path.md)
  for lane selection and the fog state.
- [Weather light curve container shape](weather-light-curve-container-shape.md)
  for timelines, curve containers and the light clip.

## Chain in one paragraph (V unless marked)

Weather lights are not Lua or per-zone constants. They are layout objects
(`BaseObjects/Light/LightBaseObject`, grouped by `LightSetObject`) in the
weather layouts, one layout per weather resource. The client blends the queued
weather layouts by the packet-driven weather fraction, and drives every
timeline in each layout with one scalar, the Eorzea time-of-day phase. The
draw then reads the resulting light nodes through a LightEnv light list. Zone
layouts also carry their own lights, see
[Zone layout lights](#zone-layout-lights).

## Resource path

- Region table `0x03C00000` child rows of type `0x0` carry weather ids (8001..,
  see [Region weather resource rows](region-weather-resource-rows.md)) and the
  type `0x1` row carries the sky layout (`sky_celestial`, `0x29B0000E` for
  `fst_f0`). `0x0079B540` maps a resource key back to the child id: it tries
  the current region root's children first, else scans every root. The id to
  key lookup is `0x0079B050`, see the
  [runtime page](../net/weather-transition-runtime.md#region-lookup-and-resource-key).
- Each weather DAT is a `MapLayoutResourceData` layout (header `DEF_BLK`,
  count at `+0x24`, 32-byte dependency records from `+0x60`: 16-byte name,
  4-byte reversed extension, u32 key, u32 flags, pad).
- Six checked `fst_f0` weather layouts (8001, 8002, 8003, 8004, 8007, 8010) all
  hold the inventory of
  [Weather layout resource shape](weather-drawenv-resource-shape.md#class-inventory-v):
  10 lighting DrawEnv objects, 11 `FogObject`s, 29 light objects (the sky lane
  has main and sub only), and a per-lane timeline. Rain and cloud variants add
  `CubeMapBaseObject`, `EnvironmentMapDrawEnvObject` and wind objects.
- Lane naming (I): first digit 0 outdoor, 1 dungeon, 2 indoor, matching the
  dependency textures `f0f_*` (5), `f0d_11/12` (2), `f0i_21/22` (2). The client
  classifies names by the `dwev_0`, `dwev_1`, `dwev_2` substrings (`0x007E7480`,
  literals `0x00FEF4C0..`). The DrawEnv object selected at the followed
  position (name via vtable `+0xA4`, caller `0x007DCE65`) names the lane, see
  [lane selection](weather-sky-fog-environment-path.md#lane-selection-by-spatial-drawenv-name)
  and the
  [followed position](weather-sky-fog-environment-path.md#followed-position-and-direction).
- Zone layouts (fld, twn, dun, ind, air, low) also register DrawEnv, Fog,
  Light, LightSet and ScreenEnv types. `ind01` and the weather layouts also
  register `CubeMapBaseObject` and `EnvironmentMapDrawEnvObject`. Instance
  counts are in the resource page.

## Time of day

- The server-synced timestamp object (`+0x308` seconds, `+0x310` ms) is scaled by
  the double `20.571428` at `0x00FAADC0` (3600/175) in `0x0059EBA0`, split to
  whole seconds plus a ms remainder, and published as world message 100.
  Handler `0x007E1600` stores it at `WeatherManager+0x108` and `+0xF8`.
  Alternate clock: message 102 to `0x007E1820` writes `+0x120`, mode message 98
  (`0x007E0410`) writes `+0xF4`, nonzero selects `+0x120`. Meaning of that
  mode is U. Scene switch is `0x0062CDA0` (ids = case table `0x0062D0E0`).
  Reading the clock: `0x007E13F0` takes the 64-bit seconds at `+0x108`, or at
  `+0x120` when `+0xF4` is nonzero.
- Phase `0x007E13F0` = (seconds mod `86400`, dword at `0x00FE5494` that reads
  86400 in the file) / 86400 as float, whole seconds only. Day = 86400 Eorzea
  seconds, 175 real seconds per Eorzea hour.
- Per frame `0x0083B500` and `0x007E1820` feed phase to `0x0083AFF0`, which
  for every timeline in the layout sets the playback position to
  (int)(timeline length * phase) through the timeline vtable `+0x4C` (length)
  and `+0x98` (set position). So each timeline spans exactly one Eorzea day
  and phase 0.0 is Eorzea 00:00 (I: from the 86400 modulus). Keys are FCurves
  sampled in frame units (clip local ticks over the frame length global,
  10010 by default), no table of discrete hourly keys. The weather lane
  timelines are 2,400,000 ticks long (V, see
  [Weather light curve container shape](weather-light-curve-container-shape.md#scb-clip-table-and-fcr-pairing-v)).
- Local interpolation between net updates: `0x007E0BC0` adds multiplier
  (`+0x134`, message 104 handler `0x007E03D0`) * frame delta * 6.0 when
  `+0x118` (message 108) is set.

## Light record

The [model construction path](model-lightset-source.md#native-lightnode-property-source)
separately qualifies native source-field copies into Renderer::LightNode,
including the +0xE4 minimum clamp. That source-object contract does not
establish its serialized backing or a Weather input.

Runtime light node (Weather::LightNode over Renderer::LightNode, ctor
`0x008CAEB0`, base ctor `0x00C2F690`). Offsets V, names I:

| Offset | Content |
|---|---|
| `+0xC0 +0xC4 +0xC8` | RGB (luma weights 0.299, 0.587, 0.114 at `0x010D5760/58/68`) |
| `+0xCC` | alpha, written 1.0 by the clip |
| `+0xD0` | intensity |
| `+0xDC` | type: 1 directional, 2 spot (unsupported in pixel driver), 3 point, 4 ambient |
| `+0xE0 +0xE4 +0xE8 +0xEC +0xF0` | point extras, `+0xE0 +0xE4` fed to PointLightParams. The clip sources are in the [curve page](weather-light-curve-container-shape.md#light-clip-colour-and-intensity-v) |
| `+0xF4` | priority, `+0xF8` enable byte, `+0xF9` category (callback `0x01374C54`) |

Type names NONE, DIRECTIONAL, SPOT, POINT, AMBIENT are laid out in that order
at `0x010A4B0C..` (I for the numbering). Layout-side properties (`0x00AAE600`
debug dump): LightType (byte), Color (3 floats), Intensity, DecayRate,
ConeAngle, PenumbraAngle.

Layout type to runtime type (V): the light instance property apply switches on
the layout LightType getter at `0x00A9FDFD` (jump table `0x00AA0170`) and
writes `+0xDC` as 0 -> 1 directional, 1 -> 3 point, 2 -> 2 spot, 3 -> 4
ambient. Every `main_lght_*` and `sub_lght_*` node of the weather DATs under
`data/29/B0/00` has layout type 0 (161 and 161 nodes) and every `amb_lght_*`
node has 3 (144 nodes). The byte is read at lyb node `+0x44` (V in the file).
That it is the LightType property is I: it reads 0 on the directional pair, 3
on the ambient and 1 on the named torch and lamp lights of the zone layouts.

Animation (V): LightClip plugins (`LightClipCommon.cpp`, enter `0x00A0D0B0`,
update `0x00A0CD00`) write the node from the Color and Extra groups, see the
[curve page](weather-light-curve-container-shape.md). Direction of a
directional light (`0x00C20B80` over `0x00C2F8F0`) is the vector (0, 0, -1, 0)
multiplied row-vector style by the node world matrix and normalized. The
second argument of `0x00C2F8F0` is a flag, not an axis index. The world matrix
is the product of the node local matrix and a matrix reached through the
node's `+0xD4` object (I). Position is from `0x00C2F450`. The weather lights
keep the transform authored on their layout instance record (the clip
transform write is disabled, see the curve page), and the outdoor lane
timelines also move and turn the main and sub lights through SunMoon clips,
see [Lane light orbits](#lane-light-orbits). In `wtr_fine` lane 00 those
records carry X-only euler rotations of opposite sign on `main_lght_00` and
`sub_lght_00` and none on `amb_lght_00` (shape only, the sky lights carry
multi-axis rotations). The euler composition order of the layout instance
transform for rotations on more than one axis is U for lights.

## Draw-time light list and rules

`LightEnv::GetLightList` `0x00C31E30` (magic `0x4C544556` at `+4`, slots 0x40
bytes) walks LightSets (`+0x30` vector), tests enable bytes, ranks directionals
by luma*intensity and everything by priority then magnitude, skips below
`1e-5` (`0x00FEE2B4`). Ambient lights bypass ranking and are accumulated.
A global fixed light array at `0x0136E908` (count `0x0136E978`) is applied
first, source U.

`PixelLightShaderDriver` slot 2 `0x00C22DD0` builds one list of up to 4
non-ambient lights and one ambient colour per draw. Light nodes come from the
C+0x1A0 LightSet list (vtable `+0x10` count, `+0x14` array) and from the block
at C+0x308 (up to its count byte at `+0x150`, enable bytes at `+0x148+i`):

- Pass 1 walks the LightSets of C+0x1A0 in list order and each LightSet's
  lights in member order (`0x006213c0`). Non-ambient lights are appended in
  encounter order. A type 4 light counts toward N and overwrites the running
  ambient with its colour*intensity (all four lanes), it does not add.
- Pass 2 walks the C+0x308 block. Type 4 lights add colour*intensity to the
  running ambient and count toward N. Other lights are appended when their
  enable byte `+0x148+i` is zero.
- The list is then sorted by `+0xF4`, larger first, but only the entries
  appended by pass 2 (the sort starts at the pass 1 count). LightSet order is
  therefore stable.
- Ambient result: when N > 0 it is (first 16 bytes of the C+0x308 block +
  running sum / N) * 0.5 (`0x00F62F60`). When N is 0 it is the block's 16 bytes
  unchanged. Those 16 bytes are the per-node block at node `+0x134`, seeded
  to (0, 0, 0, 1.0) and summed with the node's type 4 lights, see
  [Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md#per-node-light-list-and-ambient-seed-v).
- `0x00C222A0` walks the list in order. Type 1 lights take DirLight slot 0,
  then slot 1 and on up to slot 3 where the shader declares the handles, each
  as colour*intensity on all four lanes (`0x00C20B80`), w zeroed, times 1.0
  (`0x01308D2C`). Only slots 0 and 1 are also multiplied by a tint (C+0xC04
  for slot 0, C+0xC14 for slot 1). Type 3 lights take up to four point slots
  (scale 1.0 at `0x01308D30`, params `(-1,-1,+0xE0,+0xE4)`) and type 2
  asserts. Empty slots are zeroed.
- The tints C+0xC04..0xC20 are initialised in the draw context constructor
  region (`0x00C49060..0x00C490E4`, value I 1.0) and rewritten per draw by the
  scheduler at `0x00C586DF` (C+0xC04, C+0xC0C from the node accessor
  `0x00C2DD50` with w = 1.0) and `0x00C58764`, `0x00C58787` (C+0xC14, C+0xC1C from
  node `+0x310` when node byte `+0xA0` bit 7 is set). They are per-draw-node
  tints, not weather values.

Weather lane order (V from the layout, I for the runtime LightSet keeping it):
`ltst_NN` lists `main_lght_NN`, `sub_lght_NN`, `amb_lght_NN` in that order, so
DirLight slot 0 is the main light, slot 1 is the sub light and the ambient
colour is the amb light. The `ltst_NN` members are slot records of
(instance group, ordinal) and the ordinals are 1, 2, 3 (4 in `ltst_11`, 9 to
11 in `ltst_01`) matching the `sgrp_dwev_NN` child ordinals.

## Zone layout lights

- `data/29/B0/00/03.DAT` (the `fld01` shape): 13 LightBase (layout type 0 in 3,
  type 3 in 3, type 1 in 7 named like torch and lamp lights), 3 LightSet, 3
  DrawEnv named `dwev_00`, `dwev_10`, `dwev_01`. One LightSet lists one
  directional and one ambient light, the other two read as empty member lists.
- `data/29/B0/00/01.DAT` (the `twn01` shape): 10 LightBase (type 0 in 1, type 3
  in 1, type 1 in 8), 2 LightSet that each list the same directional and
  ambient pair, 4 DrawEnv named `dwev_00`, `dwev_21`, `dwev_10`, `dwev_01`.
- Zone point lights are in no LightSet. The weather manager registers them
  and hands them to BG models and actors through LightSets it fills, see
  [Zone point light route](#zone-point-light-route).

The zone directional and ambient lights are the LightSet content of the zone
DrawEnv objects. A weather DrawEnv of the same name replaces the holder's
current DrawEnv through the lane-selection path. This object replacement
alone does not identify the LightSet list read by a selected draw.

The [ordinary MeshDrawTag source](model-lightset-source.md#component-list-and-conditional-pixellight-read)
stores the tag at render context+0x1A0; its getters expose the MINS-selected
component list. Equality between that list's entries and the current DrawEnv's
LightSets needs a separate source/selection join. The
[per-node context+0x308 block](chara-engine-samplers-and-light-inputs.md#per-node-light-list-and-ambient-seed-v)
has its own DrawEnv/LightEnv sources. The selected VertexLight branch reads
that separate block, as qualified in the
[driver source finding](model-lightset-source.md#selected-vertexlight-source).
No universal weather-versus-zone light replacement/addition rule for every
draw follows from these distinct paths.

## Zone point light route

### Node getters (V)

`LightBaseObject` (vtable `0x010A6884`,
`.?AVLightBaseObject@Layout@Stella@Lay@Engine@CDev@SQEX@@`) and
`RaptureLightBaseObject` (vtable `0x010618DC`) share slots 39 to 47, which
read the node record D that slot `+0x20` returns.

| Slot | Function | Field |
|---|---|---|
| `+0x9C` | `0x00AAE490` | byte D `+0x44`, LightType |
| `+0xA0` | `0x00AAE4A0` | D `+0x20 +0x24 +0x28`, each `(int)(c * 255.0)` (binary64 `0x00F598B0`), passed with alpha `0xFF` to `0x00ABAB00`, which multiplies each byte by the binary64 `0x010A8A18` (1/255) |
| `+0xA4` | `0x00AAE500` | D `+0x2C`, intensity |
| `+0xA8` | `0x00AAE510` | D `+0x30`, decay rate |
| `+0xAC` | `0x00AAE520` | D `+0x34`, cone angle |
| `+0xB0` | `0x00AAE530` | D `+0x38`, penumbra |
| `+0xB4` | `0x00AAE540` | D `+0x3C`, radius |
| `+0xB8` | `0x00AAE550` | D `+0x48` when byte D `+0x10` is at least 1 (`0x00A94B70`), else 0.0 |
| `+0xBC` | `0x00AAE590` | dword D `+0x40`, priority |

The point arm of the instance apply (`0x00A9FF14`, layout type 1) writes
runtime `+0xDC = 3`, the colour to `+0xC0..+0xCC`, the intensity to
`+0xD0`, the decay rate to `+0xE0`, calls `0x00C2FA40` with the vector
(1, 0, 0, 1), writes the radius to `+0xF0` and `max(0.001, slot +0xB8)` to
`+0xE4` (float `0x00FB7AB4`, `0x00AA0017..0x00AA0141`). Every arm writes
the priority to `+0xF4` (`0x00AA0149..0x00AA0158`). The debug dump
prints `+0xE0` as `DecayRate : %f` and `+0xF0` as `Radius : %f`
(`0x00AA0979`, `0x00AA0A25`). The pixel driver feeds `+0xE0` and `+0xE4`
to PointLightParams, see [Draw-time light list and rules](#draw-time-light-list-and-rules).

### Instance property flags (V)

`LightInstanceObjectProperty` (vtable `0x010A4994`) slots 29 to 36
(`0x00A9F790..0x00A9F8E0`) read the record P that slot `+0x0C` and then
slot `+0x44` return: bits 0, 1, 2 and 3 of byte P `+0`, then the floats P
`+4`, `+8`, `+0xC` and `+0x10`. `0x007FC1D0` copies them into the
`RaptureLightInstanceObjectProperty` (vtables `0x00FF2B48` and
`0x00FF2A9C`, the Stella property at `+4`) as bits 1 to 4 of byte `+0x74`
and the floats `+0x64..+0x70`, then sets bit 0 of `+0x74`. The overrides
`0x00800660..0x008006D0` (slots `+0x74` to `+0x90` of `0x00FF2A9C`) read
them back.

Slot 9 of `0x00FF2A9C` (`0x007FF260`) calls `0x007FEE00`, which acts only on
a runtime point light and returns at once when the four floats are all 1.0
(`0x007FEE3D..0x007FEECD`). Otherwise it applies the
[distance fade](#distance-fade-0x007fee00).

### Registration (V)

`0x007FDC10` registers a light with `WeatherManager::AddPointLight`
`0x007E6E90`, which inserts it into the set at manager `+0xE8` and sets
the byte `+0x1A6`. It requires the manager (`[0x0133DEF4] +0x158`), a
runtime type of 3, a layout type `+0x150` other than 0 and 1, and a changed
cached position (`0x007FC2F0`, `+0x50..+0x5C`). Slot 20 of `0x00FF2A9C`
(`0x007FED70`) calls it after the base method `0x00A9F9A0`. Slot 21
(`0x007FEDA0`) calls the base method `0x00A9FC10` and then calls it unless
the layout type is `0x4000`, `0x4001` or `0x4002`
(`0x007FEDD1..0x007FEDEF`).

### Distance fade `0x007FEE00` (V)

`0x007FEE00` runs only when bit 0 of property byte `+0x74` is set
(`0x007FEE17`) and the runtime node's `+0xDC` is 3 (`0x007FEE3D`). It reads
four floats through property slots `+0x84`, `+0x88`, `+0x8C` and `+0x90`,
called near0, near1, far0 and far1 here, and skips when each equals 1.0
(`FLD1` and `FUCOMIP` for the first, the float `0x00F54F70` for the rest,
`0x007FEE4C..0x007FEECD`). An unordered compare does not skip.

It reads a position through `0x00623C60` `+0x114`, `0x0060B360` and virtual
`+0x88` of that object's `+0x64` (`0x007FEED3..0x007FEF0B`), subtracts the
cached light position `+0x50` and takes the xyz length through
`0x00800DF0` (`0x007FEF0D..0x007FEF58`). The base is the float at property
`+0x60`, which the registration writes from node `+0xD0` right after
`WeatherManager::AddPointLight` (`0x007FDC86..0x007FDC91`). So the base is
the node intensity at registration.

The near ramp `0x007FC3B0` (called at `0x007FEF82`): when
`near0 <= d <= near1` it stores `(float)((d - near0) / (near1 - near0)) * base`
at node `+0xD0`. Otherwise, when near0 differs from near1 and `d <= near0`,
it stores 0.0. Otherwise it writes nothing.

The far ramp `0x007FC440` (called at `0x007FEF9B` with the same stack
arguments): when `far0 <= d <= far1` it stores
`(1.0 - (float)((d - far0) / (far1 - far0))) * base` at node `+0xD0`, with
the binary64 1.0 at `0x00F598A0`. Otherwise, when far0 differs from far1 and
`d >= far1`, it stores 0.0. Otherwise it writes nothing.

Both ramps compute in binary64 and store a float. I: the position is the
camera's, the same object chain the sun and moon orientation takes its
camera vector from. A light inside its near zone or between the ramps keeps
the last value written, and the order of this write against a timeline
clip's intensity write is U.

### Point light clip (V)

The vftables of `LayRapturePointLightClip` (`0x00FBE23C`),
`LayRaptureDirectionalLightClip` (`0x00FBE16C`) and
`LayRaptureAmbientLightClip` (`0x00FBE09C`) share slot 1 `0x0080FEB0` and
slot 2 `0x00810800`. The class names come from the RTTI complete object
locator at vftable -4. I: a point light clip writes its node through the
same enter and update as the lane light clips. Which timelines play a zone
point light clip, and when, is U.

### Assignment `0x007E5D10` (V)

`this` is the weather manager. The arguments are the destination LightSet,
the centre, the kind, the layout, the owner layout, the count and a seed
LightSet. With a count of 0 it returns. It seeds up to two lights from the
seed set at distance -1.0 (float `0x00F62F80`) and starts the free slots at
FLT_MAX (`0x00FBA384`). While fewer than `count` are seeded it walks the
`+0xE8` set and keeps the nearest one (count 1) or two (count 2) by
`0x007EAEF0`, the squared xyz length of the difference, replacing only on
a strictly smaller value and skipping a light already chosen. There is no
range test. The jump table `0x007E61FC` sends the kinds to three filters on
the Rapture property flags, read through slots `+0x74` (bit 0), `+0x78`
(bit 1), `+0x7C` (bit 2) and `+0x80` (bit 3):

| Kind | Admits |
|---|---|
| 0, 3 | same layout (the light's slot `+0x0C` then `+0x14` equals the layout argument), bit 2 set, bit 1 clear |
| 1, 4 | bit 3 clear or the light's slot `+0x20` equals the owner layout, bit 0 set, bit 1 clear |
| 2 | same layout, bit 1 clear |

The chosen lights go to the destination through `LightSet::AddLight`
`0x00C30820`, the first slot then the second (`0x007E61BB..0x007E61DE`).

### BG models `0x007E6800` (V)

`0x007E6800` returns unless `0x00807680` answers 1 and the layout type is
other than `0x1000`, 0 and 1. The count is 2. For a property of type 1 or
2 (BGChip or BGParts) it takes the count from property slot `+0x94` (byte
`+7` of the property node, `0x00A96A80`) and two name offsets from slots
`+0x98` and `+0x9C` (dwords `+8` and `+0xC`), and `0x007E0A50` seeds the
named lights (`0x007E0970` splits a name at `/` for a unit tree child). The
centre is half the sum of the model bounds from the model's `+0x88` method
(float 0.5 at `0x00F62F60`). The call is `0x007E5D10` with kind 0 at
`0x007E69E3`. A second caller `0x007E6A30` passes kind 2, the count from
its object's `+0x7C` method and a centre from `+0x30..+0x3C` of its `+0x60`
object (`0x007E6BBA`), not identified further.

### Actors `0x007D5750` (V for the call, I for its holder)

`0x007D5750`, called from `0x007D85D0` and `0x007D8340`, runs while bit 1
of byte `+0x58` is set. It calls `0x007E5D10` (`0x007D5816`) on the manager
from `0x007E02F0` with the destination at holder `+0x6A0`, the centre from
`0x007D4450` (the position from the holder's `+0x74` object through
`0x00AFA220` plus (0, 1.0, 0, 0)), kind 4, layout 0, the owner layout from
`0x007D43A0(0)`, count 2 and an empty seed set. I: the holder is the
screen environment controller that also selects the DrawEnv lane.

### Point light rank (V)

`LightEnv::GetLightList` `0x00C31E30` reads the cap table `0x013092DC`,
which holds 1, 4, 2, 3, 3 and 1, and the table `0x01374C58`. The rank
`0x00C2FBE0` of a point light is 0 beyond `+0xF0` plus the query radius,
else `intensity / pow(d, +0xE0)` with d the distance (`0x00C2FC26..0x00C2FC82`).
The assignment above does not call this rank.

### Install census (V)

Structure-only read of the Gridania layouts. `0x29B00002` places 68 point
lights. Their property byte P `+0` reads 0 (10 lights), 1 (23), 4 (26) or 5
(9), so 35 admit a BG model and 32 admit an actor. `0x29B00001`,
`0x29B00003` and `0xA09B0002` read 0 and 5, 1, 4 and 5, and 0 and 5. The
`0x29B00000` layout places no light. Every BG placement of the five layouts
resolves a count of 2 under the rule above. Named requests occur in `0x29B00001`, `0x29B00002`
and `0x29B00003`, some as unit tree paths such as
`isgrp_005335/lght_shop_bg_001`.

The distinct pixel programs of the `0x29B00002` models, by pass and by the
register count of `PointLightPositions`, are pass 0: 69 with none and 3
with 2, pass 1: 64 with 1 and 9 with 4, pass 2: 61 with 2. I: a BG model
draws the pixel pass whose index is its point light count. The pass
selection code is U.

## Lane light orbits

V (file, structure-only read). In the weather layout `0x29B0000F`
(`wtr_fine`) the lane timelines `00`, `01` and `20` each bind
two `LayRaptureSunMoonClip` records beside their light clips, one to
`main_lght_NN` and one to `sub_lght_NN`. Both carry the angles (90, 225,
90) degrees, radius 10, record word `+0x28` = 1 and length 2,400,000 ticks,
with offset 0 on the main light and 180 on the sub light. The lanes `10`,
`11`, `12`, `21` and `22` bind none. The `sky` and `cld` timelines carry the
same record shape, with the offsets swapped on `sky` (main 180, sub 0). The
weather layout `0x615A0010` carries the same pair in its lane `00` and no
pair in its lanes `10` to `22`, `13` and `14`.

The record fields, Move and the matrix rows are in the
[SunMoon clip](sky-celestial-layout-census.md#sunmoon-clip-v-0x01060f4c-vtable-enter-0x008d2a40-update-0x008d3990)
section, and the word `+0x28` = 1 makes `0x008D3400` rotate the node on
every update ([Gates](sun-moon-orientation.md#gates)).

I (arithmetic over those records). With `t = 2 pi k + pi / 2` for offset 0
and `k` the day phase, the orbit point is
`(-R cos t, -0.7071 R sin t, -0.7071 R sin t)`. It is below the horizon at
phase 0, at `(R, 0, 0)` at 0.25, 45 degrees above the +Z horizon at 0.5 and
at `(-R, 0, 0)` at 0.75. The sub light with offset 180 is opposite.

U. A directional light shines along `(0, 0, -1)` through its node world
matrix ([Light record](#light-record)). After `0x008D3400` that matrix
holds the look-at basis toward the camera, whose axis roles and handedness
are open on the [orientation page](sun-moon-orientation.md#look-at-basis).
So whether a lane light shines from its orbit point toward the orbit
centre or the other way is not established, and one ruling on that basis
covers both the moon facing and the lane light direction. The mode 4 path
(clip `+0x40`) and the moon phase roll on a light record are also not
traced for the lane lights.

## DrawEnv cubes

The fog parameter block, its submission to the render state and the fog colour
are in the
[environment path](weather-sky-fog-environment-path.md#from-drawenv-to-the-pixel-programs).
Renderer::DrawEnv holds a name pointer at `+0x410` and envmap pointer slots at
`+0x35C + 4*i`, i < 32 (`0x00C20410`). Weather::DrawEnv (ctor `0x008CAF80`,
0x558 bytes) is cloned five times by `0x008CB370` for the blend work set. Env
cubes are filled by the layout initializer `0x00A82020` from CubeMapBaseObject
(`+0xAC` index getter `0x008D5980`). Weather blend `0x007E6690` also renders
`LeapEnvMapTexture` (D3DPERF label) through `DrawEnvCubeBlendShaderDriver`
(inputs `drawEnvCubeMap`, `drawEnvBlendRatio`).

The sampler side of the cube bind, with slot 0 read by every `chara`
material record, is in
[Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md#sampler-records-and-the-engine-bind-v).

### Cube object reference

V. `LayoutDrawEnvObject` (vtable `0x0109FD44`) slot 30 `0x00A81640`
returns the reference at node `+0x28`, and slot 31 `0x00A81650` returns the
`+0x18` row count of the node it references, or 0 when it is null. In the
initializer `0x00A82020`, when slot 31 answers a count and slot 30 a
reference, the finder's `+0x5C` method resolves the reference to an object,
and for each `i` below the count the object's `+0xAC(i)` texture goes to
`SetEnvmapTexture` `0x00C20410` with slot `i` (`0x00A82362..0x00A823B2`).

V (file, structure-only read). In `0x29B0000F` each lane DrawEnv
`dwev_NN` references the cube object `envmap_NN`, and so does each of the
`0x615A0010` lane DrawEnvs. In `0x29B0000F`, `envmap_00` and `envmap_01` name the 64x64 cubes
`env_00_de` (`0x615F0000`) and `env_01_de` (`0x615F0015`), `envmap_10` the
`f0f_fine_` cube `0x72E8030F` that the object `f0f_fine_000` also names,
`envmap_11` `f0d_11_00`, `envmap_12` `f0d_12_00`, `envmap_20` and
`envmap_21` `f0i_21_00`, and `envmap_22` `f0i_22_00`.

V (file). Only the lane `00` and `01` timelines bind a `LayEnvMapClip`, to
`envmap_00` and `envmap_01`. Its payload is a u32 2,400,000, the target
name, a count 9 and nine pairs of a u32 key in tenths of an Eorzea hour and
a cube object name: 0 `f0f_fine_000`, 30 `f0f_fine_000`, 50
`f0f_fine_050`, 61 `f0f_fine_060`, 100 `f0f_fine_120`, 140 `f0f_fine_120`,
179 `f0f_fine_180`, 190 `f0f_fine_190` and 210 `f0f_fine_000`. In
`0x615A0010` the clip on `envmap_00` has the same keys naming the
`w0f_fine_` objects.

I. Slot 0 of a lane DrawEnv is the cube of its own `envmap_NN` object. The
clip swaps the outdoor cubes over the day in lanes `00` and `01` only, so an
indoor lane such as `21` keeps `f0i_21_00`. The runtime use of the clip
payload (class string `0x010A0EDC`) and the blend between two keys are not
traced.

Cube shapes from the installed weather bundles (counts): 128x128 RGBA8 cube,
8 mips, 524820 bytes (9 per layout, names `f0f_<weather>_`, `f0d_`, `f0i_`),
64x64 one-mip cube, 98464 bytes (`env_00_de`, `env_01_de`, keys
`0x615F0000`, `0x615F0015`).

## Unknown

- How `LayEnvMapClip` blends between two cube keys (the blend ratio
  source). Slot 0 is the slot every character material record reads.
- The source of the distance fade position and the order of the fade
  against a timeline clip's intensity write.
- The holder of `0x007D5750`, the caller of kind 2 `0x007E6A30`, and the
  pass selection of a BG model with point lights.
- Layout property byte offsets of Fog and DrawEnv objects.
- Default state before the first weather packet, the `0x0136E908` source, the
  euler order of layout instance rotations on lights.
- Meaning of clock mode message 98.
