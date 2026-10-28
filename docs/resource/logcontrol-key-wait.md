# LogControl key wait, Wait0 and page advance

This page traces how a `LogControl` holds a message until a key press, which
input raises the press commands, what raises `RaptureCommands.LogControlWait0`,
and how that command releases the `DesktopWidget._appendMessagePool` yield.
The window that hosts the NpcSay `LogControl` is in the
[NpcSay window chain](../event/npcsay-window-chain.md). The speaker name is in
[LogControl Talker and the speaker display name](logcontrol-talker-name.md).

## Input and method

Observed against FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All locators are VAs. Ghidra CLI navigated the code, and Capstone
5.0.7 listings of the identified PE checked every cited instruction and data
word. Decompiler output is navigation only. OBSERVED marks instructions, data
and decoded assets. INFERRED marks an interpretation of observed code. UNKNOWN
marks what was not traced. `tools/verify_npcsay_log_wait.py` re-checks the
listings, tables and caller sets below, and `tools/test_npcsay_log_wait.py`
covers it.

The `key_config` rows come from the sheet exports `key_config.csv`,
`key_config2.csv` and `key_config3.csv`. Their SHA-256 values equal the entries
in `xivl-client-data:manifests/tables.json`
(`9e98035b2775a07f17df82ce391e95f89b1d900e993279135ca78f50a6105c1b`,
`f191bc788357696c7dab42011742ee28d396c57ef3d40e9188facb6690816183`,
`70c0b0db86e3ad11abcdc45e301ff601174ffc2b6ea38bca80d9729415419773`).

## Command objects

OBSERVED. Each command is a static `RoutedCommand` built by an initializer that
pushes a name literal and calls the constructor `0x00928C90` on a fixed
object. The literal is referenced only by its initializer.

| Command | Object | Initializer | Literal |
|---|---|---|---|
| `RaptureCommands.LogControlWait0` | `0x0133A438` | `0x00F0E2D0` | `0x00FA8230` |
| `RaptureCommands.LogControlPressKey` | `0x0133A4D8` | `0x00F0E350` | `0x00FA8250` |
| `RaptureCommands.LogControlPressMouse` | `0x0133A578` | `0x00F0E3D0` | `0x00FA8274` |
| `RaptureCommands.LogControlPressKeyCancel` | `0x0133A618` | `0x00F0E450` | `0x00FA829C` |
| `LogControl.PressKey` | `0x0133C158` | `0x00F0F940` | `0x00FA8A40` |
| `LogControl.PressKeyCancel` | `0x0133C1F8` | `0x00F0F9C0` | `0x00FA8A54` |
| `LogControl.PressedKey` | `0x0133C298` | `0x00F0FA40` | `0x00FA8A70` |
| `ApplicationCommands.Operate` | `0x01357C88` | `0x00F21460` | `0x0106A4BC` |

The `RaptureCommands.LogControl*` names are desktop wide. The `LogControl.*`
names are per control. No executable code references the three press commands
by object except the container handler and the LogControl handler below, so
script and key-binding paths reach them by name. That reading is INFERRED.

## Input that raises the press commands

