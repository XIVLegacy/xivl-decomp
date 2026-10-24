# City airship scene sequence

Recovered `DftSrt.eventDeparture` calls
`startFadeOutCutSceneDefault`, then `startNQCutScene(departure, 1)`, then
`startNQCutScene(arrival, 1)` when an arrival key is present, and finally
`startFadeInCutSceneAfterWarp`. The calls are ordered in the client script;
the method itself does not encode a six-route table or world landing XYZ.
`DirectorBaseClass.delegateEvent` forwards the invoking owner after the
player argument when it calls the delegated method. This is why the scene
keys are the later arguments to `DftSrt.eventDeparture`, rather than fields
on a `PopulaceFlyingShip` actor.

## Playback return and event correlation

`QuestBaseClass.startNQCutScene` creates a CutScene with the quest/scenario
owner, calls `startCutScene(1, 61, selector, ...)`, receives two results,
deletes the scene only when the first result is exactly `true`, and returns
the second result. For `eventDeparture`, the selector is the literal `1`.
The common CutScene method calls `_play` in play mode 1 and returns its
result pair after its local UI/map-load handling. The playing actor is also
the target of an accepted skip prompt. The
`xivl-client-scripts:docs/cutscene-replay-skip-contract.md`, section
`Skip actor binding`, owns that UI contract.

The original LPB bytecode establishes where the return values stop:

| Method | Original decoded-chunk locator, zero-based PC | Observation |
| --- | --- | --- |
| `DftSrt.eventDeparture`, `root/proto2` | PC 9, `0x217`; PC 15, `0x22F` | Both scene calls use `CALL A=6 B=4 C=1`, requesting no results. |
| Same method | PC 19, `0x23F` | `RETURN A=0 B=1` returns no values after the fade-in helper. |
| `QuestBaseClass.startNQCutScene`, `root/proto1` | PC 10, `0x79E`; PCs 11-18, `0x7A2`-`0x7BE` | Receives the pair, tests the first against `true`, conditionally deletes, and returns the second. |
| `CutScene.startCutScene`, `root/proto8` | PC 235, `0x20BD`; PC 287, `0x218D` | `_play` supplies two values; the helper returns the pair. |
| Skip-widget callback, `root/proto2` | PCs 0-7, `0x2DC`-`0x2F8` | Ask result 1 and a nonnil bound actor reach that actor's `_skip()` call. |

Thus the first unbound return/correlation edge is between each
`startNQCutScene` return and the outer `DftSrt.eventDeparture` invocation:
the return exists, but the paired method discards it. Neither scene result
becomes an explicit return operand from this method. This does not prove
that the enclosing native event machinery cannot report method completion.
It leaves that machinery's association with this particular pair unbound.

The [native scene notice](../net/ferry-scene-notice-producer.md) establishes
an actor-local `_play` waiter, skip flag and result callback. Its `0x00CE`
body has a lifecycle selector and scene name, with four copied bytes whose
value is unbound. No traced field is established as a starting server event
ID, event name, trigger/owner actor ID, sequence number or invocation token.
In particular, the callback's result-record `+0x20` code and `+0x28` dword
are local result operands, not authenticated event or sequence identifiers.

The [StartServerOrderEventFunction receiver](start-event-fn-receiver.md)
owns the incoming trigger/owner keys, event type/name, function name and
parameter vector. Its generic native frame is a separate endpoint. The
inspected scene/result path does not bind those fields to this pair or to
the notice's four trailing bytes. The
[server-order talk admission](server-order-talk-admission.md) exchange also
does not supply that missing correlation.

`PopulaceFlyingShip` has separate attendant-menu returns. `eventIn` finishes
the client talk turn and returns its destination-choice operand after the
confirmation result equals 1 (`root/proto1` PCs 117-126,
`0x3ED`-`0x411`); its exit tail finishes the talk turn and returns nil
(PCs 160-163, `0x499`-`0x4A5`). `eventOut` finishes that turn and returns
true for ask result 1, otherwise nil (`root/proto2` PCs 29-44,
`0x6CB`-`0x707`). These are menu answers. They are not departure/arrival
playback completion, cancellation or skip reports. No inspected retail
caller joins them to an exact ordered pair of city scene keys.

### Source identity and reproduction

The selected Lua files matched canonical LF bytes in
`xivl-client-scripts:manifests/scripts.json`. The original LPBs and decoded
chunks matched `xivl-client-scripts:manifests/retail_lua_coverage.json`.
These are static observations of extraction `2012.09.19.0001`.

