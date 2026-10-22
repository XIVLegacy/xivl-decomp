# Character lighting model

Consolidated contract for how the 1.23b client lights the character materials
of `client/chara/pc/c001` (equipment `equ`, face and hair `kao`, accessories
`acc`). The vertex and pixel programs are restated as closed-form formulas and
the engine-fed inputs are listed with the page that owns each one. The final
output stage is in [Present blit gamma and dither](present-gamma-blit.md).
Tiers: V = verified from a decoded program, disassembly or file bytes,
I = inferred, U = unknown.

## Evidence

| Field | Value |
|---|---|
| Executable | `ffxivgame.exe`, build `2012.09.19.0001`, 15996808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000`, all locators VA |
| Install inputs | `_mdl/0001` files under `client/chara/pc/c001/{equ,kao,acc}`, 871 PRAM blocks, 805 of them with the light rows |
| Decoder | `tools/d3d9_shader_decode.py`, canonical text of a `vs_3_0` or `ps_3_0` token stream |
| Manifest | `config/chara_lighting_model.json`, 14 install shader pins, 2 executable shader pins, the program census, 28 static rows |
| Reference | `tools/verify_chara_lighting_model.py` holds the closed forms |
| Gates | `python3 tools/verify_chara_lighting_model.py --exe <ffxivgame.exe> --install <install root>` and `python3 tools/test_chara_lighting_model.py` |

No shader bytes, texels or decompiled bodies are recorded. A pin is a SHA-256
of the decoded text and of the constant table, plus the line count. The four
golden vectors in the test come from an untracked executor of the decoded cloth
programs. They cannot be regenerated from the repository and are a lead. The
test compares the closed forms to them at a relative tolerance of 2e-5. The
closed forms were also read against the decoded `_000` cloth listing term by
term.

## What is settled

- Every character material uses the same `vs_3_0` family and a `ps_3_0`
  family that ends in one shared tail. The shader math is plain arithmetic on
  stored values. None of the 44 distinct programs found in the c001 files (4
  vertex, 40 pixel, counted by `shaderCensus` in the manifest) carries a `def`
  constant near 2.2, 1/2.2, 2.4, 1/2.4, 12.92, 1.055 or 0.055, and the two device
  state managers are never given the sRGB write state (194) or
  the sRGB sampler state (11) as an immediate (V).
- A pixel program has at most two directional lights, four point lights (two in
  the `001` pass) and one ambient colour. Light colour is `colour * intensity`
  with no clamp and no division by pi (V).
- The lit colour is multiplied once by a per-vertex term that is
  `(1 + two vertex point lights) * vertex colour`. A missing or zero vertex
  colour darkens the whole model and every nearby vertex point light raises the
  term above the vertex colour (V).
- The output is not clamped inside the program. Values above 1 reach the scene
  target, which is 8-bit (I), and are clamped there. With the authored
  `lightDiffusePower` 2.5 the cube term alone can push a channel above 1 for
  synthetic lights, as the `daylight` golden vector does with invented inputs
  (V for the arithmetic). Whether retail light colours do so is U until the
  runtime light values are recorded (CALIBRATION).
- Three additive environment terms sit on top of the direct light: a diffuse
  cube term, a reflection cube term and a specular term. Each is attenuated by
  `1 - sat(length * influence)` where `length` is the Euclidean length of the
  lit colour after the latitude transform (V).
- The ambient colour handed to the program is not the sum of the ambient lights.
  It is `(block + running / N) * 0.5` (V, see [Light inputs](#light-inputs)).

## Light inputs

| Pixel constant | Value | Tier | Owner |
|---|---|---|---|
| `DirLightDirections[i]` | world direction of light travel, the program uses `-L` | V | [Zone lighting](zone-lighting-source-and-time-of-day.md#draw-time-light-list-and-rules) |
| `DirLightColors[i]` | `colour * intensity * tint * 1.0`, w 0. Tint is `C+0xC04` for slot 0 and `C+0xC14` for slot 1, others untinted | V | same page |
| `PointLightPositions[i]` | light position | V | same page |
| `PointLightColors[i]` | `colour * intensity * 1.0` | V | same page |
| `PointLightParams[i]` | `(-1, -1, +0xE0, +0xE4)`, the program reads z as the decay exponent and w as the radius | V | same page |
| `ambientLightColor` | `(block16 + running / N) * 0.5` when N is above 0, else `block16`. `block16` starts at `(0, 0, 0, 1)` and gains each type 4 light's `colour * intensity` | V | [Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md#per-node-light-list-and-ambient-seed-v) |
| `ambientColor` | PRAM row, rgb 0 in all but 5 blocks | V | census below |
| `modulateColor` | `(node colour rgb, 1.0) * mesh colour rgba`, node colour at node `+0xB0`, mesh colour at draw tag `+0xA0` | V | [Weather, sky and fog path](weather-sky-fog-environment-path.md#from-drawenv-to-the-pixel-programs) |
| `fogColor` | DrawEnv fog colour when the draw's fog enable (`C+0x4E4`, the complement of PRAM word bit `0x40` per `0x00C3D340`) is on, else the PRAM row `(1, 1, 1, 0)` | V | [Weather fog parameters](weather-fog-parameters.md) |
| `latitudeParam` | `(1/(max-min), -min/(max-min), C+0x1B4, 1)` | V | [ScreenEnv Glare clip](screen-env-glare-latitude.md#pixel-driver-constants-v) |
| `glareLdrScale` | `1 / C+0x1C0` | V | same page |
| shadow constants | `shadowColor`, `shadowFadeParam`, `shadowOffset0[4]`, `shadowProjMatrix0..3` | V names, U values | [Model Pixel shadow parameters](../model-pixel-shadow.md) |
| `EnableShadowFlag` | engine writes `(1, 1, 1, 1)` and zeroes lane x when directional slot 0 is used and lane y when point slot 0 is used. Every decoded program reads `max(1, lane)` so the flag has no effect | V | this page |

The ambient finalize reads the 16 byte block at `C+0x308`, copies it to the
driver at `0x00C236D4`, divides the running sum by the converted count at
`0x00C23722` and multiplies by the float at `0x00F62F60`, which is 0.5. All
three are rows in the manifest.

The node colour and tint are written through `ModelObject` slots 51 and 57
(`0x008DDF80` and `0x008DDFE0`, which store a 16 byte vector at model `+0x90`
and `+0xA0` and forward it to node `+0xB0` and `+0xD0`). Their default for a
character model, and who calls the setters, are U.

## Vertex program

The cloth family has one vertex program (`511577889b120f87` in the manifest)
and the hair and brow family one variant (`a223e42399fb3190`).

| Output | Value | Tier |
|---|---|---|
| position and `texcoord2` | clip position, `texcoord2.z` is the clip z, not a view depth | V |
| `texcoord1` | world position | V |
| `texcoord3` | world normal, not normalised, from the inverse transpose matrix | V |
| `color0` | `((world tangent) + 1) * 0.5`, w is the packed tangent sign | V |
| `texcoord4` | `(u + uvofs0.x, 1 - (v + uvofs0.y))` | V |
| `texcoord6.xyz` | `normalize(world position - camera)`, camera is the `.w` column of the view inverse transpose | V |
| `texcoord6.w` | `pow(sat((-z_view - fogParam.x) / (fogParam.y - fogParam.x)), fogParam.z)` | V |
| `texcoord7` | vertex colour rgb | V |
| `texcoord0` | `(1 + sum over 2 point lights of atten * sat(N.-L) * colour) * vertex colour`, w is the vertex colour alpha | V |

Inputs are position scaled and offset by `ModelBBoxScale` and `ModelBBoxOffSet`,
vertex colour, normal and tangent packed as `x * 2 - 1`, four blend weights and
four blend indices (multiplied by 3, three matrix rows per joint) and one uv.
The skinning branch is the bool `isSkining`. The vertex point light colour of
iteration 1 is read directly and iteration 0 is `max(1, flag) * colour 0`. The
vertex attenuation is `max(radius - d, 0.001) / max(radius * d^decay, 0.001)`.
The hair and brow variant replaces `sat(N.-L)` by
`pow(sat(dot(N, -L) * exLightParam.x + exLightParam.y), exLightParam.z)` with
the vertex constant `exLightParam`.

## Cloth pixel program

Samplers: `s0` is `reflectMap` (cube), `s1` is `lightDiffuseMap` (cube), `s2`
is `_sampler_00` (colour `_d`), `s3` is `_sampler_01` (`_s`), `s4` is
`_sampler_02` (`_n`) and `s10` is `shadowMap0`. Both cubes bind slot 0 of the
draw's DrawEnv, see [Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md#sampler-records-and-the-engine-bind-v).
`d`, `s` and `n` below are texels, `m = n.b` is the mask.

```
albedo   = lerp(d.rgb * diffuseColor, d.rgb * multiDiffuseColor, m)
curve    = 1.2 * s.rgb * (2 - s.rgb)^2
specCol  = lerp(curve * specularColor, curve * multiSpecularColor, m)
divisor  = 1 / (s.g*s.g - s.g + 0.26)              // 3.85 at s.g 0 or 1, 100 at 0.5
shin     = lerp(divisor * shininess, divisor * multiShininess, m)
refl     = lerp(s.rgb * reflectivity, s.rgb * multiReflectivity, m)

