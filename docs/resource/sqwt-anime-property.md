# Sqwt Anime property

A widget script that calls `_setProperty(part, "Anime", value)` does not set a
property named Anime. The setter renames it to `UIElement.Execute` and rewrites
the value `Show` to `ShowInactive`. The Execute property holds a string. A new
string that differs from the stored one makes a change handler look up the
command `ApplicationCommands.` plus the string and execute it. A `Window`
handles the commands that matter. `Show` therefore ends as Visibility Visible
plus a `Shown` event, and `Hide` ends as a `Hiding` event plus Visibility
Hidden. This page records the path, the values it accepts, the two gates that
look able to block it, and how a plain Visibility write differs.

The machine-checked form of every STATIC fact is
[`sqwt_anime_property.json`](../../config/sqwt_anime_property.json), checked by
`tools/verify_sqwt_anime_property.py` against the pinned bytes. The reference
model of the rule is in that tool. The command layer under this page is on the
[CommandParameter consumer](sqwt-command-parameter-consumer.md) page, the record
layout is on the [CommandParameter text grammar](sqwt-command-parameter-grammar.md)
page, and the route from a Lua string to the setter is shared with
[SQTP-03](sqwt-plain-text-reference-lookup.md).

## Verdict

- `Anime` is matched by name id, not by spelling. The id is the CRC-32 of the
  upper-cased bytes, so `anime` and `ANIME` match. The setter swaps the key for
  `UIElement.Execute` and turns the value `Show`, exactly and case sensitively,
  into `ShowInactive`. Every other value passes unchanged.
- The Execute property is a latch. The store compares the new string with the
  stored one and does nothing when they are equal. A second `Show` with no other
  Anime value between is a no-op. The store can also refuse a write before the
  compare, in cell states this page does not name.
- A changed value is looked up as a registered command. 23 names exist. Unknown
  values do nothing. The `Window` Executed handler acts on nine of them.
- `ShowInactive` sets Visibility to Visible and raises `Shown` every time.
  `Show` is unreachable from Anime because of the rewrite. `Hide` does nothing
  unless the window allows hiding. When it does, `Hide` raises `Hiding` and sets
  Visibility to Hidden unless a handler cancels it.
- The receiver byte at `+0x9C` that gates the call is set to 1 by the
  constructor and no other store is found, so the gate passes.
- The global at `0x0135708C` that skips the `Shown` raise for a window whose
  name contains `Window_LogWidget` has no writer in the executable and starts
  as zero. The skip is read as dead, INFERRED because an indirect write is not
  excluded.
- A `_setProperty` of Visibility stores the byte and raises neither `Shown`
  nor `Hiding`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All native addresses are VA.

The producing tool is Python `capstone` 5.0.7 disassembly of the pinned image.
Each STATIC row names a signature, string, table, range hash, event initializer
or reference-count check in the manifest that the verifier compares with the
pinned bytes. Signature rows are short. Longer spans are SHA-256 range hashes.
The Sqwt string helpers are named by address: `0x00445210` returns the data
pointer, `0x00445D20` compares two strings, `0x00445D60` and `0x00445E00` test a
string against a C string for equality and difference, `0x00445DB0` tests two
strings for difference, `0x004489C0` assigns a C string and `0x00447200` copies
a string. The class names come from the tracked RTTI catalog,
`config/ffxivgame.rtti.json`.

Labels follow the evidence policy. STATIC means read from instructions or data
of the pinned executable. INFERRED means concluded from a STATIC read without
tracing the use. UNKNOWN means not read. CALIBRATION marks a value that needs
an owner ruling.

## Route to the setter

`xivl-client-structs:docs/widget-native-call-contract.md`, Property access,
owns the Lua side of `_setProperty`. The tail of the route is shared with
[SQTP-03](sqwt-plain-text-reference-lookup.md).

| Step | Address | Effect |
|---|---|---|
| Name routing | `0x0075B810`, `0x0075B840` | Resolve the form object and call the builder. |
| Builder | `0x005354D0` | Takes the request id from the form `+0x98` when the leading name is empty and from `0x005353E0` otherwise. A negative id ends the call. Builds a command object with `0x0053BC30` and passes it to the value's virtual `+4`. |
| Command object | `0x00FA0BAC` | `Application::Main::Element::SetPropertyProcessor`, seven slots. The object keeps the form, the id and two names at `+0x04`, `+0x08`, `+0x0C` and `+0x10`. |
| Setter | `0x00537DE0` | Every typed slot below ends in this call. |

