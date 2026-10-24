# Music song-ID names

This catalog proposes enum identifiers and display names for SetMusic song
IDs 1 through 109, with ID 0 recorded separately in the same table. The
resource key is `0x07FE0000 + id`. The
[resource table](music-song-table.md) owns the request arithmetic, retained
file identities and loader behavior. A proposed name does not establish a
zone default, successful playback or an official resource-to-title match.

## Evidence tiers

- `STATIC`: a numeric song argument or selection relationship in authenticated
  client Lua or client tables. A call does not establish execution or a title.
- `OBSERVED`: a retained packet request or resource measurement. A request
  does not prove audible identity, and a resource file does not supply a title.
- `INFERRED`: a proposed composition name or context interpretation supported
  by a labeled secondary lead or an incomplete join. A title lead lacks an
  authenticated title-to-resource match.

The table's tier qualifies its display label. For an unidentified or
stream-less entry, `OBSERVED` qualifies the resource status only. Additional
static calls or observed packets in the evidence column keep their own tiers
and do not upgrade a secondary title. Enum identifiers are editorial
PascalCase candidates, not names extracted from the executable.

## Sources and coverage

### Resource identities

`xivl-decomp:config/music_song_resources.json`, `install.songs` keyed by
`songId`, and `xivl-decomp:docs/resource/music-song-table.md#installation-table`
record the retained `2012.09.19.0001` installation. ID 0 was empty. IDs 7,
30, 31 and 32 had 400-byte SSCF definitions with no stream. ID 7 has shipped
Lua callers, and stream-less bank audibility remains unproved in the loader
finding.

### Lua callers

The decoded extraction `2012.09.19.0001` was searched case-insensitively for
`music|bgm|song` across its complete 2,671-path inventory, then the matching
helper names were followed. Eighteen selected source files matched canonical
LF sizes and SHA-256 values in
`xivl-client-scripts:manifests/scripts.json`, manifest SHA-256
`86798306f71336ee494f12d395db3b8ea571a21224fbd99e2ef87ecd18c61300`,
at repository revision `39f6b555101b6ec76907b58560adee80da455c56`.
Canonical `lua/scripts/` paths below identify decoded inputs through that
manifest; the Lua bodies are restricted inputs, not public tracked files.

`QuestBaseClass.setMusic` forwards song and mode to the player's `_setMusic`
unchanged (`xivl-client-scripts:lua/scripts/quest/questbaseclass_common.lua:664-673`).
The three native-name index references include that helper definition and
the two replay calls (`xivl-client-scripts:lua/napi_index.json`,
`apis._setMusic`). Following helper callers found these eleven literal uses:

| Song / raw mode | Decoded source citation | Static situation or scene context |
| --- | --- | --- |
| 7 / 2 | `xivl-client-scripts:lua/scripts/quest/scenario/man/man0g0.lua:123`; `xivl-client-scripts:lua/scripts/quest/scenario/man/man0u0.lua:407`; `xivl-client-scripts:lua/scripts/chara/npc/populace/populacecutsceneplayer.lua:221` | Before intro scenes `MAN0G020`/`MAN0G030`, `MAN0U020`, or a dynamically selected replay, respectively. |
| 13 / 1 | `xivl-client-scripts:lua/scripts/quest/scenario/man/man0g0.lua:1181,1303` | `processTtrBtl001` and `processTtrBtlMagic001`, after `MAN0G010` and fade-in, before tutorial camera/scheduler operations. |
| 21 / 1 | `xivl-client-scripts:lua/scripts/quest/scenario/man/man0l0.lua:1814,1930` | The corresponding two tutorial-battle methods, after `MAN0L010` and fade-in. |
| 25 / 1 | `xivl-client-scripts:lua/scripts/quest/scenario/man/man0u0.lua:1714,1836` | The corresponding two tutorial-battle methods, after `MAN0U010` and fade-in. |
| 29 / 1 | `xivl-client-scripts:lua/scripts/quest/scenario/etc/etc304.lua:448` | `processEventPray`, after `gc010850`; the recovered branch tests argument 3 equal to 1 and special-event work index 9 equal to 20. |
| 61 / 2 | `xivl-client-scripts:lua/scripts/chara/npc/populace/populacecutsceneplayer.lua:316` | Replay cleanup after scene playback and replay-quest deletion. This does not establish every inn's default music. |

