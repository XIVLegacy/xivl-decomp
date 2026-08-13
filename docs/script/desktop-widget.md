# `DesktopWidget` decomp - the engine's UI control plane

This page maps `DesktopWidget.lpb`, the top-level client UI controller for
targeting, log and message pools, user configuration, macros, and widget
containers. The corpus contains 3,154 references, compared with 3,100 for
WorldMaster.

## File inventory

| File | LOC | Purpose |
|---|---:|---|
| `DesktopWidget.lpb` (main) | 402 | Top-level: lifecycle hooks + mode control + macro/widget command dispatch |
| `DesktopWidget_connector.lpb` | **26,564** | The "connector" - cross-class API access surface (1,135 distinct method references) |
| `DesktopWidget_itemDetail.lpb` | 10,687 | Item-detail UI rendering |
| `DesktopWidget_materia.lpb` | 410 | Materia attachment / detachment UI |
| `DesktopWidget_u.lpb` | 472 | C++ binding declarations (43 `_method_cpp/_inl` pairs) |
| `DesktopUtil.lpb` | 39 | Utility helpers (separate class) |

**Total: ~38KB of Lua source** - about 10x larger than WorldMaster.
The size lives almost entirely in `_connector` (26.5K LOC) and
`_itemDetail` (10.7K LOC). Main + materia are slim wrappers.

`WidgetBaseClass.lpb` (the parent) adds another ~4KB:

| File | LOC | Purpose |
|---|---:|---|
| `WidgetBaseClass.lpb` (main) | 421 | Widget lifecycle + UI command dispatch |
| `WidgetBaseClass_common.lpb` | 3,361 | Shared widget primitives |
| `WidgetBaseClass_u.lpb` | 241 | C++ bindings (24 pairs) |

## C++-bound API (43 methods, from `_u`)

Per the corpus-wide `_cpp/_inl` enumeration in
`docs/script/lpb-corpus.md`, DesktopWidget's 43 engine-bound
methods organize into clear domains:

### Widget container ops (8)

| Method | Role |
|---|---|
| `_createWidgetInWidgetContainer(self, ...)` | Spawn a widget into a named container |
| `_deleteCreatingWidgetInWidgetContainer(self, ...)` | Cancel an in-progress widget creation |
| `_isCreatingWidgetInWidgetContainer(self, name)` | Predicate: creation in flight? |
| `_isExistWidgetInWidgetContainer(self, name)` | Predicate: widget present? |
| `_getWidgetFromWidgetContainer(self, name)` | Lookup by name |
| `_getWidgetContainerSize(self, ...)` | Container item count |
| `_reserveWidgetContainer(self, ...)` | Pre-allocate slots |
| `_setKeyboardFocusedWidget(self, w)` / `_getKeyboardFocusedWidget(self)` | Keyboard-focus management |

### Target cursor / lock-on (12)

| Method | Role |
|---|---|
| `_getTargetCharacter(self)` | Current target |
| `_setTargetCharacter(self, char)` | Set hard target |
| `_setTargetCharacterByDisplayName(self, name)` | Set target by name |
| `_getCharacterByDisplayNameForTextCommand(self, name)` | Resolve name -> char (for `/target Name`) |
| `_getCurrentTargetCursor(self)` / `_setCurrentTargetCursor(self, c)` | Active cursor slot |
| `_initTargetCursors(self)` | Reset all cursors |
| `_setTargetCursorImage(self, ...)` | Cursor sprite |
| `_setLockonCursorImage(self, ...)` | Lock-on sprite |
| `_setAllTargetCursorMask(self, mask)` | Bulk visibility mask |
| `_lockTargetCursorControl(self)` / `_unlockTargetCursorControl(self)` | Lock cursor control to a widget |
| `_isTargetCursorControlEnabled(self)` | Predicate |
| `_setTargetableDistance(self, dist)` | Max-targeting range |

### User config (4)

| Method | Role |
|---|---|
| `_getUserConfig(self, key)` | Read config field |
| `_setUserConfig(self, key, val)` | Write config field |
| `_resetUserConfig(self)` | Reset to defaults |
| `_saveUserConfig(self)` | Persist to disk |

