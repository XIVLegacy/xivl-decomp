# Layout manager block selection

Region submission creates a layout manager for nearly every child row, but
residency inside each manager is chosen every frame from the player position.
The per-frame update resolves a current area from DivideMap folder boxes,
filters managers by the folder's name pairs, and requests only the deferred
blocks near the position. Blocks outside the selection are marked for release.
This page records that selection contract, the mode values it dispatches on,
and the static radius defaults.

## Binary and method

All addresses are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Reads are static. Decompiler listings were checked against disassembly, and
the constants and stores named below were checked against the PE bytes with
Python standard-library reads. No client execution was used. Field names are
descriptive labels, not recovered symbols.

The scene object is the MapLayoutActor returned by getter `0x00623C60`
(global `0x0133DEF4`), as described in
[Weather transition runtime](../net/weather-transition-runtime.md).
Related owners:

* [RegionInfo and LayoutInfo selection](map-layout-selector.md) owns the
  submit-all step and the region owner that retains each manager.
* [DivideMap block activation](layout-block-activation.md) owns folder
  registration, block creation and the state-1 placement callback.
* [MapLayout numeric request boundary](map-layout-request-boundary.md) owns
  the conditional resource request behind a manager.
* [Region 103 layout census](region-103-layout-census.md) and
  [Region 103 layout children census](region-103-field-children-census.md)
  hold the retail counts, cells and extents for one region.

## Two levels

| Level | Rule | Evidence |
|---|---|---|
| Region to layout managers | Every child except full-dword types 0 and 1 is submitted. No zone, bounds or radius is read. | `0x0064E830`, see the selector page |
| Manager to blocks | Per-frame position driven selection described below | `0x00650A20` and callees |

Region 103 therefore submits 15 managers from its 32 child rows (16 type-0
and one type-1 row are skipped). Submission is unconditional while block
residency is not.

## Per-frame entry

`RaptureLayoutManager` full-object vtable `0x00FBFEE4` slot 17 is
`0x00650A20`. The same function is slot 17 of the three Weather manager
vtables. It returns unless scene `+0x164` is nonzero, manager byte `+0xF4` is
set and scene mode dword `+0x178` is nonzero. It stores the mode at manager
`+0x1C0` and compares it with the previous value to form a "changed" flag, then
dispatches on manager type dword `+0x150` (the child type word) and the mode:

| Manager type | Mode | Callee |
|---|---|---|
| `0x1000` | 5 | `0x0064C330` load-all |
| `0x1000` | 1 or 2, scene `+0x174` null or its name-pair count zero | `0x0064FE90` |
| `0x1000` | other, including 6 | `0x006505D0` |
| `0x4000..0x4002` | any | `0x00650020` radius query |
| other (fields, towns, dungeons, indoor) | 2 with manager name equal to the current area manager name | `0x00650020` |
| other | 2 otherwise, 1, 3, 5 | `0x0064FE90` |
| other | 4 or 6 | `0x00650020` |

The mode is written by the scene area resolver `0x00629890`. Its constant
stores into `+0x178` are at `0x006297B9` (5), `0x00629943` (6), `0x00629BB6`
(4), `0x00629BDD` (2) and `0x00629BFF` (1). The branch conditions that pick
between them are only partly decoded (see Unknowns).

## Area resolution

`0x00629890` runs when the scene has a layout set (`+0x17C/+0x180/+0x184`
any nonzero), `+0x164` is nonzero and `0x00623E20` passes. The same readiness
test is recorded in the
[ambient fleet readiness row](ambient-garlean-fleet.md). The resolver clears
the four area slots, then fills them by asking each manager flagged at `+0xF4`
for the area under the player position through `0x00644840`. That calls
manager vtable slot `+0x174` (world to layout-local point) and `0x00644210`,
which tests the point against every registered block box (`0x0064AF50` AABBs)
and keeps the best match. The comparison keys are: smaller distance value from
`0x00640040`, then larger block priority at record `+0x0C`, then lower manager
type nibble (`+0x150 & 0xF`), then the nearer box centre.

Results: scene `+0x168` and `+0x16C` are the owning managers, `+0x170` and
`+0x174` are the matching serialized DivideMapFolder records. Type `0x1000`
managers fill the second pair (`0x00645FD0` with flag 1), every other manager
type fills the first (`0x00645FD0` with flag 0).

The record's `+0x58` array (stride 8) and `+0x5C` count are the folder's name
pairs. Each pair is `{layout name pointer, group name pointer}`. An empty
first string stands for the current area manager's name. `0x00629890` also
raises scene radius `+0x1CC` to the largest manager radius (vtable slot
`+0x188`) among named pairs.

## Block request

All three request paths share one primitive, `0x0064C200`, per block box:

* A block whose state (`+0x7C` on the full object) is absent or 4 is created
  by `0x0064BEA0` through block manager vtable slot 2 and given a priority
  word (full `+0x94`) equal to the truncated squared distance from the point
  to the block box, computed by `0x00641C10` (zero inside the box).
* A block already resident has its unload mark (`+0x8D`) cleared, state 0
  becomes 1 and the priority word is refreshed. A block in state 7 gets
  restart byte `+0x8C` set.

The selectors that call it:

* `0x0064C330` is the load-all path. It has no distance or radius threshold.
  The distance only orders the requests. It runs the body only when the box
  counts differ from the block manager's loaded plus pending counts, so it
  settles after every registered block is requested.
