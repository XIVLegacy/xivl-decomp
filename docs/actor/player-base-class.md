# `PlayerBaseClass` decomp - the player-side engine surface

This page maps `PlayerBaseClass.lpb`, its C++ bindings, Lua-defined methods, and
split work and clip-program files: about 73 KB across 7 files, with 94
C++-bound methods and 77 Lua-defined methods in the main file.

## LPB API layers

The 94 C++-bound declarations and 77 Lua-defined methods are independent API
layers in independent VMs. Inherited CharaBase declarations are not part of the
Lua method census.

## File inventory

| File | LOC | Purpose |
|---|---:|---|
| `PlayerBaseClass.lpb` (main) | **3,020** | The largest single base-class main in the corpus |
| `PlayerBaseClass_u.lpb` | 941 | 94 C++ binding declarations |
| `PlayerBaseClass_work.lpb` | 369 | Work-table accessors (`playerWork.guildleveId[N]`, etc.) |
| `PlayerBaseClass_cliprog.lpb` | 180 | Client-prog command-variation accessors |
| `PlayerBaseClass_craft.lpb` | 1 | Empty stub |
| `PlayerBaseClass_harvest.lpb` | 1 | Empty stub |
| `PlayerBaseClass_negotiation.lpb` | 1 | Empty stub |

**Total: ~73 KB of Lua source** - about 2x CharaBaseClass and
~10x WorldMaster. This is THE class.

`require()` ordering at the top of main:
```lua
require("/Chara/Player/PlayerBaseClass_craft")
require("/Chara/Player/PlayerBaseClass_harvest")
require("/Chara/Player/PlayerBaseClass_negotiation")
require("/Chara/Player/PlayerBaseClass_cliprog")
-- (then defines PlayerBaseClass methods directly in main)
```

The 4 split files compose into PlayerBaseClass via `require()`. The
3 empty stubs were probably aspect-files at one point that got
absorbed back into main.

## C++-bound API (94 methods, from `_u`)

Per the census in `docs/script/lpb-corpus.md`, organized by domain:

### Talk / emote / command lifecycle (15)

The paired `_callServer*` / `_doServer*` / `_execute*` /
`_cancel*` / `_break*` / `_can*` / `_count*` / `_is*Playing` /
`_is*PushingOut` family for player actions:

| Method | Role |
|---|---|
| `_executeTalk` / `_canExecuteTalk` / `_cancelTalk` | Talk action |
| `_executeEmote` / `_canExecuteEmote` / `_cancelEmote` | Emote action |
| `_executeCommand` / `_canExecuteCommand` / `_cancelCommand` / `_breakCommand` | Combat / system command |
| `_callServerOnCommand` / `_doServerOnCommand` | Server-RPC pair (per `docs/actor/scenario-monster-hierarchy.md`) |
| `_isCommandPlaying` / `_countCommandPlaying` | Active-command predicates |
| `_isPushingOut` | Player-being-pushed-out predicate |
| `_cancelPush` / `_cancelNotice` | Push/notice cancellation |

### Camera / lock-on / player control locks (10)

Locks that prevent normal input while a cutscene/tutorial runs:

| Method | Role |
|---|---|
| `_lockCameraControl` / `_unlockCameraControl` / `_isCameraControlEnabled` | Camera lock |
| `_lockLockonControl` / `_unlockLockonControl` / `_isLockonControlEnabled` | Lock-on lock |
| `_lockPlayerControl` / `_unlockPlayerControl` / `_isPlayerControlEnabled` | Movement lock |
| `_forceCameraTPSMode` | Force third-person camera |
| `_getLockonTarget` / `_setLockonTarget` | Lock-on target |

### Fade / cinematic primitives (9)

Mirrors QuestBaseClass_common's fade primitives - accessible from
PlayerBase too:

| Method | Role |
|---|---|
| `_fadeOut` / `_fadeIn` / `_fadeInAfterWarp` | Fade transitions |
| `_fadeInNowLoadingForNoticeEventJustInArea` | Fade with loading screen, area-context |
| `_cancelFading` / `_isFading` / `_resetFade` | Fade state |
| `_waitForFading` / `_waitForMapLoaded` | Block-on-fade primitives |

### Achievements / trophies (16)

Largest single domain in PlayerBase. Achievement system is a major
1.x sub-system:

