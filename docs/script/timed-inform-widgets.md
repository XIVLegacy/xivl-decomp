# Timed inform widgets

TutorialSuccessWidget and PublicInformDialogWidget each hide themselves after
a delay, by two different mechanisms. TutorialSuccessWidget stores the text
`3` in its window's `StringData.Value0`. With a true second argument, `setData`
sends `UIFormCommands.TimerStart`. The template answers that command with an
empty Storyboard whose `BeginTime` binds `StringData.Value0` and whose
`CompletedCommand` is `UIFormCommands.TimerFinish`. The widget hides on that
command. PublicInformDialogWidget never sends `TimerStart`. Its
`setInitialData` arms the inherited `commonTimer` work member with 5 seconds,
or 7 for the long variant, and its `processTimer` hides it. The executable
schedules a write to a timer member declared with a true operand, and calls
the Lua method `_onTimer` on the object its record resolves to once the
deadline passes. This page records the scripts, the markup, those two native
paths and a census of storyboard `BeginTime` in the installed markup.

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing or the decoded chunk bytes of the installed client scripts. The script bodies are not tracked |
| MEASURED | Read from the decoded Sqwt markup of the install. Markup bodies are not tracked |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe` |
| INFERRED | A role or join taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The script method is the one of
[Work update callback consumers](work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f219` name the direct children of the chunk root in listing
order. `tools/decode_lpb.py` yields the same decoded chunks as
`xivl extract-lpb`. No client code ran.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `widget/tutorialsuccesswidget` | `client/script/n1635q/qpqvs19yrp775rrn1635q.le.lpb` | 794 | `a4d2654311f8a332b15e4bc8876c7954a5bc83bc5b3f8764d944b2ba7a4abf21` |
| `widget/publicinformdialogwidget` | `client/script/n1635q/up8y171w4vsx619yv3n1635q.le.lpb` | 820 | `04295a7940ec4e8c4afabc210038c3a0e7c3c63cd9f36d22f743ded6fae3640f` |
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `widget/widgetbaseclass` | `client/script/n1635q/n1635q89r57y9rr.le.lpb` | 4080 | `aad807a42296e93301582defc8f15e4003b8093af672794282cd54950cc556ad` |
| `widget/widgetbaseclass_common` | `client/script/n1635q/n1635q89r57y9rr_7vxxvw.le.lpb` | 32996 | `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `widget/tutorialsuccesswidget` | 781 | `7e19a0e25e66bf8cdb9be047dbe84a311b88470eac906e2fce2ca814344714b4` |
| `widget/publicinformdialogwidget` | 807 | `5283951d4b548e280a4806ef902c0220c65b40eced6439fcd4830e6936f8de49` |
| `widget/desktopwidget_connector` | 213474 | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/widgetbaseclass` | 4067 | `11e5bab509c4dbbd7e2767d35cdf132a59b5c510c32c2e215f83cac713115140` |
| `widget/widgetbaseclass_common` | 32983 | `7c45426b1a9f168557d84e420c5767c6fdea4abf90b976b276546af4e6a5de5f` |

The markup is the decoded document that `xivl extract-resource --as sqwt`
writes for each file. The census tables below pin the templates.

| Client-relative markup | Bytes | SHA-256 | Decoded bytes | Decoded SHA-256 |
|---|---:|---|---:|---|
| `client/sqwt/widget/TutorialSuccessWidget.form` | 1389 | `2ba74357320b0df168b4ba13f61ad9494dfe943e3ce065d4876662f0aa785540` | 1381 | `ba4a1b6571002e6ac986f490821df38e773830ce6bd6d3039036629041bad95b` |
| `client/sqwt/widget/PublicInformDialogWidget.form` | 1106 | `dd1939051a6bd8b74db489c28197ba7349447e7cccfe1e05fdee978bd0210196` | 1098 | `929f3909edd4e1cd6fcd8af55453563397b189fa4b31be08f049bcda72efadd4` |
| `client/sqwt/widget/PublicInformDialogWidget.sd.form` | 1109 | `0f00360d7338424a555d05d227c6701c0efa55adcf5c4e46c13e9c7176796b93` | 1101 | `5a766ea78b4c069b6ef0480884c482c852cd54e47490602783ba93ff0bb5c329` |

The native input is retail `ffxivgame.exe`, build `2012.09.19.0001`, PE32
x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. Addresses are VAs. Instructions were read with ghidra-cli
disassembly, and every cited instruction window was compared byte for byte
with the file-backed section bytes of the PE. Strings and pointer cells were
read from the file. Decompiler output located the windows. A sentence that
rests on a decompiler rendering rather than on the listed instructions says
so.

## TutorialSuccessWidget

OBSERVED. The script defines `TutorialSuccessWidget` with superclass
`WidgetBaseClass` (root PCs 0 to 6) and three methods.

