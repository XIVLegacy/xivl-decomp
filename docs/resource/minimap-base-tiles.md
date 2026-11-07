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

## Scene layout and page inputs

On the admitted position update, `0x0065CC42` calls `0x007D4020`.
The latter queries the active layout at the supplied position through
`0x00624AC0` at `0x007D40C6`. The checked spatial branch reaches
`0x00644840 -> 0x00644210`. A successful selection writes the block
and its paired layout object to the result at `0x00644341..0x00644346`
(STATIC).

The [DivideMap folder registration](layout-block-activation.md#folder-registration)
builds `BlockAABB` from the relocated folder. At `0x006427DA..0x006427E9`,
`0x006426C0` copies folder byte `+0x60` to block `+0x10` and folder byte
`+0x63` to block `+0x11`. At `0x007D40E4..0x007D40F9`, the position
update calls the selected layout object's `+4` interface slot `+0x164`
and passes the resulting ID to `0x00662440`. The layout post resolves it
through `0x0062B8C0`, which calls active layout lookup `0x00626820`
and mapping helper `0x0062B380`. A layout miss or a zero mapped value
selects the original ID OR `0x8000` at `0x0062B8DE..0x0062B8E4` (STATIC).

The [scene-to-main event queue](../actor/chara-turn-and-field-consumers.md#turn-completion)
delivers separate layout and page values. The checked producers post when
the corresponding scene actor value changes. The layout post also requires
the active layout pointer read at `0x00662474` (STATIC).

| Value | Scene post to `0x007CE7B0` | `CharaElement` slot 8 store |
| --- | --- | --- |
| Resolved layout dword | `0x0066247E..0x00662492`, event `0x5B`, four bytes | `+0xB30` at `0x0058E553`, table cell `0x0058E6F4` |
| Block byte `+0x10`, used as the page | `0x007D40FE..0x007D4105` supplies it to `0x00662510`, event `0x5D`, one byte at `0x00662532` | `+0xB38` at `0x0058E5C5`, table cell `0x0058E6FC` |
| Block byte `+0x11` | `0x007D410A..0x007D4111` supplies it to `0x00662540`, event `0x5E`, one byte at `0x00662560` | `+0xB39` at `0x0058E5E4`, table cell `0x0058E700` |

When `0x004D72F0` returns the local `CharaElement`, `0x00565060` places
it first in the list used by `0x006795D0`. The control retains that first
element at `+0x9E0` at `0x0067963F..0x00679649`.
At `0x0067968C..0x006796E3`, it copies the changed layout
and page to `+0x9F4` and `+0x9FC` and separately copies `+0xB39` to
`+0x9FD`. A changed pair can set the load request bit `+0xA69 & 0x10`
at `0x00679710` when `QuestMapIndex` is `-1` and either layout or page
override is absent (STATIC).

The request at `0x00679C5D..0x00679CD7` requires `+0xA69 & 0x10`
and the `Ready` bit `+0x480 & 0x20`. The `Ready` descriptor
`0x0133FF34` selects `+0x480` at `0x00673C34..0x00673C4E`.
The `Layout` descriptor
`0x0133FEEC` uses `+0x470` and the `Rect` descriptor `0x0133FF10`
uses `+0x478`. Their initializers name the properties at `0x00F11A90`
and `0x00F11AB0`, and the constructor initializes both to `-1`
at `0x0067A080..0x0067A0B1`. `Layout` supplies its dword when present.
`Rect` compares its dword with `-1`, then supplies its zero-extended
low byte when present. The ordinary branch otherwise reads the
corresponding retained field at `0x00679C9D..0x00679CC1`. When an override is absent,
the availability branch rejects retained layout `-1`. It zero-extends
the retained page before comparing with dword `-1` at
`0x00679C89..0x00679C92`, so byte `0xFF` can pass that comparison
and reach the request as 255 (STATIC).

After the dirty and Ready gates, `+0xA6A & 4` forces layout `101` and
page `0` at `0x00679CCD..0x00679CD2`. It can take that branch with both
explicit overrides present or on a failed retained availability check.
The constructor clears that bit at `0x0067A9E3..0x0067AA12`. The property
handler at `0x0067391C` is a checked writer that sets it. `0x006712B0`
requires a nonnull checker and a nonnegative layout at
`0x00671343..0x00671359` before storing layout and page at
`MapLoadChecker +0x18` and `+0x1C`. The caller clears its dirty bit after
the helper returns at `0x00679CD7` (STATIC).

The published SetMap application fields are region, zone and mode. The
checked MiniMap route derives its inputs from the active scene and the
selected block. The live layout mapping, block selection and `Ready`
timing for a particular session remain UNKNOWN.
[SetMap framing and native base](https://github.com/XIVLegacy/xivl-client-structs/blob/180ab8aaa49b0157fee8996abe4d27e05b588bda/docs/map-layout-selector.md#L31-L56).

### Serialized DivideMap folder page

The page is an unsigned byte at raw LYB node `+0x60` for the checked
`RefObjects/SharedFolder/DivideMap/DivideMapFolderObject` shape. The
manager tests that class at `0x0064F588`. When node byte `+0x62` has bit
0 clear, it passes the relocated node to the deferred registration at
`0x0064F5AE`. The copy at `0x006427DA..0x006427E2` supplies the page
byte at `+0x60` to `BlockAABB +0x10` (STATIC).
[Folder registration](layout-block-activation.md#folder-registration).

The loader calls `0x00C1F110` at `0x00642E87` before manager dispatch.
With payload `+0x0D` bit 0 clear and `+0x0E` equal to 2, it calls
`0x00C1EF90` at `0x00C1F12C`. The node loop at
`0x00C1F006..0x00C1F01C` adds the relocation delta to each nonzero
dword identified by a field descriptor's offset. The folder's
`0x70`-byte declaration places those dwords at
`+0x04, +0x08, +0x10, +0x18, +0x50, +0x58, +0x64`. None spans
`+0x60` (STATIC).
[LYB relocation](bgchip-model-binding.md#lyb-relocation-and-attachment).
[Declared folder shape](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/world-layout.md#L232-L236).

For both installed samples below, the relocation flag is clear and the
variant is 2. Use the LYB payload and node declarations to enumerate the
first inventory entry's definition list. Select the exact folder class
and apply the `+0x62` bit test. An in-memory Python replay of the
header, kind-table, node-field and inventory relocations with base
`0x10000000` resolves the same definition offsets and preserves
`+0x60..+0x63` on every selected node (MEASURED).
[LYB payload and node declarations](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/world-layout.md#L93-L134).

| Installed DAT and resource ID | Size and SHA-256 | LYB payload file offset | Deferred folders and first page locator |
| --- | --- | --- | --- |
| `data/72/42/00/07.DAT`, `0x72420007` | 35552 bytes, `902cde4a009619b27161f578ed6fa3e659ab31f70c5ac2745b3d51abbce9c57b` | `0x11D0` | 1 of 2 exact-class definitions. Node at payload `0x77E0`, page at file `0x8A10`. |
| `data/29/D9/00/01.DAT`, `0x29D90001` | 1197104 bytes, `bc40d7ff57ab8998978532cce4564e2488ed81d6d10b8f7ede3e1586b0b26523` | `0x99D0` | 229 of 230 exact-class definitions. First deferred node at payload `0xE29D0`, page at file `0xEC400`. |

The samples establish the field's serialized position. Whether a live
session selects any of these folders is UNKNOWN. The mapping from zone 140 or
layout 5005 to resource `0x72420007` remains UNKNOWN.
[Bridge resource identity](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/docs/bridge-map-identity.md#L40-L46).
[Bridge evidence limits](https://github.com/XIVLegacy/xivl-client-data/blob/44544085b003cd6f0530cdd994d6faf9c1a5b057/docs/bridge-map-identity.md#L69-L76).
[MapLayout exemplar identity](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/world-layout.md#L25-L28).

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

## Viewport center and scale inputs

STATIC. The authenticated MiniMap declaration at form line 19 sets
`Function=1`. It declares no local `Scale`, `MaxScale`, `MinScale`,
`FocusPointColumn`, `PlayerPosX` or `PlayerPosY` value. The property
initializers and constructor establish these separate holders.

| Property | Descriptor and holder | Constructor value and locator |
| --- | --- | --- |
| `Function` | `0x0133FE38`, `+0x448` | 0 at `0x00679FEB..0x00679FFF` |
| `Scale` | `0x0133FE5C`, `+0x450` | 0 at `0x0067A004..0x0067A01E` |
| `MaxScale` | `0x0133FE80`, `+0x458` | 1 at `0x0067A023..0x0067A03D` |
| `MinScale` | `0x0133FEA4`, `+0x460` | 0.5 at `0x0067A042..0x0067A060` |
| `PlayerPosX`, `PlayerPosY` | `0x01340228`, `0x0134024C`, `+0x4F8/+0x500` | Both 0 at `0x0067A3DC..0x0067A40B` |
| `FocusPointColumn` | `0x01340300`, `+0x53C` | -1 at `0x0067A5B7..0x0067A5CD` |

The name registrations are at `0x00F119F0..0x00F11A69`,
`0x00F11D80..0x00F11DD9` and `0x00F11E60..0x00F11E79`.
The `Scale` setter arm selects its holder at
`0x00673B85..0x00673BA6`. The draw path reads separate actual-scale fields below.

### Automatic position and sheet conversion

With an admitted first local `CharaElement`, `0x0067963F..0x00679673`
calls `0x004CFE30` with selector 1 and copies the returned X and Z
lanes to `0x00675840`. The getter adds parent positions when present.
[World-position getter](../net/target-cycle-and-pick.md#range).
`0x00675851..0x00675864` retains the pair at `+0x968/+0x96C`.
For nonzero `Function`, `0x00679717..0x00679728` submits this pair
to center helper `0x00672D30` on this update. The selected `Function=1`
declaration admits that arm. Successful actor acquisition remains a condition.

The admitted `SSDReader` reads `mapNavi_data` columns 3 and 4 as signed
integers, converts each to f32 and subtracts it from negative zero at
`0x006766BA..0x00676706`. It reads columns 8 through 11 into four
f32 fields at `0x00676708..0x0067676A`. Materialization copies these
values to control fields at `0x0067812B..0x00678161`.

| Control fields | Selected sheet values |
| --- | --- |
| `+0x638/+0x63C`, denoted `Ox,Oy` | Negated columns 3 and 4 |
| `+0x65C/+0x660`, denoted `Bx,By` | Columns 8 and 9 |
| `+0x664/+0x668`, denoted `Dx,Dy` | Columns 10 and 11 |

The checked `MapLoadChecker` first converts `_layout` column 1's unsigned
byte to f32 at `0x006702E2..0x00670300`. A resolved `mapNavi_data`
column 5 is converted as an unsigned dword, using the `2^32` correction
at `0x00670545..0x00670559`. If the stored f32 is nonzero,
`0x0067055D..0x00670573` replaces checker `+0x10` with it.
On an admitted piece result, `0x00679DBB..0x00679DC4` applies
`CVTTSS2SI` followed by `CVTSI2SS` and stores the result at control
`+0x614`, denoted `r`. This value supplies the coordinate resolution.
The finite contract below requires a positive `r` within the signed
conversion range. Zero, nonfinite and out-of-range inputs remain UNKNOWN.

### Center admission and draw coordinates

`0x00672D30` first compares the proposed pair with retained
`+0x640/+0x644`. On a difference it calls `0x00672A80`, then stores
a changed returned pair and invalidates affected draw state at
`0x00672D77..0x00672E13`. Let the retained pair be `Px,Py`.
For navigation row `+0x9F0` other than 1, `0x00676D7B..0x00676DF3`
computes `Cx=f32((Px-Ox)*r+Ox)` and
`Cy=f32((Py-Oy)*r+Oy)`. It widens the stored f32 inputs to binary64
for subtraction, multiplication and addition, then stores f32. This
converts the retained position into drawing coordinates.

For row 1, `0x00676DFC..0x00676E7E` instead computes
`Cx=f32(Dx/2+Bx+Ox)` and `Cy=f32(Dy/2+By+Oy)` with binary64
arithmetic before the f32 stores. The center helper returns its input
unchanged for row 1 at `0x00672A8B..0x00672A9E`.

For the other rows, `0x00672AA4..0x00672BCD` forms these stored f32
intermediates, with widened binary64 arithmetic inside each expression:
`k=f32((u/a)/r)`, `hx=f32(W*k/2)`, `hy=f32(H*k/2)`,
`Lx=f32(Bx/r)`, `Ux=f32((Bx+Dx)/r)`,
`Ly=f32(By/r)`, and `Uy=f32((By+Dy)/r)`.
Here `u` and `a` are control `+0x610/+0x60C`, and `W,H` are
`+0x14C/+0x150`. For each axis it compares stored
`delta=f32(proposed-O)` minus/plus the corresponding half extent against
the lower/upper bound, widening the stored delta, half extent and bound
to binary64. A lower violation selects `f32(L+h+O)`, otherwise
an upper violation selects `f32(U-h+O)` (`0x00672BCD..0x00672C70`).
If `U-L < 2*h`, the later correction overrides that result with
`f32((L+h+O)-f32(h-(U-L)/2))` at `0x00672C75..0x00672D22`.
The binary64 outer arithmetic and nested f32 conversion are distinct.
Positive finite dimensions, scale and resolution bound this analysis.
Native argument validation and the ambient floating-point rounding mode
remain UNKNOWN.

### Actual scale and explicit properties

The constructor loads f32 1 and stores it at `+0x610` and `+0x614`
at `0x0067A753..0x0067A794`. On nonzero `Function`, admitted piece
materialization writes actual scale `+0x60C=1` at
`0x0067841A..0x00678445`, reclamps the retained center and requests
`Scale=1` through the property holder subject to its flags. This
property update and the actual-scale store are separate operations.
The draw at `0x00676BBB..0x00676BF1` computes
`drawScale=f32(double(+0x610)*double(+0x60C))`.

An admitted `Scale` change selects a stepped transition or a direct update. Callback `0x00673803..0x0067383C` starts step count zero and
captures the old actual scale when `+0x600==1` and `+0x601!=0`.
Its other arm calls `0x00673640` directly. That helper stores actual
scale, reclamps the center and invalidates affected draw state.
`0x0067996A..0x006799DE` advances the selected transition and stops it
at step 6. Elapsed-time cadence and transition rounding remain UNKNOWN.

`PlayerPosX/Y` are position readbacks on the checked update path.
`0x00675840` derives their integer values from `(position-offset)/100`,
or requests -1 for navigation row 1. The checked callback keeps their tags `0x1C/0x1D` on the integer
readback path. A general writable center property remains UNKNOWN. With explicit layout/page overrides, other center choices
include the sheet focus pair selected by `FocusPointColumn` and the retained
map midpoint (`0x00679DCE..0x00679E21`). Those are separate from the
nonzero-Function automatic actor arm.

Live actor readiness, selected row, property overrides, later writes,
rotation, frame delivery and painting remain UNKNOWN. Mapping from SetMap region or zone values to the native layout key
remains UNKNOWN.

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

## Drawing stream delivery

STATIC. The masked base-tile request uses DrawingToolContext slot `+0x20`.
Its cell `0x00FA2904` selects `0x00559F60`, which obtains the tool state
through `0x00559DE0` and forwards to `0x0054DF20`. The builder creates
`DrawImageWithMask` through `0x005542A0` and appends `0x50` bytes through
`0x0054CCC0`. Constructor `0x005542DD..0x00554349` installs vtable
`0x00FA2A10` and stores destination, image source, mask source and color
at `+0x04`, `+0x14`, `+0x24` and `+0x34`. Image ID, mask ID and flag
are at `+0x44`, `+0x48` and `+0x4C`. This command layout is distinct
from the `0x40`-byte unmasked `DrawImage` layout.
[Drawing context builders](npcsay-backdrop-draw.md#from-element-to-quad-code).

With tool dirty bit `+0x5A & 0x40`, `0x0054E971..0x0054EA64` sends
its retained byte stream through `0x0054B0D0`. The producer sends at most
`0x78` bytes per successful request. Nonfinal chunks use local command
`0x19` and the final chunk uses `0x1A`. An empty stream also sends
`0x1A`. Successful sends advance tool `+0x54`. Completion clears the
producer byte range and this dirty bit. `0x0054B11F` adds `0x15` for
the ScenePacket command, yielding `0x2E` or `0x2F`.
[WindowActor dispatch](sqwt-icon-control-textures.md#observed-ordinary-control-to-renderer).

The WindowContext dispatch table at `0x008CF3B8` has direct entries
`0x008CF41C -> 0x008CEEC9` for local `0x19` and
`0x008CF420 -> 0x008CEEE1` for local `0x1A`. Both pass the payload
and byte count to `0x008CEBE0`. If context `+0x12E` is already 1,
`0x008CEBE6..0x008CEC12` clears the previous retained stream and
resets that byte before appending. The final-command arm resets offset
`+0x7C` to zero and sets `+0x12E=1` at `0x008CEEF0..0x008CEEF7`.

The draw path requires the checked WindowContext state `+0x128=3` and
nonzero `+0x12B` at `0x008CE7FF..0x008CE813`. After the earlier
renderer-slot pass, `0x008CEAFB..0x008CEB32` additionally requires
alpha `+0x10C` above the constant at `0x00FB7A64`, `+0x12E=1`,
and either `0x00CA3E00()==1` or `+0x12F=0` before calling
`0x008CE530`. The earlier actor/category test at
`0x008CE7D9..0x008CE7F9` can return before this path.

`0x008CE530` reads the retained begin/end pointers at `+0x14/+0x18`
and starts at offset `+0x7C`. After the nonzero result from `0x007F8B60` at `0x008CE71B..0x008CE722`,
`0x008CE730..0x008CE783` copies the context color from
`+0x100..+0x10C`, calls each command's virtual `+4`, and advances by
the returned byte count. It stops at the retained end or the draw-state
stop byte. The `DrawImageWithMask` cell `0x00FA2A14` selects
`0x00696950`, whose checked return is `0x50` at `0x00696A41`.
This joins the base-tile command to the [masked texture consumer](#masked-tile-sampling).
The WindowContext drawing stream owns this delivery. Stream delivery, live state values, texture readiness, intervening
updates and actual painting remain UNKNOWN.

### The declared area image

The authenticated MiniMap form also declares `GlAreaImageId=0x61C10004`
at line 19. This property is descriptor `0x01340324`, tag `0x23`,
registered at `0x00F11E80..0x00F11E99`. The setter arm at
`0x0067412F..0x00674150` selects holder `+0x544`. Activation skips
`-1` and passes an admitted ID to embedded image `+0x900` at
`0x00672754..0x00672766`. The active-change callback requires
control `+0x20 & 1` and updates that image at
`0x0067396F..0x00673986`.

The checked consumer is a later entry pass, separate from the base-piece
loop. The caller requires control `+0x958 != -1` at
`0x00677DE9..0x00677DEF`. `0x00677E46`, `0x00677E9C` and `0x00677EF2` call the walker
`0x0067DAC0`, which calls `0x0067B980`. That callee requires entry
`+0x3D` bits 0, 1 and 2 and a positive entry float at `+0x14`
(`0x0067B9B5..0x0067B9DC`). It selects image `+0x900` in both the
masked branch through `0x0067BC4A..0x0067BC78` and the unmasked
request at `0x0067BD36..0x0067BD45`. The entry flags and positive-float gate control this later area pass.
Entry population, selected geometry and live pass admission remain UNKNOWN.

## Masked tile sampling

For a tile admitted to the masked branch, `0x00671025..0x00671075`
builds both the local destination and tile source as `(0, 0, 256, 256)`
under the pushed tile transform. The mask source uses a separate pixel
rectangle. Let `W,H` be the viewport from control `+0x14C/+0x150`,
`Cx,Cy` the map center computed at `0x00676D6F..0x00676E7E`, and
`X,Y,w,h` the tile rectangle at `TileElement +8/+0xC/+0x10/+0x14`.
The draw scale `s` is the stored f32 product of control
`+0x610 * +0x60C` at `0x00676BBB..0x00676BF1`. Control
`+0x508/+0x510` supplies the configured mask extents `Mx,My` at
`0x00676F43..0x00676F62`. These are `MaskImageWidth` and
`MaskImageHeight` (initializers `0x00F11D00/0x00F11D20`), with
constructor defaults of 256 at `0x0067A410..0x0067A451` (STATIC).

For positive finite viewport dimensions and finite intermediates,
`0x00671090..0x006711E1` selects `S=W, q=f32(Mx/W)` when `W <= H`
and `S=H, q=f32(My/H)` otherwise. The f32 origin intermediates are
`bx=f32(S/2-Cx*s)` and `by=f32(S/2-Cy*s)`. The stored mask rectangle
is `(f32((bx+X*s)*q), f32((by+Y*s)*q), f32(w*s*q), f32(h*s*q))`.
The branch widens the stored f32 `q,bx,by` for the final binary64
arithmetic and stores the rectangle as f32. These are pixel coordinates before
texture normalization (STATIC).

The admitted executor at `0x00696950..0x00696A37` multiplies the
command color by the current context color and passes both texture
lookup results. `0x00CBA2C0` forms corners from each source rectangle's
`(x,y)` and `(f32(x+w),f32(y+h))`. The far corners are rounded to
f32 before normalization at `0x00CBA3C1..0x00CBA3F2` for the tile and
`0x00CBA42A..0x00CBA44C` for the mask. A nonnull texture wrapper
supplies its own width and height at `+0x18/+0x1C`. Each corner is
multiplied by the corresponding f32 reciprocal and stored as f32
(tile at `0x00CBA45A..0x00CBA4D8`, mask at
`0x00CBA4EB..0x00CBA56B`). The batch has stride `0x1C`.
Converter `0x00419A20` maps descriptor `0x0130C29C` to float2
position at `+0`, `UBYTE4N COLOR0` at `+8`, and float2
`TEXCOORD0/1` at `+0xC/+0x14`. The flush binds the tile to stage 0
and the mask to stage 1 at `0x00CBA1CC/0x00CBA1DA`. A null wrapper
skips its UV normalization and
can reach generic default texture substitution at `0x0041BEE0`.
The substituted texel values remain UNKNOWN (STATIC).

On a draw-mode transition, `0x00CA4600..0x00CA4635` flushes the
previous mode, sets mode 3, and selects vertex and pixel program indices
`(2, 2)` through `0x00CA2E80`. An existing mode 3 reuses its state.
The pinned PE program pointer and size cells give the following ranges.
Set `exe` to the executable's absolute path and pass the file offset
to `python3 tools/d3d9_shader_decode.py "$exe" <offset> --listing`
from the repository root (MEASURED).

| Program | Pointer and size cells | Payload VA, file offset, and byte length | Raw payload SHA-256 |
| --- | --- | --- | --- |
| Vertex 2 | `0x0130BB08`, `0x0110933C` | `0x01109700`, `0xD09700`, `0x1A4` | `71b662d09c8a4e76cdb2d3780602c80cb320bf992fdd68ab56b5fe86d7938df1` |
| Pixel 2 | `0x0130BB88`, `0x01109360` | `0x0110A780`, `0xD0A780`, `0x144` | `3137109322925c3c6934df9081dd08b07e4d6a5493cc6593cb6074b6359db5f0` |

The vertex program forwards both UV sets independently and forwards
normalized packed `COLOR0` for interpolation. The color packer at
`0x00CB9640` clamps each finite lane to `[0,1]`, multiplies by 255
and truncates. The pixel program
samples the tile from sampler 0 at UV 0 and the mask from sampler 1 at
UV 1. Before blending, RGB is tile RGB times tint RGB and alpha is
tile alpha times tint alpha times mask red. The sample of mask alpha
has no role in this equation. The sample and multiply instructions
request partial precision (STATIC).

When the shared flush reaches `0x00CA3513/0x00CA3540`, it requests
clamp for both samplers. Filter setup at `0x00CA3890` uses the shared
selector at `0x01376494`, initialized to 1 at `0x00CA3E73` and
changeable at `0x00CA4142`. The masked mode leaves that selector in
place. The checked selector-0 requests use point magnification and
minification with linear mip filtering. Selector 1 requests linear
magnification and mip filtering, and anisotropic minification for
signed modes 0 through 2 or linear minification for signed mode 3.
Other selectors or signed modes skip these checked filter writes
(STATIC). [Shared sampler requests](sqwt-icon-control-textures.md#observed-size-dependent-numeric-sampler-requests).

The shared flush uses shader wrapper calls `0x0041C440/0x0041C460`
when backend byte `0x013763C0` is zero, and
`0x00420860/0x004208B0` in its alternate branch. Both select the
indexed program objects in the checked mode
(`0x00CA3554..0x00CA36A9`). Device feature and state-cache checks
still govern delivery. The active backend, effective sampler and
blend state, final compositing, and live painting remain UNKNOWN.
[Shared flush states](npcsay-backdrop-draw.md#states-and-programs).

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
