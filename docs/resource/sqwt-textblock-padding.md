# Sqwt TextBlock padding in EquipWidget

TextBlock adds the scaled Control padding and border to its measured size.
Its text placement uses the same horizontal insets and adds the top inset
to the line position. The EquipWidget repair amounts use a right inset of
two pixels at scale one.

## Input and method

Evidence IDs EQ-PAD-DATA (decoded client data), EQ-PAD-CONTROL and
EQ-PAD-DRAW (STATIC), EQ-SKIN-FIELDS (STATIC). This receipt covers the
1.23b executable (build 2012.09.19.0001, PE32 i386, base 0x00400000,
15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9).
Addresses are VAs. Evidence comes from decoded SQWT metadata, read-only PE
data and bounded disassembly. Live appearance is unverified.

## Equipment amount controls

Read each eight-byte SQEX header and decrypt its payload with Blowfish
keyed by the basename. Verify byte-exact re-encryption and parse only
namespace-local metadata with DOCTYPE refused. The three dictionaries
are the form's ResourceDictionary.Source values at lines 20-22. The
window selects default.style at line 1, which names default.skin at
its own line 1. No other resource was opened.

| SQWT path | Installed bytes and SHA-256 | Decoded bytes and SHA-256 |
| --- | --- | --- |
| widget/EquipWidget.form | 89421, 2010deb69a2792dbe73be410500492c7c508eae2740b9db28f0eb1315d552bb9 | 89413, 4ce68aef535ab53ff1311e5dc109d10b1e90558ae75b914aba2f26691bf00bd8 |
| common/WINBasisLight.tpl | 6233, bc50dbd15b1dee7175845c97628909b0d81a9cbda056e831786c0537e3eca939 | 6225, 6459bb0150901966e65c72fdadfb8074bc10b0613ccf230d50840f05ce23da03 |
| widget/EquipWidget.tpl | 4831, a145e3147c423c771a9e6d8122169fa9316ebd70d0077037fc3fc4fc012ed266 | 4823, 1ca0e91d04372a2a81b7ea6116f52cebcd823d70ef49e3ea2b21745afc2d775b |
| widget/EquipItemListWidget.tpl | 15765, b42d268934e275c3242ec31f879617484a63b47a9a6104055f66a34285ee4a2a | 15757, ee6b5688a6cc67f1d623ac25cb91a01de371894df732709b0320483daf562461 |
| common/default.style | 353783, 632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0 | 353775, 33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6 |
| common/default.skin | 141967, 6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640 | 141959, 929e65e9d5f191292d9b044e3e659ba115f18f6b3d8a82894046a7c75cbcad5a |

Two of 77 TextBlocks in the form declare Padding, and none of the TextBlocks
in its three dictionaries declares it. The two are TextBlock_RepairEquipCost
(line 800, SqwtStyle TBL_basis_moneyBackground) and TextBlock_CurrentMoney
(line 812, SqwtStyle TBL_basis_moneyBackground_green). Each sets
Padding=0,0,2,0, Width=92, Margin=2,0,0,0, TextAlignment=Right,
HorizontalAlignment=Left, VerticalAlignment=Center, and Grid.Column=1.
Each omits BorderThickness and TextWrapping. Both are inside the
Grid_RepairEquipCost descendant of Grid_RepairEquipment, whose own
Visibility at line 627 is Collapsed. The two amount rows are observable
when the repair grid is shown.

The TextBlock style entries at default.style lines 3210-3219 and 3246-3255
both select Frame skin BOD_basis_moneyBackground_bg in SkinNormal,
SkinHover, SkinActive, SkinDisabled and SkinFocused, then name NULL
in SkinAnchored, SkinReserved and SkinDroppable. For the five named-skin
states, font colors are #fff3f3f3 (disabled #80f3f3f3) for the repair
amount and #ff99ffb3 (disabled #8099ffb3) for current money. Their edge
color is #ff262626 (disabled #801a1a1a). The referenced CellSkin is
default.skin lines 1388-1397. It declares 3 by 3 cells, no HitMargin
or SkinMargin, and SkinBorderThickness=2,0,2,2. All nine cells are Stretch.
In record order the source x starts are 754, 758, 760 with widths 4, 2, 4.
The y starts are 196, 200, 202 with heights 4, 2, 4.

## Control padding property