STATIC. Slot 0 is `0x0053BD00`. Slots 1 to 6 format their value into a Sqwt
string and call the setter with the stored fields and that string. A Lua string
takes slot 3 and reaches the setter as given.

| Slot | Address | Value | Text |
|---|---|---|---|
| 1 | 0x005425B0 | int | %d |
| 2 | 0x00542680 | bool | True or False |
| 3 | 0x00542710 | string | as given |
| 4 | 0x00542730 | one float | %f |
| 5 | 0x00542800 | two floats | %f,%f |
| 6 | 0x005428F0 | four floats | %f,%f,%f,%f |

## Setter

The receiver is a `FormElement` (vtable `0x00FA1200`). The setter takes four
stack arguments and pops them: `A1` the part name, `A2` the property name, `A3`
the value string and `A4` the request id. STATIC.

| Step | Address | Effect |
|---|---|---|
| Record and part | `0x00537E38..0x00537E5D` | `0x00535690` loads the form record of `A4`. When `A4` is 0 and the record field `+0x2C` is 0 the setter returns. `0x00537950(A1, A4)` returns the part and a null part returns. The meaning of `+0x2C` is UNKNOWN. |
| Name match | `0x00537E95..0x00537EC7` | Interns `A2` through `0x009108E0` and compares its id with the id of the static name `Anime` (object `0x01336B1C`, id field `0x01336B24`, built once from `0x00FA06E8`). |
| Id | `0x009107A0` | Upper-cases each byte through `0x009DB9ED` and folds it into a CRC-32 with initial value all ones, polynomial `0xEDB88320` and a final complement. |
| Key swap | `0x00537EC7..0x00537F12` | On a match the property key becomes the static name `UIElement.Execute` (object `0x01336B08`, built once from `0x00FA06F0`). Otherwise it stays the interned `A2`. |
| Rewrite | `0x00537F13..0x00537F36` | When `A3` equals `Show` (C string `0x00FA0704`) the value copy is assigned `ShowInactive` (`0x00FA070C`). The test reads `A3` byte for byte. |
| Gate | `0x00537F3B..0x00537F53` | When the receiver byte `+0x9C` is not 1 and the value copy equals `ShowInactive` (C string `0x00FA071C`) the setter returns. The gate sits after the Anime branch and tests every property. |
| Engine | `0x00537F55..0x00537F68` | Calls the string property engine `0x00912850` on the part with the key, the value copy and flag 0. |

STATIC. Only the Anime match uses the key swap and the rewrite. Any other
property reaches the gate with its own key and the original value.

## Receiver flag

The `FormElement` constructor `0x0053AF60` stores 1 into the byte at `+0x9C`
(`0x0053AFD9`). The only read is the gate at `0x00537F3B`. A scan of the
disassembly for stores to `+0x9C` finds the constructor store and stores that
belong to other classes, so the byte stays 1 and the gate never blocks.
STATIC for the constructor store and the read. INFERRED for the absence of other
stores, because an indirect store is not excluded.

## Execute property

| Item | Value |
|---|---|
| Name object | `0x0135FF80`, built at `0x00F275C0` from `0x010783B4`. |
| Registration | `0x00991FDB` with info `0x0135FFA4`. |
| Info | Table `0x01078610`, getter `0x0095E690`, setter `0x0095F6B0`, both stored at `0x00F275F6`. |
| Value | A Sqwt string, empty by default. |

The setter `0x0095F6B0` returns at once when its first stack argument is
nonzero. Otherwise it calls the wrapper `0x0095E830` (`ret 0x1c`) on the cell at
`ecx + 0x24` with the change thunk `0x0095D890`. The wrapper tests mask `0x40`
of the cell flag byte at `+4`. When it is set the wrapper resolves the value
through `0x00916310`, the text-reference resolver also used by
[SQTP-03](sqwt-plain-text-reference-lookup.md), and a result equal to the
sentinel `0x01356DA8` sets mask `0x10` and returns without storing. Otherwise,
and when `0x40` is clear, it calls the string store `0x0095E500`. The meanings
of the masks `0x01`, `0x02` and `0x40` are UNKNOWN. STATIC.