T = normalize(v4.xyz*2-1)   B = cross(T, N0) * (v4.w*2-1)   N0 = normalize(v3)
(x, y) = n.xy - 0.5         z = sqrt(0.25 - x*x - y*y)
q = normalize(x*normalPower, y*normalPower, z)
N = q.x*B + q.y*T + q.z*N0                          // not renormalised
V = normalize(v6.xyz)       Rv = V - 2*dot(V,N)*N
F = pow(1 - sat(abs(dot(V,N))), fresnelExp)
fSpec = sat(s.g + (1 - s.g) * F)     fDiff = sat(bias + (1 - bias) * F)   // bias = fresnelLightDiffBias

for each light (2 directional, 4 or 2 point):
  col = light colour,  att = 1 for directional
  point: d = |v1 - P|, att = max(R - d, 0.001) * (1/(R*d^p) if R*d^p >= 0.001 else 1000), L = (v1 - P)/d
  dN = sat(dot(N, -L))      dR = sat(dot(Rv, -L))
  diffuse += col * att * dN
  spec    += col * att * dR^shin * (1 - (1 - dN)^8)

lit  = (ambientColor + ambientLightColor + diffuse) * albedo * v0.rgb
len  = |(lit * latitude.x + latitude.y) * latitude.z|            // Euclidean length of the rgb vector
out  = lit
     + albedo * v7 * cubeD(N, lod lightDiffuseMapLod).rgb * (fDiff * lightDiffusePower)
         * (1 - sat(len * lightDiffuseInfluence))
     + cubeR(Rv, lod reflectMapLod).rgb * refl * fSpec * refAlphaRestrain
         * (1 - sat(len * reflectMapInfluence))
     + specCol * fSpec * spec * (1 - sat(len * specularInfluence))