### User macros (8)

| Method | Role |
|---|---|
| `_getUserMacroData(self, idx)` / `_setUserMacroData(self, idx, data)` | Macro body (the command sequence) |
| `_getUserMacroIcon(self, idx)` / `_setUserMacroIcon(self, idx, icon)` | Macro icon |
| `_getUserMacroTitle(self, idx)` / `_setUserMacroTitle(self, idx, title)` | Macro title |
| `_saveUserMacro(self)` | Persist all macros |

### Log / message pool (3)

| Method | Role |
|---|---|
| `_appendLogPool(self, ...)` | Append to log pool (chat / system messages) |
| `_appendMessagePool(self, ...)` | Append to message pool (popups) |
| `_clearLogPool(self)` | Wipe log |

### Misc UI control (8)

| Method | Role |
|---|---|
| `_parseTextCommand(self, text)` | Parse `/command arg1 arg2` text input -> action |
| `_sendCountDown(self, ...)` | Trigger countdown UI element |
| `_getLastAttacker(self)` | Most-recent damager (combat target tracking) |
| `_waitForCameraTutorial(self)` | Block until camera-tutorial completes |
| `_waitForItemSearchWidget(self)` | Block until item-search widget closes |
| `_waitForTargetTutorial(self)` | Block until target-tutorial completes |

## Lua-side API (DesktopWidget main, 402 LOC)

The main file's overrides + Lua-defined methods include:

### Lifecycle hooks

- `_onInit(self)` - calls `_callSuperClassFunc("_onInit")`, sets up
  the work table
- `_onLoop(self)` - per-frame tick (UI update loop)
- `_onPreWarp(self)` / `_onPostWarp(self)` - fired before / after a
  zone change
- `_onPreCutSceneCancel(self)` / `_onPostCutSceneCancel(self)` -
  fired around cutscene-cancel events
- `_onCreatedWidgetInWidgetContainer(self, widget)` - observer for
  widget-creation events

### Mode control

| Method | Purpose |
|---|---|
| `desktopMode` | Field: current desktop mode |
| `getModeLevel(self)` | Read mode level |
| `orderDesktopWidgetMode(self, mode)` | Request mode change |
| `cancelDesktopWidgetMode(self, mode)` | Cancel pending mode (used in man0g0.lua line 244-ish for cutscene cleanup) |
| `initDesktopInitialParameter(self, ...)` | One-time init |

### Macro / widget commands

| Method | Purpose |
|---|---|
| `commandMacro(self, macro_data)` | Execute a user macro |
| `cancelMacroCommand(self)` | Cancel running macro |
| `isMacroCommandPlaying(self)` | Predicate |
| `commandCreateWidget(self, ...)` | Create a widget by command |
| `cancelWidgetCommand(self)` | Cancel widget creation |
| `isCreateWidgetCommandPlaying(self)` | Predicate |
| `commandAboutWidget(self, ...)` | "About" command on a widget |
| `cancelCommandAboutWidget(self)` | Cancel |
| `isCommandAboutWidgetPlaying(self)` | Predicate |
| `getSystemCommand(self, ...)` | System-command lookup |
| `createWidget(self, name, ...)` | Direct widget creation entry |

### Forwarding / wrappers

| Method | Forwards to |
|---|---|
| `showMessage(self, ...)` | Wraps `_appendMessagePool` |
| `showLog(self, target, kind, ...)` | Wraps `_appendLogPool` (called by `worldMaster:say` with kind=32, `worldMaster:notify` with kind=33) |
| `notify(self, ...)` | Convenience wrapper |
| `cancelAllTarget(self)` | Clears all targeting via `_setTargetCharacter(nil)` etc. |
| `cueAttentionOnClient(self, ...)` | Trigger attention cue |
| `openPublicInformDialogWidget(self, ...)` | Open the public-info popup |
| `recordRequestInformation(self, ...)` | Record info-request action |
| `updateBazaarPackage(self, ...)` | Bazaar inventory refresh |

## DesktopWidget_connector (26,564 LOC, 1,135 method refs)

