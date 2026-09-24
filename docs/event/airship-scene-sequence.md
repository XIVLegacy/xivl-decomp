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
