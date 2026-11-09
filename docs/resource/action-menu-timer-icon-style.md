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

## ActionMenu Window template and style write sets

OBSERVED markup. Form lines 9 through 14 select
ControlTemplate_Window_ActionMenu and Style_ActionBarWindow from the merged
ActionMenuWidget.tpl dictionary. Each key occurs once in that dictionary.
The full selected subtrees occupy template lines 2 through 79 and 583 through
614, respectively. Both declare TargetType Window. The Style also declares
an empty SqwtIsAutoSetSkin attribute. Its only direct child is Style.Triggers.
The template has Grid_Content and its ContentPresenter at lines 3 and 4,
Setter SqwtTopOffset `32` at line 6, then ControlTemplate.Triggers at line 7.
The presenter declares ClipToBounds False. Neither selected source declares
an own transform assignment.

The form, template and default.style identities are the pinned inputs on this
page. These observations use retained xivl-tools revision
`f2a9a2b5a6a4c450458db7a493c25fcdcd70aca0` extractions, each with a passing
exact source-replay receipt. Original identities and retained file seals were
rechecked. Decode each original basename with `extract-resource --as sqwt`,
replay with `verify-extraction --source`, and use Python Expat to select the
exact keys and decoded line ranges above. This method does not materialize
or run the templates.

### Complete template trigger and action order

The table lists direct actions in document order. Begin-action names and
StopStoryboard BeginStoryboardName values are literal strings.

| Template lines | Trigger | Ordered actions |
| --- | --- | --- |
| 8 through 21 | Shown, Name WindowOpen | Stop ActionMenu_ClosingAnimation, stop ActionMenu_HidingAnimation, begin ActionMenu_ShownAnimation |
| 22 through 37 | Closing, Name WindowClose | Stop ActionMenu_HidingAnimation, stop ActionMenu_ShownAnimation, begin ActionMenu_ClosingAnimation, then NonDisplayPopupHelp commands targeting Label_ChangeBattleRegimen and Label_BattleRegimenStarting in that order |
| 38 through 53 | Hiding, Name WindowHide | Stop ActionMenu_ClosingAnimation, stop ActionMenu_ShownAnimation, begin ActionMenu_HidingAnimation, then the same two popup-help commands in that order |
| 54 through 56 | UILuaCommands.Cancel | SoundPlayerAction FileNumber `1` |
| 57 through 59 | ApplicationCommands.MoveFocus, SourceType Button | SoundPlayerAction FileNumber `3` |
| 60 through 62 | MouseEnter, SourceType Button | SoundPlayerAction FileNumber `3` |
| 63 through 65 | UILuaCommands.ExecuteCommand | SoundPlayerAction FileNumber `42` |
| 66 through 68 | UILuaCommands.ExecuteCommandError | SoundPlayerAction FileNumber `41` |
| 69 through 71 | UILuaCommands.PushSlotPrev | SoundPlayerAction FileNumber `3` |
| 72 through 74 | UILuaCommands.PushSlotNext | SoundPlayerAction FileNumber `3` |
| 75 through 77 | Deactivated, SourceType Window | ExecuteCommandAction UILuaCommands.Deactivated |

Shown, Closing, Hiding, MouseEnter and Deactivated are EventTriggers. The
other six are CommandTriggers. SourceType is absent except where listed.
The selected template has no SourceName declaration. Its four popup-help
ExecuteCommandActions at lines 35, 36, 51 and 52 declare
SQWTDesignCommands.NonDisplayPopupHelp. Their CommandTarget bindings name
only the respective ElementName, omitting Path and Mode. They are the only
bindings in either selected source. The Deactivated action omits
CommandTarget and CommandParameter. Every ExecuteCommandAction in the pair
omits CommandParameter.

Each template Storyboard declares these children in order: an origin property
animation, a VisualOpacity double animation, then two SqwtTopOffset double
animations. Each origin track at lines 13, 27 and 43 has only
Storyboard.TargetProperty RenderTransformOrigin. Its single discrete keyframe
at lines 14, 28 and 44 has only Value `0.5,0.5`. BeginTime, Duration and
KeyTime are omitted on these track/keyframe pairs.

| Template lines | DoubleAnimation property | From | To | BeginTime | Duration | FromAccelType / ToAccelType |
| --- | --- | --- | --- | --- | --- | --- |
| 16 | VisualOpacity | `0` | `1` | `0:0:0` | `0:0:0.3` | omitted |
| 17 | SqwtTopOffset | omitted | `-4` | `0:0:0` | `0:0:0.2` | SpeedUp / SlowDown |
| 18 | SqwtTopOffset | `-4` | `0` | `0:0:0.2` | `0:0:0.1` | SlowDown / SpeedUp |
| 30, 46 | VisualOpacity | `1` | `0` | `0:0:0` | `0:0:0.3` | omitted |
| 31, 47 | SqwtTopOffset | omitted | `-4` | `0:0:0` | `0:0:0.1` | SlowDown / SpeedUp |
| 32, 48 | SqwtTopOffset | `-4` | `32` | `0:0:0.1` | `0:0:0.2` | SpeedUp / SlowDown |

