# GTEX and PWIB loader fields

This page records the fields consumed by the retail 1.23b client loaders for
standalone GTEX and PWIB resources. The reviewed `ffxivgame.exe` has SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The GTEX fields and PWIB boundary fields below are big-endian. The SEDBRES
consumer described below also handles an in-place endian conversion.

## GTEX

The dispatcher at `0x004323d0` selects the GTEX loader at `0x00431f30` after
checking the `GTEX` signature. The loader consumes this header surface:

| Offset | Width | Loader-backed meaning |
|---|---:|---|
| `+0x06` | 1 | Client texture-format table index |
| `+0x07` | 1 | Mip level count |
| `+0x09` | 1 | Texture flags |
| `+0x0a` | 2 | Width |
| `+0x0c` | 2 | Height |
| `+0x0e` | 2 | Depth |
| `+0x10` | 4 | Optional surface-offset table base, relative to the blob |
| `+0x14` | 4 | Nonzero source-data base relative to the blob; zero selects a supplied source view |

Flag bit 0 selects a cube texture, otherwise bit 1 selects a volume texture,
otherwise the object is a 2D texture. When both type bits are set, the cube
branch wins. Bit 2 changes one creation argument to 4; its higher-level meaning
is not established.

The constructors at `0x00418bf0`, `0x00418d00`, and `0x00418e00` retain the
fields. The creation path at `0x004312d0` passes width, height, depth, mip count,
and the table-selected D3D format to `D3DXCreateTexture`,
`D3DXCreateCubeTexture`, or `D3DXCreateVolumeTexture` as appropriate.

When the offset-table base is nonzero, `0x00432500` selects one eight-byte
entry per face and mip level and reads its first big-endian dword as a
per-surface offset. It returns:

```text
blob + source-data base + per-surface offset
```

The upload loop at `0x00431e20` passes that result through `0x00431080` to a
D3DX load-from-memory call. With a zero source-data base, it instead adds the
per-surface offset to the supplied view's pointer, after checking the offset
against that view's byte length. The [selected PWIB consumer](#selected-restxb-consumer)
below selects this branch. This makes `+0x14`, not `+0x1c`, the proven data
boundary. No function in the reproduced loader path reads `+0x1c`, so it has
no promoted extent meaning.

A header-only census of 21,161 retail GTEX resources found every `+0x14` value
within its file, with no zero values. The observed bases were 32, 48, 64, and
96 bytes; 48 occurred 11,272 times, 32 occurred 6,974 times, 64 occurred 2,914
times, and 96 occurred once. The census corroborates the loader field but does
not narrow the format to those four values.

### Cube upload boundary

For flag bit 0, loader `0x00431f30` selects constructor `0x00418d00`.
Its type scalar 2 reaches texture-object `+0x0c` through `0x00431710`.
Creation `0x004312d0` calls `d3dx9_41.dll!D3DXCreateCubeTexture` at VA
`0x0043139b`, through thunk VA `0x009fc786`, with its output pointer at
object `+0x30`. Microsoft's
[creation contract](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxcreatecubetexture)
identifies that output as `IDirect3DCubeTexture9`.

Upload loop `0x00431e20` iterates outer ordinal `f` from 0 through 5 and
inner mip `m` with `0 <= m < retainedMipCount`, read from object `+0x24`.
VA `0x00431e70..0x00431e7a` selects eight-byte table entry
`headerMipCount * f + m`, using header byte `+0x07` as the face stride.
At VA `0x00431f00`, it passes `f` and `m` as the first two stack arguments
to uploader `0x00431080`. In the type-2 branch, stack tracking at VA
`0x0043110b..0x0043111a` establishes the virtual call arguments as the
object `+0x30` pointer, unmodified `f`, unmodified `m`, and an output-surface
pointer. No face permutation intervenes.

The API interpretation of virtual slot `+0x48` is
[`GetCubeMapSurface`](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3dcubetexture9-getcubemapsurface).
Microsoft's
[`d3d9.h` interface declaration](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L1378-L1404)
places it at zero-based slot 18, or byte offset `0x48` for 32-bit pointers.
Its first parameter is `D3DCUBEMAP_FACES`, followed by the mip level.
The observed numerical mapping is `f -> API face argument f`.
Microsoft's
[`D3DCUBEMAP_FACES` definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcubemap-faces)
assigns those arguments as follows:

| Table face ordinal | API argument | API face |
|---:|---:|---|
| 0 | 0 | `D3DCUBEMAP_FACE_POSITIVE_X` |
| 1 | 1 | `D3DCUBEMAP_FACE_NEGATIVE_X` |
| 2 | 2 | `D3DCUBEMAP_FACE_POSITIVE_Y` |
| 3 | 3 | `D3DCUBEMAP_FACE_NEGATIVE_Y` |
| 4 | 4 | `D3DCUBEMAP_FACE_POSITIVE_Z` |
| 5 | 5 | `D3DCUBEMAP_FACE_NEGATIVE_Z` |

