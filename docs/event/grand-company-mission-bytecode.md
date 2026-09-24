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

The aftermath scene assets each contain one setup `NumberClip`.
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

The familiar predicates were sampled with `nil`, 0, 1, `false`, and `true`;
the finale predicates used the corresponding extra-argument tuples. The
six ask methods were sampled with choices 0 and 1. Client calls were
recording stubs; their effects outside these methods were not simulated.

## Provenance and limits

The [source table](grand-company-mission-bytecode-sources.csv) identifies
all 18 original `client/script` LPBs by path and SHA-256, plus the SHA-256
of each decoded Lua 5.1 chunk. Decoding those LPBs with
`xivl-client-structs/tools/decode_lpb.py` reproduced all 18 chunks used by
the bytecode pass byte-for-byte. The method and PC locators above identify
the relevant instructions in those chunks.
The trace domain was `nil`, 0, 1, `false`, and `true` for extra arguments,
with sampled choices 0 and 1. That coverage does not prove behavior for
arbitrary inputs, client rendering, or historical server dispatch.
