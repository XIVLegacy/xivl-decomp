# Actor friend and enemy classification

The recovered 1.23b scripts use several actor relations. The desktop's player
and enemy target modes depend on the actor's player method, battle-property
Boolean and class-defined battalion. Name color additionally depends on death,
party, content and occupancy state. Target HP has a separate positive-maximum
test. None of these observations establishes a universal hostile actor flag or
server attack permission.

## Sources and confidence

Native input is retail `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86,
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Native addresses below are VAs; RVA is VA minus
`0x00400000`. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 decoded the
selected original file-backed ranges after full input size/hash checks.
They did not execute the client or open a mutable analysis database.

Script evidence is authenticated recovered Lua, not original Lua source or
observed execution. The decoded corpus archive has 14385427 bytes and SHA-256
`0e8f902f7a2f592fc1220d41b89a3f35ec395cfb261806d4bd590a530099ae31`,
pinned by `xivl-client-scripts:manifests/private_lua_corpus.json`, input
revision `40006d5d716583d78690a6f3ef50ca1bc41dddee`. Every selected member's
size/hash matched `xivl-client-scripts:manifests/scripts.json` at revision
`0badc824adb353ca36ea41a61ee255a92a3231e2`. Line locators below refer to those
archive members. The archive need not be extracted to inspect them.

| Archive member | SHA-256 |
| --- | --- |
| `widget/desktopwidget_connector.lua` | `9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01` |
| `widget/targetparameterwidget.lua` | `b3f1058cd2e25f94dde2358e5695c410cdf84f8386bec2a3c7421b7b1f7fa901` |
| `judge/depictionjudge.lua` | `8db4d613d72ddf1921e3c9c4a961714371d2eac38fcc3b0211023105cb41a3e1` |
| `chara/charabaseclass.lua` | `f17b3e62a137d8f524ba88d55296e1fb064512dc803f7a85dc980dd4bcbc9625` |
| `chara/charabaseclass_cliprog.lua` | `ea04844921f8820562afd172845cab7baf2df235725d20ad179bb713754c87fd` |
| `chara/charabaseclass_parameter.lua` | `748c02dbf42329dd0b942642b5fe68b35f422db61c482954e3b00d0338f12893` |
| `chara/charabaseclass_battle.lua` | `ca7d9bd52320e8aa62ecd853a0ce8d9e2e0ddc76fc504729b501b14b9bfcd959` |
| `chara/npc/npcbaseclass.lua` | `f18c6c3198db0402431ca6aa683b601afbf5a0dc5c22481b3c1da1deabef0dc2` |
| `chara/player/playerbaseclass.lua` | `6226b3fa15dfdbad279b7dba453f8a3b76fcb8b68bad6e14f5403d52987f76e4` |
| `chara/npc/monster/fighter/fighterallyopeningattacker.lua` | `97eaa229de8442c241236a75e36ef66c57ddd8ae91dd021722005a17fb08b624` |
| `chara/npc/monster/fighter/fighterallyopeninghealer.lua` | `8814f06af6986984b176a6d5efcb092fab6aa0a9ab243e287855168c341c598c` |

Fresh original bytecode was not available for these selected predicates.
Simple method bodies and comparisons below have recovered-script confidence.
The complex player-fallback and content/occupancy fallthrough rendering remains
qualified below.
The independently promoted depiction-judge bytecode identity and ordinary
hate/occupancy conjunction are retained in
[work-field inventory](../net/work-field-inventory.md#npcworkhatetype-is-a-single-field-client-should-populate).
That prior check does not authenticate every new branch as bytecode.

## Target modes and property indices

`DesktopWidget.checkPlayerModeTarget`, archive
`widget/desktopwidget_connector.lua:1885-1913`, returns true in this order:

1. `actor:isPlayer() == true`.
2. `actor:isPropertyEnabled(3) == false`.
3. `actor:getBattalion() == 1`.

Otherwise it returns false. `checkEnemyModeTarget` at lines 1916-1930 returns
false when the former is true, and true otherwise. For initialized actors
whose property value is Boolean, enemy mode is therefore non-player AND battle
property true AND battalion other than 1. These are desktop filter predicates,
not proof that an attack command can execute or that a server accepts damage.

The same connector's `isValidModeTarget` at lines 1969-2066 selects these
predicates from local `work.targetMode`: 2 calls the player predicate at
1973-1980, and 4 calls the enemy predicate at 1985-1992. It can subsequently
apply `isInvalidEnmityTarget`. `_onCheckTargetable` calls this mode filter at
1801-1809 and rejects its false result. `requestAutoLockonTarget` at
2913-2955 rejects a true player-predicate result at 2942-2948 before calling
`lockonCurrentTarget`. These are concrete recovered call edges, not runtime
observations or an equivalence with every target/command eligibility test.

`isPropertyEnabled(i)` at `chara/charabaseclass_cliprog.lua:41-65` reads Lua
`charaWork.property[i]`. Index 2 additionally returns false if Lua property 1
is falsey. `isBattleCharacter()` at
`chara/charabaseclass_battle.lua:180-188` returns property 3. Target admission
`isValidTarget` at connector lines 1847-1882 first rejects property 2 false,
then rejects a dead actor when config 43 is true or enemy mode is true.
Death admission calls `isDead`; name color calls `isDeadMode`.

Lua indices are one-based. The native array bridge `0x00D2F220 ->
0x00D2E0A0` checks `1 <= index <= [container+0x58]`. At `0x00D2E108` it calls
`0x00D1A620`, which decrements a positive index, then passes it to the child
descriptor's slot 7 at `0x00D2E12B`. The Boolean reader `0x00D28890` passes
that normalized index through `0x00D25900`, adding one bit per element.
`0x00D11C00` splits a bit position into byte offset `position >> 3` and bit
`position & 7`. These original instruction observations agree with
[native Boolean layout](../script/job-action-acquisition.md#native-boolean-layout).
Consequently Lua property 3 selects canonical property `[2]`, not `[3]`.
The wire names and hashes are owned by
`xivl-client-structs:docs/actor-hostility-classification.md#wire-inputs`.

