# Motion name lookup and native request

RaptureMotionClip supplies its resolved name and request to `0x007B9920`.
That method first tests MCB availability, then queries CIBT, MCB and MTB
resources and companion names through the actor's resource interface.
A matching MTB name in an authored bank does not establish acceptance or
playback. A [provider initialization and search profile](actor-motion-resource-provider.md)
qualifies a concrete implementation conditionally. Selected root contents and
the lower motion-object conversion remain unresolved.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode
checked the selected retained Ghidra instructions against original PE bytes
and independently decoded them. Original PE reads supplied vtable cells and
literal values. No target execution supplied these observations.

The [RIDTBL provider](combat-motion-name-provider.md) owns the earlier
serialized-name lookup and its callback/scheduler identity condition.
This continuation does not establish a particular selected invocation.

## Entry and MCB availability

RaptureMotionClip's interval `[0x0082F456,0x0082F498)` supplies the name
and request to `0x007B9920` at `0x0082F493`, with ECX equal to the selected
actor address plus 0x4C0. Let M be that receiver, A=`u32(M+4)`, N the
incoming name and I the incoming request. This caller alone does not prove
that A equals the selected actor.

The method returns zero when A's virtual +0x268 returns nonzero or byte
M+0xAB is nonzero. With CharaActor's original primary table `0x00FC0D34`
retained, the first method is `0x00669650`, returning bit 6 of
`u32(A+0x2B70)`. A further refusal checks prefix `cbem`, A+0xCD0 and
helper `0x0065A580`. The selected Castrum `cbbm_...` names do not match
that prefix.

At `0x007B99D1`, helper `0x00665F60` converts N to a 16-byte name through
`0x0062E2D0`, then calls A's virtual +0x10 with pointers to type
`0x006D6362` and that name (`0x00665F95`). It returns whether the result
is nonzero. The [published MCB/MTB types](../actor/battle-npc-body-parts.md)
identify the type as MCB. The conversion preserves ordinary names of at most
15 bytes with an in-range NUL; it does not bound its source scan to 16 bytes.

A successful test copies N to selected-name buffer V. On failure, only a name
with N[2]='n' has the fallback: copy N, change byte 2 to 'b', and repeat the
MCB test at `0x007B9A2B`. An empty V returns zero. The selected `cbbm_...`
names have N[2]='b', so their first MCB lookup must succeed.

## Type/name interface and resource-provider boundary

Original CharaActor cell `0x00FC0D44` and RaptureActor cell `0x00FEA51C`
both contain `0x00620640`. With either primary table retained, this method
handles virtual +0x10. A null `u32(A+8)` returns zero after a diagnostic.
Otherwise `[0x006206CE,0x006206F3)` forwards both pointer arguments
unchanged to virtual +0x10 of B=`u32(A+8)` and returns its result.

The later helper `0x00662720(A,type,name)` converts the name in the same
way and calls the same actor virtual +0x10. Neither wrapper implements a
string search or RES traversal. The wrappers alone do not identify B's concrete
table, resource roots, bank order, duplicate-name precedence or loading behavior.
The [provider construction profile](actor-motion-resource-provider.md)
qualifies that indirect call at `0x006206D9` when its CharaActor initialization
path executes and retains the resulting RaptureReferenceResource. Its actual
root vector, order and fallback graph at a selected lookup remain unproved.
Co-resident resource entries cannot substitute for those identities and state.

## Request assembly

After accepting V, `0x007B9920` constructs a 0x4C-byte request Q, copying
the first 0x18 bytes from I. Its direct resource-query calls produce:

