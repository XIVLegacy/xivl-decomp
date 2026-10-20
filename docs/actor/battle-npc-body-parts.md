# Battle NPC body-part state

The 1.23b client keeps part existence, packed breakage state, model
presentation, and combat text separate. The existence booleans initialize
Lua predicates. Opcode `0x0144` supplies a packed state word whose low byte
contains eight breakage bits. A set head bit is `0x08`, and the native model
consumer selects part-specific break and recovery schedulers. A captured
combat-result message independently supplies the displayed part name.

The result parameter selects a localized part-name row. It does not alter
the breakage bits or establish the meaning of the init `partsName` scalar.
Recovery state must reach the serial queue before the result/action meant
to consume it. A recovery message alone does not restore a broken part.

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

### What the init `partsName` scalar establishes

**Answer:** it is one `integer32` value per actor, stored and returned
unchanged. An active table lookup, monster-name-group interpretation, or
per-part use of that scalar is not established.

The authenticated NPC battle script declares and assigns the scalar at
lines 19-22 and 48-54. `NpcBaseClass.getPartsName`, lines 73-81, takes no
part argument and returns `npcWork.battleCommon.partsName` directly.
`CharaBaseClass.getPartsNameID`, battle script lines 1787-1811, returns
`1` for a player, that NPC getter's result when property 3 is enabled,
and nil otherwise. Neither getter indexes a table, computes a stride,
or combines the scalar with a part index.

A hash-checked exact-identifier search of all 2,671 decoded Lua files found
only the scalar declaration, assignment and read, those two getter
definitions, and the call from `getPartsNameID` to `getPartsName`.
There was no caller of `getPartsNameID`. An original-PE byte search also
found none of the ASCII or UTF-16LE spellings `partsName`, `getPartsName`,
or `getPartsNameID`. These are bounded negative searches: they do not
exclude computed Lua access, hashed native lookups, or another consumer
outside the decoded corpus. They support stored/getter-only use in the
inspected Lua, not a claim that the value is globally unused.

The result message's part-name row is a separate byte supplied by the
server, traced below. No inspected edge derives that byte from the init
scalar. In particular, neither the getter's `ID` suffix nor custom names
in `xtx_text_partsName` proves that the init scalar is a row ID or a name
group. That interpretation remains unproven.

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
| 6 | `0x04` | Supported candidate `Other (Right)` | Native `init_pb06` lane; m021 motion group `701_part` has keys corresponding to right-side bones if the row-list base is zero. The base remains unjoined. |
| 7 | `0x02` | Supported candidate `Other (Left)` | Native `init_pb07` lane; m021 motion group `702_part` has keys corresponding to left-side bones under the same condition. |
| 8 | `0x01` | Supported candidate `Other (Outer)` | Native `init_pb08` lane; m013 has a direct `init_pb08_1` model-state resource. Exact shell mesh ownership and a selective tail/ore bone join remain unproven. |

The generic labels are `xtx_text_partsName.csv`, row IDs 1-8, English
fields `1` and `2`. The direction arrays are
`CharaBaseClass.processGetPartsDirection`, battle script lines 886-908:
`{0,2,8,0,1,2,8,4}`, and `processGetPartsWideDirection`, lines 911-933:
`{false,true,true,false,false,false,false,false}`. The generic text rows
alone do not prove a native lookup from a part index to that row. The table
therefore preserves the stronger captured head join and the weaker
script/data correspondences separately. Generic rows 6-8 contain U+00D7
placeholders rather than anatomical names.

The seven category names come from
`xivl-captures:studies/lodestone-manual/derived/tables/incapacitation-monster-parts.csv`
and the associated `incapacitation-weaponskills.csv`. The monster table
contains five distinct categories; the weaponskill table supplies the two
remaining right/left limb labels. The preserved source page is
`sources/lodestone-manual/objects/pages/effect-inducing-tactics.md:41-58`.
It is manual evidence, not a client index enum. The irregular custom-name
groups in `xtx_text_partsName` do not establish a numeric stride that closes
the `Other` index assignments.

### Model-resource witnesses and the bone-mapping limit

**Answer:** indices 6, 7 and 8 are proven to select the `init_pb06`,
`init_pb07` and `init_pb08` scheduler lanes. The authored resources support
the proposed Right, Left and Outer assignments. They do not yet prove an
unconditional runtime bone/mesh mapping for every named example. None of
the three category candidates is refuted by the inspected resources.

The resource observations below use the same retail extraction as the
executable. Python `struct` readers inspected SEDBRES directories, SCB
resource tables, MTB groups/channels and SKL records; `hashlib` and
PowerShell `Get-FileHash` checked identities. All offsets in this subsection
are **file offsets within the named resource**, not executable addresses.
MTB framing follows [the container finding](../resource/mtb-container-framing.md)
and [the motion-blob finding](../resource/mtb-fixed-motion-calculator.md).
SKL primary-name and parent records follow
[skeleton framing](../resource/skeleton-bone-framing.md).

