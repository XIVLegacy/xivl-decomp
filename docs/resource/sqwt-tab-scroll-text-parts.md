# Sqwt TabControl, ScrollViewer and TextBox parts

The chat log window `LogWidget` is built from five Sqwt controls that draw
through skin parts: TabControl, TabItem, ScrollViewer, TextBox and LogControl.
This page records the part keys and template parts each control owns, how the
install's style files give those parts skins, the measure and arrange rules,
the property names the executable registers, and the elements and attributes
the installed forms use. It ends with what a painter for installed forms needs
before it can paint `LogWidget`.

The machine-checked form of every STATIC fact is
[`sqwt_tab_scroll_text.json`](../../config/sqwt_tab_scroll_text.json), checked
by `tools/verify_sqwt_tab_scroll_text.py` against the pinned bytes. The
reference models of the layout rules are in that tool. The install census tool
is `tools/census_sqwt_tab_scroll_text.py`. The form loading path is on the
[form and template path resolution](sqwt-form-locale-resolution.md) page, the
`LogWidget` resources are summarised in the
[chat log window](../script/chat-log-window-and-nameplate.md) page, and the
`Margin` attribute shapes are on the
[TextBlock Margin inputs](sqwt-textblock-margin-inputs.md) page.

## Verdict

- Each control on this page draws through part keys of the form Type:Part or
  Type:Part:SubPart, which the executable resolves through the style file to a
  Frame part that holds seven skin names. STATIC.
- The seven skin slots are read in the order SkinNormal, SkinActive,
  SkinDisabled, SkinFocused, SkinHover, SkinReserved, SkinAnchored, and
  SkinDroppable is not read. STATIC.
- An undefined SkinReserved slot takes the SkinActive skin and an undefined
  SkinAnchored slot takes the SkinFocused skin. STATIC for the replacement,
  INFERRED that undefined means an absent Property row.
- TabControl builds a template Grid whose column definitions have the unit type
  bytes 0, 2 and 0 and whose row definitions have 0 and 2, where 2 is the
  constructor default of one star. STATIC for the bytes, INFERRED that 0 means
  Auto.
- The left RepeatButton, the TabPanel and the right RepeatButton sit in row 0
  and columns 0, 1 and 2, and a ContentPresenter that shows the
  SelectedContentPanel spans the three columns of row 1. STATIC.
- The TabControl itself carries the key TabControl:Background_TabPanel, the
  SelectedContentPanel carries TabControl:Background_SelectedContent and the
  name PART_SelectedContentPanel, and the arrow buttons carry
  TabControl:LeftButton and TabControl:RightButton. STATIC.
- The arrow buttons are Visible only while the viewport is smaller than the
  extent, the left button is enabled while the offset is not zero, and the right
  button is enabled while the offset plus the viewport is smaller than the
  extent. STATIC for the arithmetic, INFERRED for the field names.
- The TabControl constructor sets both arrow RepeatButtons to Collapsed, so they
  stay Collapsed until the scroll changed handler runs. STATIC.
- SelectedIndex starts at -1, and a Visible TabItem that is added while nothing
  is selected becomes the selection. STATIC.
- The items handler at 0x009BBD10 acts only when the action byte of its argument
  is 0, which the base handler treats as the add action, and it returns without
  selecting or appending when the last item is missing, is not a TabItem, or has
  a Visibility byte other than 2 while nothing is selected. STATIC for the
  branches, INFERRED for the meaning of the action byte.
- When SelectedIndex is not -1 at an add, the handler appends the content
  element of the last TabItem to the SelectedContentPanel children unless that
  element's parent already is the panel, and sets it Hidden, so every tab
  content is a child of the panel from add time. STATIC for the calls, INFERRED
  for the content element and for the parent field at +0x240.
- The content element of a selected TabItem is appended to the
  SelectedContentPanel children and set Visible, and the content of a deselected
  TabItem is set Hidden. STATIC for the calls, INFERRED for the content element.
- Arrange and measure are virtual slots 54 and 55 of the offset-0 vftable,
  called from the layout core at 0x0092FFFC and 0x0092BAD6. STATIC.
- Control measure removes BorderThickness plus Padding from the available size,
  measures each template child with the remainder and adds the same sum back to
  the largest child size. STATIC.
- Control arrange gives each template child the rectangle inset by
  BorderThickness plus Padding and returns the final size. STATIC.
- ScrollViewer measure removes the same inset sum before the Control measure and
  adds it back after, so the inset counts twice. STATIC.
- ScrollContentPresenter measure stores the child desired size as the extent and
  measures the child again at the larger of its size and the available size when
  it overflows, and its arrange stores the final size as the viewport and
  arranges the child at no less than its desired size. STATIC for the
  arithmetic, INFERRED for the field names.
- ItemsControl owns the measure and arrange pair at slots 54 and 55 that
  Selector, ListBox, TabControl and TabPanel inherit, and the pair changes the
  size on the item axis only when MaxVisibleItems differs from 10000000 or
  MinVisibleItems differs from 0. STATIC.
- TextBox arrange, which LogControl inherits, insets the child by
  BorderThickness plus Padding, raises the child height to MinVisibleLines lines
  when needed, caps it at MaxVisibleLines lines for a Stretch alignment, and
  centres it in the leftover height. STATIC for the arithmetic, UNKNOWN for
  three host fields.
- ScrollBarVisibility text matches Disabled, Hidden, Visible and AutoHidden to
  1, 2, 3 and 4 without regard to ASCII case, and any other text, including
  Auto, gives 0. STATIC.
- ScrollViewer defaults to HorizontalScrollBarVisibility 1 (Disabled) and
  VerticalScrollBarVisibility 3 (Visible). STATIC.
- ScrollViewer computes each scroll bar's Visibility from its mode, the extent
  and the viewport. Auto gives Visible when the extent exceeds the viewport and
  Collapsed otherwise, Disabled gives Collapsed, Hidden gives Hidden, Visible
  gives Visible, and AutoHidden gives Visible when the extent exceeds the
  viewport and Hidden otherwise. STATIC.
- ScrollableWidth and ScrollableHeight are the extent less the viewport with a
  floor of 0. STATIC.
- Visibility text maps Collapsed and False to 0, Hidden to 1 and every other
  text, including Visible, to 2. STATIC.
- The default part table holds 66 records keyed by part name. STATIC.
- No installed style file defines a Caret widget, so Caret:Background resolves
  to no skin. INFERRED.
- The chat scroll bar style SCB_chat defines no HorizontalBackground or
  VerticalBackground part. INFERRED.
- The LogWidget part closure is read from the style file that the form root
  names, common/default.style, which defines every style in the closure. Each
  leaf style of the closure is defined in several of the install's style files
  with rows that sometimes differ, so a merge of all style files is not the
  source of those rows. INFERRED.
- The installed LogWidget form and template use each of the five controls with
  the attributes and styles listed in the census tables. INFERRED.
