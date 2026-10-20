# PRAM packed render-state consumers

The material factory retains a PRAM payload pointer. An ordinary material
application reads a packed word at that pointer and sends selected bits to
the Direct3D 9 state manager. This record describes that static path, its
context guards, and its alpha-reference conversion. It does not establish
a complete material format or a live rendered result.

## Target and method

- Target: retail FINAL FANTASY XIV 1.23b, build `2012.09.19.0001`.
- Executable: `ffxivgame.exe`, `15996808` bytes.
- SHA-256: `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- PE32 image base: `0x00400000`. All addresses below are VAs.
- Instruments: Apple LLVM `llvm-objdump` `17.0.0` bounded disassembly for the
  handoff, dispatch, alpha-reference and first five state rows. Ghidra `12.1.4`
  disassembly, cross-reference and memory reads of the same image for the
  blend, anti-aliasing, depth-bias, light-mask and context-byte rows.
- Confirmation: map every displayed instruction through the PE section table
  and compare its encoded bytes with the supplied executable. Read referenced
  strings, lookup tables, and floating-point constants from the same image.
- Tiers: V = verified from cited instructions or image bytes, I = inferred
  from verified observations, U = unknown or untraced.

Function labels inferred from catalogs are leads. The locators and observed
instructions below carry the findings independently of those labels.

## Payload handoff

| Locator | Observation |
|---|---|
| `0x00C8FF64-0x00C8FF6B` | The factory passes the string at `0x011071DC`, whose retained text is `PRAM`, to `0x00C697E0`. |
| `0x00C8FF72-0x00C8FF7F` | Its aligned result selects the inline body at section pointer plus `0x10`. The other branch copies the section body into an allocated buffer. |
| `0x00C90018` | For the already interpreted resource version below `0x1A`, the factory sets word bit `0x0800` in that body. |
| `0x00C900BD-0x00C900E6` | The factory passes the selected body pointer and section size to the constructor at `0x00C3D980`. |
| `0x00C3D9CE-0x00C3D9DB` | That constructor stores its body-pointer argument at object offset `+0x30`. |
| `0x00C3EE71-0x00C3EE8B` | The material application reaches `0x00C3D340` through its selected material object. |
| `0x00C3D384-0x00C3D38D` | The state consumer loads the body pointer from `+0x30`. Equality with the cached material at `0x01374CA8` skips the packed-state setup block. |

The word is a runtime little-endian 16-bit read at body offset `+0x00`.
The factory calls preprocessing at `0x00C8FF36` before selecting PRAM.
This record does not specify that preprocessing or infer the serialized
endianness of every resource variant.

## State-manager dispatch

The helper at `0x004236E0` first calls the cache predicate at `0x00424010`.
On a cache miss, `0x004236FB-0x00423704` forwards the state identifier and
value through manager device virtual slot `+0x24`. The ordinary wrapper
constructor writes vtable `0x00F660F8` at `0x00439015`. Its slot `+0x24`
contains `0x00438740`. At `0x00438740-0x00438756`, that target forwards
the two arguments through the underlying device slot `+0xE4`, the
Direct3D 9 `SetRenderState` interface position. The official
[`d3d9.h` declaration](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L361-L422)
contains three `IUnknown` slots and 55 device methods through
`SetRenderState`. Its zero-based index is `57`, and `57 * 4 = 0xE4`
for the PE32 pointer width.

The numeric state names follow Microsoft's
[D3DRENDERSTATETYPE definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3drenderstatetype).
The binary supplies the identifiers and values.

| Packed state | Consumer and setter | Observed device state |
|---|---|---|
| Bit `0x0002` | `0x00C3D3EB-0x00C3D43C`, setter `0x0041C1D0` | Identifier `15`, `D3DRS_ALPHATESTENABLE`, receives `1` when set and `0` when clear. |
| Bit `0x0008` | `0x00C3D393-0x00C3D3A2`, setter `0x0041D0E0` | Identifier `22`, `D3DRS_CULLMODE`, receives lookup entry `0` or `1` from `0x00F597C8`. Those entries are `1` and `2`. |
| Bit `0x0010` | `0x00C3D535-0x00C3D54A`, setter `0x0041C2B0` | Identifier `14`, `D3DRS_ZWRITEENABLE`, receives the extracted Boolean. |
| Bit `0x0800` | `0x00C3D54F-0x00C3D566`, setter `0x0041C270` | Identifier `7`, `D3DRS_ZENABLE`, receives the extracted Boolean. |
| Bit `0x0001` and context floats | `0x00C3D4FD-0x00C3D530`, setter `0x0041C1F0` | Identifier `27`, `D3DRS_ALPHABLENDENABLE`, receives `1` when the bit is set or either context float at `+0xBF0` or `+0xC00` compares below `1.0`. Otherwise it receives `0`. |

The cull values mean no culling and clockwise-face culling respectively,
according to Microsoft's
[D3DCULL definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcull).
This does not determine the winding conversion in another graphics backend.

The cull setup precedes a context guard. The stores to context bytes `+0x4E4`
and `+0x773` require context byte `+0xB9E` to be zero. The remaining setup
also requires the predicate at `0x00C47AC0` to return zero. Those alternate
paths and the predicate's meaning are outside this record. The sections below
cover the other bits the consumer reads.

## Alpha reference

When bit `0x0002` is set, `0x00C3D3FE-0x00C3D429` reads a float from
body `+0x04`, multiplies it by context float `+0xBF0`, adds the double
at `0x010A8A18`, and stores the result as a 32-bit float. That double is
`0.003921568859368563`. The call to `0x00BA0B70` supplies internal
comparison index `6` and the stored float.

At `0x00BA0B90`, the helper forwards both to `0x0041CF50`. The setter
reads table `0x00F596C0`, whose entries are `1, 2, 3, 4, 5, 6, 7, 8`.
Index `6` therefore selects value `7` for state identifier `25`,
`D3DRS_ALPHAFUNC`, at `0x0041CF57-0x0041CF67`. Microsoft's
[D3DCMPFUNC definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcmpfunc)
identifies value `7` as `D3DCMP_GREATEREQUAL`.

The reference conversion at `0x0041CF6C-0x0041CFC2` multiplies the float
argument by double `255.0` at `0x00F598B0`, adds double `0.5` at
`0x00F59898`, temporarily selects x87 truncation toward zero, and converts
to a signed 64-bit integer. It takes the low 32 bits and compares them with
`255` as unsigned. Values above `255` select `255`. The selected
value is sent to identifier `24`, `D3DRS_ALPHAREF`. This is not a
clamp-to-zero operation for negative inputs. NaN, infinity, overflow, and
unusual floating-point environments have no consumer interpretation here.

## Blend factors and operation

The blend selection at `0x00C3D489-0x00C3D4F5` runs for every material
application that reaches the packed-state block, independent of bit
`0x0001`. It tests bit `0x0100` first, then bit `0x0200`, and selects a
blend-operation index and a destination-factor index. The source-factor
index is always `6`.

| Word bits | Operation index | Destination index |
|---|---|---|
| `0x0100` set, `0x0200` ignored | `0` | `1` |
| `0x0100` clear, `0x0200` set | `4` | `1` |
| Both clear | `0` | `7` |

The operation setter `0x00A53E00` jumps to `0x0041D010`. That routine maps
the index through the table at `0x00F5971C` and sends the result to
identifiers `171` (`D3DRS_BLENDOP`) and `209` (`D3DRS_BLENDOPALPHA`). The
first entries of the table are `1, 4, 5, 2, 3, 1, 2, 3, 4, 5, 7, 8`. Index
`0` selects `1` and index `4` selects `3`. The factor setter `0x00BA0BC0`
calls `0x0041D050`. That routine maps both indices through the table at
`0x00F596E0` and sends the results to identifiers `19`, `20`, `207`, and
`208` (`D3DRS_SRCBLEND`, `D3DRS_DESTBLEND`, `D3DRS_SRCBLENDALPHA`,
`D3DRS_DESTBLENDALPHA`). The first entries of that table are
`1, 2, 3, 4, 9, 10, 5, 6, 7, 8, 11, 14`. Index `6` selects `5`, index `1`
selects `2`, and index `7` selects `6`.

With Microsoft's
[D3DBLEND](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblend)
and
[D3DBLENDOP](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblendop)
definitions, the three cases are:

| Word bits | Operation | Source | Destination |
|---|---|---|---|
| Neither `0x0100` nor `0x0200` | `D3DBLENDOP_ADD` | `D3DBLEND_SRCALPHA` | `D3DBLEND_INVSRCALPHA` |
| `0x0100` | `D3DBLENDOP_ADD` | `D3DBLEND_SRCALPHA` | `D3DBLEND_ONE` |
| `0x0200` alone | `D3DBLENDOP_REVSUBTRACT` | `D3DBLEND_SRCALPHA` | `D3DBLEND_ONE` |

The alpha-channel identifiers receive the same mapped values as the color
identifiers. This path does not write identifier `206`. Whether the blend
stage is active depends on identifier `27`, described above. Table entries
beyond those listed are not specified here.

## Anti-aliasing request

At `0x00C3D469-0x00C3D47E` the consumer extracts bit `0x0004`, stores it at
`0x01328F10`, and calls the setter at `0x0041C340`. The setter reads a mode
dword at `0x01329878` and sends an immediate code through the state helper:

| Mode | Identifier | Bit set | Bit clear |
|---|---|---|---|
| `0` | `154` | `0x314D3241`, bytes `A2M1` | `0x304D3241`, bytes `A2M0` |
| `1` | `181` | `0x434F5441`, bytes `ATOC` | `0` |
| `2` | `181` | `0x41415353`, bytes `SSAA` | `0` |

Other mode values write nothing. Microsoft's identifier list names `154`
`D3DRS_POINTSIZE` and `181` `D3DRS_ADAPTIVETESS_Y`. The values written are
four-character codes, not values from those states' ordinary domains. Reading
them as a driver-specific transparency anti-aliasing request is an inference.
Their effect on a given host is not established here.

## Light mask and context bytes

The consumer stores two derived context bytes before the alpha and blend
setup:

- At `0x00C3D3CD-0x00C3D3D8`, bit `0x0400` is stored at context byte
  `+0x773`.
- At `0x00C3D3B7-0x00C3D3C8`, bit `0x0040` is negated and passed to
  `0x00C483C0`, which stores it at context byte `+0x4E4`. The stored byte is
  `1` when bit `0x0040` is clear.

The routine at `0x00C3D5D0` stores zero to both bytes. Other callers of
`0x00C483C0` exist at `0x00C43AE1` and `0x00C440EB` and are not traced.

Byte `+0x773` has two readers:

| Locator | Observation |
|---|---|
| `0x00C22C32`, in `0x00C22A90` | After a guard on a byte at offset `+0x58` of a caller object, a loop appends up to four entries from the light block at context `+0x308`. The block's count byte is at `+0x150`, its entries start at `+0x30` with stride `0x24`, and entries whose dword at `+0xDC` equals `4` are skipped. When `+0x773` is zero, the loop also requires the per-entry enable byte at block `+0x148+i` to be nonzero. When it is nonzero, that enable byte is not consulted. |
| `0x00C230E5`, in `0x00C22DD0` | When `+0x773` is nonzero, the routine sends a four-lane splat of the float at `0x00FB7A60`, which is `1.0`, to the constant slot at `+0x128` of its object and returns. The constant uploads to slots `+0x130` through `+0x13C` and the later light-block pass, which also reads the enable byte at `+0x148+i` at `0x00C236A6`, do not run. |

The nonzero case therefore bypasses the enable mask in the first routine and
skips the later constant fill in the second. It does not skip the first
routine's light accumulation. The names of the constant slots and the
rendering meaning of the bypass are not established here.

Byte `+0x4E4` has three readers through the getter at `0x00C483D0`:
`0x00C22B03` in `0x00C22A90`, and `0x00C22EEF` and `0x00C23044` in
`0x00C22DD0`. Each reader proceeds when `+0x4E4` or the neighboring byte
`+0x4E5` is nonzero, so a set bit `0x0040` takes that path only when
`+0x4E5` is nonzero. The path in `0x00C22DD0` copies context data through
`0x00C48A30` into an object constant slot. The meaning of the data is not
established here.

## Depth bias

When bit `0x1000` is set, `0x00C3D444-0x00C3D461` loads the body floats at
`+0x24` and `+0x20` and calls `0x004182C0` with `+0x24` as the first
argument and `+0x20` as the second. That routine stores both floats at
`0x01328F1C` and `0x01328F20` and calls `0x0041CC00`. The first argument is
sent unchanged to identifier `175`, `D3DRS_SLOPESCALEDEPTHBIAS`. The second
argument is converted to double, divided by the double at `0x00F59890`, and
sent to identifier `195`, `D3DRS_DEPTHBIAS`. That double is `16777215.0`.
When the bit is clear, the block does not call the routine, so these two
states keep their earlier values.

## Packed word bit map

| Bit | Observed use in `0x00C3D340` | Tier |
|---|---|---|
| `0x0001` | Alpha blend enable, identifier `27`, also forced by the context floats | V |
| `0x0002` | Alpha test enable, identifier `15`, with alpha reference from body `+0x04` | V |
| `0x0004` | Anti-aliasing request code through `0x0041C340` | V for the code, I for the meaning |
| `0x0008` | Cull mode lookup, identifier `22` | V |
| `0x0010` | Depth write enable, identifier `14` | V |
| `0x0020` | Not read by `0x00C3D340` | V for the absence, U for other readers |
| `0x0040` | Negated into context byte `+0x4E4` | V for the store, U for the effect |
| `0x0080` | Not read by `0x00C3D340` | V for the absence, U for other readers |
| `0x0100` | Selects additive blend with destination `ONE` | V |
| `0x0200` | Selects reverse-subtract blend with destination `ONE` when `0x0100` is clear | V |
| `0x0400` | Context byte `+0x773`, light mask bypass and constant fill skip | V for the store and readers, U for the rendering meaning |
| `0x0800` | Depth test enable, identifier `7`, set by the factory for older resources | V |
| `0x1000` | Slope-scale depth bias from body `+0x24` and depth bias from body `+0x20` | V |

Bits `0x2000`, `0x4000`, and `0x8000` are not read by `0x00C3D340`. The
callees `0x00C40D00`, `0x00C402B0`, `0x00C56F30`, and `0x00C56FF0` are not
traced for reads of the word, so an absence here covers the consumer body only.

## Installed-file census

The word at PRAM body `+0x00`, read as a little-endian signed 16-bit value
by a Rust model-resource parser outside this repository, was tallied over the
`client` and `data` trees of one installed retail client. The parser exposes
the stored word, so the factory's version-dependent `0x0800` bit is not
applied. These counts are installed-file existence only. They do not say
which mesh is drawn, in which zone, or under which gameplay state.

- Meshes: meshes whose name resolves to a material that carries a PRAM
  word and whose word equals the row. Meshes with no matching material or no
  PRAM are not counted.
- Files: model files with at least one material carrying the word. A file is
  counted once per word whether or not a mesh uses that material, so a word
  can have more files than meshes.

| Word | Bits set among the listed ones | Meshes | Files |
|---|---|---|---|
| `0x0809` | `0x0001`, `0x0008`, `0x0800` | 3,261 | 1,629 |
| `0x080A` | `0x0002`, `0x0008`, `0x0800` | 42 | 42 |
| `0x080B` | `0x0001`, `0x0002`, `0x0008`, `0x0800` | 314 | 186 |
| `0x0810` | `0x0010`, `0x0800` | 119 | 100 |
| `0x0812` | `0x0002`, `0x0010`, `0x0800` | 88 | 73 |
| `0x0813` | `0x0001`, `0x0002`, `0x0010`, `0x0800` | 47 | 47 |
| `0x0817` | `0x0001`, `0x0002`, `0x0004`, `0x0010`, `0x0800` | 1 | 1 |
| `0x0818` | `0x0008`, `0x0010`, `0x0800` | 19,240 | 7,403 |
| `0x0819` | `0x0001`, `0x0008`, `0x0010`, `0x0800` | 1,744 | 1,757 |
| `0x081A` | `0x0002`, `0x0008`, `0x0010`, `0x0800` | 8,547 | 6,740 |
| `0x081B` | `0x0001`, `0x0002`, `0x0008`, `0x0010`, `0x0800` | 478 | 435 |
| `0x081D` | `0x0001`, `0x0004`, `0x0008`, `0x0010`, `0x0800` | 1 | 1 |
| `0x081E` | `0x0002`, `0x0004`, `0x0008`, `0x0010`, `0x0800` | 1 | 1 |
| `0x0909` | `0x0001`, `0x0008`, `0x0100`, `0x0800` | 144 | 143 |
| `0x0919` | `0x0001`, `0x0008`, `0x0010`, `0x0100`, `0x0800` | 1 | 1 |
| `0x0C01` | `0x0001`, `0x0400`, `0x0800` | 201 | 199 |
| `0x0C10` | `0x0010`, `0x0400`, `0x0800` | 21 | 21 |
| `0x0C12` | `0x0002`, `0x0010`, `0x0400`, `0x0800` | 982 | 889 |
| `0x0C13` | `0x0001`, `0x0002`, `0x0010`, `0x0400`, `0x0800` | 4 | 4 |
| `0x0C18` | `0x0008`, `0x0010`, `0x0400`, `0x0800` | 50 | 49 |
| `0x0C1A` | `0x0002`, `0x0008`, `0x0010`, `0x0400`, `0x0800` | 103 | 133 |

The rows sum to `35389` meshes over `21` words. No counted word has bit
`0x0020`, `0x0040`, `0x0080`, `0x0200`, or `0x1000` set. Among these model
materials, the depth-bias path and the reverse-subtract blend are therefore
never selected, and context byte `+0x4E4` is always stored as `1`.

## Interpretation boundary

For an interpreted runtime word `0x081A`, the listed bits enable alpha
testing, clockwise culling, depth writes, and depth testing on the stated
path. Word `0x0818` has the same listed bits except alpha testing is
disabled. This is arithmetic over the observed masks, not evidence that
either word belongs to a particular model, shader variant, or gameplay state.

Alpha reference depends on the body float and context factor. Blending also
depends on context. Neither is determined by the word alone. The observations
do not establish texture aliases, a fixed cutoff, skeleton matrices, shader
output equivalence, another device-wrapper variant, or live retail parity.