| Script under `lua/scripts/` | Canonical SHA-256 |
| --- | --- |
| `quest/scenario/defaulttalk/dftsrt.lua` | `a8ed70ef14ae6eebce5c403c4982c72b3fbe84f0916409ca052274e97f685d48` |
| `chara/npc/populace/populaceflyingship.lua` | `02e39950c60bc96b1b6996aecf2c6230dafb8bc13b17ff6b857a74733f14e61f` |
| `quest/questbaseclass_common.lua` | `cf61a5687dbc7b6accd26c8cd3a138decc0372fb72518ac6f2054f125bc1465e` |
| `director/directorbaseclass.lua` | `45c87f548a82e81681433d6ad89c6de6fc62f313cead66e72bf8b8cf7aa55a82` |
| `gamedata/cutscene_common.lua` | `f6db9559b3f805d2738c073c7db9dd102fd5c33a056e789ff2fb2f8aff395bd1` |
| `widget/cutsceneskipwidget.lua` | `0f570ae13cca80944ecbb625e29be056799b046869357f6d8d44e8ac5eab136e` |

For example, the DftSrt carrier
`tp5rq/r75w9s1v/6549pyqq9yz/64qrsq.le.lpb` has 757 bytes and SHA-256
`989D8F897DBE769090EF7D90EB35FBE4B061EF2314BBA24BE23D85C4BEA729B6`.
Its decoded 744-byte chunk has SHA-256
`72762BFBE567A34AF198BCF342F42581E2CE83ABB65A46A034A5472FE35A17E7`.
From an authorized `xivl-client-scripts` checkout, reproduce the decoded
chunk and canonical source with explicit private output paths:

```console
python tools/retail_script.py --input <client-script-root>/tp5rq/r75w9s1v/6549pyqq9yz/64qrsq.le.lpb --jar tools/vendor/unluac/unluac_2025_12_23.jar --decoded-out <private-output>/dftsrt.luac --script-out <private-output>/dftsrt.lua
```

Inspect the decoded chunk with the `ChunkReader` and `instruction` decoder
in `xivl-client-scripts:tools/build_quest_contracts.py`. Each listed offset
is `prototype["offset"] + 4 * PC`. For DftSrt, `root/proto2` starts at
`0x1F3` and has 20 code words. Check the original call/result operands,
rather than inferring returns from a decompiler's temporary variable names.

## Scene assets

The scene assets have city-specific departure (`*000`) and arrival
(`*010`) names. Their byte identities were checked directly:

| City | Scene | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Gridania | `zep0g000` | 111,520 | `6dc88355de9859a5ec0ad4ee856086eb37ebea6916acaf657d84649e1d11e893` |
| Gridania | `zep0g010` | 115,488 | `1958c9fadf4f147177f19d779ebf3e2c3df2bf86e0c01c4479e804e83eeaadbb` |
| Limsa Lominsa | `zep0l000` | 117,408 | `7b2009d319ebce90d41e9d7091a7e4bb4d0e10e8783b74c013b4a71b72a52197` |
| Limsa Lominsa | `zep0l010` | 159,648 | `5ab93053dfa7f8d0d71d27241a0c00ae0f89f803af9dd912345af135fa5715d6` |
| Ul'dah | `zep0u000` | 129,424 | `71634fb82d460530bbda822dea9bb5ea7b5e8154559788611a2cd376fdc6966f` |
| Ul'dah | `zep0u010` | 149,824 | `5923855356423f238a412bf578470e8d26d04cb034e5e1a201a52768cc205ee5` |

A direct parse of the six hash-matched scene assets finds actor dictionaries
and cutscene transforms. Those transforms are cinematic placements,
not authenticated public-zone spawns or arrival coordinates. A current
server route's choice of a departure/arrival pair is an implementation
decision unless joined to retail route evidence; asset presence alone does
not authenticate that choice.

## Parsed actor and scheduler boundary

A direct parse of the six hash-matched files yields 77 actor-dictionary records and
74 structurally plausible spatial records in total. The setup
stream accounts for 24 of those spatial records; the wider scan also finds
later timeline placements. These are parser counts, not a count of actors
simultaneously visible or proof of world-spawn locations.

