# Bounded MTB FixedCurve scalar sampling

The selected-memory consumer at 0x00a30910 has a type-2 curve branch that
loads four float32 lanes per key into a FixedCurve. Its scalar evaluator
clamps to endpoint values and, for two finite ordered keys, invokes a cubic
Hermite interpolation helper. This establishes one small native curve codec
and scalar sample relation. A translation, rotation or scale assignment to
the state consumed by hierarchy routine 0x00a3f960 remains unconnected.

## Identity and method

Original: retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
The original PE32 machine is 0x014c, optional-header magic 0x010b and image
base 0x00400000. All addresses are VA. Subtract the base for RVA and map
through the original PE section table.

Python 3.14.7 standard-library PE reads and direct LLVM llvm-objdump 21.0.0
from the Swift 6.3.3 toolchain produced the observation, using COFF i386 and
Intel syntax. The direct decoder executable SHA-256 is
47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f.
Original and actual decoder identities matched before and after both audit
batches. Eleven bounded decoder children exited 0 and were reaped. Each had
a finite 20-second timeout and combined 8388608-byte output cap. All 1196
instructions and 3947 instruction bytes matched the original PE mapping.
Raw listings and instruction bytes were transient only. No target execution
or resource payload was observed.

The tracked [mtb-native-memory-selection.md](mtb-native-memory-selection.md) remains the canonical upstream
selection contract. Its conditional file-base and helper-effect limitations
remain. The tracked vtable catalog provided FixedCurve as an identity lead.
The Original RTTI and original table slot were independently read below.
No source name or inferred type supplies the codec arithmetic.

## Selected-memory cursor route

The complete own body [0x00a30910,0x00a313e9) includes the previously
published consumer and its normal RET 4 epilogue. At 0x00a30c30 it invokes
0x00a2d510 with literal 0x0109942c, whose original NUL-terminated text
is Curve. Let V denote that returned pointer and W the separate pointer
loaded by the consumer at 0x00a30c44. This does not identify an actual
resource row or establish V's extent.

The consumer supplies flag zero and V to 0x00a2da40 at 0x00a30c3f.
The bounded false-flag route writes V into its supplied context's first
dword at 0x00a2da51 and returns through 0x00a2de63..0x00a2de67.
That selected route has no outgoing call. Its large true-flag path remains
outside this finding.

The complete helper 0x00a2d840 reads that context pointer and the bit-3
predicate in byte W+0x0c. It returns V+8 when the bit is set and V+4
otherwise. The consumer adds four at 0x00a30cb9, producing cursor
V+12 or V+8 respectively. Address arithmetic alone establishes no valid
file span or semantic header flag.

At 0x00a30ce3 the consumer calls property selector 0x00a30110 before
reading a type byte. For its bit-0-set path, the bounded selector clears
that bit and returns one of two addresses from the stated global string
storage route. It does not advance the record cursor. Those runtime string
bytes were not read, so no property name or translation/rotation/scale
meaning follows. On the first loop iteration taking this property path,
the type pointer is therefore the cursor above, conditional on the helpers
and intervening allocations preserving the live pointers and context.
Other property-name paths can consume cursor bytes and remain unresolved.

The complete byte reader 0x00a2d810 captures the supplied cursor, advances
it by one and returns its original byte in AL. At 0x00a30cf8 the consumer
calls that reader, zero-extends AL and dispatches through the table at
0x00a313ec. Its entry for byte value 2 is at 0x00a313f4 and contains
0x00a30d45. That branch calls 0x00a2df10 at 0x00a30d54. Let R denote
the cursor after the type byte. The type-2 codec starts at R, not at
the preceding type byte.

## Type-2 key record and runtime layout

The complete loader [0x00a2df10,0x00a2dfed) captures R from its supplied
cursor pointer at 0x00a2df36. On the successful construction route, it
reads n=LE16(R+1) at 0x00a2df58 and supplies that count to constructor
0x00a2e590. Byte R+0 is not interpreted by this loader. It sets its source
pointer to R+3 at 0x00a2df79.