These are `STATIC` observations of authenticated decompiler text.
Original-bytecode branch reachability, execution and audible identity remain
unverified by these observations. This name-based search does not exclude anonymous
or dynamic routes, native-only callers or music selected inside scene assets.

The literal 61 in `startCutScene(1,61,1)` is not song evidence. In
`xivl-client-scripts:lua/scripts/chara/npc/object/musicchange.lua:52` and
`xivl-client-scripts:lua/scripts/judge/preface/cutsceneoncebeaconprefacejudge.lua:31`,
it is the cutscene helper's desktop-widget mode. The helper forwards that
argument to widget ordering/cancellation, while `_play` receives only varargs
(`xivl-client-scripts:lua/scripts/gamedata/cutscene_common.lua:1064,1144,1256-1258,1289`).

### Packet observations

All 54 members in `xivl-captures:sources/pcap-1.23b/manifest.yaml` were read
and matched their exact sizes and SHA-256 values. The manifest SHA-256 was
`baaa15f99f7f5d865bf2ec0dbcc1e454d034382b0559edccb0085c57c4061cf5`,
at capture repository revision `0891d86152749863c0e64eb7d09a8f88086307c4`.
This pins each filename and its member digest below. The archive container
was not rehashed.

The admitted game-domain census found 26 `0x000C` requests across 13 members,
using 12 song IDs and 13 song/mode pairs. All were connection 0, main lane,
server-to-client, with a 40-byte type-3 subevent containing a 24-byte game
packet. Song is u16 little-endian at game `+0x10`; raw mode is u8 at `+0x12`
(`xivl-decomp:docs/net/music-control-runtime.md#wire-fields`).

Reconstruction followed `xivl-captures:docs/pcap-decoding.md#decode-contract`
and `xivl-captures:tools/extractors/extract_streams.py`. It inspected 84
admitted connections in both directions, including chat: 19,760 outer frames
and 65,864 subevents. Canonical framing checks passed for all members.
Admitted directions had no internal IP-declared sequence gap, conflicting
overlap, body remainder, compression mismatch or IP-declared trailing bytes.
Six raw link-padding bytes were excluded in 38 main/s2c directions; they were
not missing application tails. Non-game traffic, including TLS and lobby,
stayed outside this census. This is coverage of the retained admitted domain,
not every historical session or uncaptured interval.

The six witnesses in
`xivl-captures:studies/music-000c-wire/derived/evidence-map.md` were reproduced.
The full locators below extend that selected study. Each filename is a member
row in the pinned source manifest. Connection/frame ordinals are zero-based;
completing capture-packet numbers are one-based. Stream offsets belong to the
connection direction, and subevent offsets to the decoded frame body. They
are not pcap-file offsets. All rows have `OBSERVED` packet confidence.

