# Grand Company mission bytecode branches

An offline bytecode pass over 18 `Com0[lgu]1..4` and `Com5[lgu]0..1`
scenario chunks recovered client-side argument and choice behavior that
readable decompilation can obscure. All 18 examined `.luac` chunks matched
the bytes decoded from the corresponding `.le.lpb` files. Their
readable Lua exports do not match the published client-script manifest's
canonical text hashes, so this note relies on compiled bytecode and the
bounded control-flow pass, not on textual equivalence.

## Familiar entry and aftermath

The three familiar-entry methods choose the ordinary fade only when their
extra argument is Lua boolean `true`. Numeric `1` is different; `nil`, 0,
1, and `false` take the after-warp fade path.

| Quest / chunk | Entry method | Scene |
| --- | --- | --- |
| 111401 / `Com0l1` | `processEvent_020` | `COM0L105`, mode 2 |
| 111601 / `Com0g1` | `processEventUrianger` | `COM0G105`, mode 2 |
| 111801 / `Com0u1` | `processEvent_020` | `COM0U105`, mode 2 |

The corresponding aftermath methods (`Com0l1.processEvent_030`,
`Com0g1.processEventUriangerMore`, and `Com0u1.processEvent_030`) forward
an extra argument to their NQ scenes `COM0l110`, `COM0G110`, and
`COM0U110` respectively. No default for that argument is in these methods.
The value sent by the historical retail server remains unknown; neither
zero nor an actor ID is justified as a replacement.

The aftermath scene assets each contain one `NumberClip`.
Their decoded initial values are separate from that missing forwarded
argument:

| Scene | NumberClip offset | Initial value | Scene SHA-256 |
| --- | ---: | ---: | --- |
| `com0l110` | `0x1C854` | 1 | `14e65131816c1565dc0a2c7037595f0b7d675f6b28f0f684a04c96728a930fa2` |
| `com0g110` | `0x7D0` | 0 | `0426afa3a584e160368d4e7cfa05ec95e4f440122574a06cf0a866e6f0ec7a40` |
| `com0u110` | `0xBE6C` | 1 | `d5592f033a39f38ac38cecf89c507fa9e026c0c71e87861fdf3912f061338db2` |

The three `client/cut/<scene>/<scene>` file hashes and NumberClip offsets
above identify the inspected scene assets. In the pinned executable, the
`GetNumberRegisterClip` factory at VA `0x00A4F860`, constructor at
`0x00DFAE70`, and execution at `0x00DFAEC0` identify each scene's
register-1 reader. These serialized
values are not a proven fallback when the NQ playback argument is
missing, nor proof of the server's historical choice for any player.

## Aftermath recognition branch

