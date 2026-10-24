# City lift menu and scene contract

Retail `ElevatorStandard` supplies separate Limsa Lominsa and Ul'dah menus
and twelve literal scene choices. Its menu invocation returns an answer;
another invocation with mode 1 or 2 plays a selected scene. The inspected
retail evidence does not join a named, spawned Crow's Lift or Ul'dah
airship-landing actor to one of these methods, nor join the returned menu
answer to a subsequent mode invocation or world destination.

This contract covers extraction `2012.09.19.0001`. It establishes client
text, script operands, and a native after-warp control boundary. It does not
establish historical server acceptance, a destination XYZ, or observed
playback.

## Script owner and menu operands

`xivl-client-scripts:lua/scripts/chara/npc/object/elevatorstandard.lua:1-25`
defines `ElevatorStandard` as an `NpcBaseClass`. `initForEvent` calls
`self:_setGroundOn(false)` and
`self:_loadTextDataPermanently(2112, "elevatorStandard")`.
The receiver `self` is the text owner and the owner passed when creating
the selected CutScene. No literal actor-class ID is supplied here.

Each city method has the following contract, authenticated against its
original LPB:

- Mode 0 calls `self:ask(self, questionRow, 3)` and returns its one result.
- Mode 1 calls `self:playElevatorCutSeane(firstScene)` with no results.
- Mode 2 calls the same helper with `secondScene`, also with no results.
- Other modes return no values and select no scene in these methods.

The spelling `playElevatorCutSeane` is the retail method name.
The question is `Select a floor.`; the three answers occupy the following
three rows, with `Quit.` last. `NpcBaseClass.ask` constructs the array
`questionRow + 1` through `questionRow + 3` and forwards it to
`DesktopWidget.askForEventMode`. These are answer positions 1, 2 and 3,
distinct from the city method's mode argument.

| Method | Question row | Answer 1, English text or referenced row | Answer 2, English text or referenced row | Mode 1 scene | Mode 2 scene |
| --- | ---: | --- | --- | --- | --- |
| `elevatorAskLimsa001` | 13 | Row 14: `2nd -` placeName 1077 | Row 15: `4th - Airship Landing.` | `elv0l012` | `elv0l01a` |
| `elevatorAskLimsa002` | 17 | Row 18: `1st -` placeName 1075 | Row 19: `4th - Airship Landing.` | `elv0l021` | `elv0l02a` |
| `elevatorAskLimsa003` | 21 | Row 22: `1st -` placeName 1075 | Row 23: `2nd -` placeName 1077 | `elv0l0a1` | `elv0l0a2` |
| `elevatorAskUldah001` | 25 | Row 26: `2nd -` placeName 3077 | Row 27: `3rd - Airship Landing.` | `elv0u012` | `elv0u01a` |
| `elevatorAskUldah002` | 29 | Row 30: `1st -` placeName 3076 | Row 31: `3rd - Airship Landing.` | `elv0u021` | `elv0u02a` |
| `elevatorAskUldah003` | 33 | Row 34: `1st -` placeName 3076 | Row 35: `2nd -` placeName 3077 | `elv0u0a1` | `elv0u0a2` |

Sources: `xivl-client-scripts:lua/scripts/chara/npc/object/elevatorstandard.lua:27-180`;
`xivl-client-scripts:lua/scripts/chara/npc/npcbaseclass_event.lua:389-415`;
`xivl-client-data:csv/elevatorStandard.csv`, rows 13-36, English header `1`.
The table pairs each method's menu with its separate scene branches. It
does not assert that retail fed answer 1 or 2 back as the mode argument.
No such feedback call appears inside these methods, and the bounded
decoded-Lua search for the six exact method names found their definitions
only. A server-supplied method invocation remains possible.

