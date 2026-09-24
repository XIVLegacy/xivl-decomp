# Dungeon layout timeline corpus

The FFXIV 1.23b client contains 24 regional dungeon layout DATs.
A typed decode of every timeline node found 332 nodes: 299 compiled SCB
packages and 33 intentional zero-pointer, zero-size stubs.

The compiled packages contain 1,433 clip entries and 1,116 controlled actors.
The actor population is 20 system actors, 298 unit actors, and 798 unit-member
actors. The 19 decoded clip classes include:

| Clip class | Count |
| --- | ---: |
| `LayTransformClip` | 410 |
| `LayCollisionOnOffClip` | 408 |
| `LaySEClip` | 254 |
| `LayVFXClip` | 106 |
| `LayRapturePointLightClip` | 51 |
| `LaySetPosClip` | 48 |
| `ShowHideClip` | 30 |
| `IfClip` | 24 |
| `LaySEFadeOutClip` | 24 |
| `LayRaptureCallSchedulerClip` | 19 |

The remaining nine classes account for 59 clips. Raw flags, start units,
tracks, clip IDs, payload bytes, controlled actors, and active durations were
retained during the decode rather than normalized away.

## Raw string locator index

The companion [dungeon layout token index](dungeon-layout-token-index.csv)
records 8,373 selected `(token class, string)` rows across the 24
hash-pinned SqPack DAT files used by this corpus; their occurrence counts sum
to 9,849. Each row pins the DAT key, relative path, SHA-256, first byte offset,
and number of occurrences. The scanner reads each DAT file as stored and does
not decompress or resolve individual SqPack resources. Offsets are relative to
the complete DAT file, and repeated strings retain only their first offset.

`build_dungeon_layout_token_index.py` regenerates the CSV from a local client
installation and rejects any DAT whose hash differs from the pinned inputs.
It scans printable ASCII runs of at least four bytes and retains selected
timeline/control names plus keyword-matched resource-like strings. This proves
raw byte presence in the indexed DAT files only. Its string categories are
lexical locator aids, not decoded ownership edges: presence, proximity, and
names do not establish a resource-entry mapping, timeline link, actor owner,
activation, or runtime behavior.

## Per-layout coverage

| Layout | Internal name | Known content | Compiled | Stubs | Clips |
| ---: | --- | --- | ---: | ---: | ---: |
| 111 | `sea0Dungeon01` | Mistbeard Cove | 17 | 2 | 81 |
| 112 | `sea0Dungeon02` | unresolved | 7 | 0 | 39 |
| 113 | `sea0Dungeon03` | Cassiopeia Hollow | 12 | 0 | 32 |
| 114 | `sea0Dungeon04` | Shposhae map pages | 21 | 0 | 47 |
| 115 | `sea0Dungeon05` | unresolved | 7 | 0 | 37 |
| 116 | `sea0Dungeon06` | U'Ghamaro Mines | 18 | 0 | 194 |
| 211 | `roc0Dungeon01` | Dzemael Darkhold | 18 | 3 | 93 |
| 212 | `roc0Dungeon02` | unresolved | 8 | 0 | 47 |
| 213 | `roc0Dungeon03` | unresolved | 13 | 0 | 65 |
| 214 | `roc0Dungeon04` | The Aurum Vale | 11 | 0 | 48 |
| 215 | `roc0Dungeon05` | unresolved | 20 | 0 | 88 |
| 216 | `roc0Dungeon06` | unresolved | 10 | 0 | 50 |
| 311 | `fst0Dungeon01` | The Mun-Tuy Cellars | 6 | 2 | 30 |
| 312 | `fst0Dungeon02` | The Tam-Tara Deepcroft | 12 | 4 | 66 |
| 313 | `fst0Dungeon03` | The Thousand Maws of Toto-Rak | 12 | 2 | 58 |
| 314 | `fst0Dungeon04` | unresolved | 16 | 8 | 55 |
| 315 | `fst0Dungeon05` | unresolved | 7 | 4 | 31 |
| 316 | `fst0Dungeon06` | unresolved | 22 | 6 | 88 |
| 411 | `wil0Dungeon01` | unresolved | 11 | 0 | 52 |
| 412 | `wil0Dungeon02` | Nanawa Mines | 23 | 2 | 100 |
| 413 | `wil0Dungeon03` | Eastern Thanalan (place-name; duty unresolved) | 5 | 0 | 31 |
| 414 | `wil0Dungeon04` | Copperbell Mines | 10 | 0 | 50 |
| 415 | `wil0Dungeon05` | Cutter's Cry | 11 | 0 | 47 |
| 416 | `wil0Dungeon06` | unresolved | 2 | 0 | 4 |