OBSERVED. The key-binding sheet binds the desktop commands to input. In
`key_config.csv` the row with id 10, labelled `OK`, binds
`\[@KeyOperate1=Enter]` to `RaptureCommands.LogControlPressKey`. The row with
id 15 binds `\[@KeyOperate2=]`, an empty default, to the same command. The row
with id 228 binds the pad button `RRight` to it, and the row with id 249 binds
`LeftClick` to `RaptureCommands.LogControlPressMouse`.
`key_config2.csv` ids 29, 34 and 37 give `Enter`, `RRight` and `LeftClick` for
the same two commands. `key_config3.csv` ids 10, 15, 228 and 249 repeat the
`key_config.csv` layout with `NumPad0` in place of `Enter`. The key-binding
route that turns these rows into bindings is in the
[movement command origin](../input/player-movement-command-origin.md#parsed-targets-and-installed-groups).

OBSERVED. The `LogControl` handler `0x005958B0` also raises
`RaptureCommands.LogControlPressKey`. When the incoming command is
`ApplicationCommands.Operate` and the event is not yet handled, it executes
the object `0x0133A4D8` with the container pointer at control `+0x1B08` as the
target (`0x005959A9..0x005959DE`). The NpcSay control is not hit-testable and
is not a tab stop, so this path is INFERRED not to carry the NpcSay press.
Which physical key or click produces `Operate` for another control is outside
this page.

OBSERVED. The `CommonAskWidget` template listens for
`RaptureCommands.LogControlPressKey` with a `CommandTrigger` whose action is
`UILuaCommands.Operate`. The same key press therefore also operates an ask
list. The template is `client/sqwt/widget/CommonAskWidget.tpl`, decoded with
the recipe in `xivl-client-data:docs/sqwt-npc-dialogue-structure.md`.

## Container fan-out

OBSERVED. `RaptureElementContainer` keeps a vector of live `LogControl`
objects at container `+0x17848`. The `LogControl` constructor `0x00595A20`
appends itself through `0x004DBAD0` at `0x00595FB0`, and the destructor path
removes itself through `0x004D9B30` at `0x00591D75`. Each call passes the
container stored at control `+0x1B08`.

OBSERVED. Container handler `0x004D9D70` (container interface `+0xB4`, slot
word `0x00F912C0`) acts on these commands after the base handler returns and
only while the handled byte behind event `+0x10` is zero.

| Event command | Action | Locator |
|---|---|---|
| `LogControlWait0` | When the dword at event `+8` is nonzero, calls `0x00575900` with receiver `H+0x45C`. Sets the handled byte `+0x19`. | `0x004D9E8F..0x004D9EAD` |
| `LogControlPressKey`, `LogControlPressMouse` | Increments the global at `0x0132FB8C`, stores the old value in the parameter record, then executes `LogControl.PressKey` on each registered control plus `0xB4`. Sets the handled byte when an execution returns 1. | `0x004D9EB2..0x004D9EC2`, `0x004DA015..0x004DA0D4` |
| `LogControlPressKeyCancel` | Executes `LogControl.PressKeyCancel` on each registered control plus `0xB4` with parameter 0. | `0x004D9EC8..0x004D9F8B` |

OBSERVED. The event constructor `0x00929100` stores zero in the dword at event
`+8` (`0x00929137`). A writer that makes it nonzero for a `LogControlWait0`
event was not found, so whether the release raise in slot 3
passes the `+8` test is UNKNOWN. That field is not shown to be an executed
flag, a phase or a parameter.

OBSERVED. Callback `0x00575924..0x0057596B` resolves the identity
`0xC0000024` and calls `0x006DCA80` on a nonnull result, which stores byte
`+0x79 = 1`. The identity names the local `DesktopWidget`
([actor instantiation](../actor/actor-instantiation-and-self-player.md#local-desktopwidget)).
That the stored-to object equals the receiver of `_appendMessagePool` is
INFERRED from the identity and from the `desktopWidget` global used by
`DesktopWidget.showMessage`.

OBSERVED. The press and mouse commands do not distinguish their source after
fan-out. Both reach every registered `LogControl`, including controls with no
key wait, which ignore the press (see the release gate below). The purpose of
the serial at `0x0132FB8C` is UNKNOWN. The release routine compares the
parameter at event `+0x7C` with the global `0x01270CE0` and increments
`0x0133BDD8` when they differ (`0x00590CF0..0x00590D02`).

## LogControl handler

OBSERVED. Handler `0x005958B0` (receiver `control + 0xB4`) calls the base
handler `0x0095A980` and then acts on an unhandled event.

| Event command | Action | Locator |
|---|---|---|
| `LogControl.PressKey` | Calls the press routine `0x00591180` | `0x00595996..0x005959A4` |
| `LogControl.PressKeyCancel` | Repeats `0x00591180` and the per-frame update `0x005954D0` once per pending wait, then clears the wait counters | `0x0059593B..0x0059598E` |
| `ApplicationCommands.Operate` | Executes `LogControlPressKey` on the container | `0x005959A9..0x005959DE` |
| four scroll commands at `0x0135C0B0`, `0x0135DCB8`, `0x0135C210`, `0x0135D850` | Reset the scroll state at control `+0x1E58` and `+0x1E50`, then call `0x00590F40`, which raises `LogChanged`. Three of the four are initialized as `ScrollViewer.LineUpCommand`, `TextBoxBase.PageUpCommand` and `TextBoxBase.ScrollToTopCommand` | `0x005958E9..0x0059592B` |

OBSERVED. The press routine `0x00591180` requires the module cached at control
`+0x1B0C` to be nonzero. It calls LogDocument interface slot 3
(`0x00590BB0`) with the start row at control `+0x1F00`, the page row count at
`+0x179C`, the container at `+0x1B08` and the event. When the call returns 1
it raises `TextChanged` through `0x005910F0`, recomputes the start row as the
value at document `+0x150` (`I+0x128`) minus the page row count, with a floor
of 0, sets the event handled byte, and, when slot 4 then reports no wait,
writes the Visibility property value 1 (Hidden) on the object embedded at
control `+0x1B10` through `0x0093C2A0` (`0x005911EE..0x00591212`). The
per-frame update `0x005954D0` writes the same property with 2 (Visible) while a
wait is pending and 1 otherwise (`0x00595610..0x00595633`). That the object at
`+0x1B10` is the `PART_WaitCursor` is INFERRED from the part name in the
constructor and the wait-dependent visibility. The source of the page row count
is UNKNOWN.

## LogDocument wait state

OBSERVED. The LogDocument interface sits at document `+0x28` with table
`0x00FA9504`: slot 0 `0x00599500`, slot 1 `0x00593950`, slot 2 `0x00594A20`,
slot 3 `0x00590BB0`, slot 4 `0x00590960`. Let `I` be the interface receiver, so
`I + 0x48` is the control. These fields drive the wait.

| Field | Meaning |
|---|---|
| `I+0x134` | Pending wait counter. Slot 4 returns it |
| `I+0x130` | Outstanding wait weight |
| `I+0x12C` | Row index of the last row that carries a wait weight, or -1 |
| `I+0x128` | Row bound that the control subtracts the page row count from to get its start row. The release routine sets it to -1 when no wait remains and otherwise to `I+0x140` plus 1 (`0x00590E35..0x00590E51`) |
| `I+0x140` | Current row |
| `I+0x138` | Row count of the text area |
| `LogAttribute+0x4C` | Wait weight of a row |
| `LogAttribute+0x90` | Page-break flag of a row |

The names are INFERRED from the arithmetic below. They are not recovered
symbols.

OBSERVED. Update slot 2 (`0x00594A20`) arms the wait when the control's
`IsEnableKeyWait` bit is set (`0x00595070..0x00595079`). For each appended row
it adds the row's `LogAttribute+0x4C` to `I+0x134`. When that weight is nonzero
it stores the row in `I+0x12C` and adds the weight to `I+0x130`
(`0x0059509B..0x005950CA`). When the weight is zero, the row is not the last row
and its index equals the page boundary, it sets the page-break flag
`LogAttribute+0x90` and adds one to `I+0x134` (`0x005950CC..0x005950EB`). The
page boundary advances by the page row count each time. A message that is
longer than the visible rows therefore stops after each full page. The writer
of `LogAttribute+0x4C`, and so the text token that gives a row a wait weight,
was not traced and is UNKNOWN.

OBSERVED. While `I+0x134` is nonzero the update does not append further rows.
The append branch of slot 2, around `0x00594F40`, is entered only when
`I+0x134` is zero.

OBSERVED. The `IsDisplayLastOne` bit (`0x00594EAC`) selects a replace
behavior. When new rows are pending, no wait is pending and no timer is armed,
the update sums the heights of the rows already shown, removes that span
through document slot 6, and then appends the new rows. This is INFERRED to
show only the newest message.

OBSERVED. The flag-1 suffix at dispatcher `+0x177B0`, which the
[pool feed contract](logcontrol-pool-feed.md#fifo-flags-publication-and-lifetime)
appends to message text, is default-constructed by the container constructor at
`0x004DC1F1` and destroyed at `0x004DBC96`. The only other reference by
displacement is the read in `0x004D6D90` at `0x004D6DCB`. A scan of `.text` for
the displacement found no store. The suffix is therefore INFERRED to be empty,
so the wait that holds a message comes from its text and not from this suffix.
A store through another displacement or a pointer is not excluded.

## Key release and Wait0

OBSERVED. Slot 3 (`0x00590BB0`) returns 0 at once when `I+0x134` is zero
(`0x00590BFA`, branch to `0x00590C2A`), without writing the wait fields.
Otherwise it scans from row `I+0x140` for the first row with a nonzero `+0x4C`
or a set `+0x90`.

OBSERVED. Two further exits wipe the wait state without a release. When the
current row `I+0x140` is not below the row count `I+0x138`, the routine stores
0 in `I+0x130`, -1 in `I+0x12C` and 0 in `I+0x134` and returns 0
(`0x00590C12..0x00590C2C`). When a row object lookup in the scan returns null
(`0x00590C7E`, `0x00590CBD`), it takes the same stores at `0x00590DA3` and
returns 0. Neither exit raises
`LogControlWait0` or `LogControl.PressedKey`, so the wait counters can be
cleared without releasing a parked message. Which row states reach these exits
at run time was not traced.

1. A nonzero `+0x4C` subtracts that weight from `I+0x130` and `I+0x134`, and
   clears `I+0x12C` when `I+0x130` reaches zero (`0x00590CC3..0x00590CE1`).
2. A set `+0x90` subtracts one from `I+0x134` and clears the flag
   (`0x00590D2F..0x00590D3A`).
3. The scan stops after the first row that released anything, and the routine
   re-arms the next page-break flag from the following rows.

OBSERVED. After the scan the routine raises two commands only when the
released wait weight is nonzero and `I+0x130` is zero
(`0x00590E4C..0x00590E5D`). It executes `LogControlWait0` with the container
plus `0xB4` as target (`0x00590E9A`) and `LogControl.PressedKey` with the
control plus `0xB4` as target (`0x00590EEC`). A release that consumed only a
page break therefore raises neither command. It returns 1 whenever it
consumed a press.

OBSERVED. Update slot 2 raises the same two commands in two other places.
When the new rows are taller than the area (`virtual +0x1C`) and `I+0x12C` is
not -1 (`0x00594AE3`), it raises Wait0 at `0x00594B1E` and `PressedKey` at
`0x00594B6D`, then clears `I+0x12C`, `I+0x130` and `I+0x134`
(`0x00594B8A`). When the rows fit and `I+0x12C` is below the new row count, it
raises them at `0x00594C86` and `0x00594CD3` and subtracts one from `I+0x134`.
These are INFERRED to auto-release a wait when later rows push the waiting
row out. Row heights come from document slot 11 (`0x00591950`), which forwards to the
text area. The text area side was not traced.

OBSERVED. The keypress route, in order, is `RaptureCommands.LogControlPressKey`
or `PressMouse`, container fan-out, `LogControl.PressKey`, the press routine
`0x00591180`, release slot `0x00590BB0`, `TextChanged` through `0x005910F0`,
and, when the last wait weight is gone, `LogControlWait0` and
`LogControl.PressedKey`.

## Release of the message yield

OBSERVED. `_appendMessagePool` at `0x006ECED0` clears `DesktopWidget +0x79`
before it requests the yield, and checker `0x00713A20` completes when that byte
is nonzero ([message-wait contract](logcontrol-message-wait.md)). The container
callback above stores `+0x79 = 1` when it runs.

INFERRED. A `LogControlWait0` raised by the last wait weight of a displayed
message resumes the parked Lua thread. The inference needs the callback to run,
which depends on the dword at event `+8` being nonzero (see the container
fan-out). It also needs the callback's target to be the receiver of
`_appendMessagePool`, which is INFERRED above. The actual resume time is not
observed.

OBSERVED. The Lua `DesktopWidget.setDesktopModeDetail` ends by calling
`sendDesktopCommand("RaptureCommands.LogControlPressKeyCancel")`
(`widget/desktopwidget_connector.lua:8998`). A desktop mode change that
reaches the end of that method therefore runs the cancel fan-out, which
repeats the press routine for each pending wait and zeroes the counters. When
the pending waits include the wait weight of a displayed message, its release
raises Wait0, so the mode change is INFERRED to release a parked message, under
the same condition on the dword at event `+8`. The Lua
`NpcSayWidget.cancelLogWait` and `LogWidget.cancelLogWait` send the same
command to their own control by name and have no caller in the shipped
scripts.

## Boolean property storage

OBSERVED. The property setter `0x00591620` dispatches on the property id.
`Talker` is id 11 at control `+0x19D8`, `LastCategory` id 12 at `+0x19E0`,
`IsEnableKeyWait` id 13 at `+0x19E2` and `IsDisplayLastOne` id 14 at
`+0x19E3`. The boolean setter `0x0093E220` reads bit 5 of the byte, so the
tests `& 0x20` at `0x00595073` and `0x00594EAC` read those two properties.

## Events raised

OBSERVED. `0x005910F0` raises the routed event at `0x0135DBEC`, named
`TextChanged` by the initializer `0x00F253A0`, with the control as source, and
then calls `0x00590F40` to raise `LogChanged` (`0x0133C334`, initializer
`0x00F0FAC0`). Its callers are the press routine, the per-frame update
`0x005954D0` and three other sites in `0x00594068`, `0x0059419A` and
`0x00594972`. The per-frame update calls it when interface slot 1 of the
document (`0x00593950`, which stores the module) or update slot 2 reports a
change.

## Open items

- The writer of `LogAttribute+0x4C`, and so the text token that arms a wait.
- The writer of the dword at event `+8` that gates the Wait0 callback, and so
  whether the raise in slot 3 reaches the callback.
- Which row states reach the two wait-wiping exits of slot 3.
- The source of the page row count at control `+0x179C` and the meaning of the
  press serial at `0x0132FB8C`.
- The body of document slot 11 and the exact definition of the area height at
  slot 7.
- How `LinesPerSec` paces appended rows. The per-frame update reads property
  `+0x1964`, and the timer arithmetic was not traced.
- Whether a press that is not consumed by any control (no wait pending) is
  passed on to another binding for the same key.
- The identity of the object embedded at control `+0x1B10`.

## Reproduction

Use the identified executable in Ghidra or any x86-32 disassembler and read the
cited ranges. Check the command initializers, the container handler compares,
the registry vector offset, the LogControl handler, the press routine, slot 3
and slot 2 around the cited addresses, and the property setter jump table at
`0x00591774`. Run `python tools/verify_npcsay_log_wait.py --exe <ffxivgame.exe>`
to repeat the listing, literal, caller and table checks. Export the three
`key_config` sheets with `xivl extract` and compare the cited rows. No
instruction body, byte dump or decompiled body is needed in a published
artifact.

## Consumer notes for bahamut-client

A client may implement these parts of the contract.

- Hold a message in a key-wait `LogControl` only after a wait weight or a page
  break has been armed, and let one press release one wait. A press with
  nothing pending is not consumed and does nothing.
- Raise Wait0, and release the parked `say` yield, only when the last wait
  weight is released, not on a page-break release. Releasing the yield is
  INFERRED, and the wait state can also be wiped with no release when the
  current row is past the row count or a row lookup fails.
- Bind `Enter`, the pad `RRight` button and a left click to the press
  commands as `key_config` does, and broadcast them to every registered
  control. The same press also operates a visible ask list.
- Treat the desktop mode change as a release: the cancel fan-out flushes every
  pending wait.
- Play the template's `PressedKey` sound when Wait0 is raised, as recorded in
  the NpcSay window chain.
- Show the wait cursor part while a wait is pending and hide it when none
  remains. That the part is the wait cursor is INFERRED.

These stay CALIBRATION until traced: the gate on the Wait0 callback, the token
that arms a wait, the page row
count, the `LinesPerSec` pacing, the auto-release timing in update slot 2, and
the press serial. A client that models only the last message and a single wait
per message stays within the observed behavior for one-line speech.
