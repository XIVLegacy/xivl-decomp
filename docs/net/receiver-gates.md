# Receiver gates for two-slot receivers

This page classifies the dispatch pattern and gate semantics of slot 1,
`Receive`, for all 36 two-slot Network and System receivers. It pairs with the
LuaActorImpl slot map in
`docs/net/actorimpl-receiver-dispatch.md`.

## TL;DR - 36 of 36 2-slot Receivers classified

| Pattern | Count | Recognition signal |
|---|---:|---|
| **A1.0** (guarded dynamic_cast) | 3 | `CALL FUN_009da6cc; TEST EAX,EAX; JZ <skip>; CALL <handler>` |
| **A1.1** (unguarded dynamic_cast) | 24 | `CALL FUN_009da6cc; CALL <handler>` - NO null-check |
| **A2.1** (pack-and-forward) | 5 | Read several fields from payload + receiver state, PUSH args, CALL one handler. No actor type validation. |
| **A2.2** (engine-root forwarding) | 3 | `CALL FUN_00cc7510` (navigate engine root) + downstream calls. Not actor-bound. |
| **A2.3** (debug command parser) | 1 | `ExecuteDebugCommandReceiver`, 1136 B - heavy Utf8String parsing |

## A1 - `__RTDynamicCast` family (27 receivers)

Common shape:

```c
auto ctx = arg0;                               // packet context
auto target = __RTDynamicCast(ctx, 0, src_TD, tgt_TD);  // FUN_009da6cc
// A1.0 path - null check:
if (target == NULL) return;                    // gracefully skip
// A1.1 path - no null check, direct dispatch:
target->doSomething(...);                      // non-virtual member fn
```

