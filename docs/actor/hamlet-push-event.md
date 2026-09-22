# Hamlet PushEvent presentation

The recovered FFXIV 1.23b `PopulaceHamletPushEvent` bytecode defines actor
roles, support dialogue, gesture selectors, and craft-choice return values. It
is a client presentation and selection contract. It does not authorize item
changes, select BG-object damage stages, or identify the missing server
entrypoints.

## Actor roles

The 61-instruction `initTypeWork` prototype assigns eleven actor classes. A
direct actor-class-table path exists for eight of the twelve related rows;
blank paths below are not filled from class-file existence.

| Actor class | Actor-class path | Work type | Role and appearance |
| ---: | --- | ---: | --- |
| 1500435 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 1 | human c008 |
| 1500387 | unresolved | 1 | human c008 |
| 1500436 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 1 | human c008 |
| 1200360 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 2 | harvest type 1, b989/e004 |
| 1200361 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 2 | harvest type 2, b989/e002 |
| 1200362 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 2 | harvest type 3, b989/e003 |
| 1200381 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 3 | support, b989/e001 |
| 1200369 | unresolved | 4 | hidden supply-cache variant, b987/e001 |
| 1200370 | unresolved | 4 | hidden supply-cache variant, b987/e002 |
| 1200371 | unresolved | 4 | hidden supply-cache variant, b987/e003 |
| 1200372 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 4 | hidden supply cache, b987/e004 |
| 1500437 | `/Chara/Npc/Populace/PopulaceHamletPushEvent` | unresolved | human c005; owner path only |

The reconstructed bytecode corrects damaged chained equalities in the
decompiled Lua: 1500435 and 1500387 are type 1, while all four b987 actors are
type 4. Actor 1500437 is absent from `initTypeWork`; its class path does not
justify inventing a work type.

The four b987 and four b989 appearances establish available model variants.
They do not assign damage stages. The audited battle data uses 1200372,
b987/e004, for every supply cart, so substituting e001-e003 as progressive
damage states would be reconstruction.

## Dialogue and gestures

The recovered methods contain 21 `sayText` calls. Nineteen run a scheduler
before speaking, using ten packed selectors:

| Selector | Lane and bank | Calls |
| --- | --- | ---: |
| `0x0502B000` | `cmn/em1/0043` | 1 |
| `0x15190000` | `cmn/liu/0400` | 1 |
| `0x15194000` | `cmn/liu/0404` | 3 |
| `0x15195000` | `cmn/liu/0405` | 2 |
| `0x15196000` | `cmn/liu/0406` | 2 |
| `0x151A4000` | `cmn/liu/0420` | 1 |
| `0x151A7000` | `cmn/liu/0423` | 2 |
| `0x151AA000` | `cmn/liu/0426` | 1 |
| `0x151B2000` | `cmn/liu/0434` | 4 |
| `0x151B3000` | `cmn/liu/0435` | 2 |

All ten banks exist for both c005 and c008, producing 20 hash-identified
humanoid scheduler profiles. Those resources prove the gestures for humanoid
owners. They do not show that a b987 or b989 BG object can play a humanoid
scheduler.

The 207-instruction `talkToSupport` prototype calls `askForEventMode` once per
menu iteration and stores its result once. Repeated menu calls in the recovered
Lua text are a decompiler artifact. For every gesture-bearing line, the order
is `_runCharaScheduler(selector)` followed by `say`. Choice 1 returns to the
external owner without `finishCliantTalkTurn`; the other completed or exit
paths finish the client talk turn as encoded.

## Craft-choice return matrix

`talkToCraft` opens one five-choice widget. Cancel, non-positive results, and
choice 5 return nil. Choices 1 through 4 return one item ID selected by the
choice and crafter group. One `getStateMainSkill` call yields two compared
values: either value in 29-31 selects group 1, either value in 32-34 selects
group 2, and all other pairs select group 3.

| Choice | Client description | Group 1 | Group 2 | Group 3 |
| ---: | --- | ---: | ---: | ---: |
| 1 | defense and evasion | 10011231 | 10011235 | 10011239 |
| 2 | attack power | 10011232 | 10011236 | 10011240 |
| 3 | Regen | 10011234 | 10011238 | 10011242 |
| 4 | reset enmity and block enemy orders | 10011233 | 10011237 | 10011241 |

These are exact client return values, not transaction authority. The recovered
client does not establish server inventory validation, skill validation, item
consumption, delivery, or effect application.

## Hamlet success result

The Hamlet-specific success entrypoint is
`InstanceRaidHamletDefense.localClearEvent`. It prints NPC line 2, checks that
`_countHamletDefenseScore()` is non-nil, and then calls
`askHamletDefenseScoreWidget(getContentID())`. The widget reads individual
score rows and totals and returns base ask result 1 on confirmation.

`InstanceRaidBaseClass.clearEvent` does not call `localClearEvent`; it performs
the generic countdown, desktop-mode, information-widget, and notification
cleanup. The static sources do not identify the server call or packet that
invokes the Hamlet-specific method, its ordering relative to the ending scene
and score publication, or the required attached-director lifetime. A generic
score-widget open is not an equivalent replacement.

## Provenance and evidence boundary

The recovered Lua has SHA-256
`3c63fb270df9c96830cc5215e0317d571e13d620c06cfa07a8389b131bf6c8c8`;
the decoded Lua 5.1 chunk used to reconstruct the damaged branches has SHA-256
`b67f55b1f999b8250aee8bc9801d9d0d81353e4a1ce106a51968bcf36c145a9b`.
The retail text sheet has SHA-256
`0b72489923dea98b319b7957ebf012eb513f12f142b84ae3fe6e9f68347302cb`.
The Hamlet director Lua and bytecode hashes are
`5dd3cfec23db4ccb531e57973be26b80b9d97e5b1594b37d0c80ea2d28ce9968`
and
`ff329e9dec60838a0d0fd5e2c42a27aba60365bf52b44bcb8d6d43c709eb1a00`.

The evidence proves the eleven work assignments, eight direct class-path
rows, humanoid dialogue choreography, craft return matrix, and success-widget
client entrypoint. The external PushEvent argument owners, result handoff,
server transactions, BG-object state meanings, success invocation, result
ordering, and failure type remain unavailable. They must not be supplied from
class or asset existence alone.
