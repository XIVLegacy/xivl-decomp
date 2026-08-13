# Director / Quest hierarchy - per-class decomp

This page maps `DirectorBaseClass`, `QuestDirectorBaseClass`,
`SimpleQuestBattleBaseClass`, and representative per-instance scripts including
OpeningDirector, QuestDirectorMan0g001, and QuestDirectorCom0l601.

## Headline finding: per-instance scripts are nearly empty

**Most per-instance
director / quest scripts are class registrations with little or no
custom code.** All meaningful behavior lives in the engine bases.

Examples:
- `OpeningDirector.lpb` (15 lines): registers `OpeningDirector
  extends DirectorBaseClass`, defines an empty `init()` override.
- `QuestDirectorMan0g001.lpb` (8 lines): just class registration,
  zero method overrides.
- `QuestDirectorCom0l601.lpb` (17 lines): registers the class,
  overrides `getOwnClientQuestIdAsSimple()` to return `111406` (the
  quest ID). That's it.

Server-driven events (KickEvent / EventStart / EndEvent / etc.) 2. The DirectorBaseClass
machinery handling those events 3. The accompanying Quest script's `processEvent*` /
`processTtr*` methods 4. The work-table state-sync layer

The per-instance director's Lua file just exists to make the class
registry happy.

## DirectorBaseClass - the canonical Director API (412 lines)

`/Director/DirectorBaseClass.prog` exposes the full Director surface
that every concrete director inherits. Method inventory:

### Work-table API

| Method | Purpose |
|---|---|
| `getTempWork(self, key)` | Read transient work field |
| `setTempWork(self, key, val)` | Write transient work field |
| `getSaveWork(self, key)` | Read persistent work field |
| `setSaveWork(self, key, val)` | Write persistent work field |
| `getSyncWork(self, key)` | Read server-synced work field |
| `updateSyncWork(self, ...)` | Push sync-work update onto wire |
| `processUpdateWork(self, ...)` | Process incoming wire work-update |
| `_onUpdateWork`, `_updateWork` | Internal hooks |
| `initWork(self, save_init, temp_init, sync_init)` | Initialize all 3 work-table slots |
| `initWorkSyncTag(self, ...)` | Set up sync-tag mapping for work fields |

### Work-table type system

Work-table fields are declared with type tags:

```lua
self.directorWork._temp = {
    {"directorId", "integer32"},        -- typed field
    {"_assignForChild", 240},           -- 240-byte child-assigned slab
}
self.directorWork._sync = {
    {"contentCommand", "integer32"},
    {"contentCommandSub", "integer32"},
    {"syncBuffer", "array", 128, "boolean"},   -- bool[128]
    {"_assignForChild", 64},                   -- 64-byte child slab
}
self.directorWork._tag = {
    {"contentCommand", 1, {"contentCommand"},
     {"contentCommandSub"}, {"syncBuffer"}},   -- tag-group binding
}
```

Type names observed: `integer32`, `boolean`, `array` (with element
count + element type). Plus the meta-marker `_assignForChild` which
reserves bytes for subclass-assigned fields.

The wire format (BE serializer + dirty counter) on the binary side maps directly to
these Lua type declarations.

### Lifecycle

| Method | Purpose |
|---|---|
| `_onInit(self, director_id, ...)` | Engine-internal init; calls `_callSuperClassFunc("_onInit")`, sets up directorWork sub-tables, then `self:init(...)` |
| `init(self, ...)` | **Abstract** - subclasses override to do per-director init. OpeningDirector / QuestDirectorMan0g001 leave this empty |
| `_init(self, ...)` | Internal alternate init |
| `_onFinalize(self)` | Calls `processUIFinalize`, `processFinalize`, then resets player content-command-variation if non-zero |
| `processFinalize(self)` | **Abstract** - subclasses do per-director cleanup |

### UI lifecycle