Each entry's first big-endian dword selects the source offset by the GTEX
arithmetic above. Nonzero header `+0x14` uses helper `0x00432500`.
Zero `+0x14` uses the supplied view and checks only the start offset at VA
`0x00431ee5`. Neither inspected path reads the entry's second dword to bound
the upload. A missing table selects offset zero.

The cube branch obtains destination surface dimensions and reaches the common
surface upload at VA `0x00431273..0x004312a5`. It forwards the resolved source
pointer unchanged to `d3dx9_41.dll!D3DXLoadSurfaceFromMemory`, through thunk
VA `0x009fc76e`, with source and destination rectangles starting at zero and
using those dimensions. The GTEX callsite passes zero as the uploader's last
argument, which is distinct from the flag-bit-2 argument. No source copying,
row or column reversal, rotation, texel shuffle, or face-specific source
coordinate transform was observed in the complete `0x00431e20` and
`0x00431080` bodies before this API boundary. The
[D3DX upload contract](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxloadsurfacefrommemory)
handles format conversion, including compressed formats. Imported-library
operations and actual device behavior are outside this observation.

This establishes face-major table consumption and API argument identity for
the pinned loader. No retail cube resource or live cube upload was inspected.
The 2D-only corpus boundary below remains unchanged. Source alias names,
stored texel orientation, producer intent, face-to-camera basis, renderer
sampling conventions, and end-to-end rendered orientation remain unknown.

### Retail format mappings

The format index reaches two parallel tables. Creation and upload functions
`0x004312d0` and `0x00431080` read a little-endian dword from
`0x00f637b8 + index * 4` and pass it as `D3DFORMAT`. Helpers `0x00433240`
and `0x00433300` read the bits-per-pixel dword and block-format byte from a
0x28-byte metadata record at `0x00f63c3c + index * 0x28`.

Only three indices occur in the 21,161-file retail census:

| GTEX index | Files | D3DFORMAT value | Direct3D name | Bits per pixel | Block format |
|---:|---:|---:|---|---:|---|
| 4 | 664 | 21 | `D3DFMT_A8R8G8B8` | 32 | no |
| 24 | 13,587 | `0x31545844` | `D3DFMT_DXT1` | 4 | yes |
| 26 | 6,910 | `0x35545844` | `D3DFMT_DXT5` | 8 | yes |

The numeric-to-name assignments follow Microsoft's authoritative
[`D3DFORMAT`](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat)
definition. The numeric API table, rather than a nearby metadata label,
establishes these names.

### 2D writer field boundary

The complete normal control flow of loader `0x00431F30`, upload loop
`0x00431E20`, source helper `0x00432500`, uploader `0x00431080` and creation
method `0x004312D0` establishes this field-use boundary. This is static
consumption, not device acceptance of a rewritten resource.

| Stored field | Consumer and rule |
|---|---|
| Format byte `+0x06` | Creation and upload index the API table at `0x00F637B8`. Index 24 requests DXT1. Index 3 reads API value 22 at `0x00F637C4`, or `D3DFMT_X8R8G8B8`; index 4 requests `D3DFMT_A8R8G8B8`. Preserve the distinction between the two 32-bit formats. |
| Mip count `+0x07` | The 2D constructor retains it unchanged; `0x00431E20` uses the retained count for iteration and the header count for table indexing. Creation forwards it to D3DX. Zero mips gives no iterations in this upload loop. |
| Width/height `+0x0A/+0x0C` | Loader decodes big-endian words; the 2D constructor raises zero to one before retention and creation. Upload uses destination surface dimensions obtained through the Direct3D object, not a stored payload-length word. |
| Table base `+0x10`, source base `+0x14`, entry first dword | Honored by the source arithmetic above. With a supplied view and zero source base, `0x00431EE5` checks only the start offset against the view length. |
| Entry second dword | Not read by these upload/source methods as an extent guard. The descriptor builder writes an encoded size there; preserving or recomputing it remains necessary for a coherent file. |
| Flags `+0x09`, depth `+0x0E` | Retain the texture-type/creation semantics above. A 2D profile does not establish the cube or volume resource contract. |

The index-3 metadata record at `0x00F63CB4` supplies 32 bits per pixel and
a zero block-format byte at `+0x21`, giving the linear source-pitch path.
The numeric API names follow the linked Microsoft `D3DFORMAT` definition.
An icon described as XRGB must first be authenticated and shown to carry
index 3; a 32-bit payload alone cannot distinguish it from index 4. No new
icon member identity or runtime upload was established here.

