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
callback. Live ActionMenu binding activation, namescope and target lifetime,
setter admission, and the relative scheduler order of the sibling
animations remain unresolved.
