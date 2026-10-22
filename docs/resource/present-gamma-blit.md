# Present blit gamma and dither

The last stage of a frame copies the scene target to the back buffer with a
dedicated pixel program. The program raises each colour channel to a gamma
exponent and rounds to 8 bits with a 4 by 4 ordered dither. This page records
that stage, the state ids the client does and does not send to the device, and
the default exponent. The lighting that feeds it is in
[Character lighting model](chara-lighting-model.md). Tiers: V = verified from
cited instructions or a decoded program, I = inferred, U = unknown.

## Evidence

Executable `ffxivgame.exe`, build `2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`, all locators VA. The two embedded programs were decoded with
`tools/d3d9_shader_decode.py` and are pinned in `config/chara_lighting_model.json`
(`gammaBlitVertex`, `gammaBlitPixel`). The executable rows are checked by
`python3 tools/verify_chara_lighting_model.py --exe <ffxivgame.exe>`.

## Call path (V)

| Locator | Observation |
|---|---|
| `0x00A52C50` | Per-frame present function. It copies the scene target of the post-effect object to the back buffer. |
| `0x00A52F51..0x00A52F5D` | Full-size path. Pushes the mode word 3 and calls the blit `0x0041F020`. |
| `0x00A53124..0x00A53136` | Second call with mode 3 on the other path of the same function. When the target is not 16 by 9 it clears the target and blits into a centred rectangle first (I for the selection test, which reads `0x00A47140`). |
| `0x0041F020` | Blit. Modes 0 to 2 are `StretchRect` with the filter table at `0x00F59764`. Mode 3 draws a quad with the gamma programs. |
| `0x0041D9E0..0x0041DB41` | Registers the two programs, a vertex program of `0x140` bytes at `0x00F58528` and a pixel program of `0x390` bytes at `0x00F58668`, and resolves the constant `s_gammaCalibration` (string `0x00F5937C`, also `$s_gammaCalibration` at `0x00F586D8`) into renderer `+0x198`. |
| `0x0041F58B..0x0041F5C5` | Reads the float at renderer `+0x17C`, copies it to all four lanes of a 16 byte constant and submits it through the handle at renderer `+0x198`. |

The renderer is the object behind the global `0x01329428`.

## The exponent (V unless marked)

- The exponent is the float at renderer `+0x17C`. Getter `0x0041BB10`, setter
  `0x0041BAE0` (stores only when the value differs).
- The display creation functions `0x0041E830` and `0x0041EA30` (the second
  carries the source name `CreateDisplayNative_`) write the float at
  `0x00F54F70`, which is 1.0, when the field is not already 1.0, at
  `0x0041EA02..0x0041EA23` and `0x0041EBEE..0x0041EC11`.
- `GraphicsManager` (RTTI vftable `0x0109AE70`, slot 3 at `0x00A47100`) reads
  its field `+0x44`, calls the setter, and `0x00A4788E..0x00A47893` loads that
  field from the getter, so the exponent is 1.0 until `GraphicsManager+0x44` is
  written (I for the load order). The writer of that field is U.
- The client has no configuration key or UI string named gamma. The RTTI
  `PostEffectController::GammaCallibration` exists as a one-slot class in the
  `GameManagerActor` post-effect controller family, and whether it reaches this
  field is U.

## The program (V)

The vertex program passes the quad through and transfers the uv. The pixel
program has one sampler `s_colorSampler` (`s0`) and one constant
`s_gammaCalibration` (`c0`). With `T(x, y)` the ordered-dither threshold below,
`(x, y)` the pixel position taken from `vPos`, and `c` one channel of the sampled
colour:

```
v      = |c| ^ gamma                    // log, multiply, exp
scaled = v * 255
whole  = floor(scaled)
bit    = 1 when (scaled - whole) - T(x, y) >= 0, else 0
out    = (whole + bit) / 255
```

Alpha is 1.0, so it scales to exactly 255 and stays 1.0. The threshold is
`rows[y mod 4][x mod 4] / 32` with

| y mod 4 | x mod 4 = 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| 0 | 25 | 17 | 29 | 21 |
| 1 | 9 | 1 | 13 | 5 |
| 2 | 31 | 23 | 27 | 19 |
| 3 | 15 | 7 | 11 | 3 |

The sixteen numerators are the odd values 1 to 31 once each. A channel with a
fraction of 0.5 rounds up in 8 of the 16 cells. The reference functions are
`dither_threshold` and `present_channel` in `tools/verify_chara_lighting_model.py`.

## Consequences

- At exponent 1.0 the blit changes no value by more than one 8-bit level and
  adds no brightness. Given the same scene target as input, a consumer that
  presents without this stage matches the retail blit output within one level,
  plus dither noise. The scene target itself comes from the post-effect object
  whose filters are untraced (U).
- An exponent above 1 darkens and one below 1 brightens, with 0 and 1 fixed.
  The blit input is the scene target of the post-effect object, so the exponent
  acts on whatever that target holds. Its order against the Glare composite and
  the `PostEffectController` filters is I and not shown by the call path.
- The stage does not convert between sRGB and linear space.

## No sRGB state ids (V)

The two device state managers are `0x004236E0` (render state, arguments id and
value) and `0x004236A0` (sampler state, arguments stage, type and value). A scan
of every direct call in the first section finds these immediate ids.

| Manager | Immediate ids | Call sites with the id in a register |
|---|---|---|
| Render state | 7, 8, 14, 15, 19, 20, 22, 23, 24, 25, 27, 52, 57, 58, 59, 154, 156, 168, 171, 174, 175, 181, 185, 190, 191, 192, 193, 195, 206, 207, 208, 209 | 12 |
| Sampler state | 1, 2, 3, 5, 6, 7, 8, 9, 10 | 5 |

The register-fed render sites at `0x0041D120..0x0041D1CD` read ids from the
table at `0x00F597D4`, which holds 189, 56, 186, 53, 187, 54, 188 and 55. The
other register-fed sites push constants set a few instructions earlier. Neither
set contains 194 (`D3DRS_SRGBWRITEENABLE`) or the sampler type 11
(`D3DSAMP_SRGBTEXTURE`). The scan covers the two manager entry points and does
not exclude a direct device call elsewhere.

## Unknowns

- The writer of `GraphicsManager+0x44` and so the exponent after startup.
- The runtime parameters of the `PostEffectController` filters
  (`ColorCorrectionFilter`, `LUTColorCorrectionFilter`, `LatitudeFilter`,
  `FogFilter`, `ShadowFilter`, `SsaoFilter`, `DofFilter`, `BlurFilter` and the
  target-effect filters), whose RTTI is in `config/ffxivgame.rtti.json`. Their
  defaults and activation are not traced here.
- The back buffer format and the scene target format of the final pass. The
  Glare scratch targets are 8-bit, see
  [Render target formats](screen-env-glare-latitude.md#render-target-formats-v-i).
