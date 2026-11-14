# MiniMap marker entries

The `MapScreenControl` of the [MiniMap base tiles](minimap-base-tiles.md)
page keeps retained marker entries for its data rows and for the actors of
the scene. This page records the marker properties, where an entry takes its
position, how arrange places, fades and orders an entry, how an actor entry
picks its row, and the header words of the marker packages. Control offsets
are offsets of that control object. It supplements the
[direction and view cone](minimap-direction-and-view-cone.md) page and the
"Marker wrapper and row context" section of the
[Sqwt XPath data sources](sqwt-xpath-data-sources.md) page.

## Inputs and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI disassembly and
cross references supplied the instructions. Reads of the file-backed PE
sections supplied strings and float constants. Decompiler output was used to
orient only. The markup checks use `xivl extract-resource` on the same build.
The package checks read file headers directly. No target process ran.

## Marker properties

STATIC. Each initializer pushes the name literal, loads the descriptor and
stores the tag in the descriptor dword at `+0x14`. Each setter arm pushes the
descriptor and passes the holder address in ECX.

| Property | Name literal | Initializer | Descriptor | Tag | Holder and setter arm |
| --- | --- | --- | --- | --- | --- |
| `Filter` | `0x00FC25B8` | `0x00F11AF0` | `0x0133FF58` | `0x08` | `+0x484`, arm `0x00673CAB..0x00673CCC` |
| `TypeDataSource` | `0x00FC261C` | `0x00F11BE0` | `0x01340054` | `0x0F` | `+0x4B0`, arm `0x00673DD0..0x00673DF4` |
| `ActorMarkerDefaultTemplate` | `0x00FC262C` | `0x00F11C00` | `0x01340078` | `0x10` | `+0x4B8`, arm at `0x00673E18` |
| `GlMarkerDefaultTemplate` | `0x00FC2648` | `0x00F11C20` | `0x0134009C` | `0x11` | `+0x4C0`, arm at `0x00673E42` |
| `DefaultSparkleFile` | `0x00FC267C` | `0x00F11C60` | `0x013400E4` | `0x13` | `+0x4D0`, arm at `0x00673E96` |
| `DefaultSparkleSequence` | `0x00FC2690` | `0x00F11C80` | `0x01340108` | `0x14` | `+0x4D8`, arm at `0x00673EC0` |
| `MaskImageId` | `0x00FC26A8` | `0x00F11CA0` | `0x0134012C` | `0x15` | `+0x4E0`, arm at `0x00673EEA`. The constructor registers it with -1 at `0x0067A386..0x0067A3A1` |

