# MiniMap base tiles

The standard MiniMap control builds its base tile list from native sheet
rows and maintains it against the viewport. This list is separate from the
[XML marker lists](sqwt-xpath-data-sources.md#marker-wrapper-and-row-context).

## Inputs and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI locates the native
routes. LLVM objdump 21.0.0 and reads of the file-backed PE sections confirm
the branches, vtable cells, and constants. The asset checks use `xivl`
sheet extraction and GTEX inspection on the same build.

The authenticated [MiniMap form](sqwt-xpath-data-sources.md#marker-wrapper-and-row-context)
declares `MapScreenControl` with `Width=256`, `Height=256` and
`MaskImageId=0x61C10000` at line 19 (MEASURED). The mask descriptor
`0x0134012C` (tag `0x15`, initializer `0x00F11CA0`) selects holder `+0x4E0`
at `0x00673EE3..0x00673F04`. Activation skips ID `-1` at
`0x006726FA..0x006726FD`. The admitted activation and active property
change paths pass the value to embedded image `+0x7EC` through
`0x00991090` at `0x00672706` and `0x006738BB`. The image retains the ID
at its own `+0x58` (STATIC).

## Native piece producer

`MapLoadChecker` cell `0x00FC3310` selects `0x00670210`. After an admitted
`_layout` row lookup, it adds the requested page value to column 2 at
`0x00670305..0x00670317`. The `mapNavi_data` path uses the computed key
when available and substitutes row 1 on a failed availability check at
`0x0067037F..0x0067038C`. Column 6 of a resolved row becomes the piece
row key at `0x006703A8..0x006703BB`. The update at
`0x00679D71..0x00679D98` rejects a piece key of `-1` and starts a new
read only when the piece key differs from the retained key `+0x9EC`.
`0x00678000` clears the old pieces through `0x00677F50` and resets
`+0x600` at `0x0067805E`. It creates `SSDReader` through `0x0067EE60`,
stores it at control `+0x5F8` and registers it at `0x006780B6` (STATIC).

The reader retains the navigation row key at `+0x40` and the piece row key
at `+0x44`. Callback cell `0x00FC33F0` selects `0x006765E0`. State 0
reads the `mapNavi_data` row, including the negated map offsets from
columns 3 and 4 at `0x006766BA..0x00676706`. State 1 requires a present
`2Dmap_piece` sheet and an admitted row before it reads columns 0 through
5 at `0x006768CB..0x00676974`. The piece record receives the six words
at `0x006769BD -> 0x0067ACA0` and enters the reader list at `0x006769DD`.
The intervening `map_marker_data` lookup is conditional. The state-1
piece lookup is reached independently of its result (STATIC).

`0x00679946` calls `0x006780D0` on the control. The reader must be present
and return nonzero from slot `+4` at `0x00678101..0x00678125` before
materialization. A piece with column 1 equal to `-1` passes the piece
selector. Otherwise column 1 must occur in the range at control
`+0x584..+0x588` (`0x006781CB..0x0067823F`). The admitted piece
constructor `0x00680270` receives column 0 shifted left 16 bits,
the rectangle from columns 2 through 5, the reader's map offsets, and
the fallback image at control `+0x67C`. It stores the rectangle with
the two offsets added to its origin and converts its dimensions to tile
counts by multiplication by `1/256` and conversion to integers. The
constant is at `0x00FC3400`. The piece is appended to the container at
`+0x95C`, whose sentinel is `+0x960`, at `0x006782CB..0x006782FD`.
After the materialization pass the control releases the reader and sets
`+0x600` to 1 at `0x00678620` (STATIC).

## Viewport and draw

The active viewport update at `0x006762F0` walks retained pieces and calls
`0x00675DD0` at `0x00676514`. The callee tests existing tile rectangles
against the requested rectangle through `0x00673000` at `0x00675EEA`
and removes disjoint tiles. It advances grid coordinates in 256-pixel
steps and creates missing visible entries at `0x00676215..0x0067626A`.
For a grid column `x` and row `y` inside the piece bounds, the tile ID is
`base + grid_width * y + x`. An out-of-bounds coordinate selects
`0x61C10005` at `0x00676210` instead. `TileElement` constructor
`0x0067BDA0` stores the rectangle and fallback image, then calls
`0x00991090` on its embedded image with the selected numeric ID. The
fallback image itself is initialized with `0x61C10005` at `0x006726C4..0x006726CF`
(STATIC). [Fallback GTEX identity](https://github.com/XIVLegacy/xivl-tools/blob/604210657f4858fb69744a3a92c90f3369040057/docs/formats/gtex-pwib.md#L279-L285).

The ordinary draw at `0x00676B00` requires nonzero `+0x600` and a nonnull
`+0x5F4` at `0x00676B44..0x00676B58`. It submits the viewport rectangle
`(0, 0, width, height)` through context slot `+0x4C` at `0x00676C71`.
The base piece loop at `0x00676F3B` reaches `0x006736D0` at `0x00677074`
before the later marker and area passes. A nonempty piece visits its
retained tiles through `0x00673743 -> 0x00670F60`. This helper pushes
the tile transform, builds local 256-by-256 rectangles, and checks
the supplied mask image ID at `0x00671084..0x0067108A` (STATIC).

| Mask holder | Draw request |
| --- | --- |
| ID differs from `-1` | The helper builds a separate mask sample rectangle from the viewport, scale and tile position at `0x00671090..0x006711E1`. Slot `+0x20` at `0x0067121C` receives the tile and mask through `DrawImageWithMask`. Executor `0x00696950` looks up both IDs, requires a resolved first texture, a nonzero second ID and a zero flag dword, then passes both lookup results to `0x00CA4600` at `0x00696A37`. |
| ID equals `-1` | Slot `+0x1C` at `0x0067125E` receives the tile image and the fallback image retained at tile `+0x74`. The generic `DrawImage` executor can use the second image when the first is unresolved. |

The branches use the [ordinary drawing context command builders](npcsay-backdrop-draw.md#from-element-to-quad-code).
The selected MiniMap mask asset is `data/61/C1/00/00.DAT`, 262176 bytes,
SHA-256 `a0c0028ad9952201d5803c29c72000032f5b3e10672820bada3a462c87a5558e`.
It inspects as 256 by 256 A8R8G8B8 GTEX (MEASURED).

## Selected Bridge piece and reproduction

For the checked layout `5005` with page `0`, `_layout` column 2 selects
`mapNavi_data` row 70, whose column 6 selects piece 1081.
[Pinned Bridge row join](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/docs/bridge-map-identity.md#L8-L20).

| Piece 1081 field | Measured value | Use |
| --- | ---: | --- |
| Columns 0 and 1 | `40904`, `-1` | Base `0x9FC80000` after the shift, unconditional piece selector |
| Columns 2 and 3 | `0`, `0` | Origin before the map offsets |
| Columns 4 and 5 | `1536`, `1024` | 6 by 4 tiles |

For the admitted grid coordinate `(0, 0)`, the formula selects
`0x9FC80000`, which maps to `data/9F/C8/00/00.DAT`. The installed asset
is 41008 bytes, SHA-256
`d6df98c5d71df66be98038034ca39e77b26e2a7cec4502f49ef691a688181415`.
It inspects as 256 by 256 DXT1 GTEX with two mips (MEASURED).
[Resource ID path convention](https://github.com/XIVLegacy/xivl-tools/blob/604210657f4858fb69744a3a92c90f3369040057/docs/formats/sedb-res.md#L31-L37).

The sheet definitions are `0x01030360` and `0x01030511`, and their
decoded block declarations select the next three IDs in each family.
Set `client` to the absolute installation root containing `data/` and
use a fresh scratch directory.
[Pinned piece inventory](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/manifests/sheet_inventory.csv#L5)
and [pinned navigation inventory](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/manifests/sheet_inventory.csv#L480).

```sh
scratch=$(mktemp -d)
for dat in 01/03/03/60 01/03/03/61 01/03/03/62 01/03/03/63 \
           01/03/05/11 01/03/05/12 01/03/05/13 01/03/05/14; do
    dest="$scratch/sources/data/$dat.DAT"
    mkdir -p "$(dirname "$dest")"
    ln -s "$client/data/$dat.DAT" "$dest"
done
xivl extract "$scratch/sources" --output "$scratch/csv"
shasum -a 256 "$scratch/csv/2Dmap_piece.csv" "$scratch/csv/mapNavi_data.csv"
xivl inspect "$client/data/9F/C8/00/00.DAT"
```

The selected extraction produces 525 rows across the two sheets with no
missing trailing values, absent blocks or conflicts. The CSV digests
match the [pinned piece manifest](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/manifests/tables.json#L123-L128)
and [pinned navigation manifest](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/manifests/tables.json#L3843-L3848).

This establishes the tile request for the selected layout, page and
admitted coordinate. Live layout and page selection, the first visible
coordinate, texture readiness, and actual painting remain UNKNOWN.