- No installed form sets TabStripPlacement, SelectedIndex, MaxVisibleItems or
  MinVisibleItems on a TabControl, TabStripPlacement or IsSelected on a TabItem,
  or MinVisibleLines or MaxVisibleLines on a TextBox or LogControl. INFERRED.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All native addresses are VA.

Producing tools are Python `capstone` 5.0.7 disassembly of the pinned image,
`ghidra` CLI `0.2.2` on Ghidra `12.1.4` for cross-references, function sizes
and decompiler views, and the RTTI walk in the verifier. The decompiler views
were leads. Each STATIC row names a signature, string, table, RTTI array or
range hash in the manifest that the verifier compares with the pinned bytes. A
range hash pins a whole function, so a reading of its instructions on this page
holds only while the bytes match.

Install facts come from the `client/sqwt` tree of a contributor-supplied
install whose `game.ver` reads `2012.09.19.0001`. The 15-byte `game.ver` has
SHA-256 `3dbeed87ae1f2805bec69d63137e3deca9dd444ad5caa3ea5dedb68b8a50bfef`. The
SQEX files were decoded in memory with `xivl-tools:tools/blowfish.py`, SHA-256
`7c4208f5d3b7acf20bfb67ddcba706cbb10bf4d1c6c1677b4b11d8685774f90e`, keyed by
each file's base name. The census keeps names and counts only. No resource
bytes are kept.

Labels follow the evidence policy. STATIC means read from instructions or data
of the pinned executable. INFERRED means observed in the install or concluded
from a STATIC read without tracing the use. UNKNOWN means not read.
CALIBRATION marks a value that needs an owner ruling. AUTHORED marks a choice
a painter makes while an UNKNOWN stays open.

## Classes and layout slots

Each class below has three vftables. The offset-0 vftable is the one with the
most slots, and its slots 54 and 55 are the arrange and measure overrides. The
base chain lists the classes between the class and `FrameworkElement`, read
from the RTTI class hierarchy at each vftable's locator. STATIC.

| Class | Base chain | Offset-0 vftable | Arrange (slot 54) | Measure (slot 55) |
|---|---|---|---|---|
| TabControl | Selector, ItemsControl, Control | 0x01081654 | 0x00960BF0 | 0x00960A40 |
| TabPanel | ListBox, Selector, ItemsControl, Control | 0x01081A7C | 0x00960BF0 | 0x00960A40 |
| TabItem | HeaderedContentControl, ContentControl, Control | 0x01080E6C | 0x00937480 | 0x00937310 |
| TabItemBorder | ContentPresenter | 0x01080C24 | 0x0099CA00 | 0x0092A250 |
| SelectedContentPanel | Panel | 0x010813E4 | 0x0092A300 | 0x0092A250 |
| ScrollViewer | ContentControl, Control | 0x010733D4 | 0x00970DF0 | 0x00970070 |
| ScrollContentPresenter | ContentPresenter | 0x010829A4 | 0x009C2EC0 | 0x009C2C90 |
| TextBoxBase | Control | 0x0107573C | 0x00937480 | 0x00937310 |
| TextBoxScrollViewer | ScrollViewer, ContentControl, Control | 0x01074F74 | 0x00970DF0 | 0x00970070 |
| TextBox | TextBoxBase, Control | 0x0106EDB4 | 0x00957B80 | 0x00937310 |
| LogControl | TextBox, TextBoxBase, Control | 0x00FA935C | 0x00957B80 | 0x00937310 |
| ContentControl | Control | 0x0106B634 | 0x00937480 | 0x00937310 |
| ItemsControl | Control | 0x0106FADC | 0x00960BF0 | 0x00960A40 |
| Selector | ItemsControl, Control | 0x01072874 | 0x00960BF0 | 0x00960A40 |
| ListBox | Selector, ItemsControl, Control | 0x01072214 | 0x00960BF0 | 0x00960A40 |
| Control | none | 0x0106BB54 | 0x00937480 | 0x00937310 |

`TabControl` is `Sqwt::Controls::TabControl`, `TabPanel` is
`Sqwt::Controls::Primitives::TabPanel`, `SelectedContentPanel` is
`Sqwt::Controls::TabControl::SelectedContentPanel`, `TextBoxBase` is
`Sqwt::Controls::Primitives::TextBoxBase`, `Selector` is
`Sqwt::Controls::Primitives::Selector`, and `LogControl` is
`Application::Main::SqwtInterface::CustomControl::LogControl`. The others are
`Sqwt::Controls` classes. A base chain of `none` means `FrameworkElement` is
the next base.

The measure core at `0x0092B880` (virtual slot 27) calls the virtual at `+0xDC`
from `0x0092BAD6`, which is slot 55. The arrange helper at `0x0092FD20`, reached
from the arrange core at `0x009302F0` (slot 26), calls the virtual at `+0xD8`
from `0x0092FFFC`, which is slot 54. Both overrides take an output pointer and a
size pointer. STATIC.

`ItemsControl` owns the pair `0x00960BF0` and `0x00960A40`. Its own offset-0
vftable, which its constructor stores and the manifest pins by hash, holds the
pair, while the `Control` vftable holds `0x00937480` and `0x00937310`. The
`Selector`, `ListBox`, `TabControl` and `TabPanel` vftables carry the
`ItemsControl` pair. `TabItem`, `TextBoxBase` and `ContentControl` use the
`Control` pair. `TextBox` and `LogControl` share one arrange override and use the
`Control` measure. `ScrollViewer` and `ScrollContentPresenter` have their own
pairs. The pair `0x0092A250` and `0x0092A300` is the one that
`SelectedContentPanel` carries, and `TabItemBorder` carries the same measure.
STATIC.

## Layout rules

`Control` stores `Padding` at `+0x2A4` and `BorderThickness` at `+0x2B8`. Both
are four floats in the order left, top, right, bottom. The readers at
`0x009371B0` (`Padding`) and `0x00937260` (`BorderThickness`) multiply each side
by a float from a virtual call and truncate it to an integer through the CRT
conversion at `0x009D6600`. The rules below assume a scale of 1. The inset sum
is `BorderThickness` plus `Padding`, left plus right horizontally and top plus
bottom vertically. STATIC.

- Control measure (`0x00937310`): the child available size is the available size
  less the inset sum. Each template child is measured with it. The result is the
  largest child width and height plus the inset sum. STATIC.
- Control arrange (`0x00937480`): every template child gets the rectangle at the
  left and top insets with the final size less the inset sum. The final size is
  returned unchanged. STATIC.
- ScrollViewer measure (`0x00970070`): the available size is reduced by the inset
  sum, the Control measure runs, and the inset sum is added to the result. The
  inset therefore counts twice. STATIC.
- ScrollViewer arrange (`0x00970DF0`): the Control arrange runs, then two
  virtual calls at `+0x70` and `+0x74` of the scroll info object pass the floats
  at `+0xA24` and `+0xA2C`. The object is the one at `+0x3B8` when set and the
  one at `+0xB60` otherwise. What those floats are was not traced. STATIC for the
  calls.