| Scene | SCB file offset | Actor records | Setup spatial | Wider spatial | Inner block span sum |
| --- | ---: | ---: | ---: | ---: | ---: |
| `zep0g000` | `0x16498` | 8 | 5 | 16 | 8,800,000 |
| `zep0g010` | `0x19050` | 11 | 3 | 8 | 12,050,000 |
| `zep0l000` | `0x18840` | 14 | 6 | 14 | 8,100,000 |
| `zep0l010` | `0x22658` | 14 | 3 | 10 | 11,100,000 |
| `zep0u000` | `0x1BAA8` | 15 | 4 | 15 | 8,700,000 |
| `zep0u010` | `0x203D8` | 15 | 3 | 11 | 10,800,000 |

Each embedded `SEDBSCB` has a first `@CBLK` with raw span `9,000,000` and
zero clips. The last column sums the other `@CBLK` span words, read at
block offset `+0x24`; the clip count is at `+0x28`. These sums are not
playback durations: branch order, waits, and scene completion are not
reconstructed by this header read. In particular, the common outer
`9,000,000` value does not prove that each movie plays for nine seconds.
The actor and spatial counts use `decompile_airship_cutscene_setup.py`
against those six scene files. The block fields were read directly from
each hash-matched SCB.

## Scene-local actor dictionaries

The six hash-pinned scene assets contain 77 scene-local kind-1 dictionary
records. Each stores an index, a NUL-terminated label, and a four-byte value
at record offset +0x30. The table preserves the literal fields and offsets
read from those assets. These entries do not establish persistent world-actor
identities, route assignment, world placement, or playback.

