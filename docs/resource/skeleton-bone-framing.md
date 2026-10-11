# Bounded SKL record and hierarchy framing

The retail skeleton consumer establishes a 176-byte record stride, three
string indices, and four hierarchy indices for an explicitly selected
little-endian `SEDBSKL ` block. This finding is limited to one identified
resource and the native equations below. Transform contents remain opaque.

## Inputs and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is VA `0x00400000`. All native addresses below are VA.
Subtract `0x00400000` for RVA.

Resource: original installed `client/chara/pc/c001/skl/0001`, size `54672`
bytes, SHA-256
`d90a756fc1cba6f03bb8dc102b367feb905b5e57a00d209ff4fccddaa17cd031`.
The path selects this input. It does not establish an automatic resource
selection rule or a runtime appearance relationship.

Producing tools: `ghidra` CLI `0.2.2`, Ghidra `12.1.4 PUBLIC`, JDK
`26.0.2.1`, and Python `3.14.7` standard-library `pathlib`, `hashlib`,
`struct`, and bounded byte searches. The configured Ghidra program is
`ffxivgame.exe`, language `x86/little/32/default`. Native observations use
`ghidra disasm <VA> -n <bounded instruction count> --json` and
`ghidra decompile <VA> --with-params --json`. The instruction bytes at the
cited locators were independently compared with the identified original
executable through its PE section mapping. Decompiled types and signatures
are not treated as source contracts. This method uses an existing analyzed
program, rather than a fresh-project analysis or a live client run.

## Skeleton consumer identity

The [RTTI catalog](../../config/ffxivgame.rtti.json) identifies
`SQEX::CDev::Engine::Dw::SceneGraph::Resource::SkeletonResourceNode`
at vtable RVA `0x00c9a91c`. Reading its original executable table confirms
three function pointers: VA `0x00a45e80`, `0x00a45940`, and `0x00a45e90`.
The original complete-object locator and type descriptor confirm the type
identity. The
[vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl)
records slot `1` at function RVA `0x00645940` with the auto-analysis symbol
`FUN_00a45940`.

At VA `0x00a45940..0x00a4595d`, that function dispatches the supplied
handle's vtable slot at byte offset `+4` with node member `+0x10`, then
forwards the returned pointer to VA `0x00e0de10` at callsite
`0x00a45955`. VA `0x00e0de1a` calls `0x00e0dd00`. Exact argument types,
resource ownership, and the native outer container resolver are unresolved.

## Explicit container selection