- ScrollContentPresenter measure (`0x009C2C90`): the default measure at
  `0x0092A250` gives the largest child desired size, starting from the size that
  the virtual at `+0x9C` returns. That size is stored as the extent. When the
  child exceeds the available size in either direction the child is measured
  again at the larger of its size and the available size per direction, and the
  second size is stored and returned. STATIC for the arithmetic, INFERRED for
  the extent fields.
- ScrollContentPresenter arrange (`0x009C2EC0`): the final size is stored as the
  viewport, the child is arranged at the larger of the final size and the
  presenter's own desired size per direction, and the final size is returned.
  STATIC for the arithmetic, INFERRED for the viewport fields.
- ItemsControl measure and arrange (`0x00960A40`, `0x00960BF0`): the measure runs
  the Control measure and then adjusts its result. The arrange adjusts the final
  size, runs the Control arrange with the adjusted size and returns it. Both
  adjust the size along the orientation of the item panel only when there is at
  least one item and `MaxVisibleItems` differs from `10000000` or
  `MinVisibleItems` differs from `0`. The adjustment clamps the size through
  `0x00931AE0` between a bound from `MinVisibleItems` and a bound from
  `MaxVisibleItems`, each the first item's extent times the count plus the
  panel's chrome, where an unset maximum gives `10000000`. STATIC for the guard,
  INFERRED for the bounds and the clamp argument order. `MaxVisibleItems` is
  stored at `+0x370` and `MinVisibleItems` at `+0x378`, with the defaults
  `10000000` and `0` set in the `ItemsControl` constructor.
- TextBox arrange (`0x00957B80`): the child height starts as the final height.
  For a vertical alignment byte at `+0x24E` equal to 3 (Stretch in the name
  table at `0x010687B4`, INFERRED) with `MaxVisibleLines` set, the height is
  capped at the line count times the line height (`+0x1964`) less an integer
  behind `+0x17F0`, then adjusted by the fields at `+0x113C` and `+0x4A4`. The
  height is raised to `MinVisibleLines` times the line height in the same way.
  The child rectangle is at the left inset and at the top inset plus half the
  leftover height, with the width and height less the inset sums. The half is
  the double at `0x00F59898`. `MinVisibleLines` is at `+0x176C`,
  `MaxVisibleLines` at `+0x1774`, `MaxLines` at `+0x177C` and `MaxLength` at
  `+0x1764`. The constructor defaults are `MinVisibleLines` 1, `MaxVisibleLines`
  `0x7FFFFFFF`, `MaxLines` 0 and `MaxLength` 0. STATIC for the arithmetic,
  UNKNOWN for the three host fields.

## Style part resolution

A part key is resolved by `0x00933E70`. It splits the key at `:` or `,`, uses
`Frame` when no part is named, finds the part in the current style and follows a
referenced style when the part is not a leaf. A leaf part reads seven condition
properties into seven slots, in this order.

| Slot | Condition |
|---:|---|
| 0 | SkinNormal |
| 1 | SkinActive |
| 2 | SkinDisabled |
| 3 | SkinFocused |
| 4 | SkinHover |
| 5 | SkinReserved |
| 6 | SkinAnchored |

STATIC. `SkinDroppable` appears in the style files and is not read here. When
slot 5 holds the object at `0x01361220` it is replaced by slot 1, and the same
object in slot 6 is replaced by slot 3. INFERRED that the object means an
absent Property row.

The style files group parts by widget type. A part row names a reference type
and a style, as in `Background_SelectedContent` of `TBC_chat`, which references
the `Window` style `TBC_chat_selectedContent`. The `Frame` part of that style
holds the condition rows. INFERRED from the install.

The executable also holds a table of 66 default part records. Each record is
`0x30` bytes. Its first `0x20` bytes are eight floats, the key pointer is at
`+0x20`, and three more pointers follow. The lookup at `0x008E0EB0` compares the
key of the caller's object with each record's key and stores the matching
record's address at `+0x10` of the caller's object. The record fields were not
traced. Every record holds eight floats between 0.3 and 1.0 and three pointers
into the image. STATIC for the table and the lookup, UNKNOWN for the fields. The
records below belong to the controls on this page. The other records belong to
widgets outside it.

| Record | Key |
|---:|---|
| 2 | ScrollBar:LeftButton |
| 3 | ScrollBar:RightButton |
| 4 | ScrollBar:VerticalSlidingThumb |
| 5 | ScrollBar:HorizontalSlidingThumb |
| 6 | ScrollBar:UpButton |
| 7 | ScrollBar:DownButton |
| 8 | ScrollBar:VerticalBackground |
| 9 | ScrollBar:HorizontalBackground |
| 10 | TextBox:HorizontalScrollBar:LeftButton |
| 11 | TextBox:HorizontalScrollBar:RightButton |
| 12 | TextBox:HorizontalScrollBar:HorizontalSlidingThumb |
| 13 | TextBox:HorizontalScrollBar:HorizontalBackground |
| 14 | TextBox:VerticalScrollBar:VerticalSlidingThumb |
| 15 | TextBox:VerticalScrollBar:UpButton |
| 16 | TextBox:VerticalScrollBar:DownButton |
| 17 | TextBox:VerticalScrollBar:VerticalBackground |
| 26 | ScrollViewer:HorizontalScrollBar:LeftButton |
| 27 | ScrollViewer:HorizontalScrollBar:RightButton |
| 28 | ScrollViewer:HorizontalScrollBar:HorizontalSlidingThumb |
| 29 | ScrollViewer:HorizontalScrollBar:HorizontalBackground |
| 30 | ScrollViewer:VerticalScrollBar:VerticalSlidingThumb |
| 31 | ScrollViewer:VerticalScrollBar:UpButton |
| 32 | ScrollViewer:VerticalScrollBar:DownButton |
| 33 | ScrollViewer:VerticalScrollBar:VerticalBackground |
| 42 | TextBox:Background |
| 52 | TabItem:Background_Header |
| 53 | TabControl:Background_SelectedContent |
| 54 | TabControl:Background_TabPanel |
| 56 | LogControl:Background |
| 57 | LogControl:WaitCursor |
| 58 | LogControl:HorizontalScrollBar:LeftButton |
| 59 | LogControl:HorizontalScrollBar:RightButton |
| 60 | LogControl:HorizontalScrollBar:HorizontalSlidingThumb |
| 61 | LogControl:HorizontalScrollBar:HorizontalBackground |
| 62 | LogControl:VerticalScrollBar:UpButton |
| 63 | LogControl:VerticalScrollBar:DownButton |
| 64 | LogControl:VerticalScrollBar:VerticalSlidingThumb |
| 65 | LogControl:VerticalScrollBar:VerticalBackground |

## Part keys set by the constructors

Each row is a `push` of a string address in the named constructor or accessor.
The key rows assign a skin key to an object. The name rows give a template part
its name. STATIC.

