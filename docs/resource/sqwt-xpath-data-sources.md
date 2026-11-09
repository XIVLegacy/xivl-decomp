# Sqwt XPath data sources

Sqwt forms feed lists and item templates from XML data through XPath
bindings. A list control binds to a data source declared in the form, and the
templates of its items bind to attributes of one row. This page records how the
1.23b client declares those data sources, how a binding with an XPath resolves
and evaluates, how `ItemsSource` turns the result into items, and how scripts
fill the rows. It ends with the rule a binding layer should follow for
`MainMenuWidget`, `MiniMapWidget` and `AchievementPopupWidget`.

The machine-checked STATIC findings are recorded in
[`sqwt_xpath_binding.json`](../../config/sqwt_xpath_binding.json), checked by
`tools/verify_sqwt_xpath_binding.py` against the pinned bytes. The reference
model of the evaluator, the node select and the row range is in that tool.
The markup shapes are names and attribute shapes only. No markup body is
reproduced.

## Verdict

- A data source is a `ResourceDictionary` entry. `SqwtXmlDataMaker` and
  `RaptureSsdDataReader` are the two element types that declare one, and both
  derive from `Sqwt::ResourceDictionary`.
- A binding with an XPath does not copy data. It writes a handle to the XML
  node, or one attribute string of the current row, into the target property.
- `ItemsSource` receives the handle and builds one item for each child of the
  node. Each item has its row as `DataContext`, and the item templates read row
  attributes with `XPath=@name` bindings.
- The three forms use only two XPath shapes, a slash path on the list and an
  at-sign path on the row. The evaluator also supports bracket predicates and
  `text()`. The attribute predicate occurs only in the config key markup, and
  `position()` and `text()` occur in no markup.
- Scripts fill a `SqwtXmlDataMaker` row by row through the list natives. The
  sheet reader fills its rows itself from a sheet and a column list.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | Capstone 5.0.7 x86-32 disassembly of the file-backed PE sections, `ghidra` CLI listings and decompilations as leads, and `xivl extract-resource --as sqwt` for the markup of an install |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and
SHA-256 were checked before the observations. Decompiler output supplied
leads. Each STATIC fact below was read in the instructions, and most are pinned
by a row of `staticChecks`. A fact the instructions suggest but do not close is
marked INFERRED. A fact that the install markup shows is marked MEASURED. A
fact that no tracked artifact settles is marked UNKNOWN, and a value that only a
choice can settle is marked CALIBRATION. This was static analysis, without
client execution.

## Data source declarations

The markup declares a data source inside a window resource dictionary.

| Element | Attributes | Elements in the install |
|---|---|---:|
| `SqwtXmlDataMaker` | `x:Key`, `XPath`, `Name`, optional `Source` | 297 |
| `RaptureSsdDataReader` | `x:Key`, `XPath`, `Name`, `SheetName`, `XmlFormat`, optional `Row` | 15 |

Of the makers, 274 carry `x:Key`, `XPath` and `Name`, 18 add `Source`, a file
that preloads the document, and 5 carry only `x:Key` and `XPath` (MEASURED).
The `Source` makers belong to the config key forms and the staff roll, not to
the three forms of this page (MEASURED). Declared `XPath` values have the shape `/N` in
271 elements and `/N/N` in 41 (MEASURED).

The base class arrays reached from the object locators of both classes in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) list
`Sqwt::ResourceDictionary` as the first base after the class itself (STATIC,
locators at RVA `0xD79D34` and RVA `0xD59FC0`, arrays pinned by the RTTI rows
of `staticChecks`). The dictionary owns the
properties that every data source shares.

| Index | Property | Offset | Role |
|---|---|---|---|
| 0 | `Document` | `0x3c` | The owned XML document |
| 1 | `x:Key` | `0x4c` | The resource key that `StaticResource` finds |
| 2 | `XPath` | `0x54` | The path of the list root, for example the text of a one-segment path |
| 3 | `XPathNode` | `0x44` | The node that `XPath` selects, read as a handle string |
| 4 | `Source` | `0x5c` | A file path that preloads `Document` |
| 5 | `IsReadOnly` | `0x64` | Read-only flag |

The property initializer stores indices 2 and 3 at `0x00F25FEF` and
`0x00F2600F`. The getter table is at `0x00982D20`, the setter table at
`0x00983D58` and the shared change callback at `0x009835E0`. A change of
`XPath` runs `0x00982DA0`, which calls the node select `0x00982A30` with the
`XPath` text, the document root and no create flag, and stores the result as
`XPathNode` (STATIC).

The node select takes an absolute path and returns nothing for a path that does
not start with `/`. It cuts the first segment at the next `/`, finds the child
of that name in the child vector at node `+0xBC` and recurses on the rest. A
path of `/` alone returns the node. With the create flag it appends a missing
child and continues (STATIC for the walk, create flag included).

### SqwtXmlDataMaker rows

The row lookup `0x00944850` serves the `Index` property of the maker. When the
maker has no row root it calls the node select with the create flag set
(`0x009448AE`) and keeps the root at `+0x204`. It then appends child elements
until the requested index exists (STATIC). The list natives write through that
route. The property table, the `Action` handlers and the `Filtered` view are in
`xivl-client-structs:docs/stub-overload-native-contract.md`, section Lists.
Two further maker methods at `0x00943370` and `0x00943980` call the same
reselect `0x00982DA0` as the XPath change. Their roles are untraced.