The store `0x0095E500` first applies a precedence exit (`0x0095E507..0x0095E526`)
keyed on its fifth argument, the setter's third argument, and on masks `0x01` and
`0x02`. It returns 0 and stores nothing when mask `0x02` is set and the argument
is 0, or when mask `0x02` is clear, the argument is 1 and mask `0x01` is clear.
When mask `0x02` is clear and the argument is 0 it clears mask `0x01` and goes
on. What the engine call `0x00912850` passes as that argument is UNKNOWN, so a
cell state that refuses an Anime write is not excluded. Past the exit the store
compares the stored string with the new one through `0x00445DB0`. When they
differ it sets flag `0x04`, assigns the new string and, with the notify argument
1, raises the change through `0x0095DCE0`. When they are equal it does nothing.
STATIC.

The thunk jumps to virtual `+0x18` of the owner at `+0x2C`. The control table
on the [FontEdge bridge](sqwt-font-edge-property-bridge.md) page carries
`0x0092DF20` at that slot, and it calls the change handler `0x0093BD80` at
`0x0092DF3C`. STATIC for the thunk, that entry and the call. That the owner of
an Anime part uses this table entry is INFERRED. The join from the engine call
`0x00912850` to this setter is INFERRED. It rests on the registration and on
the descriptor test below, and the property lookup `0x00910F60` is not traced.

