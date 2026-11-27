# Sqwt pointer route to a widget

This page traces how a delivered pointer event reaches a widget in a presented
SQWT form: window hit order, the modal and input gates, the disabled rule,
enter and leave production, and the Button and ListBox release paths. It
starts where the
[mouse preview producer](sqwt-mouse-preview-producer.md) stops. No client ran.

## Input and method

The input is retail `ffxivgame.exe` 1.23b `2012.09.19.0001`, `15996808`
bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. Ghidra CLI decompilation, cross references and function lists
located the bodies. The load bearing points were read again with Capstone 5.0.7
over file backed PE sections. Class tables come from
`config/ffxivgame.vtable_slots.jsonl` (RVA plus base). Markup is from an
installed client under `client/sqwt/`, decoded with
`xivl-tools:tools/blowfish.py` (the key is the full file name with its suffix
and an 8 byte header is skipped). Markup sizes and hashes are those of the raw
installed carriers.

Tiers are STATIC (read from instructions or data), MEASURED (read from decoded
markup), INFERRED (a stated join) and UNKNOWN.

E is a complete element and I = E+`0xB4` is its input interface. In the event
arguments `+0x04` is the descriptor, `+0x08` the source, `+0x0C` the original
source, `+0x10` points to the handled byte at `+0x19`, and `+0x17` is the stop
byte. Descriptor byte `+4` is the routing strategy and byte `+5` the id, as in
[Executed event delivery](../resource/sqwt-command-parameter-consumer.md#executed-event-delivery).

## Findings owned elsewhere

| Subject | Home |
| --- | --- |
| Win32 mouse messages, capture, the `0x203` arm and the report record | [Sqex mouse numeric callback](sqex-mouse-numeric-callback.md) |
| Raw mouse report into DesktopWindow | [RaptureMouseDevice report provider](sqwt-rapture-mouse-device.md) |
| Per button event production, target order (preferred pointer, explicit target, hit query) and the drag gated preferred pointer | [Mouse preview producer](sqwt-mouse-preview-producer.md) |
| Child first hit walk, IsHitTestVisible, Button Click gates, ClickMode default, Command dispatch, press focus arm and focus writer | [CommandParameter consumer](../resource/sqwt-command-parameter-consumer.md) |
| ListBox release selection, SelectionChanged raise, toggle before Command | [ComboBox drop-down commands](../resource/sqwt-combobox-dropdown.md) |
| WidgetBase SelectionChanged arm, the index reset and the row style table | [Main menu command route](../script/main-menu-command-route.md#selection) |
| Skin slot read order and RenderSkin row lookup | [Tab, scroll and text parts](../resource/sqwt-tab-scroll-text-parts.md), [TimerIcon style](../resource/action-menu-timer-icon-style.md) |
| Modal group records and the Show routine | [Window style and template event order](../resource/sqwt-window-trigger-order.md) |
| SqwtDrawOrder sorted sibling insertion | [Keyframe target binding](../resource/sqwt-keyframe-target-binding.md) |
| ListBox Operate arm | [Window style and template event order](../resource/sqwt-window-trigger-order.md) |

## Dispatch tables

STATIC. Byte table `0x0091D818`, indexed by id minus 1, selects an arm of jump
table `0x0091D760`. Each arm calls one interface virtual. Names, ids and
routing come from the calls to descriptor constructor `0x0097F6C0`.

| Id | Event | Routing | Interface offset (slot) |
| --- | --- | ---: | --- |
| `07` / `17` | PreviewMouseDown / PreviewMouseUp | 2 | `+0xA4` (41) / `+0xA8` (42) |
| `08` / `18` | MouseDown / MouseUp | 0 | `+0xBC` (47) / `+0xC0` (48) |
| `0D` / `0F` | PreviewMouseLeftButtonDown / Up | 2 | `+0xAC` (43) / `+0xB0` (44) |
| `0E` / `10` | MouseLeftButtonDown / Up | 0 | `+0xC4` (49) / `+0xC8` (50) |
| `13` / `15` | PreviewMouseRightButtonDown / Up | 2 | `+0xB4` (45) / `+0xB8` (46) |
| `14` / `16` | MouseRightButtonDown / Up | 0 | `+0xCC` (51) / `+0xD0` (52) |
| `09` / `0A` | PreviewMouseEnter / MouseEnter | 2 / 0 | `+0xF0` (60) / `+0xF4` (61) |
| `0B` / `0C` | PreviewMouseLeave / MouseLeave | 2 / 0 | `+0xF8` (62) / `+0xFC` (63) |
| `11` | PreviewMouseMove | 2 | `+0x80` (32) |
| `20` / `22` | GotKeyboardFocus (descriptor `0x0135945C`) / LostKeyboardFocus | 0 | `+0xE8` (58) / `+0xEC` (59) |
| `4D` / `4E` | GotAnchor / LostAnchor | 0 | `+0xDC` (55) / `+0xE0` (56) |
| `2A` | Executed | 0 | `+0x104` (65) |

The generic PreviewMouseDown and PreviewMouseUp arms raise the left or right
preview on the same element right after its generic handler, so the pairs
interleave element by element along the tunnel.

Route walker `0x0091E1C0` calls only the target for routing 1. Otherwise it
collects the target and its `+0x60` parents, appends the desktop and reverses
the list (`0x00932760`). `0x0091D210` then walks root first for routing 2 and
target first for routing 0. Only stop byte `+0x17` ends the walk. The handled
byte does not.

## Hit order across windows

STATIC. DesktopWindow query `0x0091A4E0` walks an ordered container at
D+`0x490` from the end to the beginning (`0x004E4020` is the tree decrement).
The first nonnull child result wins. An exhausted walk returns the desktop
interface. A preferred pointer or an explicit target skips this query.

STATIC. The key is the window float at `+0x70` (`SqwtDrawOrder`). Insert
`0x00927890` goes right when the node key is not above the new key
(`0x009278B0..0x009278CB`), so among equal keys the latest registration is hit
first. The hit order is therefore draw order high to low, latest registration
first among equals. Registration helper `0x0091AF80` erases the old entry
first. The registration callers read were Window primary slots 10
(`0x00923DC0`), 16 (`0x00923100`, parent is the desktop) and 19
(`0x00921FF0`), the SqwtModalOrder arm `0x00923B15..0x00923B56` (which first
sets draw order from `+0x3D0`) and Popup `0x009A53D0`. Popup sets its draw
order to that of the element its primary `+0xB0` returns plus the double 0.1
at `0x00FBAD50`, then registers. That the element is the owner window is
INFERRED.

### Raise

STATIC. `0x0093FD70` removes and adds a visual again in its parent and, when
its index changed, calls primary `+0x4C` (slot 19, registration for a Window).
Three kinds of path reach a raise:

- The IsActive arm of property callback `0x009238A0`
  (`0x00923A3D..0x00923A81`). When true it calls `0x00923790(1)` (modal group
  registration), `0x0093FD70`, sets the skin active bit and raises Activated.
  The IsKeyboardFocusWithin override `0x00926870` copies E+`0x15D` bit 5 into
  IsActive. Keyboard focus entering a window therefore raises it among equals.
- The show routine `0x00925AA0 -> 0x009237D0` raises the window through
  `0x0093FD70` (`0x00923811`) when focus is not already within it, then raises
  the window `0x00923020` selects if that is a different one.
- Attaching to the desktop (`0x00923100`) registers the window and raises its
  blocker, if any. `0x00923190` and `0x00923D10` (interface slot 57) can also
  raise a blocker.

Other raise paths are not excluded. That a press raises a window through the
left press focus arm into IsActive is INFERRED. No separate raise on press call
was seen in the Window mouse handlers read.

### Modal block

STATIC. Window query `0x00923090` compares itself with
`0x00923020(window)`. When they differ and its own hit walk finds something, it
sets a blocked out byte and `0x0091A4E0` returns null at once. The router then
returns before any route and does not update the hovered element, so it sends
no event and no leave.

The block fires for any Window that is not its own blocker and has content
under the point, above or below the blocker in draw order. It does not fire when
no Window walk finds anything. The query then returns the desktop, so a modal
window does not by itself block a point that hits no Window. What the desktop
or world does with that event is UNKNOWN. Popups register in the same
container, but their query is not `0x00923090`, so this test does not run for
them. Whether a popup escapes a modal block in practice is UNKNOWN.

Blocker choice `0x00922E10` scans modal group 0, then the registered group of
the window (`+0x438`), each from the end, for the first visible modal
(E+`0x3C8` bit 5). With none, the window itself is selected. Records are
ordered by SqwtModalOrder (E+`0x3D0`). SqwtModalGroup (`+0x3EC`) is
constructed with 0.

### Window input gate

STATIC. `SqwtIsEnableInput` is E+`0x3E0` bit 5, constructed true at
`0x009249F5`, setter arm `0x009259CC`. The
[client structs contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/sqwt-window-input-property.md)
owns the property. When false, these Window handlers mark the event handled:

| Handler | Slot | Function |
| --- | ---: | --- |
| PreviewMouseDown | 41 | `0x00922170` |
| PreviewMouseUp (sets `+0x19`) | 42 | `0x009221A0` |
| PreviewMouseMove | 32 | `0x009220B0` |
| PreviewMouseEnter | 60 | `0x009220E0` |
| PreviewMouseLeave | 62 | `0x00922120` |
| PreviewKeyDown | 37 | `0x009221F0` |
| PreviewKeyUp | 35 | `0x009221C0` |

These events tunnel, so descendants see them handled. The window still takes the
hit. Nothing here passes the point to a lower window. Wheel slots are not among
these overrides.

Window MouseLeftButtonDown `0x00922950` acts only on an unhandled event whose
original source is the window (drag or resize start, a conditional preferred
pointer, then handled). `0x00922A50` ends it. Element self test `0x0092B640`
has no read of IsEnabled (E+`0x134`), so that body does not drop a disabled
element. Its helpers, including `0x00931D20`, `0x00926D40` and the rectangle
shaping at `0x0067AE10`, were not traced. Hit shaping by skin HitMargin is
UNKNOWN.

## Disabled rule

STATIC. Interface slot 3 is `0x00939E30`. Effective enabled is the InputElement
bit read by `0x0091C3F0` and E+`0x134` bit 0 on the element and each parent, up
to global `0x013597F8`. Router `0x0091E980` calls it on the target
(`0x0091EBBB..0x0091EBE2`). When false it sets A+`0x19`, A+`0x14` and
A+`0x15` to 1 before routing. A press or release on a disabled element, or a
descendant of one, travels the whole route already handled. Button press,
Button release, ListBox release and the focus arm test handled and do nothing,
so there is no click, selection or focus change.

Handled does not stop delivery. Handlers without a handled test still run:
IsMouseOver, ButtonBase MouseLeave (which clears IsPressed), the MouseEnter
skin recompute, Selector GotKeyboardFocus, the Selector MouseLeave index write
and the Selector Operate arm. The last is a command route and not this mouse
router.

## Enter and leave

STATIC, router `0x0091E980`. Global `0x01357028` holds the element under the
pointer. Old and new are compared on each router call that gets a target, and C
is their nearest common ancestor (`0x0091C5C0`).

1. Before the main event, if old differs from new and old and C exist, the
   router lists old and its parents up to but excluding C. It keeps those whose
   interface slot 19 is true (`0x00939E80`, local IsHitTestVisible, E+`0x15A`
   bit 5). The handled bytes for these come from the enabled test on the old
   target (`0x0091EA95..0x0091EB13`).
2. The main event follows, with handled reset from the enabled test on the new
   target.
3. After it, unless the stop byte is set, the same kind of list from new up to C
   is sent, with handled reset from the new target
   (`0x0091EC17..0x0091ED56`).
4. Each list is built leaf first and reversed (`0x00932760`) for the preview
   pass. Preview events run outermost first and the ordinary events leaf first,
   for both leave and enter.
5. `0x0091E360` sends each listed element its own event with source fields from
   that element, then routes it normally. Parents also see bubbled MouseEnter
   and MouseLeave. Handlers that test for their own original source react to
   their own events.
6. With no old element, C is null and neither list is sent.

IsMouseOver: PreviewMouseEnter `0x0093A410` sets E+`0x15F` bit 5 when the
original source is the element, and PreviewMouseLeave `0x0093A480` clears it.
Neither tests handled. Every listed, locally hit test visible element on the
entered chain becomes IsMouseOver, disabled elements included. The stores are
also recorded in
[render transform](../resource/sqwt-render-transform.md). Removing a hovered
subtree resets the hovered global to the desktop
(`0x0091DAE3..0x0091DB12`). The removed elements get no leave from that arm.

MouseEnter `0x0092A790` (own source) requests a skin recompute before its
handled test. MouseLeave `0x0092A810` (own source) requests recompute 0 when
unhandled. Whether or not handled, it requests 0 again when the current status
is 4 (`0x0092A875..0x0092A8A1`). Both bodies call `0x0090F560` with a pointer.
That this selects a cursor is INFERRED. No keyboard focus request was seen in
the FrameworkElement, Button or Selector enter and leave bodies.

Selector MouseEnter `0x0096FBB0` writes MouseEnteredIndex only when unhandled.
MouseLeave `0x0096FC40` writes it with no handled test. The value is the row
index of the event source, or -1 outside a row. GotAnchor `0x0096FAA0`
(unhandled only) writes AnchoredIndex. Mouse driven anchor behaviour is
UNKNOWN.

## SqwtSkinStatus

STATIC, `0x00930C20..0x00930CB1`. The value is recomputed from state. The
argument matters only when it is 5. The step is skipped when the object at
E+`0x1FC` exists with `+0xD0` bit 5 clear. That this is
SqwtIsAutoSetSkin false is INFERRED. The first matching row wins.

| Order | Condition | Value |
| --- | --- | ---: |
| 1 | local IsEnabled, E+`0x134` bit 0, clear | 2 |
| 2 | request is 5 | 5 |
| 3a | E+`0x2A0` bit 0 set, bits 1 and 4 set, IsMouseOver | 5 |
| 3b | E+`0x2A0` bit 0 set otherwise | 1 |
| 4 | bit 3 set and IsKeyboardFocusWithin (E+`0x15D` bit 5) | 3 |
| 5 | IsAnchorWithin (E+`0x160` bit 5) | 6 |
| 6 | IsMouseOver (E+`0x15F` bit 5) | 4 |
| 7 | otherwise | 0 |

Row 1 reads only the local IsEnabled. This function does not give value 2 to a
child of a disabled ancestor.

E+`0x2A0` bit 0 is written by `0x00929980`. Its direct callers are the Button
IsPressed change (`0x0097EB70 -> 0x0097ECA0`), selector container selection
(`0x00961480`), `0x00964940` (call site not traced), two Window IsActive sites,
and Thumb left down and up (`0x009CA380`, `0x009CA4C0`). `0x00929980` requests
the current value again, so a current 5 persists until the next request, and
`0x0097ECA0` then requests 0 or 1. The only direct caller of the bit 1 setter
`0x0092A1A0` is the TabItem constructor (`0x009BB279`). `0x0092A5F0` sets bit 3
from skin slot 3 (SkinFocused) and bit 4 from slot 5 (SkinReserved), each when
the entry pointer and its first dword are nonzero.

INFERRED: status value N shows skin slot N, in the order 0 SkinNormal, 1
SkinActive, 2 SkinDisabled, 3 SkinFocused, 4 SkinHover, 5 SkinReserved and 6
SkinAnchored. The slot order and the `7 * group + condition` row lookup are
STATIC on their own pages. The published delivery from the status property to
that condition covers a retained literal on an admitted Label peer. An
automatic value on a Button peer was not traced. Whether `Skin="NULL"` on
SkinFocused sets bit 3 is UNKNOWN.

## Button and ToggleButton

STATIC. Press arming is on PreviewMouseLeftButtonDown (slot 43,
`0x0097EFF0`) and release on PreviewMouseLeftButtonUp (slot 44,
`0x0097F080`). Each requires an unhandled event and the local hit test bit.
Release snapshots and clears IsPressed, then clicks when ClickMode is 2 and the
snapshot was set. It makes no separate pointer over test.

STATIC. ButtonBase MouseLeave (slot 63, `0x0097F130..0x0097F18F`) clears
IsPressed (E+`0x3AE` bit 5) with no original source test and no handled test,
then runs `0x0092A810`. The ButtonBase enter slots 60 and 61 are the shared
handlers, which do not set it. So when a leave is delivered, dragging off a
pressed Button and back and then releasing raises no Click, and releasing on a
Button that was not armed raises no Click there. A modal blocked query sends no
leave. That leaving a hit visible descendant while staying inside a pressed
Button also clears IsPressed is INFERRED from the per element bubbling leave.
Moving from the Button into a descendant sends the Button no leave.

No preferred pointer acquisition was seen on an ordinary press. The published
ButtonBase route to that setter is the drag path, armed only when both
DragDrop.Id and DragDrop.SourceCategory are nonzero. The drag path does set the
preferred pointer. For a Button without those values, other elements keep
receiving enter and move while the mouse button is held. DragDrop defaults are
UNKNOWN, so that a given Button lacks them rests on its markup (INFERRED).
Thumb and draggable controls are not covered. Win32 capture is a separate
layer.

In slot 13 (`0x0097ECE0`) the primary slot 71 call at `0x0097ED20` precedes the
Command check at `0x0097ED22`. ToggleButton and CheckBox slot 75
(`0x0095D470`) invert IsChecked (E+`0x3B4` bit 5). RadioButton slot 75
(`0x00974640`) calls it only when unchecked. The checked change handler
`0x0095D2F0` calls slot 73 (SqwtImagePartsIndex 1, raise Checked
`0x0135A6F4`) or slot 74 (index 0, Unchecked `0x0135A6E8`). CheckBox slots 73
and 74 thunk to the same. Radio slot 73 (`0x00974AE0`) first unchecks the other
buttons of its group (`0x00974A00`). The checked look is the image parts index
and not SqwtSkinStatus.

Button Executed `0x0097EDB0` handles an unhandled ApplicationCommands.Operate:
it marks it handled (`0x0097EE41`), requests skin value 5 and raises Click.

## ListBox rows

### Selection on release

STATIC. ListBox, Selector and ListBoxItem use the FrameworkElement down handler
(slot 43, `0x0092EB90`). ListBoxItem overrides none of slots 43 to 52. Nothing
selects on press. ListBox slot 44 (`0x0096ED00..0x0096ED3B`) looks up the row
that contains the event source (`0x00960DE0`) when the release is unhandled,
and calls `0x0096EC00(index, 0, 1)` for a nonnegative index. It tests no
pressed state and no matching press row. A press elsewhere followed by a
release over a row selects that row. The row Button is then not armed, so it
raises no Click.

`0x0096EC00` writes SelectedIndex, where an equal value is skipped by the
setter, then raises Selection (`0x0135B518`, id `3B`) for any index other than
-1. SelectionChanged (`0x0135B5CC`, id `3C`) comes only from the SelectedIndex
change callback. So SelectionChanged fires on an index change and Selection on
every such release. PreviewMouseLeftButtonUp tunnels, so the ListBox raises
both before the row Button inside it sees the release. `0x0096ED00` does not
set handled.

For items source lists `0x00960DE0` returns the index of the matching item, or
of the generated container (vector at accessor `+0x3D0`) that is the source or
contains it (`0x00929800` recursive walk, index math `0x00965790`). That a
SystemMenu row Button and its content are found by this walk is INFERRED.
Whether a row Button covers the whole row hit area is UNKNOWN. A release on a
part of the row container outside a disabled Button would not be pre handled by
that Button.

### Focus on press

STATIC. The left press focus arm runs `0x0092EA10` only when the event is
unhandled and the element is the original source. A type tested arm at
`0x0092EA4A` is separate. On the ordinary non desktop arm
(`0x0092EA90..0x0092EB82`):

- A leaf that is enabled, Focusable and passes primary `+0xE4` (IsTabStop)
  takes focus.
- A leaf that is enabled and Focusable but not a tab stop tries its first
  focusable descendant (mode 9, `0x0092E020`), then the ancestor search
  `0x0092B150`.
- A leaf that is not enabled or not Focusable runs only the ancestor search.
- The ancestor search needs enabled, Focusable, visible and tab stop. With no
  ancestor, an owner fallback (primary `+0xB0`, mode `0x0F`) is tried.

MEASURED: the main menu form declares IsTabStop False on both lists and the row
DataTemplates carry an IsTabStop False Setter. That the row focus target is
`Button_Command` is INFERRED. Default Focusable and IsTabStop values of the row
parts and the actual recipient of a press are UNKNOWN. Selector
GotKeyboardFocus `0x009640B0` writes SqwtFocusedIndex (descriptor
`0x0135AB38`) with the row of the focused source
(`0x009640D6..0x00964149`), and LostKeyboardFocus `0x00964160` writes -1 once
focus is no longer within. That the keyboard cursor moves to the row on press
is INFERRED.

### SystemMenu markup

MEASURED. `widget/MainMenuWidget.tpl`, raw carrier 13335 bytes, SHA-256
`82a503ee4f4faa7b6b329f4295a1158aa194a1dea530333935093e328902deb2`.

- `Style_ListBox`, lines 189 to 195: EventTrigger `SelectionChanged` runs
  `UILuaCommands.SelectionChanged`, CommandParameter
  `{CommandParameter Int, %s}`, PropertyName1 `SelectedIndex`.
  `ListBox_SystemMenu` (form line 29) and `ListBox_MainMenu` use this style.
- `DataTemplate_ListBoxItem_System`, lines 152 to 166: one `Button_Command`
  with no Command, CommandParameter or ClickMode. IsEnabled binds
  `@SystemEnable`, and the icon and text have IsHitTestVisible False.
- Window triggers play sound 3 on MouseEnter and GotKeyboardFocus from a
  Button, and sound 4 on Click and on ApplicationCommands.Operate from a
  Button.

In this form and template the only command a row drives is
SelectionChanged. Other global styles and templates were not exhausted, and
window level triggers such as Activated are separate. The
[main menu route](../script/main-menu-command-route.md#selection) records that
the script arm calls `processUICommandSelectionChanged` and writes
SelectedIndex -1. That write raises SelectionChanged again with -1, which the
same arm drops, so a later click on the same row changes the index again.
Whether the Lua callback completes before the Button release is UNKNOWN. A
locally disabled `Button_Command` under the pointer pre handles press and
release, so there is no row selection, press focus move or Click.

### Operate on a source row

STATIC. Selector Executed `0x0096ED40` runs its base, then for
ApplicationCommands.Operate (`0x01357C88`) looks up the row of the event source
and, when found, executes `Selector.SelectIndex` (`0x0135B530`) with a kind 1
record on itself (`0x0096ED83..0x0096EE02`). The Operate branch has no handled
test. The SelectIndex branch requires an unhandled kind 1 event, calls
`0x0096EC00(index, 0, 1)` and marks it handled. Button Executed marks Operate
handled and raises Click, and the Executed walk ends only on the stop byte.
That Operate on a focused row Button both clicks it and selects the row,
provided no callback sets the stop byte, is INFERRED. Delivery of the key to
that Button is a separate condition.

## Right button

STATIC. Across the 130 72 slot tables of the vtable catalog whose slot 15 is
the preferred pointer setter, slots 46, 51 and 52 hold only the InputElement
forwarders, and slot 45 holds the forwarder or the thunk `0x00929F40` to it. The
dedicated right button slots therefore add nothing on these classes. Right input
also produces the generic MouseDown and MouseUp pairs. Control MouseDown has
the double click raise below. The generic MouseDown overrides in
`TextBoxBase::TextBoxScrollInfo` (slots 47 and 48) and the application
`MainElement` (slot 47), and application slot 13 overrides, were not read.
Stored handlers, triggers and input bindings still receive the events.

MEASURED. All 1123 `.form`, `.tpl`, `.style` and `.xml` files under
`client/sqwt` were decoded. None contains `RightClick` or `MouseRightButton`.
The only MouseBinding `MouseAction` values are `LeftClick` (18) and `LeftUp`
(7). World right click behaviour outside SQWT widgets is UNKNOWN.

## Double click

STATIC. Control MouseDown `0x00936F70` raises PreviewMouseDoubleClick
(`0x013590E8`) and MouseDoubleClick (`0x013591A8`) on itself when A+`0x44` is
2, the source A+`8` is itself and IsHitTestVisible is set. MEASURED: no decoded
markup file subscribes MouseDoubleClick or LeftDoubleClick. STATIC: the
`RAPTURE` window class is registered with style `0x0B`, which includes
`CS_DBLCLKS` (`0x00402A71`, RegisterClassExW at `0x00402AD3`). That the mouse
callback is attached to that window is INFERRED.

INFERRED from the callback and provider tables: the second press of an operating
system double click arrives as `0x203`, which sets current and alternate on but
not primary on, while the producer makes downs from primary on. That press would
produce no MouseDown unless it shares a sample with the first press, so it would
arm no Button, while a ListBox would still select on its release. The live
result is UNKNOWN. Neither a delivered double click nor a lost second press is
established.

## Focus movement codes

STATIC. FrameworkElement Executed `0x0092D1F0` handles
ApplicationCommands.MoveFocus (`0x01357968`) with its byte parameter, through
interface slot 25 (`0x0091CD60`) and slot 26 (`0x0092E650`, jump tables
`0x0092E830..0x0092E87B`). Codes 1 to 4 are directional, 9 is the first
focusable descendant (`0x0092E020`), 10 the nearest focusable tab stop ancestor
(`0x0092B150`), 11 focus history (`0x0091D030`), 13 and 14 self when able else a
descendant, and 15 logical focus. Codes 7 and 8 (arm `0x0092E7D9`) and 12 (arm
`0x0092E7BA`) were not traced. ListBox slot 26 `0x00961760` answers 15 with the
SqwtFocusedIndex row under flags, and DesktopWindow `0x0091A140` returns nothing
for 9.

## Enter and a default button

STATIC. The executable holds no ASCII or UTF-16LE `IsDefault`, `IsCancel` or
`DefaultButton` text. Window does not override interface KeyDown slot 38
(shared `0x0091F440`). The Window, DesktopWindow and RaptureElementContainer
Executed bodies (`0x00926320`, `0x0091A170`, `0x004D9D70`) contain no direct
compare with the Operate descriptor. MEASURED: no decoded markup contains those
three names.

INFERRED, as a conclusion from absence: there is no native form default button.
Enter is data. The `key_config` rows bind it to LogControlPressKey,
ApplicationCommands.Operate and MoveFocus 9, as the
[main menu route](../script/main-menu-command-route.md#decide-and-chat-input)
and [key wait](../resource/logcontrol-key-wait.md) pages record. Operate goes to
the keyboard focus route. MoveFocus 9 asks for a focusable descendant and is not
a button search. The order and handled interplay of the three Enter rows, and
the effect of `key_config` row 251 (`LeftClick` to MoveFocus with no
parameter), are UNKNOWN.

MEASURED: `NamePhase.tpl` (raw carrier 16255 bytes, SHA-256
`74885fb996a8d41daa8b714c8ee02d7ab80c3f2f311313e3185d4c387d3c80a3`) binds
Enter on its name TextBoxes to MoveFocus 10. LogWidget and ItemSearchWidget bind
Return to script PressEnter commands.

## Character creation forms

MEASURED. Under `system/bootup/charactercreation`, excluding `.sd.`, `.de.`,
`.fr.` and `.ja.` names, there are 88 `.form` and `.tpl` files (48 Window roots
and 40 ResourceDictionary roots). Element tags count 353 Button, 15
ListBoxItem, 6 CheckBox, 4 ListBox and 2 TextBox, with no MouseBinding.
EventTrigger RoutedEvent counts include 38 MouseEnter, 36 Click, 28 GotAnchor,
11 LostAnchor and 6 MouseLeave.

STATIC. The concrete DesktopWindow subclass with these input tables in the RTTI
catalog is `Application::Main::RaptureElementContainer`. The
`Application::Main::Menu::Bootup::CharacterCreation` classes RacePhase,
NamePhase and CharacterCreationBasePhase have only 2 and 22 slot tables. RTTI
also holds `Sqwt::EventHandler` and `Sqwt::RoutedEventHandler` instantiations
over CharacterCreation classes. Their subscription sites were not traced. That
the phase classes are not elements, and that the creation forms are SQWT windows
under this desktop so the sections above apply, are INFERRED. How the phase
classes receive Click and Command results is UNKNOWN.

## Open items

- Popup hit layering, including whether popups escape a modal block.
- Hit shaping by HitMargin and the untraced hit test helpers.
- Mouse driven anchor behaviour, and whether `Skin="NULL"` sets skin bit 3.
- Default Focusable, IsTabStop and DragDrop values per class.
- What the desktop or world does with a point that no Window takes.
- Raise paths beyond those listed, the `0x00964940` skin bit caller and
  MoveFocus codes 7, 8 and 12.
- No live client behaviour was observed.

## Reproduction

1. Check the executable digest.
2. Read 88 bytes at `0x0091D818` and the dword table at `0x0091D760`, then the
   virtual offset of each arm. For names, find every call to `0x0097F6C0` and
   read the pushed literal, the id and routing byte stores and the descriptor
   in ECX.
3. Build a slot matrix from `config/ffxivgame.vtable_slots.jsonl` for the 72
   slot tables of InputElement, FrameworkElement, ButtonBase, ListBox,
   ListBoxItem, Window and DesktopWindow.
4. Disassemble `0x0091E980`, `0x0091E1C0`, `0x0091E360`, `0x0091D210`,
   `0x0091A4E0`, `0x0091AF80`, `0x00927890`, `0x00923090`, `0x00922E10`,
   `0x009238A0`, `0x00939E30`, `0x0093A410`, `0x0093A480`, `0x00930C20`,
   `0x0097F130`, `0x0097ECA0`, `0x0096ED00`, `0x0096EC00`, `0x0096ED40`,
   `0x009640B0`, `0x0092EB90`, `0x0092EA10`, `0x00936F70`, `0x0092E650`,
   `0x00402A30`, `0x00923100`, `0x009237D0`, `0x00923D10`, `0x0092A790`,
   `0x0092A810`, `0x00929980`, `0x0092A5F0` and `0x00932760`.
5. Read the Window setter arms at `0x009258EB..0x00925A50`. Property names come
   from calls to `0x009108E0`.
6. Decode the cited markup, check sizes and hashes and read the cited lines.