* `0x00650020` and `0x0064FE90` first compare a snapshot (`0x0064E320`, the
  position with its low three bits replaced by a manager value, plus the
  radius) against the previous one at manager `+0x1BC` and skip the frame when
  nothing moved, unless the changed flag is set or `0x0064E3E0` finds a moved
  secondary focus point. When they proceed:
  1. `0x00645660` marks every non-resident-flagged block with unload flag
     `+0x8D` set and priority `0x400`.
  2. The query radius is scene `+0x1D0` plus manager radius from vtable slot
     `+0x188`. `0x0064C580` builds the cube of half-extent radius around the
     point and `0x0064AC60` walks the block box tree, keeping boxes that
     intersect the cube and whose centre distance (`0x00640040`) is within
     the radius. Each hit goes to `0x0064C200`.
  3. `0x0064FE90` and `0x006505D0` additionally walk the area record's name
     pairs. Pairs whose layout name equals the manager name (or empty) call
     `0x0064E220`. With a nonempty group name it fetches that group's block
     list from the manager map at `+0x1D4` (`0x006461E0`) and requests each
     block, gated by the radius for field-like manager types. With an empty
     group name it runs the radius query.
  4. `0x0064E550` repeats the radius query for each extra focus point in scene
     vector `+0x128` (point at entry `+0x60`, radius `+0x70`).
  5. `0x0064DD30` releases every block still marked, either immediately
     through the factory (`0x01365788` slot `+0x38`) or into the deferred
     list at block manager `+0x30`.

[DivideMap block activation](layout-block-activation.md#selection-and-deferred-creation)
lists `0x0064E200`, `0x006505D0` and the slot 17 tail call as callers of the
`0x0064C330` body and leaves their scene meanings open. The load-all label and
the mode dispatch above are the interpretation added here.

Folders whose serialized byte `+0x62` bit 0 is set are not in the AABB
vector. They are created when the manager is built (`0x0064DEB0`) and are not
subject to the selection.

## Selection contract

The observed behavior, as an ordered summary:

1. A manager exists for every child whose type dword is not 0 or 1.
2. Each manager holds its DivideMapFolder cells with box, priority, name list,
   name pairs and the byte `+0x62` resident bit.
3. Every frame, the player position selects the best containing cell by the
   order in "Area resolution". Blocks of the managers named by that cell's
   pairs (or a radius query when the cell has none) are requested when they
   lie within scene `+0x1D0` plus the manager radius. Without a resolved area,
   the load-all path applies in mode 5.
4. Residency is recomputed each frame. Blocks not hit are marked and released.

## Default mode and radius defaults

Verified by disassembly and a PE byte scan of the retail image above.

* Scene mode dword `+0x178` is 5 (load-all) only through setter `0x00629760`,
  which flips scene byte `+0x1E8`. The scene constructor `0x0062D520` leaves
  `+0x1E8` clear and sets `+0x1E9` to 1 at `0x0062D737`. With `+0x1E8` clear
  and `+0x1E9` set, `0x00629890` yields mode 2, 1, 3 or 4 from the resolved
  area pairs, so ordinary area play is area and radius driven rather than
  load-all. The setter has several callers (`0x00612D55`, `0x00612E5F`,
  `0x00612ED4` and later ones) whose conditions were not read.
* Scene `+0x1D0` is initialized by `0x0062D520` from the float at
  `0x00FA2B8C`, which is 16.0. The load is at `0x0062D6C0` and the store at
  `0x0062D6C8`. Scene `+0x1CC` falls back to the float at `0x00FB6E84`, which
  is 128.0. The
  [floor model visibility](floor-model-visibility.md) page records the same
  `+0x1D0` term in the background range tests.
* The manager radius read by vtable slot `+0x188` (stub `0x006576D0`) is the
  manager float written at `0x0064F66F` in the manager build function
  `0x0064F430`. It is the float at offset `+0x3C` of the layout root object
  held at manager `+0x7C`, or 128.0 when that object is absent, multiplied by
  scene float `+0x338`. The layout-side value was not read.
* In constructor `0x0062D520`, scalar load `0x0062D7B3` reads literal
  `0x00F54F70`, whose binary32 bits `0x3F800000` encode 1.0. Scalar store
  `0x0062D7DC` writes it to scene `+0x338`. `0x0062D564` copies incoming ECX
  to the ESI object base used by that store. ghidra-cli instruction
  listings and the referenced literal were compared with the original PE.
  Later object calls to `0x0062AEB0` and `0x00629C40`, and subsequent helper
  and setter effects, remain untraced. This is a direct initializer store,
  not a guaranteed retained value at constructor return or runtime use.
* The query adds scene `+0x1D0` to the already-scaled manager radius above.
  With the layout root object absent and the multiplier equal to 1 at the
  manager-radius computation, the published terms give 16.0 + 128.0 = 144
  yalms. When the layout root is present, its float replaces 128.0. The
  direct initializer does not establish either runtime condition.

## Unknowns

* The retained scene `+0x338` value at constructor return or runtime use,
  and the `+0x3C` value in each region 103 layout root, so no region 103
  yalm radius is established.
* Which condition selects each mode (1 to 6) in `0x00629890`, and what scene
  `+0x1F0 == 3` and bytes `+0x1E8/+0x1E9/+0x1F9/+0x1FB` mean at runtime. The
  ambient fleet page records the same unresolved state 3.
* The meaning of the first dword of a name pair when it is not a layout name
  (the area resolver treats it as a float for type `0x1000`).
* Whether a field zone uses mode 5 (load-all) on entry. Only a runtime trace
  can show it.
* The resident bit of each cell in the region 103 layout files.