The 2D constructor normalizes width at `0x00418C1B..0x00418C28` and height
at `0x00418C2B..0x00418C34`. It retains the input mip count through
`0x00418C59/0x00418C78` and `0x0043176B/0x00431770`. Creation forwards
zero mips at `0x00431491/0x00431494`, while the upload loop's test at
`0x00431E65/0x00431E67` prevents uploads for that count. This distinguishes
constructor normalization, API arguments and actual loop iterations.

The audit used the pinned 15,996,808-byte executable, Python 3.12.0,
pefile 2024.8.26 and Capstone 5.0.7, x86-32 at image base `0x00400000`.
Entry-seeded decoding reached every normal local branch in the five methods
without an unresolved branch. Calls into D3DX and device virtual methods
remain external boundaries. The native methods do not establish complete
source-footprint bounds, table bounds or a general malformed-input policy.

#### GTEX index 22

Decimal GTEX index 22 selects little-endian dword 26, `0x0000001A`, from VA
`0x00F63810`, the entry at `0x00F637B8 + 4 * 22`. The observed 2D creation
path reads it at VA `0x00431449`, pushes it at VA `0x0043145F`, and passes it
to `d3dx9_41.dll!D3DXCreateTexture` at VA `0x0043149E` through thunk VA
`0x009FC77A` and PE import slot VA `0x00F3E60C`. Upload reads the same table
at VA `0x00431218` and `0x00431281`, passing the selected format to the
D3DX volume and surface load-from-memory paths.

Its metadata record is at VA `0x00F63FAC`,
`0x00F63C3C + 0x28 * 22`. The first dword is 16, read as bits per pixel by
`FUN_00433240` at VA `0x00433247`. The block-format byte at record `+0x21`,
VA `0x00F63FCD`, is zero, read by `FUN_00433300` at VA `0x00433307`.
The record's pointer at `+0x24`, VA `0x00F63FD0`, selects the retained label
`R5G5B5A1` at VA `0x00F63BA8`. That label conflicts with the consumed
numeric API format name and does not corroborate it.

Microsoft's authoritative
[D3DFORMAT](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat)
definition assigns 26 to `D3DFMT_A4R4G4B4`, an unsigned 16-bit ARGB format
with four bits per channel. For a little-endian 16-bit stored value, Blue is
bits 0 through 3, Green bits 4 through 7, Red bits 8 through 11, and Alpha
bits 12 through 15. This channel layout follows the API format definition.
It is distinct from the client metadata label.