| Method | Purpose |
|---|---|
| `processUIInit(self, ...)` | UI setup (called once at director start) |
| `processUIUpdate(self, ...)` | Per-frame UI tick |
| `processUIFinalize(self)` | UI teardown (called from `_onFinalize`) |
| `closeAllOwnedContentWidget(self)` | Force-close all widgets the director created |

### Event / RPC

| Method | Purpose |
|---|---|
| `_onEventCancel(self, ...)` | Hook for event-cancel |
| `_onNoticeRejected(self, ...)` | Hook for client-rejected notice |
| `_callFunction(self, fn_name, ...)` | Reflective method call (helper) |
| `_callSuperClassFunc(self, method_name, ...)` | The standard super-call idiom |

### Content-command variation

| Method | Purpose |
|---|---|
| `getContentCommandVariation(self)` | Returns `(directorWork.contentCommand, directorWork.contentCommandSub)` |
| `setContentCommandVariation(self, ...)` | Set the contentCommand pair |
| `getUseContentsCommand(self)` | (in QuestDirectorBaseClass) - returns `worldMaster._getMyPlayer().getQuestContentsCommandPermitFlag()` |
| `getKindContentsInformation(self)` | Returns the content-info kind tag |
| `recordRequestInformation(self)` | Record an info-request from the player |
| `canRequestInformation(self)` | Predicate: can the player request info now? |

### Other

| Method | Purpose |
|---|---|
| `processMapOpenMessage(self, ...)` | Hook for the player opening the map (relevant for journal/map markers) |
| `_resetFade(self)` | Reset the fade-overlay state (used between cinematics) |
| `_getMyPlayer()` | (worldMaster method) - get the local player handle |

## QuestDirectorBaseClass (85 lines)

`/Director/Quest/QuestDirectorBaseClass.prog` extends
`DirectorBaseClass` and adds the quest-bound director surface:

```lua
function QuestDirectorBaseClass:init(...)
    self.questDirectorWork._temp = {{"_assignForChild", 16}}
    self.questDirectorWork._sync = {{"_assignForChild", 32}}
    if self:getOwnClientQuestId() ~= nil then end  -- presence check
    self:initAsQuestDirector(...)
end

function QuestDirectorBaseClass:initAsQuestDirector(...)  -- abstract
end

function QuestDirectorBaseClass:getUseContentsCommand()
    return worldMaster:_getMyPlayer():getQuestContentsCommandPermitFlag()
end

function QuestDirectorBaseClass:getOwnClientQuestId()  -- abstract
end

function QuestDirectorBaseClass:processFinalize()
    self:getOwnClientQuestId()
    -- (empty conditionals - likely cleanup of per-quest state)
end
```

Adds a second work table (`questDirectorWork`) on top of the base
`directorWork`, both with `_temp` (16-byte child slab) and `_sync`
(32-byte child slab). These are SMALLER than the base director's
slabs (240 / 64 bytes) - quest-specific state is leaner than
director-wide state.

`getUseContentsCommand` - reads the player's "is content-command
enabled?" flag. This is the gate for whether the player can issue
combat commands during a content-instance scene (e.g., during the
SEQ_005 combat tutorial in man0g0).

## SimpleQuestBattleBaseClass (43 lines)

`/Director/Quest/SimpleQuestBattle/SimpleQuestBattleBaseClass.prog`
extends `QuestDirectorBaseClass`. The base for **60 combat-tutorial
quest directors** (every "fight a single mob" tutorial in the game).

```lua
function SimpleQuestBattleBaseClass:eventContentGiveUp(A1, target)
    return worldMaster:ask(self, worldMaster, 25230, 2, target)
end

function SimpleQuestBattleBaseClass:getOwnClientQuestId()
    return self:getOwnClientQuestIdAsSimple()
end

function SimpleQuestBattleBaseClass:getOwnClientQuestIdAsSimple()  -- abstract
end
```