`DesktopWidget.askForEventMode` opens `Ask/AskWidget`, returns
`getAskResult()` after a successful `selectWidgetYield`, and otherwise keeps
its initial result 0. `AskWidget` stores the numeric `Answers` control
suffix on `UILuaCommands.Operate`; `AskBaseClass.getAskResult` reads that
stored result. Thus selecting the third displayed `Quit.` answer is an
answer value 3, not proof of a mode-3 server request. The menu passes false
for paging and cancellation; the widget's separate Cancel result `-3`
requires its `canCancel` flag to be true.

Sources: `xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua:12038-12090`;
`xivl-client-scripts:lua/scripts/widget/ask/askwidget.lua:111-248,389-415`;
`xivl-client-scripts:lua/scripts/widget/ask/askbaseclass.lua`,
`getAskResult`, `setBaseAskResult`, and `getBaseAskResult`.

## Floor labels and named-place joins

The English place-name rows give these independently stored names:

| placeName row | English header `1` | Use in the inspected evidence |
| --- | --- | --- |
| 1075 | `Chocobo Stables` | Limsa first-floor answer token. |
| 1077 | `Mizzenmast Inn` | Limsa second-floor answer token. |
| 1079 | `Crow's Lift` | Named place and map-marker label. |
| 3075 | `Wellhead Lift` | Named Ul'dah lift place and map-marker label. |
| 3076 | `Merchant Strip` | Ul'dah first-floor answer token. |
| 3077 | `Hustings Strip` | Ul'dah second-floor answer token. |
| 3519 | `Airship Landing` | Named Ul'dah place and map-marker label. |

Source: `xivl-client-data:csv/xtx_placeName.csv`, the listed row IDs.
The elevator menu uses an explicit English `Airship Landing.` literal for
Limsa's fourth floor and Ul'dah's third floor. Those literals have no
place-name key operand, so they do not themselves choose between the
several `Airship Landing` rows in `xtx_placeName`.

The Limsa second-floor text has a real localization discrepancy. English
rows 14 and 23 retain the exact sheet token
`[@sheet:022814ff0e7874782f706c6163654e616d65f204350203]`.
Its literal expression operands after `xtx/placeName` are 1077 and 1;
row 1077 is `Mizzenmast Inn`. Japanese text in those elevator rows names
the adventurers' guild's Drowning Wench, while the German and French cells
also name that establishment. The distinct English placeName row 1071 is
`The Drowning Wench`. Replacing the English token's 1077 with 1071 would
change the evidence. No runtime rendering was observed to resolve this
discrepancy.

For comparison, the retained English tokens in elevator rows 18/22 decode
to `(1075, 1)`, rows 26/35 to `(3077, 1)`, and rows 30/34 to `(3076, 1)`.
Integer decoding follows the retained prefix-expression grammar in
`xivl-tools:docs/formats/ssd-sheet.md`, section
`Rich-string read and text-export contract`; raw tokens remain authoritative.

The map-marker sheet supplies positive named-place joins:

| `2Dmap_marker` rows | Header `16`, placeName key | Header `13`, UI class |
| --- | ---: | --- |
| 1726, 1802 | 1079, `Crow's Lift` | `MapMarkerUnder` |
| 1871 | 1079, `Crow's Lift` | `MapMarker` |
| 2720, 2823, 2911 | 3075, `Wellhead Lift` | `MapMarker` |
| 2912 | 3519, `Airship Landing` | `MapMarkerSymbol` |

Source: `xivl-client-data:csv/2Dmap_marker.csv`, listed row IDs.
These are map-label records. Their map coordinates, resource instances,
and `@5204/i...` property references are not elevator actor-class IDs,
spawn transforms, or menu invocation operands.

