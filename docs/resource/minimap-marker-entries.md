# MiniMap marker entries

The `MapScreenControl` of the [MiniMap base tiles](minimap-base-tiles.md)
page keeps retained marker entries for its data rows and for the actors of
the scene. This page records the marker properties, where an entry takes its
position, how arrange places, fades and orders an entry, how an actor entry
picks its row, the position visibility gate, the quest map index, the
navigation window markup and the header words of the marker packages. Control offsets
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

STATIC. The four source properties register like the marker properties
above. Each setter arm passes its descriptor and the holder address to
`0x0067D480`.

| Property | Name literal | Initializer | Descriptor | Tag | Holder and setter arm |
| --- | --- | --- | --- | --- | --- |
| `GlMarkerSource` | `0x00FC25D8` | `0x00F11B60` | `0x0133FFC4` | `0x0B` | `+0x490`, arm `0x00673CE7..0x00673CFB` |
| `SsdMarkerSource` | `0x00FC25E8` | `0x00F11B80` | `0x0133FFE8` | `0x0C` | `+0x498`, arm `0x00673D2E..0x00673D42` |
| `MapMarkerSource` | `0x00FC25F8` | `0x00F11BA0` | `0x0134000C` | `0x0D` | `+0x4A0`, arm `0x00673D6A..0x00673D7E` |
| `GroupMarkerSource` | `0x00FC2608` | `0x00F11BC0` | `0x01340030` | `0x0E` | `+0x4A8`, arm `0x00673DA6..0x00673DBA` |

STATIC. Four list passes hand their node lists to `0x006805F0` and walk their
retained entries through `0x0067DBD0`, whose per-entry call is `0x0067BEC0`.
The pass is named by the source property of its holder.

| Pass | Function | Source holder | Entry list |
| --- | --- | --- | --- |
| SSD | `0x00678C80` | `+0x498` | `+0x300` |
| Map | `0x00678FE0` | `+0x4A0` | `+0x370` |
| Group | `0x00679190` | `+0x4A8` | `+0x3A8` |
| GL | `0x00679340` | `+0x490` | `+0x338` |

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

STATIC. `0x0067D030` gives a row entry the order value
`y / control+0x1AC + C+0x0C + E+0x34` through `0x00940500`
(`0x0067D403..0x0067D448`), C being the walk context
`(control, W, H, base)` and E`+0x34` the row's `Y` attribute. The callers of
`0x0067DE10` set the base per list:

| Caller | Entry list | Base |
| --- | --- | --- |
| `0x006743B0` (`0x006743F2`) | `+0x338`, GL | f32 6.0 (`0x00FC33C0`) |
| `0x00674410` (`0x00674452`) | `+0x300`, SSD | f32 7.0 (`0x00FC33C4`) |
| `0x00675C30` (`0x00675D34`) | `+0x370`, Map | 0.0 (`XORPS` at `0x00675D01`) |
| `0x00675C30` (`0x00675D9C`) | `+0x3A8`, Group | f32 8.0 (`0x00FBAC18`) |

STATIC. After the actor walk, `0x006759F0` turns the entry of the actor at
control `+0x9E0`. It takes -0.0 (`0x00F62FA0`) less the float at actor `+0xAC`,
converts it to degrees modulo 360 through `0x006701B0` (binary64 180.0 at
`0x00F62F40`, pi at `0x00F62F48`, 360.0 at `0x00FC3300`), adds 180.0 and
writes the angle through `0x00990C10` to the transform at control `+0x59C`
(`0x00675A8E..0x00675ACC`). It then reads `MarkerCenterControlName`
(`+0x528`) and the float 0.5 (`0x00F62F60`) and sets the transform centre
through `0x00990C40`. The centre arithmetic was not read instruction by
instruction.

INFERRED. `0x0093C2A0` is the Visibility setter (1 hidden, 2 visible),
`0x0092EBD0` the Margin setter, `0x009402C0` an opacity setter and
`0x00940500` an order setter. An entry is a zero-sized slot at the placed
point. The self entry orders above every other actor entry. The list bases
order the row lists map, GL, SSD, group from the bottom before the share of
the height and the `Y` attribute are added. The self entry turns by 180
degrees less its actor's heading in degrees.

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

