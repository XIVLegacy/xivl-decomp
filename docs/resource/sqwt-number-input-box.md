# Sqwt NumberInputBox integer range

The EventItemSelectWidget form uses NumberInputBox custom controls for its 16
item quantities. Their signed integer range stores both a requested value
and the current bounded value. An effective change raises ValueChanged.

## Input and method

Evidence IDs NI-FORM-1 (decoded client data), NI-FACTORY-1, NI-RANGE-1, and
NI-EDIT-1 (STATIC). Target is 1.23b build 2012.09.19.0001, PE32 i386, image
base 0x00400000, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
Addresses below are VAs. Observations come from decoded SQWT markup,
read-only PE data, and bounded disassembly. Live behavior is unverified.

## Form and dictionary structure

Decrypt the SQEX payload using the asset basename as Blowfish key, assert
byte-exact re-encryption, and parse namespace-local names and attributes. Only
the three ResourceDictionary.Source paths explicitly imported at form lines
5-7 were opened. None imports another dictionary.

| SQWT path | Installed bytes and SHA-256 | Decoded bytes and SHA-256 |
| --- | --- | --- |
| widget/EventItemSelectWidget.form | 33984, 814733cf4cecf378226de4714b338e1bfa0889845e580dcb8461a3bd03813e0d | 33976, 00b954b0d754c87ab32096d4b96a98e516b7463526982246c0aedb7bb459b459 |
| common/WINBasis.tpl | 6278, 27d03aca012d1554a3cf8ecd1b5e8da22cc7c815c6f92a1d6d0557dad3bcc5aa | 6270, d3227d9200d301ba715250e393d8e09c0ca2d39080754e1ed517692fa7d50700 |
| common/LABBasisNumberInputBox.tpl | 1059, 91c6aec1f2c3445faad6570d934eb4392afcd0aabdff36ae8cfbc69a48d269af | 1051, 546672a02cb526a810bbe191a574832499b3208ff294df5940c184fb3e729922 |
| widget/EventItemSelectWidget.tpl | 855, 43d52789842ccb2d0f83f7b84e5813e9cf42a46b83b298c9e3edd4608b7252cb | 847, 7e1862c15d0a44ce74026eec07ab45272284050c719bf4da30125e7fd9015ce2 |

All 16 row grids contain a Label with selected template
ControlTemplate_Label_NumberInputBox and style Style_Label_NumberInput,
then one CustomControl child with Type NumberInputBox. The Label is named
Label_N, and the child CustomControl_NumberInput_N, for N 1 through 16.
Rows repeat every 19 lines, with the first Label/CustomControl at 50/51 and
the last at 335/336. There are zero literal Slider elements. Each row's
hit-area Label also selects Style_Label_NumberInput, for 32 total uses.
The widget dictionary declares this as a Label style at line 2. Lines 4-14
map MouseEnter/GotAnchor to UILuaCommands.ButtonFocused and
MouseLeave/LostAnchor to UILuaCommands.LostFocus.

The common numeric dictionary declares ControlTemplate_Label_NumberInputBox
with target Label at line 2. Lines 3-7 contain a ContentPresenter and a
ValueChanged EventTrigger, with command NumberInputBox.ValueChanged,
one integer from PropertyName1=Value, and a sound action. Separately
lines 11-15 declare Style_NumberInputBox. The Style at line 11 sets
TargetType to `{x:Type Slider}`, names the style through x:Key, and carries
an empty SqwtIsAutoSetSkin attribute. Its only direct Setter child, at
line 12, sets Property FontSize to Value 14. FontSize is absent from the
Style attributes. The Style.Triggers entry opens at line 13. Its
ValueChanged EventTrigger specifies SourceType Slider at line 14 and
executes NumberInputBox.ValueChanged at line 15 with an Int parameter
from PropertyName1 Value. The selected form contains zero references
to that Slider style. WINBasis declares the Window-target template
ControlTemplate_Window_WINBasis at line 2.

## Custom control factory

At 0x00549735..0x00549769 the custom factory compares the type name with
NumberInputBox (string 0x00FA159C), allocates 0x3B88 and calls 0x0069D140.
The matching alternate factory at 0x0054CFBA..0x0054CFCF calls 0x005552D0,
which allocates the same size and calls the same constructor. The
0x00546838..0x00546857 branch returns 0x3B88 for that type. Constructor
0x0069D173 calls 0x009985A0, which installs SliderTemplate<int> vtable
0x01079454 and calls 0x009679E0 at 0x009985CC. The latter installs
RangeBase<int> vtable 0x010701DC. The most-derived constructor installs
NumberInputBox vtable 0x00FCB2AC at 0x0069D187 and sets class descriptor
0x013467C4 at 0x0069D2FE, constructed from its name at 0x0069D2D9..0x0069D2E4.
Class names resolve through the [RTTI catalog](../../config/ffxivgame.rtti.json).

