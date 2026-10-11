# Ordinary format-6 STMS payload lanes

The ordinary inline STMS path preserves the current order of four format-6
byte lanes under the conditions below. This establishes a bounded payload
contract, separate from declaration conversion and shader interpretation.

## Evidence identity

| Field | Value |
| --- | --- |
| Module | `ffxivgame.exe`, retail build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | `0x00400000` |
| Architecture | PE32 `coff-i386` |
| Producing tool | `llvm-objdump`, Apple LLVM `17.0.0`, bounded read-only disassembly |
| Address convention | Locators are VAs. RVA = VA - `0x00400000`. |

The executable identity was recomputed. Displayed instruction bytes were
checked independently against file bytes through the PE section table.
The size-table value was read through that same VA-to-file mapping.

## Size and preliminary processing

**Observation.** Size helper VA `0x00C93C80` reads BE16 usage at descriptor
`+0x0C`, BE32 format at `+4` and BE32 component count at `+8`. Usage `0xFF`
returns fixed size `2` at VAs `0x00C93C96` through `0x00C93CA6`. Format `5`
takes a separate branch at VAs `0x00C93CCD` through `0x00C93CE3`.

For format `6`, component count `4` and usage other than `0xFF`, the lookup
at VA `0x00C93CE4` reads a little-endian dword at
`0x00F64274 + 4 * 6 = 0x00F6428C`. Its value is `1`. Multiplication at
VA `0x00C93CEB` returns ordinary size `4`.

Preliminary routine VA `0x00C93770` calls this helper at VA `0x00C937F3`
and divides its result by component count at VA `0x00C937FD`. This tuple
therefore selects width `1`. Tests at VAs `0x00C9383E` through
`0x00C93848` send width `1` to VA `0x00C938D2`, bypassing both payload
swap loops. That descriptor branch writes no payload bytes. The initial
state comparison at VA `0x00C93776` can skip processing altogether.

For inline data, STMS body byte `+0x0E` is zero. VAs `0x00C9380B` through
`0x00C9383B` derive the pointer after the `16`-byte header and descriptor
array, then add the descriptor byte offset. Descriptor count and descriptor
stride, not format, determine this inline base.

## Inline staging and copy boundary

**Observation.** Parser callsite VA `0x00C8E0B3` requests preliminary state
`1`. The following call at VA `0x00C8E0BA` targets VA `0x00C935D0`, whose
entry immediately returns. VAs `0x00C8E0BF` through `0x00C8E129` derive the
inline source and item-count-times-stride byte count, then call factory
VA `0x00419640`. This source derivation has no format-6 transformation.

The existing [STMS copy finding](../vertex-declarations.md#conditional-stms-byte-order)
records the factory and staging route for a nonzero source pointer. Its
consumer at VA `0x0041A59A` uses helper VA `0x009D4600` to copy the byte
range. The consumer call and its source, destination and length argument
operations at VAs `0x0041A593` through `0x0041A59A` were independently
inspected with `llvm-objdump`. The published nonoverlapping copy finding
covers matching byte positions, alignment, bulk, tail and accelerated paths.
Those copy operations do not depend on the STMS format selector.

**Interpretation, high confidence within these conditions.** The format-6,
component-count-4 ordinary descriptor has four one-byte components. Its
preliminary branch and the observed valid copies preserve their current
byte-lane order. This path does not reverse the group as a dword or
normalize the components on the CPU. The independent
[declaration finding](wrb-vertex-declaration.md#bounded-numeric-lookups)
records the Type mapping. Declaration conversion remains a separate fact.

## Conditions and unresolved boundaries

This finding requires a valid inline source, ordinary usage other than
`0xFF`, component count `4`, disjoint descriptor payloads and valid
nonoverlapping copy ranges. Other descriptors can overlap the same bytes
and enter their own swap loops. External data, prior producer history,
later mutable writes, allocation failures, backend selection, skip flags
and alternate upload routes remain outside the finding.

No actual model payload, TEXCOORD meaning, skinning weight or index, shader
binding, successful live upload or retail draw follows. Reproduce the
bounded claim from the exact identity, size-table entry, size/count
division, width branches, inline pointer and cited copy operations.