These observations use the pinned `ffxivgame.exe` identity above,
ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1`, and image base VA
`0x00400000`. Bounded constants and import identity were independently
checked against the PE. The analyzed program's stored SHA-256 matched the
file pin. Inspected instruction encodings matched the pinned executable.
No fresh import or live device validation ran.

This establishes a generic client table mapping and API-side channel layout.
It does not establish that a font resource uses index 22, the contents or
extent of such a resource, or an actual device texture format. The
[D3DXCreateTexture contract](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxcreatetexture)
permits the returned texture format to differ from the requested format.
Index 22 is outside the format indices observed in the retained GTEX census.

### Encoded surface sizes

The exact-address helper at `0x00433420` calculates one 2D surface size from
width, height, and format index. For non-block formats it returns:

```text
width * height * bitsPerPixel / 8
```

For block formats it returns:

```text
ceil(width / 4) * ceil(height / 4) * blockBytes
blockBytes = 8 when index == 24, otherwise 16
```

The descriptor builder at `0x00432120` calls that helper for each face and mip,
writes the cumulative prior size as the first big-endian dword of the
eight-byte table entry, writes the helper result as the second big-endian
dword, and advances the cumulative offset by that result. Its volume branch
multiplies the 2D helper result by the surface depth. The core header
initializer at `0x00432590` writes only the fixed bytes through `+0x17`; it
does not define a fixed header field at `+0x1c`. With the retail table base of
24, `+0x1c` is the second dword of entry zero.

Every one of the 41,217 retail table entries has a second dword equal to the
formula above. Every table is monotonic and every declared surface is in range
and non-overlapping. The final surface ends exactly at EOF in all 21,161 files.
Two adjacent pairs in the sole eight-mip DXT1 resource have eight bytes of
alignment space after an eight-byte surface; all other adjacent offsets equal
the preceding declared size.

The retail corpus contains only flag value zero, 2D textures, depth one,
offset-table base 24, format indices 4/24/26, and one to eight mips. Cube,
volume, flag bit 2, missing-table, and other format-index size behavior remain
outside the retail-supported surface boundary.

## PWIB

PWIB is a split container. The streaming loader at `0x004ea560` reads and
byte-swaps four header dwords. Its three boundary fields are:

| Offset | Width | Loader-backed meaning |
|---|---:|---|
| `+0x04` | 4 | Total size and second-segment end |
| `+0x08` | 4 | First-segment offset |
| `+0x0c` | 4 | Second-segment offset |

The exact segment spans are therefore:

```text
first  = [field(+0x08), field(+0x0c))
second = [field(+0x0c), field(+0x04))
```

Helpers `0x004ebed0` and `0x004ebf00` compute those two lengths. The streaming
path supplies them independently to `0x004e8bf0` and `0x004e8ca0`. It reads
the first allocation directly through `0x00453030` at `0x004ea72a`, and the
second through `0x004e8b90` at `0x004ea761`; that wrapper also calls
`0x00453030`. These lengths are bytes.

The resource consumer at `0x00a642f0` maps both spans with the same boundary
arithmetic and requires an `SEDB` signature at the first-segment offset. It
does not establish that the first segment is a complete standalone SEDB file.
The generic consumer forwards the second span separately, as described below.
Consequently PWIB must not be modeled as an unbounded 16-byte prefix followed
by an ordinary nested SEDB extent.

A header-only census of 3,544 retail PWIB resources found no unordered
boundaries, total-size mismatches, or missing `SEDB` signatures at the first
offset. The first offset was 16 in every observed file. First-segment lengths
ranged from 96 to 4,486,464 bytes and second-segment lengths from 376 to
5,592,404 bytes. Those retail observations corroborate the general loader
arithmetic without replacing it with fixed constants.

### Native consumer boundary

The reproduced generic path establishes first-span block selection and
second-span context forwarding. It does not establish a complete split-buffer
resource contract. This bounded negative covers `0x00a642f0`, its SEDBRES
reader and generic type dispatcher, and the registered `bin`/`trb` recursive
callbacks listed below. It does not cover every concrete resource handler.

At `0x00a64484` through `0x00a644d7`, the consumer obtains the second byte
length and requests a view from the input object's virtual slot `+0x24`, with
the second offset, that length, and alignment 16. When the view is nonnull,
`0x00a644d7` stores it at parser-context `+0x0c`. This assignment also occurs
with a supplied context; only creation of a zero-initialized local context is
conditional. The first view is requested separately at `0x00a644fa` through
`0x00a64505`, using the first offset and first byte length. Its virtual slot
`+0x04` supplies the pointer used for the `SEDB` check.

For subtype bytes `RES` or `res` at first-span `+0x04..+0x06`, the consumer
constructs reader `0x00e0d4c0` and calls `0x00a3d6c0`. Other subtypes are
derived from four bytes at `+0x04`: zero and space bytes are skipped, uppercase
ASCII letters are lowered, and remaining bytes are accumulated by an
eight-bit shift. `0x00a3c800` looks up that value and the consumer invokes the
selected handler's virtual slot `+0x04` or `+0x0c`. No filename participates
in this selection.

For the SEDBRES branch, let `S` be the first view's pointer. Reader initializer
`0x00e0d120` uses the following fields after any endian conversion:

| First-span offset | Width | Observed use |
|---|---:|---|
| `+0x08` | 4 | Compared against 4000 after checking `SEDBRES ` |
| `+0x0c` | 1 | Nonzero triggers endian conversion unless already initialized |
| `+0x2c` | 4 | Initialization marker `0x494e4954` in native memory |
| `+0x30` | 4 | Entry count `N` |
| `+0x34` | 4 | Byte offset of names, relative to payload base `D` |
| `+0x38` | 4 | Count of consecutive NUL-terminated names |
| `+0x3c` | 4 | Scalar returned by reader virtual slot `+0x28` |
| `+0x40` | `16*N` | Entry table `T` |

The payload base is `D = S + 0x40 + 16*N`. Entry `i` at `T + 16*i` supplies
a name index at `+0x00`, a byte offset `o` at `+0x04`, and byte length `L` at
`+0x08`. The fourth dword has no promoted meaning here. Pointer accessor
`0x00e0c9b0` returns `D + o`; length accessor `0x00e0cb40` returns `L`.
Constructor `0x00e0d4c0` requests a block view from the first view's virtual
slot `+0x24`, with offset `(D + o) - S` and length `L`. Its requested alignment
is a power of two no greater than 128, reduced according to pointer, offset,
and the first view's virtual `+0x0c` value `A`: it chooses the largest `2^k`,
`0 <= k <= 7`, for which `((S & 0x7f) | ((D + o) - S) | A) & (2^k - 1)`
is zero. These are first-view-relative byte
ranges, not demonstrated offsets into the second span.

Names begin at `D + field(+0x34)`. `0x00e0c8d0` searches those names for the
literal blocks `RESOURCE_TYPE` and `RESOURCE_ID`. Each present metadata block
reduces the exposed entry count by one, giving `V = N - metadataCount`.
Pointer, length, name, view, and ID accessors check an unsigned index against
`V`; the type accessor `0x00e0cd00` checks against `N`. Type values are dwords
at the type block's payload plus `4*i`. ID accessor `0x00e0c940` reads a
16-byte pair at the ID block's payload plus `16*i`: selector 1 returns the
second qword; other selectors return the first. Named pointer accessor
`0x00e0caa0` also selects a requested occurrence among the exposed entries.
View and name accessors are `0x00e0c9e0` and `0x00e0ccd0`; visible-count
accessor `0x00e0d6b0` returns `V`.

The initializer mutates first-span storage. When `S+0x0c` is nonzero and the
marker is absent, it clears that byte and calls `0x00e0ce20`, which swaps the
dword at `+0x08`, word at `+0x0e`, qwords at `+0x10/+0x18`, four dwords at
`+0x30`, and the first three dwords of every table entry. It swaps `N` type
dwords on that conversion path. Before marking an uninitialized buffer, it
also reverses `2*N` ID qwords when the endian byte is zero, through
`0x00a3ca30`. `0x00e0d0c0` writes the native marker. Dispatcher `0x00a3bf10`
reverses both selected ID qwords before passing them onward. These operations
do not copy or transform the second span.

These index checks do not validate `D`, name indices, the name walk, metadata sizes, or
`o + L` against the first span's length. The signature and version failures
call an assertion callback. Whether the requested extent is enforced depends
on the concrete virtual slot `+0x24` implementation, which is outside this
finding. The reader uses 32-bit offset/count arithmetic without a demonstrated
overflow check.

Reader virtual slot `+0x14` (`0x00e0ca10`) returns the saved parser context.
`0x00a3bf10` carries it with the selected block pointer or view, length, type,
name, and ID pair into `0x00a44660` or `0x00a44730`, then `0x00a443a0`.
The registration at `0x00a3c4f0` binds `bin` (`0x0062696e`) and `trb`
(`0x00747262`) to vtable `0x00fb6e68`. Its callbacks `0x00a3d700`,
`0x00a3d7d0`, and `0x00a3d880` construct another reader through
`0x00e0d450` or `0x00e0d4c0` and re-enter the generic parser with that context.
They add no demonstrated second-span read or seek.

The first unresolved edge for second-span interpretation is the concrete type
callback at `0x00a443a0` virtual slots `+0x04/+0x08/+0x0c` (calls at
`0x00a445be/0x00a44576/0x00a445fb`), or the corresponding non-RES dispatch
at `0x00a642f0`. A specific callback must be tied to the supplied resource
type and shown to consume context `+0x0c`, with its offset base, byte units,
length checks, and transformations. The searched generic family supplies no
cross-buffer offset resolution and no second-span byte read. This is not a
negative claim about unsearched concrete handlers, nor evidence for a texture,
palette, compression, or standalone SEDB interpretation.

### Container validation and replacement boundary

The generic methods establish field use and some local predicates. They do
not establish a complete member-replacement profile. Let `S`, `N`, `D` and
`V` retain the definitions above. A preservation rule below means that these
observations do not justify discarding or changing the information.

| Field | Validation and consumer | Preservation boundary |
|---|---|---|
| PWIB big-endian `+0x04` | `0x004EBF00` subtracts `+0x0C` to obtain the second length; `0x00A644B1` requests that view. | Keep the actual second end consistent with the container extent. These callers do not compare total to enclosing length or exclude subtraction underflow. |
| PWIB `+0x08/+0x0C` | `0x004EBED0` subtracts first offset from second offset; `0x00A64505` requests the first view. | Preserve ordered, contained split spans. The observed first offset 16 is not a loader-enforced constant. |
| SEDBRES `+0x10` | `0x00E0CE6B..0x00E0CEC8` converts a qword. Neither initializer `0x00E0D120` nor constructor `0x00E0D4C0` compares it with the first-view length. | Preserve this scalar's source representation. Conversion does not establish a validated total, nor permission to ignore it in other handlers. |
| Count `+0x30` | `0x00E0D277` reads `N` to form the table/payload boundary. Accessors bound indices against `N` or `V`. | Preserve physical order, metadata associations and exposed count. No checked multiply/add or enclosing table-extent guard is established here. |
| Names offset/count `+0x34/+0x38` | `0x00E0D2AC..0x00E0D307` forms the payload-relative names start and builds pointers by consecutive NUL scans. | Preserve all referenced ordinals and termination. No supplied-length bound constrains these scans. |
| Row `+0x00` | `0x00E0CCEC/0x00E0CCF2` use a name ordinal after checking the visible row index. | It is not a resource ID. No name-ordinal bound or uniqueness predicate is established. |
| Row offset/size `+0x04/+0x08` | `0x00E0C9CC/0x00E0C9D0` return `D+offset`; `0x00E0CB5C` returns size; `0x00E0D618` requests the member view. | Preserve the subtype's ranges and offset bases. Concrete view containment remains unresolved. |
| Row `+0x0C` | The audited initializer, converter, constructor and row accessors do not consume it as a kind. | Preserve it as opaque. Generic types instead come from `RESOURCE_TYPE` through `0x00E0CD34`; IDs come from `RESOURCE_ID` through `0x00E0C940`. |

Streaming loader `0x004EA560` checks that the initial read returned 16 bytes
at `0x004EA63A`. For the first segment it compares
`(returnedCount + 15) & 0xFFFFFFF0` with
`(computedLength + 15) & 0xFFFFFFF0` at `0x004EA746`, after the rounding
operations at `0x004EA731..0x004EA743`. Equality of those rounded 32-bit
values does not require an exact read length; a short read in the same
16-byte bucket passes this comparison.
The RES version branch at `0x00E0D235/0x00E0D23C` tests unsigned
version at least 4000. Signature/version failures call assertion callbacks
and continue if those callbacks return, so they are not demonstrated hard
rejections.

Rows may select the same name ordinal without a local uniqueness check.
Equal strings in distinct ordinals are not rejected by these methods either.
This does not establish arbitrary suffix-overlap encoding: the names are
built from consecutive terminator scans. Named lookup `0x00E0CAA0` selects
an occurrence in visible row order, while metadata lookup `0x00E0C8D0`
selects the first matching row among all physical entries. Sorting,
deduplicating or moving metadata therefore lacks a preservation argument.

The member-view alignment is derived at `0x00E0D5AA..0x00E0D5E5` using the
formula above. It can be any power of two from 1 through 128. The second
PWIB view separately requests alignment 16. These methods establish no
universal member offset/length multiple-of-4 or multiple-of-16 predicate.
Concrete subtype requirements remain separate.

Extent enforcement remains unresolved at the view interface. `0x00A643F2`
loads a caller-supplied input whose slot `+0x24` is invoked at
`0x00A644B1/0x00A64505`. Constructor `0x00E0D4C0` loads and saves its
caller-supplied first view at `0x00E0D4E0/0x00E0D505`, then invokes that
view's slot `+0x24` at `0x00E0D618`. These functions do not establish the
input views' concrete vtables. The next evidence is an authenticated caller
that identifies both dynamic vtables, followed by complete audits of those
slot targets for offset, length, overflow, alignment and error handling.

The `bin`/`trb` callbacks `0x00A3D700`, `0x00A3D7D0` and `0x00A3D880`
construct nested readers and re-enter the generic parser without a local
total/name/member-span check. This establishes recursive consumption, not
subtype acceptance or safe arbitrary relocation of nested resources.

This audit used the pinned executable and the Python/pefile/Capstone profile
specified in the 2D field boundary. Complete entry-seeded normal CFGs of the
named methods and row accessors terminated at returns without unresolved
normal branches. Exception edges and callees beyond the named methods were
not assumed to add or omit checks. No runtime acceptance was observed.

### m520/e001 texture-bank boundary

The `client/chara/mon/m520/equ/e001/top_tex1/0000` through `0007`
resources are each 103,600 bytes. The `xivl-tools` `xivl inspect` reader at
commit `bcc31ced63f326195ff533b3eadd25d25d7c2d56` identifies all eight as
bounded PWIB resources with first segment `[16, 5296)` and second segment
`[5296, 103600)`. It reports the second segment as opaque; this parser output
does not establish a GTEX texture, pixel format, palette, or element meaning.

The source identities below were checked against the client stamped
`2012.09.19.0001`. Its executable matches the pinned
`ffxivgame.exe` hash above.

| Resource | Bytes | SHA-256 |
|---|---:|---|
| `m520/equ/e001/top_tex1/0000` | 103,600 | `23b3f4cbd2a25e3d43f4864bbf8b79f9b7d68322b96849bb044483c62286314d` |
| `m520/equ/e001/top_tex1/0001` | 103,600 | `8c2cf135908ac641d3cf95324e66b0c0e5b8f4af5ec2dbab558d87b5faa28408` |
| `m520/equ/e001/top_tex1/0002` | 103,600 | `2a47a92d74231689d947dba473ab07ca89c6cebb3ee26212e9d18915fe8fc9e7` |
| `m520/equ/e001/top_tex1/0003` | 103,600 | `bcddb821e5057ab208b5c555ecbe93b214425c291469f2dad2e9cdcfeca96f41` |
| `m520/equ/e001/top_tex1/0004` | 103,600 | `1fd2d138b6b726050679a8fb962ca900a429982695d5f228d4e8b047a6c53eca` |
| `m520/equ/e001/top_tex1/0005` | 103,600 | `1817ebf25a986c2ae54c24794ff7f68e7d46fdddf50beb2e5afb96177ef8007c` |
| `m520/equ/e001/top_tex1/0006` | 103,600 | `a0c62abbc19804ecf8578e2d9bdb2cd4ac99878e8be1f3c4028587cdcafd2c3d` |
| `m520/equ/e001/top_tex1/0007` | 103,600 | `4f7c681ae338eed7558ee999c3ecdd0d186fd7ee2ad496136e33e7e7f3d86aa5` |

These identities pin the resource files. The concrete interpretation below
covers one selected block in `0000`; it does not associate a texture index
with an actor-class ID or prove which appearance any retail encounter selected.

### Selected RES/txb consumer

For the pinned `m520/equ/e001/top_tex1/0000` resource, visible entry 6 selects
a concrete split texture contract: its first-span `txb` block supplies a GTEX
descriptor, and the descriptor's surface offset selects DXT1 source bytes in
the second span. This is a static contract of the selected handler and its
node method; no observed runtime invocation or rendered appearance is claimed.

The first span begins with `SEDBRES ` and has endian byte zero. Its native
little-endian entry count is 11, with nine exposed entries after the two
metadata blocks. The payload base is first-relative 240. The `RESOURCE_TYPE`
payload starts at first-relative 4504; its dword at `+4*6` is `0x00747862`
(`txb`). Entry 6 has payload offset 3976 and byte length 96, selecting
first-relative `[4216, 4312)`, or file `[4232, 4328)`. This metadata, rather
than its filename or a neighboring block, selects the handler.

Registration `0x00c4ab70` installs factory vtable `0x010f2c44` with key
`0x00747862`: the vtable write is at `0x00c4acd5`, key assignment at
`0x00c4acdf`, and registration call at `0x00c4ad01`. Wrapper
`0x0060dac0` obtains the manager through `0x0060d8a0`, then inserts through
`0x00a3be10` and `0x00a44860`. Dispatcher `0x00a44730` looks up the same
key through `0x00a44b00`. Function `0x00a475a0` calls this registration at
`0x00a47747`. Factory predicates at slots `+0x14/+0x18` both
resolve to `0x00b73290`, which returns true. With context flag `+0x14` zero,
the allocated view path in `0x00a443a0` therefore calls factory slot
`+0x0c`, `0x00c78b70`, at `0x00a445fb`. Its preceding size pass calls
slot `+0x08`, `0x0060d980`; slot `+0x10`, `0x005afd10`, supplies node size
`0x68`. The raw slot `+0x04` is outside this selected view-path finding.

At `0x00c78ba1`, callback `0x00c78b70` loads parser-context `+0x0c`.
Constructor `0x00c788c0` installs node vtable `0x010faddc` and calls
`0x00c786e0`, which retains the first block view at node `+0x50` and the
second view at `+0x58`. Its node virtual slot `+0x3c` resolves to
`0x00c78a00`. That initializer checks `SEDBtxb\0` and version at least
one through `0x00c78e60`, then reads the native word at block `+0x0e`.
For a word no greater than `0x30`, the descriptor is at
`block + 0x30 + word`; otherwise it is at `block + dword(block+0x30)`.
The selected block has endian byte zero, version one, word `0x40`, and
dword `0x40`, so node `+0x60` receives `block+0x40`, file offset 4296.
The initializer does not demonstrate a descriptor-offset extent guard;
the selected descriptor and table fit within the verified 96-byte block.
Failure of the signature/version check calls diagnostics and an assertion
callback, then continues to descriptor selection if those calls return.

The callback retains the views; it does not immediately upload their bytes.
When this node's virtual slot `+0x18`, `0x00c787f0`, executes, it loads
the descriptor and second view at `0x00c7882a/0x00c7882e` and calls
`0x00c74660` at `0x00c78862`. That method forwards both through
`0x00418f90` to `0x004323d0`, which selects `0x00431f30` by the descriptor's
`GTEX` signature. The concrete node-vtable join proves this deferred consumer;
it does not prove when a retail encounter invokes it.

The selected 32-byte descriptor has these big-endian scalar values:

| Descriptor offset | Value | Selected use |
|---|---:|---|
| `+0x06/+0x07/+0x09` | 24 / 1 / 0 | DXT1 format index, one mip, 2D flags |
| `+0x0a/+0x0c/+0x0e` | 256 / 256 / 1 | Width, height, depth |
| `+0x10` | 24 | Eight-byte surface table starts at descriptor `+0x18` |
| `+0x14` | 0 | Use the supplied second view instead of descriptor-relative source data |
| Entry 0 `+0/+4` | 0 / 32768 | Byte offset and declared encoded size |

The three one-byte values in the first row have no endian conversion.
`0x00431f30` allows a zero source-data base only when a source view is
supplied. At `0x004320db..0x004320ec`, it obtains that view's byte length
through virtual slot `+0x08` and pointer through `+0x04`, then calls
`0x00431e20`. The selected flags choose one face, and one mip chooses table
entry zero. With source-data base zero, `0x00431e9b..0x00431ee1` reads the
first big-endian table dword; `0x00431ee5` compares it unsigned against the
supplied byte length. An offset at or beyond that length skips the upload.
For an accepted offset, `0x00431eeb/0x00431eef` adds the supplied pointer,
and `0x00431f00` calls `0x00431080` with the resulting source pointer.

Thus the selected cross-buffer resolution is:

```text
descriptor = firstBlockPointer + 0x40
table      = descriptor + BE32(descriptor+0x10)
source     = secondViewPointer + BE32(table+8*surfaceIndex)
```

For entry zero, the source begins at second-relative zero, file offset 5296.
Upload `0x00431080` uses index 24 to read `D3DFORMAT 0x31545844` from
`0x00f637b8 + 24*4`, and four bits per pixel with a nonzero block-format byte
from the metadata record at `0x00f63c3c + 24*0x28`. The uploader obtains
destination surface dimensions through its Direct3D virtual methods. For a
256 by 256 destination it computes compressed source row pitch 512 bytes
and passes the source, format, pitch, and dimensions to
`D3DXLoadSurfaceFromMemory` at
`0x004312a5`, through import thunk `0x009fc76e`. Those 64 block rows imply a
32768-byte footprint, agreeing with entry zero's second dword and the GTEX
size formula above. No reproduced selected-path instruction reads that
second dword to bound the upload. The native check guards the start offset,
not `offset + footprint`; descriptor/table extents and addition overflow
also have no demonstrated guard in these methods.

For the pinned file, offline extent checks verified descriptor and table
within the first block and source `[0, 32768)` within the 98304-byte second
view, corresponding to file `[5296, 38064)`. The selected path byte-swaps
GTEX scalar values into local native values and hands the compressed source
bytes directly to D3DX; it demonstrates no second-span copying, decompression,
or endian conversion before that API boundary. It does not establish the
remaining second-span bytes, other entries, resource variants, or a complete
standalone first-span resource.

This contract supports a bounded decoder follow-up for this exact RES/txb
entry: parse the first-span descriptor and resolve its encoded surface against
the separately bounded second span. A decoder must enforce whole-range
bounds itself. Broader handler coverage needs a separately selected type and
callback; runtime appearance selection remains a separate discriminator.

## Evidence boundary

The GTEX claim is reproduced from the functions above plus format helpers
`0x00431710`, `0x00433300`, and `0x00433240`. Tracked Rosetta sources preserve
the dispatcher, loader, three texture constructors, and upload-loop bodies
under `src/ffxivgame/_rosetta/`.

The cube upload finding used ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK
`26.0.2.1`, and image base VA `0x00400000`, reusing an analyzed program whose
stored executable SHA-256 and independently hashed executable matched the
pin above. Exact-entry decompilation covered `0x00431f30`, `0x00418d00`,
`0x00431710`, `0x004312d0`, `0x00431e20`, `0x00432500`, `0x00431080`, and
the two import thunks. Bounded disassembly of the complete upload loop,
source-offset helper, and uploader, plus the cube creation callsite,
corroborated stack argument order and pointer forwarding. The import query
corroborated both D3DX names. No fresh import, retained raw output, resource
inspection, live device validation, or retail session forms part of this
cube finding.

The PWIB claim uses the streaming path, exact length helpers, request helpers,
and resource consumer listed above. The consumer extension was reproduced
with committed `DecompileToText.java` and `FindCallers.java` through
`tools/ghidra/run-headless.ps1`, using fresh read-only imports, Ghidra 12.1.3,
JDK 21, and image base `0x00400000`. The decisive sections include all functions
named in the native consumer boundary; caller `0x007cbd50` corroborates supplied
context forwarding. Instruction encodings and the reader/handler vtable slots
were checked against the pinned PE bytes. No raw bodies or projects form part
of this tracked finding.

The selected RES/txb extension used the same fresh-import runner and committed
`DecompileToText.java`, Ghidra 12.1.3, JDK 21, image base, and binary pin.
The registration, factory predicates, dispatcher, constructors, deferred node
method, GTEX source-view branch, creation/upload helpers, and import thunk
named above were reproduced as complete exact-entry sections. Both producers
completed in read-only mode without analysis timeout. Factory and node vtables,
format metadata, import identity, and instruction encodings were checked
against the pinned PE; the selected resource hash, metadata, descriptor fields,
and byte ranges were checked directly against the pinned resource.
The additional direct registration call at `0x00a47747` was corroborated
against its PE instruction encoding.

This finding establishes static loader arithmetic for the exact retail build.
GTEX flag bit 2 is propagated as creation value 4 and as an otherwise unused
upload argument, but no reproduced consumer assigns it a stable meaning. The
selected RES/txb finding explains one surface's use of the second segment. It
does not establish every PWIB handler or cross-build stability.