| Member | Song/mode | Frame | Stream offset | Decoded subevent offset | Completing packet |
| --- | --- | ---: | --- | ---: | ---: |
| `combat_autoattack.pcapng` | 13/1 | 18 | `0x7FA` | 0 | 62 |
| `combat_autoattack.pcapng` | 57/4 | 128 | `0x4282` | 0 | 283 |
| `combat_skills.pcapng` | 13/1 | 25 | `0xB81` | 168 | 85 |
| `combat_skills.pcapng` | 57/4 | 85 | `0x253F` | 0 | 177 |
| `from_gridania_to_blackshroud.pcapng` | 57/4 | 233 | `0x620D` | 80 | 755 |
| `gridania_to_coerthas.pcapng` | 57/4 | 173 | `0x3DD9` | 0 | 590 |
| `gridania_to_coerthas.pcapng` | 83/4 | 202 | `0x6137` | 0 | 670 |
| `gridania_to_coerthas.pcapng` | 55/4 | 1356 | `0x37020` | 0 | 4119 |
| `local_leve_complete.pcapng` | 73/4 | 83 | `0x28BD` | 64 | 282 |
| `local_leve_complete.pcapng` | 57/4 | 194 | `0x6522` | 0 | 658 |
| `local_leve_complete.pcapng` | 73/4 | 206 | `0x6A9E` | 0 | 692 |
| `local_leve_complete.pcapng` | 57/4 | 361 | `0xBE64` | 0 | 1240 |
| `local_leve_complete.pcapng` | 73/4 | 372 | `0xC3D1` | 0 | 1291 |
| `local_leve_complete.pcapng` | 57/4 | 463 | `0xF635` | 0 | 1581 |
| `login.pcapng` | 61/1 | 8 | `0x385` | 40 | 961 |
| `mount_unmount_chocobo.pcapng` | 83/4 | 2 | `0x74` | 0 | 18 |
| `mount_unmount_chocobo.pcapng` | 57/4 | 13 | `0x3D2` | 0 | 81 |
| `move_out_of_room.pcapng` | 51/4 | 29 | `0x8FB` | 0 | 87 |
| `party_battle_leve.pcapng` | 14/1 | 112 | `0x38C3` | 0 | 202 |
| `party_battle_leve.pcapng` | 12/1 | 2870 | `0x5674D` | 0 | 4523 |
| `party_battle_leve.pcapng` | 81/1 | 3350 | `0x65536` | 0 | 5203 |
| `party_battle_leve.pcapng` | 52/4 | 3448 | `0x6803E` | 0 | 5444 |
| `return_to_inn.pcapng` | 61/4 | 36 | `0xB32` | 0 | 139 |
| `teleport_to_camp_nine_ivies.pcapng` | 52/4 | 69 | `0x161E` | 80 | 304 |
| `teleport_to_gridania.pcapng` | 51/4 | 55 | `0x18BC` | 80 | 179 |
| `war_quest_update2.pcapng` | 67/4 | 637 | `0xEA33` | 80 | 837 |

#### Zone, area and situation limits

The following are `OBSERVED` wire-order relationships. An active-map or
destination association is `INFERRED`. SetMap application `+0x04` remains an
observed numeric field: the inspected contract does not establish that it
indexes `_zoneParam`
(`xivl-captures:studies/director-wire-identity/derived/zone-controller-identity.md`).
Thus no numeric SetMap value below is an authenticated geographic assignment.
Member filenames are situation leads only. Area, exact battle/event
eligibility, and day/night remain unknown for every packet row.

| Music witness | Observed connection-local context | Interpretation |
| --- | --- | --- |
| `login.pcapng`, f8, 61/1 | Earlier SetMap f2/+0 has tuple `[209,244,40,0]`; later `ZoneMasterPrvI0` appears at f19/+384. | Preceding numeric 244; active-map interpretation and login/room situation are inferred. |
| `gridania_to_coerthas.pcapng`, f202, 83/4 | SetMap f175/+0 has `[103,150,40,0]`; `ZoneMasterFstF0` at f186/+1584. Next SetMap is f844. | Preceding numeric 150 with no intervening SetMap or internal stream gap; active map and mount situation are inferred. |
| `gridania_to_coerthas.pcapng`, f1356, 55/4 | SetMap f1319/+0 has `[102,145,40,0]`; `ZoneMasterRocR0` at f1330/+1584. | Preceding numeric 145 with no intervening SetMap or internal stream gap; active-map interpretation is inferred. |
| `from_gridania_to_blackshroud.pcapng`, f233, 57/4; `gridania_to_coerthas.pcapng`, f173, 57/4 | Later SetMap f235 or f175 has numeric 150; later `ZoneMasterFstF0` at f247/+1432 or f186/+1584. | Upcoming numeric-150 transition. Destination music association is inferred; current map is unknown. |
| `move_out_of_room.pcapng`, f29, 51/4 | Later SetMap f30 has numeric 155; `ZoneMasterFstF0` at f42/+1432. | Upcoming numeric-155 transition; room-exit context and destination music association are inferred. |
| `return_to_inn.pcapng`, f36, 61/4 | Later SetMap f37 has numeric 244; `ZoneMasterPrvI0` at f49/+624. | Upcoming numeric-244 transition; inn context and destination music association are inferred. |
| `teleport_to_camp_nine_ivies.pcapng`, f69, 52/4 | Later SetMap f72 has numeric 151; `ZoneMasterFstF0` at f83/+1584. | Upcoming numeric-151 transition. Named destination and music association are inferred. |
| `teleport_to_gridania.pcapng`, f55, 51/4 | Later SetMap f58 has numeric 206; `ZoneMasterFstF0` at f69/+2608. | Upcoming numeric-206 transition. Named destination and music association are inferred. |
| `war_quest_update2.pcapng`, f637, 67/4 | No SetMap retained; later `ZoneMasterWilW0` at f639/+1208. | Role-class witness only; numeric map and exact quest situation unknown. |
| Both combat members, 13/1 then 57/4 | No SetMap/ZoneMaster retained in either member. | Combat situation is a filename lead. The cross-member numeric-154 candidate remains blocked by missing TCP spans. |
| `mount_unmount_chocobo.pcapng`, 83/4 then 57/4 | No SetMap/ZoneMaster retained. | Mount situation is a filename lead; active map unknown. |
| `local_leve_complete.pcapng`, all six rows | Alternation 73/4 -> 57/4 occurs three times; no SetMap/ZoneMaster retained. | Exact leve/event and map unknown. |
| `party_battle_leve.pcapng`, all four rows | 14/1 -> 12/1 -> 81/1 -> 52/4; no SetMap/ZoneMaster retained. | Battle/leve situation is a filename lead; exact event and map unknown. |

