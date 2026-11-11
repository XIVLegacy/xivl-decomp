# Private-area region layouts

Regions 202 (`prv_s0`), 204 (`prv_f0`), 205 (`prv_w0`) and 209 (`prv_i0`)
submit only type `0x3000` children. Each child layout carries its own
LaySettings translation, and within one region no two children have
collision extents that overlap in both X and Z. This page records those
translations and extents. Row framing and the submission rule are in
[RegionInfo and LayoutInfo selection](map-layout-selector.md): every child
whose full-dword type is not 0 or 1 is submitted, and the selection loop reads
no zone value. The region 202 rows are also listed, without names or
translations, in [MiniMap base tiles](minimap-base-tiles.md).

This page does not establish completed loading, the manager a scene draws
first, an entry position, or any server coordinate.

## Input and method

| Item | Identity |
| --- | --- |
| Install | Retail FINAL FANTASY XIV 1.23b, `game.ver` `2012.09.19.0001` |
| Region table | `data/03/C0/00/00.DAT`, resource `0x03C00000`, 52336 bytes, SHA-256 `c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74` |
| Region rows | Python 3 `struct`, little-endian `0x30`-byte rows from `0x40`, root count at `+0x24`, child count at root `+0x0C`, child type at `+0x04`, resource key at `+0x08`, kind dword at `+0x0C` and the 15-byte name copy from `+0x10` |
| Layouts | `xivl export-zones <install> --output <dir> --layout <key>`, built from `XIVLegacy/xivl-tools` commit `604210657f4858fb69744a3a92c90f3369040057`, one explicit layout per run |
| Translation | The zone transform the export records for every placement, which is the `LaySettingsObject` `+0x40` translation (`xivl-tools:docs/formats/world-layout.md`, "LaySettingsObject") |
| Extent | Minimum and maximum of the exported OBJ vertex positions, world space, Y up, after the full transform chain of `xivl-tools:docs/formats/zone-geometry.md` |

Evidence labels:

- STATIC: read directly from the region table rows.
- MEASURED: computed from the export output with the method above.
- INFERRED: a reading that joins those figures with the cited selector
  contract. The join is stated.

Every exported placement of each layout has the same zone transform, with
zero rotation and unit scale. The export's zone name equals the region child
name in every row below. Placements is the length of the sidecar placement
table, and faces count the PHB hull and CollisionBox faces of the OBJ. Extents are rounded to two
decimals.

## Region 202

STATIC. Root 202 at file `0x8950`, token `prv_s0`, root `+0x08`
`0x72420000`, ten children, root `+0x20` zero. The weather row `wtr_fine`
(`0x8980`) and the sky row `sky_celestial` (`0x89B0`) are types 0 and 1 and
are not submitted. MEASURED for the translation and the four columns after it.

| File offset | Child key | Resource | Name | Kind | Translation | Collision X | Collision Y | Collision Z | Placements | Faces |
| --- | ---: | --- | --- | --- | --- | --- | --- | --- | ---: | ---: |
| `0x89E0` | 5002 | `0x72420000` | `prv_s0_inn01` | `0x2` | (0, 0, 0) | -6.00..6.00 | 0.00..8.20 | -34.74..8.02 | 64 | 21061 |
| `0x8A10` | 5003 | `0x72420001` | `prv_s0_mlt01` | `0x2` | (-160, 0, -160) | -208.75..-114.99 | -3.49..11.25 | -224.24..-122.95 | 349 | 99791 |
| `0x8A40` | 5001 | `0x72420002` | `prv_s0_wrk01` | `0x2` | (160, 0, 160) | 147.85..172.16 | -0.76..12.00 | 108.20..211.80 | 243 | 77450 |
| `0x8A70` | 5004 | `0x72420006` | `prv_s0_gov01` | `0x102` | (160, 0, -160) | 133.53..186.47 | 0.00..13.25 | -192.92..-133.18 | 1 | 12309 |
| `0x8AA0` | 5005 | `0x72420007` | `prv_s0_hal01` | `0x102` | (-320, 0, -320) | -336.17..-303.66 | -0.90..13.24 | -337.83..-291.01 | 35 | 10854 |
| `0x8AD0` | 5006 | `0x72420008` | `prv_s0_gov02` | `0x102` | (-160, 0, 160) | -186.47..-133.53 | -0.30..8.20 | 143.12..186.82 | 14 | 6790 |
| `0x8B00` | 5008 | `0x72420009` | `prv_s0f_wrk01` | `0x102` | (480, 0, 480) | 467.83..492.18 | -0.60..12.19 | 433.74..526.26 | 114 | 83962 |
| `0x8B30` | 5007 | `0x7242000A` | `prv_s0w_wrk01` | `0x102` | (320, 0, 320) | 301.42..338.58 | -0.43..24.79 | 313.48..441.52 | 272 | 140193 |

