# Work update callback consumers

The installed `_onUpdateWork` methods of `CharaBaseClass` and `PlayerBaseClass`
read the `desktopWidget` global without a nil test on the paths that reach the
DesktopWidget. `CharaBaseClass` forwards every update except `_init` and
`charaWork/commandAcquired` to the connector method
`processCharacterParameterUpdated`. `PlayerBaseClass` calls specific
`desktopWidget:processUpdate*` methods for its `playerWork` slots and sends the
remaining slots to the `CharaBaseClass` method through its registered parent
([native class parents](native-class-registration-parents.md), STATIC). For the
local player the connector repaints the PlayerParameterWidget rows when the slot
is `stateAtQuicklyForAll`. This page records those script chains, the group
names the slot strings come from, and what the evidence leaves open.

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe`, as recorded on the cited tracked page |
| INFERRED | A role or ordering taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The scripts are the `client/script` chunks of retail build `2012.09.19.0001`.
Each original LPB is unwrapped by the rules in [LPB format](lpb-format.md)
(`xivl extract-lpb`), and the decoded Lua 5.1 chunk is listed instruction by
instruction with the pinned unluac `--disassemble` listing of
`xivl-client-scripts:tools/vendor/unluac/PROVENANCE.json`. PCs are zero-based
instruction indices of one function. Prototype labels such as `main/f23` name
the direct children of the chunk root in listing order. The listing is the
authority for every OBSERVED fact below. No client code ran.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `chara/charabaseclass` | `client/script/729s9/729s989r57y9rr.le.lpb` | 13821 | `c7cd1dad84f7d9b723f5a10fb66108da9d9d6ef5d7c15186b48ec55092465238` |
| `chara/charabaseclass_parameter` | `client/script/729s9/729s989r57y9rr_u9s9x5q5s.le.lpb` | 14673 | `e60a1ad866e4275e2f4e2e536697ac78a4926161820ade6ac07d939c6cf446bc` |
| `chara/player/playerbaseclass` | `client/script/729s9/uy9l5s/uy9l5s89r57y9rr.le.lpb` | 25846 | `32182274f3886d0cdf6329f8a82e6592790c9b3038e00bebe5b2b38702bbd53e` |
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `widget/playerparameterwidget` | `client/script/n1635q/uy9l5su9s9x5q5sn1635q.le.lpb` | 5073 | `bd2056e45a2119fcd7778e17f2ad6baa9647eadd72e0c1068e07bdeadccb5422` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `chara/charabaseclass` | 13808 | `101cafa86ebcb362b502e47dd33e262ede31b4939cc7fe945ab571e1afd0d3a9` |
| `chara/charabaseclass_parameter` | 14660 | `cbf1856bed20810193e8b7d3868bb469e6bf6955de781fe8ce285bdd050e4ee8` |
| `chara/player/playerbaseclass` | 25833 | `1530dc2b53353888ccc6b47f78920ebd2c2d876335b9968196312153b0db464a` |
| `widget/desktopwidget_connector` | 213474 | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/playerparameterwidget` | 5060 | `bc63510ffde6412ea29f39bdf356a88258b106b7330ff580cb5312a4a6f36d43` |

