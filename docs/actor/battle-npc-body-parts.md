# Battle NPC body-part state

The 1.23b client keeps part existence, packed breakage state, model
presentation, and combat text separate. The existence booleans initialize
Lua predicates. Opcode `0x0144` supplies a packed state word whose low byte
contains eight breakage bits. A set head bit is `0x08`, and the native model
consumer selects part-specific break and recovery schedulers. A captured
combat-result message independently supplies the displayed part name.

This evidence establishes the bit order and a captured head break. It does
not establish a selectable parts UI or an exact index assignment for all
three Lodestone `Other` categories.

## Inputs and method

Native observations use retail Windows 1.23b `ffxivgame.exe`, extraction
`2012.09.19.0001`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
preferred image base `0x00400000`. All native addresses below are VAs.
Ghidra assembly listings were checked instruction by instruction against
the original PE bytes. `pefile` 2024.8.26 and Capstone 5.0.7 in x86-32 mode
independently read the selected instructions and dispatch tables. These are
static observations, not an executed client session.

Lua observations use the authenticated decoded corpus identified by
`xivl-client-scripts:manifests/scripts.json`, extraction
`2012.09.19.0001`. Its producing tools are
`xivl-client-structs:tools/lpb_pipeline.py`, `tools/decode_lpb.py`,
`unluac.jar`, and CRLF-to-LF canonicalization. The cited Lua paths are
reproduction locators for local retail-derived inputs, not published source
bodies. Their SHA-256 values are listed below. CSV cells were inspected
directly and checked against `xivl-client-data:manifests/tables.json`.

## Initializer and existence flags

After the NPC actor-class argument, the supplied suffix partitions as:

```text
true, true, 10, 0, 1, exists[1], ..., exists[8], 0
| super |  | |  |   |------ battle block ------|  | event block count
          | |  partsName
          | aggro
          battle block count
```

`NpcBaseClass._onInit`, `chara/npc/npcbaseclass.lua:110-117`, passes the
two booleans to its superclass. Lines 224-244 read a counted battle block,
call `initForBattleCommon` after its count, then advance by count plus one.
The following zero is the event-common block count at lines 245-264.
It is not a ninth existence flag. The count `10` comprises `aggro`,
`partsName`, and the eight booleans.

`NpcBaseClass.initForBattleCommon`,
`chara/npc/npcbaseclass_battle.lua:13-70`, declares
`npcWork.battleCommon.partsExists` as `array,8,boolean`. It stores vararg 1
as `aggro`, vararg 2 as the scalar `partsName`, and vararg `2+i` as
`partsExists[i]`, for `i=1..8`. `partsName` is separate from the eight flags;
it must not be confused with a part index. The initializer makes no target
or UI registration call.

The inspected consumers are:

| Consumer | Observed use |
| --- | --- |
| `NpcBaseClass.isPartsExists`, NPC battle script lines 84-93 | Returns the indexed boolean unchanged. |
| `CharaBaseClass.isParts`, `chara/charabaseclass_battle.lua:858-883` | For an NPC, returns that boolean only when property 3 is enabled; otherwise returns false. |
| `CharaBaseClass.isBreakedParts`, battle script lines 936-953 | Returns false when `isParts(i)` is false, otherwise calls `_getSubStatBreakage(i)`. |
| `ItemBaseClass.processCanUseForTargetParts`, `item/itembaseclass_common.lua:3803-3865` | Its optional part check accepts target parts 0/1 immediately. For other parts it checks item support, target existence, and breakage, returning message IDs 32524, 32525, or 32526 on the corresponding rejection. |

The item check is conditional: base `canUseWithPartsCheck` returns false at
lines 3794-3800. An exact-name search of all 2,671 decoded scripts found no
override. Thus this code establishes a conditional action-eligibility
predicate, not an active part-selection interface. No rendered parts UI
consumer was identified in that corpus. In particular, setting an existence
flag alone is not evidence that a separate target, part HP bar, or menu
appears. Main actor targetability is not established by these flag readers.

## Part indices and Lodestone categories

The native getter proves the index-to-mask column independently of the
anatomical names. The name column has the following narrower evidence:

| Client part index | Breakage mask | Lodestone category / label | Evidence boundary |
| ---: | --- | --- | --- |
| 1 | `0x80` | `body`; outside the seven manual categories | Generic text row 1. The item part check bypasses index 1, and the model consumer forces its visual state off. |
| 2 | `0x40` | `Limbs (Right)` / `right arm` | Generic text row 2 and direction entry 2. This is a script/data correspondence, without a captured right-limb break. |
| 3 | `0x20` | `Limbs (Left)` / `left arm` | Generic text row 3 and direction entry 8. This is a script/data correspondence, without a captured left-limb break. |
| 4 | `0x10` | `Limbs (Lower)` / `legs` | Generic text row 4 and direction entry 0. This is a script/data correspondence, without a captured lower-limb break. |
| 5 | `0x08` | `Head` / `head` | Generic text row 5, direction entry 1, and captured head-part incapacitation messages paired with `0x08`. |
| 6 | `0x04` | Candidate `Other (Right)` | Unconfirmed category join. Direction entry 2 matches part 2; generic text row 6 contains a U+00D7 placeholder. |
| 7 | `0x02` | Candidate `Other (Left)` | Unconfirmed category join. Direction entry 8 matches part 3; generic text row 7 contains a U+00D7 placeholder. |
| 8 | `0x01` | Candidate `Other (Outer)` | Unconfirmed category join. Direction entry 4; generic text row 8 contains a U+00D7 placeholder. |

The generic labels are `xtx_text_partsName.csv`, row IDs 1-8, English
fields `1` and `2`. The direction arrays are
`CharaBaseClass.processGetPartsDirection`, battle script lines 886-908:
`{0,2,8,0,1,2,8,4}`, and `processGetPartsWideDirection`, lines 911-933:
`{false,true,true,false,false,false,false,false}`. The generic text rows
alone do not prove a native lookup from a part index to that row. The table
therefore preserves the stronger captured head join and the weaker
script/data correspondences separately.

The seven category names come from
`xivl-captures:studies/lodestone-manual/derived/tables/incapacitation-monster-parts.csv`
and the associated `incapacitation-weaponskills.csv`. The monster table
contains five distinct categories; the weaponskill table supplies the two
remaining right/left limb labels. The preserved source page is
`sources/lodestone-manual/objects/pages/effect-inducing-tactics.md:41-58`.
It is manual evidence, not a client index enum. The irregular custom-name
groups in `xtx_text_partsName` do not establish a numeric stride that closes
the `Other` index assignments.

## Opcode 0x0144 and exact bit order

The breakage byte is **application offset `+0x00`**. This is inner game-body
offset `+0x10`, or complete actor subpacket offset `+0x20`: the actor
subpacket header and the game-message prefix are each 16 bytes. It is not
offset zero of either header. The application is eight bytes long.

`0x0058CCA0` dispatches opcode `0x0144` through byte table
`0x0058D994` and target table `0x0058D930` to `0x0058D44B`.
That branch advances past the game prefix and calls `0x00588C50`.
At `0x00588C5A-0x00588C5C`, the latter reads application `+0x00` as a
**u32**, storing the full packed word at CharaElement `+0xF8`.
It also stores the mode byte and motion-pack word separately, then emits
an eight-byte scene event of subtype `0x3B` at `0x00588CF9-0x00588D04`.
The low byte is therefore consumed as part of a real packed-word path.

The `_getSubStatBreakage` native binding is registered by `0x0074B090`
under the string at `0x00FD7BC4`. Implementation `0x006FA020` obtains
the current packed word through `0x0075C1D0 -> 0x004D9970` and
`0x00585410`; the last helper reads CharaElement `+0xF8`.
At `0x006FA078-0x006FA0AE`, it accepts indices 1-8 and uses the eight
targets at `0x006FA0D4` to select shifts `7,6,5,4,3,2,1,0`, followed
by `& 1`. Other indices return false. Thus:

```text
isBroken(part) = (breakageByte & (1 << (8 - part))) != 0
head: part 5 -> bit 3 -> 0x08
```