The `0x72420007` translation agrees with
`xivl-client-data:docs/bridge-map-identity.md`, "Layout resource".

## Regions 204 and 205

STATIC. Root 204 at file `0x8B90`, token `prv_f0`, root `+0x08`
`0x729F0000`, eight children. Root 205 at file `0x8D40`, token `prv_w0`, root
`+0x08` `0x72A60000`, nine children. Both have root `+0x20` zero, and each
starts with a `wtr_fine` row of type 0 and a `sky_celestial` row of type 1.
MEASURED for the translation and the four columns after it.

| Region | File offset | Child key | Resource | Name | Translation | Collision X | Collision Y | Collision Z | Placements | Faces |
| ---: | --- | ---: | --- | --- | --- | --- | --- | --- | ---: | ---: |
| 204 | `0x8C20` | 5022 | `0x729F0000` | `prv_f0_inn01` | (0, 0, 0) | -6.18..8.53 | -1.00..9.46 | -52.71..9.00 | 49 | 30101 |
| 204 | `0x8C50` | 5023 | `0x729F0001` | `prv_f0_mlt01` | (-160, 0, -160) | -208.13..-115.36 | -2.04..10.79 | -221.00..-122.00 | 390 | 160069 |
| 204 | `0x8C80` | 5021 | `0x729F0002` | `prv_f0_wrk01` | (160, 0, 160) | 147.83..172.18 | -0.60..12.19 | 113.74..206.26 | 114 | 83962 |
| 204 | `0x8CB0` | 5024 | `0x729F0006` | `prv_f0_gov01` | (160, 0, -160) | 146.68..183.72 | -1.00..16.61 | -191.73..-132.01 | 12 | 6851 |
| 204 | `0x8CE0` | 5025 | `0x729F0007` | `prv_f0s_wrk01` | (320, 0, 320) | 307.85..332.16 | -0.76..12.00 | 268.20..371.80 | 243 | 77450 |
| 204 | `0x8D10` | 5026 | `0x729F0008` | `prv_f0w_wrk01` | (480, 0, 480) | 461.42..498.58 | -0.43..24.79 | 473.48..601.52 | 272 | 140193 |
| 205 | `0x8DD0` | 5012 | `0x72A60000` | `prv_w0_inn01` | (0, 0, 0) | -26.01..26.01 | 0.00..9.36 | -36.61..8.53 | 15 | 26723 |
| 205 | `0x8E00` | 5013 | `0x72A60001` | `prv_w0_mlt01` | (-160, 0, -160) | -210.22..-115.00 | -3.12..11.00 | -228.25..6.53 | 308 | 110358 |
| 205 | `0x8E30` | 5011 | `0x72A60002` | `prv_w0_wrk01` | (160, 0, 160) | 141.42..178.58 | -0.43..24.79 | 153.48..281.52 | 272 | 140193 |
| 205 | `0x8E60` | 5014 | `0x72A60006` | `prv_w0_gov01` | (160, 0, -160) | 134.07..185.93 | -0.01..14.15 | -191.15..-132.45 | 1 | 6595 |
| 205 | `0x8E90` | 5015 | `0x72A60007` | `prv_w0_gov02` | (-160, 0, 160) | -178.87..-141.13 | -0.50..19.54 | 117.00..160.96 | 2 | 12182 |
| 205 | `0x8EC0` | 5017 | `0x72A60008` | `prv_w0f_wrk01` | (320, 0, 320) | 307.83..332.18 | -0.60..12.19 | 273.74..366.26 | 114 | 83962 |
| 205 | `0x8EF0` | 5016 | `0x72A60009` | `prv_w0s_wrk01` | (480, 0, 480) | 467.85..492.16 | -0.76..12.00 | 428.20..531.80 | 243 | 77450 |

