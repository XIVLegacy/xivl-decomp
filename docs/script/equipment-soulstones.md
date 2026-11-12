# Equipment soul crystals

The retail FFXIV 1.23b Equipment window uses a special soul-crystal button and
list backed by important-item package 101. It does not equip a soul crystal
through the ordinary equipment-slot request. Selecting a compatible crystal
submits job-change command 12017; selecting an incompatible crystal explains
which weapon class is required.

## Sources

The decoded `xivl-client-scripts:lua/scripts/widget/equipwidget.lua` has
SHA-256 `A6EC61E6C7348BC72BE4638B832E4C630A93E9480D9C97CB0E5F4D4847A88FE2`,
as recorded by `xivl-client-scripts:manifests/scripts.json`. Source locators
below use this decompile's line numbers. CSV identities are recorded in
`xivl-client-data:manifests/tables.json`, extraction `2012.09.19.0001`.

One mode condition requires original bytecode. Retail
`client/script/n1635q/5tp1un1635q.le.lpb` has SHA-256
`f7959c210e4f06900bde9ae8fc714ac112be50f20459109c4611f2b38844ced3`.
Decoding with `xivl-client-scripts:tools/retail_script.py` gives Lua SHA-256
`2672baca238ab5fb4eee096a3f5fba6a6f6fb37ce0ce846a34787a3a65c0815d`.
Its prototype 82, original lines 2544-2630, instructions 168-172 tests
`equipStep == 101 or equipStep == 111`. Both reach the enabled soul-button
branch at instructions 173-186. The decompile at lines 5166-5180 loses the
101 alternative. The fixed Lua 5.1 parser in
`xivl-client-scripts:tools/build_quest_contracts.py` reproduces this branch.

## Important-item backing

`updateImportantList` (`:6786-6882`) enumerates package 101 from index 1 through
its capacity and stops at the first absent item. It sets list 2's
`SourceFirstIndex` to 0, `SourceCount` to package capacity and
`FilteredSortKey` to `catalog`. `setItemToXmlLight` (`:6200-6335`) makes only
catalog IDs 2000201 through 2000207 visible. Other important items do not
appear as selectable soul crystals. The explicit sort key is known; its
default direction is outside this Lua trace.

| Catalog ID | English name | Icon | Base class | Job |
| --- | --- | --- | --- | --- |
| 2000201 | Soul of the Paladin | 61680 | 3 Gladiator | 16 Paladin |
| 2000202 | Soul of the Monk | 61681 | 2 Pugilist | 15 Monk |
| 2000203 | Soul of the Warrior | 61682 | 4 Marauder | 17 Warrior |
| 2000204 | Soul of the Dragoon | 61683 | 8 Lancer | 19 Dragoon |
| 2000205 | Soul of the Bard | 61684 | 7 Archer | 18 Bard |
| 2000206 | Soul of the White Mage | 61685 | 23 Conjurer | 27 White Mage |
| 2000207 | Soul of the Black Mage | 61686 | 22 Thaumaturge | 26 Black Mage |

`xivl-client-data:csv/_item.csv` rows 2000201-2000207 column 0 assigns
`Important/ImportantItemStandard`. The same IDs in `csv/xtx_itemName.csv`
column 5 give the names; `csv/itemData.csv` column 36 gives the icons.
`ItemBaseClass.getItemIcon`
(`xivl-client-scripts:lua/scripts/item/itembaseclass_common.lua:4337-4349`)
reads that column. Its `isEquipment` (`:113-127`) accepts only catalog IDs
3900000 through 9999999, excluding these soul crystals. Base/job names use
`xivl-client-data:csv/xtx_text_jobName.csv` column 1. Numerical pairings also
appear in `chara/charabaseclass_ffxivbattle.lua:240-314,398-444`.

The button's initial visibility is the OR of seven `hasItem(101,catalog)`
checks (`:478-548`). A rebuilt nonempty filtered list also makes it visible
(`:6857-6866`). This rebuild does not explicitly hide a previously visible
button when the list becomes empty.

## Selection, help and refresh

Focusing `Button_JobStone` calls `jobstoneSlotFocused` (`:8382-8591`) and
enters step 121. It clears ordinary slot selection, ends repair mode, shows
`Grid_JobStoneList`, and hides the bag/gil and ordinary item list. Its heading
is text row 3685, `Soul Crystal`. Help rows 3688, 3689 and 3690 distinguish a
compatible owned crystal, no compatible owned crystal, and a class without a
job. The last branch tests base-class ID greater than 28.

