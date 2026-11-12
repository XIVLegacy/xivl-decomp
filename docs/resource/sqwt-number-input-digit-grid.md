# Sqwt NumberInputBox digit grid and assets

NumberInputBox lays out one NumberLabel per decimal place in a Grid. A
ContentLabel draws each digit from the effective value. The installed
styles supply the frame textures for the digit arrows and limit buttons.

## Input and method

Evidence IDs NI-VIS-DATA (decoded client data), NI-VIS-GRID, NI-VIS-DIGIT,
NI-VIS-STYLE and NI-VIS-DRAW (STATIC). Target is 1.23b build
2012.09.19.0001, PE32 i386, base 0x00400000, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
Addresses are VAs. Evidence comes from decoded SQWT metadata and bounded
disassembly. See the [integer range contract](sqwt-number-input-box.md)
for the effective Value and edit commands. Live appearance is unverified.

## Installed assets

Verify the PE size and hash before reading it. Decrypt each listed SQWT
text resource using the basename as the Blowfish key after the eight-byte
SQEX prefix. Verify exact re-encryption and parse only metadata. The
EventItemSelectWidget root selects common/default.style at line 1, which
names common/default.skin. The skin's CellSkins group at line 2 names
common/default.dds. The installed texture resolution to
common/default.win32.gtex is established by the [NpcSay backdrop texture route](npcsay-backdrop-draw.md). Its exact size and hash were rechecked.

| SQWT path | Installed bytes and SHA-256 | Decoded bytes and SHA-256 |
| --- | --- | --- |
| widget/EventItemSelectWidget.form | 33984, 814733cf4cecf378226de4714b338e1bfa0889845e580dcb8461a3bd03813e0d | 33976, 00b954b0d754c87ab32096d4b96a98e516b7463526982246c0aedb7bb459b459 |
| common/default.style | 353783, 632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0 | 353775, 33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6 |
| common/default.skin | 141967, 6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640 | 141959, 929e65e9d5f191292d9b044e3e659ba115f18f6b3d8a82894046a7c75cbcad5a |
| common/default.win32.gtex | 4194336, c7f11f369e21b19b79fbe174bb10f78f7312b366383db9774e873238b2d5d88f | GTEX, see tracked texture contract |

Form lines 38-51 put the first named NumberInputBox in a Label, in Auto
column 2 of the row grid. The Label selects
ControlTemplate_Label_NumberInputBox, LAB_null, Style_Label_NumberInput,
and FontSize 14. This form selection repeats for all 16 rows. The template
contains the ContentPresenter described in the linked tracked page.
The form does not directly request Style_NumberInputBox. Child sizing
cannot be read from that separate Slider style.

## Control and template Grid

The NumberInputBox constructor at 0x0069D140 initializes embedded Grid
at C+0x2B50 through 0x009750D0 (call 0x0069D207). That function installs
Grid vtable 0x01074554 at 0x00975101. The template at C+0xA24 is constructed
with the owner and Grid arguments at 0x0069D1AE..0x0069D1BC and installed
on the Control by 0x00938170 at 0x0069DC03. The constructor appends
three row definitions at 0x0069D3F5..0x0069D47F, with unit/value tuples
(0,0), (2,1.0), (0,0). Byte 2 is star and byte 0 is identified as Auto in
the [TabControl template evidence](sqwt-tab-scroll-text-parts.md).
Grid setters 0x00976C40, 0x00976C80, 0x00976CC0 and 0x00976D10 place
row, column, row span and column span respectively.

