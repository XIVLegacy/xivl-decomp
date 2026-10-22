# Sky celestial layout census

What the region sky row (`sky_celestial`) holds, which shader constants its
models read and which clip supplies them. Static read of the pinned install and
`ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`, all addresses are VAs). Counts and shapes only, no retail values.
Tiers: V = verified from file bytes or instructions, I = inferred, U = unknown.
Context pages: [weather-sky-fog-environment-path.md](weather-sky-fog-environment-path.md),
[weather-light-curve-container-shape.md](weather-light-curve-container-shape.md),
[zone-lighting-source-and-time-of-day.md](zone-lighting-source-and-time-of-day.md),
[pram-packed-render-state.md](pram-packed-render-state.md),
[mtb-container-framing.md](mtb-container-framing.md).

## Census across the install (V)

The region table has 53 type 1 rows. They name 30 distinct DAT keys. Of those, 21
files exist in the install and serve the 47 `sky_celestial` rows, 6 keys
(`0x29BF0000`, `3`..`7`) are the test rows of one root, and 3 keys have no file.
All 21 sky layouts reference the same 8 model keys `0x5CE50005` through
`0x5CE5000C`. Nine of them add the two meteor models `0x5CE5000E` and `F`.
Three add five `dmy_sky_*` test models (`0x5CE50000`..`0x5CE50004`). No model of any of the 21 layouts declares
a `verticalColor<N>` or `lightColor<N>` constant. The sphere model is the only
one that declares `gradationAmbient`, and it is the same key in all 21.

## Region 103 layout `0x29B0000E` (V)

`MapLayoutResourceData` 1.1.0, 25536 bytes, 58 dependency records, an embedded
`lyb` at file offset `0x780` and 5 embedded `SCB` blobs at `0x4760`, `0x4FB0`,
`0x5960`, `0x5CF0` and `0x6080`.

| Dependency kind | Count | Notes |
|---|---:|---|
| `trb` models | 10 | keys `0x5CE50005`..`0x5CE5000C`, `0x5CE5000E`, `0x5CE5000F` |
| `mtb` | 1 | key `0x5CE60000`, 303 bytes, holds a `SpuBinary` type 5 property |
| `txb` textures | 4 | keys `0x615F0007`..`0x615F000A` |
| VFX packs `vfx_meteo_001..003` | 3 | each with `vtex` 6, `vmdl` 4, `veff` 1, `leaf` 1, `vins` 1 |

Layout nodes: 10 `BGPartsBaseObject` (names `sky0_bg`, `sky0_sphere`, `sky0_star`,
`sky0_sun`, `sky0_moon`, `sky0_moonchild`, `sky0_indoor`, `sky0_fog`,
`sky0_meteor118`, `sky0_meteor120`), 10 UnitTree groups, 11 InstanceObjects, 2
DrawEnv, 9 Fog, 2 LightBase (`main_lght_sky`, `sub_lght_sky`), 1 LightSet, 6
TimeLine (`time_sky`, `time_dwev_sky`, `time_moonchild`, `time_meteor118`,
`time_meteor120`, `time_meteor`) and 3 VFX. Group names: `sgrp_sky` (children
`sky0_star`, `sky0_sun`, `sky0_moon`, `time_sky`, `sky0_bg`, `sky0_fog`),
`sgrp_dwev_sky` (the two lights and `time_dwev_sky`), `sgrp_indoor`, and
`sgrp_meteor` plus `sgrp_meteor117`, `118`, `120`, `121`, `122`, `123`.

Transforms (V): every instance of `sgrp_sky`, `sgrp_dwev_sky` and `sgrp_indoor`
has zero translation, zero rotation and unit scale. The meteor group instances
carry translations. The only non-identity rotations in the sky and light groups
are a tilt about X on the `sky0_star` group (one UnitTree child record) and
plus and minus a quarter turn about X on `main_lght_sky` and `sub_lght_sky`. Every
BG model node stores a bounding box. The dome half extents are of the order of
thousands of units (`sky0_bg` largest, then `sky0_star`, `sky0_sphere`,
`sky0_fog`), `sky0_indoor` is a box of a few hundred, the sun and moon are
flat discs of a few hundred and tens of units.

## Models, programs, PRAM, constants (V)

