# Sqwt CommandParameter consumer

A parsed `CommandParameter` record leaves the Sqwt command layer in an
`Executed` event. One native handler on the form element reads the record kind,
picks one of four event parameter classes, and calls the Lua method
`_onUICommandEvent` on the widget script object (or `_onUICommandRequest`, by
subscription form), but only when the script has subscribed to the exact
template, control and command triple. A record with no subscription, an unknown
command name, or a kind with no matching class produces no error and no Lua
call. Everything from the handler body onward is STATIC. That an `Executed`
event (id `0x2A`) is what reaches the handler is INFERRED, because the
registration adds the handler for a different, unnamed event (id `0x59`) and
the join between the two is not established, see
[Executed event and the form handler](#executed-event-and-the-form-handler).

This page records that path, the subscription gate in front of it, and the
values a script receives. It builds on the
[CommandParameter text grammar](sqwt-command-parameter-grammar.md), which owns
the record layout and the parser. It does not establish what any individual
script does with the values.

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

## Executed event and the form handler

`Sqwt::Input::RoutedCommand` slot 2 (`0x00927FA0`) builds a `PreviewExecuted`
event, routes it, and when no handler stops it builds the `Executed` event
(id `0x2A`, static `0x01358738`, initializer `0x00F21CB0`, which pushes the
name string `Executed`) and routes that. Event construction copies the record as
described in the grammar page. STATIC.

The candidate consumer of that event for script-backed widgets is a member
function of `Application::Main::Element::FormElement`, `FUN_00536280`. Its
constructor stores the address at `0x0053AFAF` and builds a
`Sqwt::RoutedEventHandler<Application::Main::Element::FormElement>` object at
form element `+0xA0` through `0x00543AD0`. The subscription call adds that
handler object to the control through the control's `+0xB4` interface
(`0x0053ABC1..0x0053ABD0`, event static `0x01336A84`, id `0x59`, initializer
`0x00F0BEE0`). That initializer pushes no name string, unlike the `Executed`
initializer. STATIC.

The handler argument layout, read from the code, is below.

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
(`0x00536454`, `0x00536433`) that this page does not follow. Any other id
returns. STATIC.

How an event with id `0x2A` reaches a handler that the subscription call adds
for the event at `0x01336A84` is not established. The event matching rule of
`RoutedEventHandler<T>` and the role of the first virtual call at
`0x0053ABC1` are UNKNOWN. The handler body branches on the id of the event it
receives and reads `Executed` fields on the `0x2A` branch, so the id `0x59`
registration may act as a catch-all. That reading is INFERRED and is the reason
every delivery step below depends on an unproven join.

### Entry gates

Before the subscription lookup the handler returns, with no Lua call, when any
of these hold. STATIC.

- The boolean returned by `0x00919D30` after `0x0091A1B0` is 1
  (`0x005362CC..0x005362D8`).
- Argument `+0x04` or `+0x08` is null (`0x005362DE`, `0x005362E9`).
- Form element `+0x100` is zero (`0x00536535`).
- The entry found through form element `+0xD4` has a null pointer at `+0x2C`
  (`0x005364F4..0x00536517`).

The handler also takes a side path for command object `0x0133BD38` when the
global at `0x0133664C` equals the form element: it calls `0x00535D00`
(`0x0053651D..0x00536530`) and then continues. The effect of that call is
UNKNOWN.

A separate gate object at form element `+0x27C`, when present and active
(byte `+8`), sees the command, the source and the address of the record before
the lookup (`0x005363D5..0x005363F7`). When its virtual `+0xA8` returns 1 the
handler sets the handled flag and returns. For the six pad-UI commands
`UICommands.Operate`, `Cancel`, `Close`, `Selection`, `SelectionChanged` and
`SelectionChangedOnce` (objects from the `0x9C` stride table at `0x013442E8`,
entries 1 to 6) it also keeps a one-shot flag at `+0x1F4` that resets when the
frame counter at `[0x0091A1B0 result + 0x46C]` changes
(`0x0053632A..0x005363C9`). The one-shot block runs only when the command
object has a nonzero word at `+0x78` (`0x0053632D`). The identity and role of
the gate object are UNKNOWN.

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
element the handler was invoked for. That it is the element that carries the
subscription, and not the original event source, is INFERRED from the plain
registration adding the handler to each named control (`0x0053ABC1`). STATIC for
the keys.

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

The call goes through `0x006F5640`. It resolves the script object and returns
without a Lua call when any of these hold (`0x006F5651..0x006F5697`). STATIC.
The reasons are unexplained gates.

- The word at `+0xC` of the resolved object is null (`0x006F5654`).
- The call at `0x006F566E` returns zero (`0x006F5675`).
- The word at `+0x128` of that object differs from the global at `0x0130C778`
  (`0x006F567D`).
- `0x00763030` returns true and the byte at `+0x161` of that object is nonzero
  (`0x006F568E..0x006F5697`).

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
fields. `NumberInputBox` registers a handler through `0x0069DB94` whose body
`0x0069C1F0` compares integer 1 (`+0x80`) with zero and integer 0 (`+0x7C`) with
`-1` before setting the handled flag, so it reads two signed integers by
position (STATIC for the reads). The census has 9 values for the command
`NumberInputBox.ValueChanged` (`0x01344CA8`). All 9 are the text
`{CommandParameter Int, %s}` with `PropertyName1` set to `Value`, a one integer
record (kind 1, or kind 0 when the property is empty). Markup therefore does not
supply integer 1, and the value that handler compares at `+0x80` comes from the
part of the record the one integer text leaves unfilled. What that part holds is
UNKNOWN. The 79 two integer brace values of the census are on `Selector.*` (24),
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
3. Raise an `Executed` event carrying the record and the command. That this
   event is what reaches the form handler is INFERRED, see the caveat after the
   list.
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

Items 1 to 2 and 5 to 6 are STATIC for the code that runs once the handler is
entered. The Executed-to-handler join of item 3 is INFERRED, so delivery
depends on the unresolved id `0x59` subscription for every step after it. Item
4 is STATIC except for the template map expansion and the lookup of the
template name for controls outside a template, which are INFERRED. Item 7 is
STATIC for the debounce and the depth limit and UNKNOWN for when a stored event
is released.

Delivery also stops silently on the entry gates, the `0x006F5640` gates and the
pad one-shot gate, none of which has an explained purpose. A client that
implements the contract without them delivers in cases where retail does not.

## Surviving gaps

- Blocking: how an event with id `0x2A` reaches a handler added for the
  unnamed event at `0x01336A84` (id `0x59`), and the event matching rule of
  `RoutedEventHandler<T>`.
- The role of the gate object at form element `+0x27C`, of the form element
  flag at `+0x100`, of the call `0x00535D00` for command object `0x0133BD38`,
  and of the selector gates at `0x006F5654`, `0x006F5675`, `0x006F567D` and
  `0x006F568E`.
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
   push slots, the strings, the two condition bytes and the masked signatures
   named in the manifest.
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
6. For the census, follow the reproduction in the grammar page and group each
   value by the `Command` attribute of its owner. Keep client bytes, decoded
   documents and executable listings outside the repository.
