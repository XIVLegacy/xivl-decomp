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
  literals `0x00FEF4C0..`). The SPATIAL DrawEnv object the camera is in (zone
  layout object, name via vtable `+0xA4`, caller `0x007DCE65`) picks which lane
  of the weather layout is shown, see
  [lane selection](weather-sky-fog-environment-path.md#lane-selection-by-spatial-drawenv-name).
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
transform write is disabled, see the curve page). In `wtr_fine` lane 00 those
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
- Zone point lights are in no LightSet. Their route into the draw is U
  (the C+0x308 block and the fixed array at `0x0136E908` are the candidates).

The zone directional and ambient lights are the LightSet content of the zone
DrawEnv objects. A weather DrawEnv of the same name replaces that DrawEnv as
the current one (the lane selection path), so while a same-named lane exists
the zone pair is replaced and does not add to the weather pair (I: that the
draw reads its C+0x1A0 LightSets from the holder's current DrawEnv). With no
same-named lane the zone DrawEnv and its pair stay in use (I).

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

Cube shapes from the installed weather bundles (counts): 128x128 RGBA8 cube,
8 mips, 524820 bytes (9 per layout, names `f0f_<weather>_`, `f0d_`, `f0i_`),
64x64 one-mip cube, 98464 bytes (`env_00_de`, `env_01_de`, keys
`0x615F0000`, `0x615F0015`).

## Unknown

- How day/night picks between cube keys (blend ratio source), and which
  element is `CubeMapBaseObject` index 0 per layout. Slot 0 is the slot every
  character material record reads.
- Layout property byte offsets of Fog and DrawEnv objects.
- Default state before the first weather packet, the `0x0136E908` source, the
  euler order of layout instance rotations on lights.
- Meaning of clock mode message 98.