| Element | Object offset or allocation | Placement and evidence |
| --- | --- | --- |
| Track, PART_Track | C+0x410 | Ctor 0x0099C460, name at 0x0069D348, attached 0x0069D393 and requested Collapsed with byte 0 at 0x0069D3DC..0x0069D3DF. |
| Two NULL-style Label children | C+0xAF4, C+0xE60 | Ctor 0x009742A0. Attached at 0x0069D48F and 0x0069D4D2, style writes 0x0069D536 and 0x0069D578. First keeps row/column 0, second sets row 2 at 0x0069D597. No painted NIB asset assigned to these labels. |
| Background FrameworkElement, style group Label | C+0x3658 | Ctor 0x0092F640 at 0x0069D242. Row 1, column 1 at 0x0069D5D3..0x0069D5DB. Type group Label set at 0x0069D60C and style at 0x0069D649. Its column span is the digit count at 0x0069EA93..0x0069EA9B. |
| Maximize Button | C+0x2EF0 | Ctor 0x00974410 installs Button vtable 0x01073CAC. Row 1, column 0 default. HorizontalAlignment byte 2, vertical 1 at 0x0069D81E..0x0069D836. Command Slider.MaximizeValue at 0x0069D7F1..0x0069D7F8. |
| Minimize Button | C+0x32A4 | Same Button ctor. Row 1, final column at 0x0069EAF6. HorizontalAlignment byte 0, vertical 1 at 0x0069D84E..0x0069D866. Slider.MinimizeValue at 0x0069D801..0x0069D80C. |
| TextBox editor | C+0x11D0 | Ctor 0x0095C300 installs 0x0106EDB4. Row 1, column 1 at 0x0069D941/0x0069D9FA and column span equals digit count at 0x0069EAA0..0x0069EABE. Constructor sets VisualOpacity 0 (0x0069D9B9..0x0069D9C5, descriptor 0x0135988C, Visual+0x48). Digit Operate path sets opacity 1 and gives it focus at 0x0069C020..0x0069C064. |
| NumberLabel digit cells | heap, one 0x1248-byte object per place | Allocates/constructs at 0x0069E4B7..0x0069E4DA and attaches to Grid child list at 0x0069E555..0x0069E55F. Row 0 default with row span 3 at 0x0069E62D..0x0069E630. |

The rebuild at 0x0069E330 is called by both 0x0069EBE0 (slot 70, reached
for Maximum from 0x00967337..0x00967345 when owner byte +0x20 bit 0 is
set) and 0x0069EC10 (slot 10, after the base call at 0x0069EC18).
It deletes the old digit cells at 0x0069E3BA..0x0069E42B before
formatting Maximum from C+0x344 through 0x00988A90 and taking the string
length at 0x0069E447..0x0069E461. This rebuild replaces the cells even
when the decimal place count is unchanged. The default formatter uses %d at
0x01077268. For nonnegative Maximum this is the decimal place count,
including one place for zero. It clears and recreates columns at
0x0069E8FA..0x0069EA7A. Outer columns use unit 0 and value 0. Each digit
column uses the default ColumnDefinition constructor 0x00995450, unit 2
and value 1.0. The digit vector is in increasing place order. It assigns
column count, then count-1, down to 1 to those cells
(0x0069E959..0x0069E9D2, 0x0069EA2C..0x0069EA38). In left-to-right layout
this gives max button, most-significant to least-significant digits,
then min button. This sequence is INFERRED from STATIC cell coordinates.
Button/label/grid alignment name translations follow the shipped enums.

## Digit controls and Value binding

NumberLabel constructor 0x0069DC40 installs its own Label-derived vtable
0x00FCB574. It constructs a Grid at D+0x700 (0x0069DCCD) and installs
the embedded template at D+0x630 through 0x00938170 at 0x0069E2D7.
The Grid row tuple sequence is the same (0,0), (2,1.0), (0,0) at 0x0069DD84..0x0069DE10.
It creates RepeatButton children at D+0xAA0 and D+0xE54 through
0x009988C0, which stores vtable 0x01079924. The first keeps row 0 and
has center/bottom alignment byte pair (1,2). The second sets row 2
at 0x0069E075 and uses center/top (1,0). A ContentPresenter at D+0x36C
is in row 1 at 0x0069E06D with horizontal and vertical content alignment
both byte 3 at 0x0069E04D..0x0069E065. The property names are
HorizontalContentAlignment and VerticalContentAlignment at
0x0107A7DC/0x0107A7F8. The constructor's Content string request at
0x0069E0A5 produces a child which the subsequent cast identifies as
ContentLabel (RTTI descriptor 0x012BD09C). After a successful cast it
sets that child's style group Label and its style NIB_basis_digit
(0x0069E0FE..0x0069E15D), and sets both alignments to byte 3.