This is a direct packed actor-state read. It does not require interpreting
the separate `0x0179` twenty-status-ID array as the source of these bits.
The mode-only `ChangeActorSubStatModeBorderReceiver` route remains distinct:
its existing evidence in
`xivl-client-structs:manifests/s2c_payload_decoding_lua_bound.json#payloadStructs.s2c_0x0144`
describes the Lua mode callback, not the complete native presentation path.

## Break and recovery presentation

The scene dispatcher `0x00662D30` maps subtype `0x3B` through its
`0x00663C08` / `0x00663AE8` tables to `0x006638A4`. That case reads the
packed word at `+0x00` and calls `0x007BF270` on actor `+0x1110`.
On first publication that helper applies it through `0x007BB740`.
Later publications enqueue type 2, drained through
`0x007BF2E0:0x007BF39E-0x007BF3B7` into the same apply helper.
The apply helper calls `0x007A8970` at `0x007BB7CC`.

`0x007A8970:0x007A89B0-0x007A8A2A` decodes the low byte into eight
visual lanes, in the same descending bit order for parts 2-8. Lane 1 is
explicitly false: bit `0x80` is queryable by the getter but cannot turn on
the part-1 model scheduler through this function.

| Transition | Native resource request | Observable conclusion |
| --- | --- | --- |
| A part 2-8 bit becomes set | `init_pbNN_1`, from strings `0x00FE7658-0x00FE76AC`; selection at `0x007A8B24-0x007A8C2C` | Requests that model's broken-part scheduler. |
| A previously active part bit clears | Releases its stored on handle and requests `init_pbNN_0`, strings `0x00FE75F8-0x00FE764C`; branch at `0x007A8C45-0x007A8D79` | Requests that model's recovery scheduler. |
| Required model/resource lookup is absent | Null guards at `0x007A8AEC`, `0x007A8AFC`, `0x007A8B71`, and `0x007A8CA6` | No corresponding scheduler is started on that branch. |

The routine also constructs `init_pb_%c%c_1` at
`0x007A8D9F-0x007A8DD0` from the part-6/7 states. This is an additional
combined resource request, not a separate wire field. The routine reads the
packed state and model resources, not `partsExists`. Existence flags therefore
do not gate this particular visual path. Actual mesh changes, motion, VFX,
and visibility depend on the selected model's authored resources; this native
selection is not proof that every monster visibly loses or restores geometry.
Queued application timing follows the action substatus-kick mechanism described
in [model-state transitions](model-state-color-transitions.md).

Combat text is independently specified by `worldMaster.csv`, English field
`1`: row 30112 formats the actor possessive and a `xtx/text_partsName` phrase
followed by `incapacitated.`; row 30113 uses the same inputs followed by
`no longer incapacitated.`. These are the break and recovery messages.
`xtx_status.csv` rows 223021-223028 also carry `Incapacitation` status text
and corresponding break/recovery templates. The presence of those templates
does not prove that `0x0144` alone creates a status icon, chat message, popup,
or part-selection UI. The captured break text below comes from a combat result.

## Captured break witnesses

The complete retained 54-capture corpus contains 992 reconstructed s2c
`0x0144` packets in 28 captures. Twenty-four have application byte zero
equal to `0x08`; the other 968 have zero. The small public payload-sample
product is not an exhaustive count of those packets. All 54 framing checks
reported no trailing bytes or compression-flag mismatches.

The following same-frame combat-result and state joins provide named break
witnesses. Connection, frame, and subevent ordinals are zero-based, direction
is s2c, and the lane is `main`.

| Capture / connection / frame | 0x0144 subevent / source actor | Result text ID / parameter | Resolved English part |
| --- | --- | --- | --- |
| `party_battle_leve.pcapng` / 0 / 2217 | 0-4 / `1158679843` | 30112 / 45 | maw |
| `party_battle_leve.pcapng` / 0 / 2406 | 0 / `1158679894` | 30112 / 42 | horns |
| `war_quest_update2.pcapng` / 0 / 362 | 0-4 / `1163947557` | 30112 / 46 | mandibles |
| `war_quest_update2.pcapng` / 0 / 546 | 0 / `1163947556` | 30112 / 46 | mandibles |

