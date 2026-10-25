# Action queue SCB handoff and Scheduler profile

The checked CharaActor-table path carries a selected SCB resource into a
base Scheduler S and returns a CutSceneObject W holding S at W+8. The
queue stores W at +0x10. This supplies another conditional factory profile
to qualify for motion consumers; the global Acc/Mcc factory contract does
not alone identify this path's registry or updater.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 x86-32 compared
selected retained Ghidra instructions with original PE bytes and independently
decoded their boundaries. Table cells, COLs and type names came from the PE.
No target execution supplied these observations.

The [bank-routing finding](../actor/animation-bank-routing.md#food-and-drink-bank-composition)
owns the `main`/empty-name SCB search. The [queue contract](../actor/action-queue.md)
owns staged source/target fields, queue+0x10 dispatch and actor+0x12F0
lifecycle. The generic [CutSceneObject contract](timeline-weather-record-view.md#synchronous-scheduler-callback)
owns constructor `0x00A05000`, W+8=S, table/RTTI and slot 61. Its
LayScheduler-specific caller and effects are separate from this input path.

## Queue resource and owner

Let Q be incoming ECX to `0x00844330`, A=`u32(Q+0x18)` and
C=`u32(A+0x12F0)`. CharaActionQue table `0x0103E428` slot 8 selects
this method. Its arguments 1/2 are the resource-array pointer/count; denote
arguments 3/4 by I/T. After its SCB search returns nonzero R at
`0x0084442C`, the call at `0x00844489` supplies receiver C and
`(u32(A+0xDC),I,R,T,0)`. C must be nonnull. These instructions do not
replace A with T's staged source or resolved target.

CharaActor initialization branch `0x0065F9D9..0x0065FA28` constructs C
through `0x0080E8C0` with `(G,u32(A+0x118),A,0,-1)`, where
G=`0x0060B370(u32(A+0x114))`, and stores its return at A+0x12F0
at `0x0065FA22`. It installs RaptureSchEffectController primary/secondary
tables `0x00FF5628/0x00FF55F4` at C/C+4. Embedded C+0x38 construction
through `0x0080E6A0 -> 0x0080EFC0` retains G at embedded+4 and A
at embedded+8, then installs RaptureSchEffectContainer table `0x00FF55E4`
at `0x0080E6E7`. Allocation, normal ABI and retained objects/fields/tables
remain conditions through the queue call.

## Context and embedded dispatch

`0x0080E070` prepares stack context K, including a 0x1A0-byte copy of
T at K+0x68 and source/target binding branches. At `0x0080E160` it
calls `0x0080CC00` with the same C and `(u32(A+0xDC),I,R,&K,0)`.
Valid T storage, row count and rows remain input conditions.

On its surviving count/collection branch, `0x0080CC00` calls embedded
C+0x38 virtual +4 with `(R,&K)` at `0x0080CCD4`. Under its constructed
table, cell `0x00FF55E8` selects `0x0080E7A0`, which calls
`0x0080F3C0(embedded,R)` at `0x0080E7DF`. Later virtual calls and
collection insertion must preserve returned W. These methods invoke W virtual
+0xEC and +0xC8; the zero fifth input bypasses the optional +0x2C call
in `0x0080CC00`.

`0x0080F3C0` loads A from embedded+8. Under CharaActor table
`0x00FC0D34`, A virtual +0x13C selects `0x0061FF10`, returning
U=`u32(A+0x40)`. The [constructor-qualified input profile](action-queue-findvins-input.md)
joins U to the world-owned RaptureActorContainer, then follows its world and
resource graph through the three virtual calls into FindVins constructor
`0x008D0DE0`. This requires that construction path and retained objects,
tables and resource links; it does not classify an arbitrary U. Successful
adapter vector publication and the selected live inputs remain unproved.
Denote the resulting staged adapter dword by Dv.

The later A virtual +0x214 call at `0x0080F47B` has a separate concrete
CharaActor-table target: slot 133 is `0x0065B6D0`, returning A+0x118C
with plain `RET`. Direct getter `0x00634860` returns receiver+4 with
plain `RET`, yielding A+0x1190 for `0x00635690`. They leave the pending
`(R,0,Dv)` arguments stacked. `0x00635690` passes them to `0x00A02C60`
at `0x006356ED`, using singleton helper `0x00A02360`'s return as receiver J.
Both consuming methods use `RET 0x0C`. Treating either getter as consuming
the pending arguments would break this ABI.

## Base Scheduler and wrapper return

`0x00A02C60` constructs SchedulerFactory F through `0x00A26020`
at `0x00A02D43`. Let V6 be its retained third input after the zero-input
fallback. Nonzero Dv is retained unchanged. For zero Dv, the branch
`0x00A02C92..0x00A02CEF` substitutes a newly allocated `0x008581B0`
constructor result, or zero on allocation failure, in both EBX and the incoming
third-argument cell. Its constructor input comes through another indirect
call at `0x00A02CD8`; that input remains unqualified. F receives:

```text
(R,u32(J+0x74),u32(J+0x68),u32(J+0x6C),u32(J+0x78),V6)
```

F's installed table `0x010984E0` slot 1 selects `0x00A26640` at
`0x00A02D66`. F+0x14 derives from J+0x6C; equality to the
[global Acc/Mcc registry](combat-motion-name-provider.md#global-registration-and-root-construction)
was not established by these inputs.

Both allocation branches in `0x00A26640` directly invoke base Scheduler
constructor `0x00A1EBE0` at `0x00A266DA` or `0x00A26783`. The
checked body has no later derived Acc/Mcc construction or receiver-table
installation. The [base constructor](scb-clip-factory-composition.md#evidence-identity)
installs Scheduler tables `0x01097A54/0x01097A40`, independently confirmed
by original RTTI. Parser/state guards and possible release of S govern whether
a nonzero output survives. The caller extracts S from its returned output
cell at `0x00A02D68..0x00A02D6C`.

It then supplies S to `0x00A05000` at `0x00A02E03..0x00A02E06`,
yielding W under the linked generic wrapper contract. `0x00A02E0D` stores
W in the caller's incoming first-argument cell at entry ESP+4; `0x00A02F05`
loads the same cell for return. Allocation failure stores zero there. A
separate CutSceneItemObject retains S at +4 and W at +8; it is not W.

With normal return/cleanup and retention through intervening calls, W returns
through `0x00635690`, `0x0080F3C0`, `0x0080E7A0`, `0x0080CC00`
and `0x0080E070`. After `0x00843F40`, `0x008444AE` stores it at
Q+0x10 and `0x008444BF` invokes W virtual +0xEC with &(Q+8).
CutSceneObject table `0x010942E4` slot 59 is `0x00A043B0`, distinct
from slot-61 callback `0x00A03C50`. Its checked body has no direct call/jump
to updater entries `0x00A1F3C0`, `0x00A1E820`, `0x00A1E710` or
`0x00A1E380`. External/indirect callee effects remain unqualified.

## Remaining bindings and reproduction

This establishes a conditional queue-to-base-S ownership relation. The registry,
parsed provider and retained-item updater qualifications below compose with it
under their stated conditions. Selected live actor/FindVins inputs, retained
J/registry/provider state, actual framework execution and selected callbacks remain
unproved. No equality with a root Acc/Mcc, selected invocation,
command, normal/hard assignment or playback follows.

Authenticate the PE above and compare retained instructions with original
bytes and independent x86-32 decoding. Check `0x00844330`, `0x0080E070`,
`0x0080CC00`, `0x0080E7A0`, `0x0080F3C0`, `0x0061FF10`,
`0x0065B6D0`, `0x00634860`, `0x00635690`, `0x00A02C60`,
`0x00A26020`, `0x00A26640`, `0x00A05000`, `0x0080E6A0`,
`0x0080EFC0`, `0x0080E8C0`, `0x00A043B0` and actor setup excerpt
`[0x0065F9D9,0x0065FA28)`, plus cited cells/RTTI. Distinguish complete
instruction unions from enclosing spans with unlisted alignment gaps. Follow
plain-RET getters, output cells, receiver adjustments and five/three-argument
cleanup separately.

## Queue registry and MotionClip implementation

Singleton `0x00A02360` reads global `0x01364A98` at `0x00A02375`,
constructs J through `0x00A02030` on its null branch, and publishes the
return at `0x00A023BA`. Getter `0x00A01540` returns `u32(J+0x6C)`
with plain `RET`. The [parameter and registry construction contract](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
owns initializer `0x00A01A50`, its registry replacement at `0x00A01BF3`
and the requirement to register again after replacement.

For the queue capture, `0x00A02D2B/0x00A02D32` load and push
G=`u32(J+0x6C)` as factory argument 4. `0x00A26046` stores it at
F+0x14. Both base-construction branches in `0x00A26640` supply it as
Scheduler argument 3, stored at S+0x7C at `0x00A1EC9F`. The parser
`0x00A24E70` uses plain `RET`, leaving the pushed mode 0/1 as constructor
argument 7; its parsed result is argument 6 at S+0x50. Constructor
`0x00A1EBE0` consumes all seven dwords with `RET 0x1C`.

Under the [synchronous builder contract](scb-clip-factory-composition.md#synchronous-builder-invocation),
S+0x7C reaches local builder+8, then the class-key lookup receiver is G+8
at `0x00A28641`. Thus the retained source relation is:

```text
u32(S+0x7C) = u32(F+0x14) = u32(J+0x6C) at capture
```

CDevSchedulerManager slot 3 at `0x00635760` first calls the base registrar
`0x00A4C2A0` at `0x00635899` and continues on nonzero AL. The base
MotionClip insertion at `0x00A4C49C..0x00A4C4BD` pairs literal
`0x0109B778` with factory `0x00A4DCD0`, using J from `0x00A02360`
and G from `0x00A01540`. The Rapture replacement then uses:

| Operation | Key | Factory | Original call |
|---|---|---|---|
| Erase through `0x0063F920` | `0x00FBD21C` | - | `0x00636CAF` |
| Insert through `0x0063F710` | `0x00FBD228` | `0x0063C180` | `0x00636CDD` |

Each obtains `u32(returnedJ+0x6C)+8` through those same plain-RET
getters. All three literal keys spell `MotionClip` followed by NUL.
The [equal-key erase/insertion contract](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
means successful erase followed by successful insertion replaces the earlier
factory; insertion alone retains an existing equal key. The
[factory identity](combat-motion-name-provider.md#factory-identity) qualifies
`0x0063C180 -> 0x0082EF70` as RaptureMotionClip, table `0x010337EC`,
slot 1 `0x0082F120`.

This profile requires the registration calls and queue creation to retain the
same J and G, valid key/tree/arena state, and the accepted Rapture pair through
builder lookup. A later registry replacement or mutation can break that
relation. No equality to the separate slot-5 global registry is needed.

## Eager parser and serialized name provider

A returned S need not have completed parsing. `0x00A26640` calls
`0x00A26380` on the same S at `0x00A266F4` on one branch, or at
`0x00A267AB` after the resource virtual +0x2C AL test passes. The
alternative `0x00A267C6` sets bit 0x40 in S+0x91 and skips that direct
parser invocation. The relation here requires the branch that reaches
`0x00A26380` and its publication.

At `0x00A2655A`, that parser supplies S and its arena to `0x00A25DC0`,
then stores returned ResourceReference Rr at S+0x74 at `0x00A26561`.
The helper supplies `u32(u32(S+0x50)+0x38)` as constructor
`0x00A27450` argument 3, stored at Rr+0x10 at `0x00A2746E`.
With P=`u32(S+0x50)`, valid storage, successful construction and retention:

```text
u32(Rr+0x10) = u32(P+0x38)
```

Original ResourceReference table `0x01098604` slots 2/3 are
`0x00A276A0/0x00A27560`. This is the same provider source used by the
[published RIDTBL formula](combat-motion-name-provider.md#resourcereference-and-serialized-provider).
With callback context+0x0C=S, retained parsed fields/provider and a constructed
RaptureMotionClip, its nonnegative index can therefore use that formula on
this base queue path. The existing bounds, source-NUL and 15-character
conversion conditions still apply. This does not establish a selected index,
authored resource, callback or name match.

## Retained item and Scheduler update

The item K created at `0x00A02E53..0x00A02E62` stores S at K+4,
W at K+8, input argument 2 at K+0x0C and zero at K+0x10. Its table
`0x01093F00` and original RTTI identify CutSceneItemObject. The creation
path also stores W at S+0xB8 at `0x00A02E2F`.

Let H=`u32(J+0x50)` and Tail=`u32(H+4)` before insertion. At
`0x00A02E7F`, helper `0x008EA4E0` receives `(H,Tail,&Klocal)`.
It allocates node N and stores N+0=H, N+4=Tail and N+8=K, returning
with `RET 0x0C`. The caller increments J+0x54 through `0x00A032B0`,
stores N at H+4 at `0x00A02E95`, and stores N at Tail+0 at
`0x00A02E9B`. This appends N before H in an intact ring. Constructor
`0x00A02030 -> 0x00D51550` initializes H+0=H and H+4=H, with
J+0x54 zero. Successful allocation and normal capacity-guard return remain
conditions.

`0x00A025C0` retains its incoming ECX as J. It skips iteration if
`u8(J+0x20)!=0` or its argument points to a zero dword. Its earlier
clock-rate setter still runs before those guards. On the surviving path it
starts at N=`u32(H)`, compares N with H, and follows N+0 at the iteration
tail. With retained links, it can reach the appended N.

The per-item branch samples b=`u8(S+0x91)&0x20` and state=`u32(K+0x10)`.
The common update block is reached directly when b is zero or, for nonzero b,
state is outside 0/1/2. Other paths need the following true helper results;
false results can skip this item, and the temporary factory/callbacks remain
retention conditions:

| State when b is nonzero | Required path to common update |
|---|---|
| 0 | S+0x6C virtual +0x2C, then `0x00A020C0(J,S)`, then `0x00A01E70(J,S)`; state becomes 3 |
| 1 | `0x00A02170(J,S)`, then `0x00A01E70(J,S)`; state becomes 3 |
| 2 | `0x00A01F10(J,S)`; state becomes 3 |

These are branch predicates, without semantic readiness or success claims.
K+0x10 starts at zero; no assumption is made about S's initial b.

The common block calls W virtual +0x64 at `0x00A02823`, selecting
`0x00A03740` under table `0x010942E4`. With retained nonnull W+8,
it returns the float at W+0x24. After local scaling or its S+0xC8 override,
the block calls S virtual +0x34 at `0x00A02871`. Base Scheduler cell
`0x01097A88` selects `0x00A1B810 -> 0x00A20390`, which sets the
clock rate. No selected numeric rate is inferred.

Finally `0x00A0287D/0x00A02880` load ECX through N+8 -> K+4 -> S,
and `0x00A02883` directly calls `0x00A1F3C0`. Under the same J,
retained N/K/S/W, loop guards and common branch, this is the exact constructed
S. It composes with the [existing updater/context chain](scb-clip-factory-composition.md#secondary-collection-callers-and-callback-context)
under that chain's own collection, iterator, state and table conditions.

Retention is substantive: `0x00A02B20(J,W)` can match K+8 through
`0x00A01790`, move K to J+0x58/+0x5C, clear the old N+8 and remove
N through `0x00DB48C0`. Cleanup `0x00A02A10 -> 0x00A022B0` can
release and clear K+4=S and K+8=W. The iteration and insertion use
EnterCriticalSection/LeaveCriticalSection on J+8, but those static calls do
not establish object lifetime or exclude reentrant callbacks.

## Framework callback entry

The normal SceneThread update contains a conditional static call path into
the queue updater. Let H be SceneThread and T=`u32(H+0x38)`, retaining the
constructed objects, tables and fields through intervening calls. The
[thread and initialization contract](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
owns the upstream thread state and initialization order.

Original SceneThread table `0x00F93168` slot 2 selects `0x004E9320`.
The thread-procedure calls at `0x00D35630..0x00D3563F` and
`0x00D35664..0x00D35673` preserve H for that slot. At
`0x004E9371/0x004E9377`, the selected method reloads T and directly calls
`0x00A49B10` with ECX=T.

That method calls T slot 5 at `0x00A49BD5`, then the returned object's
slot 11 at `0x00A49BDE`. AL must be zero at `0x00A49BE0`.
On this normal-return branch, `0x00A4A118` calls `0x00A4B470` with
`(100,499,X)`, independently of the earlier T+0x3C branch. Original cells
`0x01093550/0x010936C8` supply those inclusive group bounds. X is the
address of the local dword at ESP+0x0C before the pushes at
`0x00A4A109..0x00A4A10F`; its dynamically calculated value is unqualified.
AL nonzero instead selects the branch at `0x00A4A2B7`, whose single-group
calls use 466 and 520, not the group below.

### Constructed record and group membership

Framework constructor `0x00A4A730` installs table `0x0109B16C`.
The selected SceneThreadImpl construction calls it at `0x004EA89C` and
installs table `0x00F931D4` at `0x004EA8B8`. Let B=T+0x1D8.
The constructor's `0x00A4A967..0x00A4A9A8` writes:

| Field | Constructed value |
|---|---|
| B+0/B+4 | B/B, an initially self-linked record |
| B+8 | Adapter `0x00A68EF0` |
| B+0x0C | Bound receiver T |
| B+0x10 | Thunk `0x008F70D0` |
| B+0x14 | Zero receiver adjustment |
| B+0x18 | Group 130 from original cell `0x01093580` |
| B+0x1C | Zero |
| B+0x20 | `0x01093128`, the original string `UPDATE_CUT_SCHEDULER` |

SceneThread slot 1 `0x004E8460` calls T slot 1 at `0x004E8470`.
For the constructed SceneThreadImpl table, `0x004E84D0` then directly
calls `0x00A48BE0` at `0x004E84E0`. After virtual slot-7 initialization,
that method enrolls the records. At `0x00A48D5B..0x00A48D90`, it inserts
B before head `L+8+8*q`, where
`L=u32(u32(u32(0x01364A90)+0x0C)+0x0C)` and
`q=u32(B+0x1C)+2*u32(B+0x18)`. Retaining the constructed fields gives
q=260 and head L+8+16*130, the group-130 head consumed by `0x00A00DD0`.
Successful initialization and retained list membership remain conditions.

### Dispatch and argument cleanup

`0x00A4B470` chooses `0x00A4B220` when both u8(D+0x24) is nonzero
and u8(D+0x26) is zero, for D=`u32(0x0137B460)`. Otherwise it chooses
`0x00A4B200`. It stores the pointer at `u32(0x01364A90)+0x20`, then
calls `0x00A00DD0` with the same `(100,499,X)` at `0x00A4B4CD`.

`0x00A00DD0` traverses the inclusive group range with a sixteen-byte stride.
For a nonempty group, it transfers the ring to a work ring, removes a record
and reinserts it into the original group before calling the dispatch pointer
with `(0,B,X)` at `0x00A00E77..0x00A00E82`. Group 130 lies in that range.
The record must survive traversal and prior callbacks; concurrent or reentrant
mutations can prevent this visitation.

The complete body `[0x00A4B200,0x00A4B217)` replaces its first argument with
`u32(B+0x0C)` and its second with B+0x10, then tail-jumps `u32(B+8)`.
The profiling alternative `0x00A4B220` makes the same three-argument call
at `0x00A4B290`, subject to its instrumentation returning normally and
preserving B. Both routes supply `(T,B+0x10,X)` to `0x00A68EF0`.

That adapter's complete body `[0x00A68EF0,0x00A68F05)` pushes X, forms
ECX=T+`u32(B+0x14)`=T and calls `u32(B+0x10)` at `0x00A68F02`.
The exact five-byte thunk `[0x008F70D0,0x008F70D5)` is
`MOV EAX,[ECX]; JMP [EAX+0x68]`, preserving T and the pending X.
Both original slot-26 cells `0x00F9323C/0x0109B1D4` select `0x00A48970`.
Its complete twelve bytes call singleton `0x00A02360`, move returned J into
ECX and tail-jump to `0x00A025C0` at `0x00A48977`.

The updater's `RET 4` at `0x00A02A06` consumes the adapter-pushed X.
The adapter has plain `RET`; the profiling dispatcher removes its forwarded
twelve bytes, and the group walker removes its own three dispatch arguments.
The group walker has `RET 0x0C`; `0x00A4B470` has plain `RET`, and
`0x00A4A125` removes its caller's three arguments.

This qualifies the static dispatcher, receiver and argument path. It still
requires the zero-AL branch, normal returns, retained callback/list state and
tables, and a singleton return retaining the queue's same J. Actual execution,
selected queue/clip state and playback were not observed.

## Profile and update reproduction

Use the PE identity and byte/union procedure above. Registry-specific excerpts
are `0x00A02375..0x00A0237A`, `0x00A023B3..0x00A023BF`,
`0x00A01BC2..0x00A01BF9`, getter `0x00A01540`, the queue factory
capture, both `0x00A26640` constructor/parser branches, base field stores,
and `0x00636C93..0x00636CE2`, plus the base MotionClip insertion and
three literal keys. Check exact argument cleanup and the named provider
publication through `0x00A26380 -> 0x00A25DC0 -> 0x00A27450`.
The singleton global operands `0x01364A98/0x01365604` lie in the PE's
virtual data tail and have no original file-backed scalar values.

For item delivery check complete bodies `0x00A025C0`, `0x00A02C60`,
`0x008EA4E0`, `0x00D51550`, `0x00A032B0`, `0x00A03740`,
`0x00A03600`, `0x00A1B810`, `0x00A20390`, `0x00A02B20`,
`0x00A02A10`, `0x00A022B0`, `0x00A01790` and `0x00DB48C0`,
plus original tables/RTTI and critical-section imports. Preserve unlisted
alignment gaps, including `[0x00A02AEB,0x00A02AF5)`, separately from
listed instruction unions. Check empty and nonempty ring insertion and
the exact retained payload loads before composing with the linked updater.
For incoming J check `[0x00A48970,0x00A4897C)` and original slot-26
cells `0x00F9323C/0x0109B1D4`; distinguish this helper's supplied receiver
from actual invocation of the owning framework slot.

For the constructed callback path, check complete retained instruction unions
`0x004E9320`, `0x004E8460`, `0x004E84D0`, `0x00A4A730`,
`0x00A48BE0`, `0x00A49B10`, `0x00A4B470`, `0x00A00DD0` and
`0x00A4B220`, plus the exact adapter/dispatcher/thunk bodies above.
Keep the unlisted gap `[0x00A00E2A,0x00A00E30)` separate.
Authenticate the thread and constructor excerpts, the `RET 4`, original
group/name cells and SceneThread/Framework/SceneThreadImpl slots.
In a retained-instruction CFG model with calls returning normally and
`0x00A49BE2`'s nonzero-AL branch disabled, `0x00A4A118` dominates the normal
return at `0x00A4A33A`. The model excludes exceptions, nonreturning callees
and arbitrary indirect-callee state changes.
