# Weather, sky and fog environment path

How the pinned 1.23b client turns the type 0 (weather) and type 1 (sky) rows of
a RegionInfo into sky, fog and light state, and how a zone position picks the
DrawEnv lane that is drawn. Static read of `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`) and the install. All addresses are VAs. No retail values are
recorded, only addresses, counts and shapes. Tiers: V = verified from cited
instructions or file bytes, I = inferred, U = unknown. Offsets in the DrawEnv
sections come from decompiled listings of the named functions, and the
accessors are anchored by their assertion strings.

Owners of the neighboring facts:

- Row framing is in [RegionInfo and LayoutInfo selection](map-layout-selector.md)
  and the row tables in [Region weather resource rows](region-weather-resource-rows.md).
- The packet path, the id to key lookups, the weather queue, the fraction
  and the mode-one Sky registration are in
  [Weather transition runtime](../net/weather-transition-runtime.md).
- The DAT inventory is in
  [Weather layout resource shape](weather-drawenv-resource-shape.md).
- Light nodes and the draw-time list are in
  [Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md).
- Curve containers are in
  [Weather light curve container shape](weather-light-curve-container-shape.md).

## Rows are consumed by WeatherManager, not by the layout submit loop

The submit loop at `0x0064E830` skips full-dword types 0 and 1. The weather
and sky rows are ordinary MapLayoutResourceData layouts created through
LayoutWorld and owned by WeatherManager. The runtime page owns each step of
that path. These points add to it:

- Fallback rules (V, instructions at `0x0079B190` and `0x0079B050`). Both walk
  the region's type 0 children and keep the first child whose value is nonzero
  as the fallback. The id walk (`0x0079B190`) compares child `+0x58` with the
  request and falls back to the first nonzero child id. The key walk
  (`0x0079B050`) returns child `+0x60` of the id match and otherwise of the
  first type 0 child, whose key is nonzero. The sky walk (`0x0079B220`) returns the
  key of the first type 1 child. The runtime page states the same id
  rule as the first nonzero type-zero child key.
- The first-nonzero rule gives these fallback ids over the 61 roots of the
  pinned region table: 8001 in 45 roots, 8002 in 1, 8010 in 2 and none in 13.
  Two roots have a first type 0 child with id 0.
- Queue bound (V, `0x007E80F0`). When two weather entries are already queued
  the register helper erases one before appending, so the queue holds two. An
  assertion `size <= 2` in `WeatherManagerLayout.cpp` guards the result. The
  runtime page documents the 12-byte entries and the elapsed reset.
- Region set callers (V, cross-references to `0x00624180`): `0x0062AB54` (region
  manager build path), `0x00626E1E`, `0x008182FC` and the dispatcher arm at
  `0x0062944A`. A zone-in with no SetWeather yet therefore reaches the first
  type 0 row through the build callers (I for the ordering).
- The sky row is also found by name. `0x0062AEB0` looks up the layout named
  `sky_celestial` (string `0x00FB9924`) and shows or hides its eight
  `sgrp_meteor` groups from the masks at scene `+0x358..+0x360` (scene state
  3 uses `+0x35C` alone).

## Time keying