Every child of regions 204 and 205 has kind dword `0x102`.

## Region 209

STATIC. Root 209 at file `0x9130`, token `prv_i0`, root `+0x08`
`0xA09B0000`, five children, root `+0x20` zero, with a `wtr_fine` row of type
0 and a `sky_celestial` row of type 1. Every child has kind dword `0x102`.
MEASURED for the translation and the four columns after it.

| File offset | Child key | Resource | Name | Translation | Collision X | Collision Y | Collision Z | Placements | Faces |
| --- | ---: | --- | --- | --- | --- | --- | --- | ---: | ---: |
| `0x91C0` | 5303 | `0xA09B0000` | `prv_i0f_inn01` | (160, 0, 160) | 153.82..168.53 | -1.00..9.46 | 107.29..169.00 | 46 | 30027 |
| `0x91F0` | 5301 | `0xA09B0001` | `prv_i0s_inn01` | (-160, 0, -160) | -166.00..-154.00 | 0.00..8.20 | -194.74..-151.98 | 60 | 20629 |
| `0x9220` | 5302 | `0xA09B0002` | `prv_i0w_inn01` | (0, 0, 0) | -26.01..26.01 | 0.00..9.36 | -36.61..8.53 | 15 | 26689 |

## Patterns

MEASURED from the tables above.

- In regions 202, 204 and 205 the room translations follow one plan. The inn
  sits at the origin, the `mlt01` room at (-160, 0, -160), the `gov01` room at
  (160, 0, -160) and the region's own `wrk01` ward at (160, 0, 160). The
  `gov02` room is at (-160, 0, 160) in regions 202 and 205, and region 204 has
  none. Only region 202 has a `hal01` room, at (-320, 0, -320).
- The wards of the other two cities sit at (320, 0, 320) and (480, 0, 480).
  Region 202 places the `w` ward at 320 and the `f` ward at 480. Region 204
  places `s` at 320 and `w` at 480. Region 205 places `f` at 320 and `s` at 480.
- Each city's ward layout appears once in each of the three regions. The three
  copies have the same set of PHB resources, the same placement and face
  counts, and the same extent relative to their own translation: `s` wards
  X -12.15..12.16 and Z -51.80..51.80, `f` wards X -12.17..12.18 and
  Z -46.26..46.26, and `w` wards X -18.58..18.58 and Z -6.52..121.52. Their DAT
  files differ in SHA-256, so they are not byte copies.
- Region 209 places its three inns at (160, 0, 160) for `f`, (-160, 0, -160)
  for `s` and the origin for `w`.
- Within each of regions 202, 204, 205 and 209, no two submitted children have
  extents that overlap in both X and Z. Some pairs overlap in X alone or in Z
  alone.

## Type 0x3000 census