The rebuild associates effective Value descriptor 0x0135ADF4 with a
binding at 0x0069E729..0x0069E750. The constructor installed a
Sqwt.Data.DigitConverter at C+0x38FC (0x0069D24F..0x0069D255,
0x0069EC30, type name 0x00FCB14C). The rebuild formats a per-digit place
parameter starting at 1 and multiplying by 10 at 0x0069E7D7..0x0069E7E5.
At 0x0069E7BE..0x0069E7C5 it attaches the binding to Content descriptor
0x01358F94 (name at 0x0106B16C). Converter vtable 0x00FCB104 has entry
0x0069ECF0 at +0x38. That method reads the source and parameter as
integers, computes signed (Value / place) % 10 at 0x0069ED8B..0x0069ED94
for nonzero place, and formats the result through 0x00988A90 at
0x0069EDA4. The same place reaches the step-command builder 0x0069C310
at 0x0069E7D2. The stock arrows therefore step by +/- the selected place.

Constructor sets D+0x1240 to zero at 0x0069DD30. On the ordinary
content change path, 0x0069C950 replaces exact text "0" with a space
while that flag is clear. Rebuild calls 0x0069CA50 on the ones place
(0x0069E7E0..0x0069E865), setting the flag and replacing an existing
space with "0". The routed integer ValueChanged adapter 0x00966B70
calls vtable +0x120 at 0x00966B8A..0x00966B9B. NumberInputBox override
0x0069CB20 loads effective Value and calls 0x0069CA50 on higher places
while Value / place is positive. The traced flag writes are sticky
within the current NumberLabel allocation. A higher zero starts blank
and can display zero until the next rebuild after the reveal flag is set.
This is INFERRED from the flag and text writes. ValueChanged reveals the
significant places. The routed RepeatButton MouseDown handler at
0x0069CCA0 also stores the selected digit and calls 0x0069CBA0, which calls
0x0069CA50 on the places from ones through the selection at 0x0069CBEC.
The handler is registered for both arrows at 0x0069DE78..0x0069DE95 with
descriptor 0x0135942C. Physical pointer ingress remains unverified.
The stock hand-in uses nonnegative bounds. Negative display and
overflow parity are outside this slice.

Both arrows begin Hidden at 0x0069E1DA..0x0069E1F2. Rebuild writes
Visibility byte 1 for Hidden or 2 for Visible based on the equality of
the digit pointer with C+0x3B84 (0x0069E9E0..0x0069EA13). It finally
selects vector element zero (ones) at 0x0069EB1A..0x0069EB2A and
refreshes old/new arrows. This defines shown arrows for the selected
place and preserved layout space for unselected ones.

## Style and atlas selection

The property wrapper 0x0092F4C0 writes SqwtStyle descriptor 0x01358958 to
object +0x228. Background style NIB_basis_background is set at
0x0069D628..0x0069D649, max/min at 0x0069D6C3..0x0069D72C, and editor
at 0x0069D958..0x0069D979. RepeatButton ordinarily selects type RepeatButton
(0x00998957, literal 0x01079794). The digit constructor explicitly
overrides both arrows' group to Button (0x0069DF48..0x0069DF6A and
0x0069DFC5..0x0069DFE7), using the same type setter 0x00930BA0.
That setter writes SqwtStyleControlName descriptor 0x01358838 to +0x220
at 0x00930BFC..0x00930C10. The arrow style names follow at
0x0069DF87..0x0069DFA8 and 0x0069E003..0x0069E024. The general style
lookup 0x0092A5F0 passes +0x220 and +0x228 into 0x00934210, which calls
0x00933E70 for the Frame default part. See the [shared style-part resolution](sqwt-tab-scroll-text-parts.md). The explicit Button
override is needed for the arrow lookup.

