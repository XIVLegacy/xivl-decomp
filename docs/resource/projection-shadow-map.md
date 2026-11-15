# Projection shadow map operands and defaults

The main light shadow of the pinned client is one depth map that a caster
pass fills and the receiver pixel programs sample through `shadowMap0`. This
page records the values behind the receiver driver's operand blocks, the
settings reset and the setters it applies, the cascade setup of the shadow
effect, the default map size, the shadow transform with its cascade
ranges, light box, trapezoid fit and receiver sampler, the two caster
programs with their depth
bias, the weather ScreenEnv Shadow section that the applier copies into the
settings, the caster selection mask and its bit setters, the serialized
byte behind a BG model's caster bit, and the DrawEnv name test of the
weather manager. The [Model Pixel shadow parameters](../model-pixel-shadow.md) page owns
the receiver handles and the arithmetic of `0x00C8BA20`. The
[cloth pixel program](chara-lighting-model.md#cloth-pixel-program) section
owns the receiver tap and fade expression of a shipped program.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` disassembly and
cross-reference queries, and reads of the file-backed PE sections for
constants, vtables and the embedded program bytes. Every lane, offset and
constant below was read from instruction text or section bytes. Everything
here is static.

O is the receiver payload object that `0x00C67920` passes to `0x00C8BA20`
and `P = O+0x51C`, as on the
[Model Pixel shadow parameters](../model-pixel-shadow.md#shadow-names-and-payload-sources)
page. The context setters below take their receiver in ECX and write fields
at the same offsets from it.

## Receiver operand blocks

STATIC. With bit `0x01` of `0x01376028` clear, `0x00C8BBC4..0x00C8BC9A`
sets the bit and builds the block `0x01375FE8` through `0x004201A0` from four
rows in argument order. XMM2 holds 0.5 (`0x00F62F60`, loaded at
`0x00C8BB20`), XMM0 holds 0, and the other lanes come from 1.0
(`0x00F54F70`) and -0.5 (`0x00F62FE8`):

| Row | Lanes |
| ---: | --- |
| 0 | 0.5, 0, 0, 0 |
| 1 | 0, -0.5, 0, 0 |
| 2 | 0, 0, 1, 0 |
| 3 | 0, 0, 0, 1 |

With bit `0x02` clear, `0x00C8BCCE..0x00C8BD76` sets it and writes four
rows from `0x01375FA8` with -0.5, -1.5 (`0x01106790`), 0.5 and 1.5
(`0x00FA2CB4`):

| Row | Lanes |
| ---: | --- |
| 0 | -0.5, -1.5, 0, 0 |
| 1 | -1.5, 0.5, 0, 0 |
| 2 | 0.5, 1.5, 0, 0 |
| 3 | 1.5, -0.5, 0, 0 |

With bit `0x04` clear, `0x00C8BD86..0x00C8BE26` writes a second pattern from
`0x01375F68` with -1.0 (`0x00F62F80`), -2.0 (`0x00FA5134`), 1.0 and 2.0
(`0x00F62F64`): (-1, -2), (-2, 1), (1, 2), (2, -1), lanes 2 and 3 zero.
The offset loop loads `0x01375FA8` for both accepted values 0 and 1 of
`P+0x208` (`0x00C8BE2E..0x00C8BE46`). The only cross-reference to
`0x01375F68` is the write at `0x00C8BDCE`.

INFERRED. Read with the
[conditional matrix and offset processing](../model-pixel-shadow.md#conditional-matrix-and-offset-processing),
`shadowProjMatrix0..3` is the cascade matrix times a block that halves x,
halves and negates y and keeps z and w, and `shadowOffset0[i]` is
`(P+0x1F4 / size) * row_i + (0.5, 0.5, 0, 0)`.

## Context setters

STATIC. These setters write the context fields that the receiver driver
reads through O and P:

| Setter | Field | Receiver read |
| --- | --- | --- |
| `0x00C487A0` | colour, 16 bytes at `+0x1D4` | `shadowColor` source C |
| `0x00C47B60` | enable byte `+0xB9C` | |
| `0x00C47C00` | dword `+0x51C` | P+0 |
| `0x00C48060` | low byte of `+0x51C` | caster mode |
| `0x00C47DB0` | cascade count `+0x520` | P+4 |
| `0x00C47DC0` | split `+0x634 + 4 * i` | P+0x114 + 4 * count, at P+4 equal to 1 |
| `0x00C47CE0` | bias factor `+0x644` | |
| `0x00C47D50` | slope `+0x64C` | |
| `0x00C47CC0` | cascade bias scale `+0x650` | |
| `0x00C47FC0` | fade range `+0x70C` | P+0x1F0 |
| `0x00C480B0` | offset radius `+0x710` | P+0x1F4 |

The getters `0x00C47D20`, `0x00C47D30` and `0x00C47D40` load `+0x650`,
`+0x644` and `+0x648`. `0x00C47F90` returns whether the low byte of `+0x51C`
equals 2.

## Settings reset and apply

STATIC. The reset `0x00A5B6F0` writes these defaults into its settings
object S. `0x00A5AF60` applies them when bit `0x40` of its flag argument is
set (`0x00A5B302..0x00A5B506`), through ShadowEffect setters that forward to
the context setters above.

| S offset | Default | Constant | Apply call | Context setter |
| --- | --- | --- | --- | --- |
| `+0x580` | 0.5, 0.5, 0.5, 0.7 | `0x00F62F60`, `0x00FB9310` | `0x00A58AE0` | `0x00C487A0` colour |
| byte `+0x530` | 1 | | `0x00A58930` | `0x00C47B60` enable |
| `+0x534` | 0.002 | `0x0109C414` | `0x00A58A60` | `0x00C47CE0` bias factor |
| `+0x538` | 2 | | `0x00A58EF0` | `0x00C48060` caster mode |
| `+0x53C` | 1.0 | `0x00F54F70` | `0x00A58AA0` | `0x00C47D50` slope |
| `+0x540`, `+0x544` | 0.2 | `0x00FA4978` | `0x00A58E60` with 0 and 1 | |
| `+0x548` | 16.0 | `0x00FA2B8C` | `0x00A58EB0` | `0x00C47FC0` fade range |
| `+0x54C` | 10.0 | `0x00FA95A0` | `0x00A58F30` | |
| `+0x550` | 12.0 | `0x00FEB198` | `0x00A58F70` | |
| `+0x554` | 0.5 | | `0x00A59060` | `0x00C480B0` offset radius |
| `+0x558` | -1.0 | `0x00F62F80` | `0x00A590A0` | |
| `+0x55C` | 1.0 | | `0x00A590E0` | |
| `+0x560`, `+0x570` | zero vectors | | `0x00A58B20`, `0x00A58B60` | |
| `+0x590..+0x598` | 12.0, 0, 0 | `0x00FEB198` | `0x00A58C00` with 0, 1, 2 | `0x00C47DC0` splits |
| byte `+0x532` | 0 | | `0x00A58BA0` | |

The value stores are at `0x00A5B82C..0x00A5B90D`. Fields without a context
setter in the table were not followed past their ShadowEffect setter.

## Cascade setup and map size

STATIC. `0x00A59220` calls `0x00C47C00(0x100)`, `0x00C47DB0(1)` and
`0x00C487A0` with (0.4, 0.4, 0.4, 0.5) (`0x00F62F7C`, `0x00F62F60`), then
passes its `+0x14` with index 0 and its `+0x18` with index 1 to `0x00C47C80`
(`0x00A5923E..0x00A59295`). These values precede any settings apply.

The ShadowEffect constructor `0x00A592E0` copies `+0x30` of the
configuration object that `0x01323914` points to into its own `+4`
(`0x00A592E9..0x00A592FF`). `0x00401650` constructs that object at
`0x01323748` through `0x00406AB0` and stores its address in `0x01323914`
(`0x0040168D..0x004016B1`). `0x00406AB0` writes `0x400` to `+0x30`
(`0x00406AEE`, `0x00406B1B`). A runtime writer of `+0x30` was not traced.

INFERRED. The default shadow map is 1024 texels square, and the client
starts with one cascade.

## Shadow transform update

STATIC. `ShadowTransform::Update` is `0x00C621D0`, named by the assert
string `SQEX::CDev::Engine::Dw::Renderer::ShadowTransform::Update` at
`0x010F6AE8`. It takes the context C as its argument and writes into its
receiver T.

- It calls `0x00C47F50(0)`, which writes byte C `+0x77B`.
- With byte C `+0x684` set (`0x00C47E30`) the light direction is C `+0x664`
  (`0x00C47E40`). With it clear the direction is C `+0x674`
  (`0x00C47E70`), and the normalized copy is also stored at T `+0x230`.
  Either is normalized through `0x0042EAD0` and stored at T `+0x220`.
- It copies the 32 bytes at C `+0x738` (`0x00C47E80`) to T
  `+0x200..+0x21F`, the view matrix C `+0x20` (`0x00C47930`) to T `+0x180`
  and the projection C `+0x60` (`0x00C47940`) to T `+0x1C0`.
- With `m` the projection copy, `T+0x244 = m[14] / (m[10] - 1.0)` and
  `T+0x248 = m[14] / (m[10] + 1.0)` (`0x00C62346..0x00C62384`, double 1.0 at
  `0x00F598A0`).
- The second byte of C `+0x51C` (`0x00C47C30`) selects the next call: 0
  calls `0x00C61260` once (`0x00C623A2`), 1 calls `CalcCascadeMatrix`
  `0x00C61F00` (`0x00C623C5`), and any other value asserts.

`0x00C47C00(0x100)` in the [cascade setup](#cascade-setup-and-map-size)
writes that dword, so the cascade path is the one the setup selects.

INFERRED. T `+0x244` and `+0x248` are the camera near and far distances of
a GL-style projection.

## Cascade ranges

STATIC. `CalcCascadeMatrix` `0x00C61F00` asserts
`renderState.GetMultiShadowCount() > 0` (`0x010F6C28`) on the count C
`+0x520` (`0x00C47DE0`). For cascade i the near distance is T `+0x244` for
i = 0 and split i - 1 otherwise, and the far distance is split i, the
splits being C `+0x634 + 4i` (`0x00C47DF0`). The last far distance adds the
fade range C `+0x70C` (`0x00C47FF0`, `0x00C61FC4..0x00C61FCD`).

When C `+0x65C` (`0x00C48100`) is at least 0.0 (`0x00FB7A64`) it replaces
the first near distance (`0x00C61FFD`). When that value also exceeds the
first far distance the client logs `Shadow Warning near=%f, far=%f`
(`0x010F6C50`). The settings reset stores -1.0 at S `+0x558`, whose apply
call `0x00A590A0` is listed under
[Settings reset and apply](#settings-reset-and-apply).

The loop at `0x00C62090..0x00C620B7` stores each range in T `+0x244` and
`+0x248` and calls `0x00C61260(out, i, C)`.

## Light box and fit modes

STATIC. `ShadowTransform::CalcShadowMatrix` is `0x00C61260`, named at
`0x010F6B50`.

- The centre c of the bounds T `+0x200..+0x21F` comes from `0x00BE5980`
  and r is the length of max minus c. d is r, or 200.0 when r is below the
  double 200.0 at `0x010F6E10` (float `0x00FD25CC`). The box call is
  `0x0043A080(out, -r, r, -r, r, -d, 2d)` (`0x00C61402`, double 2.0 at
  `0x00F63028`).
- The light view comes from `0x007DF590` (`0x00C6169F`). Its up vector
  changes when the absolute y of the direction exceeds the double
  0.9999899864196777 at `0x010F6E08` (`0x00C61623`).
- The low byte of C `+0x51C` is the mode. Mode 0 takes the branch at
  `0x00C6174C`. Mode 1 takes `0x00C61912`, which scales by the double 0.8
  at `0x00FA55D0` and calls `0x00C8B530`. Mode 2 calls the fit
  `0x00C60BE0` (`0x00C61E22`) with the near and far distances T `+0x244`
  and `+0x248`, the focus fraction C `+0x700 + 4i` (`0x00C47FE0`) and an
  out byte. When the out byte is 1 it calls `0x00C47F50(1)`. Any other
  mode asserts at `0x00C61E4A` with line `0xD8`.
- `0x0043A080` calls the orthographic builder `0x004393B0` at
  `0x0043A1A0`, which loads the double -2.0 at `0x00F66328` for its z
  scale (`0x00439457`).

The [reset table](#settings-reset-and-apply) writes 2 to S `+0x538`, whose
apply `0x00A58EF0` reaches `0x00C48060`, the writer of that low byte. The focus fraction setter `0x00C47FA0` writes C
`+0x700 + 4i`, and its only caller is `0x00A58E7A` in `0x00A58E60`, the
apply of S `+0x540` (i = 0) and `+0x544` (i = 1) whose reset default is
0.2 (`0x00FA4978`).

## Trapezoid fit

STATIC. `0x00C60BE0` clears its out byte (`0x00C60C0C`) and calls
`0x00C5C350` (`0x00C60C6F`) with the projection, the view, the focus
fraction and the near and far distances, then `0x00C5C6B0` (`0x00C60C7C`).
The out byte is set to 1 when `0x00C5C6B0` returns 0 (`0x00C60EFF`,
`0x00C60F14`). With the out byte 1 it calls `0x00C5D1C0` (`0x00C60F45`),
otherwise the box fit `0x00C5C8F0` (`0x00C60F58`). `0x00C5D1C0` has this
one caller.

`0x00C5C6B0` takes a vector through the matrix twice (`0x0042F210` at
`0x00C5C6C9` and `0x00C5C7A6`) and compares each result with 0.0
(`0x00F62F70`).

In `0x00C5D1C0`:

- the float 4.0 at `0x00FC3328` (`0x00C5D43D`) averages four points and
  the float 2.0 at `0x00F62F64` (`0x00C5D7FA`) halves a sum
- the floats -1e16 at `0x010F6DF8` and 1e16 at `0x010F6DFC` seed the
  minimum and maximum searches (`0x00C5DCE8`, `0x00C5DCF0`, `0x00C5DF92`,
  `0x00C5DFC7`, `0x00C5E330`)
- with lambda the stored extent and delta the focus value minus the stored
  minimum, `0x00C5DDF3..0x00C5DE46` computes
  `lambda * delta * (1.0 + 0.0) / ((1.0 - 0.0) * lambda - 2.0 * delta)`
  (doubles `0x00F598A0`, `0x00F63028`) and adds it and the absolute value of
  the minimum to the translation float at `+0x34` (`0x00C5DE75`)
- the double -0.5 at `0x00F599C8` scales a shear term (`0x00C5E179`)

After the fit, `0x00C60BE0` computes `mid = min + (max - min) * 0.5` over
its points (`0x00C6100E`, double `0x00F59898`) and calls
`0x0043A080(out, 1, -1, 1, -1, -500 - mid, 500 - mid)` (`0x00C61074`,
doubles -500.0 at `0x010F6E00` and 500.0 at `0x00FF1318`, -1.0 at
`0x00F62F80`). Four `0x0042EC50` calls at `0x00C61108..0x00C61153` combine
that matrix with the fitted one.

INFERRED. `0x00C5C6B0` returns 1 when the light direction projects inside
the view, so the trapezoid fit runs while the light is outside the view
and the box fit runs otherwise. The expression above is the trapezoid warp
parameter with the focus point mapped to 0. The depth of the fitted map is
linear along the light over 1000 units centred on the fitted points, with
the z row of the depth box replacing the warp's z.

## Receiver sampler

STATIC. The RTTI catalog gives
`SQEX::CDev::Engine::Dw::Renderer::shaderDriver::ShadowMapShaderDriver` the
4-slot vtable at RVA `0xcd4684` (VA `0x010D4684`): `0x00C2D540`,
`0x00C675E0`, `0x00C67920` and `0x0053C440`.

`0x00C67920` tests `0x00C47A70`. When it is true, for each cascade i below
the count C `+0x520` and below 2, it binds the cascade texture
(`0x00C47CA0(i)` virtual `+0x10`) to sampler `s = 10 + i` through
`0x00418410` and calls:

| Call | Device writes | Values |
| --- | --- | --- |
| `0x00418300(s, 3, 3, 0, 7)` at `0x00C679AD` | `0x0041CC60`, sampler states 1, 2 and 3 through the table `0x00F59750` (1, 2, 3, 4) | ADDRESSU and ADDRESSV 4 (BORDER), ADDRESSW 1 (WRAP) |
| `0x00418380(s, 0, 0xC, 1)` at `0x00C679B9` | `0x0041BE40` | not decoded |
| `0x004183C0(s, 1, 1, 0, 0)` at `0x00C679DF` | `0x0041CD00` | MAGFILTER 1 and MINFILTER 1 when `0x004181C0` returns 0 to 3, then MAXMIPLEVEL 0, MIPMAPLODBIAS 0.0 and MIPFILTER 0 for the fourth argument 0 (`0x0041CE22..0x0041CE59`) |
| `0x00418350(s, 0xFFFFFFFF)` at `0x00C679FB` | `0x0041CCC0`, sampler state 4 | BORDERCOLOR white |

The state numbers are the Direct3D 9 sampler state values. `0x0041CD00`
writes MIPFILTER 2 before the fourth-argument test and 0 after it.

INFERRED. `shadowMap0` is sampler 10, point sampled with no mip levels, and
a tap outside the map reads the white border.

## Caster programs and depth bias

STATIC. The per-cascade loop of `0x00C270C0` stores
`[0x010D4250 + 4 * i]` through `0x00C47CC0` (`0x00C27824..0x00C27831`). The
three floats there are 1.0, 1.2 and 2.0. It picks the caster program with
`0x00C47F90` (`0x00C278B1`): mode 2 takes `[0x01374E6C]+4` through
`0x00C5B7A0`, any other mode `[0x01374E6C]` through `0x00C5B790`.

`0x00C5B7D0` fills that pair (`0x00C5B7EE..0x00C5B852`):

| Entry | Bytes | Size | Constant table names |
| ---: | --- | --- | --- |
| 0 | `0x010F60B0` | `0x2E0` | `diffuseSampler`, `shadowBufferZBias`, `uvIndex`, `viewProjectionMatrix`, `worldViewMatrix` |
| 1 | `0x010F6390` | `0x240` | `diffuseSampler`, `shadowBufferZBias`, `uvIndex`, `worldViewProjectionMatrix` |

Both blobs start with the `ps_3_0` version token and hold a `CTAB` comment
with these names and the compiler string `Microsoft (R) HLSL Shader Compiler
9.26.952.2844`.

The RTTI catalog gives `DepthBufferTrapezoidSceShaderDriver` the vtable at RVA
`0xcf665c` (slots `0x00C5B9F0`, `0x00C5BA10`, `0x00C5BB70`) and
`DepthBufferShaderDriver` the one at RVA `0xcf6670` (slots `0x00C5B9F0`,
`0x00C5BAA0`, `0x00C5BCE0`).

`0x00C5BB70` takes the factor from `+0x648` when that is not negative and
from `+0x644` otherwise (`0x00C5BC6E..0x00C5BC86`), multiplies it by
`+0x650`, then by the binary64 100000.0 at `0x010F67D8` and divides by the
binary64 16777215.0 at `0x00F59890` (`0x00C5BC91..0x00C5BCB9`) before the
submit at `0x00C5BCC3`. With the defaults, the factor 0.002 and scale 1.0
give `200 / 16777215`.

`0x00C5BCE0` reads `+0x644`, `+0x648` and `+0x650` and passes the slope
`+0x64C` (getter `0x00C47D70`) to `0x004182C0` (`0x00C5BD55..0x00C5BD5E`),
which reaches `0x0041CC00`. That routine sets render state `0xAF` to its first
float and render state `0xC3` to its second float divided by 16777215.0
(`0x0041CC17`, `0x0041CC2A..0x0041CC47`).

INFERRED. `0xAF` and `0xC3` are 175 and 195, the D3D9 values of
`D3DRS_SLOPESCALEDEPTHBIAS` and `D3DRS_DEPTHBIAS`. The default caster mode 2
draws program 1 with the shader bias of `0x00C5BB70`, and the slope reaches
the device only through the `0x00C5BCE0` driver.

## Weather ScreenEnv Shadow section

STATIC. The ScreenEnv applier `0x00A73BE0`, called from `0x007DD3F0` at
`0x007DD3FF` and `0x007DD413`, takes S as its first argument and the
ScreenEnv receiver as its second. It reads a version byte through
`0x00A94B70` on the object that the receiver's virtual `+0x20` returns and
calls the settings reset `0x00A5B6F0` on S before copying any section. For
a version of 4 or later (`CMP` with 4 at `0x00A73E44`) and a true
"has Shadow" getter it copies the section into S
(`0x00A73E51..0x00A73FC4`):

| ScreenEnv vtable slot | Getter | Node field | S field |
| --- | --- | --- | --- |
| `+0x17C` | `0x00AB0100` | the node pointer is nonzero | gate |
| `+0x184` | `0x00AB0120` | bit 0 of byte `+0x5C` | byte `+0x530`, the other rows run only when it is set |
| `+0x194` | `0x00AB0320` | bit 2 of `+0x5C` | byte `+0x531` |
| `+0x1C4` | `0x00AB0170` | bit 1 of `+0x5C` | byte `+0x532` |
| `+0x18C` | `0x00AB0250` | float `+0x1C` | `+0x534` |
| `+0x1CC` | `0x00AB01C0` | `+0x10..+0x18`, w = 1.0 | `+0x560` |
| `+0x19C` | `0x00AB0280` | `+0x20`, `+0x24`, `+0x28`, `+0x34` | `+0x580` |
| `+0x1A4` | `0x00AB0370` | float `+0x38` | `+0x590` |
| `+0x1AC` | `0x00AB03A0` | float `+0x3C` | `+0x594` |
| `+0x1B4` | `0x00AB03D0` | float `+0x40` | `+0x598` |
| `+0x1BC` | `0x00AB0400` | float `+0x44` | `+0x59C` |
| `+0x1DC` | `0x00AB0480` | float `+0x2C` | `+0x53C` |
| `+0x1E4` | `0x00AB04B0` | float `+0x30` | `+0x540` |
| `+0x1EC` | `0x00AB04E0` | float `+0x48` | `+0x548` |
| `+0x1F4` | `0x00AB0510` | float `+0x50` | `+0x54C` |
| `+0x1FC` | `0x00AB0540` | float `+0x54` | `+0x550` |
| `+0x204` | `0x00AB0570` | float `+0x58` | `+0x554` |
| `+0x20C` | `0x00AB05A0` | float `+0x4C` | `+0x544` |
| `+0x1D4` | `0x00AB0430` | bit 3 of `+0x5C` | byte `+0x5A0` |
| `+0x214`, version 7 or later | `0x00AB05D0` | float `+0x60` | `+0x55C` |

The slots are those of the `RaptureScreenEnvBaseObject` vtable
`0x01061BAC` (RTTI catalog RVA `0xc61bac`). Each getter calls its
receiver's virtual `+0x20` and reads the node that the returned object
holds at `+0x20`. The debug dump `0x00AB1980` prints the strings
`Shadow Enabled: %s` (`0x010A78E0`), `Shadow DirFixed: %s` (`0x010A78CC`),
`Shadow Bias: %f` (`0x010A78BC`), `Shadow Dir: (%f, %f, %f)`
(`0x010A78A0`) and `Shadow Color: (%f, %f, %f, %f)` (`0x010A7880`) after
the slots `+0x184`, `+0x1C4`, `+0x18C`, `+0x1CC` and `+0x19C`.

INFERRED. Read with the reset table above, a weather ScreenEnv with a
Shadow section replaces the colour (`+0x580`), the first split
(`+0x590`), the fade range (`+0x548`), the offset radius (`+0x554`) and
the bias factor (`+0x534`) of the defaults, and a clear bit 0 of node
`+0x5C` leaves the enable byte `+0x530` zero.

Reproduce with `ghidra disasm 0x00a73be0 -n 420`, the vtable cells from
`ghidra memory read 0x01061d28 164`, `ghidra disasm` of each getter, and
`ghidra x-ref to 0x010a78e0`.

## Caster selection

STATIC. The caster pass of `0x00C270C0` requests its draw list with
`0x00C38AD0(0x11)` at `0x00C27302`. Unless byte `0x0136EB11` is set, which
sends the request to `0x00C71190`, that query keeps every drawable of its
list whose dword `+0xA0` holds every bit of the argument (`AND` and `CMP`
at `0x00C38B6F..0x00C38B77`), so the pass takes drawables with bits `0x01`
and `0x10` set. The pass draws the same mask per cascade through
`0x00C26580(0x11)` at `0x00C278DB`.

Before the cascade loop the pass measures each listed drawable's bounding
sphere against a centre point and keeps those whose distance minus radius
is at most 500.0 (`0x00FA4960`, compare at `0x00C27683`). It unions their
bounds with `0x00857440` at `0x00C276A2` and hands the union to
`0x00C48810` at `0x00C276FB`. When no drawable qualified it calls the
enable setter `0x00C47B60(0)` at `0x00C2770F`.

The drawable constructor `0x00C2EA70` writes `0x3D` to `+0xA0` at
`0x00C2EBE4`, so bits `0x01` and `0x10` are set from construction. Five
setters toggle one bit each, writing `flags ^ bit` when the bit's current
state differs from the Boolean argument:

| Setter | Bit |
| --- | --- |
| `0x00C2E210` | `0x04` |
| `0x00C2E240` | `0x08` |
| `0x00C2E270` | `0x2000` |
| `0x00C2E2A0` | `0x10` |
| `0x00C2E2D0` | `0x20` |

The four direct callers of `0x00C2E2A0` reach the drawable through their
object's virtual `+0x11C` and then that result's virtual `+0x148`:

| Caller | Table entry in the RTTI catalog |
| --- | --- |
| `0x007CDC60` | slot 128 (`+0x200`) of `RaptureActor` `0x00FEA50C`, `BgModelActor` `0x00FBA43C`, `WeaponActor` `0x01057EE4`, `CDevActor` `0x00FBC03C` and the other actor tables of that family |
| `0x00A08230` | slot 28 (`+0x70`) of `RaptureCutCharacterListener` `0x00FBE8BC` and `CutCharacterListener` `0x01094F44` |
| `0x00625DA0` | slot 11 (`+0x2C`) of `Application::Scene::Actor::Map::LayDefaultModelObject` `0x00FBAD6C` |
| `0x00A75B70` | slot 11 (`+0x2C`) of `Lay::Default::External::Dw::LayDefaultModelObject` `0x0109EF4C` |

`0x007D1980` calls `0x007CDC60` at `0x007D19E8` for the actor at its
`+0x08` when bit 0 of its state byte is set. The argument starts as bit 3
of that byte and is cleared when `0x00443E40(7, 0x1D, 0)` returns zero,
when `0x007CDA60` returns a value of 0 or less, or when dword `0x012C5E38`
is zero (`0x007D1980..0x007D19E0`).

The BG instance setup `0x00AA97C0` writes the caster bit of a BG model from
a serialized byte. [BG caster bit source](#bg-caster-bit-source) owns that
route.

The other callers of `0x00C38AD0` push mask `0x01` (`0x00C264FD`,
`0x00C2689D`, `0x00C26CF6`, `0x00C289DB`, `0x00C2939F`, `0x00C29CFA`,
`0x00C2AA32` and `0x00C2AE9E`) or `0x81` (`0x00C4D329`). `0x00C267A8`
passes the argument of `0x00C26580`, and `0x00C2A41E` passes EDI, which
this read did not trace. No immediate mask holds bit `0x04` or `0x08`.

Reproduce with `ghidra disasm 0x00c38ad0 -n 62`, `ghidra disasm 0x00c270c0
-n 320`, `ghidra disasm 0x00c2e2a0 -n 14`, `ghidra x-ref to 0x00c2e2a0`,
`ghidra x-ref to 0x00c38ad0`, `ghidra disasm 0x007d1980 -n 42` and
`ghidra disasm 0x00aa9930 -n 60`.

## BG caster bit source

STATIC, with the PE section decode of the method section and Capstone
5.0.7 x86-32 decodes of the cited ranges.

`0x00AA97C0` takes a property object P through its instance argument's
virtual `+0xBC` and keeps it when P's virtual `+0x04` returns 1. When P's
virtual `+0xC4` returns null, it runs P's virtual `+0xB8` and then reads
the display model M through P's virtual `+0xC4` (`0x00AA97DE..0x00AA9884`). It calls P's getters `+0x78` through
`+0x90` and passes each result to M's virtual `+0x24` through `+0x3C` in
the same order (`0x00AA9942..0x00AA99D4`). So getter `+0x80` feeds M's
slot `+0x2C`, and getter `+0x8C` feeds slot `+0x38`.

The property tables of the RTTI catalog hold these cells:

| Table | `+0x78..+0x90` | `+0xB8` | `+0xC4` |
| --- | --- | --- | --- |
| `RaptureBGChipInstanceObjectProperty` `0x00FF242C` | `0x00A96930`, `0x00A96960`, `0x00A96990`, `0x00A969C0`, `0x00A969F0`, `0x00A96A20`, `0x00A96A50` | `0x00A97550` | `0x00CB1C80` |
| `RaptureBGPartsInstanceObjectProperty` `0x00FF2534` | `0x00A97840`, `0x00A97870`, `0x00A978A0`, `0x00A978D0`, `0x00A97900`, `0x00A97930`, `0x00A97960` | `0x00A98460` | `0x00CB1C80` |

Each getter calls its own virtual `+0x0C` and then virtual `+0x44` of the
result, and reads one byte of the pointer that returns. Getters `+0x78`,
`+0x7C`, `+0x80` and `+0x84` return bits 0, 1, 2 and 3 of byte `+6`,
`+0x88` returns byte `+4`, `+0x8C` byte `+5`, and `+0x90` bit 4 of byte
`+6` (`0x00A96930..0x00A96A77`, `0x00A97840..0x00A97987`). Getter `+0x80`
is `0x00A96990` and `0x00A978A0`.

`0x00A98460` stores the object that virtual `+0x04` of the manager at
`0x01365B04` returns for argument 0 at P `+0x0C`
(`0x00A9847D..0x00A9848E`), and `0x00CB1C80` returns P `+0x0C`. Creator
`0x0062C430`, a cell of the table at `0x00F93284`, allocates `0x6C` bytes,
constructs them with `0x0062BE80` and returns the object plus 4. The
constructor writes `LayDefaultModelObject` table `0x00FBAD6C` at object
`+4` (`0x0062BED6`). INFERRED: the manager call of `0x00A98460` reaches
this creator, so M's table is `0x00FBAD6C`. Its cells:

| M slot | Method | Effect on the drawable |
| --- | --- | --- |
| `+0x24` | `0x00625D20` | bit `0x04` setter `0x00C2E210` |
| `+0x28` | `0x00625D60` | bit `0x08` setter `0x00C2E240` |
| `+0x2C` | `0x00625DA0` | bit `0x10` setter `0x00C2E2A0`, the caster bit |
| `+0x30` | `0x00625DE0` | bit `0x20` setter `0x00C2E2D0` |
| `+0x34` | `0x00625E20` | `0x00C3B080` |
| `+0x38` | `0x00625E60` | writes `arg & 3` to drawable `+0x2C4` when bit 7 of the byte argument is set |
| `+0x3C` | `0x00625EA0` | calls the model's virtual `+0x84` with the negated Boolean |

Each method reads M `+0x08`, calls its virtual `+0x11C` for the model and
then the model's virtual `+0x148` (`0x006576B0`, model `+0xD8`) for the
drawable, except `+0x38`, which reads model `+0xD8` directly. So the caster
bit of a BG model takes getter `+0x80`, bit 2 of byte `+6`.

The getter's pointer chain. P's virtual `+0x0C` (`0x008002B0`) forwards to
virtual `+0x08` of the subobject at P `+4`, and P's virtual `+0x08`
(`0x00800830`) to that subobject's virtual `+0x04`. Table `0x010A899C`
(`LayoutInstanceObjectPropertyBase`, written by `0x00ABA680`) holds
`0x00ABA630` at `+0x04`, which stores its argument at subobject `+4`, and
`0x008D5570` at `+0x08`, which returns it. Instance slot `0x00A99EB0`
(table cell `0x00FF28DC`) passes the instance interface T, its own address
plus 4, to the property's virtual `+0x08` (`0x00A99ED3..0x00A99EDC`).
So the getter's `+0x0C` call returns T. `RaptureLayoutInstanceObject` table
`0x00FF279C` holds `0x00A99D10` at `+0x44`, `0x00CB1C80` at `+0x3C` and
`0x008D54C0` at `+0x40`. `0x00A99D10` returns the first dword of the
pointer that `+0x3C` returns, T `+0x0C`, or 0 for a null pointer.
`0x008D54C0` stores its argument at T `+0x0C`.

T `+0x0C` has two writers:

- A root instance. Slot 1 `0x007FE900` passes the setup descriptor's
  first pointer plus `0x10` to slot `+0x40` (`0x007FE920..0x007FE935`).
  [UnitTree child creation](bgchip-model-binding.md#unittree-child-creation-and-admission-source)
  places the Instance node in that descriptor, so T `+0x0C` is the
  Instance node plus `0x10`.
- A UnitTree child. `0x007FD830` matches descriptor `+0x08` against child
  record `+0x1C` in the parent Instance node's `+0x34/+0x38` array
  (`0x007FD97A..0x007FD9A1`, match saved at `0x007FDBFC`), and passes the
  matching 12-byte descriptor, or 0, to the new child's slot `+0x40`
  (`0x007FDA74..0x007FDA80`).

INFERRED. A BG model casts the main light shadow when bit 2 of byte `+6`
is set in the node that the first dword of its instance's 12-byte
descriptor references. That descriptor is the Instance node's own
`+0x10..+0x1B` for a root instance, and the parent array entry whose
`+0x08` equals the child ID for a UnitTree child. A child with no match
passes 0, `0x00A99D10` returns 0 and the getter reads address 6. The
reading assumes that the setup runs after the drawable exists. The model
initializer of the [floor model render path](floor-model-render-path.md)
is the lead for that order.

Reproduce with a decode of `[0x00AA97C0,0x00AA99EB]`, the cells above from
the PE sections, decodes of `[0x00A96930,0x00A96A78)`,
`[0x00A97840,0x00A97988)`, `[0x00A98460,0x00A984CE]`,
`[0x0062C430,0x0062C4B1]`, `[0x0062BE80,0x0062BF0B]`,
`[0x00625D20,0x00625ECF]`, `0x008002B0`, `0x00800830`, `0x00ABA630`,
`0x008D5570`, `0x008D54C0`, `0x00A99D10`, `0x00CB1C80`, `0x006576B0`,
`[0x00A99EB0,0x00A99EDF]`, `[0x007FE900,0x007FE935]` and
`[0x007FD960,0x007FDC00]`.

## DrawEnv name test

STATIC. `0x007DCDF0` calls the position query `0x007DB9C0` at `0x007DCE28`
and `0x007DB940` at `0x007DCE53`, and takes the nonzero object O that the
second leaves in its local. It moves the dword `+0x24` to `+0x20` and stores
O at `+0x24`. When O differs from the previous value it reads a name through
O's virtual `+0xA4`, copies it into `+0xC0` through `0x009D29FB`, and sets
byte `+0x1A4` of the object that `0x007E02F0` returns to 1 when
`0x009D6090(name, "dwev_0")` returns zero and to 0 otherwise
(`0x007DCE58..0x007DCEAD`, string at `0x00FEE454`).

`0x007E91D0` scales the tick count at its argument's `+0x10` by 30.0
(`0x00FE5490`) and divides it by 300000.0 (`0x00F67020`). It then moves the
float `+0x1A0` by that step times 0.05 (`0x00FA7AE0`): down while byte
`+0x1A4` is set and up while it is clear (`0x007E9351..0x007E93AF`), and
clamps the result to 0 and 1.0.

INFERRED. O is the DrawEnv at the queried position and `0x009D6090` is a
substring search, so the byte is set for a DrawEnv whose name lacks
`dwev_0`. The readers of `+0x1A0` are not traced here, so no link from
this value to the shadow settings is established.

Reproduce with `ghidra disasm 0x007dce53 -n 30`, `ghidra memory read
0x00fee454 8` and `ghidra disasm 0x007e91d0 -n 400`.

## DirFixed direction and the Shadow blend

STATIC. The apply calls of the [reset table](#settings-reset-and-apply)
for the Shadow section's DirFixed and Dir fields reach these context
setters. Each apply reads the ShadowEffect at `+0x10`, takes its virtual
`+0x18` result plus `0x30` as the context C and tail-jumps to the setter.

| S field | Apply call | Pushed at | Setter | C field |
| --- | --- | --- | --- | --- |
| byte `+0x532` | `0x00A58BA0` | `0x00A5B367..0x00A5B371` | `0x00C47E00` | byte `+0x684` |
| `+0x560` | `0x00A58B20` | `0x00A5B3A4..0x00A5B3B5` | `0x00C47E10` | 16 bytes at `+0x664` |
| `+0x570` | `0x00A58B60` | `0x00A5B3E8..0x00A5B3F9` | `0x00C47E50` | 16 bytes at `+0x674` |

The [ScreenEnv table](#weather-screenenv-shadow-section) copies the
DirFixed bit (slot `+0x1C4`) into byte `+0x532` and the node Dir (slot
`+0x1CC`) into `+0x560`. With the
[shadow transform update](#shadow-transform-update), a set DirFixed makes
the light direction the node Dir, normalized into T `+0x220`.

`ShadowTransform::CalcShadowMatrix` `0x00C61260` scales T `+0x220` by a
scalar it holds on its stack, adds the bounds centre that `0x00BE5980`
returns, and passes that sum and the centre to the light view
`0x007DF590` (`0x00C61416..0x00C6169F`).

INFERRED. The light eye sits on the Dir side of the casters' centre, so a
DirFixed lane's light travels along the negated node Dir. The writer of C
`+0x674` in ordinary play is not traced here.

STATIC. `0x007E2550` has one caller, `0x007E313A` in `0x007E2AF0`. It
takes a first source on the stack, the fraction t as its second stack
argument, a second source in ESI and the receiver in EDI. It returns
unless the virtual `+0x17C` (has Shadow) of all three is true. It then
writes each Shadow getter of the [ScreenEnv table](#weather-screenenv-shadow-section)
into the receiver through the slot 4 bytes above the getter:

- The bytes of slots `+0x184` (enable), `+0x194`, `+0x1C4` (DirFixed) and
  `+0x1D4` take the first source's value while 0.5 (`0x00F62F60`) is
  greater than t, else the second source's (`0x007E261F`, `0x007E2662`,
  `0x007E26E8`, `0x007E2890`).
- The floats and vectors, for example slot `+0x18C` (bias) at
  `0x007E26A5..0x007E26CA` and slot `+0x1CC` (Dir) at
  `0x007E28B9..0x007E2902`, take `first + (second - first) * t`.
- Before the copies it returns unless both sources hold the enable byte
  or both hold the slot `+0x1D4` byte (`0x007E25A4..0x007E25FD`).

The fraction that `0x007E2AF0` passes, and which of the weather and the
spatial fractions drives it, was not traced.

Reproduce with `ghidra disasm 0x00a58b20 -n 30`,
`ghidra disasm 0x00c47e00 -n 30`, `ghidra disasm 0x00a5b302 -n 160`,
`ghidra disasm 0x00c61405 -n 120`, `ghidra x-ref to 0x007e2550` and
`ghidra disasm 0x007e2550 -n 400`.

## Open

- Which of C `+0x664` and `+0x674` holds the sun in ordinary play, and the
  consumer of byte C `+0x77B`.
- The meaning of `0x00418380(s, 0, 0xC, 1)` and the value `0x004181C0`
  returns.
- The role of `DepthShadowFilter` (vtable RVA `0xcfbe58`) in the shadow
  path.
- The direction source in ordinary play, the meaning of the Shadow section
  fields that have no context setter here, and cut scene shadow clips.
- Which ScreenEnv a position selects. The segment and the triangle word
  route are in the
  [position query](weather-sky-fog-environment-path.md#position-query-0x007db9c0)
  section, and category `0xD` is still open.
- The caster byte of a UnitTree group child or of a leaf of a nested
  UnitTree, whose parent array this page does not trace, and the order of
  the BG setup and drawable creation.
- Which objects register as receivers.