| Call | Type | Name | Stored result |
|---|---|---|---|
| 0x007B9B33 | 0x63696274, CIBT | V | Q+0x40 |
| 0x007B9B49 | 0x006D6362, MCB | V | Q+0x18 |
| 0x007B9B5F | 0x006D7462, MTB | V | Q+0x2C |
| 0x007B9B7A | MCB | V with byte 1='h' | Q+0x1C |
| 0x007B9B90 | MTB | same h variant | Q+0x30 |
| 0x007B9BAB | MTB | V with byte 1='f' | Q+0x34 |
| 0x007B9BC1 | MCB | same f variant | Q+0x20 |
| 0x007B9BDC | MCB | V with byte 1='m' | Q+0x24 |
| 0x007B9BF2 | MTB | same m variant | Q+0x38 |
| 0x007B9C0D | MCB | V with byte 1='t' | Q+0x28 |
| 0x007B9C23 | MTB | same t variant | Q+0x3C |

The [CIBT format](../actor/ifrit-nail-action-banks.md) owns that type tag.
These variants change the second character, separately from the third-character
n-to-b fallback. The table records supplied names and stored pointer results,
without proving that any query succeeds.

The low byte of Q+0x48 receives `u8(M+0xA9)` before that counter increments
(`0x007B9A7F..0x007B9AA8`). Increment wrap replaces zero with 1. Constructor
`0x007B20C0` writes 1 at M+0xA9 at `0x007B2128`, before its reset call
`0x007ABEF0`. The counter at a selected request remains a state condition.
The [constructor owner profile](actor-motion-resource-provider.md#constructor-owner-profile)
also identifies the actor supplied to M+4 and the action controller's owner
field, without proving the selected callback's target. Return AL is loaded from Q+0x48 at
`0x007B9C49`; it is not the lower routine's success result. A nonzero return
therefore does not prove playback.

## Immediate submission and motion-object boundary

At `0x007B9C34`, helper `0x0065E140` receives the result array at Q+0x18.
Its complete body `[0x0065E140,0x0065E145)` is `MOV AL,1; RET 4`.
The normal direct-call path therefore calls `0x007B9830(M,Q)` at
`0x007B9C44`. The other branch's 0x4C-byte queue copy through `0x007C7490`
is not selected by this constant-true callee.

`0x007B9830` repeats the refusal checks. It dereferences Q+0x18 to copy
a 15-byte name from that object's +0x20, appends NUL, and calls
`0x007B8720(M,Q)` at `0x007B988C`. The earlier boolean MCB test does not
prove that the subsequent MCB query returned a retained nonnull pointer.

The lower routine obtains five objects through
`0x0065DB60(A,resourceResult)`, using Q+0x18/0x1C/0x20/0x24/0x28.
This helper returns zero for a null resource or null actor virtual +0x134
result. Otherwise it supplies the resource to virtual +0x0C of that returned
object. With the CharaActor table retained, +0x134 is `0x007CD4F0`, returning
`u32(A+0x1C)`. That object's concrete table and conversion are a second
unproved provider edge, at `0x0065DB95`.

A null primary converted object takes the refusal branch at
`0x007B8851..0x007B886A`. On the later reached loop, each nonnull converted
object is configured through virtual methods and receives virtual +0x0C at
`0x007B8B8F`, with a prepared local request and scalar flag. The checked
interval `[0x007B8ACE,0x007B8BA8)` covers the conditional five-entry loop.
The concrete returned objects, their effects and successful playback remain
unproved.

## Reproduction and claim boundary

Authenticate the PE above, map VAs through its section table and independently
decode the named bodies with Capstone x86-32. Check the Rapture caller,
`0x007B9920`, `0x00665F60`, `0x00662720`, `0x00620640`, `0x0065E140`,
`0x007B9830`, `0x007B8720`, `0x0065DB60` and the cited vtable cells.
The selected query interval is `[0x007B9B25,0x007B9C49)`, and the
resource-wrapper interval is `[0x006206CE,0x006206F3)`.
The [Castrum resource finding](../actor/castrum-presentation.md#motionclip-names-and-resident-mtb-entries)
owns original name and companion-resource matches.

These observations establish a typed name interface and conditional request
submission. They do not bind a selected bank to the actor's current provider,
prove disk loading, establish successful lower request handling or observe
playback. The linked provider profile owns its conditional search algorithm.
These findings supply no
command, variant, spawn, phase or authoritative encounter policy.