The four Button Frame styles occupy default.style lines 1361-1370 (Up),
1373-1382 (Down), 1385-1394 (Max), and 1397-1406 (Min). Normal and
Disabled use the _N skin. Hover, Focused and Active use _H, with
SkinOffset 1,1 on Active. Anchored, Reserved and Droppable are NULL.

| Button | Normal source x,y,w,h | Hover source x,y,w,h | HitMargin | Skin lines |
| --- | --- | --- | --- | --- |
| Up | 560,865,20,20 | 500,863,20,20 | 2,4,2,2 | 709-713 |
| Down | 640,862,20,20 | 220,864,20,20 | 2,2,2,4 | 703-707 |
| Max | 706,864,20,20 | 582,865,20,20 | 2 | 715-719 |
| Min | 684,864,20,20 | 662,864,20,20 | 2 | 721-725 |

Each is a 1x1 Stretch CellSkin. The Label digit style at lines 2133-2142
uses NIB_basis_digitSelection in Hover, Active and Anchored and NULL
in the other states. The Label background style at lines 2145-2154
uses NIB_basis_background_N for Normal, Hover, Active, Disabled,
Focused and Anchored and NULL for Reserved and Droppable.
The TextBox style at 5677-5681 points its Background to the Window
NIB_basis_textBox Frame at 243-252, which selects
NIB_basis_textSelection for Normal/Hover/Active/Disabled/Focused.

| Skin | Source x starts / widths | Source y starts / heights | HitMargin | Skin lines |
| --- | --- | --- | --- | --- |
| Digit selection | 28,38,40 / 10,2,10 | 852,864,866 / 12,2,12 | 4 | 406-415 |
| Text selection | 598,610,612 / 12,2,12 | 534,546,548 / 12,2,12 | 4 | 417-426 |
| Background | 754,760,762 / 6,2,6 | 180,186,188 / 6,2,6 | 2 | 428-437 |

All three are 3x3 Stretch grids with SkinBorderThickness 2,0,2,2.

## Measure, arrange and text draw

NumberInputBox vtable 0x00FCB2AC and NumberLabel vtable 0x00FCB574 both
inherit Control measure at slot 55 (0x00937310), arrange at 54
(0x00937480), and background brush draw at 34 (0x00936DB0). The Control
measure/arrange operates on its attached template Grid. Grid vtable
0x01074554 selects measure 0x00976D60 and arrange 0x00978640. There is
no independent numeric-widget coordinate painter in these overrides.
The ordinary text draw comes from the nested ContentLabel, vtable
0x0106B3A4. Its measure 0x00935810 constructs the text run and gets
width/height at 0x009358E9..0x00935915. Draw 0x00935D40 reads the
Content cell +0x33C, skips empty text at 0x00935D8E..0x00935DA6, builds
the run from the Control font aggregate at 0x00935E3E..0x00935E98, and
submits the ordinary draw at 0x00936136 to 0x0098B470, which forwards
to the drawing context. The body reads font colors +0x300/+0x308 at
0x009360B3..0x00936103. This proves the use of ordinary text rendering
for decimal glyphs.

The four button sources are 20x20 texture rectangles. Their exact
destination sizes remain unverified. The remaining
native boundary is the selection and placement of Frame skin cells
after the shared 0x00934210/0x00933E70 lookup. In particular, the
Button/RepeatButton slot-34 0x00936DB0 reads the Background brush.
It does not identify the renderer that consumes the selected Frame
cells. Tracing the selected style results through the scene packet
to the 0x008E0000 family would settle any HitMargin expansion, actual
destination size, and draw order. Runtime opacity, native first-frame
pixels, and output at non-unit scale remain UNKNOWN.
