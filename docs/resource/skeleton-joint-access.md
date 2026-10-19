# Bounded skeleton Joint association and name lookup

The retail constructor associates each construction ordinal with one Joint,
one pose record and one hierarchy-output record. The identified name lookup
returns an unsigned 16-bit ordinal on exact byte-string equality and
`0xffffffff` on its observed miss paths. An indexed-Joint accessor and a
runtime final-output getter remain unresolved. The constructor matrix
transfer is established by the [affine inverse finding](skeleton-constructor-affine-inverse.md).

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 machine is `0x014c`, with image base VA `0x00400000`.
All native addresses below are VA. Subtract `0x00400000` for RVA.

Producing tools are `ghidra` CLI `0.2.2`, the existing analyzed
`ffxivgame.exe` program, and Python `3.14.7` standard-library PE reads.
The [constructor-pose finding](skeleton-constructor-pose.md) records Ghidra
`12.1.4 PUBLIC`, JDK `26.0.2.1` and language `x86/little/32/default`.
Those prior observations are provenance, without a fresh version or language
assertion here. Bounded `ghidra disasm <VA> -n <count> --json` listings
were independently compared against the original executable through its
original PE section table. Original PE reads independently checked the
RTTI anchors and table targets. Size and SHA-256 matched before and after.
No decompiler-inferred signature supplies a field contract.

The original table at VA `0x0109a31c` has complete-object locator
`0x011878a4`, type descriptor `0x012eef1c`, and mangled type
`.?AVJoint@Detail@SceneGraph@Dw@Engine@CDev@SQEX@@`.
This anchors `SQEX::CDev::Engine::Dw::SceneGraph::Detail::Joint`.
Original RTTI also corroborates the existing `CharacterSkeletonNode`
anchor at table VA `0x0109a2dc` and `SkeletonResourceNode` anchor at
`0x0109a91c`. These identities do not establish a complete object layout.