For each of n entries, 0x00a2df80..0x00a2dfb9 reads four consecutive
native little-endian float32 values, advancing four bytes after each read.
It calls 0x00a32070 with entry ordinal and the four-lane local tuple.
At 0x00a2dfd4 the loader stores the final cursor R+3+16*n back through
the supplied cursor pointer. At 0x00a2dfd6 it stores the constructed curve
pointer through its supplied output cell. For n=2, the codec consumes 35
bytes after its type byte, or 36 bytes including that byte. These are
conditional reads and stores, not proof that those source bytes exist.

The complete constructor [0x00a2e590,0x00a2e649) writes table pointer
0x01098c88 at the curve object's +0, the supplied property-name pointer
at +4 and count n as a native dword at +0x0c. Its allocation route retains
the key-array pointer at +8. Pool acquisition, allocation, construction
and outgoing helper preservation are not fully closed by this finding.
A successful valid object with these retained fields is an explicit premise.
The loader does not establish safe allocation-failure behavior.

The complete indexed writer [0x00a32070,0x00a320ab) compares its unsigned
index against curve+0x0c, then uses ptr32(curve+8)+16*index. Its four
x87 float32 load/store pairs copy the supplied lanes in order into offsets
0,4,8,12. It returns with eight-byte argument cleanup. Native NaN payload
or exceptional floating-point behavior is not established.

Original RTTI identifies this table as
.?AVFixedCurve@Animation@Dw@Engine@CDev@SQEX@@. The table's preceding
COL pointer is at 0x01098c84 and names COL 0x01186c8c, whose
TypeDescriptor is 0x012ee2ac. The original dword at 0x01098c94 selects
0x00a320b0 for slot 3. This joins the loader's constructed runtime type
to a scalar evaluator. It does not prove an actual caller invokes that slot.

## Restricted scalar sample relation

The complete evaluator [0x00a320b0,0x00a3223a) reads the dword count at
curve+0x0c and the key array at curve+8. Its first stack argument is a
float32 scalar x. For an empty count it returns x87 zero. For finite
nonempty ordered keys whose native comparison conversions preserve the stated
order, the entry and endpoint comparisons return the first
key's lane 1 when x<=first lane 0, or the last key's lane 1 when
x>=last lane 0. The observed return removes four argument bytes.
Unordered comparisons are not assigned a malformed-input contract here.

Restrict the interior relation to exactly two keys with finite increasing
lane-0 values t0<t1 and finite t0<x<t1, with native comparisons reflecting
those inequalities. This comparison-path premise depends on execution state,
rather than following from finite serialized inputs alone. The observed search reaches
0x00a321c4, supplies the first key and second key addresses, and calls
0x00a31f70 at 0x00a3222d with scalar x. No general key-count search or
malformed ordering contract is required for this restricted relation.

Let key i have four stored lanes (ti,vi,ai,bi). The complete call-free
helper [0x00a31f70,0x00a3203b) reads lane 0 and lane 1 of both keys,
lane 3 of the left key and lane 2 of the right key. Its exact-scalar
interpretation is:

```text
u = (x-t0)/(t1-t0)
y = v0 + (3*u*u-2*u*u*u)*(v1-v0)
    + (t1-t0)*((u*u*u-2*u*u+u)*b0 + (u*u*u-u*u)*a1)
```

This equation is the ideal real-arithmetic interpretation of the instruction
sequence, not a claim that its native finite-precision result equals the ideal
value. In exact-scalar mathematics, this is cubic Hermite interpolation. b0 is
the left endpoint's outgoing derivative coefficient and a1 is the right
endpoint's incoming derivative coefficient. These mathematical roles do not
supply a channel name, authored unit or clip timing convention.