The cast normalises a `Component::Lua::GameEngine::LuaControl` (or `ActorBase`) input pointer to the specific subclass needed for the dispatch method. **In A1.1 receivers (the majority), if the cast fails (actor isn't of the expected subclass), the dispatcher invokes the handler with `this = NULL`** - typically silently no-ops or crashes depending on the handler.

### A1.0 - Guarded casts (3 receivers)

| Receiver | Target subclass | Post-cast | Notes |
|---|---|---|---|
| `SetDisplayNameReceiver` | `CharaBase` | `CALL FUN_006faff0` | Decoded in `docs/event/status-condition-receivers.md` (similar pattern) |
| `SetNoticeEventConditionReceiver` | `DirectorBase` | `CALL FUN_006f1380` | Has fallback to `ActorBase[+0x118]` on cast failure |
| `SendLogReceiver` | `CharaBase` AND `WorldMaster` | `CALL FUN_00772650` | TWO target classes - likely tries CharaBase first, falls back to WorldMaster |

### A1.1 - Unguarded casts (24 receivers)

All cast and dispatch without checking. Sorted by target subclass:

**Cast to `MyPlayer`** (local-player only - 12 receivers, "Half" of A1.1):
| Receiver | Handler (post-cast CALL target) |
|---|---|
| `AchievementPointReceiver` | `FUN_006e2dd0` |
| `AchievementTitleReceiver` | - (no direct CALL detected in 64-byte window) |
| `AchievementIdReceiver` | - |
| `AchievementAchievedCountReceiver` | `FUN_00704690` |
| `AddictLoginTimeKindReceiver` | - |
| `AttributeTypeEventEnterReceiver` | `FUN_006e11e0` |
| `AttributeTypeEventLeaveReceiver` | `FUN_006e1200` |
| `ChocoboReceiver` | `FUN_006de370` |
| `ChocoboGradeReceiver` | - |
| `GoobbueReceiver` | - |
| `VehicleGradeReceiver` | - |
| `SetCommandEventConditionReceiver` | `FUN_006f1c20` |
| `EntrustItemReceiver` | `FUN_006efbd0` |

**Cast to `CharaBase`** (any character with stats - 4 receivers):
| Receiver | Handler |
|---|---|
| `ChangeActorExtraStatReceiver` | `FUN_006fa980` |
| `ChangeSystemStatReceiver` | - |
| `ChangeActorSubStatModeBorderReceiver` | `FUN_006eecb0` |
| `SetDisplayNameReceiver` (A1.0 - listed above) | `FUN_006faff0` |

**Cast to `NpcBase`** (NPCs / mobs - 5 receivers):
| Receiver | Handler |
|---|---|
| `ExecutePushOnEnterTriggerBoxReceiver` | `FUN_00cc7510` (engine root navigate) |
| `ExecutePushOnLeaveTriggerBoxReceiver` | `FUN_00cc7510` (engine root navigate) |
| `HateStatusReceiver` | - |
| `SetEventStatusReceiver` | `FUN_006e67c0` |
| `SetTalkEventConditionReceiver` | `FUN_006f29b0` |

**Cast to `PlayerBase`** (any player - 3 receivers):
| Receiver | Handler |
|---|---|
| `JobChangeReceiver` | Re-calls __RTDynamicCast (multi-cast variant) |
| `GrandCompanyReceiver` | - |

**Cast to `AreaBase`** (zones / hamlets - 1 receiver):
| Receiver | Handler |
|---|---|
| `HamletSupplyRankingReceiver` | `FUN_006f3310` |

**Cast to `DirectorBase`** (directors - 1 receiver):
| Receiver | Handler |
|---|---|
| `SetNoticeEventConditionReceiver` (A1.0 - listed above) | `FUN_006f1380` |

The "-" entries indicate the post-cast scan didn't find a direct `CALL rel32` within 64 bytes; these receivers likely just `RET` after the cast (the cast itself triggers the side effect via subclass-bound logic during RTTI walk), OR the handler is reached via a conditional path the scan missed. Worth follow-up Ghidra GUI on these for completeness.

## A2 - Inline dispatch (9 receivers)

### A2.1 - Pack-and-forward (5 receivers)

Read payload fields + receiver state, PUSH multiple args, CALL one
handler. No actor-type validation; the dispatch routing presumes the
caller has already validated.

| Receiver | Size | Handler | Args |
|---|---:|---|---|
| `ChangeShadowActorFlagReceiver` | 32 B | `FUN_006dbe50` | 2 args (packed byte + receiver ptr) |
| `SetEmoteEventConditionReceiver` | 32 B | `FUN_006f2a90` | 3 args (word at +0x5a, +0x59, +0x58) |
| `SetPushEventConditionWithCircleReceiver` | 80 B | `FUN_006f2b70` | 9 args (geometry: floats + bytes at +0x60..+0x68) |
| `SetPushEventConditionWithFanReceiver` | 96 B | `FUN_006f2c30` | 11 args (geometry: floats at +0x60/+0x6c/+0x70 + bytes) |
| `SetPushEventConditionWithTriggerBoxReceiver` | 80 B | `FUN_006f2d00` | 9 args (geometry: short at +0x6c + bytes + 4-byte at +0x5c) |

The three `SetPushEventCondition*` variants are clearly the SAME shape
with different geometry payloads (circle = radius+center, fan =
radius+angle+direction, triggerbox = bounding-box). Each dispatches to
its own 0x2f2bxx handler family.

### A2.2 - Engine-root forwarding (3 receivers)

Navigate the engine context root via `FUN_00cc7510` (the
"engine root navigation" trampoline), then dispatch downstream. Not
actor-bound - these are "global" or "session-level" updates.

| Receiver | Size | First CALL | Downstream |
|---|---:|---|---|
| `HamletDefenseScoreReceiver` | 48 B | `FUN_00cc7510` (root nav) | `FUN_006f2210` |
| `SyncMemoryReceiver` | 144 B | `FUN_00cc7510` (root nav) | `FUN_00cc73b0`, `FUN_00775a30`, `FUN_00cc9330` |
| `SetTargetTimeReceiver` | 464 B | (math first - time conversion via `MUL/DIV` with constant 0x3e8 = 1000), then `FUN_00cc7510` (root nav) | `FUN_0035bda0`, `FUN_004a0370` |

### A2.3 - Debug command parser (1 receiver)

| Receiver | Size | Pattern |
|---|---:|---|
| `ExecuteDebugCommandReceiver` | 1136 B | Heavy Utf8String construction (`FUN_00447260` x N, `FUN_00046fb0` x N) - parses a GM/dev command string and dispatches via inline lookup |

This is the GM-only debug-command path; ordinary game opcodes don't go
through it. Probably runs only in dev/test builds OR when an authorized
client sends a debug payload.

## Practical gate cheat-sheet - for silent-drop debugging

For any silent-drop symptom, this map answers: "if my opcode X is
landing wire-side but the client doesn't react, what's the gate?"

```
opcode -> LuaActorImpl::slot (via docs/net/actorimpl-receiver-dispatch.md)
       -> Receiver (via slot map)
       -> Receive body pattern (this doc):
            Pattern A1.0 - guarded cast: client receives if actor IS-A target subclass; null-checks fall through silently
            Pattern A1.1 - unguarded cast: client dispatches with NULL on cast failure -> typically silent no-op
            Pattern A2.x - inline: client ALWAYS dispatches; gate (if any) is in the downstream handler
```

**Most likely silent-drop causes for each pattern**:

- **A1.1 (24 receivers)**: server sent payload targeting wrong actor type. E.g., sending an `AchievementPoint` packet to an NPC actor -> cast to `MyPlayer` fails -> handler runs with `NULL this` -> no-op. **Fix: server must verify actor type before emission.**

- **A1.0 (3 receivers)**: cast failed gracefully (null-check path). Same root cause as A1.1 but no crash risk. **Fix: same as A1.1.**

- **A2.1 (5 receivers)**: handler runs unconditionally. If client doesn't react, the downstream handler likely gates on some receiver-state flag set by an earlier packet. **Fix: trace what packet primes the receiver's state before this one.**

- **A2.2 (3 receivers)**: navigates engine root - most often "fire and forget" updates. Silent-drop likely means the engine root's downstream state isn't initialised. **Fix: check that prior session-setup packets landed.**

- **A2.3 (1 receiver - ExecuteDebugCommandReceiver)**: probably authentication-gated. Client won't run debug commands unless authorised. **Fix: not relevant for normal gameplay.**

## Application to SEQ_005

Cross-referencing the SEQ_005 cinematic packet sequence against this
map:

| Opcode | LuaActorImpl slot | Receiver | Pattern | Gate |
|---|---:|---|---|---|
| `0x012F` Kick | 56 | KickClientOrderEventReceiver | **B** (heap-alloc, 5-slot) | `actor[+0x5c] != 0` + 3-way state machine on `context_root[+0x128]/[+0x12c]` |
| `0x0130` RunEventFunction | 57 | StartServerOrderEventFunctionReceiver | **B** | `actor[+0x7d] != 0` |
| `0x0131` EndEvent | 58 | EndClientOrderEventReceiver | **B** | 102-case dispatcher, 12 active types |
| `0x0136` SetEventStatus | 48 | SetEventStatusReceiver | **A1.1** (unguarded cast to `NpcBase`) | If actor isn't NpcBase, handler dispatches with NULL -> silent no-op |
| `0x016B` SetNoticeEventCondition | (no LuaActorImpl slot - owned by event handler) | SetNoticeEventConditionReceiver | **A1.0** (guarded cast to `DirectorBase` with fallback to `ActorBase[+0x118]`) | If actor isn't DirectorBase, writes to `ActorBase[+0x118]` instead (the "orphaned conditions" hypothesis) |
| `0x0166..0x016A` SetPushEventCondition* | (no LuaActorImpl slot) | SetPushEventConditionWith*Receiver | **A2.1** (pack-and-forward geometry to `FUN_006f2bxx`) | None - always runs |

For the SEQ_005 hang, the critical conditions are:

- **Kick (`0x012F`)**: `+0x5c` flag plus
  `context_root[+0x128/+0x12c]` state. The clearer is
  `MyPlayer::vtable[66]`; see `docs/net/kick-dispatcher-clearer.md`.
- **SetEventStatus (`0x0136`)**: silent no-op if the target is not NpcBase.

## Cross-references

- `docs/net/receiver-class-inventory.md` - the 43 Receivers
  + their vtable RVAs; this doc walks the 36 of them that have 2-slot
  vtables; the other 7 are 5/6-slot receivers covered by separate
  decomps
- `docs/net/actorimpl-receiver-dispatch.md` - the 35 of
  42 Receivers mapped to LuaActorImpl/NullActorImpl slots; this doc
  reads as the receiver-side half of the dispatch path
- `docs/event/kick-order-event-receiver.md` - KickReceiver
  slot 2 + `+0x5c` gate; A1 family analogue for the 5-slot
  receivers
- `docs/event/start-event-fn-receiver.md`
- `docs/event/end-order-event-receiver.md` - 102-case
  dispatcher; analogue of A2.3 for 5-slot receivers
- `docs/event/status-condition-receivers.md`
  - SetEventStatus + SetNoticeEventCondition detail generalized here
- `docs/resource/dynamic-cast-hierarchies.md` - the
  481-callsite RTDynamicCast sweep that this doc filtered down to
  the 27 receiver-internal casts
- `build/dynamic_cast_callsites.json` - raw sweep output (input to
  this doc's analysis)