| Scene | Index | Stored label | Stored value | Record offset |
| --- | ---: | --- | ---: | ---: |
| `zep0g000` | 4 | `PC` | 0 | `0x16628` |
| `zep0g000` | 5 | `Airship` | 1200090 | `0x16664` |
| `zep0g000` | 6 | `Memama` | 1001706 | `0x166A0` |
| `zep0g000` | 7 | `Pfarahr` | 1001707 | `0x166DC` |
| `zep0g000` | 8 | `Npc_hyuranosu` | 1000299 | `0x16718` |
| `zep0g000` | 9 | `Npc_Mikotte` | 1000786 | `0x16754` |
| `zep0g000` | 13 | `ZamuQ` | 1001711 | `0x16808` |
| `zep0g000` | 14 | `sentyou` | 1001781 | `0x16844` |
| `zep0g010` | 4 | `PC` | 0 | `0x191E0` |
| `zep0g010` | 5 | `Airship` | 1200090 | `0x1921C` |
| `zep0g010` | 6 | `Lionnellais` | 1500055 | `0x19258` |
| `zep0g010` | 7 | `Hida` | 1500056 | `0x19294` |
| `zep0g010` | 8 | `Memama` | 1001706 | `0x192D0` |
| `zep0g010` | 9 | `Pfarahr` | 1001707 | `0x1930C` |
| `zep0g010` | 10 | `Beaudonet` | 1001708 | `0x19348` |
| `zep0g010` | 11 | `Fryswyde` | 1001709 | `0x19384` |
| `zep0g010` | 12 | `Willielmus` | 1001710 | `0x193C0` |
| `zep0g010` | 13 | `QZamqo` | 1001711 | `0x193FC` |
| `zep0g010` | 17 | `sentyou` | 1001781 | `0x194A0` |
| `zep0l000` | 4 | `PC` | 0 | `0x189D0` |
| `zep0l000` | 5 | `Airship` | 1200090 | `0x18A0C` |
| `zep0l000` | 6 | `Ajin_Zukajin` | 1001700 | `0x18A48` |
| `zep0l000` | 7 | `Raplulu` | 1001701 | `0x18A84` |
| `zep0l000` | 8 | `G_Zentsa_Rhof` | 1001702 | `0x18AC0` |
| `zep0l000` | 9 | `Aldyet` | 1001703 | `0x18AFC` |
| `zep0l000` | 10 | `Murlskylt` | 1001704 | `0x18B38` |
| `zep0l000` | 11 | `Aenore` | 1001705 | `0x18B74` |
| `zep0l000` | 12 | `T_Noggya_Bham` | 1600093 | `0x18BB0` |
| `zep0l000` | 13 | `L_Nophlo` | 1500206 | `0x18BEC` |
| `zep0l000` | 14 | `Wineburg` | 1500207 | `0x18C28` |
| `zep0l000` | 15 | `Merchant023` | 1001493 | `0x18C64` |
| `zep0l000` | 16 | `Ruga` | 1000678 | `0x18CA0` |
| `zep0l000` | 19 | `sentyou` | 1001781 | `0x18D20` |
| `zep0l010` | 4 | `PC` | 0 | `0x227E8` |
| `zep0l010` | 5 | `Airship` | 1200090 | `0x22824` |
| `zep0l010` | 6 | `Ajin_Zukajin` | 1001700 | `0x22860` |
| `zep0l010` | 7 | `Raplulu` | 1001701 | `0x2289C` |
| `zep0l010` | 8 | `G_Zentsa_Rhof` | 1001702 | `0x228D8` |
| `zep0l010` | 9 | `Aldyet` | 1001703 | `0x22914` |
| `zep0l010` | 10 | `Murlskylt` | 1001704 | `0x22950` |
| `zep0l010` | 11 | `Aenore` | 1001705 | `0x2298C` |
| `zep0l010` | 12 | `T_Noggya_Bham` | 1600093 | `0x229C8` |
| `zep0l010` | 13 | `L_Nophlo` | 1500206 | `0x22A04` |
| `zep0l010` | 14 | `Wineburg` | 1500207 | `0x22A40` |
| `zep0l010` | 15 | `Merchant023` | 1001493 | `0x22A7C` |
| `zep0l010` | 16 | `Lady031` | 1001226 | `0x22AB8` |
| `zep0l010` | 20 | `sentyou` | 1001781 | `0x22B5C` |
| `zep0u000` | 4 | `PC` | 0 | `0x1BC38` |
| `zep0u000` | 5 | `Airship` | 1200090 | `0x1BC74` |
| `zep0u000` | 6 | `STANGYTH` | 1500208 | `0x1BCB0` |
| `zep0u000` | 7 | `LUNNIE` | 1500209 | `0x1BCEC` |
| `zep0u000` | 8 | `GUILLESTET` | 1001712 | `0x1BD28` |
| `zep0u000` | 9 | `H_CIDJAA` | 1001713 | `0x1BD64` |
| `zep0u000` | 10 | `AUTGAR` | 1001714 | `0x1BDA0` |
| `zep0u000` | 11 | `AHLDBYRT` | 1001715 | `0x1BDDC` |
| `zep0u000` | 12 | `NEYMI_FUNOMI` | 1001716 | `0x1BE18` |
| `zep0u000` | 13 | `GOODIFE` | 1001717 | `0x1BE54` |
| `zep0u000` | 14 | `LADY030` | 1001215 | `0x1BE90` |
| `zep0u000` | 15 | `PEDESTRIANU006` | 1001119 | `0x1BECC` |
| `zep0u000` | 16 | `PEDESTRIANU009` | 1001122 | `0x1BF08` |
| `zep0u000` | 17 | `ADVENTURER088` | 1001062 | `0x1BF44` |
| `zep0u000` | 20 | `sentyou` | 1001781 | `0x1BFC4` |
| `zep0u010` | 4 | `PC` | 0 | `0x20558` |
| `zep0u010` | 5 | `Airship` | 1200090 | `0x20594` |
| `zep0u010` | 6 | `STANGYTH` | 1500208 | `0x205D0` |
| `zep0u010` | 7 | `LUNNIE` | 1500209 | `0x2060C` |
| `zep0u010` | 8 | `GUILLESTET` | 1001712 | `0x20648` |
| `zep0u010` | 9 | `H_CIDJAA` | 1001713 | `0x20684` |
| `zep0u010` | 10 | `AUTGAR` | 1001714 | `0x206C0` |
| `zep0u010` | 11 | `AHLDBYRT` | 1001715 | `0x206FC` |
| `zep0u010` | 12 | `NEYMI_FUNOMI` | 1001716 | `0x20738` |
| `zep0u010` | 13 | `GOODIFE` | 1001717 | `0x20774` |
| `zep0u010` | 14 | `LADY030` | 1001215 | `0x207B0` |
| `zep0u010` | 15 | `PEDESTRIANU006` | 1001119 | `0x207EC` |
| `zep0u010` | 16 | `PEDESTRIANU009` | 1001122 | `0x20828` |
| `zep0u010` | 17 | `ADVENTURER088` | 1001062 | `0x20864` |
| `zep0u010` | 20 | `sentyou` | 1001781 | `0x208E4` |

Provenance: recovered
`quest/scenario/defaulttalk/dftsrt.lua:eventDeparture`,
`director/directorbaseclass.lua:delegateEvent`, and
`quest/questbaseclass_common.lua:startNQCutScene` establish the script
sequence. `client/cut/zep0*/zep0*` files supply the six sizes and
hashes and SCB offsets above identify the source scenes;
`decompile_airship_cutscene_setup.py` produced the parsed roles and
dictionaries from those hash-matched files.
No retail runtime probe or
new capture was used.
