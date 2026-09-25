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