The committed
[RTTI catalog](https://github.com/XIVLegacy/xivl-decomp/blob/087be26a5bea9b0260092e1cef9dd3dd01a21da8/config/ffxivgame.rtti.json),
[vtable slot catalog](https://github.com/XIVLegacy/xivl-decomp/blob/087be26a5bea9b0260092e1cef9dd3dd01a21da8/config/ffxivgame.vtable_slots.jsonl)
and
[client vtable index](https://github.com/XIVLegacy/xivl-client-structs/blob/08de0f7428a133a689627ac0cbf60d97daed2a71/manifests/rtti_vftable_index.json)
provide identity and slot-order leads. The last index's `vftableVAs`
contains those three table VAs. The Joint slot catalog records 13 slots.
The original executable confirms all 13 targets in that ordering.
Catalog agreement alone does not prove the behavior below.

## Constructor association

Let `S` denote the scoped skeleton receiver, `i` its zero-based global
construction ordinal, and `ptr32` a native 32-bit pointer read.
These are observed address equations, rather than an accessor API:

| Address | Constructor equation |
|---|---|
| `J_i` | `ptr32(S+0x5c) + 0x1c*i` |
| `P_i` | `ptr32(S+0x40) + 0x100*i` |
| `O_i` | `ptr32(S+0x44) + 0xa0*i` |
| Joint pose association | `ptr32(J_i+0x0c) = P_i` |
| Joint output association | `ptr32(J_i+0x10) = O_i` |

VA `0x00a40c6b..0x00a40cbd` calculates these offsets from the same
ordinal and performs the two pointer writes. The constructor initializes
the ordinal at `0x00a40c28..0x00a40c2f`, advances it at
`0x00a4126d..0x00a41270`, and advances the three offsets by `0x1c`,
`0xa0` and `0x100` at `0x00a41240..0x00a4124d`.
It retains that ordinal across its outer container loop. Confidence is
direct instruction observation within this constructor.

VA `0x00a40597` zero-extends the skeleton's word at `+0x50`.
The allocation path at `0x00a405ba..0x00a40633` uses that count,
`0x1c`-byte elements, an array header, constructor target `0x00a41eb0`
and exposed Joint base `S+0x5c`. VA `0x00a41eb0..0x00a41ec6` writes
the Joint vtable, clears the low ten bits of `+0x14`, and clears pointer
`+0x18`. These stores are not a complete default-state contract.

The pose and output allocation path stores bases at `S+0x40` and
`S+0x44` at `0x00a406eb` and `0x00a40734`. Instructions
`0x00a407c5..0x00a407cc` advance the exposed bases by one pose record
and one output record. The prior constructor-pose finding owns the
preceding output record's identity semantics and the parent-index relation.

VA `0x00a40b20..0x00a40b39` accumulates container counts in a dword
but stores the low word to `S+0x50`. This finding establishes that
16-bit storage width, without a general overflow, maximum-count,
allocation-failure or arbitrary-skeleton validation contract.

## Primary string and ordinal table

The [record-framing finding](skeleton-bone-framing.md) owns the serialized
primary-name index and its container getter interpretation. VA
`0x00a40e45..0x00a40e5f` stores the destination pointer at `J_i+0x04`
and copies the primary string through its NUL terminator. The destination
comes from constructor-allocated memory at `0x00a40beb..0x00a40c1c`.
No string contents or encoding meaning are supplied by this finding.

VA `0x00a4120e..0x00a41263` builds one four-byte entry per global
ordinal in the table at `S+0x60`. Starting with `h=0`, each nonzero byte
`b` is processed in this order with 32-bit integer operations:

```text
h = h XOR sign_extend_8_to_32(b)
h = h XOR (h << 7)
```

The entry's word at `+0` receives `h >> 16`, with a logical right shift.
Its word at `+2` receives the low 16 bits of `i`. At
`0x00a41409..0x00a4141e`, the constructor passes the table, its
zero-extended word count, entry size `4` and comparator address
`0x00a3f660` to `0x009d5de0`. The comparator and called implementation
are outside the inspected bodies. Their ordering contract is unresolved.

VA `0x00a3f7e0..0x00a3f8ec` accepts a byte-string pointer through one
stack argument and reads receiver fields `+0x50`, `+0x60` and `+0x5c`.
It computes the same signed-byte hash, searches entry hash words, rewinds
equal-hash candidates, and zero-extends each candidate's ordinal word at
entry `+2`. Instructions `0x00a3f880..0x00a3f891` select
`ptr32(J_ordinal+0x04)` using the `0x1c` Joint stride.
Instructions `0x00a3f895..0x00a3f8c7` compare the input and stored
strings byte-for-byte through NUL. The equality return at
`0x00a3f8e2..0x00a3f8e9` places that zero-extended ordinal in EAX.
Observed miss paths at `0x00a3f8d7..0x00a3f8df` return `0xffffffff`.
This is an ordinal return, not a Joint pointer return.

The midpoint entry is read before a zero-count guard is established.
Collision continuation has no independently established count boundary.
Do not infer null safety, bounds safety, case folding, Unicode comparison,
or a malformed-input rejection rule. High-bit bytes use signed extension
for hashing and exact byte comparison for equality. Confidence is direct
instruction observation, with those safety and ordering gaps retained.

## Saved pose getters and output distinction

Slot numbers are zero-based in the original Joint table.

| Slot | Target VA | Direct return |
|---|---|---|
| `1` | `0x008d5570` | `ptr32(J+0x04)`, the associated primary-string pointer |
| `2` | `0x007143e0` | `ptr32(J+0x08)`, an auxiliary-string pointer or zero on the constructor path |
| `9` | `0x00a41ed0` | `ptr32(J+0x0c) + 0x90` |
| `10` | `0x00a41ee0` | `ptr32(J+0x0c) + 0xb0` |
| `11` | `0x00a41ef0` | `ptr32(J+0x0c) + 0xd0` |
| `12` | `0x00a41f00` | Dword `J+0x14` masked with `0x3ff` |

The prior constructor-pose finding establishes saved translation,
quaternion and scale groups at pose `+0x90`, `+0xb0` and `+0xd0`.
Thus slots `9..11` expose saved pose groups. They do not expose the
hierarchy output associated at `J+0x10`. Slot `12` has no established
semantic meaning here and must not be called the construction ordinal.
The constructor writes or clears the auxiliary pointer at
`0x00a40e8a..0x00a40ed5`. Its semantic purpose remains unresolved.

Slots `3` and `4` at `0x00a45f70` and `0x00a45fe0` delegate through
`J+0x18` and handle pose byte `+0xf3`. The delegated targets remain
uninspected. Slots `6..8` at `0x00a46310`, `0x00a462b0` and
`0x00a46050` select current pose groups or components on fixed comparison
paths. Slot `5` at `0x00a46340` has a distinct fixed comparison path.
Some unmatched paths return success without establishing a destination
write. These observations do not provide a safe generic property-query
contract or a final-output getter. The
[MTB translation write finding](mtb-joint-translation-write.md#joint-target-row-construction)
owns the matching slot 6 and 7 property literals and their target-row consumer.

## Hierarchy output and remaining access gaps

The constructor-pose finding owns the first matrix at output `+0x00`,
inherited four-lane scale at `+0x80`, and quaternion at `+0x90`.
Fresh disassembly corroborates matrix stores at
`0x00a3feb6`, `0x00a3feb9`, `0x00a3feda` and `0x00a3fee7`, scale
store `0x00a3fd9a`, and quaternion stores `0x00a3fdb6` and
`0x00a3fde5`. Retain its flat notation `M[i,j] = flat[4*i+j]`.
The additional output range `[+0x40,+0x80)` remains semantically opaque.
No palette or inverse-bind interpretation follows from its stores.

VA `0x00a41300..0x00a4134d` reads `J+0x10` into ECX, supplies a stack
destination to direct callee `0x006522d0`, and transfers four returned
16-byte blocks into pose `[+0x40,+0x80)`. The [affine inverse finding](skeleton-constructor-affine-inverse.md)
establishes the callee's arithmetic and this constructor transfer. Runtime
final-output getter access remains unresolved.

The bounded walk `[0x00a3f770,0x00a3f79e)` iterates the Joint array in
`0x1c` steps and calls `0x00a45f30`. The prior constructor-pose finding
owns the target's reset semantics. It is not an indexed accessor.
The partial range `[0x00a3f6b0,0x00a3f756)` backs up pose and output
bases by their prefix strides and reaches cleanup paths, including Joint
slot `0` dispatch. It ends before branch target `0x00a3f75f`, so complete
function behavior is not established. Neither range supplies final-output
read access. No indexed-Joint return or runtime final-output getter is
established by these walks. The association
equations do not authorize calling a direct memory read a retail getter.

The [bounded access finding](skeleton-joint-output-access.md) owns an indexed
primary-string return and an in-body associated output-pointer consumer.
Neither establishes a standalone indexed-Joint API or runtime matrix getter.

## Reproduction and limits

Verify the executable identity, PE32 machine and image base. Resolve the
named RTTI and table addresses through the original section table and
check the Joint target ordering against the committed catalog. Obtain
bounded listings at the following starts and instruction counts. Overlaps
and incidental tails must not be interpreted as additional functions.

| Start VA | Instruction count |
|---|---:|
| `0x00a40560` | `180` |
| `0x00a40990` | `90` |
| `0x00a3f960` | `300` |
| `0x00a40af0` | `635` |
| `0x00a41300` | `80` |
| `0x00a3f7e0` | `111` |
| `0x00a41eb0` | `10` |
| `0x00a41ed0` | `4` |
| `0x00a41ee0` | `3` |
| `0x00a41ef0` | `3` |
| `0x00a41f00` | `3` |
| `0x008d5570` | `2` |
| `0x007143e0` | `2` |
| `0x00a45f70` | `42` |
| `0x00a45fe0` | `40` |
| `0x00a46050` | `214` |
| `0x00a462b0` | `35` |
| `0x00a46310` | `19` |
| `0x00a46340` | `16` |
| `0x00a3f770` | `21` |
| `0x00a3f6b0` | `60` |

The last range has gaps in the existing instruction listing. Its 60
instructions are confined to `[0x00a3f6b0,0x00a3f756)`. Do not fill
gaps or follow its outgoing branch. Compare every retrieved instruction's
bytes independently against the original file mapping. Check pointer
widths, word count and ordinal widths, stride arithmetic, hash operation
order, equality branches and return widths. Recheck size and SHA-256
without exporting original bytes or raw listings.

These are static executable observations. No resource input, native
execution, live client session or rendered result is included. Portable
floating-point parity retains the
[quaternion-recovery finding's](skeleton-quaternion-recovery.md) unresolved
CPU-estimate and environment constraints. Mesh palettes, mesh-index mapping,
weights, bind or inverse-bind meaning, axes, handedness, units, shader
conventions, selected animation input, runtime selection and visual parity remain
unresolved. Constructor association and saved-pose pointers do not close
those gaps.