STATIC. The rest of `0x00680810`:

- With an empty `ActorMarkerDefaultTemplate` (`+0x4B8`) it casts the entry
  element to `SparkleControl` through `0x009DA6CC` and passes the file to the
  Source setter `0x00990470` and the sequence to the Sequence setter
  `0x00990250` ([SparkleControl page](sparklecontrol-source-sequence.md))
  (`0x00680A3A..0x00680AAD`). A nonempty name is passed to `0x00912850`
  instead (`0x00680ABE..0x00680AD7`).
- `0x004CFE30` with selector 1 gives the actor position. Lane 0 goes to
  E`+0x08`, lane 2 to E`+0x0C` and lane 1 to E`+0x10`
  (`0x00680AEC..0x00680B18`).
- With a non-null `Filter` (`+0x484`) whose first byte is nonzero, a
  nonzero type and no `m00020` override, it clears bits `0x01` and `0x04` of
  E`+0x3D` (`0x00680B62..0x00680B8D`). The text of the filter is not
  compared. The override stores 1 in a local flag at `0x00680A1C`, and
  `0x00680AB4` stores 0 on the other path.

STATIC. The update `0x006795D0` collects the actors through `0x00565060`
(`0x0067961B`) and clears the self actor slot `+0x9E0` before the rebuild
(`0x00679624`).

INFERRED. The dword at actor `+0x270` is the marker number that
`_setMapMarker` stores ([map marker lanes](../actor/map-marker-lanes.md)), so
the low six bits of an actor's marker number select its `2Dmap_actor_data`
row and the self actor takes row 0. A control with a nonempty `Filter` shows
the self entry and the overridden entries and hides every other actor entry.

STATIC. The only set of bit 2 of the element byte `+0x26D` is
`OR byte ptr [ESI+0x26D], 4` at `0x0058E407` in the `CharaElement` scene event
dispatcher. Its switch subtracts `0x4A` from the event and jumps through the
table at `0x0058E6B0` (`0x0058E0DF..0x0058E0EB`, table reference at
`0x0058E0F4`). The cell `0x0058E6DC`, index 11, sends event `0x55` to
`0x0058E3EC`, which calls virtual `+0x2C` of the `+0xBA0` subobject with the
result of `0x004D6750` and then sets the bit (`0x0058E3EC..0x0058E407`).
The cell `0x0058E6F4` of event `0x5B` is index 17.

STATIC. Among the 85 call sites of the scene post `0x007CE7B0`, the only one
that pushes the immediate event `0x55` is `0x0065FF37` in `0x0065FDA0`, with
a four-byte value 1 (`0x0065FF24..0x0065FF37`). The route through
`0x008064A0`, whose event is its argument, was not resolved. `0x0065FDA0`
has one caller, the actor update `0x006679C0` at `0x00668150`. It acts only
while bit `0x400000` of the scene actor dword `+0x2B70` is set and its step
counter is 0 (`0x0065FDB4..0x0065FDDC`). After the post it clears that bit
(`0x0065FF3C`) and later stores `0xED` at `+0x1170` (`0x0065FF57`).

STATIC. A byte search for the `+0x2B70` displacement followed by the
immediate `0x400000` finds only the set at `0x0065AA7C` and the test at
`0x0065FDCA`. `0x0065AA70(value)` sets the bit and stores the value at
`+0x1170` only when the value differs from `+0x1170`, and stores 0 there
instead when bit 0 of `[[this+0x2B5C]+0x4C]` is set (`0x0065AA70..0x0065AAA2`).
It has 182 direct callers between `0x007A3B43` and `0x007C1CB4`.

STATIC. `AND byte ptr [reg+0x26D], 0xFB` clears bit 2 of the element byte at
`0x00585DF5`, `0x00589D35` and `0x00589D91`.

INFERRED. Bit 2 of `+0x26D` reports that the scene actor finished a
requested motion change, so an actor entry waits until that report.

