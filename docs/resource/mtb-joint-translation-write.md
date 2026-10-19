# MTB scalar curve application to Joint translation

An instruction path in the identified executable calls a curve's scalar slot
and writes its float32 result into a Joint's current translation at pose
+0x80. The narrow route joins a parsed curve group, a runtime descriptor, a
target-row pointer and a single enabled translation bit. It requires the same
curve and Joint objects at those joins. The physical writes and caller
coordinates below do not establish a selected asset's playback, scheduling or
sample units.

This route belongs to the Curve-section record of an MTB container. The
SpuBinary route of the [motion blob finding](mtb-fixed-motion-calculator.md)
fills rows built by the same constructor `0x00e08fd0` through its own
calculator and is not covered here. Both routes are called from the layer
sample caller `0x00e0b720`. Within `c001/act` only five fishing-pose containers
carry a type-2 property, as the
[container framing finding](mtb-container-framing.md#reach-of-the-tracked-curve-findings)
records, so this route does not describe the locomotion clips of the
[bank census](../actor/locomotion-bank-census.md).

## Identity and method

The input is retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9. The
word Original below means this executable.
PE32 machine 0x014c, optional-header magic 0x010b and image base
0x00400000 were checked. Addresses are VA, intervals have exclusive ends,
and ptr32 denotes a native little-endian 32-bit pointer read.

The direct Swift 6.3.3 LLVM 21.0.0 llvm-objdump executable has SHA-256
47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f.
The retained audit used Node standard-library PE mapping and COFF i386
Intel-syntax decoding. Original and actual decoder identities matched before
and after the audit. All 15 retained decoder children exited 0 and were
reaped, each with a 20-second deadline and combined 8388608-byte output
limit. All 1703 decoded instructions and 5389 encoded instruction bytes
matched the original PE mapping. Raw listings and bytes remained transient.
No target execution or selected resource payload supplied this finding.

The [scalar codec](mtb-fixed-curve.md) owns the type-2 record,
FixedCurve construction, original RTTI, scalar slot and restricted two-key
Hermite relation. Its allocator, helper-effect, data stability, native
comparison and floating-point premises remain required. The
[native memory selection](mtb-native-memory-selection.md) finding owns the
selected-memory contract. The [container framing](mtb-container-framing.md)
finding owns the Curve record header, including the count and mask words read
below. The [Joint access](skeleton-joint-access.md),
[constructor pose](skeleton-constructor-pose.md) and
[local matrix pipeline](skeleton-output-float4-submission.md#local-output-matrix-production)
own the associated Joint/pose record and hierarchy consumer. Function
catalogs and the Original function-boundary map supplied address leads.
Each new instruction claim below was checked in the identified executable.

## Parsed curve group and descriptor transfer

Let C be the runtime container pointer retained at loader receiver +0x54
by 0x00a30be5 on the single-clip construction route. Let V be the selected
Curve node and G the eight-byte runtime curve-group record. The consumer
stores a curve-pointer-array base A into G+0 at 0x00a30ca8, writes its
allocated count byte at G+4 at 0x00a30ca5, and uses G+5 as the filled
count. For each constructed curve K, 0x00a30eb3..0x00a30ebd writes
ptr32(A+4*old_byte(G+5))=K and then increments that byte. The type-2 route
therefore supplies a FixedCurve pointer at that ordinal under the scalar
codec's successful-construction and retained-pointer conditions.

At 0x00a30f11 the consumer calls 0x00a2b420 with ECX=C and arguments
0, a context cell containing V, and G. The complete helper appends an
eight-byte entry (V,G) to clip zero's property table through C+0x0c and
increments that table's dword filled count. The destination table must have
valid capacity. No capacity check occurs in this helper.

For clip ordinal q and property ordinal j, 0x00a2b5d0 selects the
8-byte clip descriptor at ptr32(C+0x0c)+8*q, then its 8-byte entry j.
On its in-range and nonzero-filled-count route it returns this handle H:

| Handle offset | Width | Value |
|---|---|---|
| +0 | 4 | ptr32(G+0), the curve pointer array A |
| +4 | 2 | zero-extended byte G+5, the filled curve count |
| +6 | 2 | native word returned by 0x00a2d970 for the entry's V |

The original container table at 0x01098b7c has slot 9 at 0x01098ba0
pointing to 0x00a2b5d0. Its caller 0x00e09257 dispatches through offset
+0x24. The mask helper reads byte ptr32(C+0x30)+0x0c bit 3. If set,
it reads the native word at V+0x0a, otherwise V+6. There is no byte swap
in this helper. This gives a header-mask location and transfer, conditional
on that context field and the original node still being the same objects.
It does not establish that any selected resource has mask 1.

The complete binding factory 0x00e09130 accepts a source pair containing
C at +0 and target-search context N at +4. Its virtual call at
0x00e09257 fills local H. Its name-lookup branch calls 0x00a3b390 at
0x00e09261. With a nonzero returned row T and nonzero H+6, it calls
0x00e0b840 at 0x00e0928c. On a normal append return R, it writes T to
R+0, H's first dword to R+4 and H's second dword to R+8, at
0x00e09291, 0x00e09297 and 0x00e0929e. Thus R is a 12-byte record:
(target-row pointer, curve-array pointer, uint16 count, uint16 mask).

The append wrapper supplies receiver U+0x18 to 0x00e0c040. After that
call it obtains the vector end at U+0x20 and returns end-12, conditional
on the updated vector and retained live route state. The allocator/vector
helper is outside this proof. The factory's other calls and its ID-lookup
branch are also unclosed. In particular, no ABI rule supplies their effects.

The own lookup body 0x00a3b390 calls each row node's slot 1 and comparator
0x009d244e. On a zero comparator result it returns ptr32(N)+16*i for the
selected word index i. Its scan can wrap to index zero. This describes the
row-return branch, conditional on comparator and getter effects, rather
than proving a general string-comparison or safe lookup API.

A bounded actual pairing block at 0x00dfcbbe..0x00dfcc09 places a live
container pointer in a local pair's first dword and a live owner pointer
plus four in its second dword, then invokes 0x00e09130 at 0x00dfcc04.
Its preceding acquisition and selection are outside this block. The block
supplies an actual pairing caller, not an unconditional loader-to-owner
identity.

## Joint target-row construction

The complete 0x00e08fd0 takes destination T in ECX and a node J as its
first stack argument. It stores J at T+0 and initially clears T+4, T+8
and T+0x0c. Its three virtual property queries are:

| Callsite | Original literal | Original slot | Output cell |
|---|---|---|---|
| 0x00e09001 | trans at 0x0109aa48, 6 bytes including NUL | 7 | T+4 |
| 0x00e09010 | quat at 0x0109aa40, 5 bytes including NUL | 6 | T+8 |
| 0x00e0901f | scale at 0x0109aa38, 6 bytes including NUL | 7 | T+0x0c |

Original Joint table 0x0109a31c has slot 6 at 0x0109a334 pointing to
0x00a46310 and slot 7 at 0x0109a338 pointing to 0x00a462b0. Let
P=ptr32(J+0x0c). With stable matching literals and DF clear, the complete
slot-7 getter writes P+0x80 for trans and P+0xc0 for scale. The complete
slot-6 getter writes P+0xa0 for quat. Those three matching query paths
have no outgoing calls. Their own save/restore sequences preserve the row
constructor's live ESI, EDI and EBX and return with eight-byte argument
cleanup. Under disjoint valid record and stack storage, the result is:

| Row offset | Pointer |
|---|---|
| T+0 | J |
| T+4 | P+0x80, current translation |
| T+8 | P+0xa0, current quaternion |
| T+0x0c | P+0xc0, current scale |

The bounded node-array loop 0x00dfc761..0x00dfc7bc actually calls this
constructor at 0x00dfc7af with node ptr32(array+4*i), destination advanced
by 16 bytes each iteration. The surrounding allocation, eventual row-array
ownership and node selection are not proved by that bounded loop. The
conditional Joint route requires its node pointer to have the audited
Joint table, rather than assigning Joint identity to every node it accepts.

## One scalar translation write and its callers

Let U enter 0x00e0a260 in ECX. Its vector at U+0x18 uses begin
ptr32(U+0x1c) and end ptr32(U+0x20). It iterates 12-byte records. For
one record R, 0x00e0a2a8..0x00e0a2b9 loads T=ptr32(R), translation
pointer D=ptr32(T+4), quaternion pointer ptr32(T+8), and scale pointer
ptr32(T+0x0c). A zero D skips the entire record.

Restrict the selected route to a valid one-record vector, word(R+0x0a)=1,
word(R+8)>=1, A=ptr32(R+4), and K=ptr32(A) the same valid FixedCurve
object supplied by the codec. At 0x00e0a2d0 the sampler loads the mask.
At 0x00e0a300..0x00e0a30b it loads K, its vtable and scalar slot 3.
The original FixedCurve slot at 0x01098c94 contains 0x00a320b0.
Callsite 0x00e0a31d supplies K in ECX and one float32 sample coordinate
on the stack. At 0x00e0a31f it stores the returned x87 ST0 to float32
at D. The mask-bit clear, remaining bit tests and loop tail take no other
curve, converter or normalization call on this mask-1 route. The count
word is not used to guard this scalar array access, so valid A[0] is an
independent premise. The record's count is descriptive, not a safe bound.

With the preceding Joint row, this store targets P+0x80, the first current
translation component. It is a float32 conversion of the evaluator's native
ST0 result, subject to ambient floating-point state. It is not an assertion
of ideal Hermite parity. The remaining translation lanes, quaternion and
scale are not written by this selected scalar path, provided its target is
separate from its local and control storage and other state.

The complete caller 0x00e0b720 checks byte U+0x39. When nonzero, it calls
0x00e0a000 at 0x00e0b731 and 0x00e09720 at 0x00e0b74f, then calls
0x00e0a260 at 0x00e0b75e. Its first scalar argument is forwarded through
x87 float32 load/store to the sampler. Those preceding routines' effects
remain unclosed, so the stated U, vector, binding and coordinate values
must hold at the final call. They are not assumed to survive by ABI.

The complete outer caller 0x00e01530 has a route selected by clear bit 0
of byte L+0x90. It iterates component pointers between L+0x24 and L+0x28.
For an enabled component it loads a float32 coordinate from L+0x14 at
0x00e0160f, and supplies that coordinate plus dword L+0x98 to
0x00e0b720 at 0x00e01632. This establishes a concrete coordinate-field
caller, conditional on normal iteration and preserved live values. It
supplies no seconds/frame interpretation, rate, duration or repeat policy.
Its flag-set alternate route and 0x00e0b7d0 are outside the selected path.

## Conditional hierarchy consumption and surviving gaps

For composition, require the same P that the Joint exposes to be the pose
receiver later used by 0x00a3f960, with the sampled field unchanged until
that read. The published local matrix helper at 0x00a41ca0 reads current
translation for its TRS regeneration route and installs it into the local
matrix's last row. Require byte P+0xf2=0 so it does not take the alternate
stored-matrix copy route. The published hierarchy calculation then consumes
that local translation through the local-matrix call at 0x00a3f980.
The sampler's selected own path does not clear P+0xf2 or establish when
hierarchy recalculation is scheduled.

All stages require valid nonwrapping readable and writable spans, stable
node/container/curve/row/vector identities, stable dispatch slots and
executing instructions, and normal returns with the live registers, stack
and fields required by each later observation. No shared ABI or allocator
contract supplies those conditions. In particular, the scalar call must
retain the sampler's live EBP destination, EDI record, EBX mask, local
fields, saved registers and control words, and return with its four-byte
argument removed and a valid ST0 result. The restricted published
FixedCurve numerical route and all its constant/data separation and
nontrapping floating-point premises remain required.

Every construction, context, vector, row, pose, key-array, local and output
write must be disjoint from executing instructions, stable literals,
dispatch data and protected codec constants. Distinct writable/control and
read-only input spans must not alias in ways that change the stated later
reads or return route. This is an explicit stability and separation premise,
not inferred runtime memory protection. The complete query bodies establish
only the matching property paths. Unmatched query behavior is not widened.

This supports the conditional first-component translation assignment for a
FixedCurve and a bound Joint. Actual loaded MTB identity, resource validity,
name matching, row lifetime, container acquisition, prior-call physical
preservation, dirty-state protocol and animation-to-hierarchy scheduling
remain open. No selected clip execution, complete TRS playback, root motion,
axis convention, default frame rate, deformation palette or shader mapping
follows from this path.

## Reproduction and coverage

Verify the original identity and actual direct decoder above. Decode each
listed interval with encoded bytes enabled, and compare every encoded byte
through the original PE section table. The installed direct decoder is
under the Swift 6.3.3 toolchain's usr/bin directory. For each row use:

```sh
"$decoder" -d --x86-asm-syntax=intel --start-address="$start" --stop-address="$stop" "$identified_executable"
```

Use a finite 20-second child deadline, combined 8388608-byte output bound,
and wait for actual child exit/reaping. Complete own-body coverage does not
mean transitive closure of its outgoing calls. The last two rows are bounded
blocks, not complete-function or caller-wide proofs.

| Start VA | Exclusive stop | Instructions | Bytes | Coverage |
|---|---|---|---|---|
| `0x00a30910` | `0x00a313e9` | 811 | 2777 | complete parsed-curve consumer |
| `0x00a2b420` | `0x00a2b44b` | 14 | 43 | complete group append |
| `0x00a2b5d0` | `0x00a2b652` | 51 | 130 | complete 8-byte descriptor getter |
| `0x00a2d970` | `0x00a2d990` | 10 | 32 | complete native header-mask getter |
| `0x00e09130` | `0x00e09325` | 178 | 501 | complete binding factory own body |
| `0x00a3b390` | `0x00a3b43b` | 63 | 171 | complete row lookup own body |
| `0x00e0b840` | `0x00e0b8a4` | 36 | 100 | complete record append wrapper own body |
| `0x00e08fd0` | `0x00e09027` | 33 | 87 | complete target-row constructor |
| `0x00a462b0` | `0x00a4630a` | 35 | 90 | complete Joint slot7 pointer getter |
| `0x00a46310` | `0x00a4633d` | 17 | 45 | complete Joint slot6 pointer getter |
| `0x00e0a260` | `0x00e0a5dc` | 276 | 892 | complete sampler own body |
| `0x00e0b720` | `0x00e0b767` | 25 | 71 | complete sampler caller own body |
| `0x00e01530` | `0x00e0164c` | 99 | 284 | complete layer sample caller own body |
| `0x00dfc761` | `0x00dfc7bc` | 35 | 91 | bounded node-array row-construction loop |
| `0x00dfcbbe` | `0x00dfcc09` | 20 | 75 | bounded container and target-table pairing block |