| Method | Role |
|---|---|
| `_achieveTrophy` / `_canGetTrophy` / `_isAchievedTrophy` | Trophy completion |
| `_isDoneAchievement` / `_isDoneAchievementRateList` | Status predicates |
| `_clearAchievementRateCache` | Cache invalidation |
| `_countAchievementCategory` / `_countAchievementItem` / `_countAchievementRateList` | Counts |
| `_countEnableAchievementTitle` / `_getEnableAchievementTitle` | Title visibility |
| `_getAchievementCategoryId` / `_getAchievementItemId` / `_getAchievementPoint` | ID/point queries |
| `_getAchievementRate` / `_getAchievementRateList` | Progress rate |
| `_getAchievementSheetDataIcon/Item/Point/Title` | Spreadsheet data |
| `_getAchievementTitle` / `_setAchievementTitle` | Active title |
| `_hasAchievementItem` / `_hasAchievementTitle` | Predicates |

### Inventory storage (4)

| Method | Role |
|---|---|
| `_canStoreItem` | Predicate: can the player accept this item? |
| `_countStoredItem` / `_getStoredItem` | Stored-item queries |
| `_haveEnmityCharacters` | Has enmity-relationship characters |

### Cutscene replay (5)

Replay system for completed quests:

| Method | Role |
|---|---|
| `_isCompletedCutSceneReplayQuest` | Quest-completion predicate |
| `_getCutSceneReplaySnpcCoordinate/Nickname/Personality/Skin` | Snpc spawn data for replay |

### Hamlet defense / behest content (7)

Content-instance state queries:

| Method | Role |
|---|---|
| `_countHamletDefenseScore` / `_getHamletDefenseScore` / `_getHamletDefenseScoreAll` | Hamlet defense scoring |
| `_getNMRushUpdateTime` | NM (Notorious Monster) rush event timer |
| `_getCompanyBehestTime` / `_getNormalBehestTime` | Behest timers |
| `_getOccupancyContentsTime` | Occupancy content timer |

### Touch / movement (3)

| Method | Role |
|---|---|
| `_isTouching` / `_setTouchAttribute` | Touch-event state |
| `_turn` | Programmatic turn |

### Inn / homepoint (2)

| Method | Role |
|---|---|
| `_readyInnBed` | Trigger inn-bed UI |
| `_setPosDirInn` | Set inn position + facing |

### GC / chocobo / GM (4)

| Method | Role |
|---|---|
| `_getBelongGrandCompany` / `_getGrandCompanyRank` | GC state |
| `_getChocoboGrade` / `_getChocoboRidingGrade` | Chocobo training/riding levels |
| `_getGMRank` | GM rank |
| `_isEnabledGoobbue` | Goobbue mount predicate |

### Misc (5)

| Method | Role |
|---|---|
| `_chat` | Send chat message |
| `_setMusic` | Change BGM |
| `_setWeather` | Change weather |
| `_getWarpRecastTime` | Aetheryte warp cooldown |
| `_isEventPlaying` | Event-mode predicate |

## Lua-side API (PlayerBaseClass main, 77 methods)

The main file's 77 Lua-defined methods compose the C++ primitives
into higher-level operations:

### Identity / demographics (10)

`isPlayer`, `isMyPlayer`, `isValidName`, `isFemale`, `isMale`,
`getTribe`, `getNation`, `getGuardian`, `getBirthday`,
`getInitialTown`

### Quest state (7)

| Method | Purpose |
|---|---|
| `getScenarioQuest(self, idx)` / `getScenarioQuestLength(self)` | Scenario quest table |
| `getGuildleveQuest(self, idx)` / `getGuildleveQuestLength(self)` | Guildleve quest table |
| `isQuestComplete(self, quest_id)` | Per-quest completion |
| `updateQuestComplete(self, quest_id)` | Mark complete |
| `processUpdateQuestComplete(self, ...)` | Engine-side update handler |

### Grand Company state (6)

| Method | Purpose |
|---|---|
| `getGrandCompanyRank(self)` / `getGrandCompanyRankLinear(self)` | Rank queries |
| `getGrandCompanySealCount(self)` / `getGrandCompanySealMax(self)` | Seal economy |
| `getGrandCompanyNeedSealNextRank(self)` | Next-rank threshold |
| `isPrebelongGrandCompany(self)` | "Was previously a member" predicate |

### NPC linkshell (3)

| Method | Purpose |
|---|---|
| `hasNpcLinkshell(self)` | Has any NPC LS membership |
| `isNpcLinkshellChatCalling(self)` | Pending chat call |
| `getNpcLinkshellChatLinkshellLength(self)` | Active LS count |

### Command system - the biggest group (15)

