# Main menu command route

The `key_config` sheet binds `Home` and `Subtract`, as row defaults, to the
command `UILuaCommands.MainMenu`. The DesktopWidget connector subscribes to that
command and, past its gates, toggles static widget 1, which it creates as
`MainMenuWidget`. The widget fills a list named `MainMenu` with 19 rows whose
row indices run from 0 to 18, and its selection handler maps the selected index
to a widget name or a player command. `UILuaCommands.Cancel` and
`UILuaCommands.WidgetClose` both end in the widget's close method, which hides
it. This page records the sheet rows, the script chain, the form facts and what
the evidence leaves open.

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| MEASURED | Read from the decoded sheet rows or the decoded Sqwt markup of the install. Rows and markup bodies are not tracked |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe`, as recorded on the cited tracked page |
| INFERRED | A role or join taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The script method is the one of
[Work update callback consumers](work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f76` name the direct children of the chunk root in listing order.
`tools/decode_lpb.py` yields the same decoded chunks. No client code ran.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `widget/mainmenuwidget` | `client/script/n1635q/x91wx5wpn1635q.le.lpb` | 19491 | `c0e1782a3ca183fcaa520a16320946f62d3fc861c74978cde19ea20fcd064b0a` |
| `widget/communitymenuwidget` | `client/script/n1635q/7vxxpw1qlx5wpn1635q.le.lpb` | 1210 | `b64803fdfc9a8be150bc015af58b16a67b0edaa5395854705579dd96ac3adc14` |
| `widget/widgetbaseclass` | `client/script/n1635q/n1635q89r57y9rr.le.lpb` | 4080 | `aad807a42296e93301582defc8f15e4003b8093af672794282cd54950cc556ad` |
| `widget/widgetbaseclass_common` | `client/script/n1635q/n1635q89r57y9rr_7vxxvw.le.lpb` | 32996 | `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `widget/desktopwidget_connector` | 213474 | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/mainmenuwidget` | 19478 | `185d856d406d46ba7416444ddc2212b68036f7d56d0ee62148be2567630c90b2` |
| `widget/communitymenuwidget` | 1197 | `832761807f87d257c46d9fced14243f0c7226e85e34329ec1829cdb1bac384aa` |
| `widget/widgetbaseclass` | 4067 | `11e5bab509c4dbbd7e2767d35cdf132a59b5c510c32c2e215f83cac713115140` |
| `widget/widgetbaseclass_common` | 32983 | `7c45426b1a9f168557d84e420c5767c6fdea4abf90b976b276546af4e6a5de5f` |

The markup is the decoded document that `xivl extract-resource --as sqwt`
writes for each file below. `widget/MainMenuWidget.form` is the file the opener
loads per
[Sqwt form locale resolution](../resource/sqwt-form-locale-resolution.md).

| Client-relative markup | Bytes | SHA-256 | Decoded bytes | Decoded SHA-256 |
|---|---:|---|---:|---|
| `client/sqwt/widget/MainMenuWidget.form` | 3798 | `b7da0ae2a409ab4002b9bac5474bfe73fa07d32543a2882d8acdfd7dcd80e431` | 3790 | `dcf22124f067eddcb70acbbbad10a8fceddb0a17c138ba21dd20129d10739a90` |
| `client/sqwt/widget/MainMenuWidget.tpl` | 13335 | `82a503ee4f4faa7b6b329f4295a1158aa194a1dea530333935093e328902deb2` | 13327 | `c0ba17e8ef7c4710e92590c0c428b6e75c9c7fa7d11ea75332ebed497b0230fb` |
| `client/sqwt/widget/CommunityMenuWidget.form` | 1334 | `e19720c4c65f1a69ec2d2777d691dc4560c499634606cb93d7d9a4d3d6597786` | 1326 | `708c47bb442d8dd52d903bf15727b1997b91c7b3676bae967d2e1fe88866cea7` |
| `client/sqwt/common/WINBasisWithTitle.tpl` | 7175 | `636d5dd7dbdb0b9d12b08cea875ee0bde729a1e9de9c1cd58eea2d7f66d06daa` | 7167 | `93c745038827c5608f09f4aeb2b4186c06138f8f7f7d45c0a2a2483faf11f88b` |
| `client/sqwt/common/default.style` | 353783 | `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` | 353775 | `33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6` |

