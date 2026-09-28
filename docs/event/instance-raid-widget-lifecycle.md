# Instance-raid widget lifecycle

The recovered FFXIV 1.23b instance-raid scripts define a shared start, relogin,
clear, and failure presentation contract. Hamlet Defense adds exact title,
start-effect, HUD, and result surfaces. This note records client ownership; it
does not prescribe a server adapter or treat direct widget construction as an
equivalent path.

Native addresses refer to the pinned executable with image base `0x00400000`
and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Base start sequence

`InstanceRaidBaseClass.startEvent` performs these operations in order:

1. Store the content ID and event type.
2. Clear `instanceRaidWork.clearFlag`.
3. Configure the countdown timer.
4. Call `processLogin(false)` and subclass `processStartEvent(...)`.
5. Execute the supplied cutscene, or its fade path.
6. Fade in and wait one second.
7. Optionally call subclass `processStartEffect()`.
8. Dispatch subclass `openInformationWidget()`.
9. Set `instanceRaidWork.initFlag`.

This order makes the information widget a subclass-owned endpoint after the
cutscene and start effect. Constructing a form by index bypasses the recovered
owner and does not reproduce login or relogin state.

Twelve recovered client directors inherit this base contract: Aurum Vale,
Beacon Battle, Cutter's Cry, Dark Moogle, Hamlet Defense, Hyper Ifrit, Lesser
Garuda, Lesser Ifrit, Lesser White General, Normal Garuda, Normal Ifrit, and
Normal White General. Empty two-line subclasses still inherit the base
lifecycle; absence of an override is not absence of behavior.

## Hamlet presentation order

The three opening cutscenes own the location title. Their marker maps through
`cutscene_common` to an effect and then to a title widget hosted by desktop
slot 14's `SplashEffectWidget`:

| Hamlet | Opening scene | Marker | Effect | Title widget |
| ---: | --- | --- | ---: | --- |
| 1 | `ham0s201` | `2DEffectLocation8` | 12 | `HamletDefenseTitleWidget1` |
| 2 | `ham0f301` | `2DEffectLocation9` | 13 | `HamletDefenseTitleWidget2` |
| 3 | `ham0w201` | `2DEffectLocation10` | 14 | `HamletDefenseTitleWidget3` |

After the opening cutscene returns, Hamlet's `processStartEffect` prints NPC
line 1 and opens `DutyCommencedWidget1`, `2`, or `3` in desktop slot 13. One
second later, `openInformationWidget` opens the persistent
`HamletDefenseWidget` in slot 15 and the auto-closing
`HamletDefensePopupWidget` in slot 16.

The matching ending scenes retain separate success markers:

| Hamlet | Ending scene | Marker | Effect | Widget |
| ---: | --- | --- | ---: | --- |
| 1 | `ham0s202` | `2DEffectDutySuccess1` | 9 | `DutyCompleteWidget4` |
| 2 | `ham0f302` | `2DEffectDutySuccess2` | 10 | `DutyCompleteWidget5` |
| 3 | `ham0w202` | `2DEffectDutySuccess3` | 11 | `DutyCompleteWidget6` |

The title presentation is therefore cutscene-authored. Manual title-widget
indices, `_loadForm`, or a direct title open are not the recovered stock path.
The score and ranking widgets are event-mode asks, not the live HUD slots. See
[Hamlet PushEvent presentation](../actor/hamlet-push-event.md) for the separate
`localClearEvent` score gate and its missing server invocation.

## Toto-Rak attached event lane

Opcode `0x0130` consumes an existing event context; it does not create one.
The receive path at `0x004DCFFF` resolves the packet object and dispatches its
vtable slot `+0x24` at `0x004DD012`. The receiver thunk at `0x0089E2BB` reaches
`0x006E1140`, which loads `owner+0xF8`. The lane consumer at `0x00896FE0`
requires non-null `lane+0x08` and rejects dispatch when that field is null.

The attach/replace routine begins at `0x00896AF0`. Its visible EventStart or
UI-command caller is at `0x006F56AF`; the context write occurs at
`0x00896B72`, the UI-command token is stamped at `0x00896B75`, and the attached
flag at `lane+0x1C` is set at `0x00896BAA`. Detach captures and clears
`lane+0x08` at `0x00896F0D`; replacement clears it again at `0x00896F32`
before installing the successor.

Consequently, receiving or emitting opcode `0x0130` alone does not prove that
the required event lane is attached, that its token matches, or that the
context remains alive through dispatch.

## Provenance and evidence boundary

The recovered `InstanceRaidBaseClass` Lua has SHA-256
`2f6ea8cff45b471ed8ce05c8905af75bbe199e61a712cf6f0e96c9a653badeb9`;
`InstanceRaidHamletDefense` has SHA-256
`5dd3cfec23db4ccb531e57973be26b80b9d97e5b1594b37d0c80ea2d28ce9968`;
the desktop-widget connector has SHA-256
`f64deeccf4a0d76c7e57f59f06e889306539b3cc3973f786cab6dc96312eafe0`;
and `cutscene_common` has SHA-256
`b4cac0ff7bb1c86071c532a44ea0a861e56347f79ebf1285348fa66cb9aa42af`.

The evidence establishes client call order, subclass inheritance, Hamlet
surface ownership, cutscene marker mappings, and the native attached-lane
consumer. It does not establish the original server message sequence, relogin
replay policy, clear/failure acknowledgement ordering, Hamlet success-method
invocation, or safe zone-exit timing. Those remain server-side historical
gaps; direct UI construction or a naked `0x0130` packet cannot fill them.
