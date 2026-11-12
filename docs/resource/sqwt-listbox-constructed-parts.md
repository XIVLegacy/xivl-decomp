# Sqwt ListBox constructed parts

`ListBox` constructs an embedded `VirtualizingStackPanel` and
`ListBoxScrollViewer`, supplies the panel to `ItemsPanel`, and names the viewer
`PART_ListBox_ScrollViewer`. The panel constructor initializes an axis byte
whose height-axis consumers support a conditional vertical stacking rule.
The embedded viewer gives the item panel a clipped viewport and applies a
changed scroll offset to the panel origin. Effective `TabPanel` placement
and painted gaps remain UNKNOWN.

## Input and method

| Field | Value |
|---|---|
| Binary | `ffxivgame.exe` |
| Client | FINAL FANTASY XIV 1.23b, build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| PE image base | `0x00400000` |
| Tool | Ghidra through `ghidra-cli 0.2.2` |

All addresses are VA. RVA is VA minus `0x00400000`. Ghidra disassembly,
focused decompilation and referenced data were checked against section-mapped
bytes of the identified PE. STATIC labels instruction and data observations.
INFERRED labels their interpretation. UNKNOWN labels an unresolved boundary.
This is static evidence. Runtime outcomes are UNKNOWN.

The [parts study](sqwt-tab-scroll-text-parts.md#classes-and-layout-slots)
owns the inherited layout-slot roles and class chains. The
[class metadata method](class-metadata.md#method) describes the RTTI and
constructor joins used here.

## Constructed members

The `ListBox` constructor is `0x0096D980`. At `0x0096DA46` it forms
receiver `+0x4B0` and calls `0x009B8660` at `0x0096DA4E`. It forms
receiver `+0x7E0` at `0x0096DA6A` and calls `0x0096D140` at
`0x0096DA73`. The constructors install the primary vftables below. STATIC.

| Member offset | Constructor | Primary vftable | Complete-object locator | Type descriptor |
|---|---|---|---|---|
| `+0x4B0` | `0x009B8660` | `0x0107F7B4` | `0x011816B0` | `0x012E3C64` |
| `+0x7E0` | `0x0096D140` | `0x01071F74` | `0x0117CF34` | `0x012E456C` |

The first type descriptor has the name
`.?AVVirtualizingStackPanel@Controls@Sqwt@@`. The second has
`.?AVListBoxScrollViewer@ListBox@Controls@Sqwt@@`. Each vftable's preceding
pointer selects its complete-object locator, whose `+0x0C` pointer selects
the type descriptor. The name starts at descriptor `+8`. STATIC.

The viewer constructor receives the panel pointer as its first stack argument.
Its second argument is the panel's `+0x300` interface address, or zero when
the panel pointer is null. The interface vftable is `0x0107F5F4`, whose
locator `0x011817D0` records object offset `0x300`. The viewer stores the
second argument at viewer `+0x3B8` at `0x0096D1BA`. The outer constructor
passes the viewer back to interface slot `+0x24` at `0x0096DB9E..0x0096DBAE`.
The slot selects `0x009B78F0`, which stores it at interface `+8`. STATIC.

## Content, Name and ItemsPanel joins

At `0x0096D19C`, the viewer constructor supplies its first argument to
setter `0x00934FF0`. At `0x0096D1E5`, it calls `0x00912D50` with the
string `PART_ListBox_ScrollViewer` at `0x01071C0C`. At
`0x0096DCBB`, the outer `ListBox` constructor passes the same embedded
panel to `0x00963540`, which selects outer storage `+0x35C`. STATIC.

The setters select these descriptors. Each initializer passes the named string
to `0x009108E0`, which stores the string pointer in the descriptor. The joins
establish `Content`, `Name` and `ItemsPanel` identities independently of
decompiler-assigned names. STATIC.

| Property | Setter | Descriptor | Initializer | Name string |
|---|---|---|---|---|
| Content | `0x00934FF0` | `0x01358F94` | `0x00F221E0` | `0x0106B16C` |
| Name | `0x00912D50` | `0x01356BC8` | `0x00F20620` | `0x01068310` |
| ItemsPanel | `0x00963540` | `0x0135ABEC` | `0x00F23D50` | `0x0106F7D8` |

## Constructed item viewport

The outer constructor inserts its embedded viewer as a visual and logical
child at `0x0096DB8E..0x0096DB99` through `0x00940B30` and `0x009313C0`.
This is separate from the outer ListBox Template property. STATIC.

The viewer inherits the ScrollViewer constructor at `0x0096D16A`. The
base constructs a Grid at viewer `+0x3C0`, a ScrollContentPresenter at
`+0x760` and a template at `+0xA5C` with that Grid as its root
(`0x009733BE..0x00973419`). Viewer table cell `0x01071FA0` selects
`0x0096D600 -> 0x00971EF0`, which adds the presenter to the Grid
at `0x00971FD1..0x0097200C` and assigns the template through
`0x00938170` at `0x00972054`. STATIC.

The [template application path](sqwt-progress-template-style.md#conditional-style-inheritance)
attaches the Grid and calls virtual `+0x10C`. Viewer cell
`0x01072080` selects `0x00934A50`. With unparented content, it reads
the viewer Content at `+0x340` and calls `0x0092B340` with the default
content-source name. That lookup searches logical children for a
ContentPresenter with matching ContentSource and does not descend into
child ContentControls. The constructed presenter defaults to `Content`
at `0x0099D6BF..0x0099D6DE`. The callback supplies the panel to the
presenter through `0x0099CEE0` or `0x0099CF10` at
`0x00934AB8..0x00934AC3`. Presenter callback `0x0099D0A0` then
attaches the supplied panel at `0x0099D0E0..0x0099D110`. STATIC for
the branches and callsites, INFERRED for their default viewer composition.

The ScrollContentPresenter inherits the default ClipToBounds and
[render-size clip path](sqwt-number-input-button-frames.md#hand-in-viewport-evidence).
Its arrange keeps its final viewport separate from the child extent
(`0x009C2EC0`). The viewer arrange forwards the presenter's
`ViewportHeight` and `ViewportWidth` at viewer `+0xA24` and `+0xA2C`
to the panel interface `+0x70/+0x74` at `0x00970E1A..0x00970E3C`.
Those fields belong to presenter descriptors `0x013633E4` and
`0x01363408`, named at `0x00F2A9C0` and `0x00F2A9F0`.
Panel interface entries `0x009B79F0` and `0x009B7A20` store changed
values at interface `+0x20/+0x24`. STATIC.

In the installed standard `widget/ItemListWidget.form`, line 65 sets
`ListBox_TabItem_1` Height to 200, sets horizontal scrolling to Disabled
and selects `ControlTemplate_ListBox`. Its matching definition in
`widget/ItemListWidget.tpl` lines 64-65 declares a bare ContentPresenter.
The encoded form and template SHA-256 values are
`65a8c82bc93664231778fe1b704b27254e03a759f4ec37fa23e6ae2f516a46be` and
`ba8b86dbf36558e6086721524833d17528be08b11bf72c062d0b76fd318de7d5`.
MEASURED from the decoded installed resources. The viewer's constructed
viewport bounds the row subtree independently of the outer presenter.
INFERRED. Exact scissor dimensions and initial row cut remain UNKNOWN.

## Bound row focus and viewport

Assigning ItemsPanel to a VirtualizingStackPanel saves the panel and its
scroll owner at ItemsControl `+0x3F0/+0x3F4`
(`0x0096320F..0x00963267`). The `SqwtFocusedIndex` descriptor
`0x0135AB38` has index 9, initialized at `0x00F23E80`. Callback
`0x00963140` uses branch `0x009632DD` for that index. With a
nonnegative request different from the index of the current focus object,
it calls `0x009627B0` with the requested index and flag 0. A nonnull
ItemsSource at `+0x354` selects `0x00962440`. STATIC.

For an eligible bound row without a retained container, and for a retained
container whose `+0x15D` mask `0x20` is set, the checked branches at
`0x009624EC..0x009624F8` and `0x00962567..0x00962573` call the
scroll request at `0x00961E40` only if `0x0091B7B0` returns 1.
That predicate requires a current visible
[AnchorElement](sqwt-window-trigger-order.md#attachment-and-conditional-focus-notifications).
The branch at `0x00962580` can also reach the request through
`0x00962350`, with the same predicate. The row, focus and caller flag
checks in `0x00962440` qualify these branches. STATIC.

For a valid nonnegative vertical index with bound source, panel and
scroll owner, `0x00961E40` obtains the cumulative row start and end by
summing preceding cached extents through `0x00961550` for the index and
index plus one. It reads vertical offset and viewport height through
panel interface `+0x28/+0x2C`. For ordinary finite inputs, the first
branch requests the row start when it is less than or equal to the
current offset. Otherwise it requests end minus viewport height when the row end
is past the viewport bottom. Both requests call interface `+0x6C` at
`0x00961F88` or `0x00961FDA`. The panel cell selects `0x009B7D00`,
which bounds an ordinary finite request to the scrollable extent with a
floor of zero, writes a changed offset at interface `+0x1C` and
notifies the scroll owner. STATIC. Admission depends on an eligible focus
request and a visible anchor.

The viewer publishes changed offset and viewport through
`0x00973D40 -> 0x00973660`. ItemsControl subscribes to ScrollChanged
at `0x00963267..0x00963284` and the handler at `0x00963020` copies
the vertical values to `+0x470/+0x474`. The update at `0x00964940`
requires nonzero viewport height to process containers. With `+0x47C`
mask `4` set, its finite comparisons use the viewport to skip
offscreen rows, except for the first row, last row and focused index
plus its immediate neighbors (`0x00964C82..0x00964CF5`). The constructor
sets that mask at `0x00963B7F..0x00963BA2`.
`SqwtDesignData.StringValue2` can change it through `0x00960410`.
These are conditional container-management rules. The effective mask after
styling and actual row focus for the installed form remain UNKNOWN.

## ShopBuy first-tab focus requests

The [standard ShopBuy carrier and binding](sqwt-xpath-data-sources.md#carriers-and-script-feed)
identify `widget/ItemListWidget.form` and the English ShopBuy LPB
`client/script/n1635q/9rz/r2vu8pln1635q.le.lpb`. The latter is
22929 bytes, SHA-256
`dc669e4277d43549f806925b5ada10f0be3915b03e01d02836146c70e939e227`.
Its decoded chunk is 22916 bytes, SHA-256
`d7c50c4d7de9eab15430868cb514d6407649c18cdc40cbc441e2af4dca6d7e16`.
PCs are zero-based Lua 5.1 instructions. The linked LPB replay gives the
decode method. The instructions were read with unluac 2025-12-23.

MEASURED. ShopBuy `setInitialData` (`main/f2` PCs 35 to 36) sets
`work.listbox` to 1. `getListBoxName` (`main/f14` PCs 0 to 3)
returns `ListBox_TabItem_1` for 1. `updateListFocus` (`main/f6`)
first requires `work.updatecount` equal to 0 (PCs 0 to 5). In the
nonempty branch it caps `work.focus` at focus count minus 1, then
writes `SqwtFocusedIndex` on that named list, calls `setWindowFocus`,
and calls `setFocusedIndex` with the same list and index
(PCs 43 to 82). The inherited `setFocusedIndex` in
`widget/widgetbaseclass_common` `main/f114` PCs 0 to 4 writes
the same property. The [native property path](https://github.com/XIVLegacy/xivl-client-structs/blob/8a8930c6da61d1ff8b3d442fe169396805394d23/docs/native-input-ui-boundaries.md#visibility-property-path)
describes the generic named control and integer property write.

MEASURED. ShopBuy `setWindowFocus` (`main/f8` PCs 0 to 14) rejects a
nil or empty control name. It sets logical focus, then requests keyboard
focus for the named control only when
`desktopWidget:_getKeyboardFocusedWidget()` returns self. Common
`main/f162` writes `LogicalFocus`. Common `main/f161` calls
`_setKeyboardFocusedControl(nil, name)`, using the
[default container path](https://github.com/XIVLegacy/xivl-client-structs/blob/8a8930c6da61d1ff8b3d442fe169396805394d23/docs/talk-native-contract.md#keyboard-focus).

STATIC. The [bound focus path](#bound-row-focus-and-viewport) joins the
property write to a scroll request only through its eligible branches.
For caller flag 0 and ItemsControl `+0x15D` mask `0x20` clear,
`0x00962440` instead checks the stored index at `+0x388` through
`0x009624C1 -> 0x00962350`. That helper also uses `0x0091B7B0`.
This predicate tests only nonnull global `0x01356F34` and
`AnchorElement+0x158` equal to 2. The keyboard-focus interface and
anchor-event recipient are separate globals. The
[keyboard binding path](sqwt-window-trigger-order.md#attachment-and-conditional-focus-notifications)
documents a conditional AnchorElement visibility writer. The focus
request and the index write alone do not establish that the anchor is
visible on this invocation.

## Layout0 vertical keyboard focus

MEASURED. The game master's `key_config` definition `0x01030169` uses
data, enable and offset resources `0x0103016A`, `0x0103016B` and
`0x0103016C`. Decode the fields using the
[retained source definitions](https://github.com/XIVLegacy/xivl-client-data/blob/e34b3da2cfa4bd023f029eae9af31413ab63dbe8/docs/movement-binding-rows.md#retained-source-definitions).
The replayed `key_config.csv` is 63816 bytes with SHA-256
`9e98035b2775a07f17df82ce391e95f89b1d900e993279135ca78f50a6105c1b`.
The primary focus entries have these zero-based column values.

| Row | Column 3 default | Column 8 | Column 9 | Column 10 | Column 11 |
| --- | --- | --- | ---: | ---: | --- |
| 1013 | `KeyUpFocus1` resolves to `UP` | `ApplicationCommands.MoveFocus` | 1 | 2 | True |
| 1031 | `KeyDownFocus1` resolves to `Down` | `ApplicationCommands.MoveFocus` | 1 | 4 | True |

In both rows column 2 is 0, column 4 is `**`, and columns 5 through
7 are 0, True and False. The defaults therefore qualify ordinary
unmodified KeyDown and repeat bindings. Earlier rows 1007 and 1025
carry the same default keys for `UILuaCommands.PushSlotPrev` and
`UILuaCommands.PushSlotNext`. See
[Main Menu key bindings](../script/main-menu-command-route.md#key-bindings)
and the
[default field groups](https://github.com/XIVLegacy/xivl-client-data/blob/e34b3da2cfa4bd023f029eae9af31413ab63dbe8/docs/movement-binding-rows.md).

STATIC. The enabled range `(1001, 48)` feeds group 1 at
`0x00553397..0x005533A5`. Profile 0 selects `key_config`.
Registration of a non-Desktop form root with a resolved indexed
container and nonnull container root assigns the binding provider at
`0x005387AC`. For that root,
`0x005472F7..0x00547310` selects group 1, subject to current profile
`[0x01336BC4]` and route delivery. The type-0 emitter formats the
integer at `0x00551CFE..0x00551D42`. The column-11 flag reaches the
test at `0x00551DB6`, which appends `SqwtShowAnchor=True` from
`0x00FA1DA8` when the byte is 1. The parser
`0x009483B5..0x009483E5` writes markup `+0x16B`. The ordinary
KeyBinding constructor call at `0x009475B7` passes the option into
the binding flags at `+0x9C`, where mask `0x02` carries it.
The [configuration and provider trace](../input/player-movement-command-origin.md#configuration-text-and-substitutions)
and the
[ordinary KeyBinding parser and matcher](sqwt-command-parameter-consumer.md#actionmenu-keyboard-aliases)
give the construction and eligibility gates.

STATIC. The keyboard walker samples that binding mask and calls command
slot `+0x08` at `0x00921233`. For `MoveFocus`, the RoutedCommand
implementation `0x00927FA0` returns the handled byte of the last
event it dispatched. The
[PreviewExecuted dispatch](sqwt-command-parameter-consumer.md#executed-event-delivery)
visits ListBox input slot `+0x100` when eligible. The ListBox
constructor installs input table `0x010720EC` at `0x0096D9DE`.
Cell `0x010721EC` selects `0x009641E0`. That handler checks the
handled byte, complete ListBox `+0x47C` mask 4, nonnull ItemsSource
at `+0x354`, command `0x01357968`, and a nonnegative row index
resolved from the current anchor-event recipient `[0x01356FCC]`
(`0x00964223..0x009642DC`). An eligible changed candidate calls
`0x009637A0` at `0x0096446F` for Down or `0x00964546` for Up,
then sets the command handled byte. The setter updates
`SqwtFocusedIndex` at `+0x388` and can invoke its callback
synchronously through `0x00926E80 -> 0x00953BA0`.

STATIC. Only after the command returns exactly 1 and the binding mask
is set does `0x00921235..0x00921246` call `0x0091B530` with 1.
A nonnull AnchorElement then receives a Visibility 2 request at
`0x0091B54E`. The index-based scroll inside the command samples the
anchor before that request. Existing visibility, earlier bindings,
the preview and gesture gates, and the accepted focus target qualify
the join.

### Realized focus after the visibility change

STATIC. The
[AnchorVisibleChanged route](sqwt-window-trigger-order.md#attachment-and-conditional-focus-notifications)
can deliver the visibility change to the current anchor-event recipient.
For ListBox, ListBoxItem and Button, input slot `+0x110` selects
`0x0092C2B0` from tables `0x010720EC`, `0x0107AE8C` and
`0x01073B84`. The ListBoxItem and Button constructors install
their input tables at `0x0099E188` and `0x00974454`. After the
holder refresh, the handler calls `0x0092B4B0` at
`0x0092C2D9` only if the anchor gate is open and input slot
`+0x10` returns nonzero. Those tables select `0x0091C4A0` for that
slot, which compares the receiver with the current keyboard-focus
interface `[0x01357020]`.

STATIC. `0x0092B4B0` supplies a local zero-origin rectangle with the
focused element's arranged width and height to its primary slot
`+0xAC` at `0x0092B51E`. For the checked Button, ListBoxItem, Grid,
VirtualizingStackPanel and ScrollContentPresenter tables that slot
selects `0x0092B530`. It adds the element's layout origin and
continues to its logical parent at `0x0092B553..0x0092B5B2`.
The constructed viewer's cell `0x01072020` instead selects
`0x00970EF0`, which can request a new vertical offset through the
panel scroll interface slot `+0x6C` at `0x00971077`. A focused
realized row with this current parent chain can therefore receive
a rectangle-based reveal after the visibility change. INFERRED for
the retained composition.

The later container pass can call `0x00962440(index, 1)` at
`0x00965175` if it runs with nonzero viewport height and a stored
nonnegative focus index from an ItemsControl with the focus-within
mask set. This is another conditional entry into the index scroll
path. If focus did not reach the requested row before
AnchorVisibleChanged, the rectangle path uses the actual focused
element. A first key's delivery, realized row, and resulting offset
remain UNKNOWN.

## Offset application and scrollbar geometry

STATIC. A changed panel offset notifies the viewer through its primary
`+0x11C` cell `0x01072090`, selecting `0x00971970`. The queue
drain at `0x00973ED7` calls `0x00973D40`, which samples the
panel interface and forwards the values to `0x00973660`.
The event constructor `0x00973FE0` uses descriptor `0x0135BBFC`
and copies viewer `+0x384` into event `+0x34`. The viewer handler
cell `0x010720C4` selects `0x00970E50`. If the event handled
byte through `+0x10` is clear, its source `+0x08` is this viewer,
content `+0x340` is nonnull and viewer byte `+0x3B4` mask `0x20`
is set, the handler negates the event's current vertical offset at
`0x00970E96..0x00970EA6` and passes it to setter `0x009C2C50`
on the embedded presenter at viewer `+0x760`. The base viewer
constructor sets that mask at `0x00973381..0x0097338B`.
The setter supplies presenter `+0x2EC` and descriptor
`0x01363498` to `0x009B9320`. The initializer at `0x00F2AAA0`
names it `VerticalOffset` from `0x01082800`.

STATIC. Presenter arrange calls `0x0099CA00` at `0x009C2FDA`.
The latter calls the presenter's virtual `+0x104` with the
child and its calculated rectangle. Presenter cell `0x01082AA8`
selects `0x009C29A0`, which takes the child rectangle's width and
height and reads the y origin from presenter `+0x2EC`
(`0x009C29D9..0x009C2A23`). Helper `0x0099C850` submits the
rectangle through `0x0093A870`. In the constructed content join
this translates the panel by minus the current vertical offset
inside the separate clipped presenter. INFERRED for the composition
and resulting coordinate. The retained row-start override producer
`0x00995ED0` writes the thread-local child entry at `+0x10` in the
same `+0x2154` table read during panel arrange. New containers call
it at `0x00964F7A` with their accumulated row start.
`0x00961250` also supplies cumulative row starts at `0x0096132E`.
Thus the two placements are local row start and translated panel origin.
The complete frame order and hit target remain UNKNOWN.

STATIC. The embedded ListBox viewer constructor requests vertical
visibility mode 0 through `0x009730D0` at `0x0096D1A1..0x0096D1B1`,
using descriptor `0x0135BE6C`. Mode 0 is
[Auto](sqwt-tab-scroll-text-parts.md#scrollviewer-and-scrollcontentpresenter).
The template builder `0x00971EF0` appends one default star definition
and one Auto definition to each Grid axis
(`0x00971F1C..0x00971FCC`), then appends the presenter without a
cell override (`0x00971FD1..0x0097200C`). The lazy vertical bar
`0x00972500` appends its bar to the same Grid's children and assigns
column 1 at `0x009725B2`, through
[the Grid column setter](sqwt-number-input-digit-grid.md#control-and-template-grid).
The default row remains 0. The presenter therefore uses the star
column and the vertical bar uses the Auto column to its right.
INFERRED for the relative placement. The first-tab carrier sets
horizontal mode Disabled and gives no local vertical override.

MEASURED. The first-tab ListBox names `LIB_basis`. Its declared vertical
part in `common/default.style` is `SCB_basis` (decoded lines 5726 to
5730). The [style file identity and parts](sqwt-tab-scroll-text-parts.md#identity-and-method)
cover the same source. It maps the bar's up and down buttons, vertical
thumb and vertical background at lines 5623 to 5631. The viewer installs
the ListBox part map at `0x0096D1FF`; its vertical entries at
`0x012E44F0..0x012E450F` give ListBox-specific style-key and part-name
pairs. The relative grid and style evidence gives no fixed pixel width
for the Auto column. Its effective width, exact scissor and pixel gap
remain UNKNOWN.

## Panel axis and local stacking requests

The panel constructor initializes `Orientation` storage at panel `+0x304`
from byte `1` at `0x009B86EE`, through helper `0x009B87F0`. The helper
reads the supplied byte at `0x009B87FE` and writes it at `0x009B8811`.
Setter `0x009B7F60` joins the same storage to descriptor `0x013624E4`.
Initializer `0x00F29BA0` names that descriptor `Orientation` from string
`0x0107F578`. STATIC. This is a constructor input and storage observation,
not a universal effective default or an enum text mapping.

The panel's primary vftable has arrange slot `54` equal to `0x009B81B0`
and measure slot `55` equal to `0x009B7FD0`. Both branch on byte `+0x304`.
Zero selects width-axis arithmetic and nonzero selects height-axis arithmetic.
The child desired-size components are `+0x128` for width and `+0x12C` for
height, also described in the
[Track request evidence](sqwt-track-rectangle-requests.md#scrollbar-measure-and-track-request).
Thus the constructor's byte `1` selects vertical arithmetic in these consumers.
STATIC for the branches and component reads, INFERRED for the vertical
designation.

The measure accumulators start at zero. In the height-axis branch at
`0x009B80C5`, the height accumulator adds child desired heights. For ordinary
finite inputs, the width accumulator is raised only by a greater child width.
When panel `+0x2F8` is nonzero, calls through the `+0x300`
interface can replace an accumulated result component. STATIC. The local sum
does not by itself define the final measure result in that case.

Arrange seeds both cursors to zero at `0x009B81DB..0x009B81E1`. For
unchanged nonzero `Orientation`, it requests available width and child desired
height, then advances the height cursor by a fresh read of child `+0x12C`
at `0x009B830B..0x009B831D`. There is no independent fixed-gap addend in
this increment or the measure height accumulation. STATIC. `Orientation` is
reread after the child arrange request, so axis stability is a condition of
the vertical stacking interpretation.

Before each admitted child's request, arrange calls `0x00995A90` at
`0x009B826D`. The helper looks up the child in a thread-local context table
at `+0x2154`, returns a found entry's float at `+0x10`, and returns zero
at `0x00995B09` when absent. For ordinary finite inputs, a nonzero return
replaces the selected cursor and either signed zero leaves it unchanged.
The comparisons at `0x009B8294` and `0x009B82B5` use double zero at
`0x00F62F70`. They also select replacement for infinity or unordered status.
A NaN can reach the cursor if execution proceeds with unordered status.
Exception-mask state and actual NaN execution remain UNKNOWN. STATIC for the
comparison and lookup paths.

For ordinary finite inputs, an unchanged nonzero axis and absent or zero
overrides, local arrange requests therefore begin at `y=0` and advance in
child desired-height steps without an independent panel gap. INFERRED. Child
desired sizes can already include other layout effects and are reread after
the request. Painted-content gaps, override producers, later axis writes,
effective `TabPanel` placement and all runtime outcomes remain UNKNOWN.
The measure cross-axis comparison at `0x009B80DD` skips replacement for
unordered status, so its finite rule is not an unrestricted mathematical
maximum over arbitrary nonfinite inputs.

## Reproduction

Select the identified executable in an analyzed Ghidra project. Focused
commands through `ghidra-cli` include:

```sh
ghidra disasm 0x0096D980 -n 250 --json
ghidra disasm 0x0096D140 -n 60 --json
ghidra disasm 0x00963540 -n 18 --json
ghidra disasm 0x0096DB8E -n 12 --json
ghidra disasm 0x009733BE -n 25 --json
ghidra disasm 0x00971FD1 -n 41 --json
ghidra disasm 0x00934A50 -n 45 --json
ghidra disasm 0x0099D0A0 -n 50 --json
ghidra disasm 0x00970DF0 -n 35 --json
ghidra disasm 0x0096320F -n 35 --json
ghidra disasm 0x009632DD -n 27 --json
ghidra disasm 0x009624EC -n 9 --json
ghidra disasm 0x00962567 -n 20 --json
ghidra disasm 0x00961EEF -n 65 --json
ghidra disasm 0x009B7D00 -n 49 --json
ghidra disasm 0x0091B7B0 -n 8 --json
ghidra disasm 0x00551DB6 -n 6 --json
ghidra disasm 0x0092121F -n 17 --json
ghidra disasm 0x009641E0 -n 64 --json
ghidra disasm 0x00964466 -n 9 --json
ghidra disasm 0x0096453D -n 9 --json
ghidra disasm 0x0092C2B0 -n 24 --json
ghidra disasm 0x0092B4B0 -n 31 --json
ghidra disasm 0x0097105A -n 9 --json
ghidra disasm 0x00970E50 -n 40 --json
ghidra disasm 0x009C29A0 -n 41 --json
ghidra disasm 0x009C2C50 -n 19 --json
ghidra disasm 0x0096D1A1 -n 9 --json
ghidra disasm 0x00971F1C -n 73 --json
ghidra disasm 0x00964C82 -n 44 --json
ghidra disasm 0x009B8660 -n 86 --json
ghidra disasm 0x009B7FD0 -n 150 --json
ghidra disasm 0x009B81B0 -n 120 --json
ghidra disasm 0x00995A90 -n 44 --json
ghidra disasm 0x00F23D50 -n 5 --json
ghidra disasm 0x009108E0 -n 30 --json
```

Use the setter and initializer addresses in the property table for the other
joins. Read referenced RTTI, strings and constants from their PE sections,
following the pointer recipe above. For the keyboard path compare row keys
1013 and 1031 by the linked definition and check the input and primary
table cells against the pinned PE. Compare the cited instructions and data
with the identified executable. Keep raw output, executable bytes and Ghidra
project state outside the tracked tree.
