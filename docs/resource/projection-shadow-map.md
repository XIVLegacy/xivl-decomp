# Projection shadow map operands and defaults

The main light shadow of the pinned client is one depth map that a caster
pass fills and the receiver pixel programs sample through `shadowMap0`. This
page records the values behind the receiver driver's operand blocks, the
settings reset and the setters it applies, the cascade setup of the shadow
effect, the default map size, and the two caster programs with their depth
bias. The [Model Pixel shadow parameters](../model-pixel-shadow.md) page owns
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

## Open

- The fitted projection of `ShadowTransform::Update` and its depth range.
- Which surface the receivers sample and its sampler state.
- The direction source in ordinary play and the runtime writers of the
  settings object, such as weather data and cut scene shadow clips.
- Which objects register as casters and receivers.
