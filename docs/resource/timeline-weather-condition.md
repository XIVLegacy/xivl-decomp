# TimeLine weather-condition evaluator

The native TimeLine property path contains a conditional evaluator with a
data-derived count, a queue-recovered ID operand and first-match bitmap
selection. Its concrete record-view and backend construction bindings remain
unresolved. The static behavior below therefore narrows the condition boundary
without establishing automatic activation by an added weather ID.

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra instruction listings were checked against the original PE with
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode.
These are static observations. No client execution or playback was observed.

## Entry and record-view boundary

RaptureTimeLineBaseObject's slot 7 cell at `0x01062098` points to
`0x008D5440`. It adjusts ECX by four and tails to `0x00AA89A0`, which retains
the supplied raw record pointer at object `+4` and returns true. This
attachment endpoint does not establish the later record-view binding.

The RTTI-named RaptureTimeLineInstanceObjectProperty table `0x00FF2D2C`
has slot 9 cell `0x00FF2D50` targeting `0x007FDCA0`. Let P be its incoming
property interface and C=P-4 its complete receiver. The method calls
`0x007FC7C0(C)`, forwards its argument through `0x00AA2930(P)`, and calls
`0x007FCBF0(C)` when byte P+`0x40` is nonzero. This gate is C+`0x44`.
The forwarding body uses P+`0x0C` and passes the dereferenced input dword
to that retained receiver's slot 24 when the receiver is nonnull.

`0x007FCBF0` calls P slot 4 twice, discards the first return, then calls
the second return's slot 19 and the next return's slot 8 at
`0x007FCC03..0x007FCC1F`. Call the last returned view V. It reads property
Q from V+`0x1C` at `0x007FCC21`. Null Q exits without changing the evaluator
state. The intermediate pointers have no null checks in this body.

P slot 4 cell `0x00FF2D3C` targets `0x008006E0`. This reads the vtable at
incoming ECX+4, advances ECX by four, and tail-dispatches through that table's
slot 3. The [property construction contract](timeline-weather-record-view.md)
resolves that embedded adjusted interface and its stored first return. The
caller-supplied receiver's slot 19 and subsequent record view remain unjoined
to the attached serialized TimeLine record. Matching serialized field offsets
alone does not close this requirement.

## Queue ID and first matching entry

With Q nonnull, `0x007FCC2C..0x007FCC35` obtains the owner through
`0x007E02F0` and calls `0x007E14D0(owner)`. The owner getter calls
`0x00623C60`, which reads global `0x0133DEF4`, and returns that actor's
`+0x158` when nonnull. The evaluator does not guard a null owner.

