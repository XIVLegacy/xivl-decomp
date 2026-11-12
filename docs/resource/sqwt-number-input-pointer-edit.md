# Sqwt NumberInputBox pointer edit

A short primary-button press and release on an eligible stock quantity
arrow selects the digit on MouseDown and requests the signed step on
Click. A separate MouseRepeat route can also request a step. An effective
change can supply the subscribed hand-in widget with its new count.

## Inputs and method

Target is FINAL FANTASY XIV 1.23b build 2012.09.19.0001, `ffxivgame.exe`,
PE32 i386, base `0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VAs. Evidence IDs NI-POINTER-ROUTE, NI-POINTER-EDIT
and NI-POINTER-COMMAND are STATIC. NI-POINTER-ASSET and NI-POINTER-LUA
are decoded client data. Ghidra CLI, Apple LLVM 21.0.0 and an independent
PE section reader establish the instruction and table identities.
Executable verification was static.

The [integer range](sqwt-number-input-box.md) page identifies the SQWT
inputs, names and exact container and decoded digests. The selected
`common/LABBasisNumberInputBox.tpl` subtree was decoded and re-encrypted
byte-exactly. The [hand-in script contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/event-item-select-widget-contract.md)
identifies `eventitemselectwidget.le.lpb` by the tracked encoded resource
path. The two base script identities are recorded in
`xivl-client-scripts:manifests/retail_lua_coverage.json` under
`widget/widgetbaseclass` and `widget/widgetbaseclass_common`. Their LPBs
and decoded Lua 5.1 payloads matched the manifest lengths and hashes.
The Lua findings use instruction PCs and absolute decoded-payload byte
positions from `xivl-client-scripts:tools/build_quest_contracts.py`.

## Short primary-button gesture

The [numeric callback](../input/sqex-mouse-numeric-callback.md) supplies
mask 1 to the primary-on and off fields for admitted cases `0x201` and
`0x202`. The [provider](../input/sqwt-rapture-mouse-device.md) and
[preview producer](../input/sqwt-mouse-preview-producer.md) own the
conditional feed and point-target join. On the admitted route with exact
mask 1 the named preview descriptors select the left-button methods.
This is a conditional static join from the callback state. The running
provider, callback installation and actual pointer sample remain outside
this trace.

Let C be the NumberInputBox, D the selected NumberLabel and B either
RepeatButton at D+`0xAA0` or D+`0xE54`. The [digit grid](sqwt-number-input-digit-grid.md)
establishes initial selection and visibility. The [button frame](sqwt-number-input-button-frames.md)
page establishes the arranged, clipped hit box. The
[child-first point query](sqwt-command-parameter-consumer.md#local-hit-flags-and-child-first-query)
can select a child whose own local hit flag is set even when an ancestor
has its local hit flag clear. RepeatButton shares that query path. The
following case assumes the selected arrow is visible, eligible and is
the retained recipient of both press and release, with no repeat or
double-click report and with the local handled and stop gates admitted.

| Input event | Dispatcher and RepeatButton table cell | Effect on the admitted path |
| --- | --- | --- |
| PreviewMouseLeftButtonDown, id `0x0D` | `0x0091D4BE`, `+0xAC`, `0x010798A8 -> 0x0097EFF0` | After the base call, a clear handled byte and true local IsHitTestVisible admit the IsPressed write. The eligible update sets bit `0x20` at B+`0x3AE`. |
| MouseDown, id `0x08` | `0x0091D514`, `+0xBC`, `0x010798B8 -> 0x00936F70` | The call at `0x00936FA5` invokes the ordinary handler store through `0x0091F4B0`. Its dispatch checks the owner, stop byte and store pointer at `0x0091F4B3..0x0091F4D5`. |
| PreviewMouseLeftButtonUp, id `0x0F` | `0x0091D4D0`, `+0xB0`, `0x010798AC -> 0x0097F080` | The base call precedes the handled and hit flag gates. The method snapshots IsPressed, clears it on the eligible update branch, and invokes `0x0097EBA0` at `0x0097F11E` for ClickMode 2 and the set snapshot. |
| MouseRepeat, id `0x1C` | `0x0091D424`, `+0xA0`, `0x0107989C -> 0x00998890` | After the base handler, a clear handled byte and exact mask 1 issue a separate Click through `0x009987C0` at `0x009988B2`. |

The generic producer submits the preview down at `0x0091AA79`, then
MouseDown at `0x0091AA97` when the first call returns a target. Its
MouseDown descriptor is `0x0135942C`, initialized with id `0x08` and
strategy 0 at `0x00F22C40..0x00F22C56`. Its dispatcher calls the
`+0xBC` method before named MouseLeftButtonDown specialization. The
repeat pair at `0x0091AB1F` and `0x0091AB3D` uses a separate provider
mask and descriptors. MouseRepeat descriptor `0x013593B4` is initialized
with id `0x1C` and strategy 2 at `0x00F22BB0..0x00F22BC6`. The producer's
separate up argument reaches preview at `0x0091ABCE`.

The NumberLabel constructor registers the same MouseDown handler on both
B input interfaces at `0x0069DE78..0x0069DE95`. The handler at
`0x0069CCA0` stores D at C+`0x3B84`, refreshes the old and new arrow
visibility and reveals places through D (`0x0069CCB2..0x0069CCD1`).
The constructor also requests Focusable false on both arrows at
`0x0069E1B3..0x0069E1C8` through descriptor `0x0135975C`.
This selection is separate from keyboard focus. ButtonBase supplies
ClickMode 2 and starts IsPressed clear. The IsPressed initializer
writes flags `0x80` at `0x00F25E39`. In `0x0097F277..0x0097F2D3`
that seeds low metadata bit `0x02`, admitting the internal pressed
mutation when retained. The default ClickMode 2 takes the release
Click branch and leaves the earlier preview-down ClickMode 1 branch
outside this short-press profile. The count-2 branch in `0x00936F70`
is also outside this profile.

## Click target, step and effective value

At `0x0069E7CA..0x0069E7D2` the rebuild calls the digit command builder
with the decimal place and C. `0x0069C340..0x0069C3F5` installs the
`Slider.AddCommandParameter` command `0x013609B0` on both B objects,
sets kind-1 parameter integer 0 to the place or its negation, and
calls the explicit CommandTarget setter `0x00954180` with C for both
arrows. For the ones place, Up and Down therefore request `+1` and `-1`.
The rebuild also selects the ones place before a player changes it.

`0x0097EBA0` raises Click id `0x38` with B as source. RepeatButton
input slot 13 selects `0x0097ECE0`, and its Click case calls primary
slot 71 `0x0097EB60`. With the stored CommandTarget retained, the
lookup at `0x0097ED2E` returns B+`0x378`'s pointee C, and the
recipient adds `0xB4` before calling command `+8` at `0x0097ED73`.
The command was constructed as a RoutedCommand at `0x00F27FE9`.
The [command route](sqwt-command-parameter-consumer.md#target-and-route)
therefore runs PreviewExecuted and then Executed under its ordinary
handled and stop gates, using C's explicit interface.

NumberInputBox input table cell `0x00FCB288` maps Executed virtual
`+0x104` to `0x0069C5E0`, which calls integer Slider at `0x0069C614`.
Its base store runs first at `0x009970AA`. C's exact Executed
handler `0x0069C1F0` consumes a negative step below `-1` when integer
1 is zero and the sum is at most zero. For an unhandled
AddCommandParameter, the Slider adds integer 0 to the effective Value,
quantizes by SmallChange, and calls the typed Value setter at
`0x00997169`. The constructed SmallChange is 1. The usual signed
range clamps the resulting request. Only a changed effective value
raises integer ValueChanged (`0x0135ADE8`, id `0x30`, strategy 0) with
C as source and original source (`0x00967215..0x009672C8`).

## Conditional hand-in command delivery

The Label that wraps each NumberInputBox selects
`ControlTemplate_Label_NumberInputBox`. Its EventTrigger at template
line 5 declares `RoutedEvent=ValueChanged`, `SourceType=Slider` and
no SourceName. The action at line 6 supplies command
`NumberInputBox.ValueChanged`, `PropertyName1=Value`, a one-Int format
and no CommandTarget. The independent sound action is at line 7.

EventTrigger setter `0x009B1680` selects RoutedEvent name resolution
through `0x009B1F50` for property index 0, and SourceType storage at
`+0x54` for index 2. Descriptor resolution through `0x00914BB0` calls
`0x0097F900`, whose registry resolves the named event. The checked
ValueChanged initializer supplies the single literal at `0x0106FD84`
and descriptor `0x0135ADE8` (`0x00F23F70..0x00F23F86`).

With the form and template attached as declared, the bubbled event can
reach the containing Label. Label input slot 13 (`0x010737C0`)
selects `0x00935380`, which calls `0x00938BA0`. Its current-template
trigger pass runs under the stop and template guards. EventTrigger
virtual `+8` (`0x0107D55C`) selects `0x009B1820`. Its descriptor
comparison is at `0x009B1865..0x009B186B`. The SourceType comparison
at `0x009B1871..0x009B189E` uses event source `+0x08`, so this is a
class test on C. Integer Slider constructor `0x009985A0` installs
class descriptor `0x0136064C` with name Slider at
`0x009986F7..0x0099871E`. NumberInputBox `0x0069D2D9..0x0069D2FE`
appends its name through the same constructor `0x00933290` before
replacing the class pointer. That constructor copies the prior class
path and appends a dot-delimited component. `0x009335D0` uses the
source's current class descriptor and `0x00933400` searches its path
for the dot-delimited `Slider` component (`0x00933439..0x00933496`).
This admits the NumberInputBox subclass when the descriptor and
source are retained.

The admitted `ExecuteCommandAction` reads the source property at
`0x009B29D4`, formats and parses the Int record (`0x009B2B1F`), and
uses C as the default target (`0x009B2B40..0x009B2B7E`). This produces
an Executed command on C's interface with one integer, the effective
Value. The ordinary form subscriber uses the named control's own
handler store. The [subscription and argument gates](sqwt-command-parameter-consumer.md#subscription-gate)
remain conditions for delivery. No form-controller gate is named for
EventItemSelectWidget in the checked seven-widget table.

| Installed script | Location | Observed operation |
| --- | --- | --- |
| `EventItemSelectWidget.initAsk` | `root/proto0` PCs 207-210 and 243-250, last call byte 1683 | Loop 1 through 16 registers `setControlCommandCondition` for each `CustomControl_NumberInput_` plus its row number and `NumberInputBox.ValueChanged`. |
| `WidgetBaseClass.setControlCommandCondition` | `widgetbaseclass_common` `root/proto55` PCs 0-4, call byte 17114 | Passes the control, command and numeric priority 5 to `_setUICommandCondition`. |
| `WidgetBaseClass._onUICommandEvent` | `widgetbaseclass` `root/proto6` PCs 6-12, call byte 2650 | Forwards to `processUICommandEvent`. |
| `WidgetBaseClass.processUICommandEvent` | `widgetbaseclass_common` `root/proto13` PCs 189-195, call byte 10738 | The checked command has no earlier matching arm. The default branch forwards all five arguments. |
| `EventItemSelectWidget.processUICommandDefault` | `root/proto5` PCs 0-3, call byte 5351 | The command match calls `setOperateButtonEnable`, which reads the current widget values. |

The registered numeric priority selects `_onUICommandEvent` under the
normal form and Lua actor gates. For a root form lookup the one-int adapter at `0x0075D550`
uses `0x008A3FE0` and `0x006F58A0`, which joins the supplied-context
gate at `0x006F593C`. The resulting static chain is conditional on the
ordinary input target, successful form and trigger attachment, accepted
range write, live owner and eligible scheduler. Clamped no-change edits
produce no new ValueChanged event. Keyboard, pad, editor focus, repeat
cadence, alternate pointer targets, command loss under concurrent
priority-5 events, actual sound and original live delivery remain open.

## ShopEditWidget mode 12

The [ShopBuy selection contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/c3340f42170201f3aedcdbb89ed04012cff22c5f/docs/shop-buy-selection-contract.md#row-and-quantity-selection)
owns the row, count and confirmation result. Here the quantity control is
the standard `widget/ShopEditWidget.form` carrier (9177 bytes, SHA-256
`23fb6f4c044126501e718779db8f2fe8512f5a3acdfc5c13cc70c2fc00c75fc5`,
decoded SHA-256
`1009c0fdfc908746c26f1c1b5e854c8f8bc07fbc67a0cdfb32f4f32168370c13`).
Line 5 merges the same numeric dictionary described above. At lines
76 and 77, `Label_NumberInput_1` selects
`ControlTemplate_Label_NumberInputBox` and contains
`CustomControl_NumberInput` of type `NumberInputBox`. The child
declares no range or focus overrides and does not request the separate
Slider style. `Button_Done` and `Button_Back` at lines 122 and 123
declare `UILuaCommands.Operate`. These identities were checked by SQWT
roundtrip and XML start-element line numbers (MEASURED).

The ShopEdit LPB `n1635q/r2vu561qn1635q.le.lpb` and the shared widget
LPBs matched the encoded and decoded pins in
[xivl-client-scripts](https://github.com/XIVLegacy/xivl-client-scripts/blob/c3340f42170201f3aedcdbb89ed04012cff22c5f/manifests/retail_lua_coverage.json).
Read the zero-based prototype PCs with the same Lua 5.1 replay as above.

- ShopEdit `main/f0` PCs 68-71 register `NumberInputBox.ValueChanged`
  on the named numeric control before initial setup. The shared
  `widgetbaseclass_common` helper `main/f55` PCs 0-4 calls
  `_setUICommandCondition` with priority 5. The selected template
  action uses the effective Value and record kind 1. The
  [native subscription](sqwt-command-parameter-consumer.md#subscription-gate)
  stores numeric condition kind 0. These kinds are separate.
- `main/f1` PC 360 selects mode 12, PC 371 sets count 1, and PCs
  373-420 derive the maximum. It starts at the current item's
  `stackMax`. When supplied money is below `pricedata * stackMax`,
  it substitutes floor of money divided by `pricedata`. For item
  IDs strictly between 1000002 and 1000101 it caps that maximum at
  999. PC 429 supplies value 1, minimum 1 and that maximum.
  `main/f4` writes Minimum, Maximum and Value in that order at
  PCs 6, 12 and 16. `main/f1` PCs 454-495 request quantity
  control visibility according to the `stackable == 1` branch.
- `processAfterShow` (`main/f3`, PCs 0-4) requests `Button_Back`
  when its explicit argument is Boolean true. `main/f5` first
  sets logical focus to that control, and requests keyboard focus
  only when the desktop's focused widget already equals self
  (PCs 4-14). This gives a conditional initial request rather
  than a measured live recipient.

### Focus before the quantity step

STATIC. The RepeatButton preview-down base call at `0x0097EFFE`
reaches `0x0092EB90`. After the base returns, a clear handled byte
and original source equal to the arrow admit its call to
`0x0092EA10` at `0x0092EBB7`. With the constructor's Focusable
false retained on the arrow, the ordinary non-Window path reaches
`0x0092B150` at `0x0092EB0F`. This searches complete logical parents
through `+0x240`, checking registry exclusion, enabled/input
admission, Focusable, visibility and tab admission, and requests the
first eligible ancestor's input `+0x38` at `0x0092EB25`. This occurs
before the MouseDown digit-selection handler and release Click.

The NumberLabel constructor's arrow-child insertion callback
(`0x0069DE97..0x0069DECC`) and NumberInputBox's digit insertion
callback (`0x0069E5BF..0x0069E5DC`) reach `0x009955D0` for the
embedded Grids. Both controls install their Grid templates through
`0x00938170`. On the admitted current-template branch
`0x00936EB9..0x00936ED7`, the owner attaches its Grid. These insertion
paths use `0x009313C0`, which can store the complete parent at
`0x0093145B` and the input parent through `0x00931405 -> 0x0091E180`
at input `+0x60`. The resulting path is arrow, digit Grid, NumberLabel,
box Grid, NumberInputBox. It assumes the selected roots and attachment
stores are retained.

Grid construction requests Focusable false (`0x00995CE4..0x00995CEA`).
Label construction requests IsTabStop false through `0x0097436F`
and `0x00937FC0`. NumberInputBox inherits the Control constructor's
Focusable true request at `0x00939063` and initial IsTabStop true
at `0x00938F49..0x00938F53`. On that retained, admitted path
the ancestor candidate is the NumberInputBox. Its input table
`0x00FCB184` selects `0x0091E3E0` at `+0x38` and
`0x0069C2B0` at `+0x68`. Selection mode `0x0F` delegates to
`0x0092E650 -> 0x0092E5F0`, which can retain the box interface.
The separate mode 9 arm can choose a retained digit or the first
vector digit. Preview cancellation, candidate redirection, actual
pointer containment and current widget/input guards remain conditions.

The admitted focus store at `0x0091E75C` precedes the
GotKeyboardFocus delivery at `0x0091E880`. When the box receives it,
its `+0xE8` entry `0x00930D20` calls `0x0093D370` at
`0x00930DED`. Input `+0x14` selects `0x0091C4B0`, which returns
true for the current focus interface itself or any of its input
parents. The guarded refresh stores the sample at box `+0x15D` bit 5.
The LostKeyboardFocus entry `0x009299D0` similarly resamples the
current interface chain. The registered getter ordinal 5 selects
`0x0093A2F6 -> 0x0093E220` and reads that stored bit.
The ordinary property read formats True or False and the
[WidgetBase Lua getter](https://github.com/XIVLegacy/xivl-client-structs/blob/8a8930c6da61d1ff8b3d442fe169396805394d23/docs/chocobo-naming-native-contract.md)
converts the result to Boolean through case 6 at `0x005382A6`.

The short release step and current ValueChanged template action
follow the paths above. ShopEdit `main/f8` PCs 0-14 checks wait status
and the zero operation, then forwards integer argument 1 to
`updateNumber`. `main/f9` PCs 0-8 stores the integer only if
the current named control's IsKeyboardFocusWithin read is Boolean true.
Done consumes that stored count through the linked ShopBuy selection
contract. A clamped no-change step produces no new ValueChanged.
The LPB can reject the callback even after a changed range value if
focus has moved before delivery. This trace establishes the
conditional short-press route and leaves live focus acceptance,
scheduler timing, other modes and repeated presses open.