The connector is the **cross-class API integration surface**. It
references methods from many other classes - `_chat`, `_countMember`,
`_countStack`, `_getEquippingItem`, `_getMember`, `_getNetStatUser`,
`_haveEnmityCharacters`, `_isAttached`, `_isDealing`, `_isEquipping`,
plus dozens of widget-container ops, target ops, etc.

This file is the implementation of how DesktopWidget composes the
underlying engine bindings into higher-level UI behaviours. It's
the LARGEST single Lua file in the entire shipped script corpus
by a wide margin.

## DesktopWidget_itemDetail (10,687 LOC)

Item-detail rendering - the UI that pops up when you mouse-over
or right-click an item. References:
- `_getCatalogID(self, item)` - item's catalog ID
- `_getMaxStack(self, item)` - stack-size cap
- `_isEquipping(self, item)` - predicate
- `getAttachedMateriaCount(self, item)` - materia count
- `_format(self, ...)` - string formatting
- (plus ~hundreds more)

## WidgetBaseClass - the parent (425 LOC main + 3,361 _common)

WidgetBaseClass is the abstract widget base. Methods include:

### Lifecycle

- `_onInit(self, ...)`, `_onFinalize(self)`, `_onLoop(self)`,
  `_onTimer(self, dt)`, `_onHoverHelp(self, ...)`
- `_setLoopInterval(self, ms)` - per-widget tick rate
- `_setParentWidget(self, parent)` - parent in hierarchy
- `processFinalize(self)` - cleanup

### UI command dispatch (the talk-flow's UI side)

- `_onUICommandEvent(self, evt)` / `_onUICommandRequest(self, req)` -
  paired event/request hooks
- `processUICommandEvent(self, ...)` / `processUICommandRequest(self, ...)`
- `processWidgetCreated(self, ...)` / `processWidgetDeleted(self, ...)`

### Form / sheet loading

- `_loadForm(self, ...)`, `loadFormData(self, ...)`
- `_loadKeyTemporarily(self, key)` / `_loadMultiKeyAsync(self, ...)`
- `_onLoadMultiKeyAsync(self, ...)`
- `loadSpreadSheetDataAsync(self, ...)`
- `processSpreadSheetDataAsync(self, ...)` /
  `processSpreadSheetDataLoaded(self, ...)`
- `requestLoadSpreadSheetData(self, ...)`
- `requestSsdLoadSheet(self, ...)` /
  `requestSsdLoadKeyMin/Max(self, ...)`

### Sub-targets

- `executeSubTarget(self, ...)`, `requestSelectSubTarget(self, ...)`
- `processSubTargetDecided(self, ...)`

### Work table

- `desktopWidgetWork`, `commonTimer`
- type tags: `actor`, `boolean`, `integer32`, `nesting`, `select`

## Implications for client

### User config / macro persistence is client-side

The `_saveUserConfig` / `_saveUserMacro` family writes to local disk on the client, not
back to the server.

### `_parseTextCommand` is the slash-command entry

When the user types `/target Name` or `/macro 5`, the engine calls
`desktopWidget:_parseTextCommand(text)` on the client. The parsed action then becomes a
Lua call into the appropriate handler.

### `_waitFor*Tutorial` blocks are tutorial-flow gates

Three tutorials are surfaced as blocking primitives:
- `_waitForCameraTutorial` (camera control onboarding)
- `_waitForTargetTutorial` (targeting onboarding)
- `_waitForItemSearchWidget` (item-search UI demo)

These are called from tutorial scripts (the SimpleQuestBattleBaseClass hierarchy
decomped in `docs/event/director-quest-hierarchy.md`) to pause the script until the player
completes the corresponding tutorial.

## Cross-references

- `docs/actor/world-master.md` - companion (the other major Lua
  global; WorldMaster wraps DesktopWidget for `say`/`notify`)
- `docs/script/lpb-corpus.md` - the 43 desktopWidget declarations are
  the third-largest C++ surface in the corpus
- `docs/event/director-quest-hierarchy.md` - DirectorBaseClass uses
  desktopWidget.closeAllOwnedContentWidget for cleanup
- `docs/actor/scenario-monster-hierarchy.md` - QuestBaseClass's cinematic
  primitives all wrap desktopWidget calls
