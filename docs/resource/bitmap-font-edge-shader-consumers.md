# Bitmap font flag-bit-0x02 shader consumers

The bitmap dispatch path with flag bit `0x02` set conditionally selects
vertex entry 6 and pixel entry 4. The pixel program combines two texture
samples with two interpolated color fields. Its RGB depends on the first
sample and both color fields. Its alpha depends on the second sample and
second color field's alpha, which the dispatcher replaces from the first
color argument. The property named FontEdge is not joined end to end to
this bitmap dispatch by this finding.

The [plain shader finding](bitmap-font-shader-consumers.md) owns vertex
entry 6 and the flags-bit-`0x02`-clear pixel entry 3.
The [texture finding](bitmap-font-texture-consumers.md) owns selector
packing, batch delivery, and declaration conversion.
The [runtime bridge](bitmap-font-runtime-bridge-consumers.md#br-02-payload-order-and-second-resource-offset)
owns the TEXC second-resource offset and the conditional removal of that
resource on the flag-clear path. These findings identify submitted wrappers,
not the actual textures bound in a live device.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instruction instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |
| Independent token interpreter | Bounded Python shader-model-2 parser and mathematical CPU evaluator |

Input size, SHA-256, PE32 machine, optional-header identity, and image base
were checked before reading file-backed instructions and data. The full
bounded vertex and pixel payloads were independently decoded. No Ghidra
import, stored-program query, REA finding, retail-input CI run, or live
rendering test supplies this finding.

The primary definitions are
[Microsoft win32metadata d3d9types.h](https://github.com/microsoft/win32metadata/blob/75ec935a4f1ac56345daa9f7f0bc21db3e68ac68/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9types.h),
SHA-256 `5ba31e75a3ea03c1a71b48f39d966b6f67967b22bb9e4abcc419a3118269cd6a`,
and Microsoft's
[instruction](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/instruction-token),
[source parameter](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/source-parameter-token),
and [destination parameter](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/destination-parameter-token)
contracts. These define operand lengths, masks, swizzles, modifiers, and
register types. The executable supplies the retail program evidence.

All addresses are VA. Subtract `0x00400000` for RVA. Instructions, payload
identities, and decoded token fields have confirmed confidence within the
bounds. Mathematical formulas interpret those fields under the primary
operation definitions, without claiming bit-exact device execution.

## BS-01: Conditional selection and alpha replacement

In `FUN_00CA55A0`, `0x00CA55A3` through `0x00CA55C1` copy the fifth
explicit color argument's 16 bytes into a local. Flag low-byte bit `0x02`
is tested at `0x00CA55B9`. On its set branch, `0x00CA55D0` through
`0x00CA55D5` replace the local's fourth component from the fourth explicit
color argument's fourth component. The second color's original alpha does
not survive this replacement.

The set branch retains both resource arguments. If state global
`0x01376490` differs from 7, it passes pixel selector 4 and vertex selector
6 to `FUN_00CA2E80` at `0x00CA55EC` through `0x00CA5614`.
When that global already equals 7, this setup call is bypassed. The
[plain shader finding](bitmap-font-shader-consumers.md#selected-program-pair)
owns the conditional creation, selected-object delivery, and unresolved
final context binding. The same indexed pixel array selects entry 4.

| Stage and entry | Pointer locator | Size locator | Payload VA | Byte length | Payload SHA-256 |
|---|---|---|---|---:|---|
| Vertex 6 | `0x0130BB18` | `0x0110934C` | `0x01109E80` | `552`, `0x228` | `e1a5d394adce4f39cba93d4a33dbbfbeaa0314a3301a9da206c3e910f5238295` |
| Pixel 4 | `0x0130BB90` | `0x01109368` | `0x0110AA80` | `440`, `0x1B8` | `88d48d0be4558fa9a9ccb0284c1be66088cd8cf5427725db96d95864954bea18` |

## BS-02: Additional packed color field

Descriptor `0x0130C23C` supplies stream 0, vertex offset `0x10`, internal
format 3, semantic 9, and component argument 4. Converter
`FUN_00419A20` maps the type through byte `0x00F57D9F` and the
semantic through bytes `0x00F57DC6..0x00F57DC8`, producing type 8,
usage 5, and usage index 1. Microsoft's
[D3DDECLTYPE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddecltype)
and [D3DDECLUSAGE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddeclusage)
identify these as `UBYTE4N` and `TEXCOORD1`. The four bytes are normalized
in memory component order by division by 255.

Vertex entry 6 declares input 2 as TEXCOORD1 at payload offset `0x100`,
VA `0x01109F80`. It copies all four input-2 components to coordinate
output 1 at payload offset `0x200`, VA `0x0110A080`, without swizzle,
modifier, or arithmetic. Under the coordinate output/input contract cited
by the plain shader finding, vertex `+0x10` thus supplies pixel input 1.
The existing vertex `+0x0C`, `+0x14`, and `+0x18` forwarding remains
pixel inputs 0, 2, and 3 respectively. The submitted vertex stride is
`0x2C`, as recorded by the texture finding.

| Source field | Submitted vertex offset | Pixel input | Component use in pixel entry 4 |
|---|---|---|---|
| First color argument, packed through `FUN_00CB9640` | `+0x0C` | 0 | RGB |
| Modified second color argument, packed through the same helper | `+0x10` | 1 | RGB and alpha |
| Packed selector vector | `+0x14` | 2 | Same four-component dot selector for both samples |
| Float2 normalized source coordinates | `+0x18` | 3 | Same XY sample coordinates for both samplers |

On the non-null second-resource branch in `FUN_00CB9280`, the fifth
explicit argument's four float components are packed through
`FUN_00CB9640` at `0x00CB947F` through `0x00CB94B5`. Stores
`0x00CB94BA` through `0x00CB94C9` put the result at vertex `+0x10`
in each of four local records, spaced `0x2C` bytes apart. The texture
finding owns that packer's finite-component clamp, scaling, and truncation
and the subsequent batch submission. This is conditional packed-field
delivery. The first color packing occurs at `0x00CB939B` through
`0x00CB940B`, with stores `0x00CB9448` through `0x00CB9457` at
vertex `+0x0C`. Both color fields therefore undergo the packer's finite
component clamp, multiply by 255, and integer truncation before UBYTE4N
normalization. Caller float colors and interpolated shader colors are distinct
stages. No runtime second-resource identity or final stage binding is proven.

## BS-03: Complete pixel interpretation

The version token identifies pixel shader 2_0. The parser consumes the entire
`440`-byte payload and requires exact END at offset `0x1B4`. A bounded
comment precedes executable definitions. Declarations at offsets
`0xD0`, `0xDC`, `0xE8`, and `0xF4` declare input 0 RGB, input 1 RGBA,
input 2 RGBA, and input 3 XY. Offsets `0x100` and `0x10C` declare
2D samplers 0 and 1. Constant definition `0xB8` supplies `(1,0,0,0)`.

Let `T0` and `T1` be four-component samples from samplers 0 and 1 at
input 3 XY. Let `S` be input 2, `C.rgb` input 0 RGB, and `D` input 1.
The texture finding establishes normalization by the first wrapper's
requested width and height. Pixel entry 4 reuses that normalized XY for both
samplers. It does not renormalize against the second wrapper's extents or
require equal atlas dimensions. Equal normalized coordinates do not imply
equal pixel coordinates in different textures.
The decoded operations imply:

```text
f = dot4(T0, S)
e = dot4(T1, S)
output0.rgb = D.rgb + f * (C.rgb - D.rgb)
output0.a   = D.a * e
```

| Pixel offset | VA | Effect of active destination components |
|---|---|---|
| `0x118` | `0x0110AB98` | Sample sampler 0 at input 3 XY into temporary 0 |
| `0x128` | `0x0110ABA8` | Sample sampler 1 at input 3 XY into temporary 1 |
| `0x138` | `0x0110ABB8` | Dot temporary 0 with input 2 into temporary 0 X |
| `0x148` | `0x0110ABC8` | Dot temporary 1 with input 2 into temporary 1 W |
| `0x158` | `0x0110ABD8` | Set temporary 1 XYZ to constant 1 |
| `0x164` | `0x0110ABE4` | Multiply temporary 1 by input 1 into temporary 2 |
| `0x174` | `0x0110ABF4` | Copy input 1 RGB into temporary 3 XYZ |
| `0x180` | `0x0110AC00` | Form input 0 minus input 1, in reversed RGB order, in temporary 0 YZW |
| `0x194` | `0x0110AC14` | Add temporary 0 X times that RGB difference to temporary 2 RGB |
| `0x1A8` | `0x0110AC28` | Copy temporary 2 to color output 0 |

The `0x180` MAD sources use swizzle `0x1B`, meaning WZYX. Its second
source has NEG modifier 1. Destination mask `0x0E` writes only YZW.
The temporary-3 W component is not initialized, but the active YZW lanes
read its ZYX components. Temporary-3 W and input-0 W are not consumed
by this operation. The following MAD reverses the RGB difference back
through WZYX and writes only XYZ. Losing NEG, treating WZYX as identity,
or ignoring the destination masks changes the interpretation.

Operation semantics follow Microsoft's
[texture sample](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texld---ps-2-0),
[dot product](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dp4---ps),
[multiply](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/mul---ps),
[multiply and add](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/mad---ps),
and [copy](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/mov---ps)
contracts. Executable arithmetic destinations carry partial-precision
modifier 2. The complete program contains no saturation, relative addressing,
predication, instruction controls, branch, or discard. No compiler metadata
string was used to infer the operation sequence.

The formula was independently checked with a mathematical CPU interpreter
on distinct finite samples, basis selectors, a fractional selector, and
unequal color components and alpha. Counterfactual texture swaps, missing
NEG, missing dispatcher alpha replacement, and consumption of undefined
masked-off temporary components were detected. This validates the bounded
token interpretation. It does not reproduce interpolation, sampler behavior,
GPU rounding, rasterization, or presented output.

## Reproduction bounds and remaining gaps

Verify the identified input before PE mapping. Read pointer and size dwords
from the table, then exactly the stated payload lengths. Check both hashes.
Decode version, bounded comment length, operation length, declaration type,
register type, destination masks, source swizzles, modifiers, and final END.
Reject unsupported forms rather than guessing. Inspect these additional
start-inclusive, end-exclusive ranges:

| Purpose | Reads |
|---|---|
| Selection, argument positions, alpha replacement | `0x00CA55A0..0x00CA5666` |
| Declaration fields and conversion | `0x0130C23C` plus 8 bytes, `0x00F57D9F` plus 1 byte, `0x00F57DC6` plus 2 bytes, `0x00419A20..0x00419AD5` |
| Packed second color and local records | `0x00CB9280..0x00CB95A0` |

The [Sqwt FontEdge finding](sqwt-font-edge-property-bridge.md) records
conditional property and style-bit paths. It leaves the external Boolean
setter and final virtual renderer-to-BitmapFont dispatch unresolved. Shared
flag values do not establish those joins.

Actual texture identities, their semantic face or outline roles, final
SetTexture and shader binding success, atlas format, sampler settings,
interpolation, floating precision, blend, alpha test, render-target processing,
live branch values, rendered coverage, and original UI appearance remain
unresolved. Both color inputs and texture samples are mathematical names,
not proven semantic style names.

Keep executable and shader bytes, decoded listings, assembly, decompiled
bodies, projects, and raw working output local. This page retains only
identities, locators, observations, interpretation, and limits.
