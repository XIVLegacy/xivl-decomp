# Transmission Tower rendering

This note separates the retail Castrum Novum Transmission Tower layout from
weather and reconstruction assets that can replace its initial render state.
Native addresses are VAs in the pinned FFXIV 1.23b executable,
`ffxivgame.exe`, PE32 i386, 15,996,808 bytes, with image base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Content and layout identity

Client tables join content 13, Castrum Novum Transmission Tower, to zones 251
and 264, zone parameter 5014, and layout 511. Region resource
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`
maps that layout to `lak_l0_dun01` at DAT key `0x03e7000b`.

The 22,512-byte layout at `data/03/E7/00/0B.DAT` has SHA-256
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
`sgrp_shutter`. Its serialized node GID 935 is a separate identifier.
The declared instance at file `0x2E10` references that UnitTree at
`0x2480` through instance `+0x3C`. Its direct child keys are unique.
Child `+0x14/+0x18/+0x1C` supplies the name, definition and key.

| Child key | File offset | Child name | Definition |
| --- | --- | --- | --- |
| 1 | `0x2620` | `l0f0_q1_shtr1_h_001` | BGParts `l0f0_q1_shtr1_h` at `0x4130` |
| 2 | `0x2650` | `open` | TimeLine `time_bg_gate_open` at `0x4800` |
| 3 | `0x2680` | `clos` | TimeLine `time_bg_gate_close` at `0x4828` |

The instance's authored translation is `(0, 0, 22.66200065612793)`.
The UnitTree and key-1 placement have zero translation. All three have
zero rotation and unit scale. These values describe the serialized placements.

### Authored transform and sound clips

TimeLine `+0x14/+0x18` gives the SCB reference and length. Both
`LayTransformClip` records select actor ordinal 0 at record `+3`. Its
`LayUnitMemberActor` record holds key 1 at `+0x20`. Original table cell
`0x0109F6DC` selects getter `0x00A78950` for that field. This joins both
clips to the same direct shutter member.

| Alias | SCB file range | Actor ordinal 0 | Transform record | Extent |
| --- | --- | --- | --- | ---: |
| `open` | `[0x4C50,0x5240)` | `0x4D20` | `0x4E60` | 100 |
| `clos` | `[0x5240,0x57F0)` | `0x5310` | `0x5470` | 25 |

Both transforms start at raw value 0. Original table cell `0x010A14FC`
selects `0x00A0DBC0`, which reads the end at record `+0x10`.
The ends equal their active block spans, 1,000,000 for open and 250,000
for close. Each block also contains one `LaySEClip`. The open sound span
is 10,000 to 300,000. The close span is 0 to 290,000 and exceeds its block
span by 40,000. Each scheduler also declares an empty block.

### Translation curves

Both transform records store mode 0 at `+0x14`, flags `0x0004` at
`+0x18`, curve ordinal 0 at `+0x1A`, and the signed extent at `+0x1C`.
The `@FCURVE` chunks start at file `0x4F10` and `0x5500`. The u16
offsets at `0x4F32` and `0x5522` are zero and select the first
descriptors at `0x4F40` and `0x5530`. The resolver at `0x00A220A0`
uses 16-byte units for these offsets. The MTB bodies at `0x4F60`
and `0x5550` are 518 and 454 bytes respectively.

Each body declares `TransForm` mask `0xE077`. The X and Z translation and
three rotation channels are type-0 zero constants. The three scale
channels are type-0 unit constants. The varying Y channel is a type-2
Hermite curve at file `0x5030` or `0x5620`. Its records contain four f32
lanes, in coordinate, value, incoming and outgoing order. Native
`0x00A2DF10`, `0x00A320B0` and `0x00A31F70` establish that ordering and
evaluation. Open has ten keys from `(0, 0)` to `(100, 4.5)`. Close has
six from `(0, 4.5)` to `(25, 0)`, including the negative key near the end
at coordinate 24. The installed curve supplies the intermediate values
and tangents.

`MoveRatio.tx` at file `0x5128` or `0x56D8` is type 4 with flag zero.
Its two keys range from `(0, 0)` to `(100, 1)` or `(25, 1)` respectively.
Native loader `0x00A2E0C0` and evaluator `0x00A33610` agree with the
serialized f32 value followed by a signed 16-bit coordinate.

### Conditional native application

For retained `LayTransformClip` table `0x010A14DC`, begin resolves the
curve descriptor at `0x00A0ED5A` and caches Y at clip `+0x40`. The
`0x00A0DC50` path takes local elapsed value L and signed duration d,
bounded below by 1. It samples MoveRatio at extent times L divided by d,
clamps the ratio to [0,1], and multiplies by the extent. The ideal sample
coordinate over either authored interval is L divided by 10,000.
Floating-point rounding and actual elapsed values govern delivered samples.

Update `0x00A0E6A0` calls the base update first at `0x00A0E6B5`.
Transform sampling then requires an operation context and clear flag
`0x02`. The selected flags admit X, Y and Z sampling, including Y at
`0x00A0E794`. Mode 0 stores the sampled vector at clip `+0x88/+0x8C/+0x90`.
An optional context `+0x48` hook at `0x00A0EC44` receives translation
and rotation by pointer. It precedes the context `+0x34` translation
call at `0x00A0EC5C`. The submitted translation is the unmodified
sample when the hook is absent or preserves that vector. The sampled
rotation and scale branches are skipped. On normal return, the update
still submits rotation through context `+0x3C` at `0x00A0EC76`. That
argument is the retained vector when the hook is absent or preserves it.
Clear flag `0x40` skips the scale dispatch.

The shared [clip delta path](ferry-door-schedulers.md#conditional-ordinary-clip-delta)
governs conditional update steps. These clips do not establish a Tower
wall-clock duration, first or last visible sample, context side effects,
simultaneous request arbitration, collision or audible output.

The offsets were recovered from a declaration-driven LYB and SCB walk
with the [world layout format](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/world-layout.md)
and the [SCB layout](cut-scheduler-clip-layout.md). A bounded Python
struct walk resolves the MTB descriptors. LLVM i386 disassembly and
original PE table reads establish the native paths.

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
`G`, `B`, and `A` to its stored name and sends each sampled value to
every material entry in the active container. `Attr` is therefore a four-
channel RGBA material-parameter broadcast, not a Tower point-light control.
The [Lay subclass finding](uldah-halloween-material.md#selected-record-and-inherited-consumption)
locates the stored-name input path and distinguishes the channel-selector
names from those output suffixes; its selected serialized values remain unjoined.

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
