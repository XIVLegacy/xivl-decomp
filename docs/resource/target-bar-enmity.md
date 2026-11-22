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
