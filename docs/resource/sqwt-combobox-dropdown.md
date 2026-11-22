# Sqwt ComboBox drop-down commands

`ComboBox.OpenDropDown` and `ComboBox.CloseDropDown` request opposite
values of the ComboBox's `IsDropDownOpen` property. A
`Selector.SelectIndex` command from a retained source can also request
closure. The property callback sends `DropDownClosed` on its closed branch.
These are static command paths. A Popup with a placement target also
has a target-relative arrange path. Native key routing and the conditions
that give a Journal list row input ownership remain open.

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
the same command object at `0x0096EE26` and can call `0x0096EC00`.
The conditional selection semantics and the resulting ListBox event
order require a separate trace.

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
The script-side row keys and publish call are recorded in
xivl-client-scripts:docs/journal-list-row-writes.md, Quest category rows.

| SQEX input below `client/sqwt` | Bytes | SHA-256 |
| --- | --- | --- |
| `widget/JournalListWidget.form` | 3434 | `c72aeec944ae2072d4189cd0890bba6ca25d414d21393ee5d6eb086e017a939d` |
| `widget/JournalListWidget.tpl` | 8692 | `12f30656cf5b8945ebec43f7c7756bc47a3e67ab8fe9160d09a2f15abb8a616f` |

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
ghidra disasm 0x009684CC -n 30
ghidra disasm 0x0096853B -n 26
ghidra disasm 0x009685C2 -n 90
ghidra disasm 0x009693B3 -n 42
ghidra disasm 0x009A5650 -n 18
ghidra disasm 0x009A5230 -n 90
ghidra disasm 0x009A5C41 -n 17
ghidra disasm 0x0092EDF0 -n 17
ghidra disasm 0x0092FEFC -n 80
```