Padding is a Control property inherited by TextBlock. Initializer
0x00F223A0 associates descriptor 0x01359220 with literal Padding at
0x0106B8D4 and writes descriptor index 2. Control construction at
0x00938CCB..0x00938CDF passes descriptor, callback 0x00937A70 and receiver
C+0x2A4. The Control property setter path 0x00938282..0x009382A3
supplies the same offset and descriptor to 0x00939340. Its ordinary
text path calls 0x009158A0 at 0x00939396, then 0x00931D70 at 0x009393BB.
The parser 0x009158A0..0x00915AAC follows comma separators and returns
four floats. The four-value path overwrites the second through fourth
outputs from the second, third and fourth numeric tokens. No percent
conversion applies to the two selected literals. Full generic parser
admission and malformed values are out of scope.

The [Control layout rules](sqwt-tab-scroll-text-parts.md#layout-rules)
record the inset readers and scale.
0x009371B0 reads C+0x2A4/+0x2A8/+0x2AC/+0x2B0, multiplies each component
by the virtual scale, calls 0x009D6600, and converts each truncated
integer back to float before storing the L/T/R/B tuple. The separate
BorderThickness reader is 0x00937260. In the formulas below, I is the
component-wise sum of scaled Control Padding and scaled Control
BorderThickness.

## TextBlock measure and draw

The tracked TextBlock primary vtable 0x0106E844 points to 0x00955990
in slot 34, 0x00955350 in slot 54 and 0x00955EF0 in slot 55.
The arrangement body 0x00955350..0x00955362 copies the supplied final
size. The measure body 0x00955EF0 formats and measures text before
reading the two Control inset tuples at 0x00956143 and 0x00956151.
At 0x00956156..0x009561BA it adds I.left+I.right to measured text width
and I.top+I.bottom to measured text height, and returns those dimensions.
The measured width input is available width on the wrapping branch
(0x00955F4E..0x00955F55), with 10000000 on the non-wrapping branch
(0x00955F46, float at 0x0106E544). The insets are added after that
measurement. This is TextBlock's override, distinct from Control's
template-child measurement.

The draw body 0x00955990 first calls the Control background-brush path
0x00936DB0 at 0x009559D9. The draw then calls BorderThickness and Padding
readers at 0x009559E5 and 0x009559F3. Instructions 0x009559F8..0x00955A2E
compute I.left, I.top, I.right, and W_inner = W_arranged - I.left - I.right.
The arranged width is returned via 0x00939880 from C+0x178. The right
alignment enum is 1 (string 0x01068B40 and table entry 0x012DFF18).
TextAlignment descriptor 0x0135A348 is installed on C+0x344 in the
constructor and read at 0x00955B0C. The ordinary single-run alignment path
computes right-aligned offset W_inner - runWidth at 0x00955DFE..0x00955E00
and adds I.left at 0x00955E0A..0x00955E14. For left alignment its offset
stays zero. Values 2/3 on this path multiply the remainder by 0.5
(0x00955DF0..0x00955DF8). The y submission adds I.top to the current line
position at 0x00955E29..0x00955E3C. The adjusted pair reaches the
DrawingToolContext draw call at 0x00955E4B. Bottom inset contributes to
desired height but is not read by this draw path. The truncated/ellipsis
branch uses its own width budget from the same W_inner, at
0x00955C51..0x00955C95.

For the selected amount controls at scale 1, assuming 92 actual arranged
pixels and zero separate Control BorderThickness, W_inner is 90 and the
normal right-aligned run ends 2 pixels left of the right edge of the
TextBlock. Text measurement gains 2 pixels in desired width. Margin and
the skin's SkinBorderThickness are separate inputs. This is INFERRED
from the asset and instruction observations. Runtime layout position,
actual bounds under compression and original screenshots remain UNKNOWN.

## Separate skin properties

SkinBorderThickness and HitMargin are distinct from Control Padding.
The file-backed skin parser 0x009A7B00 has independent lookups for
HitMargin (literal 0x0107C514, instruction 0x009A7B63), SkinMargin
(0x0107C520, instruction 0x009A7D52) and SkinBorderThickness (0x0107C52C,
instruction 0x009A7F56). The staging sequence 0x009A8131..0x009A81A2 and
copy 0x009A0E00..0x009A0E12 put the four HitMargin values in record
+0x04..+0x10, SkinBorderThickness in +0x14..+0x20, and SkinMargin
left/top/horizontal-sum/vertical-sum in +0x24..+0x30. The selected
SkinBorderThickness input maps to (2,0,2,2) in that second tuple.
The [NpcSay backdrop](npcsay-backdrop-draw.md#open-items) also identifies
0x008E13A0. Its first four floats come from the skin record's HitMargin
tuple (copy at 0x008E13F2..0x008E1412), and the pre-clamp rectangle
calculation at 0x008E1414..0x008E148D subtracts left/top from x/y and adds
left+right/top+bottom to width/height. Whether this helper is used by
the selected TextBlock's frame remains UNKNOWN. The frame submission
route and nine-cell destination geometry are outside this trace.
