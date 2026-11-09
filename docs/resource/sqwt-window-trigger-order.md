# Sqwt Window style and template event order

The identified ffxivgame.exe processes an admitted Window Style trigger
collection before its admitted ControlTemplate collection on the non-0x4F
route below. This is conditional static event processing, not initial property
evaluation or markup materialization order.

## Input and method

FINAL FANTASY XIV 1.23b build 2012.09.19.0001, PE32 x86 little endian,
image base 0x00400000, size 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
Locators are VAs and object offsets are hexadecimal. Ghidra CLI disassembly,
original PE section mapping, dispatch-table reads and RTTI establish the
observations. No execution or rendered result is asserted.

Let E be a valid complete Sqwt::Window, I=E+0xB4 its event interface, and A a
valid event argument with a valid descriptor at A+4. Original table cell
0x01069F90 points to COL 0x01179078, whose offset field at 0x0117907C is
0xB4. Type descriptor 0x01269E20 names .?AVWindow@Sqwt@@.

## Conditional event passes

Window interface table 0x01069F94 has slot +0x34 at 0x01069FC8, selecting
0x00922030. For descriptor id byte other than 0x4F, the call at 0x00922042
enters this chain:

    0x00935380 -> 0x00938BA0 -> 0x0092D060 -> 0x00939800

The calls unwind into the following collection passes. These final pass
guards are necessary conditions after earlier base and event handling, not
a complete event-admission predicate.

| Pass | Selected pointer and collection | Final pass guards | Entry callback |
| --- | --- | --- | --- |
| Style in 0x0092D060 | I+0x148 = E+0x1FC, then object+0x24 | A+0x17 equals 0 and Style is nonnull | Primary +8 at 0x0092D142 |
| ControlTemplate in 0x00938BA0 | I+0x264 = E+0x318, then object+0x24 | A+0x17 is not 1, descriptor and Template are nonnull | Primary +8 at 0x00938C5E |

The Style wrapper calls its base at 0x0092D070 and reads the Style pointer at
0x0092D0BE. After the call at 0x00938BE6 returns, the Template wrapper reloads
A+0x17 at 0x00938BEB, reads the Template pointer at 0x00938BFB and selects
its collection at 0x00938C07. Its literal byte guard differs from the Style
pass. No unproved Boolean normalization is assumed.

Each pass walks its collection from stored begin toward end in 4-byte steps,
passing E, A and the collection to entry primary +8. Entry AL=1 ends only that
collection walk, at 0x0092D144..0x0092D146 or 0x00938C60..0x00938C62. The
caller does not use the Style wrapper's AL as a universal stop. Normal return
therefore places any admitted Style pass before any admitted Template pass.
This does not prove that both passes or all matching actions execute.

The descriptor-id 0x4F arm instead invokes complete-object primary +0x138 at
0x0092205B. The Template wrapper also performs primary callbacks for ids
0x19 and 0x1A before its base call. Outer wrapper 0x00935380 has a later
collection pass at I+0x2A0/E+0x354. Its semantic identity remains UNKNOWN.
These paths do not establish nested or reentrant event order.