The remaining actor join is exact: the named place/map marker must be
associated with a spawned actor-class ID, that actor's
`/Chara/Npc/Object/ElevatorStandard` class path, and the exact city method
and mode supplied to it. The inspected `actorclass.csv`,
`actorclass_graphic.csv`, and `actorclass_mapObj.csv` have numeric typed
fields, without a stored class-path string. The inspected
`2Dmap_actor_data.csv` supplies marker resource paths and instances;
`2Dmap_marker.csv` supplies the map-label records above. Searches of those
sheets and `objectItemStorage.csv`, `objectEventDoor.csv`, `objectBed.csv`,
`mapObjPortDoor.csv`, `elevatorStandard.csv`, and `xtx_displayName.csv`
found no `ElevatorStandard` class path or named-lift-to-method binding.
Exact-name searches of the client-data manifests and derived products
found no such binding.
`xivl-client-data:manifests/staticactor_class_paths.json` also contains no
`ElevatorStandard` record; its declared non-NPC coverage alone would not
establish this missing join.

Consequently, the menus support the stated floor labels and scene
operands, but do not prove the entering floor of a particular spawned
actor. In particular, neither the `001/002/003` method suffixes nor the
scene's `1/2/a` characters authenticate that floor. The Wellhead Lift
marker also does not prove that it uses the Ul'dah three-floor menu.

## Playback, control and return boundary

`playElevatorCutSeane(sceneName)` gets `worldMaster:_getMyPlayer()`, calls
that player's `_fadeOut(1)` and `_waitForFading()`, then calls
`worldMaster:createCutScene(sceneName, self)`. On the returned scene it
calls `startCutScene(1, 61, 1)`, receives two values, calls `_delete()`
unconditionally on that scene, then calls the player's
`_fadeInAfterWarp()`. It returns no values. The two scene results have no
test or propagation in this helper.

The original elevator LPB's `root/proto8` has 24 code words at decoded
chunk offset `0x898`. PC 13 (`0x8CC`) creates the scene with the method's
name argument and receiver. PC 18 (`0x8E0`) is
`CALL A=5 B=5 C=3`, receiving two results from `startCutScene`.
PCs 19-22 (`0x8E4`-`0x8F0`) delete and fade in without a branch; PC 23
(`0x8F4`) is `RETURN A=0 B=1`, returning no values. The first unbound
playback-result edge is therefore the returned scene pair versus any
result or event identity on the outer elevator method: the helper
discards the pair, and its mode-1/2 callers request no helper results.