The earlier numeric-154 candidate cannot be carried into the combat files:
`xivl-captures:studies/music-000c-wire/derived/map-context.md#nearest-retained-map-candidates`
records 35,811/44,008 missing bytes across those cross-member joins. No
connections or member files were concatenated for the census above.

### Zone and area sheets

No music-ID column or song-name join was established. At client-data revision
`492c0e2701747bf63e91cb7ce419e9743f63c3f5`, all 803 canonical CSV names and
798 sheet-inventory names were searched for `music|bgm|song|audio|sound`,
and all 803 hydrated typed headers for `music|bgm|song`. No matching sheet
name or header was found. The 25 selected geography, replay, map/navigation,
aetheryte, regional-text and debug sheets matched exact sizes and SHA-256
values in `xivl-client-data:manifests/tables.json`. Broader header/cell
searches were discovery checks, not authentication of every CSV.

The selected geographic columns were structural: `_zoneParam` place key,
`zoneGroupParam` place key and zone children, `regionParam` region/group
range, `_region` region code, and `_layout` usage/place links. Their owners
are `xivl-client-data:tools/mappings/zones.py:17-18,192-224`,
`xivl-client-data:docs/zone-group-place-rows.md` and
`xivl-client-data:tools/build_zone_name_catalog.py:115-157`. Replay column 0
was a scene key, not a song ID
(`xivl-client-data:csv/cutReplay.csv`, row 11000101, logical column 0;
`xivl-client-scripts:docs/cutscene-replay-skip-contract.md:173-181,204-215`).

`xivl-client-data:tools/mappings/zones.py:248-250` defines `dayMusic`,
`nightMusic` and `battleMusic` as server output fields with `const:0` and no
CSV source. These are not retail day/night/battle defaults. Text discovery
found 214 matching cells in 86 sheets, all declared `str`; textual mentions
did not identify a numeric music consumer. In particular,
`xivl-client-data:csv/debugCommand.csv`, row 30008, logical columns 0 and 5,
describes a music test, while no song-ID meaning was established for its
numeric fields. Anonymous numeric fields remain untraced; matching a value
in 1-109 does not identify a music column.

The packet geography candidates have these separate `STATIC` sheet joins:

| Conditional zone input | Literal sheet join | Geographic label |
| --- | --- | --- |
| 145 | `xivl-client-data:csv/_zoneParam.csv`, row 145 col 0 = 4006; `xivl-client-data:csv/xtx_placeName.csv`, row 4006 col 1 | Coerthas Eastern Lowlands |
| 150 | `xivl-client-data:csv/_zoneParam.csv`, row 150 col 0 = 2007; `xivl-client-data:csv/xtx_placeName.csv`, row 2007 col 1 | Central Shroud |

Columns here are zero-based logical positions after the CSV row ID. Applying
these labels to SetMap requires the missing packet-field-to-zone-table edge.
Neither the song 55/145 nor 83/150 observation closes it. The Central Shroud
place key is shared by zone rows 150,162,164-168, so it also cannot choose a
unique layout instance. These are geography candidates, not accepted music
defaults or proof of subarea/day/night selection.

### Secondary track-list leads