| Control | String | Role | String VA | Reference VA |
|---|---|---|---|---|
| TabControl | TabControl:Background_TabPanel | Frame key of the TabControl | 0x01081150 | 0x009BC430 |
| TabControl | TabControl:LeftButton | Key of the RepeatButton at +0x4AC | 0x0108118C | 0x009BC681 |
| TabControl | TabControl:RightButton | Key of the RepeatButton at +0x860 | 0x010811A4 | 0x009BC701 |
| TabControl | PART_SelectedContentPanel | Name of the panel at +0x25A0 | 0x010811BC | 0x009BC810 |
| TabControl | TabControl:Background_SelectedContent | Frame key of the panel at +0x25A0 | 0x010811D8 | 0x009BC853 |
| TabControl | TabControl | Type name | 0x01081144 | 0x009BC3E8 |
| TabItem | TabItem:Background_Header | Frame key of the TabItem | 0x01080A7C | 0x009BB230 |
| TabItem | TabItemBorder | Type name of the header border | 0x01080A98 | 0x009BB04D |
| ScrollViewer | ScrollViewer | Type name | 0x0107302C | 0x00973589 |
| ScrollViewer | ScrollViewer:Background | Background key | 0x0107303C | 0x009735D2 |
| ScrollViewer | PART_HorizontalScrollBar | Template part | 0x01073054 | 0x00970973 |
| ScrollViewer | PART_LeftButton | Template part | 0x01073070 | 0x009709C8 |
| ScrollViewer | PART_RightButton | Template part | 0x01073080 | 0x00970A21 |
| ScrollViewer | PART_Track | Template part | 0x01073094 | 0x00970A7E |
| ScrollViewer | PART_Thumb | Template part | 0x010730A0 | 0x00970AC8 |
| ScrollViewer | PART_VerticalScrollBar | Template part | 0x010730AC | 0x00970B63 |
| ScrollViewer | PART_UpButton | Template part | 0x010730C4 | 0x00970BB8 |
| ScrollViewer | PART_DownButton | Template part | 0x010730D4 | 0x00970C11 |
| ScrollViewer | PART_Track | Template part | 0x010730E4 | 0x00970C6E |
| ScrollViewer | PART_Thumb | Template part | 0x010730F0 | 0x00970CB8 |
| TextBox | TextBox | Type name | 0x0106EBC4 | 0x0095C76E |
| TextBox | TextBox:Background | Background key | 0x0106EBCC | 0x0095C7B6 |
| TextBoxBase | PART_ContentHost | Template part | 0x01074D90 | 0x0097D59B |
| TextBoxBase | PART_TextArea | Template part | 0x01074DA4 | 0x0097D5D5 |
| TextBoxBase | TextBoxScrollViewer | Type name of the content host | 0x01074D7C | 0x0097BB0A |
| TextBoxBase | TextBoxScrollInfo | Type name of the scroll info | 0x01074DB8 | 0x0097BC87 |
| TextBoxBase | Caret | Type name of the caret | 0x01074DCC | 0x0097B988 |
| TextBoxBase | Caret:Background | Background key of the caret | 0x01074DD4 | 0x0097B9BA |
| TextBoxBase | PART_Caret | Template part | 0x01074DE8 | 0x0097BA2E |
| LogControl | LogControl | Type name | 0x00FA8EBC | 0x00595EDB |
| LogControl | LogControl:Background | Background key | 0x00FA8EC8 | 0x00595F25 |
| LogControl | PART_WaitCursor | Template part | 0x00FA8EE0 | 0x0059601E |
| LogControl | PART_WaitCursor | Template part | 0x00FA8EF0 | 0x0059608D |
| LogControl | LogControl:WaitCursor | Key of the wait cursor | 0x00FA8F00 | 0x005960AC |
| LogControl | PART_ContentHost | Template part | 0x00FA8F30 | 0x00591410 |
| LogControl | PART_HorizontalScrollBar | Template part | 0x00FA8F44 | 0x0059149E |
| LogControl | PART_VerticalScrollBar | Template part | 0x00FA8F60 | 0x0059153E |

Four of the keys are assigned to the control itself:
`TabControl:Background_TabPanel` to the `TabControl`,
`TabItem:Background_Header` to the `TabItem`, `TextBox:Background` to the
`TextBox` and `LogControl:Background` to the `LogControl`. The call is
`0x00930BA0` with the control as `this`. The `ScrollViewer` constructor passes
`ScrollViewer:Background` to `0x0092F460` with the control as `this`. STATIC.

The scroll bar parts of `ScrollViewer`, `TextBox` and `LogControl` are keyed in
the default part table as `Type:HorizontalScrollBar:Part` and
`Type:VerticalScrollBar:Part`. The `LogControl` accessors look the bars up in
its content host by the paths below. STATIC.

| Path |
|---|
| PART_ContentHost:PART_HorizontalScrollBar:PART_RightButton |
| PART_ContentHost:PART_HorizontalScrollBar:PART_LeftButton |
| PART_ContentHost:PART_HorizontalScrollBar:PART_Thumb |
| PART_ContentHost:PART_HorizontalScrollBar:PART_Track |
| PART_ContentHost:PART_VerticalScrollBar:PART_DownButton |
| PART_ContentHost:PART_VerticalScrollBar:PART_UpButton |
| PART_ContentHost:PART_VerticalScrollBar:PART_Thumb |
| PART_ContentHost:PART_VerticalScrollBar:PART_Track |

The `LogWidget` form binds `ScrollViewer.LineUpCommand` and
`ScrollViewer.LineDownCommand` to `LogControl_Main_1:PART_ContentHost`, so the
content host is a scroll viewer. `TextBoxBase` names its host class
`TextBoxScrollViewer`. INFERRED.

## TabControl

The `TabControl` constructor at `0x009BC270` builds the template as members of
the control. The reading below is of that constructor, which the manifest pins
by hash. STATIC.

| Member | Offset | Class |
|---|---:|---|
| Left arrow button | `+0x4AC` | RepeatButton |
| Right arrow button | `+0x860` | RepeatButton |
| Tab strip | `+0xC18` | TabPanel |
| Template root | `+0x2200` | Grid |
| Template children list | `+0x24A8` | Grid children |
| Selected content panel | `+0x25A0` | SelectedContentPanel |
| Content presenter | `+0x2898` | ContentPresenter |
| Item container generator | `+0x2B5C` | TabControlItemContainerGenerator |
| Control template | `+0x2B84` | ControlTemplate |
| Scroll changed handler | `+0x2C58` | ScrollChangedEventHandler, target `0x009BB320` |

The template Grid has three column definitions with the unit type bytes 0, 2 and
0, and two row definitions with 0 and 2. The definition constructors set the
byte 2 and a value of 1.0 by default, so 2 is one star, and 0 is read as Auto
from the usual unit order. The children are added in the order left button,
right button, tab strip, content presenter. The grid cell
setters at `0x00976C80` (column), `0x00976C40` (row) and `0x00976D10` (column
span) place the right button in column 2, the tab strip in column 1 and the
content presenter in row 1 with a column span of 3. The left button keeps the
default cell, column 0 and row 0. The content presenter shows the
`SelectedContentPanel`. STATIC.

The `TabStripPlacement` property is stored at `+0x4A8` with a default byte of 1.
The enum names for that byte were not read. Both the `TabControl` and the
`TabItem` register it. UNKNOWN.

