# Chara engine samplers and light inputs

How a character material pixel program gets its two environment cube
samplers and its light constants from the engine, and where the engine side
of that data lives. Static read of `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, 15996808
bytes, PE32 image base `0x00400000`, all locators VA) plus a structure-only
read of the installed `client/chara` tree and `data/29/B0/00/0F.DAT`. No
retail bytes are recorded. Only addresses, counts, register numbers and
shapes. Tiers: V = verified from cited instructions or file bytes,
I = inferred, U = unknown.

Instruments: Ghidra 12.1.4 through ghidra-cli (`disasm`, `x-ref`, `strings`,
`memory read`) over the executable, and a local ps_3_0 token decoder run on
the embedded shader bytes. Offsets of vtable slots are byte offsets from the
RTTI vftable listed in `config/ffxivgame.rtti.json`.

Neighbouring owners:

- Pixel formulas for the PRAM families, and the non-PRAM constant list, are in
  [Character material pixel shaders and colour flow](chara-material-shader-colour-flow.md).
- The light node, the draw-time light list and the ambient accumulation are in
  [Zone lighting source and time of day](zone-lighting-source-and-time-of-day.md).
- The DrawEnv fog path and the lane selection are in
  [Weather, sky and fog environment path](weather-sky-fog-environment-path.md).
- The call chain into `SetEnvmapTexture` is in
  [DrawEnv texture-binding boundary](draw-env-texture-binding-boundary.md).
- The weather layout object inventory is in
  [Weather layout resource shape](weather-drawenv-resource-shape.md).

## Summary

- The two cube samplers of a character material, `reflectMap` and
  `lightDiffuseMap`, are bound by a flag bit and a slot byte in the PRAM
  sampler record, not by name. Both records read slot 0 across the whole
  `chara` corpus, so both samplers bind the same cube, slot 0 of the draw's
  DrawEnv (V).
- That cube is a layout file resource (`CubeMapBaseObject`). The only slot
  writers that do not copy another DrawEnv's pointer are the layout loops and
  the weather pool assignment (V).
- The per-node ambient block that seeds the draw ambient colour is the 16
  bytes at node `+0x134`, seeded to (0, 0, 0, 1.0) and summed with the node's
  type 4 lights (V).

## Pixel program inputs (V)

Program `mtc001e216pc_a_000.fpo`, the ps_3_0 program of the c001 equipment
material `e216` (`pch`), at file offset 3520 of
`client/chara/pc/c001/equ/e216/pch_mdl/0001`. The PRAM block of that file
starts at file offset 13760. The `_001` sibling at file offset 9312 is not
decoded here.

CTAB register map for the light and environment inputs:

| Name | Register | Note |
|---|---|---|
| `reflectMap` | `s0` | cube sampler (declared texture type 3) |
| `lightDiffuseMap` | `s1` | cube sampler (declared texture type 3) |
| `_sampler_00 _01 _02` | `s2 s3 s4` | 2D |
| `shadowMap0` | `s10` | 2D |
| `DirLightDirections`, `DirLightColors` | `c187..c188`, `c189..c190` | two directional lights |
| `PointLightPositions`, `PointLightColors`, `PointLightParams` | `c171..c174`, `c175..c178`, `c179..c182` | four point lights |
| `ambientLightColor`, `ambientColor` | `c191`, `c200` | two ambient terms |
| `latitudeParam` | `c199` | |
| `reflectMapLod`, `reflectivity`, `reflectMapInfluence`, `refAlphaRestrain` | `c207`, `c208`, `c209`, `c211` | |
| `lightDiffuseMapLod`, `lightDiffusePower`, `lightDiffuseInfluence` | `c212`, `c213`, `c214` | |
| `fresnelExp`, `fresnelLightDiffBias` | `c215`, `c216` | |
| `specularInfluence` | `c204` | |

The CTAB has no hemisphere constant and no single light direction constant.
All directional lighting comes through `DirLightDirections[2]` and
`DirLightColors[2]`.

Observed arithmetic, in the order the program applies it:

- Direct light: `albedo * (ambientColor + ambientLightColor + sum over the two
  directional lights of sat(N . -L) * colour + point loop)`, multiplied by
  `v0`. The per-light accumulation is two `rep` loops, the point loop selects
  its slot with `cmp` ladders over `c171..c182`.
- Diffuse cube term: `lightDiffuseMap` is read with `texldl` at the perturbed
  world normal, LOD in `.w` from `lightDiffuseMapLod`. The term is `albedo *
  v7 * cube * (lightDiffusePower * view factor)`, scaled by `1 - sat(length(
  latitude(direct)) * lightDiffuseInfluence)`, and added on top of the direct
  light. The view factor is `lrp_sat` of the fresnel `pow(1 - |N . V|,
  fresnelExp)` between `fresnelLightDiffBias` and 1.
- Reflection: `reflectMap` is read with `texldl` at the reflected view vector,
  LOD from `reflectMapLod`. The sample is multiplied by `reflectivity` (mixed
  with `multiReflectivity` by the colour mask), the fresnel factor and
  `refAlphaRestrain`, and attenuated by `1 - sat(length * reflectMapInfluence)`.
- Specular uses the same attenuation form with `specularInfluence`.
- Tail: shadow term, fog mix with `fogColor`, `* modulateColor`, the
  `latitudeParam` scale and offset, then `* glareLdrScale.x`.

The PRAM block of this material carries `lightDiffuseMapLod` 6.3,
`lightDiffusePower` 2.5, `lightDiffuseInfluence` 0.58, `reflectMapLod` 4.0,
`reflectMapInfluence` 0.58, `fresnelExp` 2.0, `fresnelLightDiffBias` 0.6,
`ambientColor` 0 and `ambientLightColor` 0. The strings `ambientColor`,
`lightDiffuseMap`, `reflectMap`, `lightToneMap` and `shadowMap0` are not
defined as standalone strings in the executable. A search for
`lightDiffuseMap` and `reflectMap` finds only `cubeLightDiffuseMap` at
`0x010D2AFC` and `cubeReflectMap` at `0x010D2AEC`, referenced from `0x00C13D70`
and from a table at `0x01308BEC`, a different consumer that is not traced here.
So no engine code resolves `ambientColor` by name. `ambientLightColor` is
resolved by the driver, see
[colour flow](chara-material-shader-colour-flow.md#client-side-drivers-for-the-non-pram-constants-v-locators).
(I: `ambientColor` therefore keeps its PRAM value unless a colour row writes
it. The colour-row lanes in the colour-flow page do not include it.)

The cube resources in the weather bundles are 128 by 128 RGBA8 with 8 mip
levels, see
[DrawEnv cubes](zone-lighting-source-and-time-of-day.md#drawenv-cubes). By
arithmetic, level 6 is 2 by 2 and level 7 is 1 by 1, so a sample at LOD 6.3
lies between those two levels (I: an irradiance-like blur). The `texldl` LOD
is the PRAM float, so a lower authored value reads a sharper level.

## Sampler records and the engine bind (V)

PRAM sampler record, fields read by the bind routine at `0x00C402B0`:

| Record byte | Use |
|---|---|
| `+0x03` | Flag byte. Bit `0x80` marks an environment sampler. Bit `0x20` passes the state block at `+0x10`. |
| `+0x0C` | Sampler argument passed to the bind helper `0x00C46020`. `0xFF` skips the record. |
| `+0x0D` | Environment slot. Signed byte, the routine skips the record above `0x1F` unsigned. |
| `+0x10` | Sampler state block, passed by address when flag bit `0x20` is set. |

Layout of the PRAM body that those reads assume: sampler count at body `+0x14`,
a table of record offsets at the body offset stored at body `+0x18`, and a
table of sampler name offsets at the body offset stored at body `+0x1C`. The
first candidate string of a record starts at record `+0x20`.

`0x00C402B0` has two callers, both in `0x00C3D340` (the packed-state consumer
described in [PRAM packed render state](pram-packed-render-state.md)), at
`0x00C3D57C` and `0x00C3D597`. The routine runs only when the draw context
byte at `C +0x776` is set (`0x00C47AC0`). For each record with flag bit `0x80`:

| Condition | Observed operation |
|---|---|
| `C +0xC38` is null | Nothing is bound for the record. |
| `+0x0D` above `0x1F` | Nothing is bound. |
| Otherwise | `C +0xC40` receives the slot, `GetEnvmapTexture` (`0x00C20480`) is called on the DrawEnv at `C +0xC38`. |
| Slot holds a texture | The object at texture `+0x40` is bound through `0x00C46020` with the record argument. A zero object skips the bind. |
| Slot is empty | The bind helper receives entry 4 of the pointer vector at `[0x01374DB8] +0x14` instead. |

(I: the entry 4 object is a default texture. Its identity is not traced.)
Records without flag bit `0x80` take the other branch. When the flag byte
shares a bit with the byte at `0x01374D78`, the record binds global textures
through `0x00C3F630` for flag bits `0x01`, `0x02`, `0x04` and `0x08`.
Otherwise it binds the per-record texture the caller supplies. The executable holds the strings `Global00Texture`, `Global01Texture`
and `Global02Texture` at `0x010D7384`, `0x010D73A0` and `0x010D73BC`. No
`DrawEnv0...` string exists, and no routine compares a record's candidate
strings with anything on the environment branch.

Corpus scan over the installed `client/chara/{pc,mon,wep}` files whose path
ends in `_mdl` and that hold `SEDBshd` (5462 `pc`, 163 `mon`, 322 `wep`, 5947
total), parsing every PRAM sampler record by the layout above:

| Group | Records with flag bit `0x80` | Flag byte | `+0x0C` | `+0x0D` |
|---|---|---|---|---|
| `pc` | 14804 | all `0xA0` | all 0 | all 0 |
| `mon` | 566 | all `0xA0` | all 0 | all 0 |
| `wep` | 900 | all `0xA0` | all 0 | all 0 |

That is 16270 records. By name table, 8186 are `lightDiffuseMap` and 8084 are
`reflectMap`. 8084 PRAM blocks carry both and 102 carry `lightDiffuseMap`
alone. The first candidate string of these records is `DrawEnv00Texture_0` in
10932 records and `DrawEnv00Texture<N>_0` with N from 1 to 36 in the rest, a
few lack the `_0` tail. Slot 0 is read in every case, so the candidate string
does not choose the slot.

Result: a null DrawEnv binds nothing, an empty slot binds the default object,
and both engine samplers bind slot 0 of the draw's DrawEnv. `reflectMap` and
`lightDiffuseMap` therefore read the same cube resource. Their different
sampling (reflected view vector at `reflectMapLod`, normal at
`lightDiffuseMapLod`) is the whole difference. This replaces the sampler name
reading in the colour-flow page.

## Where the cube comes from (V unless marked)

Writers of the DrawEnv slot array (`SetEnvmapTexture`, `0x00C20410`, see the
[binding boundary](draw-env-texture-binding-boundary.md)):

| Caller | Observed source |
|---|---|
| `0x00A82020` | Layout initializer. Loop `i` from 0, element `i` of the object's array getter (slot `+0xAC`, `0x008D5980`) stored into slot `i`. |
| `0x00A816C0` | Second loop of the same shape, in slot 34 of the `LayoutDrawEnvObject` vftable (`0x0109FD44`, entry at `0x0109FDCC`). |
| `0x008CB370`, `0x0083BBE0`, `0x007D4E30`, `0x007D6180` | Each reads `GetEnvmapTexture(i)` from another DrawEnv and stores the pointer, with no generation. |
| `0x007E6300` | Reads the DrawEnv argument's slots, then stores entries of a pooled list into slots. |

`0x008D5980` is a bounds-checked array getter: array pointer at `+0x50`,
24-bit count at `+0x54`, a null return for an index at or above the count.
Its address is in vftable slots 42 and 43 of RTTI
`SQEX::CDev::Engine::Lay::Stella::Layout::CubeMapBaseObject` (vftable
`0x010A659C`) and of
`Application::Scene::Actor::Map::Layout::RaptureCubeMapBaseObject` (vftable
`0x01061704`).

So slot `i` of a layout DrawEnv holds `CubeMapBaseObject` texture `i` at
initialization. Every other writer copies a pointer or assigns an entry of a
pooled list. `Leap` for the DrawEnv fog parameters (`0x007D4E30`) copies the
source's 32 slot pointers rather than blending them. (I: `0x007E6300` assigns
weather blend render targets, since it is reached only from the weather
manager function `0x007E8910`.) The string
`WeatherManager::LeapEnvMapTexture` is pushed at `0x007E673C` in `0x007E6690`
ahead of a call to `0x009FC74A`, and the call to `0x007E5160` follows before
the paired call to `0x009FC750` (I: a profiler marker pair). The cube
render pass itself is not traced. `DrawEnvCubeBlendShaderDriver` (RTTI vftable `0x010F8E1C`) has an init at
`0x00C67E50` resolving the constants `drawEnvCubeMap` and `drawEnvBlendRatio`
(strings `0x010F8DB4`, `0x010F8DC4`) and an apply at `0x00C67EA0`. The apply
reads `C +0xC3C`, `C +0xC40` and `C +0xC44`, requires the ratio strictly
between the float at `0x01086650` and 1.0, calls `GetEnvmapTexture` on the
`C +0xC3C` object with the slot at `C +0xC40`, and binds the result. `C +0xC40`
is the slot the environment bind at `0x00C402B0` last stored. `C +0xC3C` is
the node `+0xF8` value stored by `SetDrawEnvParameter`, see the
[environment path](weather-sky-fog-environment-path.md#from-drawenv-to-the-pixel-programs).
Which pass runs this driver, and who writes `C +0xC44`, are U.

Layout cube names in the weather layout `data/29/B0/00/0F.DAT` (`wtr_fine`),
read as strings from the file:

| Group | Names |
|---|---|
| Lane envmaps | `envmap_00`, `envmap_01`, `envmap_10`, `envmap_11`, `envmap_12`, `envmap_20`, `envmap_21`, `envmap_22` |
| Outdoor time slices | `f0f_fine_000`, `f0f_fine_050`, `f0f_fine_060`, `f0f_fine_120`, `f0f_fine_180`, `f0f_fine_190` |
| Dungeon | `f0d_dwev11_01..04`, `f0d_dwev12_01`, `f0d_dwev12_02` |
| Indoor | `f0i_dwev21_01..03`, `f0i_dwev22_03`, `f0i_dwev22_04` |
| Fixed | `env_00_de`, `env_01_de` |

(I: the three-digit suffixes of the `f0f_fine_` names read as Eorzea hours 0,
5, 6, 12, 18 and 19 scaled by 10.) The class strings `LayEnvMapClip`
(`0x010A0EDC`, RTTI `.?AVLayEnvMapClip@Clip@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@`
at `0x012F3278`) and the `EnvironmentMapDrawEnvObject` layout class exist. The rule by which the clip
selects or blends a time slice is U.

## Same machinery for actors and terrain (V unless marked)

`DrawScheduler::SetDrawEnvParameter` (`0x00C58310`, see the
[environment path](weather-sky-fog-environment-path.md#from-drawenv-to-the-pixel-programs))
takes the DrawEnv for a node from node `+0xF4`, or the scheduler default at
scheduler `+0x10` when that is null. It stores the choice at `C +0xC38`. A
character model and a terrain piece take the same path, so the cube a
character samples is the cube of whichever DrawEnv the scheduler selects for
that node.

Setters on `SQEX::CDev::Engine::Fw::SceneObject::ModelObject` (vftable
`0x0109CE2C`) and `Application::Scene::RaptureModelObject` (vftable
`0x010653A4`), catalogued as `vfunc37` and `vfunc46`:

| Slot | Function | Observed stores |
|---|---|---|
| 37 | `0x008DD020` (`0x008DD510` for the Rapture class) | Argument to `this +0x288`. When the virtual at `+0x148` (it returns `this +0xD8`, `0x006576B0`) returns nonzero, the argument is also stored at that object's `+0xF4`. |
| 46 | `0x008DD060` (`0x008DD570`) | Argument to `this +0x290`. The same virtual's object receives it at `+0xF0`. |

(I: slot 37 installs the model's DrawEnv and slot 46 its light source. The
function that calls either setter for a character model is U.) Cut-scene
lighting uses separate machinery. The executable has the RTTI names
`CutDrawEnvListener`, `CutLightEnvListener`, `RaptureCutLightEnvListener`, the
clip names `LayAmbientLightClip`, `LayRaptureAmbientLightClip`,
`LayRaptureDirectionalLightClip`, `LayRapturePointLightClip` and
`LayRaptureSpotLightClip`, and `RaptureDrawEnvNameClip`.

## Per-node light list and ambient seed (V)

`0x00C628F0` builds a node's light block. Its argument is the node. The first
dword of its `this` object is the fallback DrawEnv when node `+0xF4` is null.
The routine does nothing when node byte `+0x2A8` has bit 0 set, which the
routine sets after the sources below, and it seeds only when node dword
`+0xA0` has bit `0x400`. The sources below run only when node byte `+0x2A8`
has bit 1. The block base is node `+0x134`.

Seed:

- `0x00C69C90` zeroes block `+0x130..+0x147` (node `+0x264..+0x27B`), sets the
  bytes at node `+0x284`, `+0x285`, `+0x286` to 0 and node `+0x287` to 1.
- `0x00C628F0` then stores (0, 0, 0, w) at node `+0x134..+0x143`, with w
  loaded from the float at `0x00F54F70` (bytes `0000803F`, 1.0).

So the 16 bytes at node `+0x134` start as (0, 0, 0, 1.0). The draw context
receives the block address in `SetDrawEnvParameter`'s caller region: the
instruction at `0x00C5870A` stores node `+0x134` into `C +0x308`. That
resolves the 16 byte base of the block the pixel driver reads, see the
[draw-time light list](zone-lighting-source-and-time-of-day.md#draw-time-light-list-and-rules).

Fill, through `0x00C69D20(light, flag)` (called at `0x00C62A35`, `0x00C62BB4`,
`0x00C62CF4`):

| Light type (`+0xDC`) | Observed operation |
|---|---|
| 4 (ambient) | Block 16 bytes `+=` (`+0xC0 +0xC4 +0xC8 +0xCC`) times the intensity at `+0xD0`, on all four lanes, as one packed add. Nothing is appended. |
| other | Appended when the count byte (block `+0x150`, node `+0x284`) is below 8 and the category counter is below 4. The category counter is block `+0x152` (node `+0x286`) when `flag == 1`, else block `+0x151` (node `+0x285`). The slot is at block `+0x10 + n * 0x24` with the light pointer at slot `+0x20`, and the flag is stored at block `+0x148 + n`. |

Since the add is packed, lane w of the block becomes 1.0 plus the light's
`+0xCC` times its intensity for each ambient light summed. The pixel shader
reads only the xyz lanes of `ambientLightColor` (I).

Sources, in order of the routine:

1. A container of light nodes: node `+0xF0`, else the pointer at DrawEnv
   `+0x328` (`0x00C1F330`) of the DrawEnv selected as node `+0xF4` or the
   scheduler default. Each member must pass the magic `0x4C544E44` at light
   `+0xFC`, an enable byte at `+0xF8` and a nonzero type. Skipped entirely
   when the dword at `0x01374E74` is nonzero. Members call `0x00C69D20` with
   flag 0. A dword at node `+0x26C` is incremented for each one that is not
   type 4 (its use is U).
2. `LightEnv::GetLightList` (`0x00C31E30`, magic `0x4C544556` at `+4`) on the
   object at node `+0x290`.
3. `GetLightList` on the LightEnv embedded at DrawEnv `+0x1A0` (`0x00C1F350`)
   of the DrawEnv selected as above (I for that selection).

For 2 and 3 the routine computes a pair of 16 byte vectors with `0x0062E180`
(`this` node `+0x70`, input node `+0x30`) and passes it as argument 2. It also
passes the block address plus `0x130` and two remaining-capacity values
computed as 4 minus node `+0x285` and 4 minus node `+0x286`. `GetLightList`
forms a centre and a radius from the vector pair (half the sum, and half the
length of the four-lane difference, with the 0.5 at `0x00F62F60`). (I: it
picks lights near that sphere.) The ranking and the per-light skips are in the
zone lighting page. Each returned light is applied through `0x00C69D20` with a
flag taken from a returned bit mask (I for the mask).

The pixel driver Apply (`0x00C22DD0`) then reads `C +0x308`: the count at block
`+0x150`, the slot pointers from block `+0x30` at stride `0x24`, the enable
bytes at block `+0x148 + i`, and the first 16 bytes (copied at
`0x00C236D4`). The `0.5` and `/N` ambient formula applied to those bytes is
owned by the zone lighting page. (I: pass 2 of that page never meets a type 4
slot for a block built here, because `0x00C69D20` adds type 4 lights into the
16 bytes and never stores them in a slot.)

## Unknowns

- Which cube slice the weather layout selects at a given Eorzea hour, and the
  blend law of `LayEnvMapClip` between slices.
- Which cube the zone DrawEnv supplies when no weather lane matches, and
  which element is `CubeMapBaseObject` index 0 for a given layout.
- Which code calls the slot 37 and slot 46 setters for a character model, and
  what default DrawEnv a character draw gets before that.
- Which render pass runs `DrawEnvCubeBlendShaderDriver`, and the writer of
  `C +0xC44`.
- The pass choice between `_000` and `_001`, and the decoded math of `_001`.
- The identity of the default object bound for an empty slot, and the
  sampler state bytes at record `+0x10`.
- Cube pixel values. No cube texel was read.
