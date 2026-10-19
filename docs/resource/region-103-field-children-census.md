# Region 103 layout children census

Render-side counts and shapes of the 15 layouts that region 103 submits:
model counts, model file sizes, placement counts and DivideMap cells. Counts
and declared cell boxes only. No retail bytes or bodies are recorded.

Method: header scans of the retail 1.23b layout files (`data/29/B0/00/NN.DAT`)
and a read-only listing of each layout's DivideMap cells (declared folder boxes
in world space and the placements each cell holds). The cell listing tool is
not published in this repository, so the cell and placement figures are
derived observations that a reader cannot reproduce from tracked tools.
The collision-side extents, the region table census and the child inventory
are in [Region 103 layout census](region-103-layout-census.md). Region 103 is
the root of the 61-root table with 32 children (`0x03C00000`, see
[RegionInfo and LayoutInfo selection](map-layout-selector.md)). The weather
and sky rows are in
[Weather sky, fog and environment path](weather-sky-fog-environment-path.md).

Of the 32 rows, 16 weather (type `0x0`) and one sky (type `0x1`) row are
skipped. The other 15 are submitted: one each of `0x1000`, `0x2001`, `0x2003`
and `0x4000`, five of `0x2000` and six of `0x2002`. Residency inside each
submitted layout is position driven, see
[Layout manager block selection](layout-block-streaming.md).

## Per-layout shape (unique brt model keys, summed brt DAT bytes, assembled placements)

| Layout | models | brt DAT bytes | placements | cells |
|---|---|---|---|---|
| twn01 | 669 | 70,671,580 | 5,615 | 213 |
| ind01 | 362 | 44,336,796 | 1,575 | 21 |
| low | 201 | 42,721,239 | 765 | 105 |
| fld01 | 296 | 31,680,925 | 11,198 | 848 |
| fld02 | 326 | 31,698,984 | 11,965 | 725 |
| fld03 | 314 | 32,837,854 | 10,916 | 804 |
| fld04 | 281 | 27,932,753 | 10,285 | 885 |
| fld05 | 285 | 28,228,399 | 11,737 | 747 |
| dun01..06 | 141..192 each | 15.4..20.9 MB each | 3,116..4,981 each | 100..179 each |
| air01 | 11 | 2,093,903 | 11 | 1 |

Model sharing: twn01 and ind01 share no model (union 1,031 equals the sum).
The five field layouts share heavily (sum 1,502, union 565). The six
dungeons union to 303. The union over all 15 layouts is 2,109 models against
3,702 summed, so the other 13 layouts add 1,078 models beyond the 1,031 of
twn01 and ind01. Under this brt key comparison, low shares no model with the
town or the fields. The trb comparison in the collision-side page finds one id
shared by low and fld04. The two bases were not reconciled.

## Declared cell bounds

World-space x/z bounds of the declared cell boxes, with the number of cells
whose x/z box contains the Central Shroud field point (333.3, 5.9, -943.3)
used for the position query below:

| Layout | Type | Cells | Cells containing the point | World bounds x | World bounds z |
|---|---|---|---|---|---|
| `fst_f0_low` | `0x1000` | 105 | 8 | -3402..3261 | -3745..2668 |
| `fst_f0_fld01` | `0x2000` | 848 | 7 | -1233..1307 | -1524..438 |
| `fst_f0_fld02` | `0x2000` | 725 | 0 | 719..2912 | -2133..-171 |
| `fst_f0_fld03` | `0x2000` | 804 | 0 | -2009..-52 | -3545..-1374 |
| `fst_f0_fld04` | `0x2000` | 885 | 0 | -3038..-928 | -1470..795 |
| `fst_f0_fld05` | `0x2000` | 747 | 0 | -699..1881 | -1204..2117 |
| `fst_f0_twn01` | `0x2001` | 213 | 0 | -643..702 | -2415..-858 |
| `fst_f0_ind01` | `0x2003` | 21 | 0 | -412..265 | -1756..-1179 |
| `fst_f0_dun01` | `0x2002` | 179 | 0 | -1047..-646 | -2404..-1947 |

Declared boxes enclose the collision vertex extents of the collision-side
page. The one exception at the printed precision is ind01 maximum X, 265 here
against 266 there. The rounding of the printed values was not recorded.

Every layout has one zero-box, zero-placement cell named as the resident
block.

## Cells near the Central Shroud field point

Cells with placements greater than 0 whose declared x/z box contains the
point, and cells and placements within a distance of it:

| Layout | cells containing | cells within 100 | within 200 | within 400 | placements within 100 / 200 / 400 |
|---|---|---|---|---|---|
| fld01 | 7 | 23 | 45 | 120 | 299 / 484 / 1,822 |
| low | 8 | 10 | 12 | 26 | 74 / 78 / 195 |
| twn01 | 0 | 3 | 19 | 53 | 17 / 133 / 1,138 |
| ind01 | 0 | 0 | 0 | 4 | 0 / 0 / 229 |
| fld02..05, dun01..06, air01 | 0 | 0 | 0 | 0 | 0 |

The cells of fld01 carry zone tokens `fst0Field01` (509 cells),
`fst0Field01a` (213), empty (124) and `fst0Town01a` (1). These sum to 847 of
the 848 cells and the remaining cell was not classified. The ground at the
point lives in `0x29B00003`, a type `0x2000` child. The twn01 whole-layout
world box overlaps the point but none of its cells contain it. The fld01
containing cells are one 64 by 64 class cell tagged with the field zone name
plus six tall far-range cells, and the low layout supplies far-range cells
over the same area.

## Unknowns

* Per-frame GPU draw counts and the GPU bytes of a loaded field are
  unmeasured.
* The cell and placement figures were not reproduced with a published tool.