Scroll arrows. The constructor sets both buttons Collapsed through the
Visibility setter at `0x0093C2A0`, at `0x009BC6D6` for the left button and at
`0x009BC756` for the right button, each after a call to `0x0093C3C0` on the same
button that was not traced. They stay Collapsed until the scroll changed handler
runs. STATIC, covered by the constructor range hash. The scroll changed handler at
`0x009BB320` reads three floats of its event argument at `+0x24`, `+0x2C` and `+0x44`. In the order extent, offset
and viewport, it sets both buttons Visible when the viewport is smaller than
the extent and Collapsed otherwise. It enables the left button when the offset
is not zero and the right button when the offset plus the viewport is smaller
than the extent. The Visibility setter at `0x0093C2A0` stores to the property at
`+0x158`. STATIC for the arithmetic, INFERRED for the field names. Whether the
arrows show for the two `LogWidget` tabs depends on the measured header widths,
which this page does not compute.

Selection. `Selector` stores `SelectedIndex` at `+0x480` with a default of `-1`.
The items handler at `0x009BBD10` runs the base handler at `0x0096F8F0` and then
acts only when the byte at `+4` of its argument is 0. The base handler reads the
same byte as 5 for a reset and as 0 for per-item work, so 0 is INFERRED to be
the add action. The handler takes the last item, at the count less 1, through
`0x00961420`, which gives no item for a negative index, and casts it to `TabItem`
through the CRT dynamic cast at `0x009DA6CC`, INFERRED from the `Object` source
and `TabItem` target descriptors it takes. A missing or non-`TabItem` item ends
the handler. STATIC for the branches.

- `SelectedIndex` is `-1` and there is at least one item: a `TabItem` whose
  Visibility byte at `+0x158` equals 2 is selected through `0x0096EC00` with its
  index and the flags 0 and 1, and the handler ends. A `TabItem` with any other
  Visibility returns at once, so it is neither selected nor appended.
- `SelectedIndex` is not `-1`, or there are no items: the content element comes
  from `0x009BAA00`, which returns the member at `TabItem +0x10DC`, INFERRED to
  be the content element. When the element's field at `+0x240` already holds the
  panel at `+0x25A0` the handler ends. Otherwise it appends the element to the
  children list at `+0x2848` through `0x0068B9F0`, signals the change through
  `0x00813050` and the list's callback when one is set, and sets the element
  Hidden by passing the Visibility byte 1 to `0x0093C2A0`. INFERRED that the
  field at `+0x240` is the parent. With no items the cast gives no item and the
  handler ends.

Every tab content is therefore a child of the `SelectedContentPanel` from the
moment its item is added, Hidden unless it is the selection. The selection
changed handler at `0x009BBB00` sets `IsSelected` false on removed items and true
on added items. For each added item it appends the item's content element to the
`SelectedContentPanel` children unless the field at `+0x240` already holds the
panel, sets the content Visible, and for removed items it sets the content
Hidden. STATIC for the calls, INFERRED for the content element.

Tab headers. The `TabItem` constructor at `0x009BB0F0` creates a `TabItemBorder`
and keys itself with `TabItem:Background_Header`. `TabItem` registers
`IsSelected` and `TabStripPlacement`, and `HeaderedContentControl` supplies
`Header`. Which of the seven skin slots a selected header draws was not traced.
The install gives `TBI_basis_header` the skins `TBI_basis_header_N` for Normal,
`_A` for Active, `_H` for Hover and `_R` for Reserved. UNKNOWN, see the open
questions.

Tab strip layout. `TabPanel` derives from `ListBox`, and its items panel,
orientation and spacing were not traced. UNKNOWN, see the open questions.

The `LogWidget` template addresses the panel by the path
`TabControl_Log:PART_SelectedContentPanel` and the headers by
`TabControl_Log:TabItem_1` and `TabControl_Log:TabItem_2`, so those names are
part of the contract. The panel path animates `VisualOpacity` in storyboards.
INFERRED.

## ScrollViewer and ScrollContentPresenter

The `ScrollViewer` constructor at `0x00973190` registers its properties and
keys itself with `ScrollViewer:Background`. The part names below belong to its
template, which two builders at `0x00970940` and `0x00970B30` create.
`HorizontalScrollBarVisibility` is stored at `+0x3B0` with a default of 1, and
`VerticalScrollBarVisibility` at `+0x3B2` with a default of 3. STATIC.

`TextBoxScrollViewer` derives from `ScrollViewer` and uses its measure and
arrange. The content host of `TextBox` and `LogControl` is therefore a
`ScrollViewer` for layout. STATIC.

The viewport, extent and offset properties are registered on both
`ScrollViewer` and `ScrollContentPresenter`. The presenter keeps the extent from
its measure and the viewport from its arrange, as described in the layout rules.

Scroll info update. The function at `0x00973660` takes a viewport size, an
extent size and the two offsets. It stores them in the properties below, derives
the scrollable sizes, and sets the computed visibility of each bar. When the
horizontal bar part at `+0xC74` is absent it stores the horizontal offset itself,
and likewise the vertical offset with the bar part at `+0xC78`. When a bar part
exists it passes the visibility, the scrollable size, the viewport and the large
change to the bar. STATIC for the stores, INFERRED for the roles of the bar
arguments.

| Property | Offset | Value |
|---|---:|---|
| ExtentHeight | `+0x36C` | extent height |
| ExtentWidth | `+0x374` | extent width |
| HorizontalOffset | `+0x37C` | horizontal offset argument |
| VerticalOffset | `+0x384` | vertical offset argument |
| ScrollableHeight | `+0x38C` | extent height less viewport height, floor 0 |
| ScrollableWidth | `+0x394` | extent width less viewport width, floor 0 |
| ViewportHeight | `+0x39C` | viewport height |
| ViewportWidth | `+0x3A4` | viewport width |
| ComputedHorizontalScrollBarVisibility | `+0x3AC` | from the mode at `+0x3B0` |
| ComputedVerticalScrollBarVisibility | `+0x3AE` | from the mode at `+0x3B2` |

The computed visibility uses the Visibility values below (0 Collapsed, 1 Hidden,
2 Visible) and the mode and extent against viewport as follows.

| Mode | Value | Extent fits the viewport | Extent exceeds the viewport |
|---|---:|---|---|
| Auto | 0 | Collapsed | Visible |
| Disabled | 1 | Collapsed | Collapsed |
| Hidden | 2 | Hidden | Hidden |
| Visible | 3 | Visible | Visible |
| AutoHidden | 4 | Hidden | Visible |

STATIC. The `LogWidget` chat menu `ScrollViewer` sets `Auto`, so its bar shows
only while its content exceeds the viewport, and each `LogControl` sets
`AutoHidden`, so its bar stays Hidden until the log exceeds the viewport. How
Hidden and Collapsed differ in layout follows the Visibility rules of the layout
core, which this page does not trace. INFERRED.

