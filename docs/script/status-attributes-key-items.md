# Attributes magic values and important items

The retail 1.23b Attributes widget reads Attack Magic Potency from
`charaWork.battleTemp.generalParameter[24]` in Lua. It builds its important-item
list from the local player's package 101. A learned quest flag, an item-reward
notification or a nonempty unrelated package does not supply either input.

## Evidence and method

Native observations use `ffxivgame.exe`, build `2012.09.19.0001`, size
15,996,808 bytes, preferred image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs. File-backed PE reads and Capstone 5.0.7 x86-32
decoding authenticated the cited instructions and registration strings.
These are static observations, without a live display or packet replay.

Lua paths below are relative to `xivl-client-scripts:lua/scripts/`.
Canonical LF bytes matched `xivl-client-scripts:manifests/scripts.json`:

| Member | SHA-256 |
| --- | --- |
| `widget/statuswidget.lua` | `29a01e8dadae3a9dd57a42d453f21338bcf33aec436fe509f6e565f263151385` |
| `widget/desktopwidget_connector.lua` | `9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01` |
| `chara/charabaseclass.lua` | `f17b3e62a137d8f524ba88d55296e1fb064512dc803f7a85dc980dd4bcbc9625` |
| `chara/charabaseclass_ffxivbattle.lua` | `d1db23d11f911ea3205ca72868cc9fca22cefe0d3f88a6706cbb035d92130bda` |

The corresponding original LPBs and decoded payloads matched
`xivl-client-scripts:manifests/retail_lua_coverage.json` and were parsed through
EOF with `tools/retail_script.py:decode_lpb` and
`tools/build_quest_contracts.py:ChunkReader`. In particular, the StatusWidget
resource is `client/script/n1635q/rq9qprn1635q.le.lpb`, SHA-256
`2809eaf420ced30da99c28786a2fe5ab798050b9d654103cba7b3059926016d4`;
its decoded payload SHA-256 is
`9668b0bc032dfcacc3f19f285bb5bd5673ad72c3da8d0fc68e893d087b27bb90`.
Prototype and PC locators below use that parser's zero-based numbering.

## Magic presentation

`StatusWidget.updateBattleParameter` calls these player getters and forwards
their results to the named setters (`widget/statuswidget.lua:1230-1265`).
The getters take no explicit value argument here. The neighboring crafting
getters' argument 1 does not belong to the magic calls.

| English text row | Getter | Lua `generalParameter` index | Control |
| --- | --- | ---: | --- |
| 15024, Attack Magic Potency | `getAttackMagic` | 24 | `Label_AttackMagic` |
| 15025, Healing Magic Potency | `getHealMagic` | 25 | `Label_HealMagic` |
| 15026, Enhancement Magic Potency | `getReinforceMagic` | 26 | `Label_ReinforceMagic` |
| 15027, Enfeebling Magic Potency | `getWeekMagic` | 27 | `Label_WeakMagic` |
| 15028, Magic Accuracy | `getMagicRate` | 28 | `Label_MagicHit` |
| 15029, Magic Evasion | `getMagicEvasion` | 29 | `Label_MagicEvasion` |

The work reads are explicit in
`chara/charabaseclass_ffxivbattle.lua:493-564`. Preserve the client's
`getWeekMagic` spelling. `widget/statuswidget.lua:534-591` supplies text
category 214 and the listed row keys to the title controls. The English
values come from `xivl-client-data:csv/xtx_text_paramName.csv`, rows
15024-15029, SHA-256
`aebca8a881055ecf060a03347300a3107294951b8d11f7b0c2eaf5ba4d60c0ba`.
This is a static row join; native text-category-214 resolution was not traced.

`setAttackMagic` (`widget/statuswidget.lua:1684-1700`) selects
`Label_AttackMagic/TextBlock_BaseParameter` and passes `tostring(value)` to
`setText`. It has no positive-value, class, job or stone predicate. The other
five setters use the same presentation pattern (`:1703-1790`).
`initParameter` first sets `Grid_Physics` and `Grid_Magic` visible
(`:773-792`); the crafting/gathering branches shown there change their own
grids. The inspected magic setter does not hide a zero value.