This direct collection order is separate from the generic
[handler-store registration order](sqwt-command-parameter-consumer.md#handler-registration).

## Storage and materialization joins

| Property | Assignment helper | Receiver field adjustment | Pointer helper and guarded changed-pointer store |
| --- | --- | --- | --- |
| Style | 0x0092F220, descriptor 0x01358A30 | +0x1FC at 0x0092F23F | 0x009C3820 at 0x0092F245, store at 0x009C38A2 |
| Template | 0x00938140, descriptor 0x01359118 | +0x318 at 0x0093815F | 0x00926F10 at 0x00938165, store at 0x00926FA0 |

The selected Style textual branch constructs through 0x009A6230 at
0x0093081E, materializes through 0x0099AF80 at 0x00930833, then calls
assignment helper 0x0092F220 at 0x00930842. Empty text has a null-assignment arm. An existing nonnull Style
with identical key text skips construction. The selected Template branch
constructs through 0x00936C00 at 0x009386BA, materializes at 0x009386CF,
then calls assignment helper 0x00938140 at 0x009386DE. Equal existing key text also bypasses construction.
These are within-branch sequences, not an order between the two properties.

Let T be the constructed Style or ControlTemplate receiver. Their
constructors call common FrameworkTemplate constructor 0x0099B3A0 at 0x009A6264 and 0x00936C0F. It constructs a
TriggerCollection at T+0x24 through 0x009C27B0, callsite 0x0099B3DF, and
retains owner E at T+0xC8 at 0x0099B450. On these distinct-object construction
paths each created Style and ControlTemplate has its own embedded collection.
Separate installed pointer fields alone do not prove universal nonaliasing.

Reader 0x009495E0 inserts EventTrigger, CommandTrigger and Trigger into the
selected T+0x24 through 0x0094E1D0 at 0x00949698, 0x0094973F and
0x009497D0. Each selected insertion precedes recursive content reading at
0x009496C7 through its branch or back-jump. Constructors receive T and T+0xC8.
EventTrigger and Trigger reach TriggerBase construction 0x009C2620 through
0x009B1BD0. It stores the pair at trigger+0x3C/+0x40 at
0x009C2694/0x009C2697. CommandTrigger instead calls 0x009C24B0 at
0x009B1182, which stores the same pair at 0x009C24B8/0x009C24BB.

The [selected initial Template Setter path](sqwt-template-initial-enabled-receiver.md)
and [Style owner path](sqwt-style-reverse-binding.md) retain their own guards.

## Installation notification

An admitted changed-pointer installation can synchronously visit the installed
collections after materialization and pointer storage, before the assignment
helper returns. This is a conditional event route, not an initial evaluation
of every Trigger's tested property.

The selected textual Style and Template calls above pass notification 1.
Their pointer helpers retain holder-admission and equal-pointer exits. The
changed-pointer arms store before the conditional notification call.

| Property | Changed-pointer store | Notification helper call | Construct event, then call supplied callback |
| --- | --- | --- | --- |
| Style | 0x009C38A2 | 0x009C38BF -> 0x009C3720 | 0x009C3798, then 0x009C37B6 |
| Template | 0x00926FA0 | 0x00926FB5 -> 0x00926DA0 | 0x00926E18, then 0x00926E36 |

Event constructor 0x00590380 initially stores shared routed-event descriptor
0x01356D9C at A+4 and the supplied property descriptor at A+0x70, the latter
at 0x005903F8. It clears A+0x14 through A+0x17 and derives A+0x78 mask 2
from the supplied notification argument at 0x005903F4..0x00590413.
The selected Style and Template descriptors are 0x01358A30 and 0x01359118.
Initializer 0x00F206D0 supplies the name DependencyPropertyChanged and id
0x42 to descriptor 0x01356D9C through 0x0097F6C0 at 0x00F206E6.
The id reaches descriptor byte +5 at 0x0097F70D. This is an initializer
join, not a read of a running descriptor instance.

The assignment wrappers supply owner E and zero adjustment to the callbacks.
Style callback 0x0092DAB0 calls E primary +0x18 at 0x0092DAEB. Template
callback 0x00937A70 does likewise at 0x00937AA1. Original PE cell
0x0106A0D4 selects 0x00934B60, whose base call at 0x00934BA4 reaches
0x0092DF20 -> 0x0093BD80. The latter calls generic dependency notification
0x00912AC0 at 0x0093BDC5 before testing current A+0x78 mask 2 at
0x0093BDCA. A cleared mask skips the routed tail.

With that mask admitted and current A+0x70 still equal to a supplied
installation descriptor, the descriptor-specific branches reach the common tail. Metadata-selected
callbacks remain possible. The tail stores E at A+8/+0x0C, requests property
text through 0x00911020 at 0x0093BEBD and calls I+0x34 at 0x0093BED2
without testing the text reader's return. The conditional Window collection
passes above apply thereafter. Intervening callbacks, nested notifications,
reentry and descriptor mutation were not excluded.

Property Trigger evaluation below checks the current A+0x70 against its
current retained tested-property descriptor. An event initially constructed
for Style or Template therefore does not by itself establish initial
evaluation of an IsAnchorWithin Trigger.

The selected Trigger markup constructor uses table 0x0106DA3C. Its end-hook
cell 0x0106DA50 selects the bare-return 0x00946AA0. Generic reader
0x00947040 calls markup +0x14 at 0x0094728A on normal completion. The
rejected-child branch at 0x0094727A skips that hook. This hook alone performs
no initial evaluation. Constructor, setter, child-reader, listener and binding
activation effects remain separate joins.

## Ordinary ActionMenu property elements

The installed `client/sqwt/widget/ActionMenuWidget.form` visits its
Window.Template property element before Window.Style on the admitted
ordinary textual route. Authenticate the two SQEX carriers below, decode
their bodies after byte 8 with `xivl-tools:tools/blowfish.py` using the exact
basename as the key, and check the decoded SHA-256 before parsing.

| Carrier | Bytes | SHA-256 | Decoded SHA-256 |
| --- | ---: | --- | --- |
| ActionMenuWidget.form | 40815 | d8a9dffea87cf0d5cea066a7f3d27b4749d7c2ac8b8fff33ecf5bc40fc0711b1 | c576f44ef70c45cf50fd1b8a4b0dcaf813b665a607fb647482521a43dc5a8286 |
| ActionMenuWidget.tpl | 47246 | 18c4967b39ebce0c0b8a82d5d706b569bd538a2a5ea6b8c54bcb434612538be6 | 2c239989bd6bfc4022cd4a18499f78f9957b1791165f806644da4581698c4871 |

The Window's direct children are Window.Resources (line 2),
Window.Template (line 9), Window.Style (line 12), and Grid (line 15).
The resources merge ActionMenuWidget.tpl. Each property element has one
StaticResource child, selecting `ControlTemplate_Window_ActionMenu` and
`Style_ActionBarWindow` respectively. The root has no Template or Style
attribute.

LLVM 21.0.0 disassembly and original PE table reads establish the join.
The ordinary DOM parser appends an opening child at
`0x00DC28BA -> 0x00DC34E0 -> 0x00945C80 -> 0x00813050` to its
parent's `+0xBC` collection. The Window reader uses the ordinary built-in
route at `0x0094B68E -> 0x00947040` when a registered factory does not
consume the node first. It reads that collection through table cell
`0x01129C14 -> 0x00942530`, then visits its entries in ascending index
at `0x00947276`. It stops on child rejection.

The Window.Template and Window.Style branches construct property wrappers
at `0x0094BA6D` and `0x0094BB00`, retaining the same Window E. Each reads
its own children inline. The StaticResource arm
`0x0094A0C0 -> 0x004EC490` retains E at wrapper `+0x170` and the
property-class name at `+0x11C`. The ResourceKey attribute handler
`0x009463A0` requests Template or Style assignment on E through
`0x0094649F -> 0x00912850`, before either property wrapper's end hook.
The normal owner-property parser dispatch reaches the materialization
and assignment arms above. The authored sequence therefore requests
Template before Style. The same order applies to successful changed
pointer stores when both requests are admitted and current state survives
callbacks.

Template notification callback `0x00937A70` first calls owner primary
`+0x18`. A retained Template ordinal selects
`0x00937CC9 -> 0x00937CB0 -> 0x00937C3A`, which calls
`0x00936DF0` at `0x00937C7F`. With a current Template, a nonnull
root at its `+0xB0` and null root logical parent at `+0x240`, the
helper requests visual and logical attachment at `0x00936ECF` and
`0x00936ED7`. This attachment attempt can occur before the Style
child is visited.

The direct installation events start with their own property descriptors.
Neither matches the resolved IsAnchorWithin descriptor `0x013594FC`
when the plain Trigger reaches its comparison at `0x009B063F` with
both identities retained. That comparison gates the read for the
`IsAnchorWithin=False` Trigger (template carrier line 601). The
first matching owner event, nested notifications, callback reentry and
first storyboard capture remain UNKNOWN.

## Subsequent focus and anchor refresh

Initializer `0x00F229A0` supplies the IsKeyboardFocusWithin string at
`0x0106BF00` and descriptor `0x01359378` to `0x009108E0`. It writes
ordinal 5 at `0x00F229AF` and metadata `0x80` at `0x00F229B9`. These are
initializer assignments, not observations of a current descriptor instance.

Callback `0x0093B700` first dispatches E primary `+0x18` at `0x0093B732`.
After return, current A+0x78 mask 2 and current descriptor ordinal
`[A+0x70]+0x14` gate its switch. Ordinal 5 selects `0x0093B927` through
original table cell `0x0093BD4C`. That arm calls current E primary `+0x64`
at `0x0093B92F`. If E retains Window primary table `0x0106A0BC`, cell
`0x0106A120` selects `0x00926870`. Its RTTI locator `0x01179064` has
complete-object offset zero and identifies the Window type above.

Method `0x00926870` performs separate local-property work and may notify
descriptor `0x013571C0` at `0x0092690C`. After normal return, its call at
`0x00926918` passes E and the saved incoming argument to `0x0093D490`.
This joins an admitted current IsKeyboardFocusWithin notification to the
[anchor source refresh](sqwt-render-transform.md#isanchorwithin-and-ismouseover),
conditional on descriptor identity surviving the generic call and the stated
current gates and tables. An arbitrary ordinal-5 descriptor is not identified
as IsKeyboardFocusWithin by its ordinal alone.

The linked refresh contract owns source sampling, comparison, changed-arm
storage and notification arguments. Any resulting anchor event remains subject
to the current-descriptor and shared-action gates.

This is a subsequent property-refresh route. It does not establish first
Window attachment, listener arming, initial IsAnchorWithin=0 Trigger evaluation,
first BeginStoryboard or endpoint capture. Generic and local-property callbacks,
reentry, current table retention and object lifetime remain conditions. Other
initialization and notification paths are not excluded.

## Attachment and conditional focus notifications

Ghidra CLI, LLVM disassembly and original PE tables establish these paths.
Window constructor 0x009246F0 installs its tables at 0x00924724 and
0x0092472A. UIElement base constructor 0x0093C750
initializes IsKeyboardFocusWithin at E+0x15D (0x0093C9C5..0x0093CA17).
Metadata 0x80 sets holder bit 1 and leaves value bit 5 clear. The
[IsAnchorWithin holder](sqwt-render-transform.md#isanchorwithin-and-ismouseover)
at E+0x160 has the same initial bits. The input parent I+0x60 starts null at 0x0091E117.

### Desktop attachment

The Window markup branch calls its constructor at 0x0094B61D and builds
its wrapper at 0x0094B675 before entering its own recursive reader at
0x0094B68E. For nonnull E, wrapper constructor 0x00946830 can pass E to
the current parent markup +0x10 at 0x009468B9. The DesktopWindow branch
at 0x0094B6D1..0x0094B6F3 builds a base wrapper with +0x10 cell
0x0106D678 -> 0x00946AB0. With wrapper+0x14 equal to nonnull desktop D+0x194,
that handler dispatches +4 through cell 0x01069214 -> 0x0091A2B0 (COL 0x01178D90, offset 0x194).
That helper can insert E visually at 0x0091A2CC and logically at 0x0091A2E8
before the child reader, subject to its D+0x78 and D+0x240 tests.

The [Window show routine](sqwt-anime-property.md#show-and-hide-routines)
has a separate route. Nonnull desktop D admits its +0x194 table +8 call
at 0x00925AFD (0x01069218 -> 0x0091A270). This can insert E visually at
0x0091A28A when E+0x78 differs from D and logically at 0x0091A29F when
E+0x240 is zero. On return, show requests the Visibility write at 0x00925B17
and calls 0x009237D0 at 0x00925B23. It can request focus through I+0x38 at
0x00923889 if selection returns E, the incoming byte equals 1, E+0x15D bit 5
is clear and E+0x15C bit 5 is set.

Visual insertion 0x00940B30 writes E+0xA4 at 0x00940B69 and calls parent
primary +0x3C at 0x00940C49. Logical insertion passes I to current parent
input +0x50 at 0x00931405. DesktopWindow cell 0x01069274 and Window cell 0x01069FE4 select 0x0091E180.
For a valid first parent it requests an append of I at 0x0091E1AD, stores the
parent interface at I+0x60 at 0x0091E1B2 and sets I+0x70 mask 4 at 0x0091E1B5.
The admitted Parent store at E+0x240 (0x0093145B) and the Parent descriptor
0x01358B3C notification call (0x009314A2) follow. The insertion bodies
directly write neither tested holder nor the focus global, and callbacks can
reenter. The tail call at
0x009314C6 searches from the complete parent's +0x240 in 0x0092B820.
With a nonnull anchor recipient at 0x0092B829, a parentless desktop exits at
0x0092B833 even when its interface equals the recipient. A matching ancestor can receive +0xD4 at 0x0092B85A.
For the checked desktop and Window tables, 0x0092FBA0 can request focus at
0x0092FBCC if its +0xE4 predicate is false and mode-9 selection succeeds.

### Focus transfer and refresh

Window I+0x14 (0x01069FA8 -> 0x0091C4B0) tests the current keyboard-focus
interface at 0x01357020 and its input parents. I+0x48 adds the
[anchor gate](sqwt-render-transform.md#isanchorwithin-and-ismouseover).
The allocation arm at 0x0091BF89 and 0x0091BFA5 assigns a constructed
Sqwt::Input::AnchorElement to 0x01356F34 (COL 0x01178E38). Its +0x158 is Visibility
(0x013596BC, 0x0093C897..0x0093C8AE) and must equal 2. Separate global
0x01356FCC supplies the anchor event recipient.

The [focus request](sqwt-command-parameter-consumer.md#conditional-actionmenu-focus-request)
reaches 0x0091E3E0. The direct new-target arm requires both virtual guards,
a different target, a nonnull result from 0x0091C370 at 0x0091E502, no
redirect, and a preview cancellation byte unequal to 1 at 0x0091E694.
Cancellation can return a success-valued AL without the store. With a desktop
present, the walk returns the input below it, the desktop target, or null
for a disjoint chain. With no desktop it can return the last nonnull input
at 0x0091C39E..0x0091C3A3.

On that arm, 0x0091E75C stores the target input interface in 0x01357020.
The call at 0x0091E762 enters 0x0091B890. It can dispatch LostAnchor to
a nonnull old recipient at 0x0091B925 if the Visibility gate passes. It
reloads the Visibility owner at 0x0091B92A after that dispatch, stores the
new recipient at 0x0091B949 and checks the gate and new recipient before
GotAnchor at 0x0091B9AD. After the helper returns, 0x0091E3E0 can dispatch
LostKeyboardFocus at 0x0091E7B4, separate LostFocus and GotFocus events,
then GotKeyboardFocus at 0x0091E880.

These events pass three route flags of 1 and require a current descriptor
and nonzero target input+0x64 in 0x0091E1C0. The walk at 0x0091E2A6 collects
ancestors up to the desktop boundary and can append the desktop separately.
For retained class 0, 0x0091D210 walks the list reversed at 0x0091E31D
backwards, target before ancestors. It calls input +0x34 at 0x0091D2A6
and stops further delivery when A+0x17 is nonzero at 0x0091D2AC. If the
Window receives the event with its descriptor and tables retained, the
[base dispatch chain](#conditional-event-passes) reaches 0x0091D320.

| Event (descriptor, id) | Window cell and handler | Refresh call and changed notification |
| --- | --- | --- |
| GotAnchor (0x01356F5C, 0x4D) | 0x0106A070 -> 0x00921EA0 | 0x0092C01A -> 0x0093D510, 0x0093D57B |
| LostAnchor (0x01356F68, 0x4E) | 0x0106A074 -> 0x0092C070 | 0x0092C079 -> 0x0093D5A0, 0x0093D60B |
| GotKeyboardFocus (0x0135945C, 0x20) | 0x0106A07C -> 0x00930D20 | 0x00930DED -> 0x0093D370, 0x0093D3DB |
| LostKeyboardFocus (0x013595B4, 0x22) | 0x0106A080 -> 0x009299D0 | 0x009299F1 -> 0x0093D400, 0x0093D46B |
| AnchorVisibleChanged (0x01356F50, 0x4C) | 0x0106A0A4 -> 0x0092C2B0 | 0x0092C2B8 -> 0x0093D630, 0x0093D69B |

0x0091B9D0 can dispatch AnchorVisibleChanged at 0x0091BACB. GotKeyboardFocus
has a gated call at 0x00930DE5 before refresh. After optional holder processing,
the refreshes require either holder bit 0 or 1 set and compare value bit 5
with the sampled byte. Equality skips the store and notification, including
false-to-false. A change calls 0x0099C680 with complete E, callback
0x0093B700 and the matching descriptor.

If E retains both defaults, old focus and anchor paths lie outside E, and
focus moves to I or an input descendant, a delivered GotAnchor can notify
IsAnchorWithin before GotKeyboardFocus can notify IsKeyboardFocusWithin.
This requires the gate to stay open through the anchor refresh. With the gate
closed, a false anchor sample against retained false skips the explicit changed
notification. These STATIC paths depend on
current state, callback return and delivery. The first matching ActionMenu
event, initial false Trigger evaluation and first action remain UNKNOWN.

### ActionMenu show and focus retirement

The authenticated [ordinary ActionMenu form](#ordinary-actionmenu-property-elements)
declares no Visibility, Focusable, Anime or IsAnchorWithin on its root.
The UIElement constructor in the Window base chain supplies byte 2
to the Visibility initializer at `0x0093C8AA -> 0x00973EF0`. The
Control constructor requests Focusable=true at `0x0093905C..0x00939063`.
These are holder and request facts. Effective visibility, focus and
callback delivery still depend on current object state.

Authenticate the following LPBs under `client/script/n1635q` with
the [retail script manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/8d2aa874ed4609c54f6e0d7a3330dadedb94878b/manifests/retail_lua_coverage.json).
Read the decoded chunks through EOF using that revision's
`tools/retail_script.py:decode_lpb` and
`tools/build_quest_contracts.py:ChunkReader`. PCs are zero-based
within each prototype.

| Script | Carrier | Bytes | Carrier SHA-256 | Decoded SHA-256 |
| --- | --- | ---: | --- | --- |
| ActionMenuWidget | 97q1vwx5wpn1635q.le.lpb | 25126 | bceeae7dc8ac024767ba65a7c1ecbf8682f7c4e89b5c4eb55310d4e00e58af31 | 826b09ef44102a51a34f53276ecafed348744688b8ce987831e82f7833fdcecc |
| WidgetBaseClass common | n1635q89r57y9rr_7vxxvw.le.lpb | 32996 | c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c | 7c45426b1a9f168557d84e420c5767c6fdea4abf90b976b276546af4e6a5de5f |
| DesktopWidget connector | 65rzqvun1635q_7vww57qvs.le.lpb | 213487 | 0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425 | 685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621 |

The Desktop connector's `root/proto168` obtains static slot 7 at PC 2
and, when the computed show flag equals true at PCs 31/32, calls
its `show` at PC 34 with only the receiver. An exact true third
explicit argument exits the show branch at PCs 35/36 after that call.
With ordinary inherited dispatch, WidgetBaseClass common `root/proto40` checks
`forceHideFlag` and `isShow` before any show request. An exact true
result at PCs 13/14 returns before the request. Its `isShow` at
`root/proto45` reads Visibility at PC 2 and compares against Hidden
at PC 3. A value unequal to Hidden and Anime unequal to Hide at PC 10
take the true path. If these getters resolve to the retained root,
this branch can leave the initial anchor value false without a
new show request. The inherited work path requests Anime=Show
at PC 37 when its first explicit argument is unequal to false
at PCs 27/28. The selected no-extra-argument call supplies nil there.

On the [admitted native show path](sqwt-anime-property.md#show-and-hide-routines),
`0x00925B17` requests Visibility 2. The comparison at
`0x009BCBA9..0x009BCBAB` skips the changed store and callback for the
same value. A changed admitted value can notify with descriptor
`0x013596BC`. The derived IsVisible setter at
`0x0093B7F8 -> 0x0093A3B0` uses descriptor `0x013593D8`. Its
PreviewIsVisibleChanged route calls primary `+0x7C` at `0x0093BA2B`.
The Window implementation `0x009261E0` requires a null logical parent
E+`0x240` at `0x00926202` for its recursive show request. A retained
attached Window bypasses that request. Visibility, IsVisible and the
Shown descriptor `0x013573B0` at `0x00925B4E` are distinct from a
matching IsAnchorWithin property notification. The show bookkeeping
and its conditional focus request still apply.

On a separate non-show branch of the Desktop connector
`root/proto168`, when `getConfigFlag(1)` does not equal false,
`checkKeyboardFocused` returns exactly true and the slot-7 widget's
`isShowMacro` returns exactly false, PC 72 requests
`changeFocusedWidget` with Desktop itself as the target. The
[admitted focus transfer](#focus-transfer-and-refresh) stores the new
keyboard-focus interface before sending LostAnchor to the old
recipient. If the route reaches this ActionMenu Window, the low-bit
test at `0x0093D5D8` must admit the E+`0x160` holder. With retained
true and false from the anchor-gated `I+0x48` sample,
`0x0093D609/0x0093D60B` stores false and requests a notification
with descriptor `0x013594FC`. An equal false sample skips both
at `0x0093D5E6` and continues to the call at `0x0093D617`.
The script comparison alone leaves the current native focus interface
and anchor recipient unresolved.

The [keyboard alias path](sqwt-command-parameter-consumer.md#actionmenu-keyboard-aliases)
supplies a separate gate writer. For a matching admitted command
whose result equals 1, the KeyBinding flag from SqwtShowAnchor can
reach `0x00921246 -> 0x0091B530`. A nonnull AnchorElement receives
the Visibility 2 request at `0x0091B54E`. Its primary `+0x7C` cell
`0x01069840 -> 0x0091B9D0` can raise AnchorVisibleChanged at
`0x0091BACB` for the current nonnull anchor recipient. This is the
separate global gate object's path. A delivered event can refresh the
Window holder through the existing route above.

The first effective root getter results, native focus and recipient
identities, first matching ActionMenu event and first Effect_OFF
action capture remain UNKNOWN.

## Property Trigger and shared event flags

Original Trigger primary table +8 cell 0x0107D0E0 selects 0x009B0620.
At 0x009B0626..0x009B0642 it requires a descriptor with id byte 0x42,
a nonnull pointer stored at trigger+0x7C, and equality between the dword
at A+0x70 and the dword at the pointed object's +8. It reads a property through 0x00911180, conditionally
compares through 0x009AF8D0 when trigger+0x84 is nonempty, and calls action
walker 0x009B1820 at 0x009B0695. False-return paths remain possible.
This joins property Trigger evaluation to entry +8 without establishing its
initial evaluation or first matching event.

The action walker reads shared A+0x14/+0x15/+0x16 and writes
A+0x14/+0x15/+0x16/+0x19. Both A+0x14 and A+0x15 equal to 1 take its
early return-true arm. Action +8 call 0x009B1996 also depends on the shared
bytes and action+0x3C/+0x3D. Ordered visits therefore do not establish
independent or unconditional execution. The meanings of all these flags
and complete reentry behavior remain outside this finding.

## Tested-property reference producer

Let R be a complete property Trigger, D=R+0x18 its dependency interface and
Q=R+0x90 its embedded property object, under retained current-table and object
validity conditions. Constructor `0x009B0990` supplies Q to the R+0x7C holder
constructor at `0x009B09FC`. The holder stores Q at `0x009B0B2E`. The Trigger
constructor installs Q table `0x0107D08C` at `0x009B0A51` and sets Q+8 to
zero at `0x009B0A57`. R+0x7C is the pointer holder, not the tested descriptor.
Registration helper `0x0098B400` follows at `0x009B0AAB`. Its complete effects
and later constructor state remain unclosed.

Original dependency table `0x0107D09C` has RTTI locator `0x01180960`, offset
0x18 and Trigger type descriptor `0x012E8174`. Its +0x24 cell `0x0107D0C0`
selects `0x009C24E0`, which reads [D+0x28]=[R+0x40], the retained owner O.
Its +0x18 cell `0x0107D0B4` selects generic dependency notification
`0x00912AC0`. These observations require the current tables to remain valid.

Initializer `0x00F29280` supplies literal Property at `0x0107D020` and
descriptor `0x01361A54`, then writes ordinal zero at `0x00F2928F`.
Initializer `0x00F292E0` supplies descriptor array `0x012E81D4` and count 3
to info `0x01361AC0`. It installs info table `0x0107D0EC`, parser
`0x009B0810` at info+0x28 and zero adjustment at info+0x2C. The original
info +0x0C cell `0x0107D0F8` selects adjusting bridge `0x009B2090`, not
`0x0091B440`. On a nonnull supplied D, that bridge subtracts 0x18 at
`0x009B209A`, adds current info+0x2C and calls current info+0x28 at
`0x009B20B8`. These are initializer and table joins, not runtime parser
selection or delivery observations.

If lookup selects that current info/parser and Property ordinal zero, the
parser obtains O through D+0x24 at `0x009B0896`. With nonnull O it supplies
the incoming name and O to `0x009B0490` at `0x009B08BD`. The helper calls
`0x00910F60` on O at `0x009B04AC`. A nonnull info return admits the store
of the descriptor output at Q+8 at `0x009B04B9`. Failure skips this explicit
store and can leave the earlier value. Simple-name resolution is descriptor
lookup through the owner's info list, not the evaluator's current-value read.
This Q+8 is the dword used by the event-descriptor gate above. A name must
actually resolve to IsAnchorWithin before that specific identity can be claimed.

The descriptor write precedes holder cleanup, admission and pointer comparison.
After possible helper processing, holder mask 2 at `0x009B08EA` can exit.
On the admitted arm, current [R+0x7C] is compared with Q at `0x009B08F2`.
Equality branches at `0x009B08FB` past the later pointer-change notification.
State-bit processing can already have occurred. Pointer equality does not mean
that the parser performed no earlier mutation.

On the admitted changed-pointer arm, helper processing precedes the Q store at
R+0x7C at `0x009B0933`. Call `0x009B0935` sends notification 1 through
`0x00999E10` for the Trigger's own Property descriptor `0x01361A54`.
Its immediate receiver is the holder address. The supplied callback receives
complete R and thunk `0x009B04D0` adjusts it to D before forwarding through
D+0x18 to `0x00912AC0`. That descriptor and route are distinct from the
owner's tested descriptor at Q+8 and Trigger primary +8 evaluation.

The selected direct constructor/parser/writer path establishes reference
readiness without directly calling `0x009B0620`, `0x009B1820` or the value
reader `0x00911180`. It does not recursively close registration, resolver,
listener, binding, constructor, notification or attachment effects. Actual
parser delivery, name-resolution success, current state/table retention and
lifetime remain conditions. Initial owner-state evaluation, first action,
BeginStoryboard capture and Style/Template priority remain unestablished.

## Registration and queued activation

The selected constructor call at `0x009B0AAB` supplies D and info
`0x01361AC0` as explicit arguments to `0x0098B400`. The helper retains
D at `0x0098B41A`, forms info+0x0C at `0x0098B41E` and directly stores
`[D+0x10]=info+0x0C` at `0x0098B45A`. This selects descriptor information,
separate from D+0x18 listener storage. Container calls and synchronization
remain indirect effects. This direct store does not establish listener arming
or tested-value evaluation.

Trigger base construction reaches `0x00912DB0` with D through calls at
`0x009B09C7`, `0x009B1C06` and `0x009C266F`. It clears D+0x10/+0x14/+0x18
and requests an append of D to current context+0x20D8 at `0x00912E9F`.
Insertion and survival remain conditions. The request precedes installation
of Trigger dependency table `0x0107D09C` at `0x009B09F5`, so an arbitrary
reentrant drain is not guaranteed to see that table.

Drain `0x00910CD0` removes the selected queued receiver before calling its
current virtual +0x28 at `0x00910DC9`. With D and its Trigger table retained,
original cell `0x0107D0C4` selects generic activation `0x00912190`.
That method sets receiver+0x20 mask 1 at `0x00912199`. Its current D+0x18
entries receive virtual +0x0C at `0x009121F5`. The separate D+0x14 loop
requests attachment through `0x00911F50` with flag 1 at `0x00912252`.
A nonzero result admits that entry's virtual +0x0C at `0x0091229B`.
The concrete Binding example on the
[reverse-binding page](sqwt-style-reverse-binding.md#observed-attachment-activation-and-retention)
uses its own endpoints and mode. It does not identify current entries for
this Trigger.

These entry activations are distinct from Trigger primary +8 evaluation and
from Q's virtual +8, whose original cell `0x0107D094` selects `0x00933210`.
Ordinary listener dispatch at `0x00911525` supplies descriptor and sender to
the listener receiver. Trigger primary +8 instead consumes an owner and
property event. Matching slot numbers do not join those call contracts.
Current entry population, the first matching owner-property event, initial
predicate evaluation and first BeginStoryboard capture remain unknown.
Context, construction, reentry, current tables and lifetime remain conditions.

### Plain ActionMenu Trigger

The selected [ActionMenu Window Style](action-menu-timer-icon-style.md#actionmenu-window-template-and-style-write-sets)
uses a plain IsAnchorWithin Property, a literal False Value and a
BeginStoryboard action named Effect_OFF. The checked direct constructor,
literal attribute and action child paths leave the Trigger's own D+0x14 and
D+0x18 entry pointers at their null seeds from 0x00912E11 and 0x00912E14.

The ordinary attribute path calls 0x00912850 at 0x00946A8B. The pending
Binding arm requires the opening brace at 0x00912974 and an admitted
Binding parse before 0x009129FD calls 0x00912340. Its deferred arm stores
the container at D+0x14 at 0x0091281E and appends at 0x0091282D. Neither
literal starts with a brace. The listener helper 0x00911E40 stores its
container at +0x18 of the supplied receiver at 0x00911E97. Its calls in
0x00911ED0 supply the explicit destination or a configured source. The
checked direct paths supply no such call on this Trigger's D.

The BeginStoryboard child uses its own dependency interface. Parent markup
+0x20 at 0x00949970 selects 0x0094EB10 through cell 0x0106DA5C. It
appends to the Trigger's action collection at R+0x5C at 0x0094EB2C.
The collection callback selects 0x009B2170 through cell 0x0107D50C and
stores the Trigger interface on the action at action+0x40. The Storyboard
reader attaches a timeline to the action at 0x00949D93.

With both entry pointers still null at delivery, 0x00912190 sets
D+0x20 mask 1, skips the listener callback at 0x009121F5 and branches
at 0x0091220F to the epilogue. This arm has no owner read, predicate
comparison or action dispatch. The separate primary +8 callback still
requires a current matching owner-property event. Attribute preprocessing,
indirect callbacks, reentry and external entry attachment remain conditions.
The first matching owner event and first storyboard capture are unknown.

## Reproduction

Authenticate the input above and use PE section mapping to reproduce table
cells 0x01069FC8 and 0x0107D0E0 and the RTTI joins. With Ghidra CLI, read
these start-inclusive, end-exclusive core intervals. Include all branch
returns rather than stopping at the first RET.

| Procedure | Code interval |
| --- | --- |
| Window interface callback | [0x00922030,0x00922087) |
| Outer wrapper | [0x00935380,0x0093544A) |
| Template wrapper | [0x00938BA0,0x00938C7B) |
| Style wrapper | [0x0092D060,0x0092D165) |
| Base wrapper | [0x00939800,0x0093987B) |
| Property Trigger callback | [0x009B0620,0x009B06A7) |
| Shared action walker | [0x009B1820,0x009B19F2) |
| Common FrameworkTemplate constructor | [0x0099B3A0,0x0099B4AC) |
| TriggerBase constructor | [0x009C2620,0x009C26E2) |
| CommandTrigger owner assignment | [0x009C24B0,0x009C24C1) |

Follow the property assignment helpers, selected construction callsites and
reader insertion callsites above for their separate storage joins. Reproduce
the installation route through 0x009C3820, 0x00926F10, 0x009C3720,
0x00926DA0, 0x00590380, 0x0092DAB0, 0x00937A70, 0x00934B60,
0x0092DF20 and 0x0093BD80. Resolve original PE cells 0x0106A0D4
and 0x0106DA50 separately. Retain the event initializer 0x00F206D0
and descriptor constructor 0x0097F6C0, and the markup end-hook callsite.
Do not omit the generic notification or metadata-selected callbacks before
the routed tail. The
conditional construction excerpts do not establish complete materialization
or source-attribute order.

## Interpretation boundary

The event-pass order and selected storage/dispatch joins are STATIC. Actual
allocation, resource admission, installed pointer values, event production
and matching, object lifetime and rendering were not observed.

The selected Window's serialized attribute-vector order and original XML
attribute order remain unjoined. Initial property-trigger evaluation, listener
activation, setter precedence, animation clock creation and capture order,
stop resolution and nested events remain UNKNOWN. In particular the
[ActionMenu Style and Template](action-menu-timer-icon-style.md#actionmenu-window-template-and-style-write-sets)
listen for distinct property, event and command inputs. Source order alone
does not place those events on one timeline or establish full-form admission.