The scrollbar visibility text is parsed by `0x00914460` and written by
`0x00914500`. The compare is the C-locale ASCII case-folding compare at
`0x009D2346`, with a locale-aware path at `0x009D237B` that the code takes when
the global at `0x01363F24` is nonzero. STATIC.

| Text | Value |
|---|---:|
| Auto | 0 |
| Disabled | 1 |
| Hidden | 2 |
| Visible | 3 |
| AutoHidden | 4 |

The Visibility text is parsed by `0x00914580` and written by `0x009145F0`.

| Text | Value |
|---|---:|
| Collapsed | 0 |
| Hidden | 1 |
| Visible | 2 |

`False` maps to Collapsed and every other text maps to Visible, including
`True`, `Visible` and the empty text. STATIC.

## TextBox and LogControl

`TextBox` derives from `TextBoxBase`, which derives from `Control`. `LogControl`
derives from `TextBox`. The `TextBox` constructor at `0x0095C300` keys the control
with `TextBox:Background`. `TextBoxBase` creates a `TextBoxScrollViewer` named
`PART_ContentHost`, a text area named `PART_TextArea`, a `TextBoxScrollInfo` and
a caret. The caret is named `PART_Caret` and keyed `Caret:Background`. No
installed style file defines a `Caret` widget, and the default part table has no
caret record. INFERRED that the caret draws no skin.

A `TextBox` holds a `Sqwt::DocumentModel::TextDocument`. The chat input sets no
`Text`, and every installed `TextBox` element sets `CaretIndex`. INFERRED.

`LogControl` adds the properties in the registered names table below. The chat
log window page records a per-frame update that feeds the control from the log
module, so the form carries no line text. The `LogWidget` form sets the
presentation values `FontSize`, `FontEdge`, `FontEdgeColor`, `LogFormat`,
`LogFormat2`, `IndentHasName`, `IndentNoName` and `LinesPerSec`. The
`LogControl` constructor creates a wait cursor named `PART_WaitCursor` keyed
`LogControl:WaitCursor`. Painting the lines is outside this page, see the open
questions.

## Registered property names

The executable registers these names in static initializers laid out per class
in the string table. The grouping is by address adjacency. The offsets named
elsewhere on this page were checked against the setters. INFERRED.

| Owner | Registered names |
|---|---|
| TabControl | TabStripPlacement, SelectedContent |
| TabItem | IsSelected, TabStripPlacement |
| Selector | SelectedIndex, SelectedItem |
| ItemsControl | MaxVisibleItems, MinVisibleItems |
| ScrollViewer | CanContentScroll, ComputedHorizontalScrollBarVisibility, ComputedVerticalScrollBarVisibility, ExtentHeight, ExtentWidth, HorizontalOffset, VerticalOffset, HorizontalScrollBarVisibility, VerticalScrollBarVisibility, ScrollableHeight, ScrollableWidth, ViewportHeight, ViewportWidth |
| ScrollContentPresenter | ViewportHeight, ViewportWidth, ExtentHeight, ExtentWidth, HorizontalOffset, VerticalOffset |
| TextBoxBase | IsReadOnly, AcceptsReturn, HorizontalScrollBarVisibility, VerticalScrollBarVisibility, TextChanged |
| TextBox | SelectionStart, SelectionLength, CaretIndex, TextWrapping, MaxLength, MinVisibleLines, MaxVisibleLines, MaxLines, TextAlignment, FirstVisibleLineIndex, VisibleLines, VerticalTextAlignment, TextUI, IsReplaceUnaccept, IsValidFirst |
| LogControl | LogChanged, LinesPerSec, LinesPerSecHi, LinesBeginHi, Categories, LogFormat, LogFormat2, IndentHasName, IndentNoName, SignalText, SignalCommand, Action, Talker, LastCategory, IsEnableKeyWait, IsDisplayLastOne |

## Install census

The census covers the form, template, style and skin files of the install's
`client/sqwt` tree and counts the five controls in the forms and templates.
The first table lists, per control, the elements, the files that hold them, the
distinct attribute names, the distinct `SqwtStyle` values and how many of those
values name a style defined for that control in some style file. One form fails
to parse as XML because it uses an unbound prefix, and the census skips it.
INFERRED.

| Control | Elements | Files | Attribute names | Style values | Defined |
|---|---:|---:|---:|---:|---:|
| TabControl | 27 | 25 | 17 | 6 | 6 |
| TabItem | 102 | 25 | 18 | 4 | 4 |
| TextBox | 122 | 52 | 45 | 4 | 4 |
| ScrollViewer | 27 | 21 | 17 | 4 | 4 |
| LogControl | 6 | 4 | 21 | 2 | 2 |

Every `SqwtStyle` value used on these controls is defined in a style file. The
full attribute and style counts are in the manifest.

`LogWidget` uses each control with these attributes and styles.

| Control | Elements | Style | Attributes |
|---|---:|---|---|
| TabControl | 1 | TBC_chat | FontSize, IsHitTestVisible, IsTabStop, Name, SqwtStyle |
| TabItem | 2 | TBI_chat | Focusable, FontSize, FontStyle, Header, HorizontalContentAlignment, Margin, MinWidth, Name, SqwtDesignData.StringValue2, SqwtStyle, VerticalContentAlignment |
| TextBox | 1 | TEB_chat_input | Anchor, CaretIndex, CharacterCasing, FontHeightSpacing, IsUndoEnabled, MaxLength, MaxLines, Name, Padding, SqwtStyle, Style |
| ScrollViewer | 1 | ScrollViewer_chat | MaxHeight, MinHeight, SqwtStyle, VerticalScrollBarVisibility |
| LogControl | 2 | LogControl_chat | FontEdge, FontEdgeColor, FontHeightSpacing, FontSize, FontStyle, IndentHasName, IndentNoName, IsEnableKeyWait, LinesPerSec, LinesPerSecHi, LogFormat, LogFormat2, Margin, Name, SqwtStyle, Style, VerticalScrollBarVisibility |

`ScrollViewer` sets `VerticalScrollBarVisibility` to `Auto`, which parses to 0,
and `LogControl` sets it to `AutoHidden`, which parses to 4. `TabItem` sets
`Header` to a text-id reference and the first tab sets `Margin` to `2,0,0,0`.
Both `LogWidget` `TabItem` elements set `MinWidth` to 64. `MaxLength` on the
chat input is `512b`, which the chat window page records as a byte limit.

The `LogWidget` form and template address controls and parts by paths that
contain a colon. A painter has to resolve every path below. INFERRED.

