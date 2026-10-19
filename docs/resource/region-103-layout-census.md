# Region 103 layout census

Counts and extents of the RegionResourceData children and the layouts they
name. Static read of the retail 1.23b layout files, no client execution. The
region table is resource `0x03C00000` (`data/03/C0/00/00.DAT`, 52336 bytes).
Row framing and the selection rule are in
[RegionInfo and LayoutInfo selection](map-layout-selector.md): every child
whose full-dword type is not 0 or 1 is submitted, and no zone value takes
part. The table framing is also documented in
`xivl-tools:docs/formats/region.md`.

Evidence labels: STATIC marks a direct read of the table or layout headers.
DERIVED marks a figure computed from layout contents with the named tool.
INFERRED marks an interpretation of those figures. The render-side model,
placement and cell figures are in
[Region 103 layout children census](region-103-field-children-census.md).
The per-frame residency rules are in
[Layout manager block selection](layout-block-streaming.md).

## Table census (STATIC)

61 roots, 1028 children, 1089 rows, ending at EOF. Child type dwords by
count: `0x0` 454, `0x1` 53, `0x1000` 49, `0x2000` 161, `0x2001` 28, `0x2002`
152, `0x2003` 20, `0x3000` 66, `0x4000` 28, `0x4001` 14, `0x4002` 3.

Several region keys share one root `+0x08` value. Region keys whose root
`+0x08` is `0x29B00000` (the Black Shroud set `fst_f0`): 103, 106, 115, 122,
123. Others: 101/117/126/127 (`0x29D90000`, sea_s0), 102/119/120/211/212
(`0x28D90000`, roc_r0), 104/107/116/124/125/130/213/214 (`0x615A0000`,
wil_w0), 105/129 (`0x03E70000`, lak_l0), 108/134/135 (`0x92050000`, sea_s1),
109/132/133 (`0xABF40000`, roc_r1). Roots 201 and 203 have zero children. A
distinct region key alone does not isolate names or resources, as the
selector page records for name lookup.

## Region 103 children (32 rows, 15 submitted)

Root 103: token `fst_f0`, `+0x20` zero. Child rows by type: 16 weather
(`0x0`), 1 sky (`0x1`), then:

| Type | Key | Name | DAT bytes | Models (trb) | PHB hulls |
|---|---|---|---|---|---|
| `0x1000` | `0x29B00000` | fst_f0_low | 664720 | 201 | 0 |
| `0x2000` | `0x29B00003`..`07` | fst_f0_fld01..05 | 1.69M..1.91M | 281..326 | 190 (fld01), 186 (fld02) |
| `0x2001` | `0x29B00001` | fst_f0_twn01 | 1300928 | 669 | 343 |
| `0x2002` | `0x29B00008`..`0D` | fst_f0_dun01..06 | 0.58M..0.89M | 141 (dun01) | 219 (dun01) |
| `0x2003` | `0x29B00002` | fst_f0_ind01 | 460368 | 362 | 53 |
| `0x4000` | `0x29B00018` | fst_f0_air01 | 41504 | 11 | 1 |

Layout `0x29B00000` shares 0 trb model ids with twn01, ind01, fld01, fld02,
fld04, fld05, air01 and 1 with fld04 (trb entries compared by resource id).
The render-side page compares brt model keys instead and finds no model
shared between low and the fields. The two bases were not reconciled.

## Extents (DERIVED from the xivl-tools zone export, collision hulls only)

The figures come from `xivl export-zones`, documented in
`xivl-tools:docs/cli.md` and `xivl-tools:docs/formats/zone-geometry.md`.
World bounds of collision vertices after the full transform chain
(zone * instance * unitTree * group * child), Y up. Layout `0x29B00000` has
no PHB entries, so a collision export yields 0 placements and no extent. Its
render placement extent is not derived here.

| Layout | Placements | Min X,Y,Z | Max X,Y,Z |
|---|---|---|---|
| fld01 | 2734 | -1120,-40,-1440 | 1184,86,352 |
| fld02 | 2907 | 928,-31,-2016 | 2912,116,-288 |
| fld03 | 2700 | -1888,-4,-3488 | -160,102,-1440 |
| fld04 | 2416 | -2976,-52,-1440 | -928,70,672 |
| fld05 | 2836 | -96,-25,352 | 1760,70,2016 |
| twn01 | 2132 | -383,-40,-2400 | 544,71,-881 |
| ind01 | 106 | -400,-16,-1746 | 266,40,-1180 |
| air01 | 1 | 21,-14,-1203 | 37,4,-1177 |
| dun01..06 | 1683..2955 | see below | |

Layout zone origins (placement zone translation): fld01..05 and air01 zero.
twn01 and ind01 `(32, 12, -1312)`. dun01 `(-864,-24,-2208)`, dun02
`(224,-36,-96)`, dun03 `(1216,-40,736)`, dun04 `(1824,-12,-288)`, dun05
`(-544,0,-3232)`, dun06 `(-1440,-76,256)`. Dungeon world extents sit under
the field tiles: dun02 Y -73..8 with 110 of 110 occupied 32-unit XZ cells
inside fld01, dun01 (155 of 155) inside fld03, dun06 (101 of 101) inside fld04,
dun03 (139 of 141) inside fld05, and dun04 83 of 144 inside fld02.

## Position queries (DERIVED)

32-unit XZ cells of collision vertices, per layout. The query points are
example positions in the Central Shroud field and the Gridania guild area.
They are not values read from the client.

| Position (X,Y,Z) | Layouts with geometry in that cell |
|---|---|
| Central Shroud field point (333.3, 5.9, -943.3) | fld01 only, vertex Y 3.9..30.4 in the cell |
| Gridania guild point (58, 4, -1215) | twn01 (Y -20.6..26.6), ind01 (Y -7.4..14.1), air01 (Y -14..4.1) |

fld01 has 4 render-classified placements within 60 units of the field point
and 9915 collision vertices within 100 of it. twn01 reaches z -881 at its north
edge but has no collision cell at the field point. fld01 contains no vertex
within 100 units of the guild point.

## Overlap (DERIVED, shared occupied 32-unit cells over the smaller set)

Field tiles overlap each other at under 1 percent (fld01 with fld02..05: 11 to
16 cells). fld01 and twn01 share 30 cells (5.1 percent of twn01's 585).
twn01 and ind01 share 43 cells (89.6 percent of ind01's 48). air01 lies inside
twn01 (4 of 4) and half inside ind01. This is consistent with the five field
layouts tiling the region plane with the town cut out of fld01.

## Consequences (INFERRED)

* The terrain at the Central Shroud field point is in the `0x2000` child fld01
  (`0x29B00003`). The `0x2001` and `0x2003` children (twn01, ind01) contain no
  collision geometry there.
* Retail submits all 15 children regardless of zone, so managers for
  fld01..05, the six dungeon layouts, low and air01 are all submitted together.
  Dungeon geometry lies under the field tiles and is placed by its own zone
  origin.

## Unknowns

* The extent and placement count of the `0x1000` low layout (no PHB entries).
* Whether the client applies each layout's zone origin as the export does.
* Which blocks of each layout are resident at runtime. The selection rules are
  static and the live state is unmeasured.
