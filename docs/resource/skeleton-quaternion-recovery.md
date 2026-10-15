# Bounded SKL constructor quaternion recovery

The native helper at VA `0x00a419a0` recovers four quaternion lanes from the
upper-left 3 by 3 lanes of its matrix input. Its ordered SIMD comparisons,
candidate equations and reciprocal-square-root refinement are established.
Portable bit-for-bit floating-point recovery remains unresolved.

## Identity and scope

The executable is retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is VA `0x00400000`. Every address below is VA.
Subtract `0x00400000` for RVA.

The fixed
[constructor-pose finding](https://github.com/XIVLegacy/xivl-decomp/blob/7e89227155fbd84386906de13bd3c7b1a5e3a78b/docs/resource/skeleton-constructor-pose.md)
establishes the selected constructor's local matrix, hierarchy calculation
and quaternion composition. This finding narrows only that calculation's
recovery gap. It adds no resource input or selector coverage.

Consumer identity remains anchored by the committed
[RTTI catalog](https://github.com/XIVLegacy/xivl-decomp/blob/3f5133e7ac93da20fa1ae0c7b11a444bdcfaec26/config/ffxivgame.rtti.json)
and
[vtable slot catalog](https://github.com/XIVLegacy/xivl-decomp/blob/3f5133e7ac93da20fa1ae0c7b11a444bdcfaec26/config/ffxivgame.vtable_slots.jsonl).
The RTTI entries identify `SkeletonResourceNode` at vtable RVA `0x00c9a91c`,
`CharacterSkeletonNode` at `0x00c9a2dc`, and `HierarchyDataContainer` at
`0x00d3bd64` within the namespaces recorded in those catalogs. The slot
catalog records the last table's slot `9` at function RVA `0x00a11c30`.
These identity anchors do not expand the recovery helper's semantics.

Producing tools are `ghidra` CLI `0.2.2`, its existing analyzed
`ffxivgame.exe` program with language `x86/little/32/default`, and Python
standard-library PE reads and independent symbolic lane analysis. The
constructor-pose finding records Ghidra `12.1.4 PUBLIC` and JDK `26.0.2.1`.
Those prior version observations are retained without a fresh installation
or independent version assertion here. Fresh bounded disassembly matched
the original executable's instruction bytes through its PE section table.
No decompiler-inferred type or name supplies an arithmetic contract.

| Purpose | VA half-open extent | Instruction count |
|---|---|---:|
| Recovery helper | `[0x00a419a0,0x00a41c45)` | 166 |
| Hierarchy callsite context | `[0x00a3f960,0x00a3fa50)` | 46 |
| First quaternion converter | `[0x007d8fe0,0x007d90c8)` | 70 |
| Second quaternion converter | `[0x0042ffa0,0x00430342)` | 188 |

At `0x00a3f9fb..0x00a3fa22`, the hierarchy caller passes a local matrix
copy through the first stack argument and an output destination through
ECX. The helper retains that destination in EAX at `0x00a419a9` and reads
the matrix pointer at `0x00a419ab`. Stores at `0x00a41c30` and
`0x00a41c3a` write the four returned lanes. The return at `0x00a41c42`
removes one pointer argument. The caller passes the returned output pointer
to the established quaternion composition at `0x00a3fa2a..0x00a3fa39`.
The prior finding owns that composition's operand order and constructor
scope.

## Own-body physical register and stack contract

The physical observation covers the complete helper
`[0x00a419a0,0x00a41c45)`: `677` bytes and `166` instructions.
The producing instrument is LLVM `llvm-objdump` from the Swift `6.3.3`
toolchain, SHA-256
`47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f`,
with `--disassemble --start-address=0x00a419a0 --stop-address=0x00a41c45`.
Python `3.14.7` PE mapping and instruction-byte comparison independently
matched the complete body to the identified executable. The body has no
calls, branches, string instructions or exceptional transfers. Its only
control transfer is the final normal near `RET` at `0x00a41c42`, with
four-byte immediate cleanup.

The following deductions assume ordinary flat 32-bit addressing without
wrap, uninterrupted execution of the verified stable body, valid accesses,
and no fault or asynchronous register or memory interference. Let `T` be
entry ESP, `A` be `T-4` rounded down to a multiple of `16`,
`F=A-0x60`, and `O` be entry ECX retained in physical EAX. All memory
ranges below are half-open byte ranges.

The complete body has no explicit reference, assignment, partial register
write or implicit effect on EDI. Physical EDI retains its entry value
within this body under those assumptions. This is not an ABI or caller
save-slot inference.

The initial save at `0x00a419a0` writes entry EBP to `[T-4,T)`.
At `0x00a419a1`, physical EBP becomes `T-4` and retains that value
until `0x00a41c41` reloads the actual current bytes of that saved slot.
Entry EBP is recovered if the saved bytes are preserved. Disjoint
output `[O,O+16)` and saved slot `[T-4,T)`, together with external
memory stability, suffice but are not necessary for that equality.

ESP becomes `T-4` after the save, `A` after alignment at
`0x00a419a3`, and `F` after allocation at `0x00a419a6`. It stays
at `F` until `0x00a41c3f` restores `T-4` from the still-held
frame EBP. The pop at `0x00a41c41` advances ESP to `T`. The normal
near return reads its actual target from `[T,T+4)`, advances ESP by
four and adds the immediate four, leaving `ESP=T+8`. This final ESP
relation holds even if the popped EBP bytes changed, provided the verified
epilogue executes normally. Return to the entry return target separately
requires stability of `[T,T+4)`.

All `29` explicit local stores lie within `[F,F+0x60)`, below and
separate from the saved EBP slot under nonwrapping arithmetic. The two
eight-byte output writes at `0x00a41c30` and `0x00a41c3a` target
`[O,O+8)` and `[O+8,O+16)`. Output can overlap the saved EBP slot
or the return target. The first output write also precedes the second
source read from `[F+8,F+16)` at `0x00a41c34`, so output/local
aliasing can change the second output half. Balanced save and pop do not
prove alias independence. No input/output/caller alias relationship or
whole-caller register preservation follows from this own-body contract.

## Ordered radicands and refinement

Retain the flat notation `M[i,j] = flat[4*i+j]`. It does not assign axes,
units or handedness. The helper reads matrix rows `0..2` at
`0x00a419ae..0x00a41a36`. Their fourth lanes do not affect the returned
quaternion. It does not read row `3`.

Let `a=M[0,0]`, `b=M[1,1]` and `c=M[2,2]`. The shuffles at
`0x00a41a3a..0x00a41a56` gather `(a,b,c,a)`, `(b,c,a,b)` and
`(c,a,b,c)`. The arithmetic at `0x00a41a5d..0x00a41a82` establishes
the following native operation order. Each `add`, `sub` and `mul` denotes
one SSE float32 instruction. Regrouping or fused arithmetic is not part
of this order.

```text
t  = add(add(b,a),c)
Dx = add(sub(sub(a,b),c),1)
Dy = add(sub(sub(b,c),a),1)
Dz = add(sub(sub(c,a),b),1)
Dw = add(t,1)
```

The constant `1` is fixed native float32 data at `0x00f54f70`.
RSQRTPS at `0x00a41a85` evaluates all four radicands and supplies one
estimate `r_i` for each lane. The bytes do not establish a portable
algorithm for the estimate's bits. With fixed native float32 `0.5` at
`0x00f62f60`, the instructions at `0x00a41aef..0x00a41b0a` perform:

```text
p   = mul(r_i,D_i)
p   = mul(p,r_i)
e   = sub(1,p)
v   = mul(0.5,r_i)
e   = mul(e,v)
e   = add(e,r_i)
h_i = mul(e,0.5)
```

Over exact mathematical scalars, `h_i = r_i*(3-D_i*r_i*r_i)/4`.
Thus `2*h_i` is one Newton correction of the reciprocal-square-root
estimate. This radicand refinement is not an observed quaternion length
normalization pass. The ideal relation `h_i = 1/(2*sqrt(D_i))` assumes an
exact reciprocal square root and does not replace the native algorithm.

## Candidate lanes and selection

The gathers and arithmetic at `0x00a41a8d..0x00a41acd` supply the
off-diagonal numerators. Here `Mij` abbreviates `M[i,j]`. This table gives
exact-scalar lane equations. Each numerator sum or difference is computed
by a separate native instruction before its candidate is multiplied by
the corresponding refined lane.

| Candidate | Four lanes | Scaling locator |
|---|---|---|
| x | `h_x*(Dx, M01+M10, M02+M20, M12-M21)` | `0x00a41b74..0x00a41b87` |
| y | `h_y*(M01+M10, Dy, M12+M21, M20-M02)` | `0x00a41b97..0x00a41ba8` |
| z | `h_z*(M02+M20, M12+M21, Dz, M01-M10)` | `0x00a41bb7..0x00a41bc8` |
| w | `h_w*(M12-M21, M20-M02, M01-M10, Dw)` | `0x00a41bd9..0x00a41be3` |

Ordered CMPLTPS instructions test `a<c` at `0x00a41b46`, `a<b` at
`0x00a41b4e`, `b<c` at `0x00a41b52`, and `0<t` at `0x00a41b60`.
The bitwise masks at `0x00a41b94..0x00a41bf4` select exactly one
candidate according to these predicates:

```text
if ordered(0 < t): select w
else if ordered(a < c) and ordered(b < c): select z
else if ordered(a < b): select y
else: select x
```

For finite diagonal lanes and finite computed trace, positive trace selects
w. Trace zero uses the diagonal path. On that path the largest diagonal
selects its candidate, with ties resolved x before y before z. The trace
is the ordered native computation above. Reassociating its additions can
change selection near zero.

All four radicand lanes and all four candidates are evaluated before final
selection. No four-component norm calculation or common division follows
the selection. Selected-only evaluation, output normalization and an
exact-sqrt replacement are not established native equivalents.

## Mathematical sign relation

The two bounded quaternion converters independently reproduce the
complete `R(q)` polynomials in the constructor-pose finding. For an ideal
unit quaternion `q=(x,y,z,w)` and exact-scalar `M=R(q)`, each
`D_i=4*q_i*q_i` and candidate numerator vector is `4*q_i*q`.
Using ideal positive `1/sqrt(D_i)`, the selected result is
`sign(q_i)*q` for the selected nonzero component.

This relation gives a positive selected component in the ideal unit-input
case. It does not require `w>=0` on every branch and does not add a unit
assumption to the serialized quaternion contract. It does not establish a
serialized-quaternion-copy shortcut. Native conversion, comparison,
estimate, refinement and instruction rounding remain part of the path.

## Reproduction and limits

Verify the executable size, SHA-256, PE32 machine type and image base before
inspection. Map each bounded VA range through the original PE section
table and compare every disassembled instruction's bytes. Obtain the four
bounded listings with `ghidra disasm <VA> -n <instruction count> --json`
using the starts and counts in the scope table. Read the fixed constants
through the same mapping. Keep the executable, listings and analysis state
outside the public tree.

Independently interpret MOVSS register and memory semantics, SHUFPS and
unpack lane selectors, ordered arithmetic, comparison predicates and the
bitwise masks. Check the exact operation trees for trace, radicands and
refinement separately from scalar polynomial identities. Check each
candidate vector, both converter polynomials and the unit-input sign
identity. The diagonal selection was checked for all `343` integer triples
in `[-3,3]`, covering strict orders, equal maxima and trace zero. These
synthetic symbolic checks do not execute the native instructions.

The instruction algorithm and ordered selection are established within
these bounds. Portable bit parity remains unresolved because the executing
CPU's RSQRTPS estimate and MXCSR rounding, exception and denormal state
were not observed. NaN payload, exception and signed-zero parity are not
established. Native instruction order alone does not close those gaps.

No resource values or bytes were inspected for this recovery finding. It
does not expand constructor selector coverage, establish a general
normalization rule, authored bind pose, inverse-bind meaning, world axes,
handedness, units, skinning, MTB semantics, animation or runtime resource
selection. No native execution, live client session, rendered result or
visual parity was established.