The Shposhae annotation is a map-page association, not a historical
instance dispatch. Pinned `xivl-client-data:csv/_zoneParam.csv` joins zone
235 to place 1122 (Shposhae); `mapNavi_data.csv` rows 5000 and 5002-5005
join that place to layout 114. The full direct row provenance is in
`xivl-client-data:docs/shposhae-map-identity.md`. No equivalent pinned row
in this evidence names layout 112 as Shposhae.

Each DAT's identity is fixed by its layout family and
content hash. Notable examples are U'Ghamaro layout 116,
`d8ff9874f392367fa111a75997915257a465bf8fc4c613cafa035cd12f6e733a`;
Dzemael layout 211,
`98bc9d3a0111de81a95b01a3e1c9a94453f51f11169634eea0c80f97d329b368`;
Tam-Tara layout 312,
`360bef1ab5917e3d2e87e4a946c3fc848a7d76ced5159e21a5bb01af1cd4f111`;
and layout 413,
`c669f65d3f0897f8c328ebe51f2dbc8417dcfb9ec327af49eba14de9320afa06`.

The canonical client-data catalog records layouts 413 and 415 as
`wil0Dungeon03`/`wil0Dungeon05`, with `placeNameId` 3002/3123 and MapNavi
rows 4400/5400. Pinned `xtx_placeName.csv` rows 3002 and 3123 name Eastern
Thanalan and Cutter's Cry. Pinned `mapNavi_data.csv` row 4400 identifies
region 104/layout 413 but carries placeholder place-name IDs; rows 5400-5404
and 5411-5422 identify region 104/layout 415 and use place-name ID 3123.
The `_layout.csv`, `mapNavi_data.csv`, and `xtx_placeName.csv` hashes are
`2fd242794ec24288b8d4f6f54878b6d8f9241a80eea7a4ce4f2e62df0286ba88`,
`a33f166fe9ec1ced44f2c614f849c295113352e9f8b7b03b7ecb818a53925a3f`, and
`81467ef42e8aeba82fe95f6c4249356550c02e41e6734abf9fdd194051dc1714`,
respectively.
The decoded CSV corpus is pinned by
`xivl-client-data:manifests/private_csv_corpus.json` at archive SHA-256
`006f9438a8cfd9277376f0ab28474500c67e4665050aa631cae64c9e6f38a5b0`.
The layout DATs are `0x615A000B` at SHA-256
`c669f65d3f0897f8c328ebe51f2dbc8417dcfb9ec327af49eba14de9320afa06` and
`0x615A000D` at SHA-256
`aaa14a56cb812cc82f91ab62b3df01c61a2fe6d9e95b74d7ca0df3a4c3fd3df4`.
These rows support static map/layout place-name associations. Layout 413's
place-name row does not identify its dungeon duty; layout 415's row directly
names Cutter's Cry. The map/layout rows do not establish historical instance
dispatch or which zone row selected either layout.

## Paired door-timeline placements

A fresh typed extraction of the layouts matched the saved
`timeline_instance_placements.csv` and `timeline_unit_tree_owners.csv`
byte-for-byte. The scanner follows each compiled timeline's direct UnitTree
member target pointer and then reads that owner's serialized `isgrp` instance
placements. In the nine layouts below, 29 door-named owner groups have exact
matching instance sets for their `open` and `clos` timelines. The extractor
was `build_dungeon_layout_animation_atlas.py`, source SHA-256
`3f7e3ee6486d95ab0b654e202936b834b60a048b1a2053327c4ad87c7902658d`.

