# Sqwt bound targets for discrete property keyframes

An admitted `Storyboard.TargetName` change can resolve and cache a target
before Begin. The discrete property evaluator uses that target and passes
each due keyframe's retained text to the generic property setter.

## Inputs and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses are VAs at image base `0x00400000`. Ghidra CLI
cross-references and LLVM objdump 21.0.0 identify the instructions. Original
PE section reads supply the strings, descriptor pointers and dispatch cells.

The standard `client/sqwt/widget/ActionMenuWidget.tpl` carrier is 47246
bytes, SHA-256 `18c4967b39ebce0c0b8a82d5d706b569bd538a2a5ea6b8c54bcb434612538be6`.
Its 47238-byte decoded document has SHA-256
`2c239989bd6bfc4022cd4a18499f78f9957b1791165f806644da4581698c4871`.
The `xivl-tools:tools/blowfish.py` SQEX method uses the original basename as
the key and decodes the body after the eight-byte header. Re-encryption
reproduces the carrier body. Python Expat gives the element locations.
See the [ActionMenu input record](action-menu-timer-icon-style.md#inputs-and-method)
for the same carrier and the CLI extraction route.

## OBSERVED ActionMenu declarations

The two selected `CommandTrigger` branches are under `ControlTemplate.Triggers`
at line 313 of `ControlTemplate_Label_MenuSlot` at line 311. Each selected
`PropertyAnimationUsingKeyFrames` declares `Storyboard.TargetProperty`
as `Visibility` and binds `Storyboard.TargetName` `OneWay` from
`RelativeSource` `TemplatedParent`.

| Command declaration | Animation line | Binding path | Discrete keyframe line | KeyTime | Value |
| --- | --- | --- | --- | --- | --- |
| `UILuaCommands.ChangePageNext`, 314 | 317 | `Name` | 318 | `0:0:0.2` | `Hidden` |
| `UILuaCommands.ChangePageNext`, 314 | 323 | `StringData.Value0` | 324 | `0:0:0` | `Visible` |
| `UILuaCommands.ChangePagePrev`, 333 | 336 | `Name` | 337 | `0:0:0.2` | `Hidden` |
| `UILuaCommands.ChangePagePrev`, 333 | 342 | `StringData.Value1` | 343 | `0:0:0` | `Visible` |

## OBSERVED page peer binding initialization

The checked `ActionMenuWidget.form` declares four page Labels under the
Canvas at line 43. Each Label at lines 44, 76, 108 and 140 sets
`StringData.Value0` and `StringData.Value1` through a peer `ElementName`
with `Path=Name` and `Mode=OneWay`. See the
[form input record](action-menu-timer-icon-style.md#inputs-and-method)
and [peer declaration table](https://github.com/XIVLegacy/xivl-client-scripts/blob/880a3e742f123b7e0e7c502c8ee1047eb8fed3dd/docs/action-menu-page-controls.md#L99-L119).

The qualified setter at `0x0095EDD4..0x0095EDF6` selects the attached
string provider for `0x0135FA28` or `0x0135FA70`. Its per-owner lookup
`0x009600E0` can create the setter proxy through `0x00993650`,
`0x009932A0` and `0x0095F990`. The constructor calls the DependencyObject
constructor at `0x0095F9BD`, sets proxy `+0x20` mask `0x02` at
`0x0095F9C2`, and stores the original Label at `+0x2C` at `0x0095FA16`.
The Binding constructor reads this flag and calls proxy virtual `+0x0C`
at `0x009123B9`. That slot selects `0x00657330` and returns the Label,
which is stored as the forward write receiver at Binding `+0x44`.

The [ElementName source arm](sqwt-binding-source-arms.md#what-the-constructor-reads)
supplies an initially null source. The parser copies `ElementName` into
Binding `+0x64` at `0x00912505`, stores mode 1 at `0x00912755`, and
appends the binding to the proxy's deferred list at `+0x14` through
`0x009127F3..0x0091282D`. The proxy enters the load context's activation
queue at `+0x20D8` through `0x00912E9F` during construction.

On the ordinary textual form path, the reader calls `0x0096A580` at
`0x0096A8A2` before load completion. The normal completion branch
`0x0096AC4E..0x0096AC59` merges the load context through `0x009925C0`
and drains activation through `0x00910CD0` before notifying form
completion listeners at `0x0096ACD3`. The merge appends the activation
queue at `0x00992AEA..0x00992AF0` and transfers per-owner attached
provider entries for matching registers through `0x00992EB8`,
`0x009911A0` and `0x0095F4D0`. Thus the normal drain follows form parsing
and makes the transferred proxies available to the receiving context.

Proxy activation cell `0x0106F4AC` selects `0x00912190`. Its deferred
walk calls `0x00911F50` at `0x00912252`. With a null source, this routine
reads Binding `+0x64`, parses the element name at `0x009120B3` and calls
`0x00910980` at `0x009120C5` with parent fallback enabled. The walk can
ascend from the proxy to its owning Label and from the Label to the
Canvas through Label slot `+0x0C` (`0x0092B5D0` reads `+0x240`).
Canvas child add at `0x00995676` calls `0x009313C0`, which can store the
logical parent at `0x0093145B` and append the child at `0x00931482`.
Canvas slot `+0x20` uses `0x00910A10`, which enumerates its logical
children and compares their name keys at `+0x1C` (`0x00910A4B..0x00910A50`).
The resolver can therefore select a peer declared later in the form
when the expected named siblings have been added before the drain. This
walk is separate from the [Lua FormElement cache](sqwt-name-lookup-cache.md#lookup-0x00537950).

On a hit, `0x009120CD` sets the source at Binding `+0x3C`.
The simple `Name` Path resolves descriptor `0x01356BC8` and stores it
at Binding `+0x5C` through `0x00912110..0x00912128`. Registration at
`0x0091213B` requests the source listener for `Name` and the target
listener on the proxy. The registration helper can skip insertion when
the container is unavailable (`0x00911E9A..0x00911E9F`). The attach
still returns true at `0x00912140` on that path. A successful deferred
attach invokes Binding activation at `0x0091229B`, which requests the
initial OneWay transfer. Later notifications require an inserted listener.
The forward arm reads the source name through `0x00911020` at
`0x00999466` and requests a flag-0 write on the owning Label at
`0x00999A07`. The qualified setter selects that Label's provider proxy
again. Both string descriptors use `0x0095F6B0` to write proxy `+0x24`
through `0x0095E830`. An admitted change can notify the owning Label
through `0x0095D890`. Label virtual `+0x18` selects `0x00934B60`,
which reaches the dependency listener path through `0x0092DF20` and
`0x0093BD80`. This supplies the owner-side notification for the
storyboards' `StringData` bindings described below when registered.

A missing source returns false at `0x00912144`. The deferred walk
then deletes that Binding at `0x00912280`, zeros its entry at
`0x0091228C` and releases the deferred list at `0x00912324..0x0091232C`.
This failed entry cannot wait for a later peer. On a successful attach,
the `Name` setter at `0x00912D20` can publish a changed value through
`0x009CB580` and `0x009CB460`. Its callback updates the name key at
`0x00912B5D` and requests listener delivery at `0x00912B69` when the
event flag is set. Mode 1 requires the retained source pointer and
`Name` descriptor to match at `0x00998A8A..0x00998A94`. It rereads
that source's current name without resolving the original `ElementName`
again on this notification path.

This establishes the conditional initialization and rename routes. Live
form selection, registration and write admission, replacement or removal
of a bound peer, and timing under reentry remain unresolved.

## OBSERVED ControlTemplate owner and binding

| Locator | Observation |
| --- | --- |
| `0x009386B6..0x009386DE`, `0x00936C0F..0x00936C14`, `0x0099B450` | Template property application constructs `ControlTemplate` with the requesting control. The base constructor retains the control at template `+0xC8`. The selected path materializes the resource before requesting Template assignment. |
| `0x0106D9D4`, `0x0094BE34..0x0094BE6E`, `0x0094E9FA` | The FrameworkTemplate markup child callback recognizes `.Triggers`. It passes the native template from markup `+0x124` to `0x0094E9A0`, which stores it at Triggers markup `+0x74`. |
| `0x0106D9F8`, `0x00949716..0x0094973F`, `0x009C2694..0x009C2697` | The `CommandTrigger` parser passes that template and its `+0xC8` owner to `0x009B1110`, then adds the trigger to the template collection at `+0x24`. The TriggerBase constructor stores the template and control at trigger `+0x3C` and `+0x40`. |
| `0x00949970`, `0x00949D8C..0x00949D93`, `0x00949F49..0x00949F93` | The parser adds BeginStoryboard to the trigger, Storyboard to the action, and the property animation to Storyboard before reading each one's contents. The collection callbacks preserve the parent links. See [the shared action and timeline construction](sqwt-style-reverse-binding.md#observed-templated-parent-for-a-style-animation). |

Attribute cell `0x0106DC8C` selects `0x00946C40`, which sends the property
animation's markup attributes through `0x00946A30` to `0x00912850`.
Initializer `0x00F27800` names attached descriptor `0x01360130`
`Storyboard.TargetName`. Registration at `0x00992020` constructs the string
provider with property info `0x01360154`. The qualified setter fallback at
`0x0095EDD4..0x0095EDF6` obtains its per-receiver proxy and calls `0x00912850`
on that proxy. The proxy constructor stores the original timeline at
`+0x2C` at `0x0095FA16`.

The binding parser calls receiver virtual `+0x24` for `TemplatedParent` at
`0x00911C19..0x00911C28`. The proxy's `+0x24` and `+0x0C` cells
`0x0106F4A8` and `0x0106F490` select `0x00910AB0` and `0x00657330`.
These forward the request through the proxy's original timeline. The
property animation's cell `0x0106D758` continues through Storyboard and
BeginStoryboard to the CommandTrigger subobject. CommandTrigger cell
`0x0107D2BC` selects `0x009C24E0`, which returns the control retained at
full trigger `+0x40`. On this construction path, the binding source is the
control requesting the ControlTemplate.

`OneWay` stores mode 1 at `0x00912755`. Activation at `0x009993A2` or a
matching source notification at `0x00998A96` calls Binding virtual `+0x10`
at cell `0x01079CE8`. Its body `0x009993C0` reads source object and
descriptor `+0x3C` and `+0x5C` without a converter at `0x00999458..0x00999466`.
On the direct text path it requests a flag-0 write to target object and
descriptor `+0x44` and `+0x60` at `0x009999F9..0x00999A07`.

The owner's `Name` descriptor `0x01356BC8` uses DependencyObject getter
`0x00910CB0`, which copies owner `+8`. The `StringData.Value0` and
`StringData.Value1` descriptors are `0x0135FA28` and `0x0135FA70`.
For those qualified names, `0x00910F60` keeps the owner in its object
output at `0x00910F70` and finds the descriptor through `0x0095EF70` at
`0x00910FF1`. The attached getter `0x0095EE10` resolves the corresponding
per-owner value. The [qualified property study](sqwt-qualified-property-lookup.md#qualified-generic-lookup)
records the provider lookup gates.

## OBSERVED target cache and Begin

The timeline markup constructor walks parent links at
`0x0094F0B0..0x0094F0B9`. It passes a non-null terminal markup `+0x10`
to `0x009A69E0` at `0x0094F0C2`, seeding the timeline's target pointer in
the thread-local map at context `+0x213C` before attributes are read.
The terminal markup holds the requesting control on this template path.
See the [template receiver record](sqwt-template-initial-enabled-receiver.md).

The attached string setter at `0x0095F6B0..0x0095F6D2` passes proxy
`+0x24` to `0x0095E830` with notification enabled. A changed admitted
value can notify the original timeline through `0x0095D890`.
Timeline virtual `+0x18` selects `0x009AC500` for this animation. On a
`TargetName` descriptor match, it reads the attached text at `0x009AC56D`,
parses the name path at `0x009AC582`, and calls `0x00910980` at
`0x009AC592` with parent fallback enabled. `0x009A69E0` at `0x009AC599`
stores the resulting pointer, including null, at map node `+0x24`.
On parent fallback, CommandTrigger cell `0x0107D2A4` selects `0x009C24D0`,
which returns the template from full trigger `+0x3C`. ControlTemplate cell
`0x00FCB0C8` selects `0x0099AF70`, which returns the requesting control.
The separate `TargetProperty` arm reads attached descriptor `0x01360178`
and stores its parsed property descriptor through `0x009A6990` at
`0x009AC5F4`.

Storyboard Begin `0x009AC4B4` and child preparation `0x009AC364` both
check the target before calling setup. Lookup `0x009A6B50` returns a
non-null cached pointer at `0x009A6BBF..0x009A6BC4`. A missing or null
entry retries the attached name through `0x009A6A20`. Thus an admitted
binding change can replace the seeded target before Begin, and Begin can
retry a failed lookup. A non-null cached pointer bypasses the retry.

## OBSERVED discrete keyframe writes

Markup callback `0x00949FF0` accepts `DiscreteValueKeyFrame` and creates
and appends a `StringKeyFrame` through `0x009AE680` at `0x0094A04D`.
Descriptor `0x01363840` selects `KeyTime` at frame `+0x28` and uses the
timespan parser `0x009152B0` through `0x009CB4F0`. Descriptor `0x01363864`
selects `Value` at frame `+0x38` through `0x009CB580`. The respective
setter calls are `0x009CB0DC` and `0x009CB0BA`.

Setup cell `0x0106D780` selects `0x009AE290`. Setup orders the frames,
supplies the final KeyTime to the Duration setter at `0x009AE35D`, and
copies KeyTime and Value text into the sample records at
`0x009AE3F3..0x009AE478`. It resets the cursor and timeline counters.
Evaluator cell `0x0106D784` selects `0x009ADDB0`. On each sample it checks
the target through `0x009A6B50` and the property through `0x009A66C0`
(`0x009ADDBA`, `0x009ADDC6`). With a non-null target and a nonempty
property descriptor, the forward arm compares sample record qword `+0`
against keyframe time. For each due record it passes the retained Value
text at record `+8` to `0x00912850` with flag 0 at
`0x009ADEB2..0x009ADEC1`, then advances the cursor by `0x60`.

An admitted zero-span callback can supply an elapsed-zero record at
`0x009ACAB2..0x009ACB68`, which can apply a due zero-time keyframe.
The keyframe setup copies the values and prepares the cursor before the
callback.

## Storyboard child order and zero-span writes

Each selected Storyboard has eight direct timeline children. In declaration
order, four outgoing children bind their target name to the owner's `Name`,
followed by four incoming children that bind to `StringData.Value0` for Next
or `StringData.Value1` for Prev.

| Children in order | Properties in order | Next declaration lines | Prev declaration lines |
| --- | --- | --- | --- |
| 1-4, outgoing | `Visibility`, `Canvas.Top`, `VisualOpacity`, `SqwtDrawOrder` | 317, 320, 321, 322 | 336, 339, 340, 341 |
| 5-8, incoming | `Visibility`, `Canvas.Top`, `VisualOpacity`, `SqwtDrawOrder` | 323, 326, 327, 328 | 342, 345, 346, 347 |

Both `Canvas.Top` and both `VisualOpacity` children in each Storyboard
declare `Duration="0:0:0.2"`. The outgoing and incoming `SqwtDrawOrder`
DoubleAnimations declare `From`/`To` of `0.5`/`0` and `0`/`0.5`,
respectively, and omit `Duration` and `Velocity`.

The DoubleAnimation and discrete markup arms at `0x00949EB9` and
`0x00949F5A` call `0x0094E2B0` before reading the child contents.
The call at `0x0094E2C8` appends at the collection end through `0x0096C140`
(`0x0096C169..0x0096C19C`). Storyboard setup cell `0x0107C378` selects
`0x009CAB40`. It walks child pointers from group `+0x138` to `+0x13C`
in increasing order, calls target-gated child preparation at `0x009CABA4`,
and passes the largest returned span to the group Duration setter at
`0x009CABE1` before resetting the group at `0x009CABF0`. Storyboard
evaluator cell `0x0107C37C` selects `0x009CAC70`. It walks the same
collection forward, passing its input record unchanged to each child
clock at `0x009CACB7` before advancing at `0x009CACC6`. On a normal
visit without reentry or collection changes, this preserves declaration
order for preparation and sampling.

For a freshly prepared, unpaused group with positive finite span, zero
BeginTime, SpeedRatio one, repeat count one and AutoReverse false, an
elapsed-zero input takes the below-span branch at
`0x009ACA7B..0x009ACAAC`. Its remainder is zero at
`0x009ACBE2..0x009ACBF1`, so control skips the group evaluator via
`0x009ACC88` and reaches the count check at `0x009ACECD`. With nondecreasing input
and no overflow, the children first receive the first admitted nonzero
group sample. That sample can already be the group's terminal sample.
An incoming zero-time `Visibility` keyframe can therefore be due on
that child visit even though the group skipped its elapsed-zero sample.

The Timeline constructor supplies the zero-filled qword at `0x013637C0`
to the Duration wrapper at `0x009AD637..0x009AD64D`. With retained
zero Duration and unset Velocity, DoubleAnimation setup still divides
by Duration at `0x009B49BF`. If setup returns normally and the freshly
prepared, unpaused child has zero span and the same forward, single-repeat
clock settings, its first admitted nonnegative input reaches the terminal
branch at `0x009ACAB2..0x009ACB68`. That branch supplies zero in sample
qwords `+0` and `+8` before advancing the count at `0x009ACB6A`.
DoubleAnimation evaluator cell `0x0107DD24` selects `0x009B4A70`.
With a resolved target and property, the sample at `+8` equals the
evaluator duration, so `0x009B4AE3..0x009B4B03` selects the retained
endpoint without using the setup rate. The evaluator formats the
result and calls `0x00912850` at `0x009B4CA0`. With the declared `To`
and default offset retained, the outgoing lane requests `0` and the
incoming lane requests `0.5` on their respective visits.

The zero-duration setup exception state, live binding activation,
namescope and target lifetime, setter admission, callback cadence, and
rendered order remain unresolved. See the
[bounded clock path](sqwt-double-animation-clock.md#bounded-nonrepeat-samples-and-completion)
for the shared sample and completion gates.

## OBSERVED mixed numeric target writes

The selected Next and Prev DoubleAnimations use the same outgoing Name and
incoming peer paths as the discrete tracks. Every numeric child binds
`Storyboard.TargetName` OneWay from TemplatedParent. The declared endpoints
and durations are as follows. Each arrow denotes the declared From and To.

| Track | Next outgoing / incoming | Prev outgoing / incoming | Duration |
| --- | --- | --- | --- |
| `Canvas.Top` | 0 -> -20 / 20 -> 0 (320, 326) | 0 -> 20 / -20 -> 0 (339, 345) | 0:0:0.2 |
| `VisualOpacity` | 1 -> 0 / 0 -> 1 (321, 327) | 1 -> 0 / 0 -> 1 (340, 346) | 0:0:0.2 |
| `SqwtDrawOrder` | 0.5 -> 0 / 0 -> 0.5 (322, 328) | 0.5 -> 0 / 0 -> 0.5 (341, 347) | Omitted |

The DoubleAnimation markup arm appends the timeline at `0x00949EB9`,
calls the shared target-seeding markup constructor `0x0094F030` at
`0x00949EC4` and then reads the contents at `0x00949EE9`. Attribute
cell `0x0106DC68` selects `0x00946C20`, which calls the same generic
attribute handler `0x00946A30` as the discrete lane. The native
DoubleAnimation and discrete timeline cells have the same functions
for these joins.

| Slot | DoubleAnimation cell | Discrete cell | Shared function |
| --- | --- | --- | --- |
| `+0x0C`, owner | `0x0107DCE0` | `0x0106D740` | `0x009AC410` |
| `+0x24`, TemplatedParent | `0x0107DCF8` | `0x0106D758` | `0x00910AB0` |
| `+0x18`, property change | `0x0107DCEC` | `0x0106D74C` | `0x009AC500` |

Thus the same attached string proxy, owner walk and timeline target cache
apply. The `Storyboard.TargetProperty` change reads the attached text,
constructs its generic property key at `0x009AC5E9` and caches it at
`0x009AC5F4`. The DoubleAnimation evaluator calls target lookup
`0x009A6B50` at `0x009B4AB2` and cached property lookup `0x009A66C0`
at `0x009B4ABE`. A nonnull target and nonempty property key are
required at `0x009B4AC8..0x009B4ADD`. The evaluator formats its
binary32 sample through `0x00988D50` at `0x009B4C88` and calls the
generic text setter on that target with flag 0 at `0x009B4CA0`.

For an admitted sample on a resolved page Label, the native sinks are as
follows.

- `Canvas.Top` takes the qualified setter's attached fallback at
  `0x009128F3`. Its descriptor `0x0135FEA8` registers a float provider
  at `0x009921C2..0x009921EA`. The provider's `+4` cell `0x01078594`
  selects the per-owner lookup `0x009600E0`. Its float setter
  `0x0095EAF0` passes proxy `+0x24` to `0x0095E7B0` at
  `0x0095EB10`. An admitted changed value can notify the Label through
  `0x0095D890`. Label virtual `+0x18` reaches `0x0093BD80` through
  `0x00934B60` and `0x0092DF20`. The event's `+0x78` mask `0x02`
  and the descriptor match at `0x0093C10C` admit the parent route.
  Label `+0x60` selects `0x0092AF60`, returning the logical parent at
  `+0x240`. With that parent present, `0x0093C20E` calls
  `0x0093B250` on the Canvas to queue arrange work. Canvas arrange
  checks the child property at `0x009B9712` and reads it at
  `0x009B973F`, with numeric conversion at `0x009B974E`. The
  resulting Y value enters the child arrange rectangle at
  `0x009B981D..0x009B9845` on the admitted path.
- `VisualOpacity` selects inherited Visual descriptor `0x0135988C`
  and index 4. Setter cell `0x009400EC` selects `0x0093FFC6`, which
  passes Label `+0x48` to the same float wrapper at `0x0093FFE0`.
  The `0x0093FE00` callback can send the four Visual opacity values
  through drawing context `+0x44` at `0x0093FEBF..0x0093FEE8` when
  `0x0135C2B0` is zero and the context is nonnull. DrawingToolContext
  cell `0x00FA2928` selects `0x0055A050`, which stores overall
  opacity first at tool state `+0x70` and sets dirty mask `0x08`.
  The [tool flush](npcsay-backdrop-draw.md#from-element-to-quad-code)
  supplies these 16 bytes as scene packet `0x15` (local command 0).
  On the [ordinary WindowActor route](sqwt-icon-control-textures.md#observed-ordinary-control-to-renderer),
  WindowContext cell `0x008CF3B8` selects `0x008CECCB`. It calls
  `0x008CC2A0`, which writes the four body values to WindowContext
  `+0x11C`, `+0x110`, `+0x114` and `+0x118`, in that order. The
  WindowContext draw path at `0x008CE83D..0x008CE8ED` copies those local
  lanes into effective state at `+0x100..+0x10C`. With a parent, it
  multiplies the four lanes by the parent's corresponding effective values.
- `SqwtDrawOrder` takes the inherited Visual index-11 path described
  under [local paint order](#observed-local-sqwtdraworder-paint-order).
  Both zero-span lanes still require normal setup return and a supplied
  terminal sample. With default Velocity retained, setup divides by
  the zero duration at `0x009B49BF`. An admitted Q=0, D=0 evaluator
  takes the endpoint copy at `0x009B4AF7..0x009B4AFD` and can request
  the outgoing 0 and incoming 0.5 writes, in group declaration order.
  The equal-value store gate and paint ordering conditions still apply.

The positive-duration lanes omit Velocity and acceleration controls. With
the default branches retained, setup forms the absolute endpoint difference
at `0x009B4947..0x009B4968`, then stores the binary32 rate after scaling
by 1000 and dividing by D at `0x009B4977..0x009B49C1`. The nonterminal
linear evaluator adds the rate contribution to the start for ascending
endpoints at `0x009B4C00..0x009B4C0A` or subtracts it for descending
endpoints at `0x009B4C54..0x009B4C5E`. The shared terminal branch
copies the retained To endpoint. See the [default clock arithmetic](sqwt-double-animation-clock.md#default-timericon-percent-evaluation)
for the separate binary32 stores and [bounded samples](sqwt-double-animation-clock.md#bounded-nonrepeat-samples-and-completion)
for the scheduler gates.

This joins the selected native write paths to the resolved Labels. Live
binding activation and property admission, runtime floating-point exception
state, later arrange and render scheduling, and the effect of WindowContext
opacity on final compositing remain unresolved.

## OBSERVED page storyboard completion target

The Next and Prev Storyboards at template lines 316 and 335 both declare
`CompletedCommand` as `UILuaCommands.FinishAnimation`. Their
`CompletedCommandTarget` bindings specify `RelativeSource=TemplatedParent`
and omit both `Path` and `Mode`.

Storyboard construction initializes the pointer holder at `+0x160`
to null with descriptor `0x013616D0` at `0x009A68D0..0x009A68EC`. Before reading
the storyboard attributes and children at `0x00949DC3`, the common
markup handler walks parent `+0x0C` links at `0x00949D5C..0x00949D79`.
When terminal markup `+0x10` holds a nonnull owner, the handler passes it
to the holder setter `0x009A65A0` at `0x00949D87`. On the selected
ControlTemplate construction path that owner is the requesting outgoing
Label. The preseed precedes processing of the explicit binding.

Storyboard markup attribute cell `0x0106DC44` leads through `0x00946C00`
and `0x00946A30` to the generic property setter. The `CompletedCommandTarget`
descriptor selects the direct Storyboard setter arm at
`0x009A6C24..0x009A6C45`. It uses `+0x160`, separate from the attached
`TargetName` proxy and timeline target cache. TemplatedParent dispatch
at `0x00911C19..0x00911C28` starts at the Storyboard itself. Its cells
`0x0107C350` and `0x0107C338` select `0x00910AB0` and `0x009AC410`,
which follow timeline owner `+0xE0`. BeginStoryboard's `0x009ADC90`
continues to the CommandTrigger subobject, where `0x009C24E0` returns
the same Label from trigger `+0x40`.

Binding construction initializes mode 0 at `0x00999C85`. An omitted path
leaves the source descriptor null at `0x009120D7..0x009120E3`. Storyboard
activation cell `0x0107C354` selects the shared listener walk `0x00912190`.
The Label activation path through `0x00937CD0` and `0x0092A060` also calls
that walk at `0x0093CC5D`. On admitted activation, `0x00999350` retains
the source and target endpoints through
`0x00917220` and dispatches the mode-0 forward arm at `0x00999390` to
`0x009993C0`. The null source descriptor selects `0x00999A23`, which formats
the source pointer through `0x00915D00` and `0x00917FB0` and requests a
flag-0 target write at `0x00999A67`. The holder setter calls `0x00918760`
through `0x009169A0` at `0x009A5F20`. Its pointer decoder `0x00916860`
looks up the parsed pointer in the Sqwt thread-local map at context
`+0x2118`, and `0x009187AF` casts the result to `FrameworkElement`.
The holder helper `0x009C3820` skips pointer replacement on equality at
`0x009C3877..0x009C3879`. For an admitted changed value it retains the new
pointer, releases the old pointer, and stores the result at
`0x009C388F..0x009C38A2`.

In the forward single-repeat terminal branch described above, the group
evaluator call at `0x009ACB68` walks the child clocks at `0x009CACB7`.
With normal returns and no reentry or collection changes, the child
visits finish before the group completion call at `0x009ACEDD`.
Storyboard cell `0x0107C384` selects `0x009CAB00`, which checks for the
group's own completion and enters `0x009AD450`. That routine disables
the timer, clears running byte `+0x128` and calls `0x009A6490` through
cell `0x0107C380` at `0x009AD478`. With a command at `+0x158`, the
callback reads the retained target directly from `+0x160`. It adds
`0xB4` when nonnull and calls the command's `+8` slot at `0x009A64C9`.
The `CompletedCommand` setter at `0x009A6C4E..0x009A6CAE` uses name
lookup `0x00928280` and fallback `0x00928F90`. For a retained
`RoutedCommand` and nonnull Label target, the [explicit command route](sqwt-command-parameter-consumer.md#target-and-route)
raises `Executed` from the outgoing Label if `PreviewExecuted` allows it.

### Interpretation and limits

The preseed supplies the outgoing Label. A successful forward transfer
of that same Label leaves the retained pointer in place. Completion
uses the pointer then held, with a null-target branch if the holder is
empty. The group order places any admitted writes from its terminal
child visits before this dispatch. The Lua `FinishAnimation` handler
clears `work.slotMoving` on delivery. See the
[page-control script record](https://github.com/XIVLegacy/xivl-client-scripts/blob/880a3e742f123b7e0e7c502c8ee1047eb8fed3dd/docs/action-menu-page-controls.md#L67-L97).
The trace does not establish live binding activation, child write
admission, behavior under reentry, or receipt of the completion by Lua.

## OBSERVED local SqwtDrawOrder paint order

The checked form declares `Label_MenuSlot_1` through `Label_MenuSlot_4`
as direct children of the same Canvas at form line 43. None of the four
labels declares a local `SqwtDrawOrder` attribute. Canvas child-add cell
`0x0107FE90` selects `0x00995650`. With a nonnull child, it adjusts the
receiver from the `+0x194` subobject and calls `0x00940B30` at
`0x00995663`. That routine inserts the child and stores the parent
at child `+0xA4` (`0x00940B5D..0x00940B69`).

Visual descriptor `0x01359988` is named `SqwtDrawOrder` and has index
`0x0B` (`0x00F23390..0x00F2339F`). The Visual constructor initializes
its float at `+0x70` to zero at `0x00940701..0x0094071B`. The constructor
chain preserves the receiver for Label and Canvas. Label reaches
FrameworkElement through ContentControl and Control at `0x009742CA`,
`0x009361BC` and `0x00938CAC`. Canvas reaches FrameworkElement through
Panel at `0x009B98CA` and `0x00995B9D`. Both reach the Visual constructor
through UIElement at `0x0092F66D` and `0x0093C78C`. Visual property
info uses setter dispatch `0x0093FF90` (`0x00F233DA`), whose index-11
cell `0x00940108` selects `0x009400BA`. That arm parses the numeric text
through `0x0095E7B0`. On a changed admitted value, `0x009413B6..0x009413CF`
stores the binary32 value and requests notification. Callback `0x0093FE00`
selects the index-11 arm through cells `0x0093FF8B` and `0x0093FF78`, then
calls `0x0093FD70` at `0x0093FF35`. An equal finite value takes the skip
at `0x009413B4`.

With a parent present, `0x0093FD70` reads the child's old index, removes
it and reinserts it into that same parent's collection at `+0xAC`
(`0x0093FD73..0x0093FDAA`). Adding a second child promotes the single
slot to a list and sets parent `+0xB0` mask `0x01`
(`0x0093FC62..0x0093FCBB`). In the list case, `0x0093F560` searches
backward. The x87 comparison at `0x0093F5DC..0x0093F5EC` stops at the
last existing finite value less than or equal to the incoming value.
`0x0093FBE0` inserts after that entry, or at the front if there is no
match. Finite values therefore sort in ascending order. A new insertion
or reinsertion at an equal value follows the existing ties.

When the child index changes, global byte `0x0135C2B0` is zero, and
child `+0xA0` holds a drawing context, `0x0093FDD4..0x0093FDE6` passes
a predecessor to drawing-context virtual `+0x8C`. Helper `0x0093F730`
searches backward from the child for the preceding sibling with a
nonnull drawing context and can return null. DrawingToolContext cell
`0x00FA2970` selects `0x0054BB10`, which submits local operation
`0x0D` with the child and predecessor contexts when the command producer
is present. See the
[local command transport](sqwt-icon-control-textures.md#observed-ordinary-control-to-renderer).

WindowActor dispatch selects `0x007FA883` for `0x0D` and resolves the
child's renderer context through registry `0x0134E0A4`. A successful
lookup reaches WindowContext dispatch at `0x007FA8FF`. Cell `0x008CF3EC`
selects `0x008CF2E8`, which resolves a nonnull predecessor when supplied
and requires an existing renderer parent at child `+0x70`. The arm
removes and reinserts that child in the same parent at
`0x008CF34C..0x008CF35F`. Insertion `0x008CE440` puts it after a
predecessor found in that parent's child list, at the beginning for a
null predecessor, or at the end for a resolved predecessor absent from
the list (`0x008CE45A..0x008CE4F5`). Renderer creation and initial
parenting also request this list insertion through interface `+0x0C`
using the preceding drawing context (`0x0093F880`, `0x0054BBB0`,
`0x0093F9CD..0x0093F9F9`, `0x007FA504..0x007FA514`).

WindowContext draw `0x008CE7D0` requires state `+0x128` equal to 3 and
byte `+0x12B` nonzero, subject to the actor pass check at
`0x008CE7D9..0x008CE7F9`. When the child pass is admitted at
`0x008CEB47..0x008CEB58`, it visits the parent context's `+4/+8` child
vector forward and recursively calls draw at `0x008CEB85`. On the path
where the renderer list mirrors visual sibling order, an admitted
child at `0` is visited before a child at `0.5` under the same parent.
Maintaining that order requires corresponding parentage and initial order,
then each changed index using a null predecessor or one present in that
renderer list. An unchanged visual index sends no reorder command.
The actual ActionMenu targets, accepted renderer commands, visibility,
resulting pixels and input ordering remain unresolved.
