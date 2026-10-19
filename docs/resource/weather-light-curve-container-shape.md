# Weather light curve container shape

Where the time-varying light, fog and vertical fog values of a weather layout
sit in the install, and which parts decode with the tracked MTB pages. Counts
and shapes only, no retail values. Static read of the `fst_f0` weather DAT
`data/29/B0/00/0F.DAT` (`wtr_fine`, 129,856 bytes) and of `ffxivgame.exe`
(SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`). Tiers: V = verified from file bytes or instructions,
I = inferred, U = unknown.

This page owns the timelines, the curve containers and the light clip
semantics. The MTB framing and property record sizes are in
[MTB container framing](mtb-container-framing.md) and the key layout in
[MTB FixedCurve sampling](mtb-fixed-curve.md). The DAT inventory and clip
class names are in
[Weather layout resource shape](weather-drawenv-resource-shape.md). The row
selection of `0F.DAT` (the first type 0 row of the `fst_f0` root, id 8001) is
in [Region weather resource rows](region-weather-resource-rows.md) and the
[environment path](weather-sky-fog-environment-path.md). The clock, light node
and draw-time list are in
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md).

## Static lyb nodes carry no animated colour (V)

The `lyb` payload of `0F.DAT` is a normal node-type table (SEDB frame at file
offset `0x3C0`, payload header `0x18`). Node types of interest:

| Class | Count | Node size | Reference fields |
|---|---:|---|---|
| LightBaseObject | 29 | `0x50` | class `+4`, name `+8` only |
| DrawEnvObject | 10 | `0x3C` | class, name, node references at `+0x18/+0x20/+0x24/+0x28/+0x30`, one float at `+0x34` |
| FogObject | 11 | `0x18` | class, name, record reference at `+0x10` |
| LightSetObject | 10 | `0x1C` | class, name, member list at `+0x10/+0x18` |

A LightBaseObject node holds no colour or intensity reference. Its words are
a fixed per-object template (the same prefix on main, sub and ambient nodes).
Each FogObject record (12 dwords) is the same default template on the three
nodes read. So the colour, intensity and fog values come from the timelines.

## Timelines are SCB blobs holding MTB containers (V)

Between `0x8870` and the end of the lyb frame the DAT holds 12 SCB blobs
(one sound definition, sky, cloud, the eight lanes and `time_sqnc_24`), found
as 12 `@CRST` chunks, with nested `SEDB mtb` containers named
`FCurveNNNN.fcr`. The `time_dwev_00` SCB (file
offset `0xF2D0`) binds `main_lght_00`, `sub_lght_00`, `amb_lght_00`,
`fog_00`, `envmap_00`, `scev_00`, `vfog_00`, `lightMapOcclusionColor` and
`diffuseColor_00` through clip classes, and 8 MTB containers follow it.

Every container checked is version `0x43`, header size `0x40`, declared size
zero, with the tracked framing. Walking the Curve sections with the property
record sizes of the framing page ends cleanly on all 8 containers of the
`time_dwev_00` timeline.

| Group name list | Curve section shape (property types and key counts) |
|---|---|
| `Root TransForm MoveRatio Color Extra CharaBias VfxBias` (light clips) | Root: no properties. TransForm: 9 type 0 constants. MoveRatio: one type 4 property with 2 keys. Color: 6 type 2 curves (3 with 12 to 14 keys, 3 with 8 to 14). Extra: one type 0 constant. CharaBias: 3 type 2 curves (8 to 11 keys). VfxBias: 3 type 0 constants |
| `Root Range TopColor MiddleColor BottomColor` (vertical fog clip) | Root 3 constants, then three sections each with 3 type 2 curves of 12 keys, 3 type 2 curves of 10 keys and 1 constant |
| `Root Color Fog CharaBias VfxBias` (fog clip) | Color: 7 type 2 curves of 12 keys. Fog: 3 type 2 curves of 12 keys and 3 constants |
| `Root Attr` (two containers) | one section of 3 curves of 11 keys plus 1 constant, or 4 constants |
| `Root Glare` (glare clip) | 7 properties, 4 type 2 curves of 7 to 16 keys |

Key times are the first lane of each type 2 key. Over every curve read the
times are monotonic and span `0.0` to `240.0` (a few light curves end at
`210.0` and hold their last key by the clamp). A span of 240 units is one
Eorzea day at 10 units per hour, which matches the timeline position
`(int)(length * phase)` of `0x0083AFF0`. That rule and the clock inputs are in
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#time-of-day).
The timeline length is a tick count rather than 240, see the SCB section
below.

## Section roles and property names (V)

The first dword of every Curve section payload is a group-name index (low 31
bits, into the group-name table). The curve container finds a group by name
(`0x00A2B720`, container vtable `+0x28`) and a property inside it by name
(`0x00810FE0`). The 14 mask-selected property names are filled by the
initializer `0x00A2FE50` into the string table at `0x01365164` (stride `0x1C`,
mask to slot map in `0x00A30110`):

| Mask | `0x1` | `0x2` | `0x4` | `0x2000` | `0x4000` | `0x8000` | `0x10` | `0x20` | `0x40` | `0x100` | `0x200` | `0x400` | `0x800` | `0x1000` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Name | tx | ty | tz | rx | ry | rz | sx | sy | sz | qrot | qx | qy | qz | qw |

Section order in every light container equals the group name order, so the
roles of a light container are:

| Group | Properties in the install | Read by |
|---|---|---|
| Root | none | nothing in LightClip |
| TransForm | tx ty tz translation, rx ry rz rotation in degrees (degrees to radians with pi/180 in `0x00A0E6A0`), sx sy sz scale | `0x00A0ECA0`, `0x00A0E6A0` |
| MoveRatio | tx, type 4 | same |
| Color | tx ty tz sx sy sz | `0x00A0D0B0`, `0x00A0CD00` |
| Extra | tx, optionally ty tz sx sy sz | same |
| CharaBias, VfxBias | 3 properties each | not LightClip |

Over the 525 light-shaped containers in the DATs of `data/29/B0/00`, Extra is
tx alone as a type 0 constant in 464 and tx as a type 2 curve in 60 (with at
most one further curve). Color holds six type 0 constants in 357, constant tx
ty tz with type 2 sx sy sz in 100, six type 2 curves in 35, a type 2 tx with
constant ty tz and type 2 sx sy sz in 23, constant tx ty tz with type 4 sx sy
sz in 6 and constant tx ty tz sx with type 2 sy sz in 4. Every light container
has a type 4 MoveRatio ahead of Color, so a section walk meets types 0, 2 and 4
before it reaches Color. The record sizes are in the
[framing page](mtb-container-framing.md#property-record-sizes).

## Light clip colour and intensity (V)

LightClip enter (`0x00A0D0B0`) binds Color tx ty tz sx sy sz to six curve slots
(CurveR CurveG CurveB CurveRS CurveGS CurveBS in its assertion strings) and
Extra tx ty tz sx sy sz to six more (CurveIntensity first). Update
(`0x00A0CD00`) samples each bound curve through its vtable `+0x0C` with one
float and stores the raw float, with no clamp, no 255 scale and no gamma:

| Node offset | Value |
|---|---|
| `+0xC0 +0xC4 +0xC8` | Color tx*sx, ty*sy, tz*sz (R*RS, G*GS, B*BS) |
| `+0xCC` | constant 1.0 (`0x00F54F70`) |
| `+0xD0` | Extra tx, the intensity, written before the flag tests below |
| `+0xE0` | Extra ty |
| `+0xE8 +0xEC +0xF0` | Extra tz, sx, sy |
| `+0xE4` | Extra sz floored at 0.001 (`0x00FB7AB4`) |

The clip does not multiply colour by intensity. The draw does, see
[zone-lighting-source-and-time-of-day.md](zone-lighting-source-and-time-of-day.md).
The curve argument is the clip local time in ticks divided by the frame length
global at `0x012ED240`. That global reads 10010 in the image and
`0x00A20490(fps)` sets it to `(int)(300000 / fps)`. Callers are in
`0x00A025C0` and `0x00A036E0`. Whether the weather scheduler changes it from
10010 is U, so the argument at phase 1.0 is 239.76 (10010) or 240 (10000).
That the clip local time equals the timeline seek position of `0x0083AFF0` is I.

Colour writes are skipped when record byte `+0x19` has bit 7 set. The
`+0xE0..+0xF0` writes are skipped when record byte `+0x18` has bit 0 set. Every
light clip record of `0F.DAT` has a `+0x18` byte of `0x07` or `0x0B` and a
`+0x19` byte of zero, so only Color and Extra tx are applied. The shared
transform update (`0x00A0E6A0`) returns before writing anything when bit 1 of
`+0x18` is set, so these lights keep the transform of their layout node and
never apply TransForm or MoveRatio. The TransForm constants are therefore not
the light axis source for weather lights.

## SCB clip table and fcr pairing (V)

An SCB is a 0x30 byte header followed by tagged chunks `@CRST`, `@CRES`,
`@CATT`, `@CCPT`, `@CACT`, `@CDPT`, `@CTRK`, `@CBKT`, `@CBLK`, `@CCNT`,
`@CBIS` and three more `@CRES` chunks. Parts that matter for lights:

* `@CACT` lists the actors in index order (`main_lght_00`, `sub_lght_00`,
  `amb_lght_00`, `fog_00`, `envmap_00`, `scev_00`, `vfog_00`,
  `lightMapOcclusionColor`, `diffuseColor_00` for lane 00).
* `@CBLK` holds the block name at `+0x10`, a u32 length at `+0x24`, a u32 clip
  count at `+0x28` and the clip records from `+0x40`. The length reads
  2,400,000 in the sky, cloud and eight lane SCBs of `0F.DAT` (the first sound
  definition SCB reads 9,000,000 and `time_sqnc_24` reads 24,000,000). The fcr
  Header length float reads 240.0 and light records carry 240 at `+0x1C`.
* A clip record is a u16 size, a u8 class index, a u8 actor index and a u32
  time, then class fields. The class index points into a per-SCB class name
  list (`LayRaptureDirectionalLightClip` is 0 in the sky, cloud, `00`, `20`
  and `01` SCBs and 3 in the other lanes, so the index is not portable across
  SCBs).
* The FCURVE `@CRES` chunk is a u16 count, then one u16 per entry giving the
  entry offset in 16 byte units from the first entry. An entry is the
  `FCurveNNNN.fcr` name followed by its container. Offsets are monotonic, so
  entry order is file order. NNNN is an authoring number and is not an index.
* A light clip finds its container by the u16 at record `+0x1A` (clip enter
  `0x00A0ECA0` reads it and resolves it with `0x00A20090` and `0x00A220A0`).
  Fog, vertical fog, glare and material clips use the u32 at `+0x14`.

Light entry indices read from the records of `0F.DAT` (each selects a
light-shaped container and no two lights of an SCB share one):

| SCB | main | sub | amb |
|---|---|---|---|
| sky | 1 | 0 | none |
| cld | 1 | 0 | 2 |
| 00 | 1 | 0 | 7 |
| 20 | 1 | 0 | 2 |
| 01 | 1 | 0 | 4 |
| 10, 11, 12, 21, 22 | 3 | 4 | 2 |

Pairing the lights with the light containers in file order would give the main
light the sub container in five SCBs (sky, cld, 00, 20, 01) and the ambient
container in five (10, 11, 12, 21, 22). Fog,
vertical fog and glare of lane 00 read entries 3, 2 and 6 and the two Attr
containers belong to `diffuseColor_00` and `lightMapOcclusionColor` through
entries 4 and 5.

## Open

* The scheduler frame rate in effect for the weather timelines, which sets the
  curve argument scale (10010 or 10000).
* Whether clip local time tracks the timeline seek position one to one.
* The fog record field roles against the Fog and Color sections are in
  [Weather fog parameters and fog mix](weather-fog-parameters.md).
* The meaning of Extra ty tz sx sy sz for point lights beyond the node offsets
  above. Weather lights do not apply them.
