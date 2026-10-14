# Battle Regimen combination and target modes

This page records the client-side combination kill switch, soul IDs, and
targetMode map. It extends [Battle Regimen chain UI](battle-regimen.md) with the
client Lua state and target-mode findings.

Evidence identity: the canonical `xivl-client-scripts:manifests/scripts.json`
manifest records `gameVersion` `1.23b` and extraction `2012.09.19.0001`. The
members used by this page are pinned by these SHA-256 values:

| Script member | SHA-256 |
|---|---|
| `lua/scripts/chara/charabaseclass_battle.lua` | `CA7D9BD52320E8AA62ECD853A0CE8D9E2E0DDC76FC504729B501B14B9BFCD959` |
| `lua/scripts/chara/charabaseclass_parameter.lua` | `748C02DBF42329DD0B942642B5FE68B35F422DB61C482954E3B00D0338F12893` |
| `lua/scripts/command/game/combinationstartcommand.lua` | `BCC82286234BE22D766BBA6C85C7C21CD384C251274028A64F2A3BB21D7B9DC9` |
| `lua/scripts/widget/desktopwidget_connector.lua` | `9C33F21C1F70A0056147E716D53300634EFABE5B744EF6E8690114DB21613A01` |
| `lua/scripts/widget/combinationwidget.lua` | `B4ED0528A981D473289705C6B59E376296709193CBEA44938C7A169EA50A7D46` |
| `lua/scripts/widget/configtargetwidget.lua` | `4AC9D8F1C4EFCBBF4D6E0458D629D9AD8CE1A6CE155E775F167B604333F6649F` |
| `lua/scripts/widget/targetselectmodewidget.lua` | `41397033D9B8866C78D7C7DEFDE73A41B0CE6DF9FECEC34B290F1B7BA0AFBE62` |
| `lua/scripts/widget/partyparameterwidget.lua` | `0B01E896004946CA1D1FA625E3947A80DC0F39A2BAB3EECFF7104C93E0AD12E1` |

The soul names and item categories are cross-checked in
`xivl-client-data:docs/inventory-cross-check.md` against `csv/_item.csv`
(SHA-256 `F41DBA38473A8A4014826C5B0CD639E12C6652700C9F760660FCB241D7469DDA`)
and `csv/xtx_itemName.csv` (SHA-256
`917C34EB0621B7D7A8261145023D0BB95CD8CA9EB52288CEB66E80D08F0D78C2`). The
matching `csv/itemData.csv` catalog is pinned by SHA-256
`F17193D832B375761CD01A37B25FF7D2C2947967DDC5BA4C2B16C0507D422912`.

## 1. Soul-stone IDs: all seven resolved

Recovered client records identify the following soul items and icons
(icon path `Important/ImportantItemStandard` for all):

| Key item | Soul | Icon id |
|---:|---|---:|
| 2000201 | Soul of the Paladin | 61680 |
| 2000202 | Soul of the Monk | 61681 |
| 2000203 | Soul of the Warrior | 61682 |
| 2000204 | Soul of the Dragoon | 61683 |
| 2000205 | Soul of the Bard | 61684 |
| 2000206 | Soul of the White Mage | 61685 |
| 2000207 | Soul of the Black Mage | 61686 |

## 2. Regimen = "Combination": full client chain found

A case-insensitive search for `egimen` over the 2,671 members listed by the
canonical script manifest returns one hit, and the `Combination*` family
completes the chain. This result is limited to the manifest members.

Control (start permission + stack counts) - all hardcoded stubs, no overrides
anywhere in the recovered tree:

- `lua/scripts/chara/charabaseclass_battle.lua:1092-1098` - `canStartCombination` returns
  `false`.
- `lua/scripts/chara/charabaseclass_battle.lua:840-846` -
  `getStackedCombinationNum` returns `0`.
- `lua/scripts/chara/charabaseclass_battle.lua:849-855` -
  `getStackedCombinationTimer` returns `0`.
- `lua/scripts/chara/charabaseclass_parameter.lua:200-206` -
  `getMyCombinationStackedNum` returns `0`.

Starter command constants (`command/game/combinationstartcommand.lua`):

- `getCommandRangeCode` -> 2; `getCommandTargettingMode` -> 1
  (note the double-t spelling, verbatim from the client).
- `canAimForRelation` / `canFireForRelation` -> `(false, false, true)`.
- `useWeaponRangeInformation` -> `(true, true)`; `isUseActionGauge` -> false.
- `CombinationManagementCommand.isActionMenu` -> false (regimen starter is not a
  normal action-menu entry); `CombinationStatus` is an empty subclass.

Party-list display:

- `lua/scripts/widget/desktopwidget_connector.lua:16307-16325` -
  `getStackedCombinationCountByPartyMember(member)` returns
  `getPartyMemberActor(member):getMyCombinationStackedNum()`; a call in
  `processCharacterParameterUpdated` (`:4841-5340`) feeds `updateStackedCombination`.
- `lua/scripts/widget/partyparameterwidget.lua:454-476` -
  `setStackedCombination` shows the icon
  when count > 0, hides otherwise.
- `lua/scripts/widget/partyparameterwidget.lua:750-771` -
  `setCombination(member, visible)` toggles
  visibility; `:1270-1281` builds the control name
  `ListBoxItem_N:IconControl_BattleRegimenDisplay` - the per-party-member
  queued-action icon.

