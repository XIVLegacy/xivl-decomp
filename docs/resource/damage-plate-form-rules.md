# DamagePlate form rules

[Documentation index](../README.md) | [Numeric presentation](damage-plate-numeric-presentation.md) | [Combat result path](../net/combat-result-element.md#damage-plate)

This page records the native rules that decide how the identified
`DamagePlateWidget.form` is opened, laid out, coloured and replayed: the
form path, the per-plate row selection, the five-slot ring, the icon and
text roles, the status icon map, the colour classifier inputs, Canvas
measurement, the Foreground path to the drawing peer and the storyboard
clock. These are
static mechanics. They do not establish rendered pixels or a live plate.

## Identity and method

The executable is `ffxivgame.exe`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All locators are VAs. Subtract the image base for
RVAs.

The method is Python capstone 5.0.7 disassembly of file-backed PE section
bytes mapped through the section table, ghidra-cli 0.2.2 cross-reference
and function queries on Ghidra 12.1.4, direct little-endian data reads and
RTTI traversal from complete object locators. Decompiler renderings were
leads only.

STATIC denotes inspected instructions or original data. INFERRED denotes a
bounded interpretation of them, with the join stated. MEASURED denotes a
count or value read from a cited tracked study or catalog. UNKNOWN retains
an unresolved edge. Install observations come from a retail 1.23b install,
game.ver `2012.09.19.0001`. They decode sheet definitions with
`xivl extract-resource --as scrambled-xml` (`xivl-tools:docs/cli.md`) and
read the enable, row-offset and string resources that each definition
names by the contracts in `xivl-tools:docs/formats/ssd-sheet.md`. They name
resources, ranges, counts and token structure only, never cell text.

The bodies and data that the text role and status icon map sections cite
were read from the file through the section table and hashed as follows.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00795410,0x007955E9)` text role setter | 473 | `5a4902b9b66965bc5c50c1989220714a6bbe0991f299eb50b279bae198160cbf` |
| `[0x007955F0,0x0079562B)` text pair request | 59 | `76d7e70c1a8429635212a2280b99430f9364e283ff617f9c5762542ddd781a9a` |
| `[0x009556A0,0x009556D0)` TextBlock Text setter | 48 | `98841cb2cc781d2c8ab2a4c2fbfa1c5d51d5e8ecd5b0da0d32d644a104f7c152` |
| `[0x007950A0,0x00795277)` DamagePlate constructor | 471 | `5c50626ccf33909a94ba3219b28ab0dd3dc437b9392c8a133ed48c52076ec54e` |
| `[0x00795030,0x00795094)` status icon lookup | 100 | `5f7218082306a42be01c7282a160584c00ef793cc3fa5a86302cc8dd7a1c5468` |
| `[0x00A44B00,0x00A44B69)` map find | 105 | `3be84050d552bc5febe06a6be1b509ce5db9dd0cc3a4d6ea9b57ade5bde24229` |
| `[0x00F198C0,0x00F198D6)` map static initializer | 22 | `5b3fa628627243a49bb03d083da97f8856e536fad327a813119b81e7e0f97a70` |
| `[0x00F33430,0x00F3343A)` map exit thunk | 10 | `39514000703add6d0fd5eda5a361babeeef40c2a767632dbb74cee71ecdc7951` |

| Data, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00FE1610,0x00FE162C)` status icon pair table | 28 | `c90167018d9c46749f60164530f741d2a1aaddd01a6f43732214edcb2d0a26cb` |
| `[0x00FE1878,0x00FE1882)` text markup literals | 10 | `b19440260bdfe901fd018f250530f3dc24201301f5a0fbc67d9a11a151374e34` |

## Form file selection

The list initializer pushes the UTF-16 literal
`\system\ingame\DamagePlateWidget.form` at `0x00FE1888` and calls
`0x009104A0` at `0x0079518D`. That entry is a jump to the raw reader
`0x00910190`, which calls `0x0090FBC0` and `0x0090F960`. Neither
`0x00910190`, `0x0090F960` nor `0x0090FBC0` directly references the language
token global `0x01356B38`, its getter `0x0090F950` or the insertion routine
`0x0096A3B0`. The only raw references to `0x01356B38` are at `0x0090F909`,
`0x0090F936` and `0x0090F951`, outside those bodies. The getter is called
only from `0x008E310A`, `0x0096A497` and `0x009A1F68`, and the insertion
routine only from `0x0096BB07`. STATIC. The other callees of `0x00910190`,
including `0x0090FD10`, are not followed here.

