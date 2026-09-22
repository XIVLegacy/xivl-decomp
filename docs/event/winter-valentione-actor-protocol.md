# Winter and Valentione actor protocol

The decoded quest and actor-class rows distinguish two all-city event
surfaces: a Heavensturn quest that continues Starlight dialogue, and a
Valentione actor protocol with three roles in each capital. Neither examined
script supplies a city BG scheduler or weather call.

## Heavensturn handoff

Quest 110802, `spl0i4` / `Gone with the Snow`, has a Japanese title beginning
with code points `U+964D U+795E U+796D` (Heavensturn). That prefix identifies
the event despite English
dialogue referring back to Starlight. `Spl0i4.processEventHin` and
`processEventClearAfterItem` each branch on city type 1, 2, or 3. The
decoded text rows and display names identify their representatives:

| City type | City | Representative / display ID | Actor class ID | Marker ID / map |
| ---: | --- | --- | ---: | --- |
| 1 | Limsa Lominsa | Ninipu / 1500154 | 1001824 | 11500801 / 121 |
| 2 | Gridania | Hastridie / 1300042 | 1001825 | 11500802 / 321 |
| 3 | Ul'dah | Wysskoen / 1600244 | 1001826 | 11500803 / 421 |

The actor-class rows associate those three IDs with the representatives'
display IDs, but their client class-path fields are empty. The `115008xx`
marker rows place the representatives on the listed city maps; they do not
prove that these event actor classes were spawned at those positions. No
matching local spawn row was found. The quest sheet lists Dragon Kabuto
(8012604) as the direct reward; the other three dragon-kabuto item variants
have no exact surviving grant flow in the examined source.

## Valentione city roles

The decoded actor-class rows bind all nine IDs below to
`/Chara/Npc/Populace/PopulaceValentMaster`. Its
`getTownMasterType` method independently compares those exact IDs and returns
1 for Limsa Lominsa, 2 for Gridania, and 3 for Ul'dah. This is an actor-ID to
client-script association, not an inference from a class file's existence.

| Role | Limsa Lominsa | Gridania | Ul'dah |
| --- | --- | --- | --- |
| Certifier / daily chocolate | 1001841 Nicolaa | 1001842 Olyffe | 1001843 Ophellia |
| Glory of Love attendant | 1001844 Rhela Nbolo | 1001845 Doyoh Lihzeh | 1001846 Qhom Jinjhal |
| Party matchmaker | 1001847 Loloju | 1001848 Momoga | 1001849 Popoka |

The nine rows have no matching local spawn rows in the examined SQL snapshot.
The recovered actor script implements the interaction roles, but does not
establish historical spawning, city decoration activation, or weather
control. The examined `Spl0i4` and `PopulaceValentMaster` scripts contain
character-scheduler calls but no `_runBgScheduler`, weather setter, or
`SpecialEventWork` access. That negative is limited to these scripts.

## Provenance

Quest identity is decoded `xtx_quest.csv` row 110802. Its direct Dragon
Kabuto reward is `quest_new_reward.csv` row 110802, joined to
`xtx_itemName.csv` row 8012604. The city
dialogue branches are recovered `quest/scenario/spl/spl0i4.lua` methods
`processEventHin` and `processEventClearAfterItem`, joined to
`spl0i4.csv` text rows 2-37. Representative names are
`xtx_displayName.csv` rows 1500154, 1300042, and 1600244. Marker IDs,
maps, and display joins are `quest_marker.csv` rows 11500801-11500803.
Actor-class ID, display ID, and client class-path fields are
`gamedata_actor_class.sql` rows 1001824-1001826 and 1001841-1001849.
Valentione's exact ID branches are recovered
`chara/npc/populace/populacevalentmaster.lua:getTownMasterType`.
`build_winter_valentione_actor_protocol_atlas.py` produced the reviewed
`heavensturn_city_branch_contract.csv`, `heavensturn_reward_contract.csv`,
and `valentione_city_role_contract.csv` joins; those outputs are derivations,
not separate retail observations.