`eventContentGiveUp` - the "are you sure you want to give up?" dialog
when a player tries to abandon the combat tutorial. Calls
`worldMaster:ask(...)` with text ID `25230` and ask-mode `2`. The
target argument is the player handle.

`getOwnClientQuestId` delegates to `getOwnClientQuestIdAsSimple`
which subclasses override with a quest-ID literal. So the entire
per-quest customization is "what's my quest ID."

## Concrete subclasses

### `OpeningDirector` - for cinematic dispatch (15 lines)

```lua
require("/Director/DirectorBaseClass")
_defineClass("OpeningDirector", "DirectorBaseClass")

function OpeningDirector:init(A0)  -- empty
end
```

The CLIENT-side OpeningDirector is just a marker class - all behavior is in
DirectorBaseClass + server-driven events.

### `QuestDirectorMan0g001` - the man0g0 combat-tutorial director (8 lines)

```lua
require("/Director/Quest/QuestDirectorBaseClass")
_defineClass("QuestDirectorMan0g001", "QuestDirectorBaseClass")
-- (no method overrides)
```

Notable: **inherits from `QuestDirectorBaseClass` directly, NOT
from `SimpleQuestBattleBaseClass`.** The man0g0 combat tutorial uses
the simpler base. The 60-subclass `SimpleQuestBattleBaseClass`
hierarchy is for class-quest combat tutorials (Gcl/Gcg/Gcu prefixes
= Gladiator / Goldsmith / Gladiator-Ul'dah class tutorials).

### `QuestDirectorCom0l601` - a representative SimpleQuestBattle subclass (17 lines)

```lua
require("/Director/Quest/SimpleQuestBattle/SimpleQuestBattleBaseClass")
_defineClass("QuestDirectorCom0l601", "SimpleQuestBattleBaseClass")

function QuestDirectorCom0l601:getOwnClientQuestIdAsSimple()
    return 111406
end
```

The full per-quest customization: just the quest ID number. All other
behavior shared via the base classes.

## Implications for client's SEQ_005 work

The decomp confirms several things:

## Director sub-namespaces (from corpus survey)

The full Director hierarchy under `/Director/` (28 root subclasses
of DirectorBaseClass + 115 QuestDirectorBaseClass subclasses):

```
/Director/DirectorBaseClass.prog                             (412 LOC)
+-- OpeningDirector.prog                                     (15 LOC empty)
+-- /Director/Quest/QuestDirectorBaseClass.prog              (85 LOC)
|   +-- QuestDirectorMan0g001..030.prog                      (8 LOC each)
|   +-- QuestDirector{Bsm,Cls,Job,Etc,...}*.prog
|   +-- /Director/Quest/SimpleQuestBattle/SimpleQuestBattleBaseClass.prog (43 LOC)
|   |   +-- QuestDirectorGcu30101.prog                       (17 LOC)
|   |   +-- QuestDirectorCom0l601.prog                       (17 LOC)
|   |   +-- ... (60 subclasses total - all just override
|   |            getOwnClientQuestIdAsSimple)
|   +-- ... (~115 QuestDirector* subclasses)
+-- /Director/Guildleve/GuildleveBaseClass.prog              (32 subclasses)
+-- /Director/InstanceRaid/InstanceRaidBaseClass.prog        (12 subclasses)
+-- ... (other director subtypes)
```

## Cross-references

- `docs/script/lpb-format.md` - wrapper format + filename cipher
  (prerequisite for this decomp)
- `docs/script/lpb-corpus.md` - corpus-wide patterns + pipeline
- `docs/event/director-quest-framework.md` - architectural overview (this
  doc grounds the abstract findings there in concrete decomp)
- `docs/event/director-base-hooks.md` - the C++-side
  DirectorBase slot map; this doc shows the Lua-side companion)
- `docs/net/sync-writer.md` - the C++-side typed
  serializer; the work-table type tags `integer32` / `array` /
  `boolean` here map directly to those types)
