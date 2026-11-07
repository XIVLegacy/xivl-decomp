# ActionMenu TimerIcon style and texture lookup

The standard ActionMenu command labels use a shared template with an ordinary
`IconControl` and a `TimerIcon`. The timer declares `TimerIconId` 526 and a
style that animates `Percent`. When the native timer renderer receives 526 in
the overlay slot, its icon lookup selects resource `0x1C59020E`.

## Inputs and method

The install's `game.ver` identifies build `2012.09.19.0001`. These are the two
standard named containers under `client/sqwt` and their decoded documents.

| Path | Container size | Container SHA-256 | Decoded size | Decoded SHA-256 |
| --- | --- | --- | --- | --- |
| `widget/ActionMenuWidget.form` | 40815 | `d8a9dffea87cf0d5cea066a7f3d27b4749d7c2ac8b8fff33ecf5bc40fc0711b1` | 40807 | `c576f44ef70c45cf50fd1b8a4b0dcaf813b665a607fb647482521a43dc5a8286` |
| `widget/ActionMenuWidget.tpl` | 47246 | `18c4967b39ebce0c0b8a82d5d706b569bd538a2a5ea6b8c54bcb434612538be6` | 47238 | `2c239989bd6bfc4022cd4a18499f78f9957b1791165f806644da4581698c4871` |

`xivl-cli` 0.1.0 from `xivl-tools` revision
`fd4c97d89eab3bbbf657021297e3d78156c63319` decoded both original basenames
with `extract-resource --as sqwt`. Each extraction passed
`verify-extraction --source` against its named container. Python Expat
supplies the decoded element locations below. See
`xivl-tools:docs/resource-extraction.md`, Extract one resource.

