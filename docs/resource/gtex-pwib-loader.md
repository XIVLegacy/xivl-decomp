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
| `+0x14` | 4 | Source-data base, relative to the blob |

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
D3DX load-from-memory call. This makes `+0x14`, not `+0x1c`, the proven data
boundary. No function in the reproduced loader path reads `+0x1c`, so it has
no promoted extent meaning.

A header-only census of 21,161 retail GTEX resources found every `+0x14` value
within its file, with no zero values. The observed bases were 32, 48, 64, and
96 bytes; 48 occurred 11,272 times, 32 occurred 6,974 times, 64 occurred 2,914
times, and 96 occurred once. The census corroborates the loader field but does
not narrow the format to those four values.

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
definition. Client metadata independently labels the same indices
`A8R8G8B8`, `DXT1`, and `DXT5`.

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

These identities pin the resource files, not the interpretation of
their opaque second segment. They do not associate a texture index with an
actor-class ID or prove which appearance any retail encounter selected.

## Evidence boundary

The GTEX claim is reproduced from the functions above plus format helpers
`0x00431710`, `0x00433300`, and `0x00433240`. Tracked Rosetta sources preserve
the dispatcher, loader, three texture constructors, and upload-loop bodies
under `src/ffxivgame/_rosetta/`.

The PWIB claim uses the streaming path, exact length helpers, request helpers,
and resource consumer listed above. The consumer extension was reproduced
with committed `DecompileToText.java` and `FindCallers.java` through
`tools/ghidra/run-headless.ps1`, using fresh read-only imports, Ghidra 12.1.3,
JDK 21, and image base `0x00400000`. The decisive sections include all functions
named in the native consumer boundary; caller `0x007cbd50` corroborates supplied
context forwarding. Instruction encodings and the reader/handler vtable slots
were checked against the pinned PE bytes. No raw bodies or projects form part
of this tracked finding.

This finding establishes static loader arithmetic for the exact retail build.
GTEX flag bit 2 is propagated as creation value 4 and as an otherwise unused
upload argument, but no reproduced consumer assigns it a stable meaning. The
finding does not explain PWIB's second segment or establish cross-build
stability.