In each join the result target equals the state source. The application starts
at inflated subevent `+0x20`; byte zero is `08`. The first row's full
application is `0800400000000000`; the others are `0800000000000000`.
Text parameters are resolved by `xtx_text_partsName` rows 45, 42, and 46,
not treated as part indices. Source locators are
`xivl-captures:studies/battle-result-backfit/derived/battle-result-rows.csv`
physical lines 293, 327, 567, and 601, and the original capture frame/subevent
coordinates above. This is recorded wire and message evidence, not a video
observation of the rendered result.

The first witness has a preceding zero state at frame 1990, subevent 12,
then `0x08` at frame 2217, subevents 0-4. Its `0x013A` result is frame
2217, subevent 8, with source `43723073` and target `1158679843`.
The two party-capture joins match both actor directions. The war-capture
result source is `43722707`, while the state target is `43723073`;
only result target versus state source matches in those two joins.

| Capture | SHA-256 |
| --- | --- |
| `party_battle_leve.pcapng` | `6327e5e1f5cbd51a9baaa9bcbacf53ca51c50a98fe4b66ae3e6bdecd9198089f` |
| `war_quest_update2.pcapng` | `bf43b5ee32c58e5dc6f8149b071bd9c5cd5cc0aaa55ea3528872308fd7be2429` |

No later same-source `0x0144` zero was found after those nonzero groups in
the retained corpus. Recovery resource selection and the recovery text
template are statically established; a captured recovery transition remains
unverified. Repeated `0x08` packets are repeated state publications, not
proof of repeated new incapacitations.
The complete battle-result product contains four 30112 messages and no
30113 recovery message across 531 retained result-family packets and 622
decoded rows, as recorded by the study's `derived/accounting.json` and
`derived/battle-result-rows.csv`.

## Reproduction and remaining limits

Use the pinned executable with Ghidra's assembly view or Capstone to inspect
the native VAs above. Read the two dispatch tables as data, not instructions.
Check all eight entries at `0x006FA0D4` and their shifts; checking only the
head case cannot establish bit order. Original-file comparison authenticated
2,657 listed instructions across the eight native reader/presentation
functions and both dispatch tables. This authenticates the selected evidence,
not the whole imported analysis database.

For the capture join, use `xivl-captures:tools/extractors/extract_streams.py`
(`reconstruct_lanes`, `parse_outer_frames`, `maybe_inflate`) and
`tools/validate_framing.py`. The battle-result interpretation is owned by
`studies/battle-result-backfit/README.md` and its producer. Its
`world-master-messages.csv` row 30112 is the localized-text join.

| Authenticated decoded Lua input | SHA-256 |
| --- | --- |
| `chara/npc/npcbaseclass.lua` | `f18c6c3198db0402431ca6aa683b601afbf5a0dc5c22481b3c1da1deabef0dc2` |
| `chara/npc/npcbaseclass_battle.lua` | `e277df19d98ce7abf062e9345fa5b73e051fe8085d15b07780d26d5e0470fbf0` |
| `chara/charabaseclass_battle.lua` | `ca7d9bd52320e8aa62ecd853a0ce8d9e2e0ddc76fc504729b501b14b9bfcd959` |
| `item/itembaseclass_common.lua` | `7620492917230c3e060ed84d3197bf164c234a3ab73e48a0939bc65107f6b7fa` |

| Inspected CSV input | SHA-256 |
| --- | --- |
| `xtx_text_partsName.csv` | `1ddf82e711696002aff83943a13fd93b03a5884e2a5ae8db8160fbed3a6d7f59` |
| `worldMaster.csv` | `2e2e2dd5cd9651388f6fa575b4229081ad0452165e78b725df0a5b5b7cf7c643` |
| `xtx_status.csv` | `60eca3b76d1d9799589d9059adeb1e7c3d142e07609a1ad0a053c5e75d9b96ee` |

Unknowns remain the native index-to-name lookup for the generic labels,
conclusive `Other (Right/Left/Outer)` index assignments, an active rendered
parts-selection UI consumer, model-specific visible break/recovery effects,
and an observed recovery. Neither the initial flags nor a broken bit proves
server damage thresholds, incapacitation duration, loot rules, or action
selection. No server implementation is part of this evidence.