| Method | Purpose |
|---|---|
| `command(self, ...)` | Top-level command dispatch |
| `delegateCommand(self, ...)` | Delegate to sub-handler |
| `canCommand(self, ...)` | Predicate |
| `_onCommandRequest(self, ...)` / `_onCommandEvent(self, ...)` / `_onCommandCancel(self, ...)` / `_onCommandRejected(self, ...)` | Server-RPC pairs |
| `_onPreCommand(self, ...)` / `_onPostCommand(self, ...)` | Pre/post hooks |
| `getCastCommand(self)` / `getCastEndTime(self)` | Active cast info |
| `getComboInformation(self)` | Battle Regimen state |
| `getOtherClassAbilityCountInformation(self)` | Cross-class ability counts |
| `setEmoteSitCommandVariation(self, ...)` | Emote-sit variation |

### Content command / content widget (8)

| Method | Purpose |
|---|---|
| `getQuestContentsCommandPermitFlag(self)` / `setQuestContentsCommandPermitFlag(self, flag)` | The combat-command gate (per `docs/event/director-quest-hierarchy.md`) |
| `setContentCommandVariation(self, ...)` | Set the active content-command set |
| `setPlaceDrivenCommandVariation(self, ...)` / `resetPlaceDrivenCommandVariation(self)` | Place-driven (e.g. inside an inn) command set |
| `commandAboutWidget(self, ...)` / `cancelCommandAboutWidget(self, ...)` / `processCancelCommandAboutWidget(self, ...)` | About-widget commands |
| `isCommandAboutWidgetPlaying(self)` | Predicate |
| `commandAboutDebug(self, ...)` | Debug command |

### Information requests (4)

| Method | Purpose |
|---|---|
| `recordRequestInformation(self, ...)` / `canRequestInformation(self)` | Record + gate info-request actions |
| `getGiftCountInformation(self)` | Gift-count display |
| `getRestBonusExpRate(self)` | Rest-bonus xp multiplier |

### Achievements / bonus (2)

| Method | Purpose |
|---|---|
| `isAcquiredAdditionalCommand(self, command_id)` | Predicate |
| `isRemainBonusPoint(self)` | Has unspent bonus points |

### Touch / movement event hooks (5)

| Method | Purpose |
|---|---|
| `_onTouch(self, ...)` | Touch event (see correction below) |
| `_onMoveAtSit(self, ...)` | Movement-while-sitting |
| `_onChocoboRentalRide(self, ...)` / `_onChocoboWarpRide(self, ...)` | Chocobo-mounting events |
| `_onGetGoobbue(self, ...)` | Goobbue-pet acquisition |

#### Correction: `_onTouch` is contextual-command activation

The hash-pinned handler has the decompiled form `function(self, A1, A2)` and is
assigned to `_onTouch`. Its bounded branches implement proximity-driven
contextual-command activation and support documenting the signature as
`_onTouch(self, selector, enter)`: the first argument selects behavior and the
second activates or clears that behavior. Source:
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`;
`sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`;
`lines=1502-1595`; `extraction=2012.09.19.0001`.

| Selector | Direct handler behavior | Hash-pinned evidence |
|---:|---|---|
| `1` | On activation, probes command `22004`; when that command exists and can fire, sets variation `30003` with priority value `5`. On clearing, resets the same variation and priority value. | `xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`; `sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`; `lines=1541-1578`; `extraction=2012.09.19.0001` |
| `2` | Sets sit variation `10002` on activation and clears it with `nil`. | `xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`; `sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`; `lines=1580-1590`; `extraction=2012.09.19.0001` |
| `5` | Inside an instance raid, obtains static actor `24301` and executes variation `30004` with mode `1` on activation or `2` on clearing. | `xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`; `sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`; `lines=1505-1536`; `extraction=2012.09.19.0001` |

Selectors `3`, `4`, and `6+` reach no selector-specific branch. Inside an
instance raid, the handler returns after the selector-`5` test, so every other
selector reaches no later branch. Source:
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`;
`sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`;
`lines=1510-1593`; `extraction=2012.09.19.0001`.

The command identities used by this flow are bounded by their decoded client
table namespaces:

| ID | Bounded identity | Retail-data evidence |
|---:|---|---|
| `22004` | `Fish`. | `xivl-client-data:manifests/tables.json#csv/xtx_command.csv`; `row=22004`; `extraction=2012.09.19.0001`; `xivl-client-data:derived/command_battle_params.csv#id=22004` |
| `22005` | `Herd`, as an identity only. | `xivl-client-data:manifests/tables.json#csv/xtx_command.csv`; `row=22005`; `extraction=2012.09.19.0001`; `xivl-client-data:derived/command_battle_params.csv#id=22005` |
| `30003` | `Fish` in the place-variation namespace. | `xivl-client-data:manifests/tables.json#csv/xtx_command_place.csv`; `row=30003`; `extraction=2012.09.19.0001` |
| `30004` | Its English table text is `***`, so no semantic label is assigned here. | `xivl-client-data:manifests/tables.json#csv/xtx_command_place.csv`; `row=30004`; `extraction=2012.09.19.0001` |
| `10002` | In the `csv/xtx_command_variableemote.csv` namespace, `10002` is `Sit`. | `xivl-client-data:manifests/tables.json#csv/xtx_command_variableemote.csv`; `row=10002`; `extraction=2012.09.19.0001`; `xivl-client-scripts:manifests/scripts.json#lua/scripts/command/system/emotesitcommand.lua`; `sha256=D30462749C4B93A6BCAF09C32A61AF8E55632C71BF713662C38167FAEDA6F513`; `lines=13-16`; `extraction=2012.09.19.0001` |
| `24301` | Static `PlaceDrivenCommand` actor used by selector `5`. | `xivl-client-data:manifests/staticactor_class_paths.json#id=24301`; `xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`; `sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`; `lines=1505-1536`; `extraction=2012.09.19.0001` |

Selector `1` does not reference `22005`; the bounded selector body references
`22004`, while the separate `PlaceDrivenCommand` mapping maps variation
`20004` to `22005`. Sources:
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`;
`sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`;
`lines=1541-1578`; `extraction=2012.09.19.0001`, and
`xivl-client-scripts:manifests/scripts.json#lua/scripts/command/system/placedrivencommand.lua`;
`sha256=720E1D329BDB1B89C40C51A00D256ABC45CC0B67CF3601062C5A1D5644E1C9DC`;
`lines=17-20`; `extraction=2012.09.19.0001`.

The hash-pinned schema declares four parallel contextual-command arrays on
`playerWork`:

| `playerWork` field | Declared type |
|---|---|
| `variableCommandPlaceDriven[4]` | `integer16` |
| `variableCommandPlaceDrivenSub[4]` | `integer32` |
| `variableCommandPlaceDrivenTarget[4]` | `actor` |
| `variableCommandPlaceDrivenPriority[4]` | `integer8` |

Source:
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`;
`sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`;
`lines=427-461`; `extraction=2012.09.19.0001`.

The setter and resetter manage at most four active entries through ordered
insertion and removal. The value `5` supplied by selector `1` is stored in
`variableCommandPlaceDrivenPriority`; the array slot is computed separately.
Source:
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`;
`sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`;
`lines=2250-2459`; `extraction=2012.09.19.0001`.

NPC `pushCommandIn` and `pushCommandOut` events call the player's
`setPlaceDrivenCommandVariation` and `resetPlaceDrivenCommandVariation`
methods, so they use the same four-entry state and API. Sources:
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/npc/npcbaseclass.lua`;
`sha256=F18C6C3198DB0402431CA6AA683B601AFBF5A0DC5C22481B3C1DA1DEABEF0DC2`;
`lines=528-552`; `extraction=2012.09.19.0001`, and
`xivl-client-scripts:manifests/scripts.json#lua/scripts/chara/player/playerbaseclass.lua`;
`sha256=6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`;
`lines=2250-2459`; `extraction=2012.09.19.0001`.

Local retail evidence bounds the engine side to two reviewed callers:

| Local evidence name | VA | Direct observation |
|---|---:|---|
| `Player_invokeLua_onTouch_primary` | `0x00898d20` | Validates the primary record at object offset `0x04` and invokes `_onTouch`. |
| `Player_invokeLua_onTouch_secondary` | `0x00898eb0` | Validates the secondary record at object offset `0x10` and invokes `_onTouch`. |

Source: `retail:ffxivgame.exe#sha256=9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9;va=0x00898d20,0x00898eb0`, recorded in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
These observations assign neither opcode nor enter/leave semantics to the two
paths. Source:
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json),
observations `Player_invokeLua_onTouch_primary` and
`Player_invokeLua_onTouch_secondary`.

### Login / event lifecycle (10)

| Method | Purpose |
|---|---|
| `_onInit(self, ...)` | Init |
| `_onLoginEvent(self, ...)` | Fired at login |
| `_onPreEvent(self, ...)` / `_onPostEvent(self, ...)` | Around event-mode entry/exit |
| `_onUpdateWork(self, ...)` | Work-table update hook |
| `_onReceiveDataPacket(self, ...)` | Generic data-packet receiver |
| `_onReceiveTimingPacket(self, ...)` | Timing-packet receiver |
| `_onReceiveAchievementId(self, ...)` / `_onReceiveAchievementRate(self, ...)` | Achievement updates |
| `_onReceiveLimitAddicted(self, ...)` | "Limit reached" notification |