`com0l110` and `com0u110` each test their register-fed `NumberClip` with
an `IfClip` that selects one of two branch tracks in block `c02`.
`com0g110` has no such test. The client facts below come from Ghidra
decompilation and disassembly of the pinned `ffxivgame.exe` (image base
`0x00400000`, addresses are VAs). The scene facts come from the three
scene files whose SHA-256 values are in the
[aftermath table](#familiar-entry-and-aftermath). D is a serialized clip
record with the shared header on the
[clip layout page](../resource/cut-scheduler-clip-layout.md#clip-records),
and all values are little-endian.

### IfClip record

The constructor `0x00A291E0` installs the
`SQEX::CDev::Engine::Cut::Scheduler::IfClip` vftable `0x010988EC`. It and
the branch builder `0x00A29390` walk this layout:

| Offset | Width | Field |
| --- | ---: | --- |
| `D+0x10` | 2 | `s16` track passed to the track call |
| `D+0x12` | 1 | `u8` branch count |
| `D+0x14` | | First branch |

Each branch is a 4-byte header followed by its conditions. The builder
copies the target into the branch object at +0x08 and the mode into
+0x0C.

| Branch offset | Width | Field |
| --- | ---: | --- |
| `+0x00` | 2 | `s16` target track |
| `+0x02` | 1 | `u8` condition count |
| `+0x03` | 1 | `u8` combine mode |
| `+0x04` | | First condition |

The condition iterator `0x00A2AA50` advances 8 bytes per condition, or
20 bytes when the type byte is 1.

| Condition offset | Width | Field |
| --- | ---: | --- |
| `+0x00` | 1 | `u8` operator |
| `+0x01` | 1 | `u8` type |
| `+0x02` | 2 | `s16` index of the tested clip |
| `+0x04` | 4 | `s32` value, or 16 string bytes when the type is 1 |

### Evaluation

Slot 1 of the IfClip vftable, `0x00A29690`, rebuilds the branches through
`0x00A29390` and tests them in record order with `0x00A28F30`. The first
branch that passes supplies its target track. When none passes, the
target stays -1. The method then calls `0x00A20000` with `s16(D+0x10)`
and the target.

`0x00A28F30` reads the combine mode. Mode 0 passes when every condition
holds, so an empty list passes. Mode 1 passes when any condition holds,
so an empty list fails. Any other mode fails.

`0x00A2A9A0` evaluates one condition. It looks up the clip at the
condition's index through `0x00A21430` and fails when no clip is there.
For type 1 it passes the string from the clip's vtable +0x40 method and
the condition string to `0x00A2A950`, which accepts operators 0 (always),
1 and 2 (16-byte `strncmp` equal and not equal) and fails any other. For
every other type it passes the value from the clip's vtable +0x3C method
and the condition value to `0x00A2A8A0`:

| Operator | Holds when |
| ---: | --- |
| 0 | Always |
| 1 | clip value == condition value |
| 2 | clip value != condition value |
| 3 | clip value > condition value, signed |
| 4 | clip value >= condition value, signed |
| 5 | clip value < condition value, signed |
| 6 | clip value <= condition value, signed |
| 7 | (clip value AND condition value) != 0 |
| 8 | (clip value AND condition value) == 0 |
| Other | Never |

`0x00A20000` forwards to the track object at +0x58 of the object at +0x0C
of the slot argument. That object holds one node per track index in its
+0x08 array. Its walks read node +0x00 as the parent, +0x04 as the first
child, +0x08 as the next sibling and the byte at +0x0C as the enable
flag. With target -1, `0x00A26B70` clears the flag on every child of
the node for `s16(D+0x10)`. Otherwise `0x00A26B20` clears it on every
child of the target node's parent, when the node has one, and then sets
it on the target node.
The begin test `0x00A26B00`, shown on the
[clip timing page](../resource/cut-scheduler-clip-timing.md), returns
false unless the clip's track node and every node on its parent chain
have the flag set.

### NumberClip register feed

GetNumberRegisterClip (vftable `0x0113A9DC`) slot 2, `0x00DFAEC0`, looks
up the clip at index `s16(D+0x10)` through `0x00A1FFB0`. It reads the
value for register `s16(D+0x12)` through vtable +0x44 of the object at
+0xF0 of the `0x00A20070` result and stores it through the target clip's
vtable +0x38 method. In the NumberClip vftable `0x0113AF9C`, +0x38 is
`0x00DFB230`, which stores the word at clip +0x38, and +0x3C is
`0x00DFB240`, which returns it. An IfClip condition on a NumberClip
therefore compares the word that the register feed stored.

### Scene tables

Each scene has one GetNumberRegisterClip, and each one reads register 1
into the scene's NumberClip:

| Scene | GetNumberRegisterClip | Payload | NumberClip index | Block |
| --- | --- | --- | ---: | --- |
| `com0l110` | `0x1C8DC` | clip 14, register 1 | 14 | setup |
| `com0g110` | `0x918` | clip 22, register 1 | 22 | `c01` |
| `com0u110` | `0xBE80` | clip 15, register 1 | 15 | setup |

The register-fed IfClips sit in block setup. Each branch has one type 0
condition and combine mode 0, and `s16(D+0x10)` equals the record's own
track word `s16(D+0x08)`.

| Scene | IfClip (track) | Branch 1 | Branch 2 | No match |
| --- | --- | --- | --- | --- |
| `com0l110` | `0x1CAD8` (43) | clip 14 == 1: track 44 | clip 14 != 9999: track 46 | 9999 |
| `com0u110` | `0xBFA4` (10) | clip 15 == 0: track 11 | clip 15 == 1: track 13 | every value except 0 and 1 |

The branch tracks hold no clips. Block `c02` places one RaptureMesClip on
the track after each target. The message word 0 is `u32(D+0x14)` from the
[RaptureMesClip payload](../resource/cut-scheduler-clip-payloads-ranks-7-13.md#rapturemesclip).

| Scene | Target | RaptureMesClip (track) | Message word 0 |
| --- | ---: | --- | ---: |
| `com0l110` | 44 | `0x1CFEC` (45) | 34 |
| `com0l110` | 46 | `0x1D010` (47) | 58 |
| `com0u110` | 11 | `0xC41C` (12) | 58 |
| `com0u110` | 13 | `0xC440` (14) | 32 |

The other IfClips test actor clips, not a NumberClip:

| Scene | IfClip (track) | Tested clips | Targets |
| --- | --- | --- | --- |
| `com0l110` | `0x1C950` (27) | RaptureGetActorCategoryClip `0x1C868` (clip 15): == 1, != 9999 | 28, 30 |
| `com0u110` | `0xBED8` (24) | RaptureGetActorCategoryClip `0xBE00` (clip 12): == 1, == 3, != 9999999 | 25, 27, 29 |
| `com0g110` | `0x720` (17) | RaptureGetActorNumberClip `0x610` (clip 12): == 3 or == 9 (mode 1), then RaptureGetActorCategoryClip `0x5D8` (clip 10): == 3, == 1, == 2 | 18, 20, 22, 24 |

The camera targets in `com0l110` and `com0u110` also hold no clips, and
their LocalCameraClips sit on the tracks after them (29 and 31, then 26,
28 and 30).

### Interpretation and limits

- INFERRED: each target track is the parent of the following track, so
  selecting target N lets the clips on track N+1 begin. The enable walk
  above is observed, but the parent links come from the scene's `@CTRK`
  track table, which this page does not decode.
- INFERRED: message word 0 names the text row. The values match the
  variant pairs `com0l1` 34 and 58 and `com0u1` 32 and 58 in
  `xivl-client-data:docs/grand-company-dialogue-variants.md`. The
  RaptureMesClip payload page leaves the meaning of the word UNKNOWN.
- Under both inferences, `com0l110` plays row 34 for register value 1 and
  row 58 for any other value except 9999. `com0u110` plays row 58 for 0,
  row 32 for 1 and neither row for any other value.
- UNKNOWN: whether the extra `startNQCutScene` argument of the aftermath
  methods reaches register 1. The register source behind vtable +0x44 of
  the +0xF0 object was not traced to the Lua call.
- UNKNOWN: how `com0g110` chooses between its two `c02` blocks. No IfClip
  in that scene reads NumberClip 22.

## Level-22 finale predicates

| Quest / method | Scene branch | Other branch |
| --- | --- | --- |
| 111404 `Com0l4.processEvent_020(a)` | `a ~= 1`: `com0l410` | numeric `a == 1`: dialogue |
| 111604 `Com0g4.processEventClear(a,b)` | numeric `a == 0 and b == 0`: `com0g410` | every other sampled tuple: dialogue |
| 111804 `Com0u4.processEvent_050(a)` | `a ~= 1`: `com0u410` | numeric `a == 1`: dialogue |

In particular, `(false, false)` is not `(0, 0)` for Gridania. The branches
establish client presentation selection only, not the server's reason for
choosing a tuple or the quest's completion policy.

The scene dictionaries for all three `410` branches contain
`Cid` (actor class 1001572) and `Ebrelnaux` (1060011). Their records are
byte-identical across the three scenes, but the enclosing assets have
distinct SHA-256 identities:

| Scene | Cid offset | Ebrelnaux offset | Scene SHA-256 |
| --- | ---: | ---: | --- |
| `com0l410` | `0x68F58` | `0x68F94` | `ab0c92f5736cc408661acb95e79fbd0e26c26a691913b331240e2be994976e42` |
| `com0g410` | `0x68838` | `0x68874` | `0f6eb2614085e11b91b327add82c36adc0db43d992042e1c5e77b6fb642f017d` |
| `com0u410` | `0x68988` | `0x689C4` | `4b176033b0baf575b3c04340eaf7f27e6820886b4a6a41363157cc99c0996ac1` |

These are cutscene dictionary identities, not persistent NPC spawns,
combat participants, or evidence of the server's scene-selection trigger.

The six `Com5[lgu]0..1` dungeon-entry ask methods in the examined pass call
`ask` once and return the saved choice. On sampled choice 1 they leave the
talk turn open for the next handoff; on sampled decline 0 they close it.
Repeated `ask` expressions in a readable decompile must not be implemented
as repeated prompts without a bytecode check.

## Nymiene and Aergfloh talks

The `com0l4` talk methods below and `Etc3u1.processEvent020` were read
directly from a Lua 5.1 instruction listing of the decoded chunks in the
[source table](grand-company-mission-bytecode-sources.csv). `Etc3u1` is
the class path of static actor 110726 in
`xivl-client-data:manifests/staticactor_class_paths.json`. Each `say`
call passes the row as its second argument and 0 as its third. This page
records row numbers only. The row text belongs to the decoded `com0l4`
and `etc3u1` text sheets.

| Quest / method | Branch | Rows in call order |
| --- | --- | --- |
| 111404 `Com0l4.processEventNymiene(a)` | numeric `a == 1` | 44, 46, 47 |
| 111404 `Com0l4.processEventNymiene(a)` | any other value | 45, 46, 47 |
| 111404 `Com0l4.processEventNymiene_01` | none | 57 |
| 111404 `Com0l4.processEventAergfloh` | none | 48, 49 |
| 111404 `Com0l4.processEventAergfloh_01` | none | 50 |
| 110726 `Etc3u1.processEvent020(a)` | boolean `a == true` | 38, 39, 40, 41 |
| 110726 `Etc3u1.processEvent020(a)` | any other value | 63, 39, 40, 41 |

Every method opens with `startCliantTalkTurn` and `_runCharaScheduler`
and closes with `finishCliantTalkTurn`. `Etc3u1.processEvent020` calls
`_runCharaScheduler` again between rows 39 and 40. The `_01` methods and
`processEventAergfloh` take three parameters and contain no conditional
instruction. `processEventAergfloh_01` says row 50 alone, not rows 48
and 49.

The two branches are one `EQ` with A = 0 on R3, the fourth parameter
after the quest, player and event owner. When R3 equals the constant,
the `EQ` skips the following `JMP` and the first row plays. Otherwise
the `JMP` reaches the second row.

| Chunk | Method | `EQ` PC (chunk offset) | Raw word | Constant | Equal edge | Other edge |
| --- | --- | --- | --- | --- | --- | --- |
| `com0l4` | `processEventNymiene` | 7 (`0xD81`) | `0x01C10017` | K4, number 1 | PC 9, row 44 | `JMP` to PC 15, row 45 |
| `etc3u1` | `processEvent020` | 7 (`0x1379`) | `0x01C10017` | K4, boolean `true` | PC 9, row 38 | `JMP` to PC 15, row 63 |

Lua 5.1 equality between values of different types is false. Boolean
`true` therefore takes row 45 in `Com0l4`, and numeric 1 or `nil` takes
row 63 in `Etc3u1`. The two quests test different value types in the
same argument position.

The `com0l4` root binds closures 6, 7, 8 and 9 to `processEventNymiene`,
`processEventNymiene_01`, `processEventAergfloh` and
`processEventAergfloh_01` at PCs 27, 30, 33 and 36. The `etc3u1` root
binds closure 15 to `processEvent020` at PC 54. These branches establish
client presentation selection only. UNKNOWN: the value the retail server
passed in either argument, and whether it chose a `_01` method.

## Level-25 fifth-quest predicates

The closing talks of the three fifth company quests branch on their fourth
parameter. They were read from a Lua 5.1 instruction listing of the decoded
`com0l5`, `com0g5` and `com0u5` chunks in the
[source table](grand-company-mission-bytecode-sources.csv) and sampled with
`nil`, 0, 1, `false` and `true` in that parameter. Each `say` call passes
the row as its second argument and 0 as its third. This page records row
numbers only. The row text belongs to the decoded `com0l5`, `com0g5` and
`com0u5` text sheets.

| Quest / method | Branch | Rows in call order |
| --- | --- | --- |
| 111405 `Com0l5.processEvent_015(a)` | numeric `a == 1` | 51, 71, 68, 52, 53, 54, 55, 56, 57, 58, 59, 60 |
| 111405 `Com0l5.processEvent_015(a)` | any other value | 51, 71, 68, 52, 53, 54, 55, 56, 57, 58, 64, 69, 70, 60 |
| 111605 `Com0g5.processEvent030(a)` | numeric `a == 0` | 48 to 59, 60, 61, 62, 84 |
| 111605 `Com0g5.processEvent030(a)` | numeric `a == 1` | 48 to 59, 82, 83, 84 |
| 111605 `Com0g5.processEvent030(a)` | any other value | 48 to 59, 60, 61, 62, 84 |
| 111805 `Com0u5.processEvent030(a)` | numeric `a == 1` | 59, 60, 61, 62, 89, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 74 |
| 111805 `Com0u5.processEvent030(a)` | any other value | 59, 60, 61, 62, 89, 63, 64, 91, 92, 67, 68, 69, 70, 71, 72, 73, 74 |

Rows 48 to 59 of `Com0g5` play in ascending order. The sampled values
`nil`, `false` and `true` take the any-other-value row list in each
method, and 0 takes it in `Com0l5` and `Com0u5`. Every method opens with
`startCliantTalkTurn`, calls `_runCharaScheduler` between rows and closes
with `finishCliantTalkTurn`. `Com0l5.processEvent_015` calls `_wait(3)`
after row 71, `Com0g5.processEvent030` calls `_wait(1.5)` before row 84,
and `Com0u5.processEvent030` calls `_wait(2)` twice between rows 70
and 71. No method returns a value.

Each test is an `EQ` with A = 0, so the equal edge skips the following
`JMP`. `Com0g5` copies R3 to R4 at PC 79 and tests the copy twice. The
branches rejoin before the final rows: `Com0l5` at PC 95 for row 60,
`Com0g5` at PC 138 for row 84 and `Com0u5` at PC 76 for row 68.

| Chunk | Method | `EQ` PC (chunk offset) | Raw word | Constant | Equal edge | Other edge |
| --- | --- | --- | --- | --- | --- | --- |
| `com0l5` | `processEvent_015` | 66 (`0x12B9`) | `0x01C50017` | K20, number 1 | PC 68, row 59 | `JMP` to PC 77, rows 64, 69, 70 |
| `com0g5` | `processEvent030` | 80 (`0x14D3`) | `0x02418017` | K6, number 0 | PC 82, rows 60, 61, 62 | `JMP` to PC 102 |
| `com0g5` | `processEvent030` | 102 (`0x152B`) | `0x02468017` | K26, number 1 | PC 104, rows 82, 83 | `JMP` to PC 119, rows 60, 61, 62 |
| `com0u5` | `processEvent030` | 48 (`0x1815`) | `0x01C38017` | K14, number 1 | PC 50, rows 65, 66 | `JMP` to PC 61, rows 91, 92, 67 |

`Com0l5.processEvent_015_01` also declares four parameters, but no
instruction reads R3, and every sampled value says row 47 alone. The
`com0l5` root binds closure 13 to `processEvent_015` at PC 48, the
`com0g5` root binds closure 14 to `processEvent030` at PC 51 and the
`com0u5` root binds closure 18 to `processEvent030` at PC 63.

The scene calls of the same chunks name these NQ scenes. The
[cut preview owner page](../resource/grand-company-cut-preview-owners.md)
records the scene files.

| Chunk | Method | Branch | Scene, arguments after the name |
| --- | --- | --- | --- |
| `com0l5` | `processEvent_010` | none | `COM0l110`, 1, 0 |
| `com0l5` | `processEvent_elevator_nq1` | `ask` result == 1 | `elv0l110`, 1, 0, then `com0l610`, 1, 0 |
| `com0l5` | `processEvent_elevator_nq2` | `ask` result == 1 | `elv0l110`, 1, 0 |
| `com0g5` | `processEvent005` | none | `com0g610`, 1 |
| `com0u5` | `processEvent025` | none | `com0u610`, 1 |
| `com0l6` | `processEvent_010` | none | `com0l510`, 1, 0, R3 |
| `com0g6` | `processEventNq` | none | `COM0G510`, 1 |
| `com0u6` | `processEvent_005_03` | none | `com0u510`, 1, 0, R3 |

These branches establish client presentation selection only. UNKNOWN: the
value the retail server passed in the fourth parameter, and whether it
dispatched `Com0l5.processEvent_010`.

## Bytecode trace locators

The following locators use zero-based Lua 5.1 instruction PCs within the
named method. Call offsets are byte offsets in the decoded chunk identified
by the [source table](grand-company-mission-bytecode-sources.csv). The `EQ`
PC is the conditional instruction; the two following PCs are its sampled
successor edges. These records make the bounded branch observations
independently inspectable in the original chunks.

| Chunk | Method | `EQ` PC | Relevant call PC (chunk offset) |
| --- | --- | ---: | --- |
| `com0l1` | `processEvent_020` | 7 | `startNQCutScene` 6 (`0x92E`) |
| `com0g1` | `processEventUrianger` | 7 | `startNQCutScene` 6 (`0x953`) |
| `com0u1` | `processEvent_020` | 7 | `startNQCutScene` 6 (`0x8B1`) |
| `com0l1` | `processEvent_030` | - | `startNQCutScene` 8 (`0xE7A`) |
| `com0g1` | `processEventUriangerMore` | - | `startNQCutScene` 8 (`0xA5C`) |
| `com0u1` | `processEvent_030` | - | `startNQCutScene` 8 (`0x9B6`) |
| `com0l4` | `processEvent_020` | 12 | `startNQCutScene` 44 (`0x121E`) |
| `com0g4` | `processEventClear` | 21, 26, 28 | `doSalute` 7 (`0xF1D`) |
| `com0u4` | `processEvent_050` | 20 | `startNQCutScene` 57 (`0xFF5`) |
| `com5l0` | `processEvent_010_01` | 14 | `ask` 13 (`0xF9A`) |
| `com5g0` | `processEvent_010_1` | 14 | `ask` 13 (`0x9B6`) |
| `com5u0` | `processEvent_010_01` | 14 | `ask` 13 (`0xE49`) |
| `com5l1` | `processEvent_030_1` | 14 | `ask` 13 (`0xD9F`) |
| `com5g1` | `processEvent_020_1` | 14 | `ask` 13 (`0x10FA`) |
| `com5u1` | `processEvent_020_1` | 14 | `ask` 13 (`0xB54`) |
| `com0l4` | `processEventNymiene` | 7 | `say` 13 (`0xD99`), 19 (`0xDB1`) |
| `com0l4` | `processEventNymiene_01` | - | `say` 11 (`0xEDD`) |
| `com0l4` | `processEventAergfloh` | - | `say` 11 (`0xFBD`), 16 (`0xFD1`) |
| `com0l4` | `processEventAergfloh_01` | - | `say` 11 (`0x10BA`) |
| `etc3u1` | `processEvent020` | 7 | `say` 13 (`0x1391`), 19 (`0x13A9`) |
| `com0l5` | `processEvent_015` | 66 | `say` 75 (`0x12DD`), 84 (`0x1301`), 102 (`0x1349`) |
| `com0g5` | `processEvent030` | 80, 102 | `say` 89 (`0x14F7`), 111 (`0x154F`), 126 (`0x158B`), 151 (`0x15EF`) |
| `com0u5` | `processEvent030` | 48 | `say` 54 (`0x182D`), 65 (`0x1859`), 83 (`0x18A1`) |
| `com0l5` | `processEvent_010` | - | `startNQCutScene` 7 (`0xD6C`) |
| `com0l5` | `processEvent_elevator_nq1` | 17 | `ask` 16 (`0x14F8`), `startNQCutScene` 26 (`0x1520`), 31 (`0x1534`) |
| `com0l5` | `processEvent_elevator_nq2` | 14 | `ask` 13 (`0x16D8`), `startNQCutScene` 23 (`0x1700`) |
| `com0g5` | `processEvent005` | - | `startNQCutScene` 6 (`0x847`) |
| `com0u5` | `processEvent025` | - | `startNQCutScene` 6 (`0x16AA`) |
| `com0l6` | `processEvent_010` | - | `startNQCutScene` 8 (`0xFE2`) |
| `com0g6` | `processEventNq` | - | `startNQCutScene` 6 (`0x1C96`) |
| `com0u6` | `processEvent_005_03` | - | `startNQCutScene` 8 (`0xEAC`) |

The familiar predicates were sampled with `nil`, 0, 1, `false`, and `true`;
the finale predicates used the corresponding extra-argument tuples. The
six ask methods were sampled with choices 0 and 1. Client calls were
recording stubs; their effects outside these methods were not simulated.
The `say` rows for the talk methods come from the instruction listing,
not from sampling. Their two `say` PCs are the equal and other edges.
The fifth-quest talks were sampled with `nil`, 0, 1, `false`, and `true`
in the fourth parameter. Their listed `say` PCs are the first row of each
edge and then the first row after the edges rejoin.

## Provenance and limits

The [source table](grand-company-mission-bytecode-sources.csv) identifies
the 18 original `client/script` LPBs of the bytecode pass and the
`etc3u1` LPB by path and SHA-256, plus the SHA-256 of each decoded Lua
5.1 chunk. Decoding those LPBs with
`xivl-client-structs/tools/decode_lpb.py` reproduced all 18 chunks used by
the bytecode pass byte-for-byte and produced the listed `etc3u1` chunk.
The `com0l5`, `com0g5`, `com0u5`, `com0l6`, `com0g6`, `com0u6`, `gcl104`,
`gcl105` and `gcl106` rows were decoded with this repository's `tools/decode_lpb.py`, which also reproduced
the listed `com0l1` and `com0l4` chunks. Their instruction listing used the
Lua 5.1 chunk reader and instruction decoder of
`xivl-client-scripts/tools/build_quest_contracts.py`.
The method and PC locators above identify the relevant instructions in
those chunks.
The trace domain was `nil`, 0, 1, `false`, and `true` for extra arguments,
with sampled choices 0 and 1. That coverage does not prove behavior for
arbitrary inputs, client rendering, or historical server dispatch.