UNKNOWN: the meaning of bit `0x02` of actor `+0x274`.

## Position visibility

STATIC. The name literal `PosVisibility` at `0x00FC282C` is registered by
the initializer `0x00F11F70` with the descriptor `0x01340420` and tag
`0x2A` (stored at `0x01340434`). The constructor `0x00679ED0` registers the
holder `+0x57C` (`0x0067A6CC..0x0067A6D8`).

STATIC. The update `0x006795D0` sets the holder from `0x0067972D`:

- When the dword `+0xA54` equals the dword `+0x9F4` and the dword `+0xA58`
  equals the byte `+0x9FC`, the value is `1 + (dword +0x4F8 != -1)`, written
  when it changed (`0x0067972D..0x00679755`, `0x006797F3..0x0067983D`).
- Otherwise, when `Function` (`+0x448`) is not 0, the holder is left
  unchanged (`0x0067975A..0x00679760`).
- Otherwise, when `QuestMapIndex` (`+0x56C`) is -1, the value is 1
  (`0x00679766..0x0067976F`, `0x00679844..0x00679870`).
- Otherwise the key `+0x9F4` is aliased, `0x14B` to `0x141`, `0x83` to `0x79`
  and `0x1A6` or `0x1AF` to `0x1A5` (`0x00679775..0x006797A1`), and compared
  with `+0xA54`. Equal keys give `1 + (dword +0x4F8 != -1)`
  (`0x006797EB..0x0067983D`). Unequal keys write 1 through `0x00675540` and
  pass the row count of the resource at `+0x9E8` (`[+0xDC] - 1`) to
  `0x00942DB0` (`0x006797AA..0x006797E1`).

The s2c `0x018D` consumer tests the byte `+0x57C` against 2 at `0x00671574`
([0x018D consumer](../net/s2c-018d-client-consumer.md)).

STATIC. `+0x4F8` is the `PlayerPosX` holder. The name literal `PlayerPosX`
at `0x00FC272C` is registered by the initializer `0x00F11D80` with the
descriptor `0x01340228` and tag `0x1C` (stored at `0x0134023C`). The setter
arm `0x00673F61..0x00673F82` passes the descriptor and the holder
`+0x4F8` to `0x00596950`, and the constructor registers the descriptor at
`0x0067A3DC..0x0067A3DE`. The writer `0x00675840` tests `+0x9F0` against 1,
reads `+0x638`, `+0x63C` and the binary64 at `0x00F55AD0`, and writes the
holder through the descriptor (`0x0067584A..0x006758FF`).

STATIC. The aliases above are, in decimal, 331 to 321, 131 to 121, and 422
or 431 to 421.

INFERRED. `PosVisibility` is 2 while the control shows the map that holds the
player and `PlayerPosX` is set, so the `0x018D` rows are taken only then.
Which values `+0xA54` and `+0xA58` hold was not traced.

## Quest map index

STATIC. The name literal `QuestMapIndex` at `0x00FC2814` is registered by the
initializer `0x00F11F20` with the descriptor `0x013403D8` and tag `0x28`
(stored at `0x013403EC`). The setter arm `0x00674201..0x00674228` passes the
descriptor and the holder `+0x56C` to `0x00596950`. The typed writer
`0x00675480` passes the same descriptor and holder to `0x00596520`.

STATIC. The SSD pass `0x00678C80` reads the holder at `0x00678CBD`. With
`+0x56C` not -1 and a source at `+0x498` whose `+0x10` is not null:

- it calls virtual `+0x20` of the source for its row list, formats the
  index with `%d` (`0x00FC29D4`) and looks up the row whose `ssdindex`
  (`0x00FC29D8`) equals that text through `0x00DC2F00`
  (`0x00678CCA..0x00678D5B`)
- with no such row it takes the index as a position through `0x0067B3F0`
  when it is below the row count (`0x00678D8A..0x00678DB5`)
