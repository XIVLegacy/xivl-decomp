# Actor action queue and motion dispatch

This page maps the seven client classes that queue a character action, drive
its motion, and render its visual result. The per-class storage location in
`CharaActor` remains unresolved.

## The action subsystem (7 classes)

The "action" subsystem in `App::Scene::Actor::Chara::*` handles the
entire pipeline of "character executes a battle command":
queueing -> controller orchestration -> motion playback -> visual
rendering.

| Class | Vtable RVA | Slots | Role |
|---|---|---:|---|
| **`CharaActionQueBase`** | `0xc3e37c` | 14 | Abstract base for action queues |
| **`CharaActionQue`** | `0xc3e428` | 14x2 = 26 (multi-inh) | Concrete action queue |
| **`CharaActionPreLoadQue`** | `0xc3e3b8` | 14 | Pre-load queue (caches resources for upcoming actions) |
| **`CharaActionController`** | `0xc3e468` | 5 | Orchestrator (drives execution) |
| **`Status::CharaActionMotionController`** | `0xbe7fb4` | 4 | Motion playback driver |
| **`CharaActionVisualBase`** | `0xbe4434` | 32 | Abstract base for action visuals |
| **`CharaActionVisual`** | `0xbe4544` | 32 | Concrete visual (mesh/material/effects during action) |

Each class has its OWN vtable (set at `[this]` in its constructor),
so they're allocated as **separate heap-or-inline objects** that
CharaActor holds pointers to - NOT inline-embedded at known offsets
in CharaActor's body.

## Multi-inheritance on `CharaActionQue` (26 slots)

`CharaActionQue` shows 26 slots in the slot dump because it has
**two vtables** (multiple inheritance). Pattern:

- Each slot index 0..13 has TWO entries (one from primary base
  `CharaActionQueBase`, one from a secondary base - possibly an
  `IActionQueueListener` interface).
- 13 of the slots have CharaActionQue-specific overrides (the *
  marker in the slot dump). That's a substantial amount of
  queue-specific behaviour: enqueue, dequeue, peek, clear, validate,
  serialize, etc.

## Slot maps (class-specific overrides only)

`CharaActionController` (5/5 slots all unique to this class):
- `slot[0]` = FUN_008462f0 - destructor
- `slot[1]` = FUN_008450b0 - candidate `Init` or `Update`; role not established
- `slot[2]` = FUN_00844080 - small (just past CharaActionQue::slot9)
- `slot[3]` = FUN_00845430
- `slot[4]` = FUN_00844090 - small (16 B sibling of slot[2])

`CharaActionMotionController` (4/4 slots all unique):
- `slot[0]` = FUN_007a0bd0 - destructor
- `slot[1]` = FUN_007ac9c0
- `slot[2]` = FUN_007a0be0 - small (16 B after slot 0)
- `slot[3]` = FUN_007c5940

`CharaActionPreLoadQue` (9 unique + 5 inherited):
- 9 unique overrides at slots 0, 2, 3, 4, 5, 6, 7, 8, 9
- Slots 1, 10, 11, 12, 13 inherited from QueBase

The **5-slot Controller + 4-slot MotionController** pair is the
"narrow waist" - small interfaces driving the larger Que / Visual
machinery.

## Pipeline (inferred)

```
Battle Command arrives  ->  PreLoadQue resolves resources
                               (BattleCommand metadata,
                                animation pack ID, VFX,
                                sound bank, etc.)
                                       |
                                       down
                          ActionQue enqueues the action
                                       |
                                       down
                          ActionController dispatches
                          (drives the state machine)
                                       |
                          +-----------+------------+
                          down                          down
              MotionController                   ActionVisual
              (skeletal animation)               (mesh + VFX render)
```

The PreLoad step matches FFXIV's well-known "pre-cast resource
download" behaviour (visible in client log files when entering a
new zone - the client pre-loads action animations for nearby
characters).

## Per-class storage location in CharaActor - not established

CharaActor must hold pointers to these subsystems. None of the
class vtable VAs are written into a `CharaActor + offset` slot in
CharaActor's ctor - confirmed via grep for
`MOV [ESI+disp32], <action_vtable>`. So CharaActor stores
**pointers** (set via separately-called `new`-style allocators),
not inline-embedded objects.

The exact storage offsets remain unidentified. Their derivation point is the
allocation pattern
`CALL operator_new; PUSH ...; CALL <class_ctor>; MOV [ESI+offsetX], EAX`
in each constructor caller, including callers of `FUN_008462b0` for
CharaActionQue.

## Practical impact for client

When the server sends a `BattleAction` or equivalent packet:
1. The client's `CharaActionQue` enqueues the action.
2. The `PreLoadQue` ensures the relevant motion + VFX resources
   are loaded.
3. `ActionController` dispatches the queued action.
4. `ActionMotionController` plays the skeletal animation
   (mapped from the action's motion-pack ID).
5. `ActionVisual` renders mesh / VFX overlays.

## Cross-references

- `docs/actor/architecture.md` - actor and battle architecture
- `docs/actor/status-controllers.md` - status controller map
- `include/actor/chara_actor.h` - CharaActor field-offset catalog