The plate therefore requests the plain path, not the language-inserting
opener on the [path resolution](sqwt-form-locale-resolution.md#the-opener)
page. That page also finds no loader of `.sd.form` or `.sd.tpl` files. A
supplied 1.23b install holds `DamagePlateWidget.form`,
`DamagePlateWidget.tpl`, `DamagePlateWidget.sd.form` and
`DamagePlateWidget.sd.tpl` in `client/sqwt/system/ingame` and no language
variant. The plain `.form` is the selected document. INFERRED from the
static path and the install census.

## Plate creation and row selection

The creation gates and anchor resolution are on the
[combat result](../net/combat-result-element.md#damage-plate) page. Plate
routine `0x00795630` decodes the row effect ID E at `0x007956D4..0x00795729`
into these locals. STATIC.

| Field | Expression |
|---|---|
| kind | `(E >> 27) & 15` |
| s9 | `(E >> 9) & 63` |
| s9n | `(E >> 9) & 15` |
| s8 | `(E >> 8) & 63` |
| low5 | `E & 31` |
| low2 | `E & 3` |
| id18 | `E & 0x3FFFF` |

E equal to zero exits before any slot work. `kind - 1` indexes the
ten-entry table at `0x007967CC`. Kind 0 and kinds above 10 take the
default branch `0x007963AE`. STATIC.

Every branch makes the same four kinds of request. The icon request
`0x00794A80` receives the slot's `IconControl_Icon` and a value. The palette
request `0x00794C00` receives the digit roles, both text roles, the
classification and a final selector. The text request `0x007955F0` gives
`TextBlock_Additional` one text ID and `TextBlock_DamageType` another. The
digit request `0x00795280` receives either -1 or the row numeric dword at
record `+0x3C + 20*row`. Each branch then pushes one command name and joins
the dispatch tail at `0x007963FC`. The table lists every branch. "num" is
the row numeric dword, and a dash is -1. STATIC.

| Kind | Condition | Icon | Palette | Additional | DamageType | Digits | Command |
|---|---|---|---|---|---|---|---|
| 1, 2, 7 | s9 = 0 | - | class, selector 1 | `0x7D2` | - | - | `MissPlateAnime.Popup` |
| 1, 2, 7 | s9 = 1 | - | class, selector 1 | `0x7D3` | - | - | `MissPlateAnime.Popup` |
| 1, 2, 7 | s9 = 5 | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |
| 1, 2, 7 | other s9, low2 = 3 | - | class, selector 0 | `0x7D1` | - | num | `DamagePlateAnime.Popup` |
| 1, 2, 7 | other s9 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 3 | s9 in 1..8 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 3 | other s9 | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |
| 4, 10 | none | status icon | class, selector 3 | `0x7D6`, parameter id18 | - | - | `StatusPlateAnime.Popup` |
| 6 | s8 = 1 | - | fixed | - | - | num | `CurePlateAnime.Popup` |
| 6 | s8 = 2 | - | class, selector 0 | - | `0x7D4` | num | `DamagePlateAnime.Popup` |
| 6 | s8 = 3 | - | fixed | - | `0x7D4` | num | `CurePlateAnime.Popup` |
| 6 | s8 = 4 | - | class, selector 0 | - | `0x7D5` | num | `DamagePlateAnime.Popup` |
| 6 | s8 = 5 | - | fixed | - | `0x7D5` | num | `CurePlateAnime.Popup` |
| 6 | other s8 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 8 | none | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |
| 9 | s9n = 1, low5 = 1 | - | class, selector 0 | - | `0x7D4` | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 1, other low5 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 2, low5 = 0 | - | fixed | - | - | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 2, low5 = 1 | - | fixed | - | `0x7D4` | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 2, low5 = 2 | - | fixed | - | `0x7D5` | num | `DamagePlateAnime.Popup` |
| 9 | other | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 0, 5, above 10 | none | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |

"class" means the classification palette below. "fixed" means the branch
skips `0x00794C00` and writes the eight float lanes at `0x012C4214` to
Foreground and FontEdgeColor on each non-null digit and text role itself.
Those lanes are `float32(n/255)` for Foreground `(255,142,255,174)` and
FontEdgeColor `(255,0,52,104)`. Inside `0x00794C00` only final selector 2
selects that same table, at `0x00794C03..0x00794C3F`. Selectors 0, 1 and 3
all select by classification. STATIC.

The [text role](#text-role) section traces how each text ID and parameter
become TextBlock Text and Visibility.

The scatter offset branch excludes only `CurePlateAnime.Popup` and
`StatusPlateAnime.Popup`, so `MissPlateAnime.Popup` and
`DamagePlateAnime.Popup` plates both take it. STATIC, from the comparisons
at `0x007964C0..0x00796545`.

## Text role

Pair request `0x007955F0(roleA, roleB, idA, idB, paramA, paramB)` is a
thiscall that returns with `ret 0x18` at `0x00795628`. When roleA is
non-null it calls `0x00795410(roleA, idA, paramA)` at `0x00795608`. When
roleB is non-null it calls `0x00795410(roleB, idB, paramB)` at
`0x00795622`. STATIC.

Slot creation looks up `TextBlock_Additional` (`0x00FE18F8`) into slot
`+0xA0` at `0x00795873..0x00795895` and `TextBlock_DamageType`
(`0x00FE1924`) into slot `+0xB4` at `0x007958BF..0x007958E1`. The plate
routine loads them into EDI and EBP at `0x00795917..0x0079591E`. Each of
the 16 calls to `0x007955F0` passes EDI as roleA and EBP as roleB. Direct
calls `0x00795410(role, -1, -1)` at `0x00795960` and `0x00795970` reset
both roles before the kind switch, and the default branch repeats them at
`0x007963D8` and `0x007963E8`. A rel32 call scan of `.text` finds exactly
those 16 and 6 call sites. STATIC.

| Text ID | Role | Parameter | Call sites |
|---|---|---|---|
| `0x7D1` | Additional | -1 | `0x00795B38` |
| `0x7D2` | Additional | -1 | `0x00795A5D` |
| `0x7D3` | Additional | -1 | `0x00795AA7` |
| `0x7D4` | DamageType | -1 | `0x00795DDF`, `0x00795EE8` |
| `0x7D5` | DamageType | -1 | `0x00795F35`, `0x0079603A` |
| `0x7D6` | Additional | id18 | `0x00795C8D` |

Every other text ID and parameter at these sites is -1. The `0x7D6` call
reloads `[ESP+0x1C]` at `0x00795C7B` and pushes it as paramA. That cell
holds `E & 0x3FFFF` from `0x00795729` and `0x0079573D`. Table
`0x007967CC` sends only kinds 4 and 10 to `0x00795C50`, and no direct jump
targets it, so only those kinds pass P = id18. STATIC.

Setter `0x00795410(role, T, P)` returns at once for a null role
(`0x00795442`). For T < 0 it builds a string from the empty literal at
`0x00FE14D6`, writes it as Text through `0x009556A0` at `0x0079547F` and
then writes Visibility byte 0 through `0x0093C2A0` at `0x007954A8`. For
T >= 0 it first writes Visibility byte 2 at `0x007954C2` and then builds
markup in a string stream. Constructor `0x005440F0` runs with mode 3 and
installs table `0x00FA0DE4`, whose complete object locator names
`.?AV?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@`.
STATIC.

For P >= 0 (`0x007954E9`) the setter inserts `@` (`0x00FE187C`), T, `/i`
(`0x00FE1878`) and P. Otherwise it inserts `@` (`0x00FE1880`) and T.
`0x004E7290` inserts the literals and `0x005434A0` the integers.
`0x005404E0` extracts the string at `0x0079553B`, and `0x009556A0` writes it
as Text at `0x00795565`. The integer inserter calls `num_put` table
`0x00F91BC8` slot `+0x1C` through `0x0053BA60`. That slot is `0x004E79D0`,
which formats with the literal `ld` at `0x00F91C18`. STATIC. T and P are
therefore written as signed decimal, given the fresh stream's default base.
INFERRED. The Text written is `@T` or `@T/iP`. The plate writes `@2001`
through `@2005`, or `@2006/i` followed by id18.

`0x009556A0` pushes descriptor `0x0135A300` and calls `0x009BA4E0` with
control `+0x33C`. Registration `0x00F23800` names that descriptor `Text`
(`0x0106E67C`). Registration `0x00F22900` names the `0x0093C2A0` descriptor
`0x013596BC` `Visibility` (`0x0106BEB8`). `0x009BA4E0` calls the
`0x00916310` preprocessing at `0x009BA516` only when property bit `0x40`
is set (`0x009BA4E6`). STATIC. The enum names of Visibility values 0 and 2
are not established here.

The [plain text reference](sqwt-plain-text-reference-lookup.md) page,
sections "Default table and named language fields", SQTP-01, SQTP-03 and
SQTP-04, owns the resolution of that markup. Under its admission
conditions, `@N` reads row N of the default table `xtx/_text_ui` in the
field the language table selects. Each `/iV` segment appends a tag 0
integer argument, which expression `0xE8` reads one-based.
`xivl-client-data:manifests/sheet_inventory.csv`, row `xtx/_text_ui`,
identifies definition `0x0B450082`. Text IDs `0x7D1` through `0x7D6` are
therefore `xtx/_text_ui` rows 2001 to 2006, and a status plate supplies
id18 as argument 1 of row 2006. INFERRED, conditional on property bit
`0x40` at byte `+4` of each role's Text property and on the SQTP admission
gates.

In the install, definition `0x0B450082` gives each of its `ja`, `en`,
`de`, `fr` and `chs` sheets a block 0 that begins at row 0. The block 0
enable resource that the definition names for each language holds the
pair `(2001, 6)`, one enable range covering rows 2001 to 2006, and rows
2000 and 2007 have empty row-offset spans. Install observation. No tracked
artifact states the text of rows 2001 to 2005, so their meanings are not
recorded here.

In the same blocks, row 2006 holds no literal text. Its string body is two
handler `0x28` tokens, which the
[rich-text handlers](rich-text-sheet-and-case-handlers.md#rt-03-handler-0x28-named-sheet-text)
page traces. Each names sheet `xtx/status` as a literal string expression
and takes supplied argument 1 as its row. `sheet_inventory.csv` lists that
sheet as definition `0x0B450073`. The first token gives literal field 24. The second
gives the language's name field, field 3 in the `en` block and field 1 in
the `ja` block. Install observation.

The installed `xtx/status` definition `0x0B450073` has one sheet per
language, and each lists its own logical fields. The `ja` sheet lists
fields 1, 20 and 24, and its `SHEET|DEF` parameter names field 1. The `en`
sheet lists fields 2 to 7, 21 and 24, and its `SHEET|DEF` parameter names
field 3. Install observation, STATIC for the schema. Each token field is
therefore a logical field of the definition for its block's language.
`xivl-client-data:docs/status-names.md`, section "Columns and limits",
numbers `xtx_status.csv` columns as zero-based sheet positions and records
column 24 as the encoded icon control and columns 3 and 1 as the English
and Japanese names. Reading logical field N as that column N is INFERRED,
from those positions and from the `SHEET|DEF` name fields, which fall on
the same columns 3 and 1. The `FUN_00CAF330` field-to-column mapping that
RT-03 cites is not traced for this sheet. RT-03 shows that a missing sheet
or row fails the whole formatted row, so an id18 that is not an
`xtx/status` row fails row 2006. STATIC there. What the TextBlock shows
after that failure is UNKNOWN.

## Icon role

Slot creation looks up `IconControl_Icon` (`0x00FE18D4`) into slot
`+0x28` at `0x007957B1..0x007957E2`. Every plate branch calls `0x00794A80`
with that pointer. A null pointer does nothing. A negative value requests
Visibility 0 and returns. A nonnegative value requests Visibility 2,
formats the value with `%d` (`0x00FE17D4`) and writes it through
`0x0096A010`, whose descriptor `0x0135B23C` is named `IconDatas`. STATIC.

Only kinds 4 and 10 pass a value other than -1. That value is
`0x00795030(id18)`, which searches the map at `0x0134C8F8` and returns its
node `+0x10` or -1 on a miss. STATIC. Every numeric plate therefore requests
a collapsed icon, so the declared IconID 30 is not shown by those paths.
INFERRED from the requests, conditional on property admission. The
[status icon map](#status-icon-map) holds three keys, so a status plate
requests a visible icon only for id18 231001, 231002 or 231003. INFERRED,
from the constructor fill and the writer scan in that section.

## Status icon map

Static initializer `0x00F198C0` calls `0x007A0AD0` with ECX `0x0134C8F8`
and passes `0x00F33430` to `0x009D25C2`. `0x007A0AD0` allocates a head
node, links it to itself, sets its byte `+0x15` to 1 and stores size 0 at
object `+8`. `0x00F33430` loads the same object and jumps to `0x0081F1B0`.
The dword at `0x00F48F3C` is `0x00F198C0`. It follows the dwords
`0x00F19820`, `0x00F19850`, `0x00F19880` and `0x00F198B0` and precedes a
zero dword. STATIC. Reading `0x00F48F3C` as a dynamic initializer table
entry and `0x009D25C2` as an exit registration is INFERRED from that
layout.

The object is a `std::map<int,int>` with its head at `+4` and size at `+8`.
Nodes hold the key at `+0xC`, the value at `+0x10` and a nil flag at
`+0x15`, and `0x00A44B00` compares keys signed. INFERRED from those
layouts. No RTTI names the container.

The only writer is constructor `0x007950A0`, which installs table
`0x00FE1E34`, `.?AVDamagePlate@Chara@Element@Main@Application@@`. Its only
call site is `0x0058B768`. EBP is zero from `0x00795107`. At `0x00795220`
the constructor compares size dword `0x0134C900` with EBP and skips the
fill when it is nonzero. The loop at `0x00795228..0x0079525A` reads the
value at `0x00FE1614 + 8i`, passes the key address `0x00FE1610 + 8i` to
`0x00871D00` on the map, and stores the value through the returned cell. It
stores the first pair before any test, then continues while the next key is
at least zero (signed `jge` at `0x0079525A`). STATIC. Reading `0x00871D00`
as the map's `operator[]` is INFERRED from that store.

The `.rdata` table at `0x00FE1610`, file offset `0x00BE1610`, holds these
pairs and ends with key -1 at `0x00FE1628`. STATIC.

| Key, id18 | Value, icon ID |
|---|---|
| 231001 | 10001 |
| 231002 | 10002 |
| 231003 | 10003 |

The only reader is `0x00795030(key)`, a cdecl routine whose only call site
is `0x00795C55` in the kind 4 and 10 branch. It calls `0x00A44B00`, a
lower bound with the signed compare at `0x00A44B1A` and an exact key check
at `0x00A44B3F` that falls back to the head. When the found node equals
the head `[0x0134C8FC]` it returns -1 (`0x0079506F`). Otherwise it returns
node `+0x10` (`0x0079508A`). STATIC.

A scan of the whole file for the little-endian dwords `0x0134C8F8`,
`0x0134C8FC`, `0x0134C900`, `0x00FE1610` and `0x00FE1614` finds them only
at file offsets `0x395041`, `0x39505A`, `0x395239`, `0xB198C1` and
`0xB33431`, `0x395052`, `0x395222`, `0x39522D`, `0x395250` and `0x395256`,
and `0x395233`, in that order of value. Each lies in the reader, the
constructor fill, the initializer or the exit thunk. The dwords
`0x00FE1618` through `0x00FE1628` and `0x0134C8F4` have no hits. STATIC. No
sheet loader or Lua native writes the map. INFERRED from the scan, which
does not see computed addresses.

The icon request therefore receives 10001, 10002 or 10003 for those three
id18 values and -1 for every other id18. A -1 collapses the icon role. A
listed value requests Visibility 2 and IconDatas `10001`, `10002` or
`10003`. INFERRED, through the `std::map<int,int>` layout, the
`operator[]` reading of `0x00871D00` and the writer scan above. The request
side is STATIC in the [icon role](#icon-role). The
[resource calculation](../actor/player-nameplate-icons.md#form-and-resource-evidence)
maps those IDs to `0x1C5A0001`, `0x1C5A0002` and `0x1C5A0003`. INFERRED
for this form, because the path from the DamagePlate IconControl through
`0x0096A010` to texture manager `0x004B6670` is traced on that page and
not again for this widget.

`xivl-client-data:derived/status_names.csv` holds rows 231001 to 231003 of
`status`, definition `0x01030198`. Its `status_col_25` values for them are
10012, 10015 and 231, so the map is not that column. MEASURED.
`xivl-captures:studies/battle-result-backfit/derived/battle-result-rows.csv`
column `effect_id` has 57 kind 4 rows with 13 distinct id18 values from
223013 to 223999 and no kind 10 row. None is a map key. MEASURED. Every
captured status plate therefore requested a collapsed icon. INFERRED.

## Per-plate Window writes

Before the branch table, every plate writes three properties on the slot
Window held at wrapper `+0x2C`. STATIC.

| Call | Descriptor | Value |
|---|---|---|
| `0x0079598E` to `0x0092EFB0` | `0x01358A0C`, HorizontalAlignment | byte 1 |
| `0x007959D0` to `0x0092EBD0` | `0x013589E8`, Margin | four floats `(0, 0, [D+0x1B4], [D+0x1AC])` |
| `0x007959DF` to `0x0093C3C0` | `0x0135975C`, Focusable | 0 |

D is the global at `0x01356E10`, which the
[selection producers](../input/chara-selection-producers.md) page names
the desktop global. HorizontalAlignment text reader `0x00914670`, reached
through `0x00927200` from the FrameworkElement string setter at
`0x00930568..0x00930589`, maps `Left` to 0, `Center` to 1, `Right` to 2 and
any other text to 3. Byte 1 is therefore Center. STATIC.

The FrameworkElement constructor at `0x0092F6D5..0x0092F7D1` binds
ActualHeight to `+0x1AC` and ActualWidth to `+0x1B4`. If D has that layout,
the native Margin carries the desktop's actual width in lane 2 and its
actual height in lane 3. MeasureCore `0x0092B880` adds lanes 0 and 2 to the
horizontal size and lanes 1 and 3 to the vertical size. INFERRED. Which of
lanes 0 and 2 is the left edge is not established. UNKNOWN.

## Five-slot ring

The plate list is per anchor element. The combat drain calls
`0x00795630` on the list at anchor `+0xD3C`, and the list initializer runs
from the element constructor. Slot pointers live at list `+0x14 + 4*i`,
indexed by the low nibble of list `+0x10`. A slot whose pointer is null is
built once at `0x00795747..0x007958F3` and kept. `0x00795931..0x00795952`
advances the nibble and wraps it to zero after 4. STATIC.

Each plate reuses the next slot's existing Window and retained role
pointers. Its own pre-branch requests reset both text roles to empty and
collapsed. The branch then rewrites the icon, palette, text and digit roles
before dispatching the command. Nothing in `0x00795630` stops or rewinds a
previous storyboard before the dispatch. STATIC.

The dispatch tail requests the show path `0x0096DE30(1, 0)` on the slot
container, resolves the command name through `0x00928280` and calls the
resolved object's virtual `+8` with the Window's `+0xB4` part as target
(`0x00796711..0x00796761`). STATIC. When that command reaches a
BeginStoryboard begin on the same retained objects, the
[clock](sqwt-double-animation-clock.md#setup-current-value-and-repeated-begin)
page shows that begin repeats setup, resets elapsed counters and resets the
timer origin. A reused slot would then restart its popup storyboard from
its declared zero-duration initializers. INFERRED. Trigger admission of the
command and the trigger-to-BeginStoryboard path are UNKNOWN.

## Colour classifier inputs

`0x00794BB0(e)` receives the element that `0x00794F30` resolves. That is
the row target for kinds 7 to 10 and the record source otherwise. It
returns 2 for a null element. It returns 0 when e equals `0x004D86B0`,
which returns container `+0x17838` or, when that is null, `+0x17834`.
Otherwise it calls `0x00588B40(e)` for a four-byte record R and returns 1
when byte `R+0` is nonzero or halfword `R+2` differs from `0xFFFF`, else 2.
STATIC.

`0x00588B40` passes element `+0x80` to `0x00574F60`. That holder returns
`(0, 0xFFFF)` when its pointer is null, and otherwise calls the held
object's virtual `+0xB8`. The holder's other entry `0x00574A23..0x00574A40`
dynamically casts its object between `NullActorImpl` (`0x01270AD4`) and
`LuaActorImplInterface` (`0x01270A90`), both in
`Application::Lua::Script::Client`. STATIC.

| Held object | Table | Slot `+0xB8` | R |
|---|---|---|---|
| `NullActorImpl` | `0x00FE02AC` | `0x00761A50` | `(0, 0)` |
| `LuaActorImpl` | `0x00FDFB2C` | `0x00759CF0` | forwards to its `+8` object's virtual `+0x60` with `&impl+4` |

`LuaActorImpl` constructor `0x00759510` stores its script object at `+8`.
The script classes with tables at `ActorBase` `0x00FD4FE4`, `CharaBase`
`0x00FD5CAC`, `PlayerBase` `0x00FD5E04` and `MyPlayer` `0x00FD785C` share
slot `+0x60` `0x0071B1E0`, which writes `(0, 0xFFFF)`. `NpcBase`
`0x00FD647C` overrides it with `0x006E3530`, which writes as byte 0 the `sete` result
of the dword compare `[npc+0x154] == K` and as halfword 2 the value
`[npc+0x158]`. With a the `&impl+4` argument, K is
`[[[[a]+0x220]+4]+0x10]`. `0x00CC7510` dereferences a and jumps to
`0x00CD7910`, which adds `+0x220`, and `0x0057AB60` copies `+0x10` of the
object at `+4` of that cell. STATIC.

The `NpcBase` constructor sets `+0x154` to zero and `+0x158` to `0xFFFF`
at `0x006F3740..0x006F376E`. A `.text` scan for paired stores to those
two offsets finds only `0x0089D030`, slot 1 of `HateStatusReceiver` table
`0x0105733C`, which copies a dword and a halfword from its receiver.
`xivl-opcodes:opcodes.json` row `SetEnmityIndicator` binds that receiver to
clientbound opcode `0x0195`. STATIC.

The three classifications therefore select as follows. INFERRED from the
paths above.

- 0 for the element held at container `+0x17838`, or `+0x17834` when that
  is null.
- 1 for an element whose held implementation is `NullActorImpl`, and for an
  `NpcBase` script object after a hate status write whose halfword is not
  `0xFFFF` or whose dword equals K.
- 2 for a null element, a null holder, a non-NPC script class, and an
  `NpcBase` object still at its constructor values when K is nonzero.

The identity of K and of the two container selections is UNKNOWN. The
nameplate consumer `0x0054B8E0` reads the same record and maps byte 1 to
state 4, halfword `0` or `0xFFFF` to 0, below `0x32` to 1, below `0x50` to
2 and the rest to 3. That mapping is STATIC. It is not used by the plate.

## Canvas measurement

`.?AVCanvas@Controls@Sqwt@@` has primary table `0x0107FFCC`. Compared
slot by slot with `Panel` table `0x01078B74` through the end of both tables,
it overrides only slot 0 and slot `+0xD8`. Its
slot `+0xDC` is FrameworkElement `0x0092A250`. `StackPanel` table
`0x0107F304` overrides both slots, with `0x009B7440` and `0x009B71D0`.
STATIC.

MeasureCore `0x0092B880`, shared by these classes at slot `+0x6C`, calls
slot `+0xDC` at `0x0092BAF0` with the available size. Slot `+0xDC` is
therefore the measure override and `+0xD8` the arrange override. INFERRED
from the call site and the Canvas `+0xD8` body, which positions children.

FrameworkElement `0x0092A250` starts from the pair returned by slot `+0x9C`
(`0x008E92C0`, element `+0x288`). It walks the logical children list at
`+0x254`, calls child Measure `0x0093CD90` with the same available size,
and raises each component to the child's DesiredSize at `+0x128` and
`+0x12C`. It reads no Canvas attached property. STATIC. `+0x254` points to
the embedded `.?AVLogicalChildrenList@FrameworkElement@Sqwt@@` at `+0x258`.
The Panel collection add entries `0x00995620` and `0x00995650` call
`0x009313C0`, which sets the child's logical parent at `+0x240` and appends
the child through `+0x260` at `0x00931478`. STATIC.

Child Measure `0x0093CD90` skips MeasureCore and yields `(0, 0)` when the
Visibility byte at `+0x158` is zero. A Hidden child, byte 1, is still
measured. Each measured component then passes through `0x009D6600` and
`fild` at `0x0093CEC0..0x0093CEEC` before it is compared with and stored to
DesiredSize at `+0x128`. The SSE2 path of `0x009D6600` converts with
`cvttsd2si`, so desired sizes are whole numbers truncated toward zero.
STATIC. MeasureCore then replaces the width with Width at
`+0x1C4` and the height with Height at `+0x1BC` when either differs from
the sentinel -20000000.0. It clamps by MinWidth `+0x1E4`, MaxWidth
`+0x1D4`, MinHeight `+0x1DC` and MaxHeight `+0x1CC`, each scaled by slot
`+0xC4`, and adds Margin. STATIC.

A Canvas therefore does not measure as zero. Without Width or Height it
reports the larger of its `+0x288` pair and its largest visible child's
desired size, plus Margin. INFERRED. The StackPanel measure branch taken
when byte `+0x2FC` is zero gives an unbounded width and sums child desired
widths at `0x009B7292`. That is the horizontal layout. Five Canvas/TextBlock
pairs in it spread by their Canvas widths. INFERRED. A collapsed digit
leaves its Canvas at the `+0x288` pair.

The `+0x288` pair is `(+0x280 - (+0x270 + +0x278), +0x284 - (+0x274 +
+0x27C))` times slot `+0xC8`, recomputed by `0x00929F90`. The constructor
sets `+0x280` and `+0x284` to 16.0 and the four thickness lanes to zero at
`0x0092FA89..0x0092FB0D`. When `+0x290` is non-null, `0x0092A5F0` sets
the `+0x280` pair to zero through `0x00929C40` and then loads data through
`0x00934210`. Slot `+0xC8` defaults to the float `0.8` at `0x012E035C`
when the element has no parent at `+0x240`. STATIC. Reading `0x0092A5F0`
as skin application is INFERRED from that load. The value this pair holds
for each DamagePlate Canvas after style and skin application is UNKNOWN.

Canvas arrange `0x009B94D0` skips a child whose desired width or height is
zero. Otherwise it reads `Canvas.Left`, `Canvas.Right`, `Canvas.Top` and
`Canvas.Bottom` (descriptors `0x0135FE60`, `0x0135FEF0`, `0x0135FEA8`,
`0x0135FF38`). The Right and Bottom forms subtract the child's desired
width or height. STATIC.

## Foreground to the drawing peer

Control holds Foreground at `+0x300` with flags at `+0x304`, and
FontEdgeColor at `+0x308`. Each cell is a pointer to an interned four-float
value. The native wrappers `0x00938020` and `0x00938050` call store
`0x00939200` with `+0x300` and `+0x308`. An accepted store releases the old
value through `0x00916DF0` and interns the new one through `0x00916EB0`. The constructor initializes
Foreground to four lanes of the sentinel -20000000.0 and FontEdgeColor to
four zero lanes at `0x00938DED..0x00938E3C`. STATIC.

Store `0x00939200` takes a source argument. A source of 0, used by the
DamagePlate palette, clears flag bit 0 and is refused only when flag bit 1
is set. A source of 1 is accepted only when bit 0 or bit 1 is already set.
An accepted value that differs from the stored one sets bit 2 and, when
requested, notifies the change callback. STATIC, from
`0x00939220..0x0093924E`. While flag bit 1 is clear, a native write
therefore overrides a value from source 1 and blocks later source-1 writes
until the flags change. While bit 1 is set, the native write is refused and
source 1 is accepted. INFERRED. Which markup or Setter path uses which source is
UNKNOWN.

Change callback `0x00937A70` indexes byte table `0x00937CB8` with the
descriptor index. Foreground, index `0x0B`, selects `0x00937BD5`, which
calls `0x009370B0` unless bit 0 of its argument `+0x78` is set.
FontEdgeColor, index `0x0C`, selects the exit `0x00937C84`. `0x009370B0`
requires `0x0093F060` to report a peer and obtains it through `0x0093F880`.
It returns without a push when flag bit 0 at `+0x304` is set or when
Foreground still equals the sentinel. Otherwise it calls peer slot `+0x9C`
with Foreground and FontEdgeColor. STATIC. A FontEdgeColor write alone
therefore reaches the peer only with a later Foreground push. INFERRED.

DrawingToolContext table `0x00FA28E4` slot `+0x9C` is `0x0055A4E0`. It
copies Foreground lanes 1, 2, 3 and 0 to record `+0xAC`, `+0xB0`, `+0xB4`
and `+0xB8`, the FontEdgeColor lanes likewise to `+0xBC..+0xC8`, ORs 1
into the byte at `+0x58` and stores 1 to the byte at `+0x5C`. Its only data reference is that table cell. A scan
of `0x00540000..0x00560000` finds no other store to record `+0xB8`. STATIC.
The record order puts lane 0 last. Classification 2, `(255,255,255,255)`
with edge `(255,26,26,26)`, therefore reads as ARGB lanes, opaque white
with a dark grey edge. INFERRED.

The style parser `0x0099E5C0` reads Frame attributes `Skin`, `FontColor`
and `FontEdgeColor` (`0x0107B268`, `0x0107B270`, `0x0107B27C`). Those strings
have no other reference. STATIC. How a Frame FontColor reaches the drawing
record, and whether it wins over a pushed Foreground at draw time, is
UNKNOWN. `0x009370B0` pushes only to the element's own peer and reads no
parent. No Foreground inheritance from a root Window to descendant
TextBlocks is found on this path. Inheritance elsewhere is UNKNOWN.

## Storyboard clock and root Window properties

The units of evaluator inputs Q and D are milliseconds, and Q comes from
the RaptureSqwtInterface QueryPerformanceCounter clock. The
[clock](sqwt-double-animation-clock.md#clock-source-units-and-epoch) page
holds the evidence. The epoch is the first admitted timer visitation after
Begin. STATIC there.

Window `Left` and `Top` are descriptors `0x013570A0` and `0x013570C4`, with
float cells at `+0x37C` and `+0x384` and change callback `0x009238A0`.
Indexes 0 and 1 select `0x009238FB`, which calls helpers on the object at
`0x013573E0` and then `0x0093B250`. STATIC. A Top
animation is therefore not inert. How Top composes with the per-plate
Margin, alignment and anchor offset is UNKNOWN.

`RenderTransform` and `RenderTransformOrigin` on the Window are UIElement
members. The [render transform](sqwt-render-transform.md) page shows the
ScaleTransform centre is `RenderTransformOrigin * RenderSize` and that the
matrix reaches the Window's own `VisualTransform`. Whether the draw path
applies that matrix to the whole Window subtree is UNKNOWN there and here.

## Surviving gaps

- The text of `xtx/_text_ui` rows 2001 to 2005, which no tracked artifact
  states.
- Property bit `0x40` at byte `+4` of each role's Text property (`+0x33C`),
  whose source is not traced. It gates markup resolution.
- The TextBlock result when row 2006 fails for an id18 that is not an
  `xtx/status` row.
- How the renderer draws the `xtx/status` field 24 icon control in plate
  text.
- A retail kind 4 or 10 plate with id18 231001 to 231003, and the
  DamagePlate IconControl path to the texture manager.
- K and the two container selections in the classifier.
- The `+0x288` pair of each DamagePlate Canvas after style and skin.
- The source argument used by markup attributes and style Setters.
- How a Frame FontColor reaches the drawing record.
- Command trigger admission and the trigger-to-BeginStoryboard path.
- Window Top placement and RenderTransform composition with the subtree.

## Reproduction

Verify the executable identity and PE32 base above. Disassemble the cited
windows with Python capstone in x86 32-bit mode over file-backed section
bytes and cross-check calls and references with ghidra-cli. Read the jump
table at `0x007967CC`, the float lanes at `0x012C4214` and `0x012C4294`, the
UTF-16 literals and the descriptor names directly from the file. Resolve
each cited table through its Complete Object Locator to the RTTI
TypeDescriptor name. Scan `.text` for raw immediates and call targets to
reproduce the reference counts. Read the status icon pair table at
`0x00FE1610` as little-endian signed dwords, scan the whole file for the
map and table dwords, and check each hashed range against the SHA-256
tables in Identity and method. Decode the install sheet definitions and
resources as Identity and method states. Keep generated listings, decoded
assets and retail bytes outside tracked files.