- from the row it reads `Layout` (`0x00FC29E4`) and `Floor` (`0x00FC29EC`)
  through `0x00670850` and writes them through the `Layout` writer
  `0x00674640` (descriptor `0x0133FEEC`, holder `+0x470`) and the `Rect`
  writer `0x006746A0` (descriptor `0x0133FF10`, holder `+0x478`). It writes
  `Ready` 1 through `0x00674700` (descriptor `0x0133FF34`, holder `+0x480`)
  and sets the load request bit `+0xA69 & 0x10`
  (`0x00678DBB..0x00678E05`)
- it reads `X` (`0x00FC29F4`) and `Z` (`0x00FC29F8`) through `0x00670990`,
  and when the pair differs from `+0x630/+0x634` (`0x0067AAE0`) stores it
  there and calls `0x00672D30` (`0x00678E0D..0x00678E5E`)

STATIC. `0x00672D30(pair)` compares the pair with `+0x640/+0x644`, passes a
changed pair through `0x00672A80`, stores the result at `+0x640/+0x644` and
sets bits of `+0xA68` (`0x00672D30..0x00672E13`). Another update calls it
with `+0x630` while `+0x56C` is not -1 (`0x006783CD..0x006783E5`).

STATIC. The constructor `0x00679ED0` registers `0x00671E00` six times through
`0x006804F0` (`0x00679F55..0x00679FC7`). `0x00671E00` refuses a row whose
dword `+4` is 8. For a row with a nonempty `Layout` (`0x00FC2988`), a zero
sixth argument and `+0x56C` not -1, it refuses the row unless its `ssdindex`
(`0x00FC2990`) equals `+0x56C` (`0x00671EBF..0x00671EE9`). The following
compare at `0x00671EEF..0x00671F21` tests the control's target layout and
page against themselves, so a matching row is kept whatever its own layout.
On the other branch a row whose `HideLayout` (`0x00FC299C`) equals a set
`+0x9F8` is refused (`0x00671F47..0x00671F65`).

STATIC. Two native callers of `0x00675480` write the index:

- `MapNavigationWidget` vtable `0x00F9EFB4` slot 8, `0x004FCA20`. With its
  `+0x254` equal to 3 it calls `0x004F1850(1, &[+0x258])` and passes
  `[+0x258]` to `0x00675480` on the control at `+0x24C`
  (`0x004FCA24..0x004FCA56`).
- `MapMenuTopJournal` vtable `0x00F9D3D0` slot 3, `0x004FCEB0`. For an
  argument other than 8 it passes the dword at `this+0x10+4*argument` to
  `0x00675480` on the control at `[[this+8]+0x24C]`
  (`0x004FCED9..0x004FCEEB`).

INFERRED. A `QuestMapIndex` write shows the `ssd_marker_data` row of that
`ssdindex`, loads its layout and floor as the shown page, and centres the
map on its `X` and `Z`. Who sets `MapNavigationWidget` mode 3 was not
traced.

## Navigation window markup

MEASURED. `client/sqwt/widget/MapNavigationWidget.form` (decoded SHA-256
`2012be5906707b43eadadcde937045257eed7735e9afec7a43b6b2bab5a4fa35`),
decoded with `xivl extract-resource`:

- The map control declares `Filter="1"` and binds `TypeDataSource`,
  `GroupMarkerSource`, `GlMarkerSource`, `SsdMarkerSource` and
  `MapMarkerSource`. `GroupMarkerSource` binds the `SqwtXmlDataMaker`
  `group_marker_data` (`Name="GroupMakerData"`, `XPath="/root/marker"`),
  which declares no `XmlFormat`.
- The form's Buttons, ListBoxes and Sliders are `Button_Close`
  (`Command="UILuaCommands.Operate"`), `ListBox_NavigationMenu`,
  `ListBox_MarkerSelectMenu` and `Slider_TransparencyConfig`.
  `Map.ScaleUp` and `Map.ScaleDown` occur only as key commands of the map
  control.

`MiniMapWidget.form` declares no `GroupMarkerSource`.

