# Transmission Tower rendering

This note separates the retail Castrum Novum Transmission Tower layout from
weather and reconstruction assets that can replace its initial render state.
Native addresses refer to the pinned FFXIV 1.23b executable with image base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Content and layout identity

Client tables join content 13, Castrum Novum Transmission Tower, to zones 251
and 264, zone parameter 5014, and layout 511. Region resource
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`
maps that layout to `lak_l0_dun01` at DAT key `0x03e7000b`.

The 22,512-byte layout DAT has SHA-256
`b7598ff9aedc1af4cda3cefb3372cc83e49e51445b2eb04ba77905d250827088`.
It contains 67 placed instances across four divide-map folders, 12 nested
unit-tree members, two draw-environment objects named `dwev_20` and `dwev_00`,
five authored lights, one screen-environment graph, and two compiled
timelines.

Both DrawEnv records set feature-mask bit 2 and its conditional flag. Native
getters at `0x00a81710`, `0x00a81730`, and `0x00a81740` return the enabled
conditional value, an otherwise unlabeled scalar of 500.0, and mode 0. The
binary does not provide safe retail names for the scalar or mode, so those
fields remain semantically unresolved.

## Authored lights and screen environment

Native light property tables identify serialized byte `+0x44` as LightType.
Named comparison resources encode directional, point, and ambient lights as
0, 1, and 3. All five Tower lights encode 1 and are therefore point lights.

| Light | RGB | Intensity | Radius value |
| --- | --- | ---: | ---: |
| `lght_0001` | `#a8fdf0` | 1.0 | 40.0 |
| `lght_0002` | `#fc1603` | 1.0 | 6.0 |
| `lght_0003` | `#389afc` | 3.0 | 20.0 |
| `lght_0004` | `#fda964` | 1.0 | 21.2 |
| `lght_0005` | `#fff9ca` | 1.5 | 11.0 |

Each light has zero decay and a cone-angle value of pi/6. The same radius
candidate is serialized twice; only one position has a recovered native
label, so the second is not independently named here.

The `scev_0001` graph links `glare_0001`, `shadow_0001`,
`ColorCorrection_0001`, `Blur_0001`, and `VolumetricLight_0001`. Their
parameter banks are present, but all five native enabled bits are false in the
retail Tower layout.

The complete timeline inventory is only `time_bg_gate_open` and
`time_bg_gate_close`; it contains no `LayRapturePointLightClip`. A comparison
dungeon layout does contain explicit point-light timelines, so the absence is
structural rather than a decoder blind spot. The Tower's five point lights are
authored layout state, not a light animation bank controlled by its shutter
scheduler.

## Shutter contract

Layout instance `isgrp_000012` binds packet instance ID 12 to unit tree
`sgrp_shutter`. Its serialized node GID 935 is a different identifier. The
unit tree exposes `open` and `clos`.

`time_bg_gate_open` has a raw SCB active-block span of 1,000,000 units. Its
transform starts at raw value 0 and its sound at 10,000. The close block has
a raw span of 250,000, with sound and transform both starting at 0. No
client SCB timebase is established here. These resources describe the
callable shutter presentation; they do not define the server condition that
opens it.

## Weather and material overrides

Retail Mor Dhona resource 8032, `wtr_xmas`, is labeled Dalamud Thunder and
has DAT SHA-256
`4f462d9ddaf1917b90d519973c42d3ccbd5358930eb5dbfb92b00f09419800bc`.
It adds six thunder dependencies over resource 8030 while retaining the same
FCurve property structure and values. Recovered client Lua contains no static
content-13 to weather-8032 assignment. Resource locality and the thunder
payload make it a comparison candidate, not proof that the duty selected it.

The pinned executable's WeatherManager registration at `0x007e7480`
compares names against `dwev_0`, `dwev_1`, and `dwev_2` family substrings
(literals at `0x00fef4c0`-`0x00fef4e8`). Its caller at
`0x007dce65`-`0x007dce8c` detects a changed spatial-object pointer, obtains
that object's name through vtable offset `0xa4`, and passes the name into
registration. This is a name-family classifier and a selected-object path,
not a static choice of the Tower's `dwev_20` or `dwev_00`. The selected
spatial object and active exact suffix remain unobserved historically.

The native `RaptureMaterial2Clip` implementation gives an independent ABI for
weather material overrides. Initialization at `0x00835250` resolves an
`Attr` FCurve and four channels. Application at `0x00835470` appends `R`,
`G`, `B`, and `A` to the controlled actor name and sends each sampled value to
every material entry in the active container. `Attr` is therefore a four-
channel RGBA material-parameter broadcast, not a Tower point-light control.

A non-retail weather payload previously compared during reconstruction,
resource 8076 with SHA-256
`6a6c23836b0d6c5955eb0078bf7b23ca3249dde20857249f151c393886c36900`,
inherits all 2,565 FCurve records from its restored Starlight source. Its 119
changed bytes only zero five dependency entries. Its `time_dwev_00` lane adds
`diffuseColor_00` with `Attr=(0,0,0,0)` and `lightMapOcclusi` with
`Attr=(1,1,1,1)` relative to retail clear weather 8001. Those values explain
what the custom payload commands; they do not identify the retail Tower
weather.

## Evidence boundary

The retail artifacts prove layout identity, initial layout components, light
types and parameters, shutter ownership and timing, the DrawEnv name-family
classifier, and the Material2 ABI.
They do not prove the duty's historical weather ID, exact active DrawEnv lane,
server weather bootstrap, or the final enabled state after a weather blend.
WeatherManager behavior is documented separately in
[Weather transition runtime](../net/weather-transition-runtime.md). A custom
payload or a forced weather result must not be promoted into the original
retail assignment.
