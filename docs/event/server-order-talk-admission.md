# Server-order talk admission

Event type `51` is the server-order form of normal talk type `1`. A client can
send it while the talk block is pending at `PlayerManager+0x0c`, before its
normal event callback has installed a delegate wait. EndEvent with selector
`2` admits the matching pending block. Selector `0` does nothing for this type.

## Evidence identity and method

These are direct static observations from retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs at image base `0x00400000`. The observations were
checked with Capstone `5.0.7`, x86 32-bit mode, and pefile `2024.8.26` for
VA-to-file-offset translation. Instruction and table observations have high
confidence. Runtime scheduling and the state of any particular client require
separate live evidence.

## Request producer

`0x00897310` calls the event classifier at `0x008939c0`. Its active branch
stores a client-side block at `+0x08` and calls `0x008962c0`. Its server-order
branch stores a client-side block at `+0x0c` at `0x008974c7`, then calls the
EventStart builder wrapper `0x0075e510` at `0x008974eb`.

The wrapper calls `0x0078de50` at `0x0075e5c4` to convert the condition kind.
The jump table at `0x0078debc` sends kind `1` to `0x0078deaa`, which reads
the byte `0x33` from `0x012c3f80`. Thus a normal talk condition produces a
type-51 request on this branch. This is a concrete producer, not evidence
that the byte is invalid or that every talk starts in this branch.

## EndEvent selector

The event-type table at `0x008a149c`, used by `0x008a13a0`, maps type `51`
to case `7` at `0x008a141a`. That case calls `0x008a10c0`. Its selector
table at `0x008a10e4` has these targets:

| Selector | Target | Effect |
|---|---|---|
| `0`, `1`, `4`, `5` | `0x008a10e1` | Return without changing the pending block. |
| `2` | `0x008a10d7` -> `0x008a0640` -> `0x006e1100` -> `0x00893660` | Admit the matching pending block. |
| `3` | `0x008a10dc` -> `0x008a0660` -> `0x006e1120` -> `0x008937a0` | Remove the matching pending block. |

The two active thunks call `0x0078dee0` to convert the server-order type back
to its condition kind. Type `51` converts to kind `1`. Admission at
`0x00893660` requires a pending block, matching condition kind through its
virtual method at `+0x18`, and matching event name through `0x00445d20`.
Parser `0x0076c3b0` reads payload dword `0` at `0x0076c414`. Constructor
`0x0089d070` stores it at receiver `+0x08` at `0x0089d0b1`.
Receiver `0x0089e2d0` resolves the player identified by this actor field.
Thunk `0x006e1100` reads that actor's `+0xf8` to select its PlayerManager.
The reply must identify that player and echo the request type and event name.
The request's NPC owner is not the EndEvent actor field, and `0x00893660`
does not compare an NPC owner ID.

Admission sets `+0x1d` directly when there is no prior active block. Otherwise
it sets `+0x1c` and requests or awaits the prior block's completion. If the
old block's virtual readiness check at `+0x10` succeeds, it returns with
`+0x1d` clear and waits for completion. The cancellation path calls
`0x00cd09a0` at `0x0089373b`. For a thread with suspended frames,
`0x00cd09a0` reaches `0x00ccd560`, which installs the checker vtable
`0x0110e504`. Its method at `0x00ccfce0` returns the cancellation result
stored at `0x0130ced0`.

The scheduler compares this result at `0x00cf1d0c` and calls `0x00cce1c0`.
That function calls `0x00ccd910(1)` at `0x00cce21a`. Case `1` notifies the
registered listener through vtable `+0x08` at `0x00ccda3b`.
`0x00cd0940` registers the listener through `0x00ccdda0`; its fourth
argument is the PlayerManager supplied by the block launcher `0x008962c0`.
PlayerManager's listener at `0x00896260` routes the block-thread token to
`0x00893b00`, which clears the old active block and sets `+0x1d` when
admission has set `+0x1c`.
The normal completion handler at `0x00893ab0` performs the same latch
transition. The trace establishes these paths, not completion of every
possible prior-thread state.

The common tail of MyPlayer's method at `0x0070a350` calls `0x00893410`
at `0x0070a518`. Once `+0x1d` is set, `0x00893410` moves the pending
`+0x0c` block to active `+0x08`, clears the pending slot, and invokes its
virtual method at `+0x20`. The client-side block implementation at
`0x00896950` calls `0x008962c0`, which invokes the condition callback.
The normal talk callback can then make its own type-1 request and wait for
the server delegate. Admission is a distinct exchange before that delegate.

## Interpretation boundary

Dispatching a delegate against a type-51 request skips this admission exchange.
Ending that request with selector `0` cannot compensate, because its target is
the explicit return above. This establishes a protocol mismatch independently
of any added delay, guessed patch, or interpretation of a failed delegate's
return parameters. It does not prove the full cause of a particular live lock.

The third word of outgoing EventUpdate is not a unique failure discriminator.
Builder `0x0075e670` copies a caller-supplied signed byte into that word at
`0x0075e710`. The constant byte `1` at `0x012c3f72` is supplied by multiple
paths, including RunEventFunction gate failure, a later failure branch, and
continuation of an active block. A live sender return address and context are
needed to distinguish them.

## Reproduction

Read the listed ranges and data tables directly from an authorized matching
executable. This minimal decoder emits local disassembly only:

```python
import hashlib
from pathlib import Path
import capstone
import pefile

raw = Path("orig/ffxivgame.exe").read_bytes()
assert hashlib.sha256(raw).hexdigest() == (
    "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
)
pe = pefile.PE(data=raw)
base = pe.OPTIONAL_HEADER.ImageBase
decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for va, size in ((0x00897310, 0x1f0), (0x0075e510, 0xc0),
                 (0x0078de50, 0x6c), (0x008a13a0, 0xc4),
                 (0x008a10c0, 0x24), (0x008a0640, 0x40),
                 (0x006e1100, 0x40), (0x00893660, 0x140),
                 (0x008937a0, 0x140), (0x00893410, 0x81)):
    offset = pe.get_offset_from_rva(va - base)
    for insn in decoder.disasm(raw[offset:offset + size], va):
        print(f"{insn.address:08x} {insn.mnemonic} {insn.op_str}")
```

Inspect the conversion tables at `0x0078debc` and `0x0078df4c`, the event
tables at `0x008a149c` and `0x008a1464`, and the selector table at
`0x008a10e4` using the same PE translation. The notification chain above
provides the additional function locators for a scheduling-path review.

The [EndEvent receiver](end-order-event-receiver.md) owns the outer dispatch
layout. [PlayerManager dispatch](../net/dispatcher-subscriber-swap.md) owns
the active and pending field layout.