The [common playback and actor-local result contract](airship-scene-sequence.md#playback-return-and-event-correlation)
and [native scene notice](../net/ferry-scene-notice-producer.md)
own the generic CutScene completion/skip and `0x00CE` boundaries.
Those observations do not bind an elevator scene's notice to the original
menu answer, a starting server event, or a warp acknowledgement.
The [incoming function receiver](start-event-fn-receiver.md) is the
separate endpoint for trigger/owner keys, named function and arguments;
no inspected lift source joins that frame to a specific named lift.

The native-facing control edge is independently identifiable. In the
hash-pinned `ffxivgame.exe`, Capstone x86-32 decoding of original PE bytes
confirms the `_fadeInAfterWarp` registration's bridge `0x006DE7B0` at VA
`0x00732287`, its string operand `0x00FD6974` at `0x00732339`, and the
bridge's virtual dispatch through `+0x100`. The vtable cell at
`0x00FD795C` contains `0x006E3260`; that method calls `0x0075B500` and
`0x0075B300` at `0x006E327A/0x006E3281`, then writes 1 to receiver byte
`+0x166` at `0x006E3286`. The
[after-warp loading contract](company-warp-loading-boundary.md)
owns the further chain and its limits. This control operation supplies
neither a destination nor evidence that a lift warp was accepted.

## Original LPB locators and identity

The elevator LPB's six menu prototypes each contain 21 code words:

| Method suffix | Prototype | Code offset | Mode-0 ask / return PCs | Mode-1 / mode-2 call PCs |
| --- | --- | --- | --- | --- |
| `Limsa001` | `root/proto1` | `0x2A0` | 6 / 7 | 13 / 19 |
| `Limsa002` | `root/proto2` | `0x388` | 6 / 7 | 13 / 19 |
| `Limsa003` | `root/proto3` | `0x470` | 6 / 7 | 13 / 19 |
| `Uldah001` | `root/proto4` | `0x558` | 6 / 7 | 13 / 19 |
| `Uldah002` | `root/proto5` | `0x640` | 6 / 7 | 13 / 19 |
| `Uldah003` | `root/proto6` | `0x728` | 6 / 7 | 13 / 19 |

PCs are zero-based; the byte offset is `codeOffset + 4 * PC`.
PCs 0, 9 and 15 compare the input mode against 0, 1 and 2. For all except
`Limsa003`, PC 6 is `CALL A=2 B=5 C=2` and PC 7 is
`RETURN A=2 B=2`; `Limsa003` uses A=3 for both. PCs 13 and 19 have
`B=3 C=1`, requesting no results. PC 20 is
`RETURN A=0 B=1`. The original branches authenticate the separate menu
and playback invocations, rather than relying on decompiler temporaries.

Supporting original-chunk locators are `NpcBaseClass.ask`,
`root/proto10`, code `0xD49`, PCs 0-7 for row-array construction and
19-20 for call/return; `DesktopWidget.askForEventMode`, `root/proto237`,
code `0x1C081`, PCs 34-40 for result read, close and return;
`AskWidget.processUIOperatorCommandRequest`, `root/proto4`, code
`0xCD5`, PCs 5-15 for control-suffix extraction and storage; and
`AskBaseClass.getAskResult`, `root/proto5`, code `0x426`.

All selected Lua sources matched canonical LF bytes in
`xivl-client-scripts:manifests/scripts.json`; their original carriers and
decoded chunks matched
`xivl-client-scripts:manifests/retail_lua_coverage.json`.

| Script under `lua/scripts/` | Canonical SHA-256 |
| --- | --- |
| `chara/npc/object/elevatorstandard.lua` | `29da07aef7c46a3e8886dd8cc14deb23af29f17362e9f8a1c8b18094a0651bfa` |
| `chara/npc/npcbaseclass_event.lua` | `5417b1bdf762162cad240a84715d0fc85958dbe0181b653348a66c02ee4d7c80` |
| `widget/desktopwidget_connector.lua` | `9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01` |
| `widget/ask/askwidget.lua` | `c5032a2d2d1264c4a01165834f6248a01dc1c4bf7b9f791ce8baf511295398af` |
| `widget/ask/askbaseclass.lua` | `642f3112d4663004fdc0b79daf0a5faf8b7588ead58d2cd8e6747821bda54adb` |

The elevator carrier
`client/script/729s9/wu7/v8057q/5y5o9qvsrq9w69s6.le.lpb` has 2,502
bytes and SHA-256
`207f1b2b5f042d920b44096adc21dea538dc6522acdbbb00d491dfcd824e5ab0`.
Its decoded 2,489-byte chunk has SHA-256
`ea03582c99d32a17fb840749954737f15785c36ffc97c6b3cde53703446203e2`.
Reproduce with `xivl-client-scripts:tools/retail_script.py` and inspect
the decoded chunk with `ChunkReader` and `instruction` in
`xivl-client-scripts:tools/build_quest_contracts.py`.

The selected CSV copies matched `xivl-client-data:manifests/tables.json`.
Primary text and map evidence pins are:

| Sheet | SHA-256 |
| --- | --- |
| `elevatorStandard.csv` | `27ec0dfc38d79960c9151bf814b3706c1e6518973cc59088757eb8cf3e201787` |
| `xtx_placeName.csv` | `81467ef42e8aeba82fe95f6c4249356550c02e41e6734abf9fdd194051dc1714` |
| `2Dmap_marker.csv` | `ec907b8121be1dfa274c8422f8431021c3c4aa1d74ae195652532a5398395215` |

Native addresses are VAs in `ffxivgame.exe`, preferred image base
`0x00400000`, size 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Fresh `pefile` translation and Capstone 5.0.7 x86-32 decoding checked the
listed control instructions and vtable cell. No target execution or
scene-transform-to-world-coordinate inference was used.