MEASURED. `MapNavigationWidget.form` holds no `UILuaCommands.Activated`
text. Its window template `ControlTemplate_Window_MapNavigationWidget` in
`client/sqwt/widget/MapNavigationWidget.tpl` (file SHA-256
`da386f16168c709a1533abe76ec55d6675533079fc34c4d8be719b60cd977483`,
decoded SHA-256
`5394098335ccfa4fb6577ee09d1aefbcaaed4f6a7e34332c9313f16f389cc6be`)
declares `EventTrigger RoutedEvent="Shown" Name="WindowOpen"`, whose
`BeginStoryboard` Storyboard has `CompletedCommand="UILuaCommands.Activated"`
and a `DoubleAnimation` of `VisualOpacity` from 0 to 1 over `0:0:0.2`. The
`MapNavigationWidget.sd.tpl` variant (decoded SHA-256
`4b9e44a2082a9a21cd46404f9ac26292d54fc03c1a652b7c7c2f872ba5abd22b`) carries
the same completion. Many other window templates of `client/sqwt` carry an
`Activated` completion too, among them the `WINBasis` families.

INFERRED. The navigation window's `Activated` command reaches its script when
the `Shown` fade completes, about 0.2 s after the window shows.

MEASURED. `client/sqwt/common/MapMarker.tpl` (decoded SHA-256
`68a685d900291186ba0b6472e8a6f9054459d2b2d874913725ee0fb8fe1e5555`)
declares the template `MapMarkerParty` as a StackPanel of
`TextBlock_Caption` (style `TBL_chat_inputParty`, `Text` bound to `@Text`)
and `SparkleControl_MapMarker` (`Source="common/mapMarker.le.spk"`,
`Sequence` bound to `@SparkleSequence`, 32 by 32).

INFERRED. The navigation window shows the self entry and no other actor
entry, and party members reach it only as group rows.

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
ghidra disasm 0x00F11B60 -n 20
ghidra disasm 0x00673CE0 -n 60
ghidra disasm 0x00680A13 -n 100
ghidra disasm 0x00675A2D -n 60
ghidra disasm 0x0067D3A2 -n 50
ghidra disasm 0x006743B0 -n 25
ghidra disasm 0x00674410 -n 25
ghidra disasm 0x00675CF0 -n 50
ghidra disasm 0x00F11F70 -n 5
ghidra disasm 0x0067A6C0 -n 10
ghidra disasm 0x0067972D -n 80
ghidra disasm 0x0058E0D8 -n 10
ghidra memory read 0x0058E6B0 108
ghidra disasm 0x0058E3EC -n 9
ghidra x-ref to 0x007CE7B0
ghidra disasm 0x0065FDA0 -n 140
ghidra disasm 0x0065AA70 -n 14
ghidra find bytes 702b000000004000
ghidra x-ref to 0x0065AA70
ghidra disasm 0x00F11D80 -n 5
ghidra disasm 0x00673F61 -n 10
ghidra disasm 0x00675840 -n 60
ghidra disasm 0x00F11F20 -n 5
ghidra disasm 0x00674201 -n 12
ghidra disasm 0x00675480 -n 16
ghidra disasm 0x00678CBD -n 120
ghidra disasm 0x00672D30 -n 80
ghidra disasm 0x006783C0 -n 12
ghidra disasm 0x00671E00 -n 130
ghidra find bytes 001e6700
ghidra x-ref to 0x00675480
ghidra disasm 0x004FCA20 -n 20
ghidra disasm 0x004FCEB0 -n 20
```

For the event `0x55` scan, disassemble the instructions before each
`0x007CE7B0` call site and read the third push before the call. Read the
strings at `0x00FC2814`, `0x00FC272C`, `0x00FC2988` to `0x00FC29F8` and the
vtable slots of `MapNavigationWidget` (RVA `0xB9EFB4`) and
`MapMenuTopJournal` (RVA `0xB9D3D0`) in the
[vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl).

Repeat the initializer read for each address in the property tables. Extract
the forms, `widget/MapNavigationWidget.tpl`, `widget/MapNavigationWidget.sd.tpl`
and `common/MapMarker.tpl` with `xivl extract-resource` and read
line 19 and the three reader elements of the minimap form, the map control,
readers and controls of the navigation form, and the `MapMarkerParty`
template. Read the first 0x18 bytes of each package.
