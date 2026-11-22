# Target-bar enmity

The native FormElement updates `Label_HateSignal` from the actor behind the
current target cursor. A helper maps an identity-match byte and a 16-bit
indicator to five command levels and suppresses an unchanged level.

## Input and method

STATIC. The target is `ffxivgame.exe`, FINAL FANTASY XIV 1.23b build
`2012.09.19.0001`, PE32 i386, image base VA `0x00400000`,
`15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs. Ghidra CLI decompilation supplied leads.
LLVM objdump disassembly, section-mapped PE reads, command literals and
vtable cells corroborated the claims. No retail execution was performed.

## Form and selected actor

FormElement constructor `0x0053AF60` constructs the helper at form
`+0x1C4` through `0x00552660` (`0x0053B09C..0x0053B0B4`).
The helper initializes its cached level to 9999, its Label pointer to null,
and its command vector empty (`0x0055268D..0x005526A1`).
The form-loaded path `0x005387F0` passes the selected form to
`0x0054C9F0` at `0x00538A3C`. That routine looks up
`Label_HateSignal` (ASCII literal `0x00FA2498`) and stores the
Label result in helper `+4` at `0x0054CA32..0x0054CA3F`.
Lookup `0x0053E240` selects the named form entry and casts the part
from FrameworkElement to Label at `0x0053E289..0x0053E2AB`.

The form update `0x005399C0` requires both AutoAttack parts at form
`+0x1B8` and `+0x1BC` (`0x00539A17..0x00539A29`).
It obtains TargetElement through `0x004D7310`, reads its current cursor
at `+0xB4`, and reads the selected actor pointer at cursor `+0x0C`
(`0x00539A37..0x00539A50`). For a nonnull actor it reads the packed
indicator through `0x00588B40` at `0x00539AE6`, then calls
`0x0054B8E0` on helper `+0x1C4` with option 0 at `0x00539AF5`.
An absent cursor or actor skips this call. That branch makes no level-zero
request and does not clear the helper's cached level.

`0x00588B40` reads the actor's `+0x80` wrapper and calls
`0x00574F60` (`0x00588B40..0x00588B4C`). The latter supplies byte 0
and word `0xFFFF` at output `+2` when the wrapped implementation is
null. Otherwise it invokes implementation virtual `+0xB8`
(`0x00574F60..0x00574F83`). The LuaActorImpl table cell
`0x00FDFBE4` selects `0x00759CF0`, which calls the control at
implementation `+8`, virtual `+0x60`, with the output and context
(`0x00759CF2..0x00759D04`). For NpcBase that slot selects
`0x006E3530`.

## NPC fields and level selection

NpcBase reader `0x006E3530` resolves the context and obtains an identity
dword through `0x0057AB60`. The latter reads its receiver `+0x10`
(`0x0057AB87..0x0057AB8E`). The NPC reader compares that result with
NpcBase `+0x154` and writes the equality byte to output `+0`, then
copies NpcBase word `+0x158` to output `+2`
(`0x006E3574..0x006E359D`). The
[battle-log identity path](battle-log-line-path.md#input-and-method)
records the local-player identity interpretation of this shared accessor.

The [opcode catalog](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/opcodes.json)
identifies `0x0195` with HateStatusReceiver. Its apply routine
`0x0089D030` casts the routed actor to NpcBase, then stores the
application payload dword at `+0` and word at `+4` to those fields
(`0x0089D04B..0x0089D05B`). The routine's local input uses offsets
`+4` and `+8` after its descriptor prefix. NpcBase constructor
`0x006F3650` initializes the identity dword to zero through
`0x00CC9320` at `0x006F3764` and the word to `0xFFFF` at
`0x006F376E`. The
[retained capture study](https://github.com/XIVLegacy/xivl-captures/blob/2030749093dac43506863694bcb65580100c5063/studies/group-combat-wire-contracts/README.md#enmity-0x0195)
owns the wire values, source actors and capture limitations.

Helper `0x0054B8E0` returns immediately when its Label is null
(`0x0054B90C..0x0054B910`). With option 0 it applies these tests in
order. Comparisons of the word are unsigned.

| Packed input | Level | Instructions |
| --- | ---: | --- |
| Byte equals 1, for any word | 4 | `0x0054B92C..0x0054B93B` |
| Otherwise word equals 0 or `0xFFFF` | 0 | `0x0054B93D..0x0054B94E` |
| Otherwise word 1 through 49 | 1 | `0x0054B950..0x0054B95B` |
| Otherwise word 50 through 79 | 2 | `0x0054B95D..0x0054B963` |
| Otherwise word 80 through 65534 | 3 | `0x0054B95D..0x0054B963` |

A nonzero option first calls `0x004B5170` and forces level 0 when that
predicate returns false (`0x0054B916..0x0054B92A`). The NamePlate
caller at `0x006A37E2..0x006A37F5` supplies its own option. That
branch's predicate and options remain separate from the form's literal 0.

## Installed target-bar declaration

MEASURED. The standard-density `widget/TargetParameterWidget.form`
imports `widget/TargetParameterWidget.tpl` at decoded line 6.
Its `Grid_Current` contains `Label_HateSignal` at line 26, selecting
`ControlTemplate_Label_HateSignal`, with Visibility Collapsed and hit
testing disabled. The [target-caption form identity](../script/target-caption-text.md#lua-choice-and-form)
pins the form. The template carrier is 6551 bytes, SHA-256
`3785f11561392833e809c64ea663b39b3ae5c3857dfed532c0ccc40320e34339`.
Its decoded markup is 6543 bytes, SHA-256
`07d3ecac5f3bb811ee9bd0f8e827ecb864e95047db79ece8e01b0d2b0743e924`.
Both installed files were decoded with basename-keyed Blowfish,
re-encrypted byte-for-byte, and parsed as namespace-aware XML.

The Label template begins at line 17. Six Borders at lines 19-24 start
with opacity zero. Their style keys are `BOD_basis_hateLevel1/2/3`
and `BOD_basis_hateLevel4_1/2/3`. The
[Border skin minimum](sqwt-animation-skin-clock.md#border-minimum-and-drawing)
derives a minimum from a resolved 1-by-1 CellSkin. Successful style and skin
resolution remain necessary for the template's intrinsic extent and draw.

| Command level | Declared Label and Border changes | Template lines |
| --- | --- | --- |
| 0 | Collapse the Label, zero all six Border opacities, stop LoopAnime. | 27-35 |
| 1 through 3 | Show the Label, set only the matching Border opacity to 1, zero the other five, stop LoopAnime. | 37-65 |
| 4 | Show the Label, zero level-1 and level-2 Borders, retain level-3 Border opacity 1, and begin LoopAnime. | 67-73 |

The level-4 declaration includes that steady level-3 Border and a Forever
storyboard. Its three level-4 Borders ramp from 0 to 1 in 0.1 seconds,
starting at 0.1, 0.2 and 0.3 seconds. All three ramp from 1 to 0 over
0.05 seconds starting at 0.5 seconds (lines 74-79).
These are declared setters and animation operands. Native attachment,
stop/restart arbitration and observed pixels remain conditional.

## Conditional LoopAnime group clock

STATIC. The Storyboard primary table `0x0107C32C` selects group setup
`0x009CAB40` at `+0x4C`, group evaluation `0x009CAC70` at `+0x50`,
iteration reset `0x009CAC00` at `+0x5C`, and group Stop
`0x009CACE0` at `+0x3C`. These cells and the following instruction ranges
were checked with section-mapped PE reads and LLVM objdump. The
[DoubleAnimation clock](sqwt-double-animation-clock.md#setup-current-value-and-repeated-begin)
owns target resolution, endpoint capture, the timer epoch and scalar evaluation.

Group setup walks its retained child vector at `+0x138..+0x13C`, calls
`0x009AC360` on each child, and takes the unsigned maximum returned qword
(`0x009CAB85..0x009CABCA`). It writes that maximum through the Duration
setter at `0x009CABE1`, then calls `0x009AD020` at `0x009CABF0`.
Child setup stores its returned span at child `+0xE8/+0xEC`
(`0x009AC385..0x009AC38D`). For a finite child with repeat count one,
SpeedRatio one and AutoReverse false, the span returned by
`0x009AD169..0x009AD1E0` is BeginTime plus Duration.
The [native defaults](sqwt-double-animation-clock.md#default-timericon-percent-evaluation)
establish those omitted child properties.

INFERRED. With the installed operands above, the group repeats at the end
of the last fade, nominally 0.55 seconds. Its period is the maximum child
span, not the maximum bare Duration. The
[decimal parser qualification](sqwt-double-animation-clock.md#clock-source-units-and-epoch)
still applies. This does not assign exact parsed millisecond values without
closing the locale and x87 environment.

STATIC. Forward finite-duration boundary handling evaluates the endpoint
before incrementing the iteration count at `0x009ACB68..0x009ACB71`.
For another non-reversing iteration, `0x009ACBA2..0x009ACBAC` calls
virtual `+0x5C` with false. The group implementation visits each retained
child through `0x009ACF40` (`0x009CAC44..0x009CAC59`). With false,
that helper clears child counters `+0x118/+0x11C`, marks the child active,
and zeroes previous and accumulated time `+0x100..+0x10C`
(`0x009ACF50..0x009ACF9D`). It conditionally publishes AnimeTime and
calls virtual `+0x48`, which reaches the DoubleAnimation no-op through
`0x009AC3B0 -> 0x009AC3A0`. It neither recaptures endpoints nor writes
the target opacity. The ordinary remainder path skips a zero remainder
at `0x009ACBE7..0x009ACBF1`. An exact positive wrap therefore leaves
the endpoint as the last target write on that visit.

Group evaluation walks the retained vector in order and calls each child's
`0x009AC820` (`0x009CACB4..0x009CACC9`). A child before BeginTime
returns at `0x009AC990..0x009AC9A8`. A completed finite child returns
at `0x009ACA30..0x009ACA3C`. An eligible DoubleAnimation converts its
result and calls the ordinary property setter at
`0x009B4C88..0x009B4CA0`. This group path has no separate blend step.

INFERRED. For these installed non-overlapping rise and fade intervals, the
completed rise does not keep overwriting the later fade. A delayed track
does not impose its From value before BeginTime. The last admitted opacity
write is retained between eligible samples, including across a group reset.
Replaying the same ordered samples with retained property snapshots can
therefore represent this hold. Writing every child's clamped start or end
on every visit would change it. This does not settle arbitrary competing
storyboards or property-listener side effects.

STATIC. Group Stop calls every child's virtual `+0x3C` at
`0x009CAD24..0x009CAD3A`, then enters Timeline Stop at `0x009CAD42`.
`0x009AC3C0` disables its timer and clears the active byte without restoring
the target property. Beginning the retained group again repeats setup and
resets its clock. Setup and clock reset do not write target opacity.
INFERRED. Stop preserves the last property write unless a separate setter
changes it. A later Begin likewise retains it until an eligible sample writes.
The level-0 through level-3 triggers above contain their own opacity setters
before Stop. No original live pulse or rendered sample was observed here.

## Command and cache

The null-terminated UTF-16 pointer table at `0x0126DA64` contains
`RaptureCommands.HateLevel0` through `RaptureCommands.HateLevel4`,
in level order, followed by a null cell at `0x0126DA78`.
Constructor `0x00552660` resolves each name through `0x00928280`
at `0x005526DA` and appends the returned pointer.

A level equal to the cached dword returns without a command
(`0x0054B96A..0x0054B96D`). A changed level dispatches only if the
command vector contains that index and the pointer is nonnull.
It builds a parameter record through `0x0052D330` and invokes command
virtual `+8` with the Label interface `+0xB4` and null trigger args
(`0x0054B96F..0x0054B9CC`). The
[routed-command path](sqwt-command-parameter-consumer.md#target-and-route)
qualifies PreviewExecuted, Executed and parent traversal when that command
is the ordinary RoutedCommand. The helper then stores the new level at
`0x0054B9E2`, including when no command pointer was available. Its cache
is a level cache. This path has no separate selected-actor identity cache.

These static joins establish the form's command input and thresholds.
The exact selected form, resource attachment, command admission, rendered
skin and original live timing remain conditional. The word's interpretation
as a percentage, a server enmity formula and update cadence remain unknown.

## Target name foreground

STATIC. The form gate also constructs TargetParameterWidgetOperator,
constructor `0x008D8BA0`, primary table `0x0106376C`. Its frame slot
`+0xA0`, cell `0x0106380C`, selects `0x008D8A20`. FormElement calls
that slot at `0x00539A15`, before the separate AutoAttack-part guard
used by the enmity helper. The constructor resolves TextBlock_Name
through `0x00696030` and retains it at operator `+0x20`
(`0x008D8C29..0x008D8C48`). The installed form declares this unique
TextBlock at line 45, as the [caption contract](../script/target-caption-text.md#lua-choice-and-form)
records. The four cached colour lanes at operator `+0x24..+0x30`
start as binary32 1.0 (`0x008D8BFD..0x008D8C1A`).

Frame `0x008D8A20` obtains TargetElement through `0x004D7310`, reads
current cursor `+0xB4`, then actor `cursor+0x0C`. A null cursor or actor
returns without a foreground write or cache reset
(`0x008D8A2D..0x008D8A45`). For the admitted character actor it reads
NamePlate at actor `+0xBA0`, virtual `+0x14`, with literal part 1
(`0x008D8A4D..0x008D8A62`). NamePlate table cell `0x00FCF9A0`
selects `0x006A29D0`, which returns `this+0x68+part*0x10`.
Part 1 therefore supplies the four lanes at NamePlate `+0x78..+0x84`.

The [nameplate native contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/presentation-native-contract.md#nameplate-natives)
traces the int and four float `_setNameplateColor` arguments through
`0x006E3E00 -> 0x0075B570 -> 0x006A3AD0`. The leading int is the part.
The setter stores the final float first, followed by the preceding three
floats, in that selected record. For part 1 it returns after these stores. The
record is A,R,G,B under the documented RGBA interpretation. Those four
lanes supply the target foreground sample.

It compares the four sampled lanes with its cache at
`0x008D8A64..0x008D8AE3`. A difference requests the typed colour setter
`0x00938020(sample,0,1)` on retained TextBlock_Name at `0x008D8AF1`,
then copies the sample into all four cache lanes at
`0x008D8AF6..0x008D8B1D`. This setter supplies descriptor
`0x01359244`, named Foreground by `0x00F22540..0x00F2254A`.
The [inherited property contract](sqwt-qualified-property-lookup.md#unqualified-foreground-and-completion)
owns that descriptor. The typed call uses it directly. Property admission
and later drawing remain conditions. Equal samples skip the setter across
frames and actor changes. The absent-target return precedes sampling.

The actor's [depiction rules](../net/work-field-inventory.md#npcworkhatetype-is-a-single-field-client-should-populate)
already record the NPC claim/occupancy and party colour inputs.
Authenticated DepictionJudge.judgeNameplate supplies part-1 colours in
`main/f0`, including the getHateType arms at zero-based PCs 425-475
and 482-556. The selected branches read the claim and party
inputs. The enmity word feeds the separate indicator described above.
Server claim policy remains outside this trace.

The installed TargetParameterWidget Lua has separate HP and rank paths.
`setParameter`, `main/f7` PCs 0, 12-16 and 29-32, requests
PRB_basis_newHpLarge by default and PRB_basis_newUdLarge when isUndead
returns exactly true. `setRank`, `main/f11` PCs 0-44, requests the numeric
level or `??` text and its visibility under the
[published actor guards](../script/target-selection-receivers.md#populacestandard-work-and-target-guards).
Their authenticated LPB identity is in the caption contract. The examined
paths establish depiction-driven target-name foreground and an undead HP
skin choice. A relative-level colour table and additional difficulty
writers remain untraced.

Ghidra CLI supplied focused leads. LLVM disassembly and direct PE reads
corroborated the frame call, vtable cells, colour stores, comparisons and
Foreground identity. Authenticated original Lua 5.1 bytecode corroborated
the named script branches. No original client ran and no target-switch
pixels or live difficulty colours were observed.

## Target status icons and level-text boundary

STATIC. After sampling the name foreground, the same target operator frame
`0x008D8A20` samples NamePlate icon slots 1 through 3. Constructor
`0x008D8BA0` formats `IconControl_StatusIcon_%d` from literal `0x01063620`
and resolves each named form control through `0x00696030`
(`0x008D8C58..0x008D8CA4`). It retains slots 1, 2 and 3 at operator
`+0x4C`, `+0x50` and `+0x54`. Their separate integer caches at `+0x38`,
`+0x3C` and `+0x40` initialize to -1. The authenticated base form declares
those exact three IconControls at lines 40, 41 and 43, each with Width and
Height 24, IconDatas 220, IsHitTestVisible False and Visibility Collapsed.
The [caption input](../script/target-caption-text.md#lua-choice-and-form)
pins this form's carrier and decoded identity.

The frame's existing null-cursor and null-actor returns precede this loop.
On its admitted current cursor actor, NamePlate is actor `+0xBA0`.
Frame `0x008D8B22..0x008D8B89` calls its virtual `+0x18` with indices 1,
2 and 3. Cell `0x00FCF9A4` selects `0x006A29E0`, which reads the integer
at NamePlate `+0x30 + index*4`. These are icon IDs, distinct from the
four float lanes read by the foreground getter.

Each sample is compared with its corresponding cache. An equal sample
skips both property requests (`0x008D8B41..0x008D8B44`). A changed sample
below zero requests Visibility 0, Collapsed, through `0x0093C2A0` with
arguments 0,1 (`0x008D8B6C..0x008D8B78`). That branch does not clear
IconDatas. A changed sample at least zero first requests Visibility 2,
Visible, with the same arguments, then calls `0x00695470` on the retained
IconControl (`0x008D8B4E..0x008D8B65`). This helper formats one signed
decimal integer with `%d` and requests IconDatas through `0x0096A010`
with arguments 0,1 (`0x006954A8..0x006954F5`). Both branches cache the
sample at `0x008D8B7D`.

The [nameplate native contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/presentation-native-contract.md#nameplate-natives)
records `_setNameplateIcon` and its category/position/ID arguments.
`0x006A2B30` stores a positive supplied ID in the slot sampled here and
stores -1 for an ID at most zero. The target consumer itself accepts a
stored zero as nonnegative, but that setter does not produce zero.
A position greater than 1 can form a multi-cell world NamePlate IconDatas
string. This target consumer formats only the retained scalar ID, not that
string. Its slots map to NamePlate's primary-left, primary-right and
secondary-left controls, not the separate secondary-right slot 4.

The [installed icon producer](../actor/player-nameplate-icons.md#other-controls-and-native-state-icons)
owns the Lua classification and icon identities. For example, the admitted
non-player property-3 battalion arm supplies primary-right 517 when
getAggro is positive and 518 otherwise. Their authenticated images are the
red spiked disc and green disc. A retail-dealer arm supplies 220 in that
same slot. Player and group/status branches supply slot 1, while repair and
dealer branches can supply slot 3. This is a reuse of actor NamePlate
status/service icons. Neither the slot names nor those branches establish
a self-versus-target level comparison or a difficulty colour table.

MEASURED. The same base form's TextBlock_LebelText at line 42 selects
`TBL_namePlate_levelText`. In authenticated `common/default.style`, lines
3306-3315, its Frame part has Skin NULL and FontColor `#ffffff99` for
SkinNormal, SkinHover, SkinActive, SkinFocused, SkinAnchored and
SkinReserved. Those conditions have FontEdgeColor `#ff262626`.
SkinDisabled has FontColor `#80ffff99` and FontEdgeColor `#801a1a1a`.
SkinDroppable declares Skin NULL without either colour. The common style
identity is in the
[numeric presentation input table](damage-plate-numeric-presentation.md#identity-and-method).
These selected declarations supply no relative-level binding. The installed
setRank path described above supplies text and visibility, not a colour.

The native icon loop has no separate actor-identity cache or null-target
reset. Equal icon IDs across actor changes skip writes. A fresh -1 cache
and a first -1 sample preserve the form's declared collapsed state.
The loop itself has no additional per-control null test, so its property
requests require the named current controls to have resolved and their
property types to be admitted. The frame's actor admission, property
priority, other writers, actual icon resource loading and rendered pixels
remain conditions. These findings do not establish a relative-level
classifier elsewhere in the executable. Its producer and a visual consumer
remain UNKNOWN. No original client ran.
