# Escape routing and key sheet selection

This page records which key configuration sheet is live, how a null target
command reaches widgets, native game elements and the desktop, and what
Escape does in ordinary play on the default sheet. No client ran and nothing
here is a live observation.

## Input and method

Retail `ffxivgame.exe` 1.23b (`2012.09.19.0001`), `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. Ghidra CLI disassembly and cross references, with selected
instructions checked with `llvm-objdump` and RTTI, vtable cells and tables read
from the PE. Installed LPB scripts were decoded with
`xivl-client-scripts:tools/retail_script.py` and read as bytecode listings from
the pinned `unluac_2025_12_23.jar --disassemble`, with zero based PCs. Markup
was extracted with `xivl extract-resource --as sqwt` and sheets with
`xivl extract` from `xivl-tools` commit `9c44b51`. The markup census covered
1155 decoded `client/sqwt` SQEX files and the script census 2671 decoded Lua
chunks.

Tiers are STATIC (instructions or data, bytecode listings included), INFERRED
and UNKNOWN. A statement marked "decompiled Lua" was read only through
decompilation and is INFERRED.

Related pages: [main menu command route](../script/main-menu-command-route.md),
[target command handler](../net/target-command-handler.md),
[movement command origin](player-movement-command-origin.md),
[CommandParameter consumer](../resource/sqwt-command-parameter-consumer.md),
[CharaElement interface attachment](chara-interface-attachment.md) and the
client structs
[user config store](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/user-config-store-contract.md).

## Sheet selector

STATIC.

- Profile index global `0x01336BC4` lies in the zero filled tail of `.data`, so
  it is 0 at load. Its only direct writer is setter `0x00547340`, called only
  from `0x004DB4B3` and `0x004DBA6E`. Computed writers are not excluded.
- Index 0, 1 and 2 are `key_config`, `key_config2` and `key_config3`.
  `0x00547320` takes -1 as the current global.
- Both callers read the int record (group 0, key 9) of the game config store
  `W+0x17430` through `0x00443E40(0, 9, 0)`, pass it to the setter and reparse
  through `0x0054CC50` (`0x004DB4A5..0x004DB4C1`,
  `0x004DBA60..0x004DBA7C`). `0x00443E40` addresses record
  `(group * 32 + (key & 0x1F)) * 0xBC` and returns its `+0x60`.
- Group 0 key 9 is `input_mode`, an int with range words 0 and 1 and
  constructor default 0. A per character `game` file overrides it. The range is
  not enforced on the traced paths.

So `key_config` is selected by default and `key_config2` when `input_mode` is 1.
No traced path writes 2, so `key_config3` was not found as a live profile.
This selector is distinct from the SystemConfig `[2,8]` read at `0x004B3197`
and from game store `[2,8]`, the `keyconfig` override string.

### Reselect

STATIC.

- The game store is built with listener `W+0x4A4`
  (`0x004DC11D..0x004DC12B`, constructor `0x004C4750`, stored at store `+0x20`
  by `0x004B66F0`). The listener is `0x004DB800(store, group, key, flag)`.
- It acts only for the game store (`0x004DB80C..0x004DB818`). For group -1 or 0
  it rereads `input_mode`, sets the global and reparses
  (`0x004DBA56..0x004DBA7C`), whatever the key.
- A file load (`0x004B8540`) calls it with group -1 and key -1.
- The Lua int setter `0x0075C980` calls `0x00443DD0` and then
  `0x004B6780(0, group, key)`, which calls the listener. The Lua string setter
  and reset also call `0x004B6780` (`0x0075CA98`, `0x0075CA56`).
- `0x00443DD0` alone does not notify. Native writers `0x00508470`,
  `0x005086B0`, `0x0052DD90` and `0x00528F20` call it with a pair from the table
  at `0x00F94D88` and make no listener call. Table entry 9 is (0, 9). Which
  controls bind which entry was not traced, so not every write is known to
  reselect.
- `0x004DAF00` (callers `0x004DB5B0`, `0x004DCC73`, `0x004DCCA0`) empties the
  `keyconfig` string (`0x004DB468..0x004DB48F`), then reselects and reparses.
  When each caller runs was not traced.
- The first parse is `0x00553BE0` calling `0x0054CC50` after the three sheets
  complete.

`ConfigKeyboardWidget` `ToggleButton_Controler` carries Lua config work id 10
([keyboard widget contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/config-keyboard-widget-contract.md)).
The desktop connector maps id 10 to Lua (group 1, key 10), native (0, 9)
(`getConfigCategory`, `main/f484` PCs 55 to 58), and `setConfigWork`
(`main/f485`) calls `_setUserConfig` (PCs 15 to 19). The toggle help text is
`@77414` in the form markup, a gamepad oriented keyboard layout whose mappings
are locked while it is on.

### Type A and Type B

STATIC. `ConfigKeyboardAskWidget.form`
(`071b4a7312a8fbdef4693232ae8a1175be4cb2beaa6c70764f597ad124740b87`) `Button_1`
sends `Default` to `KeyConfig_Default`, which `ConfigKeyboardWidget.tpl`
(`5d5fcca1c4df415c4efbfbe25d84548d055384d267c5827179a40980ce123bc9`) turns into
`KeyConfig.Default` with the same parameter. `KeyConfig.Default` (command
object `0x01340C78`, initializer `0x00F12580`) is handled at
`0x00683EE9..0x00683F49`. Parameter 0 selects profile 0 and nonzero selects
profile 2 (`0x00683F0A..0x00683F11`). It calls
`0x00682930(j, W, text, 1, profile)` for j = 0 and 1, which for a profile other
than 1 clears substitution map j and gathers it again from that profile text.
It writes neither `0x01336BC4` nor `input_mode`. So Type A refills the
substitution maps from `key_config` and Type B from `key_config3`, while the
live rows still come from the selected profile. UI text rows 1495 and 1496 name
the two, and rows 77701 and 77702 describe them as the FFXIV and MMORPG
standard resets. `KeyConfig.Save` is covered by
[its command contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/keyconfig-save-command-contract.md).

### Sheet defaults

STATIC data. `key_config` and `key_config3` share 310 row ids. With the text
after `=` removed from each `[@name=...]` token in columns 3 and 12 to 15, every
cell matches except 8: rows 187 (`Map.ScaleDown`) and 188 (`Map.ScaleUp`),
columns 12 to 15, hold literal `PageUp` and `PageDown` swapped between the two.
Column 3 of those rows matches. The sheets therefore differ in variable
defaults and in those literal cells.

| Rows | Command | `key_config` | `key_config3` | `key_config2` |
| --- | --- | --- | --- | --- |
| 20, 21 | `UILuaCommands.Cancel`, `MoveFocus` 10 | Escape | Back | Rows 40 and 41, literal Escape |
| 22, 23 | Same pair, second slot | Empty | Decimal | Not compared |
| 115 | `UILuaCommands.MainMenu` | Home | Escape | Rows 44 Home, 45 Subtract |
| 116 | `UILuaCommands.MainMenu` | Subtract | Subtract | Not compared |
| 117 to 162 | ShortCut menus and map | Empty except row 141 `Key2DMap1` = M | Not compared | Menu letters N G K I J P Y B U and map M |
| 153 | `ShortCutActionLogoutMenu` | Empty | `Escape Control` | Not compared |

For rows 20 to 23, 115 and 116, column 3 and columns 12 to 15 agree. What retail
character files held for `input_mode` and `keyconfig` is UNKNOWN.

## Two commands in row order

STATIC. On `key_config`, Escape issues row 20 `UILuaCommands.Cancel` and then
row 21 `ApplicationCommands.MoveFocus` with int 10. The binding walker runs the
command Execute (`0x00921233`). Only a return of 1 (handled) reaches the gesture
slot that marks the key event handled (`0x00921235..0x0092126D`). Later
bindings without HandledEventsToo are then skipped
(`0x0092119F..0x009211AD`). Execute returns the handled byte of the last routed
event (`0x00927FA0`). So `MoveFocus 10` is issued only when nothing handled
Cancel.

## Null target route

STATIC unless marked.

- With keyboard focus `0x01357020` null, nothing is routed.
- `0x0091EE90` snapshots the route pairs with a desktop pair first
  (`0x0091ED80`). When focus is not the desktop, the pair of the focused root
  moves to the back (`0x0091EEF4..0x0091F00B`). For Executed (bubble) the list
  is reversed (`0x0091F01A..0x0091F046`): focused pair first, the other pairs in
  reverse vector order, and the desktop pair last when focus is elsewhere. With
  focus on the desktop the desktop pair is last. With no pair for the focused
  root key, nothing is routed (`0x0091F0A0..0x0091F0D0`).
- Every pair is walked. Only stop byte `+0x17` ends the walk, and handled byte
  `+0x19` does not (`0x0091F134..0x0091F1B8`).
- Each pair calls `0x0091E1C0(event, R, isFocusedPair, 0, 1)`
  (`0x0091F192..0x0091F199`). The desktop is not appended. An element joins the
  chain only if `isFocusedPair` is 1 or its interface byte `+0x70` has bit 1
  (`0x0091E267..0x0091E272`). The parent walk stops before the desktop.

### Interface bit 1

STATIC.

- The InputElement constructor sets bit 1 (`0x0091E103..0x0091E123`), and
  `Sqwt::UIElement` constructor `0x0093C750` clears it on its interface at
  `+0xB4` (`0x0093CBF4`).
- On a UIElement the bit then mirrors property `IsRoutedInputEventInactive`
  (descriptor `0x01359548`, ordinal 3, initializer `0x00F22950`, value byte
  E+`0x15B` bit 5, cleared by the constructor at `0x0093C916`). Changed callback
  `0x0093B700`, arm `0x0093B758..0x0093B769` (table `0x0093BD38`), copies it
  into E+`0x124` bit 1, which is interface `+0x70`.
- Setter `0x0093C360` has two direct callers. Window constructor `0x009246F0`
  passes false (`0x00924B65..0x00924B6B`). DesktopWindow constructor
  `0x0091B020` passes true (`0x0091B2E6..0x0091B2ED`) just before storing itself
  in desktop global `0x01356E10` (`0x0091B2F3`).
  `RaptureElementContainer` constructor `0x004DBF40` runs it. The write and
  notify path is `0x0099C780 -> 0x0099C680 -> 0x00590380`, which sets argument
  `+0x78` bit 1 that gates the callback switch.
- The property also has a by name setter arm (`0x0093D125`, reached through a
  data cell). The name occurs in none of the 1155 decoded sqwt files and 2671
  Lua chunks. Computed writes are not excluded.
- By RTTI, `TargetElement` and `CharaElement` derive from `RaptureElement` and
  `InputElement` and not `UIElement`. `Sqwt::Window` and the desktop derive from
  `UIElement`. No traced setter clears bit 1 on the two native elements
  (`0x0091C3E0`, which the TargetElement constructor calls, writes bit 0).
- `TargetElement` enrolls its own route entry: constructor `0x00566880` passes
  `this+8` to `0x0091DFB0` (`0x00566A34..0x00566A37`).

So a null target command reaches the chain from the focused element to its root
window, then native game elements such as `TargetElement`, then the desktop,
even when another window holds focus. The desktop chain starts at the desktop,
whose bit admits it as an unfocused pair (`0x0091E260..0x0091E2A0`). Windows
without focus contribute an empty chain. The limits are computed writes and
live route vector contents, which were not observed.

Arrival is not action. The Lua form handler never runs for an already handled
event ([handler matching](../resource/sqwt-command-parameter-consumer.md#handler-matching)),
and `TargetElement` slot 65 and the FrameworkElement MoveFocus arm require the
handled byte clear. The desktop script therefore acts only on commands nothing
earlier handled, with the desktop as sender, so its control name is `_widget`.

## Who handles Cancel

STATIC. Base Executed handler `0x0091D8F0` runs the Lua subscriptions first and
then sets handled when the command is in the element consumed list
(`0x0091D870`, interface flag `+0x70` bit 4, table `0x01357030`). The Lua form
handler `0x00536280` does not set handled for a script delivery. Its `+0x19`
writes are the form gate object arm (`0x00536300..0x005363FB`). Which live
forms own a gate object was not traced.

The consumed list is filled from the Window `ConsumedCommand` property
(property ordinal 15 at `0x00F20CCF`, callback `0x009238A0`, switch
`0x009238E4..0x009238F4`, arm `0x00923B5B`). The arm clears the list, splits the
text on commas and adds each command through `0x0091F5D0`.

Scripts rewrite the property. `WidgetBaseClass.initCommon`
(`widgetbaseclass_common` `main/f0` PCs 71 to 82) calls
`initConsumedCommmand` (the client spelling) for every widget whose window name
is not `Desktop`. That method (`main/f1` PCs 0 to 68) writes a
`ConsumedCommand` that starts with `UILuaCommands.Operate`,
`UILuaCommands.Cancel`, `WidgetClose`, `Shown`, `Activated`, `OpenUserMacro`,
`ShortCutAction` and `UIOperatorCommands.Close`, `OpenChild` and `Request`,
followed by the form's other entries. That `_onInit` calls `initCommon` before
the widget `init` is decompiled Lua. A raw string census over the decoded chunks
finds the property in two classes besides the common one: `NpcSayWidget` resets
Cancel and `ActionMenuWidget` resets only `OpenUserMacro`. `Desktop.form`
(`c1c55278e57ea5cd557bc14e26fb751ab2bd1d77d637f5e250bd46529186a24e`) does not
list Cancel.

So every non desktop `WidgetBaseClass` widget except NpcSay consumes Cancel at
run time, whatever its form says. A census of forms alone is wrong: 25 non `sd`
`widget/*.form` files list Cancel, and `MainMenuWidget.form` does not but
consumes it after init.

## Who handles MoveFocus 10

STATIC.

- Every FrameworkElement that receives an unhandled MoveFocus marks it handled
  and runs its focus move slot `+0x64` (`0x0092D23A`,
  `0x0092D2FD..0x0092D326`). The arm has no focus or source test, so the desktop
  does this too when nothing earlier handled the command.
- `TargetElement` slot 65 `0x00565E10` requires a clear handled byte, a current
  cursor, its enable byte and `ChangeTargetMode` 0. For `MoveFocus 10`, a zero
  `RaptureUserControl` counter and a pick predicate that accepts null then
  select null on the current cursor index (`0x00565E8D..0x00565EDD`,
  `0x00562D30`). When the counter or predicate blocks, the command is still
  marked handled (`0x00566079`).
- `CharaElement` slot 65 `0x00589490` compares five command objects
  (`0x01339218`, `0x013390D8`, `0x01339178`, `0x013392B8`, `0x013393F8`), none
  of them Cancel or MoveFocus, in its linear body.

## No native gameplay Cancel branch

`UILuaCommands.Cancel` is object `0x0133B838` (initializer `0x00F0F2D0`). Its
cross references are the initializer, the exit destructor and one compare at
`0x00536629` in the form handler. That compare is a per form, once per frame
limit shared by Operate, Cancel, WidgetClose, Selection, SelectionChanged and
SelectionChangedOnce (`0x00536622..0x00536663`, flag form `+0x1EC`). STATIC,
with computed references not excluded. The only native game effect of Escape
found is the separate `MoveFocus 10` target clear.

## Escape cases

Open auto-translate completion takes Escape first, as the client structs
[auto-translate contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/autotranslate-selection-contract.md)
records.

### Focus in a widget window

STATIC for consume, walker and route rules. Cancel bubbles from the focused
element to its window. The window subscriptions run, the window consumes it, so
row 21 is not issued and target and sub target are untouched. Only the focused
chain acts. This is not a walk of windows by draw order. A widget with no Cancel
behaviour still consumes Escape.

| Widget | Behaviour | Locator | Tier |
| --- | --- | --- | --- |
| Base class | `processUICommandCancel` forwards to close | [root slot contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/desktop-widget-root-slots.md) | STATIC |
| MainMenuWidget | `processUICommandClose` hides | [main menu route](../script/main-menu-command-route.md) | STATIC |
| LogWidget | With `Grid_ChatMenu` focus within: hide the chat menu and focus `Button_ChatInput`. Otherwise save input history and `setDeactiveBatch` | `processUICommandCancel` PCs 0 to 18, `setDeactiveBatch` PCs 0 to 28 | STATIC |
| ActionMenuWidget | With a decided target and a macro bar shown: hide the bar only. Otherwise hide it, clear animation and call `updateActionMenuWidget(false, false)` | `processUICommandEvent` PCs 107 to 143 | STATIC |
| ActionMenu follow on | That call clears the decided flag and, by `getConfigFlag(1)` (`nohide_action`, default 0), hides the menu or returns focus to Desktop | connector `main/f168` PCs 18 to 72 | STATIC calls, INFERRED resulting focus |
| StatusEffectWidget | With keyboard focus: send `ApplicationCommands.FocusOrderToBottom` | decompiled Lua | INFERRED |
| ConsoleIconTray, ItemUse | `cancelFocus`, `previousSequence` | decompiled Lua | INFERRED |

Other widgets were not audited one by one. Of 202 decompiled
`client/script/n1635q` chunks, 153 mention Cancel or `setCancelCondition`. The
TextBox KeyDown step before the desktop bindings was not traced, so that Escape
reaches the binding from chat entry is INFERRED.

### NpcSayWidget focused

NpcSay leaves Cancel unhandled, so Cancel continues to the other pairs and the
desktop, where the cutscene arm below runs if its gates pass (STATIC). Row 21
then follows. That a FrameworkElement in the focused chain takes MoveFocus, so
that `TargetElement` sees it handled, is INFERRED. Whether NpcSay ever holds
keyboard focus was not traced.

### Desktop

1. `TargetElement` and other native pairs ignore Cancel. `DesktopWidget` handles
   it only in cutscene mode: with `isCutSceneMode()` true it shows or skips
   static widget 12, or shows static widget 13 (connector `main/f62` PCs 24 to
   53). It requires `work.mode > 1` and sender `_widget` (PCs 0 to 20, whose
   focus test affects only OpenUserMacro and ShortCutAction). STATIC. This arm
   is reached by any unhandled Cancel, not only under desktop focus.
2. The desktop does not consume Cancel, so `MoveFocus 10` follows.
   `TargetElement` precedes the desktop pair.

| State | Effect | Locator | Tier |
| --- | --- | --- | --- |
| Sub target selection | Escape clears the sub cursor. `afterTargetChanged(subIndex, nil)` calls `subTargetDecided(nil)`, which ends the mode. The main target stays. `executeSubTarget` making the sub cursor current with Desktop focus is decompiled Lua | `afterTargetChanged` PCs 66 to 75, `subTargetDecided` PCs 0 to 12, `cancelSubTargetSelect` PCs 0 to 8 | STATIC script arms, INFERRED native clear to callback join |
| Main target held | Current target set to null. `afterTargetChanged` calls `cancelLockon` for cursor 1 and nil, and `_onTargetChanged` plays the cancel sound | `afterTargetChanged` PCs 3 to 9, `_onTargetChanged` PCs 0 to 15 | STATIC arms, INFERRED join |
| No target | Null is selected again, and select sends only on change | [send conditions](../net/target-command-handler.md#send-conditions) | STATIC |
| Cursor control locked | Target kept, MoveFocus still handled | `0x00565EAD`, `0x00566079` | STATIC |

That ordinary world play leaves keyboard focus on the desktop is INFERRED
(scripts return focus with `changeFocusedWidget(Desktop)`).

### Cast cancel and system menu

No Escape path to a cast cancel was found. `cancelPlayerCastCommand`
(connector `main/f297`, command 21006) appears in the connector and
ActionMenuWidget chunks only (STATIC census). The ActionMenu route is a repeat
press of a custom slot: `getConfigFlag(15)` (native (0, 14) `action_cancel`,
default 0) on, a pressed slot of type 2 equal to the casting command and cast
time above 0 (`actionmenuwidget` `main/f13` PCs 42 to 85). STATIC. Other native
cast cancel causes were not searched, and any server response to a target clear
during a cast is UNKNOWN.

On the `key_config` row defaults Escape never opens the main menu (STATIC). A
`keyconfig` substitution can rebind `KeyMainMenu1` and `KeyMainMenu2`. On
`key_config3` defaults, row 115 is Escape and row 153 is Escape with Control for
the logout menu.

### Desktop commands under window focus

STATIC for the route: an unhandled `UILuaCommands.MainMenu` with a null target
reaches the desktop while a window holds focus. The common consumed list does
not include MainMenu, and the DesktopWidget MainMenu arm has no keyboard focus
test, though it has the gates of the
[main menu route](../script/main-menu-command-route.md#desktop-branch). End to
end behaviour of Home on `key_config`, or Escape on `key_config3`, with a window
focused is UNKNOWN: raw key handling in the focused control, per form
consumption and the resulting widget state were not traced.

## Open items

- Live keyboard focus and route vector contents in ordinary play.
- Computed writes to interface bit 1 or `IsRoutedInputEventInactive`.
- The controls that bind the `0x00F94D88` store writers.
- Focus after a window closes or ActionMenu hides, and timing against
  animations.
- Form gate object (`form+0x27C`) consumption, the early return at
  `0x005362CC..0x005362D8` and TextBox raw Escape outside auto-translate.
- Cancel behaviour of widgets outside the table, including modal and Ask
  flows.
- Pad Cancel, and when each `0x004DAF00` caller runs.

## Reproduction

1. Check the executable digest.
2. List references to `0x01336BC4` and `0x00547340`. Disassemble `0x00443E40`,
   `0x00547320`, `0x00547340`, `0x004DB4A5`, `0x004DB800..0x004DBAC4`,
   `0x0075C980`, `0x004B6780`, `0x004DC11D`, `0x004C4750` and `0x004B66F0`.
   List references to `0x00443DD0` and read 28 dword pairs at `0x00F94D88`.
3. Disassemble `0x00683E80..0x00683F6C` and decompile `0x00682930`.
4. Disassemble `0x00921150..0x00921284`, `0x0091EE90..0x0091F1C6`,
   `0x0091ED80`, `0x0091E1C0`, `0x0091E090`, `0x0093C750..0x0093CBF8`,
   `0x00566880..0x00566A3C` and `0x0091DFB0`. Read the RTTI base arrays of
   vtables `0x00FA4F9C`, `0x00FA7B2C`, `0x00F911BC` and `0x01069F94`. For bit 1
   read `0x00F22950`, `0x0093B700..0x0093B781` with table `0x0093BD38`, the
   references to `0x0093C360`, `0x00924B40..0x00924B70`,
   `0x0091B2B0..0x0091B2F9`, `0x0099C780`, `0x0099C680`, `0x00590380` and
   `0x0093D125`.
5. Disassemble `0x0091D8F0`, `0x0091D870`, `0x009238D0..0x009238F4` with byte
   map `0x00923CF0` and table `0x00923CC0`, `0x00923B5B..0x00923C82`,
   `0x00F20CC0`, `0x0092D1F0..0x0092D326`, `0x00565E10..0x00565EE2` and
   `0x00589490`. List references to `0x0133B838`.
6. Read the cited script PCs from these carriers under `client/script/`.

| Script | Carrier | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| widgetbaseclass_common | `n1635q/n1635q89r57y9rr_7vxxvw.le.lpb` | 32996 | `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c` |
| desktop connector | `n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| actionmenuwidget | `n1635q/97q1vwx5wpn1635q.le.lpb` | 25126 | `bceeae7dc8ac024767ba65a7c1ecbf8682f7c4e89b5c4eb55310d4e00e58af31` |
| npcsaywidget | `n1635q/wu7r9ln1635q.le.lpb` | 2215 | `798ba922eef828d0d0e204435a3147c7c3714319f3535a2c710d41ee6d284920` |
| logwidget | `n1635q/yv3n1635q.le.lpb` | 19090 | `d99be24e3e64e9d105b25b722d119ee349c5f9244ddddf4e2e1f048c32355ad7` |
| configkeyboardwidget | `n1635q/7vw413z5l8v9s6n1635q.le.lpb` | 1613 | `d360c57e4d76157a0521ca64a3d6d65b1f6aa91acc9ef915c6da6819006e5da9` |
| configkeyboardaskwidget | `n1635q/7vw413z5l8v9s69rzn1635q.le.lpb` | 499 | `46a42f88aecfbb66981587efdbbff63d4a192da881e708d4394c14a19acedcb8` |
