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
