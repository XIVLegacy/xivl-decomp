# Character material pixel shaders and colour flow

Scope: how the pixel programs and PRAM blocks shipped for `client/chara/pc/c001`
(equipment `equ`, face and hair `kao`, accessories `acc`) turn textures and
constants into colour. Static read of the installed 1.23b resources plus
`ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`, all locators VA). No retail bytes are recorded. Counts and shapes
only. Tiers: V = verified by reading the cited program or instruction,
I = inferred.

## Method

* 994 `_mdl/0001` files under `c001/{equ,kao,acc}` parsed. Each holds one or
  more `SEDBshd` sections whose `SHD` chunk has `FILE` children
  (`*_000.vpo`, `*_000.fpo`, `*_001.fpo`) and one `PRAM` chunk.
* PRAM layout read as `render mode i16`, `u16`, `float alpha reference` (body
  `+0x04`, consumed per [pram-packed-render-state.md](pram-packed-render-state.md)),
  counts, then data and name offset tables. Parameter record: stage byte,
  component count, four floats at record `+0x10`. Sampler record: magic
  `0x0102`, candidate strings.
* Pixel programs decoded as ps_3_0 with the embedded CTAB. 39 distinct pixel
  programs, in `_000` and `_001` pairs. The `_001` sibling has the same colour
  logic with a shorter body (I: lighter pass, selection rule not traced).

## Material families

The material name tail `<part><family>_<variant>` (such as `tc_a`, `ds_a`,
`hh_b`, `fi_d`, `fs_c`) is the key the client matches in
[appearance-color-rows.md](../actor/appearance-color-rows.md). Families seen:
`c` cloth, `s` skin, `h` hair (also the face `fh` brow and etc material), `i`
iris. Counts of PRAM blocks by family in this tree: cloth-like (`cc dc fc gc hc
jc kc mc nc oc pc qc ...`) several hundred, skin (`ds gs ts`) 25 or more,
hair 10 (`hh_b`, `fh_b`), iris 6, face skin 6.

## Per family colour formula (V)

All families end with the same tail: shadow factor, fog mix with `fogColor`,
`* modulateColor`, `latitudeParam` as `(c * x + y) * z`, then
`* glareLdrScale.x` into `oC0.rgb`. Output alpha is `tex.alpha * v0.w *
modulateColor.w`. Pixel input `v0` and `v7` come from the vertex program
(`o1 = lightAccum * vertexColour`, `o8 = vertexColour`), so a missing or zero
mesh colour stream multiplies the result toward black.

Common mix, with `m` the family mask below:

    base  = tex * diffuseColor.rgb
    multi = tex * multiDiffuseColor.rgb
    albedo = lerp(base, multi, m)

Specular colour, shininess and reflectivity use the same `m` with their
`multi*` twins. `multiDiffuseColor.w` is a mask gain only in the skin, hair and
face rows below.

| Family | Samplers (PRAM name -> texture suffix) | Albedo texture `tex` | Mask `m` |
|---|---|---|---|
| cloth `c` (tc, dc, mc, pc, jc, ...) | `_sampler_00` = `_d`, `_01` = `_s`, `_02` = `_n` | `_d.rgb` | `_n.b` (sampler_02 .z) |
| body skin `s` (e001 ds, ts, gs) | `_00` = `_n`, `_01` = `_d`, `_02` = `_s`, plus `lightToneMap` | `_d.rgb` | `_n.b * multiDiffuseColor.w` |
| face skin `fs_c` | `_00` = `_s`, `_01` = `_d`, `_02` = `_n`, plus `lightToneMap` | `fac_d.rgb` | `_s.r * multiDiffuseColor.w` |
| hair `hh_b`, brow and etc `fh_b` | `_00` = `_s`, `_01` = `_n` (no `_d`) | `_s.r` (grey, replicated) | `_n.b * _s.r * multiDiffuseColor.w` |
| iris `fi_d` | `_00` = `_s`, `_01` = `_n` (no `_d`) | `_s.r` (grey) | `_n.b` |

