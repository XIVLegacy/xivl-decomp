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
  `0x00C1F6A0` and stores the result at C `+0x32C..0x358`. The fog colour
  (C `+0x32C..0x338`) goes to C `+0x30C` through `0x00C48A10`, and the fog
  vector (C `+0x33C..0x344`) to C `+0x31C` through `0x00C48390`. The enable byte
  C `+0x4E4` is a separate store (`0x00C483C0`).
- PixelLightShaderDriver Init `0x00C21120` resolves `latitudeParam`, `fogColor`,
  `ambientLightColor`, `lightCount`, the `light<N>bit<M>` masks,
  `EnableShadowFlag`, DirLight/PointLight arrays (or `lightColors` family),
  `vfogRange/Top/Mid/BottomColor`, `modulateColor`, `glareLdrScale`,
  `exAlphaParam`. The Apply at `0x00C22DD0` writes:
  - `latitudeParam` = 4 floats: scale = 1 / (C `+0x1BC` - C `+0x1B8`), an offset
    from the constant at `0x00F62FA0` minus C `+0x1B8` times scale, C `+0x1B4`,
    and the constant at `0x00FB7A60`.
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

Each frame `0x007DCDF0` runs a position query (`0x007DB9C0`, a short segment
test at the camera or player position) that returns two zone layout objects,
A in output slot 0 and B in slot 1. The two class ids tested at the end of the
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
- A becomes the holder's current DrawEnv (`+0x1C`, previous in `+0x18`). Its
  name (vtable `+0x08`) is then looked up with `0x007E4630`, an exact name
  `std::map` find on the manager at `+0x38`. Node `+0x64` of the hit holds the
  blended work-set Weather::DrawEnv that `0x007E7C50` and `0x008CB370` build
  per weather DrawEnv name. When the key exists the holder's `+0x1C` is
  replaced by that work-set DrawEnv, otherwise A stays.
- The two layouts being blended (`0x007E4CA0`, `0x007E4690`) are matched by the
  same exact names, so a lane is a DrawEnv name (`dwev_00 01 10 11 12 20 21 22`
  in the weather layouts).

So the lane is chosen by exact name equality between the zone layout object
that contains the position and a DrawEnv of the weather layout. The prefixes
`dwev_0`, `dwev_1`, `dwev_2` only classify transitions (`0x007E7480`), the
outdoor flag of `0x007E6460` and a fog parameter pass over the `dwev_1` and
`dwev_2` entries in `0x007E49E0` (I for its purpose). No default lane name is
read anywhere. When the query returns nothing, `+0x1C` keeps its previous value
(U for the state before the first hit). The literal `dwev_21` in `0x007E4690`
feeds a compare whose result is unused (U). What the replaced DrawEnv does to
the zone's own lights is in
[Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md#zone-layout-lights).

## Unknowns

- Field meaning inside the 12-dword fog record and the 0x50 light node beyond
  the shapes in the resource page, and which values the time clips override.
- The clear colour producer and default.
- The default state of the holder's current DrawEnv before the first query hit.
- The sky gradient producer (FCurve payload) and how SunMoon clips place the
  sun and moon from seconds-of-day.
- Whether the three-entry LaySettings draw-env list in weather layouts is the
  default or a spatial fallback.
