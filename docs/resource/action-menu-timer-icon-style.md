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

`Style_TimerIcon` targets `TimerIcon` in the same dictionary. It declares an
empty `SqwtIsAutoSetSkin` attribute and sets `SpkSource` to
`common/timerEffect.le.spk` at template lines 557 and 558.

| Template lines | Declaration | Action |
| --- | --- | --- |
| 560 to 564 | `RunningValue` 1 | Starts `TimerIcon_PercentAnime`. Its `DoubleAnimation` targets `Percent`. `From` binds to `ToPercent` and `Duration` binds to `TimerDuration`, both `OneWay` from `TemplatedParent`. `AnimeTime` binds `OneWayToSource` to `TemplatedParent.AnimeTime`. The animation declares no `To` value. |
| 567 to 574 | `RunningValue` 2, 3, or 0 | Pauses, resumes, or stops the same named storyboard, respectively. |
| 562, 576 to 579 | `TimerIcon.PercentAnimeEnd` | The storyboard names this as its completion command. The matching trigger sets `SpkSeq` to `No_anime` and then `Effect_start`, and executes `UILuaCommands.TimerEnd`. |

The omitted `To` follows the native endpoint setup when the target and
property resolve. See
[DoubleAnimation setup and TimerIcon Percent capture](sqwt-double-animation-clock.md#setup-current-value-and-repeated-begin).
The [TimerIcon recast inputs](sqwt-timer-icon-recast-inputs.md#observed-recast-inputs-and-derived-values)
record the native calculation of `ToPercent` and `TimerDuration`, the
`RunningValue` notification order, and the `AnimeTime` text representation.

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