alpha = s.a * v0.a
```

The tail, in order (V):

```
shadow = shadowColor.w * shadowFadeParam.z * (1 - litFraction) * (1 - fade)
sf     = lerp(1, shadowColor.rgb, shadow)
c      = out * sf
c      = lerp(c, fogColor.rgb, min(fogColor.a, v6.w))
c      = c * modulateColor.rgb
c      = (c * latitude.x + latitude.y) * latitude.z
rgb    = c * glareLdrScale.x            alpha = alpha * modulateColor.a
```

The shadow term is four taps of `shadowMap0` at the projected xy plus
`shadowOffset0[k].xy`. A tap is lit when the stored depth minus
`(proj.z + shadowOffset0[k].z)` is at or above 0, where `proj.z` is the
undivided z of the projection. `litFraction` is lit taps over 4 and
`fade = sat((v2.z - shadowFadeParam.x) / shadowFadeParam.y)`, so shadow strength
falls to zero with distance.

The reflection term of the main pass multiplies the cube sample by
`refAlphaRestrain`. A second cloth family (`clothCubeAlphaPixel000` in the
manifest) multiplies by `lerp(cubeR.a, 1, refAlphaRestrain)` instead, so the
reflection cube alpha gates the term (V).

## The two passes

Each material ships `_000` and `_001` pixel programs. `_001` is the same math
without the shadow term (no `shadowMap0`, no shadow constants), with two point
lights instead of four and a smaller instruction count in every family (V). The
pass is chosen by bit 0 of the pixel permutation flags, which `0x00C3D790` builds
from the draw context byte at `C+0x770` (setter `0x00C479E0`, getter
`0x00C47A70`) together with the caller's flags. The mesh draw tag at `0x00C439C0`
calls the setter at `0x00C43B16` with the pushed register `ebx` under conditions
of `0x00C47BF0`, `0x00C47AA0` and a record flag. The value of `ebx` is not
traced (U), so whether this call clears the byte is I. Which bit value selects `_000` is U.

## Other families

These are decoded and pinned and share the tail above. The details below are
summaries (I) except where marked.

- Body skin `ts` and `ds` (V structure): the Lambert term is replaced by a
  `lightToneMap` lookup per light, see
  [How lightToneMap uses the ramp](chara-global-texture-names.md#how-lighttonemap-uses-the-ramp-v).
  The coordinate is `u = sat(0.5 - 0.5 * dot(N, -L)) * 0.96775 + 0.015625` and
  `v = mask * 0.5 + 0.25` where the mask is `_sampler_00.b`. The ramp colour is
  saturated, multiplied by the light colour and summed, then multiplied by the
  albedo. The cube and specular terms are as in cloth. The `EnableShadowFlag`
  lanes enter through `max(1, lane)` and do not change `u`.
- Face skin `fs`: the same ramp structure with the face albedo texture. Its
  `reflectMap` is the per-instance cube of the face resource, not a DrawEnv cube.
- Hair and brow `hh`, `fh`: albedo is `_s.r * diffuseColor` (grey), opacity is
  `_s.g`, and the exponent is `lerp(shininess, multiShininess, mask)` used
  directly without the divisor. The per-light diffuse is
  `pow(sat(fold(dot(N,-L) + velvetParam.x) * exLightParam.x + exLightParam.y), exLightParam.z)`
  and the specular lobe is `pow(fold(sat(dot(Rv,-L)) + velvetParam.y), exponent)`
  times `1 - (1 - diffuse)^8`, where `fold(t)` is `2 - t` for `t` at or above 1.
  Every c001 hair and brow block carries `exLightParam (0.5, 0.5, 3, 0)` and
  `velvetParam (0, 0.5, 0, 0)`.
- Iris `fi`: albedo is `_s.r * diffuseColor`, the specular colour is
  `2 * _s.g * specularColor`, the exponent is `_s.b * shininess`, the reflectivity
  is `_s.g * reflectivity` and the program has three fresnel bias constants.
  The iris `reflectMap` is the per-instance cube described in
  [Chara global texture names](chara-global-texture-names.md#whether-the-mapping-also-feeds-reflectmap-v).

## Material parameter census

Parsed from the PRAM blocks of the 994 c001 `_mdl/0001` files (871 blocks).

| Row | Value | Blocks |
|---|---|---|
| `lightDiffusePower` | 2.5 | 805 |
| `lightDiffuseInfluence` | 0.58 | 805 |
| `lightDiffuseMapLod` | 6.3 | 805 |
| `reflectMapInfluence` | 0.58 | 805 |
| `fresnelLightDiffBias` | 0.6 | 805 |
| `refAlphaRestrain` | 1.0 | 805 |
| `specularInfluence` | 0.58 or 0.26 | 789 and 16 |
| `fresnelExp` | 2.0 or 2.5 | 789 and 16 |
| `normalPower` | 1.0 or 0.5 | 789 and 16 |
| `reflectMapLod` | 4.0, 1.0 or 2.5 | 797, 7 and 1 |
| `modulateColor` | (1, 1, 1, 1) | 871 |
| `glareLdrScale` | (1, 0, 0, 0) | 871 |
| `fogColor` | (1, 1, 1, 0) | 871 |
| `ambientLightColor` | (0, 0, 0, 0) | 871 |
| `EnableShadowFlag` | (0, 0, 0, 0) | 806 |
| `shininess` | 0.02 to 160, most common 1.0, 0.3, 0.4 | 871 |
| `multiShininess` | 0.02 to 4 | 791 |
| `reflectivity`, `specularColor` | 0.097 to 3.0 and 0.097 to 2.0 per channel | 805 and 871 |
| `diffuseColor`, `multiDiffuseColor` | 0 to 1.1 and 0.05 to 1.3 per channel | 871 and 805 |
| `ambientColor` | rgb 0 except (60, 20, 0), (2.2, 2.2, 2.2), (2.4, 0.3, 0.3), (3.5, 1.896, 1.575) and (0.34, 0, 0) | 801 plus 65 with only alpha 1, and 5 |

The 66 blocks without the light rows carry `shininess 12` and are not lit
character materials. The values are authored per material, so a consumer reads
them from the PRAM block and does not substitute defaults. The pixel program
CTAB defaults (`diffuseColor 0.5`, `lightDiffusePower 0.67`) are never used for
a PRAM-carrying material.

## For a consumer of this contract

1. Take the program formulas above as the target and compare against them
   before changing any single term.
2. Keep every term unclamped until the end and clamp once to the 8-bit target.
3. Build `ambientLightColor` as `(block + running / N) * 0.5`, `DirLightColors`
   as `colour * intensity`, and give the vertex term `(1 + vertex point
   lights) * vertex colour` its place in front of the whole lit colour.
4. Treat shininess as a divisor input: the authored exponent is
   `shininess / (s.g^2 - s.g + 0.26)`, so a value of 0.3 is an exponent between
   1.2 and 30.
5. Sample `lightDiffuseMap` at the perturbed normal with the authored LOD and
   `reflectMap` at the reflection vector. A missing cube is a missing term, not
   a white one.
6. Apply no sRGB conversion to textures or to the output, and expect the final
   quantisation described in [Present blit gamma and dither](present-gamma-blit.md).

## Unknowns

- The runtime light colours, intensities, tints and ambient block for a
  character draw in a given zone, weather and hour (curve values are not
  recorded).
- The default node colour and tint of a character model and the code that sets
  them.
- Which bit value of the pixel permutation selects `_000`.
- The runtime values of the shadow constants for a character draw.
- The texels of the tone ramp and of the two cubes.
- Full transcriptions of the skin, face, hair and iris programs beyond the
  structure above. The pins in the manifest fix the decoded text for a
  follow-up.
