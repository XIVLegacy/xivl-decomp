# Bounded skeleton Joint and output access

A native name-lookup caller calculates a Joint address from the returned
ordinal, reads that Joint's associated hierarchy-output pointer, and saves
the pointer beside a 16-bit ordinal. A separate complete body returns an
indexed Joint's associated primary-string pointer. Neither observation
establishes a standalone indexed-Joint API or runtime final-matrix getter.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The original PE32 machine is `0x014c`, optional-header magic `0x010b`,
and image base VA `0x00400000`. All native addresses below are VA.
Subtract `0x00400000` for RVA, then map through the original section table.

The bounded method uses the existing analyzed `ffxivgame.exe` program,
`ghidra x-ref to <VA> --json` and `ghidra disasm <VA> -n <count> --json`.
An independent audit repeated the same query specifications and compared
every retrieved instruction with the original executable through its PE
mapping. It checked original RTTI anchors, exact catalog table extents,
xref displacement or literal corroboration, field widths and address
arithmetic. Executable size and SHA-256 matched before and after.
No decompiler-inferred signature supplies a field contract.

The committed
[RTTI catalog](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/config/ffxivgame.rtti.json),
[slot catalog](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/config/ffxivgame.vtable_slots.jsonl)
and
[client vtable index](https://github.com/XIVLegacy/xivl-client-structs/blob/08de0f7428a133a689627ac0cbf60d97daed2a71/manifests/rtti_vftable_index.json)
provide identity and slot-order leads. Original anchors corroborate
`CharacterSkeletonNode`, `SkeletonResourceNode` and `Joint` at table VAs
`0x0109a2dc`, `0x0109a91c` and `0x0109a31c`. Original table targets
agree with their respective catalog extents of `1`, `3` and `13` slots.
Reads remain within a maximum `32` slots at each granted table. Entries
outside those catalog extents supply no additional slot claims.

## Committed association basis

Let `S` denote the scoped skeleton receiver, `i` a construction ordinal,
and `ptr32` a native 32-bit pointer read. The committed
[Joint association](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/docs/resource/skeleton-joint-access.md#L47-L85)
establishes `J_i = ptr32(S+0x5c) + 0x1c*i` and
`ptr32(J_i+0x10) = O_i`, the associated hierarchy-output record.
Its
[primary-string and ordinal finding](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/docs/resource/skeleton-joint-access.md#L87-L129)
owns the primary-string association at Joint `+0x04` and the name lookup
at `0x00a3f7e0`: a zero-extended 16-bit ordinal on observed equality,
or `0xffffffff` on observed miss paths. Those meanings come from that
committed basis, rather than from catalog agreement.

## Count and indexed primary-string return

The complete body at `[0x00a6d540,0x00a6d54b)` follows receiver
`+0x04`, then `+0x08`, and returns the skeleton word at `+0x50`
zero-extended to EAX. This establishes a 16-bit count read and 32-bit
return, without a general maximum-count or overflow contract.

The complete body at `[0x00a6d550,0x00a6d56d)` reads one 32-bit
stack index `i`, follows the same receiver chain, and returns
`ptr32(ptr32(S+0x5c) + 0x1c*i + 0x04)` in EAX. The committed primary
string association identifies this as the associated primary-string
pointer. It is a 32-bit pointer return, rather than a Joint pointer.
This body contains no index admission, upper count check or null check.

The adjacent complete body at `[0x00a6d520,0x00a6d53d)` rejects a
null supplied string with `0xffffffff`. On its other displayed path it
follows receiver `+0x04`, then `+0x08`, and tail-transfers to
`0x00a3f7e0`. That wrapper's null-input path does not establish null
or count safety inside the underlying lookup.

## Associated output pointer consumer

Within caller `0x00c6c1f0`, callsite `0x00c6c321` invokes the
established name lookup. Instructions at `0x00c6c326..0x00c6c328`
compare returned EAX with EBX and take a signed-less-than branch away
from the following pointer path. At `0x00c6c32a..0x00c6c33d`, the
caller follows a receiver pointer's `+0x08`, reads that skeleton's
Joint base at `+0x5c`, and calculates:

```text
J = ptr32(S+0x5c) + 0x1c*EAX
```

The calculation consumes the full returned 32-bit EAX. The equality
comparison and branch at `0x00c6c340..0x00c6c342` compare this calculated
address with EBX and skip its use on equality.

The resulting transfers are directly observed:

| Source locator | Transfer |
|---|---|
| `0x00c6c34b` | Read `ptr32(J+0x10)` |
| `0x00c6c34e` | Store that 32-bit pointer at `ptr32(ESI+0x10) + 4*EBP` |
| `0x00c6c354` | Separately store low word AX at `ptr32(ESI+0x14) + 2*EBP` |

The committed association identifies the loaded pointer as the Joint's
associated `O_i`. This proves an in-body pointer consumer and pointer-table
store alongside a word ordinal table. It does not prove a returned output
pointer through an accessor, matrix-content reads, matrix copies or a mesh
palette. No upper count check appears between the displayed lookup call
and the associated pointer read.

The earlier window displays EBX zeroed at `0x00c6c214`. It also shows
requests scaled by `4` and `2`, retained bases at receiver `+0x10` and
`+0x14`, and corresponding dword and word initialization stores.
The
[pointer-initialization finding](skeleton-output-pointer-initialization.md)
owns retained output-base and first-Joint pointer stores, the direct incoming
receiver route and local EBX tracking across the filled instruction window.
The called allocation targets and outgoing callees remain uninspected.
Thus the comparisons above are reported against EBX as observed. A
complete zero-sentinel, null-safety, signed-index-validation, allocation
or ownership contract is not established.

## Other bounded callers and table prefixes

At `0x00a3eb50`, another caller consumes the established lookup result,
maps the observed `0xffffffff` miss to zero, and stores AX into its word
array at receiver `+0x14`, indexed by ESI. This is ordinal consumption.
Its window ends at `0x00a3eb6c`, before a complete return is established.

The windows at `0x008dd880` and `0x008dd910` reach the established
lookup through receiver `+0x334`, then `+0x08`. Other paths use an
uninspected alternate target. These windows add no indexed-Joint return
or output accessor contract. The first omits some return paths. The
second includes its return at `0x008dd976` and an incidental next-body
prefix, which supplies no additional function claim.

The approved `SkeletonResourceNode` table targets `0x00a45e90` and
`0x00a45940` were inspected only as incomplete prefixes. The latter
stores a supplied pointer at receiver `+0x08`. That store has no
established semantic role here. Their referenced data and outgoing
targets remain uninspected.

## Reproduction and limits

Verify executable identity and original PE mapping. Check original RTTI
anchors and catalog target ordering at the three table VAs above. Repeat
only the four xref queries to `0x00a3f7e0`, `0x0109a2dc`,
`0x0109a91c` and `0x0109a31c`, then obtain these bounded listings:

| Requested start VA | Instruction count | Retrieved VA envelope |
|---|---:|---|
| `0x00a6d520` | `80` | `[0x00a6d520,0x00a6d60b)` |
| `0x00a3ead0` | `60` | `[0x00a3ead0,0x00a3eb6d)` |
| `0x008dd880` | `45` | `[0x008dd880,0x008dd8fb)` |
| `0x008dd910` | `45` | `[0x008dd910,0x008dd985)` |
| `0x00c6c2d0` | `80` | `[0x00c6c2cf,0x00c6c3e5)` |
| `0x00c6c1f0` | `60` | `[0x00c6c1f0,0x00c6c2c0)` |
| `0x00a45e90` | `6` | `[0x00a45e90,0x00a45eaa)` |
| `0x00a45940` | `5` | `[0x00a45940,0x00a4594d)` |

The request at `0x00c6c2d0` retrieves the existing instruction that
starts at `0x00c6c2cf`. The envelopes are not contiguous decoding claims.
Keep gaps, incidental tails and outgoing targets uninterpreted. All
`381` retrieved instructions, covering `1116` instruction bytes, were
independently matched against the original. The frozen original read
ledger covers `2625` bytes with `2401` unique bytes, excluding whole-file
identity hashing. The ordinal query returns seven references in five
caller bodies. The table xrefs have only bounded literal corroboration,
not additional decoded-body claims. Keep raw listings and original bytes
outside the public tree.

These are static executable observations. No resource input, native
execution, live client session or rendered result is included. The
[constructor-pose finding](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/docs/resource/skeleton-constructor-pose.md#L158-L208)
owns the selected output interpretation. The
[affine inverse finding](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/docs/resource/skeleton-constructor-affine-inverse.md#L151-L155)
owns the constructor transfer into pose `[+0x40,+0x80)`. Neither supplies
runtime getter timing, contents or a general output-range copy contract.
The separate [Float4 submission finding](skeleton-output-float4-submission.md)
owns conditional submission from saved associated pointers.
Hierarchy-output `[+0x40,+0x80)` remains opaque. Native float parity
retains the
[recovery finding's limits](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/docs/resource/skeleton-quaternion-recovery.md#L174-L185).

Standalone indexed-Joint access, final-output getter behavior, null and
count safety, collision bounds, lifetime, ownership, palette ordering,
mesh indices, weights, bind and inverse-bind meaning, axes, units,
handedness, MTB channels, animation, update timing, runtime resource
selection and visual parity remain unresolved. No uninspected caller
or callee inherits the observed contracts.