The change handler tests the event descriptor against `0x0135FF80`
(`0x0093BFE2`) and reads the new value. It builds `ApplicationCommands.` (string
`0x0106C200`) plus the value through `0x0044AB90`, looks the name up through
`0x00928280` and, when found, builds a parameter record through `0x004DDCF0` with
argument 0. It then calls the command's virtual `+8` with the record, the target
`part + 0xB4` and 0 (`0x0093C0BE`). STATIC. The record has kind 1, an empty
string and integer 0 equal to 0, per the
[consumer page](sqwt-command-parameter-consumer.md#other-consumers-of-the-record).

## Commands

The registry is the list at `0x01358728`. `0x00928280` walks it and compares each
command's name at `+0x24` with the wanted name through `0x00445D20`, which tests
length and bytes, so the match is case sensitive. The 23 `ApplicationCommands.`
commands are constructed at start-up. STATIC.

| Command | Object | Name |
|---|---|---|
| Show | 0x01357648 | 0x0106A37C |
| ShowInactive | 0x013576E8 | 0x0106A398 |
| Hide | 0x01357788 | 0x0106A3BC |
| Close | 0x01357828 | 0x0106A3D8 |
| Copy | 0x013578C8 | 0x0106A3F4 |
| MoveFocus | 0x01357968 | 0x0106A410 |
| MoveWindow | 0x01357A08 | 0x0106A430 |
| ResizeWindow | 0x01357AA8 | 0x0106A450 |
| DestroyWindow | 0x01357B48 | 0x0106A474 |
| InvisibleWindow | 0x01357BE8 | 0x0106A498 |
| Operate | 0x01357C88 | 0x0106A4BC |
| OperateRepeat | 0x01357D28 | 0x0106A4D8 |
| NotMovedFocus | 0x01357DC8 | 0x0106A4FC |
| SetProperty | 0x01357E68 | 0x0106A520 |
| GetProperty | 0x01357F08 | 0x0106A540 |
| ShowWindow | 0x01357FA8 | 0x0106A560 |
| Focus | 0x01358048 | 0x0106A580 |
| ImeOff | 0x013580E8 | 0x0106A59C |
| ShowHelp | 0x01358188 | 0x0106A5B8 |
| CloseHelp | 0x013582C8 | 0x0106A5E8 |
| MoveHelp | 0x01358368 | 0x0106A608 |
| FocusOrderToBottom | 0x01358408 | 0x0106A628 |
| Null | 0x01358688 | 0x0106A688 |

The command class is `Sqwt::Input::RoutedCommand`. Its slot 2, `0x00927FA0`,
raises `PreviewExecuted` (id `0x29`, tunnelling) at the target through the event
constructor `0x00929100`. It raises `Executed` (id `0x2A`, bubbling) the same way
only when `PreviewExecuted` did not come back handled (`0x00928099`,
`0x009280AA`). The
constructor keeps a pointer to the handled byte at `+0x10`, the byte at `+0x19`,
the command at `+0x1C`, the record text at `+0x28` and the record integers from
`+0x7C`. STATIC. The other commands in the table have handlers on other classes,
or none, and this page does not decode them.

## Window handler

`Sqwt::Window` has a secondary table at `0x01069F94` for the subobject at
`+0xB4`. Its slot 65, offset `+0x104`, is `0x00926320`. The generic dispatcher
`0x0091D320` maps event id `0x2A` to virtual `+0x104` through the byte table
`0x0091D818` and the jump table `0x0091D760`, as the
[consumer page](sqwt-command-parameter-consumer.md#dispatch-to-the-handler-store)
records. The route runs slot 13 of each element from the target to the root and
stops when the handled byte is set. For a `Window` that slot is `0x00922030`.
For every event id except `0x4F` it calls `0x00935380`, which calls `0x00938BA0`,
which calls `0x0092D060`, which calls `0x00939800` and so the dispatcher. STATIC
for the call edges. The branches inside those functions that could skip the
dispatcher are not decoded, so the delivery is INFERRED as a whole.

The handler calls the base handler `0x0092D1F0`, then returns when the handled
byte is already set. Otherwise it compares the command with the objects below.
Every matched command sets the handled byte. STATIC.

| Command | Object | Compare | Effect |
|---|---|---|---|
| Show | 0x01357648 | 0x0092635E | handled, show routine with arguments 1, 0, 1 |
| ShowInactive | 0x013576E8 | 0x0092638E | handled, show routine with arguments 1, 1, 1, or the log widget path |
| Close | 0x01357828 | 0x0092641C | handled, calls 0x00921B00 |
| MoveWindow | 0x01357A08 | 0x00926446 | handled, integer 0 selects one of four one-unit moves |
| ResizeWindow | 0x01357AA8 | 0x00926554 | handled, integer 0 selects one of four one-unit resizes |
| Hide | 0x01357788 | 0x00926662 | handled, hide routine, or the log widget path |
| DestroyWindow | 0x01357B48 | 0x00926700 | handled, clears the byte pointed to by window +0x430 when set, and destroys |
| InvisibleWindow | 0x01357BE8 | 0x0092675C | handled, clears the byte pointed to by window +0x430 when set, Visibility Hidden |
| ShowWindow | 0x01357FA8 | 0x009267A1 | handled, calls 0x00926250 with the record text |

Integer 0 of an Anime record is 0, so `MoveWindow` and `ResizeWindow` take code
0, which subtracts 1.0 from one window float and applies it. The effects of
`Close`, `DestroyWindow` and `ShowWindow` beyond their calls are UNKNOWN.

## Show and hide routines

The show routine `0x00925AA0` takes three stack arguments, the
bookkeeping argument, `always raise Shown` and `Visible`. STATIC.

1. It reads whether the window is visible through slot `+0x5C` of the `+0xB4`
   table, `0x00939AA0`, and keeps the answer.
2. It notifies the object at `[0x01356E10] +0x194` when that global is set. The
   effect is UNKNOWN.
3. It sets Visibility through `0x0093C2A0` to 2 (Visible) when `Visible` is
   nonzero and to 1 (Hidden) otherwise.
4. It calls `0x009237D0` with the bookkeeping argument. What that call does and
   what the argument means are UNKNOWN. `Show` and `ShowInactive` both pass 1.
5. It reads visibility again. It raises `Shown` (event id `0x47`, static
   `0x013573B0`) when the window is now visible and was not, or when `always
   raise Shown` is 1. `Shown` has this one raise site.

`Show` passes 1, 0, 1 and `ShowInactive` passes 1, 1, 1, so `ShowInactive`
raises `Shown` even when the window was already visible. A counter at window
`+0x43C` brackets the routine.

The hide routine `0x00921F30` acts only when the byte at window `+0x135` has
bit 0 set. It raises `Hiding` (id `0x49`, static `0x013573C8`) through the
window's virtual `+0x134` with the event object, not through a direct routed
raise, so a subclass override takes part. The event object carries a cancel byte
at `+0x1C`. When a handler sets that byte the routine returns. Otherwise it
loads the pointer at window `+0x430`, clears the byte it points to when the
pointer is set, and sets Visibility to 1 (Hidden). `DestroyWindow` and
`InvisibleWindow` clear the same byte through the same pointer, read as `[esi + 0x37C]`
on the `+0xB4` subobject. The `Hide` branch calls `0x0092ABC0` after
either path. STATIC.
The way a handler sets the cancel byte is UNKNOWN.

## Visibility and Anime

The Visibility descriptor is `0x013596BC` (name `0x0106BEB8`) and its byte sits
at window `+0x158`. The word converter `0x00914580` reads `Collapsed` as 0,
`Hidden` as 1, `False` as 0 and any other text as 2. The printer `0x009145F0`
writes the three words back. STATIC. Reading the byte as the Visibility
property is INFERRED from the setter `0x0093C2A0`, which stores through that
descriptor, and from the visibility test below.

| Word | Value |
|---|---|
| Collapsed | 0 |
| Hidden | 1 |
| Visible | 2 |

Slot `+0x5C`, `0x00939AA0`, returns true when the byte is 2 on the window and on
every ancestor up to the root object at `0x013597F8`. STATIC.

The two routes differ as follows.

| Write | Route | Effect |
|---|---|---|
| Anime `Show` | Latch, `ShowInactive`, show routine | Visibility Visible, bookkeeping, `Shown` every time the latch changes. |
| Anime `Hide` | Latch, hide routine | `Hiding`, then Visibility Hidden unless cancelled. |
| Visibility `Visible` | String property engine | The byte becomes 2. No `Shown`, no bookkeeping, no latch. INFERRED. |

The generic route is INFERRED because the converter and the engine join for the
Visibility property are not traced.

## Window_LogWidget global

The `ShowInactive` branch and the `Hide` branch each search the string reached
through `window +0x08`, read as the window name, for `Window_LogWidget` with the
substring search `0x009D6090` (`0x009263AE`, `0x00926682`). That `+0x08` holds
the name is INFERRED. When the string contains it and the byte
at `0x0135708C` equals 1 the branch skips the routine. `ShowInactive` then sets
Visibility to Visible through `0x0093C2A0` without `Shown` and without
bookkeeping. `Hide` sets Hidden without `Hiding`. Both call `0x0091DEC0` first,
which is UNKNOWN. STATIC for the branches.

Exactly two 4-byte absolute references name `0x0135708C`, the two reads above.
The address lies past the file-backed end of `.data` (`0x01324000`) and below its
virtual end (`0x0137C940`), so the byte starts as zero. STATIC for the reference
count and the zero fill. That no writer exists, so the skip is dead, is
INFERRED: a register-relative write is not excluded, and the neighbours
`0x01357080`, `0x01357084` and `0x01357088` each have one absolute store at
`0x00924D88..0x00924D9C`, so the area holds a small cluster of globals.

## Rule for a UI runtime

The twelve lines below follow from the STATIC rows. The model `anime_step` in
`tools/verify_sqwt_anime_property.py` implements them and the manifest examples
pin its output.

1. Input: the part name N, the property name P and the value text V of a _setProperty call on a loaded form. A part that does not resolve ends the call.
2. P selects this rule when the CRC-32 of its upper-cased bytes equals that of Anime. Any other P is not this rule.
3. Let W be ShowInactive when V is exactly Show, byte for byte, and V otherwise.
4. Each part holds one latched Execute string that starts empty. When W equals the latch the call ends with no effect. Otherwise the latch becomes W and the call continues. A cell state not read here can refuse the write before the compare, and then the latch does not change.
5. The command is the one named ApplicationCommands. followed by W, matched byte for byte. When no command has that name the call ends.
6. The command executes on the part with an empty parameter record. PreviewExecuted tunnels first and the call ends when a handler marks it handled. Otherwise Executed bubbles from the part to the nearest Window, whose handler runs the effect below.
7. ShowInactive: set Visibility to Visible, then raise Shown on the window every time.
8. Hide: when the window allows hiding, raise Hiding. When no handler cancels it, set Visibility to Hidden.
9. InvisibleWindow: set Visibility to Hidden and raise nothing.
10. Show: set Visibility to Visible and raise Shown only when the window was not visible before. Anime never produces this command.
11. A _setProperty of Visibility stores the value and raises neither Shown nor Hiding.
12. The receiver byte and the Window_LogWidget global need no state. The first always passes and the second is read as never set.

A recording that models a window root can treat the part as the window. A
runtime that implements the latch drops line 4 only at the cost of raising
`Shown` on every repeated `Show`. The latch is STATIC in the store and INFERRED
in its join to the engine call, and the store can refuse before the compare in
cell states that are UNKNOWN, so line 4 carries the open CALIBRATION question
below. Line 6 follows the early exit at `0x00928099`: a tunnelling handler that
marks `PreviewExecuted` handled suppresses the Window effect.

## Open questions

| Label | Question |
|---|---|
| INFERRED | The owner-qualified name resolution inside 0x00912850 and the property lookup 0x00910F60 are not traced. The join from the engine call to the info setter 0x0095F6B0 rests on the registration at 0x00991FDB and on the descriptor test in the change handler. |
| INFERRED | The chain from the Window element's slot 13, 0x00922030, to the generic dispatcher follows direct call edges through 0x00935380, 0x00938BA0, 0x0092D060 and 0x00939800. The branches inside those functions that could skip the dispatcher are not decoded. |
| UNKNOWN | The effects of 0x009237D0, 0x0091DEC0, 0x0092ABC0, 0x00921B00, 0x00926250, 0x0093CC70 and of the notification sent to the object at [0x01356E10] +0x194 are not decoded. |
| UNKNOWN | How a Hiding handler cancels the hide is not traced. The cancel byte sits in the Hiding event object at +0x1C, set by a handler outside this page. |
| INFERRED | No store to FormElement +0x9C other than the constructor is found by the instruction scan, so the receiver byte is read as constantly 1. An indirect store is not excluded. |
| UNKNOWN | Window slot +0x34, 0x00922030, forwards each Executed event to the object at window +0x3A8 when it is set. That object is not identified. |
| UNKNOWN | The cell flags 0x01, 0x02 and 0x40 tested by 0x0095E830 and 0x0095E500 are not named, and the value the engine call 0x00912850 passes as the info setter's third argument is not traced. The store returns without writing when flag 0x02 is set and that argument is 0, or when flag 0x02 is clear, the argument is 1 and flag 0x01 is clear. The reference branch can end the call with flag 0x10 set and no store. Whether an Anime part reaches those states is not known. |
| INFERRED | No instruction names 0x0135708C except its two reads and its address is zero filled, so the Window_LogWidget skip reads as dead. A register-relative or runtime write is not excluded. The neighbouring globals 0x01357080, 0x01357084 and 0x01357088 each have one absolute store at 0x00924D88..0x00924D9C, so the area holds a small cluster of globals. |
| CALIBRATION | The latch suppresses a second Anime Show that has no other Anime value between, and the store can refuse a write before the latch compare in the cell states above. Applying either changes what a script sees when it calls Show twice. A live walk of the retail client decides whether to apply them. |

## Reproduction bounds

Verify the identified input before PE32 section mapping. Decode at known
instruction boundaries. The ranges below are start-inclusive and end-exclusive.

| Purpose | Instruction or data reads |
|---|---|
| Setter | `0x00537DE0..0x00537FA0` |
| Pickup chain | `0x005354D0..0x0053555F`, `0x0053BC30..0x0053BC9E`, `0x00542710..0x0054272C` |
| Execute property | `0x00F275C0..0x00F27614`, `0x00991FDB..0x00991FF0`, `0x0095D890..0x0095D89A`, `0x0095E500..0x0095E582`, `0x0095E830..0x0095E8C9`, `0x0095F6B0..0x0095F6DA` |
| Change handler | `0x0092DF39..0x0092DF41`, `0x0093BFE2..0x0093C0C0` |
| Commands | `0x00F20F89..0x00F21AA1`, `0x00928280..0x0092832C`, `0x00927FA0..0x0092812B` |
| Window handler | `0x00926320..0x009267CF`, `0x0091D326..0x0091D349`, `0x0091D6BC..0x0091D6C9`, `0x0091D760`, `0x0091D818`, `0x01069F94`, `0x0106A098` |
| Window delivery | `0x00922038..0x00922047`, `0x00935390..0x00935395`, `0x00938BE3..0x00938BEB`, `0x0092D06B..0x0092D075` |
| Routines | `0x00925AA0..0x00925BA9`, `0x00921F30..0x00921FD9`, `0x00939AA0..0x00939ADB` |
| Visibility | `0x00F22900..0x00F22924`, `0x0093C2A0..0x0093C2CE`, `0x00914580..0x00914626` |
| Global | the file bytes of every section for the little-endian address `0x0135708C` |

Keep executable bytes, decoded instruction listings, decompiled bodies,
projects, and working output local. This page retains only identities,
locators, observations, interpretation, and limits.
