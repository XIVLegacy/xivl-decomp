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

Open also checks two retained pointers following the property
branch at `0x00968B1B..0x00968B4A`. Close does not take that branch.
The virtual call's purpose and its effects are unresolved.

The later branch compares argument `+0x08` with handler-relative
`+0x410` at `0x00968BBF`. If equal, it compares the command with
`Selector.SelectIndex` at `0x00968BC7` and takes the tested close update
at `0x00968BDE..0x00968C1A`. The earlier `0x0096ED40` route compares
the same command object at `0x0096EE26` and can call `0x0096EC00`.
The conditional selection semantics and the resulting ListBox event
order require a separate trace.

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
```
