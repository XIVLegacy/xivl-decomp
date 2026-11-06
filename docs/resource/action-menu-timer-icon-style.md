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

The template declares `TimerIcon.PercentAnimeEnd` as the completion command.
The native Storyboard hook dispatches a retained command when present.
Installation and delivery of this declared command remain unresolved. See
[DoubleAnimation samples and Storyboard completion](sqwt-double-animation-clock.md#bounded-nonrepeat-samples-and-completion).

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