Tool: a Python port of the repository SHD and PRAM readers plus a D3D9
constant-table (`CTAB`) reader over the vertex and pixel bytecode. All programs
are shader model 3 (`vs_3_0` and `ps_3_0`). The PRAM word is the little-endian
`i16` at PRAM offset 0.

| Node (model key) | Material programs (pixel passes) | PRAM word | PRAM-supplied pixel names | Non-PRAM float constants |
|---|---|---|---|---|
| `sky0_bg` (`..05`) | `sky0_00_blak0_1h` (000..002 identical) | `0x0818` | diffuseColor, modulateColor, glareLdrScale | latitudeParam |
| `sky0_sphere` (`..08`) | `sky0_ad_sphere_1h` (000..002 identical, 2316 bytes) | `0x0919` | gradationAmbient, gradationDiffuse, verticalEx1Color, verticalEx1Param, verticalEx2Color, verticalEx2Param, verticalEx2Alpha, verticalEx3Color, verticalEx3Param, verticalEx3Alpha, verticalVec, mainLightEx1Param, mainLightEx2Param, subLightEx1Param, subLightEx2Param, exAlphaParam, modulateColor, glareLdrScale | DirLightColors (2), DirLightDirections (2), latitudeParam |
| `sky0_star` (`..09`) | `sky0_ad_const_1h` and `sky0_ad_star_2h` | `0x0909` both | diffuseColor, exAlphaParam, modulateColor, glareLdrScale and for the star program multiDiffuseColor | latitudeParam |
| `sky0_sun` (`..0A`) | `sky0_ad_sunx0_1h` | `0x0909` | diffuseColor, diffuseScale, fogColor, exAlphaParam, modulateColor, glareLdrScale | latitudeParam |
| `sky0_moon` (`..06`) and `sky0_moonchild` (`..07`), distinct files with identical shapes | `sky0_00_blak0_1h` and `sky0_ad_moon0_4h` | `0x0818` and `0x0809` | diffuseColor, exAlphaParam, modulateColor, glareLdrScale | latitudeParam |
| `sky0_indoor` (`..0B`) | `sky0_00_indoor_1h` | `0x0818` | diffuseColor, fogColor, modulateColor, glareLdrScale | latitudeParam |
| `sky0_fog` (`..0C`) | `sky0_00_fog_1h` | `0x0809` | diffuseColor, fogColor, exAlphaParam, modulateColor, glareLdrScale | latitudeParam |
| `sky0_meteor*` (`..0E`, `..0F`) | `sky1_ad_meteor` (000..002 identical), plus `sky0_00_blak0_1h` in `..0F` | `0x0909` and `0x0818` | diffuseColor, diffuseScale, fogColor, exAlphaParam, modulateColor, glareLdrScale | latitudeParam and the shadow family (shadowColor, shadowFadeParam, shadowOffset0, shadowProjMatrix0..3) |

Vertex programs read only `worldMatrix`, `worldITMatrix`, `worldViewProjMatrix`,
`ModelBBoxOffSet`, `ModelBBoxScale`, plus `viewITMatrix` (every program but the sphere), `worldViewMatrix` (sun, indoor, fog, meteor), `fogParam`
(sun, indoor, fog, meteor, vertex-side PRAM) and `uvofs0..2` (star, moon, const).
`isUseInstancing` is a bool register. The sphere vertex program has no camera
constant and no `z = w` forcing. The black, sun, star, moon, indoor and fog vertex
programs compute a camera-to-vertex direction from the translation lane of
`viewITMatrix` and write it to a texcoord output.

Samplers (CTAB): the star `sky0_ad_star_2h` program has `_sampler_00..02`, the
const, star and moon programs have `_sampler_00`, the meteor pixel passes declare
`shadowMap0` (register s10) and carry no PRAM sampler. Each PRAM sampler's third
candidate string equals a layout dependency name of a `txb` record, which
resolves the texture without a path lookup: star sampler 2 to `0x615F0007`,
sampler 0 to `0x615F0008`, sampler 1 and the const sampler to `0x615F0009`, moon
to `0x615F000A`.

Vertex inputs (V from `dcl`): sphere and black bg take position and normal.
Sun takes position, color0, normal. Star takes position, color0, color1, normal,
texcoord0..2. Moon takes position, color0, color1, normal, texcoord0..1.
Indoor and fog take position, normal, blendindices0, texcoord6, texcoord7.
Const takes position, normal, texcoord0..1. Meteor takes position, color0, normal.