Other lanes: cloth `_s.rgb * reflectivity` and `_s.g` feed specular and
fresnel, `_s.a` is opacity. Hair opacity is `_s.g`, iris `_s.a`, face skin
`fac_s.a`, body skin `_s.a`. `_n.xy` is the tangent-space normal with z
rebuilt, which is why `_n.b` is free to carry the colour mask.

No `_s` texture is a hue tint map. The tint mask
is the blue channel of the normal map in cloth, body skin, hair and iris, and
the red channel of `_s` only in the face skin material. In hair and iris the
only luminance source is `_s.r`, and the only hue source is the constants.

## Sampled texture facts (shapes only)

* Cloth `_d` textures in a sample of 341 gear textures are low in saturation on
  average, and the `_n.b` mask covers a wide and varying share of the texels, so
  on average a substantial part of a cloth surface takes `multiDiffuseColor`.
* Hair h001 `_n.b` covers only part of the texels. Face f001 `fac_d` is a
  chromatic skin tone. `iri_s` is chromatic but only `.r` is read as albedo.
* Body e001 `_n.b` mask coverage differs between the top and dwn parts.

## PRAM defaults (V)

Counts by value pattern for `diffuseColor / multiDiffuseColor` across c001:

* Cloth: `(1,1,1,1)/(1,1,1,1)` is the most common row, so `lerp` is the
  identity and the model shows its `_d` texture untinted. Authored non-white
  `diffuseColor` rows also exist on some cloth materials.
* Skin (`ds gs ts fs`): `(1,1,1,1)/(1,1,1,1)`.
* Hair, face brow, iris: `(0.7,0.7,0.7,1)` for `diffuseColor` and
  `multiDiffuseColor` (iris and `fc_d` have `multiDiffuseColor.w = 0`). These
  render as 0.7 grey times `_s.r` until a colour is written.
* All: `modulateColor (1,1,1,1)`, `ambientColor 0`, `ambientLightColor 0`,
  `glareLdrScale 1`, `EnableShadowFlag 0`, `specularColor 1` (hair 0.7).
* Render modes in c001 materials: `0x81A` (alpha test, ref 0.5) most common,
  `0x81B` (hair and brow, ref 0.075 for h001, 0.0 for f001 brow), `0x818`,
  `0x813`.

## What PRAM does not supply (V, from CTAB minus PRAM names)

Pixel stage, cloth e014 `tc_a`: `DirLightColors[2]`, `DirLightDirections[2]`,
`PointLightColors[4]`, `PointLightParams[4]`, `PointLightPositions[4]`,
`latitudeParam`, `shadowColor`, `shadowFadeParam`, `shadowOffset0[4]`,
`shadowProjMatrix0..3`, and samplers `reflectMap`, `lightDiffuseMap`,
`shadowMap0` (and `lightToneMap` for skin and face skin). Hair and face add
nothing new. Vertex stage: `worldMatrix`, `worldITMatrix`, `worldViewMatrix`,
`worldViewProjMatrix`, `viewITMatrix`, `jointMatrices[144]`,
`ModelBBoxScale`, `ModelBBoxOffSet`, `PointLight*[2]`, bool `isSkining`, int
loop `i0`.

