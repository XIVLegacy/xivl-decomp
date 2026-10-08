# WRB vertex declaration boundary

The native mesh descriptor path builds compact elements and translates them
into the layout of `D3DVERTEXELEMENT9`. Its format and usage lookups select
numeric D3D9 declaration values. This establishes a static declaration
boundary. The exact join from the stream lookup helper to an `STMS` child
and the descriptors of an actual retail skeletal mesh remain unresolved.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VA. The PE32 image base is `0x00400000`.
Subtract that base for RVA and map through the original PE section table.

The observations use the existing analyzed `ffxivgame.exe` program in
Ghidra `12.1.4`, queried through `ghidra-cli` `0.2.2`. Decompiled output
guided the bounded instruction queries. The descriptor packing, converter
instructions and constant windows were independently compared with the
original executable through its PE mapping. API names and normalization
below are interpretations against Microsoft's primary D3D9 documentation.

## Descriptor and intermediate record

Routine `0x00c8dc10` obtains a pointer through `0x00c8f5a0`. The helper's
body was not inspected. The ordinary descriptor loop uses a 16-byte source
record beginning 16 bytes after the returned pointer. Instructions at
`0x00c8e3a7` position the loop pointer at source record `+0x0c`.
Instructions at `0x00c8e46d..0x00c8e517` swap the consumed dwords and words
and assemble this intermediate record on the stack:

| Source lane | Intermediate record lane | Later declaration use |
|---|---|---|
| Descriptor `+0x00`, BE32 | `+0x04` | Offset |
| Descriptor `+0x04`, BE32 | `+0x10` | Format lookup row |
| Descriptor `+0x08`, BE32 | `+0x0c` | Component lookup column |
| Descriptor `+0x0c`, BE16 | `+0x14` | Usage lookup pair |
| Descriptor `+0x0e`, BE16 | `+0x18` low word | Bit 15 retained separately |
| Native stream-loop value | `+0x00` | Stream |
| Returned pointer `+0x08`, BE32 | `+0x08` | Copied header lane, absent from the declaration element |

At `0x00c8e572`, the record pointer is passed to `0x00c37c70`.
Its available-capacity path calls `0x00c37630`, which copies `0x24` bytes
without reordering lanes. The descriptor pointer advances by `0x10` at
`0x00c8e5a2`. The ordinary path calls `0x00c6ab90` at `0x00c8e608`.
That routine calls the declaration builder `0x00c6a9d0` at
`0x00c6accf`. The builder accesses field records through `0x00c376b0`,
whose address calculation at `0x00c3773b..0x00c37742` has stride `0x24`.
Single-field branches and later rebuilds in `0x00c8dc10` do not establish
one universal resource-import rule.

## Compact element packing

The builder packs each field record into eight bytes at
`0x00c6aa5c..0x00c6aa95`:

| Compact lane | Source |
|---|---|
| `+0`, u8 | Low byte of record `+0x00` |
| `+1`, u8 | Low byte of record `+0x10` |
| `+2`, u8 | Low byte of record `+0x14` |
| `+3`, u8 | Bit 15 of record's word at `+0x18` |
| `+4`, u16 | Low word of record `+0x04` |
| `+6`, u16 | Low word of record `+0x0c` |

The push at `0x00c6aa90` lowers ESP by four before the `+3` byte write
at `0x00c6aa95`. Its destination is therefore the original element base
plus three, without overwriting the word at `+6`.

The builder appends the compact terminator at `0x0110cf10`, whose stream
byte is `255` and remaining lanes are zero. At `0x00c6ab26` it calls
`0x004188b0`, which forwards the compact array to `0x00419a20` at
`0x0041891d`.

## API array conversion

The complete scoped converter `0x00419a20` emits these fields:

| API element field | Observed emitted value | Instruction VA |
|---|---|---|
| Stream, u16 at `+0` | Zero-extended compact byte `+0` | `0x00419a50..0x00419a57` |
| Offset, u16 at `+2` | Compact word `+4` | `0x00419a5c..0x00419a60` |
| Type, u8 at `+4` | Byte at `0x00f57d8f + 4*compactByte1 + compactWord6` | `0x00419a53..0x00419a77` |
| Method, u8 at `+5` | Zero | `0x00419a93` |
| Usage, u8 at `+6` | Byte at `0x00f57db4 + 2*compactByte2` | `0x00419a69..0x00419a8f` |
| UsageIndex, u8 at `+7` | Byte at `0x00f57db5 + 2*compactByte2` | `0x00419a82..0x00419a8b` |

Compact byte `+3` is not read by this converter. It advances by eight bytes
until the next compact stream byte equals `255`. Instructions at
`0x00419aac..0x00419abb` append the constant element from `0x00f57e30`:
Stream `255`, Offset `0`, Type `17`, Method `0`, Usage `0`, UsageIndex `0`.
At `0x00419ad3`, it submits the array and an output pointer through device
vtable offset `0x158`.