## Pixel program of the sphere (V from the bytecode, I for names)

Input: texcoord0.w (equal to 1 from the vertex program) and the world-space
normal in texcoord3. The program normalizes the normal, then:

* Takes `h = dot(verticalVec, N)`.
* Base colour is gradationDiffuse. For each of the three verticalEx lobes it forms
  `pow(h * Param.x + Param.y, Param.z)` with a guarded `log2` (a `cmp` clamps
  at `1e-5`), multiplies by the lobe Color. Lobe 1 adds to the base. Lobes 2 and 3
  blend toward their colour by `pow(saturate(h * Alpha.x + Alpha.y), Alpha.z)`.
* Adds gradationAmbient.
* Adds two glow terms per directional light, each `pow(d * Ex.x + Ex.y, Ex.z)`
  with `d = dot(-DirLightDirections[i], N)`, scaled by `DirLightColors[i]`,
  using mainLightEx1/2Param for light 0 and subLightEx1/2Param for light 1.
* Multiplies by modulateColor, then `rgb * latitudeParam.x + latitudeParam.y`,
  then `* latitudeParam.z`, then `* glareLdrScale`.
* Alpha is `pow(saturate(h * exAlphaParam.x + exAlphaParam.y), exAlphaParam.z)`
  times texcoord0.w.

The pixel program depends on the vertex normal and the lights only. It has no
camera or world position input. verticalEx1 has no Alpha constant in the pixel
program. The sun, star, const, moon and fog pixel programs apply the same
`pow(saturate(n.y * exAlphaParam.x + exAlphaParam.y), exAlphaParam.z)` factor to
alpha with `n` the world-space normal. The PRAM exAlphaParam of the sun, star,
const and moon programs has a zero scale and a bias and power of one, which makes
this factor one for any normal.

## Fit to the existing strict preload (V from reading `constants.rs` and `zone.rs`)

Strict model preload refuses a program when a float constant is named by neither
the PRAM nor the engine table. Over every vertex and pixel program of the 15
model keys of the 21 sky layouts, no float constant is left unfilled, so no
engine stand-in has to be added. Matrices, ModelBBox, uvofs, fogParam,
modulateColor, glareLdrScale, latitudeParam, DirLightColors, DirLightDirections,
the shadow family and diffuseScale are in the engine table. Everything else is
PRAM. The sphere's gradation and vertical lobes would draw from their PRAM
defaults, which are authoring test colours, so they need the clip values below.
Pixel pass 000 is the default pass and the three passes are byte-identical.
`runtime_materials.rs` accepts only the words `0x0818` and `0x081A` (nonblended)
and rejects the sky words `0x0919`, `0x0909` and `0x0809`. The shader dialect
(`cmp`, `pow`, `log`, `exp`, `nrm`, `rsq`, `texld`) is in the translator opcode
list.

## PRAM render word of the sky materials (V)

Consumer `0x00C3D340`. Bits used by the sky words:

| Bit | Effect |
|---:|---|
| `0x0001` | alpha blend enable (also forced when a context float at `+0xBF0` or `+0xC00` is below 1.0) |
| `0x0008` | cull mode lookup entry 1, clockwise cull. Set in all six words |
| `0x0010` | depth write |
| `0x0800` | depth test |
| `0x0100` | blend op ADD with source SRCALPHA and destination ONE (additive) |
| `0x0200` without `0x0100` | blend op REVSUBTRACT with SRCALPHA and ONE |
| neither | blend op ADD with SRCALPHA and INVSRCALPHA |
| `0x0004` | alpha-to-coverage toggle through `0x0041C340` |
| `0x0002` | alpha test (not set by any sky word) |

Blend tables: factors at `0x00F596E0` are ZERO, ONE, SRCCOLOR, INVSRCCOLOR,
DESTCOLOR, INVDESTCOLOR, SRCALPHA, INVSRCALPHA. Ops at `0x00F5971C` are ADD, MIN,
MAX, SUBTRACT, REVSUBTRACT, ADD. The blend calls are `0x00A53E00` (ops, device
states 171 and 209) and `0x00BA0BC0` (factors, states 19, 20, 207, 208).

| Word | Depth test | Depth write | Blend | Materials |
|---:|---|---|---|---|
| `0x0818` | on | on | off | black bg, black disc of moon, moonchild and meteor group, indoor |
| `0x0919` | on | on | additive | sphere |
| `0x0909` | on | off | additive | star, const, sun, meteor |
| `0x0809` | on | off | alpha | moon textured part, fog |

