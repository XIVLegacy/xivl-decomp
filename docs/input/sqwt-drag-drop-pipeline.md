# Sqwt drag and drop pipeline

The [mouse preview producer](sqwt-mouse-preview-producer.md) records how a
Control down arms a drag, and [scroll and drag events](sqwt-scroll-and-drag-events.md)
lists the routed event names. This page records the global drag state, the
move and release paths that raise the routed events and the argument object
they carry.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations come from Ghidra 12.1.4 through ghidra-cli disassembly of
the cited functions, cross-references and PE section reads of the literals.

## Global drag state

Control down `0x009398D0` writes these globals when it arms a drag
(`0x009399F9..0x00939A27`). STATIC.

| Global | Written value |
| ---: | --- |
| `0x0135934C`, `0x01359350` | the pair returned through `0x0067AB40` for the pointer, taken as the grab offset |
| `0x01359338` | first converted `DragDrop.Id` |
| `0x0135933C` | second converted value, the source category |
| `0x01359340` | 0, the target Id |
| `0x01359344` | 0, the current target |
| `0x01359348` | the source element |

The same routine ORs `0x10` into byte `+0xDC` of the source element.

## Move 0x0093A020

Control move calls its base `0x0091C6C0`, then continues only when element
`+0xDC` bit `0x10` is set and `0x0093F060` answers 1 (`0x0093A06A..0x0093A086`).
STATIC.

1. It converts the pointer to the source's local space through `0x00919BB0`,
   subtracts the grab offset (`0x0093A0A7`, `0x0093A0BC`), adds the ancestor
   offsets through `0x0053CA40` and hands the vector to virtual slot `+0xA8` of
   the object `0x0093F880` returns (`0x0093A0CD..0x0093A0E1`). The dragged
   element's visual follows the pointer.
2. It reads the old target Id from `0x01359340` (`0x0093A0E6`), then runs the
   hit test `0x00919E70` with the source category from `0x0135933C`. A result
   equal to the desktop root visual (`[0x01356E10] + 0xB4`) counts as no target
   (`0x0093A110..0x0093A129`).
3. The candidate filter is the virtual method `0x0093E910`, which has one call
   site at `0x0092B7FA` and a vtable slot at `0x0106C404`. It rejects the
   source element itself (`0x0093E96F`), requires both `DragDrop.Id`
   (`0x013603B8`) and `DragDrop.DestCategory` (descriptor `0x01360448`,
   initializer `0x00F27C90`, literal `0x01078538`) to be set, requires the bit
   test of the candidate's category against the source category to be nonzero
   (`0x0093E9E5`) and stores the candidate's Id at `0x01359340`
   (`0x0093EA2D`). A rejected candidate clears its result field
   (`0x0093EA68`).
4. When the picked element differs from `0x01359344`, a nonzero old target
   receives DragLeave first, built with descriptor `0x013617B8`
   (`0x0093A14A`), then a nonzero new target receives DragEnter, built with
   descriptor `0x013617AC` (`0x0093A199`). Each is raised through `0x0091F1E0`.
   The new target is stored at `0x01359344` (`0x0093A1C3`).

The four drag descriptors are initialized at `0x00F28F90` (DragEnter, id
`0x50`, literal `0x0107C7B4`), `0x00F28FC0` (DragLeave, `0x51`, `0x0107C7C0`),
`0x00F28FF0` (DragOver, `0x52`, `0x0107C7CC`) and `0x00F29020` (Drop, `0x53`,
`0x0107C7D8`), each with routing byte 0. The cross-references of the DragOver
descriptor `0x013617C4` are its initializer and a destructor-time data
reference only, so no code raises DragOver. STATIC.

## Release 0x00939E90

Control up calls its base `0x0091C870`, then requires the same two gates as the
move (`0x00939EDA..0x00939EF4`). With a nonzero current target
(`0x00939F07`) it raises DragLeave with descriptor `0x013617B8` and then Drop
with descriptor `0x013617D0` on that target, both with the source, the pointer
position, `0x01359338` and `0x01359340` (`0x00939F3B..0x00939FA5`). It then
clears bit `0x10` of `+0xDC`, zeroes `0x01359338` through `0x01359348`
(`0x00939FBC..0x00939FDD`) and calls slot `+0xAC` of the visual object
(`0x00939FEC`). A release with no current target raises nothing. STATIC.

## DragEventArgs constructor 0x009AC220

The constructor takes the descriptor, the source element, a pointer to the
position, the Id and the target Id and returns through `RET 0x14`. It sets the
vftable `0x0107C7F4` (`0x009AC260`) and stores the source at `+0x1C`, the Id
at `+0x20`, the target Id at `+0x24` and the two position floats at `+0x28`
and `+0x2C`. It formats each Id with `sprintf_s` into an 11-byte buffer using
the literal `0x%x` (`0x0107C7E0` and `0x0107C7E8`), the Id at `+0x30` and the
target Id at `+0x3B` (`0x009AC282`, `0x009AC293`). STATIC.

The method at `0x009AC2B0`, the vtable slot at `0x0107C7F8`, returns the
`+0x30` buffer for a zero argument and the `+0x3B` buffer otherwise. It is the
argument-slot accessor that the
[command parameter consumer](../resource/sqwt-command-parameter-consumer.md)
reaches for the parameter names `%0` and `%1`. That mapping of `%0` to the
Id string and `%1` to the target Id string is INFERRED from the argument
order, because the lookup that calls the accessor was not read here.

## Constructors of the installed carriers

The widget_c Equip forms are named by C++ constructors, not by Lua. The
`EquipWidget` constructor `0x0052A080` pushes `\widget_c\EquipWidget.form`
(`0x00F94EF8`, one data reference at `0x0052A0B0`). The `ActionEquipWidget`
constructor `0x005297E0` pushes `\widget_c\ActionEquipWidget.form`
(`0x00F93424`, one data reference at `0x00529810`). STATIC. The declarations
that arm and receive a drag in those templates are in
`xivl-client-data:docs/sqwt-hover-and-drag-census.md`.

## Boundary

The consumers of the visual slots `+0xA8` and `+0xAC`, any drop highlight, the
inheritance of `DragDrop` values from a template container to the Button
inside it and the script that receives `UICommands.Dropped` were not traced.
No live retail session was observed.

## Reproduction

Disassemble `0x009398D0`, `0x0093A020`, `0x00939E90`, `0x0093E910` and
`0x009AC220` and match the locators. Run `x-ref to` on `0x013617C4` and
expect no code reference. Read the literals at `0x0107C7E0` and `0x0107C7E8`
and expect `0x%x` followed by a zero byte.