### RaptureSsdDataReader rows

The reader adds `SheetName`, `XmlFormat` and `Row` as indices 0 to 2 of its own
property table, with the index stores at `0x00F14B1F`, `0x00F14B3F` and
`0x00F14B5F` (STATIC). The row builder
`0x006986B0` returns without work unless `SheetName` is nonempty and the parsed
`Row` vector names at least one row (STATIC, gate at `0x006986ED..0x00698732`).
The ordinary MiniMap's [map reader](#map-reader-row-request) has a
conditional writer. Its separate SSD reader's producer remains UNKNOWN.

Per row, the builder selects the list root through the node select with the
create flag set (`0x006987C1`), appends one row element and writes one
attribute for each column of `XmlFormat` that has a name. A column with an
empty name is skipped and keeps its position, which is why the marker readers
begin their column list with a comma (INFERRED from the builder loop and the
column lists). The cell text depends on the column type. The row number is
written as decimal text under the attribute `ssdindex` (INFERRED from the
adjacent `%d` format and the `ssdindex` literal). The builder ends with the
node reselect `0x00982DA0`.

`Row` is a list separated by spaces and commas. A token `a` names one row. A
token `a-b` names a range, with the smaller number first. For positive `a`,
`a-0` and `a-` start an open range at `a`. Both `0-0` and `0-` select
row 0. The parser `0x00698EC0` stores a range as `low`, `-1`, `high`
and an open range as `low`, `-2`, and the step
code of the builder reads the high end of a range as included (INFERRED, step
logic at the end of `0x006986B0`). `Row` of `0-33` therefore names 34 rows.

| Row text | Encoding | Rows of a 100-row sheet |
|---|---|---|
| `0-33` | `0, -1, 33` | 0 to 33 |
| `5-3` | `3, -1, 5` | 3, 4, 5 |
| `12-0` | `12, -2` | 12 to the last |
| `0-0`, `0-` | `0` | 0 |
| `1, 4-5` | `1, 4, -1, 5` | 1, 4, 5 |

## Binding resolution

The binding object `B` is the `Sqwt::Data::Binding` object of the
[converter parameter page](sqwt-binding-converter-parameter.md). The binding
markup extension parser `0x00912340` reads the keys `Converter`,
`ConverterParameter`, `BackConverterParameter`, `ElementName`, `Path`, `XPath`,
`Source`, `Name` and `Mode` (STATIC for the key literals that carry a store).
It stores the values in the binding object `B`.

| Field | Name |
|---|---|
| B+0x3c | source |
| B+0x44 | targetObject |
| B+0x4c | converter |
| B+0x54 | xpath |
| B+0x5c | sourceProperty |
| B+0x60 | targetProperty |
| B+0x64 | elementName |
| B+0x68 | converterParameter |
| B+0x6c | backConverterParameter |
| B+0x70 | path |
| B+0x74 | sourceKey |
| B+0x78 | name |

The pre-parser that fills `B+0x3c` before this constructor runs, and the keys
it overwrites it with, are in [Sqwt binding source arms](sqwt-binding-source-arms.md).

The resolver `0x00911F50` runs when the binding attaches to its target. With a
nonempty `XPath` it takes the object named by `Source` as the source, and the
target element itself when `Source` is empty. It then stores the source
property at `B+0x5C`. The property is `XPathNode` when the first character of
the `XPath` is `/`, and `DataContext` for any other first character (STATIC).
The two property info objects are `0x01356C24` and `0x01356C10`.

On the `DataContext` path the resolver tests bit `0x2` of the byte at `+0x20` of
the source. When it is set, the source is replaced by the result of its virtual
slot 3 called with the arguments `0` and `1`, stored through `0x00998C20`
(STATIC, `0x0091205E` to `0x00912071`). The meaning of the flag, likely a
redirection of a templated element to its parent, is UNKNOWN.

