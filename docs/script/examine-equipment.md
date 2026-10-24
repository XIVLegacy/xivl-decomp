# Examine equipment

The retail 1.23b player-check action opens `PcProfileWidget` for the examined
actor. Its recovered Lua displays ordinary gear through that actor's
`_getEquippingItem(slot)` and displays the active class/job in its text header.
Neither that script nor the identified profile form and equipment template
declares a soul-crystal control or crystal-icon synthesis path. The form
declares 22 ordinary gear buttons; the Lua slot map handles 18 of them.
These are static declarations, not a visual observation of the retail window.

The local Equipment window's package-101 crystal chooser is a separate
[contract](equipment-soulstones.md). Its owned crystals and job-change
command do not establish a remote equipment slot or an Examine requirement.

## Evidence identity

Lua locators below are canonical decompile line numbers in
`xivl-client-scripts:lua/scripts/`, extraction `2012.09.19.0001`.
Each selected member was normalized by replacing CRLF byte pairs with LF
and matched its size and SHA-256 in
`xivl-client-scripts:manifests/scripts.json`:

| Member | SHA-256 |
| --- | --- |
| `widget/pcprofilewidget.lua` | `8DB01ACB3CA10557CFF4B5E5946252B71F8CF4005E25FA641376F81C77F324D6` |
| `widget/pcinformationwidget.lua` | `4A6EEDF404C16C226DB197352C50D1242EA1D757396775A8FBDBE06B212CB2AC` |
| `widget/desktopwidget_connector.lua` | `9C33F21C1F70A0056147E716D53300634EFABE5B744EF6E8690114DB21613A01` |
| `chara/charabaseclass.lua` | `F17B3E62A137D8F524BA88D55296E1FB064512DC803F7A85DC980DD4BCBC9625` |
| `chara/charabaseclass_ffxivbattle.lua` | `D1DB23D11F911EA3205CA72868CC9FCA22CEFE0D3F88A6706CBB035D92130BDA` |
| `chara/player/playerbaseclass.lua` | `6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4` |

Native observations use retail Windows `ffxivgame.exe`, 15,996,808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, preferred image base `0x00400000`. All addresses below are VAs.
The instruction observations used the existing Ghidra function listings,
compared each selected instruction's bytes with the PE, and decoded those
bytes independently with pefile `2024.8.26` and Capstone `5.0.7`, x86-32 mode.
No target execution or captured Examine transaction is claimed.

## Open and request

`DesktopWidget.executePlayerCheck`
(`widget/desktopwidget_connector.lua:14072-14090`) gets the main target,
requires `isOtherPlayer(target)`, and calls
`openMainMenuRootWidget("PcProfileWidget", nil, target)`.
The short `PcInformationWidget` script is not the detailed gear window:
its operator choices 1 and 2 use the listed target name for Tell and party
invitation (`widget/pcinformationwidget.lua:50-81`). This entry path opens
`PcProfileWidget` directly, rather than proving a wrapper-to-profile call.

Profile initialization stores the supplied actor with `setArgActor`, creates
`ItemDetailWidget`, registers the gear buttons and `UILuaCommands.Shown`, and
clears the gear displays (`widget/pcprofilewidget.lua:1507-1875`). On Shown,
`processUICommandDefault` (`:1968-2015`) submits:

```lua
desktopWidget:executePlayerSystemCommand(24238, true, nil, examinedActor)
```

The connector resolves `_getStaticActor(24238)` through
`PlayerBaseClass.getSystemCommand`, checks the command actor, retains the
explicit examined actor for 24238, and invokes `canCommand` and then `command`.
The resulting explicit operand sequence is
`player:command(commandActor, true, nil, nil, nil, examinedActor, nil, nil, nil, nil)`.
Sources: `widget/desktopwidget_connector.lua:13794-13977` and
`chara/player/playerbaseclass.lua:1802-1817,1911-1975`.

