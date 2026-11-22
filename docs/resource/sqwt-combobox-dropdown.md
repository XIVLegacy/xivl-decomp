# Sqwt ComboBox drop-down commands

`ComboBox.OpenDropDown` and `ComboBox.CloseDropDown` request opposite
values of the ComboBox's `IsDropDownOpen` property. A
`Selector.SelectIndex` command from a retained source can also request
closure. The property callback sends `DropDownClosed` on its closed branch.
The named left-button release handlers also request selection and closure
when their source maps to a row. A Popup with a placement target has a
target-relative arrange path. Native key routing and the conditions that
give a Journal list row input ownership remain open.

## Input and method

The input is the 1.23b Windows `ffxivgame.exe`, 15996808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Ghidra 12.1.4 string references identify the command and
property objects. The branch and call claims below come from disassembly.

| Name | String VA | Object VA | Registration reference |
| --- | --- | --- | --- |
| `ComboBox.OpenDropDown` | `0x01070794` | `0x0135B008` | `0x00F24189`, `0x00F2419C` |
| `ComboBox.CloseDropDown` | `0x010707AC` | `0x0135B0B0` | `0x00F24209`, `0x00F2421C` |
| `IsDropDownOpen` | `0x010707C4` | `0x0135AFC0` | `0x00F24260`, `0x00F24265` |
| `DropDownClosed` | `0x01070828` | `0x0135B170` | `0x00F243B0`, `0x00F243BE` |
| `Selector.SelectIndex` | `0x01072624` | `0x0135B530` | `0x00F246A9`, `0x00F246BC` |

## Routed command

STATIC. The handler at `0x00968AA0` calls `0x0096ED40` at
`0x00968AAB` before examining its own commands. At `0x00968AB0`
it reads a byte through the pointer in argument `+0x10`. A nonzero byte
skips the two explicit drop-down commands and goes to the later source
check. The command object at argument `+0x1C` is compared with the Open
object at `0x00968ABF` and the Close object at `0x00968B58`.

Both branches use a byte at object `+0x4A8`. If bit `0x10` is set,
the branch calls `0x009157C0` with the `IsDropDownOpen` descriptor.
Bit `0x02` gates the update. Open sets bit `0x20` when clear
(`0x00968AF7..0x00968B16`). Close clears bit `0x20` when set
(`0x00968B8C..0x00968BAD`). Both changed branches set bit `0x04`
and pass the property and callback `0x009680D0` to `0x0099C680`.
The full set of flag roles is unresolved.

Open checks the retained list first and then the popup at
`0x00968B1B..0x00968B4A`. On the first nonnull result it calls slot
`+0x38` of the subobject at `+0xB4`. Close does not take that branch.
The virtual call's purpose and its effects are unresolved.

The later branch compares argument `+0x08` with the retained
`PART_ListBox` at handler-relative `+0x410` at `0x00968BBF`.
The close path subtracts `0xB4` from the handler base at
`0x00968BD0`, so this field is object-relative `+0x4C4`.
If equal, it compares the command with
`Selector.SelectIndex` at `0x00968BC7` and takes the tested close update
at `0x00968BDE..0x00968C1A`. The earlier `0x0096ED40` route compares
the same command object at `0x0096EE26` before the retained-source
branch.

### Selector payload and events

