# Weather layout resource shape

Shapes and counts of the weather, sky and zone layout DATs that feed the
DrawEnv, Fog and Light objects. Static header and string scans of the
`fst_f0` family under `data/29/B0/00` in an install whose `ffxivgame.exe` has
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
No payload bytes, colour values or decompiled bodies are recorded. Tiers:
V = verified from file bytes, I = inferred, U = unknown.

This page owns the container, the class inventory and the object names. The
row tables that name these DATs are in
[Region weather resource rows](region-weather-resource-rows.md). The packet
to row to queue path is in
[Weather transition runtime](../net/weather-transition-runtime.md). The
neighboring pages own the rest:

- [Weather, sky and fog environment path](weather-sky-fog-environment-path.md)
  for row consumers, lane selection and the draw state.
- [Weather light curve container shape](weather-light-curve-container-shape.md)
  for the timelines and curve containers.
- [Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md)
  for light nodes and the draw-time light list.

## Container (V)

Each `wtr_*` child of a region root (type word 0, skipped by the layout
submission loop) names a DAT. Checked for `data/29/B0/00/0F.DAT` (`wtr_fine`,
129,856 bytes), `10.DAT` (131,904), `1E.DAT` (88,912) and `20.DAT` (118,768).
All four:

- start with the `MapLayoutResourceData` name and version `1.1.0`, the same
  container as the zone layout DATs, with a count at header `+0x24` (28, 36, 23
  and 38 respectively) and a resource reference table of `bxt`, `btm` and
  `fcss` type tags
- embed an SEDB `lyb` frame after the table (`0F.DAT`: frame at file offset
  `0x3C0`), the same frame family as zone layouts

## Class inventory (V)

Typed scans of `data/29/B0/00/0F..17.DAT` and `1A..20.DAT` give one identical
inventory for every weather layout:

| Class | Count | Node bytes |
|---|---|---|
| DrawEnvObject | 10 (`dwev_sky`, `dwev_cld`, `dwev_00 10 20 11 21 12 22 01`) | 0x3C |
| FogObject | 11 (`fog_default`, `fog_sky`, `fog_cld`, same 8 buckets) | 0x18, record 0x30 (12 dwords) |
| LightBaseObject | 29 (`main_lght`, `sub_lght` for sky, `main sub amb` for cld and each bucket) | 0x50 |
| LightSetObject | 10 (`ltst_sky ltst_cld ltst_<bucket>`) | 0x1C |
| ScreenEnvBaseObject | 9, each with Glare 9, Shadow 9, ColorCorrection 1, VolumetricLight 1, Blur 1 | 0x3C |
| EnvironmentMapDrawEnvObject | 14 or 15 (`envmap_<bucket>` plus time-sliced cube names) | 0x1C |
| CubeMapBaseObject, WindBaseObject | 1, 2 | 0x18, 0x28 |
| TimeLineBaseObject | 14 (`time_sky_00`, `time_wtr_00`, `time_dwev_se sky cld` and buckets, `time_sqnc_24`) | 0x28 |

A DrawEnvObject binds its FogObject, LightSetObject, EnvMap list and ScreenEnv
by node references (fields at `+0x18/+0x20`, `+0x24`, `+0x28`, `+0x30`, one
float at `+0x34`). The sky and cloud DrawEnvs bind a LightSet only. The
LaySettings draw-env count is 3 in each weather layout. A FogObject holds a
record reference at `+0x10`, and the 12-dword record is the same default
template on the three nodes read. A LightBaseObject holds no colour or
intensity reference, only a fixed per-object template, so the time-varying
values come from the timelines.

## Class names in the lyb frame

In `0F.DAT` (`wtr_fine`, V), class names present once unless a count is given: `DrawEnv`, `DrawEnvObject`,
`DrawEnvObjects` (3), `EnvironmentMapDrawEnv`, `FogObject`, `LightBaseObject`,
`LightSetObject`, `ScreenEnv`, `ScreenEnvObjects` (5), `ColorCorrection`,
`GlareObject`, `BlurObject`, `VolumetricLight`, `ShadowObject`, `TimeLine`,
`TimeLineBaseObject`, `CubeMapBaseObject`, `WindBaseObject`, `SEBaseObject`.
The counts of the plural names are string occurrences, not instance counts.
Instance counts are in the table above.