A `Source` binding with a slash path therefore reads the data source named by
its `x:Key`. A binding without `Source` and with an at-sign path reads the
`DataContext` property of its own target element while the flag above is
clear. See [MiniMap marker context inheritance](#marker-context-inheritance)
for the selected template path from the container to that property.

## Evaluation

The evaluator `0x009993C0` runs on every update of the binding. It first reads
the source property into a working string `W`. The handle string is the lookup
key of an object in a thread-local table (`0x00916860`, table at thread block
`+0x1C` plus `0x2118`), and `0x00918FD0` casts the result to `Sqwt::Object`.
The row and list handles are these objects. The evaluator then resets the child
range of the node to `0` to `0xFFFF` through virtual slot 5 of
`Sqwt::Xml::XmlElemenBase` (`0x0099AE30`), and branches on the first character
of the `XPath`.

| Shape | Result | Label |
|---|---|---|
| `/N` | node handle of the source when the XPath starts with the XPath of the source | STATIC |
| `/N/N` | same test and result as the one-segment form because the whole text is compared | STATIC |
| `/N/N[@N=N]` | handle of the first child whose attribute equals the value, else the handle of the source | INFERRED |
| `/N[position() ge N and position() le N]` | handle of the source with a child index range stored on it | INFERRED |
| `@N` | attribute of the current row, empty text when absent | STATIC |
| `text()` | text of the current row element | INFERRED |

- The slash branch compares the source `XPath` property with the start of the
  binding `XPath` using `strncmp` with the length of the source path. A
  mismatch returns without writing the target (STATIC, `0x009995E4`). A match
  without a `[` leaves `W` unchanged, so the target receives the same handle
  that the source property gave.
- The attribute branch calls the element method at slot 7 of the element
  vtable at RVA `0xC79EF0` (`0x0099ADB0`). It returns the stored value or the
  address of an empty string, and the evaluator copies it into `W` (STATIC).
- The `text()` branch copies the string at object `+0x70` (INFERRED as the
  element text).
- Every branch except the mismatch ends with one write of `W` into the target
  property `B+0x60` of `B+0x44` as a string (STATIC, `0x009999F9`). The target
  parses the string with its own setter, as a markup attribute would be parsed
  (INFERRED).
- A binding whose source property is missing (`B+0x5C` is zero) ignores the
  `XPath`. It writes the string form of the source object itself (STATIC,
  `0x00999A23`).

The attribute predicate reads the name between `@` and `=` and the value
between `=` and `]`. It asks the child list for the first child with that
attribute pair and writes the handle of the child (INFERRED, the list lookup at
`0x00DC2F00` is not decoded). The position predicate splits the text after `[`
on `[`, `]` and space. It starts with the zero-based bounds `0` and
`count - 1`. After `position() ge N` or `N le position()` it sets both bounds
to the larger of themselves and `N`, and after `position() le N` or
`N ge position()` it sets both bounds to the smaller of themselves and `N`, so
`position() ge 10 and position() le 4` gives 4 to 4. A number that follows
`position()` replaces the pending number, which `position() 7 ge 9 ge
position()` shows by giving 9 to 9. The word `and` clears the operator, the
pending number flag and the position-seen flag (`0x0099996F` to `0x00999976`).
It clamps both bounds to the child range and stores them at node `+0x08` and `+0x0C` through
slot 5 (STATIC for the token tests and arithmetic, INFERRED for the reading as a
range). The reader of the stored range is not traced (UNKNOWN).

| Predicate text | Children | Stored range |
|---|---:|---|
| `position() ge 3 and position() le 8` | 20 | 3 to 8 |
| `position() le 4` | 20 | 0 to 4 |
| `position() ge 25` | 20 | 19 to 19 |
| `3 le position() and 8 ge position()` | 20 | 3 to 8 |
| `position() ge 10 and position() le 4` | 20 | 4 to 4 |
| `position() 7 ge 9 ge position()` | 20 | 9 to 9 |
| `position() and ge 3` | 10 | 0 to 9 |

## Items controls

`ItemsControl` has twelve properties, and `ComboBox` and `ListBox` inherit them.

| Index | Property |
|---|---|
| 0 | `SortKeyAttr` |
| 1 | `ItemsPanel` |
| 2 | `ItemContainerGenerator` |
| 3 | `ItemCount` |
| 4 | `MaxVisibleItems` |
| 5 | `MinVisibleItems` |
| 6 | `ItemsSource` |
| 7 | `ItemTemplate` |
| 8 | `ItemTemplateSelector` |
| 9 | `SqwtFocusedIndex` |
| 10 | `SourceFirstIndex` |
| 11 | `SourceCount` |

The string setter `0x00965220` handles `ItemsSource` as index 6 (jump table
`0x009654A0`). It resolves the handle string to an object, stores it at control
`+0x354`, copies the object payload pointer to `+0x46C` and queues a refresh
(STATIC, `0x0096536D`). A handle that resolves to nothing clears the items
(`0x00961100`). The `ItemTemplate` string is at `+0x344` and the
`ItemTemplateSelector` string at `+0x34C`.

The refresh `0x00964580` walks the children of the node from `SourceFirstIndex`
over `SourceCount` rows, clamped to the child count. The three forms set
neither property (MEASURED). For each row, `0x009620C0` does the following.

1. A row whose `visibility` attribute is exactly `False` gets zero height and
   no item content (STATIC, `0x00962162`).
2. Any other row gets a container in the items panel. The container's
   `DataContext` is cleared and then set to the row object (STATIC, the
   property setter `0x0092F310` with info object `0x0135885C`).
3. `0x00960570` applies the template. When `ItemTemplateSelector` is nonempty,
   `0x009601D0` takes the `template` attribute of the row as the template
   name, falls back to `ItemTemplate` when the attribute is empty, and sets
   the container `ContentTemplate` (STATIC). When the selector is empty, the
   `ItemTemplate` name applies to every row.
4. The same routine copies a nonempty `visibility` value of the row to the
   container visibility property (INFERRED).

The child list that the control walks is the element's own list, or a second
list at element `+0xE0` when that pointer is set (STATIC, `0x0099AE00`). The
second list fits the filtered view of the maker (INFERRED).

## Script feed

The script natives `_setListProperty`, `_updateList`, `_addList`,
`_removeList`, `_clearAllList` and `_getListProperty` write the properties of a
named `SqwtXmlDataMaker`. Their argument and property contract is in
`xivl-client-structs:docs/stub-overload-native-contract.md`, section Lists, and
in `manifests/stub_overload_native_call_contract.json`. The list name that a
script passes is the `Name` attribute of the maker and not its `x:Key`. The
`AetheryteListWidget` form declares the makers `Name="ListBox"` with
`XPath="/ListBox"` and `Name="RegionList"` with `XPath="/RegionList"`, and the
native record names those two lists (MEASURED for the form, INFERRED for the
join). The `XPath` text without the slash differs from the `Name` in 263 of the
292 makers with a `Name`, so the `XPath` text is not the join key (MEASURED).
The `Name` join rests on the `AetheryteListWidget` match (INFERRED).

The first write to a list creates its root from `XPath` and appends the rows
(STATIC). After that, `XPathNode` and the bindings see the same node. What
redraws a bound list after later writes is not traced (UNKNOWN).

## The three forms

The form facts come from `widget/<Name>.form`, the file the opener loads per
[`sqwt-form-locale-resolution.md`](sqwt-form-locale-resolution.md), and the
`.sd.form` variants have no loader. The `.sd.form` variant of MiniMapWidget
differs in `type_data` (`Row` is `1-`), and the `widget_c` and `.sd.form`
variants of MainMenuWidget carry no `ItemTemplateSelector` on `ListBox_MainMenu`.

### MainMenuWidget

| x:Key | XPath | Name |
|---|---|---|
| `XmlData_System` | `/System` | `SystemMenu` |
| `XmlData_Main` | `/Main` | `MainMenu` |

`ListBox_SystemMenu` binds `ItemsSource` to `XmlData_System` with `/System`
and `ItemTemplate` `DataTemplate_ListBoxItem_System`. `ListBox_MainMenu` binds
to `XmlData_Main` with `/Main`, `ItemTemplate` `DataTemplate_ListBoxItem_Main`
and an `ItemTemplateSelector`. The selector value is a `StaticResource`
reference to a key that no markup file declares and no executable string
holds (UNKNOWN whether it resolves to a nonempty selector).

Row attributes read by the item templates:

| Template | Attributes |
|---|---|
| `DataTemplate_ListBoxItem_System` | `Help`, `SystemEnable`, `SystemIcon`, `SystemName` |
| `DataTemplate_ListBoxItem_Main` | `Help`, `MainEnable`, `MainName` |
| `DataTemplate_ListBoxItem_Return` | `FValue0`, `FValue1`, `Help`, `IValue0`, `MainEnable`, `MainName`, `TimerVisibility` |

### AchievementPopupWidget

| x:Key | XPath | Name |
|---|---|---|
| `XmlData_Achievement` | `/Achievement` | `Achievement_Maker` |

`ListBox_AchievementList` binds `ItemsSource` to `XmlData_Achievement` with
`/Achievement` and `ItemTemplate` `DataTemplate_ListBoxItem_AchievementPopup`.
That template reads the row attributes `help`, `icon`, `name`, `point`,
`point1`, `point10`, `point10v`, `reward`, `rewardvisible` and `style`. The
popup form shares `AchievementPanel.tpl` with the achievement list form, whose
list template reads more attributes.

### MiniMapWidget

| x:Key | XPath | Name |
|---|---|---|
| `type_data` | `/root/type` | `TypeData` |
| `gl_marker_data` | `/root/marker` | `GLMakerData` |
| `ssd_marker_data` | `/root/marker` | `ssd_marker_data` |
| `map_marker_data` | `/root/marker` | `map_marker_data` |

`type_data` is a sheet reader over `2Dmap_actor_data` with `XmlFormat`
`Type,SparkleFile,SparkleSequence` and `Row` `0-33`. `ssd_marker_data` and
`map_marker_data` are sheet readers over `2Dmap_marker` that declare no `Row`.
Their `XmlFormat` lists name the columns `X`, `Y` or `Z`, `SparkleFile`,
`SparkleSequence`, `Region`, `Layout`, `Floor` and `Template`, and the second
adds `HideLayout`. `gl_marker_data` is a `SqwtXmlDataMaker`.

The window binds no `ItemsSource`. The `CustomControl` of type
`MapScreenControl` binds `TypeDataSource`, `GlMarkerSource`, `SsdMarkerSource`
and `MapMarkerSource` to the four lists with `Mode=OneWay` and the same slash
paths, so each property receives a list handle. The marker templates in
`common/MapMarker.tpl` also read the row attributes `SparkleSequence`,
`Text` and `Visibility`.

#### Marker wrapper and row context

Replay the installed carriers under `client/sqwt` with
`xivl extract-resource --as sqwt` and `verify-extraction --source`.
Python Expat identifies the element lines. LLVM objdump 21.0.0
confirms the selected native instructions in the pinned PE.

| Carrier | Bytes | SHA-256 | Decoded SHA-256 |
| --- | ---: | --- | --- |
| `widget/MiniMapWidget.form` | 2943 | `83428fa59fa9d26951c1c646d40b87fbbfd9ea0b537ae30e3af4b1ff36560360` | `7c3d9d8a0c69291d21b705c8823444fc5ab89d4ee3ae06eda80caab040e85c88` |
| `common/MapMarker.tpl` | 15166 | `c9a9ddb8568e07bf67564e96328722c647a086ded15907900f73092c6706dba5` | `68a685d900291186ba0b6472e8a6f9054459d2b2d874913725ee0fb8fe1e5555` |

The form merges `common/MapMarker.tpl` at line 11 and declares
`MapMarkerTemplate` as both `GlMarkerDefaultTemplate` and
`SsdMarkerDefaultTemplate` on the map control at line 19. This key
names the outer `ControlTemplate` at template line 2. Its inner
`ContentControl` at line 5 binds `Template` with `XPath=@Template`
and `Mode=OneWay`. The binding omits `Source` and supplies no literal
inner template key (MEASURED). With a resolved row, the generic
[binding resolution](#binding-resolution) and [evaluation](#evaluation)
paths request an attribute string write. A row without `Template`
supplies empty text.

The selected native SSD and map source setters at `0x00673D42` and
`0x00673D7E` pass the holders at map control `+0x498` and `+0x4A0`
to the source store. The `SsdMarkerDefaultTemplate` descriptor
`0x013400C0` (tag `0x12`) uses the holder at `+0x4C8` through
`0x00673E80..0x00673E86`. With the corresponding source and node-list
pointer present, the SSD path at `0x00678E72..0x00678EAB` and map path
at `0x00679022..0x00679059` pass the list result to `0x006805F0`.
The map path also requires `+0xA68` mask `0x80` clear and `+0x600`
nonzero at `0x00679008..0x0067901C`. A nonnull `+0x4C8` supplies
the same wrapper name to both calls. A null holder supplies the empty
object at `0x01266B10` (STATIC).

Both paths visit their retained marker entries through `0x0067DBD0`
at `0x00678F2E` and `0x006790D1`. The per-entry call at `0x0067DC1C`
enters `0x0067BEC0`. For an entry with mask `0x02` set at `+0x3D`
and nonempty wrapper name, `0x0067C038..0x0067C03F` requests a
`Template` write on the retained element. The name key at
`0x0133FDD4` is initialized from `Template` at `0x00F119A0`. After the
write request returns, the routine casts the row from `DOM_Node` to `Sqwt::Xml::XmlElement`
at `0x0067C04A..0x0067C057`. At `0x0067C062` it passes that result
to the outer element's `DataContext` setter `0x0092F310`, which
uses descriptor `0x0135885C` and holder `+0x208` (STATIC).
See [Control Template application](sqwt-template-initial-enabled-receiver.md)
for the generic named resource route.

#### Marker context inheritance

The outer template request precedes the outer row assignment. With the
selected wrapper instantiated and its root retained, the ordinary
`ContentControl` paths below can carry `DataContext` to the inner control
(STATIC).

| Path | Locators and conditions |
| --- | --- |
| Context already on the owner | For a nonnull root with no logical parent, `0x00936EB9..0x00936ED7` calls the parent attachment path. The inner control's parent getter cell `0x0106B640` returns `+0x240` through `0x0092B5D0`. The [inherited refresh](sqwt-progress-template-style.md#conditional-style-inheritance) at `0x00931489 -> 0x00912C10` can copy the registered property from the owner. `DataContext` metadata `0x30` at `0x00F22039` admits the inherited read and write and qualifies the descriptor for inherited-name collection. |
| Context assigned after materialization | Changed-property handling at `0x00912B6E..0x00912BC8` checks notification mask `0x10`, reads the value and sends mode-1 writes to dependency children. The `ContentControl` child getter/count at `0x00937140`/`0x00937170` expose its retained template `+0x318`. The `ControlTemplate` getter/count cells `0x00FCB0CC`/`0x00FCB0D0` expose the nonnull root `+0xB0`. The named template constructor `0x00936C00 -> 0x0099B3A0` retains the base descriptor list at instance `+0x10` from `0x00912E65 -> 0x0098B400`. The array at `0x012DFC44` contains one descriptor, `0x01356BC8`. On the resulting `DataContext` descriptor miss, mode 1 recurses at `0x00912A5B..0x00912A97`. |

The inner `DataContext` getter cell `0x0092BFE8` selects `0x0092BE70` and
reads holder `+0x208` at `0x0092BE75..0x0092BE7B`. The setter branch
`0x00930876..0x00930897` writes through `0x00932260` and `0x009C3820`.
The holder initializer at `0x0092F8B0..0x0092F8C7` supplies descriptor
`0x0135885C` to `0x00965600`, which derives its initial low bits from
descriptor metadata. Mode 1 refuses a write with both low bits clear
at `0x009C383F..0x009C385F`. An admitted different pointer is stored at
`0x009C38A2` and can notify listeners (STATIC).

For the source-less binding on the ordinary inner control, the clear-bit
path at `0x0091205E..0x0091208D` resolves `DataContext` on the control
itself. The base constructor clears source redirection bit `0x02` at
`0x00912E1C`. The `OneWay` string at `0x01068424` sets mode 1 at
`0x0091273C..0x00912755`. On admitted activation, the inner control cell
`0x0106B65C` leads through `0x00937CD0`, `0x0092A060` and `0x0093CC20`
to the [binding activation walk](sqwt-style-reverse-binding.md#observed-attachment-activation-and-retention)
at `0x00912190`. Binding activation can seed from the row already in
the holder through forward slot `+0x10` at `0x0099939A..0x009993A2`.
A subsequent notification for the matching source and descriptor can
use the same forward slot at `0x00998A8A..0x00998A9B` (STATIC).

#### Inner template key result

The forward evaluator sends the attribute text to the inner `Template`
setter as a mode-0 write at `0x009999F9..0x00999A07`. An absent `Template`
attribute supplies empty text. A successful read of a null context also
formats empty text through `0x00931E60 -> 0x00915D60 -> 0x009182B0`.
The evaluator's object-lookup miss at `0x009994A1..0x009994A3` sends that
working text to the target (STATIC).

- With successful allocations the ordinary `ContentControl` constructor
  installs a default template with a `ContentPresenter` root at
  `0x009362A2..0x00936306`. The default name starts empty at
  `0x0099B208..0x0099B20E`. The parser's same-name check at
  `0x00938644..0x00938697` bypasses application and assignment when the
  current template exists and its name equals the incoming text by exact
  case-sensitive bytes. An empty write can retain this default template.
  An equal nonempty key can retain a rootless template without retrying
  application (STATIC).
- A different name, or an absent current template, takes the allocation
  and resource application path. Empty text can enter this path with a
  nonempty owner class name at `0x0099AFF9..0x0099B04A` and
  `0x0096A610..0x0096A67D`. A resource lookup miss at
  `0x0094A2BD..0x0094A2C4` returns false at `0x0094A30B` without
  materializing a root in that attempt. Application stores the result
  at template `+0xCC` at `0x0099B053`. The parser still calls the
  assignment helper `0x00938140` at `0x009386DE` after application returns.
  An admitted write can retain the unresolved template at `+0x318` through
  the store at `0x00926FA0`. Visual attachment uses the separate `+0x334`
  state through `0x00936DF0`, which can retry application when the retained
  template root is null (STATIC).
- The fresh template retains the requesting inner control at template
  `+0xC8` through `0x00936C00 -> 0x0099B3A0`. The parser materializes it
  before assignment at `0x009386CF`. A resource match at `0x0094A2F6`
  returns success after the markup request. Root presence at template
  `+0xB0` has its own check. A nonnull root with no logical parent can
  attach to the same control at `0x00936ECF/0x00936ED7` (STATIC).
- The admitted change callback at `0x00937C3A` requests detach when the
  previous property template equals the applied template and has a root.
  Visual removal at `0x00940800` requires the root's visual parent
  `+0xA4` to match the control. Logical removal at `0x0092FBE0`
  separately checks the control's child vector before its parent-holder
  write. The callback can retry the current rootless template at
  `0x00936E11`. A persistent nonempty miss leaves `+0x334` unchanged
  after the detach requests, with no new root attached. This can leave
  the old root detached even while its template pointer remains applied.
  A later empty key has a separate path that can update `+0x334` without
  a root at `0x00936EA7` (STATIC).

The selection and attachment locators were checked with LLVM objdump and
GNU objdump 2.46.1 x86-32 decoding, with direct PE reads for the ordinal
jump cells and RTTI identities.

The SSD and map readers at form lines 8 and 9 omit `Row`.
The [reader gate](#rapturessddatareader-rows) leaves initial row
selection to a separate producer. The first live row and inner key,
binding activation order, GL marker routing and painting remain UNKNOWN.

#### Retained marker placement

STATIC. The map source at control+`0x4A0` reconciles into its
`+0x368` container, then visits the range at `+0x370/+0x374`
(`0x00679022..0x006790D1`). The SSD source at `+0x498` uses its
`+0x2F8` container and `+0x300/+0x304` range
(`0x00678E72..0x00678F2E`). The GL path retains a separate container
at `+0x330`.

After the DataContext request, `0x0067C067..0x0067C0A0` stores the
entry coordinates as X divided by resolution plus the X offset and
Z divided by resolution plus the Y offset. Helper `0x00670990` reads
the row's X and Z attributes and returns the supplied zero for empty
text. The map path passes control fields `+0x614/+0x638/+0x63C`
at `0x0067906D..0x006790CE`. The SSD path passes 1, 0 and 0
at `0x00678EB0..0x00678F2B`.

The arrange pass visits those entries at `0x00675CF8..0x00675D34`
for map and `0x00675CE0..0x00675CF1` for SSD, through the
per-entry call `0x0067DE5C -> 0x0067D030`. It requires entry
`+0x3D` masks `0x02` and `0x04`. It subtracts the retained center
and scales by the product of control `+0x610/+0x60C/+0x614`
rounded to f32. For finite scaled deltas and half-extents, it checks
inclusive half-viewport bounds at `0x0067D053..0x0067D15C`. With the optional named-element
check passed or skipped, it requests Visible and the local Margin
at `0x0067D3A2` and `0x0067D3FC`. The refusal arm requests Hidden
at `0x0067D463`.

The separate [area-image pass](minimap-base-tiles.md#the-declared-area-image)
can consume the same SSD and map entries. It adds the radius bit and
positive radius gate at `0x0067B9B5..0x0067B9DC`. The placement gate
above requires only the other two bits. Live row selection, viewport,
property admission, child drawing and pixels remain unknown.

#### Map reader Row request

STATIC. The [piece request](minimap-base-tiles.md#native-piece-producer)
at `0x00679D78..0x00679D98` starts a new SSDReader only when the piece
key differs from both `-1` and control `+0x9EC`. The navigation key
may already have changed at `0x00679D20`. A repeated piece key
skips starting a new reader even when that navigation key changes.

The new reader receives the scope from the control's primary `+0xB0`
at `0x00678089`. Cell `0x00FC363C` selects `0x0092B1F0`, which
follows the logical parent at `+0x240` and calls its `+0xB0`.
The checked Window cell `0x0106A16C` returns the Window itself.
The reader retains the supplied scope at `+8`.

STATIC. State 0 of callback `0x006765E0` reads signed column 7 from
the selected `mapNavi_data` row at `0x00676774..0x00676787`.
With a nonnull scope it looks up `map_marker_data` at
`0x006767B8`. The cast at `0x006767E2` must resolve a
`RaptureSsdDataReader`. This route selects the named reader,
while the control's list property retains a separate handle.
Column 7 equal to `-1` supplies empty text. Otherwise the writer
uses format `%d-` at `0x00FC255C`. The typed Row call
`0x0067684C -> 0x00699960` requests mode 0 and notification 1.
State 1 proceeds to the piece lookup even if that cast fails.
The callback does not check the Row setter's return.

The setter uses Row holder `+0xD4` and callback `0x006996C0`.
An admitted changed string reparses the range at `0x006996F3`.
A positive start followed by a trailing hyphen uses the
[open range](#rapturessddatareader-rows). Text `0-` instead
selects row 0 at `0x00698FC9..0x00699049`. Empty text
leaves an empty vector. The separate reader and render gates
still require availability, current source handles and materialization.

#### Conditional Gridania map row

MEASURED. The installed region resource `0x03C00000` names layout
keys 321 and 331 for child resources `0x29B00001` and `0x29B00002`.
With either numeric manager admitted and page 0 supplied, the selected
`_layout` column 2 yields navigation key 2700 or 2800, respectively.
Both available `mapNavi_data` rows select piece 1211 in column 6 and
marker start 3701 in column 7. The three decoded sheet identities match
`xivl-client-data:manifests/tables.json`.

STATIC. A new piece reader is required, so a retained piece key of
1211 bypasses the request. With the state-0 scope, lookup, cast and
write admitted, the writer supplies `3701-`. The range starts at
3701 and the installed row exists. Its selected template key is
`MapMarkerSymbol`, declared at line 47 of `common/MapMarker.tpl`.
The row supplies `common/mapMarkerSymbol.le.spk` and sequence
`ms00007`. The row's `HideLayout=-1` cannot reject through the
comparison at `0x00671F47..0x00671F65`, which skips the equality
test when the retained control field is also -1. Live manager and
page selection, row admission, and marker painting remain unknown.

#### Separate navigation SSD requests

MEASURED. The two navigation forms select `quest_marker` for
their `ssd_marker_data` readers and `Function=0` for their map
controls. The [ordinary MiniMap declaration](#minimapwidget) selects
`2Dmap_marker` and `Function=1`. Decode these two carriers with the
same SQWT extraction and source verification above.

| Carrier | Bytes | SHA-256 | Decoded SHA-256 |
| --- | ---: | --- | --- |
| `widget/MapNavigationWidget.form` | 22364 | `137c5542a8dd41fcafe85b66d8954e238b8e13d884fd6c0d89d3316c6440ef0e` | `2012be5906707b43eadadcde937045257eed7735e9afec7a43b6b2bab5a4fa35` |
| `widget_c/MapNavigationWidget.form` | 13169 | `4188d7e07416b8bf2da1ce9549c0af4ab0a3601728e8dc300031138799bda43f` | `7a7d3dda65c51759f7d5aba2a4adb0f25d0ad6a83a962c82eb6c2bdadf9d8b12` |

STATIC. Navigation initialization at `0x00529393` caches its
named reader at widget `+0x250`. With widget `+0x254=3`,
`0x004FCA40` calls `0x004F1850` to request a decimal Row on
that reader through `0x00912850` at `0x004F194D`.
The next call at `0x004FCA56` requests QuestMapIndex on the
different control at widget `+0x24C`.

The navigation LPB `client/script/n1635q/x9uw9o139q1vwn1635q.le.lpb`
is 15740 bytes with SHA-256
`873ee821c979b9c0959743bc619444ec01bd39c96f71235235278bb602b4d0bb`.
Its decoded SHA-256 is
`21cc7f2f9c6e2a56e02620e18748fa9dd70bf02dab6a889fd6083826d60528c2`.
Using the [LPB replay](minimap-base-tiles.md#ordinary-minimap-ready-requests),
`root/proto43` at decoded code offset `0x3D03` calls `_setProperty`
at zero-based PC 5 with self, nil scope, `ssd_marker_data`, `Row`
and the first explicit argument. The ordinary MiniMap LPB and its
checked base and connector chunks have
none of the three literal keys `ssd_marker_data`, `map_marker_data`
or `Row`. A file-backed .text E8/E9 scan identifies `0x0067684C`
as the only direct branch to `0x00699960`. Generic Row writes can
reach `0x00699818` separately.

The reader constructor `0x006999C0` starts with a null Row holder
and empty parsed key vector. The exact native `ssd_marker_data`
reference through cell `0x0126ADB8` occurs in the navigation
initializer at operand `0x0052932B`. Its scoped result at
`0x00529393` belongs to that navigation widget's `quest_marker`
reader. Map control registration at `0x00672693..0x006726A3` also
requires `Function=0`. Function 1 skips that registration and has
a separate SSD list consumer at `0x00678E72..0x00678EAB` when the
source and list are present. Exact string and file-backed operand
scans, LLVM x86-32 decoding and the pinned carrier/chunk replays
bound these exclusions. Indirect writes can take the generic Row
path. A producer for the ordinary SSD reader and actual live reader
identities remain UNKNOWN.

## Census

The install markup holds 1155 SQEX files below `client/sqwt`. Of 1121 bindings
with an `XPath`, all have `Mode=OneWay` and 479 name a `Source` (MEASURED). The
`ItemsSource` bindings number 150, in 133 `ListBox` and 17 `ComboBox`
elements. The shapes of all binding `XPath` values are below.

| Shape | Bindings |
|---|---:|
| `/N` | 135 |
| `/N/N` | 38 |
| `/N/N[@N=N]` | 306 |
| `@N` | 642 |

The 306 predicate bindings belong to the config key markup and none belongs to
the three forms. No form uses a `position()` predicate, `text()`,
`SourceFirstIndex` or `SourceCount`.

## Rule for the binding layer

- Model a data source as a named list of rows. A row is an ordered map of attribute names to strings and the list keeps the XPath it was declared with.
- Declare one list for each SqwtXmlDataMaker or RaptureSsdDataReader element, found by x:Key from bindings and by Name from the script list natives.
- Evaluate a binding with an XPath from the resolved source and the current row. A slash XPath reads the list through the source and an at-sign XPath reads one attribute of the current row.
- Leave the target unchanged when a slash XPath does not start with the XPath of its source.
- Hand the list to the items control for an ItemsSource binding, and build one item for each row whose visibility attribute is not False.
- Give each item its row as DataContext and choose its template from the row template attribute when the ItemTemplateSelector of the control is nonempty, else from ItemTemplate. For ListBox_MainMenu, treating the markup selector as nonempty is a CALIBRATION choice until the SqwtDefaultTemplateSelector reference is settled (UNKNOWN, see Open).
- Evaluate the at-sign bindings of an item against its row and write the attribute text through the same string parsing as a markup attribute. A missing attribute reads as the empty string.
- Feed the MainMenuWidget and AchievementPopupWidget lists from the script list natives by Name. Their bindings use only the slash and at-sign shapes, so the bracket predicates and text() need no support for these forms.
- Build the Type, SsdMarker and MapMarker lists of MiniMapWidget from their sheets by XmlFormat and Row, feed the GlMarker list from whatever feeds GLMakerData (UNKNOWN, see Open, so the binding layer's choice is CALIBRATION), and pass the four Source properties of the map control as list handles.

The client facts behind the rule, restated as contract lines:

- A data source is a ResourceDictionary entry declared by a SqwtXmlDataMaker or RaptureSsdDataReader element and keyed by its x:Key.
- A binding with an XPath reads the XPathNode property of its source when the XPath starts with a slash and the DataContext property of its source otherwise.
- A slash XPath that does not start with the XPath of its source writes nothing to the target.
- A slash XPath without a bracket writes the node handle of the source to the target.
- An at-sign XPath writes the named attribute of the current row to the target, or the empty string when the row has no such attribute.
- ItemsSource takes the node handle and builds one item for each child of that node whose visibility attribute is not False.
- Each item gets its row as DataContext and its template from the row template attribute when ItemTemplateSelector is nonempty, else from ItemTemplate.
- A RaptureSsdDataReader builds rows only when its Row value names at least one row.

## Open

- UNKNOWN: The list names that the MainMenuWidget and AchievementPopupWidget scripts pass to the list natives.
- UNKNOWN: What redraws a bound list or item after the first resolution when a row attribute or the row count changes.
- UNKNOWN: The remaining MapScreenControl list consumers and the ordinary SSD reader's Row writer. The [map reader request](#map-reader-row-request) and [marker wrapper](#marker-wrapper-and-row-context) establish separate steps of the map path.
- UNKNOWN: Whether the SqwtDefaultTemplateSelector reference of ListBox_MainMenu resolves to a nonempty selector, because no markup file or executable string declares that key.
- UNKNOWN: Ordinary initial contents of the MiniMap `GLMakerData` list. The [coordinate marker path](../actor/map-marker-lanes.md#coordinate-circles) supplies a separate script writer.
- UNKNOWN: The reader of the child index range that a position predicate stores on a node.
- CALIBRATION: The frame on which a list write reaches the bound items. A binding layer chooses the next frame until a retail capture settles it.

## Reproduction

Verify the executable identity, then run
`python tools/verify_sqwt_xpath_binding.py --exe <ffxivgame.exe>` for the
byte rows, the string rows, the jump tables and the property index stores.
Add `--install <game root> --xivl <xivl executable>` to decode `client/sqwt`
and recompute the markup census and the form facts. The model, the node select,
the range arithmetic and the row range are covered by
`tools/test_sqwt_xpath_binding.py`.

The surviving gaps are the redraw signal after the first resolution, the
`MapScreenControl` list consumers outside the selected route, the ordinary
SSD reader's Row writer, the script list names of two forms, the selector key, initial
`GLMakerData` contents, the reader of the child range, the decoded
child lookup by attribute and the per-property string setters that parse the
written text.