The established
[PWIB boundaries](https://github.com/XIVLegacy/xivl-tools/blob/c364f4cc8d8d85f7bc900b7f5d5e44687ac66f06/docs/formats/gtex-pwib.md#pwib-contract)
and
[RES directory fields](https://github.com/XIVLegacy/xivl-tools/blob/c364f4cc8d8d85f7bc900b7f5d5e44687ac66f06/docs/formats/sedb-res.md#res-subresource-directory)
provide the framing used here. PWIB fields at `0x04`, `0x08`, and `0x0c`
are big-endian u32 values `54672`, `16`, and `54672`. The second segment
is empty, and no bytes follow the PWIB total.

All following ranges are absolute file offsets and half open.

| Selection | Container range | Header bytes | Entry count | Payload base | Selected physical entry | Relative offset | Entry size | Selected range |
|---|---|---:|---:|---:|---:|---:|---:|---|
| Root RES | `[16,54672)` | 64 | 6 | 176 | 3 | 4992 | 49226 | `[5168,54394)` |
| Nested RES | `[5168,54394)` | 64 | 4 | 5296 | 0 | 0 | 17812 | `[5296,23108)` |

The explicit physical entry path is `[3,0]`. The selected eight-byte tag
is `SEDBSKL `, including its final space. Directory selection is an input
to this finding. Neither nearby entries nor the path name establishes
native selection. Other bytes and container metadata retain their existing
framing or remain opaque.

## Native framing observations

Let `B` denote the selected SKL block start. `LE32` denotes an unsigned
little-endian dword read. The identified client uses native little-endian
loads on the observed byte-`+0x0c`-zero path.

| Locator | Direct observation |
|---|---|
| `0x00e0dd00..0x00e0dd33` | Compares all eight tag bytes with `SEDBSKL ` |
| `0x00e0dd6f..0x00e0dd7b` | Byte `B+0x0c` equal to 1 invokes `0x00e0d790`, then clears that byte |
| `0x00e0dd7f..0x00e0dd96` | Accepted arithmetic branch requires `LE32(B+0x08) >= 7000` and zero remainder when dividing `LE32(B+0x48)` by `0xb0` |
| `0x00e0ddd4..0x00e0dddf` | Forms record base `R = B + 0x30 + 16 * (LE32(B+0x30) + 1)` |
| `0x00e0dde5..0x00e0de03` | Calls `0x00e0eaa0` with record base `R`, record count `LE32(B+0x48) / 176`, string base `R + LE32(B+0x34)`, and string count `LE32(B+0x38)` |
| `0x00e0eaf6..0x00e0eb80` | Registers consecutive string references and scans toward NUL with a local `0x200` step bound, advancing once after each scan |
| `0x00e0eb86..0x00e0ebfe` | Registers record references and advances the supplied record pointer by `0xb0` |

These observations establish framing and references into supplied memory.
They do not establish an owned deserialized bone type. Error callbacks are
visible, but malformed-input rejection and termination behavior remain
unresolved. The scan step bound is not an encoding or a native truncation
policy claim.

The selected shape has the following measured fields. Widths apply only to
the named fields. Other header and directory bytes remain opaque.

| Offset from B | Width | Observation in the selected input | Interpretation |
|---|---:|---|---|
| `0x08` | 4 | 7000 | Unnamed scalar used by the native threshold |
| `0x0c` | 1 | 0 | Observed path without conversion |
| `0x0e` | 2 | `0x30` | SEDB header size metadata |
| `0x10` | 4 | 17812 | Declared size equals the selected extent |
| `0x30` | 4 | 1 | Count used in the native record-base equation |
| `0x34` | 4 | 16720 | String base offset relative to R |
| `0x38` | 4 | 99 | Counted strings |
| `0x3c` | 4 | Uninterpreted | Unnamed scalar |
| `0x44` | 4 | 0 | Directory field, purpose unresolved |
| `0x48` | 4 | 16720 | Native record-array byte length |

The fixed header is `[B,B+0x30)`, the four extended dwords occupy
`[B+0x30,B+0x40)`, and one 16-byte directory row occupies
`[B+0x40,B+0x50)`. The directory row's fields at `B+0x40` and `B+0x4c`
are unresolved. The native loader reads `B+0x48` directly and forms `R`
from the count equation. It does not use the measured zero at `B+0x44`
to find the first record. Arbitrary directory rows and nonzero values there
are outside the measured shape.

## Record indices and hierarchy

The original RTTI table at RVA `0x00d3bd64` confirms
`SQEX::CDev::Engine::Dw::ResourceLoader::HierarchyDataContainer`
and contains the three string accessors below among its 34 pointers.
On their reference-memory path, each reads a record index, checks it against
the string-table count, and resolves a string reference.

| Record offset | Width | Interpretation | Native locator |
|---|---:|---|---|
| `0x00` | 4 | Primary record-name index | `0x00e11de0..0x00e11e15`, lookup at `0x00e0e460..0x00e0e4bc` |
| `0x04` | 4 | Auxiliary string index, purpose unresolved | `0x00e11e20..0x00e11e57` |
| `0x08` | 4 | Auxiliary string index, purpose unresolved | `0x00e11e60..0x00e11e97` |
| `0x38` | 4 | Parent record index | Getter `0x00e11cf0`, insertion `0x00e0e15e` |
| `0x3c` | 4 | First-child record index | Getter `0x00e11d00`, insertion `0x00e0e17a..0x00e0e191` |
| `0x40` | 4 | Next-sibling record index | Getter `0x00e11d10`, traversal `0x00e0e0be` and `0x00e0e0f8`, insertion `0x00e0e12f..0x00e0e134` |
| `0x44` | 4 | Self record index | Getter `0x00e11c10`, insertion `0x00e0e18e` and `0x00e0e12f` |

The hierarchy names are interpretations grounded in the insertion algorithm.
VA `0x00e0e1b0..0x00e0e204` skips records whose `+0x38` is
`0xffffffff`, then calls `0x00e0e150`. That function selects the record
indexed by `+0x38` and sets its `+0x3c` to the current record's `+0x44`
when empty. Otherwise `0x00e0e070..0x00e0e149` follows `+0x40` from
the selected record's `+0x3c` and appends the current `+0x44`.
`0xffffffff` is the no-link sentinel on these paths. Callsite VA
`0x00e10e77` invokes the assembly routine `0x00e0e1b0`.

Every record byte in `[0x0c,0x38)` and `[0x48,0xb0)` remains opaque.
No NUL-terminated string text is published. Its character encoding is
unresolved. The primary index identifies a name used by lookup. Auxiliary
index purposes and hierarchy ordering rules remain unresolved.

## Selected extent accounting

For `B = 5296`, native `R = 5376`. The length `16720` gives exactly
`95` records of `176` bytes. The string base is `22096`. A bounded scan
of exactly `99` NUL-terminated strings ends at `23107`.

| Range | Bytes | Role | SHA-256 |
|---|---:|---|---|
| `[5296,5344)` | 48 | Fixed header | `33aeb5a08e81dd668aaeb3c94056ed2b547b39e80de4755186988550699a3e21` |
| `[5344,5360)` | 16 | Extended dwords | `f2107ca82372cd94da4b6271ce72239a3076c875b920b99b133211ea906beb24` |
| `[5360,5376)` | 16 | Directory row | `dc4ce418773fd019da73d564ccce9946d295e2c106273ec9d6a043091e8aa269` |
| `[5376,22096)` | 16720 | Record array | `05aa9ef7270f971457a69ce4bf1e9fc8aa8ff86051af0d3fe4e9c33041733a7e` |
| `[22096,23107)` | 1011 | Counted string pool | `d3d912c3ed54577033bf6c4c68bd2f499ad468a3ff51b42763829608aefb7f3e` |
| `[23107,23108)` | 1 | Opaque post-pool slack | `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d` |

The final byte is observed zero. Its alignment purpose is unresolved. Its
span and digest are retained, and no pool-to-end closure is claimed.
These six ranges partition all `17812` selected bytes without gaps or
duplicate leaf bytes. Complete accounting does not imply complete semantics.

For this input, all three string indices in each of the 95 records are below
99, and the 95 primary indices are unique. The longest counted string is
14 bytes. All hierarchy links are either `0xffffffff` or below 95, every
self index equals its record ordinal, and exactly one record has no parent.
Parent links are acyclic. Each first-child and next-sibling chain is acyclic
and contains exactly the records whose parent index selects that record.
These measurements independently corroborate the native hierarchy
interpretation for this input.

## Reproduction and limits

Verify both input sizes and SHA-256 identities before reading. Read PWIB
boundary fields as big-endian u32. Apply only the established RES fields
and explicit physical entries `[3,0]`. Bound directory arithmetic and every
selected range by its containing extent. Reproduce the native equations
with little-endian dword reads, then bound every record read and NUL search
by `[5296,23108)`. Count exactly the declared strings and retain the
remaining span as opaque. Compute partition hashes without exporting
records, names, transforms, or resource bytes.

Use the cited native targets with bounded disassembly. Resolve the two
vtable RVAs through the original PE section table, including their
complete-object locators and type descriptors. Compare the cited instruction
bytes against the identified original executable. Check string-index
bounds, link bounds, self ordinals, parent acyclicity, sibling acyclicity,
and parent-to-child-chain equality independently of a future parser.

This finding does not establish other scalar versions, multiple SKL
entries, the conversion path, header flags, directory semantics beyond the
native equations, native outer selection, or runtime lifetime. Unresolved
transform contents include translation, rotation representation and order,
scale, matrix layout, inverse bind data, coordinate space, handedness, and
units. Mesh skin palettes, weights, joint selection, MTB channels, actions,
timing, frame rate, interpolation, defaults, and live rendering parity
remain unresolved. No bind skeleton rendering, animation, skinning, or
full skeleton-format support follows from this bounded framing.
