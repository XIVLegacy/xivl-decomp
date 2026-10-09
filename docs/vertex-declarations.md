# Vertex declaration descriptor lanes

The retail client maps compact vertex usage `4` to Direct3D usage `10` with
UsageIndex `1`, conventionally `COLOR1`. The mesh descriptor parser and the
compact declaration builder expose a linked usage field. This establishes a
declaration semantic, not a geometric meaning or a particular vertex format.

## Evidence identity

| Field | Value |
|---|---|
| Module | `ffxivgame.exe`, retail build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | `0x00400000` |
| Language | `x86:LE:32:default` |
| Producing tool | `ghidra-cli 0.2.2`, Ghidra read-only disassembly, decompilation and memory queries |
| Address convention | All locators below are VAs. RVA = VA - `0x00400000`. |
| Confidence | High for the observed field operations and call edges. API names are interpretations anchored to the cited Direct3D ABI. |

The supplied executable's size and SHA-256 were recomputed. Ghidra's
`currentProgram.getExecutableSHA256()`, image base and language ID agree with
that identity. Decompiled types and inferred variable names are not the basis
for the field claims. The underlying instructions establish the operations.

## Compact descriptor conversion

**Observation.** At VA `0x00419A20`, the converter reads compact descriptors
at an `8`-byte stride. Its output elements also have an `8`-byte stride. The
field operations at VAs `0x00419A50` through `0x00419A97` are:

| Output offset | Input or operation | Direct3D field interpretation |
|---|---|---|
| `+0`, word | Zero-extend compact byte `+0` | Stream |
| `+2`, word | Copy compact word `+4` | Offset |
| `+4`, byte | Lookup at VA `0x00F57D8F`, indexed by `4 * byte(+1) + word(+6)` | Type |
| `+5`, byte | Zero | Method |
| `+6`, byte | Lookup at VA `0x00F57DB4`, indexed by `2 * byte(+2)` | Usage |
| `+7`, byte | Lookup at VA `0x00F57DB5`, indexed by `2 * byte(+2)` | UsageIndex |

The loop tests compact byte `+0` against `0xFF`. After the loop, the converter
appends an `8`-byte table-sourced end element. At callsite VA `0x00419AD3`,
it invokes vtable byte offset `0x158` on the object loaded from VA
`0x01329834`. The arguments are that object, the output element array and the
converter's output pointer. The function returns at VA `0x00419AE0`.

**Interpretation.** The output layout agrees with Microsoft's
[D3DVERTEXELEMENT9](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dvertexelement9).
Counting the `IDirect3DDevice9` methods from `QueryInterface`, including
`IUnknown`, in the pinned
[Microsoft header](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L429-L520)
places `CreateVertexDeclaration` at zero-based slot `86`. In this PE32 client,
`86 * 4 = 0x158`. The observed argument shape agrees with that method.
This supports identifying the call as declaration creation. It does not
establish a successful return or a later draw using the declaration.

## Usage 4 mapping

**Observation.** Compact byte `+2` is doubled at VA `0x00419A75`. For value
`4`, the usage lookup selects VA `0x00F57DBC`. Reading only that entry gives
functional Usage `10` and UsageIndex `1`. The converter stores these into
output `+6` and `+7` at VAs `0x00419A8F` and `0x00419A8B`.

**Interpretation.** Microsoft's
[D3DDECLUSAGE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddeclusage)
defines `D3DDECLUSAGE_COLOR` as `10`, so this entry declares `COLOR1`.
The usage lookup is independent of the Type lookup. Usage `4` alone does not
select a packed or normalized format, Stream `4`, Offset `4`, or a component
count.

## STMS block identity

**Observation.** Getter VA `0x00C8F5A0` passes the literal `STMS` rooted at
VA `0x01106FD4` and an occurrence argument to selector VA `0x00C698F0`.
At VAs `0x00C8F5AF` through `0x00C8F5B6`, a successful selection advances
the returned chunk pointer by `0x10`. Thus the block base `B` used below is
the selected STMS body after its `16`-byte chunk header.

The selector uses tag reader VA `0x00C2D7D0` to read the chunk's first dword
into a terminated string. It compares contents through VA `0x0044F9A0` at
callsite VA `0x00C69970` and requires equal lengths at VAs `0x00C6997C`
through `0x00C6998E`. Only equal tags increment the occurrence counter.
A successful occurrence match returns the chunk cursor at VA `0x00C69A0F`.
This establishes an exact STMS selection rather than a similarity between
intermediate layouts.