Native observations use the same `ffxivgame.exe` as the
[TimerIcon Percent study](sqwt-timer-icon-percent.md#input-and-method).
The PE32 x86 image has size 15996808, image base `0x00400000`, and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native locators are VAs. Ghidra CLI queries and LLVM objdump 21.0.0
identify the instructions. Original PE section reads confirm the property
string, callback table, renderer slots and dispatch cells.

## OBSERVED form and style declarations

`Window_ActionMenuWidget` merges `widget/ActionMenuWidget.tpl` at form line 5.
The 40 labels `Label_CommandIcon_1` through `Label_CommandIcon_40` reference
`ControlTemplate_Label_MenuIcon` at form lines 51 through 156. Their four
page ancestors `Label_MenuSlot_1` through `Label_MenuSlot_4` declare,
respectively, `Visible`, `Hidden`, `Hidden`, and `Hidden` at form lines 44,
76, 108, and 140.

The `Label` template contains `Grid_Command` with sibling
`IconControl_Command_1` and `IconControl_Command_2` at template lines 149
through 153. Both controls declare centered alignment and a 64 by 64 size.
`IconControl_Command_2` is the `TimerIcon`, with `TimerIconId` 526 and
`Style` referencing `Style_TimerIcon` as a static resource. Neither sibling
declares a local `Visibility`, `VisualOpacity`, or `Percent` value. The
ordinary `IconControl` declares no literal image ID or source there.

The `Label` template sets its owner's `RenderTransform` to `ScaleTransform`
at template line 150. `Grid_Command` declares 44 by 44 at line 151.
Neither that Grid nor the TimerIcon declares a local transform or
`ClipToBounds`. The `Label` template has no track for either scale axis.
It targets `RenderTransformOrigin` at lines 186 and 197. The containing
page template's ContentPresenter declares `ClipToBounds` false at line
312. These declarations leave effective ancestor state and arranged
placement open.

`Style_TimerIcon` targets `TimerIcon` in the same dictionary. It declares an
empty `SqwtIsAutoSetSkin` attribute and sets `SpkSource` to
`common/timerEffect.le.spk` at template lines 557 and 558.

| Template lines | Declaration | Action |
| --- | --- | --- |
| 560 to 564 | `RunningValue` 1 | Starts `TimerIcon_PercentAnime`. Its `DoubleAnimation` targets `Percent`. `From` binds to `ToPercent` and `Duration` binds to `TimerDuration`, both `OneWay` from `TemplatedParent`. `AnimeTime` binds `OneWayToSource` to `TemplatedParent.AnimeTime`. The animation declares no `To` value. |
| 567 to 574 | `RunningValue` 2, 3, or 0 | Pauses, resumes, or stops the same named storyboard, respectively. |
| 562, 576 to 579 | `TimerIcon.PercentAnimeEnd` | The storyboard names this as its completion command. The matching trigger sets `SpkSeq` to `No_anime` and then `Effect_start`, and executes `UILuaCommands.TimerEnd`. |

The Begin action names `TimerIcon_PercentAnime` with `Name`.
The Pause, Resume and Stop actions each use `BeginStoryboardName` for
that same value. Those attributes are at template lines 561, 568, 571
and 574 (OBSERVED).

The omitted `To` follows the native endpoint setup when the target and
property resolve. See
[DoubleAnimation setup and TimerIcon Percent capture](sqwt-double-animation-clock.md#setup-current-value-and-repeated-begin).
The [TimerIcon recast inputs](sqwt-timer-icon-recast-inputs.md#observed-recast-inputs-and-derived-values)
record the native calculation of `ToPercent` and `TimerDuration`, the
`RunningValue` notification order, and the `AnimeTime` text representation.
The [default Percent evaluator](sqwt-double-animation-clock.md#default-timericon-percent-evaluation)
records the native linear sample calculation and terminal endpoint write.

TimerIcon registers its `AnimeTime` descriptor `0x01362FDC`, index 4, at
`0x00F2A5E0..0x00F2A603` with the string at `0x0108220C`. Setter cell
`0x009C19C0` selects `0x009C1573`, which passes control `+0x2E8` to timespan
setter `0x009BF130`. The Timeline can publish its own `AnimeTime` before
evaluation. On the checked construction path, `TemplatedParent` resolves
to the style owner and an admitted `OneWayToSource` listener requests a
write to that owner. Live binding activation, delivery, and lifetime remain
unresolved. See
[Timeline AnimeTime publication](sqwt-double-animation-clock.md#timeline-animetime-publication)
and [Sqwt style animation reverse binding](sqwt-style-reverse-binding.md).

## OBSERVED Storyboard completion route

The declaration at template line 562 omits `CompletedCommandTarget`. On the
checked style materialization path, `0x0099B025..0x0099B04A` passes Style
`+0xC8` to `0x0096A610`. The terminal markup constructor called at
`0x0096A688` clears its parent at `+0x0C` and stores that owner at `+0x10`
(`0x00949356..0x00949359`). The Storyboard constructor initializes
`CompletedCommandTarget` descriptor `0x013616D0` with a null pointer at
Storyboard `+0x160` (`0x009A68D0..0x009A68EC`). The Storyboard markup reader
walks markup `+0x0C` to the terminal object at `0x00949D5C..0x00949D79`.
When its `+0x10` is non-null, the reader passes it to `0x009A65A0` with mode
0 and notification 1 before reading the Storyboard contents at `0x00949DC3`.
The helper passes descriptor `0x013616D0` and Storyboard `+0x160` at
`0x009A65AB..0x009A65C5`. The admitted pointer store is at `0x009C38A2`.
This path supplies the style owner as the default target for the declaration.

The `CompletedCommand` setter at `0x009A6C4E..0x009A6CAE` and the
`CommandTrigger` setter at `0x009552C1..0x0095531A` resolve their name
through `0x00928280` with fallback `0x00928F90`. The setters retain the
selected command at Storyboard `+0x158` and trigger `+0x80`, respectively.
The declaration gives both setters `TimerIcon.PercentAnimeEnd`. On an
admitted same-name resolution, both can retain the same command object.
The fallback can create a `RoutedCommand` on a registry miss. See
[command name resolution](sqwt-command-parameter-consumer.md#command-name-resolution).

When the completion hook finds a retained command, `0x009A64A7..0x009A64C9`
adds `0xB4` to a non-null target and calls command virtual `+8`. For
`RoutedCommand`, this is `0x00927FA0`. It routes `PreviewExecuted` first and
routes `Executed` only if the preview did not mark the event handled. With
the retained TimerIcon target, the explicit-target route sets unset original
source and source at `0x0091E20C` and `0x0091E217` and starts `Executed` at
that target. The null-target branch in `0x0091F590` instead calls
`0x0091EE90` when global `0x01357020` is non-null. See
[Storyboard completion](sqwt-double-animation-clock.md#bounded-nonrepeat-samples-and-completion)
and [routed target dispatch](sqwt-command-parameter-consumer.md#target-and-route).

TimerIcon interface cell `0x010823C0` selects `0x0092D060`. After the base
dispatch, `0x0092D075..0x0092D0B8` requires a clear event stop byte at
`+0x17` for this path. At `0x0092D0BE..0x0092D142` it reads the installed
Style at interface `+0x148` and calls entries in Style's trigger collection
at `+0x24` when that Style is present. The markup path
creates `CommandTrigger` at `0x0094972E..0x0094973F`. Its cell `0x0107D2DC`
selects `0x009B11F0`, which requires event id `0x2A` and event `+0x1C`
equal to trigger `+0x80`. It also checks a retained parameter filter when
present. On admission it calls the trigger action walker at `0x009B1256`.
The walker invokes admitted actions in collection order at
`0x009B198B..0x009B1996`.

For an admitted action without a target name, Setter cell `0x0107D5F8`
selects `0x009B22C0`, which resolves the event source at `+0x08` and requests
the property write at `0x009B2395`. ExecuteCommandAction cell `0x0107D7FC`
selects `0x009B28C0`, which reaches command dispatch at
`0x009B2B40..0x009B2B7E`. The template's action order thus supplies the two
sequence requests before the `UILuaCommands.TimerEnd` request on the
checked normal-return path. Live style activation, command allocation and
retention, interruption or reentry between actions, and delivery of
`TimerEnd` to a Lua subscriber remain unresolved. See
[form and subscription delivery](sqwt-command-parameter-consumer.md#subscription-gate).

TimerIcon registers `SpkSource` as descriptor `0x013630D8`, tag `0xB`, and
`SpkSeq` as `0x013630FC`, tag `0xC`, at `0x00F2A6D0..0x00F2A709`. The
callback `0x009C0330` reads the wrappers at control `+0x338` and `+0x340`
in arms `0x009C04A5` and `0x009C04C7`. The arms call `0x009BFA30` and
`0x009C02E0`. With a nonempty source, a package result, and admitted
drawing context, the source path forwards through virtual `+0x60` at
`0x009BFBD7`. With an admitted context, the sequence path forwards through
virtual `+0x64` at `0x009C0304`.

If both sequence assignments reach the same DrawingToolContext staging
field before it submits the command, `Effect_start` can replace `No_anime`
there. The static lookup and installed package and texture are documented
in [ActionMenu completion package](sparklecontrol-source-sequence.md#actionmenu-completion-package).

### TimerEnd default parameter and root Event

STATIC. The `TimerEnd` action declares only `Command`. Its constructor
`0x009B2D00` calls the base initializer `0x00954C00` at `0x009B2D2B`.
That initializer passes the parameter holder at `+0x54` to `0x00953B10`
at `0x00954C99`. The holder points to `0x01361160` after `0x00953B1F`,
the shared [kind-0 default](sqwt-keyframe-target-binding.md#default-completion-record-and-event-arguments).
With no property names, `0x009B2A5B..0x009B2AAF` copies the default
and skips reparsing. The call at `0x009B2B74..0x009B2B7E` supplies the
copied record, the resolved TimerIcon source and the triggering event.
The RoutedCommand event-argument constructor copies kind 0 to event `+0x24`
at `0x0092916A..0x00929170`. The source event can also supply route
flags through `0x0092801B..0x00928049` and `0x009280D4..0x00928110`.

If `Executed` reaches the ActionMenu Window and matches its plain
priority-5 root subscription, the Window is the subscription sender and
the TimerIcon remains the routed source. The kind-0 adapter selected at
`0x00536689` uses an empty template string at `0x006F5740`. The admitted
Event path at `0x00896AF0` passes exactly `nil`, `_widget` and
`UILuaCommands.TimerEnd` through `0x00749220..0x00749262` after the
resolved Lua receiver. The shared Lua wrapper forwards five slots, so
both payload slots are nil. See the
[TimerEnd registration and refresh](https://github.com/XIVLegacy/xivl-client-scripts/blob/7941b9bca4d4015c939c50eb68d95bc2ec44f723/docs/action-menu-recast-inputs.md#timerend-refresh).
Actual PreviewExecuted admission, priority gating and live delivery
remain UNKNOWN.

## Initial transform and clip forwarding

STATIC. An unchanged default [ScaleTransform](sqwt-render-transform.md#scaletransform-object)
returns the shared matrix object referenced by `0x012E6594` at
`0x00990DEE..0x00990DF5`. Startup `0x00F26AE0..0x00F26B00` initializes
that object's matrix with the identity builder `0x0042FCF0`. The Visual
transform update at `0x009412E1..0x00941309` also selects that pointer
when the VisualTransform storage's cached matrix is null. It compares
that matrix with the supplied transform's
matrix through `0x0098FBB0` and skips the dirty path on equality. A
default transform declaration alone therefore need not stage a matrix.
The WindowContext matrix consumer at `0x008CD0B6..0x008CD0E3` also
skips an equal identity when both context transform fields are null.
Later writes and effective parent matrices remain separate inputs.

STATIC. The `ClipToBounds` callback's tag `0x0E` reaches `0x00939BD0`
through `0x0093BCFA`. The helper requests a zero rectangle when the bit
is clear, or a rectangle at `(0, 0)` with width and height each at least
one when set. It calls the VisualClip setter at `0x00939C8C`. The setter
skips a zero request against the default zero rectangle
(`0x00941DC2..0x00941DDC`). Changed VisualClip tag 8 forwards through
`0x0093FE6E..0x0093FE86` and proxy slot `+0x4C`.
The proxy target `0x0055A0E0` stages the rectangle and flush
`0x0054EB68..0x0054EBB1` submits it as local `0x04`.

WindowContext case `0x008CEDBB` calls `0x008CD6D0`, which reaches
`0x008CD110`. It ignores an all-zero rectangle when local clip byte
`+0x55` is already clear (`0x008CD136..0x008CD189`). Inherited clip is
separate. The parent update passes its retained clip at `+0x58` to the
child update at `0x008CDD7E..0x008CDDAE`. A false local setting does not
settle the effective stock scissor. See
[translation-only placement and draw order](sparklecontrol-source-sequence.md#timericon-draw-order-and-translation-only-placement).

## OBSERVED TimerIconId handoff

| Locator | Observation |
| --- | --- |
| `0x00F2A550..0x00F2A569`, `0x009C0DD8..0x009C0E12` | Registration uses `TimerIconId` at `0x010821DC` and descriptor `0x01362F4C` with tag 0. The constructor associates it with the control dword at `+0x2BC`. |
| `0x009C0372..0x009C038A`, `0x010825B8` | The callback maps tag 0 through tables `0x009C0584` and `0x009C0554` to a call through TimerIcon vtable offset `+0x104`. The slot selects `0x009C1BE0`. |
| `0x009C1BE0..0x009C1C24` | The method resizes the icon vector at control `+0x2AC` to six dwords with `-1` fill for added entries. It calls the ordinary forwarder `0x00969FA0` at `0x009C1BF8`. It then stores control `+0x2BC` into the sixth dword at `0x009C1C20`. |
| `0x00969FA0..0x00969FD4`, `0x0054F110..0x0054F22C` | When the ordinary forwarding gates admit the update, the context copies the vector into drawing state during the call and prefixes the ID count. This copy sees the prior sixth dword, which is `-1` if the resize just added it. |
| `0x0096A0B9..0x0096A0EA` | The inherited `IconDatas` update also calls vtable offset `+0x104` after parsing up to five icon positions. This is another entry to the same method on the TimerIcon vtable. |

With six forwarded IDs, the state vector has seven dwords including the count.
The producer at `0x0054EEA9..0x0054EEF9` selects local command `0x15` when
that vector has at least five dwords. Dispatch cell `0x008CF40C` points to
`0x008CF25E`, the shared icon list arm. The arm passes the local command to
the renderer factory and calls renderer vtable offset `+0x1C` with the count
and ID array. The factory and context gate also appear in the
[TimerIcon Percent study](sqwt-timer-icon-percent.md#observed-property-and-renderer-handoff).

`RenderTimerIcon` vtable cell `0x01066AE0` points to `0x008E49B0`. This
wrapper calls `0x008DF570`, which copies the IDs into the array at renderer
`+0x08`, keeps the list length at `+0x0C`, and returns that length. The
wrapper then stores the returned length minus one at
`+0x0E` through `0x008DF650`. With six IDs, the ordinary icon pass can
iterate five entries, subject to its own draw gates. The timer overlay reads
the sixth through the index at `0x008E5183`. See
[ordinary IconControl texture resolution](sqwt-icon-control-textures.md#observed-numeric-input-and-resource-resolution).

## Resource selection and limits

For a nonzero overlay ID, `0x008E5192` calls the same icon resolver,
`0x00CA2170`, with the WindowContext draw manager. The resolver calls the
manager at virtual offset `+4` and then passes the result to `0x00CA2070`.
On the `RaptureTextureManager` path, vtable cell `0x00FB6D98` selects
`0x004B6670`. The declared ID 526 is in the icon table's nonnegative domain.
Its quotient by 10000 is zero and its remainder is 526. The first base
`0x1C590000` at `0x004B6690` plus that remainder gives resource
`0x1C59020E`, which names
`data/1C/59/02/0E.DAT` by the standard DAT path rule. See the
[icon resource calculation](../actor/player-nameplate-icons.md#form-and-resource-evidence).

The static join is conditional on a later admitted forward carrying 526
and on the selected renderer and texture manager. Live ActionMenu update
order, ordinary command image sourcing, active page visibility, animation
evaluation and event delivery remain unresolved. The DAT's presence,
payload, resolution and rendered appearance are unverified.
