# Active job and soul-stone state

The retail 1.23b client represents an active job as a byte on each player
actor. Soul-stone possession is a separate package-101 item predicate.
The `/job` text command and Equipment screen submit command 12017; neither
request path establishes server acceptance, item acquisition or a learned
ability grant.

## Evidence

Native observations use `ffxivgame.exe`, 15,996,808 bytes, preferred image
base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses below are preferred VAs. Generated Ghidra assembly was
used as a lead; the cited instructions and callback-name bytes were decoded
again from file-backed PE sections with pefile and Capstone 5.0.7.
These are static observations, without a recorded retail execution.

Decoded Lua references use extraction `2012.09.19.0001` and canonical
`xivl-client-scripts:lua/scripts/` paths. Their byte identities are recorded
by `xivl-client-scripts:manifests/scripts.json`; CSV identities are recorded
by `xivl-client-data:manifests/tables.json`. The source boundaries of
[Actions and Traits](../script/actions-traits-menu.md) and
`xivl-client-scripts:docs/quest-script-inventory.md` also apply here.

## Active state and notification

[Player profile work accessors](../event/player-profile-work-accessors.md)
owns the s2c 0x01A4 routing and `_getJob` binding. The receiver at
`0x0089CB60` passes the payload byte to setter `0x00706DC0` on the routed
PlayerBase actor. The getter at `0x00706D90` returns the sign-extended byte
at `PlayerBase+0xF1`; non-player CharaBase and NpcBase getters return nothing.

The PlayerBase constructor at `0x006ED720` calls its base at `0x006ED748`,
zeros EAX at `0x006ED74D`, installs vtable `0x00FD5E04` at `0x006ED753`,
and stores zero to the job byte at `0x006ED77D`.
The separate initializer at `0x006F0220` reads its input byte at `+0xC6`
at `0x006F026E` and writes the actor's job byte at `+0xF1` at
`0x006F0275`. That direct store bypasses the changed-job setter and its
callback. These are distinct initialization and changed-value paths.

The changed-job setter has these effects:

| Site | Observation |
| --- | --- |
| `0x00706DEE-0x00706DFD` | Read the old byte, compare the incoming byte and exit on equality. |
| `0x00706E03` | Store the incoming byte at `+0xF1`. |
| `0x00706E67-0x00706E87` | Sign-extend the stored new byte and add its numeric value to the argument collection. |
| `0x00706E8C-0x00706E9C` | Sign-extend the saved old byte and add its numeric value next. |
| `0x00706EA8` | Select native string `_onChangeJob` at `0x00FD4CDC`. |
| `0x00706EC5-0x00706ED0` | Pass the actor, callback name and argument collection to `0x00CC7A90`. |

The numeric collection helper `0x00585020` calls `0x0078DD00`, whose
fast path writes at the current end pointer and advances it by four at
`0x0078DD29-0x0078DD37`; `0x00584F70` constructs the numeric value through
`0x00583D90`. The arguments are therefore prepared in new-job, old-job
order. An unchanged byte does not reach this notification path.

The decoded `CharaBaseClass._onChangeJob(self, newJob, oldJob)` at
`chara/charabaseclass.lua:1496-1508` calls `self:isMyPlayer()` and has an
empty conditional body. It does not use either job argument, write learned
state, or forward a widget/action refresh. This describes the recovered
method body and setter tail, without claiming that no other update can
refresh those surfaces.

