# Sqwt desktop relative source

`{Binding RelativeSource={RelativeSource Desktop}}` writes the dword at
`0x01356E10` into the binding source slot. That dword is a singleton pointer to
the `Sqwt::DesktopWindow` object: its constructor stores `this` there and its
destructor clears it, and the cross-reference query lists no other write.

This page records the writers of that dword and what they establish. It does not
establish how a binding with a nonzero slot and no element name attaches, what
an `ExecuteCommandAction` does with the desktop as its command target, or which
of the dword's readers matter to a form.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` cross-reference query of the dword, disassembly and decompilation as a lead |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and
SHA-256 were checked before the observations. Every instruction-level claim
was read from the disassembly. This was static analysis, without client
execution.

Evidence labels used below:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

## The arm

The `RelativeSource` arm of the `{Binding` pre-parser `0x00911890` takes the
token `Desktop` (`0x0106837C`) and writes the dword at `0x01356E10` into the
source slot, at `0x00911C5A` then `0x00911C60..0x00911C64`
([Sqwt binding source arms](sqwt-binding-source-arms.md#pre-parser-arms)).
STATIC.

## Writers of the dword

A cross-reference query of `0x01356E10` returns 114 references. Two are writes.

| Function | Write | Role |
|---|---|---|
| `0x0091B020` | `MOV dword ptr [0x01356E10],ESI` at `0x0091B2F3` | Constructor. `ESI` is the incoming `ECX` (`MOV ESI,ECX` at `0x0091B04A`), the object under construction. The function stores the `Sqwt::DesktopWindow` vtable at object `+0`, `+0xB4` and `+0x194` before the write. The dword at `0x013597F8` receives the same value at `0x0091B2F9`. |
| `0x0091AE80` | `MOV dword ptr [0x01356E10],EBX` at `0x0091AEFD` | Destructor. `EBX` is zero from `XOR EBX,EBX` at `0x0091AEFB`. The function stores the `Sqwt::DesktopWindow` vtable at the same three offsets on entry and clears `0x013597F8` at `0x0091AF03`. |

The constructor has one caller, `0x004DBF40` (`0x004DBF6D`). STATIC for all rows.

The other 112 references read the dword. They are not traced here, so UNKNOWN
is the state of what each reader does with the pointer.

## What follows

INFERRED. A `Desktop` source names the one live `DesktopWindow` object, and
names nothing while the window does not exist. The join is the two writers
above and the absence of a third listed write, which makes the dword either the
object or zero. An indirect write through a computed pointer is not excluded by
the query. A binding that names it is not an element of the form that holds it, so it
cannot resolve by an element name or a template part.

## Surviving gaps

- The attach step for a binding whose slot is nonzero and which has no
  `ElementName`, which the pathless `CommandTarget` of an `ExecuteCommandAction`
  would need.
- The route of a concrete `ExecuteCommandAction` target attribute into the
  generic setter `0x00912850` (the same gap as the binding source arms page).
- What a command raised on the desktop reaches: the `common/Desktop.tpl`
  `DesktopWindow` template holds sound triggers only, and the installed
  `RaptureCommands.ShowHelp` and `RaptureCommands.CloseHelp` commands are named by
  `widget/PopupHelpWidget.tpl` alone.
