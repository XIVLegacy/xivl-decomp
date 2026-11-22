# Sqwt ComboBox drop-down commands

`ComboBox.OpenDropDown` and `ComboBox.CloseDropDown` request opposite
values of the ComboBox's `IsDropDownOpen` property. A
`Selector.SelectIndex` command from a retained source can also request
closure. The property callback sends `DropDownClosed` on its closed branch.
These are static command paths. Native key routing, popup placement and
the conditions that give a Journal list row input ownership remain open.

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
result is null (`0x0096862F` and `0x00968708`). For the list, a
nonnull result also starts a lookup of its `PART_ListBox_ScrollViewer`
at `0x0096865D` using the name at `0x01070954`.

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
respectively. Placement orientation, screen-edge handling and
the runtime effect of the height cap remain open.

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
ghidra disasm 0x009685C2 -n 90
ghidra disasm 0x009693B3 -n 42
ghidra disasm 0x009A5650 -n 18
```
