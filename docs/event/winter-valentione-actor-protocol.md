# Winter and Valentione actor protocol

The decoded quest, actor-class display, and marker rows preserve a Heavensturn
quest that continues Starlight dialogue. A separate Valentione client script
groups nine actor class IDs by numeric town type. Neither examined script
supplies a city BG scheduler or weather call; the Valentione class-path and
historical spawn joins are not established by these client rows.

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

`actorclass.csv` column 5 associates those three IDs with the listed display
IDs; it contains no client class-path field. The `115008xx` marker rows place
those display IDs on the listed city maps; they do not prove that the event
actor classes were spawned at those positions. `quest_new_reward.csv` row
110802 contains Dragon Kabuto item ID 8012604. This is a reward-sheet value,
not an observed grant or a recovered completion rule.

## Valentione numeric actor groups

`actorclass.csv` column 5 joins the nine actor class IDs below to their
display-name IDs. The installed `PopulaceValentMaster.getTownMasterType`
method independently compares the same IDs and returns numeric types 1, 2,
and 3 as recorded in
`xivl-client-scripts:docs/seasonal-event-actor-contracts.md`. The decoded
actor-class sheet does not contain a class-path binding or a city placement
field. The matching ID comparisons establish how that method would classify
an actor when invoked, not that every ID was registered to that Lua path at
retail runtime.

| ID family | Type 1 | Type 2 | Type 3 |
| --- | --- | --- | --- |
| 1001841-1001843 | 1001841 Nicolaa | 1001842 Olyffe | 1001843 Ophellia |
| 1001844-1001846 | 1001844 Rhela Nbolo | 1001845 Doyoh Lihzeh | 1001846 Qhom Jinjhal |
| 1001847-1001849 | 1001847 Loloju | 1001848 Momoga | 1001849 Popoka |

The recovered actor script has dialogue and widget methods, but it does not
establish historical spawning, city labels for the numeric types, a role for
each ID family, city decoration activation, or weather control. The examined
`Spl0i4` and `PopulaceValentMaster` scripts contain
character-scheduler calls but no `_runBgScheduler`, weather setter, or
`SpecialEventWork` access. That negative is limited to these scripts.

## Provenance

Quest identity is decoded `xtx_quest.csv` row 110802. The reward-sheet item
value is `quest_new_reward.csv` row 110802, joined to `xtx_itemName.csv` row
8012604. The city dialogue branches are the `Spl0i4.processEventHin` and
`processEventClearAfterItem` methods pinned in
`xivl-client-scripts:docs/seasonal-quest-client-contracts.md`, joined to
`spl0i4.csv` text rows 2-37. Representative names are
`xtx_displayName.csv` rows 1500154, 1300042, and 1600244. Marker IDs,
maps, and display joins are `quest_marker.csv` rows 11500801-11500803.
Actor-class ID to display-ID joins are `actorclass.csv` column 5, rows
1001824-1001826 and 1001841-1001849; the English names are
`xtx_displayName.csv` at those display IDs. These seven cited CSV files were read
from the canonical `2012.09.19.0001` corpus and matched their individual
`xivl-client-data:manifests/tables.json` SHA-256 entries. The numeric
Valentione method and its installed LPB identity are in
`xivl-client-scripts:docs/seasonal-event-actor-contracts.md`.