`0x007E1600` takes the 64-bit Eorzea seconds, rolls a random value on the hour
8 transition, and calls `0x0080B5E0` with `ET mod 0x15180` (86400) as a float.
`0x0080B5E0` iterates a child vector (`+0x38..+0x3C` of its receiver) and calls
`0x00809E90` with the float per child. This is the seconds-of-day feed. The
receiver object is not resolved here (I: the sky and light children of the
weather object). The clock sources and the timeline position rule are in
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#time-of-day).
Each weather layout holds its time curves in embedded SCB blobs named
`time_dwev_<bucket>`, one per bucket. The clip classes found in them are listed
in [Weather layout resource shape](weather-drawenv-resource-shape.md#class-names-in-the-lyb-frame).
The server-supplied weather id and the region are the inputs to the choice of
layout.

## From DrawEnv to the pixel programs

- `0x007E78D0` (Leap) blends the oldest and newest queued weather DrawEnv by the
  fraction in [Weather transition runtime](../net/weather-transition-runtime.md).
  `0x007E6460` also reads the current area DrawEnv name (compared against the
  prefixes `dwev_0`, `dwev_1`, `dwev_2` in `0x007E49E0`) and blends fog
  parameters with `DrawEnv::GetFogParameter` (`0x00C203B0`). I: the first digit
  of the bucket name is an area class (outdoor, dungeon, interior), the second
  a variant.
- Renderer `DrawEnv` stores a fog parameter count at `+0x08` (max 7, asserts
  `num >= 0 && num < MAX_NUMBER_OF_FOG_PARAM` in `SetFogParameterCount`
  `0x00C201D0`), fog entries of 0x30 bytes from `+0x1C` (`GetFogParameter`,
  assert `no >= 0 && no < fog_.fogParamCount`) and env-map textures at `+0x35C`
  (32 slots, see
  [DrawEnv texture-binding boundary](draw-env-texture-binding-boundary.md)).
- `DrawScheduler::SetDrawEnvParameter` (`0x00C58310`) stores the node `+0xF8`
  value at draw context C `+0xC3C`. When the node `+0xF4` pointer (or the
  scheduler `+0x10` pointer when that is null) differs from the cached one,
  it stores it at C `+0xC38`, stores the result of `0x00C1F3E0` at C `+0x788`
  and calls `0x00C20050`. That function blends the fog entries through
  `0x00C1F6A0` and stores the result at DrawEnv `+0x32C..0x35C`. The fog colour
  (DrawEnv `+0x32C..0x338`) goes to C `+0x30C` through `0x00C48A10`, and the fog
  vector (DrawEnv `+0x33C..0x344`) to C `+0x31C` through `0x00C48390`. The
  entry fields, the layer blend and the shader side are in
  [Weather fog parameters and fog mix](weather-fog-parameters.md). The enable byte
  C `+0x4E4` is a separate store (`0x00C483C0`).
- PixelLightShaderDriver Init `0x00C21120` resolves `latitudeParam`, `fogColor`,
  `ambientLightColor`, `lightCount`, the `light<N>bit<M>` masks,
  `EnableShadowFlag`, DirLight/PointLight arrays (or `lightColors` family),
  `vfogRange/Top/Mid/BottomColor`, `modulateColor`, `glareLdrScale`,
  `exAlphaParam`. The Apply at `0x00C22DD0` writes:
  - `latitudeParam` = 4 floats: scale = 1 / (C `+0x1BC` - C `+0x1B8`), an offset
    from the constant at `0x00F62FA0` minus C `+0x1B8` times scale, C `+0x1B4`,
    and the constant at `0x00FB7A60`. The sources of C `+0x1B4..+0x1C0` and the
    clamp on C byte `+0x77C` are in
    [ScreenEnv Glare clip and the pixel tail tone constants](screen-env-glare-latitude.md).
    The 0.75 in z is divided out by the Glare composite, see the
    [post-effect composite section](screen-env-glare-latitude.md#post-effect-composite).
  - `fogColor` = C `+0x30C` rgb, alpha plus C `+0x4F8` clamped to 1 (`0x00C48A30`),
    only when C byte `+0x4E4` or `+0x4E5` is set. The `+0x4E4` setter is
    `0x00C483C0`, called per pass from `0x00C3D5D0`, `0x00C3D340`, `0x00C439C0`,
    `0x00C43FD0`. The `+0x4E5` setter `0x00C483E0` is called from `0x00A098F0`.
  - `modulateColor` = C `+0xBE4` (4 floats) times C `+0xBF4` (4 floats).
  - `glareLdrScale` = 1 / C `+0x1C0`.
  - `exAlphaParam` = 3 floats from C `+0xC24`. The `vfog*` constants are written
    in the same Apply when C byte `+0xBA0` is set and C `+0xC38` is nonnull.
  - The light slots and the ambient colour are in
    [Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#draw-time-light-list-and-rules).
- `SkyColorShaderDriver::Init` (`0xC66750`) resolves the sky material constants
  `gradationAmbient` or `verticalColorStart/End/Key/Key2` with
  `verticalColor1..8`, and `lightColorStart/End/Key/Key2` with `lightColor1..8`
  and `lightColorBlend`. The sky dome is therefore a vertical eight-stop colour
  gradient shader on the sky mesh. I: the stops are animated by the
  CelestialSphereEx clip (`0x008C8B50`, which resolves an `fcr` animation node
  container), not stored in the layout rows.

## Clear colour

A D3D9 `ClearTask::Process` at `0x00435B50` calls device `Clear` with flags
(task `+4`), colour (`+8`), z (`+0x10`) and stencil (`+0xC`). A config key
`view_clear_color` is registered in the table builder `0x004B8640` (string
`0x00F90378`, pushed at `0x004BA1AC`). No link from the fog colour (C `+0x30C`)
or the weather layouts to the clear colour was found. The clear colour source
for the play view is U.

## Lane selection by spatial DrawEnv name

`0x007DCDF0` runs a position query (`0x007DB9C0`, a short segment test at the
[followed position](#followed-position-and-direction)) that returns two
objects, A in output slot 0 and B in slot 1. The two class ids tested at the end of the
query are `0xD` and `0xB` (I for what they mean).

- B: its name (object vtable `+0xA4`) is passed with the previous name to
  `0x007E7480` and then copied into a 16 byte buffer at the holder `+0xC0`.
  Manager `+0x1A4` is set from "name lacks `dwev_0`" (the indoor flag).
- `0x007E7480(new, old)` sets WeatherManager `+0xE4` to 2 when the new name
  contains `dwev_0` and the old one contains `dwev_1` or `dwev_2` (entering
  outdoors), to 1 for the reverse (entering indoors) and to 0 otherwise. The
  60 frame spatial fade at `+0x8C` is the separate counter in the
  [runtime page](../net/weather-transition-runtime.md#independent-spatial-fade).
  What `+0xE4` drives next is I.
- A becomes the holder's current object at `+0x1C` (previous in `+0x18`).
  I: it is a ScreenEnv, see
  [Position query](#position-query-0x007db9c0). Its name (vtable `+0x08`) is
  then looked up with `0x007E4630`, an exact name `std::map` find on the
  manager at `+0x38`, which returns node `+0x64` of the hit. When the key
  exists the holder's `+0x1C` is replaced by that value, otherwise A stays.
  I: `0x007E7C50` and `0x008CB370` build the map's blend work set per weather
  object name. Which object names key this map was not traced.
- The two layouts being blended (`0x007E4CA0`, `0x007E4690`) are matched by the
  same exact names, so a lane is a DrawEnv name (`dwev_00 01 10 11 12 20 21 22`
  in the weather layouts).

So the lane name is the name of B, the layout object selected at the queried
position, and it is matched by exact name equality against the weather
layout. A collision triangle does not spell that name. It carries an index
that the owning layout maps to one of its DrawEnv objects, see
[DrawEnv index lookup](#drawenv-index-lookup). The prefixes
`dwev_0`, `dwev_1`, `dwev_2` only classify transitions (`0x007E7480`), the
outdoor flag of `0x007E6460` and a fog parameter pass over the `dwev_1` and
`dwev_2` entries in `0x007E49E0` (I for its purpose, call structure in
[Weather fog parameters and fog mix](weather-fog-parameters.md#weather-blend-v-call-structure-i-fraction)). No default lane name is
read anywhere. When the query returns nothing, `+0x1C` keeps its previous value
(U for the state before the first hit). The literal `dwev_21` in `0x007E4690`
feeds a compare whose result is unused (U). What the replaced DrawEnv does to
the zone's own lights is in
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#zone-layout-lights).

### Position query `0x007DB9C0`

V. `RaptureScreenEnvController` slot 7 (`0x007DD4C0`, vtable `0x00FEED84`)
calls `0x007DCDF0` at `0x007DD4DA`, which is the only caller of
`0x007DB9C0` (`0x007DCE28`). The output is two dwords on the caller's
stack, slot 0 (A) and slot 1 (B). The holder is the controller in ECX.

- The segment starts at holder `+0x40`, or at `+0x70` when bit `0x10` of
  byte `+0x86` is set (`0x007DBA81..0x007DBA8D`), plus 1.0 in y
  (`0x00F54F70`), w 1.0. It ends at that start plus holder `+0x50` times
  10.0 (`0x00FA95A0`), w 1.0 (`0x007DBA90..0x007DBAF8`). The query word
  is `0x14002` (`0x007DBAFD`) and the query is `0x00A95240` (`0x007DBB29`).
- A hit (the dword read at `0x007DBB2E`) gives a category through the hit
  object's virtual `+0x9C`, then `+0x4C`, then `+4`.
- Category `0xB` (`0x007DBBDE`) calls
  `0x007D4130(hit, holder + 0x90, 0, &local)` into B
  (`0x007DBBE3..0x007DBBFE`). A nonzero B goes through
  `0x007D3A90(B, holder + 0x8C)` into B (`0x007DBC03..0x007DBC15`). When
  the object that B's virtual `+4` returns holds a nonzero `+0x30`, A is
  the virtual `+0x68(0xD, [+0x30] + 8)` of the object B's virtual `+8`
  returns (`0x007DBC1A..0x007DBC43`).
- Category `0xD` (`0x007DBB66`) runs the query again with `0x10002`
  (`0x007DBB75`). A category `0xB` hit of that query goes through
  `0x007D4130` into B (`0x007DBBB1..0x007DBBCA`). A is then the first
  hit object's virtual `+0x4C` (`0x007DBBCD..0x007DBBDA`), with no
  weather swap.

`0x007D4130` takes the shape at hit `+0x2C`. Shape virtual `+0x4C` is the
data owner, and shape virtual `+0x18` minus 4 is the owner O. A null shape
or O, a zero shape virtual `+0xBC`, or a missing or short
`RaptureAttribute` table returns 0. Otherwise the u16 word W of the hit
triangle, as on the
[ground material](footstep-detector-and-ground-material.md#ground-material)
page, gives `(W >> 7) & 0x1F` to O `+4` virtual `+0x198`. O `+4` virtual
`+0x1A0` returns a container, and when both results are nonzero the
container's virtual `+0x10` with the first result is the return value
(`0x007D4244..0x007D4280`). So bits 7 to 11 of the triangle word select
the object that becomes B.

`0x007D3A90(object, holder + 0x8C)` takes the virtual `+0x13C` of its second
argument and that result's `+0x78`. When `+0x19C` or `+0x1A4` of that is
nonzero and `+0x158` is an object for which `0x007E0360` is true, it looks
the object's name (virtual `+0xA4`) up with `0x007E45B0` on `+0x158`. A
found object replaces the argument, else the argument is returned
(`0x007D3AD1..0x007D3B5A`).

`0x007E7480` is called only from `0x007DCDF0` (`0x007DCE8C`). Its store of
0.0 to WeatherManager `+0x8C` at `0x007E75F0` runs only when `+0xE4` is
nonzero and the find `0x007EB280` on `+0x20` with the name at `+0x90`
returns an entry (`0x007E759A..0x007E75E8`).

I. The `0xB` hit is an Attribute hull of the zone collision
([selected instance admission](attribute-resource-and-scene-binding.md#selected-instance-admission)),
B is the DrawEnv object the triangle's index selects, and `0x007D3A90` swaps
it for the weather layout's object of the same name. Category `0xD` is U.

I. A is a ScreenEnv. `0x007DB9C0` and `0x007DB940` pass type `0xD` and the
name of the node that B's data record references at `+0x30`. In the weather
layout `0x29D9000F` that reference of lane DrawEnv `dwev_NN` names the
`ScreenEnvBaseObject` `scev_NN` (V, file). The three DrawEnv nodes of each
zone layout `0x29D90001` and `0x29D90002` hold a null reference there (V,
file), so A comes from a swapped object for those layouts. The type id
`0xD` to class mapping is not traced. The holder is
`RaptureScreenEnvController`, and `0x007DD3F0` passes `+0x18` and `+0x1C` to
`0x00A73BE0`, which reads them through their virtuals.

V. `0x007DCDF0` falls back to `0x007DB940` (`0x007DCE53`) when the first
query leaves B null, `0x00623C60` returns nonzero and
`0x00624D10(0xB, 0)` returns zero. That routine queries by position through
`0x00629D80`, which calls `0x00624AC0` with kind 2 and returns the first
nonzero `0x00627610` result. What those volumes are is U. When B is still
null the name and the indoor flag keep their values. When bit 0 of byte
`+0x86` is set, slot 7 runs `0x007DCDF0` if byte `+0x81` is zero and then reloads that byte from
`+0x80`, else it decrements it (`0x007DD4D0..0x007DD4ED`).

### Followed position and direction

V. The constructor `0x007DA6D0` stores (0, 0, 0, 1) at holder `+0x40` and
`+0x70`, stores (0, -1, 0, 0) at `+0x50` with the float -1.0 at
`0x00F62F80`, and clears bit `0x10` of byte `+0x86`. `0x0061B5A0` creates the
controller and keeps it at `+0x470` of its object S (`0x0061B91E`).

V. `0x007D9D70` copies its argument to holder `+0x40`. Its one direct caller
is `0x006168F0` (`0x0061695B`), which then calls the holder's virtual `+0x1C`,
slot 7. `0x006195D0` calls `0x006168F0` at `0x00619B42`. `0x006168F0` picks
the position:

1. It takes the object that `0x00616310` finds for the dword at S `+0x440`
   when that dword is not `0xC0000000`. Otherwise, or when the find returns
   null, it takes the pointer at `[S+0x118]+0xD0`.
2. When that object exists and bit 0 of byte S `+0x448` is clear, the
   position is the result of the object's virtual `+0x88`.
3. Otherwise it is the 16 bytes at S `+0x3A0`.

The 16 bytes at S `+0x3A0` are the last row of the 64 byte matrix that
`0x006195D0` stores at S `+0x370..+0x3AF` earlier in the same call, built from
the object that `[S+0x114]` virtual `+0x0C` returns
(`0x00619AA4..0x00619B0D`). I: the dword at `+0x440` is an actor id, the two
objects are actors, and S `+0x3A0` is the camera position. Which actor
`[S+0x118]+0xD0` names and who sets bit 0 of S `+0x448` are U. The setter
`0x007D9DA0` writes holder `+0x50` and clears its w lane, and no direct
cross-reference to it was found. I: the query direction therefore stays
straight down. Writers of `+0x70` after construction and a setter of bit
`0x10` of `+0x86` are U.

### DrawEnv index lookup

`0x007D4130` takes the owner O of the hit shape and calls three virtuals on
`O+4`. The slot `+0x198` target `0x00A8AFF0` appears at that slot of the
`LayoutManager`, `RaptureLayoutManager`, `MapLayoutWeather`,
`MapLayoutWeatherBase` and `MapLayoutWeatherSky` vftables (`0x010A22A4`,
`0x00FBFCEC`, `0x0103CEC4`, `0x0103C9B4`, `0x0103CC3C`). The first, second and
third have complete-object offset 4 in their RTTI locators. I: O is the layout
manager of the layout that owns the hull.

V for that vftable family:

| Slot | Target | Result |
| --- | --- | --- |
| `+0x18C` | `0x00A8AFB0` | Dword at `[M+0x7C]+0x58`, the entry count |
| `+0x190` | `0x00A8AFC0` | First dword of 8 byte entry i of the array at `[M+0x7C]+0x54` |
| `+0x194` | `0x00A8AFD0` | Second dword of entry i |
| `+0x198` | `0x00A8AFF0` | The second dword of the first entry whose first dword equals the argument, or zero |
| `+0x1A0` | `0x00657680` | Dword at `M+0xCC` |

M is the interface pointer in ECX. `0x00A8E303..0x00A8E326` stores a node at
`+0x7C` of its receiver after a class test against the path at pointer
`0x012F594C` (`MiscObjects/LaySettings/LaySettingsObject`, `0x010A2714`). I:
that receiver is M, so `[M+0x7C]` is the layout's `LaySettingsObject` node.
The file shape of `+0x54` and `+0x58` below agrees. I: `M+0xCC` is the
layout's DrawEnv container. Slot `+0x10` of the
`LayoutDrawEnvContainer` vftable `0x0109EC50` (`0x00A75390`) is a map find
that returns zero on a miss.

So the triangle index is a search key in the owning layout's own table. The
value is a reference to a DrawEnv node, and the name of B is that node's
name. An index with no entry gives no B.

V (file, structure-only read of the 287 installed `MapLayoutResourceData`
files). `LaySettingsObject` `+0x54` is a reference to a run of 8 byte entries
and `+0x58` is the count. The run's node type declares one reference
descriptor, at `+4`. 258 layouts hold a nonempty table and 29 hold none.
All 649 entries reference a `DrawEnvObjects/DrawEnv/DrawEnvObject` node.
The keys are 0, 1, 10, 11, 13, 14, 20, 21 and 22. In 78 entries of 61
layouts the referenced name is not `dwev_` followed by the two digit key. 14
layouts repeat a key, where the lookup returns the first entry.

Region 101 (`sea_s0`), non-weather children with a key 20 entry:

| Layout | Name | Table, key to DrawEnv |
| --- | --- | --- |
| `0x29D90001` | `sea_s0_twn01` | 0 `dwev_00`, 1 `dwev_21`, 20 `dwev_21`, 10 `dwev_10` |
| `0x29D90002` | `sea_s0_ind01` | 0 `dwev_00`, 1 `dwev_21`, 20 `dwev_21`, 22 `dwev_22` |
| `0x29D90017` | `sea_s0_lin01` | 0 `dwev_00`, 20 `dwev_22`, 1 `dwev_01` |

The region's other non-weather children have no key 20 entry. Its weather
children map keys 0, 10 and 20 to `dwev_00`, `dwev_10` and `dwev_20`.
So an index 20 triangle owned by one of the three layouts above names lane
`21` or `22`, never lane `20` (I, with the owner reading above). Their
SHA-256 values are `bc40d7ff57ab8998978532cce4564e2488ed81d6d10b8f7ede3e1586b0b26523`
for `0x29D90001` (1197104 bytes) and
`c6d86bfea2692e612464e7e08f6604df4f26fc1ea99da16727e824c920a693d8`
for `0x29D90002` (728976 bytes). In `0x29D90001` the table starts at file
offset `0x42C5C`, and in `0x29D90002` at `0x2E61C`.

Reproduce with `ghidra x-ref to 0x007db9c0`, `ghidra x-ref to 0x007dcdf0`,
`ghidra x-ref to 0x007e7480`, `ghidra disasm 0x007db9c0 -n 200`,
`ghidra disasm 0x007d4130 -n 100`, `ghidra disasm 0x007d3a90 -n 60`,
`ghidra disasm 0x007e7480 -n 110`, `ghidra disasm 0x00a8af90 -n 40`,
`ghidra disasm 0x00a8e2e0 -n 40`, `ghidra decompile 0x007da6d0`,
`ghidra disasm 0x006168f0 -n 40` and `ghidra disasm 0x00619ac0 -n 40`, and
read the cell `0x00FEEDA0` and the floats `0x00F54F70`, `0x00F62F80` and
`0x00FA95A0`. For the file census, follow the object list of each `lyb`
payload to its LaySettings node
(`xivl-tools:docs/formats/world-layout.md`) and read the two dwords at
`+0x54` and `+0x58`.

## Unknowns

- Field meaning inside the 0x50 light node beyond the shapes in the resource
  page, and which values the time clips override. The fog entry fields are in
  [Weather fog parameters and fog mix](weather-fog-parameters.md).
- The clear colour producer and default.
- The default state of the holder's `+0x1C` object before the first query hit.
- Which DrawEnv a zone BG draw node holds at node `+0xF4`, or takes from the
  scheduler default at `+0x10`
  ([node selection](chara-engine-samplers-and-light-inputs.md#same-machinery-for-actors-and-terrain-v-unless-marked)).
  The followed position selects the controller's environment and the indoor
  flag. This page does not show that it selects the fog of every zone draw.
- The sky gradient producer (FCurve payload) and how SunMoon clips place the
  sun and moon from seconds-of-day.
- Whether the three-entry LaySettings draw-env list in weather layouts is the
  default or a spatial fallback.