| Method | Locator | Behavior |
|---|---|---|
| `init` | `main/f0` PCs 0 to 9 | `setUICommandCondition("UIFormCommands.TimerFinish")`, then `setProperty("StringData.Value0", "3")` with the string `3`, then `setDrawPriority(0.2)` |
| `setData(A1, A2)` | `main/f1` PCs 0 to 10 | `setText("TextBlock_Text", A1)` (PCs 0 to 3). When `A2` equals `true` (PC 4), `sendCommand("UIFormCommands.TimerStart")` (PCs 6 to 8). Then `show()` (PCs 9 to 10) on either branch |
| `processUICommandEvent(A1..A5)` | `main/f2` PCs 0 to 3 | When `A3` equals `UIFormCommands.TimerFinish`, `hide()` |

The comparison at `main/f1` PC 4 is a Lua equality with the constant `true`,
so only the boolean `true` sends the command. `A3` is the command name,
counted after the receiver, as forwarded by `WidgetBaseClass._onUICommandEvent`
([Desktop branch](main-menu-command-route.md#desktop-branch)).

OBSERVED. The helpers of `widget/widgetbaseclass_common` resolve these calls.

| Helper | Locator | Native call or forward |
|---|---|---|
| `setUICommandCondition(cmd)` | `main/f54` PCs 0 to 4 | `_setUICommandCondition("_widget", cmd, 5)` |
| `setProperty(name, ...)` | `main/f66` PCs 0 to 7 | `_setProperty(nil, widgetWork.common.widgetName, name, ...)` |
| `setDrawPriority(v)` | `main/f147` PCs 0 to 3 | `setProperty("SqwtDrawOrder", v)` |
| `sendCommand(cmd, p1, p2)` | `main/f52` PCs 0 to 7 | `sendControlCommand(widgetWork.common.widgetName, cmd, p1, p2)` |
| `sendControlCommand(target, cmd, p1, p2)` | `main/f53` PCs 0 to 6 | `_sendStoryboardCommand(nil, target, cmd, p1, p2)` |
| `getFormName()` | `main/f2` PC 0 | Returns nil |

`initCommon`, `main/f0`, sets `widgetWork.common.widgetName` to `Window_`
followed by the form name (PCs 40 to 45). The form name is the
`getFormName()` result, or the name argument with any path prefix removed by
`gsub(".+/", "")` when that result is nil (PCs 26 to 36). TutorialSuccessWidget
defines no `getFormName`. With a nil name the connector's `createStaticWidget`
substitutes the class name
([Population and refresh](status-effect-widget-lifecycle.md#population-and-refresh)),
so static widget 22 addresses `Window_TutorialSuccessWidget`. The two
`_sendStoryboardCommand` trailing arguments are nil here, which selects the
`WithNone` pickup in the
[storyboard command contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/boot-native-call-contract.md#storyboard-commands).

OBSERVED. In the connector, `setDefaultWidgetLocation`, `main/f142` PCs 159 to
165, calls `createStaticWidget(22, "TutorialSuccessWidget", nil, 1, false)`.
`openTutorialSuccessWidget(A1, A2)`, `main/f161`, calls
`getStaticWidget(22):setData(A1, A2)`. Its existing call-site contract is
`xivl-client-scripts:docs/gridania-battle-tutorial-branches.md`.
`processUpdateGeneralNotificationDialog(A1, A2, A3, A4, ...)`, `main/f99`, is
one producer of the `true` argument. Its arm for `A1 == 2` (PC 28) calls
`openTutorialSuccessWidget(A4, true)` (PCs 30 to 33). Its arm for `A1 == 10`
(PC 3) calls `openPublicInformLongDialogWidget(A2, A4, ...)` (PCs 5 to 9).
The callers of `processUpdateGeneralNotificationDialog` were not traced here.

MEASURED. The root of `widget/TutorialSuccessWidget.form` is a `Window` named
`Window_TutorialSuccessWidget` with `SqwtStyle` `WIN_tutorial_inform_active`,
`Focusable` `False` and `IsHitTestVisible` `False`. It merges
`widget/TutorialSuccessWidget.tpl`, uses `ControlTemplate_Window` as its
template and holds `IconControl_Icon` with `IconDatas` `541` and the
`TextBlock` `TextBlock_Text`.

MEASURED. In `widget/TutorialSuccessWidget.tpl`, `ControlTemplate_Window`
holds this trigger at decoded lines 50 to 54.

| Element | Attributes |
|---|---|
| `CommandTrigger` | `Command` `UIFormCommands.TimerStart` |
| `BeginStoryboard` | No `Name` |
| `Storyboard` | `CompletedCommand` `UIFormCommands.TimerFinish`, `BeginTime` `{Binding ElementName=Window_TutorialSuccessWidget, Path=StringData.Value0, Mode=OneWay}`, no `Duration`, no `RepeatBehavior`, no child element |

The same template also holds `Shown`, `Closing` and `Hiding` EventTriggers
with 0.2 second `VisualOpacity` storyboards, a `SoundPlayerAction` with
`FileNumber` `68` on `Shown`, and `Activated` and `Deactivated`
ExecuteCommandActions.

INFERRED. The chain is `setData(text, true)` -> TimerStart -> empty
Storyboard begun with `BeginTime` from the bound text `3` -> CompletedCommand
TimerFinish -> `processUICommandEvent` -> `hide()`. The
[Storyboard BeginTime](#storyboard-begintime) section gives the native
support and the links this chain still lacks.

## PublicInformDialogWidget

OBSERVED. The script defines `PublicInformDialogWidget` with superclass
`WidgetBaseClass` and three methods. It defines no `processUICommandEvent`,
calls no `setUICommandCondition` and sends no command.

| Method | Locator | Behavior |
|---|---|---|
| `init` | `main/f0` PCs 0 to 2 | `setDrawPriority(0.2)` only |
| `setInitialData(A1, A2, A3, ...)` | `main/f1` PCs 0 to 30 | See the next table |
| `processTimer` | `main/f2` PCs 0 to 1 | `hide()` |

| PCs | Step in `setInitialData` |
|---|---|
| 0 to 2 | `setText("TextBlock_Text")` with no text argument |
| 3 to 4 | Style `WIN_basis_active`, timer 5 |
| 6 to 9 | When `A1 == 2`, timer 7 |
| 11 to 13 | When `A1 == 3`, style `WIN_basis_yellow_active`, timer unchanged |
| 16 to 19 | `setStyle("Window_PublicInformDialogWidget", style)` |
| 20 to 27 | When `A2` is not nil, `setTextByOwner("TextBlock_Text", A2, A3, ...)` |
| 28 to 30 | `setCommonTimer(timer)` |

OBSERVED. In `widget/widgetbaseclass_common`, `setText` (`main/f89`) reaches
`setTextProperty` (`main/f182`) through `setItemText` (`main/f91`). For a nil
text, `setTextProperty` calls `_setProperty(nil, control, "Text", "")` (PCs 0
to 1 and 22 to 27). `setStyle` (`main/f146`) calls
`setControlProperty(control, "SqwtStyle", style)`. `setTextByOwner`
(`main/f95`) calls `setItemTextByOwner(nil, control, owner, id, ...)`, and
`setItemTextByOwner` (`main/f96` PCs 0 to 7) calls
`_setProperty(item, control, "Text", owner, id, ...)`. The native owner-text
route for that argument shape is recorded in
[Target caption text](target-caption-text.md#native-arguments-and-target-name).

OBSERVED. In the connector, `setDefaultWidgetLocation`, `main/f142` PCs 75 to
81, calls `createStaticWidget(8, "PublicInformDialogWidget", nil, 1, false)`.
`openPublicInformDialogWidget(A1, A2, ...)`, `main/f219`, returns when
`checkActor(A1)` is nil (PCs 0 to 5). Otherwise it calls
`getStaticWidget(8):setInitialData(1, A1, A2, ...)` (PCs 6 to 14) and
tail-calls `show()` (PCs 15 to 16). `openPublicInformLongDialogWidget`,
`main/f220`, is the same except that it passes 2 (PC 10).
`processUpdatePublicInformationDialog(A1, A2, A3, ...)`, `main/f98`, calls
`openPublicInformDialogWidget(A1, A3, ...)` (PCs 2 to 6). The helper's result
contract is `xivl-client-scripts:docs/job-grand-company-event-contracts.md`.

The chain is therefore `openPublicInformDialogWidget(owner, id, ...)` ->
static widget 8 `setInitialData(1, owner, id, ...)` -> `setTextByOwner` ->
`_setProperty(nil, "TextBlock_Text", "Text", owner, id, ...)`, then
`setCommonTimer(5)` and `show()`. The long helper arms 7 seconds.

MEASURED. `widget/PublicInformDialogWidget.form` and its `.sd.form` variant
both have a root `Window` named `Window_PublicInformDialogWidget` with
`SqwtStyle` `WIN_basis_active` and one `TextBlock` named `TextBlock_Text`.
They merge `widget/PublicInformDialogWidget.tpl` and
`widget/PublicInformDialogWidget.sd.tpl` respectively. Both templates hold the same
`UIFormCommands.TimerStart` CommandTrigger as TutorialSuccessWidget, bound to
`Window_PublicInformDialogWidget` `StringData.Value0`, at decoded lines 49 to
54 of the `.tpl` and 49 to 53 of the `.sd.tpl`. Neither has the
`SoundPlayerAction`.

OBSERVED. A byte search of all 2671 decoded chunks under `client/script`
finds the string `UIFormCommands.TimerStart` in exactly six:
`widget/achievementpopupwidget`, `widget/consoleicontraywidget`,
`widget/jobquestinformationwidget`, `widget/ask/questrewardwidget`,
`widget/tutorialsuccesswidget` and `widget/warningdialogwidget`. Neither
`widget/publicinformdialogwidget` nor the connector holds it. The template
trigger of PublicInformDialogWidget therefore has no script sender in the
corpus, and its hide runs through `processTimer`.

OBSERVED. `WidgetBaseClass._onInit`, `main/f0` of `widget/widgetbaseclass`,
jumps from PC 0 to PC 30. On that path it declares the `widgetWork._temp`
entry `{"commonTimer", "timer", true}` (PCs 47 to 51) and stores the list at
PC 66. `setCommonTimer(v)`, `main/f16` PCs 0 to 1, writes
`widgetWork.commonTimer`. `_onTimer`, `main/f17` PCs 0 to 1, calls
`self:processTimer()` with no further argument. The base `processTimer`,
`main/f18`, is empty. These are the base methods that
[confirm offer timers](../net/confirm-offer-timeout-evidence.md#timer-consumers-and-limits)
also reads.

## Common timer expiry and `_onTimer`

STATIC. Declaration operand. The `_temp` walker `0x00D1E0F0` reads element 1
of each entry (`0x00D1E1AD..0x00D1E1C0`). When that name equals
`_assignForChild` (pointer cell `0x0130D84C` -> `0x0110F6BC`) it keys on the
name. Otherwise it reads element 2 as the type tag
(`0x00D1E456..0x00D1E478`). It looks the tag up in the map at `0x01378118`
through `0x00D25DB0` (`0x00D1E4B6..0x00D1E4BD`), which calls the matched
constructor with the same Lua state and entry index. `0x00D1F170` fills that
map from `0x00D20190`. There the `timer` key (`0x00D28680`, cell `0x0130DA8C`
-> `0x0110FB48`) maps to `0x00D2CBD0` (`0x00D2033F..0x00D20358`).
`0x00D2CBD0` reads element 3 of the entry with `lua_toboolean`
(`0x00D2CBFD..0x00D2CC14`), installs vftable `0x0110FF84` and stores the
boolean at descriptor `+0x10` (`0x00D2CC4B..0x00D2CC51`). In the same map,
`array` (cell `0x0130D83C`) maps to `0x00D243E0` (`0x00D20399..0x00D203B2`)
and `2Darray` (cell `0x0130D840`) to `0x00D24670` (`0x00D203C6..0x00D203DB`).
`0x00D260D0` fills a second type map, and its only caller is the array handler
`0x00D243E0` (`0x00D24488`). In that second map the `timer` key maps to
`0x00D253F0` (`0x00D2627F..0x00D26298`), which reads element 5 with
`lua_toboolean` (`0x00D2541C..0x00D2542A`). The element-5 reader therefore
serves a timer array element and not a top-level timer entry. A top-level `{"commonTimer", "timer", true}` entry
therefore yields a timer descriptor whose operand byte is 1.

STATIC. Scheduling. The timer store `0x00D2CE00` calls `0x00CF5D10` on the
clock object when the operand byte is nonzero (`0x00D2CF0F`, `0x00D2CFDE`,
calls at `0x00D2CF4F` and `0x00D2D01B`). The store and clock are recorded in
[Timer members](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/work-binder-native-contract.md#timer-members).
`0x00CF5D10` passes its third to ninth arguments to `0x00CF4F60`, which
builds a record. At `0x00D2CFE4..0x00D2D01B` the store supplies, in that
order, its own first argument, the addresses of fields `+0x04`, `+0x18` and
`+0x28` of its second argument, the record in `ESI`, and its own third, fifth
and seventh arguments. When clock
`+0x3C` holds the sentinel `[0x0130C778]` (`0x00CF5D7D..0x00CF5D80`),
`0x00CF5D10` calls `0x00CF59A0` with the two values and the record. Otherwise
it takes a deferred path through clock `+0x30`, which was not read.
`0x00CF59A0` converts its second value through `0x00CF5240`
(`0x00CF5BA0..0x00CF5BB2`) and continues to its insertion path only for a
nonzero result. `0x00CF5240` returns 0 when the five bytes are zero
(`0x00CF5257..0x00CF52C7`) and otherwise `(seconds * 100 + hundredths) * 10`,
which is milliseconds (`0x00CF52E1..0x00CF5313`). The insertion path passes
that value and the map at clock `+0x10` to `0x00CEFB70`
(`0x00CF5BB8..0x00CF5BCB`). INFERRED from the decompiler rendering of that
path: the record is filed in that ordered map under the millisecond key, which
is the map that `0x00CF6080` walks.

STATIC. Expiry. `0x00CDA330` calls `0x00CF63E0` on the clock pointer at
`+0x184` of its object (`0x00CDA5CD..0x00CDA5D5`). `0x00CF63E0` updates the
clock snapshot and calls
`0x00CF6080` when the map count at clock `+0x18` is nonzero. `0x00CF6080`
divides the snapshot by 1000 (`0x00CF60BB..0x00CF60C3`), finds the first key
greater than that value (`0x00CF60F7..0x00CF6112`) and visits every earlier
record. For each it calls `0x00CF5EC0` with the clock as receiver
(`0x00CF6174..0x00CF617C`) and erases the record when the result is nonzero
(`0x00CF6181..0x00CF61BC`). `0x00CF5EC0` returns true without a call when
the record identity equals `[0x0130C778]` or when `0x00CD8160` on the engine
(clock `+0x00`) finds no entry. It returns false, and so keeps the record for
a later pass, when `0x00CE1BB0` returns true, entry byte `+0x7F` or `+0x7D`
is zero, or `0x00CE2650` returns false (`0x00CF5EFB..0x00CF5F3D`). Otherwise
it stores the record identity at clock `+0x3C`, builds an argument list from
record fields `+0x04`, `+0x14`, `+0x24`, `+0x28`, `+0x38` and `+0x3C`, and
calls `0x00CCF7D0` with the entry, engine `+0x128` and that list. The receiver
is the `0x00CF0910` result for engine `+0x1C0` (`0x00CF601D..0x00CF603B`). It
then restores the sentinel at clock `+0x3C` and returns true.

STATIC. Method name. The engine constructor `0x00CDC410` keeps `this` in
`ESI` from `0x00CDC437`. It copies `_onFinalize` (`0x0110E7D0`) to `+0xD4`
and `_onTimer` (`0x0110E7DC`) to `+0x128` (`0x00CDC52D..0x00CDC554`). It
constructs the clock with `0x00CF58E0`, which stores its argument at clock
`+0x00`, and passes `ESI` (`0x00CDC802..0x00CDC805`). The `0x00CCF7D0` call
above therefore receives the `_onTimer` string. `0x00CCF7D0` is the shared
named-method call recorded in
[Requested Lua frame](../event/user-data-receiver.md#requested-lua-frame).

Answer. A widget's `commonTimer` expiry raises `_onTimer` on the entry that
`0x00CD8160` resolves from the record, on the first `0x00CF6080` pass whose
millisecond snapshot reaches the deadline and whose entry passes the gates
above. The base `_onTimer` calls `processTimer`. For PublicInformDialogWidget
that is `hide()`, about 5 or 7 seconds after `setInitialData`. STATIC for each
step listed. INFERRED for three joins: that the record identity, the store's
first argument, resolves to the widget whose work holds the member, that
`0x00CF59A0` files the record under its millisecond key, and that its first
value is the previous deadline.

## Storyboard BeginTime

STATIC. Parsing. Initializer `0x00F290C0..0x00F290CF` names Timeline
descriptor `0x01361850` `BeginTime` (`0x0107C960`) with index 3. Setter
`0x009AD210` jumps through table `0x009AD314`. Index 3 is `0x009AD29D`,
which passes Timeline `+0x58` and descriptor `0x01361850` to `0x009BF130`
(`0x009AD29D..0x009AD2BB`). Duration, index 2, uses the same helper for
`+0x48`. `0x009BF130` calls `0x00916310` on the value. A result of
`0x01356DA8` sets cell flag `0x10` and returns (`0x009BF161..0x009BF17C`).
That this result marks a binding expression is INFERRED. Any other result
goes to the timespan parser
`0x009152B0` at `0x009BF186` and is stored through `0x009C2130` at
`0x009BF1AB`. The parser's units and its handling of text without a colon are
recorded in
[Clock source, units and epoch](../resource/sqwt-double-animation-clock.md#clock-source-units-and-epoch).

STATIC. Application. Reset `0x009AD020` copies `BeginTime` from `+0x58` to
`+0x110` (`0x009AD05D..0x009AD06D`). No instruction up to its returns at
`0x009AD1EC` and `0x009AD1FE` tests that cell's flag byte `+0x60`.
It copies Duration `+0x48` to the span at `+0xF8` (`0x009AD10B`,
`0x009AD157`). The tick `0x009AC820` multiplies the accumulated input `+0x100`
by SpeedRatio `+0x124` (`0x009AC8DE..0x009AC8EC`). It returns while that
product is below `+0x110` (`0x009AC990..0x009AC9A8`) and subtracts `+0x110`
before any sample (`0x009AC9D5..0x009AC9D7`). A span of all ones sends a
sample on every tick and never completes (`0x009AC9D9..0x009ACA24`).

STATIC. Empty Storyboard. Storyboard table `0x0107C32C` has `+0x38`
`0x009AC4B0`, `+0x4C` `0x009CAB40`, `+0x50` `0x009CAC70`, `+0x54`
`0x009A6490` and `+0x58` `0x009CAB00`. `0x009AC4B0` calls `+0x4C` when
`0x009A6B50` returns nonzero and stores the result as the total at `+0xE8`
(`0x009AC4B4..0x009AC4D5`). It then resets the embedded timer with due 0 and
period 1. `0x009CAB40` offers a value to Duration through `0x009AC730`
(`0x009CABCA..0x009CABE1`) and then calls `0x009AD020` (`0x009CABF0`).
`0x009AC730` calls `0x009C2130` on `+0x48` with mode 0
(`0x009AC730..0x009AC752`). The offered value is the largest `0x009AC360`
result over the children at `+0x138`, starting from 0
(`0x009CAB55..0x009CABC8`). With no child, the offered Duration is 0.
`0x009C2130` first resolves a pending flag `0x10` binding through
`0x009157C0` (`0x009C2138..0x009C214D`). In mode 0 it declines the write when
cell flag `0x02` is set (`0x009C2151..0x009C2171`) and otherwise clears flag
`0x01` and writes the value (`0x009C2179..0x009C21A5`). In `0x009AC820`, a
zero span with a count below the repeat limit sends one zero sample
(`0x009ACAB2..0x009ACB68`), advances the count (`0x009ACB6A`), and calls
`+0x58` once the count reaches the limit (`0x009ACC88`,
`0x009ACECD..0x009ACEDD`). `0x009CAB00` jumps to completion `0x009AD450` when
its argument is the receiver, as the tick passes it
(`0x009CAB00..0x009CAB0C`, `0x009ACEDA..0x009ACEDB`). `0x009AD450` disables
the timer, clears `+0x128` and calls `+0x54`. `0x009A6490`
dispatches the retained `CompletedCommand` at `+0x158` through its virtual
`+0x08`.

INFERRED. An empty Storyboard with no `Duration`, a finite repeat count of
one and SpeedRatio one completes on the first tick whose accumulated input
reaches its `BeginTime`, and raises `CompletedCommand` then. This assumes
that no write after construction sets cell flag `0x02` of Duration, and that the total at `+0xE8`, compared at
`0x009AC9AE..0x009AC9CF`, does not end that tick first. A bound text `3`
parsed by `0x009152B0` gives 3000 milliseconds.

## Storyboard BeginTime census

MEASURED. All 1154 `.form`, `.tpl`, `.style`, `.skin` and `.sml` files under
`client/sqwt` decode with `xivl extract-resource --as sqwt`. Of those, 1153
parse as XML. `system/common/Staffroll.form` fails on an unbound prefix and
contains no `Storyboard` text. The parsed documents hold 1316 `Storyboard`
elements, and 33 of them carry a `BeginTime` attribute on the `Storyboard`
element itself. No document uses property-element syntax for it.

| Client-relative template | Decoded lines | `BeginTime` | `CompletedCommand` | Children |
|---|---|---|---|---|
| `widget/TutorialSuccessWidget.tpl` | 52 | Binding `Window_TutorialSuccessWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/PublicInformDialogWidget.tpl` | 51 | Binding `Window_PublicInformDialogWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/PublicInformDialogWidget.sd.tpl` | 51 | Binding `Window_PublicInformDialogWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/AchievementPopupWidget.tpl` | 51 | Binding `Window_AchievementPopupWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/ConsoleIconTrayWidget.tpl` | 291 | Binding `Window_ConsoleIconTrayWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/GrandCompanyRecruitWidget.tpl` | 45 | Binding `Window_GrandCompanyRecruitWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/JobQuestInformationWidget.tpl` | 51 | Binding `Window_JobQuestInformationWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/QuestRewardWidget.tpl` | 51 | Binding `Window_QuestRewardWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/WarningDialogWidget.tpl` | 51 | Binding `Window_WarningDialogWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | None |
| `widget/GLInformDialogWidget.tpl` | 18 | Binding `Window_GLInformDialogWidget` `StringData.Value0` | `UIFormCommands.TimerFinish` | One `DoubleAnimation` with no attribute |
| `widget/ActionMenuWidget.sd.tpl` | 203 | Binding `CustomControl_TimerLabel` `FloatData.Value0` | `UILuaCommands.TimerEnd` | None |
| `widget/PopupHelpWidget.tpl` | 15 | Binding TemplatedParent `FloatData.Value0` | `PopupShowHelp` | None |
| `widget/HamletDefenseTutorialWidget.tpl` | 268 | Binding TemplatedParent `IntData.Value0` | None | One `DoubleAnimation` |
| `widget/HamletDefenseWidget.tpl` | 268 | Binding TemplatedParent `IntData.Value0` | None | One `DoubleAnimation` |
| `widget/StatusEffectWidget.tpl` | 17, 168 | Binding TemplatedParent `IntData.Value0` | None | One `PropertyAnimationUsingKeyFrames` and one `DoubleAnimation` |
| `widget/StatusEffectWidget.tpl` | 25, 176 | Binding TemplatedParent `IntData.Value1` | None | One `PropertyAnimationUsingKeyFrames` and one `DoubleAnimation` |
| `widget/StatusEffectWidget.tpl` | 37, 188 | `0:0:0` | None | Two `PropertyAnimationUsingKeyFrames` |
| `widget/StatusEffectWidget.sd.tpl` | 21, 178 | Binding TemplatedParent `IntData.Value0` | None | One `PropertyAnimationUsingKeyFrames` and one `DoubleAnimation` |
| `widget/StatusEffectWidget.sd.tpl` | 29, 186 | Binding TemplatedParent `IntData.Value1` | None | One `PropertyAnimationUsingKeyFrames` and one `DoubleAnimation` |
| `widget/StatusEffectWidget.sd.tpl` | 41, 198 | `0:0:0` | None | Two `PropertyAnimationUsingKeyFrames` |
| `widget/HamletDefenseWidget.tpl` | 48 | `0:0:0.4` | None | Two `DoubleAnimation` |
| `widget/ActionMenuWidget.tpl` | 594 | `0:0:0.2` | None | Two `DoubleAnimation` |
| `widget/HamletDefenseScoreWidget.tpl` | 151 | `0:0:0.2` | None | Two `PropertyAnimationUsingKeyFrames` |
| `system/installer/InstallPhase.tpl` | 25 | `0:0:0` | None | Four `DoubleAnimation` |
| `system/installer/InstallPhase.tpl` | 33 | `0:0:0` | None | 53 `PropertyAnimationUsingKeyFrames` |
| `system/installer/InstallPhase.sd.tpl` | 25 | `0:0:0` | None | Four `DoubleAnimation` |
| `system/installer/InstallPhase.sd.tpl` | 33 | `0:0:0` | None | 53 `PropertyAnimationUsingKeyFrames` |

Eleven of the 33 Storyboards have no child element. They are the rows whose
`CompletedCommand` is set, except GLInformDialogWidget. The ActionMenuWidget line
594 Storyboard is described in
[ActionMenu timer icon style](../resource/action-menu-timer-icon-style.md).

| Client-relative template | Bytes | SHA-256 | Decoded bytes | Decoded SHA-256 |
|---|---:|---|---:|---|
| `client/sqwt/system/installer/InstallPhase.sd.tpl` | 15596 | `7117e33812cd91484ca9c1c98b25dbe4e29f41db3183096723f1f26255455c7d` | 15588 | `870f66fd7b70b5e82d47219b371966a3f7e4c1afd55c751e5e9c61cbd7a2baaa` |
| `client/sqwt/system/installer/InstallPhase.tpl` | 15578 | `c0e8818505d3f4a0e352c73d04db2aba3064abbe982ed897306a7e3dc63a418b` | 15570 | `4f01bc741078072f8bbcccb240a29caaa562e43381687e44fc27821d38989eb5` |
| `client/sqwt/widget/AchievementPopupWidget.tpl` | 3185 | `184b2ebbf1dfbf3807cd3464168f4ab96275731f87503799606517505855d427` | 3177 | `7900bba2487ce10926755e33cf3dcbe3ad1f65128522ffab504f94f755e8ef7d` |
| `client/sqwt/widget/ActionMenuWidget.sd.tpl` | 40587 | `6359e2822ed2ce3cfd288f97d8bdd8b468b2b4624b5bc0ac0413d7a0e4053f9b` | 40579 | `07f70aedbb6cd8427f6fb76da03af6fac0433296b75a455dfd6ca0500178dc60` |
| `client/sqwt/widget/ActionMenuWidget.tpl` | 47246 | `18c4967b39ebce0c0b8a82d5d706b569bd538a2a5ea6b8c54bcb434612538be6` | 47238 | `2c239989bd6bfc4022cd4a18499f78f9957b1791165f806644da4581698c4871` |
| `client/sqwt/widget/ConsoleIconTrayWidget.tpl` | 20238 | `f3c362bb64d9e9b07dd60b2ba95e1b868640e43c82558b83c24d1fd32d3378f1` | 20230 | `9f1f5a2c79c2d8b35ea0d0b510c5536bcb43464da236ce1f9dea665f01937386` |
| `client/sqwt/widget/GLInformDialogWidget.tpl` | 4826 | `122a54e12f3902032547806ccfeef3e7566776b3bd0a18e577be855b797db126` | 4818 | `14cc2037d44c8d50f79ed95d8f5b0b4bb961115a498ab94d25fcd92a99dc1912` |
| `client/sqwt/widget/GrandCompanyRecruitWidget.tpl` | 2870 | `2c5478b1249131abc0d2830d235836b7214da3abf21fd90da836e8c5d92f8957` | 2862 | `e80560976538851af755e882c632bb8401c3a876116a9538a1f4b1c597e9d7ea` |
| `client/sqwt/widget/HamletDefenseScoreWidget.tpl` | 12927 | `e2acc230707af84ff468f2a4045ecbc1938e4113fedfa92a0226e04284563993` | 12919 | `9826d6cc8a05675f90004b9741596cea928b6fda5bb12fd410060b5e3a83c638` |
| `client/sqwt/widget/HamletDefenseTutorialWidget.tpl` | 34524 | `037ff8d05d7ca3d8b3c6cf17ee717a03856f190e72ff00cae2dd6e69d29ab55f` | 34516 | `2888c0415f5e9ac55db45b6de392731a96f628164319c4dc3c7a0c13267c8b82` |
| `client/sqwt/widget/HamletDefenseWidget.tpl` | 32684 | `0f9e675a2dde33975e0fc9b34a86c8aac49c5831946061db1e5bbc4e3b2ad4de` | 32676 | `91fc6d5370b814144002762503c795442496eb49a9dc1eca60fe194033853a28` |
| `client/sqwt/widget/JobQuestInformationWidget.tpl` | 3211 | `507c4df5863bcafe8355c88979bc7b9a21c92b2a280c4d427706edcf4c6adb43` | 3203 | `8f982bc6ff880e30e7e43a35a5653b99752b8c1cfaf8c72cfd157157beb91a54` |
| `client/sqwt/widget/PopupHelpWidget.tpl` | 1895 | `c914db575a00f9f37ae7a1156f58e48ad6bf394ba90e6f23592f45acc236c2ce` | 1887 | `00569baa91ca2299ec5ddb22e41dcbae820562e9f8d832bf317383395774a248` |
| `client/sqwt/widget/PublicInformDialogWidget.sd.tpl` | 3232 | `c8dbe34f7f6fb1831a89c37134085aece955cfc34af66af94b889ea18491328d` | 3224 | `c04ed041b093e61981dbe994e1d871c083233890b3c7af5914fc4ee76899a338` |
| `client/sqwt/widget/PublicInformDialogWidget.tpl` | 3210 | `92d9ed5d05fdd8cb744e31164621adf1041f4c45e0d49551fc30839c22198019` | 3202 | `4170a4395e0000c5b04c99677e5d59fe01282aa15aecbce2b0801cde199d2889` |
| `client/sqwt/widget/QuestRewardWidget.tpl` | 6616 | `b5d6dfc5c554102cf0f4c57b178e0b692b4fba62152e30103c3326ed5e48119a` | 6608 | `3bff6ad5e2c38e5964ac95e7ab4d0324fc75fbd4df954ea1805b7fcb3eb019c8` |
| `client/sqwt/widget/StatusEffectWidget.sd.tpl` | 15778 | `d5c2c40cb7887b85c4e553e32ec0654084996de0fd4ee4ba617df5a35018c252` | 15770 | `ef54905c477d1a0566a6aac5593ae7904c80cefe5c3aa31e63597c1691986209` |
| `client/sqwt/widget/StatusEffectWidget.tpl` | 15836 | `9279912458c3e690a8446b3a4018c5e484ae1d6741f7107f2e607abd30e5cf91` | 15828 | `d42e90d7da5f2c7c1456d0399b688a92d29ffc5094b5309153d6fe569eee57a3` |
| `client/sqwt/widget/TutorialSuccessWidget.tpl` | 3231 | `14e1bd46ce376e4d14ead1f30f43cf86469bcb10233ddc0e28ec9e1fba239acb` | 3223 | `78d32ca8d3f1f0fd6097f7af1974118acac243d3604a9203196e354d9a32160a` |
| `client/sqwt/widget/WarningDialogWidget.tpl` | 3182 | `f249701e336740c4c9da73709115d308e6f0242ced8c2df0a3b6e8c5a09715a3` | 3174 | `77465582523579ca4821907e426b0d783a734293d7c276e73534d6ac782ab520` |

## Open items

- NOT TRACED. The binding transfer that writes the bound `StringData.Value0`
  text into Timeline `+0x58`, and its timing relative to Begin. Reset
  `0x009AD020` reads `+0x58` without resolving a pending binding, and no
  reader of descriptor `0x01361850` other than its initializer, the property
  array cell `0x012E7FC4`, the setter and the constructor `0x009AD590` was
  found.
- NOT TRACED. Whether any write after construction sets Duration cell flag
  `0x02`, which decides whether the zero offered by `0x009CAB40` is written.
  STATIC: constructor `0x009AD590` builds the Duration cell through
  `0x009AD900` (`0x009AD637..0x009AD64D`), which copies bit 7 of descriptor
  `+0x18` into flag `0x02` (`0x009AD969..0x009AD979`). For Duration descriptor
  `0x0136182C` that word lies in zero-filled data that its initializer
  `0x00F290A0` does not write, so the flag starts clear.
- NOT TRACED. The native route from `_sendStoryboardCommand` to the
  `CommandTrigger` of the window template, and from a dispatched
  `CompletedCommand` to `_onUICommandEvent` under the `_widget` condition.
- NOT TRACED. How often `0x00CDA330` runs, which bounds how late a due
  `commonTimer` record fires, and the meaning of the gate `0x00CE2650`.
- NOT TRACED. The callers of `processUpdateGeneralNotificationDialog` and
  `processUpdatePublicInformationDialog`, and whether a network packet drives
  them.
- NOT TRACED. Whether a second `setCommonTimer` before expiry removes the
  first record. `0x00CF59A0` handles its first value before filing the second,
  and that branch was not read.

## Reproduction

Unwrap and list the five scripts as in
[Work update callback consumers](work-update-callbacks.md#reproduction), check
the decoded sizes and hashes above, and read the prototypes named in the text.
Decode the markup with `xivl extract-resource --as sqwt` and check the decoded
sizes and hashes. For the census, decode every `.form`, `.tpl`, `.style`,
`.skin` and `.sml` file under `client/sqwt`, parse each document as XML and
select `Storyboard` elements that carry a `BeginTime` attribute. For the
corpus search, unwrap every `.lpb` under `client/script` and search the
decoded bytes for the command string. Disassemble the cited windows of
`ffxivgame.exe` with ghidra-cli, compare each instruction's bytes with the
file-backed section bytes, and read the cited strings and pointer cells from
the file.