When already in a job, this focus path locates the current job's catalog with
`getJobItemId`, then displays that item's details. In base-class state it hides
item-name, item-detail and item-life panels. The current state comes from
`getStateMainSkill` and `getMainClassOrJob`.

Activating the button enters step 122 (`:762-865`), focuses
`ListBox_TabItem_2`, and chooses the first filtered row at opacity `1.0`,
falling back to the first row. It changes the heading to text 3686,
`Select Soul Crystal`, or 3687, `Remove Soul Crystal`, according to whether
base class equals the current class/job. Activating again returns focus to
the soul button. Cancelling the list restores step 121 and unmasks ordinary
equipment; cancelling from the soul button at step 121 closes the window
(`:970-1040`). Focusing regular equipment restores its item panel (`:1255-1299`).

List hover and anchored-item callbacks map filtered focus to the underlying
source index before showing item details (`:1359-1456,6040-6068,7136-7523`).
For list 2, details read package 101 at source index plus one. They do not use
the equipment-slot compatibility route. Sorting controls and bag capacity are
hidden while showing the soul list.

`updateImportantOpacity` (`:6885-7013`) keeps all seven owned crystals visible.
Crystals incompatible with the current base class are dimmed to `0.5`; a
compatible one uses `1.0`. The compatible row's `equiped` marker is visible
only when the actor is in a job. Its item icon becomes
`Button_JobStone:IconControl_JobStoneIcon`; the icon is hidden when no active
compatible crystal supplies an icon. Dimming does not disable selection.

`update` (`:1668-1693`) refreshes class/job headers and crystal opacity.
`updatePlayerItem` (`:7977-8108`) rebuilds package-101 rows and opacity after
the item-update batch reaches zero. `updateWindowDisplay` (`:5587-5713`)
maintains the soul panel at steps 121/122 and updates list focus when requested.

### Open-window refresh callers

`CharaBaseClass._onUpdateItemPackage` forwards `(actor, package, slot-list)` to
`DesktopWidget.processUpdateItemInformation`. For the local player, that
dispatcher calls `updateMainMenuModeWidget("EquipWidget", package, slot-list)`.
The helper uses `getWidget(3, "EquipWidget")` and returns if the window is
absent; otherwise it calls `DesktopWidget.updatePlayerItem` on that widget.
The latter first supplies `(0, slot-count)`, then calls the widget once per
supplied `(package, slot)`. An empty slot-list initializes the count to zero
without a per-slot completion call.

The Equipment handler decrements a positive batch count. At zero, package 101
rebuilds the important-item list and opacity. Its `updateWindowDisplay` call
omits the focus flag. That method preserves the panel at steps 121/122, but
calls `updateListFocus` only when its flag is exactly Boolean true. Thus this
batch-completion caller does not request list-focus refresh.