MEASURED. `client/sqwt/widget/MiniMapWidget.form` (the identity is on the
[direction page](minimap-direction-and-view-cone.md#inputs-and-method))
declares at line 19 `GlMarkerDefaultTemplate` and `SsdMarkerDefaultTemplate`,
both `MapMarkerTemplate`, `MaskImageId="0x61C10000"`,
`MarkerCenterControlName="SparkleControl_MapMarker"` and bindings for
`TypeDataSource`, `SsdMarkerSource`, `MapMarkerSource` and `GlMarkerSource`.
It declares no `ActorMarkerDefaultTemplate`, `DefaultSparkleFile`,
`DefaultSparkleSequence` or `Filter`.

The form's three readers declare these `XmlFormat` values:

| Key | Sheet | `XmlFormat` |
| --- | --- | --- |
| `type_data` | `2Dmap_actor_data` | `Type,SparkleFile,SparkleSequence` |
| `ssd_marker_data` | `ssd_marker_data` | `,X,Y,,,,,Z,SparkleFile,SparkleSequence,Region,Layout,Floor,Template` |
| `map_marker_data` | `2Dmap_marker` | `,X,Z,,,,,Y,SparkleFile,SparkleSequence,Region,Layout,Floor,Template,,,,HideLayout` |

None names `Text`, `Visibility` or `Radius`.

## Entry positions

STATIC. Four list passes hand their node lists to `0x006805F0` and walk their
retained entries through `0x0067DBD0`, whose per-entry call is `0x0067BEC0`.

| Pass | Function | Source holder | Entry list |
| --- | --- | --- | --- |
| SSD | `0x00678C80` | `+0x498` | `+0x300` |
| Map | `0x00678FE0` | `+0x4A0` | `+0x370` |
| GL | `0x00679190` | `+0x4A8` | `+0x3A8` |
| Type | `0x00679340` | `+0x490` | `+0x338` |

The map pass alone reads control `+0x638`, `+0x63C` and `+0x614` into the walk
context (`0x0067906D..0x00679085`). Call those `Ox`, `Oy` and `r`.

In `0x0067BEC0`, with C the walk context and E the entry:

| Locator | Operation |
| --- | --- |
| `0x0067C067..0x0067C086` | attribute `X` (`0x00FC3384`) through the float reader `0x00670990` (default 0), divided by `C+0x10`, plus `C+0x08`, to E`+0x08` |
| `0x0067C089..0x0067C0A0` | attribute `Z` (`0x00FC3380`) the same way with `C+0x0C`, to E`+0x0C` |
| `0x0067C0A3..0x0067C0CB` | attribute `Y` (`0x00F79C88`), replaced by 0 when below 0, to E`+0x34` |
| `0x0067C0D0..0x0067C0DC` | the f32 `0x7FC00000` (a quiet NaN) at `0x012EA750` to E`+0x10`, and bit `0x01` of E`+0x3D` cleared |
| `0x0067C0E1..0x0067C138` | when the attribute `Layout` (`0x00FC3378`) is the empty string, attribute `Y` to E`+0x10`. A nonempty `Layout` leaves the NaN |
| `0x0067C13E..0x0067C19C` | 0 to E`+0x14`. A nonempty attribute `Radius` (`0x00FC3370`) sets bit `0x01` of E`+0x3D` and stores its `atof` value at E`+0x14` |

`0x00670990` reads the named attribute through the row's virtual `+0x1C` and
returns `atof` of a nonempty string as f32.

INFERRED. A `map_marker_data` row stands at `(X / r + Ox, Z / r + Oy)`. Its
`Layout` cell is column 11 of `2Dmap_marker`, typed `s32` in
`xivl-client-data:manifests/map_marker_resources.json` (`sourceHeaders`), so
a row's `Layout` attribute is not expected to be empty and E`+0x10` stays NaN
for every map row.

## Arrange

STATIC. `0x006759F0` walks the actor entry list `+0x2C8` through `0x0067DCF0`,
whose per-entry call is `0x0067CBC0`. The row lists reach `0x0067D030` through
`0x0067DE10`. For an entry with bits `0x02` and `0x04` of `+0x3D` set, both
routines:

- form `k = f32(+0x610) * f32(+0x60C) * f32(+0x614)`,
  `dx = (E+0x08 - control+0x640) * k` and `dy = (E+0x0C - control+0x644) * k`
  (`0x0067D055..0x0067D0E8`)
- keep the entry only while `-W/2 <= dx <= W/2` and `-H/2 <= dy <= H/2`, with
  the binary64 0.5 at `0x00F59898`. Otherwise `0x0093C2A0` takes 1
- when `MaskImageId` (`+0x4E0`) is not -1, pass to `0x009402C0` the product of
  the height factor below and an edge factor. With `m = min(W/2, H/2)` and
  `d = sqrt(dx*dx + dy*dy)`, the edge factor is 1 while `d <= m * 0.7`
  (binary64 `0x00FC33B8`) and `(m - d) / (m * 0.3)` (binary64 `0x00FB7DE0`)
  beyond it (`0x0067D162..0x0067D230`)
- place the entry at `x = W/2 + dx`, `y = H/2 + dy`, through `0x0093C2A0` with
  2 and `0x0092EBD0` with `(x, y, W - x, H - y)`

`0x0067D030` passes control `+0x618`, E`+0x10`, floor 0 and cut 1 to the height
factor `0x00670A40` (`0x0067D16F..0x0067D18A`). `0x0067CBC0` passes floor 0.4
(f32 `0x00F62F7C`) and cut 0 for an entry whose type byte `+0x3C` is 3, and
floor 0 and cut 1 otherwise (`0x0067CCFF..0x0067CD3E`).

`0x0067CBC0` gives the entry of the actor at control `+0x9E0` the order value
f32 11.0 (`0x00FB6E8C`) through `0x00940500`, and every other actor entry
`y / control+0x1AC + 9.0` (binary64 `0x00F886D0`) (`0x0067CFAF..0x0067D004`).

INFERRED. `0x0093C2A0` is the Visibility setter (1 hidden, 2 visible),
`0x0092EBD0` the Margin setter, `0x009402C0` an opacity setter and
`0x00940500` an order setter. An entry is a zero-sized slot at the placed
point. The self entry orders above every other actor entry.

## Height factor `0x00670A40`

STATIC. `0x00670A40(a, b, floor, cut)` starts with the result 1.0. It compares
the NaN at `0x012EA750` with b through `UCOMISD` and returns 1.0 only for an
ordered equal result, which a NaN operand never gives (`0x00670A40..0x00670A64`).
With `d = |b - a|` (`0x00670A6A..0x00670A92`):

- above 16.0 (f32 `0x00FA2B8C`) it returns 0 when cut is 1, and otherwise
  continues with `d = 16`
- at or below 4.0 (f32 `0x00FC3328`) it returns 1.0
- otherwise it returns `(16.0 - d) / 12.0 * (1.0 - floor) + floor` (binary64
  `0x00FC3320`, `0x00F66338`, `0x00F598A0`)

A NaN b makes both comparisons unordered, so it returns 1.0 through the 4.0
test. INFERRED. Control `+0x618` is the self actor's height and E`+0x10` an
actor entry's height, so an actor more than 4 above or below the player
fades and, with cut 1, is gone past 16. A map row with NaN at E`+0x10` keeps
factor 1.

## Actor entries

STATIC. The update `0x006795D0` rebuilds the actor entry list `+0x2C8`
through `0x0067F680` and updates each entry through `0x00680C20`, whose
per-entry call is `0x00680810`. In `0x00680810`:

- the type is `0x00585300`, the dword at actor `+0x270` masked with `0x3F`
  (`0x006808BC`)
- the actor at control `+0x9E0` takes 0 (`0x006808C8..0x006808D2`). Another
  actor takes -1 when `0x00585290`, bit 2 of the byte at actor `+0x26D`, is
  clear (`0x006808DA..0x006808E3`), and -1 when its type is 0 or above `0x40`
  unsigned (`0x006808FA..0x00680903`)
- the low byte goes to E`+0x3C`, and a type of `0x3F` skips to
  `0x00680BE3` (`0x0068090F..0x00680915`)
- otherwise `0x00680730` formats the type as text and `0x006727A0` selects
  the `TypeDataSource` row whose attribute `Type` (`0x00FC29CC`) equals it
- with bit `0x02` of actor `+0x274` set and `0x00585310`, bits 6 to 10 of
  actor `+0x270`, equal to 2, 3, 4 or 5, the file and sequence become
  `common/mapMarker.le.spk` (`0x00FC3390`) and `m00020` (`0x00FC3690`)
  (`0x006809E9..0x00680A32`)

INFERRED. The dword at actor `+0x270` is the marker number that
`_setMapMarker` stores ([map marker lanes](../actor/map-marker-lanes.md)), so
the low six bits of an actor's marker number select its `2Dmap_actor_data`
row and the self actor takes row 0.

UNKNOWN: the writers and meaning of bit 2 of actor `+0x26D` and bit `0x02` of
actor `+0x274`.

## Marker packages

MEASURED on `client/sqwt/common`. Header words are little-endian u16 at the
listed offsets.

| Package | Bytes | `+6` | `+8`, `+0xA` | `+0xC` | `+0xE` | u32 at `+0x14` |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| `mapMarker.le.spk` | 7016 | 113 | 200, 200 | 30 | 61 | 472 |
| `mapMarkerAetheryte.le.spk` | 472 | 7 | 32, 32 | 30 | 6 | 48 |
| `mapMarkerSymbol.le.spk` | 8380 | 141 | 32, 32 | 30 | 93 | 584 |
| `timerEffect.le.spk` | 424 | 4 | 80, 80 | 30 | 9 | 36 |

In all four the first cell at `+0x14` equals `0x14 + 4 * u16(+6)`, so the
word at `+6` counts the u32 cells from `+0x14` and the first section starts
where they end. The [SparkleControl page](sparklecontrol-source-sequence.md)
owns the cell and record reads.

## Reproduction

```text
ghidra disasm 0x00F11AF0 -n 6
ghidra disasm 0x00673B85 -n 600
ghidra disasm 0x0067A380 -n 12
ghidra disasm 0x00678FE0 -n 120
ghidra disasm 0x0067C050 -n 90
ghidra disasm 0x0067D030 -n 120
ghidra disasm 0x0067CCF0 -n 25
ghidra disasm 0x0067CFA0 -n 30
ghidra disasm 0x00670A40 -n 45
ghidra disasm 0x00680810 -n 160
ghidra disasm 0x00585290 -n 4
ghidra disasm 0x00585300 -n 4
ghidra disasm 0x00585310 -n 4
```

Repeat the initializer read for each address in the property table. Extract
the form with `xivl extract-resource` and read line 19 and the three reader
elements. Read the first 0x18 bytes of each package.
