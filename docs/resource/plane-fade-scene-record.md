# Fade record and the PlaneFade filter

The client path from the fade record to the
`Application::Scene::Actor::System::GameManagerActor::PostEffectController::PlaneFade`
filter. The Lua fade natives and the record's sender arguments are in
`xivl-client-structs:docs/fade-actor-group-native-contract.md`.

## Evidence identity

- Binary: retail 1.23b `ffxivgame.exe`, size `15996808`, SHA-256
  `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
  base `0x00400000`. Addresses are VAs.
- Tools: ghidra-cli 0.2.2 with Ghidra 12.1.4. Decompiler output was a lead.
  RTTI names are those of
  [`config/ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json).
- Tiers: STATIC is read from instructions or data. INFERRED needs the stated
  assumption. UNKNOWN is not established.

## Sender and receiver

STATIC. The `Application::Main::Element::System::GameManagerElement`
constructor `0x005600C0` initializes element `+0x23C` and `+0x244` and sends
the scene packet `0x0D` under its element identity with the 4 byte actor type
`0x0E` (`0x00560115..0x00560163`). The sender `0x005606B0` posts record 0
with 0x14 bytes, four floats and a signed frame count, through `0x004D7980`
and then writes its receiver's `+0x23C` and `+0x244`. That its receiver is the
GameManagerElement is INFERRED from those two fields. `0x004D7980` adds `0x15`
to the record number, so the packet command is `0x15` and the record number at
the actor is 0
([record path](../actor/chara-look-at-consumers.md#record-path)).

STATIC. The scene actor factory table entry for type `0x0E` is `0x0079F8E0`,
which runs the `Application::Scene::Actor::System::GameManagerActor`
constructor `0x00614AE0`. The factory at container `+0x54` stores that actor
at its `+0x18`, container `+0x6C`. The actor's vtable `0x00FB8134` holds the
record handler `0x00613B10` at offset `0x274`.

STATIC. The actor constructor builds the `PostEffectController` at actor
`+0x160` (`0x00614B76..0x00614B7C`). The controller constructor `0x00621090`
builds the PlaneFade filter at controller `+0x04`
(`0x006210C6..0x006210CF`).

## Record 0

STATIC. The record 0 case (`0x00613B45..0x00613BA0`) copies the four floats,
turns the frame count `n` into `n * 300000 / 30` with signed integer
division, and calls `0x0060FA50` on actor `+0x164`, the PlaneFade filter. That
routine stores the four floats at filter `+0x08`, the converted duration at
`+0x18`, and sets the filter flag dword at `+0x04` to
`(flags & ~1) | 2`.

INFERRED, from the receiver identity above. The record that the fade natives
post therefore drives this filter.

## Controller update

STATIC. `0x006125E0` is the controller update. When filter flag bit `0x02` is
set, it calls `0x00A59F30` on the object at the global `0x01365560` with the
four floats and the duration, and clears the bit
(`0x006125EB..0x00612619`). While flag bit `0x01` is clear it asks the same
global object through `0x00A59920`, which returns its byte `+0x39`. A nonzero
answer sends message `0x19` with the 4 byte value 1 through `0x007CE7B0` and
sets bit `0x01` (`0x0061261D..0x0061264B`).

STATIC. `0x006140C0` calls the update at `0x006141C8` for the actor at
container `+0x6C`, after two tests of other objects. The ordinary camera update
reaches `0x006140C0` at `0x00619B3A` when camera byte `+0x480` is nonzero.
Rel32 calls or jumps to `0x006140C0` also stand at `0x00614DC6`,
`0x006158F3`, `0x00616301`, `0x0061731D` and `0x006177FD`.

INFERRED. The global object is the engine fade object that the filter hands
its target to, and `0x00A59920` reports that its fade finished. Its class was
not established.

## Unresolved

- Where the engine fade object draws in the frame, and whether it draws before
  or after the [name plate window](../actor/window-actor-anchor-producer.md)
  draw, are UNKNOWN. The filter path above does not settle which pixels the
  fade covers.
- The meaning of the four floats as colour and alpha stays INFERRED, as the
  fade contract states.
- The route of message `0x19` back to the element was not traced. The fade
  contract above records that `0x0055DA80` clears the sender's `+0x244` byte
  on the argument `0x19`. That the update's message is that argument is
  INFERRED.

## Reproduction

Hash the executable. Read the constructor sends at `0x00560115`, the factory
entry stores in `0x007A0450` and the type switch of `0x007A02D0`, the actor
vtable cell `0x00FB83A8`, and the ranges named above.
