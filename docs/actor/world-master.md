# `WorldMaster` decomp - the engine's world-event router

This page maps `WorldMaster.lpb` and the `worldMaster:say`,
`worldMaster:ask`, and `worldMaster:notify` calls used by quest and director
scripts. The corpus contains 3,100 references to this engine global.

## File inventory

| File | LOC | Purpose |
|---|---:|---|
| `WorldMaster.lpb` (main) | 135 | Engine lifecycle + JST calendar + cutscene wrappers |
| `WorldMaster_event.lpb` | 329 | The dialog/notification API (say, ask, notify, alert) |
| `WorldMaster_u.lpb` | 231 | **C++ binding declarations** (`_method_cpp` / `_method_inl` pairs for 45+ engine-bound methods) |
| `WorldBaseClass.lpb` | 1 | Empty marker |
| `OtherArea.lpb` | 1 | Empty marker |

So WorldMaster's effective API surface is 695 LOC across 3 files, with
the 45+ engine-bound methods documented in `_u` and the Lua-side
methods (the ones quest scripts actually call) in main + `_event`.

## The `_cpp` / `_inl` binding pattern

`WorldMaster_u.lpb` is a list of paired declarations:

```
_aimCameraChocobo_cpp        _aimCameraChocobo_inl
_aimCameraTutorial_cpp       _aimCameraTutorial_inl
_cancelAimCameraChocobo_cpp  _cancelAimCameraChocobo_inl
... (45+ pairs)
```

This is the **C++/Lua boundary marker** convention used throughout
the engine: any method declared in a `_u` file with `_<method>_cpp` +
`_<method>_inl` siblings is **engine-implemented in C++** and exposed
to Lua via the `LuaActorImpl`-style binding.

Calling pattern: Lua scripts invoke `worldMaster:_getMyPlayer()` ->
the engine routes the call to the C++ implementation `_getMyPlayer_cpp`.
The `_inl` variant is a pre-bound inline trampoline for hot calls.

This pattern means we can **enumerate the C++-bound API** for a
class by scanning its `_u` file. WorldMaster has **45+ such bindings**.

## C++-bound methods (from `_u`)

### Time / calendar

| Method | Likely role |
|---|---|
| `_getServerTime` | Real server clock (UNIX timestamp) |
| `_getHydaelynDay` / `_getHydaelynHour` / `_getHydaelynMoon` / `_getHydaelynTime` | Hydaelyn calendar accessors (in-game time) |

### Player / actor

| Method | Role |
|---|---|
| `_getMyPlayer` | Get the local player's handle (the most-called method in the binary) |
| `_getPendingCutSceneActor` | Get an actor instance pending in a scheduled cutscene |

### Tutorial control

These all take a `Tutorial` suffix - they're tutorial-specific
camera + chara-scheduler controls that bypass normal player input
during onboarding:

| Method | Role |
|---|---|
| `_aimCameraTutorial` / `_cancelAimCameraTutorial` | Force/release tutorial camera |
| `_lookAtPlayerTutorial` / `_cancelLookAtPlayerTutorial` | Force NPCs to look at player during tutorial |
| `_runCharaSchedulerTutorial` | Run a chara-scheduler step (NPC anim/move) in tutorial mode |
| `_waitForCharaSchedulerTutorialFinished` | Block script until scheduler finishes |
| `_isKeyboardOnlyTutorial` | Predicate: is the player in keyboard-only tutorial mode? |

### Chocobo

| Method | Role |
|---|---|
| `_aimCameraChocobo` / `_cancelAimCameraChocobo` | Chocobo-mounted camera control |
| `_transformIntoChocobo` / `_cancelTransformIntoChocobo` | Mount/dismount chocobo |

### Resource

