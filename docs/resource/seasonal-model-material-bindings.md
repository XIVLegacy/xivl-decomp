# Seasonal model and material bindings

Stock resources supplied the complete historical cloud material, shader pair,
model record, index/node/declaration records and embedded VINS block. The
historical model's texture IDs also matched registrations in specific stock
leaves. The selected vertex data remained a separate input; an exact stock-only
reconstruction was not established by those component matches.

The native VFX model path resolved dependencies by resource ID and ordinal,
then bound materials by indices into that ordered result. DDS path strings
inside a model were not the binding contract.

The [seasonal reconstruction finding](seasonal-memory-reconstruction.md)
owns the historical atmosphere curves and exact stock texture/shader payload
sources. This page owns the cloud model's serialized records and their native
consumers.

## Inputs and method

Native addresses are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 x86-32 decoded 5,890
original instructions at retained Ghidra boundaries in 43 selected functions.
Every boundary and instruction byte matched the original PE. This method
does not classify padding or embedded tables as instructions. No native
engine function, shader upload or client process was executed.

The historical model was original resource `0x5D1E00B4`, 25,420 bytes,
SHA-256 `610f11d4d82a543b9f91fa99875d91715fa7c2cfd22054c4c4d70135fc4d8137`.
Its source patch and extraction span are in the
[historical input inventory](historical-starlight-changed-inputs.csv).
The [stock shader comparison](seasonal-memory-reconstruction.md#controllers-and-cloud-program)
pins same-key stock and alternatives `0x5D1E00B0` and `0x5D1E0088`.
Python `struct` and `hashlib` supplied the resource field and span comparison.

## Serialized descriptor and extents

In the historical input, the SEDB payload base was file `0x30` and its
selected `ldmv` descriptor D began at file `0xA0`. The following serialized
offsets were relative to that payload base. Lengths were byte counts, not
additional offsets.

| Descriptor field | Serialized value | Selected historical file extent |
| --- | --- | --- |
| `D+0x18` | Offset `0xB8`; count at `D+0x1C` was one. | Shader-span descriptor at `0xE8`. |
| `D+0x20` | Offset `0xCC`; count at `D+0x24` was three. | Three 24-byte dependency rows at `[0xFC,0x144)`. |
| `D+0x2C/+0x30` | Offset `0x114`, length `0x4E78`. | `mxfv` model `[0x144,0x4FBC)`. |
| `D+0x40/+0x44` | Offset `0x4F8C`, length `0x190`. | `VINS` block `[0x4FBC,0x514C)`. |
| `D+0x48/+0x4C` | Offset `0x511C`, length `0x9C8`. | Vertex-program wrapper `[0x514C,0x5B14)`. |
| `D+0x50/+0x54` | Offset `0x5AE4`, length `0x838`. | Pixel-program wrapper `[0x5B14,0x634C)`. |

The shader-span descriptor overlapped the last four fields of D. Its first
four words selected those vertex/pixel wrappers and lengths. The model
header began four bytes before its `mxfv` tag; the following `VINS` block
began at the model's declared end.

Native dependency producer `0x00BC4230..0x00BC4335` obtained the selected
resource descriptor, allocated for `D+0x24` dependencies, copied each row's
16-byte ID and four-byte type from `D+0x20`, and called `0x00BBC7B0`.
The native paths below consumed the resulting descriptor pointers. The file
offset arithmetic above describes serialized bytes, not unfixed native
addresses.

## Native construction order

Factory path `0x00BC43B0..0x00BC443E` checked descriptor `+0x28 == 1`
and `+0x34 <= 1`, allocated an `0x80`-byte product and called
`0x00BE04C0`. That constructor installed table `0x010CE074`, identified
by retail RTTI as `SQEX::CDev::Engine::Vfx::Qix::Resource::QixModel`.
The factory then called `0x00BE2600`. These guards describe the checked
construction branch, not an unrestricted function-call API.

At `0x00BE26D7`, the driver read the selected model header through
`0x00C02900`. The version-at-`+8` branch at `0x00BE26F4..0x00BE26F9`
selected the following path for version 3 or later:

| Order | Native locator | Observation |
| ---: | --- | --- |
| 1 | `0x00BE2784..0x00BE2790`, `0x00C02750` | Wrapped the supplied model memory and ran header, library and record fixups. |
| 2 | `0x00BE279A..0x00BE27B0`, `0x00C1AC20` | Created material parameter/pointer storage, material flags, geometry and node state through four checked stages. A failure destroyed/reset the partial model. |
| 3 | `0x00BE27CF..0x00BE27D2`, `0x00BE2110` | Resolved dependency IDs into the model's ordered texture-pointer array. |
| 4 | `0x00BE27D7..0x00BE27DF`, `0x00BE1450` | Selected and submitted the vertex/pixel programs for each material. |
| 5 | `0x00BE27E4..0x00BE27EC`, `0x00C16DF0` | Joined material samplers, model references and node references to the constructed objects. |

These were the material/geometry stages, not the driver's complete lifetime
contract. It subsequently checked `D+0x40` at `0x00BE296D`. For the
version-3-or-later VINS branch, `0x00BE2A0B..0x00BE2A69` wrapped that supplied
block through `0x00C017F0`, created its destination and called `0x00C15110`.
The selected 400-byte historical VINS block at `[0x4FBC,0x514C)` was exact
in B4, SHA-256
`206c0ce0f98926b4dbeacb2edfbc0c5ebb162c831ce68f5c904418016bd12a72`.
B0 and 88 differed in ten bytes there. Reusing B0's material does not require
reusing its unequal VINS input.

The below-version-3 branch used a different parsed representation and
`0x00C1AC80`. Its pointer offsets must not be substituted into the checked
version-3-or-later path.

## Indexed model libraries

Constructor `0x00C0DD50` installed the
`VfxResourceFormat::VfxResourceModelAttribute` RTTI table `0x010D2284`
and the `mxfv` type word. Dispatch `0x00C0B310` selected its typed
libraries. In the version-3-or-later input, let R denote the native model
header after its endian conversion. The checked consumers used:

```text
count[k] = u32(R + 0x10 + 4*k)
base[k]  = u32(R + 0x58 + 4*k)
record   = R + u32(R + 0xA0 + 4*(base[k] + ordinal))
```

The selected records must have admitted ordinals and nonzero offsets.
This is a relative-offset table, not an array of equal-size records.

| Kind | RTTI library / selected consumer |
| ---: | --- |
| 9 | ShaderLibrary |
| 10 | MaterialLibrary; `0x00C023C0`, `0x00C195D0`, `0x00C17160` |
| 11 | VertexBufferLibrary; `0x00C175D0` |
| 12 | IndexBufferLibrary; `0x00C18910`, `0x00C175D0` |
| 13 | SpriteBufferLibrary; separate geometry branch in `0x00C18910` |
| 14 | ModelBufferLibrary; `0x00C175D0` |
| 15 | NodeValueLibrary; `0x00C17280`, `0x00C16DF0` |
| 16 | NodeLibrary; `0x00C17280`, `0x00C16DF0` |
| 17 | VertexDeclarationLibrary; `0x00C195D0` |

Header conversion `0x00C01B60` walked all 18 counts and ordinal bases,
then the summed record-offset table when conversion was needed.
Material fixup `0x00C023C0` also wrote pointers into the model memory even
when the record's endian already matched. A reconstruction must retain a
private mutable copy and must not run this fixup on shared stock file bytes.

For this version-4 model, kind 17 contained one serialized 16-bit index,
followed by two alignment bytes. Constructor `0x00C0E9D0` installed its
`BaseAttribute::Index` implementation. Its size/writer methods at
`0x00C06700` and `0x00C06730`, and fixup `0x00C026E0`, selected that
two-byte representation. Treating it as a 28-byte declaration would consume
the next block's header.

### Serialized library framing and separate records

The indexed offsets locate record starts. An interval ending at the next
indexed start can also contain the following library's header, including
headers for empty libraries. It is not necessarily a record body.
Python/pefile/Capstone checked 1,995 retained instruction boundaries and
original bytes in 27 functions for this framing, the selected record
readers/writers and the older inputs below. Two entries without retained
function boundaries added 25 instructions: a caller-supplied constructor at
`[0x00C08380,0x00C0838F)` and the RTTI-bound DataTable reader at
`[0x00C06860,0x00C068A3)`.

| Original PE locator | Observation |
| --- | --- |
| RTTI table `0x010D1024`; `0x00C06A10`, `0x00C06A69..0x00C06A71` | `BaseAttribute::ChunkInfo` had serialized size eight. Its reader consumed a kind byte at `H+0` and a 32-bit payload size at `H+4`; it did not consume the other three bytes as part of the kind. |
| `0x00C06185..0x00C061E0` in `0x00C06070` | Read ChunkInfo, advanced eight bytes, selected the typed library through `0x00C0B310`, called its Load slot, then advanced by the chunk's payload size. The next header was `H+8+size`. |
| `0x00C082D0..0x00C08367` | The library reader took its count from the first four payload bytes, called each record's Load/GetSize methods and aligned subsequent records to four bytes for version 3 or later. The first record was at `H+12`. |

Python `struct`/`hashlib` walked all 18 chunks in each pinned historical,
B4, B0 and 88 input. Every chunk count matched the indexed header, every
record start matched its relative offset, and the last chunk ended at the
declared mxfv end. The historical VertexBuffer ended at `0x3A70`, followed
by kind 12's header `[0x3A70,0x3A7C)`. The IndexBuffer ended at `0x4ED4`,
followed by empty kind 13's header `[0x4ED4,0x4EE0)` and kind 14's header
`[0x4EE0,0x4EEC)`. These bytes described libraries, rather than extra
vertex attributes or tangent inputs.

The selected separate records also had bounded consumers:

| Record / original PE locator | Observation |
| --- | --- |
| ModelBuffer; `0x00C0E70D..0x00C0E71A`, `0x00C0D310`, `0x00C0A850`, `0x00C0A900` | The factory installed ModelBuffer table `0x010D19CC` and an embedded Index at object `+0x24`. Load read two 32-bit fields, six float bounds and indices. With the selected group count one, its source indices were at `+0x20/+0x24/+0x26`. Load/Save consumed or wrote through `+0x28`, while GetSize reserved 64 bytes. |
| Index; table `0x010D14F0`, `0x00C06700/0x00C06770`; constructor supplied at `0x00C0D40A` | The mxfv version-4 index representation was two bytes. The supplied constructor installed that same Index table for the grouped references. |
| NodeValue; `0x00C0AEC0..0x00C0AFE4`, `0x00C17301..0x00C173DF` | Load copied three flag bytes, advanced past a version-3-or-later alignment byte, then copied a 32-bit field, 18 four-byte scalar slots and three indices. Its selected aligned body was 88 bytes. The separate node consumer selected kind 15 through a Node index and used the first nine scalar slots as float inputs to its matrix setup. |
| Node; `0x00C0D5A0..0x00C0D749` | Read two 32-bit fields, three indices, a version-3-or-later flag/alignment pair and any child indices. The selected child count was zero and its body was 16 bytes. |

The historical ModelBuffer's `[0x4F14,0x4F2C)` was the genuine 24-byte
region outside the checked Load/Save accesses. It was zero-filled in all
four inputs; this did not classify every possible consumer or establish an
authoring purpose for that region. The complete historical ModelBuffer,
NodeValue and Node bodies were exact in B4. The checked node scalars and
indices supplied no additional per-vertex basis table or connected tangent
producer. These record checks did not exclude an unnamed producer elsewhere.

### Selected component sources

All four inputs had 51 indexed records with the same 18 library counts.
Kinds 9 through 12 and 14 through 17 each had one record; kind 13 was empty.
The table gives historical locators and complete aligned-body comparison,
excluding the next library's header.
Stock B4 and B0 used the same record starts; stock 88 relocated the records.

| Kind / record | Historical file span | Exact stock source |
| --- | --- | --- |
| 9 / Shader | `[0x64C,0x668)` | B4, B0 and 88. |
| 10 / Material | `[0x674,0x80C)` | B0. |
| 11 / VertexBuffer | `[0x818,0x3A70)` | No complete-body match among B4, B0 and 88. |
| 12 / IndexBuffer | `[0x3A7C,0x4ED4)` | B4, B0 and 88. |
| 14 / ModelBuffer | `[0x4EEC,0x4F2C)` | B4. |
| 15 / NodeValue | `[0x4F38,0x4F90)` | B4, B0 and 88. |
| 16 / Node | `[0x4F9C,0x4FAC)` | B4, B0 and 88. |
| 17 / VertexDeclaration | `[0x4FB8,0x4FBC)` | B4, B0 and 88, including two alignment bytes. |

The complete 408-byte historical/B0 Material body had SHA-256
`fd42f436af283786a12f6950b66a2e71eb20af7862b827cda377bcaa8c692be5`.
Its 140-byte parameter payload at `[0x748,0x7D4)` had SHA-256
`351104392a7d7f624691b80d92f623198b977d621ea6311b0efad8b3e9085bb8`.
Thus B0 supplied actual historical parameter values, including the three-lane,
12-byte `exLightParam` slot at ordinal four; matching constant names alone
was unnecessary. The B4 and 88 complete material bodies differed by 14 and
two bytes respectively. Their state bytes and descriptor shapes matched,
but their parameter values did not have the same payload hash.

The 64-byte historical/B4 ModelBuffer body had SHA-256
`9a4e5f8c987cc7b2863ad64b98b6f893eb32694d478504d245b37471f3668b16`.
The shared 5,184-byte index payload at historical `[0x3A94,0x4ED4)` had
SHA-256 `1a1b9f57160f262fa1a4951909819e5150024c3ff4b538a9b0bbffc0847b8308`.
Its native source began `0x18` bytes into the IndexBuffer, using the
record's pad byte 12, and ended at the record body's end. Stock
88's corresponding payload was `[0x3AA4,0x4EE4)`.
These identities support composing selected records from different stock
carriers. They do not establish interchangeability of their complete models.

## Material and texture joins

For the checked material record M, the native consumers read these fields.
The pointer fields are results of fixup, not serialized absolute addresses.

| M offset | Consumption |
| --- | --- |
| `+4` | Texture/sampler count T. |
| `+8` | Material parameter count N. |
| `+0x0C..+0x10` | State bytes consumed by `0x00C17160` to produce material flags and state. |
| `+0x12` | Signed 16-bit relative offset of the index/name table. |
| `+0x18` | Fixup wrote `M+0x24`, the N parameter descriptors with stride eight. |
| `+0x1C` | Fixup wrote `M+0x24+8*N`, the parameter payload source. |
| `+0x20` | Fixup wrote `M + s16(M+0x12)`, the T+T+N 16-bit index entries. |

`0x00C195D0` selected parameter names from kind 6 and sampler names from
kind 7. It registered parameter metadata, copied the supplied parameter
values into native storage and built the material's data-table index list.
Shader initialization `0x00C11230`, reached through `0x00C13990` and
`0x00C123F0`, passed those registered names to the vertex and pixel
parameter lookup interfaces. A matching CTAB register layout does not
supply a missing parameter value or sampler name.

`0x00BE2110` read 24-byte dependency rows from the construction descriptor's
`+0x20`, with count at `+0x24`. For each ordinal it submitted the resource
ID through `0x00BF6D50`. It took the resolved resource's `+0x44` object,
then that object's `+0x30` pointer, and stored it in live model `+0x80`
at the same ordinal, bounded by model `+0x96`.
`0x00BF6D50` selected a context-owned lookup or the checked global fallback;
it did not resolve a DDS path literal.

At `0x00C16F89..0x00C16FD5`, the material's first T 16-bit indices selected
that texture-pointer array. The producer filled both sampler pointer arrays
at live material `+0x28` and `+0x2C`, in a material array with stride `0x4C`.
The next T indices supplied sampler names to the shader binder. Thus an
alias rewrite needs a proved ID-to-ordinal-to-texture join, not merely an
equal collection of path strings.

### Historical IDs in stock texture leaves

A Python byte scan enumerated 143,227 entries and read 140,180 regular
stock DATs, 7,444,438,570 bytes, without entering reparse points. It
classified 722 target-token occurrences in 283 files by their bounded
dependency, registration, leaf or literal positions. Thirty occurrences were
standalone `SEDBvtex` leaf registrations across the 12 examined model aliases.
This distinguished a model's reference to an ID from a texture that supplied
that ID.

All three historical dependency IDs survived as stock texture registrations.
Each row below selected one 512x512, depth-one, single-mip GTEX surface at
file `0xB4`; the ID was at `0x6C` and its `xetv` type at `0x7C`.
Native [GTEX source selection](gtex-pwib-loader.md#gtex), independently
applied with Python `struct`/`hashlib`, reproduced the surface bounds.

| Historical dependency ordinal / ID | Stock resource | Surface bytes / SHA-256 |
| --- | --- | --- |
| 0 / `1x6elwclod0001s` | `0x61A9005F` | 262,144 / `34d8fdd1a7de0ca91d398f448dae6ffb558b2f54b7b673f0c3d777b440ce0569` |
| 1 / `22aobDnormal` | `0x61A9005D` | 131,072 / `f8f379f30415e1da61a502e97b3f5df2edc94c6dfa926911abd8176a0b1040e0` |
| 2 / `4lWhacnormal_b` | `0x61A9005E` | 131,072 / `b1a5bdb7b72442920b318fe32d17fbe3369cecb9b1289f2a8e85bb3f64636876` |

The complete stock `0x61A9005F` was 262,324 bytes, SHA-256
`5c54ea9b41151db53c6a819bc8dd5d2f6fc5933c8ca6f58c55fe36c9c5b2d2f6`.
Stock `0x61A9005D` and `0x61A9005E` were each 131,252 bytes, with
SHA-256 respectively
`d6f7987eced59c2f8e6f07bec33771f7364ad938978761eb4a7f1f383ca6caec` and
`65947acd73c68f79cd2be6178af08f525326072e06146d91bbe4c0082eda52c4`.
Their GTEX format indices were 26, 24 and 24 in table order, with type flags
zero. These are concrete stock sources for the historical model's ID rows;
they must be registered in the resource context used by `0x00BF6D50`.

B4's dependency order was `[clod, normal_b, normal]`, while its material
texture-index table was `[0,2,1]`. Its three resolved surfaces had the same
corresponding hashes and GTEX metadata as the historical-ID leaves. Its local
sampler-name order was `_sampler_00`, `_sampler_02`, `_sampler_01`, unlike
the historical/B0 order `_sampler_00`, `_sampler_01`, `_sampler_02`.
The B0 and 88 resolved texture hashes differed from the historical-ID
surfaces. An exact B0 material/program source therefore does not authorize
copying its dependency list or treating B4's sampler order as historical.

## Program and geometry consumers

At `0x00BE1480..0x00BE14F4`, the material loop selected the supplied
shader descriptor and program spans. `0x00C13990` created the shader wrapper;
`0x00C0FCD0..0x00C0FD8E` submitted the supplied spans through renderer
slots at `+0xE8` and `+0xF0` and retained the two returned objects.
The exact stock historical program sources are established separately in
the [shader comparison](seasonal-memory-reconstruction.md#controllers-and-cloud-program).

`0x00C175D0` selected a ModelBuffer's first vertex/index references at
record `+0x24/+0x26`, resolved the VertexBuffer and IndexBuffer libraries,
and consumed the vertex count, stride and declaration selection. Its calls
at `0x00C1785E` and `0x00C178CE` supplied the selected vertex and index
data to the native objects. `0x00C19170` required nonempty vertex and index
libraries before entering this geometry setup. A model name or equal shader
cannot stand in for these selected geometry inputs.

Vertex fixup `0x00C025C4..0x00C025E9` read E from record V's `+6`, set
the eight-byte component table to `V+0x1C`, the following 16-bit table to
`V+0x1C+8*E`, and the vertex payload to
`V+0x1C+10*E+u8(V+0x0F)`. Index fixup `0x00C02610` selected payload
`I+0x0C+u8(I+0x0B)`. These are native pointer producers for the compared
payloads; selecting the last count-times-stride bytes alone is unnecessary.

Declaration builder `0x00C19D04..0x00C19D5F` consumed all E eight-byte
components and their corresponding 16-bit table entries. For each component
C and table value U, it built the four-byte tuple
`[u8(U), C[0], C[1], map(C[2])]`; `0x00C1B980` supplied the map.
`0x00C1E410` converted those E tuples into E renderer descriptors and
submitted the terminated list through renderer slot `+0xC0`. It used
`0x00C1E1D0` and `0x00C1E260` for its two field conversions. This path did
not expand six serialized components into eight components merely because
the vertex program declared eight input registers.

Initializer `0x00B94ABF..0x00B94AD2` installed table `0x010B590C` on
the allocated renderer interface and stored it at global `0x0136B6B0`.
That table's `+0xC0` cell was `0x00B9FCE0`, whose call at `0x00B9FCEB`
forwarded the list to `0x004188B0`. This closes the VFX declaration join to
the [compact-to-D3D9 converter](wrb-vertex-declaration.md#api-array-conversion)
at `0x00419A20`. The original PE's format and usage windows reproduced the
published bounded lookups. The six selected elements can therefore be
compared by their D3D9 semantics and formats, rather than guessed from names.

`0x00C1BAC0` multiplied vertex count by stride and submitted the selected
payload through renderer slot `+0xB8`; its optional owned-copy branch copied
the same span first. `0x00C1E080` performed the corresponding index-buffer
submission. No decoded name or texture permutation in these paths generated
replacement vertex values.

## TEXCOORD5 normal-map basis

The selected shader pair consumed TEXCOORD5 as a signed direction and
handedness for the normal-map basis. This gives the packed field a functional
meaning beyond its declaration semantic. It does not identify the tool or
algorithm that authored those vertex values.

Python `struct` decoded the complete shader-model-3 token streams, including
instruction lengths, declarations, write masks, source swizzles/modifiers,
relative-address operands, immediate definitions and CTAB bindings. The
vertex/pixel streams contained 108 and 66 instructions and ended at their
declared bounds. The exact stock B0 spans matched the historical streams.
The token definitions came from Windows SDK `10.0.26100.0`
`d3d9types.h`, SHA-256
`ce75dc368d5f37506ada4c32062a96ab4d4894fead5d21ad5844fc9f0a2e62c2`.
The [program identities](seasonal-memory-reconstruction.md#controllers-and-cloud-program)
own the complete shader hashes and source spans.

The following locators are file offsets in the historical model and stock
`0x5D1E00B0`, rather than x86 instruction addresses.

| Shader locator | Observation |
| --- | --- |
| Vertex `0x5494` | Declared input `v3` as TEXCOORD5. |
| Vertex `0x5544` | Decoded `v3.xyz` with multiply two and add minus one. |
| Vertex `0x58F8..0x591C` | Applied the three-register linear transform named `worldITMatrix` at `c4..c6`. The optional skinning branch preceded this transform. |
| Vertex `0x5930..0x5950`, output declaration `0x550C` | Repacked the transformed direction to COLOR0.xyz with multiply one half and add one half. Its COLOR0.w retained the input's normalized fourth component. |
| Pixel `0x5F88..0x5F94`, `0x60D8..0x60F8` | Received the already-signed normal in TEXCOORD3 and normalized it. Decoded the packed COLOR0 basis and normalized its direction. |
| Pixel `0x6104..0x6128` | Formed `h * (Q cross N)`, where Q was the normalized decoded COLOR0 direction, N the normalized normal, and h the decoded fourth component. |
| Pixel `0x61D8..0x61FC` | Combined the normal-map components as `x*h*(Q cross N) + y*Q + z*N`. |

The multiplication, addition and source-swizzle interpretation follows
Microsoft's [MAD instruction](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/mad---ps)
and [swizzle contract](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-ps-registers-modifiers-source-register-swizzling).
The [NRM instruction](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/nrm---ps)
defines the normalization step. These equations describe data dependence and
basis orientation, not a CPU emulation of device arithmetic or a rendered
observation. The Q direction supplied the normal map's Y basis; the signed
cross product supplied its X basis. The shader did not derive Q from the
ordinary UV coordinates.

The same vertex stream decoded stored positions as
`storedPosition.xyz * ModelBBoxScale.xyz + ModelBBoxOffSet.xyz` at
`0x5558..0x5564`. Its CTAB placed those two named constants at `c169/c170`.
An additional original-byte check covered 1,632 retained instructions in seven
native functions with the same Python/pefile/Capstone toolchain. Shader setup
`0x00C113A8..0x00C113E9` looked up the two names and retained their handles
at shader `+0x78/+0x7C`. For a supplied, valid bounding box, the default branch
at `0x00C11045..0x00C110DE` built scale as
`(max-min) * 0.5 * max.w`; `0x00C11111..0x00C11181` built offset as
`(max+min) * 0.5`. Helpers `0x00C12420`, `0x00BE58D0`, `0x00BE5980`
and `0x0042E570` supplied the difference, invalid-box check, midpoint and
scalar multiplication. These branches submitted registers 169 and 170.
Non-null shader `+0xAC/+0xB0` pointers could replace the computed vectors;
this static trace did not observe which branch a live model selected.

Model initialization `0x00C175D0` read min/max XYZ from ModelBuffer
`+0x08..+0x1C`. Its SHORT4 position branch at
`0x00C17792..0x00C177DA` put the float32 reciprocal of 32767 from
`0x010D2CBC` in the live maximum's fourth lane. The default scale therefore
accounted for compressed position coordinates, including nonuniform axis
extents. Reconstructing a geometric basis must account for that position
decode, the selected UV set, source normals, handedness, averaging and byte
quantization. A normal-map consumer alone does not authenticate a particular
tangent-frame generation algorithm.

### Native display and packing conventions

The original PE also contained a CPU tangent-display consumer and float-to-byte
packers. Python/pefile/Capstone checked 3,183 retained instruction boundaries
and bytes in 17 selected functions. Two callback entries lacked retained
Ghidra function boundaries: original caller-supplied entry pointers and linear
decodes through their first returns covered another 24 instructions. This did
not execute the functions or identify the historical model exporter.

| Original PE locator | Observation |
| --- | --- |
| `0x00C34A08`, string `0x010D5C78` | Named the enclosing `0x00C34950` consumer `SQEX::CDev::Engine::Dw::Renderer::ModelInstanceNode::DrawTangentSpace`. |
| `0x00C356B0..0x00C356BD`, `0x00C356EE..0x00C356FB` | Setup selected descriptor usages 2 and 13 into debug-row offsets `+0x0A/+0x0C`. Original WRB table entries `0x00F57DB8/0x00F57DCE` mapped those usages to NORMAL0 and TEXCOORD5. |
| `0x00C34EB8..0x00C34F08`, `0x00C34FED..0x00C35053` | The display read the normal's three bytes and the basis's four bytes, subtracted 128 and divided by float 128 at `0x00FB6E84`. It used existing vertex bytes to draw the basis. |
| `0x00C011A8..0x00C01306` | Clamped a finite float4 to `[0,1]`, multiplied by float 255 with `MULPS`, converted each component with x87 truncation and stored four bytes in input-component order. |
| `0x00D82971..0x00D82A66`, helper `0x0042EBA0` | Supplied the same clamp/multiply/truncation shape to another packed float4 consumer. |
| `0x00D740E9..0x00D74131`, `0x00D85510..0x00D85530` | A selected `VertexBufferWriter` setup used stride 16 and callback `0x00D91680`; Write called the supplied function pointer and advanced by the configured stride. |
| `[0x00D91680,0x00D91699)`, `[0x00D91590,0x00D91678)` | The callback passed two source pointers to the second helper, which retained float XYZ and put the packed float4 result into the last four bytes of its 16-byte output. It did not generate a 28-byte cloud vertex or its TEXCOORD5 field. |
| `0x00D9D6A4..0x00D9D6AA`, `[0x00D8A1A0,0x00D8A1C1)` | Another writer selected stride 32 and callback `0x00D8A1A0`, which called `0x00D89EB0`. |
| `0x00D89FA8..0x00D8A0C6` within `[0x00D89EB0,0x00D8A19F)` | That helper initialized bias XYZ to 0.5 and W to zero, multiplied a supplied float4 by 0.5 with `MULPS`, added the bias with `ADDPS`, then called the 255-based packer. XYZ therefore followed an explicit signed-vector encoding before packing. This consumed a supplied vector rather than generating one from triangle geometry. |

The VFX VertexBuffer serializer was a separate path. Original RTTI identified
vtable `0x010D1994` as
`SQEX::CDev::Engine::Vfx::VfxResourceFormat::VfxResourceModelAttribute::VertexBuffer`.
Its slot 2, `0x00C0CB30`, emitted the header, component descriptions, offsets,
padding and supplied vertex data. At `0x00C0CD82..0x00C0CD95` it passed the
existing interleaved payload to `0x009D4600` with length `vertexCount * stride`.
The separate-component fallback called `0x00C0B820`, whose format-8 branch
at `0x00C0B8C0..0x00C0B8DA` interleaved existing component bytes.
The selected byte-order fixup at `0x00C0A300` skipped format 8 in both layouts.
These paths serialized existing packed values rather than deriving the cloud
basis from positions, normals and UVs.

The WRB [usage mapping](wrb-vertex-declaration.md#api-array-conversion)
identified semantics separately from the debug display's arithmetic.
The display's `(b-128)/128` differed from the cloud shader's `2*b/255-1`.
The signed-XYZ writer's exact inputs `-1,0,1` produced bytes `0,127,255`
under the traced operations, while zero in the display's inverse would be 128.
A display decoder does not establish its inverse as the asset author's
quantizer. The two paths must remain distinct when testing reconstruction.

Both packers set x87 control-word bits 10 and 11 before `FISTP` and restored
the previous word. Intel's [rounding-control specification](https://cdrdv2-public.intel.com/874241/253665-090-sdm-vol-1.pdf),
Volume 1, section 4.8.4.1 and Table 4-9, defines that setting as truncation.
It does not set the SSE multiplication's separate MXCSR rounding mode.
The selected writers supply concrete packed-data producers, but no checked
call joined them to the historical mxfv VertexBuffer. Their existence does not
attribute that model's tangent-generation or packing convention.

### Exporter identifiers and vertex import

A bounded Python search of the original PE's ASCII and ASCII-subset UTF-16LE
printable runs checked tangent/binormal, export/author and candidate DCC or
tangent-library identifiers. It did not authenticate an exporter for this
model. The relevant Maya names identified point-registration methods, while
the tangent name identified the display consumer above. A source-file name
or a reference-inspired algorithm is not an exporter attribution.

Python/pefile/Capstone checked 679 retained original instruction boundaries
and bytes in six functions, plus the selected strings, RTTI, format table and
CodeView record:

| Original PE locator | Observation |
| --- | --- |
| `0x0109B90C`, `0x0109B928`, `0x0109B9F0`; methods `0x00A50530/0x00A50600` | `.\Graphics\MayaGrid.cpp` and `MayaGrid::Grid::RegistLine/RegistTriangle` named methods that registered two or three supplied XYZ points in 16-byte entries. They did not identify a model-export tangent calculation. |
| File `[0x00D3E9A0,0x00D3EA20)`, VA `0x0113E9A0` | The PE's CodeView record was `RSDS`, age 1, GUID bytes `38cc927be3fd7c46ab4707332dc8b601`, and a path ending `ffxivgame.pdb`. It named the game executable's PDB, without naming a cloud asset exporter. |
| Vtable `0x010D1994` slot 3, `0x00C0CDD0`; source-name string `0x010D1ED8` | Identified `VfxResourceModelAttribute::VertexBuffer::Load`. The version-3-or-later branch at `0x00C0CFF5..0x00C0CFF8` selected the interleaved payload copy below. |
| `0x00C0D119..0x00C0D156` | Loaded the supplied vertex count and stride, advanced past the serialized padding, allocated `count * stride` bytes, and copied that span into object field `+0x2C`. This import path retained the already packed vertex data. |
| `0x00C0D03C..0x00C0D0C5` | The older planar branch copied each supplied component array, then called `0x00C0B820` to interleave it. It did not recover prequantization values for format 8. |
| `0x00C0B70B`, `0x010D1B80`; format selector `0x00C0B810+8`, table `0x00C0B800` | Named `VertexBuffer::CreateVertexBuffer`. Format 8 selected `0x00C0B7AA`; `0x00C0B7AA..0x00C0B7C1` copied the supplied component-count bytes into each interleaved vertex. This alternate creation helper also consumed existing component values. |

The loader/interleaver and the supplied-vector packer are different producers:
one produces a native copy of serialized data, while the other packs a vector
that its caller already supplied. Neither checked path connected a tangent
calculation to the historical cloud's TEXCOORD5. The identifier search did
not exclude unnamed or statically linked authoring code elsewhere.

## Selected vertex data and reconstruction boundary

Each candidate selected 457 vertices with a 28-byte stride, 12,796 bytes in
total. The native-derived payload bounds and complete hashes were:

| Input | File span | SHA-256 |
| --- | --- | --- |
| Historical | `[0x874,0x3A70)` | `192137537ab446959bc74e7a74a15091335e1ba0f896e686fa7589f44267358c` |
| Stock B4 | `[0x874,0x3A70)` | `ab603b917739ce445e7014e99731e86817db4e9664f1a8160442f6bc53ac2d91` |
| Stock B0 | `[0x874,0x3A70)` | `157549275bd8b6273174b5fe3ad8aa8723c6163b88be844f377d6344acfec8e7` |
| Stock 88 | `[0x884,0x3A80)` | `157549275bd8b6273174b5fe3ad8aa8723c6163b88be844f377d6344acfec8e7` |

The source pad byte was four in historical/B4/B0 and 12 in 88. Each native
payload ended at its VertexBuffer body's end. The following 12 bytes were
the IndexBuffer library header, as bound by the
[chunk reader](#serialized-library-framing-and-separate-records).

All four sources had identical six component descriptors and selector
offsets. The original jump tables at `0x00C1B9F8`, `0x00C1E230`,
`0x00C1E2F0` and selector bytes at `0x00C1E314`, followed by the bounded
WRB conversion tables, selected these inputs:

| Byte offset in stride | D3D9 semantic | D3D9 format | Historical comparison with B4 |
| ---: | --- | --- | --- |
| 0 | POSITION0 | SHORT4 | Exact across all 457 vertices. |
| 8 | NORMAL0 | UBYTE4N | Exact across all 457 vertices. |
| 12 | COLOR0 | UBYTE4N | Exact across all 457 vertices. |
| 16 | TEXCOORD0 | FLOAT16_2 | Exact match to B4's TEXCOORD1 at offset 20. |
| 20 | TEXCOORD1 | FLOAT16_2 | Exact match to B4's TEXCOORD0 at offset 16. |
| 24 | TEXCOORD5 | UBYTE4N | 1,190 differing bytes across 449 of 457 vertices. |

Python compared every element over all 457 strides and checked that the
per-element differences summed to the complete payload's differences.
An independently checked copy recipe swapped B4's four-byte spans at offsets
16 and 20 for each vertex. All first 24 bytes of every resulting stride
matched the historical input exactly. This is an explicit reconstruction
operation; the stock declaration itself did not perform that swap.
No complete four-byte lane from B4, B0 or 88 matched the historical
TEXCOORD5 lane. The unequal full payload therefore does not establish a
missing position, normal, colour or ordinary UV source.

A Python exact-byte search enumerated the same 140,180 regular stock DATs
but searched the payload only in the 1,682 files beginning `SEDBvmdl`.
It found no exact historical vertex-payload occurrence in those carriers.
The broader [raw-DAT and embedded-model search](#embedded-models-and-raw-dat-search)
also found no exact occurrence. Neither result proved that no bit-exact
derivation from stock exists.

A declaration-aware pass used the native version-3-or-later pointer formula
on those 1,682 carriers and checked each source file's recorded hash. It
selected 91 buffers with 457 vertices and compared 271 four-component
UBYTE4N channels, including usages other than TEXCOORD5. None supplied the
exact ordered historical TEXCOORD5 channel, SHA-256
`884fb4d89ab9908ede58e57e5721d5b392d48f6c13f7738a6705be5edfe0607f`.
The comparison did not reorder vertices, search other component formats or
establish absence of an equivalent derivation.

### Stock-only basis derivation tests

Python `struct`/`math` evaluated geometric candidates from the pinned B4
positions, normals, both half-float UV sets, ModelBuffer extents and the
864 indexed triangles. Historical TEXCOORD5 was the comparison target;
predictions did not contain a historical per-vertex lookup. All 457 target
fourth components were zero, giving the shader handedness `h=-1`.

The tested families included triangle `dP/du` and `dP/dv`, cross-product
orientation, projection against the decoded normal, normalization, face/area/
corner-angle weights, alternative position/normal decodes, axis permutations
and floor/nearest/ceiling byte quantizers. Their failure did not identify the
original exporter's weighting or quantization convention. In particular,
regenerating a basis from quantized DAT fields need not reproduce values
authored before those fields were quantized.

For two representative B4-only candidates, accumulated `dP/du` was projected
against the source normal and normalized, then crossed with the normalized
source normal to produce Q. The tested normal
decode was `b/127-1` before normalization, an authoring hypothesis rather than
the shader's `2*b/255-1` decode. Packing used
`floor(clamp((Q+1)*127.5,0,255))`, with alpha zero. The table separates
the direction bytes from the 457 matching alpha bytes.

| B4 UV / accumulation weight | Equal XYZ bytes of 1,371 | Equal complete vertices of 457 |
| --- | ---: | ---: |
| UV1 / corner angle measured in UV space | 484 | 43 |
| UV0 / corner angle measured in decoded position space | 475 | 44 |

Another bounded family transformed B4/B0/88's existing packed frames using
each triangle's Jacobian between the two ordinary UV parameterizations. It
tested both frame-axis orders/signs, source-normal decodes, accumulation
weights and quantizers. Its highest byte-match candidate reproduced 155 of
1,371 direction bytes and six complete vertices; all 457 alpha bytes matched.
It did not recover the target from an existing stock basis.

A separate Python pass checked the retained hashes of all 1,682 standalone
VMDL carriers and found 91 matching-count buffers. Exact unique B4 POSITION0
byte tuples established a vertex correspondence in 89 buffers. Comparing
all 449 declared four-byte channels after that correspondence produced no
exact historical TEXCOORD5 channel. This extended the ordered-channel test
to attribute-based vertex reordering and raw channels regardless of usage.
It did not decode another packed format into equivalent vectors or search
embedded models. The broader search below also checked different vertex
counts through exact position-tuple correspondence.

No tested candidate derived the target lane. Close vector directions or matching
alpha values do not establish exact normal-map lighting, and these tests did
not execute a renderer. The remaining discriminator is an attributed source
or stock-only rule that reproduces the historical basis, including its
averaging and byte packing; original unquantized inputs or an authenticated
exporter would distinguish further hypotheses. The bounded failures do not
prove such a stock-only rule cannot exist.

### Authoring and packing controls

Python `struct`/`math` evaluated per-corner projection, normalized face
partials, UV orientation, raw sums/weighted averages, final normalization and
float32 staging. These were reference-inspired hypotheses, not executions or
complete reproductions of the following libraries and tools:

- Microsoft's [D3DX tangent-frame contract](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxcomputetangentframeex).
- Autodesk's [Maya tangent/binormal construction](https://help.autodesk.com/cloudhelp/2015/ENU/MayaLT/files/Appendix_A_Tangent_and_binormal_vectors.htm).
- The [MikkTSpace implementation](https://raw.githubusercontent.com/mmikk/MikkTSpace/3e895b49d05ea07e4c2133156cfa94369e19e409/mikktspace.c)
  at commit `3e895b49d05ea07e4c2133156cfa94369e19e409`.

No source tied any of those authoring implementations to this retail model.
The tests included the native display's 128-centered decode and the staged
signed-vector encoder above. Predictions used only stock B4 geometry,
normal/UV fields and topology. Historical values remained the comparison
target, without per-vertex fitting or a lookup of historical output bytes.

| Tested family | Candidate rows | Highest equal XYZ bytes of 1,371 | Complete vertices of 457 in that row |
| --- | ---: | ---: | ---: |
| Per-corner/partial-normalization and packing variants | 32,256 | 419 | 25 |
| Raw sums/averages without final vector normalization | 10,080 | 402 | 26 |
| Quad-partial controls | 1,152 | 381 | 21 |

Every reported row also matched the 457 zero alpha bytes. No candidate matched
all 1,828 target bytes. The quad controls solved least-squares partials from
stock positions/UVs over four-vertex adjacent-triangle pairs. The 864-triangle
topology had 1,272 interior edges and 48 boundary edges. All edge pairs and
two fixed pairing rules supplied candidate quads, without identifying a unique
original quad partition. Their failure did not exclude other source topology
or the complete referenced implementations.

Stock B4's deinterleaved TEXCOORD5 control had SHA-256
`6a3207822195a811e76f31c60f8664450d25f6d57fbc3e64b13385e11f79f3eb`.
A 96-row decode/repack control compared affine decodes, optional
renormalization, float32/float64 evaluation and eight packers. Several inverse
affine pairs reproduced its bytes by construction. No renormalized row did:
the best retained 1,166 of 1,371 XYZ bytes and 282 complete vertices.
An inverse that preserves supplied stock bytes is not a derivation of the
missing historical basis. The native pack order alone did not establish the
geometry, averaging or normalization that supplied its input vector.

A separate comparison decoded the historical target's XYZ with the cloud
shader's `2*b/255-1`: vector lengths ranged from about 0.788567 to 1.010053.
Stock B4's corresponding range was 0.851288 to 1.010053. These were
measurements of stored bytes, without feeding historical values into a
prediction. Final normalization remained an authoring hypothesis to test;
neither the shader decoder nor the traced packer authenticated that step.

### Higher-precision stock inputs

A declaration-aware Python `struct`/`hashlib` audit authenticated the retained
1,682 standalone stock VMDL files and parsed 1,804 vertex buffers. All checked
`mxfv` versions were 4. The native payload, component, format and usage
conversions above selected these declaration types:

| D3D9 type | Declared fields across the checked buffers |
| --- | ---: |
| FLOAT16_2 | 2,028 |
| FLOAT16_4 | 245 |
| SHORT4 | 2,078 |
| UBYTE4N | 3,794 |

No field used 32-bit float or normalized 16-bit integer components.
Every field's type width matched its bounded lane width. The historical,
B4, B0 and 88 declarations remained the six fields listed above, without
a separate higher-precision position, normal or basis declaration.

Exact B4 POSITION0 tuple matching covered all 457 reference positions in
89 buffers. None covered all reference tuples with a different vertex count
or duplicate reference rows. Their float fields were ordinary FLOAT16_2
TEXCOORD0/1 and, in a subset, TEXCOORD2. They supplied no connected
higher-precision POSITION0, NORMAL0, TANGENT0, BINORMAL0 or TEXCOORD5 source.
The audit did not equate the 100 FLOAT16_2 POSITION0 declarations to the
four-component reference, or prove correspondence through other transforms.

Printable metadata in the selected four models included resource paths and
aliases, without an authenticated exporter identity. Separate VINS and shader
extents remained bounded as above. The
[chunk and record readers](#serialized-library-framing-and-separate-records)
identified the apparent vertex/index tails as library headers and bounded
the actual zero-filled ModelBuffer residual. Those selected records supplied
no connected tangent producer. The declaration inventory alone did not
cover models embedded in other container types or transformed position
correspondences.

### Embedded models and raw DAT search

A sequential Python `hashlib`/byte-search pass read all 140,180 regular DATs,
7,444,438,570 bytes, after enumerating 143,227 entries without reparse points
or enumeration errors. Every read remained stable by the checked file
metadata. The retained path set and all 1,682 previously pinned standalone
carrier hashes matched. A complete repeat inventory also matched all
140,180 path/size/SHA-256 entries from the retained full-corpus manifest,
without added, missing or changed files. This established consistency
between the checked inventories, without claiming the installation's
historical provenance for every file.

The pass searched every byte offset for `SEDBvmdl`, little-endian `mxfv`,
reversed `vfxm` (bytes `76 66 78 6D`), the 12,796-byte historical vertex
payload and the 1,828-byte deinterleaved historical TEXCOORD5 sequence. Both
complete target sequences had zero contiguous occurrences. This raw-byte
result covered the stored bytes, without decoding compressed or
unrecognized representations.

The 1,739 `SEDBvmdl` signatures comprised 1,682 standalone carriers,
46 version-4 embedded models and 11 older embedded models. All 1,728
little-endian `mxfv` signatures were inside the standalone or version-4
embedded spans. The parser checked native counts, ordinal bases, offset
tables, library headers and declared model ends. The 46 embedded models
supplied 48 vertex buffers in four PWIB files. Their declaration fields were
SHORT4 positions, UBYTE4N normals/colours and ordinary half-float colour/UV
fields; none supplied a complete correspondence to the 457 reference
positions. Fourteen buffers covered only some reference tuples; 34 covered
none.

One concrete candidate was stock resource `0x15D90000`, 4,973,264 bytes,
SHA-256 `10b8d7dc501a14a640e8338b0e17d2cc521756583fe4ed68859071d004b453e6`.
Its embedded `SEDBvmdl` at file `0x28190` selected mxfv
`[0x282A4,0x2959C)` through its `ldmv` descriptor. The complete candidate
span `[0x28190,0x2A2B4)` had SHA-256
`ce1c7c1070168088c9f654fe17ccf92776abd8ceb36ca09a5cd9dd5618bcc24e`
and supplied 99 vertices. This was a structural model/declaration match to
the native format, without a proof of the enclosing PWIB model handler's
admission. The [PWIB split-container boundary](gtex-pwib-loader.md#pwib)
still applied; a raw nested signature alone was not a reusable engine input.

#### Older big-endian model inputs

The 11 older embedded models were in stock `0x15D90001`,
1,189,004 bytes, SHA-256
`9b1fca85933590e6e4d66a1b670408383519ef70b788a5118c3d61cc3c1df891`.
Their descriptor-selected models had reversed tag `vfxm`, big-endian
version 2 and variant 2.

| Native locator / Python observation | Binding |
| --- | --- |
| `ModelChunkInfo` table `0x010D114C`; `0x00C0BB11`, `0x00C0BBC0` | The below-version-3 self header was 36 bytes, rather than the version-4 indexed header. Starting after FileInfo and that header, Python walked all 17 older library chunks to each descriptor's exact model end. |
| `DataTable` table `0x010D1C58`; `0x00C067F0`, `[0x00C06860,0x00C068A3)` | The below-version-3 component record was seven bytes: three one-byte fields followed by a 32-bit component length at `+3`. |
| `0x00C0CE26..0x00C0CE32`, `0x00C0D003..0x00C0D119` | The older VertexBuffer used a one-byte element count. The selected version-2 variant took the interleaved branch, with its pad byte before the count-times-stride source; it did not take the planar import branch. |

Python parsed one buffer in each older model, with vertex counts
384, 24, 24, 6, 798, 24, 480, 60, 192, 6 and 72. For example, the first
`SEDBvmdl` at file `0x158F4` selected model `[0x15A08,0x18682)` and a
384-vertex, stride-24 buffer. Every payload ended at its actual library
body end. Native format/usage lookups labelled their fields as SHORT4
POSITION0, UBYTE4N NORMAL0, UBYTE4N or FLOAT16_4 COLOR0 and FLOAT16_2
TEXCOORD0/1. No field supplied FLOAT32 geometry or TEXCOORD5. After
converting SHORT4 component endianness for exact tuple comparison, each
buffer covered at most one of the 457 reference positions. No complete
correspondence or connected higher-precision basis source was established.
The full-corpus search found exactly these 11 reversed-tag occurrences,
all inside the qualified `SEDBvmdl` descriptor-selected spans. No standalone
or otherwise unwrapped `vfxm` candidate, new little-endian reversed-tag model,
or rejected signature remained. This did not qualify the older renderer's
complete construction path.

Across the 1,852 version-4 buffers, exact position correspondence still
selected 89 buffers and 449 declared four-byte lanes. Every comparison
covered all 1,828 target bytes, with zero exact matches. The 11 older
buffers added no complete correspondence. Different counts, duplicate
reference rows and extra positions were recorded rather than silently
discarded. These results did not equate transformed geometry or other
packed formats to the reference, combine partial meshes into a source, or
authenticate a tangent-generation algorithm. An equivalent decoded source
with a proved reference correspondence, or an attributed generator using
stock inputs, remained the discriminator for replacing the historical lane.

### Decoded attribute and raw-array alternatives

Python 3.12 and NumPy 2.5.0 decoded every field in the 89 buffers with
complete exact B4 position correspondence. The comparison retained reference
vertex order and each declared type's interpretation:

| Declared type | Fields | Decoding used |
| --- | ---: | --- |
| SHORT4 | 89 | Four signed 16-bit values, without normalization. |
| UBYTE4N | 267 | Four bytes divided by 255. |
| FLOAT16_2 | 182 | Two half floats, with the D3D expansion Z=0 and W=1. |

Each field was tested with both
[native pack profiles](#native-display-and-packing-conventions): the unsigned
clamp/255/truncate path and the signed-vector wrapper with separate XYZ/W
biases. A third control applied the cloud vertex shader's `2*x-1` to each
UBYTE4N field's XYZ, preserved its normalized W, then used the signed wrapper.
All 1,343 comparisons checked the complete 1,371 XYZ bytes and 1,828 total
bytes. None matched even the complete XYZ sequence. This tested decoded
alternatives to copying raw field bytes, without fitting vectors or changing
their geometry correspondence.

The separate stored-array search considered finite vectors in these layouts:

| Candidate storage | Interpretation |
| --- | --- |
| FLOAT32 XYZ or XYZW | Consecutive 32-bit scalar components. |
| FLOAT16 XYZW | Four consecutive half floats. |
| Signed or unsigned normalized 16-bit XYZW | Signed values divided by 32767, or unsigned values divided by 65535. |
| Normalized byte XYZW | Bytes divided by 255. |
| Signed normalized packed 10:10:10 | Three signed fields at bit offsets 0, 10 and 20, divided by 511; the high two bits were ignored. |
| BGRA or ARGB bytes | Complete four-byte channel permutations of the comparison target. |

The [D3D9 declaration definitions](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddecltype)
and [colour type](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcolor)
bound the API interpretations. Their presence in an API did not establish
that this asset used them. The packed-10 bit placement and raw-array layout
were explicit candidate interpretations.

The scalar searches used both byte orders and every byte alignment, with
tightly packed 457-vector arrays. Integer/half lookup tables and exact
float32 output preimages filtered candidates before a complete forward
comparison. Both native pack profiles used separate float32 stages and
truncation. Round-to-nearest was the explicit SSE arithmetic assumption;
the inspected packers did not set MXCSR's rounding mode. Input values were
read unchanged from stock bytes; the historical sequence served only as the
comparison target.

The callback received all 7,444,438,570 stored bytes in 140,258 blocks across
140,176 nonempty DATs; the four empty files supplied no vector input. It
carried at most 7,311 bytes across blocks, covered every possible start for
the largest 7,312-byte candidate, and reset between files. No tested codec
supplied even a complete matching XYZ sequence; the BGRA/ARGB searches also
returned zero complete occurrences. Authored positive controls, boundary
splits, preimage endpoint checks and last-row mutation controls exercised
the search without generating historical-derived predictions.

A three-component source could supply an XYZ lead, but did not source the
historical alpha. In particular, D3D's FLOAT3 and packed-10 declarations
expanded W to one. Such a source required a separate proved W operation.
Raw hits also required their container, native consumer and reference vertex
correspondence before use. The search did not decode compressed data,
interleaved padding, arbitrary vertex permutations or an unnamed codec.

### Compressed carriers and interleaved rows

The presence of zlib in the executable did not establish a compressed cloud
model. Python/pefile/Capstone checked original bytes at 3,972 retained
instruction boundaries in 19 functions, including the model readers and
these decompression surfaces:

| Original PE locator | Observation |
| --- | --- |
| `0x00D4F51E..0x00D4F521` | The initialization thunk supplied windowBits 15 to `0x00D4F440`. This selected zlib framing, rather than a raw DEFLATE stream. |
| `0x00D42609`, `0x00D42632..0x00D4264B`, `0x00D426BA..0x00D426BF` | The [PackRead path](sqpack.md#resolver-capabilities) initialized the stream, supplied 1,024-byte output blocks and drove inflate until result 1. |
| `0x00908F00`, `0x00908F3F`, `0x00909058`, `0x009090E1` | A separate file operation selected `N` or `Z`; its Z branch used initialization `0x00D40E50` and inflate wrapper `0x00D40D60`. This observation did not identify it as a model loader. |
| `0x00E3FDA2..0x00E3FDFC`, caller `0x00DA1D22` | A third wrapper supplied flush value 4 and returned decompressed length. Its caller belonged to the [stream-record path](../net/stream-record-parser-pair.md#static-evidence). |

The direct relative-call search found no unresolved instruction-boundary
candidate for those checked entry points. No literal address word for them
was found in the original PE. That finite reference search did not exclude
computed indirect calls or another decoder, and established no connection
from these paths to the cloud model.

The [vertex import path](#exporter-identifiers-and-vertex-import) read
unsigned 16-bit count and stride at `0x00C0D119/0x00C0D11D`, skipped the
source padding and copied their product. The older separate-component path
interleaved supplied arrays. These operations preserved existing values;
they supplied no decompression or tangent-generation step in the checked
branches.

The stored-stream search used Python 3.12 `zlib`, `struct` and `hashlib`.
Its [RFC 1950](https://datatracker.ietf.org/doc/html/rfc1950#section-2.2)
discriminator required method 8, CINFO 0 through 7 and the CMF/FLG check
modulo 31. These headers described 256 through 32,768-byte windows. The
decoder received the header itself, used windowBits 15, and tracked EOF,
consumed input, unused tail and output hashes through the
[Python streaming API](https://docs.python.org/3.12/library/zlib.html#zlib.Decompress.eof).
Each candidate had a 16 MiB input limit and an 8 MiB output limit, with a
256-byte first feed and 1 MiB later feeds. Decoding was one level only.
Dictionary, incomplete, error and resource-limit outcomes were accounted
for separately. A valid header alone did not authenticate a compressed
asset or supply a preset dictionary.

The authenticated inventory yielded 3,700,992 candidate starts. Reproduction
on the same hash-checked handles for all 64,194 candidate-bearing files
matched every recorded header, offset, status and output hash. Outcomes were
1,713,625 dictionary requests, 1,987,051 errors and 316 incomplete streams.
None completed or reached either resource limit. No returned output contained
`SEDBvmdl`, `mxfv`, `vfxm` or the complete historical TEXCOORD5 byte sequence.
Dictionary requests did not establish valid complete assets, and their
dictionaries remained unavailable. This result did not cover headerless
DEFLATE, another codec or a later layer of decoding.

The interleaved alternative search used the 89 complete exact position
correspondences, with native strides 28 or 32. It considered every fitting
byte offset, including offsets that overlapped declared fields, for the
float32, half4, normalized integer and packed-10 layouts listed
[above](#decoded-attribute-and-raw-array-alternatives), plus a raw four-byte
control. Multi-byte candidates used both byte orders and the applicable
native pack profiles. Source rows followed only the proved POSITION0 join.
Finite scalar seed checks preceded complete 457-row XYZ comparison; only
an XYZ survivor could proceed to the separate W check. Three-component
inputs supplied no alpha source. Reinterpreting an overlapping field was
a search hypothesis, not a native declaration or consumer binding.

Those buffers had no wholly undeclared four- or eight-byte slot: all 8,252
possible layout slots overlapped declared fields. The separate all-offset
search evaluated 43,104 codec/offset/profile interpretations. It rejected
38,513 through decoded scalar seeds and 4,549 through nonfinite seeds.
The remaining 42 received complete XYZ comparisons; none matched, so no
candidate proceeded to alpha comparison or full-source promotion. Authored
457-row positives covered all 24 codec/profile/byte-order combinations,
including normalized integer inputs, alongside padding, alignment and
last-row mutation controls. This excluded the tested interpretations in
the exact-reference buffers, without equating other geometry, partial meshes
or arbitrary channel/vertex permutations to the reference.

No equivalent historical lane was established by the compressed or
interleaved searches. A source behind a bound dictionary or another decoded
representation with a proved reference join, or an attributed basis producer
using stock inputs, remained the discriminator.

The concrete composition candidate is the historical/B0 material and shader
pair, B4's ModelBuffer and VINS, the shared index/node/declaration records,
and the stock leaves carrying the historical dependency IDs, with B4's two
UV channels repacked in historical order. It must retain
the historical parameter/sampler joins rather than combine whole alternate
models. Its remaining asset discriminator is an attributed source or a
verified derivation for the historical TEXCOORD5 values. Supplying the
historical channel values in RAM would change delivery while still supplying
historical data. The separate unmatched Halloween numeric curves remain in
the [curve finding](seasonal-memory-reconstruction.md#gridania-halloween-data-that-must-be-reconstructed).

These locators establish producers and input bindings. They do not qualify
allocation ownership, engine-thread admission, cache identity, reload lifetime
or a new live reconstruction writer. The
[reconstruction obligations](seasonal-memory-reconstruction.md#admission-reloads-and-exactness)
remain applicable. After those sources and native admission are attributed,
matched city/time/transition observations through updates and reloads would
settle rendered parity. No such construction or execution was performed.