Subject to normal floating-point execution and the ambient control state,
the native helper first computes d=float32(x-t0), h=float32(t1-t0),
u=float32(d/h) and v=float32(u-1), using double SSE intermediate
operations with explicit float32 round trips. It then combines the endpoint
values with the equivalent expression (2*v-1)*u*u*(v0-v1)+v0 and a term
d*v*(b0*v+a1*u), using x87 operations. The original float64 constant at
0x00f598a0 is exactly 1.0. Its final result is stored to float32, reloaded
to x87 ST0, and returned with 12-byte argument cleanup. This records the
scalar relationship and round-trip boundaries. It does not establish native
float parity, MXCSR/x87 state, NaN, signed-zero or exceptional behavior.
Finite input coordinates and values alone do not guarantee finite native
intermediates or output. For example, the float32 interval difference can
overflow for t0=-FLT_MAX and t1=FLT_MAX. No audited instruction establishes
MXCSR or x87 control state.

## Conditions and remaining hierarchy gap

The cursor and codec relations require valid nonwrapping input spans,
separate local/output storage and stable source bytes. Pool, allocator,
constructor and other outgoing calls must preserve the live pointer,
register, stack and field values used by the quoted later instructions.
These are explicit conditional own-body interpretations, not an ABI-wide
preservation assertion or successful resource-load observation.
The dispatch entry at 0x00a313f4 and runtime vtable slot at 0x01098c94
must retain their audited values. Constructed records, arrays and control
storage must not alias the input, indirect target data or executing code.
The full float64 constant range [0x00f598a0,0x00f598a8) must remain
immutable at the audited value 1.0 through both helper reads at 0x00a31fc5
and 0x00a31ff7. Every writable cursor, context, construction/object, key-array,
local, control and output span must be disjoint from this protected data range.
No runtime memory protection is inferred to supply those conditions.
The restricted ideal-scalar relation also requires the stated two-key count,
finite values, ordered coordinates and the described native comparison path.
All numerical pipeline and return observations require normal nontrapping
floating-point execution and sufficient free x87 stack capacity for its loads.
Ambient MXCSR and x87 precision, rounding, exception and denormal state remain
unobserved. Those premises do not promise finite native intermediates or output. The actual code has additional
paths, but they supply no broader contract here.

No actual MTB payload, selected key, property name or runtime sample call
was observed. Scalar x's units, its clock/frame conversion, duration, repeat
policy and update scheduling remain unknown. No caller-to-sampling-slot join,
vector assembly, rotation representation, property setter or write into the
pose fields used by hierarchy routine 0x00a3f960 is established.
The Joint+0x18 delegation and 0x00a41ca0 paths were not expanded because
no inspected caller connects them to this codec. This finding therefore
supports a future selected scalar-curve decoder, not replacing static pose
or claiming translation/rotation/scale playback yet. No 30-fps default,
root-motion behavior, axis convention, palette or shader claim follows.

## Reproduction and coverage

Verify the identified original and direct decoder. Run the following command
for the listed ranges with exclusive stop addresses, then compare every
encoded instruction byte through the original PE section mapping:

```text
llvm-objdump -d --x86-asm-syntax=intel --start-address=START --stop-address=STOP ffxivgame.exe
```

| Start VA | Exclusive stop VA | Instructions | Instruction bytes |
|---|---|---:|---:|
| `0x00a30910` | `0x00a313e9` | 811 | 2777 |
| `0x00a2d810` | `0x00a2d820` | 6 | 16 |
| `0x00a2df10` | `0x00a2dfed` | 69 | 221 |
| `0x00a2e590` | `0x00a2e649` | 60 | 185 |
| `0x00a32070` | `0x00a320ab` | 20 | 59 |
| `0x00a320b0` | `0x00a3223a` | 144 | 394 |
| `0x00a31f70` | `0x00a3203b` | 55 | 203 |
| `0x00a2da40` | `0x00a2da59` | 8 | 25 |
| `0x00a2de63` | `0x00a2de6a` | 4 | 7 |
| `0x00a2d840` | `0x00a2d858` | 8 | 24 |
| `0x00a30110` | `0x00a30134` | 11 | 36 |

Read the original data locators above as data, not instructions: the one
type-2 switch entry, the FixedCurve COL pointer and 20-byte COL, the
TypeDescriptor header and terminated mangled name, the scalar evaluator
slot, the terminated Curve selector and the float64 constant. Other switch
entries, property-name globals, resource bytes and execution state remain
outside this bounded finding.