The [Equipment refresh callers](../script/equipment-soulstones.md#open-window-refresh-callers)
establish separate item/work callback routes to an already open Equipment
window. The empty changed-job callback does not select those routes. A job-byte
transition alone therefore does not prove header, stone-icon or focus refresh.

## Base class, active job and stone

`getMainClassOrJob` (`chara/charabaseclass_ffxivbattle.lua:89-110`) returns
`getStateMainSkill()` for non-players. For players it returns `_getJob()`
when nonzero and otherwise the base class from `getStateMainSkill()`.
It does not test stone possession or require that the nonzero byte is one
of the seven recognized jobs.

`isJob` accepts 15 through 19 and 26 through 27 (`:223-237`).
`convertSkillId` maps each class/job pair in both directions and returns 0
for an unmatched value (`:240-314`). `getJobItemId` maps a job to its
stone, returning nil for unmatched values (`:398-444`).

The [Equipment soul-crystal table](../script/equipment-soulstones.md#important-item-backing)
records the class/job pairs, soul-item IDs, names and icons.

`hasJobStone` (`chara/charabaseclass_cliprog.lua:400-454`) repeats this
job-to-item mapping, returns false for an unmatched job, and calls
`hasItem(101, stoneId)` for a recognized job. `hasItem`
(`chara/charabaseclass.lua:1655-1689`) defaults the required stack to 1
and scans indices 1 through package capacity minus free space.
`checkSameItemInPackage` (`:1692-1717`) requires a nonnil item with an
equal catalog ID and a stack count at least that requirement.
This is a possession predicate, with no quest, active-job or learned-action
test in those methods.

Outside Equipment, the recovered consumers are:

| Consumer | Effect |
| --- | --- |
| `DesktopWidget.getPlayerSkillRank`, `widget/desktopwidget_connector.lua:17427-17456` | For a recognized job, return 0 if its stone is absent; otherwise convert to its base class and return `getSkillLevel(base)`. |
| `StatusWidget.setSkill`, `widget/statuswidget.lua:2280-2321` | For a job without its stone, set its `IconControl_` opacity to 0.5. |
| `PcMatchingFindWidget`, `widget/pcmatchingfindwidget.lua:1122-1168` | Outside its purpose-99 branch, use `hasItem(101, getJobItemId(skill))` for a job choice, or `isSkillEnabled(skill)` for a class; a failed predicate greys and disables that choice. |

The two other literal 12017 occurrences in
`widget/pcmatchingviewwidget.lua:544-550` and
`widget/pcmatchingeditwidget.lua:1437-1443` are arguments to `setText`
and `setContent`. They are UI text-row uses, rather than command submissions.

## Text command and macro requests

`xivl-client-data:csv/xtx__textCommand.csv`, row 328 (line 121), names
`/job`; the French columns also name `/activerjob`. The matching help row
in `xtx__textCommand_help.csv` describes `on`, `off`, and switching the
current class's associated job when no subcommand is supplied.
`xtx__subCommand.csv` rows 1 and 3 identify `on` and `off`.

`DesktopWidget.executeTextCommandLocal` handles command 328 at
`widget/desktopwidget_connector.lua:21619-21629`. It applies
`getSwitchFlag` to the first parsed argument. That helper (`:23045-23065`)
returns accepted/true for subcommand ID 1, accepted/false for 3, and
accepted/nil for an absent argument; other subcommands are rejected.
The accepted branch calls `executePlayerJobChange` and sets its own result
to true without copying the submission result.

`executePlayerJobChange` (`:15436-15450`) submits these values:

| Input | `executePlayerCommand` arguments |
| --- | --- |
| true, from `on` | 12017, 1 |
| false, from `off` | 12017, 2 |
| nil, from no subcommand | 12017, 0 |

The same text-command dispatcher rejects `/job` while `tutorialFlag` is
true (`:20184-20201`). Its control-lock/subtarget guard at `:20203-20239`
also excludes command 328 from the allowed set; the subtarget part has
the explicit `A8 == true` bypass shown there. These are dispatcher guards,
not reconstructed server restrictions on changing jobs.

`MacroCommand.fire` (`command/system/macrocommand.lua:24-67`) reads ten
macro lines and sends each nonempty line through
`desktopWidget:executeTextCommand(line, true, nil, previousType)`.
Thus a `/job` line reaches the same parsed text-command route. No distinct
job-specific macro request is defined by that method.

`executePlayerCommand` (`widget/desktopwidget_connector.lua:13617-13733`)
resolves a numeric command through `_getStaticActor`, rejects a nil or dead
command actor, resolves the local player and a target, then calls the player's
`command` method. It contains no job-specific possession check.

`CharaBaseClass.getCommandName` (`chara/charabaseclass.lua:399-418`) first
returns `commandJudgeMode` for `isDesktopCommandMode() == true`; otherwise
12017 and 12009 select `commandRequest` before the generic priority rules.
`CommandBaseClass.isDesktopCommandMode` returns false
(`command/commandbaseclass.lua:145-152`). Neither `GameCommandBaseClass`
nor `ChangeJobCommand` overrides it in the recovered definitions, so these
definitions select `commandRequest` for ChangeJobCommand.

`PlayerBaseClass.canCommand` (`chara/player/playerbaseclass.lua:1978-2024`)
exempts 12017 and 12009 from the existing `commandBurstBlocker` check and
skips `_canExecuteCommand` for `commandRequest`. It still calls the command's
`canFire`. `PlayerBaseClass.command` (`:1911-1975`) checks `canCommand`,
submits through `_executeCommand`, records request information and writes
`commandBurstBlocker = 1` afterward.

`ChangeJobCommand` only overrides `isUseActionGauge` to return false
(`command/changejobcommand.lua:11-17`). Its inherited
`GameCommandBaseClass.canFire` (`command/game/gamecommandbaseclass.lua:2309-2327`)
uses `processCanFire` (`:2459-2514`) and `checkFireImpl` (`:2330-2456`),
including actor-state, command-slot compatibility, recast and cost predicates.
The request-lane exemption therefore does not bypass every client predicate.
`gameCommand.csv` row 12017 (line 18) has column 37 equal to 1. With
the inherited `canFireOnDead == false` and `isAvailableOnSit == false`,
the actor-state predicate rejects raw main states 1/3 and 11/13/14.
Its column-1 branch accepts state 0 and rejects the recognized states
2, 30/31/32, 50/51 and 70. Other raw states reach its true fallback.
The relevant methods are `gamecommandbaseclass.lua:73-175,546-552,1075-1081`.
These comparisons do not establish the gameplay conditions that assign
each state or an authoritative server acceptance rule.
## Native request continuation

The original connector chunk `root/proto278`, PC 43, calls `command` with
nine operands after the command actor. An Equipment call with only a mode
therefore supplies:

```text
player:command(command12017, mode, nil, nil, nil, currentTarget,
               nil, nil, nil, nil)
```

The helper obtains `currentTarget` because its optional target argument was
nil. These are Lua operands, rather than a serialized packet. The original
connector and PlayerBase LPB identities and decoded payload hashes are pinned
by `xivl-client-scripts:manifests/retail_lua_coverage.json`. They were checked
before parsing with `xivl-client-scripts:tools/build_quest_contracts.py`.

The [native command policy path](../input/chara-selection-producers.md#native-command-construction-and-policy-dispatch)
owns construction and admission through `_executeCommand`. If
`PlayerBaseClass._onCommandEvent` receives command 12017, it takes the ordinary
`_callServerOnCommand` branch (`chara/player/playerbaseclass.lua:1823-1881`).
That callback forwards its first four varargs. The mode remains the first
operand; this branch adds no soul-item ID.

The [generic request bridge](../script/materia-command-dispatch.md#generic-command-request-bridge)
reaches `0x006E8B30`, `0x006E2FB0`, `0x0075E1C0`, `0x0089E0D0` and
constructor `0x00776760`. The constructor stores message word `0x012D` and
declared length `0xC8` at `0x00776786` and `0x0077678C`. Fresh PE-byte
comparison and Capstone decoding reproduced those instructions and the
intervening direct calls. Descriptor selection, command admission and the
callback remain conditions on this continuation. This does not establish a
historical job-change packet or its server response.

## Actions and acquisition boundaries

The current job, stone possession and acquired-command arrays are separate
inputs. `isAcquiredAdditionalCommand`
(`chara/player/playerbaseclass.lua:1112-1120`) reads
`charaWork.additionalCommandAcquired[index]`. `ActionSettingWidget.setJobAction`
(`widget/actionsettingwidget.lua:2943-3053`) hides the job-action grid unless
`getMainClassOrJob()` is recognized as a job. It then checks five entries
of that array using job-specific offsets and maps those entries through
`getAdditionalCommandList`. Its tests do not call `hasJobStone`.
The list and placement distinction is detailed in
`xivl-client-scripts:docs/action-bar-placement.md`.

`checkClassCommandPermission`
(`chara/charabaseclass_ffxivbattle.lua:113-220`) returns true for any
supplied class when `_getJob() == 0`. For a recognized active job it accepts
the following class IDs and rejects all others; an unrecognized nonzero
job also returns false:

| Active job id | Accepted class IDs |
| ---: | --- |
| 15 | 2, 8, 7 |
| 16 | 3, 4, 23 |
| 17 | 4, 3, 2 |
| 18 | 7, 23, 22 |
| 19 | 8, 2, 7 |
| 26 | 22, 2, 7 |
| 27 | 23, 3, 2 |

The Additional Action chooser uses this predicate at
`widget/actionsettingwidget.lua:1181-1195`. On an ordinary widget update,
`ActionSettingWidget.update` (`:786-856`) rereads `getMainClassOrJob`,
updates its cached job and rebuilds job, additional, equipped and godsend
presentation. Separate `commandEquip` work notifications refresh the action
menu and the open settings widget
(`widget/desktopwidget_connector.lua:4983-4999`); `exp` work notifications
also update the settings widget (`:5157-5229`). Those are demonstrated
refresh paths. The changed-job callback itself does not invoke them.

The learned-command notification is separate again:
`CharaBaseClass._onUpdateWork` handles `commandAcquired` at
`chara/charabaseclass.lua:1182-1188`, and `processUpdateCommandAcquired`
adds 26000 back to its supplied indices and forwards them to DesktopWidget
(`:1569-1579`). The DesktopWidget receiver is empty
(`widget/desktopwidget_connector.lua:6807-6810`). This does not establish
how or when server work updates, equipment updates and the job byte were
ordered during a retail job change.

`QuestBaseClass.showGetJobItemWidget`
(`quest/questbaseclass_common.lua:1154-1231`) recognizes the seven stone
catalog IDs, chooses worldMaster text 51128 for them or 33932 for other
items, opens `JobQuestInformationWidget`, optionally emits a notification,
and runs an item-specific character scheduler. The seven stone scheduler
IDs are 67108903 through 67108909, in the method's explicit item mapping.
`showGetJobAbilityWidget` (`:1084-1151`) similarly selects presentation
text 33923, 33924 or 33925 and a quest-family scheduler. Neither helper
writes inventory, the active job byte or an acquired-command array.

The initial job scenario methods pass a supplied item argument into this
presentation helper: `Mnk0j1.processEvent010_2_system`,
`Pld0j1.processEventKokuti`, `War0j1.processEventJob`,
`Brd0j1.processEventJob`, `Drg0j1.processEventKokuti`,
`Whm0j1.processEventJob`, and `Blm0j1.processEventClear`.
Their canonical paths are `quest/scenario/<family>/<family>0j1.lua`.
The explicit stone handling in the helper does not make every supplied
item argument a proven soul stone. The presentation and quest-family census
are owned by `xivl-client-scripts:docs/job-grand-company-event-contracts.md`
and `xivl-client-scripts:docs/quest-script-inventory.md`.

No inspected client path establishes the authoritative quest eligibility,
item grant, learned-ability grant, persistence, job-change rejection policy
or the server ordering that connects those separate state deliveries.
