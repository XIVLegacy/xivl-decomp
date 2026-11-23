# Sqwt scroll and drag events

The [mouse preview producer](sqwt-mouse-preview-producer.md) arms a drag when
`DragDrop.Id` and `DragDrop.SourceCategory` are nonzero. This page records
what the same executable declares for the events that follow the arming: the
ScrollViewer handler for scroll-bar events, the thumb drag event names and the
drag-and-drop routed events. It records no consumer of a drop.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Ghidra 12.1.4 through ghidra-cli decompilation of
`0x009701C0`, cross-reference queries, and PE section reads of the literals.
Reproduce by querying references to each address below and reading the NUL
terminated ASCII at it.

## ScrollViewer scroll-event handler

The `ScrollViewer` constructor `0x00973190` pushes the address `0x009701C0`
at `0x00973429`, with the viewer as the owner, and calls `0x00974180` on the
embedded member at `+0xB30` (`0x0097342F..0x00973440`). STATIC. The handler is
therefore the callback of that member. This page does not identify the member
type.

`0x009701C0(this, ?, event)` compares the dword at `event+0x08` with the dword
at `this+0xC74`, the horizontal bar part recorded in the
[ScrollViewer parts](../resource/sqwt-tab-scroll-text-parts.md). It then
switches on the byte at `event+0x20`. Every path ends by writing 1 to the byte
at `event+0x19`, including the values that call nothing. STATIC.

| Event byte | Sender is the horizontal bar | Any other sender |
| ---: | --- | --- |
| 1 | no call | virtual `+0x140` |
| 2 | virtual `+0x134` | virtual `+0x13C` |
| 3 | virtual `+0x138` | virtual `+0x130` |
| 4 | no call | virtual `+0x144` |
| 5 | virtual `+0x124` | virtual `+0x12C` |
| 6 | virtual `+0x128` | virtual `+0x120` |
| 7 and 8 | virtual `+0x14C` with the float at `event+0x1C` | virtual `+0x148` with the float at `event+0x1C` |

The virtuals are slots of the ScrollViewer itself (`this` is the receiver).
Which scroll action each event byte stands for, and what each slot does, was
not traced. The two bytes 7 and 8 are the only values that pass a payload.
The producer of the event byte is not traced here.

## Thumb drag event names

| Literal | Address | Initializer reference |
| --- | --- | --- |
| `IsDragging` | `0x010846C4` | `0x00F2AC80` |
| `DragStarted` | `0x010846D0` | |
| `DragDelta` | `0x010846DC` | |
| `DragCompleted` | `0x010846E8` | `0x00F2AD70` |
| `.?AV?$DragDeltaEventHandler@VScrollBar@Primitives@Controls@Sqwt@@` | `0x012E6290` | |
| `.?AUDragDeltaEventArgs@Primitives@Controls@Sqwt@@` | `0x012EA448` | |

STATIC for the literals and RTTI strings. The handler type is instantiated
for `ScrollBar`, so the delta event is declared on the bar's sender type. The
event routing and the handler that consumes `DragDelta` were not traced.

## Drag-and-drop routed events

| Literal | Address |
| --- | --- |
| `DragEnter` | `0x0107C7B4` |
| `DragLeave` | `0x0107C7C0` |
| `DragOver` | `0x0107C7CC` |
| `DragDrop.DestCategory` | `0x01078538` and `0x0106F42C` |
| `.?AUDragEventArgs@Sqwt@@` | `0x012E7E9C` |
| `UICommands.Dropped` | `0x00FC9048` |

`DragEnter` has one data reference, at `0x00F28F90`. `UICommands.Dropped` has
one data reference, `0x00F1451E`, a push inside the static initializer
`0x00F141A0` that builds a command name string. STATIC. None of the 18 names
starting `processUICommand` in the `xivl-client-scripts` registry
(`lua/registry.json`) contains Drop or Drag, so the script callback for the
command and the client-to-server message a drop sends were not found.
The descriptors of `DragDrop.Id`, `DragDrop.SourceCategory` and
`DragDrop.DestCategory` are in the
[qualified property lookup](../resource/sqwt-qualified-property-lookup.md) and
the `SkinDroppable` condition is declared without a skin in the style
resources.

## Open

- The scroll action behind each event byte of `0x009701C0` and its producer.
- The consumer of `DragDelta`, `DragCompleted` and `UICommands.Dropped`.
- Whether `DragEnter`, `DragLeave` and `DragOver` have any subscriber in the
  shipped forms.
- No live retail session was observed.
