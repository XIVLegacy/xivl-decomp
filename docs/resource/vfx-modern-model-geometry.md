# Modern VFX model geometry

The selected stock version-4 model has 457 vertices at stride 28, six eight-byte
components and 2592 little-endian u16 indices. This page records its serialized fields,
source ranges, distinct native layouts and conditional declaration interpretation. This
is a selected static geometry contract. Native resource creation, transfer and draw
completion remain separate questions.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build 2012.09.19.0001, size
15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9. Native addresses are
VA at PE32 image base 0x00400000. Subtract that base for RVA and map through the
original PE section table.

Input: data/5D/1E/00/B0.DAT, resource 0x5D1E00B0, size 25420 bytes, SHA-256
7dc48ec48b19e9e7bd1b6f5f50f056cb1b6275bcad3fb32d9914f907b8aaa38d. File offsets below are
decimal, half-open ranges. Record offsets are hexadecimal. Multibyte selected fields are
little-endian unless stated otherwise. The identities select one input, not every model
of the same version.

The observations use Ghidra disassembly through ghidra-cli 0.2.2 on the analyzed
ffxivgame.exe program. Instructions, direct relative targets, RTTI and referenced tables
were independently compared with the identified executable through PE mapping. Node
Buffer reads and SHA-256 establish source fields and ranges. No client execution is
involved. API interpretations follow the canonical [WRB converter
contract](wrb-vertex-declaration.md#api-array-conversion) and its primary Microsoft
citations. The [seasonal model and material
bindings](seasonal-model-material-bindings.md#indexed-model-libraries) own the common
indexed-library and native material context. The [legacy geometry
page](vfx-legacy-model-geometry.md) describes a separate version-2.2 representation.

## Selected frame

The source begins SEDBvmdl. Payload base is 48, and the selected descriptor is at 160.
Its u32 fields at file 204 and 208 hold model offset 276 and size 20088. Model base
R=48+276=324 selects [324,20412). Model FileInfo is [324, 340): endian byte 1, tag mxfv,
version 4 and variant 0. The native endian constant at 0x010D0FFC is 1. The selected
0x00C02750 discriminator therefore supplies conversion flag 0. 0x00C02870 takes its marker-1
return without swapping the FileInfo, and 0x00C01B60 skips its conditional header
swapping. This statement does not qualify every other fixup or the complete loader
invocation.

| Offset from R | Width | Selected value or role |
| --- | --- | --- |
| +0x00 | u8 | Endian marker 1 |
| +0x01 | 3 bytes | Zero, unconsumed by the checked discriminator |
| +0x04 | 4 ASCII bytes | mxfv |
| +0x08 | u32 | Version 4 |
| +0x0C | u32 | Variant 0 |
| +0x10 | 18 u32 | Library counts |
| +0x58 | 18 u32 | Ordinal bases |
| +0xA0 | 51 u32 | Model-relative record offsets |

The 18 counts at R+0x10 and 18 ordinal bases at R+0x58 describe 51 offsets at R+0xA0.
For kind k and admitted ordinal j, the selected formula is
R+u32le(R+0xA0+4*(base[k]+j)). Every base equals the preceding count sum. All 51 offsets
are nonzero and inside the model. The 18 chunk walk starts at 688, uses kind byte H+0,
payload byte count u32le(H+4), record count u32le(H+8), first record H+12 and next
header H+8+payloadBytes. Counts and indexed first-record starts agree, and the last
chunk ends 20412. This checks the declared chunk bounds and selected records, not all
semantics of every library.

| Kind | Chunk extent | Record count | Selected record extent |
| --- | --- | --- | --- |
| 11 VertexBuffer | [2060, 14960) | 1 | [2072, 14960) |
| 12 IndexBuffer | [14960, 20180) | 1 | [14972, 20180) |
| 13 SpriteBuffer | [20180, 20192) | 0 | None |
| 14 ModelBuffer | [20192, 20268) | 1 | [20204, 20268) |
| 17 VertexDeclaration | [20396, 20412) | 1 | Index [20408, 20410), then 2 alignment bytes |

The counts for kinds 0 through 17 are [1,3,3,1,1,1,22,3,8,1,1,1,1,0,1,1,1,1]. Their
ordinal bases are [0,1,4,7,8,9,10,32,35,43,44,45,46,47,47,48,49,50]. Chunk-header bytes
H+1..H+3 are unconsumed by the checked kind reader. Subsequent version-3-or-later
records align to four bytes. An indexed interval can include the next library header, so
its next indexed start is not a universal record-body end. These framing rules are
canonical in the [serialized library
contract](seasonal-model-material-bindings.md#serialized-library-framing-and-separate-records).

## Distinct native representations

The serialized source record, its in-place pointer fixups and an allocated attribute
Load object N are different layouts. For example, 0x00C02530 writes pointers into
serialized V+0x10/+0x14/+0x18. 0x00C0CDD0 instead places allocated
component/table/payload pointers at N+0x20/+0x24/+0x2C. Those offsets are not
interchangeable. The attribute Load bodies corroborate widths and source advancement
when invoked under the selected mxfv/version 4 context. This static contract does not prove
that every attribute Load body executes in the Qix in-place fixup route or that a native
allocation succeeds.

The RTTI names place VertexBuffer, IndexBuffer and ModelBuffer in
SQEX::CDev::Engine::Vfx::VfxResourceFormat::VfxResourceModelAttribute. DataTable and
Index are in VfxResourceBaseAttribute.

RTTI binds VertexBuffer table 0x010D1994 to Load 0x00C0CDD0, IndexBuffer table
0x010D19E4 to Load 0x00C0ABD0, ModelBuffer table 0x010D19CC to Load 0x00C0D310,
DataTable table 0x010D1C58 to Load 0x00C06860 and Index table 0x010D14F0 to Load
0x00C06770. The common Index size method 0x00C06700 returns 2 for the selected mxfv
context. Its special four-byte VINS case is separate.

The corresponding mangled RTTI identity anchors are:

| Table VA | Load VA | Mangled RTTI symbol |
| --- | --- | --- |
| 0x010D1994 | 0x00C0CDD0 | `.?AVVertexBuffer@VfxResourceModelAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` |
| 0x010D19E4 | 0x00C0ABD0 | `.?AVIndexBuffer@VfxResourceModelAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` |
| 0x010D19CC | 0x00C0D310 | `.?AVModelBuffer@VfxResourceModelAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` |
| 0x010D1C58 | 0x00C06860 | `.?AVDataTable@VfxResourceBaseAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` |
| 0x010D14F0 | 0x00C06770 | `.?AVIndex@VfxResourceBaseAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` |

## VertexBuffer dictionary

Let V=2072. The following offsets are relative to V. All multibyte selected fields are
little-endian.

| Offset | Width | Selected value | Source/native observation |
| --- | --- | --- | --- |
| +0x00 | u16 | 0 | Copied to attribute N+4. Broader purpose unknown. |
| +0x02 | u16 | 457 | Vertex count, attribute N+6, geometry reads at 0x00C1779D. |
| +0x04 | u16 | 28 | Stride, attribute N+8, geometry reads at 0x00C17799. |
| +0x06 | u16 | 6 | Component count E, attribute N+0x0A. |
| +0x08 | Index/u16 | 0 | First embedded Index at N+0x0C, value N+0x10. Its selected semantic purpose is unjoined. |
| +0x0A | Index/u16 | 0 | Version>=4 second Index at N+0x14, value N+0x18. 0x00C177DF sign-extends this as the native declaration-group selector. |
| +0x0C | 3 bytes | Zero | Skipped header/alignment bytes in selected Load. |
| +0x0F | u8 | 4 | Padding length, attribute N+0x1F. |
| +0x10 | 12 bytes | Zero | Serialized fixup destination area. In-place stores component, offset-table and payload pointers here. |
| +0x1C | 6*8 bytes | Component table | 0x00C025CF produces V+0x1C, stored at V+0x10. |
| +0x4C | 6*u16 | 0, 8, 12, 16, 20, 24 | Offset table, stored pointer at V+0x14. Declaration consumer uses only each word low byte. |
| +0x58 | 4 bytes | All 255 | Selected source padding. Observed fill is not a universal must-fill constraint. |
| +0x5C | 12796 bytes | Supplied packed stream | 457*28, source [2164, 14960), pointer at V+0x18. |

The modern DataTable schema is byte component-count at +0, byte storage code at +1, byte
semantic code at +2, an unconsumed alignment byte at +3 and u32 length at +4. 0x00C06860
reads those fields into native DataTable N+4/+5/+6/+8. Its GetSize is 8 for version>=3,
versus 7 with the dword at +3 for older models. The checked modern reader/writer skip
source byte +3. It is zero in all six selected entries, without a general must-zero
claim.

| Element | Source offset | Components/storage/semantic | Declared bytes | Offset word |
| --- | --- | --- | --- | --- |
| 0 | 2100 | 4/3/10 | 3656 | 0 |
| 1 | 2108 | 4/8/13 | 1828 | 8 |
| 2 | 2116 | 4/8/25 | 1828 | 12 |
| 3 | 2124 | 2/7/14 | 1828 | 16 |
| 4 | 2132 | 2/7/15 | 1828 | 20 |
| 5 | 2140 | 4/8/19 | 1828 | 24 |

The source formulas are C=V+0x1C=2100, U=V+0x1C+8*E=2148, and
payload=V+0x1C+10*E+u8(V+0x0F)=2164. The complete vertex Load body reaches RET at
0x00C0D180. Its version>=3 branch at 0x00C0CFF5 selects 0x00C0D119. That branch
multiplies zero-extended u16 count and stride, skips the padding, and passes the
supplied 12796-byte span to 0x009D4600, retaining the allocation pointer at attribute
N+0x2C. The [published import contract](seasonal-model-material-bindings.md) identifies
this as supplied packed-byte copying. No planar interleaving or value recreation is
selected here.

GetSize 0x00C0CA30..0x00C0CB1F yields 28+10*6+4+457*28=12888, matching the exact body
end 14960. The inspected Save continuation ends at 0x00C0CD54. Its later tail is
unexpanded, so this is not a full writer contract.

## IndexBuffer dictionary

Let I=14972. The actual IndexBuffer Load is 0x00C0ABD0..0x00C0ACBB.

| Offset | Width | Selected value | Source/native observation |
| --- | --- | --- | --- |
| +0x00 | u32 | 2592 | Index count, copied to attribute N+4 and passed to the geometry helper. |
| +0x04 | u32 | 0 | Copied to attribute N+8. Broader purpose unknown. |
| +0x08 | Index/u16 | 0 | Embedded Index at N+0x0C, value N+0x10. Broader selected purpose unjoined. |
| +0x0A | u8 | 4 | Primitive code, attribute N+0x14, passed at 0x00C178A0..0x00C178CE. |
| +0x0B | u8 | 12 | Padding length, attribute N+0x15. |
| +0x0C | 12 bytes | All 255 | Selected padding. The in-place fixup overwrites its first 4 bytes with a native pointer. |
| +0x18 | 2592*u16 | 0..456 | Source [14996, 20180), 5184 bytes, attribute N+0x18 owns the Load copy. |

Fixup 0x00C02610 produces I+0x0C+u8(I+0x0B)=14996 and stores that pointer at I+0x0C. The
pointer destination is inside this selected padding, not a serialized native address.
The selected pad is large enough to keep that store before the index bytes. No general
pad minimum or malformed-input safety is established.

The complete selected Load allocates and passes twice the dword count to the copy-shaped
helper. GetSize 0x00C0AAA0..0x00C0AAE4 gives 12+12+2*2592=5208, matching the record end
20180. All 2592 supplied little-endian u16 values were checked without reordering.
Minimum 0, maximum 456 and 457 distinct indices all fit the 457 vertex records. These
are offline selected-data checks, not proved native validation.

Common mapper 0x00C1DF90 maps raw code 4 to internal 4. No imported API topology name is
assigned.

## ModelBuffer and declaration selectors

Let M=20204. Its Load 0x00C0D310..0x00C0D487 reads two dwords, six binary32 lanes, an
embedded Index and 2*reference-pair-count further Index entries. The selected version 4
Index width is 2.

| Offset | Width | Selected value | Observation |
| --- | --- | --- | --- |
| +0x00 | u32 | 0 | Copied to attribute N+4, broader purpose unjoined. |
| +0x04 | u32 | 1 | Reference-pair count, copied to N+8. |
| +0x08..+0x1F | 6*binary32 | Exact bits/values below | Copied to N+0x0C..+0x23. 0x00C175D0 reads these as bound lanes. |
| +0x20 | Index/u16 | 0 | Embedded Index at N+0x24, value N+0x28. Broader selected purpose unjoined. |
| +0x22 | 2 bytes | Zero | Version>=3 alignment skipped by Load/Save. |
| +0x24 | s16 selector | 0 | First vertex reference, selects kind 11 ordinal 0. |
| +0x26 | s16 selector | 0 | First index reference, selects kind 12 ordinal 0. |
| +0x28..+0x3F | 24 bytes | Zero | Outside the selected checked Load/Save footprint, inside GetSize 64. |

| Source offset | Exact binary32 value | Bits as u32 |
| --- | ---: | ---: |
| +0x08 | -2167.89990234375 | 3305602662 |
| +0x0C | 73.64138793945312 | 1116948580 |
| +0x10 | -2167.89990234375 | 3305602662 |
| +0x14 | 2167.89990234375 | 1158119014 |
| +0x18 | 1505.3275146484375 | 1153182331 |
| +0x1C | 2167.89990234375 | 1158119014 |

The only selected pair is read into the allocated Index array at attribute N+0x30 with
native element stride 8. 0x00C175D0 reads raw M+0x24/+0x26, sign-extends both selectors,
resolves the indexed libraries, reads V+2/+4 and I+0/+0x0A, then reaches
0x00C1BAC0/0x00C1E080 with the selected source pointers. These reached calls do not
establish buffer creation or transfer.

GetSize 0x00C0A850..0x00C0A8A2 gives 56+2+2+4*1=64 for this source, while complete
selected Load and Save through 0x00C0A9C5 access only the first 36+4*1=40 bytes. The
24-byte tail is outside those checked accesses. Its zero fill does not identify an
authoring purpose or exclude every possible consumer.

Kind 17 contains Index 0 at 20408 followed by 2 alignment bytes. 0x00C19C70 resolves
that Index 0 to VertexBuffer ordinal 0, then 0x00C19D04..0x00C19D5F consumes six
eight-byte components and corresponding two-byte table entries. This is distinct from
V+0x0A, which selects the native declaration group used by geometry. Both selected
values are 0, but their roles are separately located.

## Common declaration interpretation

The modern builder at 0x00C19D04..0x00C19D5F emits tuples [u8(offsetWord),
componentCount, storageCode, map(semanticCode)]. Instruction 0x00C19D23 reads only the
low byte of each serialized u16 offset. Map 0x00C1B980 sends semantic codes
10/13/25/14/15/19 to 0/1/10/2/3/7. Shared builder 0x00C1E410 uses 0x00C1E1D0 to map
storage 3/8/7 to compact format 4/3/2. Mapper 0x00C1E260 maps those intermediate
semantics to compact usage 0/2/3/8/9/13. It appends the terminator and calls Core slot
+0xC0.

Under retained table 0x010B590C at Core global 0x0136B6B0, that slot is 0x00B9FCE0.
Calls at 0x00B9FCEB and 0x0041891D join 0x004188B0 and 0x00419A20. The canonical
[compact-to-D3D9 converter](wrb-vertex-declaration.md#api-array-conversion) provides the
exact API-field formulas and primary interface authority. The selected conditional
results are:

| Byte offset | Components | Compact format/usage | API Type/Usage/Index | Interpretation | Bytes per vertex |
| ---: | ---: | --- | --- | --- | ---: |
| 0 | 4 | 4/0 | 7/0/0 | SHORT4 POSITION0 | 8 |
| 8 | 4 | 3/2 | 8/3/0 | UBYTE4N NORMAL0 | 4 |
| 12 | 4 | 3/3 | 8/10/0 | UBYTE4N COLOR0 | 4 |
| 16 | 2 | 2/8 | 15/5/0 | FLOAT16_2 TEXCOORD0 | 4 |
| 20 | 2 | 2/9 | 15/5/1 | FLOAT16_2 TEXCOORD1 | 4 |
| 24 | 4 | 3/13 | 8/5/5 | UBYTE4N TEXCOORD5 | 4 |

All compact streams and emitted Methods are zero. The six widths partition stride 28,
and each declared length is 457 times its selected width. API normalization describes
declaration interpretation, not CPU arithmetic in this builder. TEXCOORD5 here names a
declaration semantic. Shader behavior and basis meaning remain in the separate [seasonal
contract](seasonal-model-material-bindings.md#texcoord5-normal-map-basis). No quantizer,
generated replacement value or reconstruction follows.

This join requires the selected mxfv/version-4 context, expected Index/DataTable
implementations, valid fixed-up storage and pointers, normal nested completion,
successful prior allocation, initialized scratch globals 0x01328D88/0x01328D8C and
retained Core identity. The common builder stores its return at declaration +0x4C
without a shown nonnull check. That return is not proof of a declaration resource. The
checked modern tuple window initializes 15 four-byte slots and consumes E=6. Its loop
compares word E, then passes its low byte to the common builder. No universal E<=15
validation is established.

## Packed source identities

| Source | File range | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Supplied vertices | [2164,14960) | 12796 | 157549275bd8b6273174b5fe3ad8aa8723c6163b88be844f377d6344acfec8e7 |
| Supplied indices | [14996,20180) | 5184 | 1a1b9f57160f262fa1a4951909819e5150024c3ff4b538a9b0bbffc0847b8308 |

These identify the original packed spans without publishing them. No stream is
recreated. Model-relative payload offsets 1840 and 14672 are divisible by 16. Their
file-equivalent offsets 2164 and 14996 are not runtime pointer values.

## Selected shader input profile

The embedded B0 pair has a conditional source-operand profile under the six-element
[declaration interpretation](#common-declaration-interpretation). A format-aware Node
Buffer walk of the identified B0 input reads complete SM3 instruction boundaries, CTAB
entries, declarations, source operands and structured Bool blocks. This is serialized
shader evidence. It does not select native Bool values or establish shader execution.
Offsets in this section are hexadecimal B0 file offsets, with half-open ranges.

| Stage | Complete stream | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| vs_3_0 | [0x516C,0x5B14) | 2472 | 2e168ca4e13e55f25776fad7524b5c11e7ac51caeaf63206720078e55d6eea13 |
| ps_3_0 | [0x5B34,0x634C) | 2072 | e228579ecc0d059dee6150f08ae02dabba2b33f023796c8aa0af08cbcaf210c4 |

Big-endian length fields at 0x515C and 0x5B24 delimit those streams. Their CTAB comments
occupy [0x5170,0x5440) and [0x5B38,0x5F4C), with CTAB headers at 0x5178 and 0x5B40.
The headers contain 14 and 19 constant entries. Their embedded version fields at 0x5180
and 0x5B48 are inside comments, not additional shader starts. Instruction walks after
the comments consume 108 and 66 instructions, excluding COMMENT and END, and terminate
at END offsets 0x5B10 and 0x6348, exactly at the declared stream bounds. Declaration
payloads and immediate definitions are excluded from source-operand counts.

The complete vertex input declarations are:

| DCL offset | Register | Semantic | Source operands in the complete stream |
| --- | --- | --- | --- |
| 0x5470 | v0 | POSITION0 | Four unconditional, one under b0=true. First at 0x556C. |
| 0x547C | v1 | COLOR0 | Two unconditional. First at 0x5A8C. |
| 0x5488 | v2 | NORMAL0 | One unconditional at 0x5538. |
| 0x5494 | v3 | TEXCOORD5 | Two unconditional. First at 0x554C. |
| 0x54A0 | v4 | TEXCOORD6 | Twelve under b0=true. First at 0x55F4. |
| 0x54AC | v5 | TEXCOORD7 | One under b0=true at 0x558C. |
| 0x54B8 | v6 | TEXCOORD0 | Two unconditional. First at 0x596C. |
| 0x54C4 | v7 | TEXCOORD1 | Two unconditional. First at 0x59A0. |

The CTAB Bool entries each use register set 0 and count 1. All three IF source operands
have modifier 0. The blocks are unnested, with these exact guards:

| Stage and CTAB name | Entry offset | Register | IF / source operand | True body | ELSE / false body | ENDIF |
| --- | --- | --- | --- | --- | --- | --- |
| Vertex isSkining | 0x51F8 | b0 | 0x5578 / 0x557C | [0x5580,0x57D8) | None | 0x57D8 |
| Pixel light0bit0 | 0x5C60 | b8 | 0x622C / 0x6230 | [0x6234,0x6278) | 0x6278 / [0x627C,0x6288) | 0x6288 |
| Pixel light1bit0 | 0x5C74 | b10 | 0x628C / 0x6290 | [0x6294,0x62DC) | None | 0x62DC |

The only address-register write is at 0x55E0, inside b0's true body. It writes a0.xyzw
before all 12 relative source operands. In groups of three, the relative operands at
0x55F8/0x560C/0x5620, 0x5630/0x5648/0x5660, 0x5678/0x5690/0x56A8 and
0x56C0/0x56D8/0x56F0 use constant bases c21/c22/c23 with a0.y, a0.x, a0.z and a0.w,
respectively. Each operand has relative-address bit 13 set and a following address
token. CTAB entry 0x520C names c21..c164 jointMatrices. The guard establishes no
particular address value or effective joint-matrix access bound.

Under the hypothetical vertex assignment b0=false, 13 retained input source operands
name exactly POSITION0, NORMAL0, COLOR0, TEXCOORD0, TEXCOORD1 and TEXCOORD5, with no
relative operands. These are exactly the six supplied packed semantics under the
conditional mapping above. Under hypothetical b0=true, 27 input source operands also
name TEXCOORD6 and TEXCOORD7, which are absent from the six-element packed source, and
all 12 relative operands remain. All eight original DCL records remain in both
profiles. A six-semantic read profile does not establish acceptance of a reduced
native declaration or supply values for the two absent semantics.

The pixel declarations at 0x5F7C/0x5F88/0x5F94/0x5FA0 map v0/v1/v2/v3 to
TEXCOORD0/TEXCOORD3/COLOR0/TEXCOORD4. All six input source operands are unconditional.
The sampler CTAB entries use set 3, count 1, and join these declared registers and
actual texture-source operands:

| CTAB name / entry | Sampler | DCL / dimension field | Sampler source operand |
| --- | --- | --- | --- |
| _sampler_00 / 0x5B5C | s0 | 0x5FAC / 0x5FB0 | 0x6020 |
| _sampler_01 / 0x5B98 | s1 | 0x5FB8 / 0x5FBC | 0x6094 |
| _sampler_02 / 0x5BD4 | s2 | 0x5FC4 / 0x5FC8 | 0x60D4 |

All three dimension fields contain code 2, meaning 2D. All four hypothetical b8/b10
assignments retain the same four pixel input semantics, all three sampler operands
and zero relative operands. The Bool branches can still change arithmetic results.
Thus each b0=false combination reads only the six supplied vertex semantics, while
each b0=true combination retains the TEXCOORD6/7 gap, regardless of b8/b10.

For reproduction, authenticate the B0 identity above, read the two record lengths,
and hash each complete stream. Walk little-endian DWORD instruction tokens using
opcode bits 15:0 and length bits 27:24, with COMMENT length in bits 30:16. Decode DCL
semantics separately from executable operands, consume the extra address token after
each relative source, and match the explicit IF/ELSE/ENDIF blocks before filtering
source operands for each hypothetical assignment. No other flow opcode, predication
flag, DEFB or DEFI occurs in these two streams. CTAB names, register sets, indices and
counts join the Bool and sampler operands independently of their descriptive names.

The format interpretation follows Microsoft's [shader format](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/shader-code-format),
[instruction token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/instruction-token),
[source token](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/source-parameter-token),
[relative addressing](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/shader-relative-addressing),
[DCL format](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/dcl-instruction),
[Bool IF](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/if-bool---vs)
and [sampler declaration](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dcl-samplertype---ps)
contracts. The dimension code is defined by D3DSTT_2D in Microsoft's
[d3d9types.h](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9types.h).

A retained source operand is a syntactic read, not a proof of mathematical liveness
or contribution by every lane. Arithmetic, effective indices and native/current Bool
values remain unevaluated. The inherited converter conditions remain unchanged.
Native shader creation, validation, declaration/stream acceptance, execution, bound
texture contents, sampler state and rendering remain unestablished by this static
profile. It covers only the identified B0 pair.

## Reproduction and limits

On the identified analyzed program, use bounded ghidra disasm queries with the VA or
FUN_<eight-hex-VA> selector, -n count and --json options for the Load, GetSize, fixup,
geometry and declaration locators above. Interpret referenced tables as data. Follow
table[-4] through the complete-object locator and type descriptor for RTTI. Map each
instruction and data range through the original PE sections and recompute direct
relative targets. From the identified B0 input, read the descriptor, R header,
count/base tables and declared chunks, then apply the source formulas above.
Independently check bounds, index values and packed hashes. The common downstream
converter recipe remains on the WRB page.

The relevant lookup windows are 0x00C02537..0x00C0256F,
0x00C02616..0x00C02654, 0x00C176DC..0x00C176FE,
0x00C1772C..0x00C17792 and 0x00C19C79..0x00C19CC6.

The checked native lookups test nonzero counts, reject count<ordinal using JC, and
require nonzero relative offsets. They are not universal ordinal<count or file-extent
validation. Selected ordinal zero is admitted and the independent selected bounds hold.
No complete malformed-input contract follows.

Opaque roles remain at V+0, V+8, I+4, I+8, M+0 and M+0x20. Wrapper stage 0x00C02060 is
unexpanded, including its selected ModelBuffer effects. The VertexBuffer Save tail after
0x00C0CD54 and complete 0x00C02750 alias effects remain unqualified. The source
Load/Save and modern geometry observations require the stated context and valid
pointers. Generic allocator, array and copy helpers do not establish actual callback
invocation or successful resources from caller structure alone.

Material/constant binding, native allocation and transfer success, lifetime,
factory/instance continuation, final indexed draw, activation, color/matrix calculation
and rendered output remain outside this selected geometry contract. A successful helper
return is not proof of successful underlying resource creation.