The same five resources are pinned independently in
[StatusEffectWidget lifecycle](status-effect-widget-lifecycle.md#inputs-and-method)
and the
[PlayerParameter TP consumer contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/player-parameter-tp-consumer.md).

## Callback entry points

OBSERVED. `CharaBaseClass._onUpdateWork`, `main/f23` of `chara/charabaseclass`,
takes the receiver, a struct name, a slot name and two optional indices. It
runs in this order.

| PCs | Step |
|---|---|
| 2 to 9 | `worldMaster:_getMyPlayer():getDepictionJudge():judgeNameplate(self)`, before any test of the arguments |
| 10 to 26 | For slot `_init`, or struct `charaWork` with slot `property`, the two `isPropertyEnabled` tests (arguments 1 and 5) that feed `_setNameplateVisible` |
| 27 to 32 | For slot `_init`, `processUpdateInitWork(struct)` and a return |
| 33 to 41 | For struct `charaWork` with slot `commandAcquired`, `processUpdateCommandAcquired` with both indices and a return |
| 42 to 50 | Every other update: `desktopWidget:processCharacterParameterUpdated(self, struct, slot, index1, index2)` through `getglobal desktopWidget` at PC 42, then `self` at PC 43, the call at PC 49 and the return |

The global read at PC 42 has no nil test. The `judgeNameplate` chain at PCs 2 to
9 also has none, so it needs the local player, its `depictionJudge` object and
that object's `judgeNameplate` method.

OBSERVED. `PlayerBaseClass._onUpdateWork`, `main/f30` of
`chara/player/playerbaseclass`, tests the struct name first. For struct
`playerWork` the slots `confirmWarpCommand`, `confirmRaiseCommand`,
`npcLinkshellChat`, `combo`, `consoleTray` and `expBonus` each read the
`desktopWidget` global with no nil test (PCs 4, 10, 16, 32, 38 and 44), and
`questCompleteS` or `questCompleteG` goes to `processUpdateQuestComplete`. For
struct `charaWork` the slot `command` runs the place-driven command variation
and the slot `timingCommand` reads `desktopWidget` at PC 100. Every remaining
slot reaches `_callSuperClassFunc("_onUpdateWork", ...)` (PC 50 and PC 103),
which is the `CharaBaseClass` method above.

## Desktop dependency

OBSERVED. The connector method `processCharacterParameterUpdated`, `main/f78`
of `widget/desktopwidget_connector`, returns for a nil actor (PCs 4 to 6) and
then compares the actor with `getCurrentTargetCharacter` (PCs 7 to 9). Its arms
index the value `getStaticWidget(n)` returns without testing it for nil. The
calls sit at PCs 11, 61, 187 and 221, and the unguarded index follows at PC 17
or 23 in the target arm, PC 72 or 79 in the party arm, PC 190 in the
local-player arm and PC 224 in the `stateAtQuicklyForSelf` arm. The DesktopWidget `init` method, `main/f0`,
calls `setDefaultWidgetLocation` at PC 316. That method, `main/f142`, holds the
sequence of `createStaticWidget` calls and lists static index 1
`MainMenuWidget`, 2 and 3 `LogWidget`, 4 `PlayerParameterWidget`, 5
`TargetParameterWidget`, 6 `PartyParameterWidget`, 7 `ActionMenuWidget` and 16
`StatusEffectWidget` among others (PCs 0 to 70 and following). This listing
requests their creation; it does not establish completed creation or live
availability. INFERRED. `init` runs from
the inherited `_onInit` path that `DesktopWidget._onInit` starts with
`_callSuperClassFunc("_onInit", ...)`, because the status page records the
desktop initialization requesting widget creation
([Initial show request](status-effect-widget-lifecycle.md#initial-show-request-and-remaining-unknowns)).

INFERRED. A callback path that indexes an absent `desktopWidget`, or reaches
one of the unguarded static-widget indexes above with a nil result, fails
there. This does not cover every non-initial `_onUpdateWork`: the published
`charaWork`/`commandAcquired` path returns before `main/f23` PC 42, although
it still requires the preceding worldMaster/player/judge chain. Whether the
`desktopWidget` global exists before `init` returns is NOT
TRACED, because no tracked page shows where the client first assigns it. A
headless host that assigns the global late reports `attempt to index global
desktopWidget` (AUTHORED by that host's timing, not retail evidence). The client
reports a Lua error through the handler that
[queues dialog code 40000](delegate-command-producer.md#lua-error-inside-the-called-function).
That path is conditional on lookup and dialog completion, tests a one-shot
handler byte (`+0x24`), and rests on an INFERRED choice of the Client
`ErrorHandler`. This page therefore does not claim that a retail session would
show the dialog. The [readiness gate](work-callback-readiness-gate.md)
records a bounded native record/receiver predicate and the published queue's
conditional readiness and Desktop-key gates. It does not establish that the
Desktop Lua initialization completed before this callback. The status page
leaves the order of the player and desktop initializations UNKNOWN
([Player publication and initial population frontier](status-effect-widget-lifecycle.md#player-publication-and-initial-population-frontier)).

## Parameter dispatch

OBSERVED. In `main/f78` the arms below are the ones that repaint HP, MP and TP
rows. The first two reach the TargetParameterWidget (static index 5) and the
PartyParameterWidget (static index 6). The last two reach the
PlayerParameterWidget, which static index 4 holds. The slot is argument 3 of the
method (struct is argument 2).

| Actor tested | Slot | PCs | Calls |
|---|---|---|---|
| The actor is the current target character | `stateAtQuicklyForAll` | 21 to 24 | `getStaticWidget(5):updateHp()` |
| The actor is a party member (`isJoinedCharacterAtMyParty` above zero, PCs 64 to 68) | `stateAtQuicklyForAll` | 77 to 87 | `getStaticWidget(6):updateHp(index)`, then `updateMp(index)` and `updateTp(index)` |
| The actor is the local player (`isPlayer` at PC 108, `isMyPlayer` at PC 112) | `stateAtQuicklyForAll` | 185 to 198 | `getStaticWidget(4):updateHp()`, `updateMp()` and `updateTp()`, then `updateActionMenuWidget(false)`, then the StatusWidget and EquipWidget updates when those widgets exist |
| The actor is the local player | `stateAtQuicklyForSelf` | 219 to 231 | `getStaticWidget(4):updateTp()` and `getStaticChildWidget(4, "PlayerProfileWidget"):update()` |

Each arm tests the slot string only. None tests the struct name, so the arms run
for any struct that carries that slot name.

OBSERVED. `PlayerParameterWidget.updateHp`, `updateMp` and `updateTp` call
`setParameter` with row 1, 2 and 3. `setParameter` reads
`desktopWidget:getPlayerGaugeMaxParameter()` and
`getPlayerGaugeCurrentParameter()` and writes both the maximum and the value of
the selected row through `setHPMPTPMaximum` and `setHPMPTPValue`
([TP consumer contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/player-parameter-tp-consumer.md)
owns the operand sources of the third row). A repaint of one row therefore
reads the current work members and writes the row, with no cache the listing
shows.

## Where the slot strings come from

OBSERVED. `CharaBaseClass._onInit`, `main/f21` of `chara/charabaseclass`, calls
`initCommonParameterSync` of `chara/charabaseclass_parameter` at PC 168 when its
`gameParameter` argument is true (the test at PCs 164 to 165). With the argument
false the lists are empty (PCs 204 to 207). Every actor then reaches the
`charaWork._tag` write at PC 415. The local-player flag (`isPlayer` and
`isMyPlayer`, PCs 11 to 16) only decides whether the `Self` groups are appended
(PCs 313 to 324), so an actor that carries `gameParameter` holds the groups of
the "all" list and only the local player holds the `Self` ones. The tracked
group table is
[Tag groups](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/work-binder-native-contract.md#tag-groups).
It lists `stateAtQuicklyForAll` with interval 0.3 and five members, which the
`initCommonParameterSync` listing confirms as `parameterSave.hp` and
`parameterSave.hpMax` (each with element index 1), `parameterSave.mp`,
`parameterSave.mpMax` and `parameterTemp.tp`. `stateAtQuicklyForSelf` applies
to the local player and holds `parameterSave.state_boostPointForSkill`.

STATIC. A completion target on a `0x0137` stream supplies the two callback
strings after the receiver, so a target spelled `charaWork/<group>` raises
`_onUpdateWork("charaWork", "<group>")`
([Actor work completion targets](../net/actor-work-completion-targets.md#owner-readiness-and-callback-arguments)).
INFERRED. A stream closed by `charaWork/stateAtQuicklyForAll` therefore reaches
the local-player arm above with the group name as the slot. The two pages are
joined here and no single trace shows the join end to end.

## Open items

- PARTIAL. The [native member walk](work-tag-handling.md) records a bounded
  group/member consumer and conditional scalar writer construction. The
  selected interval predicate, per-member notification discriminator and
  change-to-`_onUpdateWork` join remain unresolved. Whether the client raises
  `_onUpdateWork` for a changed member without a completion target is not shown
  ([Open](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/work-binder-native-contract.md#open)
  in the work binder contract).
- NOT TRACED. Which wire target the retail server sent for a change of HP, MP
  or TP. The captured targets in the retail pcaps are scrubbed.
- NOT TRACED. Whether a retail client repaints the rows for a stream that
  closes with `_init` or with a group other than `stateAtQuicklyForAll`. The
  listing shows that the connector arms need the slot, and an `_init` slot
  returns before them (`main/f23` PCs 27 to 32).
- NOT TRACED. The ordering that keeps `_onUpdateWork` from reaching the connector
  before the DesktopWidget init has created its static widgets. The
  [bounded native readiness gate](work-callback-readiness-gate.md) does not
  establish that Lua initialization or a universal ordering guarantee.

## Reproduction

For each resource in the input table, unwrap the original LPB with
`xivl extract-lpb`, check the decoded size and SHA-256 above, and list the chunk
with `java -jar unluac_2025_12_23.jar --disassemble`. The function for each
locator is the prototype named in the text, and the PCs count its instructions
from zero, skipping label and directive lines.