`command` selects a command name and calls `_executeCommand` only after its
admission test. If its command-event callback executes,
`_onCommandEvent` forwards the name, command actor and first four varargs to
`_callServerOnCommand` (`chara/player/playerbaseclass.lua:1820-1881`).
The [generic native request bridge](materia-command-dispatch.md#generic-command-request-bridge)
owns the conditional continuation through `0x006E8B30`, `0x006E2FB0`,
`0x0075E1C0`, `0x0089E0D0` and the `0x012D` constructor `0x00776760`.
The [native command policy path](../input/chara-selection-producers.md#native-command-construction-and-policy-dispatch)
owns descriptor lookup and `_executeCommand` admission.

This identifies the submitted Lua command and the available generic native
continuation. It does not bind an executed 24238 descriptor, its selected
policy callback, a complete serialized Examine request, or a correlated
server reply. In particular, command submission is not evidence that a
specific item update was its response.

## Remote gear cache and publication

The `_getEquippingItem` name is at `0x00FD846C`. Registration function
`0x00755D00` loads implementation `0x007088E0` at `0x00755D37`, constructs
the name at `0x00755DE9`, and registers the binding at `0x00755E19`.
The implementation calls `0x00702510` at `0x00708938`.

`0x00702510` converts the supplied Lua slot to a zero-based slot through
`0x0078BCD0` at `0x0070254F`. It obtains the receiver's item manager through
`0x006F51A0`, tests the package keyed by the dword at `0x00FE050C`, and
looks up that package and slot through `0x006EE3E0`
(`0x00702558-0x00702599`). The dword is `0xFE`, not package 101.
Absent package/item branches use empty-result helper `0x00748920` at
`0x0070257E` / `0x007025AF`; the Lua consumer treats a missing item as nil.
A present item is
wrapped through `0x006E2770` at `0x007025C7` and returned to Lua.

The receive-side package-start decoder provides a matching cache path:

| Location | Observed operation |
| --- | --- |
| `0x004DD1CA-0x004DD1D4` | The `0x0146` dispatcher arm advances past the 16-byte application header and calls `0x005764C0`, also passing the dispatcher's first explicit argument by reference. This is `[EBP+8]` in its stack frame, not a packet offset. |
| `0x005764E5-0x0057651A` | Wraps that routing identifier and forwards it, the world context and the application body to `0x0076BC20`. |
| `0x0076BC4C-0x0076BC71` | Reads body `+0:u32`, `+4:u16`, `+6:u8` into an internal record, and derives record `+7` by comparing package `+6` with `0xF9`, stored at `0x00FE0520`. The flag is derived; it is not read from body `+7`. |
| `0x0076BC76-0x0076BCC4` | Gets a world-context identifier through `0x0057AB60`, compares it with the routing identifier, and rewrites record package `0xF9` to `0xFE` when they differ. `0x0057AB87-0x0057AB8E` reads that comparison identifier from the native context's `+0x10`. |
| `0x0076BCC8-0x0076BCD7` | Selects the queue by routing identifier through `0x0076B950` and queues the package-start record through `0x0075F860`. |
| `0x0089B359-0x0089B367` | The package-start command copies both record dwords, retaining the derived flag. Its execution slot at `0x01057154` points to `0x0089B3F0`. |
| `0x0089B411-0x0089B41C` | Execution reaches `0x006F41A0` on the destination actor's item manager. |
| `0x006F41D5-0x006F41EC`, `0x006F429A-0x006F42A0` | Uses record `+6` as the package key and retains record `+7` in the package's byte `+0x20` when creating a missing package. |

The dispatcher selection is also recorded in
[receive dispatcher ranges](../net/receive-dispatch-ranges.md).
This is a static route for remote gear data, not a captured request/reply
pair. The body `+0` field and dispatcher routing identifier are separate inputs;
their equality must not be assumed.

The general cache can hold a direct item or a link to another package/slot.
`0x006EE3E0` returns an existing item pointer at slot-record `+8`; if that is
null and its `+0x0C` and `+0x10` link fields are non-sentinel, it recursively
resolves the linked package and slot (`0x006EE42C-0x006EE465`).
`0x006F46C0` accepts linked-slot updates only for selected packages `0xFD`
or `0xFE`. Its `0xFE` branch reads six-byte internal entries containing
destination slot `+0:u16`, source slot `+2:u16` and source package `+4:u8`,
and calls `0x006EE2E0` at `0x006F47E0`. That helper installs a slot record
through `0x006DF820`, with no direct item pointer and those link fields.
These are item-cache links, not soul-crystal ownership or job state.
The `0xFE` link branch does not itself insert dirty slots in this function.
The lookup and link support do not identify which full-item/link records
actually populated a retail Examine result or guarantee a refresh by links alone.
The shared wire variants and queue contract are recorded in
`xivl-client-structs:docs/bazaar-native-wire-contract.md`, section
"Incoming package operations and publication".

Publication is distinct from receipt. The shared update-end path integrates
staged packages through `0x006F7880` and processes dirty slots through
`0x007034E0`, or defers that work. The normal branch calls `0x00702E30`
at `0x00703675`. For nonempty dirty slots, `0x00702E30` constructs
`_onUpdateItemPackage` using the string at `0x00FD49C8` and dispatches it at
`0x00703045`. When package byte `+0x20` equals 1, it supplies Lua package
`-1` (`0x00702EDA-0x00702F3D`). Otherwise it converts the native package to
Lua numbering. Dirty slots are converted by adding 1 through `0x0078BCE0`
at `0x00702FD5`. Thus the special callback identifier `-1`, native cache
key `0xFE`, and incoming special package `0xF9` are different domains.
The [item publication contract](../net/retainer-item-storage.md#refresh-and-packet-ordering)
owns update ordering and deferred-publication limits.

`CharaBaseClass._onUpdateItemPackage` forwards `(actor,package,changedSlots)`
to the desktop (`chara/charabaseclass.lua:1310-1324`). The connector calls
the open profile once per changed slot
(`widget/desktopwidget_connector.lua:5896-5912`). The profile accepts only
its exact `getArgActor()` and package `-1`, then rereads the actor's
`_getEquippingItem(slot)` (`widget/pcprofilewidget.lua:1032-1047`).
This preserves examined-actor ownership across the callback and the item
read. It does not read the local player's important-item package for gear.

## Ordinary slots and item display

`slotNameToEquipSlotNum` maps these exact control names to Lua slot numbers
(`widget/pcprofilewidget.lua:1903-1965`). Unknown names return 0.
The item-update dispatch at `:1100-1410` selects the corresponding setter.

| Lua slot | Control after `Button_` | Setter |
| ---: | --- | --- |
| 1 | `PrimaryArm` | `setMainWeapon` |
| 2 | `SecondaryArm` | `setSubWeapon` |
| 5 | `ThrowingWeapon` | `setThrowWeapon` |
| 6 | `LargePouch` | `setPouch` |
| 7 | `SmallPouch` | `setBadolier` |
| 9 | `Head` | `setHead` |
| 10 | `Undershirt` | `setBodyInner` |
| 11 | `Body` | `setBody` |
| 12 | `Undergarment` | `setLegsInner` |
| 13 | `Legs` | `setLegs` |
| 14 | `Hands` | `setHands` |
| 15 | `Feet` | `setFeet` |
| 16 | `Waist` | `setWaist` |
| 17 | `Accessories_Neck` | `setNeck` |
| 18 | `Accessories_RightEar` | `setEarR` |
| 20 | `Accessories_RightBracelet` | `setWristR` |
| 22 | `Accessories_RightFinger_1` | `setIndexR` |
| 23 | `Accessories_LeftFinger_1` | `setIndexL` |

These are the profile's Lua-mapped slots, not a declaration of every
authored form control or slot the native item manager can store. The gaps
do not identify hidden crystal slots. Slot numbers are converted to native
indices by subtracting 1, rather than being sent unchanged to the native
cache lookup.

The update handler gets the item's icon, materialization permission,
fitness, materia attachment information and item-life parameters before
calling the setter (`:1056-1099`). `setEquipSlotIcon` (`:72-117`) displays
a positive item icon; otherwise it hides that icon and restores the slot's
placeholder icon when supplied. The setters at `:120-873` apply those
icons and state indicators to the named ordinary gear controls.

The first main-weapon update clears `firsttime` and enables the gear buttons
(`:1112-1189`). Focus maps the button to its slot and invokes
`displaySlotItemHelp`. That method rereads the examined actor's item and
calls `ItemDetailWidget.displayItemHelp(item,0,0,7,slot)`, or displays an empty
slot with text row 215 (`:2091-2162`). These operands establish the detail
consumer; they do not select a job crystal by catalog ID.

The package-101 checks during profile initialization use the local player's
items 2001001, 2001002 and 2001003 only to set `materialiseVisible`
(`:1537-1563`). They are not the soul-crystal IDs 2000201-2000207 and do
not supply the examined actor's gear or a crystal icon.

## Declared profile resource

The native constructor at `0x0052B810` pushes the string
`\widget_c\PcProfileWidget.form` at `0x00F9B9E8` with instruction
`0x0052B841`, passes it to `0x005296E0` at `0x0052B85C`, and installs
vtable `0x00F9F50C` at `0x0052B86F`. The RTTI catalog identifies that table
as `Application::Main::Element::Window::Widget::PcProfileWidget`
(`config/ffxivgame.rtti.json`, RVA `0x00B9F50C`). These bytes and the string
were authenticated against the executable identified above.

The matching installed named file and its direct equipment-template/style
dependencies were decoded with `xivl extract-resource <file> --as sqwt
--output <directory>`. `verify-extraction <directory> --source <file>`
replayed each source successfully. The decoder used the exact case-sensitive
basename as its SQEX key. See `xivl-tools:docs/formats/sqex.md`,
"Decoded-document export", and `xivl-tools:src/formats/src/sqwt.rs:decode`.
The executed `xivl 0.1.0` Windows tool was 15,003,648 bytes, SHA-256
`cf0029e7aaaeb4c04e7a1de9cae4c144699f773a6839a902e33faa224d78f8aa`.
Python's standard-library `xml.etree.ElementTree` parsed the exact decoded
UTF-8 bytes without rewriting them.

| Install-relative source | Source bytes / SHA-256 | Decoded bytes / SHA-256 |
| --- | --- | --- |
| `client/sqwt/widget_c/PcProfileWidget.form` | 8488 / `bcce06c729e8beee04f7d7068087d86113f7504531a426160f757720a7aecd31` | 8480 / `7cb5c79668206ac4a2c15d77d7687e17413737c2bafdaa3c6f1848e3d38703cf` |
| `client/sqwt/widget_c/PcProfileWidget.tpl` | 1325 / `2c39e019a2801b116ad6f8b16462632daa535305ed10797298d07ea23ed677cc` | 1317 / `49c5a88d1f457051254a72175b40caac36e10552f00611b38ff6a8afeddc648f` |
| `client/sqwt/common_c/default.style` | 268669 / `027e8cf950f36fe22a1409980ceb1b35b7ba1ded57b32984b35b89bafe939f79` | 268661 / `a23aba1c100805ef2762eaf13eb1e6e632bfd3e8abbb0355c99253aee3b2de9f` |

The same installation's `game.ver` was ASCII `2012.09.19.0001`, 15 bytes,
SHA-256 `3dbeed87ae1f2805bec69d63137e3deca9dd444ad5caa3ea5dedb68b8a50bfef`.
Its `patch.ver` was ASCII `1.23b`, 5 bytes, SHA-256
`cbe2f3b81d7b70aafe1eeca3ccbfc573f4b5156245bf697fc9dafd82527df9b6`.
Those labels identify the inspected installation context, not per-file
delivery history or active rendering.

In the form's SQWT XML namespace, `.//Button` selects 22 elements. All have
`Template="{StaticResource ControlTemplate_Button_EquipIcon}"` and
`SqwtStyle="BTN_null"`. Their Name set equals the 18 Lua control names
above plus these four ordinary accessory declarations:

- `Button_Accessories_LeftEar`
- `Button_Accessories_LeftBracelet`
- `Button_Accessories_RightFinger_2`
- `Button_Accessories_LeftFinger_2`

The selected Lua slot mapper and update dispatch do not handle those four
names. Their authored presence does not prove that they appear or function
in the retail window. No Button or other form element declares a job-stone
name, a soul-crystal catalog ID, or a binding from active job to an icon.

The form's merged dictionary `widget_c/PcProfileWidget.tpl` declares one
`ControlTemplate`, with key `ControlTemplate_Button_EquipIcon` and target
type Button. Its only `IconControl` is named `IconControl_EquipIconBase` and
contains one `IconData` with `IconID="365"`. The remaining visual elements
are a Grid, a Border and the hover-effect Label. It declares no additional
gear button or job/crystal binding. Its event triggers issue mouse/keyboard
focus commands. These observations use `.//ControlTemplate`,
`.//IconControl`, `.//IconData` and `.//EventTrigger` as XML locators;
namespace prefixes do not change their local names.

`Window/@SqwtStyleFile` names `common_c/default.style`. Each of the ten
distinct `(element type,SqwtStyle)` pairs used by the form and equipment
template matches a declared style under the corresponding
`StyleResources/Widget[@Type]/SqwtStyle[@Name]`. Those selected style
subtrees contain only Parts and Property elements. `BTN_null` assigns
`Skin="NULL"` for every skin condition and supplies font/offset properties.
The other selected styles assign named skin resources and font properties;
they do not declare controls or active-job bindings. This is exact XML
membership and property inspection, not proof of native style resolution.

Two dependency boundaries remain concrete. The form also references
`common_c/WINBasisWithTitle_noGrayout.tpl` and requests window template key
`ControlTemplate_Window_WINBasisWithTitle`; that exact dependency file was
absent from the inspected installation, so its template was not decoded.
The style's root Source names `common_c/default.skin`, whose image resources
were not inspected. Native-created children, fallback resource resolution,
skin images and actual rendered visibility remain outside this declaration
inventory. The available form/template/style therefore supplies no authored
crystal control, while the complete rendered appearance remains unobserved.

## Job header and crystal boundary

`setCharaSkillName` gets `getStateMainSkill`, `getStateMainSkillLevel` and
`getMainClassOrJob` from the examined actor. When class and class/job match,
it substitutes job argument 0; otherwise it retains the job. It sets
`TextBlock_SkillRank` through text row 231 with `(class,level,job)`
(`widget/pcprofilewidget.lua:45-69`).

`getMainClassOrJob` returns the base skill for a non-player; for a player it
returns nonzero `_getJob()`, otherwise the base skill
(`chara/charabaseclass_ffxivbattle.lua:91-110`). The
[active-job state contract](../actor/job-soulstone-state.md) owns the native
job byte, its receive path and its distinction from important-item
possession. Here the job value is an input to text formatting. The inspected
profile script does not use it to select an item catalog or icon.

Literal searches over both selected profile scripts found no
`Button_JobStone`, `Grid_JobStone`, `getJobItemId` or `hasJobStone`.
Inspection of their complete setters, initialization, update handler,
focus mapping and item-detail consumer found no corresponding crystal
branch. The form and equipment-template inventory above independently
establishes that those identified resources declare ordinary gear controls
and no crystal control. It does not prove rendered absence across the
missing window template, native-created children, skin resources and a
retail visual observation.

The exact remaining edges are the executed 24238 descriptor/policy to its
serialized request, that request to a correlated incoming update sequence,
and the remaining profile resource/native presentation to an observed retail
window. Adding a remote crystal slot is therefore not established as a
retail requirement by this evidence. Neither active-job text nor local
package-101 ownership supplies the missing presentation edge.

## Reproduction

Authenticate the executable and selected canonical Lua members against the
identities above. Use PE section mapping, not `VA - base` as a file offset.
Decode each selected function from its actual entry or the bounded
instruction ranges above in x86-32 mode. Confirm the data dwords
`[0x00FE050C] == 0xFE` and `[0x00FE0520] == 0xF9`, the binding string at
`0x00FD846C`, and execution-table entry `[0x01057154] == 0x0089B3F0`.
Select `0x0146` with the high dispatcher tables described in
[receive dispatcher ranges](../net/receive-dispatch-ranges.md), then follow
the listed direct calls and the retained record fields.

For Lua, inspect the cited functions in full, compare the explicit button
map with the item-update dispatch, and search the two complete profile
scripts for the crystal consumers listed above. This procedure reproduces
the static source/cache contract. For the named form, replay the three
identified SQEX exports, parse their exact documents, compare `.//Button`
Names with the Lua map, inspect the equipment template's descendants and
join each used style by element type and style name. Preserve the missing
window-template and unread-skin boundaries. A retail visual specimen or
further bounded presentation evidence is required for the rendered-crystal
question.
