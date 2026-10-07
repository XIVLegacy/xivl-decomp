# Bitmap font shader consumers

The selected bitmap font program pair forwards a packed selector to a
four-component texture dot product. The pixel program copies one interpolated
input's RGB components to color output 0 and multiplies that input's alpha by
the dot-product result. This establishes component data dependence before
render-target processing. It does not establish rendered coverage.

The [texture consumer finding](bitmap-font-texture-consumers.md) owns resource
construction, requested dimensions, packing, and batch delivery. The
[glyph consumer finding](bitmap-font-glyph-consumers.md) owns the selector
vectors and the draw-path selection. This finding extends their bounded
shader decoder boundary.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Language | `x86:LE:32:default` |
| Ghidra tools | ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1` |
| Token decoder | Independent bounded Python parser, SHA-256 `9fed01d2ffb277bbadb3c083d0350e9deb79300a6cf2b7db3285ce0050facce0` |

The executable's size, SHA-256, and PE32 image base matched the table. The
analyzed program's stored `getExecutableSHA256()` value and image base matched.
Every bounded payload and constant-data read was compared with the pinned PE.
Inspected selection and creation instruction encodings also matched the PE.
The analyzed program was reused. No fresh import or live client validation ran.

The decoder interprets shader model 2 tokens with Microsoft's token
specifications and primary `d3d9types.h` definitions. It parses the complete
bounded payloads, checks instruction lengths and supported forms, skips only
bounded comments, and requires END at the final dword. It rejects relative
addressing, source modifiers, instruction controls, predication, unsupported
operations, and destination modifiers other than none or partial precision.
This is token interpretation, not Microsoft runtime shader validation or a
bit-exact device execution result. Ghidra's x86 disassembler was used for the
selection and creation joins, never for shader operations.

The primary header source is
[Microsoft win32metadata](https://github.com/microsoft/win32metadata/blob/75ec935a4f1ac56345daa9f7f0bc21db3e68ac68/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9types.h).
Its SHA-256 is
`5ba31e75a3ea03c1a71b48f39d966b6f67967b22bb9e4abcc419a3118269cd6a`.
The same revision's
[`d3d9.h`](https://github.com/microsoft/win32metadata/blob/75ec935a4f1ac56345daa9f7f0bc21db3e68ac68/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h),
SHA-256 `a48e8132befefce1d14a8d0ffc277e7149c3c4b45d5f7427e2ac07605d65889e`,
supplies device method order.

All addresses below are VA. Subtract `0x00400000` for an RVA. Observed token
fields, constant data, and instructions have confirmed confidence within the
stated bounds. Mathematical conclusions combine those observations with the
cited API definitions.

## Selected program pair

On the flags-bit-`0x02`-clear path, `FUN_00CA55A0` passes selectors 6 and 3
to `FUN_00CA2E80` at VA `0x00CA5610` through `0x00CA5614` when state setup
is required. The helper retains them at VA `0x01376458` and `0x01376480`.
The existing texture consumer finding owns this conditional selection.

Initializer `FUN_00CA36E0` reads vertex sizes from VA `0x01109334` and
pointers from VA `0x0130BB00`, beginning with byte offset zero at VA
`0x00CA370E`. It calls `FUN_004194E0` at VA `0x00CA3723`, stores each object
at VA `0x013763C8` plus that byte offset at VA `0x00CA3763`, and advances by
four at VA `0x00CA3792`.

| Stage and entry | Pointer locator | Size locator | Payload VA | Byte length | Payload SHA-256 |
|---|---|---|---|---:|---|
| Vertex 6 | `0x0130BB18` | `0x0110934C` | `0x01109E80` | `552`, `0x228` | `e1a5d394adce4f39cba93d4a33dbbfbeaa0314a3301a9da206c3e910f5238295` |
| Pixel 3 | `0x0130BB8C` | `0x01109364` | `0x0110A900` | `272`, `0x110` | `a5ebea5a4723a2711b2f15e1f30d59680c3e917e920cc3148c1091d6083d4cd9` |

The vertex factory forwards payload and size to `FUN_0041B0F0` at VA
`0x00419558` through `0x00419562`. Its device call uses virtual offset
`+0x16C`, including VA `0x0041B1BF` through `0x0041B1CC`. The primary
`IDirect3DDevice9` declaration identifies this as `CreateVertexShader`.
The pixel loop at VA `0x00CA37A9` through `0x00CA381B`, its factory call at
VA `0x00419612`, and its device call at VA `0x0041B2DF` through
`0x0041B2EC` join the pixel payload to offset `+0x1A8`, `CreatePixelShader`.
Creation success is unverified.

With vertex dirty byte VA `0x0137645C` nonzero and backend branch byte VA
`0x013763C0` zero, the flush reads vertex object
`dword(0x013763C8 + 4 * dword(0x01376458))` at VA `0x00CA3599` through
`0x00CA359E`. Selector 6 chooses slot VA `0x013763E0`. Feature bit 1 at VA
`0x00CA35AA` can bypass the delivery branch. That branch calls
`FUN_0041C440` at VA `0x00CA35C8`.

The corresponding pixel branch reads the selected object at VA
`0x00CA3671` through `0x00CA3677`, tests feature bit 2 at VA `0x00CA3683`,
and calls `FUN_0041C460` at VA `0x00CA36A9` on its delivery branch. The
wrappers forward object `+0x04`, or zero for a null object, at VA
`0x0041C440` through `0x0041C47F`. Final render-context dispatch and binding
success remain unresolved. These are conditional static joins.

## Vertex fields and forwarding

Converter `FUN_00419A20` reads each descriptor's stream, offset, internal
format, semantic, and component argument at VA `0x00419A50` through
`0x00419A93`. Its type lookup is
`byte(0x00F57D8F + 4 * internalFormat + componentArgument)`. Its usage and
usage-index bytes are at `0x00F57DB4 + 2 * internalSemantic` and the next byte.

| Descriptor VA | Vertex offset | Internal format / semantic / component argument | D3D type / usage / usage index |
|---|---|---|---|
| `0x0130C234` | `0x0C` | 3 / 8 / 4 | 8 / 5 / 0 |
| `0x0130C244` | `0x14` | 3 / 10 / 4 | 8 / 5 / 2 |
| `0x0130C24C` | `0x18` | 1 / 11 / 2 | 1 / 5 / 3 |

All three descriptors specify stream 0. Selected lookup bytes are at VA
`0x00F57D9F` and `0x00F57D95`, with semantic pairs at VA `0x00F57DC4`,
`0x00F57DC8`, and `0x00F57DCA`.

Microsoft's [D3DDECLTYPE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddecltype)
assigns 8 to `UBYTE4N` and 1 to `FLOAT2`. `UBYTE4N` normalizes each of four
unsigned bytes by division by 255. `FLOAT2` supplies two floating components.
[D3DDECLUSAGE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddeclusage)
assigns 5 to `TEXCOORD`. The byte normalization uses component memory order.
The separately defined `D3DCOLOR` expansion is not the observed type.

The bounded vertex payload identifies vertex shader 2_0. Its declarations
associate input 1 with TEXCOORD0, input 3 with TEXCOORD2, and input 4 with
TEXCOORD3. The declarations for inputs 3 and 4 are at payload offsets
`0x10C` and `0x118`, VA `0x01109F8C` and `0x01109F98`.

| Vertex payload offset | VA | Observed forwarding |
|---|---|---|
| `0x1F4` | `0x0110A074` | All four components of input 1 to coordinate output 0 |
| `0x20C` | `0x0110A08C` | All four components of input 3 to coordinate output 2 |
| `0x218` | `0x0110A098` | First two components of input 4 to coordinate output 3 |

These paths use identity component order without arithmetic or source
modifiers. The preceding vertex operations do not write the input registers.
Microsoft's [vertex coordinate output](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-vs-registers-texture-coordinate)
and [pixel coordinate input](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-ps-registers-texture-coordinate)
contracts establish delivery with interpolation. Thus the packed selector at
vertex `+0x14` supplies pixel coordinate input 2, source coordinates at
`+0x18` supply input 3, and the field at `+0x0C` supplies input 0. The
higher-level origin and meaning of the `+0x0C` field remain unresolved.

## Pixel mathematical effects

The bounded pixel payload identifies pixel shader 2_0. Declarations at
payload offsets `0x94`, `0xA0`, `0xAC`, and `0xB8` specify four components
of coordinate inputs 0 and 2, two components of input 3, and 2D sampler 0.

Let `T` be the four-component sample from sampler 0 at coordinate input 3's
first two components. Let `S` be interpolated coordinate input 2, and `C`
interpolated coordinate input 0. The observed data dependence is:

```text
output0.rgb = C.rgb
output0.a   = C.a * dot4(T, S)
```

This formula states component selection and mathematical operations. It
does not specify bit-exact rounding or precision.

| Pixel payload offset | VA | Observed effect |
|---|---|---|
| `0xC4` | `0x0110A9C4` | Sample sampler 0 using coordinate input 3 into a four-component temporary |
| `0xD4` | `0x0110A9D4` | Four-component dot product of that sample and input 2 into temporary component 0 |
| `0xE4` | `0x0110A9E4` | Multiply that scalar by input 0 component 3 into temporary component 3 |
| `0xF4` | `0x0110A9F4` | Copy input 0 components 0 through 2 into the temporary |
| `0x100` | `0x0110AA00` | Copy the full temporary to color output 0 |

The operation meanings follow Microsoft's [texture sample](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texld---ps-2-0),
[dot product](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dp4---ps),
[multiply](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/mul---ps),
and [copy](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/mov---ps)
contracts. Each executable destination carries modifier value 2, identified
as partial precision by the primary header and
[destination token specification](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/destination-parameter-token).
The complete bounded program contains no saturation, source negation,
relative addressing, branch, or discard operation. END is at offset `0x10C`.

Combining the glyph finding's vectors, the texture finding's packed byte
order, UBYTE4N normalization, vertex forwarding, and this dot product gives:

| Observed selector entry | Selected sample component |
|---|---|
| 0 | Component 2, Blue |
| 1 | Component 1, Green |
| 2 | Component 0, Red |
| 3 | Component 3, Alpha |

The channel names follow the destination-token and coordinate-register API
contracts. For these basis-vector inputs, the selected sample component is a
multiplicative factor in output alpha. This derived table does not establish
a legal selector domain, caller default, or semantic style name.

## Surviving boundary

The program samples sampler 0. The submitted resource's final render-context
SetTexture join and actual runtime texture identity remain unresolved. No
font resource bytes were inspected. No atlas format is inferred from this
shader or from the separate generic GTEX format mapping.

Final coverage, rasterization, blend, alpha-test, color-write, render-target,
sampler filtering, address mode, sRGB setting, device precision, and appearance
remain unresolved. Microsoft's [color output register](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-ps-registers-output-color)
contract places output before applicable blending and component write control.
The finding does not establish runtime branch values, shader binding success,
caller defaults, style meanings, or every shader entry and flag path.

## Reproduction boundary

Verify the file identity, stored program SHA-256, image base, language, and
producer versions first. Read only the selected entry pointers and lengths,
then payload VA `0x01109E80` for `552` bytes and VA `0x0110A900` for `272`
bytes. Check the payload hashes above against both PE-backed and Ghidra reads.

Decode version, comment length, instruction length, destination write masks,
register types, source swizzles, modifiers, declarations, and exact END using
Microsoft's [version token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/version-token),
[comment token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/comment-token),
[instruction token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/instruction-token),
[source token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/source-parameter-token),
[destination token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/destination-parameter-token),
and [DCL format](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/dcl-instruction)
specifications plus the pinned primary headers. Confirm the operand identities
and component masks at the listed offsets rather than using shader metadata
strings or x86 disassembly to infer operations.

Keep payloads, executable bytes, decoded listings, Ghidra projects, generated
assembly, decompiled bodies, and raw working output local. This finding retains
only identities, observations, locators, mathematical interpretation, and limits.