| Path | Uses |
|---|---:|
| LogControl_Main_1:PART_ContentHost | 5 |
| LogControl_Main_1:PART_ContentHost:PART_TextArea | 3 |
| LogControl_Main_1:PART_ContentHost:PART_VerticalScrollBar | 2 |
| LogControl_Main_1:PART_ContentHost:PART_VerticalScrollBar:PART_DownButton | 2 |
| LogControl_Main_1:PART_ContentHost:PART_VerticalScrollBar:PART_Track | 2 |
| LogControl_Main_1:PART_ContentHost:PART_VerticalScrollBar:PART_Track:PART_Thumb | 2 |
| LogControl_Main_1:PART_ContentHost:PART_VerticalScrollBar:PART_UpButton | 2 |
| LogControl_Main_2:PART_ContentHost | 5 |
| LogControl_Main_2:PART_ContentHost:PART_TextArea | 3 |
| LogControl_Main_2:PART_ContentHost:PART_VerticalScrollBar | 2 |
| LogControl_Main_2:PART_ContentHost:PART_VerticalScrollBar:PART_DownButton | 2 |
| LogControl_Main_2:PART_ContentHost:PART_VerticalScrollBar:PART_Track | 2 |
| LogControl_Main_2:PART_ContentHost:PART_VerticalScrollBar:PART_Track:PART_Thumb | 2 |
| LogControl_Main_2:PART_ContentHost:PART_VerticalScrollBar:PART_UpButton | 2 |
| TabControl_Log:PART_SelectedContentPanel | 3 |
| TabControl_Log:TabItem_1 | 3 |
| TabControl_Log:TabItem_2 | 3 |
| TextBox_ChatInput:PART_ContentHost | 2 |
| TextBox_ChatInput:PART_ContentHost:PART_TextArea | 2 |

The parts the five styles reference, depth first, are below. A reference to the
`Window` style `NULL` is not defined in the style file.

| Owner | Style | Part | Reference | Referenced style |
|---|---|---|---|---|
| TabControl | TBC_chat | Background_TabPanel | Window | NULL |
| TabControl | TBC_chat | Background_SelectedContent | Window | TBC_chat_selectedContent |
| TabControl | TBC_chat | LeftButton | Button | BTN_chat_tabHeaderLeft |
| TabControl | TBC_chat | RightButton | Button | BTN_chat_tabHeaderRight |
| TabItem | TBI_chat | Background_Header | Button | TBI_basis_header |
| TextBox | TEB_chat_input | Background | Window | TEB_chat_input |
| TextBox | TEB_chat_input | RightDownSpace | Window | NULL |
| TextBox | TEB_chat_input | HorizontalScrollBar | ScrollBar | SCB_basis |
| ScrollBar | SCB_basis | UpButton | Button | BTN_basis_scrollUp |
| ScrollBar | SCB_basis | DownButton | Button | BTN_basis_scrollDown |
| ScrollBar | SCB_basis | LeftButton | Button | BTN_basis_scrollLeft |
| ScrollBar | SCB_basis | RightButton | Button | BTN_basis_scrollRight |
| ScrollBar | SCB_basis | HorizontalSlidingThumb | Button | BTN_basis_scrollSlidingH |
| ScrollBar | SCB_basis | VerticalSlidingThumb | Button | BTN_basis_scrollSlidingV |
| ScrollBar | SCB_basis | HorizontalBackground | Window | SCB_basis_bgH |
| ScrollBar | SCB_basis | VerticalBackground | Window | SCB_basis_bgV |
| TextBox | TEB_chat_input | VerticalScrollBar | ScrollBar | SCB_basis |
| ScrollViewer | ScrollViewer_chat | Background | Window | WIN_null |
| ScrollViewer | ScrollViewer_chat | RightDownSpace | Window | NULL |
| ScrollViewer | ScrollViewer_chat | HorizontalScrollBar | ScrollBar | SCB_chat |
| ScrollBar | SCB_chat | UpButton | Button | BTN_chat_scrollUp |
| ScrollBar | SCB_chat | DownButton | Button | BTN_chat_scrollDown |
| ScrollBar | SCB_chat | LeftButton | Button | BTN_chat_scrollLeft |
| ScrollBar | SCB_chat | RightButton | Button | BTN_chat_scrollRight |
| ScrollBar | SCB_chat | HorizontalSlidingThumb | Button | BTN_chat_scrollSlidingH |
| ScrollBar | SCB_chat | VerticalSlidingThumb | Button | BTN_chat_scrollSlidingV |
| ScrollViewer | ScrollViewer_chat | VerticalScrollBar | ScrollBar | SCB_chat |
| LogControl | LogControl_chat | WaitCursor | Button | BTN_chat_logWaitCursor |
| LogControl | LogControl_chat | Background | Window | WIN_null |
| LogControl | LogControl_chat | RightDownSpace | Window | NULL |
| LogControl | LogControl_chat | HorizontalScrollBar | ScrollBar | SCB_chat |
| LogControl | LogControl_chat | VerticalScrollBar | ScrollBar | SCB_chat |

The closure is read from the style file that the form root names, here
`common/default.style`, which defines every style in it. The install holds 20
style files, and many define the same style names. The table counts, for the leaf
styles of the closure, the style files that define each one including the named
file, and how many leaf styles have rows that differ in another file. Which file
the executable consults for a style missing from the named file was not traced.
INFERRED.

| Leaf styles | Fewest style files defining one | Most style files defining one | Leaf styles with differing rows |
|---:|---:|---:|---:|
| 21 | 2 | 8 | 9 |

The leaf `Frame` parts reached from the closure give the skins below. `NULL`
means no skin. The full seven-slot rows are in the manifest. The skins named here resolve
to cell skins and one animation skin in the skin file, and no named skin is
missing. INFERRED.