STATIC, over all 61 roots of the region table. Type `0x3000` children occur
under roots 0 (`_viewer`), 111, 112, 113, 202, 204, 205, 206, 207, 208, 209,
901, 911, 913, 914 and 921 to 925. No root that holds `0x3000` also holds type
`0x2001`, `0x2002` or `0x2003`. Roots 901 and 922 also hold type `0x2000`.
Roots 202, 204, 205, 207, 208 and 209 hold only types 0, 1 and `0x3000`.
Root 208 is recorded in [RegionInfo and LayoutInfo selection](map-layout-selector.md#root-and-child-construction).

## Interpretation

INFERRED, joining the selector contract with the measured extents. A normal
SetMap construction of region 202 submits all eight rooms, each at its own
translation, because the selection loop submits every eligible child and reads
no zone value. The rooms do not overlap in plan, so the ward street at
(160, 0, 160) is part of the region whichever room a viewer stands in. Which
room a viewer is in follows from the actor position inside that one region,
not from a choice among the children. The same reading holds for regions 204,
205 and 209.

## Limits

- No capture of a retail private-area session is cited here, so no retail
  entry position is established.
- The selector contract establishes submission, not completed loading or draw
  order.
- Extents count collision geometry only. Render-only parts of a layout may
  reach past them.
- The meaning of the kind dwords `0x2` and `0x102` is not traced.

## Layout identities

The exported layouts, `data/AA/BB/CC/DD.DAT` for resource `0xAABBCCDD`.

| Resource | Bytes | SHA-256 |
| --- | ---: | --- |
| `0x72420000` | 67168 | `5205b090c9255e398abea7474ed9c99a74b535586145a51aaf5f7cf544e0caf9` |
| `0x72420001` | 198832 | `18cf295141b8b2ecb160ad241e9513dbf68f113939ba6844403ad0c5946fafe7` |
| `0x72420002` | 116336 | `ee507f052c897d5d71552c2fbfe2b8f6d2bc68be7c400d135c0750cebadf596a` |
| `0x72420006` | 33416 | `9ce53774fc06eb13f35eef031700173c5f9fc66dccf4130e2c7a5ba8ae75ee6f` |
| `0x72420007` | 35552 | `902cde4a009619b27161f578ed6fa3e659ab31f70c5ac2745b3d51abbce9c57b` |
| `0x72420008` | 43392 | `ccd1ef566bf3f07bfde29a3d35c2570e55f4d173471259c9aad07f5aae7c6249` |
| `0x72420009` | 97104 | `7929fc210fb7464cf7ebf3332570d50423ff3f98c7d2ab069eaf8d4595093536` |
| `0x7242000A` | 110912 | `faf29e746e9ea8a84218724107a0d1151c0d2a14b97b44521d49d34b04be8ad4` |
| `0x729F0000` | 59616 | `acf787fe46b559f7b8c0876c7dffb30ed410e3bec5c8e4c3f843171579fdfd6c` |
| `0x729F0001` | 170960 | `66b5209d64ef9b56e5c6a76335bc4333345d0581ada8e9171d34b41636fb1d7f` |
| `0x729F0002` | 98384 | `f2fa41041b8c1b951f791f54c666dcd3a96c636c158a82ca007e2e593bcabed3` |
| `0x729F0006` | 54940 | `6a533f79adcf43915d4569ffe0aa8022d27056d581a3c919f2e23b3426d7d377` |
| `0x729F0007` | 114960 | `cc648a9963ba6a0ef3999e22a2bffb7fd39fa2439e1156b365926b6e34571180` |
| `0x729F0008` | 110912 | `1a4c1d413767246c807f31fe133bf8950a50d366696c7f704ae33872132db38d` |
| `0x72A60000` | 44832 | `be30aa206bcbd8cb484533737560cdc7039a14b39ef9080f8e36c0608dbbd574` |
| `0x72A60001` | 150336 | `3089a7e2168d2a0364ae5c1a06f4594daff547ef7d60ba2ea2452dfa8e5dfc6e` |
| `0x72A60002` | 112256 | `dea85bca9f71bcdbe0ccb6179fcfce9407d19835becaa1a9e3642b8a122835dd` |
| `0x72A60006` | 35008 | `f6891f5a8c4eaf32869d7146635e61db4bd7cf46049fea3286006aa4630632b3` |
| `0x72A60007` | 37848 | `3f37394c2530143d9d1ae93039cd3901bcdcf8111ebf5d69cb38f3fdadd78190` |
| `0x72A60008` | 97104 | `e221765b5a0ba0245afa62333b8b089d0b7ffcbc1b6f4c2dcd8b31ca34366692` |
| `0x72A60009` | 114960 | `88ae1bd196cc33c134b9b34d1057b6cc9a5edfc3b6fb0a173a495ca01d3c131b` |
| `0xA09B0000` | 61968 | `85dd36a9f30b951ada681ff3b4ec1b0d0f0843a5933deecc2660ef5c37592066` |
| `0xA09B0001` | 69920 | `83d04c5536033f4e73012b9090cc02114a04ba5e9c769398caf446b7035bb63d` |
| `0xA09B0002` | 46080 | `00df78eb3d21604fb0be0d06137634be9a026c19624a55ca87f5576b1c7f0314` |

## Reproduction

Check the region table size and SHA-256, walk its rows as in the method table,
and list the children of roots 202, 204, 205 and 209. Check each layout DAT
against the identity table, run `xivl export-zones` once per resource key, and
read the zone transform of each placement from the metadata sidecar. Take the
coordinate minimum and maximum over the `v` lines of the OBJ, and count its
`f` lines. Keep exported geometry and install bytes outside published
artifacts.
