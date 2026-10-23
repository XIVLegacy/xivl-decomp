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

### Selected component sources

All four inputs had 51 indexed records with the same 18 library counts.
Kinds 9 through 12 and 14 through 17 each had one record; kind 13 was empty.
The table gives historical locators and complete aligned-record comparison.
Stock B4 and B0 used the same record starts; stock 88 relocated the records.

| Kind / record | Historical file span | Exact stock source |
| --- | --- | --- |
| 9 / Shader | `[0x64C,0x674)` | B4, B0 and 88. |
| 10 / Material | `[0x674,0x818)` | B0. |
| 11 / VertexBuffer | `[0x818,0x3A7C)` | No complete-record match among B4, B0 and 88. |
| 12 / IndexBuffer | `[0x3A7C,0x4EEC)` | B4, B0 and 88. |
| 14 / ModelBuffer | `[0x4EEC,0x4F38)` | B4. |
| 15 / NodeValue | `[0x4F38,0x4F9C)` | B4, B0 and 88. |
| 16 / Node | `[0x4F9C,0x4FB8)` | B4, B0 and 88. |
| 17 / VertexDeclaration | `[0x4FB8,0x4FBC)` | B4, B0 and 88, including two alignment bytes. |

The complete 420-byte historical/B0 Material record had SHA-256
`a6db026ae2f14a05464c7d31b097a2c71c348b8d18bebcfc2141df1f0f39aa03`.
Its 140-byte parameter payload at `[0x748,0x7D4)` had SHA-256
`351104392a7d7f624691b80d92f623198b977d621ea6311b0efad8b3e9085bb8`.
Thus B0 supplied actual historical parameter values, including the three-lane,
12-byte `exLightParam` slot at ordinal four; matching constant names alone
was unnecessary. The B4 and 88 complete material records differed by 14 and
three bytes respectively. Their state bytes and descriptor shapes matched,
but their parameter values did not have the same payload hash.

The 76-byte historical/B4 ModelBuffer had SHA-256
`2e1ba1e88f61d077ac96817128892c8e52c6e8a73b34a0cb17edeeece44899f9`.
The shared 5,184-byte index payload at historical `[0x3A94,0x4ED4)` had
SHA-256 `1a1b9f57160f262fa1a4951909819e5150024c3ff4b538a9b0bbffc0847b8308`.
Its native source began `0x18` bytes into the IndexBuffer, using the
record's pad byte 12, and ended 24 bytes before that record's end. Stock
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
payload ended 12 bytes before the VertexBuffer record's end. Those trailing
bytes were retained as opaque record data, not counted as uploaded vertices.

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
It did not search embedded models in other container types, decode alternate
vertex encodings, or prove that no bit-exact derivation from stock exists.

A declaration-aware pass used the native version-3-or-later pointer formula
on those 1,682 carriers and checked each source file's recorded hash. It
selected 91 buffers with 457 vertices and compared 271 four-component
UBYTE4N channels, including usages other than TEXCOORD5. None supplied the
exact ordered historical TEXCOORD5 channel, SHA-256
`884fb4d89ab9908ede58e57e5721d5b392d48f6c13f7738a6705be5edfe0607f`.
The comparison did not reorder vertices, search other component formats or
establish absence of an equivalent derivation.

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