Original `updateBattleParameter`, `root/proto6`, reads Attack Magic at PC 21
and calls its setter at PC 22. The getter call has only the player receiver;
the following variable-argument call forwards its results. `setAttackMagic`,
`root/proto27`, starts at decoded offset `0x3C20` and contains no conditional
branch. This independently checks the decompile's short presentation path.

`StatusWidget.update` refreshes battle values for a nil update kind and for
`stateAtQuicklyForAll`, `gameParameter`, `battleParameter` and `exp`
(`widget/statuswidget.lua:3843-3925`). Those are refresh consumers, not an
acquisition or stat-calculation rule. A fixed actor-object offset, exact
incoming property hash/value record for each magic field, and its retail
sender transaction remain unproven in this trace. A Lua array index is not a
packet offset or a native zero-based array element number.

## Important-item list

`initImportantList` (`widget/statuswidget.lua:2478-2518`) sets
`Important_Maker.FilteredSortKey="sorttype"`, then sets
`ListBox_Important.SourceFirstIndex=0` and
`ListBox_Important.SourceCount=_getItemPackageCapacity(101)`. The two control
names come from `getListPropertyName` and `getListBoxName` (`:4023-4054`).
Package 101 is the Lua
important-item namespace; [Equipment soul crystals](equipment-soulstones.md)
owns its separate crystal-only filter.

`updateImportantList` (`:2521-2628`) enumerates Lua slots 1 through capacity.
For each slot it calls `setItemToXml(propertyList, outputRow, 101, slot)`.
Output rows start at zero. A true result advances the output row; the first
false result exits the loop. It then deletes obsolete trailing rows and
updates the list property. This is a prefix scan, not a scan that skips gaps.

`setItemToXml` (`:4094-4188`) returns false immediately when
`myPlayer:_getItem(package,slot)` is nil. Otherwise it obtains catalog ID,
icon, stackability and stack count through
`DesktopWidget.getPlayerItemInPackage` (`widget/desktopwidget_connector.lua:17794-17864`),
sets the row's icon/catalog/stack/name/sort properties, marks it visible and
returns true. It does not restrict this list to the seven soul-crystal IDs.
The connector returns `(0,0,false,0)` for a missing package or an out-of-range
slot, and uses the actual resolved item for a populated slot.

Original `updateImportantList`, `root/proto52`, calls `setItemToXml` at
PC 21, decoded offset `0x5813`. PCs 22-27 compare its result with true and
leave the loop on failure. `setItemToXml`, `root/proto79`, begins at
`0x8406` and retains the nil-item return. Later occupied slots therefore do
not become rows when an earlier slot fails to resolve. This follows the
enumeration algorithm; no retail runtime test of a sparse package is claimed.

The list applies these explicit visibility masks after building rows:

| Possessed catalog | Hidden older catalog(s) |
| --- | --- |
| 2001003, Augmented Materia Melder | 2001002, Materia Melder |
| 2001007, Chocobo Whistle | 2001004, Storm Chocobo Issuance; 2001005, Serpent Chocobo Issuance; 2001006, Flame Chocobo Issuance |

`hasImportantItem` (`:2631-2671`) scans package 101 for an equal catalog ID
and returns its Lua slot or zero. `maskImportantItem` (`:2674-2687`) ignores
zero and hides row `slot-1`; it does not delete possession. Names are from
`xivl-client-data:csv/xtx_itemName.csv`, matching rows, SHA-256
`917c34eb0621b7d7a8261145023d0bb95cd8ca9eb52288ceb66e80d08f0d78c2`.

`displayFocusedItemHelp` (`:4302-4365`) reads the selected row's `catalog`
and calls `setText("TextBlock_Importanthelp",3203,catalog)`. A zero row count
instead shows `TextBlock_NoContents`. The help input is the selected catalog
key, not the package slot, name index or a quest-completion flag.