The full decoded-corpus identifier search found no `isAttackable` or
`isBattleNpc` methods. This is a bounded identifier result, not a whole-native
attackability exclusion. The actual recovered surfaces above and
`judgeRelation` below must retain their distinct contracts.

## Allied fighters, players and party members

`chara/charabaseclass_parameter.lua:1404-1410` returns literal 0 from the base
`getBattalion()`. Both opening ally members in the source table declare
`FighterBaseClass` as parent at lines 2-8 and return literal 1 from their
`getBattalion()` override at lines 11-17. The authenticated parents are
`chara/npc/monster/fighter/fighterbaseclass.lua:2-8 -> MonsterBaseClass` and
`chara/npc/monster/monsterbaseclass.lua:2-8 -> NpcBaseClass`. Their member hashes are
respectively `556d91b5cb12d1858292cb9bf016012c47cc78c6a0e3041af4ff81ba3fb80595`
and `08aff923704ebec221743e1cdfe6d79d3173164dd5eb4de250ce1302e3629a76`.
The [native parent registrations](../script/native-class-registration-parents.md)
join NpcBaseClass and PlayerBaseClass to CharaBaseClass at `0x0075FD30` and
`0x0075FC4C`, and Player to PlayerBaseClass at `0x0078FFF4`.

The supported dispatch inference is that each opening ally reaches battalion
1 through this hierarchy. It passes player target mode even with battle
property true. A Monster path prefix therefore cannot force enemy mode.
Other scripts also override battalion; an observer needs the actual inherited
method, not an allowlist containing only these two examples. This battalion
is a script constant, with no incoming battalion field in these definitions.

`NpcBaseClass.isPlayer` returns false at npcbaseclass lines 10-16.
`PlayerBaseClass.isPlayer` returns true at playerbaseclass lines 16-22.
The CharaBaseClass base rendering returns nil at charabaseclass lines 13-18.
`PlayerBaseClass.isMyPlayer` at lines 1181-1191 compares the actor with
`worldMaster:_getMyPlayer()`. Players therefore pass player target mode
regardless of party membership. A non-player's membership alone does not
alter that mode predicate.

`chara/npc/populace/populacestandard.lua:2-19`, SHA-256
`c2d68f8652e7bba576af1b7134c9e00353f1a2f0aed760afe8b48e791f5c9558`,
declares NpcBaseClass as parent and adds `initForEvent`. It does not assign
battle property, HP or a hostility value. A Populace class name alone does
not establish the current value of those inputs.

## Name color is a separate selector

`judge/depictionjudge.lua:343-804`, `judgeNameplate`, uses slot 1 color calls.
The table preserves the recovered color sites and their tests. Qualified
fallthrough edges do not constitute a complete verified color algorithm.

| Earlier matching case | RGBA after the slot argument | Source lines |
| --- | --- | --- |
| Dead mode | `(0.6,0.6,0.6,1)` | 343-363 |
| Self player | `(1,1,1,1)` | 365-377 |
| Other player in viewer's player party | `(0.5,1,1,alpha)` | 379-413 |
| White call in the player branch; exact non-self fallthrough unestablished | `(1,1,1,alpha)` | 416-445 |
| Non-player in viewer's player party | `(0.5,1,1,1)` | 449-466 |
| Other non-player, Lua property 3 true, battalion 1 | `(0.5,1,0.5,1)` | 468-500 |
| Other battle non-player | Hate/occupancy selector below | 501-681 |
| Non-battle retainer | Owner-member `(0.7,0.5,1,1)`, other `(0.5,0.5,1,1)` | 684-740 |
| Other non-battle talkable actor | `(0.5,1,0.5,1)` | 742-770 |
| Remaining actor with map-marker range | `(0,0,0,0)` | 773-791 |
| Fallback | `(0.5,0.5,0.5,1)` | 792-804 |