STATIC. In the earlier route, `0x0096EE2D` requires command argument
kind `1` for the selection call. It takes the dword at argument
`+0x7C` and passes it to `0x0096EC00` at `0x0096EE33..0x0096EE40`.
The match writes argument byte `+0x19` at `0x0096EE45` even when
the kind did not enter the selection call. The
[kind switch](sqwt-command-parameter-consumer.md#kind-switch)
identifies kind `1` as an integer record.

The call at `0x0096EC4E` writes through the `SelectedIndex`
descriptor `0x0135B680` to object `+0x480` with callback
`0x0096F440`. The descriptor registers from the string at
`0x0107263C` at `0x00F24700..0x00F24719`. After the call,
`0x0096EC53` compares the requested index with `-1`. A
different requested index takes the path that constructs
`Selection` (`0x0135B518`) and dispatches it at `0x0096ECA5`.

`SelectionChanged` is a distinct event (`0x0135B5CC` registered at
`0x00F24860..0x00F24886`). The `SelectedIndex` descriptor has
index zero. Its callback branch at `0x0096F489` calls
`0x0096F080` with the stored index when `ItemsPanel` is present.
That routine has a path that dispatches `SelectionChanged` at `0x0096F3E9`.
The index setter `0x00926E80` compares the stored and incoming values at
`0x00926ED7` and skips its write and callback on equality.

## Named left-button release

STATIC. The [paired Desktop release](../input/sqwt-mouse-preview-producer.md#preview-production-and-specialization)
routes `PreviewMouseUp` at `0x0091ABCE` and, when it returns a target,
routes ordinary `MouseUp` at `0x0091ABF2`. Each dispatch can specialize
its event when the mask at argument `+0x40` still equals 1. The preview
specialization is `PreviewMouseLeftButtonUp`, descriptor `0x013593CC`.
The ordinary arm writes `MouseLeftButtonUp`, descriptor `0x0135978C`, at
`0x0091D57F` after its base virtual call. Initializer `0x00F22D30` supplies
id `0x10` and routing strategy 0 for that ordinary descriptor.

| Event | Dispatch call | ComboBox interface cell and target | ListBox interface cell and target |
| --- | --- | --- | --- |
| `PreviewMouseLeftButtonUp` | `0x0091D4D0`, slot 44 | `0x01070B34` -> `0x0096ED00` | `0x0107219C` -> `0x0096ED00` |
| `MouseLeftButtonUp` | `0x0091D5AE..0x0091D5B9`, slot 50 | `0x01070B4C` -> `0x00968A20` | `0x010721B4` -> `0x0091C970` |

The shared preview handler `0x0096ED00` calls `0x0091C970`, then tests
the handled byte through argument `+0x10`. When clear, it passes the
routed source at argument `+0x08` to `0x00960DE0` on the receiving
selector's item accessor at interface `+0x2EC`. A nonnegative result
enters `0x0096EC00` at `0x0096ED34` with that index and flags `0, 1`.
The index belongs to this receiver's lookup. This handler has no call
to forward a ListBox selection to the owning ComboBox.

The ordinary ComboBox handler `0x00968A20` calls `0x0091C970` and runs
its own lookup of the routed source at `0x00968A2E..0x00968A3F`. With a
nonnegative result, the property guards and open bit `0x20` permit the
close write at `0x00968A83..0x00968A8B`. It sets bit `0x04` and invokes
the `IsDropDownOpen` callback through `0x0099C680` at `0x00968A8D`.
This branch does not test whether the preview changed `SelectedIndex`.

The producer order permits a selector to request its index during
preview, followed by the ComboBox requesting closure on ordinary
release. Both requests depend on delivery to their respective handlers
and each source lookup. The Journal row's concrete native event source,
the item-container construction that makes it recognizable to each
receiver, and its effective route through the retained popup remain open.

## Retained template parts

STATIC. The template part setup at `0x009683C0` resolves these
names through `0x00910980` and retains their results on the ComboBox.

| Part | String VA | Retained field | Assigned at |
| --- | --- | --- | --- |
| `PART_ListBox` | `0x01070944` | `+0x4C4` | `0x0096863B` |
| `PART_Popup` | `0x01070988` | `+0x4C8` | `0x00968715` |
| `PART_EditableTextBox` | `0x01070970` | `+0x4CC` | `0x009686F9` |

The list and popup lookup branches explicitly clear their fields when the
result is null (`0x0096862F` and `0x00968708`). A nonnull list supplies
its `ItemsPanel` field at `+0x35C` to the ComboBox's ItemsPanel setter
`0x00963540` at `0x00968641..0x0096864B`. See
[constructed ListBox parts](sqwt-listbox-constructed-parts.md#constructed-members).
The routine also looks up `PART_ListBox_ScrollViewer` on the list at
`0x0096865D` using the name at `0x01070954`.

The same setup pairs `PART_ListBox` at `0x01070934` with
`ComboBox:ListBox:Background` at `0x010708CC`. It passes those
strings to `0x00930BA0` at `0x00968590`.

## Popup bindings

STATIC. The ComboBox type registers `MaxDropDownHeight` at
`0x00F24300` with descriptor `0x0135B1D4`. The internally constructed
popup at `0x00969170` takes the ComboBox supplied by its caller
`0x00969830`. This path sets the popup's `PlacementTarget` to that
owner through `0x009A5650` and descriptor `0x01361514`. It installs
two bindings on the popup through `0x00911ED0`.

| Source on the owner | Destination on the popup | Attach call |
| --- | --- | --- |
| `IsDropDownOpen` (`0x0135AFC0`) | `IsOpen` (`0x0136155C`) | `0x00969404` |
| `MaxDropDownHeight` (`0x0135B1D4`) | `MaxHeight` (`0x013588EC`) | `0x0096943A` |

The target descriptors register at `0x00F28CE0` and `0x00F21EA0`,
respectively.

## Target-relative popup rectangle

STATIC. The Popup constructor initializes `Placement`, descriptor
`0x01361538`, with zero at `0x009A5C49..0x009A5C66`. The property
setter `0x009A56B0` addresses the stored byte at Popup `+0x2C8`.
When `PlacementTarget` at `+0x2B4` is nonnull, the arrange override
`0x009A5230` sums that target's `+0x24` and `+0x28` offsets along
the visual parent chain at `+0xA4` (`0x009A5282..0x009A52BE`).
The rectangle width comes from the target's actual width `+0x1B4`
and its height from the incoming rectangle's `+0x10` word. The
callback adds the two Popup offset terms, scaled by its virtual
`+0xC4` result, to the summed origin. The zero placement branch
then adds the target's actual height `+0x1AC` to Y. The other branch
subtracts the Popup's current actual height instead
(`0x009A52C0..0x009A5310`). The resulting rectangle goes to
`0x009302F0` at `0x009A534B`.

The field identities are in [Visual offsets](sqwt-render-transform.md#arranged-position-in-draw-and-hit-testing)
and [Point containment](sqwt-number-input-button-frames.md#painted-dimensions-and-point-containment).
The target-relative branch contains no screen-edge choice. Upstream
writers of `Placement`, effective clipping and layer order require
separate traces.

### Height limit

STATIC. The `MaxHeight` setter at `0x0092EDF0` targets FrameworkElement
`+0x1CC` through descriptor `0x013588EC`. The arrange core calls
`0x0092FD20` at `0x00930412`. At `0x0092FEFC..0x0092FF1F`
that function takes a non-sentinel maximum from `+0x1CC` and scales
it by virtual slot `+0xC4`. The height branch at
`0x0092FFC8..0x0092FFF4` checks the effective minimum first. When
the candidate is above that minimum it caps the height by the staged
maximum. The result reaches virtual arrange slot `+0xD8` at
`0x00930012`. This traces an arrange bound. The visible row count and
popup scrollbar behavior depend on other controls.

## Installed Journal category declarations

MEASURED. The standard `widget/JournalListWidget.form` binds
`ComboBox_Category_Quest.ItemsSource` to resource
`XmlData_QuestComboBox` at `/QuestComboBox`. That resource names the
maker `QuestComboBox`. The control selects
`ControlTemplate_ComboBox` and `DataTemplate_ComboBoxItem_Quest`.
The selected template in `widget/JournalListWidget.tpl` has a
`PART_Popup` with `PART_ListBox` as its list child. The child
declares neither `ItemsSource` nor `ItemTemplate`. The popup's
`PlacementTarget` binding refers to the templated parent. It binds
`IsOpen` and `MaxHeight` to the same owner properties as the
internal popup path above and supplies no `Placement` override.
The installed `PlacementTarget` and `MaxHeight` strings omit the
outer closing brace of their binding expression. The `MaxHeight`
string names `Mode=OneWay` and the `IsOpen` string is balanced
and names `Mode=TwoWay`.
The script-side row keys and publish call are recorded in
xivl-client-scripts:docs/journal-list-row-writes.md, Quest category rows.

The standard form selects ComboBox style `COM_basis` from
`common/default.style`. Its `ListBox` part points to ListBox style
`LIB_basis_comboList`. The list's `Background` points to Window
style `WIN_basis_comboList`. The Window's normal `Frame` selects
skin `WIN_basis_comboList_frame_N`.

The selected template's DockPanel has a `PART_DropDownButton_Text`
ToggleButton that holds the `PART_Label` TextBlock. At template line 44
its `Text` binds OneWay to `Text` on the templated parent. Under
`TextBlock.InputBindings`, that label declares one
`MouseAction=LeftClick` MouseBinding to `ComboBox.OpenDropDown`
on the templated parent.
The template also declares `ApplicationCommands.Operate` to open
and `SelectionChanged` on `ComboBox` to
`UILuaCommands.SelectionChanged`. The trigger at template line 34 has
`RoutedEvent=SelectionChanged` and `SourceType=ComboBox`. Its action at
line 35 has only the `Command` attribute. It omits `CommandTarget`,
`CommandParameter`, `PropertyName1` and `PropertyName2`.

STATIC. The `ExecuteCommandAction` constructor `0x009B2D00` calls
`0x00954C00` at `0x009B2D2B`. The base constructor passes descriptor
`0x0135A244` and holder `+0x54` to `0x00953B10` at
`0x00954C82..0x00954C99`. That helper stores the shared default record
`0x01361160` at `0x00953B1F`, with kind 0 as in the
[Storyboard default record](sqwt-keyframe-target-binding.md#default-completion-record-and-event-arguments).
When the action retains those defaults, the native command uses kind 0
without an index payload. Its default target is the trigger event
source if that casts to `FrameworkElement`.
[Action substitution and dispatch](sqwt-command-parameter-grammar.md#executecommandaction-text-substitution).

| SQEX input below `client/sqwt` | Bytes | SHA-256 |
| --- | --- | --- |
| `widget/JournalListWidget.form` | 3434 | `c72aeec944ae2072d4189cd0890bba6ca25d414d21393ee5d6eb086e017a939d` |
| `widget/JournalListWidget.tpl` | 8692 | `12f30656cf5b8945ebec43f7c7756bc47a3e67ab8fe9160d09a2f15abb8a616f` |
| `common/default.style` | 353783 | `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` |

### LeftClick gesture

STATIC. The MouseBinding reader `0x00948720` installs
`LeftClick` (string `0x0106D16C`) with value zero in the
primary action table at `0x0094876E..0x00948778`. The parser
at `0x0094883E` writes a matched action to markup `+0x174`.
`0x00947679..0x00947684` passes that byte to `0x0094D870`,
which stores it at MouseGesture `+0x18` at `0x0094D8AE`.

The MouseGesture vtable at `0x0106D7F0` has slot 4
`0x0094DB20`. It calls `0x0094D8E0` at `0x0094DB53`.
On the ordinary branch, the matcher accepts event descriptor
byte `+0x05 == 0x08` at `0x0094D9BD..0x0094D9C2`.
After the common modifier and optional device-result checks,
action zero selects `0x0094D9F8` through table `0x0094DAF8`.
That case requires event byte `+0x24 == 1`. The producer of
this event byte and the state that delivers the event to the
Journal label remain open.

## Property callback

STATIC. `0x009680D0` examines the descriptor index reached from callback
argument `+0x70`. The `IsDropDownOpen` registration writes zero to the
descriptor's index at `0x00F2426F`. For index zero, the callback tests
object byte `+0x4A8` bit `0x20` at `0x00968135`. When clear, it
conditionally invokes the object referenced at `+0x4CC`, constructs
`DropDownClosed` at `0x009681D8` and dispatches it through
`0x0091F1E0` at `0x00968210`. The set branch instead checks the pointer at
`+0x4C8` at `0x0096813E`. Handler registration, repeated requests
and a live popup's focus and key precedence remain unverified.

## Drop-down button style

STATIC. The part setup at `0x009683C0` pairs string `PART_DropDownButton`
at `0x010708B8` with the string at `0x010708E8`. The pair passes to
`0x00930BA0` at `0x00968515`. The two paths in the second string name
`ComboBox:DownButton:UncheckedButton:Frame` and
`DownButton:CheckedButton:Frame`. This records the part names and their
join. The callback that chooses a path during a live open or close
remains unresolved.

## Reproduction

Use the 1.23b program in an analyzed Ghidra project.

```sh
ghidra find string "ComboBox.CloseDropDown"
ghidra x-ref to 0x010707AC
ghidra x-ref to 0x0135B0B0
ghidra disasm 0x00968AA0 -n 150
ghidra disasm 0x009680D0 -n 100
ghidra disasm 0x0096ED40 -n 120
ghidra disasm 0x0096EE20 -n 22
ghidra disasm 0x0096EC26 -n 46
ghidra disasm 0x0096F473 -n 20
ghidra disasm 0x00926ED3 -n 20
ghidra disasm 0x0096F39E -n 20
ghidra disasm 0x0091D4D0 -n 8
ghidra disasm 0x0091D558 -n 36
ghidra disasm 0x00F22D30 -n 9
ghidra memory read 0x01070B34 4
ghidra memory read 0x01070B4C 4
ghidra memory read 0x0107219C 4
ghidra memory read 0x010721B4 4
ghidra disasm 0x0096ED00 -n 25
ghidra disasm 0x00968A20 -n 45
ghidra disasm 0x009B2D25 -n 10
ghidra disasm 0x00954C82 -n 9
ghidra disasm 0x00953B10 -n 12
ghidra disasm 0x009684CC -n 30
ghidra disasm 0x0096853B -n 26
ghidra disasm 0x009685C2 -n 90
ghidra disasm 0x009693B3 -n 42
ghidra disasm 0x009A5650 -n 18
ghidra disasm 0x009A5230 -n 90
ghidra disasm 0x009A5C41 -n 17
ghidra disasm 0x0092EDF0 -n 17
ghidra disasm 0x0092FEFC -n 80
ghidra disasm 0x0094875A -n 72
ghidra disasm 0x00947666 -n 20
ghidra disasm 0x0094D9B6 -n 27
```

For the installed action, verify the template's identity above and decode
its SQEX body with `xivl-tools:tools/blowfish.py`, using the file basename
as the key. Read the trigger and action attributes at lines 34 and 35.