## Placement and draw order (b)

* V: all sky BG nodes sit at the layout origin. No sky pixel or vertex program
  takes a camera-position constant for placement. The sphere shading depends on the
  normal alone, so it does not depend on where the dome is centred.
* V: `WeatherManager::OnUpdate` (`0x007E91D0`) updates the sky slot object at
  manager `+0x7C` before the weather queue and `WeatherManager::OnDraw`
  (`0x007E6DB0`) calls its vtable `+0x58` before the weather queue entries. The
  object class `MapLayoutWeatherSky` differs from `MapLayoutWeatherBase` in the
  destructor slot only, so the sky is an ordinary layout draw.
* U: whether the sky layout is moved to the camera each frame, and by what code.
  The sphere half extent is smaller than the distance from the origin to the far
  corners of the region 103 field tiles (about 4000 units, from the collision
  extents of the region 103 layout census), which makes a fixed origin placement
  implausible (I for camera following). The mechanism was not found. The GPU draw order inside the scheduler is U.
  The PRAM words give the constraint that the black bg (opaque, depth write)
  and the sphere (additive, depth write) draw before the stars, sun and moon (no
  depth write) and that the black moon disc writes depth.
* U: far clip. The strings `enable_bgmodel_clip_far`, `common_clip_range`,
  `RaptureCameraNearFarClip` and `ModelObject::SetZPixelClipFar` exist and none was
  traced to the sky.
* The only non-lighting transform animation in the sky layout is the star dome. The
  `LayTransformClip` on `sky0_star` and on `dmy_sky_starlin` use FCurve entries with
  a TransForm group whose `ry` is a two-key type 2 curve spanning the full
  240-unit day with a full turn about Y, over a type 4 MoveRatio. Other TransForm
  properties are type 0 constants (V). The Transform clip record byte `+0x18` is `0x28` (bit 1 clear, I for applying
  the transform).

## SunMoon clip (V, `0x01060F4C` vtable: enter `0x008D2A40`, update `0x008D3990`)

Record, 48 bytes, fields from `+0x14`: three Euler angles in degrees (floats at
`+0x14`, `+0x18`, `+0x1C`), an orbit radius float at `+0x20`, an i32 degree offset
at `+0x24`, a word at `+0x28` (zero in the sun record, one in the moon and light records,
meaning U) and the clip length u32 at `+0x2C`. Enter
converts the angles to radians and wraps each to `[-pi, pi]` and stores the
offset as `(offset + 90) * pi / 180`.

`Move` (`0x008D37A0`): `k = (ticks mod length) / length` (an assertion requires
`0 <= k <= 1`) and `theta = wrap(k * 2pi + offsetRadians)`. The local position is
`(0, sin(theta) * R, -cos(theta) * R, 1)`. It is multiplied row-vector style by
the matrix of `0x0062DDC0`, which in terms of `c = cos(angle)` and `s = sin(angle)`
of the three radians `(a0, a1, a2)` has rows
`(c1c2, c1s2, -s1)`, `(s0s1c2 - c0s2, s0s1s2 + c0c2, s0c1)` and
`(c0s1c2 + s0s2, c0s1s2 - s0c2, c0c1)` with a zero translation row (hand-decoded
from the SIMD bytecode, then checked by evaluation). The result is written to the
node translation through the target vtable `+0x34`. With the sky records the sun
(offset 0) is below the horizon at phase 0, on the +X horizon at 0.25, at positive
elevation on the +Z side at 0.5 and on the -X horizon at 0.75. The moon record has
offset 180 and is opposite. Tick source and length are the clip local time and
clip length. That `k` equals the day phase is I (the block length equals the clip
length in all sky records).

Orientation: the non-mode-4 path `0x008D3400` also sets a node rotation from the
camera object, the node position and an integer `0..7` moon phase index computed
from the ET clock. The [sun and moon orientation](sun-moon-orientation.md) page
records the gates, the phase index and the roll. The facing basis of the moon
node is partly decoded there and the sun disc is not rotated by this path.