`0x007E14D0` reads owner vector bounds `+0x70/+0x74` as 12-byte entries.
For a valid one-entry queue it returns the first entry's dword `+4` at
`0x007E1555`. For two entries it returns the second entry's dword `+4`
at `0x007E1529`. Other queue counts return `0xFFFFFFFF`.
The [published append record](../net/weather-transition-runtime.md#queue-handoff-and-file-identity)
stores the recovered weather ID at `+4`, with resource key at `+0` and
constructed object at `+8`. Thus this conditional path uses the queue's
recovered ID, which can differ from the actor's normalized ID under the
[resource-key alias](limsa-seasonal-weather-extension.md#additional-ids-and-resource-key-aliases). Two queued records with the same recovered
ID still select the second record. Queue insertion's resource-key deduplication
is separate from this getter's count and position selection.

At `0x007FCC3A..0x007FCC61`, Q+`0x14` is a signed dword count. A positive
count selects Q+`0x10` as the vector. Each entry is eight bytes, and its first
u16 is zero-extended before comparison with the full returned ID. The loop
stops at the first equal entry. Duplicate condition IDs are not combined.
There is no fixed count 23 in this body. Positive counts require a valid
vector and sufficient storage. `0xFFFFFFFF` cannot match even entry ID
`0xFFFF`, because the latter is compared as `0x0000FFFF`.

For count zero or negative, or when no entry matches, `0x007FCC63..0x007FCC74`
calls `0x007FCAD0(C)` only when byte C+`0x45` equals 1. It clears that byte
only when the helper returns true. It leaves C+`0x46` unchanged. Null Q takes
the earlier direct exit and does not request this reset.

## Selected bit and conditional dispatch

For a matching entry, `0x007E0430` returns owner+`0x108` when owner+`0xF4`
is zero and owner+`0x120` otherwise. `0x007FCC87..0x007FCCB0` reads two
dwords there and calls helpers `0x009D5880`, `0x009D9970` and `0x009D9AB0`
with 64-bit divisor words `(60,0)`, `(60,0)` and `(24,0)`. The following
instructions derive a byte address from entry+2 and test one bit at
`0x007FCCE1..0x007FCCE5`. The serialized entry's remaining six bytes,
`+2..+7`, are its bitmap. Helper return conventions and clock-domain bounds
were not established here, so this does not assign Eorzea calendar or
half-hour units or prove in-range byte selection for arbitrary clock inputs.

Let b be the selected bit as Boolean, L be byte C+`0x46`, and F be byte
C+`0x45`. At `0x007FCCEA..0x007FCCF7`, a change from L to b stores L=b
and F=!b. The subsequent behavior is:

| Selected bit | Further precondition | Call and successful state update |
| --- | --- | --- |
| 1 | F equals 0 | `0x007FCD16` calls `0x007FCA30(C)`; true sets F=1 |
| 0 | F equals 1 and the separate reset-veto bit is clear | `0x007FCD63` calls `0x007FCAD0(C)`; true sets F=0 |

The reset-veto bit comes from a different chain: P slot 3, returned receiver
slot 18, then returned receiver slot 8 at `0x007FCD38..0x007FCD52`.
Bit 0 of the final view's `+0x24` suppresses reset at `0x007FCD57`.
This veto applies to a matched zero bit, while the absent-ID path above has
no such test.

`0x007FCA30` gets a receiver through P slot 30, returns false for null,
calls that receiver's slot 4, then slot 9 with 1 at `0x007FCA53/0x007FCA5E`.
Its C+`0x48` iteration follows element slot 25 and returned slot 47.
A nonnull resulting receiver whose slot 1 returns `0x0F` receives slot 41
with 1 at `0x007FCAB2`. The helper returns true after the iteration.

`0x007FCAD0` obtains the same slot-30 receiver, checks it and its minus-four
complete pointer, then calls receiver slot 9 with 0 at `0x007FCAFC`.
Its corresponding iteration uses kind selectors at `0x007FCBDC` and jump
targets at `0x007FCBCC`. Kinds 0, 1 and 2 call the slot-30 receiver's slot 4.
Kind 6 calls the child slot-4 result's slot 39 with 0, then child slot 34
with 2000. Kind `0x0F` calls the child slot-4 result's slot 39 with 0,
then child slot 41 with 2. Other kinds are skipped. It returns true after
the iteration. These arguments identify native dispatch behavior without
authenticating a concrete `ShowHideClip` receiver or a rendered result.

## Remaining additive-ID requirements

The ID operand, signed count, first-match rule and absent-ID branch are
established for this conditional evaluator. Its valid record view, owner queue,
clock input, constructed backend receivers and state remain preconditions.
The [serialized city owner joins](city-halloween-owner-joins.md) establish
resource relationships independently of those native bindings.

If the selected queue record contains a custom u16 N and the supplied vector
contains N, this loop can match its first N entry using the declared positive
count. Appending N alone does not prove the record-view construction, selected
bit, backend identity or scheduler/clip invocation needed for activation.
Neither a complete custom-ID decoration path nor visible playback is established.

To reproduce, verify the executable identity, map the cited VAs through its PE
sections, decode x86-32 instructions at the stated entry/branch locators, and
check the property-interface cells and reset-switch tables. Follow the valid
12-byte queue interpretation against the separately published append path.
Preserve the two distinct getter chains and the unresolved adjusted receiver
instead of substituting a class table based on its name.