The sheet rows are the `key_config` definition `0x01030169` with data
`0x0103016A`, enable `0x0103016B` and offset `0x0103016C`. The four resources
of the install have the byte sizes and SHA-256 values listed in
[Retained source definitions](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/movement-binding-rows.md#retained-source-definitions).
Each row was decoded with the 16 column types of the definition. A row key is
the index of its span in the offset resource, and a column number is the
zero-based position in the definition, which is the labeled column of that
page. Binding text is quoted in the escaped spelling of the `key_config.csv`
export pinned in `xivl-client-data:manifests/tables.json`.

## Key bindings

MEASURED. In every row below, column 1 is 128, column 5 is 0, column 7 is 0 and
column 11 is 1. Column 6 is 1 in rows 1001 and above and 0 in the others.

| Row key | Column 2 | Column 3 | Column 4 | Column 8 | Column 9 | Column 10 |
|---:|---:|---|---|---|---:|---|
| 11 | 0 | `\[@KeyOperate1=Enter]` | `**` | `ApplicationCommands.Operate` | 0 | `**` |
| 12 | 0 | `\[@KeyOperate1=Enter]` | `**` | `ApplicationCommands.MoveFocus` | 1 | `9` |
| 20 | 0 | `\[@KeyCancel1$=Escape]` | `**` | `UILuaCommands.Cancel` | 0 | `**` |
| 21 | 0 | `\[@KeyCancel1$=Escape]` | `**` | `ApplicationCommands.MoveFocus` | 1 | `10` |
| 115 | 0 | `\[@KeyMainMenu1$=Home]` | `**` | `UILuaCommands.MainMenu` | 0 | `**` |
| 116 | 0 | `\[@KeyMainMenu2$=Subtract]` | `**` | `UILuaCommands.MainMenu` | 0 | `**` |
| 229 | 8 | `RRight` | `**` | `ApplicationCommands.Operate` | 0 | `**` |
| 230 | 8 | `RRight` | `**` | `ApplicationCommands.MoveFocus` | 1 | `9` |
| 231 | 8 | `RDown` | `**` | `UILuaCommands.Cancel` | 0 | `**` |
| 232 | 8 | `RDown` | `**` | `ApplicationCommands.MoveFocus` | 1 | `10` |
| 234 | 8 | `RLeft` | `**` | `UILuaCommands.Function1` | 0 | `**` |
| 1001 | 0 | `\[@KeyLeftFocus1=Left]` | `**` | `ApplicationCommands.MoveFocus` | 1 | `1` |
| 1013 | 0 | `\[@KeyUpFocus1=UP]` | `**` | `ApplicationCommands.MoveFocus` | 1 | `2` |
| 1019 | 0 | `\[@KeyRightFocus1=Right]` | `**` | `ApplicationCommands.MoveFocus` | 1 | `3` |
| 1031 | 0 | `\[@KeyDownFocus1=Down]` | `**` | `ApplicationCommands.MoveFocus` | 1 | `4` |
| 1043 | 8 | `LLeft` | `**` | `ApplicationCommands.MoveFocus` | 1 | `1` |
| 1044 | 8 | `LUp` | `**` | `ApplicationCommands.MoveFocus` | 1 | `2` |
| 1045 | 8 | `LRight` | `**` | `ApplicationCommands.MoveFocus` | 1 | `3` |
| 1046 | 8 | `LDown` | `**` | `ApplicationCommands.MoveFocus` | 1 | `4` |

MEASURED. These further rows repeat a row of the table with one change.

- Rows 13 and 14 repeat row 11 with column 4 `Control` and `Alt`.
- Rows 16, 18 and 19 repeat rows 11, 13 and 14 with the empty second slot
  `\[@KeyOperate2=]`, and row 17 repeats row 12 with it.
- Rows 22 and 23 repeat rows 20 and 21 with the empty second slot
  `\[@KeyCancel2$=]`.
- For each of the four directions the six consecutive rows that start at 1001,
  1013, 1019 and 1031 hold the first slot and the empty second slot with column
  4 `**`, then both with `Control`, then both with `Alt`.

`UILuaCommands.MainMenu` occurs in rows 115 and 116 only, and
`UILuaCommands.Function1` in row 234 only. Columns 12 to 15 repeat the column 3
text in every row above except four. Rows 229 and 230 hold `RDown` and rows 231
and 232 hold `RRight` in columns 12 to 15. Which of the five binding columns a
profile selects is not restated here.

STATIC. The table loader copies column 2 as the entry type and column 4 as the
modifiers text, with `**` read as empty. Columns 8 to 11 supply the command
name, the parameter kind, the parameter text and the anchor flag of each
command
([Configuration text and substitutions](../input/player-movement-command-origin.md#configuration-text-and-substitutions)).
The rows for `UILuaCommands.MainMenu`, `UILuaCommands.Cancel` and
`UILuaCommands.Function1` therefore carry no command parameter, and each
`MoveFocus` row listed above carries parameter kind 1 with the text in column
10. The sheet holds one further `ApplicationCommands.MoveFocus` row, key 251,
with the bare gesture `LeftClick`, column 2 16, column 9 0, column 10 `**` and
column 11 0 (MEASURED). It carries no parameter and is listed in
[Active Mode key](../net/target-command-handler.md#active-mode-key). The text
inside `\[@name=default]` names a configuration key and its default value on
that page. `Home` and `Subtract` are therefore the row defaults of
`KeyMainMenu1` and `KeyMainMenu2`, and a configuration value for either key
replaces the default.

### Decide and chat input

MEASURED. In `key_config`, profile 0, the first enabled range supplies
rows 10 to 14 and 24 to 26 to the desktop binding group. Unmodified Enter
matches the defaults for `LogControlPressKey`, `Operate`, and `MoveFocus`
with argument 9 in rows 10 to 12. Control+Enter and Alt+Enter match the
`Operate` defaults in rows 13 and 14. Chat entry is a separate command.

| Sheet row | Data resource span | Default | Command and parameter |
|---|---|---|---|
| `key_config` 24 | `0x0103016A [7425,7642)` | `KeyStartChat1$=Space` | `RaptureCommands.Chat`, int 0 |
| `key_config` 25 | `0x0103016A [7642,7834)` | `KeyStartChat2$` empty | `RaptureCommands.Chat`, int 0 |
| `key_config` 26 | `0x0103016A [7834,7967)` | `/` | `RaptureCommands.Chat`, int 191 |
| `key_config3` 11 | `0x0103055A [4974,5172)` | `KeyOperate1=NumPad0` | `ApplicationCommands.Operate`, none |
| `key_config3` 24 | `0x0103055A [7430,7647)` | `KeyStartChat1$=Enter` | `RaptureCommands.Chat`, int 0 |

The same defaults appear in columns 3 and 12 through 15 of these rows.
The `key_config3` resources and both enabled ranges are pinned in
[Retained source definitions](https://github.com/XIVLegacy/xivl-client-data/blob/4855dcebbf9531bf11ee777d303b3058badcee7b/docs/movement-binding-rows.md#retained-source-definitions).
The active profile and substituted values remain conditions on a match.

STATIC. In the `ffxivgame.exe` pinned by the
[keyboard binding trace](../input/keyboard-movement-bindings.md),
`0x00946CD0` scans name/value pairs at `0x012E2D68`. Ghidra and LLVM objdump
confirm that matching `Return` and `Enter` at `0x012E2D88` and
`0x012E2D90` returns the same code `0x0D` through `0x00946DD8`. For
admitted `KeyDown` recipient chains, `0x0091E1C0` collects the recipient and
its eligible ancestors, conditionally appending the Desktop interface. It
reverses the collection at `0x0091E31D`, then `0x0091D210` dispatches kind 0
backwards at `0x0091D238..0x0091D2BE`. This visits a collected descendant
before its ancestor within that chain.

OBSERVED. Standard `AskWidget` inherits `AskBaseClass.init`, whose
`main/f0` PCs 20 to 22 request `IsModal=true` through `setModal`.
`AskWidget.processAfterShow` requests focus on the default answer's
`TemplateButton_Answer`. `client/sqwt/widget/AskWidget.tpl` line 4 names
`UILuaCommands.Operate` as that Button's command. The template is 535 bytes
with SHA-256 `e9ffd9acfb89498efd2ffac3e346e0f826082443cba64c6fc91e6b931e9189ad`.
`AskWidget.initAsk` registers the exact template and control at priority 5,
and `processUICommandEvent` `main/f4` PCs 0 to 15 sets the parsed answer
number for the matching sender and command.
[AskWidget bytecode and focus](https://github.com/XIVLegacy/xivl-client-scripts/blob/7d1a5ca6e9a06806243951c107f6796270049db2/docs/ask-widget-answer-focus.md#default-answer-and-focus).

STATIC. Button Executed cell `0x01073C88` selects `0x0097EDB0`. After the
base call, `0x0097EDE3..0x0097EDF7` requires an unhandled command and
`ApplicationCommands.Operate` (`0x01357C88`). The handler sets command
handled at `0x0097EE41` and raises Click with the Button as source and
original source at `0x0097EE4D..0x0097EE90`. The Button's
[default Click handler](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
runs its retained command. This joins Layout0 Enter to the standard Ask
answer when the command reaches its admitted focused Button.

OBSERVED. The standard `LogWidget.form` `TextBox_ChatInput` at lines 78 to
80 instead owns a local `Return` binding to `UILuaCommands.PressEnterTrigger`.
Its selected `Style_TextBox` in `LogWidget.tpl` lines 264 to 267 dispatches
`UILuaCommands.PressEnter` with `Text2` as the string operand. The
[LogWidget assets](../resource/sqwt-tab-scroll-text-parts.md#logwidget-declared-hit-filters)
pin the carriers. In `client/script/n1635q/yv3n1635q.le.lpb`, SHA-256
`d99be24e3e64e9d105b25b722d119ee349c5f9244ddd4f2e1f048c32355ad7`,
`setKeyboardFocusToChatInput` `main/f26` PCs 0 to 10 requires that the
focused widget equals the receiver before requesting textbox focus.
`processUICommandDefault` `main/f2` PCs 0 to 29 passes nonempty input to
`DesktopWidget.processInputWordAnalyze` and requests `setDeactiveBatch`
for either empty or nonempty input.

STATIC. On the conditional RaptureTextBox construction path for
`TextBox_ChatInput`, interface KeyDown cell `0x00FC1504` selects
`0x0066BB40`. After the completion-state branch it calls `0x0097A380`
at `0x0066BCD9`, which calls `0x0091F440` before its raw Return handling. The latter visits the element's
own binding collection at `0x0091F4A5`. A successful ordinary KeyGesture
execution with unchanged collection serial reaches slot 9 at
`0x0092126D`. Table cell `0x00FC5AC8` selects `0x005584F0`, whose write at
`0x005584F4` marks the keyboard event handled. The binding walker can then
skip later ordinary bindings through `0x0092119F..0x009211AD`.

OBSERVED. Both standard `AetheryteListWidget.form` lists select
`Style_ListBox`. `AetheryteListWidget.tpl` lines 18 to 21 bind the native
`SelectionChanged` event to `UILuaCommands.SelectionChanged` with the
sender's `SelectedIndex`. The form and template SHA-256 values are
`a2b0ea4f84f89b774c82de73e3458efab203f97456e2ae08210ffeb15b119751` and
`28fa33aab46a8cac2213d4506ad87865e88dafff9022f71a5c58b8ff2c15a285`.
`AetheryteListWidget.initAsk` `main/f0` PCs 12 to 19 registers
`ListBox_AetheryteList_RegionSelection` and `ListBox_AetheryteList` for that
command. The callback `main/f2` PCs 0 to 25 sets the ask result to index
plus 1 for the region and for a destination with nonzero `AetheryteID`.
`setAskParameter` `main/f3` PCs 16 to 20 requests the saved region focus
index in mode 0.
[Selection results](https://github.com/XIVLegacy/xivl-client-scripts/blob/7d1a5ca6e9a06806243951c107f6796270049db2/docs/aetheryte-list-widget-contract.md#selection-results).

STATIC. The [ListBox Operate path](https://github.com/XIVLegacy/xivl-client-scripts/blob/7d1a5ca6e9a06806243951c107f6796270049db2/docs/reward-widget-result-contracts.md#rewardselectwidget-opening-and-keyboard-selection)
requests the `SelectedIndex` write at `0x0096EC4E`. Callback `0x0096F440`
case 0 checks list `+0x35C` at `0x0096F489` before calling `0x0096F080` at
`0x0096F49F`. The reached notification branch at `0x0096F39E..0x0096F3E9`
routes descriptor `0x0135B5CC`, initialized as `SelectionChanged` id
`0x3C` at `0x00F24860`. This is distinct from the `Selection` event that
`0x0096EC00` separately raises. Actual recipient and repeated equal index
writes remain conditions outside this join.

The conditional routes distinguish Decide on an admitted Ask answer and
submit on the focused chat input. `LogControlPressKey` has the separate
[CommonAskWidget trigger](../resource/logcontrol-key-wait.md#input-that-raises-the-press-commands).
The active modal and input recipient when both surfaces are visible, and
live dispatch under an overridden profile, remain UNKNOWN.

### Chat activation

STATIC. Initializer `0x00F0DB50` constructs the RoutedCommand at
`0x01339AD8` from the `RaptureCommands.Chat` string at `0x00FA8044`
(`0x00F0DB79..0x00F0DB99`, Ghidra and LLVM objdump). The standard
`client/sqwt/common/Desktop.form` includes it in the root's
`ConsumedCommand`. `Desktop.tpl` lines 19 to 20 declare a matching
sound action with file number 2. The form and template SHA-256 values are
`c1c55278e57ea5cd557bc14e26fb751ab2bd1d77d637f5e250bd46529186a24e` and
`4fa48cf4a82e7804b34b7664aa28e8fb11884a4ae1b7be234027e95775dcc7ee`.

OBSERVED. Desktop connector `init` `main/f0` PCs 343 to 345 registers
`RaptureCommands.Chat` through `setUICommandCondition`. The shared
`main/f54` requests `_widget` and priority 5. Desktop `main/f142` PCs 14 to
29 creates primary `LogWidget` at static slot 2 with init operand 1, and
`LogWidget2` at slot 3 with init operand 2.

`processUICommandEvent` `main/f62` PCs 0 to 20 require `work.mode > 1`,
nil current keyboard-focused widget for this command, and sender
`_widget`. The `RaptureCommands.Chat` arm at PCs 55 to 85 then requires
`isShow()` and `getInputEnable()` each equal to true on static slot 2.
It sends `Log.FadeIn` to slot 3, requests `changeFocusedWidget` on slot 2
with false as the show operand, then calls `setKeyboardFocusToChatControl`
on slot 2. This arm neither reads nor updates `chatMode` or
`temporaryChatMode`. Their initialization at `LogWidget.init` `main/f1`
PCs 122 to 125 is separate.

Desktop `changeFocusedWidget` `main/f122` PCs 0 to 35 checks the actor,
enables input on the non-Desktop widget and requests widget focus if it
is not already current. The false show operand skips its show arm.
[Native focus admission](../resource/sqwt-command-parameter-consumer.md#conditional-actionmenu-focus-request)
records the native refusal conditions. `LogWidget` `main/f6` forwards the
next request to `setKeyboardFocusToChatInput`, whose `main/f26` PCs 0 to
10 require the current focused widget to compare equal to itself before
requesting `TextBox_ChatInput` focus. The caller discards both focus
results.

`RaptureCommands.Chat` with int 0, the default Space binding, makes no
text write or prefill on this arm. The standard textbox declares no
`Text` attribute. Normal submission or cancellation calls
`setDeactiveBatch`, whose `main/f32` PCs 14 to 27 clear textbox text,
hide the chat menu, reset history indices and cancel focus. This explains
opening a cleared editor without a leading space. Int 191, the default
slash binding, alone requests append of `/` (`main/f62` PCs 80 to 85).
`appendStringForChatControl` `main/f7` PCs 0 to 24 rechecks widget focus,
appends to the existing text, and sends `ApplicationCommands.ImeOff`
for this literal. This establishes the prefill after focus admission.
`RaptureCommands.Chat2` and the channel shortcuts have separate arms.

## Desktop branch

STATIC. The executable registers static command objects for
`UILuaCommands.MainMenu`, `Function1`, `Cancel`, `WidgetClose` and
`SelectionChanged`
([Command objects](../resource/sqwt-command-parameter-consumer.md#command-objects)).
The native dispatcher `0x00896AF0` calls the Lua method `_onUICommandEvent`
([Active Mode key](../net/target-command-handler.md#active-mode-key)).

OBSERVED. `WidgetBaseClass._onUICommandEvent`, `main/f6` of
`widget/widgetbaseclass`, forwards its five arguments to
`self:processUICommandEvent` (PCs 6 to 12). Below, argument 2 is the control
name and argument 3 the command name, counted after the receiver.

OBSERVED. `DesktopWidget.init`, `main/f0` of the connector, calls
`setUICommandCondition` with `UILuaCommands.Cancel` (PCs 337 to 339),
`UILuaCommands.MainMenu` (PCs 340 to 342) and `UILuaCommands.Function1` (PCs
391 to 393). `setUICommandCondition`, `main/f54` of
`widget/widgetbaseclass_common`, calls
`_setUICommandCondition("_widget", command, 5)`.

OBSERVED. `DesktopWidget.processUICommandEvent`, `main/f62` of the connector,
reaches the main menu call through these steps.

| PCs | Step |
|---|---|
| 0 to 4 | Return when `work.mode` is 1 or less |
| 18 to 20 | Return when argument 2 is not `_widget` |
| 24 to 265 | A first chain of command tests that holds no arm for `UILuaCommands.MainMenu` or `UILuaCommands.Function1` |
| 266 to 270 | Return when `work.lockUserControl` is set |
| 271 to 274 | Select the tutorial arm when `work.tutorialFlag` is true |
| 275 to 282 | Tutorial arm. A command equal to `UILuaCommands.MainMenu` (PC 276) or `UILuaCommands.Function1` (PC 279) calls `processCommandMainMenu` (PCs 281 to 282) |
| 452 to 456 | Ordinary arm. Return when `isSubTargetSelectMode()` is set |
| 457 to 464 | Ordinary arm. A command equal to `UILuaCommands.MainMenu` (PC 458) or `UILuaCommands.Function1` (PC 461) calls `processCommandMainMenu` (PCs 463 to 464) |

Neither arm passes the command to `analyzeShortCutCommand`.

OBSERVED. `processCommandMainMenu`, `main/f76` of the connector, takes the
receiver only.

| PCs | Step |
|---|---|
| 0 to 4 | Return when `isTutorialMainMenuMask()` is true. That method, `main/f160`, is true when `getTutorialMenuType()` is 4 |
| 5 to 10 | Return when `isWidgetExec(3)` is true. That method, `main/f531`, is true when `work.rootWidget[3]` is not nil |
| 11 to 15 | Return when `isSubTargetSelectMode()` is set. That method, `main/f34`, is true when `work.subTargetExecuteWidget` is not nil |
| 16 to 20 | Read `getStaticWidget(1)` and call its `isShow()` |
| 21 to 30 | When `isShow()` is false, return if `work.widgetEnableFlag[3]` is false, else call `show()` on the widget |
| 32 to 33 | When `isShow()` is not false, call `hide()` on the widget |

OBSERVED. `setDefaultWidgetLocation`, `main/f142` of the connector, calls
`createStaticWidget(1, "MainMenuWidget", nil, 3, false)` at PCs 0 to 6.
`createStaticWidget`, `main/f518`, stores the `createWidget` result in
`work.widget` under its first argument (PC 13), and `getStaticWidget`,
`main/f520`, returns `work.widget` under its argument. Static widget 1 is
therefore the `MainMenuWidget` object. `DesktopWidget.init` calls
`setDefaultWidgetLocation` at PC 316
([Desktop dependency](work-update-callbacks.md#desktop-dependency)). The
listing shows the creation request and not its completion.

OBSERVED. `show`, `main/f40` of `widget/widgetbaseclass_common`, sets
`widgetWork.common.visibleFlag` to true when its third argument is not true
(PCs 0 to 4). It returns false when `widgetWork.common.forceHideFlag` is true
(PCs 5 to 11) and then returns false when the widget is already shown (PCs 12
to 17). Past both tests it calls `processBeforeShow` (PCs 24 to 26) and, for a
first argument that is not false, writes the window property `Anime` with
`Show` (PCs 34 to 37). `initCommon`, `main/f0` of the same script, stores false
in `forceHideFlag` (PCs 65 to 67). `forceVisible`, `main/f47`, stores false for
an argument of true (PCs 9 to 11) and true for any other argument (PCs 13 to
15). `hide`, `main/f43`, returns false
when the widget is not shown (PCs 5 to 10) and, for a first argument that is
not false, writes `Anime` with `Hide` (PCs 21 to 24). `isShow`, `main/f45`,
is false when the property `Visibility` is `Hidden` or `Anime` is `Hide`.
`processCommandMainMenu` passes no argument to either call, so `show` sets
`visibleFlag` on that route.

## Form

MEASURED. The root of `widget/MainMenuWidget.form` is a `Window` named
`Window_MainMenuWidget` with `SqwtStyle` `WIN_mainmenu`,
`ConsumedCommand` `UILuaCommands.ActionMenu` and
`KeyboardNavigation.DirectionalNavigation` `Cycle`. Its two data makers and two
lists are the ones recorded in
[MainMenuWidget](../resource/sqwt-xpath-data-sources.md#mainmenuwidget). The
maker with `XPath` `/Main` has `Name` `MainMenu` and feeds `ListBox_MainMenu`.
The maker with `XPath` `/System` has `Name` `SystemMenu` and feeds
`ListBox_SystemMenu`, which sits inside `Grid_SystemMenu`. Both lists carry
`Style` `Style_ListBox` and `IsTabStop` `False`.

MEASURED. `Style_ListBox` in `widget/MainMenuWidget.tpl` targets `ListBox` and
holds one `EventTrigger` on `SelectionChanged`. Its `ExecuteCommandAction` has
`Command` `UILuaCommands.SelectionChanged`, `CommandParameter`
`{CommandParameter Int, %s}` and `PropertyName1` `SelectedIndex`. The delivery
of that shape is recorded for another widget in
[PartyParameterWidget](../resource/sqwt-command-parameter-consumer.md#partyparameterwidget).

MEASURED. `DataTemplate_ListBoxItem_Main` holds one `Button` named
`Button_Command` with `SqwtStyle` `BTN_basis_listboxSelect`. Its `Content`
binds `@MainName`, its `IsEnabled` binds `@MainEnable` and its `Help.Value0`
binds `@Help`, with `Help.Type` 1. `DataTemplate_ListBoxItem_Return` holds the
same button and a `Label_Timer`. `Button_Close` in the form has `Command`
`UILuaCommands.WidgetClose` and `SqwtStyle` `BTN_basis_winClose`.

## Rows

OBSERVED. `MainMenuWidget.init`, `main/f0` of `widget/mainmenuwidget`, makes
these calls in order.

| PCs | Call |
|---|---|
| 44 to 47 | `setControlCommandCondition("ListBox_MainMenu", "UILuaCommands.SelectionChanged")` |
| 48 to 51 | The same for `ListBox_SystemMenu` |
| 52 to 54 | `setUICommandCondition("UILuaCommands.Activated")` |
| 55 to 58 | `setCancelCondition()` and `setCloseCondition()` |
| 59 to 61 | `setUICommandCondition("RaptureCommands.TimerEnd")` |
| 62 to 63 | `work.mainMenuIndex = -1` |
| 64 to 182 | `setListText("MainMenu", row, "MainName", id)` for rows 0 to 18, and `setListPropertyTemplate("MainMenu", 15, "DataTemplate_ListBoxItem_Return")` at PCs 154 to 158 |
| 183 to 296 | `setListProperty("MainMenu", row, "Help", id)` for rows 0 to 18 |
| 297 to 307 | A loop from 0 to 18 that calls `setListProperty("MainMenu", row, "MainEnable", "True")` |
| 308 to 310 | `updateListProperty("MainMenu")` |
| 311 to 317 | `setModal(true)` and `setVisibility("Grid_SystemMenu", false)` |
| 318 to 327 | When `desktopWidget:isChinese()` is true, `setListPropertyVisibility("MainMenu", 16, false)` |

In `widget/widgetbaseclass_common`, `setCancelCondition` with no argument
(`main/f58`) calls `_setUICommandCondition("_widget", "UILuaCommands.Cancel",
5)`, `setCloseCondition` (`main/f59`) does the same for
`UILuaCommands.WidgetClose`, and `setControlCommandCondition` (`main/f55`)
passes its control name in place of `_widget`. `setListText` (`main/f175`)
forwards to `_setListTextProperty`, `setListProperty` (`main/f169`) to
`_setListProperty`, and `updateListProperty` (`main/f174`) to `_updateList`,
each with the list name and row unchanged. `setListPropertyTemplate`
(`main/f180`) writes the row property `template` and
`setListPropertyVisibility` (`main/f181`) the row property `visibility`. The
native contract of `_setListTextProperty` is in
[Desktop native call contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/desktop-native-call-contract.md).

OBSERVED. The list names the script passes are `MainMenu` (`main/f0` PCs 64 to
310) and `SystemMenu` (`main/f1` PCs 214 to 228). They equal the `Name`
attributes of the two makers and not the `XPath` text `/Main` and `/System`.
These names answer the MainMenuWidget half of the list name gap in
[Open](../resource/sqwt-xpath-data-sources.md#open). The AchievementPopupWidget
script was not read here.

OBSERVED. The row index, `MainName` text id and `Help` id of each `MainMenu`
row, with the selection arm of the next section.

| Row | `MainName` id | `Help` id | Selection |
|---:|---:|---:|---|
| 0 | 2128 | 75721 | `StatusWidget` |
| 1 | 2129 | 75722 | `EquipWidget` |
| 2 | 2102 | 75723 | `ActionSettingWidget` |
| 3 | 2115 | 75724 | `desktopWidget:executePlayerBonusPointAssign()` |
| 4 | 2103 | 75725 | `ItemListWidget` |
| 5 | 2104 | 75726 | `Ask/JournalListWidget` |
| 6 | 12001 | 75740 | `AchievementListWidget` |
| 7 | 2130 | 75739 | `PcSearchWidget` |
| 8 | 2109 | 75727 | `PartyRootWidget` |
| 9 | 2107 | 75728 | `CommunityMenuWidget` |
| 10 | 2113 | 75729 | `Ask/LinkshellListWidget` |
| 11 | 2114 | 75730 | `RetainerListWidget` |
| 12 | 2106 | 75731 | `desktopWidget:processCommandMap()` |
| 13 | 2124 | 75732 | `NpcLinkshellListWidget` |
| 14 | 2111 | 75733 | `hide()`, then `desktopWidget:executePlayerTelepo(0)` |
| 15 | 2112 | 75734 | `hide()`, then `desktopWidget:executePlayerReturn()` |
| 16 | 2105 | 75735 | `SupportDeskWidget`, or `ConfigWidget` when `isChinese()` is not false |
| 17 | 2108 | 75736 | `ConfigWidget`, or `hide()` and `desktopWidget:executePlayerLogout()` when `isChinese()` is not false |
| 18 | 2110 | 75737 | `hide()`, then `desktopWidget:executePlayerLogout()`, and nothing when `isChinese()` is not false |

`isChinese`, `main/f561` of the connector, is true when `_getLanguage()`
returns 4 or 5. The form shows the heading text `@2116` above
`ListBox_MainMenu` (MEASURED). The lookup of an `@N` reference is in
[Sqwt plain text reference lookup](../resource/sqwt-plain-text-reference-lookup.md).
The text the 19 ids resolve to is not restated here.

OBSERVED. `updateButtonMask`, `main/f8` of `widget/mainmenuwidget`, reads six
values from `getButtonMask(true)` (PCs 0 to 2) and calls `setDezionTimer()`
(PCs 12 to 13). It then rewrites `MainEnable` for rows 0 to 18 and calls
`updateListProperty("MainMenu")` (PCs 14 to 130). Every row except row 3 takes
one of the six values unchanged. Row 3 (PCs 32 to 37) takes a copy of the first
value that is forced to false when
`worldMaster:_getMyPlayer():isBattleClass()` is false (PCs 3 to 11).
`processBeforeShow`,
`main/f4`, calls `updateMainMenu` (PCs 28 to 29), which calls
`updateMainMenuCommand` and through it `updateButtonMask` (`main/f15` PCs 23 to
24 and `main/f13`). The rows are therefore re-masked on each show.

### Return countdown

OBSERVED. The sixth mask in `getButtonMask`, main/f10 PCs 44-74,
starts as the first mask. When that mask is exactly true and
`isDead()` returns exactly false, occupancy exactly true disables Return.
Otherwise the positive recast check runs only when the formal argument
is exactly true. This branch has no area or inn query. Main/f8
PCs 104-109 stores the sixth result as row 15 `MainEnable`.

Main/f11 PCs 0-11 reads `getWarpRecastTime()` and forces zero when
`isDead()` returns exactly true. The player method, main/f67 PCs 0-9
of `chara/player/playerbaseclass`, clamps the native deadline minus
[server time in whole seconds](../actor/cast-timing-clock.md#native-clock-model)
at zero. On a positive result, main/f11 PCs 12-35 write
`FValue0` equal to that result, `FValue1=0`, `IValue0=1` and
`TimerVisibility=Visible`, in that order, to `MainMenu` row 15.
The other arm writes only `TimerVisibility=Collapsed`.
Both call `updateListProperty("MainMenu")` at PCs 43-45.

MEASURED. In the standard template, `Label_Timer` contains a grid with
`IconControl_Timer` (icon 111) and `CustomControl_TimerLabel`.
The latter declares type `TimerTextBlock`. One-way XPath bindings
take `@TimerVisibility`, `@IValue0`, `@FValue0` and `@FValue1` to
`Visibility`, `IntData.Value0`, `FloatData.Value0` and
`FloatData.Value1`, respectively. The icon binds its visibility
one-way to the custom control's `Visibility`. Both children disable
hit testing. These are the properties of the selected row template.
The selector resolution remains in [Open items](#open-items).

STATIC. The custom-control factory's TimerTextBlock branch at
`0x00549B19..0x00549B51` constructs through `0x006A2120`. Its primary
table `0x00FCE644` identifies
`.?AVTimerTextBlock@CustomControl@SqwtInterface@Main@Application@@`.
The separate `TimerLabel` branch at `0x005496D1..0x00549709` uses
`0x0069BB90`. TimerTextBlock calls the TimeTextBlock base constructor
with a 250 millisecond period. Its inherited notification slot 6
(`0x008752D0`) tests descriptor `0x0135F908` (`IntData.Value0`) at
`0x008752DE`, then resets the embedded timer at `+0x370` with zero
delay and the retained period through `0x009871C0` at `0x008752FA`.
It uses the descriptor without reading the integer payload and calls
slot 70, which is a no-op for TimerTextBlock. The [Sqwt clock](../resource/sqwt-double-animation-clock.md#clock-source-units-and-epoch)
sets the elapsed-time origin on the first admitted timer pass. Subsequent
samples of the two float properties use that origin.

On each timer callback `0x008755E0` calls slot 71 (`0x006A2230`).
The two readers `0x00875130` and `0x008751F0` select
`FloatData.Value0` and `FloatData.Value1`, parse each with `atof`,
store to binary32 and truncate toward zero. `0x00875110` divides
unsigned elapsed milliseconds by 1000. For the row's finite positive
start and zero target, the displayed count is start minus whole elapsed
seconds, clamped to zero. The sample splits the count into hours,
minutes and seconds through `0x008750B0`. Constructor `0x00874FC0`
sets the TimeString mode to 1. With that mode retained,
`0x00875460` writes all three components with minimum width 2,
fill digit 0 and literal colon separators from `0x0104FA80`.
The result is `HH:MM:SS` for the ordinary short countdown, with no
game language or text-sheet selection in this formatter.
The callback sends it to [TextBlock.Text](../resource/damage-plate-numeric-presentation.md)
through `0x009556A0` at `0x00875603`.

When the sampled count reaches the target, `0x006A2230` routes
`RaptureCommands.TimerEnd` from the control's `+0xB4` interface
through `0x00927FA0` at `0x006A22DC`. The command object
`0x0133B298` is initialized from the named literal at
`0x00F0EE79..0x00F0EE99`. The per-control latch `+0x420`
is set after dispatch and cleared on a nonterminal sample.
The receiver registered at main/f0 PCs 59-61 uses the
`_widget` condition with value 5
(`widget/widgetbaseclass_common` main/f54 PCs 0-4).
MainMenuWidget `processUICommandDefault` main/f3 PCs 13-32 calls
`getButtonMask(false)`, writes its sixth result to row 15 `MainEnable`,
collapses the timer and updates the list.

The native observations use Ghidra CLI 0.2.2 and LLVM PE32
disassembly of retail `ffxivgame.exe` 2012.09.19.0001, 15996808
bytes, base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The [retail Lua coverage manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/8be1a405d2309ab4a2127df1fc745fcb6c55c861/manifests/retail_lua_coverage.json)
identifies the player script. Match the original and decoded hashes,
read the listed zero-based PCs, and compare the factory branches,
vtable cells, timer argument readers and command identity in the PE.
Same-value list refresh, control re-realization, and the visible
lifetime of the original countdown remain untraced.

## Selection

OBSERVED. `WidgetBaseClass.processUICommandEvent`, `main/f13` of
`widget/widgetbaseclass_common`, handles `UILuaCommands.SelectionChanged` and
`UILuaCommands.SelectionChangedOnce` in one arm (PCs 58 to 81).

| PCs | Step |
|---|---|
| 63 to 66 | `getSelectedIndex(control)` with argument 2 as the control. The result replaces argument 4 |
| 67 to 69 | Leave the arm when the index is below 0 |
| 70 to 75 | `processUICommandSelectionChanged(argument 1, control, index, argument 5)` |
| 76 to 80 | When the command is `UILuaCommands.SelectionChanged`, `setSelectedIndex(control)` |

`getSelectedIndex`, `main/f113`, reads the control property `SelectedIndex`.
`setSelectedIndex`, `main/f112`, writes it and replaces a nil value with -1, so
the call at PCs 78 to 80 resets the selection to -1. The index the handler
receives is the property read at PC 65 and not the `Int` parameter of the
markup action.

OBSERVED. `MainMenuWidget.processUICommandSelectionChanged`, `main/f1` of
`widget/mainmenuwidget`, compares the control with `ListBox_MainMenu` (PC 1)
and the index with the constants 0 to 18. The arms are the Selection column of
the table above. A widget name is opened by
`desktopWidget:openMainMenuRootWidget(name, self)`. Row 4 first calls
`desktopWidget:setBazaarActor()` and passes a third argument `true` (PCs 40 to
48). Rows 5 and 10 pass a third argument 1 (PCs 53 to 58 and 177 to 182). Every
arm then stores the index in `work.mainMenuIndex` (PCs 206 to 207). Only the
arms of rows 14, 15 and 18, and of row 17 under `isChinese`, call `hide()`.
For `ListBox_SystemMenu` the method calls `hide()`, reads the row properties
`SystemCommandID`, `SystemCommandInfo` and `SystemSubTarget` of list
`SystemMenu`, calls `executeCommand` with them and stores -1 in
`work.mainMenuIndex` (PCs 210 to 235).

OBSERVED. `openMainMenuRootWidget`, `main/f538` of the connector, returns false
when `getStaticWidget(17):isShow()` is true (PCs 0 to 8) and otherwise
tail-calls `openRootWidget(3, name, parent, true, ...)` (PCs 9 to 15).

INFERRED. The selection index is the zero-based position of the row in the
`MainMenu` list. The script writes rows 0 to 18 and compares the index with the
same constants, and the items control builds one item for each child of the
list node in order
([Items controls](../resource/sqwt-xpath-data-sources.md#items-controls),
STATIC). A row whose `visibility` attribute is `False` gets no item content on
that page, which applies to row 16 under `isChinese`. The script passes the
boolean false, and the bool branch of `_setListProperty` writes the text
`False`
([_setListProperty](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/stub-overload-native-contract.md#_setlistproperty),
STATIC). No single trace shows the
`SelectedIndex` value of a clicked or decided item.

## Close and focus

OBSERVED. In `main/f13` of `widget/widgetbaseclass_common`,
`UILuaCommands.Cancel` calls `processUICommandCancel` (PCs 21 to 28) and
`UILuaCommands.WidgetClose` calls `processUICommandClose` (PCs 31 to 38). The
base `processUICommandCancel`, `main/f16`, calls `processUICommandClose` with
the same arguments. `widget/mainmenuwidget` defines `processUICommandClose` and
no `processUICommandCancel`, so both commands reach
`MainMenuWidget.processUICommandClose`, `main/f2`. After a tutorial step that
runs only for tutorial menu type 1 with status 2 (PCs 0 to 49), it stores
`getFocusedIndex("ListBox_MainMenu")` in `work.mainMenuIndex` (PCs 50 to 54)
and calls `hide()` (PCs 55 to 56).

OBSERVED. `getFocusedIndex`, `main/f115` of `widget/widgetbaseclass_common`,
reads the control property `SqwtFocusedIndex`, and `setFocusedIndex`,
`main/f114`, writes it. `MainMenuWidget.processUICommandDefault`, `main/f3`,
handles `UILuaCommands.Activated` by calling
`setFocusMainMenu(work.mainMenuIndex)` when that index is 0 or more (PCs 0 to
10). `setFocusMainMenu`, `main/f20`, calls
`setLogicalFocus("ListBox_MainMenu")` and
`setFocusedIndex("ListBox_MainMenu", index)`. When `isShow()` is true and
`desktopWidget:checkKeyboardFocused(self)` is true it also calls
`setKeyboardFocusedControl("ListBox_MainMenu")`. It then stores the index in
`work.mainMenuIndex` (PCs 0 to 21).

`init` leaves `work.mainMenuIndex` at -1, so the `Activated` arm sets no focus
until a selection or a close has stored an index.

## Community menu close

OBSERVED. In `widget/mainmenuwidget` `main/f1`, row 9 calls
`openMainMenuRootWidget("CommunityMenuWidget", self)` without an extra mode
argument (PCs 166 to 172). The desktop wrapper's 3 is the root index.
Connector `openRootWidget` (`main/f535` PCs 4 to 18) refuses when
`isWidgetExec(3)` is exactly true and otherwise passes 3, the name, nil, the
parent, true and any extra arguments to `openWidget`. `isWidgetExec`
(`main/f531` PCs 0 to 8) checks for a nonnil `work.rootWidget` entry. The
successful `processWidgetCreated` path (`main/f532` PCs 17 to 27) writes the
created widget into the given root index when the slot is nil.

MEASURED. The standard `widget/CommunityMenuWidget.form` names
`Window_CommunityMenuWidget` (decoded line 1), merges
`common/WINBasisWithTitle.tpl` (line 5) and selects
`ControlTemplate_Window_WINBasisWithTitle` (line 10). The selected template
names its title `Button_Close` and declares `UILuaCommands.WidgetClose` as
its Command (decoded line 11). It declares `Focusable=False` and has no local
CommandTarget, CommandParameter or ClickMode. The separate source-named
Click trigger has only sound file 1 (lines 71 to 73). The Closing event's
storyboard has `ApplicationCommands.DestroyWindow` as CompletedCommand
(lines 31 to 44). It is a separate declaration from the title command.

OBSERVED. Community init (`widget/communitymenuwidget` `main/f0` PCs 3 to 6)
calls `setCancelCondition()` and `setCloseCondition()`. The inherited
helpers above subscribe `_widget` to `UILuaCommands.Cancel` and
`UILuaCommands.WidgetClose` with 5. The
[subscription gate](../resource/sqwt-command-parameter-consumer.md#subscription-gate)
identifies the root store and the numeric condition encoding as kind 0 with
value 5. The title Button's declared command uses the ordinary
[Button command path](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
when eligible and routed to this root. Focus and handler gates still apply.

OBSERVED. Community `processUICommandEvent` (`main/f2` PCs 0 to 13) directly
compares the command with WidgetClose and Cancel. Either match calls
`desktopWidget:closeWidgetDirect(self)` with no additional Lua control-name
or mode test. `widgetbaseclass` `_onUICommandEvent` (`main/f6` PCs 2 to 12)
sets the thread owner to self before calling that handler. Connector
`closeWidgetDirect` (`main/f528` PCs 4 to 18) rejects nil and exact-false
`_isAlive()`, then calls `closeWidgetLocal`. `closeWidgetRecursive`
(`main/f525` PCs 29 to 52) defers the current thread owner's own `close()`.
The local caller (`main/f526` PCs 0 to 13) clears that thread owner before
calling its deferred `close()`.

OBSERVED. `widgetbaseclass_common` `close` (`main/f38` PCs 0 to 6) disables
input, calls `processClosing()`, then calls `_delete()`. The base
`processClosing` (`main/f39`) returns immediately and Community has no
override. The Lua chain requests native deletion directly. The [WidgetBase delete override](https://github.com/XIVLegacy/xivl-client-structs/blob/8a8930c6da61d1ff8b3d442fe169396805394d23/docs/fade-actor-group-native-contract.md#widget-delete-override)
records the conditional native delete paths. The elapsed time from the
request to the finalizer and the delivery of any Window Closing event on
this path remain untraced.

OBSERVED. `widgetbaseclass` `_onFinalize` (`main/f4` PCs 0 to 19) first
calls the superclass. For a non-Desktop widget whose initialization is
complete it calls `desktopWidget:processWidgetDeleted(self)`, then
`processFinalize()`. The common initializer stores the widget index in
`widgetWork.common.widgetIndex` (`main/f0` PCs 59 to 61) and
`getWidgetIndex` reads it (`main/f5` PCs 0 to 3). Connector
`processWidgetDeleted` (`main/f534` PCs 0 to 25) clears the nonnil root
entry for that index when the cached root is the same widget or its
`_isAlive()` is not exactly true. A different live root at index 3 survives.

## Row button style

MEASURED. `common/default.style` declares one `SqwtStyle` named
`BTN_basis_listboxSelect` with one part, `Frame`.

| Condition | Skin | Other attributes |
|---|---|---|
| `SkinNormal` | `NULL` | `FontColor` `#fff3f3f3`, `FontEdgeColor` `#ff262626` |
| `SkinHover` | `BTN_basis_listboxSelect_H` | The same colours |
| `SkinActive` | `BTN_basis_listboxSelect_A` | The same colours, `ContentOffset` `1,1` |
| `SkinDisabled` | `NULL` | `FontColor` `#80f3f3f3`, `FontEdgeColor` `#801a1a1a` |
| `SkinFocused` | `NULL` | The same colours as `SkinNormal` |
| `SkinAnchored` | `BTN_basis_listboxSelect_H` | The same colours as `SkinNormal` |
| `SkinReserved` | `BTN_basis_listboxSelect_R_anime` | The same colours as `SkinNormal` |
| `SkinDroppable` | `NULL` | None |

The order in which the executable reads the skin slots is in
[Sqwt tab scroll text parts](../resource/sqwt-tab-scroll-text-parts.md#verdict).

## Open items

- NOT TRACED. The native route from a key gesture to the dispatcher
  `0x00896AF0`, and which subscriber receives `UILuaCommands.Cancel` while the
  menu is shown. Both the DesktopWidget and the MainMenuWidget subscribe to
  that command with the value 5
  ([Open](../net/target-command-handler.md#open) holds the same gap).
- NOT TRACED. The native list navigation that moves `SqwtFocusedIndex` for a
  `MoveFocus` command, and whether it skips a row whose `MainEnable` is false.
- NOT TRACED. Which control raises `SelectionChanged` for the decide key, and
  the `SelectedIndex` value at that moment.
- NOT TRACED. Whether the shown window receives `UILuaCommands.Activated`, and
  which row holds the focus on the first show.
- NOT TRACED. Which skin condition the button of the keyboard cursor row
  selects. The style gives `SkinFocused` no skin and `SkinAnchored` the hover
  skin, and no trace joins either condition to that row.
- NOT TRACED. Whether a selection that opens a widget leaves the menu shown.
  The selection arm calls no `hide()`.
- NOT TRACED. The writer of `work.widgetEnableFlag[3]`.
- NOT TRACED. The callers of `forceVisible`, and so whether
  `widgetWork.common.forceHideFlag` is true for static widget 1 when the
  command arrives.
- NOT TRACED. Whether the `ItemTemplateSelector` of `ListBox_MainMenu`
  resolves, which decides if the `template` property of row 15 applies
  ([Open](../resource/sqwt-xpath-data-sources.md#open)).

## Reproduction

Unwrap and list the four scripts as in
[Work update callback consumers](work-update-callbacks.md#reproduction), check
the decoded sizes and hashes above, and read the prototypes named in the text.
Decode the three markup files with `xivl extract-resource --as sqwt` and check
the decoded sizes and hashes. Decode the `key_config` rows from the four
resources with the definition's column types and select the rows whose column
8 names the commands above.