Each listed accelerated offset track declares both acceleration values `0.9`.
The template animation descendants omit TargetName and Storyboard.TargetName.
Opening Storyboard line 12 has no attributes. Closing line 26 declares only
CompletedCommand ApplicationCommands.DestroyWindow. Hiding line 42 declares
only CompletedCommand ApplicationCommands.InvisibleWindow. Thus all three
omit CompletedCommandTarget, BeginTime, Duration and RepeatBehavior.

### Complete style trigger and animation order

The IsAnchorWithin True Trigger at line 585 begins Effect_ON at line 586,
then Loop_Anime at line 593, then stops Effect_OFF at line 599. The False
Trigger at line 601 begins Effect_OFF at line 602, then stops Effect_ON at
line 610 and Loop_Anime at line 611. These are the style's only two triggers.
It declares no Setter, EventTrigger, CommandTrigger or ExecuteCommandAction.

All nine style tracks are DoubleAnimations targeting VisualOpacity. Let E1
and E2 denote the exact Storyboard.TargetName strings Border_ActionMenuFrameEffect_1
and Border_ActionMenuFrameEffect_2 in the following table.

| Template line | Begin action | Storyboard.TargetName | From | To | BeginTime | Duration |
| --- | --- | --- | --- | --- | --- | --- |
| 588 | Effect_ON | E1 | `0` | `1` | omitted | `0:0:0.2` |
| 589 | Effect_ON | E1 | `1` | `0` | `0:0:0.2` | `0:0:0.2` |
| 590 | Effect_ON | E2 | `0` | `1` | omitted | `0:0:0.2` |
| 595 | Loop_Anime | E2 | `1` | `0.2` | omitted | `0:0:0.6` |
| 596 | Loop_Anime | E2 | `0.2` | `1` | `0:0:0.6` | `0:0:0.6` |
| 604 | Effect_OFF | E1 | `0` | `1` | omitted | `0:0:0.2` |
| 605 | Effect_OFF | E1 | `1` | `0` | `0:0:0.2` | `0:0:0.2` |
| 606 | Effect_OFF | E2 | omitted | `1` | omitted | `0:0:0.2` |
| 607 | Effect_OFF | E2 | `1` | `0` | `0:0:0.2` | `0:0:0.2` |

Tracks 588, 590, 604 and 606 declare FromAccelType SpeedUp, ToAccelType
SlowDown and both acceleration values `0.8`. The other five omit those
fields. Loop_Anime's Storyboard at line 594 declares BeginTime `0:0:0.2`
and RepeatBehavior Forever. The other two style Storyboards have no
attributes. All three omit Duration, CompletedCommand and CompletedCommandTarget.

### Names, declared write sets and surviving boundary

The template begins ActionMenu_ShownAnimation, ActionMenu_ClosingAnimation
and ActionMenu_HidingAnimation. The style begins Effect_ON, Loop_Anime and
Effect_OFF. Each name has exactly one BeginStoryboard declaration in the full
dictionary, and none in the form. The two sets are disjoint. Every selected
StopStoryboard names a begin declaration within its own selected source.
Each Begin action declares only Name, and each Stop action declares only
BeginStoryboardName. Neither source declares PauseStoryboard or ResumeStoryboard. These lexical
facts do not establish independent native clock instances, namespace scope or
runtime stop resolution.

The template's declared write set is SqwtTopOffset, RenderTransformOrigin
and VisualOpacity with unnamed animation recipients. The style tests
IsAnchorWithin and names VisualOpacity on E1 and E2. It has no property
binding that reads the template's write set. Omitted From values at lines
17, 31, 47 and 606 are preserved as omissions, without asserting a live
captured value or capture order.

E1 and E2 each match one Border Name in the full form, at lines 28 and 29,
and no Name declaration in the full dictionary. Both declare Visibility
Visible and VisualOpacity 0. Their form ancestor chain is unnamed Grid 16,
Grid_Base at line 15, then Window_ActionMenuWidget at line 1. None of those
ancestors declares local VisualOpacity. The selected template's Grid_Content
and ContentPresenter also omit local VisualOpacity. This is a static
containment record, not established runtime visual ancestry or effective opacity.

