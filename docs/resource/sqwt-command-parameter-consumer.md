# Sqwt CommandParameter consumer

A parsed `CommandParameter` record leaves the Sqwt command layer in an
`Executed` routed event (id `0x2A`). The event bubbles from the command target
up the element parents, and each element dispatches it to a handler store. The
form handler is registered in those stores under a shared descriptor with the
unnamed id `0x59`, which the store dispatcher treats as a wildcard that matches
every event. One native handler on the form element then reads the record kind,
picks one of four event parameter classes, and calls the Lua method
`_onUICommandEvent` on the widget script object (or `_onUICommandRequest`, by
subscription form), but only when the script has subscribed to the exact
template, control and command triple. A record with no subscription, an unknown
command name, or a kind with no matching class produces no error and no Lua
call. The delivery chain from the command raise to the Lua call is STATIC. The
gates in front of it are partly INFERRED or UNKNOWN, as the sections below
label them. What stays open is whether every specialised control class
forwards the event to its handler store, see [Surviving gaps](#surviving-gaps).

This page records that path, the routing in front of it, the subscription gate,
and the values a script receives. It builds on the
[CommandParameter text grammar](sqwt-command-parameter-grammar.md), which owns
the record layout and the parser. The
[movement command origin](../input/player-movement-command-origin.md) page
records the same route for a different consumer. This page does not establish
what any individual script does with the values.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | Capstone `5.0.7` x86 32-bit disassembly over a read-only PE32 section mapping, with the `ghidra` CLI xrefs and the `config/ffxivgame.vtable_slots.jsonl` catalog as leads. |

All addresses are VA. Executable size and SHA-256 were checked before the
observations. Every instruction-level claim below was read from the Capstone
disassembly. This was static analysis and did not execute the client.
[sqwt_command_parameter_consumer.json](../../config/sqwt_command_parameter_consumer.json)
holds the addresses, tables, strings and masked byte signatures that
[verify_sqwt_command_parameter_consumer.py](../../tools/verify_sqwt_command_parameter_consumer.py)
re-reads from the executable.

Evidence labels used below:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

## Command objects

`Command` and `CommandParameter` of an `ExecuteCommandAction` come from the
property class
`Sqwt::DependencyPropertyInfo<Sqwt::Input::ICommandSource<Sqwt::TriggerAction>>`.
Its setter `FUN_00955010` switches on the property index. STATIC.

| Case | Property | Descriptor | Stored at |
|---|---|---|---|
| 0 | `Command` | `0x0135A1FC` | action `+0x44` |
| 1 | `CommandTarget` | `0x0135A220` | action `+0x4C` |
| 2 | `CommandParameter` | `0x0135A244` | text at `+0x78`, record holder at `+0x54` |

Case 2 (`0x00955050..0x0095509E`) copies the incoming property text into the
string at action `+0x78` through `0x0099A8D0` and then updates the record holder
through `FUN_00954500`. The text at `+0x78` is therefore the attribute text as
the property system supplied it, which is the format string that
`FUN_009B28C0` reads. That settles the `+0x78` reading that the grammar page
leaves INFERRED. STATIC.

Case 0 resolves the command by name. `0x00928280` searches the global command
list (`0x01358728`) by the name string at command `+0x24`. When the name is
absent, `0x00928F90` allocates a `0x9C` byte `RoutedCommand`
(constructor `0x00928C90`) and registers it in the same list (`0x00954F8C..0x00954FA8`).
So any `Command` attribute text yields a command object. Only a few names have a
static object. The `UILuaCommands` initializers at `0x00F0F29B..0x00F0F51B` and
`0x00F145AF..0x00F14620` register `Operate`, `Cancel`, `WidgetClose`,
`Selection`, `SelectionChanged`, `SelectionChangedOnce`, `PressEnter`,
`Function1` and `MainMenu`. Names such as `Hover` have no static object and
exist only through this create-on-demand path. STATIC.

## Executed event delivery

`Sqwt::Input::RoutedCommand` slot 2 (`0x00927FA0`) builds a `PreviewExecuted`
event (id `0x29`, static `0x0135872C`, initializer `0x00F21C80`), routes it, and
only when no handler set the handled byte builds the `Executed` event (id
`0x2A`, static `0x01358738`, initializer `0x00F21CB0`, which pushes the name
string `Executed`) and routes that. It returns the handled byte of the last
event routed. The event argument constructor `0x00929100` copies the record as
described in the grammar page. STATIC.

| Event argument offset | Use |
|---|---|
| `+0x04` | Routed event descriptor pointer |
| `+0x08` | Source element, `+0x0C` original source, both filled by the route when zero |
| `+0x10` | Pointer to the handled byte |
| `+0x17` | Stop byte, nonzero ends the route traversal |
| `+0x19` | Handled byte |
| `+0x1C`, `+0x20` | Command object, stored twice by the constructor (`89 43 1C`, `89 43 20` at `0x00929100`) |
| `+0x24` | Record kind |
| `+0x28` | Record string object |
| `+0x7C` to `+0x98` | Record integers 0 to 7 |

### Event descriptors

The descriptor constructor `0x0097F6C0` stores a routing strategy byte at `+4`,
the event id at `+5` and the name pointer at `+8`. The manifest carries one
`routedEvent` row for each of the 89 descriptor initializers that a sweep of
`.text` for calls to the constructor found. Each row is re-read from the
initializer bytes by the verifier. The id `0x21` has no initializer. The
strategy values are 0 for events that traverse from the target outward, 1 for
events delivered to the target alone, and 2 for events that traverse from the
root inward. The names bubble, direct and tunnel are INFERRED from the
traversal order below. STATIC for the values.

| Event | Id | Strategy | Static | Initializer |
|---|---|---|---|---|
| `PreviewExecuted` | `0x29` | 2 | `0x0135872C` | `0x00F21C80` |
| `Executed` | `0x2A` | 0 | `0x01358738` | `0x00F21CB0` |
| `LocationChanged` | `0x48` | 0 | `0x013573BC` | `0x00F20EA0` |
| `RenderSizeChanged` | `0x4B` | 0 | `0x013594B4` | `0x00F227B0` |
| unnamed, form handler | `0x59` | 0 | `0x01336A84` | `0x00F0BEE0` |

Ids `0x54` to `0x58` and `0x59` have no name string. Two more unnamed `0x59`
descriptors are built inline in function local statics (`0x0133F8F4` at
`0x0066EE0A`, `0x01341904` at `0x0068DFB2`). The first is registered through
interface slot 9 at `0x0066EE40`. Three descriptors share one id, so a handler
is recognised as a wildcard by its id byte and not by the pointer.

### Target and route

The `ExecuteCommandAction` invocation at `0x009B2B40` picks the target. It is
the `CommandTarget` pointer at action `+0x4C` when set, otherwise the trigger
event source at `+0x08` cast to `Sqwt::FrameworkElement` (type descriptor
`0x01269E8C`) with the dynamic cast helper `0x009DA6CC`. Either result is moved
to the element interface at `+0xB4`, and a failed cast gives a null target. It
calls command slot 2 with the record, the target and the trigger flags. STATIC.
A null target is routed from the global root at `0x01357020` through
`0x0091EE90`, which this page does not follow.

`0x0091F590` calls the route function `0x0091E1C0` with the event, the target
interface and three flag arguments that are all 1. STATIC. The route function
returns when the event descriptor or the target's owner word at `+0x64` is null,
and fills event `+0x0C` and `+0x08` from target `+0x64` and `+0x68` when they
are zero. Strategy 1 calls recipient slot 13 (`+0x34`) of the target and
returns. Any other strategy builds a list. The list starts with the target and
adds each parent through the pointer at interface `+0x60`, stopping before the
root interface (global `0x01356E10` plus `0xB4`) or at a null parent, then
appends the root unless it is the target, and reverses the list once
(`0x00932760`). With all three flags set no element is filtered out. STATIC.

Traversal `0x0091D210` runs slot 13 of each list entry and stops when event byte
`+0x17` is nonzero after a call. Strategy 0 walks from the list end, so the
target runs first and the root last. Strategies 1 and 2 walk from the list
start, so the root runs first. STATIC. An `Executed` event therefore reaches the
target, then each parent, then the root.

### Dispatch to the handler store

Slot 13 of `Sqwt::FrameworkElement` (`0x0092D060`) calls the base slot
`0x00939800`. That function sends three ids to primary virtuals and then calls
the generic dispatcher `0x0091D320` for every id, which indexes the byte map at
`0x0091D818` with the id minus 1 (bound `0x52`) and jumps through the table at
`0x0091D760`. STATIC.

- The `Executed` arm `0x0091D6BC` calls virtual slot 65 (`+0x104`).
- The `PreviewExecuted` arm `0x0091D6AA` calls virtual slot 64 (`+0x100`,
  `0x0091CC10`).
- Ids with no arm of their own, including `0x59`, take the fallback arm
  `0x0091D73A`, which does the same store dispatch as slot 64.

Base slot 65 (`0x0091D8F0`) dispatches when the owner word at interface `+0x64`
is nonnull, event byte `+0x17` is zero and the store pointer at interface
`+0x4C` is nonnull. It calls store vtable slot 1 with the owner as sender and
the event as argument. It then tests the command through `0x0091D870`, which
needs interface flag `+0x70` bit 4 and a match of the command in the per element
list held in the table at `0x01357030`, and sets the handled byte when it finds
one. That list is INFERRED to be the element's command bindings. The handled
byte is set after the store dispatch, so it stops later elements of the route
from running the form handler and not the current one. The `FrameworkElement`
override `0x0092D1F0` calls the base first. The store is a `0x24` byte object with
vtable `0x00FA6788` that `0x0091F620` creates on the first registration, so an
element nobody registered on has no store and delivers nothing. STATIC.

The sender is the owner stored at interface `+0x64` and `+0x68` by the
interface constructor `0x0091E090`. The element constructor calls it with the
element itself (`0x0093C791..0x0093C7A2`). The sender is therefore the element
whose store held the handler. STATIC.

### Handler matching

Store dispatcher `0x00571520` walks the handler vector in registration order.
For each handler it applies these rules. STATIC.

- A handler whose event descriptor at `+0x18` is null is skipped, and so is an
  event with a null descriptor.
- The handler matches when the event descriptor pointer equals the handler
  descriptor, or when the id byte `+5` of the handler descriptor is `0x59`
  (`0x005715D7`). The handler descriptor is the one registered, so a handler
  registered for the `0x59` descriptor matches every event the store dispatches.
- A matching handler runs when its byte `+0x1C` is 1 or when the handled byte
  behind event `+0x10` is zero. The base constructor `0x00542583` clears `+0x1C`,
  and nothing in the form handler path sets it. The form handler therefore never
  runs for an event that is already handled.
- The run is a call of handler vtable slot 2 with `(sender, event)`. Slot 2 of
  the form handler object (vtable `0x00FA106C`) is the thunk `0x0088AB00`. It
  loads this from the sum of the words at `+0x2C` and `+0x20`, jumps to the
  function pointer at `+0x28`, and the callee pops the two stack arguments. For
  the form that is `FUN_00536280` with the form element as this.
- After a run, event byte `+0x17` equal to 1 ends the walk and is copied to the
  event that the store was dispatching before.

That answers how an `Executed` event with id `0x2A` reaches a handler added for
the descriptor at `0x01336A84`. The registration stores that descriptor in the
handler, the descriptor id is `0x59`, and the dispatcher accepts it for any
event. The form handler then branches on the real event id itself. It also runs
for `PreviewExecuted`, where the id branch returns. STATIC.

### Handler registration

The handler object is built once per form at form element `+0xA0` by the
constructor (`0x0053AFAF`, `0x00543AD0`) as a
`Sqwt::RoutedEventHandler<Application::Main::Element::FormElement>`. The
element interface registers it through three vtable cells, shown here for
the base interface vtable `0x0106C2A4`. STATIC.

| Slot | Function | Operation |
|---|---|---|
| 9 | `0x0091F6F0` | Stores the event descriptor at handler `+0x18` and appends the handler to the store vector |
| 10 | `0x0091F6D0` | Same store write, appends through the store itself |
| 11 | `0x0091C5B0` | Thunk to slot 12 |
| 12 | `0x0091F710` | Removes the handler from the store vector when present |

The subscription call at `0x0053ABB3..0x0053ABD0` is therefore a remove
followed by an add, so one handler is present at most once per store. The first
virtual call at `0x0053ABC1` is the removal. STATIC. The same handler object is
registered in many stores, and the store that dispatches supplies the sender.
The five sites that push the `0x01336A84` descriptor are below.

| Push site | Registers | On |
|---|---|---|
| `0x00538938` | form handler | The element named `Window_PopupHelpWidget`, when no form owns the popup global |
| `0x00538AB1` | form handler | The root element, only when the form gate object was created |
| `0x0053A1A5` | a form helper listener with function `0x00533F30` | An element from the form control map |
| `0x0053A9CC` | form handler | The named control of `_setUICommandCondition` |
| `0x0053ABC9` | form handler | The root element for the control name `_widget` |

## Form handler

The handler argument layout, read from the code, is below. The arguments are
the sender and the event argument object of the table above.

| Argument offset | Use |
|---|---|
| `+0x04` | Routed event pointer, whose byte `+5` is the event id |
| `+0x08` | Event source element |
| `+0x19` | Handled flag, set to 1 by the handler gates |
| `+0x20` | Command object |
| `+0x24` | Record kind |
| `+0x28` | Record string object |
| `+0x7C`, `+0x80` | Record integers 0 and 1 |

The handler branches on the event id at `0x0053641C`. Id `0x2A` continues to the
Lua path at `0x005364F4`. Ids `0x48` and `0x4B` take different paths
(`0x00536454`, `0x00536433`) that this page does not follow, apart from the
popup positioning call described below. Any other id returns. STATIC.

### Entry gates

Before the subscription lookup the handler returns, with no Lua call, when any
of these hold. STATIC.

- The byte at `0x01356E14`, returned by `0x00919D30` after `0x0091A1B0`, is 1
  (`0x005362CC..0x005362D8`). The four `.text` instructions that name the byte
  are two reads (`0x00919D30`, `0x00919E20`) and two stores of 1 (`0x00919D60`,
  `0x0091A1F3`), each store followed by a call to `0x0096A940`. So it is a one
  way flag next to the root global. Its role as a teardown flag is INFERRED. A
  client can ignore it until it shuts a root down.
- Argument `+0x04` or `+0x08` is null (`0x005362DE`, `0x005362E9`). The
  dispatcher and the route make both nonnull for any event that reaches a
  store, so this never fires on the delivery path.
- The third dword of the subscription map at form element `+0xF8` (offset
  `+0x100`) is zero (`0x00536535`). That slot is the element count of the
  standard map layout that `0x00535B70` walks, so the gate is a fast exit for
  a form with no subscription. INFERRED for the layout. It adds no condition
  beyond the lookup that follows.
- The first entry of the control map at form element `+0xD4` has a null element
  pointer at `+0x2C` (`0x005364F4..0x00536517`).

### Form gate object

A gate object at form element `+0x27C`, when present and active (byte `+8`),
sees the event before the lookup. For an event that is not `Executed` the
handler calls gate slot 43 (`+0xAC`) with the event id
(`0x00536310..0x00536325`). For `Executed` it calls gate slot 42 (`+0xA8`) with
the word at command `+0x78`, the source, the address of the record and the word
at event `+0x9C` (`0x005363D5..0x005363F5`). A return of 1 sets the handled
flag and returns without a Lua call. A zero return, a missing gate or an
inactive gate continues to the lookup. STATIC.

For the six pad UI commands `UICommands.Operate`, `Cancel`, `Close`,
`Selection`, `SelectionChanged` and `SelectionChangedOnce` (objects from the
`0x9C` stride table at `0x013442E8`, entries 1 to 6) an active gate also gets a
one-shot flag at `+0x1F4` that resets when the frame counter at
`[0x0091A1B0 result + 0x46C]` differs from the stored frame at `+0x1F0`
(`0x0053632A..0x005363C9`). The first such command in a frame sets the flag and
goes on to gate slot 42. A second one in the same frame sets the handled flag
and returns (`0x005363C2..0x005363FB`). The one-shot block runs only when the
command object has a nonzero word at `+0x78` (`0x0053632D`).

The gate exists only for some forms. The function `0x005387F0`, which the
constructor stores as a handler at form element `+0x208`
(`0x0053B101..0x0053B10E`), creates it once, at `0x00538A6F`, through the
registry call `0x00694C70` and the factory `0x008748E0`. The event that runs
that function is not followed. The factory takes the root element name without
its first seven characters (the slice call at `0x00874856` passes start 7),
compares it with the eight strings of the table at `0x013506C8` (stride `0x54`,
built by `0x00F1CCB0`) and creates one of seven classes. A name that is not in
the table gives no gate. STATIC.

| Index | Name | Class object size |
|---|---|---|
| 0 | `None` | no object |
| 1 | `Test` | `0x20` |
| 2 | `PcSearchWidget` | `0x10C` |
| 3 | `PcSearchSelectWidget` | `0x40` |
| 4 | `PlayerInformationWidget` | `0x40` |
| 5 | `PcInformationWidget` | `0x34` |
| 6 | `TargetParameterWidget` | `0x5C` |
| 7 | `PlayerParameterWidget` | `0x24` |

When a gate is created the form handler is also registered on the root element
(`0x00538AB1`), then `0x00538AB8` calls the gate slot 1 and passes the gate to
`0x00694FD0` when that returns 1. So the gate is a per widget pad UI controller
for seven named widgets, and for every other form the handler skips it. The gate
role is INFERRED from the pad commands in the one-shot block and the class
names. The slot 42 and 43 bodies of the seven classes are not followed.

### Popup help positioning

Command object `0x0133BD38` is `PopupShowHelp` (initializer `0x00F0F6E0` passes
the string at `0x00FA8770`). When the global at `0x0133664C` equals the form
element and the command is `PopupShowHelp`, the handler calls `0x00535D00` and
then continues (`0x0053651D..0x00536535`). The global holds the form that owns
the element named `Window_PopupHelpWidget` (`0x00FA0658`), set at
`0x0053893D`. `0x00535D00` returns at once when the byte at `0x01336654` is zero.
Otherwise it reads the pending state at `0x01336638..0x01336650`, takes the
popup window root element, and moves it with the two coordinate setters
`0x00924050` and `0x009240C0`, clamping against the extent at `+0x14C` of the
root global. It never
touches the subscription map or the Lua call, so it is a layout side effect.
STATIC for the control flow, INFERRED for the placement purpose. The same call
runs from the `RenderSizeChanged` branch (`0x00536433`) when the sender is the
popup root element.

### Per-frame debounce

`UILuaCommands.Operate`, `Cancel`, `WidgetClose`, `Selection`,
`SelectionChanged` and `SelectionChangedOnce` (static objects `0x0133B798`,
`0x0133B838`, `0x0133B8D8`, `0x0133B978`, `0x0133BA18`, `0x0133BAB8`) share one
flag at form element `+0x1EC`. The handler compares the frame counter at
`[0x0091A1B0 result + 0x46C]` with the stored frame at `+0x1E8`, clears the flag
when the frame changed, and returns for any of the six when the flag is already
set. Otherwise it sets the flag and continues
(`0x00536601..0x00536659`). So at most one of the six reaches Lua per frame for
one form element. Other command names have no such limit. STATIC.

## Subscription gate

The handler builds three strings and looks them up in the map at form element
`+0xF8` through `0x00535B70`. A miss returns with no Lua call
(`0x005365DA`, `0x005365DC`). The sender is the first handler argument, the
owner of the store that held the handler. It is the element that carries the
subscription and not the original event source, because the store dispatcher
passes its own owner (see [Dispatch to the handler store](#dispatch-to-the-handler-store)).
The plain registration adds the handler to the element it names
(`0x0053A9CC`, or the root for `_widget` at `0x0053ABC9`). STATIC for the keys
and for the sender.

| Key | Source |
|---|---|
| Template name | `0x00535450(0x005357D0(sender))`, a lookup by a numeric id derived from the sender that gives the empty string when none exists |
| Control name | Name of the sender element, read through its virtual slot 2 (`0x0053656E..0x0053657C`) |
| Command name | `command +0x24` (`0x00536595`) |

Two name substitutions apply. STATIC.

- `0x00535890` copies the name of the form's root element into form element
  `+0x164`. A control name equal to that root name is replaced by the literal
  `_widget` (`0x00FA0650`, held in the static string at `0x01336A30`) at
  `0x005365B1`.
- A template name equal to the root name sets the root variant at
  `0x005365E2..0x005365F7`. That variant passes an empty first string to the
  script, and the other variant passes the template name.

The scripts add entries through two Lua natives. STATIC for the bindings.

| Native | String VA | Arguments | Storage |
|---|---|---|---|
| `_setUICommandCondition` | `0x00FD74D4` | control, command, optional number | `0x0053AB80` registers it under the root name as template key (`0x0053ABF7`), and `_widget` registers the root element |
| `_setUICommandTemplateCondition` | `0x00FD7574` | template, control, command, optional number | `0x0053AA00` inserts into the map at form element `+0x104` through `0x0053A720` |

Both natives build a condition object from the command string through
`0x007172A0`. The object carries two bytes at string `+0x54` and `+0x55`
(`0x006EB6A0` for the plain native). STATIC.

- With a number: `+0x54` is the byte at `0x0134C3FE` and `+0x55` is the low
  byte of the number (`0x006EB7F4`).
- Without a number: `+0x54` is the byte at `0x012C3F78` and `+0x55` is 0
  (`0x006EB779`).

The byte at `0x0134C3FE` holds 0 in the image and the byte at `0x012C3F78`
holds 1. The Capstone sweep of `.text` finds only reads of both, ten of the
first (`0x006EB7F4`, `0x006EBA04`, `0x006F13B5`, `0x006F1C55`, `0x006F2342`,
`0x006F29E5`, `0x006F2AC5`, `0x006F2D2D`, `0x006F2EB5`, `0x006F569D`) and two of
the second (`0x006EB779`, `0x006EB984`), so both are treated as constants.
INFERRED for the constancy.

The expansion of the `+0x104` map into per-instance entries in the `+0xF8` map
is INFERRED from `0x0053AA10`, which reads the `+0x104` map, and from the lookup
key order. The tracked script metadata lists 88 call sites of
`_setUICommandCondition` and 9 of `_setUICommandTemplateCondition`
(`xivl-client-scripts:lua/napi_index.json`, entries `apis._setUICommandCondition`
and `apis._setUICommandTemplateCondition`).

## Kind switch

With a subscription found, the handler reads the record kind at `args +0x24`
and jumps through the nine entry table at `0x005367A0`
(`0x00536663..0x0053666F`, guard `kind > 8`). STATIC.

| Record kind | Event parameter class | Values passed | Handler address |
|---|---|---|---|
| 0 | `UICommandEventParameter` | none | `0x00536676` |
| 1 | `UICommandEventParameterWithInt` | integer 0 (`args +0x7C`) | `0x005366AA` |
| 2 | `UICommandEventParameterWithTwoInt` | integers 0 and 1 (`+0x7C`, `+0x80`) | `0x005366DA` |
| 3 | `UICommandEventParameterWithTwoInt` | integers 0 and 1, the third is dropped | `0x005366DA` |
| 4 to 7 | none | no event | `0x0053674F` |
| 8 | `UICommandEventParameterWithString` | string at `args +0x28` | `0x0053671F` |
| above 8 | none | no event | `0x0053674F` |

Each class has a root and a child form. The child form takes the template name,
control name and command name. The root form is used when the root flag from
the previous section is set and supplies an empty first string.

| Variant | Child slot | Root slot |
|---|---|---|
| none | 8 (`0x00574BB0`) | 9 (`0x00574BA0`) |
| one integer | 10 (`0x00574BD0`) | 11 (`0x00574BC0`) |
| string | 12 (`0x00574BF0`) | 13 (`0x00574BE0`) |
| two integers | 14 (`0x00574C10`) | 15 (`0x00574C00`) |

Form element `+0x80` holds a pointer to a holder, not the script object. Each
thunk loads that pointer (`mov ecx,[ecx]`), takes the vtable of the object it
points to and jumps to offset `4 * slot`. The child thunks take the template
name, the control name and the subscription, and the root thunks take the
control name and the subscription. The subscription is the map value returned
by the lookup `0x00535B70`, a string object holding the command text with the
two condition bytes at `+0x54` and `+0x55`. The command name that the script
receives is that object: the root slot chain reaches `0x006F56E0` through
`0x008A3F70`, which passes it as string 2 of the parameter base constructor
`0x007132E0`, takes the priority from its `+0x55` and the event or request
byte from its `+0x54` (`0x006F5737..0x006F5759`). The slot functions are in the table at
`0x00FDFB4C` (`0x0075D510`, `0x0075D4E0`, `0x0075D590`, `0x0075D550`,
`0x0075D610`, `0x0075D5D0`, `0x0075D690`, `0x0075D650`). The catalog names the
vtable at RVA `0xBDFB2C` (VA `0x00FDFB2C`)
`Application::Lua::Script::Client::LuaActorImpl`. That
the object at form element `+0x80` has this vtable is INFERRED from the slot
offsets matching the slot bodies, since no constructor store was traced.

The grammar sets the kind to the integer count. A three integer text reaches
kind 3 and delivers its first two integers. A text of four to seven integers
delivers nothing. An eight integer text reaches kind 8, which the switch treats
as the string class with whatever the record string holds.

## Script entry

The event parameter classes share a base. The layout, read from the constructor
`0x007132E0` and the variant constructors, is below. STATIC.

| Offset | Use |
|---|---|
| `+0x04` | String 0, the template name |
| `+0x58` | String 1, the control name |
| `+0xAC` | String 2, the command name |
| `+0x100` | Priority byte, the condition byte `+0x55` |
| `+0x104` | Integer 0, or the string for the string class |
| `+0x108` | Integer 1, two integer class only |

Slot 2 of each class pushes the Lua arguments (`0x00749220`, `0x00749270`,
`0x007492D0`, `0x00749340`). It pushes string 0 as nil when its first byte is
NUL and as a string otherwise (`0x0074922B..0x00749247`), then strings 1 and 2,
then the variant values. The value pushers are `0x00748DA0` for a string,
`0x00748E00` for an integer and `0x00748EC0` for nil. The Lua method therefore
receives, after `self`:

| Class | Arguments |
|---|---|
| none | template or nil, control, command |
| one integer | template or nil, control, command, integer |
| two integers | template or nil, control, command, integer, integer |
| string | template or nil, control, command, string |

STATIC for the push sequence. The integers are 32 bit signed values from the
record. The tracked script metadata lists one occurrence of each method name
(`xivl-client-scripts:lua/napi_index.json`, entries `apis._onUICommandEvent`
and `apis._onUICommandRequest`, both in `widget/widgetbaseclass`). The parameter
lists of the script definitions are not in tracked artifacts.

The call goes through `0x006F5640`. Its argument is a pointer to a pointer.
The chain `0x00CC7510`, `0x00CD7910` adds `0x220` to the pointee, the word there
is a host object `H`, the script object `S` is the word at `H+4`, and its owner
`O` is the word at `S+0xC`. The function returns without a Lua call when any of
these hold (`0x006F5651..0x006F5697`). STATIC for the conditions.

- `O` is null (`0x006F5654`).
- The map at `+0x8` of the object that `0x006F51A0` creates lazily at `O+0xD8`
  (a `0x78` byte object built by `0x006F5100`) has no nonzero entry for the key
  0. The key is a local that `0x006F565F` zeroes and passes by pointer, and
  `0x006E5BA0` returns the stored value or 0 (`0x006F566E`, `0x006F5675`).
- The word at `O+0x128` differs from the global at `0x0130C778` (`0x006F567D`).
  The image holds `0xE0000000` there. The `.text` has 183 instructions that
  name the global, and the sampled ones return null or branch when an id word
  equals it, so it reads as the null id. INFERRED. A call therefore proceeds only
  for an owner whose id word holds the null id, which is INFERRED to be the case
  for widget script objects and not for actor-bound ones.
- `0x00763030` returns true and the byte at `O+0x161` is nonzero
  (`0x006F568E..0x006F5697`). `0x00763030` reads byte `+0xBA` of the first
  element of the list at `S+0x1C` and gives false for an empty list.

The meaning of `O+0x128`, `O+0x161` and the `+0xBA` byte beyond these
conditions is UNKNOWN, and so is what fills the key 0 entry. The first two
gates read as liveness checks on the script host pair and the last two as state
filters, all INFERRED. The first gate is the only one whose condition has an
obvious client equivalent, a script object whose owner was released.

Otherwise it compares the byte at `0x0134C3FE` (zero) with the condition byte
`+0x54` of the subscription (`0x006F569D..0x006F56A5`). STATIC.

| Condition byte `+0x54` | Subscription form | Path | Lua method |
|---|---|---|---|
| 0 | a number was given | `0x00896AF0` | `_onUICommandEvent` (`0x01056B88`) |
| 1 | no number | `0x00896D00` | `_onUICommandRequest` (`0x01056B9C`) |

The event path keeps the parameter in a two slot queue. A second event whose
priority byte is not above the stored one is rejected with a false return
(`0x00896B2A..0x00896B42`). A higher priority event takes the second slot and
runs after the first through `0x00896ED0`. The request path rejects when its
nesting count at `+0x14` is 8 or more (`0x00896D33`). STATIC. When the first
stored event is released is UNKNOWN, so the cadence at which equal priority
events are dropped is not established.

## Other consumers of the record

The record is not Lua only. Handlers on other classes read the same event
fields. `NumberInputBox` registers a handler through `0x0069DB94` for the
`Executed` descriptor itself (static `0x01358738`, through slot 10 of the
interface) and not for the wildcard. The handler body `0x0069C1F0` returns
unless the event command, read at event `+0x1C` (`81 7A 1C` at `0x0069C1F0`),
is `0x013609B0`, whose name string is
`Slider.AddCommandParameter` (initializer `0x00F27FB0`, string `0x01078E3C`).
Then it compares integer 1 (`+0x80`) with zero and integer 0 (`+0x7C`) with
`-1`, adds integer 0 to the word at box `+0x35C`, and sets the handled flag
only when integer 1 is zero, integer 0 is below `-1` and the sum is not above
zero. STATIC for the reads and the rule. The two step commands come from code and
not from markup. The box constructor path at `0x0069C340` sets command
`0x013609B0` on two command sources (`+0xAA0` and `+0xE54`) with records from
`0x004DDCF0`, whose integer 0 is the step and its negation. That builder writes
kind 1, stores the integer at record `+0x58` and zeroes the other seven integer
dwords (`0x004DDD26..0x004DDD3E`). So integer 1 of these records is always 0,
and no markup or script supplies it. STATIC.

The census command `NumberInputBox.ValueChanged` (`0x01344CA8`, initializer
`0x00F14550`) is a different command object that this handler ignores. The
only instructions that name its address are the initializer and its unwind
cleanup at `0x00E8D418`. The 9 census values for it
are all the text `{CommandParameter Int, %s}` with `PropertyName1` set to
`Value`, a one integer record (kind 1, or kind 0 when the property is empty).
They can reach a script only by name through the subscription gate. The 79
two integer brace values of the census are on `Selector.*` (24),
`UICommands.*` (13) and `UILuaCommands.*` (42) commands. The `Selector.*`,
`KeyConfig.*` and `MapScreen.*` commands have no handler traced here and their
consumers are UNKNOWN.

## Census of what reaches a script

The census is the one in the grammar page and is bounded to the installed
client that was decoded. Each `CommandParameter` value is grouped below by the
command namespace of its owner. The command name is the owner's `Command`
attribute. No `Name` value is reproduced. INFERRED for the grouping, since the
census reads attribute text and the command attribute of the same element.

| Command namespace | Values | Owners |
|---|---:|---|
| `UILuaCommands.*` | 281 | 217 `ExecuteCommandAction`, 36 `Button`, 14 `KeyBinding`, 14 `PadBinding` |
| `UICommands.*` | 19 | `ExecuteCommandAction` |
| `Selector.*` | 54 | `ExecuteCommandAction` |
| `NumberInputBox.ValueChanged` | 9 | `ExecuteCommandAction` |
| `ApplicationCommands.MoveFocus` | 18 | `KeyBinding` |
| `KeyConfig.Default`, `MapScreen.Move` | 4 | `ExecuteCommandAction` |

Of the 385 values, 281 are in the `UILuaCommands.*` namespace, the generic
script-facing commands. The reachability reading is INFERRED: the subscription
gate keys on the command name string and `_setUICommandCondition` accepts any
name, so a `UICommands.*` or `Selector.*` command could also be subscribed and
the tracked metadata does not list the subscriptions. The
147 `{Binding ...}` values are all on `ExecuteCommandAction`: 139 with
`UILuaCommands.*` commands, 4 with `UICommands.*` commands and 4 with
`KeyConfig.Default` or `MapScreen.Move`. The `PropertyName1` and `PropertyName2`
attributes that feed `%s` name `IntData.Value0` (95), `SelectedIndex` (55),
`MouseEnteredIndex` (35), `AnchoredIndex` (35), `Value` (9), `Text2` (2) and
`Talker` (2), and 2 start with `%`.

## PartyParameterWidget

The one `CommandParameter` of `widget/PartyParameterWidget.tpl` is on an
`ExecuteCommandAction` in a `ListBox` style, triggered by `SelectionChanged`,
with command `UILuaCommands.SelectionChanged`, text
`{CommandParameter Int, %s}` and `PropertyName1` set to `SelectedIndex`. The
delivery path is therefore: the property read of `SelectedIndex` on the event
source is converted to text, formatted into the template, parsed as kind 1 with
integer 0, and delivered as `UICommandEventParameterWithInt` only when the
widget script has a subscription for the sender and `UILuaCommands.SelectionChanged`.
When the property is unset the substituted text is empty, the text becomes
`{CommandParameter Int, }`, and the grammar yields kind 0 with no integers, so
the none class is delivered. Whether the script subscribes, and what its
handler does with the values, is script content that the tracked metadata does
not state. UNKNOWN.

## Consumer delivery contract

What a client implementation can build from this page, in order of the path.

1. Resolve `Command` text to a command object by exact string match on the
   command name. Create the object when the name is unknown. Do not fail.
2. For an `ExecuteCommandAction` with a non-empty `PropertyName1` or
   `PropertyName2`, format the `CommandParameter` attribute text (the `+0x78`
   string) as a C format with the first substitution and then the second.
   Resolve each name as a property of the event source converted to text, or as
   an indexed argument of the triggering event when it starts with `%`. An unset
   property gives the empty string. Parse the result with the grammar page.
   With neither name set, use the record parsed at load or from the evaluated
   binding.
3. Pick the target (the command target, else the trigger source as a
   `FrameworkElement`). Raise `PreviewExecuted` along its tunnel route. When no
   handler set the handled byte, raise `Executed` along its bubble route from the
   target to the root, and return the handled byte.
4. Deliver to a script only through a registered subscription. Key it by
   template name, control name and command name. A plain registration uses the
   form root name as the template name, a control named like the root is
   `_widget`, and a template name equal to the root name is passed to the
   script as nil.
5. Pick the class by record kind as the kind switch table states. Kinds 4 to 7
   and above 8 deliver nothing. Kind 3 delivers two integers.
6. Call `_onUICommandEvent` when the subscription was made with a number and
   `_onUICommandRequest` when it was made without one. Pass the three names
   (first as nil when empty) and then the integer, the two integers, the string,
   or nothing.
7. Limit the six generic `UILuaCommands` to one delivery per frame per form
   element. Reject an event whose priority is not above a stored one, and a
   request nested eight deep.
8. No text identifier is resolved on this path. The integer converter of the
   grammar page skips a leading `@` and the record carries raw integers and
   strings. Any lookup is script work.

Items 1 to 3 and 5 to 6 are STATIC. Item 4 is STATIC except for the template
map expansion and the lookup of the template name for controls outside a
template, which are INFERRED. Item 7 is STATIC for the debounce and the depth
limit and UNKNOWN for when a stored event is released.

Delivery also stops silently on the entry flag, the form gate, the `0x006F5640`
gates and the pad one-shot gate, which the sections above identify only as far
as stated. A client that implements the contract without them delivers in cases
where retail does not.

### What bahamut-client can implement now

Implement as written, all STATIC:

1. The routed event descriptor with id, strategy byte and name, and the table of
   89 initializer rows in the manifest. Strategy 0 walks target to root, 2 walks
   root to target, 1 reaches the target only.
2. The route builder: target, then parents, root last, skipping the root as a
   parent and appending it once, and the traversal that ends when the stop byte
   is set.
3. A per element handler store holding `(descriptor, handler, handledEventsToo)`
   in registration order, with remove then add on registration. The dispatch rule
   is descriptor identity or a handler descriptor with id `0x59`. A handler with
   `handledEventsToo` clear is skipped once the handled byte is set. The handler
   receives `(sender, event)` where the sender is the element that owns the
   store. A store exists only after the first registration.
4. The command raise: `PreviewExecuted`, then `Executed` only when the first is
   unhandled, with the target rule above and the event layout of the table
   above.
5. Registration of the form handler on the named control, on the root for
   `_widget`, and on the popup help window, and the form handler body as the
   earlier sections state, including the subscription key, the kind switch, the
   per-frame debounce and the script entry.
6. Elements that list the command in their own binding list set the handled
   byte after their store runs, which ends form handler delivery higher up the
   route.
7. `NumberInputBox` step records: integer 1 is 0, and the handler swallows only
   a step below -1 whose sum does not stay above 0.

Implement as an approximation and label it:

- The form gate is absent for all forms except the seven named widgets, and
  those widgets need their own pad UI controller to reach retail behavior.
  Without it the handler skips straight to the lookup.
- The selector gates as: the script object has an owner and the widget is
  live. The key 0 entry, the null id and the two state filters are not
  understood.
- The entry flag as never set.
- The popup positioning call as a layout side effect of `PopupShowHelp` only,
  and never as a condition of delivery.

Do not assume:

- That every control class stores events. Only the base interface and
  `FrameworkElement` slot 65 are traced. A control whose override skips the base
  would not run the form handler.
- That `Button`, `KeyBinding` or `PadBinding` raise `Executed` the way the
  trigger action does. Their raise sites are not traced.

## Surviving gaps

- Whether every Sqwt control class forwards virtual slot 65 to its handler
  store. The base interface and `FrameworkElement` do. `NumberInputBox`
  (`0x0069C5E0`) calls `0x009970A0`, whose first call is `0x0092D1F0`, and
  `ToggleButton` (`0x0095D530`) reaches `0x0092D1F0` through `0x0097EDB0`. The
  other overrides were not followed.
- The widget classes behind the seven gate objects and their slot 42 and 43
  bodies, and the event that runs the form function `0x005387F0`.
- The meaning of owner fields `+0x128` and `+0x161` and of byte `+0xBA` read by
  the selector gates, beyond the conditions stated.
- The purpose of the flag byte at `0x01356E14`, beyond its stores and reads.
- The release point of the first stored event and the cadence of `0x00896ED0`.
- The sites where `Button`, `KeyBinding` and `PadBinding` put a record into an
  `Executed` event. They are INFERRED to use the same command slot 2 route as
  the binding path in the
  [movement command origin page](../input/player-movement-command-origin.md).
- The `Lua` parameter lists of `_onUICommandEvent` and `_onUICommandRequest`, and
  every subscription list, which are script content.
- The consumers of the `Selector.*`, `KeyConfig.*` and `MapScreen.*` commands.
- The event argument source of a leading `%` property name.

## Reproduction

1. Verify the executable size and SHA-256 above.
2. Run `python tools/verify_sqwt_command_parameter_consumer.py --exe orig/ffxivgame.exe`.
   It checks the jump table, the script interface slot tables, the parameter
   push slots, the strings, the two condition bytes, the 89 event descriptor
   initializers and the masked signatures named in the manifest.
3. Disassemble `0x00536280..0x00536777` with Capstone in x86 32-bit mode and
   confirm the gates, the lookup call at `0x005365D3`, the debounce chain at
   `0x00536622` and the switch at `0x00536663`. The table at `0x005367A0` lists
   the nine targets.
4. Disassemble the thunks `0x00574BA0..0x00574C17` (note the double load of
   the first pointer), the slot functions of `0x0075D4E0..0x0075D690`, the
   chain `0x008A3F70` and `0x006F56E0`, the push slots of the four vtables at
   `0x00FD5300`, `0x00FD5310`, `0x00FD5320` and `0x00FD5330`, and the selector
   `0x006F5640`.
5. Disassemble `FUN_00955010` at `0x00955050` for the `+0x78` store and
   `0x00954F8C` for the command lookup and creation.
6. For the routing, disassemble `0x00927FA0` and `0x00929100` (command raise and
   event arguments), `0x009B2B40` (target), `0x0091F590`, `0x0091E1C0` and
   `0x0091D210` (route), `0x0092D060`, `0x00939800`, `0x0091D320`, `0x0091D8F0`
   and `0x0092D1F0` (dispatch), `0x00571520` (store dispatcher, the compare at
   `0x005715D7`), `0x0091F6F0`, `0x0091F710` and `0x0091C5B0` (registration),
   `0x00543AD0`, `0x00542583` and `0x0088AB00` (handler object), `0x0097F6C0`
   and the initializers listed in the manifest.
7. For the gates, disassemble `0x005362F8..0x00536325`, `0x005363D5..0x005363F9`,
   `0x00538A6F..0x00538AC3`, `0x00874820..0x008748E0` and the table
   initializer `0x00F1CCB0`, then `0x00535D00`, `0x00538918..0x0053894A` and
   `0x006F5640`.
8. For `NumberInputBox`, disassemble `0x0069C1F0`, `0x0069DB83..0x0069DBB2`,
   `0x0069C340` and `0x004DDCF0`.
9. For the census, follow the reproduction in the grammar page and group each
   value by the `Command` attribute of its owner. Keep client bytes, decoded
   documents and executable listings outside the repository.
