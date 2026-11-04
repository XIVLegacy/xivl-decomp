# MTB Header scalar consumer

The scalar retained from MTB Header+4 has a conditional native consumer
that initializes an endpoint and its difference from zero. This establishes
field relationships and arithmetic, without clock units, playback FPS or a
selected MCB command dispatcher.

## Source and getter

Source: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses are VAs. Original PE section bytes were
checked with pefile 2024.8.26 and independently decoded with Capstone 5.0.7
in x86-32 mode. Complete entry-rooted syntactic normal CFGs were checked
for the native spans below. Calls retain normal fallthrough; exception paths,
callee preservation and runtime object identity are not implied.

The [Header finding](mtb-container-framing.md#header-payload) owns the
Header+4 supply. Constructor `0x00A31A90` stores its third explicit f32
argument at container+0x38 at `0x00A31AF2` and installs table
`0x01098B7C` at `0x00A31AC5`. Original locator cell `0x01098B78`
points to `0x01186B04`, whose type descriptor `0x012EE168` names
`.?AVMotionCurveContainer@Animation@Dw@Engine@CDev@SQEX@@`.
Slot 2 cell `0x01098B84=0x00A2C590` binds the complete four-byte getter:
FLD f32[ECX+0x38], then plain RET. It returns the scalar in x87 ST0
without conversion.

The same getter is also at LineTextElementContainer slot 11 cell
`0x01077C00`; its target alone does not identify a motion receiver. An
exact E8 rel32 scan of executable sections found no direct CALL encoding
to it. The actual virtual caller below supplies the relevant receiver flow.

## Accessor snapshot and retention

Let N be the AnimationResourceNode receiver of `0x00A31920`. If N+0x54
is zero, it calls the published lazy `ParseMTB_` route at `0x00A31931`
with `[N+0x4C]`. On either normal branch it snapshots P=`[N+0x54]`
at `0x00A31936`. It copies N+0x74/+0x78/+0x7C to output argument 1
and writes P through output argument 2 at `0x00A3194F`. There is no
post-parser nonnull check. EAX is not a qualified container return.

After the output writes, `0x00A31955` calls `0x00A31F00` with
ECX=N+0x60 and argument `&[N+0x54]`. Its fast path reads the supplied
pointer at `0x00A31F30`, writes it to the vector-like receiver's old end
slot at `0x00A31F32`, and advances that end by four at
`0x00A31F34`/`0x00A31F37`. This path has no call, dereference through P,
table write or reference-count operation. It conditionally retains the
pointer value, without proving pointed-object lifetime. Growth through
`0x00A31E70` and diagnostic-call effects remain opaque.

After helper return, the accessor clears N+0x54 and N+0x74/+0x78/+0x7C
at `0x00A3195A..0x00A3196F`. The copied output is a pre-helper snapshot.
These observations do not prove safe ownership transfer or that every
possible N+0x54 has the constructed MotionCurveContainer table. That
source/type association, stable output and actual object lifetime remain
conditions for the typed getter join.

## Actual caller and consuming arithmetic

Let S be stable post-prologue ESP of `0x00DFCA80`, with expected call
cleanup. At `0x00DFCB51` it supplies `&[S+0x20]` and `&[S+0x10]` as
the accessor's output arguments 1 and 2. It later loads P from S+0x10,
stores it at owner+0x80 at `0x00DFCB81`, and uses that local P as ECX
for virtual +8 at `0x00DFCC43`. If P is the constructed container with
unchanged table, that call selects the getter above.

The caller stages the original f32 constant 30.0 at `0x00F932E4` twice,
then the returned ST0 scalar and zero. Its call at `0x00DFCC55` is
`0x00E08D80(this=&[S+0x38], 0.0f, scalar, 30.0f, 30.0f)`.
For arguments (a,b,c,d), that complete helper initializes:

| Receiver field | Value |
|---|---|
| +0 / +4 / +8 / +0xC | a / b / c / d |
| +0x10 | a |
| +0x14 | f32(f64(b)-f64(a)) |
| bytes +0x18 / +0x19 | 1 / 0 |

The subtraction uses CVTPS2PD, SUBSD and CVTPD2PS; no division or clock
scaling occurs. It returns its receiver with RET 0x10. At `0x00DFCC5D`,
the caller passes that returned record to `0x00E08CD0` with destination
owner+4. This complete helper transfers six float lanes through FLD/FSTP
and copies the two bytes to corresponding offsets, returning with RET 4.
Its floating transfers are not a general bit-preservation claim for arbitrary
encodings or floating-point environments.

Thus the supplied scalar becomes owner+8 and its difference from zero
becomes owner+0x18. Owner+4/+0x14 receive zero; owner+0xC/+0x10 receive
30.0. This supports an endpoint-and-difference interpretation. No inspected
consumer identifies seconds, FPS, scheduler ticks or a sampling-time domain.
The bound getter's plain RET, expected floating-point state, preserved live
registers, stack/control and readable/writable storage remain conditions.

The owner is an element in the incoming receiver's vector rooted at +0x1C.
The conditional construction profile below identifies it as a MotionControlLayer.
An actual consumer of its initialized range fields is still needed to establish
their later time domain.
Call `0x00A3E670` at `0x00DFCB07` uses incoming argument 2 as its
receiver and is not proof of this owner's constructor.

| Complete normal span | Instructions |
|---|---:|
| `[0x00A31A90,0x00A31B73)` | 68 |
| `[0x00A2C590,0x00A2C594)` | 2 |
| `[0x00A31920,0x00A3197A)` | 27 |
| `[0x00A31F00,0x00A31F69)` | 44 |
| `[0x00DFCA80,0x00DFCD03)` | 200 |
| `[0x00E08D80,0x00E08DD0)` | 18 |
| `[0x00E08CD0,0x00E08D07)` | 19 |

## Conditional context and owner construction

The reached caller `0x00A6FC50` supplies Ctx=`[Outer+0x60]`, result L
from `0x00DFC9F0(Ctx)`, and its explicit input R to `0x00DFCA80` at
`0x00A6FEE5`. It rejects zero L. The selected call also requires the
embedded interface at R+0x0C to return a pointer whose dword equals the
runtime global `0x01323868`; that guard does not identify a serialized member.
Outer, Ctx, L and the accessor's N/P roles remain distinct.

Initially nonzero Outer+0x60 bypasses construction. On its null branch,
`0x00A6FDAA` constructs Ctx through `0x00DFC6A0` and publishes its
return at Outer+0x60 at `0x00A6FDD6`. The constructor consumes two stack
arguments and returns its receiver with RET 8 at `0x00DFC831`. Its first
input is a pointer moved from an opaque `0x00C3BB80` result. The second
argument's low 16 bits are stored at Ctx+0x18; assigning the caller's earlier
prepared EDI word to that argument requires the intervening calls' expected
stack cleanup and preservation. No clock meaning is assigned to that word.

The constructor installs table `0x0113B34C` at `0x00DFC6C3`; original
locator `0x011A7450` and TypeDescriptor `0x01320FD8` identify
`.?AVAnimationController@Animation@Dw@Engine@CDev@SQEX@@`.
It zeros the Ctx+0x1C vector's pointer fields +0x20/+0x24/+0x28.
Its separate [16-byte Joint-row block](mtb-fixed-motion-calculator.md) is not
this owner vector. The opaque
`0x00DFC490(Ctx)` runs after publication and before the common element
producer, so constructor state alone does not establish later vector state.

At `0x00DFCA42`, `0x00DFC9F0` supplies a nonnull candidate to
`0x00DFD360` with argument **address Ctx+4**, not the pointer stored there.
It then supplies returned K to `0x00A31F00` on Ctx+0x1C and returns the
held value as L. Null candidate supplies zero. Successful append, retention
and opaque-call preservation remain conditions; the initially empty vector's
growth path was not closed.

`0x00DFD360` installs table `0x0113B2B4` at `0x00DFD3A0`.
Original locator `0x011A7400` and TypeDescriptor `0x01320F98` identify
`.?AVMotionControlLayer@Animation@Dw@Engine@CDev@SQEX@@`.
It stores address Ctx+4 at K+0x7C, initializes K+0x78/+0x80 to zero,
and constructs the range at K+4 with the already qualified `(0,0,30,30)`
helper. This table is distinct from the AnimationController table.

`0x00DFCAD6..0x00DFCADC` compares requested L with the actual vector entry.
Only equality can reach the selected owner load at `0x00DFCB16`; nonzero
K+0x78 instead takes an early return. Thus a found, eligible entry reaching
the scalar operation is exactly K=L, without a sole-entry assumption. The
constructed profiles require retained objects, fields and current tables;
the preexisting-context branch is not automatically assigned these identities.

With K's constructed table retained, its actual slot-15 call at
`0x00DFCCE4` selects original cell `0x0113B2F0=0x00E11DC0`.
This is the [shared vector-count getter](skeleton-embedded-container-parents.md),
here reading K+0x24/+0x28. It reads neither range field K+8 nor K+0x18.
The returned count goes to `0x00A3B680` at `0x00DFCCE9` with ECX=Ctx+4;
that body was not expanded. A count handoff does not establish range units.
The remaining scalar question is an attributable MotionControlLayer range-field
consumer, with the actual source/type, lifetime and retention conditions above.

Additional original-PE normal CFG coverage was
`[0x00A6FC50,0x00A6FEF1)`, `[0x00DFC6A0,0x00DFC834)`,
`[0x00DFC9F0,0x00DFCA75)`, `[0x00DFD360,0x00DFD454)` and
`[0x00E11DC0,0x00E11DD3)`. Catalog metadata truncated the constructor at
`0x00DFC81E`; its independently decoded 22-byte tail supplied the actual
epilogue and RET 8, without assuming the opaque error calls were nonreturning.
No uninvoked layer slot, allocator, count helper or vector-growth body was read.

## Same-name MCB selection boundary

The selected `m524/act/emp_emp/wss/base/0001` member is 239336 bytes,
SHA-256 `c64e37d06b0cf34df4e5c77d8de4b3fb9c6aad4851af223b61502a07c36e35df`.
Read-only Python directory/header measurement authenticated two MCB rows
named `chr\mon\m524\animation\a001\bt_emp_emp\mcc\base\b001\bin\cbbm_sp_01`:

| Parent directory / row | Half-open file span | Size | SHA-256 |
|---|---|---:|---|
| outer RES `[16,97200)`, row 2 | `[3920,4512)` | 592 | `f0bc6fc4ee499b0d876729c4561032b9510d38bf4e47832ecb6a586025ccc0ac` |
| outer row 4 nested RES `[6400,96800)`, row 14 | `[92032,92560)` | 528 | `e770c4eaf9b44c5eec1ec8a09f8174a672864e236af2b37fb0df4f21e5a26416` |

Both have `SEDBMCB` followed by NUL, version 2, header size 0x30 and
declared size equal to their extent. The adjacent MTB rows 3 and nested 15
are byte-identical, 1879 bytes, SHA-256
`dd9ce2e2a0f918a3839101d40a9cb21fe9f7a12d84563727c2dcb3df138d9fd3`.
All selected directory/name/header spans fit their own parents. Name,
adjacency and equal MTB bytes cannot select between the distinct MCBs.
Actual provider/root selection and a concrete MCB dispatcher remain missing;
no command payload was decoded. These rows do not join the scalar consumer
to a selected SCB MotionClip or the separate scheduler tick conversion.
