# Sqwt style animation reverse binding

A `OneWayToSource` binding can carry an animation property back to the
control that owns its style. The checked `TemplatedParent` chain for a
style's `DoubleAnimation` goes through its storyboard, action, and trigger.

## Input and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86 little endian, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI queries locate the
calls and tables. LLVM objdump 21.0.0 decodes the selected instructions,
and original PE section reads verify strings and dispatch cells. The
selected declaration is recorded in the
[ActionMenu TimerIcon style](action-menu-timer-icon-style.md#observed-form-and-style-declarations).

## OBSERVED templated parent for a style animation

For `TemplatedParent`, the generic binding parser calls the attribute
receiver's virtual `+0x24` and uses its return as the source object at
`0x00911C04..0x00911C28`. See the
[binding source arms](sqwt-binding-source-arms.md#pre-parser-arms).

| Locator | Observation |
| --- | --- |
| `0x00F21FE0..0x00F21FF9`, `0x00930463..0x00930475`, `0x00930A84` | `Style` descriptor `0x01358A30` has index 15. The FrameworkElement setter subtracts two from the index and selects `0x0093077B`. |
| `0x0093081A..0x00930833`, `0x0099B450` | The allocation path passes the control receiver to `0x009A6230`. Its base constructor stores that receiver at Style `+0xC8`. The materializer `0x0099AF80` passes the stored owner to `0x0096A610`. |
| `0x009497A7..0x009497B4`, `0x009C2694..0x009C2697` | The `Trigger` markup path passes the Style and its `+0xC8` owner to `0x009B0990`. The TriggerBase constructor reached through `0x009B1BD0` stores them at Trigger `+0x3C` and `+0x40`. |
| `0x00949970`, `0x0106DA5C`, `0x0094EB10..0x0094EB5A` | Before reading the `BeginStoryboard` contents at `0x0094998D`, the markup path adds the action to Trigger's collection at `+0x5C`. The collection holds Trigger's `+0x18` subobject at collection `+0x1C` (`0x009B1C8A`). Collection cell `0x0107D50C` selects `0x009B2170`, which copies this pointer to the action at `+0x40`. |
| `0x00949D8C..0x00949D93`, `0x00949EA4..0x00949EB9` | The markup path adds Storyboard to the action's collection at `+0x44`, and DoubleAnimation to Storyboard's collection at `+0x130`. Both call `0x0094E2B0` before loading the respective contents at `0x00949DC3` and `0x00949EE9`. |
| `0x0094D7A5`, `0x009CAEAD`, `0x0106D718` | The respective collections hold the BeginStoryboard `+0x18` subobject and the Storyboard pointer at collection `+0x20`. Collection `+0x0C` selects `0x009AC430`, which copies that owner into the added timeline at `+0xE0`. |

The DoubleAnimation markup attribute handler in cell `0x0106DC68` uses
the animation at markup `+0x74` at `0x00946C29`. Its helper `0x00946A30`
passes the attribute to the generic property setter `0x00912850` at
`0x00946A8B`.

The DoubleAnimation, Storyboard, and BeginStoryboard dependency-object
tables use `0x00910AB0` for virtual `+0x24` at `0x0107DCF8`,
`0x0107C350`, and `0x0106D7BC`. This method calls the receiver's virtual
`+0x0C`, then calls the returned parent's virtual `+0x24` if non-null.
For the two timelines, `+0x0C` is `0x009AC410`, which reads `+0xE0`.
For the action subobject it is `0x009ADC90` at cell `0x0106D7A4`, which
reads subobject `+0x28` and returns Trigger's `+0x18` subobject. Trigger
virtual `+0x24` at `0x0107D0C0` selects `0x009C24E0`, which reads
subobject `+0x28`, the owner stored at full Trigger `+0x40`.

The resulting source on this construction path is the style's control
receiver. For the checked ActionMenu declaration, that receiver is a
TimerIcon when its style is materialized through this path.

## OBSERVED listener and reverse transfer

The binding constructor `0x00912340` recognizes `OneWayToSource` at
`0x01068414` and writes mode 3 to Binding `+0xA4` at `0x00912733`.
The property receiving the binding is the target. Binding `+0x44` and
`+0x60` hold the target object and descriptor. `+0x3C` and `+0x5C`
hold the source object and descriptor.

On the immediate attach path at `0x009127D8..0x009127EC`, `0x00911F50`
resolves the source Path. For a simple name, `0x00910F60` keeps the supplied
source object in its object output and resolves the property descriptor.
`0x00911F50` stores that descriptor at Binding `+0x5C` (`0x00912128`).
`0x00911ED0` registers the binding for the target descriptor and for the
resolved source descriptor through `0x00911E40` and `0x00914250`. The latter
inserts a descriptor and binding pair through `0x00913C10` in the object's
listener container at `+0x18`.

| Locator | Observation |
| --- | --- |
| `0x00911525`, `0x01079CE0` | Property notification calls registered listeners through virtual `+8`. Binding uses `0x00998A40` there. |
| `0x00998A41..0x00998A6B`, `0x00998AC8`, `0x00998AA3..0x00998AB4` | With both object and descriptor pairs populated, mode 3 checks the changed descriptor against target `+0x60` and the sender against target `+0x44`. A match dispatches Binding `+0x14`. |
| `0x01079CEC`, `0x00998B40..0x00998B65` | Binding `+0x14` selects `0x00998AD0`. With converter `+0x4C` null, it reads the target through `0x00911020` with flag 0. On success it passes the returned text to source property setter `0x00912850` with flag 0. |

The deferred branch at `0x009127F7..0x0091282D` can retain a binding in the
target's `+0x14` list. The separate activation method `0x00912190` visits
registered bindings and attempts to attach deferred bindings. It calls
`+0x0C` for registered and successfully attached entries. For Binding,
`0x01079CE4` selects `0x00999350`. Its mode-3 cell `0x009993BC` selects the
branch that can request a reverse transfer at `0x009993A6..0x009993AC`.

## TimerIcon property join and limits

The `AnimeTime` declaration supplies no converter. With the checked owner
and binding admitted, the reverse route reads the animation's Timeline
`AnimeTime` and requests the TimerIcon `AnimeTime` write. See
[Timeline AnimeTime publication](sqwt-double-animation-clock.md#timeline-animetime-publication)
and the [TimerIcon property setter](action-menu-timer-icon-style.md#observed-form-and-style-declarations).

The same owner supplies the declaration's forward input properties.
TimerIcon `ToPercent` is descriptor `0x01362F94`, index 2, named at
`0x00F2A590..0x00F2A5A9`. Getter cell `0x009BF8C8` selects `0x009BF696`,
which passes control `+0x2CC` to float formatter `0x00988D50`.
`TimerDuration` is descriptor `0x01362FB8`, index 3, named at
`0x00F2A5B0..0x00F2A5D3`. Getter cell `0x009BF8CC` selects `0x009BF6AF`,
which passes control `+0x2D8` to timespan formatter `0x009153D0`.

## OBSERVED attachment, activation and retention

The ActionMenu template's line 563 supplies a simple `AnimeTime` Path,
`TemplatedParent` source and `OneWayToSource` mode, with no converter.
The attribute is applied while the style is materialized at `0x00930833`,
before the Style assignment request at `0x00930842`.

The binding constructor builds the Path key at `0x00912785..0x00912792`.
Key constructor `0x009108E0` clears the qualifier field at `0x009108EC`,
so the plain key skips the qualifier lookup at `0x00912799..0x009127A0`.
With the nonnull TimerIcon returned by the checked `TemplatedParent` route,
`0x009127D8..0x009127EC` calls `0x00911F50` during attribute handling.
The deferred target-list branch at `0x009127F3` is bypassed on these inputs.

With both property descriptors resolved, `0x00911ED0` stores the target
registration owner at Binding `+0x9C` and requests insertion on
DoubleAnimation at `0x00911F1A` and TimerIcon at `0x00911F30`. Both use the
endpoint's listener container at `+0x18`. A missing container is allocated
by `0x00911E40`, and a null result skips insertion at `0x00911E9A..0x00911E9F`.
Each successful insertion stores the descriptor and Binding pair, appends
the container to the Binding's list at `+0x04`, and increments the Binding's
word at `+0x14` (`0x00914274..0x00914296`). The virtual call at `0x009142A3`
uses listener-container cell `0x01068508`, which selects `0x00776340`,
a return with no transfer.

The built-in TimerIcon markup branch calls its constructor at `0x0094B8D8`
before reading its attributes at `0x0094B907`. The constructor calls at
`0x009C0DD3`, `0x0096A25B`, `0x0092F66D`, `0x0093C78C` and
`0x009405EB` preserve the control receiver through to DependencyObject
constructor `0x00912DB0`. DoubleAnimation reaches that constructor at
`0x009B546C` and `0x009AD5BD`. The constructor appends the receiver to
the current Sqwt context queue at `+0x20D8` through `0x0096C140` at
`0x00912E9F`. The drain `0x00910CD0` removes the oldest entry and calls its
virtual `+0x28` at `0x00910D89..0x00910DC9`. A TimerIcon constructed
before its style animation enters the queue first when both insertions
use the same context.

TimerIcon cell `0x010824DC` selects `0x009C1A20`, which reaches generic
activation `0x00912190` before its own `RunningValue` tests. DoubleAnimation
cell `0x0107DCFC` selects `0x00912190` directly. Activation sets receiver
`+0x20` mask 1 at `0x00912199` and calls each registered binding's virtual
`+0x0C` at `0x009121ED..0x009121F5`. Either endpoint's admitted
activation can therefore reach this binding. Binding's activation sets
`+0xA0` to 1 at `0x009992BF`, checks the target object, source object and
target descriptor at `0x00999358..0x00999369`, and dispatches mode 3 to
the reverse transfer at `0x009993A4..0x009993AC`. That transfer still
requires the source descriptor and a successful target getter.

A changed Timeline `AnimeTime` can separately use the registered
DoubleAnimation entry. The notification route through `0x00912B60..0x00912B69`,
`0x009114A0` and `0x00998A40` checks the event flag, listener entry and
matching endpoints. It has no separate test of the object's activation bit
or Binding `+0xA0`. See
[Timeline AnimeTime publication](sqwt-double-animation-clock.md#timeline-animetime-publication)
for the producer's changed-value gates.

## OBSERVED replacement and detach boundaries

The admitted Style assignment uses the control's pointer holder at
`+0x1FC` (`0x0092F23F..0x0092F245`). On a changed pointer,
`0x009C388F..0x009C38A2` retains the new pointer, releases the previous
pointer and stores the new pointer. The old pointer's deleting virtual
call follows through `0x00915680` at `0x009C38D4` when the holder's
mask `0x10` and prior mask `0x08` are set and the old pointer is nonnull.

DoubleAnimation's deleting entry `0x009B5860` calls the Timeline destructor
at `0x009B5870`. The latter reaches DependencyObject cleanup via
`0x009AD56D` and `0x00911710` when the thread has a Sqwt context.
On its registered-list path, `0x00911200` clears Binding `+0x9C` and calls
container removal at `0x0091142E..0x0091144E` when the registration owner
and its `+0x18` container are nonnull. It clears the source endpoint and
requests removal at `0x00911450..0x00911472` when that endpoint and its
`+0x18` container are nonnull. Container cell `0x01068500`
selects `0x00914100` for removal.

The static path establishes where the declared binding attaches and how
either endpoint's activation can request the first reverse transfer.
The concrete ActionMenu load pass, the first admitted property write and
the complete replacement path through every owned collection remain
unresolved.