The same clip class drives the directional lights `main_lght_sky` and
`sub_lght_sky` (a small radius and an offset of 0 or 180) and `sky0_meteor120`.
`SunMoonChild` drives `sky0_moonchild` and `sky0_meteor118`. Two clip sets
target the two sky light actors by name: the sky layout timeline `time_dwev_sky`
(constant colour containers, `main_lght_sky` offset 0 and `sub_lght_sky` offset
180) and the weather layout `time_sky_00` (curve containers, `main_lght_sky`
offset 180 and `sub_lght_sky` offset 0). The offsets are swapped between the two
(V). Which set wins per frame is U.

## CelestialSphereEx clip (V, vtable `0x0105D0AC`: enter `0x008C8B50`, update `0x008C9FB0`)

Lives in the weather layout, not the sky layout. Each of the 16 weather layouts
(`data/29/B0/00` `0F`..`17`, `1A`..`20`) holds exactly one `LayRaptureCelestialSphereExClip`
on the actor `sky0_sphere` (a sky-layout node, found by name), in the sky SCB
(`time_sky_00`), and exactly five `LayRaptureExAlphaClip`s on `sky0_moon`,
`sky0_sun`, `sky0_fog`, `sky0_moonchild` and `sky0_star`. The clip finds its
container through the record u32 at `+0x14`. In `0F` that is FCurve entry 7
(11234 bytes) and the container has 13 sections: `Root` (empty) plus the 12
groups below. All 16 containers parse with groups of this name set.

Enter copies 240 bytes from the draw environment at `ctx + 0xAA4` into a work
block B (clip `+0x3C`), update fills B, then copies B back to `ctx + 0xAA4`
(`0x007EA150`). The skip flag at clip `+0x38` is set when the actor is named
`LaySystemActor`. The curve argument is clip ticks over the frame length call
`0x00A1FFC0`.

| Group (properties) | Block B offset | Written value | Pixel constant (driver Apply `0x00C66A80`) |
|---|---|---|---|
| `Ambient` tx ty tz sx sy sz | `+0x00` | `(tx*sx, ty*sy, tz*sz)`, lane 3 untouched | gradationAmbient, `ctx+0xAA4` |
| `Diffuse` | `+0x10` | same product | gradationDiffuse, `ctx+0xAB4` |
| `Virtical_Key<n>_Color` (n = 0..2) | `+0x20 + 0x30n` | `(tx*sx, ty*sy, tz*sz, 1)` | verticalEx(n+1)Color, `ctx+0xAC4+0x30n` |
| `Virtical_Key<n>` tx ty tz | `+0x30 + 0x30n` | `(tx, ty, tz, 1)` | verticalEx(n+1)Param, `ctx+0xAD4+0x30n` |
| `Virtical_Key<n>` sx sy sz | `+0x40 + 0x30n` | `(sx, sy, sz, 1)` | verticalEx(n+1)Alpha, `ctx+0xAE4+0x30n`, null handle for n = 0 |
| `MainLight_Key<n>` (n = 0, 1) tx ty tz | `+0xB0 + 0x10n` | `(tx, ty, tz, 1)` | mainLightEx(n+1)Param, `ctx+0xB54+0x10n` |
| `SubLight_Key<n>` (n = 0, 1) | `+0xD0 + 0x10n` | same | subLightEx(n+1)Param, `ctx+0xB74+0x10n` |

The routines are `0x008C9790` (Virtical, group name built from `Virtical`, `_Key`
and the digit, then `_Color`) and `0x008C9CD0` (MainLight and SubLight). The
driver also writes `verticalVec` from a fixed 16-byte global at `0x01374E90`.
That the draw context the clip writes (`clip+0x40`, set from the scene object
at enter) is the same object the driver reads is I (same offsets).

The stop positions are the verticalEx `Param` vectors `(scale, bias, power)` in
the formula above. They are curves in 3 groups (tx, tz, and some of ty or
sx, sz) and constants elsewhere. There is no array of eight gradient stops
in the region sky.

The eight-stop variant: the driver `Init` (`0x00C66750`) takes this branch when
the program has no `gradationAmbient` handle and then resolves
`verticalColorStart/End/Key/Key2`, `verticalColor1..8`, `lightColorStart/End/Key/Key2`,
`lightColor1..8` and `lightColorBlend`. `Apply` reads them from `ctx+0x920` to
`ctx+0xAA0` (Start `+0x920`, End `+0x930`, colours from `+0x940` stride 0x10, Key
`+0x9C0`, Key2 `+0x9D0`, light Start `+0x9E0`, End `+0x9F0`, light colours from
`+0xA00`, Key `+0xA80`, Key2 `+0xA90`, blend float `+0xAA0`). The writer is
presumably `RaptureCelestialSphereClip` (enter `0x008C89C0`, update `0x008C9650`),
which was not decoded (U). No region sky model uses this variant.