Microsoft's
[element layout](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dvertexelement9)
matches these fields. The pinned
[IDirect3DDevice9 interface declaration](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L429-L520)
places `CreateVertexDeclaration` at zero-based slot `86`, or byte offset
`0x158` for a 32-bit interface. Its
[method contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-createvertexdeclaration)
accepts the element array and declaration output pointer. These jointly
support the API-call interpretation without proving a live device result.

## Bounded numeric lookups

The observed format window is `[0x00f57d90,0x00f57db4)`. Its four-byte rows
give these Type values for component indices `1` through `4`:

| Native format | Component 1 | Component 2 | Component 3 | Component 4 |
|---:|---:|---:|---:|---:|
| 0 | 17 | 9 | 17 | 10 |
| 1 | 0 | 1 | 2 | 3 |
| 2 | 17 | 15 | 17 | 16 |
| 3 | 17 | 17 | 17 | 8 |
| 4 | 17 | 6 | 17 | 7 |
| 5 | 17 | 17 | 17 | 17 |
| 6 | 17 | 17 | 17 | 5 |
| 7 | 17 | 17 | 17 | 17 |
| 8 | 17 | 17 | 17 | 17 |

The observed usage window is `[0x00f57db4,0x00f57dd8)`:

| Native usage | API Usage | API UsageIndex |
|---:|---:|---:|
| 0 | 0 | 0 |
| 1 | 1 | 0 |
| 2 | 3 | 0 |
| 3 | 10 | 0 |
| 4 | 10 | 1 |
| 5 | 11 | 0 |
| 6 | 4 | 0 |
| 7 | 2 | 0 |
| 8 through 15 | 5 | Native usage minus 8 |
| 16 | 6 | 0 |
| 17 | 7 | 0 |

The producer compares native usage against `32` at `0x00c8e3c5` and enters
a diagnostic call path on the failing case. This does not supply a semantic
mapping for usage `18` through `31`. The converter supplies no native format
or component-index bounds check. The tables above do not authorize other
indices.

Microsoft's
[D3DDECLUSAGE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddeclusage)
identifies Usage `1` as `BLENDWEIGHT` and `2` as `BLENDINDICES`. Native usage
`1` and `7` therefore select those API semantics with UsageIndex `0`.
Microsoft's
[D3DDECLTYPE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddecltype)
identifies Type `0..3` as `FLOAT1..FLOAT4`, `5` as `UBYTE4`, `6/7` as
`SHORT2/SHORT4`, `8` as `UBYTE4N`, `9/10` as `SHORT2N/SHORT4N`, `15/16` as
`FLOAT16_2/FLOAT16_4` and `17` as `UNUSED`. The API normalizes `UBYTE4N`
components by division by `255.0f` and the signed-short normalized forms
by `32767.0`. These are API contracts, not CPU arithmetic in the converter.

Conditionally, native format `3` with component index `4` selects `UBYTE4N`.
Native format `6` with component index `4` selects `UBYTE4`. Neither example
identifies an observed retail mesh's stored weights or indices. The scoped
converter translates declaration metadata. It does not sum weights, remap
bones or transform vertex data.

## Reproduction and limits

On the identified analyzed program, use `ghidra decompile <VA> --json` for
`0x00c8dc10`, `0x00c37c70`, `0x00c37630`, `0x00c376b0`, `0x00c37260`,
`0x00c6ab90`, `0x00c6a9d0` and `0x004188b0`. Use
`ghidra disasm <VA> -n <count> --json` for these bounded windows:

| Start VA | Instruction count |
|---|---:|
| `0x00c8dc10` | 700 |
| `0x00c6a9d0` | 100 |
| `0x00c6ab04` | 25 |
| `0x004188b0` | 40 |
| `0x00c6ab90` | 100 |
| `0x00c6aca6` | 25 |
| `0x00c37630` | 24 |
| `0x00c376b0` | 35 |
| `0x00c37721` | 13 |
| `0x00c37260` | 10 |
| `0x00419a20` | 56 |

The converter window ends at its return at `0x00419ae0`. It excludes the
adjacent function. Use `ghidra memory read <VA> <size> --json` for
`0x00f57d90`/`36`, `0x00f57db4`/`36`, `0x00f57e30`/`8` and
`0x0110cf10`/`8`. The four distinct constant windows contain `88` bytes.
Every retrieved instruction and constant byte in these queries agreed
with the original executable. Overlapping windows are separate queries,
not additional independent evidence.

The uninspected helper `0x00c8f5a0` prevents an exact `STMS` lookup join.
No sampled skeletal mesh establishes a weight format, index format, joint
correspondence or palette mapping. Pose, MTB, buffer-producer and shader
behavior, native execution, live rendering and retail sessions are outside
this finding.
