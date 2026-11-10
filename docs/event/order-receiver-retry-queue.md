# Order receiver submission and retry queue

A received `0x012F` KickEvent is not discarded when its receive step refuses
it. The submitter clones the receiver onto a pointer queue, and one deferred
consumer retries the front of that queue once per scheduling pass until the
receive step admits it. The queue is first in, first out, its front blocks
everything behind it, and four other receiver families share it. A kick whose
receive step answers success is applied at once and is never queued, even when
its owner is not ready.

This page owns the submitter, the queue identity, the retry cadence and the
removal paths. The receive step itself belongs to the
[KickClientOrderEvent receiver](kick-order-event-receiver.md) and the target
staging to [Notice target staging](notice-widget-lifetime.md#fresh-notice-dispatch).

## Tiers

| Tier | Meaning |
|---|---|
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe` |
| INFERRED | A join or role taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

Input: retail Windows 1.23b `ffxivgame.exe`, PE32 x86, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All addresses are VAs.

Every STATIC row was read from the disassembly listing of the Ghidra 12.1.4
auto-analysis program through the `ghidra` command line bridge (`disasm`,
`x-ref to`). Decompiler output was used only to choose which bodies to read.
Image byte values and vtable cells were read from the PE sections of the same
file. One byte-pattern scan of `.text` is stated where it bounds a claim.
No client code ran, so no row observes a live queue.

## Marker bytes

The submitter, the dispatcher and the consumer compare one-byte results
against byte cells rather than literals. Ghidra records only read references
to each cell.

| Cell | Image value | Role in the paths below |
|---|---|---|
| `0x012C41AF` | `0x01` | Success marker a receive step writes to admit |
| `0x0134C560` | zero fill | Refusal marker the kick receive step writes |
| `0x012C3120` | `0x01` | Dispatcher result "not admitted, retry" |
| `0x012C3121` | `0x02` | Consumer result that takes the set-aside branch |
| `0x0134BB04` | zero fill | Dispatcher result after the apply slot ran |

`0x0134C560` and `0x0134BB04` lie past the raw data of `.data` and inside its
virtual size, so the loader zero fills them. A computed-address writer of any
cell is NOT TRACED. Outcomes below that depend on the values are stated for the
image values.

## Submission

STATIC. `LuaActorImpl` vtable cell `0x00FDFC0C` holds `0x0076C0D0`, slot 56 in
the [receiver dispatch table](../net/actorimpl-receiver-dispatch.md). That
body builds the kick receiver on its stack through `0x0089F180` at
`0x0076C18E`, calls `0x007858C0` at `0x0076C1D0` with the impl's `+4`
subobject, the payload pointer and the receiver, then destroys the stack
receiver through `0x0089E800` at `0x0076C1E7`.

STATIC. `0x007858C0` does the following.

1. It resolves the queue pointer `Q` at `0x007858F3..0x007858FD` and compares
   the count `[Q+0x10]` with zero at `0x00785903`.
2. A nonzero count jumps from `0x0078590A` past the attempt. The receiver is
   cloned and appended with no receive call.
3. A zero count calls `0x0089D220` at `0x00785926`. That body only copies
   `byte[0x012C41AF]` to its out byte, so the test at `0x0078592D` passes and
   the common dispatcher `0x00794250` runs at `0x00785953`.
4. A dispatcher result that differs from `byte[0x012C3120]` returns at
   `0x00785962`. Nothing is queued.
5. Otherwise `0x00785966` calls `0x00794240`, a tail jump through receiver
   vtable `+4`. For the kick receiver that cell is `0x010574B4`, holding the
   heap copy `0x0089F530`. The clone pointer is appended at `0x0078598D`
   through `0x00785570`, whose only call is the append `0x007238B0` at
   `0x007855A8`.

STATIC. The dispatcher `0x00794250` calls receiver slot 2 at `0x00794265` and
compares its result byte with `byte[0x012C41AF]` at `0x00794269`. Equality
calls slot 3 at `0x00794279` and returns `byte[0x0134BB04]`. Inequality returns
`byte[0x012C3120]` without calling slot 3. It does not test what slot 3
returns.

With the image values, the kick is queued in exactly two cases: the queue was
already nonempty at submission, or slot 2 wrote the refusal marker.

## When the kick receive step refuses

STATIC. Slot 2 is `0x0089E450` (cell `0x010574B8`). It writes the success
marker first and reads the two target dwords of the object at hub `+0xC`,
which the linked pages name MyPlayer. `N` is the dword at `0x0130C778`, image
value `0xE0000000`.

| Target state | Refusal condition | Address |
|---|---|---|
| `+0x12C` differs from `N` | The key at receiver `+0xC` does not resolve through `0x00CC7A50`, or the resolved object's byte `+0x5C` is zero, or `0x006E11D0` returns nonzero | `0x0089E482..0x0089E4A8` |
| `+0x12C` equals `N`, `+0x128` differs | After `0x0089E200` on receiver `+0x6C`, the key at `+0x128` does not resolve or its byte `+0x5C` is zero | `0x0089E4C2..0x0089E4E9` |
| Both equal `N`, receiver byte `+0x80` nonzero | Always. The receiver's `+0xC` key is stored to `+0x12C` at `0x0089E4FF` | `0x0089E4EF..0x0089E50E` |
| Both equal `N`, receiver byte `+0x80` zero | Never. The success marker stands | `0x0089E4EF..0x0089E4FA` |

STATIC. `0x006E11D0` returns byte `+0x1E` of the object at `[ECX+0xF8]`. Its
receiver at `0x0089E497` is the hub `+0xC` object.

A staging kick therefore refuses once, stages its owner, and waits in the
queue. Each later attempt takes the first row and admits it when the owner
resolves, the owner's `+0x5C` is set and that `+0x1E` byte is zero. The
[ready-byte writer](../actor/kick-gate-writer.md) sets `+0x5C`.

## When the receive step admits an unready owner

STATIC. The last row above tests neither resolution nor `+0x5C`. The
dispatcher then calls slot 3, `0x0089D230` (cell `0x010574BC`), which resolves
the key at receiver `+8` and calls `0x006EE680` on the result with no null
test (`0x0089D23E..0x0089D256`).

STATIC. In `0x006EE680`, an event type byte that differs from
`byte[0x0134C3FF]` resolves the owner key and dynamic casts it
(`0x006EE830..0x006EE83B`). A null result or a zero byte `+0x5C` jumps to
`0x006EE901`. That arm calls `0x0075E3A0` at `0x006EE969` and returns. It does
not reach `0x00895D20`. Its last argument is 1 only when the type byte equals
`byte[0x012C3F7E]`, image value 5, and the acting object's `+0x12C` differs
from `N` (`0x006EE901..0x006EE91F`). The builder that call reaches is recorded
in [Notice target staging](notice-widget-lifetime.md#matching-condition-and-first-eventstart).

Such a kick is consumed on that arm. The receiver is not queued, and no later
pass retries it.

## Queue identity

STATIC. `0x00CC7510` loads `R = [ECX]` and returns `R+0x220` through
`0x00CD7910`. The submitter reads `A = [R+0x220]`, `H = [A+4]` and
`Q = [H+0xF4]`.

STATIC. Let `T` be the receiver of `0x0057A3C0`. It has three call sites, and
each passes its object `+0x510`.

| Call | Containing function | Receiver load |
|---|---|---|
| `0x004D7ADA` | `0x004D7A60` | `LEA ECX,[ESI+0x510]` at `0x004D7AD4` |
| `0x004DB4E2` | `0x004DAF00` | `LEA ECX,[EBX+0x510]` at `0x004DB4DC` |
| `0x004DCF35` | None defined | `MOV ECX,EBX` at `0x004DCF33` |

For the third site, the listing from `0x004DC6EA` to the call holds one
address load of EBX, `LEA EBX,[EDI+0x510]` at `0x004DC6EA`. The other EBX
writes in that range are loop counters at `0x004DC7E6..0x004DC8CF`, in arms
that each jump to `0x004DD3A9`.

- `0x0057A7AE..0x0057A7CA` allocates `0x14` bytes and runs `0x00581E30`,
  which zeroes `+4`, `+8`, `+0xC` and `+0x10`. `0x0057A7DF` stores the pointer
  at `T+0x20` through `0x00582E70`.
- `0x0057A98E..0x0057A991` loads and pushes `[T+0x20]` and
  `0x0057A992..0x0057A995` loads and pushes `[T+0x1C]` for the hub constructor `0x00773270`, called at `0x0057A9E9`. That body stores them
  at hub `+0xF4` (`0x007733C8`) and `+0xF0` (`0x007733BE`). The hub pointer is
  stored at `T+4` at `0x0057AA1A`.
- `0x0057AAB1` calls `0x007634B0` with `[T]`, `[T+4]` and `[T+8]`. It wraps
  the hub pointer in a holder through `0x00779B50` and `0x00778D80`, holder
  `+4` being the hub. `0x00763648` reaches `0x00CD78E0` through `0x00CC9820`,
  which stores holder slot 2's result at `+0x220` of the pointee of `[T]`
  (`0x00CD7902`). Holder vtable `0x00FDFFD0` slot 2 is `0x00CDE8F0`, which
  returns the holder. `0x00CD78E0` has one caller chain, ending at
  `0x00763648`.

So `[[x+0x220]+4]` is the hub and hub `+0xF4` is the pointer at `T+0x20`.

STATIC. `0x00578970` loads `[T+0x20]` at `0x00578A12` as the receiver of the
deferred consumer `0x005836D0`, called at `0x00578A1A`. It loads `[T+0x1C]`
at `0x00578A05` for the sibling consumer `0x00583440`, which dispatches
through `0x00773F10` at `0x005834E0` and never touches `[T+0x20]`.

INFERRED. The submitter's `R`, read from `LuaActorImpl +4`, is the same object
as the pointee of `[T]`. One writer of `+0x220` was found and no second root
was found, but the equality is a runtime identity that static reading does
not close.

STATIC. A scan of `.text` for a `[reg+0xF4]` load within 28 bytes after a call
to `0x00CC7510` finds six sites. All six append and none removes.

| Load | Function | Receiver family in the dispatch table |
|---|---|---|
| `0x007858FD` | `0x007858C0`, from slot 56 | KickClientOrderEvent |
| `0x007859ED` | `0x007859B0`, from slot 57 | StartServerOrderEventFunction |
| `0x00785ADD` | `0x00785AA0`, from slot 58 | EndClientOrderEvent |
| `0x00785C2D` | `0x00785BF0`, from slot 78 | JobQuestCompleteTriple |
| `0x0076C75E` | `0x0076C720`, slot 88 | ChangeActorSubStatStatus |
| `0x0076C932` | `0x0076C8D0` | None. It appends each pointer of the impl `+0x38` vector through `0x007238B0` at `0x0076C993` |

The first four differ only in the preflight call (`0x0089D220`, `0x0089E3F0`,
`0x0089D170`, `0x0089D340`). The scan is bounded to that instruction pattern.

## Retry cadence

STATIC. `0x005836D0` loops while `[Q+0x10]` is nonzero.

1. It reads the front pointer from the block map at `[Q+4]` with start index
   `[Q+0xC]`, four pointers per block (`0x00583724..0x00583761`), and calls
   the dispatcher at `0x00583770`.
2. A result equal to `byte[0x012C3120]` jumps to `0x00583881` at `0x0058377F`.
   The pass ends. The front stays where it is and nothing behind it is
   attempted.
3. A result equal to `byte[0x012C3121]` moves the front pointer to a local
   vector and pops it without destroying it (`0x00583791..0x0058382B`). After
   the loop, `0x0058388F..0x00583911` walks that vector from its end and
   reinserts each pointer at the front through `0x00783420`, which restores
   the original order.
4. Any other result calls the receiver's slot 0 with argument 1 at
   `0x0058383C`, pops the front, and increments a counter. The pass ends when
   the counter reaches `0x20` (`0x0058386E`).

STATIC. The dispatcher returns only `byte[0x0134BB04]` or `byte[0x012C3120]`.
With the image values, step 3 is not reachable through it, a refused front is
attempted exactly once per pass, and an admitted front is destroyed and
removed after its slot 3 call.

STATIC. The only call of `0x005836D0` is `0x00578A1A`, and the only call of
`0x00578970` is `0x004DA948` in `0x004DA680`. `0x00578970` skips the whole
sequence when byte `T+0x3B` is nonzero or `0x00CC9890` returns nonzero
(`0x00578996..0x005789A9`). Its place in the application update is recorded
in [Application hierarchy](../resource/application-hierarchy.md#ordered-application-scheduling-calls).
The interval between passes in wall time is NOT TRACED.

STATIC. Within one pass the ready-byte writer `0x00766F00` runs at
`0x005789DB`, ahead of the consumer. A control promoted in a pass can admit a
waiting kick in the same pass.

## Capacity and order

STATIC. The append `0x007238B0` writes at index `[Q+0xC] + [Q+0x10]` and
increments `[Q+0x10]` (`0x007238D4..0x00723921`). It grows the block map
through `0x00722770` and allocates `0x10`-byte blocks through `0x0044D500`.
No instruction on the append path compares the count with a queue limit. The
only bound read is the container growth constant `0x3FFFFFFF` in
`0x00722770`.

STATIC. Elements are whole receiver pointers. Every submitter appends at the
back and the consumer reads the front, so order is submission order across
all five receiver families. No path compares a new receiver with a queued one,
replaces one, or merges two for the same owner.

## Removal

STATIC. Three paths remove a queued receiver.

| Path | Address | Effect |
|---|---|---|
| Admitted or otherwise finished front | `0x0058382D..0x00583864` | Slot 0 destroy, pop front |
| `0x00578230` | `0x00578318..0x0057831C` | Resets `T+0x20` to null through `0x00582E70`. That calls `0x00581CF0`, which destroys every queued receiver through slot 0 and frees the queue |
| `0x00578090` | `0x00578145..0x00578148` | The same destruction through `0x00582E10` |

`0x00578230` is called at `0x004D9C4E` in `0x004D9BC0`, at `0x004DB0B5` in
`0x004DAF00` and at `0x004DCE82`, which lies in no defined function. Two of
those sites run the initializer `0x0057A3C0` again, which builds a fresh empty
queue: `0x004DAF00` at `0x004DB4E2`, and the body holding `0x004DCE82` at
`0x004DCF35`. Both calls of that body take their receiver from EBX
(`0x004DCE80`, `0x004DCF33`).

`0x00578090` is called at `0x004DBDA3` in `0x004DBAF0`. Two exception unwind
funclets also reach it by tail jump, at `0x00E5D8DD` and `0x00E5DB04`. Each
loads a frame-saved object pointer and adds `0x510` before the jump
(`0x00E5D8D4..0x00E5D8D7`, `0x00E5DAFB..0x00E5DAFE`).

NOT TRACED:

- Which client events reach the callers `0x004D9BC0`, `0x004DAF00` and
  `0x004DBAF0` or the call site `0x004DCE82`. No tie to a zone change, an
  actor removal or a logout is recorded here.
- Which functions own the two unwind funclets, and whether either runs in
  normal play.
- Any removal of one queued receiver by owner, by target or by age. None was
  found on the paths above, and the bounded scan found no other reader of hub
  `+0xF4`. A reader that reaches the queue through another pointer chain is
  not excluded.
- What clears a staged `+0x12C` under a waiting kick beyond the two writers
  the linked pages record, and whether a waiting kick whose owner never
  becomes ready is ever released.
- Mutable values of the marker bytes after load.
- A retail capture that places a `0x012F` ahead of its owner's readiness.

## Consequences for an implementation

- A kick that the receive step refuses is held, not dropped. It is retried
  once per scheduling pass with no timeout and no attempt limit.
- The hold is one queue for five receiver families. A waiting kick delays
  every later `0x012F`, `0x0130` and End receiver, and each of those is queued
  without an immediate attempt while the queue is nonempty.
- Kicks are not coalesced. Each received kick keeps its own queue entry in
  arrival order.
- A kick with no staged target and a zero receiver `+0x80` is not held for an
  unready owner. It is applied at once on the unready arm.
