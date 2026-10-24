# Seven-job action grant evidence

The retail 1.23b client declares all 35 seven-job actions as rewards of
specific job quests. Each command ID occurs in one same-key
`quest_new_reward` row and in that quest script's explicit ability, spell or
weaponskill reward presentation. None of these assignments was inferred from
action level, quest order or action-list order.

The exact server update or step that first acquired each action remains
**UNPROVEN**. The retained captures supply 73 true-setting records: 72 in
initialization publications and one Vengeance record in quest-111201-associated
traffic. That record establishes a true-setting request, without a prior
false state or a causal producer link. No action is established as unlocked
by job level alone.

## Action and quest table

`STATIC` below means a client-declared quest reward. It does not mean an
observed server grant. A named method is a reward-presentation method,
not an authenticated server quest step. `UNPROVEN` applies to the actual
grant and its sending step even where the static reward assignment is exact.

Evidence abbreviations use these stable sources:

- `Q<id>`: `xivl-client-data:csv/quest_new_reward.csv`, row `<id>`,
  column 79 = -4 and column 81 = the command ID. This is reward slot 7,
  using the widget's 13-column stride. Columns are sheet labels after the
  row-ID column, not raw CSV positions.
- `S/<path>:line`: `xivl-client-scripts:lua/scripts/quest/scenario/<path>`,
  the explicit `showGetJobAbilityWidget` call. Parenthesized `pN:PC`
  means original Lua `root/protoN`, zero-based PC, reproduced by
  `xivl-client-scripts:data/quest_contracts.json` and the pinned original LPB.
