# Projection shadow map operands and defaults

The main light shadow of the pinned client is one depth map that a caster
pass fills and the receiver pixel programs sample through `shadowMap0`. This
page records the values behind the receiver driver's operand blocks, the
settings reset and the setters it applies, the cascade setup of the shadow
effect, the default map size, the two caster programs with their depth
bias, the weather ScreenEnv Shadow section that the applier copies into the
settings, the caster selection mask and its bit setters, and the DrawEnv
name test of the weather manager. The [Model Pixel shadow parameters](../model-pixel-shadow.md) page owns
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

The BG instance setup `0x00AA97C0` calls seven getters of one object,
virtual `+0x78` through `+0x90`, and passes each result to virtual `+0x24`
through `+0x3C` of a second object in the same order
(`0x00AA9942..0x00AA99CE`), so the getter at `+0x8C` feeds slot `+0x38`. INFERRED: for a model whose table
starts at `0x0109EF40` that slot is `0x00A75B70`, the caster-bit forwarder.
The serialized source of the BG property values is not traced.

Reproduce with `ghidra disasm 0x00c38ad0 -n 62`, `ghidra disasm 0x00c270c0
-n 320`, `ghidra disasm 0x00c2e2a0 -n 14`, `ghidra x-ref to 0x00c2e2a0`,
`ghidra disasm 0x007d1980 -n 42` and `ghidra disasm 0x00aa9930 -n 60`.

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
`dwev_0`. No reader of `+0x1A0` in the shadow path
was found.

Reproduce with `ghidra disasm 0x007dce53 -n 30`, `ghidra memory read
0x00fee454 8` and `ghidra disasm 0x007e91d0 -n 400`.

## Open

- The fitted projection of `ShadowTransform::Update` and its depth range.
- Which surface the receivers sample and its sampler state.
- The direction source in ordinary play, the meaning of the Shadow section
  fields that have no context setter here, and cut scene shadow clips.
- Which ScreenEnv a position selects, and which serialized byte clears the
  caster bit of a layout model.
- Which objects register as receivers.