The model identities come from raw CSV joins authenticated against
`xivl-client-data:manifests/tables.json`, using Python `zipfile`, `csv`
and `hashlib`. The graphic row supplies the model base in field `6` and
joins the same-ID actor-class row; that row's field `5` references the
display row's English field `1`. Column interpretation follows
`xivl-client-data:docs/actor-appearance-crosswalk.md:36-57` and
`tools/build_actor_appearance_census.py:103-142`. The native
[base-model split and directory formatter](appearance-path-decoder.md#base-model-split)
map these bases to the listed `mNNN` resources.

| Model | Graphic base | Graphic / actor-class row ID | Display-name row ID | English name |
| --- | ---: | ---: | ---: | --- |
| `m021` | 10021 | 2101311 | 3101311 | salamander |
| `m013` | 10013 | 2107615 | 3107613 | crab |
| `m029` | 10029 | 2101705 | 3101705 | ahriman |
| `m033` | 10033 | 2102125 | 3102125 | coblyn |

The preserved manual's monster-parts table names salamander's Other Right
and Left parts as wart clusters, and the Other Outer parts as crab shell,
ahriman tail and coblyn ore cluster. Those labels supply the anatomical
candidates; the table joins above establish model identity, not mesh or
bone ownership.

| Model resource under `client/chara/mon/` | Bytes | SHA-256 |
| --- | ---: | --- |
| `m021/act/emp_emp/bid/base/0000` | 650,096 | `c840da85fcdd0e84a3ad49a0db87f26532e0f83495f9d6c3a57d274f7764ee13` |
| `m021/skl/0001` | 27,792 | `ef40c8bd75be79d4fec0f631f6f329b562438b27bd3d6361532b1866d065e2cd` |
| `m013/act/emp_emp/bid/base/0000` | 313,408 | `1d53e0812754c8ad5eba870da822b536b774777e738634e18f97338b0fb68ed7` |
| `m013/skl/0001` | 15,856 | `c3f5e8fcc30a1f439f143e6c60d8fa9ab068b0c99e536971cbebd15fe10c885a` |
| `m029/act/emp_emp/bid/base/0000` | 825,008 | `54106d7a46b1fdf14e975354a50255e2d9a3a149bb9681e0c0ba854e3068e8bb` |
| `m029/skl/0001` | 30,160 | `0feb28c9115fd473e62eddec45558a95b066394a68b0bafc3aeb8b7f40446796` |
| `m033/act/emp_emp/bid/base/0000` | 417,424 | `4a7b2b1af0ae10642d6f3e73d042c9e00595aa77312a9b5172171df6c95086a6` |
| `m033/skl/0001` | 22,064 | `4cb4b33f7c227460c7551e8b34b56010d3ead002aadfb70fef34054838614624` |

The m021 BID's nested RES at `0x97F14` has `init_pb06_1` as entry 12,
SCB `0x9BC84` of size `0x4C0`, and `init_pb07_1` as entry 13,
SCB `0x9C144` of size `0x4C0`. Their RIDTBL markers at `0x9BF74` and
`0x9C434` precede tables naming `cbbp_1_pb06` and `cbbp_2_pb07`,
respectively, plus `skl_m021b001`. Typed `MotionClip` records at
`0x9BEF8` and `0x9C3B8`, each `0x2C` bytes, have body `+0x08` equal
to 1. That field selects RIDT index 1, the respective named `bcm`
resource; index 2 repeats its name with type `btm`. This is a typed clip
reference, rather than name co-occurrence. The outer RES entries 91 and 95
contain the corresponding MTBs at `0x89F28` and `0x8AC60`, each `0x2A9`
bytes.

| MTB | Only nonempty part pair | Serialized joint keys | Corresponding m021 SKL ordinals, conditional on base zero |
| --- | --- | --- | --- |
| `cbbp_1_pb06` | Pair 2, `701_part` | `64,66,68,70,73,75,77,79` | `j_tuno_a_r` through `j_tuno_d_r`, and their `n_tuno_*_r` children |
| `cbbp_2_pb07` | Pair 3, `702_part` | `63,65,67,69,72,74,76,78` | `j_tuno_a_l` through `j_tuno_d_l`, and their `n_tuno_*_l` children |

The m021 SKL block starts at `0x1C0`; its 94 records start at `0x210`
with stride `0xB0`. For example, right `j_tuno_a_r` is ordinal 64 at
`0x2E10`, and left `j_tuno_a_l` is ordinal 63 at `0x2D60`.
This repeated right/left correspondence is stronger evidence than the
direction array, but a motion joint key is not automatically a global
SKL ordinal. The native row-list builder at `0x00A39193-0x00A391B9`
asks the container's virtual `+0x20` method for a name, then calls
`0x00A3B440`. That helper returns the first equal-name node's index plus
the key, or `0xFFFF` on the observed miss paths. The
[row-list contract](../resource/mtb-fixed-motion-calculator.md#row-index-list)
owns that arithmetic. The selected base node, constructed skeleton and
actual row list have not been joined here; the bone correspondence above
retains that condition.

For m013, nested RES entry 12 selects `init_pb08_1`, SCB `0x49918`
of size `0x4F0`. Its RIDTBL marker at `0x49C38` precedes the table naming
`cbxs_st1` and `m013e001`. Typed `MotionClip` record `0x49B8C` has
body `+0x08` equal to 1, selecting RIDT index 1, `cbxs_st1` of type
`bcm`, with the same-name `btm` at index 2. This is a direct index-8
init/model-state witness.
The inspected SKL has no shell-named primary record. Identifying the
changed shell mesh or material remains a separate requirement.

The m029 nested entry 15 selects `pb08_1`, SCB `0xC7750` of size
`0x4E0`, whose RIDTBL marker at `0xC7A60` precedes the table naming
`cbbm_pb08_dmg` and `skl_m029b001`. Typed `MotionClip` record
`0xC79EC`, body `+0x08` equal to 1, selects that `bcm` at RIDT index 1,
with its same-name `btm` at index 2. The associated MTB at `0x72108`
includes keys matching SKL tail-chain ordinals 31, 39, 45, 49, 53, 57,
61, 70, 83, 104 and 111
under the same base condition. The names run from `j_sippo_a` to
`j_sippo_tip_b`. This is a full-body damage motion, not tail-exclusive
evidence, and `pb08_1` is not the native `init_pb08_1` request.

Likewise m033 has `pb08_1`, SCB `0x63E6C` of size `0x540`, and a
RIDTBL marker at `0x6419C` preceding the table naming `cbbm_pb08_dmg`
and `skl_m033b001`. Typed `MotionClip` record `0x64118`, body `+0x08`
equal to 1, selects that `bcm` at RIDT index 1, again paired with its
same-name `btm` at index 2.
SKL ordinals 3 and 4, records `0x420` and `0x4D0`, are `n_toge_a`
and `n_toge_b`, children of `j_karada`. A spike/attachment name alone
does not establish ore-cluster mesh ownership or an init-lane bone target.
The tail and ore examples therefore remain corroborating candidates,
not independent proof of selective removal or restoration.

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
the part-1 model scheduler through this function. Setting it still stores
the full packed word and can enter the apply routine; it does not request
`init_pb01_1`. The routine can still request the combined part-6/7 resource
described below, so this is a part-1 lane conclusion rather than proof
that no presentation work occurs anywhere.

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

### Result consumers and the part-name parameter

**Answer:** 30112 and 30113 take the generic combat-log path. Their message
IDs and part-name parameter do not set or clear the breakage word. A result
can also cause ordinary action processing to consume already queued state;
that is distinct from interpreting 30113 as a restore command.

Ghidra assembly listings, authenticated against the original executable
and decoded with Capstone, establish the following `0x013A` path. Offsets
in this table are relative to the application or the staged result record,
as specified, and `i` is a zero-based target row.

| Native locator | Observation |
| --- | --- |
| `0x0058CCA0` tables `0x0058D994` / `0x0058D930`, case `0x0058D565-0x0058D56D` | Opcode `0x013A` calls `0x0058C930`, which calls `0x005873E0` and enqueues the staged record at CharaElement `+0xA80`. |
| `0x005873E0:0x00587430-0x0058745B` | Transposes message ID at application `+0x64+2*i` to record `+0x44+0x14*i`, and the part-name byte at application `+0xA0+i` to record `+0x46+0x14*i`. Application `+0xAA+i` is a separate ordinal/filter byte. |
| `0x0058DA10 -> 0x0058CA80 -> 0x0058C220` | Drains the staged result through generic action/result processing. `0x0058C220:0x0058C3A2-0x0058C3D5` sends both 30112 and 30113 to the default branch at `0x0058C5A8`; neither selects a special state-changing case. |
| `0x0058C5A8-0x0058C60B -> 0x005759A0` | Passes the message ID and zero-extended part-name byte to the logging helper. The init `partsName` scalar is not an argument on this inspected edge. |
| `0x005759A0:0x00575B7B-0x00575BED`; `0x007705A0:0x007709C5-0x007709EC` | Forwards the part-name byte as the final integer in the message-argument context. No part-index conversion occurs on this edge. |
| `0x00575BF7-0x00575C1C -> 0x00771F50`; `0x00789CD0`, vtable `0x00FE046C` | Starts the generic logger's source-display-name resolver. Its callback `0x00789F40` continues through `0x007722A0 -> 0x00771C20` for text preparation and delivery. |

The listener's RTTI type-name string at `0x012C3E28` is
`.?AVSourceDisplayNameResolverListener@BehaviorLogger@Client@Script@Lua@Application@@`.

Both CSV templates contain the same `xtx/text_partsName` sheet expression
ending in `e80d0203`. The supplied byte selects the localized phrase row:
the captured values 45, 42 and 46 resolve to maw, horns and mandibles.
It is a text-table row selector, not index 1-8 and not the initializer's
per-actor scalar. The native parameter flow and the captured joins agree.

This establishes the selected combat-log route, not every use of those
numbers elsewhere. Other fields in a result can request action presentation.
There is no recovery capture or live rendering observation proving which
action/drain accompanied retail message 30113.

### Recovery publication and ordering

**Answer:** the part's `0x0144` bit must clear for this break/recovery
mechanism to select its recovery scheduler. Message 30113 is optional for
the visual state and required only to request that recovery log text.
A later state publication can remain queued, so clearing a bit is not an
unconditional promise of an immediate rendered restore. `0x0144` replaces
the packed word, rather than applying a bit delta; the publisher must retain
the other intended state fields when clearing the affected mask.

The ordering constraint is visible in the original executable:

1. `0x00588C50:0x00588CC4-0x00588D04` increments the shared serial at
   `0x01338D2C`, publishes it as scene subtype `0x39`, then publishes the
   packed state as subtype `0x3B`. Subtype `0x39` dispatches through
   `0x00663860 -> 0x007B43B0`; instruction `0x007B43B7` stores the serial
   at actor component `+0x54`.
2. After first publication, `0x007BF270:0x007BF27C-0x007BF29B` enqueues
   type 2 with that serial and the new packed word. The first-publication
   path at `0x007BF2A7-0x007BF2BD` instead applies the word directly.
3. `0x00587210:0x00587214-0x00587262` assigns a separately incremented
   serial to the result record. A drain in `0x007BF2E0` requires a positive
   supplied serial and stops at `0x007BF332-0x007BF338` if the next queued
   serial is newer. An admitted type 2 reaches `0x007BB740` at
   `0x007BF39E-0x007BF3B7`, then the off-scheduler branch described above.

Consequently, on the same ordered receive/scene route, publish the clear
**before the result/action intended to drain it**. If a result carrying
30113 is used for that purpose, send the
`0x0144` clear first, then that result. Sending the clear afterward assigns
it a newer serial that the earlier result cannot consume on this path.
This is a client-derived ordering requirement, not an observed recovery
packet sequence.

| Server publication | Consequence on the traced path |
| --- | --- |
| Clear bit only | Updates logical state and supplies the visual recovery state; a later update may await an eligible drain. |
| 30113 only, bit still set | Requests recovery text; supplies no cleared part state and cannot independently restore the part. |
| Clear, then a result carrying 30113 | Supplies both recovery state and text in a serial order that permits that result's processing to drain the state. Actual application still requires the relevant action/drain and model resources. |

The drain is not conditional on the text ID being 30113.
`0x0065A860:0x0065A88A-0x0065A8BA` passes a result's serial to
`0x007BF2E0` for its target actors and source actor.
The action fallback `0x00845FBD -> 0x0065A960 -> 0x0065A860` is one
concrete caller; action substatus-kick timing is another path discussed in
[model-state transitions](model-state-color-transitions.md).
These conditions prevent treating either a bare log message or the arrival
of a clear packet as proof of completed visual restoration. The exact
retail recovery action, timing and visible model effect remain unproven.

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
head case cannot establish bit order. Original-file comparisons authenticated
the selected reader, presentation, result-logging and serial-drain
instructions, dispatch tables and all numbered on/off scheduler strings.
This authenticates the selected evidence, not the whole imported analysis
database. Python's `hashlib`, `zipfile`, `csv` and exact-identifier searches
checked the cited Lua and CSV bytes; every searched Lua input matched its
manifest size and SHA-256.

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
| `actorclass_graphic.csv` | `7da8241400530885e0a28ded04a03acf2771b0580a79c1f49f46ee0861010611` |
| `actorclass.csv` | `3ac9f8d1812d49101f367e2a41356be96b5d64b1fc5ca29949195f50ebe1d984` |
| `xtx_displayName.csv` | `36c5dfba312695d9f4ee090dba123dfd7792feba55d88b0062c070d98abc0505` |

Unknowns remain the native index-to-name lookup for the generic labels,
conclusive `Other (Right/Left/Outer)` index assignments, an active rendered
parts-selection UI consumer, a table/group interpretation or active lookup
of the init scalar, model-specific visible break/recovery effects, and an
observed recovery. Neither the initial flags nor a broken bit proves
server damage thresholds, incapacitation duration, loot rules, or action
selection. No server implementation is part of this evidence.