| Method | Role |
|---|---|
| `_loadWord` / `_unloadWord` | Single-word text-data load/unload (vs. quest's `_loadKeySemipermanently` for whole keys) |
| `_getSpecialEventWork` | Get the special-event work table (seasonal events) |

### Diagnostics

| Method | Role |
|---|---|
| `_printLog` | Log message (always-visible) |
| `_printDebugLog` | Log message (debug builds only) |

## Lua-side methods (from main + `_event`)

These are the methods quest/director scripts actually invoke. They
are Lua-implemented wrappers that compose the C++ primitives.

### `WorldMaster.lpb` main (135 LOC)

| Method | Behaviour |
|---|---|
| `_onInit(self)` | Engine lifecycle hook |
| `_onReceiveDataPacket(self, ...)` | **Incoming data-packet handler** - engine calls this when a packet arrives addressed to WorldMaster |
| `getServerTimeWithDebugOffset(self)` | Server time + a debug offset (the `goto lbl_6` shape suggests a debug-only branch with a `time + offset` calculation that's compiled out in release) |
| `getJSTWeekAndDay(self)` | Current Japan Standard Time week + day-of-week |
| `calcJSTWeekAndDay(self, time)` | Convert a UNIX timestamp to JST (week, day). Internal: `floor((time + 9*3600) / (7*86400))` style |
| `getJSTWeekPastTimes(self, time)` | JST seconds-since-epoch (for seasonal-event windowing) |
| `createCutScene(self, name, ...)` | Schedule a cutscene by name (wrapper around C++ cutscene scheduler) |
| `CutScene` | Cutscene factory namespace |

### `WorldMaster_event.lpb` (329 LOC) - the dialog/notification API

This is **THE most-called API in the corpus** (3,100 hits). Every
quest/director's `worldMaster:say(...)` / `worldMaster:ask(...)`
call lands here.

#### Notification family

| Method | Body shape | desktopWidget call |
|---|---|---|
| `say(self, target, A1, A2, ...)` | Forwards to `desktopWidget:showLog(target, **32**, A1, A2, ...)` | Type 32 = "say" / dialog log |
| `notify(self, target, A1, A2, ...)` | Forwards to `desktopWidget:showLog(target, **33**, A1, A2, ...)` | Type 33 = system notify |
| `alert(self, target, A1, A2, A3, ...)` | Loop-based, more complex - assembles an alert dialog with multiple lines |
| `showMessage(self, ...)` | (referenced but not directly in `_event`'s methods - defined in NpcBaseClass_event) |
| `showLog(self, ...)` | (delegated to desktopWidget) |

So `worldMaster:say(player, text_id)` -> `desktopWidget:showLog(player, 32, text_id)`.

#### Dialog choice family

| Method | Args | Behaviour |
|---|---|---|
| `ask(self, A1, A2, A3, A4, A5, A6, ...)` | (target, npc, base_text_id, num_choices, ...) | Multi-arg choice prompt. Used by SimpleQuestBattleBaseClass:eventContentGiveUp to show "give up?" with text 25230. The `select("#", ...)` pattern at the top counts varargs. |
| `askRestrictChoices(self, A1, A2, A3, A4, ...)` | (target, npc, base_text_id, num_choices, ...) | "Restrict choices" variant - builds a `[1..num_choices]` range from `base_text_id`. Returns the player's selection. |
| `askMultipleTextMacro(self, ...)` | (target, ..., macro_args) | Multi-line text-macro variant |

#### Time predicates

| Method | Body |
|---|---|
| `isHydaelynNight(self, time)` | `if 5 <= hour < 19: return false (day) else true (night)` - uses `_getHydaelynHour(time)` |
| `getGuildleveTime(self)` | Returns `(window_seconds, hour_offset)` - formula: `floor(server_time/3600 / 12 + 1) * 12 * 3600` for window start, `11 - (hour % 12)` for offset |
| `getBoostTime(self)` | **Identical body to `getGuildleveTime`** - likely a copy-paste that was never specialized |
| `getAnimaTime(self)` | (similar pattern - Anima regeneration timer) |

The Hydaelyn-time function uses **hour 5 as dawn / hour 19 as dusk**
- Hydaelyn day runs roughly from 5am to 7pm in-game time. This is
the gate for night-only quest content.

## Implications for client

### `worldMaster:say(player, text_id)` -> `showLog(player, 32, text_id)` packet shape

Type 32 is the "dialog/say" message channel.

Type 33 is the "system notify" channel - used for system-level
notifications (level up, item received, etc.).

### Hydaelyn calendar - confirmed formulas

Hour 5 <= time < 19 = day (so the Hydaelyn day is **14 hours long**; night is 10 hours).

JST calendar: `floor((time + 9*3600) / (7*86400))` is the JST week
number - `9*3600` is the timezone offset (UTC+9 for JST). Used for
seasonal events (Foundation Day, Heavensturn, etc.).

### `_loadWord` / `_unloadWord` - single-word text resource API

In addition to `_loadKeySemipermanently` (whole-key load, in QuestBaseClass), there's a
single-word variant.

### `_isKeyboardOnlyTutorial` predicate

Tutorial scripts gate behaviour on whether the player is in keyboard-only mode. The
likely path: the player's `controlMode` work-table field is read by the C++
`_isKeyboardOnlyTutorial_cpp` impl.

### `_getMyPlayer` is the most-called method in the binary

`_getMyPlayer` is the local player handle accessor - every "do something to the player"
Lua script touches this.

## Cross-references

- `docs/event/director-quest-hierarchy.md` - companion (DirectorBaseClass +
  QuestDirector flow uses worldMaster heavily)
- `docs/actor/scenario-monster-hierarchy.md` - companion (Quest +
  Chara/Npc/Player bases similarly use worldMaster)
- `docs/script/lpb-corpus.md` - corpus-wide grep recipes
- `docs/script/lpb-format.md` - wrapper format + filename cipher
- `docs/script/lua-actor-impl.md` - the `_cpp`/`_inl`
  binding pattern's binary side; the 90-slot vtable on each actor
  type implements C++ methods exposed to Lua)
- `docs/script/lua-class-registry.md` - WorldMaster is
  one of the 12 engine/utility classes registered as a root)