For other players, `alpha` is 1 when accessible in server and 0.3 otherwise.
The judge obtains the viewer, viewer player party and current content group
at lines 11-25. Party cyan precedes allied green. A talkable non-battle actor
can receive the same green as an allied battle actor by a different branch.
There is no one-to-one name-color-to-enemy-mode mapping.
The player branch at lines 379-416 renders a nested `else` against the
self-comparison. Original bytecode is needed before assigning the white
fallback to every non-self, non-party player.

The ordinary battle-NPC selector uses `getHateType()`: value 1 gives
`(1,1,0.5,1)` and 2 gives `(1,0.7,0.2,1)`. For another value, the actor's
party occupancy group including the viewer gives `(1,0.38,0.44,1)` and the
non-member case gives `(0.6,0.45,0.94,1)`. These conjunctions and their source
identity are owned by the work-field inventory linked above. `NpcBaseClass`
declares synced integer8 `npcWork.hateType` at lines 170-215 and returns it
from `getHateType` at lines 349-356. This is not a numeric hate-amount test.

The recovered content-group branch at judge lines 501-580 tests group kinds
30001/30006 and membership. It retains the same colors for hate types 1/2,
but renders `(1,0.5,0.7,1)` and `(0.6,0.6,0.9,1)` for its occupancy cases.
Its nested goto/else rendering has not been freshly checked against original
bytecode. Exact fallthrough through that branch remains unestablished.
No implementation should present this recovered rendering as a fully
verified universal color algorithm.

`getPlayerParty()` requests group 10001; `getParty()` falls back to 10002
when 10001 is absent (`chara/charabaseclass.lua:52-79`). Both the name judge
and ordinary party roster use the viewer's player-party group. The existing
work-field inventory owns that roster implication. Occupancy is a separate
group query. Neither establishes a Charm rule or a party-color-only flag.

## Actor-to-actor relation codes

`chara/charabaseclass_battle.lua:995-1089`, `judgeRelation`, supplies another
recovered channel. Its first result follows this order:

| First matching case | Result |
| --- | ---: |
| Either actor's Lua property 3 falsey | 4 |
| Same actor | 3 |
| Supplied party, or self party when omitted, includes other | 2 |
| Self battalion nonzero and equal to other battalion | 2 |
| Both players, self not in PvP | 2 |
| Both players, both in PvP | 1 |
| Both players, self in PvP but other not | No explicit return in the recovered branch |
| Self non-player, other player | 1 |
| Both non-player | 2 for equal battalions, otherwise 1 |
| Self player, other non-player | 2 for other battalion 1, otherwise 1 |

The base `isInPvP()` returns false at parameter lines 1786-1792. The asymmetric
PvP branch needs original bytecode before assigning any further result. The
recovered source also has no nil guard before the fallback party membership call. Result
codes are preserved as observed script outputs, not named wire enums or a
server damage policy. In particular, two non-player battalion-0 actors can
take result 2 while a player takes result 1 against such an actor. A relation
requires the source actor and party context as well as the target.

## Target HP and level

`widget/targetparameterwidget.lua:setRank`, lines 256-314, shows the target's
level only when `isPlayer() == false`, Lua property 3 is true and battalion is
not 1. A nil `isPlayer()` result does not meet that exact false comparison.
An allied opening fighter does not take that level-label branch.

HP display at lines 111-156 and 317-378 is independent. The desktop's target
getters at `widget/desktopwidget_connector.lua:12889-12955` return target
`getHPMax()` and `getHP()` or zeroes without a target. For a numeric maximum,
the widget shows HP when it is positive and hides it when non-positive.
Negative current HP is clamped to zero. The undead style can display current
equal to maximum; it does not change target mode. The decompiled nil guards
occur after numeric comparisons, so safe nil handling is not established.

`getHP/getHPMax -> getPartsHP/getPartsHPMax` selects Lua part 1 and returns
zero without `hasGameParameter()` (`chara/charabaseclass_parameter.lua:572-627`).
`getHpImpl/getHpMaxImpl` reads `charaWork.parameterSave.hp[part]` and
`hpMax[part]` (`chara/charabaseclass_ffxivbattle.lua:4-25`, SHA-256
`d1db23d11f911ea3205ca72868cc9fca22cefe0d3f88a6706cbb035d92130bda`).
Chara initialization sets `gameParameter` from its first `_onInit` argument
and binds HP/HPMax as work IDs 1010/1011 only on that truthy branch
(`chara/charabaseclass.lua:1045-1086,1118-1123`). Positive HP does not prove
enemy mode; friendly actors can follow this HP path.