Work callbacks provide another route. The local-player dispatcher calls the
open Equipment widget's `update(group)` for `stateAtQuicklyForAll`,
`battleStateForSelf`, `exp`, `battleParameter` or `gameParameter`. The
[original shared-branch proof](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/general-parameter-18-consumers.md#update-path)
owns the last two groups; the canonical decompile lost their OR branch.
`EquipWidget.update` refreshes the header, three parameter groups, ordinary
equipment and crystal opacity/icon. Its group argument is unused. This method
does not itself update masks, rebuild the list or request list-focus refresh.
These are conditional callback paths, without proof that a particular job-byte
packet delivers them or establishes rendered timing.

The [native actor completion-target contract](../net/actor-work-completion-targets.md)
binds the selected `charaWork/battleParameter` marker to a guarded Lua callback
attempt. Concrete live actor resolution, callback arrival and rendering remain
separate observations.

The original Lua 5.1 locators below use zero-based PCs. Method names and
canonical paths are in `xivl-client-scripts:lua/registry.json`; original
resource/payload identities are in its `manifests/retail_lua_coverage.json`.

| Source under `lua/scripts/` | Original prototype / PCs | Contract edge |
| --- | --- | --- |
| `chara/charabaseclass.lua:1310-1324` | `root/proto28`, PC 7 | Item-package callback to desktop |
| `widget/desktopwidget_connector.lua:5861-6046` | `root/proto86`, PCs 59-61, 136-140 | Local-player guard and named-widget dispatch |
| `widget/desktopwidget_connector.lua:6288-6368` | `root/proto95`, PCs 3-11; `root/proto97`, PCs 3-11 | Existing-widget lookup, batch count and slot loop |
| `widget/equipwidget.lua:7977-8108` | `root/proto124`, PCs 0-113 | Counted item updates and package-101 completion |
| `widget/equipwidget.lua:5587-5713` | `root/proto88`, PCs 64-104 | Panel preservation and exact-true focus gate |
| `widget/desktopwidget_connector.lua:4841-5340` | `root/proto78`, update PCs 216, 259, 286, 361 | Work-group calls to open Equipment |
| `widget/equipwidget.lua:1668-1693` | `root/proto7`, PCs 3-13 | Refresh targets |

The item callback source SHA-256 is
`F17B3E62A137D8F524BA88D55296E1FB064512DC803F7A85DC980DD4BCBC9625`;
its resource `729s9/729s989r57y9rr.le.lpb` SHA-256 is
`C7CD1DAD84F7D9B723F5A10FB66108DA9D9D6EF5D7C15186B48EC55092465238`.
The linked shared-branch finding pins the desktop resource and payload.
The Equipment identity above pins the remaining methods. These checked
inputs, rather than the callback names alone, support the paths.

### Ordinary gear changes

The retail `change_helm.pcapng` capture contains two command-12009
requests for Lua slot 9. The first requests removal. The second references
bag package 0, slot 113. Both server update scopes open with `0x016D`
mode 0, select package 0 (capacity 200), carry one `0x0148` item, then
select package `0xFE` (capacity 35). The first clears equipment slot 8
with `0x0152`. The second links slot 8 to package 0, slot 113 with
`0x014D`. Each scope contains two `0x0147` markers and closes with
`0x016E`. The bag records carry the removed item at slot 131, catalog
`0x007A3D64`, and the equipped item at slot 113, catalog `0x007A3F58`.
These are capture values for the two observed operations.

Use `xivl-captures:derived/payload_samples.json` under `samples.c2s`
`0x012d` indices 10 and 11 and `samples.s2c` `0x0146` indices 4-7,
`0x0148` indices 2 and 3, `0x0152` index 0 and `0x014d` index 1.
Indices are zero-based. Read application fields after the 16-byte
game-message header. The sequence is in `derived/sequences.json`,
`perCapture.change_helm.pcapng.collapsed` indices 29-35 and 65-71.
The [equipment transition census](https://github.com/XIVLegacy/xivl-captures/blob/9c827fd1d84e12f7a462e6cd5234d874c0e886c9/studies/equipment-property-correlation/derived/evidence-map.md#exact-transitions)
also joins the equip link and its source row in frame 28.

The [direct equipment cache path](examine-equipment.md#remote-gear-cache-and-publication)
handles links in `0x006F46C0`. The matching clear branch at
`0x006F4806..0x006F486D` compares the selected package with `0xFE` at
`0x00FE050C` and calls `0x006EE220` to clear each supplied slot. Both
direct `0xFE` branches omit the dirty-slot insertion used by the
`0xFD` link and non-`0xFE` clear branches. The accompanying full bag
item uses `0x006F4490`, which marks the slot dirty at `0x006F4651`
and sets the deferred-load byte at `0x006F4691`.

End publication integrates staged packages through `0x006F7880`
before `0x007034E0` publishes dirty slots. On the deferred branch
`0x0070374B..0x00703789` queues the terminator with table
`0x00FD556C`. After its pending-load checks,
`0x006EFAD1..0x006EFB5C` consumes the head terminator and calls the
ItemManager's load-finished slot. Cell `0x00FD761C` selects
`0x007036E0`, which preserves integration before dirty publication
at `0x007036E9..0x007036F1`. The native callback translates bag
package 0 to Lua package 1 through `0x00702F4F -> 0x0078BCA0`.

For that package the open EquipWidget makes a bag list entry for each
supplied changed slot. After the batch reaches zero, it updates the list,
item mask and ordinary equipment displays. `updateEquipItem` calls
`getPlayerEquipmentForParts(9)` for the head button and that getter
uses the local player's `_getEquippingItem(9)`. The native getter
reads equipment package `0xFE`, so the ordinary bag notification
rereads the integrated equipment link or clear. The original Lua
locators are `root/proto124` PCs 7-13, 18-30 and 90-96,
`root/proto17` PCs 25-29, and `root/proto18` PCs 39-41.

Each later `0x0139` sample (indices 5 and 6) has command 12009, selector
`0x7C000062`, one row, zero control value and zero row text ID.
The [result path](../net/combat-result-element.md#opcode-routes)
stages that selector as class 7 for Scene command 4. Its text-dependent
header and row branches have no text to process. The selector also
fails the [ActionCheck](../net/s2c-0193-native-state.md#actioncheck-consumers)
insert and erase guards. On the admitted
[action-queue route](../actor/action-queue.md#boundary-with-wire-and-vfx-systems),
the visual selector goes through table cell `0x00FE45A4` to
`0x00798A40 -> 0x007987F0`. The low value `0x62` selects the
`put_mes` action name through PE cell `0x012C4890`. These checked
paths establish result and action handling. An independent Gear refresh
from this result remains unproved.

The native sites were checked with ghidra-cli and LLVM objdump against
retail `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The original EquipWidget payload above was decoded with
`tools/retail_script.py` and disassembled with pinned unluac.
Rendered timing remains UNKNOWN.

#### Retained equipment controls

The package-1 batch completes on the open EquipWidget. Proto124
PCs 90-96 call list refresh, mask refresh and `updateEquipItem`
in that order. `updateEquipItem` proto17 PCs 25-29 rereads head
slot 9 and calls `setHead`. In the standard form, `Button_Head`
at line 267 selects `ControlTemplate_Button_EquipIcon`. The
template declares the base and live icon children and the life
and item badges on lines 19-24.

`setHead` proto59 PCs 0-29 calls `setEquipSlotIcon` with the live
icon, equipment icon ID, base icon and base ID 364, then writes the
Caution, Danger, PolishMAX and Materia badge visibility. The icon
helper proto53 PCs 0-37 gives the retained controls these states:

| Head slot | Live icon | Base icon |
| --- | --- | --- |
| Positive icon | Visible and supplied IconDatas | Hidden |
| Nil or nonpositive icon | Hidden and IconDatas 0 | IconDatas 364 and Visible |

The inherited visibility helpers (common proto80 and proto82)
write Visible for exact true, Collapsed otherwise, and Hidden on
the explicit hidden path. The slot-specific badge writes use
Visible and Collapsed.

`updateEquipItem` proto17 PCs 92-114 also refreshes detail help
for the current step. In step 101 with nonzero `focusedSlot`,
it calls `displaySlotItemHelp(focusedSlot,true)`. Proto112
PCs 4-30 writes title row 215 and RGB 1/1/1, and the exact-true
operand admits a fresh item read even when the slot equals the
last slot. The empty item requests help row 3603 and resets the
detail flags at PCs 72-89. Separately, `maskSlot` proto83 PCs 0-36
writes three numeric RGB values, each 0.5 or 1, and, when supplied
a help row, requests
`setHelpParameter(control,1,row)`. Common proto152 writes
`Help.Type` followed by `Help.Value0..3`, substituting numeric zero
for omitted values. The EquipWidget call sites supply at most
Value0. Common proto182 formats numeric text IDs through
`_setTextProperty` and sends a supplied string through `_setProperty`.

These are repeated requests to the existing controls. The checked
ordinary native stores compare their typed, retained values before
their changed-value notifications:

| Property family | Equal-value gate |
| --- | --- |
| Visibility | `0x009BCBA9..0x009BCBAB` compares the byte and skips the callback at `0x009BCBCD`. |
| VisualOpacity and RGB | The four indexed Visual setters call `0x0095E7B0 -> 0x00941350`. Equal finite floats take the branch at `0x009413B4` past the callback at `0x009413CF`. |
| Help.Type and Help.Value0..3 | The attached integer writer `0x0095EAC0 -> 0x009415B0 -> 0x0095E200` skips notification for an equal dword at `0x0095E259`. Changed help metadata can reach the existing help refresh checks at `0x0092DF4E..0x0092E011`. |
| Text and IconDatas | The ordinary string writer `0x009BA4E0 -> 0x00983FE0` compares length and bytes through `0x00445DB0`. Equal resolved strings take `0x00984022` past the callback at `0x0098405B`. |

Property admission and reference preprocessing precede these comparisons.
The
[Visual opacity render lanes](../resource/sqwt-keyframe-target-binding.md#observed-effective-opacity-at-the-selected-skin-quad)
and
[IconDatas conversion](../resource/sqwt-icon-control-textures.md)
describe the downstream consumers.

Inventory-row opacity uses the string branch of the XML row writer.
`EquipWidget` proto101 PCs 134-146 and proto111 PCs 62-86 pass
`"0.5"` or `"1.0"` to `setListProperty` for attribute `opacity`.
`EquipItemListWidget.tpl` line 11 binds it to `Grid_List.VisualOpacity`. The
[XML maker Action path](https://github.com/XIVLegacy/xivl-client-structs/blob/422a2c832d53c0dff50aa49e66cce92969b8ee71/docs/stub-overload-native-contract.md#sqwtxmldatamaker)
resets Action after SetAttr or Update, so the same action can run
again. Redraw with unchanged list counts remains open. The installed SQWT files were decoded
with their basename Blowfish keys and checked by round trip.

| Client-relative asset | Stored bytes and SHA-256 | Decoded bytes and SHA-256 |
| --- | --- | --- |
| `client/sqwt/widget/EquipWidget.form` | 89421, `2010deb69a2792dbe73be410500492c7c508eae2740b9db28f0eb1315d552bb9` | 89413, `4ce68aef535ab53ff1311e5dc109d10b1e90558ae75b914aba2f26691bf00bd8` |
| `client/sqwt/widget/EquipWidget.tpl` | 4831, `a145e3147c423c771a9e6d8122169fa9316ebd70d0077037fc3fc4fc012ed266` | 4823, `1ca0e91d04372a2a81b7ea6116f52cebcd823d70ef49e3ea2b21745afc2d775b` |
| `client/sqwt/widget/EquipItemListWidget.tpl` | 15765, `b42d268934e275c3242ec31f879617484a63b47a9a6104055f66a34285ee4a2a` | 15757, `ee6b5688a6cc67f1d623ac25cb91a01de371894df732709b0320483daf562461` |

## Request and incompatible-class dialog

`itemUICommandSelection` (`:1464-1494`) rejects a focus beyond the filtered
count, stores source index plus one, and immediately routes list 2 to
`checkJobChange`. The temporary `chosenPackage = 1` assignment does not select
the backing package for this branch: `checkJobChange` (`:8275-8379`) explicitly
re-reads `_getItem(101,index)` and returns if the item is absent.

A catalog/base-class pair matching the table submits:

| Current state | Explicit call |
| --- | --- |
| Base class equals current class/job | `desktopWidget.executePlayerCommand(12017, 1)` |
| Base class differs from current class/job | `desktopWidget.executePlayerCommand(12017, 2)` |

The calls contain no additional crystal ID, inventory index, equipment slot
or selected job ID. They do not call the ordinary gear equip/remove helper.
The generic player-command path owns further admission and serialization;
local submission does not establish server acceptance. The command resolves
through the `commandRequest` path, which exempts 12017 from the burst-blocker
and native `_canExecuteCommand` checks but still calls inherited `canFire`.
[Active job and soul-stone state](../actor/job-soulstone-state.md) owns the
shared command-state predicates and request continuation.

On a catalog/base mismatch, the widget scans job IDs 15 through 27 using
`getJobItemId`, obtains the required base with `convertSkillId`, then calls
`worldMaster.alert`. Row 30748 receives `(base, selectedJob, requiredBase)`
when in a base class; row 30749 receives `(currentJob, selectedJob,
requiredBase)` when in a job. Both English rows in
`xivl-client-data:csv/worldMaster.csv` explain that the selected job requires
equipping a weapon of its corresponding class. No job-change command or
automatic weapon change occurs on this branch.

The compatible branch requests exactly one local command result (`root/proto128`,
CALL C=2 at PCs 55/62) and tests its Lua truthiness at PC 64. False or nil skips
cleanup; numeric zero would be truthy if returned. This is a local return
test, without an authoritative success check. `equipedJobOnly` (`:9986-10030`) scans
ordinary equipment slots 1 through 27 for compatibility keys 2119 through
2129. A truthy first command result and falsy `equipedJobOnly` result reach
PCs 70-77: unmask equipment, focus the soul button, and explicitly call
`jobstoneSlotFocused`. This check occurs after submission; it does not reject
the request or assign the active job.

## Widget masks and limits

The original bytecode makes the soul button available in ordinary equipment
steps 101 and 111. Other gear-selection modes disable it with help 75223.
`maskEquipSlotForJob` (`:5263-5415`) disables ordinary equipment while choosing
a crystal, using help 75222, and makes the soul button's help 75221 describe
cancellation. Its ordinary-slot enabled state also controls the repair button.
`maskSlot` (`:5213-5260`) changes RGB and enable state and restores hover tab
stops when enabled. These are UI mutual-exclusion masks.

The Equipment selection handlers return while `updatecount > 0`, reject
negative selection, and check the selected package item and compatible base
class. `checkJobChange` adds no level, quest, battle, macro or mount test.
General command admission remains a separate path. The repair branch's macro
test does not apply to the soul-crystal branch.

The window's `TextBlock_Title` is a context heading. The
[player's honorific title](player-achievement-titles.md) belongs to the Status
window. `setPlayerName` (`:4677-4687`) writes the name, and
`updateMainSkillHeader` (`:1856-1893`) supplies class/job and level. No player
title getter or title-selection consumer appears in this EquipWidget script.

These findings establish recovered Lua and static-data behavior. They do not
establish server ownership updates, quest-unlock enforcement, battle legality,
or live rendered coordinates. The package enumeration's first-absent-item
behavior is a script assumption, not a general claim about native inventory
storage.
