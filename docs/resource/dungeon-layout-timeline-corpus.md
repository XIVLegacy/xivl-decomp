# Dungeon layout timeline corpus

The installed FFXIV 1.23b client contains 24 regional dungeon layout DATs.
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

## Per-layout coverage

| Layout | Internal name | Known content | Compiled | Stubs | Clips |
| ---: | --- | --- | ---: | ---: | ---: |
| 111 | `sea0Dungeon01` | Mistbeard Cove | 17 | 2 | 81 |
| 112 | `sea0Dungeon02` | Shposhae | 7 | 0 | 39 |
| 113 | `sea0Dungeon03` | Cassiopeia Hollow | 12 | 0 | 32 |
| 114 | `sea0Dungeon04` | unresolved | 21 | 0 | 47 |
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
| 413 | `wil0Dungeon03` | Cutter's Cry | 5 | 0 | 31 |
| 414 | `wil0Dungeon04` | Copperbell Mines | 10 | 0 | 50 |
| 415 | `wil0Dungeon05` | unresolved | 11 | 0 | 47 |
| 416 | `wil0Dungeon06` | unresolved | 2 | 0 | 4 |

The installed DAT identity for each row is fixed by its layout family and
content hash. Notable examples are U'Ghamaro layout 116,
`d8ff9874f392367fa111a75997915257a465bf8fc4c613cafa035cd12f6e733a`;
Dzemael layout 211,
`98bc9d3a0111de81a95b01a3e1c9a94453f51f11169634eea0c80f97d329b368`;
Tam-Tara layout 312,
`360bef1ab5917e3d2e87e4a946c3fc848a7d76ced5159e21a5bb01af1cd4f111`;
and Cutter's Cry layout 413,
`c669f65d3f0897f8c328ebe51f2dbc8417dcfb9ec327af49eba14de9320afa06`.

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

## Cutter's Cry example

Layout 413's five packages show why capability and runtime ownership must stay
separate. Two nine-second packages control a wall and collision box through
show/hide and collision clips. Three shifting-sands packages last 3.0, 5.0,
and 3.3 seconds. The layout proves those presentations exist, but no installed
resource edge identifies a server actor or trigger for them.

Twenty `sdef_*` strings in the layout are sound definitions. Three sand sound
objects are owned by the sand timelines; 17 numbered sound definitions are
resident but have no unit-tree-member or instance reference in this layout.
Their presence alone is not a callable scheduler contract.

## Evidence boundary

The corpus proves clip structure, timing, direct unit-tree ownership, and
serialized placement. It does not prove initial state, server construction,
trigger conditions, state replay, or late-join behavior. A compiled timeline
without a placed owner remains a resource definition. A placed owner without
a selected alias remains a capability. Neither should be promoted into
runtime behavior from a name match alone.