Routine VA `0x00C93770` independently reads BE32 descriptor count at `B+0`,
item count at `B+4` and stride at `B+8`, at VAs `0x00C93788` through
`0x00C937C2`. Its descriptor array begins at `B+0x10` with a `16`-byte
stride. The inline-data branch at VAs `0x00C9380B` through `0x00C9383B`
positions data after the header and descriptor array, then adds the
descriptor's byte offset. External-data handling is outside this finding.

**Interpretation.** The original parser's descriptor block is an STMS body.
This does not recover an original `STR` source type name or establish an
equivalence to a downstream decoder's storage.

## Mesh descriptor and builder connection

**Observation.** The following local field and call relationships are visible
in the original instructions:

| Locator | Bounded observation |
|---|---|
| VA `0x00C8E0A8` in function VA `0x00C8DC10` | Calls function VA `0x00C8F5A0` and retains its returned pointer as the descriptor block base `B`. |
| VAs `0x00C8E381` through `0x00C8E3A7` | The general element loop obtains its count from the big-endian dword at `B+0`. Its cursor begins at `B+0x1C`, the usage field of the first `16`-byte element at `B+0x10`. |
| VAs `0x00C8E49B` through `0x00C8E4C0` | Reads the word at the cursor, swaps its bytes, zero-extends it and stores it at `+0x14` of the intermediate record. Thus the element usage is BE16 at element `+0x0C`. |
| VAs `0x00C8E563` through `0x00C8E572` | Passes the intermediate record base to function VA `0x00C37C70`. The record base and usage store differ by `0x14`. |
| VAs `0x00C8E5A2` through `0x00C8E5AA` | Advances the element cursor by `0x10` and loops over the count. |
| VAs `0x00C37CD1` and `0x00C37645` through `0x00C3766E` | The available-capacity append calls a copier that copies the complete `0x24`-byte record, including usage `+0x14`. |
| VAs `0x00C3773B` through `0x00C37746` | Function VA `0x00C376B0` returns an intermediate record at `base + index * 0x24`. |
| VA `0x00C8E608` | The parser passes the record-owning object to function VA `0x00C6AB90`. |
| VAs `0x00C6AC4B` through `0x00C6AC77`, and `0x00C6ACCF` | Function VA `0x00C6AB90` appends that object to its stored set and calls builder VA `0x00C6A9D0`. |
| VAs `0x00C6AA43` through `0x00C6AA57` | The builder reads the stored objects and calls record getter VA `0x00C376B0`. |
| VAs `0x00C6AA5C` through `0x00C6AA88` | Copies record byte `+0` to compact byte `+0`, record word `+4` to compact word `+4`, and record byte `+0x14` to compact byte `+2`. |
| VAs `0x00C6AB20` through `0x00C6AB26`, and `0x00418915` through `0x0041891D` | The builder passes its compact descriptor array to factory VA `0x004188B0`, which passes it to converter VA `0x00419A20`. |

The parser stores usage as a zero-extended word. The builder uses only its low
byte. Value `4` survives this narrowing. The separately observed record and
compact copy helpers copy complete records. These helpers supply no semantic
usage mapping.

**Interpretation.** The descriptor usage field is linked to the compact usage
field used by the declaration converter. In the reviewed field path, usage
`4` supplies `COLOR1` at declaration creation. The separate Stream and Offset
fields retain their own input lanes. This evidence does not assign a model's
actual Stream, Offset or vertex data.

## Conditional Type for format 3 and component count 4

**Observation.** The general STMS element parser at VAs `0x00C8E46D`
through `0x00C8E4FC` and compact builder at VAs `0x00C6AA5C` through
`0x00C6AA99` preserve distinct descriptor lanes:

| STMS element field | Intermediate record field | Compact descriptor field |
|---|---|---|
| BE32 `+0`, local byte offset | Dword `+4` | Word `+4` |
| BE32 `+4`, format selector | Dword `+0x10` | Byte `+1` |
| BE32 `+8`, component count | Dword `+0x0C` | Word `+6` |
| BE16 `+0x0C`, usage | Dword `+0x14` | Byte `+2` |
| BE16 `+0x0E`, additional flag | Word `+0x18` | Its high bit becomes byte `+3` |

