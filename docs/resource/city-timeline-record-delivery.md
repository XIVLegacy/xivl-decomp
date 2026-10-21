# City TimeLine record delivery and SCB relocation

The selected Limsa, Gridania and Ul'dah Halloween show TimeLines occur in
their layout root's definition list and object inventory. Their declarations
include SCB source `+0x14` among the fields relocated by the native LYB
loader. Their class ID `0xEA3` matches the registered TimeLine creator.
On the admitted base-object creation path, the relocated node reaches the
created base's slot 7 and its attached-record cell.

This closes the structural delivery and relocation join for these three
examples. Given the selected DAT's valid loaded LYB payload, successful
creation and retained fields, `0x00AB38B0` consumes the corresponding pinned
SCB pointer/length. It does not identify an observed active city layout,
authenticate a cache hit, establish resource readiness or prove playback.

## Evidence identity and method

Native addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image
base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode decoded
the four normal instruction ranges below. Each complete range matched the
retained Ghidra listing's addresses and instruction bytes without gaps.
Python `struct` decoded the selected original DAT fields and declarations.
These were static file-backed checks, without client execution.

The [selected city inputs](city-halloween-member-context-joins.md#original-inputs-and-method)
own the DAT sizes/SHA-256, TimeLine/SCB locations and complete SCB hashes:

| City | Original DAT | LYB payload file base F | Selected TimeLine file offset T | Selected SCB file offset C / bytes |
| --- | --- | --- | --- | --- |
| Limsa `obj1_h` | `data/29/D9/00/01.DAT` | `0x99D0` | `0xEC034` | `0x120250 / 0x370` |
| Gridania `hlw1a` | `data/29/B0/00/01.DAT` | `0xD5B0` | `0xFEC2C` | `0x137D70 / 0x330` |
| Ul'dah `hw1` | `data/61/5A/00/01.DAT` | `0x90B0` | `0xCA4E0` | `0xF9580 / 0x330` |

All DAT offsets below are absolute file offsets. Native heap addresses use a
separate symbol H for the loaded LYB payload base. File offsets are never
substituted directly for H or a native object pointer.

The declaration interpretation follows
`xivl-tools:docs/formats/world-layout.md`, "lyb payload and object list",
"Node declarations" and "Objects and classes". The existing
[LYB loader contract](bgchip-model-binding.md#lyb-relocation-and-attachment)
owns the resource identity checks, buffer getter, payload adjustment,
relocation-before-enumeration ordering and generic base-object factory path.
The checks here qualify its selected city inputs.

## Relocation of the selected source fields

Each selected payload has byte `+0x0D = 0`, byte `+0x0E = 2`.
Dispatcher `0x00C1F110` therefore calls `0x00C1EF90(0,H)` at
`0x00C1F12C`, then sets bit 0 of byte H+`0x0D`. An already-set bit skips
this work. The original files' clear bits establish the selected initial
branch, not the state of a later live buffer.

The helper computes delta `H - argument1`, which is H for this call. It
adjusts the kind-table and inventory pointers. Its nested extent/field loop
reads each field descriptor's first u16 as a byte offset and adds delta to
that node dword only when the dword is nonzero. The descriptor's other two
bytes are not used by this helper. It finally adjusts inventory entries.

The following declared extents contain each selected T at the indicated
zero-based ordinal, with `0x28`-byte stride. Every row has extent count 1.

| City | Kind row / extent / descriptor list | TimeLine run start / count | Selected ordinal |
| --- | --- | --- | ---: |
| Limsa | `0xF0C8 / 0xF658 / 0xF3D8` | `0xEB4F4 / 91` | 72 |
| Gridania | `0x153B4 / 0x15940 / 0x156C0` | `0xFDF5C / 109` | 82 |
| Ul'dah | `0xEAF4 / 0xF054 / 0xEDEC` | `0xC9900 / 95` | 76 |

All three descriptor lists are exactly
`(0x04,0), (0x08,2), (0x14,0), (0x1C,0), (0x20,2)`.
SCB reference `+0x14` is adjusted; its following length `+0x18` is outside
that list. Inclusion in this row alone would not exclude another declaration
writing either field: the format contract reports overlapping extents.

The complete relocation metadata of these three selected DATs supplies
that missing uniqueness check. All rows have one extent; field offsets
plus four fit their node strides. Enumerating every descriptor's four-byte
write at each declared node yields exactly one write overlapping each
selected `+0x14` dword, and none overlapping its `+0x18` length or selected
SCB span. This counts potential writes even when a source dword is zero.
No descriptor writes overlap the payload header, kind rows, extent records,
descriptor lists or inventory, so they do not change the relocation controls.
Those control regions and the final inventory writes are disjoint from the
selected source, length and SCB spans.

| City | Kind rows / count | Descriptor range / count | Extent range / count | Metadata bytes |
| --- | --- | --- | --- | ---: |
| Limsa | `0xEDE8 / 54` | `[0xF148,0xF430) / 186` | `[0xF430,0xF6B8) / 54` | 2256 |
| Gridania | `0x150D4 / 54` | `[0x15434,0x15718) / 185` | `[0x15718,0x159A0) / 54` | 2252 |
| Ul'dah | `0xE834 / 52` | `[0xEB74,0xEE44) / 180` | `[0xEE44,0xF0B4) / 52` | 2176 |

The selected nonzero source dwords therefore give:

| City | Source field / serialized value P | Length field / N | After the selected relocation |
| --- | --- | --- | --- |
| Limsa | `0xEC048 / 0x116880` | `0xEC04C / 0x370` | `u32(D+0x14) = H+0x116880` |
| Gridania | `0xFEC40 / 0x12A7C0` | `0xFEC44 / 0x330` | `u32(D+0x14) = H+0x12A7C0` |
| Ul'dah | `0xCA4F4 / 0xF04D0` | `0xCA4F8 / 0x330` | `u32(D+0x14) = H+0xF04D0` |

Here `D = H+(T-F)` and `F+P=C`. This is the selected field's relocation
derivation. It does not require a native address to equal C, or require the
loaded payload's address to equal its file offset. The source and length
still need valid storage and must remain unchanged when consumed.

## Root enumeration and class admission

The payload headers supply these inventory pointers/counts. Inventory entry
0 points to the root wrapper; wrapper `+0/+4` supplies the definition
array/count. The exact selected entry in each array points to T after
adding F to its serialized value.

| City | Inventory / count | Root wrapper | Definitions / count | Definition index / field | Matching inventory index / field |
| --- | --- | --- | --- | --- | --- |
| Limsa | `0x99E8 / 5376` | `0x2765C` | `0x2767C / 5375` | `4973 / 0x2C430` | `4974 / 0xE7A0` |
| Gridania | `0xD5C8 / 7875` | `0x3075C` | `0x3077C / 7874` | `7540 / 0x37D4C` | `7541 / 0x14B9C` |
| Ul'dah | `0x90C8 / 5595` | `0x23638` | `0x23658 / 5594` | `5386 / 0x28A80` | `5387 / 0xE4F4` |

The wrapper and definition-array declarations also supply the required
pointer fixups. These rows have extent count 1. The wrapper has one
`0x20`-byte node with descriptors
`(0,0), (8,2), (0x0C,2), (0x10,2), (0x14,2), (0x18,2), (0x1C,2)`.
The array has four-byte stride, the definition count above, and descriptor
`(0,0)`, so each selected definition entry receives the same delta.

| City | Wrapper kind / extent / descriptors | Definition-array kind / extent / descriptors |
| --- | --- | --- |
| Limsa | `0xEDE8 / 0xF430 / 0xF148` | `0xEDF8 / 0xF43C / 0xF164` |
| Gridania | `0x150D4 / 0x15718 / 0x15434` | `0x150E4 / 0x15724 / 0x15450` |
| Ul'dah | `0xE834 / 0xEE44 / 0xEB74` | `0xE844 / 0xEE50 / 0xEB90` |

This supplies the inventory-to-wrapper-to-definition pointer chain consumed
by the published manager enumeration. It establishes exact positive
membership, without a global array-order or node-ID-to-index rule.

Each selected node's class reference at T+4 names the class record below,
whose first dword is `0xEA3`. Its immediate parent names
`BaseObjects/BaseObject`, satisfying the published class-chain predicate
on these valid relocated class views. The class-record reference shape is
owned by the cited world-layout format contract.

| City | TimeLine class record | Parent-reference field / parent class | Parent-name pointer field / string |
| --- | --- | --- | --- |
| Limsa | `0x42EC0` | `0x42ED0 / 0x42D30` | `0x42D38 / 0x1B49D` |
| Gridania | `0x4B9C4` | `0x4B9D4 / 0x4B820` | `0x4B828 / 0x1EC54` |
| Ul'dah | `0x3D7AC` | `0x3D7BC / 0x3D61C` | `0x3D624 / 0x18117` |

The complete-metadata check also yields one descriptor write at each
selected wrapper's definition pointer, definition entry, T+4 class
reference, class-parent reference and parent-name pointer. None overlaps
the wrapper count, selected class ID or 23-byte parent name. These cells
are outside the relocation control regions and inventory writes.

## Creator selection and attached-record handoff

The manager's missing-binding branch calls `0x00A8B350`. Its first stack
argument is the supplied node D, retained in EBX at `0x00A8B354`. The
published class-ID getter `0x00A902B0` reads the class descriptor's first
dword. Factory slot 28 obtains a creator/category through the existing
slot-26 predicate lookup.

TimeLine creator table `0x0103C4F0` slot-6 cell `0x0103C508` selects
`0x00AA6BC0`. Its complete 15-byte body compares its argument with the
dword at `0x010A2CB8`, verified as `0xEA3`, and returns equality in AL.
Its slot-5 cell `0x0103C504` selects the published category-7 getter
`0x00AA6B60`. The
[registered TimeLine creator and base factory](timeline-weather-record-view.md#creator-interface-and-paired-factories)
own the successful category-7 product B, complete object+4 with table
`0x0106207C`. Creator search/category registration must remain intact and
select this creator; class-ID equality alone does not establish live state.

After successful creation, `0x00A8B3A1..0x00A8B3A6` pushes the same
retained D and invokes B slot 7. With normal callee-saved register
preservation, no different node replaces that EBX value on this branch.
Cells `0x01062098/0x0106209C` select the existing slot-7 attachment and
slot-8 getter endpoints `0x008D5440 -> 0x00AA89A0` and `0x008D5570`.
Their [interface-relative contract](bgchip-model-binding.md#creator-and-attached-key)
stores D at B+4 and returns that D. The handoff uses the relocated node
address, rather than a copy of its serialized bytes.

Consequently, on this selected admitted path:

```text
B.slot8() = D = H+(T-F)
u32(D+0x14) = H+P = H+(C-F)
u32(D+0x18) = N
```

The [resource-result finding](timeline-scb-resource-binding.md#attached-source-and-cache-lookup)
owns `0x00AB38B0`'s consumption of those fields, cache and copy/view branches,
and the resulting SCB payload-to-parser binding. Together the findings join
the selected city definition to that conditional parser input.

## Confidence and activation boundary

The selected raw fields, declaration membership, fixup uniqueness, class-ID
match and successful-path D-to-B handoff have direct retail-byte support. Their
composition is conditional on the selected layout actually supplying H,
successful resource identity/admission/factory checks, valid storage and
retained service mappings and fields. Already-relocated buffers take the
skip branch and need their existing pointer state authenticated separately.
No selected runtime invocation was observed.

This removes a structural source/attachment gap from the three-city
Halloween investigation. It does not recover a server opcode, calendar
trigger or successful seasonal activation. The
[member/context qualification](city-halloween-member-context-joins.md#conditional-native-composition)
retains the selected parent/TimeLine context, same-U lookup, Limsa group
registration ordering and separate operation-provider/indexed-actor/material limits.
Cache-hit payload identity and resource readiness remain separate evidence.

## Reproduction

Authenticate the executable and the three DAT identities above. Decode these
half-open original instruction ranges and compare their complete bytes with
the retained listings:

| Function | Half-open VA range | Bytes |
| --- | --- | ---: |
| Variant dispatcher | `[0x00C1F110,0x00C1F159)` | 73 |
| Selected relocation | `[0x00C1EF90,0x00C1F081)` | 241 |
| Base creation/attachment | `[0x00A8B350,0x00A8B3E6)` | 150 |
| TimeLine creator predicate | `[0x00AA6BC0,0x00AA6BCF)` | 15 |

Read the five four-byte PE cells listed in the creator section. For each DAT,
read the 24-byte payload header, selected T's 12-byte prefix and eight-byte
SCB reference/size pair, inventory entry 0, the eight-byte root wrapper prefix,
the selected four-byte definition and matching inventory entries, class ID,
parent-reference/name-pointer fields and the 23-byte parent name including
NUL: 99 bytes per city. Read each city's complete contiguous metadata span
from its kind-row start to extent-range end in the uniqueness table:
6684 bytes total. Decode 16-byte rows, their 12-byte extents and four-byte
descriptors using the native count/stride widths. For each descriptor offset
o and node index i, test four-byte write `F+nodeStart+i*stride+o` against the
selected fields, SCB span and control regions; nodeStart is the extent's
serialized first-node value. Check the selected root/list/TimeLine
rows within those metadata spans and reproduce the exact write counts above.
This uses 6981 unique asset bytes, plus 479 code and 20 PE-data bytes.
Reuse the cited generic manager, registration, attachment/getter and
resource-result contracts.
No complete object inventory, original SCB payload, resident leaf or live
client state is required for this structural proof.