| Type | Style | SkinNormal | SkinActive | SkinHover | SkinReserved |
|---|---|---|---|---|---|
| Window | TBC_chat_selectedContent | TBC_chat_selectedContent | TBC_chat_selectedContent | TBC_chat_selectedContent | NULL |
| Button | BTN_chat_tabHeaderLeft | BTN_chat_tabHeaderLeft_N | BTN_chat_tabHeaderLeft_H | BTN_chat_tabHeaderLeft_H | BTN_chat_tabHeaderLeft_H |
| Button | BTN_chat_tabHeaderRight | BTN_chat_tabHeaderRight_N | BTN_chat_tabHeaderRight_H | BTN_chat_tabHeaderRight_H | BTN_chat_tabHeaderRight_H |
| Button | TBI_basis_header | TBI_basis_header_N | TBI_basis_header_A | TBI_basis_header_H | TBI_basis_header_R |
| Window | TEB_chat_input | TEB_chat_input_bg_N | TEB_chat_input_bg_H | TEB_chat_input_bg_H | NULL |
| Button | BTN_basis_scrollUp | BTN_basis_scrollUp_N | BTN_basis_scrollUp_H | BTN_basis_scrollUp_H | BTN_basis_scrollUp_H |
| Button | BTN_basis_scrollDown | BTN_basis_scrollDown_N | BTN_basis_scrollDown_H | BTN_basis_scrollDown_H | BTN_basis_scrollDown_H |
| Button | BTN_basis_scrollLeft | BTN_basis_scrollLeft_N | BTN_basis_scrollLeft_H | BTN_basis_scrollLeft_H | BTN_basis_scrollLeft_H |
| Button | BTN_basis_scrollRight | BTN_basis_scrollRight_N | BTN_basis_scrollRight_H | BTN_basis_scrollRight_H | BTN_basis_scrollRight_H |
| Button | BTN_basis_scrollSlidingH | BTN_basis_scrollSlidingH_N | BTN_basis_scrollSlidingH_H | BTN_basis_scrollSlidingH_H | NULL |
| Button | BTN_basis_scrollSlidingV | BTN_basis_scrollSlidingV_N | BTN_basis_scrollSlidingV_H | BTN_basis_scrollSlidingV_H | NULL |
| Window | SCB_basis_bgH | SCB_basis_bgH | SCB_basis_bgH | SCB_basis_bgH | NULL |
| Window | SCB_basis_bgV | SCB_basis_bgV | SCB_basis_bgV | SCB_basis_bgV | NULL |
| Window | WIN_null | NULL | NULL | NULL | NULL |
| Button | BTN_chat_scrollUp | BTN_chat_scrollUp_N | BTN_chat_scrollUp_H | BTN_chat_scrollUp_H | NULL |
| Button | BTN_chat_scrollDown | BTN_chat_scrollDown_N | BTN_chat_scrollDown_H | BTN_chat_scrollDown_H | NULL |
| Button | BTN_chat_scrollLeft | BTN_chat_scrollLeft_N | BTN_chat_scrollLeft_H | BTN_chat_scrollLeft_H | NULL |
| Button | BTN_chat_scrollRight | BTN_chat_scrollRight_N | BTN_chat_scrollRight_H | BTN_chat_scrollRight_H | NULL |
| Button | BTN_chat_scrollSlidingH | BTN_chat_scrollSlidingH_N | BTN_chat_scrollSlidingH_H | BTN_chat_scrollSlidingH_H | NULL |
| Button | BTN_chat_scrollSlidingV | BTN_chat_scrollSlidingV_N | BTN_chat_scrollSlidingV_H | BTN_chat_scrollSlidingV_H | NULL |
| Button | BTN_chat_logWaitCursor | BTN_chat_logWaitCursor | BTN_chat_logWaitCursor | BTN_chat_logWaitCursor | NULL |

The style `SCB_chat` has no `HorizontalBackground` or `VerticalBackground` part,
so the style gives the chat scroll bars no track skin. The text box scroll bar
style `SCB_basis` has both. INFERRED. Whether the executable then draws nothing
or a fallback is UNKNOWN, since the default part table holds
`ScrollBar:VerticalBackground` (record 8) and
`LogControl:VerticalScrollBar:VerticalBackground` (record 65) with untraced
fields, see the open questions.

## Open questions

- UNKNOWN. Which skin slot a selected TabItem header draws. The style gives
  TBI_basis_header separate Active and Reserved skins. Trace targets are the
  IsSelected change path of TabItem and the consumer that picks one of the seven
  slots that 0x00933E70 reads for TabItem:Background_Header. A painter needs an
  authored choice until then.
- UNKNOWN. How the TabPanel places its TabItem headers. Trace targets are the
  TabPanel constructor at 0x009BCFC0, which calls the base constructor at
  0x0096D980, and the TabPanel vftable at 0x01081A7C beyond slots 54 and 55, for
  the items panel, orientation and spacing. A painter needs an authored
  placement until then.
- UNKNOWN. How HorizontalOffset and VerticalOffset move the content child during
  arrange. The ContentPresenter arrange at 0x0099CA00 was not traced.
- UNKNOWN. The meaning of the TextBox arrange host fields at +0x113C and
  +0x04A4, of the integer behind +0x17F0, and of the eight floats and three
  pointers in each default part record.
- UNKNOWN. How a part whose style is NULL or undefined resolves. The resolver at
  0x00933E70 returns without writing a skin, and the loader at 0x00934210 names
  fallback styles, but the join was not traced. The default part records for
  ScrollBar:VerticalBackground (8) and LogControl:VerticalScrollBar:VerticalBackground
  (65) are part of the same question.
- UNKNOWN. How LogControl paints its lines. The lines come from the native log
  document, which this page does not trace.
- UNKNOWN. Whether Lua or a command changes the tab selection before the first
  paint.
- UNKNOWN. How the caret is drawn. It has the part PART_Caret and the key
  Caret:Background, and no style file defines it.

## Reproduction

Run `python tools/verify_sqwt_tab_scroll_text.py`, adding `--exe
orig/ffxivgame.exe` to compare the pinned bytes. Run `python
tools/census_sqwt_tab_scroll_text.py --install <client/sqwt> --blowfish
<xivl-tools/tools/blowfish.py>` to recount the install. The decode keeps every
file in memory and prints counts only.

## Painter requirements for LogWidget

A painter for installed forms needs these rules before it can admit and paint
`LogWidget`. Each item cites the section above.

1. Admit the five element names with the attribute sets in the `LogWidget`
   table. The values that need a parser are `AutoHidden` and `Auto` for the
   scroll bar visibility, the text-id references in `Header`, `IndentHasName`
   and `IndentNoName`, and the byte-suffixed `MaxLength`.
2. Resolve each `SqwtStyle` through the style file named by the form root, read
   the seven condition slots of every leaf `Frame` part in the slot order, and
   apply the Reserved and Anchored fallbacks. INFERRED, since the lookup order
   for a style missing from that file was not traced.
3. Build the `TabControl` template as the Grid described above, keep the arrow
   buttons Collapsed from construction until a scroll changed event arrives, and expose the content panel as
   `PART_SelectedContentPanel` and the headers as the named `TabItem` elements.
   Expose `PART_ContentHost` with `PART_TextArea` on the `TextBox` and on each
   `LogControl`, and `PART_VerticalScrollBar` with its track, thumb and arrow
   parts under the `LogControl` host.
4. Draw `TBC_chat_selectedContent` behind the selected content and
   `TBI_basis_header` behind each header, whose slot is an open question. Drawing
   nothing for the `NULL` tab panel frame is INFERRED, since the resolution of a
   `NULL` part is UNKNOWN, so a painter marks it as an authored choice.
5. Follow the add rules. While nothing is selected, select the last added item
   when it is a Visible `TabItem` and ignore any other item without appending it.
   With a selection, append the added item's content to the content panel unless
   it is already there, and set it Hidden. On a selection change append the
   content unless it is already a child, set it Visible, and set the content of a
   deselected item Hidden.
6. Apply the Control measure and arrange rules for every control here, the
   ScrollViewer double inset, and the presenter extent and viewport rules. The
   TextBox arrange reduces to the Control arrange when the line limits are the
   defaults and the final height is at least the one-line floor.
7. Draw the chat input frame from `TEB_chat_input` with no text and no caret
   skin, the log scroll bar parts from `SCB_chat`, and the bar Visibility from
   the table in the `ScrollViewer` section. Drawing no caret and no track skin is
   INFERRED, since the caret draw and the resolution of an undefined part are
   UNKNOWN, so a painter marks both as authored choices.
8. Treat the `Style_TextBox` and `Style_LogControl` template styles as inert for
   a static draw, since they hold only triggers.

Items 1 to 8 do not settle the open questions above. A painter that needs the
selected header skin, the header placement, the `NULL` part resolution or the
caret before those are answered has to label its choice as authored.