The builder narrows format to `8` bits, component count to `16` bits and
usage to `8` bits. The additional flag's original name and meaning are
unknown.

The converter's Type lookup at VAs `0x00419A53` through `0x00419A77` uses
only compact byte `+1` and word `+6`. It does not use usage byte `+2` or
additional flag byte `+3`. For format selector `3` and component count `4`,
the lookup selects VA `0x00F57D9F`, whose functional value is Type `8`.

For format selector `3`, component count `4` and usage other than `0xFF`,
size helper VA `0x00C93C80` obtains unit size `1` from the format-3 entry
at VA `0x00F64280` and multiplies it by component count at VAs
`0x00C93CE4` through `0x00C93CF1`, giving size `4` for this tuple.
Usage `0xFF` instead takes a fixed-size override after the test at VA
`0x00C93C96`. Format selector `5` takes a separate branch at VAs
`0x00C93CCD` through `0x00C93CE3`. These special branches are outside the
format-3 size observation. The preliminary routine VA `0x00C93770` has
observed swap branches for widths `2` and `4`. No general usage-independent
payload or endian contract follows.

**Interpretation.** Microsoft's
[D3DDECLTYPE](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3ddecltype)
defines Type `8` as `D3DDECLTYPE_UBYTE4N`, four unsigned byte components
normalized by dividing each by `255`. The original format-3 size entry
independently corroborates four byte components for the bounded ordinary
usage tuple. Normalization at declaration creation follows from Type `8`,
not from semantic usage or the unknown additional flag.

This is a conditional declaration format. Actual model metadata must
independently establish format `3` and component count `4` before applying
it. `COLOR1` alone establishes neither value and must not select a packed
decoder. Signed normal or tangent reconstruction is unproved.

## Bounded reproduction

1. Verify the module size, SHA-256, image base and Ghidra program identity.
2. Use `ghidra disasm` at the mesh and container locators above. Follow the
   returned block pointer, general loop cursor, byte swap, record base passed
   to the append call and record-owning object passed to the builder.
3. Inspect record copier VA `0x00C37630`, record getter VA `0x00C376B0`,
   builder VA `0x00C6A9D0` and factory VA `0x004188B0`. Confirm each cited
   field operation against instructions, rather than inferred source types.
4. Inspect converter VA `0x00419A20` through its return at VA `0x00419AE0`.
   Check output offsets, separate lookup indexes and vtable arguments.
   Read only the `2`-byte usage entry at VA `0x00F57DBC` with
   `ghidra memory read` to reproduce the functional pair.
5. Compare the observed layout and call with the cited Microsoft ABI. Count
   interface methods from its exact `IDirect3DDevice9` declaration.
6. Inspect getter VA `0x00C8F5A0`, selector VA `0x00C698F0`, tag reader
   VA `0x00C2D7D0` and comparison VA `0x0044F9A0`. Read only the literal
   at VA `0x01106FD4` to reproduce STMS selection and the header skip.
7. Inspect the STMS count and inline-data calculations in VA `0x00C93770`.
   Check the separate parser and compact builder lanes above. Read only the
   selected Type entry at VA `0x00F57D9F` for format `3` and count `4`.
8. Inspect size helper VA `0x00C93C80` and only the format-3 unit-size entry
   at VA `0x00F64280`. Keep its usage-`0xFF` override and format-`5` branch
   separate from the ordinary format-3 observation.

## Unresolved boundaries

An original geometric meaning or source enum name for mesh usage `4` is
unknown. Neither `COLOR1` shader consumption nor this declaration mapping
establishes a binormal interpretation.

The STMS descriptor linkage and conditional format-3/count-4 Type are
established. Original `STR` naming, equivalence to a downstream decoder and
the additional flag's meaning remain unknown. Actual model format, component
count, Stream, Offset and payload values require separate evidence.
The format lookup is distinct from the usage lookup. Vertex bytes, signed
normal or tangent reconstruction, fallback streams, missing blend inputs,
TEXCOORD6/7 defaults and dynamic skinning data remain unproved.

The complete ordering routine, every allocation or insertion path, malformed
or out-of-range descriptors and alternate parser branches are outside this
finding. No retail session, live GPU acceptance, selected model shader or
runtime rendering result was validated.