The root WIN_null style occupies default.style lines 15 through 26. E1 and
E2 select BOD_actionMenu_frameEffect_1 and BOD_actionMenu_frameEffect_2 at
lines 5037 through 5048 and 5049 through 5060. These three complete subtrees
have no Parts Reference, transform or VisualOpacity declaration. For each
border style, SkinNormal, SkinHover, SkinActive, SkinDisabled and SkinFocused
reference its same-named skin. SkinAnchored, SkinReserved and SkinDroppable
reference NULL. WIN_null references NULL in all eight conditions. The style
root names common/default.skin. Its contents remain outside this finding.
The popup command targets match one form Label Name each at lines 223 and
232 and none in the dictionary. Both select the
[small popup-help template](#small-popup-help-label-declarations).
Their Content declarations bind OneWay to the corresponding control's Content.
Those form bindings are outside the two selected Window sources.

The meaning and effect of the Style's empty SqwtIsAutoSetSkin attribute remain UNKNOWN.

The conditional native [Window event passes](sqwt-window-trigger-order.md)
visit an admitted Style collection before an admitted ControlTemplate
collection. This is separate from
[initial materialization order](sqwt-window-trigger-order.md#ordinary-actionmenu-property-elements)
and first trigger evaluation.
Native name scope, implicit animation recipients, binding activation, initial
property-trigger evaluation, effective ancestor composition, popup command
delivery, completion delivery and rendered state remain UNKNOWN. The
[conditional Style clock path](#conditional-style-capture-and-named-stops)
covers its selected capture and stop calls. Distinct names and declared write sets do not prove
commutativity, full-form admission or a live result.

### Conditional Style capture and named stops

STATIC. The authenticated False trigger begins Effect_OFF before the
two named stops. On the matching owner-event and action route, plain
Trigger slot +8 at `0x0107D0E0 -> 0x009B0620` supplies the installed
Style's embedded TriggerCollection C at Style+`0x24` to its action
walk. The BeginStoryboard cell `0x0106D7DC -> 0x009AF6F0` updates the
current action at C+`0x1C` through `0x009C24F0`, retains C at
Begin+`0x68` and calls the Storyboard at `0x009AF73F`. A group
target hit at `0x009AC4B4` admits setup at `0x009AC4CD` and ordered
child preparation through `0x009AC360`. Each child requires its
own target hit to reach setup. A group miss skips setup and can
still reach the timer reset at `0x009AC4EB`. Admitted setup is
synchronous and precedes the later timer-dispatched sample.

For the Effect_OFF line 606 track, the [target lookup](sqwt-keyframe-target-binding.md#observed-target-cache-and-begin)
must retain or find the named E2 Border. With the VisualOpacity
descriptor `0x0135988C` and the checked Border property-info path
retained, the read at `0x009B4896 -> 0x00911020` reaches the
VisualOpacity ordinal-4 arm `0x0093F2DA`. It formats the Border's
local float at `+0x48`. DoubleAnimation converts that text back to a
scalar at `0x009B48A9`. With a successful read, the omitted From takes
this value at `0x009B48F0/0x009B48F6`, and the explicit To selects 1
at `0x009B4911`. An admitted setup with a failed property read instead
leaves the temporary scalar zero before offsets. Setup writes the
track's state. The local input is separate from effective drawn
opacity.

StopStoryboard cell `0x0107DABC -> 0x009B4270` with an empty cache
searches the supplied C through `0x009C2530`. The plain Trigger
name arm `0x0107D0E4 -> 0x009B1740` returns the first matching
retained action name. A name miss skips dispatch at `0x009B428F`.
A hit passes zero at `0x009B4296` and calls the named Begin's
Stop at `0x009B4298`. The checked Begin cell
`0x0106D7E0 -> 0x009AF760` requests clearing the current action when
its retained collection is nonnull and the Stop argument is zero
at `0x009AF76C..0x009AF775`, then stops its own timelines. The
clear does not compare the stopped Begin with the collection's
current action. The Storyboard Stop path calls each child at
`0x009CAD2B`, then its own stop at `0x009CAD42`. The timer stop at
`0x009AC3D1/0x009AC3D6` resets its timer and clears the timeline's
`+0x128` byte. These Stop bodies perform no final property sample,
restore or completion command.

STATIC. The collection constructor clears C+`0x20` at `0x009C27C9`.
At `0x009C24FA..0x009C2510`, the replacement helper calls the
current occupant's Stop with argument 1 if that byte is 1, the
current pointer is nonnull and differs from the replacement. A
named clear uses null as the replacement, so it can also stop the
current occupant. The nested Stop skips clearing the collection.
With the byte still zero, the False walk can prepare Effect_OFF, then clear the
collection's current action by stopping the previously begun
Effect_ON or Loop_Anime that retains the same C. Those calls
disable the named timelines while the newly prepared Effect_OFF
timer can remain armed. The separate True walk can similarly
prepare Effect_ON and Loop_Anime before stopping a previously
begun Effect_OFF. These conclusions require the stated guards
and retained action objects.

First matching event, target cache contents, numeric capture,
clock phase, overlapping writes, reentry, later C+`0x20` state
and effective composition remain UNKNOWN.

### Conditional SqwtIsAutoSetSkin property path

The input is the identified retail ffxivgame.exe, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. Locators are VAs. Ghidra CLI 0.2.2
provided focused string, xref and disassembly queries in the existing analyzed
program. Original section-mapped PE bytes independently matched the inspected
instructions and referenced strings, property table and RTTI data.

STATIC. Initializer `0x00F28DD0..0x00F28DE9` associates the ASCII literal
SqwtIsAutoSetSkin at `0x0107BFDC` with descriptor `0x013615D0` and ordinal
zero at `0x013615E4`. The one-entry property-info initializer at
`0x00F28DF0..0x00F28E1A` supplies table `0x012E7D64` and count one to
info object `0x013615F4`, then installs getter `0x009A62F0` and setter
`0x009A6360`. Its table `0x0107C040`, locator `0x01180338` and type
descriptor `0x012E7D20` identify
`.?AV?$DependencyPropertyInfo@VStyle@Sqwt@@@Sqwt@@`.

STATIC. Style construction at `0x009A6269..0x009A62D4` installs primary
table `0x0107BFFC` and seeds value bit `0x20` true in Style `+0xD0`.
Locator `0x011802DC` and type descriptor `0x012DFCFC` identify
`.?AVStyle@Sqwt@@`. The constructor derives packed bits 0 and 1 separately
from metadata `0x013615E8`. Getter `0x009A62F0..0x009A631F` accepts ordinal
zero and passes normalized value bit 5 to `0x00989060`. This
constructor value is distinct from later property assignment.

STATIC. Setter `0x009A6360..0x009A6389` accepts ordinal zero and passes
the owner, descriptor, callback `0x009A6220`, incoming mode and notification
flag one to helper `0x00941AD0` for Style `+0xD0`. The helper calls
preprocessor `0x00916310`. Sentinel `0x01356DA8` sets packed bit `0x10`
and returns before conversion. Its non-sentinel branch calls Boolean converter
`0x00988F90`, then packed store `0x0095E290`.

INFERRED under the published
[ordinary Boolean text conditions](sqwt-is-enabled-literal-setter.md): when
preprocessing preserves the supplied empty wrapper, the comparator global is
zero and the relevant pointers are non-null, empty text converts to false.
It is not a presence-only true value on this branch. This does not establish
that a selected Style markup attribute reaches this provider or store.

STATIC. In `0x0095E2AF..0x0095E2DD`, mode zero is rejected when packed bit
1 is set. An admitted mode-zero write clears bit 0. At
`0x0095E2DF..0x0095E301`, a differing Boolean replaces value bit 5 and sets
bit `0x04`. With notification flag one, the changed path requests helper
`0x0095DDC0` at `0x0095E311`. The supplied callback, when reached, dispatches
virtual `+0x18` at `0x009A6220..0x009A6225`. The identified Style table
selects `0x00912AC0` for that slot. Equal values skip this changed-value
notification request. Cleanup and property admission remain conditions.

UNKNOWN. The exact markup path that selects info `0x013615F4`, the live
preprocessing and packed admission state, and the downstream reader of Style
`+0xD0` bit 5 remain unresolved. This bounded chain establishes a property fragment,
constructor value and conditional Boolean write. It does not establish the
meaning or effect of the empty attribute in a live ActionMenu style, automatic
skin assignment, resource readiness, callback side effects or rendering.

### Style resource-tag wrapper boundary

STATIC. On the identified retail executable, resource dispatcher
`0x0094A550` compares the child name with Style at `0x0106CCF4` through
`0x009D244E` at `0x0094A6BB..0x0094A6D2`. Its equality branch calls
constructor `0x0094E8E0` at `0x0094A6EE`, then recursive reader
`0x00947040` at `0x0094A706`.

The constructor installs table `0x0106D9CC` at `0x0094E951`. Locator
`0x0117A880` and type descriptor `0x012E1DF4` identify
`.?AVXamlFrameworkTemplate@Markup@Sqwt@@`. At
`0x0094E90D..0x0094E933`, it copies parent `+0x18` into wrapper `+0x18`
and initializes wrapper `+0x10` to zero. This is a concrete markup-wrapper
join, not an established selection of the Style property-info object.

Table cell `0x0106D9D8`, virtual `+0x0C`, selects attribute handler
`0x00948FF0`. Its fallback call at `0x00949158` reaches the generic
attribute handler `0x00948B60`. Neither that call nor the inherited `+0x18`
value establishes the actual supplemental provider for the selected Style
resource. The exact context that supplies or replaces that value, and the
connection to info `0x013615F4`, remain UNKNOWN. No downstream skin-flag
reader or empty-attribute effect follows from this wrapper boundary.

The input identity and Ghidra CLI 0.2.2 method are the same as the conditional
SqwtIsAutoSetSkin finding above. Original section-mapped PE reads confirmed
the cited instructions, Style literal, vtable cell and RTTI pointers. The
queries reused the existing analyzed program without executing the PE or
changing annotations.

## Battle-regimen ToggleButton declarations

OBSERVED markup. Form line 220 declares the sole ToggleButton,
`ToggleButton_ChangeBattleRegimen`, with static-resource Template key
`ControlTemplate_ToggleButton_BattleRegimen` and SqwtStyle
`TOG_actionMenu_battleRegimen`. It declares command
`UILuaCommands.ChangeCombinationMode`, the same named CommandTarget,
Bottom alignment and Focusable False. Its `SqwtDesignData.StringValue0`
binding names `Label_ChangeBattleRegimen`, with no Path or Mode there.
The help Label at form line 223 binds Content OneWay to that
ToggleButton's Content. The
[omitted-path handle consumer](sqwt-qualified-property-lookup.md)
records the separate native lookup boundary.

The selected ControlTemplate occupies template lines 354 through 373
and targets `{x:Type ToggleButton}`. Its only child is
`ControlTemplate.Triggers`. It declares no visual child, initial
Setter, cloned Name or checked-state property Trigger.

| Template lines | Event and SourceType ToggleButton | First command | Second command |
| --- | --- | --- | --- |
| 356 through 359 | MouseLeave | `UILuaCommands.LeaveButton` | `SQWTDesignCommands.NonDisplayPopupHelp` |
| 360 through 363 | LostAnchor | `UILuaCommands.LeaveButton` | `SQWTDesignCommands.NonDisplayPopupHelp` |
| 364 through 367 | MouseEnter | `UILuaCommands.EnterButton` | `SQWTDesignCommands.DisplayPopupHelp` |
| 368 through 371 | GotAnchor | `UILuaCommands.EnterButton` | `SQWTDesignCommands.DisplayPopupHelp` |

Each event declares two ExecuteCommandActions in that order. The first
binds CommandParameter to TemplatedParent.Name, without an explicit Mode.
The second binds CommandTarget OneWay to
TemplatedParent.SqwtDesignData.StringValue0. These references name the
owner, without a declared per-clone Name or ElementName binding inside
this selected template.

Form line 1 names `common/default.style`. Under Widget Type ToggleButton,
its `TOG_actionMenu_battleRegimen` style at lines 3452 through 3455
contains CheckedButton and UncheckedButton parts. Both reference Widget
Type Button, selecting `TOG_actionMenu_battleRegimenOn` and
`TOG_actionMenu_battleRegimenOff` respectively. Their Button styles at
lines 1613 through 1624 and 1601 through 1612 declare Frame conditions.
Normal and Disabled use their corresponding `_N` skin. Hover, Anchored
and Reserved use `_H`. Active uses `_H` with SkinOffset `1,1`.
Focused and Droppable use `NULL`. Those selected style subtrees contain
no IsChecked property setter or trigger. The source skin and textures
are separate inputs.

The style container is 353783 bytes, SHA-256
`632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0`.
Its decoded document is 353775 bytes, SHA-256
`33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6`.
The form, template and style named containers were decoded with
`xivl-tools` revision `f2a9a2b5a6a4c450458db7a493c25fcdcd70aca0`,
using `extract-resource --as sqwt` and exact `verify-extraction --source`
replay. Python Expat provides the decoded locations. The form/template
digests agree with the inputs above.

These are static declarations. The conditional native initial skin
profile below has a separate constructor and renderer proof. Internal
Button construction, style precedence, trigger subscription, command
execution, template materialization, input and actual rendering remain
separate boundaries.

### Conditional initial Toggle skin

STATIC. The [constructor checked value and image-index seeds](../input/game-controller-startup.md#constructor-checked-value-and-image-index)
are false and zero respectively, subject to later writes. The
[published checked callback](../input/game-controller-startup.md#checked-state-callback-changes-the-image-part-index)
requests index one or zero under its own mutation guards.

At `0x0095D70E..0x0095D72D`, the constructor supplies
`ToggleButton:UncheckedButton:Frame,CheckedButton:Frame` from
`0x0106F100` to `0x00930BA0`. Its target is the empty Sqwt-string
object `0x01266B10`, with zero recorded length and NUL data at
its pointer `0x01266B22`. The
[unnamed own-key path](sqwt-progress-template-style.md)
assigns SqwtStyleControlName, separately from the scalar image index.

The comma-group loop `0x0093463E..0x009346E2` resolves in string
order. At `0x00934678` it calls `0x00933E70`, then advances the
row by `0x1C` and group by one at `0x009346D4..0x009346DD`.
The checked leaf path `0x00933FF8..0x00934032` requests
SkinNormal and stores its result in row slot zero. Thus native key
group zero is UncheckedButton, even though the style XML lists the
CheckedButton declaration first. Reference, dictionary and resource
resolution retain the [skin resolver's conditions](sqwt-skin-texture-filename.md).

For fresh WindowContext C, construction zeros `C+0x120/+0x124`
at `0x008CE388/0x008CE38E`. Fresh RenderSkin R, constructed
through `0x008E2940`, has primary table `0x01066934` and zeros
`R+0xE4/+0xE8` at `0x008E29A1/0x008E29A7`.
Factory `0x008CCFCE..0x008CD04D` creates R when `C+0xD4`
is absent, then forwards the two context values through
`0x008E09F0/0x008E1880` at `0x008CD02A..0x008CD049`.
The stores at `0x008E09F4/0x008E188D` set R's group and condition.
The factory installs R in main renderer slot `C+0xC4` at
`0x008CD092..0x008CD098`.

RenderSkin cell `0x01066940`, slot `+0x0C`, selects
`0x008E2CA0`. A dirty flag can call resolver `0x008E2A50`.
The load at `0x008E2CBD..0x008E2CE1` uses group `R+0xE4`
and condition `R+0xE8`. For nonnegative conditions zero through six,
the row address is `R+0x7C + 4*(7*group + condition)`.
A nonnull selection reaches `0x008E1540` at `0x008E2CFE`.
This finite profile supplies no arbitrary negative-condition bound.

With the fresh zero seeds retained, no intervening index or condition
write, accepted constructor keys and successfully resolved selected
style rows, group zero and condition zero select UncheckedButton Frame
SkinNormal. The ActionMenu declaration maps that part to its Off style.
The seed does not require a changed callback. Cached or replaced
renderers, later property writes, resource readiness, draw submission
and the actual first live paint remain unverified.

The cited instructions, vtable cells, RTTI and string objects were read
through Ghidra CLI 0.2.2 and independently checked against LLVM 21.0.0
and the pinned PE's section-backed bytes.

## Battle-regimen Starting Button declarations

OBSERVED markup. Form line 228 declares the ordinary Button
`Button_BattleRegimenStarting`. Its static-resource Template key is
`ControlTemplate_Button_BattleRegimenStarting`, and its SqwtStyle is
`BTN_actionMenu_battleRegimenStarting`. It declares command
`UILuaCommands.ExecuteBattleRegimen`, the same named CommandTarget,
Bottom alignment and Focusable False. Its `SqwtDesignData.StringValue0`
binding names `Label_BattleRegimenStarting`, with no Path or Mode there.

The owner Grid at form line 227 declares Visibility Collapsed. The Button
has no local Visibility or Content declaration. At form line 232, the help
Label binds Content OneWay to the Button's Content. It selects
`ControlTemplate_Label_PopupHelp_MenuIconSmall`, SqwtStyle NULL,
IsHitTestVisible False and VisualOpacity 0. Its unnamed Grid and StackPanel
ancestors are at lines 230 and 231. The StackPanel declares Focusable False
and IsHitTestVisible False.

The selected ControlTemplate occupies template lines 482 through 501 and
targets Button. Its only child is `ControlTemplate.Triggers`. It declares
no visual child, initial Setter, cloned Name or property Trigger.

| Template lines | Event and SourceType Button | First command | Second command |
| --- | --- | --- | --- |
| 484 through 487 | MouseLeave | `UILuaCommands.LeaveButton` | `SQWTDesignCommands.NonDisplayPopupHelp` |
| 488 through 491 | LostAnchor | `UILuaCommands.LeaveButton` | `SQWTDesignCommands.NonDisplayPopupHelp` |
| 492 through 495 | MouseEnter | `UILuaCommands.EnterButton` | `SQWTDesignCommands.DisplayPopupHelp` |
| 496 through 499 | GotAnchor | `UILuaCommands.EnterButton` | `SQWTDesignCommands.DisplayPopupHelp` |

Each event declares two ExecuteCommandActions in that order. The first
binds CommandParameter to TemplatedParent.Name, without an explicit Mode.
The second binds CommandTarget OneWay to
TemplatedParent.SqwtDesignData.StringValue0. This selected template declares
no ElementName binding. The
[omitted-path handle consumer](sqwt-qualified-property-lookup.md#actionmenu-design-data-strings)
records the separate native lookup boundary.

Form line 1 names `common/default.style`. Under Widget Type Button,
`BTN_actionMenu_battleRegimenStarting` occupies style lines 1637 through
1648. It contains one Frame part at line 1638 and eight condition properties.

| Style line | Frame condition | Skin | SkinOffset |
| --- | --- | --- | --- |
| 1639 | SkinNormal | `TOG_actionMenu_battleRegimenOn_H` | omitted |
| 1640 | SkinHover | `TOG_actionMenu_battleRegimenOn_H` | omitted |
| 1641 | SkinActive | `TOG_actionMenu_battleRegimenOn_H` | `1,1` |
| 1642 | SkinDisabled | `TOG_actionMenu_battleRegimenOn_H` | omitted |
| 1643 | SkinFocused | `NULL` | omitted |
| 1644 | SkinAnchored | `TOG_actionMenu_battleRegimenOn_H` | omitted |
| 1645 | SkinReserved | `TOG_actionMenu_battleRegimenOn_H` | omitted |
| 1646 | SkinDroppable | `NULL` | omitted |

The style root at line 1 names `common/default.skin`. The selected Frame
part has no Reference, Widget type reference, CheckedButton or
UncheckedButton declaration. SkinNormal is an explicit `_H` reference.
This is a declaration about an ordinary Button style, separate from the
ToggleButton's checked and unchecked parts.

The three original named containers and decoded documents have the sizes
and digests in [Inputs and method](#inputs-and-method) and the ToggleButton
style input record above. They were decoded with `xivl-tools` revision
`f2a9a2b5a6a4c450458db7a493c25fcdcd70aca0` using
`extract-resource --as sqwt`. Each fresh extraction passed exact
`verify-extraction --source` replay. Python Expat provides the decoded line
locations. The recipe uses each original basename unchanged:

```sh
xivl inspect <original-named-file> --as sqwt
xivl extract-resource <original-named-file> --output <new-private-output> --as sqwt --format json
xivl verify-extraction <new-private-output> --source <original-named-file> --report json
```

These are static declarations. Native Button construction, initial skin
condition selection, style precedence, resource readiness, template
materialization, omitted-path binding activation, routed-event delivery,
command execution and actual first live paint remain UNKNOWN. The selected
style's SkinNormal reference does not establish a rendered initial state.
The referenced skin and textures are separate inputs.

## Battle-regimen effect Label declarations

OBSERVED markup. `Label_BRSEffect` selects a Label template with command-driven
scale and opacity animation declarations. The markup does not establish live
activation or a rendered effect.

The same original named inputs and extraction/source-replay method recorded in the
[Starting Button study](#battle-regimen-starting-button-declarations) supply these observations.
The retained extraction seals and original identities were rechecked. Python Expat supplies
decoded element start and end lines.

### Owner and initial declarations

Form line 229 declares `Label_BRSEffect`, with static-resource Template key
`ControlTemplate_Label_BRSEffect` and SqwtStyle `LAB_actionMenu_BRSEffect`.
It declares VerticalAlignment Bottom, Focusable False, IsHitTestVisible False
and VisualOpacity 0. It declares no local Content, Visibility, dimensions or
RenderTransform.

Its parent `Grid_BattleRegimenStartingButton` at form line 227 declares
Visibility Collapsed. The remaining form ancestors are the unnamed Grid at
line 16, `Grid_Base` at line 15 and `Window_ActionMenuWidget` at line 1.
Those ancestors declare no local Visibility or VisualOpacity values. These
local declarations do not establish effective live visibility or opacity.

### Selected template and ordered animation declarations

The template at lines 527 through 556 targets Label. Its two direct children
are a Setter at line 528 and `ControlTemplate.Triggers` at line 529. The
Setter names RenderTransform with value ScaleTransform. The template declares
no visual child, ContentPresenter or named cloned element.

The first CommandTrigger, at lines 530 through 542, names
`UILuaCommands.BRSEffectAnimeStart`. Its only action is BeginStoryboard
`IconLoopAnime` at line 531. The Storyboard at line 532 declares RepeatBehavior
Forever. Its children occur in the following order.

| Template line | Declared animation type | TargetProperty | Declared values |
| --- | --- | --- | --- |
| 533 through 535 | PropertyAnimationUsingKeyFrames | RenderTransformOrigin | One DiscreteValueKeyFrame at line 534, Value `0.5,0.5`, no KeyTime |
| 536 | DoubleAnimation | SqwtDrawOrder | From `0`, To `0.5`, no Duration |
| 537 | DoubleAnimation | RenderTransform.ScaleX | From `1`, To `1.6`, Duration `0:0:0.4`, ToAccelType `SlowDown`, ToAccelValue `0.8` |
| 538 | DoubleAnimation | RenderTransform.ScaleY | From `1`, To `1.6`, Duration `0:0:0.4`, ToAccelType `SlowDown`, ToAccelValue `0.8` |
| 539 | DoubleAnimation | VisualOpacity | From `1`, To `0`, Duration `0:0:0.4`, ToAccelType `SlowDown`, ToAccelValue `0.8` |

The second CommandTrigger, at lines 543 through 554, names
`UILuaCommands.BRSEffectAnimeStop`. It first declares StopStoryboard at line
544, with BeginStoryboardName `IconLoopAnime`. It then declares an unnamed
BeginStoryboard at line 545, containing the Storyboard at line 546. That
Storyboard declares these children in order.

| Template line | Declared animation type | TargetProperty | Declared values |
| --- | --- | --- | --- |
| 547 through 549 | PropertyAnimationUsingKeyFrames | RenderTransformOrigin | One DiscreteValueKeyFrame at line 548, Value `0.5,0.5`, no KeyTime |
| 550 | DoubleAnimation | SqwtDrawOrder | From `0.5`, To `0`, no Duration |
| 551 | DoubleAnimation | VisualOpacity | To `0`, Duration `0:0:0`, no From |

The stop Storyboard declares no RepeatBehavior. Neither Storyboard declares a
completion command or completion target. None of these animation declarations
supplies Storyboard.TargetName. These are declaration strings, not established
native target resolution, endpoint capture, clock, acceleration or repeat rules.

### Frame skin declarations

Form line 1 names `common/default.style`. Under Widget Type Label at style
line 1964, `LAB_actionMenu_BRSEffect` occupies lines 2385 through 2396.
Its only part is Frame at line 2386. SkinNormal, SkinHover, SkinActive,
SkinDisabled and SkinFocused at lines 2387 through 2391 all reference skin
`LAB_actionMenu_BRSEffect`. SkinAnchored, SkinReserved and SkinDroppable at
lines 2392 through 2394 reference `NULL`. These properties declare no SkinOffset.

The style root at line 1 names `common/default.skin`. No effect binary filename
or SpkSource is declared in the selected Label template or style subtree. The
named skin file and its resource or texture contents are separate inputs and
were not read for this finding.

### Boundary

Template selection and materialization, effective ancestor state, property
admission, command delivery, animation targets, clock sampling, repeat and stop
semantics, acceleration, skin resolution, resource readiness, draw submission
and actual live appearance remain UNKNOWN. The owner opacity and ancestor
Visibility declarations alone do not prove whether a later animation runs or
whether an effect is visible.

## Small popup-help Label declarations

OBSERVED markup. The battle-regimen help labels select a shared Label template
with a ContentPresenter and two command-driven opacity animation declarations.
The markup does not establish command delivery or live popup appearance.

The same original named inputs and extraction/source-replay method recorded in the
[Starting Button study](#battle-regimen-starting-button-declarations) supply these observations.
The retained extraction seals and original identities were rechecked. Python Expat supplies
decoded element start and end lines.

### Help-label owners and initial declarations

The two form declarations selecting
`ControlTemplate_Label_PopupHelp_MenuIconSmall` are `Label_ChangeBattleRegimen`
at line 223 and `Label_BattleRegimenStarting` at line 232. Their Content
bindings are OneWay from the corresponding ToggleButton or Button Content.
Both declare HorizontalContentAlignment Center, SqwtStyle `NULL`, Margin
`2,0,2,0`, Width `24`, IsHitTestVisible False and VisualOpacity `0`. Neither
Label declares a local Visibility or RenderTransform.

The first Label's ancestor chain includes Grid 221 and StackPanel 222 below
`Grid_BattleRegimenButton` at line 219. The second includes Grid 230 and
StackPanel 231 below `Grid_BattleRegimenStartingButton` at line 227. The two
named Grids declare Visibility Collapsed. Their common remaining form ancestors
are unnamed Grid 16, `Grid_Base` at line 15 and `Window_ActionMenuWidget` at
line 1. Other nodes in these ancestor chains declare no local Visibility or
VisualOpacity. StackPanel 231 declares Focusable False and IsHitTestVisible
False. StackPanel 222 omits those two properties. These omissions do not
establish native defaults or effective live state.

The corresponding controls at form lines 220 and 228 declare
SqwtDesignData.StringValue0 bindings naming their help Labels, with no Path or
Mode. Their selected event templates bind popup CommandTarget OneWay to
TemplatedParent.SqwtDesignData.StringValue0 at template lines 358 through 370
and 486 through 498. The
[omitted-path handle consumer](sqwt-qualified-property-lookup.md#actionmenu-design-data-strings)
records the separate native lookup boundary.

### Selected template graph

The template at lines 80 through 105 targets Label. Its direct children occur
in this order: ContentPresenter at line 81, Setter at line 82 and
`ControlTemplate.Triggers` at line 83. The ContentPresenter declares
IsHitTestVisible False and ClipToBounds False. It has no child and no local
Content, dimensions, style, transform, name or binding declaration. The Setter
names RenderTransform with value ScaleTransform.

The first CommandTrigger, lines 84 through 93, declares command
`SQWTDesignCommands.DisplayPopupHelp` and Name `DisplayPopupHelp`. Its only
action is an unnamed BeginStoryboard at line 85, containing the Storyboard at
line 86. The second CommandTrigger, lines 94 through 103, declares command
`SQWTDesignCommands.NonDisplayPopupHelp` and Name `NonDisplayPopupHelp`. Its
only action is an unnamed BeginStoryboard at line 95, containing the
Storyboard at line 96.

| Template lines | Ordered Storyboard child | Declared values |
| --- | --- | --- |
| 87 through 89 | PropertyAnimationUsingKeyFrames targeting RenderTransformOrigin | One DiscreteValueKeyFrame at line 88, Value `0.5,0.5`, no KeyTime |
| 90 | DoubleAnimation targeting VisualOpacity | To `1`, Duration `0:0:0.2`, no From |
| 97 through 99 | PropertyAnimationUsingKeyFrames targeting RenderTransformOrigin | One DiscreteValueKeyFrame at line 98, Value `0.5,0.5`, no KeyTime |
| 100 | DoubleAnimation targeting VisualOpacity | To `0`, Duration `0:0:0.2`, no From |

The first two rows belong to DisplayPopupHelp and the last two to
NonDisplayPopupHelp. Neither trigger declares StopStoryboard. Neither
Storyboard declares Name, RepeatBehavior, CompletedCommand or
CompletedCommandTarget. The selected template declares no scale-axis animation,
Storyboard.TargetName, popup positioning value, skin key, SpkSource or effect
binary filename. It does not declare a Content binding on its ContentPresenter.

Form line 1 names `common/default.style`, whose root names
`common/default.skin`. The help owners' SqwtStyle literal is `NULL`. This
finding does not interpret that literal as a native no-draw guarantee. The
referenced skin file and any texture or effect contents were not read.

### Boundary

Template materialization, Content transfer to the presenter, resource
resolution, effective ancestor state, omitted-path binding activation, command
target selection, command delivery, animation endpoint capture, clocks,
interpolation, concurrent storyboard behavior, popup placement, draw submission
and actual live appearance remain UNKNOWN. A declared opacity destination does
not establish a running clock or a visible popup.

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

## Centered one-cell arrange

The selected `Grid_Command` declares no tracks. Its children do not
declare attached Grid positions or spans. The default definition builder
at `0x00975247..0x009752E7` creates one definition per axis with type 2,
weight 1, minimum 0 and maximum 10000000. The ordinary one-track path
assigns the available 44 units to the sole weighted track in each axis
(`0x00975980..0x00975A3E`, `0x00976531..0x009765EF`) and starts its
offset at zero. The child call at
`0x00978A53..0x00978A8C` passes the resulting `(0, 0, 44, 44)` cell.
STATIC for the selected unchanged sizes at layout scale 1.

The generic child arrange at `0x0092FD20` preserves explicit size
lanes through the slot-size check at `0x0092FE45..0x0092FEC9`. The
TimerIcon arrange slot at `0x0092A300` returns the supplied extents.
The two Center arms at `0x009301A0..0x009301B4` and
`0x0093022B..0x0093023F` add half the difference between slot and
arranged extent without clamping the offset to zero. With the declared
64 by 64 child, scale 1 and unchanged margins and size bounds, the
result is `(-10, -10)` relative to the Grid origin and an arranged
extent of 64 by 64. `0x00939720` retains that extent and forwards the
position through VisualOffset. STATIC for the admitted full-arrange
path. Effective ancestor placement, clip and rendered coverage remain
UNKNOWN.

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