`updateImportantList` does not repeat the initialization's `SourceCount`
assignment. The effect of changing package capacity while the existing list
remains open was not established. Capacity and callback delivery must be
distinguished from successful list rendering.

## Native package numbering and publication

The native item access path preserves the distinction between Lua package
numbers, native package codes and list row numbers:

| Locator | Observation |
| --- | --- |
| `0x00755FA0`, string `0x00FD84A4`, target `0x00708A20` | Registers `_getItemPackageCapacity`; the target calls `0x00702740` at `0x00708A76`. |
| `0x00702771-0x0070279D` | Converts the package argument, looks up the actor's package and takes a missing-package branch. |
| `0x007027BC` | Reads the resolved package's capacity at package `+8`; the missing-package path returns zero. |
| `0x00755A60`, string `0x00FD7FF4`, target `0x007087A0` | Registers the player `_getItem` path; `0x007087F8` calls `0x00702400`. |
| `0x00702440` / `0x00702484` | Calls package converter `0x0078BC90` and slot converter `0x0078BCD0`, each subtracting one. |
| `0x0076BC4C-0x0076BC52` | The package-start decoder reads application actor u32 at `+0`, capacity u16 at `+4`, and package u8 at `+6`. |

Thus Lua important-item package 101 selects native/wire package code 100
(`0x64`); Lua slot 1 selects native slot 0. `SourceFirstIndex=0` is a list
property setting and does not change those item-access operands.

The [shared package publication finding](../net/retainer-item-storage.md#refresh-and-packet-ordering)
owns the framed `0x016D`, `0x0146`, full-item/removal operations and `0x016E`
route. Package-start execution `0x0089B3F0` calls `0x006F41A0`; end processing
through `0x00703700` can publish staged storage through `0x006F7880` or defer
it. Receipt of a record alone is not proof of publication or a Lua callback.
The exact full-item variant used in a historical important-item grant remains
unproven here.

After publication, `0x00702E30` prepares `_onUpdateItemPackage` for nonempty
dirty-slot sets. Calls at `0x00702F4F` and `0x00702FD5` convert the native
package and slots back to Lua numbering; the dispatch call is `0x00703045`.
`CharaBaseClass._onUpdateItemPackage` forwards actor, package and slot list to
`DesktopWidget.processUpdateItemInformation` (`chara/charabaseclass.lua:1310-1325`).
Its original `root/proto28` jumps from PC 0 over the early return at PC 1,
then calls the connector at PC 7, decoded offset `0x2904`. The decompiled
`goto`/return rendering must not be treated as a disabled notification.

The connector's local-player branch obtains the open `StatusWidget` and
calls `updateMoneyList(package)` (`widget/desktopwidget_connector.lua:6029-6041`).
That method rebuilds important items only for package 101, and refreshes focus
and help if Attributes tab 2 is selected (`widget/statuswidget.lua:2437-2475`).
Its package-100 branch updates Grand Company points instead. There is no
equivalence between those two Lua package numbers.

## Reproduction and limits

The native methods can be inspected with the repository's documented Ghidra
runner against an explicit matching executable:

```powershell
tools\ghidra\run-headless.ps1 -Binary "<ffxivgame.exe>" -Script tools\ghidra_scripts\DecompileToText.java -OutputDirectory tools\ghidra\logs\status-package-read -ScriptEnvironment @{ DECOMP_VAS = '0x00702740,0x00702400,0x0076BC20,0x00702E30' }
```

Use a new output directory as required by the runner. For the Lua branches,
decode the manifest-selected original LPBs, parse the complete payload with
`ChunkReader`, and inspect the named prototype/PC pairs above. Authenticate
the selected CSV members against `xivl-client-data:manifests/tables.json`.

These paths establish required client inputs and refresh boundaries. They do
not diagnose a particular server session, authenticate a stat formula, prove a
key-item acquisition/persistence transaction, or demonstrate live rendering.
