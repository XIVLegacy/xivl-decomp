# Judge sheet globals

The globals `statusSheet` and `commandSheet` are not created by native code and
not by the area master. The static actor `JudgeMaster` creates them from its
`_onInit`, together with the sheets that its `CommonJudge` static actor
prepares. This page records the script chain, the order of the creations, the
readers that depend on them, and the limits of what is established.

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode of the installed client scripts. The script bodies are not tracked |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe`, as recorded on the cited tracked page |
| INFERRED | A role or ordering taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The binary is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The scripts are the `client/script` chunks of the same installation, unwrapped
by the rules in [LPB format](lpb-format.md) and listed instruction by
instruction as Lua 5.1 bytecode. Names such as `JudgeMaster` and `statusSheet`
are Lua constants, so they do not occur as strings in `ffxivgame.exe`. A scan
of the executable for each of them, as ASCII and as UTF-16LE, found none, so no
native code writes these globals by name.

The bytecode listing is the authority for every script fact below. A decompiler
rendering of `JudgeMaster._onInit` omits the `_getStaticActor(320001)` call that
the bytecode holds.

## The creation chain

### JudgeMaster

`xivl-client-data:manifests/staticactor_class_paths.json` maps static actor id
`310001` to `/Judge/JudgeMaster` and `320001` to `/Judge/CommonJudge`.
`WorldMaster._onInit` calls `_getStaticActor(310001)`, as
[Actor instantiation](../actor/actor-instantiation-and-self-player.md#static-actors)
records. `JudgeMaster` extends `JudgeBaseClass`. Its `_onInit` has 20
instructions and performs these steps in this order (OBSERVED):

| Step | Call |
|---:|---|
| 1 | `_callSuperClassFunc("_onInit")`, which runs `JudgeBaseClass._onInit` |
| 2 | `self:prepareSpreadSheet("command")` |
| 3 | `self:prepareSpreadSheet("status")` |
| 4 | `_getStaticActor(320001)`, the `CommonJudge`, with the result not stored |
| 5 | `_getTutorialJudge()`, which calls `_getStaticActor_cpp(320006)` |
| 6 | `_getStaticActor(320013)`, the `ChocoboJudge` |
| 7 | `_prepareAllCommandStaticActor()` |

`JudgeBaseClass._onInit` calls `_callSuperClassFunc("_onInit")`, then
`initText()`, then `init()` (OBSERVED). `JudgeBaseClass` defines `initText` and
`init` as empty methods, and `JudgeMaster` does not override either. The
tracked call-site files agree with the count of two `_getStaticActor` call
sites and with one `_prepareAllCommandStaticActor` site:
`xivl-client-scripts:lua/scripts/judge/judgemaster.calls.json`, lines 25, 30
and 33.

The shipped scripts call `_getTutorialJudge` once, at step 5. The statement in
`xivl-client-structs:docs/global-native-contract.md` that no shipped script
calls it holds only for natives, because the call-site index lists natives and
`_getTutorialJudge` is a script function defined in `global_u`.

### prepareSpreadSheet in the Judge classes

`JudgeBaseClass.prepareSpreadSheet(path, name)` has 35 instructions (OBSERVED).
When `name` is false or nil it takes the tail of `path` after the last `/`,
splits it on `_`, joins the pieces with `string:lowerCamelCase`, and appends
`Sheet`. It then returns `_createActor(name, "SpreadSheet", name ~= nil, path)`.
The reuse flag is therefore true for every call from the Judge classes.
Unlike the area variant described in
`xivl-client-structs:docs/create-actor-native-contract.md`, section Script
side, this variant does not test `_isExistActor` first. It leaves the
existing-name case to the reuse branch of `_createActor`.

`_createActor` publishes `globals[name]` for a nonempty name by a raw set and
runs `SpreadSheet._onInit(self, path)`, which calls `_setFilename(path)`
(STATIC, from the create-actor page). The creation core leaves an already set
global alone, as the
[Actor instantiation](../actor/actor-instantiation-and-self-player.md) page
records for `0x00CD7970`.

### Sheets created by the Judge classes

| Global | Table path | Creator |
|---|---|---|
| `commandSheet` | `command` | `JudgeMaster._onInit` step 2 |
| `statusSheet` | `status` | `JudgeMaster._onInit` step 3 |
| `itemDataSheet` | `itemData` | `CommonJudge.init` |
| `equipmentSheet` | `equipment` | `CommonJudge.init` |
| `weaponSheet` | `weapon` | `CommonJudge.init` |
| `armorSheet` | `armor` | `CommonJudge.init` |
| `accessorySheet` | `accessory` | `CommonJudge.init` |
| `gameCommandSheet` | `gameCommand` | `CommonJudge.init` |
| `gameCommandBasicSheet` | `gameCommandBasic` | `CommonJudge.init` |
| `compatibilitySheet` | `compatibility` | `CommonJudge.init` |
| the global derived from `exp_BPCost` | `exp_BPCost` | `CommonJudge.init` |

The paths and the call order of `CommonJudge.init` are OBSERVED. `CommonJudge`
defines no `_onInit`, so `JudgeBaseClass._onInit` reaches `init` through step 4
above. The global name follows from the derivation rule. The tracked rule maps
`guildleve_UI` to `guildleveUISheet` and `mapNavi_data` to `mapNaviDataSheet`,
so the pieces after the first keep their remaining characters. By the same rule
`exp_BPCost` gives `expBPCostSheet`, which is INFERRED because no reader of it
was found among the scanned scripts.

`compatibilitySheet` has a second creator, the area `loadCommonTableData`,
which uses the guarded variant. The Judge creation runs first (INFERRED from
the order below), so the area call returns the existing sheet.

The area master creates 22 other sheets and `cutReplaySheet`, and calls none of
`command`, `status` and the nine `CommonJudge` paths other than `compatibility`.
Those are listed on the create-actor page.

## Readers

All reads below are OBSERVED as `GETGLOBAL` of the lowercase name followed by a
method call on the sheet.

| Reader | Sheet | Calls |
|---|---|---|
| `StatusBaseClass._onInit` | `statusSheet` | after `_callSuperClassFunc("_onInit")` and `id = self:_getStaticActorID()`, `statusSheet:_loadKeySemipermanently(id, id)` |
| `StatusBaseClass._onFinalize` | `statusSheet` | after the super call, `statusSheet:_unloadKey(id, id)` |
| `StatusBaseClass` accessors | `statusSheet` | `statusSheet:_getData(self:getStatusId(), column)`, for example column 25 for the icon |
| `CommandBaseClass._onInit` | `commandSheet` | super `_onInit`, `id = self:_getStaticActorID()`, `commandSheet:_loadKeySemipermanently(id, id)`, then `self:init()` |
| `CommandBaseClass._onFinalize` | `commandSheet` | super `_onFinalize`, `commandSheet:_unloadKey(id, id)`, then `self:processFinalize()` |
| `CommandBaseClass.getCommandData` | `commandSheet` | `commandSheet:_getData(self:getCommandId(), column)` |
| `GameCommandBaseClass` | `gameCommandSheet`, `gameCommandBasicSheet` | `_getData`, `_loadKeySemipermanently` and `_unloadKey` |
| `GameCommandBaseClass` | `compatibilitySheet` | `_getData` at row `getCommandCompatibilityKey()` |
| `BattleCommandBaseClass`, `CraftJudge`, `ActionSettingWidget` | `gameCommandSheet` or `gameCommandBasicSheet` | reads by `GETGLOBAL` |

The first instruction of the compiled `CommandBaseClass._onInit` is an
unconditional jump over a short block that calls the super `_onInit` and
returns. The live path is the one in the table. A decompiler renders that jump
as `do break end` and `do return end`, which is not a guard.

Neither base class tests that its sheet exists. A static command or status
actor whose `_onInit` runs before the creator has published its sheet indexes a
nil global and the Lua call raises an error.

`_getStaticActorID` returns the actor identity masked with `0xFFFFF`
(`xivl-client-structs:docs/nameplate-group-native-contract.md`, section Actor
identity natives). That the result is the static actor id is INFERRED there. The
status ids run `221000` to `253003` and the command ids `12002` to `30101` in
`xivl-client-data:manifests/staticactor_class_paths.json`, and the tracked
sheet inventory holds the `status` and `command` tables that these keys select
(`xivl-client-data:manifests/sheet_inventory.csv`).

## Timing

`WorldMaster._onInit` yields on `_getStaticActor(310001)` until the new
control carries its ready flag (STATIC). In queue states 0 to 9 each bind waits
for the ready byte of the previous instance, and the order is `Debug`,
`WorldMaster`, the self player, then the area master (STATIC, from the
[creation order](../actor/actor-instantiation-and-self-player.md#creation-order)
section). The ready byte is set at the init-thread completion `0x00CCE1C0`.
The link from that completion to the return of `_onInit` is INFERRED.

If that link holds, `JudgeMaster._onInit` has returned before `WorldMaster`
completes, so `commandSheet`, `statusSheet` and the nine `CommonJudge` sheets
exist before `Player._onInit` and the area `_onInit` start. The two conditions
that would break it are NOT TRACED. Whether the ready flag of a static actor
waits for its `_onInit`, and whether `_createActor` resumes its caller only
after `SpreadSheet._onInit` has returned, are both open on the cited pages.

Step 7 depends on step 2. `_prepareAllCommandStaticActor` (`0x006E2000`) reads
the id vector at `0x0134B77C..0x0134B780`, which the load of the sheet named
`command` fills, and requests the static actors that do not exist
(`xivl-client-structs:docs/global-native-contract.md`, STATIC). The command
static actors therefore start after `commandSheet` exists. That page places the filler of the vector inside the load of the `command`
sheet. No Lua call in this chain is tied to it.

## Implementation contract

1. Treat `JudgeMaster` (static actor `310001`) as a script class whose
   `_onInit` must run on a yieldable coroutine, once, when the static actor is
   first created. `WorldMaster._onInit` starts it.
2. Run the steps of the `JudgeMaster._onInit` table in order. Each
   `prepareSpreadSheet` call is a `_createActor` with class `SpreadSheet`,
   reuse flag true and the table path as payload.
3. The creating call publishes `globals[name]` and runs `SpreadSheet._onInit`,
   so `_setFilename` receives the path. The path selects the table by schema
   name, as the area sheets do.
4. Run `CommonJudge._onInit` (static actor `320001`) from step 4. It prepares
   the nine paths of its table in order.
5. Row data is not part of creation. The rows come from the yielding load
   natives that the readers call with the static actor id as both key
   arguments.
6. A second creation of a name that already exists returns the existing sheet
   through the reuse flag.
7. Create these sheets before the self player and the area master run their
   `_onInit`. This ordering is INFERRED.

## Not traced

- Whether the ready flag of a static actor waits for the return of its
  `_onInit`.
- Whether `_createActor` for a `SpreadSheet` resumes the creating thread before
  or after `SpreadSheet._onInit` returns.
- What triggers the fill of the command id vector read by
  `_prepareAllCommandStaticActor` during the `command` sheet load.
- The global name produced for `exp_BPCost`, which is derived by rule only.
- The bodies of `init` in `GameCommandBaseClass` and its subclasses. They may
  read further globals.
- Whether an `_onInit` that raises an error still sets the ready byte.

## Reproduction

The script facts are read with the unwrap rules of
[`tools/decode_lpb.py`](../../tools/decode_lpb.py) and a Lua 5.1 bytecode
lister over `judge/judgemaster`, `judge/judgebaseclass`, `judge/commonjudge`,
`status/statusbaseclass`, `command/commandbaseclass` and
`command/game/gamecommandbaseclass`. The executable scan is a byte search of
`ffxivgame.exe` for `commandSheet`, `statusSheet` and `JudgeMaster` in ASCII
and UTF-16LE.