## Native presentation cache fallback

The selected native color path accepts the script's choice. The registration
and conversion are owned by
`xivl-client-structs:docs/presentation-native-contract.md#nameplate-natives`:
callback `0x006E3E00` supplies actor identity, part and four floats to
`0x0075B570`. That helper resolves identity through `0x004D9970`, returns on
a null result, and uses `N = C + 0xBA0`, where C is the resolved native
CharaElement pointer at that call. It loads N's vtable slot 2 (`+0x08`).
The NamePlate vtable `0x00FCF98C`, cell `0x00FCF994`, selects `0x006A3AD0`.
This is not an offset from a LuaControl, CharaBase userdata or arbitrary
actor interface pointer.

The setter stores four 32-bit floats at `N+0x68+0x10*part`, with alpha first
and RGB following. Component labels follow the script's RGBA interpretation.
The direct native observation is the argument-to-float storage order, not a
hostile-state bit. For the script's part 1:

| Value | NamePlate-relative | Resolved C-relative |
| --- | --- | --- |
| Alpha | `+0x78` | `+0xC18` |
| Red | `+0x7C` | `+0xC1C` |
| Green | `+0x80` | `+0xC20` |
| Blue | `+0x84` | `+0xC24` |

The constructor `0x006A3C00` installs the concrete vtable at `0x006A3C47`
and initializes these four floats to 1 at `0x006A3C71..0x006A3C83`, loading
`0x3F800000` from `0x00F54F70`. Thus white can be an initial value, not a
completed classification. At part 1 the setter only caches the floats.
The tick consumer `0x006A3560` reads them at `0x006A3910..0x006A392A`,
compares the presentation cache at `N+0x98..+0xA4`, calls `0x00938020` on a
change at `0x006A39AF`, and updates that comparison cache.

A client-side reader can use this cache for the currently selected color
after the judge has run, while that C/N binding is still valid. It cannot
recover a unique friendly/hostile Boolean from it. Keep actor lifetime and
rebind state, constructor defaults, update ordering and the separately
resolved target identity as conditions. No persistent pointer validity or
atomic snapshot is established. A fixed actor displacement for the work-pool
battle property or a standalone hostile-state setter remains unestablished.

The fresh complete linear ranges used for these bounded native observations
are below. Every range had complete original PE raw backing and complete
x86-32 decoding. Hashes authenticate the selected bytes, not runtime behavior.

| VA, byte extent | Purpose | SHA-256 |
| --- | --- | --- |
| `0x0075B570`, 73 | Identity resolution and N receiver adjustment | `46d3f0265c76aeb66215bb0b239f90c78686f6c75f72ece860d50b9158ec78be` |
| `0x006A3AD0`, 146 | Float stores and part-1 return | `c0d10b7d09456dfb596df7864f8f6db8a10a1bb50a0bddcb7c8d7f9ae3f318ae` |
| `0x006A3C00`, 1500 | Vtable and initial colors | `eccdd3da449913f43ca9f3b3cd793514a9a57242a311284c5cdd85d24bd32274` |
| `0x006A3560`, 1322 | Color cache consumer | `5a53380a1cf09083a9acd03087f0c2e1d2ad15e58f0707af3565dadbbf99bf2c` |
| `0x00D2E0A0`, 208 | Bounded array-index read and child dispatch | `f7e5f801556b1c0e1fe11b34b7a3463559038961435c39df66c6852eac23ccbf` |
| `0x00D1A620`, 18 | Positive index minus one | `e12c04a3b627c1c74e6978f5cfdd9ba4c09a756e582573bca7b61fdfe61a36d3` |
| `0x00D28890`, 128 | Boolean child read | `a563811b94a1b0b9ee483924b517e544833a9ec174287f76cff9ec61f47c6bca` |

## Remaining evidence joins

Target-mode reconstruction for a supplied initialized actor is supported by
the recovered scripts and the native index conversion. Complete incoming-wire
reproduction across creation, ID reuse, reset/rebind and class changes is not
established. The owning wire contract lists those exact identity/context and
initial-state gaps. Player-fallback and content/occupancy fallthrough still need original
bytecode; occupancy packet-to-current-group-record association is also open.
None is resolved by a class-name substring, an actor-ID kind, HP presence,
the initial hate type, a Grand Company allegiance byte or the nameplate cache.