PRAM candidate strings for the engine samplers are placeholders that the
binder does not read, see
[Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md#sampler-records-and-the-engine-bind-v):
`reflectMap` and `lightDiffuseMap` both name `DrawEnv00Texture_0`,
`lightToneMap` names `c001f000f_t.dds` or `c001e000t_t.dds` with
`Global00Texture_0` or `Global02Texture_*`, iris `reflectMap` names
`c001f000f_y.dds` with `Global01Texture_1`. Binding of the two cube samplers is
by record flag bit and slot byte, not by name (V, same page).

## Client side drivers for the non-PRAM constants (V locators)

* `PixelLightShaderDriver` vftable `0x010D3BC4` (RTTI
  `ffxivgame.rtti.json`), slot 1 `0x00C21120` resolves handles by constant
  name through lookup `0x00420900`, and slot 2 `0x00C22DD0` writes them per draw
  from the draw context `C` and the scene light list. The constant list and the
  `latitudeParam`, `modulateColor`, `glareLdrScale` and `fogColor` formulas are
  owned by
  [weather-sky-fog-environment-path.md](weather-sky-fog-environment-path.md).
  The light list and ambient accumulation are owned by
  [zone-lighting-source-and-time-of-day.md](zone-lighting-source-and-time-of-day.md).
  The `modulateColor` alpha lanes are also covered in
  [floor-material-alpha.md](floor-material-alpha.md).
* Findings specific to this page: `latitudeParam` takes the form
  `(1.0, 0.0, ...)` when `0x00C47F80` returns true and the scale `x` is above 1.
  `ambientLightColor` is built from list lights whose type word (`+0xDC`) is 4
  (ambient), colour times intensity (I: averaged and scaled by the float `0.5`
  at `0x00F62F60`).
* `VertexLightShaderDriver` vftable `0x010D3BB0`, slots `0x00C20EA0`,
  `0x00C22A90`. Shadow constants come from `ShadowMapShaderDriver`
  (init `0x00C675E0`, see [model-pixel-shadow.md](../model-pixel-shadow.md)).
* Colour written over PRAM: `0x006BA250` and siblings per
  [appearance-color-rows.md](../actor/appearance-color-rows.md).

## Zero constants hazard (I)

With every non-PRAM constant zero, `shadowFadeParam.y = 0` makes
`rcp(shadowFadeParam.y)` infinite and, in IEEE evaluation, `0 * inf` can give
NaN even though `shadowColor.w = 0` would otherwise cancel the shadow term.
Direct3D 9 shader `mul` defines `0 * x` as 0, so this affects only an
evaluation that follows IEEE rules. A zero `latitudeParam` would black out the
whole output. This is an inference from the program text, not an observed draw.

## Not established

* Which pass (`_000` or `_001`) the client selects per draw.
* The cell values of the `var_equip` colour rows (retail data, not recorded).
* Where the context fields `C+0xBE4..0xC00` get their runtime values for a
  character draw. The sources of `C+0x1B4..0x1BC` are in
  [ScreenEnv Glare clip and the pixel tail tone constants](screen-env-glare-latitude.md),
  which leaves the writer of `C+0x1C0` unknown.

## Row to material matching (V)

The family to colour row mapping is traced end to end in
[appearance-color-rows.md](../actor/appearance-color-rows.md).
Summary: the material instance name is the `STR` chunk string of the instance
resource (`mt` + 8 character model id + 4 character key). The key tested by
the apply routine is the group letter (`m t d g s p f h`) plus a colour suffix
(`s_a..s_d`, `i_a..i_d`, `h_a..h_d`). The test is ends_with, exact and case
sensitive (`0x00630AA0`). The written name `diffuseColorR` is a lane 0 write
into the `diffuseColor` vec4 of the matched instance (`0x00C3F310`). Names that
end in `R G B A` (or `X Y Z W`) select lane 0 to 3 and are stripped to the base
name, other names such as `shininess` write lane 0.

## Unset-row behaviour (I)

The apply routine skips a cell only when it is at or above 999.0 (a NaN cell
also skips) and writes every other cell literally, including 0 and negative
values (`COMISD` at `0x006BAB24`, `JBE` at `0x006BAB28`, double at `0x00FD3FC8`).
A lane keeps its PRAM default only when its cell is skipped, and a cell that
reads 0 overwrites the default with 0. Nothing in the routine treats any row
pattern as unset or substitutes a default. The lanes a colour row can write are
fixed by the suffix family:

* `s_*` (row slot 8, columns 0..2): `diffuseColorR/G/B`.
* `i_*` (row slot 9, columns 0..5): `diffuseColorR/G/B` and
  `multiDiffuseColorR/G/B`.
* `h_*` (row slot 10, columns 0..2 and 7..9): `diffuseColorR/G/B` and
  `specularColorR/G/B`.

Cloth `c_*` materials are not reached by colour rows, so this affects only skin,
iris and hair or brow materials. Which cells the retail `var_equip` rows hold
for any given lane value is not established here, so the visible result of a
row that carries 0 cells is not established. It would need a read of those
rows from the install or a client walk.
