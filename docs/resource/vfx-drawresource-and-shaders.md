# Qix DrawResource, model shaders and render state

The DrawResource control is the module that draws a model for one unit. It
appears in 1,062 of the 1,069 current effects. This page records its links and
resource block, the flipbook texture matrix, the material element that carries
the blend and depth state, the render state routine that turns that state into
Direct3D 9 render states, the shader records that a vmdl blob holds and the named
constants those shaders read. It ends with what a renderer needs to draw this
family and what is unknown.

The tables the control belongs to are on the
[control module registry](vfx-control-module-registry.md) page, the arrays that
declare and feed it are on the [effect data body](vfx-effect-data-body.md) page
and the frame update is on the [effect runtime](vfx-effect-runtime.md) page. The
machine-checked form of every STATIC fact is
[`vfx_effect_runtime.json`](../../config/vfx_effect_runtime.json), checked by
`tools/verify_vfx_effect_runtime.py`. The identity and method section of the
[container page](vfx-qix-resource-container.md#identity-and-method) applies here
unchanged, and so do its tier labels.

## The DrawResource module

The Rapture table descriptor `DrawResource` has the signature `030200000800`,
a work buffer of 40 bytes with 4 byte alignment and the slots below. STATIC,
from the manifest rows.

| Slot | Address | Role |
|---|---|---|
| `+0x68` | `0x00d66690` | construct, calls `0x00d86e40` with the block and the state at work `+0x10` |
| `+0x6c` | `0x00d666b0` | destruct, calls `0x00d87100` |
| `+0x78` | `0x00d66790` | calc, calls `0x00d838b0` with the tick delta |
| `+0x7c` | `0x00d667f0` | draw, INFERRED from its callee `0x00d87180` |
| `+0x8c` | `0x00d666d0` | not traced |
| `+0x90` | `0x00d66730` | not traced |

The static link storage starts at work `+0x20`. The draw slot reads the color
pointer from work `+0x20` and the matrix pointer from work `+0x24`, and it draws
only when the float at color `+0x0c` is above 0.0. STATIC, `0x00d667f0`.

In all 7,575 DrawResource nodes of the current effects the link entry holds
exactly two static link codes and both are unit local codes naming nodes of the
same unit. The first names a color node and the second a matrix node. INFERRED
from the install.

| Link | Module and nodes |
|---|---|
| color | `ColorRGBARoot:CoordRoot` 7,418, `ColorRGBABlink` 80, `ColorRGBABlink:CoordRoot` 76, `ColorRGBAWorld:CoordWorld` 1 |
| matrix | `MatrixRoot:MatrixRoot` 5,874, `MatrixLinkEx:MatrixLinkEx` 1,575, `MatrixLink:MatrixLink` 126 |

The [generic color-root arithmetic](../actor/garuda-plume-color-root.md) and the
[selected MatrixRoot mode-0 calculation](vfx-root-matrix-mode-zero.md) have bounded
static contracts. Current per-node factors and inputs, other matrix modes and
draw-time outcomes remain unresolved. For comparison, the calc of
`Position3D:CoordRoot` at `0x00ba4c60` copies the three floats that its first slot
pointer names into its work area at `+0x10` and sets the fourth float to 1.0.
STATIC.

## Resource block

The control's parameter block holds a pointer at `+0x00` to a record and a kind
byte at `+0x04`. The record carries a 24 byte reference at its `+0x08`: a name, the
reversed tag and a flags dword whose bit 1 marks the reference as unused. STATIC
for the reference reads in `0x00d85df0` and `0x00d86bc0`. The kinds are read by
`0x00d86e40`, `0x00d85e70` and `0x00d87180`, which switch on kinds 1 to 4, 7 and 8.
INFERRED: kinds 1, 3 and 8 name one model, kinds 2 and 4 name lists of 0x2c and
0x34 byte elements and kind 7 reads a second reference at record `+0x30`.

| Kind | Blocks in the install | Reference |
|---|---:|---|
| 1 | 7,382 | one model |
| 3 | 162 | one model |
| 8 | 12 | one model with a flipbook |
| 4 | 10 | list of 0x34 byte elements |
| 2 | 9 | list of 0x2c byte elements |

The 7,556 blocks of kinds 1, 3 and 8 all carry the tag `ldmv`, the reversed `vmdl`,
and a clear unused bit, and 7,555 of them name a model of the veff's own model
reference array. INFERRED from the install.

## Material element

For each part of the model the binder at `0x00d86010` reads a 0x2c byte element.
Offsets are relative to the element. STATIC unless noted.

| Offset | Width | Field |
|---|---:|---|
| `+0x00` | 24 | texture reference, INFERRED |
| `+0x1e` | 2 | texture slot, INFERRED |
| `+0x20` | 8 | pointer to the material parameter record pointers and their count |
| `+0x28` | 1 | blend byte |
| `+0x29` | 1 | cull byte |
| `+0x2a` | 1 | depth function index |
| `+0x2b` | 1 | flags |

The blend byte maps through `0x00d815d0` to the engine blend mode in the table
below and is stored at node `+0x46`. The cull byte is kept when it is 1 or 2 and
becomes 0 otherwise, and is stored at node `+0x45`. The depth function index is
stored at node `+0x47`. Bit 0 of the flags sets bit 2 of the node flag word at
`+0x30` and bit 1 sets bit 0. The binder then writes the draw color, four floats,
into the material constant table slot that holds `controlColor`. The parameter
records that follow switch on a type byte with the codes 1 to 0x16 and write
vectors, samplers and matrices to material slots. Only the color write and the
type 0xa case, which INFERRED takes the flipbook matrix, were followed.

| Blend byte | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Engine blend mode | 8 | 1 | 2 | 3 | 4 | 7 | 6 | 7 |

A blend byte above 7 gives engine blend mode 0 through the same routine. STATIC,
`0x00d815d0`.

## Flipbook texture matrix

Resource kind 8 draws a model whose texture matrix steps through a sprite sheet.
The state at work `+0x10` holds a tick word at `+0x04` and two signed bytes at
`+0x01` and `+0x02`. STATIC, `0x00d87180`.

- The frame is the tick word divided by 5000.0.
- The column count is the u16 at record `+0x26` and the row count is the u16 at
  record `+0x2c`.
- The cell origin is `(frame % columns) / columns` and `(frame / columns) / rows`.
- The cell scale is `1 / columns` and `1 / rows`, multiplied by the signed bytes.
- The matrix is built by `0x00d82a70`. INFERRED: it reaches the draw through the
  override struct whose callback `0x00d818d0` the first type 0xa material
  parameter applies.

Calc adds the tick delta times the float that record `+0x28` names to the tick
word and wraps the result by `0x00d814a0`. The wrap tests the frame against the
s16 limit at state `+0x08`, which is columns times rows minus 1 for kind 8, and
acts when the frame reaches the limit. The mode byte at state `+0x0a` decides:
mode 0 resets the word to 0, mode 1 sets it to the limit times 5000 and mode 2 sets
it to the limit plus one times 5000 and reports false. The start word and the
mode byte are set in `0x00d86e40`. STATIC.

## Render state

The routine at `0x00c13770` applies the state of one draw node. Offsets are
relative to the node. STATIC, with the engine setters at the cited addresses.

| Node field | Engine call | D3D9 state |
|---|---|---|
| flag bit 6, unless the global flag `0x40` at `0x0136e868` is set | depth enable | `ZENABLE` (7) |
| byte `+0x47` | depth function | `ZFUNC` (23) through the table at `0x00f596c0` |
| flag bit 2 | depth write | `ZWRITEENABLE` (14) |
| byte `+0x46` is not 8 | blend enable | `ALPHABLENDENABLE` (27) |
| byte `+0x46` | blend mode | see below |
| byte `+0x45` | cull | `CULLMODE` (22) through the table at `0x00f597c8` |
| floats `+0x3c` and `+0x38` | depth bias | `SLOPESCALEDEPTHBIAS` (175) and `DEPTHBIAS` (195, divided by 16777215.0) |
| flag bit 5 | alpha test enable | `ALPHATESTENABLE` (15) |
| float `+0x34` | alpha reference, function index 6 | `ALPHAFUNC` (25) and `ALPHAREF` (24, times 255 plus 0.5, at most 255) |

The compare table at `0x00f596c0` holds 1 to 8, the D3D9 compare functions never to
always, so index 6 is greater or equal. The cull table holds 1, 2 and 3, the D3D9
none, clockwise and counterclockwise values. The routine ends by clearing and
setting bits of the flag word with the masks at `0x01308680` and `0x01308684`.

The mode setter at `0x00c0efd0` first tests byte `+0xb8` of its object. When it is
1 the setter stores the mode at `+0xc0` and sets the factor indices 1 and 0 for
source and destination, which are one and zero, with the add operation, whatever
the mode table below says. STATIC. What sets `+0xb8` was not traced. Otherwise it
handles the engine blend modes 0 to 7. Each pair
is set for color and alpha alike and the operation applies to both. The index
tables at `0x00f596e0` and `0x00f5971c` map the indices to D3D9 values: factor
indices 0 to 12 are zero, one, source color, inverse source color, destination
color, inverse destination color, source alpha, inverse source alpha, destination
alpha, inverse destination alpha, source alpha saturate, blend factor and inverse
blend factor, and operation indices 0 to 4 are add, min, max, subtract and
reverse subtract.

| Mode | Source | Destination | Operation |
|---|---|---|---|
| 0 | | | blending disabled |
| 1 | source alpha | inverse source alpha | add |
| 2 | source alpha | inverse source alpha | reverse subtract |
| 3 | source alpha | one | add |
| 4 | source alpha | one | reverse subtract |
| 5 | | | no state change |
| 6 | inverse destination color | inverse source alpha | reverse subtract |
| 7 | inverse destination color | zero | add |
| 8 | | | blend enable off, no mode change |

The control classes of the Rapture table use a second blend type setter at
`0x00d972c0` with the types 0, 1, 2, 3, 4 and 7. Type 0 turns blending off and
enables depth write and each other type turns blending on, turns depth write off
and selects the mode of the same number.

## The vmdl blob

The `vmdl` blob that a DrawResource record names has a 0x70 byte root. Offsets are
relative to root record 0. INFERRED from the install and from the readers
`0x00bc43b0`, `0x00be2110` and `0x00be1450`.

| Offset | Width | Field |
|---|---:|---|
| `+0x00` | 16 | resource name |
| `+0x10` | 4 | reversed tag `ldmv` |
| `+0x18` | 8 | shader set array and count, 20 byte entries |
| `+0x20` | 8 | texture reference array and count, 24 byte entries |
| `+0x28` | 4 | type, 1 in every vmdl |
| `+0x2c` | 8 | model chunk pointer and size |
| `+0x34` | 4 | variant, 1 when an animation chunk is present |
| `+0x38` | 8 | animation chunk pointer and size, or 0 |
| `+0x40` | 8 | instance chunk pointer and size |
| `+0x48` | 8 | vertex shader of the first set |
| `+0x50` | 8 | pixel shader of the first set |

A shader set entry is the vertex shader pointer and size, the pixel shader pointer
and size and a zero word. In the 4,993 installed vmdl the root copy at `+0x48` and
`+0x50` equals the first set in all of them, 4,944 have one set and 49 have two,
and the flags word is zero in all 5,042 sets. The texture reference arrays hold 0
(156 models), 1 (4,404), 2 (385) or 3 (48) entries, all named with the tag `xetv`.
The model chunk carries the file magic `vfxm` at its `+0x04` in its dword form
`mxfv` in 4,948 models and as the bytes `vfxm` in 45, and the instance chunk
carries `vins` the same way. The animation chunk carries `vfxa` the same way. The
selected legacy version-2.2 geometry and material framing is described in
[Legacy VFX model geometry](vfx-legacy-model-geometry.md). The
[modern geometry field dictionary](vfx-modern-model-geometry.md) records the
selected stock version-4 header, geometry fields, packed sources and conditional
declaration interpretation. The
[seasonal model and material bindings](seasonal-model-material-bindings.md)
provide its indexed-library and material context. Those bounded findings do not
establish every model, animation or instance layout.

A shader record is a 16 byte key, a big endian u32 length, 12 pad bytes and the
Direct3D 9 token stream of that length. The length plus 32 equals the record size
in all 10,084 records. The pad is zero in 9,576 and holds other bytes in 508. The
vertex shaders all begin with the version token `0xfffe0300` (vs_3_0) and the pixel
shaders with `0xffff0300` (ps_3_0). The token stream carries a CTAB comment whose
constant names are what the client binds. The install holds 181 distinct records,
65 vertex and 116 pixel. The decoder
[`tools/d3d9_shader_decode.py`](../../tools/d3d9_shader_decode.py) reads such a
stream. INFERRED.

### Constants the shaders declare

Register set 2 is float4, 0 is bool and 3 is sampler. The count is the number of
distinct records that declare the constant.

| Vertex constants | Set | Records |
|---|---|---:|
| `ModelBBoxOffSet`, `ModelBBoxScale`, `controlColor`, `depthOffSetWVPMatrix`, `fogColor`, `fogParam`, `jointMatrices`, `viewITMatrix`, `worldITMatrix`, `worldMatrix`, `worldViewMatrix`, `worldViewProjMatrix` | 2 | 65 each |
| `isSkining` | 0 | 65 |
| `textureMatrix0` | 2 | 55 |
| `isUseInstancing` | 0 | 22 |
| `uvofs0` | 2 | 19 |
| `textureMatrix1` | 2 | 17 |
| `vfxFresnelCenterColor`, `vfxFresnelOutsideColor`, `vfxFresnelParam` | 2 | 11 each |
| `ambientColor`, `ambientLightColor`, `lightColors`, `lightDirections`, `lightParams`, `lightPositions`, `shininess`, `specularColor` | 2 | 9 each |
| `light0bit0` to `light3bit1` (eight bools) | 0 | 9 each |
| `vfxVanmMergePercent` | 2 | 7 |
| `exLightParam` | 2 | 4 |
| `uvofs1`, `vfxVertexAnimNormalOffsetParam` | 2 | 3 each |
| `vfxMorphingBlend`, `vfxMorphingSrcIndex`, `vfxMorphingTargetIndex` | 2 | 1 each |

| Pixel constants | Set | Records |
|---|---|---:|
| `latitudeParam` | 2 | 114 |
| `_sampler_00` | 3 | 90 |
| `_sampler_00TexColor` | 2 | 79 |
| `latitudeMax` | 2 | 74 |
| `diffuseColor` | 2 | 44 |
| `fogColor` | 2 | 42 |
| `_sampler_01`, `_sampler_01TexColor` | 3, 2 | 31 each |
| `TextureDist_1_distortion_amount`, `Texture_lookup_2d_1_texture` | 2, 3 | 29 each |
| `vfxDistortionTex_UVScale`, `vfxDistortionTex_UVScroll` | 2 | 25 each |
| `_sampler_00TexMatrix` | 2 | 23 |
| `cubeLightDiffuseMap` (3), `lightDiffuseInfluence`, `lightDiffuseMapLod`, `lightDiffusePower` | 3, 2 | 20 each |
| `_sampler_01TexMatrix` | 2 | 17 |
| the lighting constants of the vertex table | 2, 0 | 17 each |
| `multiDiffuseColor` | 2 | 14 |
| `_sampler_02`, `_sampler_02TexColor`, `_sampler_02TexMatrix`, `cubeReflectMap` (3), `multiShininess`, `multiSpecularColor`, `normalPower`, `refAlphaRestrain`, `reflectMapInfluence`, `reflectMapLod`, `reflectivity` | 3, 2 | 12 each |
| `modulateColor`, `vfxVanmMergePercent` | 2 | 11 each |
| `normalVector` | 2 | 9 |
| `Texture_lookup_2d_2_texture` (3), `vfxDepthMap` (3), `vfxDistortionTexMulti_UVScale_1` and `_2`, `vfxDistortionTexMulti_UVScroll_1` and `_2`, `vfxSoftParticleParam` | 3, 2 | 4 each |
| `multiReflectivity` | 2 | 3 |
| `distortionMap` (3), `distortionModelCenter`, `distortionNormalPower`, `distortionParam`, `distortionPivot`, `distortionRotMatrix`, `screenMap` (3), `viewProjMatrix` | 3, 2 | 2 each |
| shader graph constants such as `Color_contrast_1_amount`, `Environment_map_cubic_1_env_tex`, `__camera_to_world`, `vfxMultiTexture_Tex2` and `vfxMultiTexture_Tex3` | 2, 3 | 1 each |

### Engine material constants

The routine `0x00c13c20` registers 21 named constants in the material table from the
rows at `0x013086a0`. A row is 32 bytes: a name pointer, a byte size, a zero word,
a word `0x22` and four default floats at `+0x10`. Rows up to 16 bytes copy
the defaults. Larger rows are initialized by `0x00bbb990` from the value that
`0x00e3acf0` returns and not from the default words. STATIC.

| Name | Bytes | Defaults |
|---|---:|---|
| `controlColor`, `modulateColor`, `fogColor`, `vfxVanmMergePercent`, `distortionParam` | 16 | 1, 1, 1, 1 |
| `distortionPivot`, `distortionNormalPower`, `distortionScreenTexSize` | 8 | 1, 1, 1, 1 |
| `distortionModelCenter` | 16 | 0, 0, 0, 1 |
| `fogParam` | 12 | 0, 100, 1, 1 |
| `uvofs0` to `uvofs3` | 8 | 0, 0, 1, 1 |
| `vfxSoftParticleParam` | 16 | 1.8, 1, 1, 1 |
| `depthOffSetWVPMatrix`, `distortionRotMatrix`, `textureMatrix0` to `textureMatrix3` | 64 | not used |

The routine `0x00c133e0` looks up the slots of these names and of `vfogRange`,
`vfogTopColor` and `vfogBottomColor` in the loaded shader's constant table and
keeps the slot indices. The model matrices, the light constants and the latitude
constants are bound by routines that were not traced.

## What a renderer needs for this family

1. Walk the pack as the [container](vfx-qix-resource-container.md) and
   [family](vfx-effect-family.md) pages describe, resolve the veff's model and
   texture references, and read the root directory of the data body page. Reject a
   root whose version is not `0x1d` or whose declarations fail the compliance check,
   and substitute the built-in default.
2. Build the unit tree from instance 0 and the child schedules, creating a child at
   its start tick. Create further children from the Generate controls as the
   [emitters page](vfx-generate-emitters.md#math-for-bahamut-client) describes.
3. For each DrawResource node take the model name from its resource block, the
   color from the linked color node and the world matrix from the linked matrix
   node, and skip the draw when the color alpha is not above 0.
4. Take each model part's blend, cull, depth function and flags from its material
   element and apply the render state table above, with the blend mode table for
   the factors.
5. Use the vertex and pixel shader records of the vmdl as vs_3_0 and ps_3_0
   programs, bind the model's textures to the samplers by the CTAB names and supply
   the named constants. The draw color goes to `controlColor`. The flipbook matrix
   of a kind 8 resource reaches the draw through the type 0xa material parameter,
   INFERRED, and the constant it lands in is UNKNOWN: the `textureMatrix0` row of
   the defaults table is marked not used, and 23 pixel records declare
   `_sampler_00TexMatrix`, so the target needs a CALIBRATION ruling. The defaults
   table covers only the engine rows of 16 bytes or smaller.

## Unknown

- Complete model setup and submission beyond the bounded
  [legacy version-2.2 sources and material framing](vfx-legacy-model-geometry.md),
  the [selected version-4 field dictionary](vfx-modern-model-geometry.md) and
  [version-3-or-later bindings](seasonal-model-material-bindings.md), including
  unqualified sprite-buffer paths and lower resource creation.
- MatrixRoot modes 1 through 10, nonzero-angle preparation, the zero-vector
  fallback and the angle and scale controls other than the Generated receivers.
  Current color factors, matrix providers and getter-backed state, source units
  and how polynomial graph values reach per-node color/matrix inputs remain
  unresolved. The time variable of the
  polynomial segments and the Generate modules are on the
  [emitters page](vfx-generate-emitters.md).
- The material constant that the type 0xa parameter writes, and so which shader
  constant receives the flipbook matrix. CALIBRATION.
- The material parameter type codes other than the color write and type 0xa, the
  sources of the model, light, fog and latitude constants and the model draw call
  that follows the constant binding.
- Whether and how a unit is drawn as a camera-facing billboard. No billboard
  routine was followed.
- The node flag bits other than 0, 2, 5 and 6 and the origin of the node depth
  bias floats.

## Verification

```text
python tools/verify_vfx_effect_runtime.py --exe orig/ffxivgame.exe
python tools/verify_vfx_effect_runtime.py --install <game-root>
python tools/test_vfx_effect_runtime.py
```

`--exe` compares the render state, blend, flipbook and material constant checks
and tables with the pinned executable. `--install` recounts the resource blocks,
the model records, the shader sets and the declared shader constants.