### Misc (5)

| Method | Purpose |
|---|---|
| `getSystemCommand(self, ...)` | System-command lookup |
| `postMapOpen(self, ...)` | Post-map-open hook |
| `checkSameItemCatalogId(self, a, b)` | Item-id equality check |
| `getWarpRecastTime(self)` | Aetheryte warp cooldown (Lua wrapper for the C++ `_getWarpRecastTime`) |
| `decodeTimingPacketInformation(self, packet)` | Parse timing packet |
| `isEventPlaying(self)` | Event-mode predicate (Lua wrapper) |

## `_work` file - playerWork accessors (369 LOC)

Implements typed accessors for the `playerWork` work-table fields,
mostly for guildleve state:

```lua
function PlayerBaseClass:getGuildleveID(idx)
    return self.playerWork.guildleveId[idx]
end

function PlayerBaseClass:getGuildleveIndexMax()
    -- linear scan of playerWork.guildleveId[] for first 0
    for i = 1, #self.playerWork.guildleveId do
        if self.playerWork.guildleveId[i] == 0 then
            return i - 1
        end
    end
    return #self.playerWork.guildleveId
end

function PlayerBaseClass:isHavingGuildleveById(id)
    -- linear scan for matching ID
end
```

Other observed methods: `isHavingGuildleveCompletedById`,
`getGuildleveDoneCount`, etc. - all wrappers around
`self.playerWork.guildleveId[]` and `self.playerWork.guildleveDone[]`.

The `playerWork` is the **per-player Lua-side state mirror** - the
client's view of player state. Server-driven property updates feed
into this via the SyncWriter mechanism (per `docs/net/sync-writer.md`).

## `_cliprog` file - command variation accessors (180 LOC)

7 methods, all `getConfirm*CommandVariation` accessors:

| Method | Purpose |
|---|---|
| `getConfirmGroupCommandVariation(self)` | "Confirm group?" command set |
| `getConfirmRaiseCommandVariation(self)` | "Confirm raise?" command set |
| `getConfirmTradeCommandVariation(self)` | "Confirm trade?" command set |
| `getConfirmWarpCommandVariation(self)` | "Confirm warp?" command set |
| `getContentCommandVariation(self)` | Active content command set |
| `getEmoteSitCommandVariation(self)` | Emote-sit command set |
| `getPlaceDrivenCommandVariation(self)` | Place-driven command set |

These return a numeric ID identifying which command variation is
active for the given UI context (confirmation dialogs, content
instances, etc.).

## Implications for client

### Distinct engine and Lua APIs

The client LuaPlayer and engine PlayerBaseClass APIs serve different VMs:

- **Engine PlayerBaseClass** (94 C++ methods + 77 Lua wrappers) ->
  consumed by **client-side** shipped `.lpb` scripts via the
  binary's Lua VM

### Client-visible fields

- `playerWork.guildleveId[]` - array of active guildleve IDs
- `playerWork.guildleveDone[]` - paired completion flags
- `playerWork.questComplete[]` - quest-completion bit table
- `playerWork.scenarioQuest[]`, `playerWork.guildleveQuest[]`
- GC state: `playerWork.companyRank`, `playerWork.companySealCount`, etc.
- NPC LS state: `playerWork.npcLinkshell*`
- Achievement state: `playerWork.achievementTitle`, etc.
- **The combat-command gate is `getQuestContentsCommandPermitFlag`**
  (already documented in `docs/event/director-quest-hierarchy.md`). Setting
  `playerWork.questContentsCommandPermitFlag` on the player work table enables
  or disables combat commands during content instances like the man0g0 SEQ_005
  tutorial.

## Cross-references

- `docs/actor/scenario-monster-hierarchy.md` - companion to the
  `CharaBaseClass` + `NpcBaseClass` inheritance context for PlayerBaseClass
- `docs/event/director-quest-hierarchy.md` -
  `getQuestContentsCommandPermitFlag` flow
- `docs/actor/world-master.md` - sibling class (the other
  most-referenced engine global)
- `docs/script/desktop-widget.md` - closes the major-Lua-base
  trilogy plus PlayerBase
- `docs/net/sync-writer.md` - the wire mechanism that
  populates the playerWork fields that PlayerBaseClass methods read.
- `docs/script/lpb-corpus.md` - corpus pipeline
