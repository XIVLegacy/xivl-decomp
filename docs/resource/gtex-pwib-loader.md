# GTEX and PWIB loader fields

This page records the fields consumed by the retail 1.23b client loaders for
standalone GTEX and PWIB resources. The reviewed `ffxivgame.exe` has SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All multibyte resource fields below are big-endian.

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
path supplies them independently to `0x004e8bf0` and `0x004e8ca0`, then reads
both allocations through `0x004e8b90`.

The resource consumer at `0x00a642f0` maps both spans with the same boundary
arithmetic and requires an `SEDB` signature at the first-segment offset. It
does not establish that the first segment is a complete standalone SEDB file.
The purpose of the second segment remains unresolved. Consequently PWIB must
not be modeled as an unbounded 16-byte prefix followed by an ordinary nested
SEDB extent.

A header-only census of 3,544 retail PWIB resources found no unordered
boundaries, total-size mismatches, or missing `SEDB` signatures at the first
offset. The first offset was 16 in every observed file. First-segment lengths
ranged from 96 to 4,486,464 bytes and second-segment lengths from 376 to
5,592,404 bytes. Those retail observations corroborate the general loader
arithmetic without replacing it with fixed constants.

## Evidence boundary

The GTEX claim is reproduced from the functions above plus format helpers
`0x00431710`, `0x00433300`, and `0x00433240`. Tracked Rosetta sources preserve
the dispatcher, loader, three texture constructors, and upload-loop bodies
under `src/ffxivgame/_rosetta/`.

The PWIB claim uses the streaming path, exact length helpers, request helpers,
and resource consumer listed above. `0x004e8bf0`, `0x004e8ca0`, and
`0x004e8b90` corroborate allocation and read sequencing but add no field
meaning. Caller `0x007cbd50` adds context only and is not required for the
layout verdict.

This finding establishes static loader arithmetic for the exact retail build.
GTEX flag bit 2 is propagated as creation value 4 and as an otherwise unused
upload argument, but no reproduced consumer assigns it a stable meaning. The
finding does not explain PWIB's second segment or establish cross-build
stability.
