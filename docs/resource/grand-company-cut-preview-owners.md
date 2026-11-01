# Grand Company cut preview owners

This page reads the preview and background setup records of the Grand
Company NQ scenes in the retail 1.23b `client/cut` tree and joins each
scene to the quest chunks that name it. The client was identified by
`ffxivgame.exe` SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` and
`game.ver` `2012.09.19.0001`. Results are authored data. They do not
establish playback order, runtime binding or server dispatch.

## Method

Each scene is the primary file `client/cut/<scene>/<scene>`. It holds one
embedded `SEDBSCB` scheduler, parsed with `tools/scb_clip_scheduler.py`
under the [cut scheduler clip layout](cut-scheduler-clip-layout.md).
Offsets are absolute in the named file. D is a clip record.

- RapturePreviewSetupClip records are 52 bytes, as in the
  [class census](cut-scheduler-clip-class-census.csv), which counts 598 of
  them in 598 schedulers. In every record below, `D+0x14` begins a
  NUL-terminated ASCII string and the bytes after the terminator are zero.
- RaptureBgSetupClip records here are 72 bytes. The table gives
  `u32(D+0x14)` and `u32(D+0x18)`.
- The stage string is the only ASCII match of the form `aaa_a0_aaa00` in
  the file. Every match lies inside the scheduler span.

The native readers of both clip classes were not traced, so the meaning of
these fields is UNKNOWN on this page.

## Scene records

| Scene | RapturePreviewSetupClip, string | RaptureBgSetupClip, words | Stage string |
| --- | --- | --- | --- |
| `com0l110` | `0x1C820`, `Com0l1` | `0x1C68C`, 101, 33 | `sea_s0_fld03` at `0x2781F` |
| `com0g110` | `0x62C`, `Com0g1` | `0x488`, 103, 33 | `fst_f0_fld05` at `0xACFF` |
| `com0u110` | `0xBE38`, `Com0u1` | `0xBCB0`, 104, 30 | `wil_w0_fld03` at `0x13FE0` |
| `com0l410` | `0x693DC`, `Com0l4` | `0x69208`, 202, 29 | `prv_s0_gov01` at `0x74ABF` |
| `com0g410` | `0x68CBC`, `Com0g4` | `0x68AE8`, 204, 29 | `prv_f0_gov01` at `0x7430F` |
| `com0u410` | `0x68E0C`, `Com0u4` | `0x68C38`, 205, 29 | `prv_w0_gov01` at `0x7448F` |
| `com0l610` | `0x62C68`, `Com0l5` | `0x62670`, 202, 36 | `prv_s0_hal01` at `0x8101C` |
| `com0g610` | `0xA8284`, `Com0g5` | `0xA7034`, 103, 39 | `fst_f0_twn01` at `0xB8A70`, `0xCFA91` |
| `com0u610` | `0x8EA78`, `Com0u5` | `0x8E028`, 104, 31 | `wil_w0_twn01` at `0xAE936` |
| `com0l510` | `0x2A34`, `Com0l6` | `0x2680`, 104, 35 | `wil_w0_fld01` at `0x154CC` |
| `com0g510` | `0x2EA5C`, `Com0g6` | `0x2E4A0`, 103, 34 | `fst_f0_fld02` at `0x49F3D` |
| `com0u510` | `0x768`, `Com0u6` | `0x678`, 104, 36 | `wil_w0_fld03` at `0x1B061` |
| `elv0l110` | none | `0x434`, 101, 21 | `sea_s0_ind01` at `0xC10`, `0x2B84` |

## Scene files

| Scene | Bytes | SHA-256 | Scheduler offset / bytes / SHA-256 |
| --- | ---: | --- | --- |
| `com0l110` | 163,120 | `14e65131816c1565dc0a2c7037595f0b7d675f6b28f0f684a04c96728a930fa2` | `0x1C18C` / 47,888 / `eeb2077ed3668bfec154caeabfb7953adfbb80871fa4e583a3813f4669f074e6` |
| `com0g110` | 45,152 | `0426afa3a584e160368d4e7cfa05ec95e4f440122574a06cf0a866e6f0ec7a40` | `0x80` / 44,912 / `2ea0078fee41d516e52ea9895c4cdb2e91ab9407fdc4625c5465b92511d56e99` |
| `com0u110` | 82,880 | `d5592f033a39f38ac38cecf89c507fa9e026c0c71e87861fdf3912f061338db2` | `0xB850` / 35,504 / `0fd8bac866dad60d4da630caa8fe0750fe4b14a892c188391cd4e9361b249a1c` |
| `com0l410` | 479,312 | `ab0c92f5736cc408661acb95e79fbd0e26c26a691913b331240e2be994976e42` | `0x68CB8` / 49,856 / `895a21ce46f84383d4a6ef336defe003cc38ed20d862fa5596510e0247434ec7` |
| `com0g410` | 477,344 | `0f6eb2614085e11b91b327add82c36adc0db43d992042e1c5e77b6fb642f017d` | `0x68598` / 49,712 / `b6358162e7b439f413d1fc03a8973010da76424724040b611d297c751f4dc016` |
| `com0u410` | 477,728 | `4b176033b0baf575b3c04340eaf7f27e6820886b4a6a41363157cc99c0996ac1` | `0x686E8` / 49,760 / `b1e80e7aac69d8479d5bdc644ce8f202d5e3f5e02185c8e57831c80e43a4eb62` |
| `com0l610` | 540,928 | `bca918b45bbe3a1ee0e11212dd4b935387556ec730f1ced4ca360d2d23d259c5` | `0x6111C` / 134,912 / `a2bbfbaa55247055a355a25d17ece631cab4f9409dd9c0f3656be1eaef630f80` |
| `com0g610` | 963,216 | `c2e0adb676a434ce5e2c511ed28f0b293a96cd473dee343e94f2e3601004a366` | `0xA5CC0` / 177,264 / `df71c36a2c183e3102a7534739857b936ffc01bbffe6da9ea7c35d3dcfd3f92d` |
| `com0u610` | 719,760 | `ed1ff0940d6eb7410829c1cb7809a947818b47d18c608fc512d01a20d05da6dd` | `0x8D2C8` / 141,296 / `300ef26388b1831308ad92fb1dda13613b906d1dfeeb9ca7b64c7e3d0ba50f74` |
| `com0l510` | 89,520 | `d3b3c1860f6599c810f1a577ec3231ddd8552ec000376a4cfb1998d490109e72` | `0x1F50` / 81,312 / `a2fa273d642ce92cefa52d82b82b1c70d42d1dbb78b3d25d60f45b143f5588c9` |
| `com0g510` | 305,520 | `503b8e77fb6fae477da7d6f983fd56e468e344f25153cb187701ad00c3fd170a` | `0x2DC8C` / 117,632 / `f4f51ee35bcb050bab3a7af0450ecc4c3023f967613d4e372ecf67f4b7675620` |
| `com0u510` | 112,128 | `6b686d9e0c1b6764319e3f5d3429757ddea082c2a88b040b159a4963634d8a77` | `0x80` / 111,888 / `cba8fb2ca33910f7e345f726deb67902bf83a726ce9ce5c5b6dfac23338d63bf` |
| `elv0l110` | 11,680 | `cebcfdffda8189352c7e5a7132714dade590659d7325b31245ccc81fd99c12f3` | `0x80` / 11,440 / `efa48466bc5dec8b018343b0e6d75fde0ecf3dfe612b88e04a144cf7ddeda136` |

## Callers

All 2,671 `client/script` LPBs were decoded with `tools/decode_lpb.py`,
and the decoded chunks were searched for each scene name without regard to
case. Only the chunks below contain a scene name. Their LPB and chunk
hashes are in the
[mission bytecode source table](../event/grand-company-mission-bytecode-sources.csv),
and the `Com0` `startNQCutScene` PCs are in the
[bytecode trace locators](../event/grand-company-mission-bytecode.md#bytecode-trace-locators).
Scene names in the chunks keep their authored case, so `COM0l110` and
`COM0G510` name the `com0l110` and `com0g510` directories.

| Scene | Calling chunk and method |
| --- | --- |
| `com0l110` | `Com0l1.processEvent_030`, `Com0l5.processEvent_010` |
| `com0g110` | `Com0g1.processEventUriangerMore` |
| `com0u110` | `Com0u1.processEvent_030` |
| `com0l410` | `Com0l4.processEvent_020` |
| `com0g410` | `Com0g4.processEventClear` |
| `com0u410` | `Com0u4.processEvent_050` |
| `com0l610` | `Com0l5.processEvent_elevator_nq1` |
| `com0g610` | `Com0g5.processEvent005` |
| `com0u610` | `Com0u5.processEvent025` |
| `com0l510` | `Com0l6.processEvent_010` |
| `com0g510` | `Com0g6.processEventNq` |
| `com0u510` | `Com0u6.processEvent_005_03` |
| `elv0l110` | `Com0l5.processEvent_elevator_nq1`, `Com0l5.processEvent_elevator_nq2`, `Gcl104.processEventZanthael`, `Gcl105.processEventZanthael1`, `Gcl106.processEvent_LimsaWarp_Zanthael_01` |

In every row with a preview string, the string equals the name of a
calling chunk with its first letter capitalized. `com0l110` is the only
preview-string scene with two calling chunks, and its string names
`Com0l1`. `elv0l110` has no preview string and four calling chunks. The
`Gcl104`, `Gcl105` and `Gcl106` methods load the name at PCs 23, 23 and
20 (chunk offsets `0x398D`, `0x7207` and `0x1B99`). The fifth
quests call their `610` scenes and the sixth quests call their `510`
scenes.

## Com0l5 scenes

### com0l610

The dictionary holds 54 actor records: 4 CommonActor, 1 CharacterActor and
49 ProxyActor. The named ProxyActor records below carry the numeric token
`u32(+0x30)` used on the
[raid cutscene actor page](raid-cutscene-actor-records.md):

| Index | Label | Record | Numeric token |
| ---: | --- | --- | ---: |
| 4 | `PC` | `0x612EC` | 0 |
| 5 | `MERLWYB` | `0x61328` | 1001568 |
| 6 | `Yshtola` | `0x61364` | 1000001 |
| 7 | `Kishi1` | `0x613A0` | 1000084 |
| 8 | `Kishi2` | `0x613DC` | 1000085 |
| 9 | `Kishi3ele` | `0x61418` | 1000095 |
| 10 | `Kishi4miko` | `0x61454` | 1000126 |
| 11 | `Kishi5ruga` | `0x61490` | 1000127 |
| 12 | `kaizoku_ruga` | `0x614CC` | 1001740 |
| 13 | `kaizoku_hyu` | `0x61508` | 1000111 |
| 14 | `kaizoku3` | `0x61544` | 1000485 |
| 15 | `halambolo` | `0x61580` | 1001747 |
| 16 | `revelidge` | `0x615BC` | 1001746 |

The remaining ProxyActor records carry audience labels. The scheduler has
33 blocks and 19 RaptureMesClip records. Message word 0 is `u32(D+0x14)`
and message word 1 is `u32(D+0x18)` from the
[RaptureMesClip payload](cut-scheduler-clip-payloads-ranks-7-13.md#rapturemesclip).
Every record's word 1 equals the token of its own actor ordinal:

| Actor ordinal (label) | Word 1 | Word 0 values |
| --- | ---: | --- |
| 5 (`MERLWYB`) | 1001568 | 26 to 34, 37 to 44 |
| 13 (`kaizoku_hyu`) | 1000111 | 35 |
| 12 (`kaizoku_ruga`) | 1001740 | 36 |

In record order the word 0 values are 26, 27, 28, 29, 30, 32, 33, 36, 37,
38, 39, 40, 41, 42, 43, 44, 34, 35 and 31.

### com0l110

The dictionary holds ProxyActor records `PC` (`0x1C34C`, token 0) and
`Yshtola01` to `Yshtola05` (`0x1C388` to `0x1C478`, all token 1000001).
Its 10 RaptureMesClip records all use actor ordinal 0, the `sys`
CommonActor, with word 1 = 1000001. Their word 0 values in record order
are 33, 34, 58, 35, 36, 37, 38, 39, 40 and 51. The register-fed branch
between rows 34 and 58 is on the
[mission bytecode page](../event/grand-company-mission-bytecode.md#aftermath-recognition-branch).

### elv0l110

The scheduler has 5 blocks and no RapturePreviewSetupClip or
RaptureMesClip record. Its dictionary holds 4 CommonActor records, the
LayoutActor `_0069`, the CharacterActor `adv100428_0079` and two
ProxyActor records: `PC` (`0x210`, token 0) and `ZANTHAEL` (`0x290`,
token 1000351).

## Interpretation and limits

- INFERRED: the preview string names the quest the scene was authored
  for. Under that reading `Com0l5.processEvent_010` plays a scene authored
  for `Com0l1`, and `com0l610` is the `Com0l5` scene.
- INFERRED: the first RaptureBgSetupClip word is a region number. Its
  values group by the stage prefix: 101 with `sea_s0`, 103 with `fst_f0`,
  104 with `wil_w0`, and 202, 204 and 205 with `prv_s0`, `prv_f0` and
  `prv_w0`.
- INFERRED: RaptureMesClip word 0 is a text row of the owning quest's
  sheet, as the mission bytecode page infers for `com0l110`, and word 1
  is the speaker's numeric token.
- INFERRED: `elv0l110` is a lift ride with `ZANTHAEL` on the
  `sea_s0_ind01` stage, from its labels, its stage string and the
  Zanthael method names of its `Gcl` callers.
- UNKNOWN: whether the retail server dispatched `Com0l5.processEvent_010`,
  and which text sheet the client resolves `com0l110` rows against when
  `Com0l5` plays it.