Clip class name occurrences: `LayRaptureDirectionalLightClip` 10,
`LayRaptureAmbientLightClip` 9, `LayRaptureFogParamClip` 8,
`LayRaptureScreenEnvGlareClip` 7, `LayRaptureSunMoonClip` 5, `LayEnvMapClip` 2,
`LayRaptureMaterial2Clip` 2, `LayRaptureVerticalFogClip` 1,
`LayRaptureCelestialSphereExClip` 1, `LayRaptureEnvSoundClip` 1 and
`LayRaptureExAlphaClip` 1. The directional and ambient counts match the 10
light-bearing timelines and the 9 that carry an ambient light, which suggests
one entry per timeline class list (I). Light key names are `MainLight_Key0/1`
and `SubLight_Key0/1`. Sky gradient names are `TopColor`, `MiddleColor`,
`BottomColor`, `Virtical_Key0..2` and `_Color`.

Object name pattern (group names, not values): `sgrp_dwev_NN` groups each
holding `main_lght_NN`, `sub_lght_NN`, `amb_lght_NN` and `time_dwev_NN`, with
`NN` in 00, 01, 10, 11, 12, 20, 21, 22, plus `sky` and `cld` variants. Matching
`dwev_NN` and `fog_NN` objects, a `fog_default`, a `time_sqnc_24_001`, an
`sgrp_wtr_24` and `sdef_outside|ind_day|night_loop|spot` sound definitions
complete the set.

RTTI for the clip classes is in `config/ffxivgame.rtti.json` under
`Application::Scene::Cut::Clip::`. Example vtable RVAs are `0xBBE09C` for
`LayRaptureAmbientLightClip` and `0xBBE154` for
`LayRaptureDirectionalLightClip`. The weather scene therefore reuses the cut
clip system.

## Sky layout (V)

Region keys that carry a type 1 row (`sky_celestial`) point at a separate sky
layout. For `fst_f0` it is `0E.DAT` (25,536 bytes, SHA-256
`d28e924b0863107258c46f868096e696324f8a528bdfdaf113639a747941ce88`):

- 10 `brt` models (`sky0_bg`, `sky0_sphe`, `sky0_star` x2, `sky0_sun`,
  `sky0_moon` x2, `sky0_indo`, meteor models) and 4 `bxt` textures
- 2 DrawEnv (`dwev_sky`, `dwev_00`), 9 Fog (`fog_default sky cld 00 01 0a 0b
  10 20`), 1 LightSet, 2 lights and 9 ScreenEnv
- 6 timelines (`time_sky`, `time_dwev_sky`, moon and meteor ones)
- no draw-env list in LaySettings

The sky is therefore a mesh layout (dome, stars, sun, moon) animated by
SunMoon and CelestialSphereEx clips, with no clear colour or gradient texture
row (I from the node inventory).

Sky row counts over the 61 roots of the region table: 47 roots have exactly
one `sky_celestial` row, 1 has six test sky rows and 13 have none. The 47 use
24 distinct sky resource keys, and roots with the same `+0x08` value share one
(roots 103, 106, 115, 122 and 123 share `0x29B0000E`).

## Zone layout draw state (V)

Zone layouts carry their own static draw objects and no `time_dwev` timeline,
only door, vfx, se and point-light timelines:

| Layout | DrawEnv | Fog | LightBase | LightSet | ScreenEnv | Other |
|---|---:|---:|---:|---:|---:|---|
| `fld01` (`03.DAT`) | 3 | 3 | 13 | 3 | 1 | |
| `twn01` (`01.DAT`) | 4 | 2 | 10 | 2 | 1 | |
| `ind01` | 5 | 1 | 33 | 2 | 1 | 1 CubeMap, 1 EnvMap |
| `air01` | 1 | 0 | - | - | - | |

Zone DrawEnv names reuse the weather bucket names (`dwev_00`, `10`, `01`,
`21`, `22`, `20`). The use of that overlap is in the lane selection section of
the [environment path](weather-sky-fog-environment-path.md#lane-selection-by-spatial-drawenv-name).
The zone light shapes are in
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#zone-layout-lights).

## Not established

- Field layouts of Fog and DrawEnv beyond the node shapes above.
- The meaning of the `NN` digits beyond the first-digit class (I: outdoor,
  dungeon, interior, see the environment path page).
- Which weather clips override which fields of the default templates.
