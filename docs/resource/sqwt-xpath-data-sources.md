# Sqwt XPath data sources

Sqwt forms feed lists and item templates from XML data through XPath
bindings. A list control binds to a data source declared in the form, and the
templates of its items bind to attributes of one row. This page records how the
1.23b client declares those data sources, how a binding with an XPath resolves
and evaluates, how `ItemsSource` turns the result into items, and how scripts
fill the rows. It ends with the rule a binding layer should follow for
`MainMenuWidget`, `MiniMapWidget` and `AchievementPopupWidget`.

The machine-checked form of every STATIC fact is
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
A reader without `Row` therefore builds nothing until something sets `Row`
(UNKNOWN: the writer).

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
token `a-b` names a range, with the smaller number first. A token `a-0` names
every row from `a` to the end of the sheet. The parser `0x00698EC0` stores a
range as `low`, `-1`, `high` and an open range as `low`, `-2`, and the step
code of the builder reads the high end of a range as included (INFERRED, step
logic at the end of `0x006986B0`). `Row` of `0-33` therefore names 34 rows.

| Row text | Encoding | Rows of a 100-row sheet |
|---|---|---|
| `0-33` | `0, -1, 33` | 0 to 33 |
| `5-3` | `3, -1, 5` | 3, 4, 5 |
| `12-0` | `12, -2` | 12 to the last |
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
`DataContext` property of its own target element, which the template content
inherits from the item container that holds the row (INFERRED for the
inheritance). That holds only while the flag above is clear on the target.

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
`common/MapMarker.tpl` read the row attributes `SparkleSequence`, `Text` and
`Visibility`, and `MapMarkerTemplate` reads `Template`. The
consumer of the four handles inside `MapScreenControl` is not traced (UNKNOWN).

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
- UNKNOWN: The consumer of the four list handles that MapScreenControl receives and the writer of Row for the sheet readers that declare none.
- UNKNOWN: Whether the SqwtDefaultTemplateSelector reference of ListBox_MainMenu resolves to a nonempty selector, because no markup file or executable string declares that key.
- UNKNOWN: What feeds the `GLMakerData` list of MiniMapWidget, because `gl_marker_data` is only a `SqwtXmlDataMaker` and no traced script or native names it.
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
`MapScreenControl` list consumers, the writer of `Row`, the script list names
of two forms, the selector key, the `GLMakerData` feeder, the reader of the child range, the decoded
child lookup by attribute and the per-property string setters that parse the
written text.
