# Bitmap font texture consumers

The selected bitmap font draw path normalizes source coordinates with the
texture wrapper's requested width and height. It packs the selector vector
into a four-byte vertex field. The observations below establish the wrapper
and vertex delivery. The [shader consumer finding](bitmap-font-shader-consumers.md)
owns the selected program pair's operations. Coverage remains unresolved.

The [glyph consumer finding](bitmap-font-glyph-consumers.md) owns the GLYP
lookup, source rectangle, resource indices, selector vectors, and draw-path
selection. This page extends that finding's resource and selector joins.
The [GTEX loader finding](gtex-pwib-loader.md#gtex) owns the generic GTEX
header and texture-format contract.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Language | `x86/little/32/default` |
| Tool | ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1` |

The file size and hash matched this identity. The analyzed program's stored
`getExecutableSHA256()` value matched the same hash, and its image base
matched `0x00400000`. Instructions and constant data were inspected through
Ghidra in an existing analyzed program. The PE32 image base and selected import
slot were also checked directly against the pinned executable. No fresh-import
reproduction or live client validation ran.

All addresses below are VA. Subtract `0x00400000` for an RVA. Observed
instructions and data have confirmed confidence within the stated ranges.
Semantic interpretations are marked separately. Placeholder `FUN_` names do
not establish recovered source-level signatures.

## Function and data locators

| Locator | VA | RVA | Role in this finding |
|---|---|---|---|
| `FUN_00CA6340` | `0x00CA6340` | `0x008A6340` | FontData resource construction |
| `FUN_00418F00` | `0x00418F00` | `0x00018F00` | Resource factory wrapper |
| `FUN_004323D0` | `0x004323D0` | `0x000323D0` | DDS and GTEX dispatcher |
| `FUN_00418BF0` | `0x00418BF0` | `0x00018BF0` | 2D texture construction |
| `FUN_00431710` | `0x00431710` | `0x00031710` | Descriptor copy and creation call |
| `FUN_004312D0` | `0x004312D0` | `0x000312D0` | Texture creation consumer |
| `FUN_00CB9640` | `0x00CB9640` | `0x008B9640` | Float-component packing |
| `FUN_00CB91C0` | `0x00CB91C0` | `0x008B91C0` | Vertex batch copy |
| `FUN_00CB90D0` | `0x00CB90D0` | `0x008B90D0` | Batch flush |
| `FUN_00CB8FB0` | `0x00CB8FB0` | `0x008B8FB0` | Declaration initialization |
| `FUN_00419A20` | `0x00419A20` | `0x00019A20` | Declaration conversion |
| Descriptor table | `0x0130C22C` | `0x00F0C22C` | Input to declaration factory |
| Selected shader payload | `0x0110A900` | `0x00D0A900` | [Selected pixel program decode](bitmap-font-shader-consumers.md#pixel-mathematical-effects) |

`xivl-client-structs:manifests/symbols.json`, `BCS-Y-2357`, records the
probable name
`SQEX::CDev::Engine::Dw::RenderInterface::D3d9::TextureImp::Create` for
`FUN_004312D0`. Its retained name string is at VA `0x00F633F0` and is
pushed at VA `0x0043131D`. This anchors the catalog name, not a signature.

## Resource identity and requested dimensions

Let `F` be the complete FontData-relative pointer used in the glyph finding.

| Observation | Instruction or data locator |
|---|---|
| `FUN_00CA6340` counts four-byte entries in the list at `F+0x40` through `FUN_0067AF30`, selects a payload pointer through `FUN_00932DF0`, and calls `FUN_00418F00` with that pointer and creation value 2. | VA `0x00CA63D0` through `0x00CA6403`, helpers VA `0x0067AF30` through `0x0067AF42`, `0x00932DF0` through `0x00932E44` |
| It reads the returned resource pointer and supplies that value to `FUN_0096C140` using the vector object at `F+0x44`. The helper's available-capacity path writes the supplied dword at the end pointer and advances that pointer by four. The vector's begin and end fields are `F+0x48` and `F+0x4C`, joining the glyph finding's resource accessor. | VA `0x00CA640B` through `0x00CA644A`, insertion VA `0x0096C146` through `0x0096C177`, accessor VA `0x00CAD123` through `0x00CAD153` |
| `FUN_00418F00` forwards the payload pointer to `FUN_004323D0` and returns its created pointer through the output object. | VA `0x00418F24` through `0x00418F51` |
| The dispatcher checks the DDS signature and calls `FUN_00431B50`. Its GTEX signature branch calls `FUN_00431F30`. | VA `0x00432400` through `0x00432425`, `0x0043246E` through `0x00432497` |
| The DDS 2D construction branch calls `FUN_004190A0`, which forwards its two extent arguments to `FUN_00418BF0`. The DDS branch returns the constructed pointer through its output object. | VA `0x00431D56` through `0x00431D84`, `0x004190C2` through `0x004190FA`, return VA `0x00431DD4` through `0x00431DF6` |
| `FUN_00418BF0` changes a zero first or second unsigned extent to 1, writes those extents into the descriptor, and identifies its allocation with retained string `CDev.Engine.Dw.RenderInterface.Texture`. | VA `0x00418C1B` through `0x00418C59`, string VA `0x00F57C78` |
| `FUN_00431710` copies the descriptor with a resource destination adjustment of `+0x0C`. The two extents become resource dwords `+0x18` and `+0x1C`. It calls `FUN_004312D0` using the same resource object. | VA `0x00418CA6` through `0x00418CAE`, `0x0043174E` through `0x0043179B` |
| The 2D branch of `FUN_004312D0` reads resource dwords `+0x18` and `+0x1C`, then pushes them as the second and third arguments after the device argument to imported `D3DXCreateTexture`. | VA `0x00431302` through `0x00431309`, `0x00431486` through `0x0043149E` |
| The call goes through thunk VA `0x009FC77A`, which jumps through import slot VA `0x00F3E60C`. Ghidra identifies the thunk as `D3DXCreateTexture`. The pinned PE32 import directory independently identifies that slot as `d3dx9_41.dll!D3DXCreateTexture`. | VA `0x0043149E`, thunk VA `0x009FC77A`, import slot RVA `0x00B3E60C` |

Interpretation: the selected resource-array entry is a complete texture
wrapper. On the observed 2D construction path, wrapper dwords `+0x18` and
`+0x1C` are the requested width and height after constructor adjustment.
Their dimension names agree with the generic GTEX creation finding.

The glyph finding's normalization consumer `FUN_00CB9280` reads the same
wrapper offsets at VA `0x00CB9551` through `0x00CB957F`. Its source origins
and endpoints therefore use requested texture width and height as their
coordinate denominators. This supports a pixel-coordinate interpretation of
that path's source rectangle.

This does not identify the runtime font asset or its DDS/GTEX branch. Actual
device surface dimensions, resizing, allocation success, texture contents,
atlas page count, and default extents remain outside the observation. Cube
and volume construction paths have separate contracts.

## Selector packing and vertex delivery

`FUN_00CB9280` reads the four float components of its sixth argument at VA
`0x00CB940D` through `0x00CB943D`, then calls `FUN_00CB9640` at VA
`0x00CB9443`. For finite input components, the packer clamps each to `[0,1]`,
multiplies by 255, truncates to an integer, and retains the low byte.
The upper bound is float32 1.0 at VA `0x00F54F70`. The multiplier is
float64 255.0 at VA `0x00F598B0`.

| Input component in memory order | Packed bit positions |
|---|---|
| 0 | 0 through 7 |
| 1 | 8 through 15 |
| 2 | 16 through 23 |
| 3 | 24 through 31 |

The clamp and conversion occupy VA `0x00CB9640` through `0x00CB9729`.
The shifts and byte joins occupy VA `0x00CB972D` through `0x00CB9742`.
No general NaN contract is assigned here.

Applying that packing to the selector vectors in the glyph finding gives:

| Selector | Packed dword |
|---|---|
| 0 | `0x00FF0000` |
| 1 | `0x0000FF00` |
| 2 | `0x000000FF` |
| 3 | `0xFF000000` |

These are numeric packing results. The
[shader consumer finding](bitmap-font-shader-consumers.md) joins them to the
selected program pair's component use. A valid selector range remains unresolved.

| Observation | Locator |
|---|---|
| The packed result is stored at vertex offset `+0x14` in four local vertex records. The first record begins at the then-current stack+`0x24`, and its packed field is stack+`0x38`. Successive records are `0x2C` bytes apart. | VA `0x00CB9467` through `0x00CB9476`, vertex pointers VA `0x00CB95F3` through `0x00CB962A` |
| The consumer passes six vertex pointers and both resource arguments to `FUN_00CB91C0`. Its copy loop copies `0x2C` bytes per vertex into the batch. | Call VA `0x00CB962E`, copy VA `0x00CB9208` through `0x00CB9263` |
| `FUN_00CB90D0` passes the batch through `FUN_0041A570`, stores stream stride `0x2C`, passes the two resources to `FUN_00418410` with stage arguments 0 and 1, and passes the declaration at VA `0x013777D0` to `FUN_0041D240`. | VA `0x00CB9107` through `0x00CB9196` |
| `FUN_00CB8FB0` creates that declaration from descriptor table VA `0x0130C22C` through `FUN_004188B0`, whose constructor calls `FUN_00419A20`. | VA `0x00CB8FEA` through `0x00CB9008`, store VA `0x00CB905E`, factory VA `0x00418915` through `0x0041891D` |

The descriptor entry at VA `0x0130C244` supplies stream 0, internal format
index 3, internal semantic index 10, vertex offset `0x14`, and component
argument 4. Converter `FUN_00419A20` produces the following numeric
declaration record from that entry.

| Output field | Value | Locator |
|---|---|---|
| Stream | 0 | VA `0x00419A50` through `0x00419A57` |
| Offset | `0x14` | VA `0x00419A5C` through `0x00419A60` |
| Type | 8 | VA `0x00419A65` through `0x00419A77`, selected byte VA `0x00F57D9F` |
| Method | 0 | VA `0x00419A2A`, `0x00419A93` |
| Usage | 5 | VA `0x00419A69` through `0x00419A8F`, selected byte VA `0x00F57DC8` |
| Usage index | 2 | VA `0x00419A82` through `0x00419A8B`, selected byte VA `0x00F57DC9` |

The converter submits the declaration array to the device's virtual offset
`+0x158` at VA `0x00419ABF` through `0x00419AD3`. The
[shader consumer finding](bitmap-font-shader-consumers.md#vertex-fields-and-forwarding)
supplies named API assignments and the selected program pair's forwarding.

Interpretation: the selector vector reaches a packed byte field at vertex
offset `+0x14` through this bounded batch and declaration path. The
[shader consumer finding](bitmap-font-shader-consumers.md) joins that field
to the selected program pair's component selection and alpha data dependence.
Rendered coverage remains unresolved.

## Selected shader entry

On the `FUN_00CA55A0` path with flags bit `0x02` clear, state setup occurs
when global VA `0x01376490` differs from 5. It passes arguments 6 and 3 to
`FUN_00CA2E80` at VA `0x00CA5610` through `0x00CA5614`. That helper retains
the first argument at VA `0x01376458` and the second at VA `0x01376480`,
setting associated dirty flags when their values differ.

Flush `FUN_00CB90D0` calls `FUN_00CA2FD0` at VA `0x00CB9170`. When dirty
flag VA `0x01376484` is nonzero and backend branch byte VA `0x013763C0` is
zero, that helper reads the shader object at
`0x01376460 + 4 * dword(0x01376480)`. The checks and indexed read occupy
VA `0x00CA35D6` through `0x00CA35E8` and `0x00CA3671` through
`0x00CA3677`. With the retained selector 3, this is array entry 3. A later
test at VA `0x00CA3683` can choose an alternate call instead of delivering
that selected object. Runtime state values and binding success are unverified.

Initializer `FUN_00CA36E0` begins the selected loop with a zero byte offset at
VA `0x00CA37A9` and builds eight shader objects. Its loop at VA
`0x00CA37B0` through `0x00CA381B` reads sizes from VA `0x01109358` and
payload pointers from VA `0x0130BB80`, calls `FUN_00419590`, retains the
objects at array base VA `0x01376460`, and advances by four bytes per entry.

| Selected entry 3 datum | Locator | Value |
|---|---|---|
| Payload pointer | VA `0x0130BB8C`, RVA `0x00F0BB8C` | VA `0x0110A900`, RVA `0x00D0A900` |
| Payload byte size | VA `0x01109364`, RVA `0x00D09364` | `0x110` bytes |

The bounded payload contains strings `ps_2_0` and
`Microsoft (R) HLSL Shader Compiler 9.24.950.2656`. These are retained
metadata observations, not decoded operations. `FUN_00419590` forwards the
payload and size to `FUN_0041B210` at VA `0x00419608` through
`0x00419612`. The latter has a device call through virtual offset `+0x1A8`
at VA `0x0041B2D8` through `0x0041B2EC` on its cache-miss path.

The [shader consumer finding](bitmap-font-shader-consumers.md) records the
bounded token decode and corresponding vertex program forwarding. Ghidra's
x86 disassembler does not establish payload operations. Final context binding,
runtime texture identity, and rendered coverage remain unresolved.

## Reproduction recipe

Use a contributor-supplied executable matching the identity table. Verify its
size and SHA-256 before analysis. Record tool versions, language, image base,
and the analyzed program's stored executable SHA-256. A read-only Ghidra
script can query `currentProgram.getExecutableSHA256()`,
`currentProgram.getImageBase()`, and `currentProgram.getLanguageID()`.
The original executable path does not replace the stored-hash check.

The following commands reproduce the bounded joins. Outputs remain local.
Preserve stack adjustments when checking the arguments and vertex offsets.

```text
ghidra summary
ghidra version
ghidra doctor
ghidra disasm 0x00CA6340 -n 25
ghidra disasm 0x00CA63D0 -n 39
ghidra disasm 0x0067AF30 -n 9
ghidra disasm 0x00932DF0 -n 34
ghidra disasm 0x0096C140 -n 25
ghidra disasm 0x00CAD120 -n 24
ghidra disasm 0x00418F00 -n 40
ghidra disasm 0x004323D0 -n 65
ghidra disasm 0x00431B50 -n 135
ghidra disasm 0x00431D56 -n 32
ghidra disasm 0x00431DAB -n 30
ghidra disasm 0x004190A0 -n 47
ghidra disasm 0x00418BF0 -n 76
ghidra disasm 0x00431710 -n 45
ghidra disasm 0x004312D0 -n 34
ghidra disasm 0x00431413 -n 52
ghidra function get 0x009FC77A
ghidra disasm 0x009FC77A -n 1
ghidra memory read 0x00F57C78 40
ghidra memory read 0x00F633F0 68
ghidra disasm 0x00CB9551 -n 40
ghidra disasm 0x00CB940D -n 21
ghidra disasm 0x00CB95F3 -n 22
ghidra disasm 0x00CB9640 -n 75
ghidra memory read 0x00F54F70 4
ghidra memory read 0x00F598B0 8
ghidra memory read 0x0110D880 64
ghidra disasm 0x00CB91C0 -n 57
ghidra disasm 0x00CB90D0 -n 62
ghidra disasm 0x00CB8FB0 -n 56
ghidra disasm 0x004188B0 -n 48
ghidra disasm 0x00419A20 -n 52
ghidra memory read 0x0130C244 8
ghidra memory read 0x00F57D9F 1
ghidra memory read 0x00F57DC8 2
ghidra disasm 0x00CA55A0 -n 55
ghidra disasm 0x00CA2E80 -n 11
ghidra disasm 0x00CA2FD0 -n 10
ghidra memory read 0x00CA36D8 4
ghidra disasm 0x00CA3392 -n 105
ghidra disasm 0x00CA35D0 -n 75
ghidra disasm 0x00CA37A9 -n 2
ghidra disasm 0x00CA37B0 -n 35
ghidra memory read 0x0130BB8C 4
ghidra memory read 0x01109364 4
ghidra memory read 0x0110A900 272
ghidra disasm 0x00419590 -n 57
ghidra disasm 0x0041B210 -n 27
ghidra disasm 0x0041B2D8 -n 15
```

Do not publish generated assembly, decompiled function bodies, executable
bytes, shader payloads, Ghidra projects, or raw working logs. This finding
retains only observations, locators, and unresolved interpretations.
