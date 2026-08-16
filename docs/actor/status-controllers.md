# Actor status controllers

This page maps the client classes that control character states such as combat,
field movement, crafting, and sitting. The active-state pointer location
remains unresolved.

## Inventory

The `App::Scene::Actor::Chara::Status::*` namespace contains
**10 status controller types** plus a main dispatcher. Each
represents a distinct character state that can be active.

| Controller | Delegate-vtable count | Likely role |
|---|---:|---|
| `CharaMainStatusController` | 1 | Main dispatcher / state-machine root |
| `CharaStatusBattle` | 7 | Combat state (rich event surface) |
| `CharaStatusBattleChocobo` | 7 | Combat while mounted on chocobo |
| `CharaStatusField` | 7 | Field exploration / movement |
| `CharaStatusFieldChocobo` | 7 | Field movement on chocobo |
| `CharaStatusFieldRidden` | 7 | Being a chocobo carrying a player |
| `CharaStatusCraft` | 3 | Crafting (synthesis loop) |
| `CharaStatusGround` | 3 | On the ground (downed?) |
| `CharaStatusPic` | 3 | "Pic" - possibly portrait / posing |
| `CharaStatusSit` | 3 | Sitting |
| `CharaActionMotionController` | (4 slots, standalone RTTI) | Motion playback for the active action |

Plus 1 standalone-RTTI controller:
- **`CharaActionMotionController`** (vtable RVA `0xbe7fb4`, 4 slots)
  - drives motion playback; not a "state" but always-on. Slot map:
  - slot 0: `FUN_007a0bd0` - destructor
  - slot 1: `FUN_007ac9c0`
  - slot 2: `FUN_007a0be0`
  - slot 3: `FUN_007c5940`

## Delegate-richness pattern

The 1/3/7 delegate-vtable counts reveal each state's complexity:

- **1 delegate** (`CharaMainStatusController`): just the basic
  "state changed" notification - appropriate for the dispatcher
  that routes events to the active sub-state.
- **3 delegates** (Craft, Ground, Pic, Sit): minimal event surface -
  these are passive/locked states. A character that's sitting only
  needs hooks for "stand up," "interrupt," and one more.
- **7 delegates** (Battle, BattleChocobo, Field, FieldChocobo,
  FieldRidden): rich event surface for the active states. Combat
  needs hit / damage / death / target-switch / cast-start / etc.;
  field movement needs zone-cross / jump / fall-damage / etc.

This is consistent with a state-pattern architecture where each
state has its own event-handler set.

## Delegate construction pattern

Each delegate is a small 12-byte object: `{ vtable, fn_ptr,
bound_this }`. The construction function (e.g. `FUN_007c3c00` for
CharaStatusBattle delegates, 99 B) sets the fields in 2 stages:

```
new_delegate->vtable = 0xfe7f10;       // generic delegate base
new_delegate->vtable = 0xfe7f70;       // CharaStatusBattle-specific
new_delegate->fn_ptr = arg1;           // callback function
new_delegate->bound_this = arg2;       // bound `this` for the callback
```

The 2-stage vtable assignment is the standard MSVC base-then-derived
construction order; the binary preserves it because each stage has
distinct behaviour (the base vtable's destructor is registered for
SEH unwind between the two stages).

## RTTI quirk: status controllers only appear as template args

The 10 `CharaStatus*` controller classes do NOT have standalone
RTTI entries with their own vtables. Instead, they appear ONLY as
template arguments to `Delegate0X<...>::DelegateHolderDynamic`
specialisations. This means:

- The Status controllers are CONCRETE classes used in delegates.
- MSVC didn't emit standalone RTTI for them, presumably because
  they're never `dynamic_cast`-targeted.
- We can identify them BY NAME (via the delegate template arg) but
  can't directly find their constructors via the standard
  vtable-write-pattern grep - we'd need to find the delegate
  constructors and trace back to the controllers they embed.

## Active-state pointer

A character must have ONE active status controller at a time.
Where is the "active controller" pointer/index stored in CharaActor?

What we know:
- It's NOT `array_1690[10]` (initially hypothesised). Those are
  10 SSE-aligned 4x4 matrix transforms at `+0x16d0..+0x18b0`,
  populated by `FUN_00664890` calling vtable slot 0x60/4=24 on each
  array element to fetch a transform matrix. **`array_1690[10]`
  is bone/attachment-point transforms, NOT status pointers.**

The active pointer remains unidentified. Its derivation point is each
delegate-constructor caller, such as `FUN_007c3c00` for CharaStatusBattle.
The caller supplies an inline CharaActor address as `ESI + offsetX`; the
dispatcher should then expose a separate `current_state` enum or pointer.

The four candidate offsets below are now confirmed as inline,
non-polymorphic sub-objects with owner back-pointers, per the constructor and
destructor LEA evidence. This narrows but does not answer the active-pointer
question: an inline member cannot itself be the "which one is active" pointer,
so that pointer is a separate field still unidentified.

## Status-controller layout in CharaActor (partial)

Four CharaStatus-related fields are confirmed as inline sub-objects in
CharaActor:

- `subobj_0fc0`, `subobj_1030`, `subobj_1070`, `subobj_1110` -
  CharaActor's ctor and dtor use `LEA ECX, [ESI+0xN]; CALL <sub_ctor>`
  at both construction and destruction. Each is a non-polymorphic member
  with an owner back-pointer; the constructor addresses identify the
  corresponding controller sub-object.

## Cross-references

- `docs/actor/architecture.md` - high-level actor architecture
- `include/actor/chara_actor.h` - CharaActor field-offset catalog
  (the sub-objects at +0xfc0/+0x1030/+0x1070/+0x1110 are candidate
  status-controller storage)
- `land-sand-boat-server/xi-private-server.md` - XI's character
  states are similarly state-pattern (the Idle / Mounted / Engaged
  / Resting / Crafting / Dead / etc. enum is a direct cousin)