The per-ID title lead is
`xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`,
located by `music_id`. Its source is the FFXIV Classic Wiki Music List,
revision 1196, source Last-Modified `Wed, 02 Jun 2021 07:09:14 GMT`, HTML
SHA-256 `53ec28c33e7134de4e5417dc76259afc0f66028cdc5aba19bdb52e9c9239ccdb`.
The source identity and limits are recorded in
`xivl-captures:sources/classic-music-reference/manifest.yaml` and
`xivl-captures:studies/classic-music-reference/derived/evidence-map.md`.
This later community crosswalk is not a retail-era observation. Its title,
usage and patch claims remain leads.

The official [Before Meteor soundtrack track list](https://www.jp.square-enix.com/music/sem/page/ff14/BM/)
and [album entry](https://www.jp.square-enix.com/music/lineup/item/SQEX-20012.html)
identify the 2013-08-14 soundtrack release. Album sequence numbers are not
SetMusic IDs. The list supplies soundtrack context, not a numeric resource
join, and does not raise any catalog row to `STATIC`.

## Catalog

Each `W(id)` below expands to the stable citation
`xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`,
row `music_id=id`. It means a secondary naming lead with `INFERRED` title
confidence. Resource-only rows cite their `install.songs` entry directly.

| ID | Proposed enum identifier | Display name | Tier | Evidence |
| ---: | --- | --- | --- | --- |
| 0 | `EmptyResource` | Empty resource | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=0]`: 0 bytes, 0 stream(s); composition unidentified. |
| 1 | `MarchOfTheMoogles` | March of the Moogles | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=1`. |
| 2 | `IntoTheAddersDen` | Into the Adder's Den | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=2`. |
| 3 | `MaelstromCommand` | Maelstrom Command | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=3`. |
| 4 | `TheHallOfFlames` | The Hall of Flames | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=4`. |
| 5 | `TheDarksEmbrace` | The Dark's Embrace | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=5`. |
| 6 | `TheDarksKiss` | The Dark's Kiss | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=6`. |
| 7 | `Streamless7` | Stream-less resource | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=7]`: 400 bytes, 0 stream(s); composition unidentified. STATIC [decoded Lua use](#lua-callers). |
| 8 | `BattleDrums` | Battle Drums | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=8`. |
| 9 | `TheSeventhGate` | The Seventh Gate | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=9`. |
| 10 | `Nemesis` | Nemesis | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=10`. |
| 11 | `PhantomsOnTheLake` | Phantoms on the Lake | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=11`. |
| 12 | `BeneathBloodiedBanners` | Beneath Bloodied Banners | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=12`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 13 | `TheForestsPulse` | The Forest's Pulse | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=13`. STATIC [decoded Lua use](#lua-callers). OBSERVED [packet request](#packet-observations); context limits apply. |
| 14 | `BathedInWoodsin` | Bathed in Woodsin | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=14`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 15 | `PennonsAloft` | Pennons Aloft | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=15`. |
| 16 | `StarlightAndSellswords` | Starlight and Sellswords | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=16`. |
| 17 | `Tempest` | Tempest | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=17`. |
| 18 | `Fury` | Fury | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=18`. |
| 19 | `Imperium` | Imperium | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=19`. |
| 20 | `Unknown20` | Unidentified | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=20]`: 1830156 bytes, 1 stream(s); composition unidentified. W(20) is tentative and questions the name/variant. |
| 21 | `NailOfTheHeavens` | Nail of the Heavens | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=21`. STATIC [decoded Lua use](#lua-callers). |
| 22 | `InTheShadowOfTheColossus` | In the Shadow of the Colossus | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=22`. |
| 23 | `PrimalJudgment` | Primal Judgment | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=23`. |
| 24 | `SteelReason` | Steel Reason | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=24`. |
| 25 | `Quicksand` | Quicksand | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=25`. STATIC [decoded Lua use](#lua-callers). |
| 26 | `DesertMoonDefied` | Desert Moon Defied | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=26`. |
| 27 | `NoQuarter` | No Quarter | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=27`. |
| 28 | `PreludeRemembrance` | Prelude - Remembrance | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=28`. |
| 29 | `Answers` | Answers | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=29`. STATIC [decoded Lua use](#lua-callers). |
| 30 | `Streamless30` | Stream-less resource | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=30]`: 400 bytes, 0 stream(s); composition unidentified. W(30) is an Answers/removal lead, not audible identity. |
| 31 | `Streamless31` | Stream-less resource | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=31]`: 400 bytes, 0 stream(s); composition unidentified. |
| 32 | `Streamless32` | Stream-less resource | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=32]`: 400 bytes, 0 stream(s); composition unidentified. W(32) is a descriptive opening-movie/removal lead, not a title. |
| 33 | `WithoutShadow` | Without Shadow | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=33`. |
| 34 | `WrathOfTheEikons` | Wrath of the Eikons | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=34`. |
| 35 | `ImperialWill` | Imperial Will | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=35`. |
| 36 | `FeverDream` | Fever Dream | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=36`. |
| 37 | `DaringDalliances` | Daring Dalliances | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=37`. |
| 38 | `FragmentsOfForever` | Fragments of Forever | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=38`. |
| 39 | `PieceOfMind` | Piece of Mind | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=39`. |
| 40 | `TheEcho` | The Echo | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=40`. |
| 41 | `FromTheHeart` | From the Heart | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=41`. |
| 42 | `InnerRecess` | Inner Recess | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=42`. |
| 43 | `Neverborn` | Neverborn | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=43`. |
| 44 | `FourteenSteps` | Fourteen Steps | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=44`. |
| 45 | `Tranquility` | Tranquility | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=45`. |
| 46 | `SacredBonds` | Sacred Bonds | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=46`. |
| 47 | `Conflagration` | Conflagration | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=47`. |
| 48 | `SupplyAndDemand` | Supply & Demand | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=48`. |
| 49 | `TearsForMorDhona` | Tears for Mor Dhona | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=49`. |
| 50 | `WhereTheHeartIs` | Where the Heart Is | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=50`. |
| 51 | `BornOfTheBoughsTheThemeOfGridania` | Born of the Boughs - The Theme of Gridania | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=51`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 52 | `EmeraldLabyrinth` | Emerald Labyrinth | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=52`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 53 | `Freedom` | Freedom | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=53`. |
| 54 | `HolyConsult` | Holy Consult | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=54`. |
| 55 | `Unspoken` | Unspoken | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=55`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 56 | `FlightlessWings` | Flightless Wings | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=56`. |
| 57 | `WhisperOfTheLand` | Whisper of the Land | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=57`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 58 | `Enraptured` | Enraptured | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=58`. |
| 59 | `NavigatorsGloryTheThemeOfLimsaLominsa` | Navigator's Glory - The Theme of Limsa Lominsa | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=59`. |
| 60 | `OnWindyMeadows` | On Windy Meadows | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=60`. |
| 61 | `BehindClosedDoors` | Behind Closed Doors | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=61`. STATIC [decoded Lua use](#lua-callers). OBSERVED [packet request](#packet-observations); context limits apply. |
| 62 | `Widdershins` | Widdershins | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=62`. |
| 63 | `AetherialSlumber` | Aetherial Slumber | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=63`. |
| 64 | `BoDown` | Bo-down | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=64`. |
| 65 | `HorizonsCalling` | Horizons Calling | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=65`. |
| 66 | `TheTwinFacesOfFateTheThemeOfUldah` | The Twin Faces of Fate - The Theme of Ul'dah | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=66`. |
| 67 | `TwilightOverThanalan` | Twilight over Thanalan | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=67`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 68 | `WithTheseHands` | With these Hands | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=68`. |
| 69 | `Decisions` | Decisions | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=69`. |
| 70 | `OpeningTheme` | Opening Theme | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=70`. |
| 71 | `NaturesBounty` | Nature's Bounty | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=71`. |
| 72 | `WhenATreeFalls` | When a Tree Falls | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=72`. |
| 73 | `ByDesign` | By Design | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=73`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 74 | `StarlightCelebration` | Starlight Celebration | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=74`. |
| 75 | `AllSaintsWake` | All Saint's Wake | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=75`. |
| 76 | `Heavensturn` | Heavensturn | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=76`. |
| 77 | `MoonfireFaire` | Moonfire Faire | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=77`. |
| 78 | `EverbindingOath` | Everbinding Oath | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=78`. |
| 79 | `CrowningAchievements` | Crowning Achievements | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=79`. |
| 80 | `FinalRespite` | Final Respite | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=80`. |
| 81 | `VictoryFanfareFull` | Victory Fanfare (Full) | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=81`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 82 | `VictoryFanfareShort` | Victory Fanfare (Short) | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=82`. |
| 83 | `EorzeaDeChocobo` | Eorzea de Chocobo | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=83`. OBSERVED [packet request](#packet-observations); context limits apply. |
| 84 | `Airborne` | Airborne | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=84`. |
| 85 | `DewdropsAndMoonbeams` | Dewdrops & Moonbeams | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=85`. |
| 86 | `RipplesInTheSea` | Ripples in the Sea | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=86`. |
| 87 | `Canticle` | Canticle | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=87`. |
| 88 | `TheSandsSecrets` | The Sands' Secrets | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=88`. |
| 89 | `DreamsAloft` | Dreams Aloft | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=89`. |
| 90 | `QuickAsSilverHardAsStone` | Quick as Silver, Hard as Stone | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=90`. |
| 91 | `BirdsOfAFeather` | Birds of a Feather | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=91`. |
| 92 | `FleeTogether` | Flee Together | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=92`. |
| 93 | `HonorAndDuty` | Honor and Duty | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=93`. |
| 94 | `Pitfire` | Pitfire | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=94`. |
| 95 | `ThePromiseOfPlunder` | The Promise of Plunder | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=95`. |
| 96 | `Unknown96` | Unidentified | OBSERVED | `xivl-decomp:config/music_song_resources.json`, `install.songs[songId=96]`: 1677448 bytes, 1 stream(s); composition unidentified. W(96) is an explicit unknown. |
| 97 | `SevenJesters` | Seven Jesters | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=97`. |
| 98 | `TheRidersBoon` | The Rider's Boon | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=98`. |
| 99 | `GoodKingMoggleMogXII` | Good King Moggle Mog XII | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=99`. |
| 100 | `Meteor` | Meteor | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=100`. |
| 101 | `TheHeroOfHatchingtide` | The Hero of Hatchingtide | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=101`. |
| 102 | `BreakingBoundaries` | Breaking Boundaries | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=102`. |
| 103 | `Relics` | Relics | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=103`. |
| 104 | `FallenAngel` | Fallen Angel | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=104`. |
| 105 | `ToTheFore` | To the Fore | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=105`. |
| 106 | `TheTugOfFate` | The Tug of Fate | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=106`. |
| 107 | `InTheArmsOfAlthyk` | In the Arms of Althyk | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=107`. |
| 108 | `RiseOfTheWhiteRaven` | Rise of the White Raven | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=108`. |
| 109 | `AgentOfInquiry` | Agent of Inquiry | INFERRED | Secondary naming lead: `xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`, `music_id=109`. |

## Unidentified and conflicting entries

- Streamed IDs **20 and 96 remain unidentified**. The wiki questions ID 20's
  name and suggests a variant of 5. That suggestion is not adopted as a name
  or enum alias. ID 96 is explicitly unknown in the source.
- ID **0** is empty. IDs **7, 30, 31 and 32** have no audio stream in the
  retained resources. Their enum identifiers describe that measured status,
  with no composition title assigned. The wiki's Answers label for 30 and
  descriptive opening-movie label for 32 remain secondary leads. Its removal
  wording does not mean the retained 1.23b files were absent.
- The other 103 rows have proposed composition names from the secondary
  crosswalk. None has an authenticated audio/title join in this evidence set.
  A static script call or observed numeric packet does not close that join.

## Reproduction and limits

Read each canonical manifest before selecting its explicitly hydrated inputs.
For Lua, search `music|bgm|song`, follow `QuestBaseClass.setMusic`, and compare
the selected files' canonical LF byte counts and SHA-256 values with
`xivl-client-scripts:manifests/scripts.json`. Check the cited argument positions
and owning methods rather than interpreting every nearby number as a song.

Read the secondary crosswalk by `music_id`, retaining its confidence and
patch qualifiers. The catalog uses its spelling for proposed display names;
enum candidates remove punctuation, capitalize word boundaries and expand
`&` to `And`. Unknown and resource-only entries keep explicit placeholders.
Compare special entries with `xivl-decomp:config/music_song_resources.json`,
`install.songs`, rather than with a wiki removal claim. No audio was extracted,
decoded, played or identified by listening for this catalog.