| Layout / DAT SHA-256 | Distinct paired `isgrp` instance IDs |
| --- | --- |
| 111 / `data/29/D9/00/08.DAT` / `4574985fb2d1e4b6068411350375777dd9d257fa2c5111f1de53a775172e67ec` | 3616-3617, 3619-3627 |
| 112 / `data/29/D9/00/09.DAT` / `0fdba99375f3282d58286d98ecb2af734170ff3cfb3369967b569488c8df46c2` | 3679-3690 |
| 116 / `data/29/D9/00/0D.DAT` / `d8ff9874f392367fa111a75997915257a465bf8fc4c613cafa035cd12f6e733a` | 487-488, 490, 492-501, 1095, 1114, 1116, 1124, 1132, 1280, 1315, 1333, 1585 |
| 211 / `data/28/D9/00/06.DAT` / `98bc9d3a0111de81a95b01a3e1c9a94453f51f11169634eea0c80f97d329b368` | 1406, 1408-1412, 1418, 1486, 1493-1496 |
| 214 / `data/28/D9/00/09.DAT` / `a4fdee1c2f4767aefd204a118173155cb89ef03713fb35a93776e64b23d7fe2c` | 1292-1293 |
| 311 / `data/29/B0/00/08.DAT` / `263c2a90566037253c69b4c32c99f1cc8b9aae1e7bd23247b9234b4c41ae7b4d` | 3156-3173, 3225-3226 |
| 312 / `data/29/B0/00/09.DAT` / `360bef1ab5917e3d2e87e4a946c3fc848a7d76ced5159e21a5bb01af1cd4f111` | 3598-3609, 3649 |
| 412 / `data/61/5A/00/0A.DAT` / `019c70f30dc49d9c26b86d7c60033eac075c44222a95c2063048df8e089a3892` | 2717-2731 |
| 414 / `data/61/5A/00/0C.DAT` / `ffbab10a565160d2dd06c8fb36f0f9e136db9eb81ad0f1a8ab6b817519d4ab49` | 2645, 2652-2661 |

These are authored layout placements and paired timeline capabilities. They
do not identify a retail `DoorServer` or other actor class, an active spawn,
the server's proximity policy, a safe packet tuple, or a visible door state.
Layout 211 includes encounter-owned door groups, so its whole set must not be
treated as ordinary automatic doors. Toto-Rak layout 313 has a separate
occupancy/door population and is not classified by this table.

## Ownership joins

Every compiled timeline has a direct unit-tree owner through a serialized
member target pointer. The corpus contains 326 such associations: 290
timelines have one owner and nine shared timelines have four owners. Of those
associations, 268 have concrete instance placements and 58 do not. The latter
are library or shared definitions, not world placements.

The placed associations expand to 1,622 instance rows. Instance identity must
come from the serialized `isgrp_######` name or another proven map-object key;
the layout node GID is not interchangeable with that instance number.

Controlled-actor ownership is weaker. Only 374 of 1,096 non-system actors have
a serialized GID that appears in a direct owner's member table, producing 440
candidate rows. Donor and internal references are common, so a matching GID or
actor name is not a universal owner join.

## Layout 413 example

Layout 413's five packages show why capability and runtime ownership must stay
separate. Two packages have raw active lengths of 9,000,000 units and control
a wall and collision box through show/hide and collision clips. Three
shifting-sands packages have raw active lengths of 3,000,000, 5,000,000,
and 3,300,000 units. The layout proves those presentations exist, but no 1.23b
resource edge identifies a server actor or trigger for them.

The direct owner groups expand to 39 exact timeline-placement rows: five rows
apiece for the door show and hide timelines, 11 for normal shifting sands, ten
for small shifting sands, and eight for large shifting sands. The door rows
repeat the same five serialized instances across the complementary timelines.
These coordinates prove authored layout targets, not active server bindings;
no current map-object or DoorServer row joins a named duty instance to them.

Twenty `sdef_*` strings in the layout are sound definitions. Three sand sound
objects are owned by the sand timelines; 17 numbered sound definitions are
resident but have no unit-tree-member or instance reference in this layout.
Their presence alone is not a callable scheduler contract.

BG-object family b996 supplies two separate sand-warp candidates:

| Variant | Bank | Bytes | Raw active units | SHA-256 |
| --- | ---: | ---: | ---: | --- |
| e001 | 0001 | 271,216 | 400,000 | `62370fad21e0e716bcabe64a4d7149ffe32b41340940f333c0b2a1fdb59f9c24` |
| e002 | 0002 | 271,408 | 400,000 | `94a2473921490f4bfc1f8a5172724ece8ced6806e3598c19e200fcb158a745af` |

Each bank contains BindActor, sound, action, and effect presentation for its
own b996 variant. They are physical sand-warp assets, but no serialized edge
joins either variant to the normal, small, or large Cutter owner group. Their
400,000-unit active blocks are distinct from the layout timelines' longer
raw active lengths. The conversion of these integers to playback seconds is
not established by the inspected assets.

## Evidence boundary

The corpus proves clip structure, timing, direct unit-tree ownership, and
serialized placement. It does not prove initial state, server construction,
trigger conditions, state replay, or late-join behavior. A compiled timeline
without a placed owner remains a resource definition. A placed owner without
a selected alias remains a capability. Neither should be promoted into
runtime behavior from a name match alone. For layout 413, the historical
runtime carrier, trigger ordering, destination, initial state, and reconnect
state remain unavailable; the static b996 candidates do not resolve them.