Container shape of the 12 groups in `0F` (key counts): Ambient and Diffuse are
six type 2 curves of 12 keys. `Virtical_Key0` has tx, ty, tz and sy as 12-key
curves with sx and sz constant. `Virtical_Key1` and `Virtical_Key2` have ty constant
and the rest 12-key curves. Each `_Color` group is six 12-key curves. `MainLight_Key0`
has three curves, the other light groups have ty constant. In `0F` the curve types are
0 and 2 only, so the existing type 0 and type 2 samplers read them. Across the 16
weather layouts the same 13-section group list holds in all of them, and layouts
`1E` and `1F` also carry type 4 single-key properties in Ambient, Diffuse, the
Virtical groups and the light groups, and in the ExAlpha ty properties of the
moon, moonchild and sun (and tx ty tz of the fog). The decode of a type 4 key
(a u32 and a u16 with two unread bytes) into a value is U.

## ExAlpha clip (V, enter `0x008C8D20`, update `0x008C8860`)

Reads group `Alpha` (tx, ty, tz) at the clip argument and, unless the actor is
`LaySystemActor`, passes `(tx, ty, tz, 1)` to the model object at actor vtable
`+0x5C`, then `+0x11C`, then `+0x104`. Together with the pixel formula this makes
tx the scale, ty the bias and tz the power of the vertical alpha factor, so with
tx zero the Alpha ty curve is the model alpha. Weather transitions blend these
through `LeapExAlphaClip2` (string at `0x00FEFDF0`). In `0F`: moon, moonchild and
star have a type 2 ty (8, 10 and 7 keys), the sun a type 2 ty (6 keys), the fog
a constant ty. tx is constant and tz is constant in all five.

## Noon snapshot (d)

Sampling `0F` (weather id 8001, `wtr_fine`) at curve argument 119.88 (ticks over
the 10010 frame length) and at 120.0 gives the same result:

* V: every group of CelestialSphereEx has all its properties. The sphere
  constants gradationAmbient, gradationDiffuse, verticalEx1..3 Color and Param,
  verticalEx2..3 Alpha and the four light Param pairs are all defined, so the
  daytime constant set is complete from this one container.
* V: ExAlpha ty at noon is zero for moon, moonchild and star, one for the sun,
  and zero for the fog dome. The moon, moonchild and star zero spans and the sun one
  span are the middle of the day and contain key time 120. The fade edges are
  short (one to three keys).
* V: visibility is keyed by the ExAlpha clip curve per model, through the
  exAlphaParam of that model. The sphere, bg and indoor models have no ExAlpha
  clip and always draw. The meteor groups are shown or hidden by `0x0062AEB0`
  from three bit masks at scene `+0x358`, `+0x35C` and `+0x360` (all three must
  hold the bit, or only `+0x35C` when scene `+0x1F0` is 3), for the eight group
  names listed at `0x012B82BC`. The default of the masks is U.
* V: the sun and moon clips need no further curves. Positions follow the SunMoon
  formula above. The directional light colours come from the main and sub sky light
  containers (FCurve entries 1 and 0 of the `time_sky_00` SCB: Color six curves,
  Extra tx constant). The light directions are the light nodes, which the SunMoon
  clip moves on the same circle (I for the sign: `-DirLightDirections[i]` points
  to the body).
* Engine-side values for the rest: modulateColor, glareLdrScale, latitudeParam and the
  uvofs and fog constants stay at the engine table values.

## Unknowns

* The mechanism that follows the camera, if any, and the sky draw order and depth
  range.
* Far clip behaviour and the `common_clip_range` setting.
* The axis roles, multiplication order and handedness of the moon facing
  rotation in `0x008D3400`, the meaning of clip mode 4, and the handedness of
  the +X and +Z axes against the compass. The gates, the moon phase index and
  the roll are decoded in the [sun and moon orientation](sun-moon-orientation.md)
  page.
* The writer of the eight-stop block and the star `mtb` (`SpuBinary`).
* The default state of the meteor masks.
* Whether the weather scheduler changes the 10010 frame length.