- `n` and the hash: the native additional-acquisition index and
  `Murmur2("charaWork.additionalCommandAcquired[n]", 0)`, as bound in
  [acquisition state](job-action-acquisition.md#named-acquisition-registrations).
- `I`: the twelve initialization publications listed below. All six flags
  in each publication are true. `V`: canonical property record 9303 and its
  quest-associated context below. `absent`: no record for that exact hash in
  the retained complete admitted game frames, not a false acquisition bit.

| Action | Command ID | Grant mechanism | Quest ID / step or level | Evidence locator |
| --- | ---: | --- | --- | --- |
| PLD Cover | 27146 | Quest reward (STATIC); grant **UNPROVEN** | `111281`; `processEventKokuti`; sending step **UNPROVEN** | `Q111281`; `S/pld/pld0j1.lua:639` (p8:19); n=0, `0x0CCA47C4`; I: 12 true records |
| PLD Divine Veil | 27147 | Quest reward (STATIC); grant **UNPROVEN** | `111282`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111282`; `S/pld/pld0j2.lua:291` (p5:4); n=1, `0x4E5D477A`; absent |
| PLD Hallowed Ground | 27148 | Quest reward (STATIC); grant **UNPROVEN** | `111286`; `processEventKokuti`; sending step **UNPROVEN** | `Q111286`; `S/pld/pld0j6.lua:429` (p5:12); n=2, `0xE16FCC00`; absent |
| PLD Holy Succor | 27149 | Quest reward (STATIC); grant **UNPROVEN** | `111283`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111283`; `S/pld/pld0j3.lua:261` (p5:4); n=3, `0x7BDFE876`; absent |
| PLD Spirits Within | 27159 | Quest reward (STATIC); grant **UNPROVEN** | `111285`; `processEventClear`; sending step **UNPROVEN** | `Q111285`; `S/pld/pld0j5.lua:207` (p5:18); n=4, `0x75BF606D`; absent |
| WAR Vengeance | 27186 | Quest reward (STATIC), true-setting OBSERVED; grant **UNPROVEN** | `111201`; `processEventJob`; sending step **UNPROVEN** | `Q111201`; `S/war/war0j1.lua:452` (p7:19); n=5, `0xFCA1FFCD`; V: record 9303 |
| WAR Antagonize | 27187 | Quest reward (STATIC); grant **UNPROVEN** | `111202`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111202`; `S/war/war0j2.lua:264` (p5:4); n=6, `0x3C7DE08B`; absent |
| WAR Collusion | 27188 | Quest reward (STATIC); grant **UNPROVEN** | `111203`; `processEventKokuti`; sending step **UNPROVEN** | `Q111203`; `S/war/war0j3.lua:345` (p6:12); n=7, `0xBC09B0C6`; absent |
| WAR Mighty Strikes | 27189 | Quest reward (STATIC); grant **UNPROVEN** | `111206`; `processEventClear`; sending step **UNPROVEN** | `Q111206`; `S/war/war0j6.lua:270` (p7:12); n=8, `0xFFB36443`; absent |
| WAR Steel Cyclone | 27192 | Quest reward (STATIC); grant **UNPROVEN** | `111205`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111205`; `S/war/war0j5.lua:230` (p4:4); n=9, `0x5D9DD75F`; absent |
| MNK Hundred Fists | 27106 | Quest reward (STATIC); grant **UNPROVEN** | `111226`; `processEventClear`; sending step **UNPROVEN** | `Q111226`; `S/mnk/mnk0j6.lua:626` (p11:12); n=10, `0x82B7D2C7`; absent |
| MNK Spinning Heel | 27107 | Quest reward (STATIC); grant **UNPROVEN** | `111222`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111222`; `S/mnk/mnk0j2.lua:407` (p7:4); n=11, `0xAE830FCA`; absent |
| MNK Shoulder Tackle | 27108 | Quest reward (STATIC); grant **UNPROVEN** | `111221`; `processEvent010_3_system`; sending step **UNPROVEN** | `Q111221`; `S/mnk/mnk0j1.lua:408` (p7:12); n=12, `0xB3663189`; I: 12 true records |
| MNK Fists of Wind | 27109 | Quest reward (STATIC); grant **UNPROVEN** | `111223`; `processEventAfget`; sending step **UNPROVEN** | `Q111223`; `S/mnk/mnk0j3.lua:505` (p10:12); n=13, `0xF83AC36B`; absent |
| MNK Dragon Kick | 27118 | Quest reward (STATIC); grant **UNPROVEN** | `111224`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111224`; `S/mnk/mnk0j4.lua:431` (p7:4); n=14, `0x4F853F31`; absent |
| DRG Jump | 27266 | Quest reward (STATIC); grant **UNPROVEN** | `111321`; `processEventKokuti`; sending step **UNPROVEN** | `Q111321`; `S/drg/drg0j1.lua:582` (p6:20); n=15, `0x35691E39`; absent |
| DRG Elusive Jump | 27267 | Quest reward (STATIC); grant **UNPROVEN** | `111323`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111323`; `S/drg/drg0j3.lua:317` (p5:4); n=16, `0xF7FCAE68`; absent |
| DRG Dragonfire Dive | 27268 | Quest reward (STATIC); grant **UNPROVEN** | `111326`; `processEvent030`; sending step **UNPROVEN** | `Q111326`; `S/drg/drg0j6.lua:267` (p8:34); n=17, `0x5A980359`; absent |
| DRG Disembowel | 27272 | Quest reward (STATIC); grant **UNPROVEN** | `111322`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111322`; `S/drg/drg0j2.lua:287` (p5:4); n=18, `0x1E5786F6`; absent |
| DRG Ring of Talons | 27277 | Quest reward (STATIC); grant **UNPROVEN** | `111325`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111325`; `S/drg/drg0j5.lua:327` (p4:4); n=19, `0x590E1EA0`; absent |
| BRD Battle Voice | 27227 | Quest reward (STATIC); grant **UNPROVEN** | `111306`; `processEventClear`; sending step **UNPROVEN** | `Q111306`; `S/brd/brd0j6.lua:456` (p6:12); n=20, `0xDCD04C7B`; absent |
| BRD Rain of Death | 27232 | Quest reward (STATIC); grant **UNPROVEN** | `111304`; `processEventClear01`, `processEventClear02`; sending step **UNPROVEN** | `Q111304`; `S/brd/brd0j4.lua:367` (p7:12); `S/brd/brd0j4.lua:394` (p8:12); n=21, `0xAD3D8C7F`; absent |
| BRD Ballad of Magi | 27237 | Quest reward (STATIC); grant **UNPROVEN** | `111301`; `processEventJob`; sending step **UNPROVEN** | `Q111301`; `S/brd/brd0j1.lua:715` (p8:19); n=22, `0x95BCF98A`; I: 12 true records |
| BRD Paeon of War | 27238 | Quest reward (STATIC); grant **UNPROVEN** | `111303`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111303`; `S/brd/brd0j3.lua:337` (p5:4); n=23, `0xB58E127E`; absent |
| BRD Minuet of Rigor | 27239 | Quest reward (STATIC); grant **UNPROVEN** | `111302`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111302`; `S/brd/brd0j2.lua:381` (p5:4); n=24, `0x02881BF0`; absent |
| WHM Presence of Mind | 27344 | Quest reward (STATIC); grant **UNPROVEN** | `111241`; `processEventKokuti`; sending step **UNPROVEN** | `Q111241`; `S/whm/whm0j1.lua:536` (p14:4); n=25, `0xFDD1F6DE`; I: 12 true records |
| WHM Benediction | 27345 | Quest reward (STATIC); grant **UNPROVEN** | `111246`; `processEventClear`, `processEventAfget`; sending step **UNPROVEN** | `Q111246`; `S/whm/whm0j6.lua:615` (p13:101); `S/whm/whm0j6.lua:679` (p14:7); n=26, `0xF9473A8B`; absent |
| WHM Esuna | 27357 | Quest reward (STATIC); grant **UNPROVEN** | `111243`; `processEvent005`; sending step **UNPROVEN** | `Q111243`; `S/whm/whm0j3.lua:366` (p8:51); n=27, `0x6EFF35E3`; absent |
| WHM Regen | 27358 | Quest reward (STATIC); grant **UNPROVEN** | `111242`; `processEvent005`, `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111242`; `S/whm/whm0j2.lua:424` (p8:78); `S/whm/whm0j2.lua:532` (p12:4); n=28, `0xDC4F03F9`; I: 12 true records |
| WHM Holy | 27359 | Quest reward (STATIC); grant **UNPROVEN** | `111244`; `processEventClear`; sending step **UNPROVEN** | `Q111244`; `S/whm/whm0j4.lua:665` (p13:158); n=29, `0xDCCE515E`; absent |
| BLM Convert | 27305 | Quest reward (STATIC); grant **UNPROVEN** | `111261`; `processEventClear`; sending step **UNPROVEN** | `Q111261`; `S/blm/blm0j1.lua:568` (p14:25); n=30, `0x41991FCA`; I: 12 true records |
| BLM Burst | 27316 | Quest reward (STATIC); grant **UNPROVEN** | `111266`; `processEventAfget`; sending step **UNPROVEN** | `Q111266`; `S/blm/blm0j6.lua:900` (p21:12); n=31, `0x6992E8EB`; absent |
| BLM Sleepga | 27317 | Quest reward (STATIC); grant **UNPROVEN** | `111264`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111264`; `S/blm/blm0j4.lua:412` (p10:4); n=32, `0x94278166`; absent |
| BLM Flare | 27318 | Quest reward (STATIC); grant **UNPROVEN** | `111263`; `processEventClear`; sending step **UNPROVEN** | `Q111263`; `S/blm/blm0j3.lua:633` (p13:7); n=33, `0x6AB11EE3`; absent |
| BLM Freeze | 27319 | Quest reward (STATIC); grant **UNPROVEN** | `111262`; `onJobQuestCompleteSecond`; sending step **UNPROVEN** | `Q111262`; `S/blm/blm0j2.lua:324` (p8:4); n=34, `0x00890531`; absent |

## Static derivation and reward text

`xivl-client-data:manifests/staticactor_class_paths.json` binds the 42
quest IDs to their `/Quest/Scenario/<job>/<class>` paths. The original
`client/script/rq9q1797qvs.san` reproduces those records.
`xivl-client-scripts:lua/scripts/quest/questbaseclass.lua:7-14` implements
`getQuestId()` by returning `_getStaticActorID()`. These joins establish the
quest owner independently of filename suffixes or selector-message literals.
They also preserve the shipped `Drg0j4` selector-message anomaly without
using it to change the owner to a Bard quest.

All 35 command IDs occur exactly once in the entire pinned
`quest_new_reward.csv`, at column 81. Their row IDs agree with all 38 literal
reward calls in the 42 original job-quest LPBs. The duplicate presentations
are Rain of Death in `Brd0j4`, Regen in `Whm0j2`, and Benediction in `Whm0j6`.
No branch is selected as the historical path solely because another
presentation of the same reward exists.

The remaining chain rows `111204`, `111225`, `111245`, `111265`, `111284`,
`111305` and `111324` have no type -4 command reward in slot 7 and no such
literal reward call. This is an inspected-data result, not a rule assigning
the missing reward from quest sequence. The older `quest_reward.csv`
contains none of the 35 exact command IDs.

`xivl-client-scripts:lua/scripts/widget/ask/questrewardwidget.lua:283-305`
computes base `13 * (slot - 1)` and reads reward type at base+1.
Its type -4 branch at lines 522-539 reads base+3 and passes that value to
text key 5053. Thus slot 7 supplies type column 79 and command column 81.
This widget renders a reward; it does not set an acquisition bit.

`xivl-client-scripts:lua/scripts/quest/questbaseclass_common.lua:1085-1151`
selects worldMaster text 33923, 33924 or 33925 from the call's kind argument
1, 2 or other. It passes the literal action ID to the information widget
and optional notification. `xivl-client-data:csv/worldMaster.csv`, rows
33923-33925, English column 1, contains the dynamic ability, spell and
weaponskill learning notices. Their `xtx/command` lookup names the supplied
action; `xivl-client-data:csv/xtx_command.csv`, column 2, supplies the English
names in the table. The helper's scheduler and presentation calls do not
write either acquired-action array.

## Complete capture record locators

The census independently reconstructed all 54 source-manifest captures,
including the admitted game-port lanes in `login.pcapng`. It reconciled all
2,014 s2c `0x0137` packets and 9,307 canonical property records against raw
frames, without using the producer's property parser. TLS and lobby traffic
were excluded by the canonical game-lane selector. Absence statements cover
complete admitted frames only. Raw TCP reconstruction retained 228 trailing
s2c bytes (six bytes in each of 38 captures) outside those complete frames;
they were not interpreted as records. Framing validation passed its admitted
frames, rather than establishing that these tails contained no further data.

The exact matches are 73 width-1/value-01 records in ten captures and
thirteen property packets. No matching false record was retained. Every
locator below is main lane 0, s2c. `frame/subevent` and stream offsets are
decoder positions within that connection/direction. Completing packet is the
original capture packet that first completed the frame, not a property packet
ordinal. Source identities are
`xivl-captures:sources/pcap-1.23b/manifest.yaml`; canonical record IDs refer to
`xivl-captures:studies/property-stream-hash-catalog/derived/property-records.csv`.

For each `I` group below, the six record IDs correspond in order to native
indices `0,12,22,25,28,30`: Cover, Shoulder Tackle, Ballad of Magi, Presence
of Mind, Regen and Convert. They are in-packet records 4-9 at application
offsets `37,43,49,55,61,67`. Each packet ends with marker `/_init`, follows
another `0x0137`, and has a `/Chara/Player/Player_work` `0x00CC` two
subevents earlier in the same frame. These are initialization-state
publications, with no packet-bound acquisition quest or grant step.

- I1: `from_gridania_to_blackshroud.pcapng`, frame/subevent `245/19`, stream `29991`, inflated offset `2320`, completing packet `768`; record IDs `344,345,346,347,348,349`.
- I2: `gridania_to_coerthas.pcapng`, frame/subevent `184/19`, stream `20546`, inflated offset `2320`, completing packet `602`; record IDs `720,721,722,723,724,725`.
- I3: `gridania_to_coerthas.pcapng`, frame/subevent `854/19`, stream `121573`, inflated offset `2320`, completing packet `2865`; record IDs `1869,1870,1871,1872,1873,1874`.
- I4: `gridania_to_coerthas.pcapng`, frame/subevent `1328/19`, stream `219176`, inflated offset `2320`, completing packet `4019`; record IDs `3413,3414,3415,3416,3417,3418`.
- I5: `login.pcapng`, frame/subevent `17/21`, stream `5457`, inflated offset `2696`, completing packet `970`; record IDs `3793,3794,3795,3796,3797,3798`.
- I6: `move_out_of_room.pcapng`, frame/subevent `40/19`, stream `7042`, inflated offset `2320`, completing packet `95`; record IDs `4070,4071,4072,4073,4074,4075`.
- I7: `moving_around_gridania.pcapng`, frame/subevent `211/19`, stream `35370`, inflated offset `2320`, completing packet `475`; record IDs `4507,4508,4509,4510,4511,4512`.
- I8: `moving_around_gridania.pcapng`, frame/subevent `1259/19`, stream `195820`, inflated offset `2320`, completing packet `3574`; record IDs `5207,5208,5209,5210,5211,5212`.
- I9: `return_to_inn.pcapng`, frame/subevent `47/20`, stream `7610`, inflated offset `2656`, completing packet `147`; record IDs `7951,7952,7953,7954,7955,7956`.
- I10: `teleport_to_camp_nine_ivies.pcapng`, frame/subevent `81/19`, stream `10441`, inflated offset `2320`, completing packet `315`; record IDs `8229,8230,8231,8232,8233,8234`.
- I11: `teleport_to_camp_tranquil.pcapng`, frame/subevent `55/19`, stream `7836`, inflated offset `2320`, completing packet `221`; record IDs `8493,8494,8495,8496,8497,8498`.
- I12: `teleport_to_gridania.pcapng`, frame/subevent `67/21`, stream `11166`, inflated offset `3296`, completing packet `189`; record IDs `8741,8742,8743,8744,8745,8746`.

All other 28 indices are absent: `1-4,6-11,13-21,23-24,26-27,29,31-34`.
No absent flag is assigned a false state or a level-only unlock.

### Vengeance and quest 111201

`V` is `war_quest_update3.pcapng`, record `9303` (CSV line 9305),
frame/subevent `140/3`, stream offset `17381`, inflated subevent offset
`504`, completing capture packet `421`. The 136-byte application declares
100 property-stream bytes. At application +1 / inner-message +17 it has
width 1, hash `0xFCA1FFCD`, and at application +6 value `01`.
The native Boolean writer interprets that as true for native index 5,
Lua index 6, action 27186. The capture member SHA-256 is
`52279E8CBC13D3F4751B5EE3CB9C91DF1895ED5C41D30672629DB3EC1C9EFD05`;
the inner-message SHA-256 is
`96A4A577A825C94039C90FE97BBC25CCB6D213E23BB6EBA9AB0EB8091B2F4583`.

Three `0x0130` requests precede it in this same s2c connection. Their
application fields are trigger `0x029B2941`, owner `0x45600029`, event type
1, event name `talkDefault` and outer function `delegateEvent`.
At application +0x49 the typed span starts with actor D0=`0x029B2941`,
actor D1=`0xA0F1B261`, then method-name string D2:

- Frame/subevent `12/0`, stream `1660`, completing packet `25`:
  `processEventClear`, followed by four nils; END at parameter offset 33.
- `85/0`, stream `10753`, completing packet `245`: `processEventJob`,
  then integer `2000203` and three nils; END at parameter offset 35.
- `139/5`, stream `17143`, completing packet `421`: `sqrwa`, followed by
  integers `2661,1,1,7,9,10`; END at parameter offset 47.

The [typed decoder](delegate-command-producer.md#typed-values-and-bounds)
defines these actor, string, numeric, nil and END tokens under its default
endian interpretation. The
[static registry](../resource/static-actor-registry.md#file-contract-and-retail-coverage)
maps `0xA0F1B261` to low ID `111201`, `/Quest/Scenario/War/War0j1`.
`xivl-client-scripts:docs/winter-is-not-coming-client-contract.md`, section
"Emote entry and conditional callback frame", establishes the NPC
`delegateEvent` callback frame `(D1,D0,O,D3,...)` using D2 as the method key.
Thus the requests identify the quest and presentation methods conditionally
selected by local dispatch. Receiver readiness, actual historical Lua
execution and the server producer of the later setting remain separate.
The `sqrwa` integers are not renamed as a server quest step.

The immediately preceding frame also contains `0x0167` at `139/4`, stream
`17143`, inflated offset `304`, completing packet `421`. Its application
has message handle `0x5FF80001` at +0, u16 topic `25086` at +4, flag 32 at
+6, parameter count 8 at +16 and copied parameters at +8..+15. A signed32
token at +8 carries default-big-endian quest ID `111201` at +9..+12;
END is at +13. In the pinned binary, Capstone/PE checks of parser
VA `0x008A2410..0x008A24C2`, constructor `0x0089FA8C..0x0089FA98` and
apply branch `0x0089FDEA..0x0089FE57` establish those fields and conditional
WorldMaster forwarding through `0x00772650`. `worldMaster.csv` row 25086
is the dynamic quest-complete text. Associating that text with this log
request is conditional on historical handle/context resolution to
WorldMaster; neither the number nor the handle proves rendering.

The Vengeance property packet also contains canonical records `9304` and
`9305`: hash `0xACC58F29`, four-byte little-endian value `4571`, and hash
`0x66C75AE8`, four-byte value zero. Exact name hashes match
`charaWork.battleSave.skillPoint[3]` and `playerWork.questScenario[12]`.
The source declarations are `charabaseclass_battle.lua:1412-1419`
(integer32[52]) and `chara/player/playerbaseclass.lua:543-550` (actor[16]),
under `xivl-client-scripts:lua/scripts/`. Neither hash has an earlier record
in any retained WAR capture. The actor-slot zero therefore does not prove
a previous quest-111201 value, a transition to completed, or a quest step.

The first missing causal edge is a producer or transaction binding from the
quest update/completion path to this exact `0x0137` publication. A prior
false state is also missing. The supported result is a quest-associated
true-setting record for the statically declared reward, not a newly earned
action or an authenticated exact grant step.

## Identity and reproduction

Source extraction is `2012.09.19.0001`, game version 1.23b. Selected Lua
canonical LF bytes matched `xivl-client-scripts:manifests/scripts.json`;
all 42 chain LPBs and decoded payloads matched
`xivl-client-scripts:manifests/retail_lua_coverage.json`. Original-bytecode
analysis reproduced all 38 reward calls and their literal command/kind
arguments. CSV bytes matched `xivl-client-data:manifests/tables.json`.
Relevant exact identities are:

- StaticActor SAN: 108,911 bytes, SHA-256
  `BB7306461B1728493242016A16D9DD5257D7512C60E423B017DE5EC7ACED3D14`.
- `quest_new_reward.csv`:
  `496D5E7D2E5057852F990E18F404524426D456C52CBCBD95544652494350FF66`.
- `quest_reward.csv`:
  `C4165E2665823F3B1281CCAA5DFFC022F79BB47E8C1FD063E3378A9D240474F7`.
- `worldMaster.csv`:
  `2E2E2DD5CD9651388F6FA575B4229081AD0452165E78B725DF0A5B5B7CF7C643`.
- `ffxivgame.exe`: 15,996,808 bytes, image base `0x00400000`, SHA-256
  `9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.
  Native windows were checked with pefile 2024.8.26 and Capstone 5.0.7.
  No target process was executed.

In `xivl-client-scripts`, reproduce the retained original-bytecode contracts:

```text
python tools/build_quest_contracts.py --check --retail-root <client/script> --csv-root <csv-directory>
```

In `xivl-captures`, check raw framing, reproduce the property catalog and
inspect the same-direction Vengeance neighborhood:

```text
python tools/validate_framing.py sources/pcap-1.23b/objects/war_quest_update3.pcapng
python tools/extractors/extract_property_stream_catalog.py --check
python tools/extractors/extract_wire_order.py sources/pcap-1.23b/objects/war_quest_update3.pcapng --direction s2c --around 0x0137 --before 8 --after 8 --actor all
```

Save the following outside the tracked tree as `check_job_reward_rows.py`.
Run from `xivl-decomp` with explicit decoded-CSV and canonical-catalog inputs:

```text
python check_job_reward_rows.py <csv-directory> <property-records.csv>
```

```python
import csv, sys
from pathlib import Path
sys.path.insert(0, "tools")
from validate_murmur2 import murmur_hash2_backward

commands = (
    27146,27147,27148,27149,27159,27186,27187,27188,27189,27192,
    27106,27107,27108,27109,27118,27266,27267,27268,27272,27277,
    27227,27232,27237,27238,27239,27344,27345,27357,27358,27359,
    27305,27316,27317,27318,27319,
)
with (Path(sys.argv[1]) / "quest_new_reward.csv").open(
        encoding="utf-8-sig", newline="") as stream:
    reader = csv.reader(stream)
    labels = next(reader)[1:]
    next(reader)
    rewards = [(row[0], dict(zip(labels, row[1:]))) for row in reader]
with Path(sys.argv[2]).open(newline="") as stream:
    records = list(csv.DictReader(stream))
total = 0
for n, command in enumerate(commands):
    quests = [key for key, row in rewards
              if row["79"] == "-4" and row["81"] == str(command)]
    assert len(quests) == 1
    name = f"charaWork.additionalCommandAcquired[{n}]"
    hashed = murmur_hash2_backward(name.encode("ascii"), 0)
    hits = [row for row in records
            if int(row["property_hash"], 16) == hashed]
    assert all(row["value_width"] == "1" and row["value_hex"] == "01"
               for row in hits)
    expected = 12 if n in (0,12,22,25,28,30) else 1 if n == 5 else 0
    assert len(hits) == expected
    total += len(hits)
    print(n, command, quests[0], f"{hashed:08X}",
          [(row["capture"], row["record_index"]) for row in hits])
assert total == 73
```

These checks reproduce the declared rewards and retained record census.
They do not supply missing server grant policy, a quest-step producer,
prior acquisition state, level-only unlocks, persistence, or historical
client rendering.