The target-side `CombinationWidget` is a separate display path. Its
`updateStackCommand` (`lua/scripts/widget/combinationwidget.lua:48-123`) reads
`desktopWidget:getTargetStackedCombinationNum()`. When that count exceeds the
retained `stackCommandNum`, it queries `getTargetStackedCombination(index)` and
`getTargetStackedCombinationTimer(index)` for indexes from the retained count
through the new count and adds entries whose command and timer are both
positive. Otherwise it refreshes slots 1 through 8, adding slots up to the
count and removing the rest. It then stores the count.
None of the three getters is defined in the 2,671 manifest members or in
`xivl-client-scripts:lua/napi_index.json`, and their names do not occur in
`ffxivgame.exe`; the executable does carry `Window_CombinationWidget` at RVAs
`0xBC1AB4` and `0xBC1BB0` (`config/ffxivgame.strings.json`). The party-list
counts above are the only stack source defined in the decoded Lua.

Turn-on verdict (definitive for this build): the party-list display path is
intact, while the target-side display depends on the three getters absent
from the corpus. The feature is hard-disabled at the Lua layer - start permission is false
and every stack count is zero. Revival would need a client DAT/Lua mod restoring
these four functions (or their native backing, if the stubs shadow native
state). The packet and runtime execution boundary remains unresolved and is
not supplied by this client finding.

## 3. targetMode system mapped (AoE-button context)

No `AoE`/`_aoe` token exists anywhere in the recovered Lua (case-sensitive,
whole tree). Target selection runs through `work.targetMode` (integer8) in
`lua/scripts/widget/desktopwidget_connector.lua`:

- Default (`getDefaultTargetMode`, `:1248-1261`): 1, or 2 when `getConfigWork(13) == 1`.
- Cycle (`changeTargetMode`, `:4457-4522`): in sub-target selection it calls
  `changeTargetOnlyPartyMember(direction, true)`; with a lock-on target it
  returns; when `configWork(13)` is not 1 it calls
  `changeTargetOnlyPartyMember(direction)`. Only when `configWork(13)` is 1
  does it step through modes 1-4 with wraparound, skipping invalid modes, so
  that cycle alternates between modes 3 (in a party) and 4.
- Validity (`isValidTargetMode`, `:4525-4567`): mode 4 always valid; mode 1 valid unless configWork(13)
  is 1; mode 3 valid only while `isJoinedPartyMyPlayer()`; mode 2 has an empty
  branch - never selectable via cycling (yet reachable as the configWork(13)==1
  default, so it is a real state, not dead).
- `setTargetMode` (`:4611-4711`): writes `work.targetMode`, calls static widget 18
  `setMode`, restores per-mode `oldTarget[]`; mode 2/3 seed from MyPlayer, mode
  4 from `oldTarget[4]` with an enmity-target check, then
  `setTargetCharacter(1, ...)` or `RaptureCommands.ChangeTargetNext`.
- Shortcut entry (`targetModeShortCut`, `:4570-4608`) is blocked unless
  configWork(13) is 0, sub-target select is off, and no lock-on target exists.
- `processUpdateGroupInformation` (`:5673-5839`) resets mode 3 to
  `getDefaultTargetMode()` in its event-10001 branch when the member count is
  not above 1.

`targetMode` is a target filter. `setDefaultWidgetLocation` registers static
widget 18 as `TargetSelectModeWidget` (`desktopwidget_connector.lua:8092-8302`);
its `setMode` (`lua/scripts/widget/targetselectmodewidget.lua:27-89`) maps 1, 2,
3, and 4 to the styles `all`, `pc`, `party`, and `monster` and to text rows
2301-2304. In `xivl-client-data:csv/xtx__text_ui.csv` (SHA-256
`23762A422118D99C4144A48B3495C52511B1B1B865E4FC50FDC61A973E3C6ECE`) those
rows read `All`, `Player`, `Party`, `Enemy` at position 0 and `All`,
`Friendly`, `Party`, `Enemy` at position 1. The shortcut commands
`UILuaCommands.ChangeTargetCircleAll`, `Player`, `Party`, and `Enemy` call
`targetModeShortCut` with 1 through 4 in `processUICommandEvent`
(`desktopwidget_connector.lua:3199-4175`).
Mode 2 is never valid in `isValidTargetMode`, so it is entered only through
`getDefaultTargetMode`.

`configWork(13)` is the `ConfigTargetWidget` operation-type radio:
`processUICommandOperate` writes 0, 1, and 2 for
`RadioButton_OperationType_A`, `_B`, and `_C`
(`lua/scripts/widget/configtargetwidget.lua:274-356`). For item 13,
`DesktopWidget.updateConfigWork` cancels the lock-on and sets `targetMode` to
`getDefaultTargetMode()` (`desktopwidget_connector.lua:23897-24016`).

There is no separate area-of-effect control in the recovered Lua. Retail text
names one: `xtx__text_ui.csv` rows 1026 and 1492 read `Area of Effect` and
`Toggle Magic Area of Effect` at position 1, and `worldMaster.csv` rows 25307
and 25308 (SHA-256
`2E2E2DD5CD9651388F6FA575B4229081AD0452165E78B725DF0A5B5B7CF7C643`) read
`Area of effect casting enabled.` and `Single target casting enabled.` No
manifest member contains 25307 or 25308 as a literal. None of this establishes
a target-select packet mapping.

A prior external lead,
`docs/battle_regimen_combination_killswitch_targetmode_2026-09-27.md` at
revision `d5fb880e17f07f31ea7f8380ff813e5d71b6b678`, described the
`targetMode` map and listed its labels and `configWork(13)` as open.

## Remaining unresolved

1. `gameCommandBasic.csv` position 119 meaning; the other help-panel
   positions are in `xivl-client-scripts:docs/action-bar-placement.md`.
2. `executePlayerEquipAction` native packet shape.
3. Unified cross-job motion-ID index.
