# NpcSay window chain

The conversation window of an NPC talk is the widget `NpcSayWidget`. This page
traces what shows it, what raises its `NpcSayStart` and `NpcSayEnd` commands,
what closes it when the event ends, and where it sits relative to the event
packets. The key wait that holds a message is in
[LogControl key wait, Wait0 and page advance](../resource/logcontrol-key-wait.md).
The speaker name is in
[LogControl Talker and the speaker display name](../resource/logcontrol-talker-name.md).

## Summary

| Question | Answer |
|---|---|
| What raises `NpcSayStart`? | The Lua `NpcSayWidget.changeDisplay(true)`. It runs on the first `UILuaCommands.TextChanged` while the window is shown, from `display(true)` in the cut scene clip handler, and from the debug command `npcsay` with argument `on` |
| What raises `NpcSayEnd`? | The Lua `changeDisplay(false)`, reached through `display(false)` in the cut scene and cut scene player scripts and the debug command `npcsay`. No `display(false)` caller was found on the ordinary event end path |
| What shows the window? | The desktop mode order `16`, placed by `PlayerBaseClass._onPreEvent`, which sets static widget 14 force visible. The `Shown` event then sets the window opacity to 0 |
| What closes the window? | `PlayerBaseClass._onPostEvent` cancels mode `16`. The fallback mode no longer forces the widget visible, so `hide` writes `Anime` `Hide`, the `Hiding` animation fades the window, and `InvisibleWindow` hides it |
| What raises `TextChanged`? | The `LogControl`, through `0x005910F0`, when its document reports new or changed rows |
| What releases a parked message? | `RaptureCommands.LogControlWait0`, raised by the LogDocument when the last wait weight is released. What gives a row a weight is UNKNOWN, see [key wait](../resource/logcontrol-key-wait.md#wait-weight-source) |
| Where does the speaker name come from? | The `Talker` property, filled from the record's source name, which is the NPC's display-name state |
| What does the EndEvent packet do to the window? | It reaches `_onPostEvent` through the PlayerManager end routines. The four end routines contain no direct call to the EventUpdate builder `0x0075E670`. Whether the block slots or `_onPostEvent` lead to a `0x012E` reply was not traced |

## Input and method

Observed against FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. Addresses are VAs. Ghidra CLI navigated the code and Capstone
5.0.7 listings of the identified PE checked the cited instructions. OBSERVED
marks instructions, data, decoded Lua bytecode and decoded forms. INFERRED marks
an interpretation of observed facts. UNKNOWN marks what was not traced.

Lua facts come from the retail Lua 5.1 bytecode, decoded with `decode_lpb.py`
and the `unluac` build vendored in `xivl-client-scripts`. Script names and
line numbers follow `xivl-client-scripts:lua/registry.json`. The form
`client/sqwt/widget/NpcSayWidget.form` and template
`client/sqwt/widget/NpcSayWidget.tpl` were decoded with the recipe in
`xivl-client-data:docs/sqwt-npc-dialogue-structure.md`, which records their
sizes and hashes. This page adds the template triggers that finding does not
list. The `.sd.form` and `.sd.tpl` variants have no loader in the executable
([form locale resolution](../resource/sqwt-form-locale-resolution.md)).
`tools/verify_npcsay_log_wait.py` re-checks the executable facts that are
listed in the two resource pages.

## Text producers

OBSERVED. `NpcBaseClass.say(quest, row, gesture, ...)` first runs a fixed
character scheduler selected by its third argument, then calls
`desktopWidget:showMessage(self, 38, quest, row, ...)`
(`chara/npc/npcbaseclass_event.lua:247-312`). `DesktopWidget.showMessage` calls
`_appendMessagePool` with the same arguments (`widget/desktopwidget.lua:369-384`).
`WorldMaster.say` supplies category 40 and no NPC source
([WorldMaster message pool](../script/worldmaster-message-pool.md)).
`startCliantTalkTurn` and `finishCliantTalkTurn` only turn and look at the
player and call no widget method (`npcbaseclass_event.lua:34-237`).

OBSERVED. `NpcSayWidget.init` sets the `LogControl_NpcSay` property `Categories`
to the string `"38,40"` (`widget/npcsaywidget.lua:36-50`), so the control admits
exactly those two categories.

## Window declaration

OBSERVED. The form `Window_NpcSayWidget` holds `Label_NpcName` with
`TextBlock_NpcName`, and `LogControl_NpcSay` with `LinesPerSec="10"`,
`IsEnableKeyWait="True"`, `IsDisplayLastOne="True"`, `LogFormat="2600"`,
`FontSize="16"`, `FontHeightSpacing="2"`, both scroll bars disabled and
`IsHitTestVisible="False"`. The control sits in a body `Label` of width 620 and
height 106. The window is `IsHitTestVisible="False"` and `IsTabStop="False"`.
No shipped script sets these properties, and the
[key-wait page](../resource/logcontrol-key-wait.md#control-properties-of-the-npcsay-and-chat-logs)
records what they do.

OBSERVED. The template `ControlTemplate_Window_NpcSay` has these triggers in
document order.

| Trigger | Actions |
|---|---|
| `EventTrigger` `Shown` | Stops the closing and hiding animations, sets `VisualOpacity` 0 on `Window_NpcSayWidget` |
| `CommandTrigger` `UILuaCommands.NpcSayStart` | Stops hiding and closing, begins `OpeningAnimation`, a `VisualOpacity` animation `To="1"` over 0.2 seconds |
| `CommandTrigger` `UILuaCommands.NpcSayEnd` | Stops opening and hiding, begins `ClosingAnimation`, `To="0"` over 0.2 seconds |
| `EventTrigger` `Closing` | Stops all three, runs a fade to 0 over 0.2 seconds, completes with `ApplicationCommands.DestroyWindow` |
| `EventTrigger` `Hiding` | Stops opening and closing, begins `HidingAnimation`, a fade to 0 over 0.2 seconds, completes with `ApplicationCommands.InvisibleWindow` |
| `EventTrigger` `TextChanged` from `LogControl_NpcSay` | Executes `UILuaCommands.TextChanged` on `Window_NpcSayWidget` |
| `CommandTrigger` `LogControl.PressedKey` | Executes `UILuaCommands.PressedKey` on `Window_NpcSayWidget` and plays a `SoundPlayerAction` with `FileNumber="50"` |

The executable contains no string `NpcSay` and no `UILuaCommands.TextChanged`,
`UILuaCommands.PressedKey`, `UILuaCommands.NpcSayStart` or
`UILuaCommands.NpcSayEnd`. Those names exist only in the form data and the Lua.
The mapping of the sound file number to a sound is UNKNOWN.

## NpcSay widget script

OBSERVED. `widget/npcsaywidget.lua` defines six methods.

| Method | Lines | Behavior |
|---|---|---|
| `init` | 11-82 | Declares `work.displayWidget` as boolean and sets it false. Subscribes to `UILuaCommands.TextChanged` and `UILuaCommands.PressedKey` through `setUICommandCondition`, resets the consumed command `UILuaCommands.Cancel`, sets `Categories`, and sets `IsHitTestVisible` false on the control and its `PART_TextArea`, `PART_ContentHost`, caret and wait-cursor parts |
| `processUICommandEvent` | 85-132 | On `UILuaCommands.TextChanged` and `isShow()` true: sets `SelectionLength` 0, reads `Talker`. A nonempty `Talker` makes `Label_NpcName` visible and sets `TextBlock_NpcName`. An empty one hides the label. Then, if `work.displayWidget` is false, calls `changeDisplay(true)`. No other command is handled |
| `processAfterHide` | 135-148 | Clears `TextBlock_NpcName` and sets `work.displayWidget` false |
| `display` | 151-159 | Calls `changeDisplay` with its argument |
| `cancelLogWait` | 162-171 | Sends `RaptureCommands.LogControlPressKeyCancel` to `LogControl_NpcSay` |
| `changeDisplay` | 174-193 | `true`: sends `UILuaCommands.NpcSayStart` and sets `displayWidget` true. Otherwise sends `UILuaCommands.NpcSayEnd` and sets it false |

The script does not read the message text and does not drain the pool. It
handles no `PressedKey` branch, so that command only reaches the widget while
the template plays the sound. `isShow` is true when the `Visibility` property
is not `Hidden` and the `Anime` property is not `Hide`
(`widget/widgetbaseclass_common.lua:1045-1067`).

The subscription gate and the delivery of the command to Lua are in the
[command-parameter consumer](../resource/sqwt-command-parameter-consumer.md#subscription-gate).
`sendCommand(name)` forwards to `sendControlCommand(widgetName, name)`, which
calls `_sendStoryboardCommand` (`widget/widgetbaseclass_common.lua:1255-1283`).
The native wrapper `0x006EBF30` and its lookup by control name are in
`xivl-client-structs:docs/native-input-ui-boundaries.md`. How the native layer
resolves a form-declared name such as `UILuaCommands.NpcSayStart` to a command
object was not traced.

## What raises NpcSayStart and NpcSayEnd

| Raiser | Command | Condition |
|---|---|---|
| `processUICommandEvent`, `TextChanged` branch (`npcsaywidget.lua:85-132`) | `NpcSayStart` | `isShow()` true and `displayWidget` false |
| `CutScene._onShowWidgetClip` (`gamedata/cutscene_common.lua:826-934`) | `NpcSayStart` through `display(true)` | The clip names `NpcSayWidget` |
| `CutScene._onHideWidgetClip` (`cutscene_common.lua:937-1030`) | `NpcSayEnd` through `display(false)` | The clip names `NpcSayWidget` |
| `CutScene.startCutScene` (`cutscene_common.lua:1064-1316`) | `NpcSayEnd` through `display(false)` | Twice, at lines 1082 and 1275, after `_loadCutScene` and after the play or replay call. Each fetches static widget 14 just before, at lines 1078 and 1271 |
| `PopulaceCutScenePlayer.processCutScenePlay` (`chara/npc/populace/populacecutsceneplayer.lua:300`) | `NpcSayEnd` through `getStaticWidget(14):display(false)` | Before `startFadeInCutSceneDefault` |
| `CommandDebuggerTEST.command`, branch `npcsay` (`commanddebugger/commanddebuggertest.lua:455-502`, index at line 498) | `NpcSayStart` or `NpcSayEnd` through `getStaticWidget(14):display(on)` | The first argument is `on` for true and `off` for false |

OBSERVED. A scan of all 2671 shipped `.lpb` scripts for the string
`getStaticWidget` finds 13 scripts. The constant index 14 occurs in exactly the
cut scene script (four calls, two of them the clip handlers above), the
populace cut scene player script and the debug command script. The other hits
pass other constant indices or a variable index, and the variable-index callers
were not followed. Three scripts contain the string `NpcSay` (the NpcSay widget
script, the desktop connector and the cut scene script), so a search on that
string alone misses the other two raisers. The string `cancelLogWait` occurs
only in the NpcSay and `widget/logwidget` scripts. The `TextChanged` raise itself is in the
[key-wait page](../resource/logcontrol-key-wait.md#events-raised).

## What shows and hides the window

OBSERVED. `DesktopWidget.setDefaultWidgetLocation` creates the window as static
widget 14 with `createStaticWidget(14, "NpcSayWidget", nil, 1, true)`
(`widget/desktopwidget_connector.lua:8197`). The meaning of the last argument is
UNKNOWN. It is INFERRED to be the initial visible flag, because `forceVisible`
re-shows a widget only when its `visibleFlag` is true.

OBSERVED. `DesktopWidget.setDesktopModeDetail(mode, level)`
(`desktopwidget_connector.lua:8550-9003`) sets a flag per mode and ends by
calling `setStaticForceVisible(14, flag)` (`8971`), then
`getStaticWidget(14):setDrawPriority(...)` (`8976`), then
`sendDesktopCommand("RaptureCommands.LogControlPressKeyCancel")` (`8998`). The
flag that feeds static widget 14 is true for modes `16`, `61` and `63`, and
false for modes `0`, `1`, `8`, `32`, `62`, `120`, `126` and `127`.

OBSERVED. Mode orders and cancels are leveled.
`orderDesktopWidgetMode(mode)` (`8335-8361`) stores the mode at its level and
applies it when the level is not below the current one.
`cancelDesktopWidgetMode(mode)` (`8363-8465`) clears the level and applies the
nearest lower nonzero level, so cancelling mode `16` returns to the level below
it, whose flag is false for the widget.

OBSERVED. `PlayerBaseClass._onPreEvent` (`chara/player/playerbaseclass.lua:2062-2081`)
locks player, lock-on and target-cursor control and calls
`orderDesktopWidgetMode(16)`. `_onPostEvent` (`2084-2107`) calls
`cancelDesktopWidgetMode(16)`, `closeAllEventModeWidget`, and unlocks the
controls. The native callers and the callback argument shapes are in the
[callback vectors](event-base-callback-arguments.md#callback-vectors). Block
events reach them through the Block record slots 1 and 2 recorded in
[pre, post and cancel timing](event-base-callback-arguments.md#pre-post-and-cancel-timing).
The Request leaves never reach `_onPreEvent` or `_onPostEvent`.

OBSERVED. `WidgetBaseClass.forceVisible(true)` clears `forceHideFlag` and calls
`show(true, false, true)` when `visibleFlag` is true. `forceVisible(false)` sets
`forceHideFlag` and calls `hide(true, false, true)`
(`widget/widgetbaseclass_common.lua:1097-1150`). `show` with its first argument
true writes `Anime` `Show`, and `hide` writes `Anime` `Hide`; `hide` ends by
calling `processAfterHide` (`868-952`, `973-1033`). The `Anime` route raises
`Shown` on show and `Hiding` on hide, and `Hiding` is followed by the hide
routine's Hidden write unless a handler cancels it
([Window handler](../resource/sqwt-anime-property.md#window-handler)).

INFERRED. The template's `Hiding` trigger begins a storyboard whose completion
command is `ApplicationCommands.InvisibleWindow`, and that is the only handler
of `Hiding` in the form. This is the reading under which the 0.2 second fade is
visible before the window becomes hidden. The way a handler sets the cancel byte
of the `Hiding` event is UNKNOWN in the Anime page and stays so here. The
`Closing` trigger and `DestroyWindow` are not on any path traced here.

Resulting sequence for an ordinary talk, OBSERVED for each step and INFERRED
as a whole:

1. `_onPreEvent` orders mode `16`. Static widget 14 becomes force visible and
   `Anime` `Show` raises `Shown`. The window is visible with opacity 0.
2. A `say` reaches the control. The control raises `TextChanged`, the
   subscribed Lua runs, shows the name label, and raises `NpcSayStart`. The
   window fades to opacity 1 over 0.2 seconds.
3. Each later row or page raises `TextChanged` again. `NpcSayStart` is not
   repeated while `displayWidget` is true.
4. The event ends. `_onPostEvent` cancels mode `16`. The cancel also ends in
   the cancel fan-out of `setDesktopModeDetail`, which releases any parked
   message ([key wait](../resource/logcontrol-key-wait.md#release-of-the-message-yield)).
   `hide` writes `Anime` `Hide`, `Hiding` fades the window, `InvisibleWindow`
   hides it, and `processAfterHide` clears the name and the flag.

The control keeps its rows after the window hides. The next show raises
`Shown`, which resets the opacity to 0, and the window stays transparent until a
new `TextChanged` raises `NpcSayStart`. That consequence is INFERRED from the
trigger table and the script.

## EndEvent and the client reply

OBSERVED. The EndEvent receiver, packet `0x0131`, dispatches through
`0x008A13A0`. Event types 0 to 5 take the invoke chain `0x008A09E0`, whose
descriptor type byte selects one of the thunks `0x006E1080`, `0x006E10A0`,
`0x006E10C0` and `0x006E10E0` ([receiver page](end-order-event-receiver.md)).
They call the PlayerManager end routines `0x008934A0`, `0x00893520`,
`0x008935B0` and `0x00893800`. None of the four contains a direct call to the
EventUpdate builder `0x0075E670`.

OBSERVED. Routine `0x008934A0` requires an active block at PlayerManager `+8`,
matches the event name through `0x00445D20`, calls the block's virtual `+0x1C`
with 0, sets the block byte `+0x1C` when it is clear, and calls the block's
slot 2 (`+8`). Routine `0x00893520` calls virtual `+0x1C` with 1, sets the
block byte `+0x1D` and calls slot 1 (`+4`). For a client-side Block record,
slot 2 clears the pending flag and calls `_onPostEvent` with true, and slot 1
raises the cancel callback and then `_onPostEvent` with false
([timing](event-base-callback-arguments.md#pre-post-and-cancel-timing)).
Which event type bytes select which thunk for a talk event was not traced.

INFERRED. The `0x012E` completion reply of a dialogue function comes from the
executor's normal completion path `0x00894AB0`
([completion serializer](../script/delegate-command-producer.md#completion-reply-serializer)),
which runs after the Lua function returns. The function returns after its last
`say` resumes. A server that waits for that reply before it sends EndEvent
therefore gets the reply after the key press and before the window hides. A
server that sends EndEvent while a message is still parked reaches
`_onPostEvent` with the message parked, and the cancel fan-out then releases it.
Whether a reply is still produced for the released function is UNKNOWN.

## Open items

- Which EndEvent type bytes select each end routine for a talk block, and the
  selector table for type 51 is in
  [server-order talk admission](server-order-talk-admission.md).
- The native resolution of form-declared command names, and the sound played for
  `FileNumber="50"`.
- The cancel mechanism of the `Hiding` event.
- The meaning of the last argument of `createStaticWidget`.
- The `Closing` and `DestroyWindow` path, which no traced caller uses for this
  window.
- The mode levels of modes `61` and `63` and their producers. Only mode `16`
  was followed to the event callbacks.

## Reproduction

Decode the NpcSay script and the cut scene, desktop and player scripts with the
repository decoder and `unluac`, and read the cited ranges. Decode the form and
template with the Blowfish recipe in the finding named above and list the
trigger elements. Read the cited ranges in the executable and run
`python tools/verify_npcsay_log_wait.py --exe <ffxivgame.exe>`. No Lua body,
instruction body, XML body or decompiled body is needed in a published
artifact.

## Consumer notes for bahamut-client

A client may implement these parts of the contract.

- Show the window from the event callbacks, not from the first message. Order
  the window visible with opacity 0 when `_onPreEvent` places mode `16`, and
  hide it when `_onPostEvent` cancels the mode.
- Raise `NpcSayStart` once per window show, on the first `TextChanged` while
  the window is visible, and play the 0.2 second fade to opacity 1. Do not raise
  `NpcSayEnd` at the end of an ordinary event. Raise it only for the cut scene
  paths above.
- Hide with the 0.2 second fade to opacity 0, then set the window hidden, then
  clear the name and `displayWidget`.
- Show `Label_NpcName` only while `Talker` is nonempty.
- Release a parked message when the mode is cancelled, as the key-wait page
  records. That the release resumes the `say` yield is INFERRED there.
- Treat a press as the bindings in the key-wait page and not as a click inside
  the text area. The window and its control are not hit-testable.

These stay CALIBRATION: the language column and operand rules of the name
macro, which EndEvent type byte ends a talk block, the sound for
`FileNumber="50"`, the order of the `0x012E` reply against a parked release, the
row template (`LogFormat`), and what holds a one-line message. `LinesPerSec`
paces only the scroll of the text area, as the key-wait page records. A client that
keeps the stand-in of showing the window only while its control holds a line
differs from this behavior at event start, where the window is shown and
transparent before any message arrives.
