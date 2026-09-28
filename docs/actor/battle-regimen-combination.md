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
| `lua/scripts/widget/partyparameterwidget.lua` | `0B01E896004946CA1DFA625E3947A80DC0F39A2BAB3EECFF7104C93E0AD12E1` |

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

- `lua/scripts/chara/charabaseclass_battle.lua:696` - `canStartCombination` returns
  `false`.
- `lua/scripts/chara/charabaseclass_battle.lua:583` -
  `getStackedCombinationNum` returns `0`.
- `lua/scripts/chara/charabaseclass_battle.lua:588` -
  `getStackedCombinationTimer` returns `0`.
- `lua/scripts/chara/charabaseclass_parameter.lua:104` -
  `getMyCombinationStackedNum` returns `0`.

Starter command constants (`command/game/combinationstartcommand.lua`):

- `getCommandRangeCode` -> 2; `getCommandTargettingMode` -> 1
  (note the double-t spelling, verbatim from the client).
- `canAimForRelation` / `canFireForRelation` -> `(false, false, true)`.
- `useWeaponRangeInformation` -> `(true, true)`; `isUseActionGauge` -> false.
- `CombinationManagementCommand.isActionMenu` -> false (regimen starter is not a
  normal action-menu entry); `CombinationStatus` is an empty subclass.

Display (fully plumbed, works if counts were nonzero):

- `lua/scripts/widget/desktopwidget_connector.lua:8753` -
  `getStackedCombinationCountByPartyMember(member)` returns
  `getPartyMemberActor(member):getMyCombinationStackedNum()`; call site at
  `:2541` feeds `updateStackedCombination`.
- `lua/scripts/widget/partyparameterwidget.lua:193` -
  `setStackedCombination` shows the icon
  when count > 0, hides otherwise.
- `lua/scripts/widget/partyparameterwidget.lua:309` -
  `setCombination(member, visible)` toggles
  visibility; `:492` builds the control name
  `ListBoxItem_N:IconControl_BattleRegimenDisplay` - the per-party-member
  queued-action icon, matching the retail "red icon" behavior.

Turn-on verdict (definitive for this build): the client display path is intact
but the feature is hard-disabled at the Lua layer - start permission is false
and every stack count is zero. Revival would need a client DAT/Lua mod restoring
these four functions (or their native backing, if the stubs shadow native
state). The packet and runtime execution boundary remains unresolved and is
not supplied by this client finding.

## 3. targetMode system mapped (AoE-button context)

No `AoE`/`_aoe` token exists anywhere in the recovered Lua (case-sensitive,
whole tree). Target selection runs through `work.targetMode` (integer8) in
`lua/scripts/widget/desktopwidget_connector.lua`:

- Default (`:558`): 1, or 2 when `getConfigWork(13) == 1`.
- Cycle (`changeTargetMode`, `:2283`): modes 1-4, wraparound both directions,
  skipping modes where `isValidTargetMode` is false.
- Validity (`:2304`): mode 4 always valid; mode 1 valid unless configWork(13)
  is 1; mode 3 valid only while `isJoinedPartyMyPlayer()`; mode 2 has an empty
  branch - never selectable via cycling (yet reachable as the configWork(13)==1
  default, so it is a real state, not dead).
- `setTargetMode` (`:2349`): writes `work.targetMode`, calls static widget 18
  `setMode`, restores per-mode `oldTarget[]`; mode 2/3 seed from MyPlayer, mode
  4 from `oldTarget[4]` with an enmity-target check, then
  `setTargetCharacter(1, ...)` or `RaptureCommands.ChangeTargetNext`.
- Shortcut entry (`targetModeShortCut`, `:2332`) is blocked unless
  configWork(13) is 0, sub-target select is off, and no lock-on target exists.
- Mode 3 is force-reset to default at `:2994` (leaving-party path - read the
  surrounding function before reusing this claim).

There is no separate AoE toggle control in the recovered Lua. Area targeting
is most plausibly a `targetMode` value or a per-command targeting mode
(`getCommandTargettingMode`-style constant per action, as the regimen starter
shows with its own `1`). The semantic labels of modes 1-4 remain unresolved;
the cited static evidence does not distinguish single, enemy, party, self, and
all-target behavior or establish a target-select packet mapping.

## Remaining unresolved

1. `setCommandInfo` columns 38/76/79/114/115/119 labels.
2. `executePlayerEquipAction` native packet shape.
3. Unified cross-job motion-ID index.
4. `targetMode` 1-4 semantic labels; `configWork(13)` meaning.