## Range properties and event

The constructor installs integer dependency-property info 0x0135AE70 at
0x00967B09..0x00967B1A. Its initializer 0x00F240F0 installs the getter
0x009670C0 and string setter 0x00967750. Vtable 0x01070320 slot 3 is
0x0091B440, which calls the setter pointer at info +0x28. The generic
property engine reaches this slot at 0x00912A47..0x00912A55 when the
control's virtual property hook declines. The property name metadata
table is at 0x012E3CA4. Constructor and typed setters associate these
signed integer cells.

| Name | Slot | Descriptor | Default | Object offset |
| --- | --- | --- | --- | --- |
| Maximum | 1 | 0x0135ADC4 | 1 | +0x344 |
| Minimum | 2 | 0x0135ADA0 | 0 | +0x34C |
| SmallChange | 3 | 0x0135AD7C | 0 at RangeBase, 1 at NumberInputBox | +0x354 |
| Value | 4 | 0x0135ADF4 | 0 | +0x35C |

The [shared property route](sqwt-anime-property.md#route-to-the-setter)
reaches the form setter.
Here, 0x00967796, 0x009677C0 and 0x00967814 select Maximum, Minimum and Value
respectively, then call 0x0095CB50. On the ordinary text path that calls
0x00988A30, which reaches strtol base 0 at 0x00988A7F after preprocessing.
It then calls the generic integer writer 0x00926E80. Writes rejected by
cell-state flags end early. An accepted write equal to the current cell
also skips the callback (0x00926ED3..0x00926EF9). The callback
0x009673E0 reads the property slot from its change record. Slots 1 and 2
recalculate, while slot 4 first copies the new request from +0x35C to
+0x370 (0x0096740C..0x00967418). At 0x009671AD..0x009671CF, signed
comparisons cap the request to Maximum, then floor it to Minimum. This
order produces Minimum for inverted bounds. The separate request survives
a clamp and is reused when either bound changes. The ordinary local path
sets the effective Value at +0x35C. The compare at 0x00967215 against prior
effective value +0x36C gates the routed event. On this path the Value
property callback runs before the producer reads and updates the event
snapshot. It places new and old integers in the event and raises
descriptor 0x0135ADE8 from receiver +0xB4 through 0x0091F1E0
(0x00967264..0x009672C8).
Initializer 0x00F23F70 names that event ValueChanged. The routed event and
the markup command NumberInputBox.ValueChanged are distinct descriptors.

## Step commands and text entry

NumberInputBox event override 0x0069C5E0 calls SliderTemplate<int> handler
0x009970A0 before handling the text editor's commands. The integer Slider
handler first lets the handler store process the routed event. The step
branch needs the handled byte at event +0x10 clear on entry. A recognized
step sets event +0x19 to one and the value write path tests that the
handled byte is one.
At 0x0099711B..0x00997129 Slider.AddCommandParameter uses the current
effective count plus signed integer 0 from event +0x7C. At
0x00997153..0x00997169 the result is quantized using signed division and
multiplication by SmallChange before typed Value setter 0x00967090, which
also uses 0x00926E80 and callback 0x009673E0. The constructor calls the
SmallChange setter 0x00967720 with 1 at 0x0069D6AF..0x0069D6B7.
The custom handler's conditional swallow of certain negative steps and
creation of positive and negative step records are covered in
[other consumers of the record](sqwt-command-parameter-consumer.md#other-consumers-of-the-record).
The constructor also assigns Slider.MaximizeValue and Slider.MinimizeValue to two separate buttons at 0x0069D7F1..0x0069D80C.
In the text editor's ApplicationCommands.Operate branch at
0x0069C622..0x0069C687 (command 0x01357C88 from 0x00F2149C), the handler
calls the same integer converter 0x00988A30 and then typed Value setter
0x00967090. The keyboard-to-editor focus sequence, full bindings,
pointer geometry, skin drawing and sound playback were not traced.

The [EventItemSelectWidget script contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/event-item-select-widget-contract.md)
covers the script writes to Minimum, Maximum and Value and consumption of
NumberInputBox.ValueChanged.
