# Chocobo rental state and widget callback

This note records the rental-specific client contract in the pinned FFXIV
1.23b executable. The image has base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Rental packet ownership

The actor packet application switch at `0x0058cca0` handles opcode `0x0197`.
For the local player, it forwards the rental payload through `0x00575150`,
sets main state 15, and applies the Chocobo appearance. The callback-data path
is gated when the current main state is already 15.

Opcode `0x0197` is therefore both a rental-data producer and a mounted-state
producer. A separate opcode `0x0134` main-state publication is not required to
establish state 15 and can change which state request receives direct
initialization.

Both paths publish scene-state commands `0x39` and `0x3a`. `0x007b43e0`
initializes the first main-state request directly and queues later changes.
The queue initializer at `0x007b4440` and `0x007bf270` supplies a 300-unit
fallback at queue offset `+0x58`; `0x007c2170` counts it down and calls
`0x007bf4d0` to drain the pending state. This explains why packet ordering can
affect when the mounted model commits even when the rental callback data is
already accepted.

The readiness filter in `0x0058cca0` accepts `0x0197` while loading. Position
opcode `0x00ce` reaches `0x0058b2a0`, which starts arrival processing at
`0x0058a090`. Whether a particular server publication order makes the mounted
model visible at arrival remains a runtime outcome; the native ordering gates
do not prove it by themselves.

## Stored rental fields

The field writer at `0x006de370` stores:

| MyPlayer field | Meaning |
| --- | --- |
| `+0x158` | pending rental expiry |
| `+0x15c` | rental minutes byte |
| `+0x15d` | Chocobo grade |
| `+0x15f` | alternate pending flag |

Grade zero stores the supplied expiry and minutes. A nonzero grade clears the
pending expiry at `+0x158`. The two receiver thunks at `0x008a3020` and
`0x008a3100` zero-extend their one-byte fields before tail-calling the MyPlayer
field writers at `0x006de3b0` and `0x006de3c0`.

This distinction separates temporary rental initialization from the permanent
Company Chocobo grade path. An absolute expiry and an initial minutes value are
different inputs: the minutes byte seeds the initial presentation, while the
expiry preserves the authoritative deadline across later client processing.

## Mounted completion callback

`0x00703f60` processes main-state completion. When current main state is 15,
completion code 1 or 20, and signed expiry `MyPlayer+0x158` is positive, it
builds the rental callback arguments:

- the pending expiry;
- a notification flag which is true for completion code 20;
- the minutes byte at `+0x15c`.

It then dispatches the rental-ride callback and clears `+0x158`. A permanent
mount marker of `0xffffffff` does not satisfy the signed-positive expiry test.
Completion code 19 instead clears the pending expiry and follows the
non-rental mounted notification path using the grade byte.

The recovered Lua chain consumes this callback as follows:

```text
PlayerBaseClass rental callback
  -> DesktopWidgetConnector.processRentalChocobo
     -> widget 23
        -> ChocoboRentalTimerWidget
           -> CustomControl_TimerLabel duration
```

The Lua path opens the rental timer only for an unexpired rental and hides it
after dismount. This script behavior agrees with the native expiry and callback
gates; widget configuration alone cannot compensate for rental data that never
reaches the callback.

## Evidence boundary

The executable and recovered Lua prove packet ownership, field storage,
main-state queueing, callback eligibility, and the widget call chain. They do
not prove a universally correct server packet order for every map-loading path,
or the exact visible delay between callback, state commit, and model reveal.
Those historical runtime outcomes require an existing capture; no new retail
probe is possible or required for the static contract.
