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
U=`u32(A+0x40)`. U virtual +0x18, its return virtual +8 and the latter
return virtual +0x6C(R) supply inputs for FindVins adapter constructor
`0x008D0DE0`. Those dynamic interfaces remain unqualified; the first
unresolved target on this table branch is U virtual +0x18 at `0x0080F3FD`.
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

This establishes a conditional queue-to-base-S ownership relation. The
selected actor table, FindVins interfaces, J+0x6C registry identity,
parser/provider state and same-S update/context invocation remain separate
edges. This base factory profile needs those joins before qualifying a selected
MotionClip. No equality with a root Acc/Mcc, selected invocation, command,
normal/hard assignment or playback follows.

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
