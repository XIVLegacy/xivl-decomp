# Sqwt render transform

A UI element holds a `RenderTransform` pointer, a `RenderTransformOrigin` pair
and a `RenderSize` pair. The markup forms that name a `ScaleTransform` write
one cached default transform of the element, whose centre is the origin times
the render size and whose factors default to one. The markup attributes
`CenterX` and `CenterY` have no writer. This page records those joins, the
matrix the transform builds, and the constructed values of the `IsAnchorWithin`
and `IsMouseOver` flags. It does not establish how a drawn element composes the
matrix with its layout offset, whether the matrix reaches hit testing, or when
the `IsAnchorWithin` value becomes true.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, cross-reference and string queries, decompilation as a lead, a PE byte search for string literals |

All addresses are VA. Subtract `0x00400000` for RVA. The executable size and
SHA-256 were checked before the observations. Every instruction-level claim was
read from the disassembly. This was static analysis, without client execution.

Evidence labels used below:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

## UIElement members

The `UIElement` constructor `0x0093C750` initializes three members through
property-slot constructors. Each property descriptor is registered by a
static-initializer thunk that pushes the name literal, calls `0x009108E0` on
the descriptor and stores a small property index.

| Property | Name literal | Descriptor | Thunk | Index | Member | Default |
|---|---|---|---|---|---|---|
| `RenderTransform` | `0x0106BF30` | `0x01359584` | `0x00F22A30` | `0x08` at `0x01359598` | element `+0x138`, one pointer, constructed at `0x0093C82A` | null pointer |
| `RenderTransformOrigin` | `0x0106BF40` | `0x013593FC` | `0x00F22A50` | `0x0B` at `0x01359410` | element `+0x140`, two floats, constructed at `0x0093C860` | `0.0`, `0.0` |
| `RenderSize` | `0x0106BF58` | `0x013595C0` | `0x00F22A70` | `0x0D` at `0x013595D4` | element `+0x14C`, two floats, constructed at `0x0093C88A` | `0.0`, `0.0` |

The defaults are the `FLDZ` at `0x0093C835`, which feeds the origin pair
constructed at `0x0093C860`, the `FLDZ` at `0x0093C865`, which feeds the render
size pair constructed at `0x0093C88A`, and the zero push that precedes the
`RenderTransform` constructor call. STATIC. The origin is a pair of floats with no
unit conversion in its setter thunk `0x0093D23D..0x0093D243`, which passes the
pair to the generic float-pair store `0x0093E480` at member `+0x140`. STATIC.

## ScaleTransform object

The constructor `0x00990AC0` of `Sqwt::Media::ScaleTransform` (vtable
`0x01078148`, RTTI `.?AVScaleTransform@Media@Sqwt@@` at `0x012E6604`) writes
the layout below. The object is `0x20` bytes (the allocation in `0x00939CB0`).

| Offset | Content | Constructor value |
|---|---|---|
| `+0x08` | cached matrix pointer | null |
| `+0x0C` | ScaleX | `1.0` |
| `+0x10` | ScaleY | `1.0` |
| `+0x14` | CenterX | `0.0` (the float at `0x00FB7A64`) |
| `+0x18` | CenterY | `0.0` |
| `+0x1C` | matrix-valid byte | `1` |

STATIC. The four setters `0x00990B10` (ScaleX), `0x00990B40` (ScaleY),
`0x00990B70` (CenterX) and `0x00990BA0` (CenterY) each compare the incoming
float with the stored one, store it when it differs and clear the valid byte.
STATIC. A transform no setter changed reports the valid byte set with a null
matrix pointer, and the matrix getter returns the shared object whose address
is stored at `0x012E6594` (INFERRED to be the identity matrix, since the
default factors are one and the centre is zero). The `RotateTransform`
constructor is `0x00990BD0` and its angle setter is `0x00990C10`.

An absent `ScaleX` or `ScaleY` therefore reads as `1.0` and an absent centre as
`0.0`. STATIC.

## Matrix

The matrix getter is vtable slot 3 of `Sqwt::Media::ScaleTransform`, function
`0x00990CA0`. When the valid byte is clear it allocates the cached matrix
through `0x0067AB60`, builds three matrices and multiplies them in this order.

1. A translation by (CenterX, CenterY, 0) through `0x00430430` is the base
   (`0x00990D16`).
2. A scale by (ScaleX, ScaleY, 1) through `0x00430350` is multiplied in
   (`0x00990D7E`, `0x00990D91`).
3. A translation by (-CenterX, -CenterY, 0) is multiplied in (`0x00990DCA`,
   `0x00990DDD`).

The multiply `0x0042EDB0` with `this` equal to the cached matrix computes, for
each row `r` of its argument, `r * this` through `0x0042EC50`, whose body forms
`v.x*row0 + v.y*row1 + v.z*row2 + v.w*row3` of `this` (`0x0042ECBF..0x0042ECFE`).
The result is written back to the cached matrix by `0x00419B60`. The
translation builder `0x00430430` stores the offset in the fourth row. STATIC for
each step. The product is `T(-C) * S * T(C)` in the row-vector convention the
fourth-row translation implies, which scales about the point (CenterX,
CenterY). INFERRED from the convention.

## Default transform of an element

The element caches one `ScaleTransform` at `+0x188` and one `RotateTransform`
at `+0x18C`. The creator `0x00939CB0` allocates a `ScaleTransform` when the
cache is null, then sets its centre to `RenderSize * RenderTransformOrigin`:
CenterX from `0x00990B70` with `[element+0x14C] * [element+0x140]` and CenterY
from `0x00990BA0` with `[element+0x150] * [element+0x144]`
(`0x00939D2A`, `0x00939D4D`). The `RotateTransform` creator `0x00939D70` does
the same with `0x00990C40` and `0x00990C70`. STATIC.

The origin is therefore a fraction of the render size, applied as a multiplier
of the size pair, and the centre is in the units of `RenderSize`. STATIC.

The origin notification uses descriptor tag `0x0B`. Callback table cell
`0x0093BD64` selects `0x0093BAE2`, which multiplies the same pair into the
centres of any existing cached transforms. The load and test at
`0x0093BB76..0x0093BB7E` skip visual application when the own transform
pointer at `+0x138` is null. Otherwise the call at `0x0093BB8B` reapplies
that pointer through `0x00940260`. A separate centre update at
`0x0093BBFF` also uses the render size pair. STATIC for these paths.

## Markup routes

| Route | Evidence |
|---|---|
| The string value `ScaleTransform` written to `RenderTransform` selects the element's cached transform. The value test is `0x00445D60` against the literal at `0x0106C218` (`0x0093D1D2..0x0093D1D9`), then `0x00939CB0` and the `RenderTransform` setter `0x0093C540` with the cached object (`0x0093D1E4..0x0093D1F0`). The literal `RotateTransform` at `0x0106C228` selects the rotate cache the same way | STATIC |
| An element whose name ends in `.RenderTransform` (the suffix literal `0x0106D4F4`, tested at `0x0094BB3F..0x0094BB56` in `0x0094AD90`) with a non-null owner constructs a `Sqwt::Markup::RenderTransformMarkupObject` (vtable `0x0106DBCC`) through `0x0094F530`. Its slot 2 `0x0094F750` writes its input value to the `RenderTransform` descriptor through the generic setter `0x00912850`, then constructs a `TransformMarkupObject` (`0x0094F5B0`) that holds the owner element at `+0x74` | STATIC |
| An attribute of that child, such as `ScaleX`, goes to slot 3 of the `TransformMarkupObject` vtable `0x0106DBF0`, the handler at `0x0094F640` (the data word `0x0106DBFC`). The `MarkupObject` base takes its attribute handler from the same slot (see [Sqwt qualified property lookup](sqwt-qualified-property-lookup.md)). It prefixes the attribute name with `RenderTransform.` (literal `0x0106DC10`) and calls the generic setter on the owner element | STATIC |
| The qualified names `RenderTransform.ScaleX`, `RenderTransform.ScaleY` and `RenderTransform.AngleZ` (descriptors `0x0135FD88`, `0x0135FDD0`, `0x0135FE18`, see [Sqwt qualified property lookup](sqwt-qualified-property-lookup.md)) are handled by `0x0093BD80`. For ScaleX it reads the value, calls `0x00939CB0` for the cached transform and writes `0x00990B10`. For ScaleY it writes `0x00990B40`. For AngleZ it calls `0x00939D70` and writes `0x00990C10`. The three writes are the calls at `0x0093BE44`, `0x0093BF66` and `0x0093BFD8`, and each joins at `0x0093BE49`, which sets a local flag, then reaches the tail at `0x0093BE4E`. The tail re-applies `+0x138` through `0x00940260` (the call at `0x0093BE7D`) when the flag is set and the pointer is non-null (`0x0093BE65..0x0093BE74`) | STATIC |
| `CenterX` and `CenterY` have no writer. The executable holds no `CenterX` or `CenterY` literal, ASCII or UTF-16, by a byte search over the whole image, and `ScaleX`, `ScaleY` and `AngleZ` occur only inside the three qualified literals. The two `ScaleTransform` centre setters `0x00990B70` and `0x00990BA0` have direct callers only at `0x00939D2A` and `0x00939D4D` in `0x00939CB0` and in the size block (`0x0093BB04`, `0x0093BB27`, `0x0093BC24`, `0x0093BC47`). The two `RotateTransform` centre setters `0x00990C40` and `0x00990C70` have those same kinds of callers (`0x00939DEA` and `0x00939E0D` in `0x00939D70`, `0x0093BB4E`, `0x0093BB71`, `0x0093BC6E`, `0x0093BC91`) and also direct callers in `0x006759F0` (`0x00675BEB`, `0x00675C11`), `0x006728A0` (`0x0067295A`, `0x0067297E`) and `0x006729A0` (`0x00672A3F`, `0x00672A63`), which this page does not trace | STATIC for the search results. That a markup `CenterX` attribute leaves the centre unchanged is INFERRED from the missing registration, since a key built by concatenation would need the literal |

The authored `CenterX="0.5"` and `CenterY="0.5"` of an installed template
therefore do not set a `ScaleTransform` centre through the markup. The centre of
such a transform is `RenderTransformOrigin * RenderSize`. INFERRED. A
`RotateTransform` centre has the other writers above, so this conclusion
does not extend to it.

## ActionMenu Window origin

The [authenticated ActionMenu inputs](action-menu-timer-icon-style.md#inputs-and-method)
declare `RenderTransformOrigin` `(0.5, 1)` on the Window at form line 1.
Form lines 9 through 13 select `ControlTemplate_Window_ActionMenu` and
`Style_ActionBarWindow`. The root, selected template and selected style
declare no own transform assignment. The template's Shown, Closing and
Hiding origin tracks at lines 13 through 14, 27 through 28 and 43
through 44 supply `(0.5, 0.5)`. The selected Window style's tracks
target named border opacity. These are declaration observations.

STATIC. The base constructor initializes the own transform pointer to
null at `0x0093C819..0x0093C830`. The literal route at
`0x0093D222..0x0093D249` retains the origin pair. On the null pointer
branch of the origin notification, the retained pair has no own
visual matrix effect. This does not establish the Window's live
pointer, ancestor composition or trigger delivery.

### Arranged position in draw and hit testing

STATIC. The [Window arrange path](action-menu-timer-icon-style.md#window-top-offset-and-arrange)
can store its adjusted position in `VisualOffset`, the binary32 pair
at Visual `+0x24/+0x28`. The changed callback at `0x0093FE10..0x0093FE54`
can pass the retained pair to current drawing context slot `+0x34`.
First context acquisition can replay a dirty pair at
`0x0093F8E9..0x0093F8FE`. These instruction and virtual table joins
were checked against the PE with LLVM x86-32 decoding.

The draw proxy `0x0055A000` truncates each finite in-range local
component toward zero and stages it as binary32 for operation `0x0E`.
Admitted delivery stores local XYZ in WindowContext at `+0x28..+0x30`.
The additive path at `0x008CDA9E..0x008CDB0B` can accumulate parent
position. The matrix path at `0x008CD90D/0x008CDA43` can compose
`M_visual * T_context * M_parent` in row-vector order. Clip delivery
and inherited context clipping remain separate inputs. A selected
ContentPresenter with `ClipToBounds=False` can still inherit a clip.

The admitted ordinary Window point route
`0x00923090 -> 0x00939B10` subtracts each current UI-side binary32
offset along visual parent `+0xA4`. It can visit visual children
before the Window self test. The propagated clip test at
`0x0097E0EC..0x0097E13E` and the Window's local rectangle test
at `0x00926D40` use ordered half-open bounds. The Window callback
`0x0092B640` has separate logical Visibility and local hit gates.
[Child-first query](sqwt-command-parameter-consumer.md#local-hit-flags-and-child-first-query).

The draw readiness value `+0x128 == 3` at `0x008CE7FF` can
arise from an update and a generic command. The visibility byte
`+0x12B` starts true. Neither field certifies delivery of the
current position. The same arranged origin can therefore reach
input as a fractional value while drawing uses an integer-staged
or earlier queued value. The live Window selection, first frame,
effective clip and final hit remain open.

## PlayerParameter Window origin

OBSERVED markup. The PlayerParameter Window selects an origin-animation
template without declaring an own RenderTransform assignment. Its selected
targeting-label template has a separate ScaleTransform Setter. This finding
bounds those declarations and the conditional native null-pointer path above.

Paths are relative to `client/sqwt`. The first three identities agree with the
[PlayerParameter structure record](https://github.com/XIVLegacy/xivl-client-data/blob/3fdca48f2ab6d61187b10a1e04f02326661270f7/docs/sqwt-player-parameter-structure.md#source-and-extraction).

| Input | Container bytes | Container SHA-256 | Decoded bytes | Decoded SHA-256 |
| --- | --- | --- | --- | --- |
| `widget/PlayerParameterWidget.form` | 5987 | `602cefe3489630240466d9d3dab9484bd59f3dbefaa33a50534f7b65ea5c5b2a` | 5979 | `88c37e2cc1b7ac5e83f24349dfeeedbdd148a44beaefc41015296ad24a5a8237` |
| `widget/PlayerParameterWidget.tpl` | 2514 | `81155d689fc5dfe64c348487b9e63470a19e342ba25bf00a6db433672a454c91` | 2506 | `ca8698a204a642943db9cb6f8757e387bc29c664ff7779734b6fa2e405fcafce` |
| `common/WINBasisLight_noGrayout.tpl` | 4303 | `be271e92c52765dd1cfd8c786c0f77144440520b84239f08db5b0c21c97c8949` | 4295 | `47271a67f14e47e0fb458e1a211de9366b17c112576e951668bd519e116ee6d8` |
| `common/default.style` | 353783 | `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` | 353775 | `33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6` |

`xivl-tools` revision `f2a9a2b5a6a4c450458db7a493c25fcdcd70aca0`
produced the decoded documents with `extract-resource --as sqwt`. The form
and both templates passed fresh exact `verify-extraction --source` replay.
The retained style extraction has a passing exact source-replay receipt. Its
file seals and original identity were rechecked. Python Expat supplies the
decoded locations. Preserve each original basename in this recipe:

```sh
xivl inspect <original-named-file> --as sqwt
xivl extract-resource <original-named-file> --output <new-private-output> --as sqwt --format json
xivl verify-extraction <new-private-output> --source <original-named-file> --report json
```

Form line 1 names Window_PlayerParameterWidget and WIN_null. Lines 5 and 6
merge the identified common and widget dictionaries. Lines 10 through 12
select ControlTemplate_Window_WINBasisLight. The complete form declares no
RenderTransformOrigin or RenderTransform attribute, transform slot, transform
binding or trigger. The selected common Window template at lines 2 through
73 has Grid_Content and ContentPresenter at lines 3 and 4, then SqwtTopOffset
and SqwtLeftOffset setters at lines 6 and 7. It declares no own transform
literal, Setter, slot or binding.

| Common-template event | Ordered actions | Origin track and keyframe |
| --- | --- | --- |
| Shown at line 9 | Stop ClosingAnimation, stop HidingAnimation, begin OpeningAnimation at lines 10 through 12 | PropertyAnimationUsingKeyFrames at lines 14 through 16, Value `0.5,0.5` at line 15 |
| Closing at line 23 | Stop OpeningAnimation, stop HidingAnimation, begin ClosingAnimation at lines 24 through 26 | PropertyAnimationUsingKeyFrames at lines 28 through 30, Value `0.5,0.5` at line 29 |
| Hiding at line 37 | Stop OpeningAnimation, stop ClosingAnimation, begin HidingAnimation at lines 38 through 40 | PropertyAnimationUsingKeyFrames at lines 42 through 44, Value `0.5,0.5` at line 43 |

Each origin track declares only Storyboard.TargetProperty RenderTransformOrigin,
and its single DiscreteValueKeyFrame declares only Value `0.5,0.5`. These
track/keyframe pairs omit BeginTime, Duration and KeyTime. The selected Window
trigger descendants have no TargetName, Storyboard.TargetName or binding-shaped
attribute. The origin property is explicit, while its native recipient remains
unresolved. The remaining Window animation properties are VisualOpacity,
SqwtTopOffset and SqwtLeftOffset.

Form line 14 declares Label_TargetingMember with SqwtStyle
`LAB_basis_unitSelected` and VisualOpacity 0. It selects
ControlTemplate_Label_TargetEffect. That template at widget-template lines
14 through 40 has the separate RenderTransform Setter ScaleTransform at line
15. Its TargetEffectOn and TargetEffectOff command triggers at lines 17 and
28 first stop the opposite named storyboard, then begin their own. Their
origin tracks at lines 21 through 23 and 32 through 34 declare BeginTime
`0:0:0` and keyframe Value `0.5,0.5`, with no Duration or KeyTime. They also
omit TargetName and Storyboard.TargetName. These are the only two other
origin tracks in the identified form and dictionaries.

The complete form, both complete dictionaries and the 12 directly named
style subtrees plus their eight distinct explicit Window part-reference
subtrees declare no ScaleX, ScaleY or AngleZ literal, slot, binding or
animation. The Window WIN_null style at lines 15 through 26 has no transform
or origin declaration. None of the other selected style subtrees adds one.
The style root names common/default.skin, whose contents are outside this
finding. Omitted explicit styles do not establish native default selections.

STATIC from the constructor and notification paths above. If an admitted
origin write reaches an element while its own transform pointer at +0x138
is null, the origin is retained and this notification skips own visual
reapplication. The selected PlayerParameter Window origin tracks and absence of a
local own-transform assignment supply a finite asset candidate for this
conditional branch. They do not prove the unnamed track's recipient or its
live pointer. The targeting Label's separate Setter cannot establish the
Window's pointer.

The missing runtime facts are the resolved origin-write recipient, its own
transform pointer at notification, and any intervening writer of that pointer
or origin. Template activation, property admission, trigger delivery, later
script or native writes, effective ancestor composition, clock execution,
hit testing and rendered state remain UNKNOWN. The declarations do not
identify a particular interaction as an origin-change producer.

## Applying the transform to the visual

`0x00940260` forwards to `0x00941290` with the descriptor `0x01359868`, which is
the `VisualTransform` property (name literal at `0x0106C4CC`, registered by the
thunk at `0x00F23290`) at `Visual +0x38`. `0x00941290` calls the transform's
vtable slot 3 (`0x00941290..` `CALL [vtable+0xC]`), compares the returned matrix
with the stored one through `0x0098FBB0`, and when they differ copies it into
the property's matrix through `0x0098F8D0`. STATIC. The element's transform is
thus held as a matrix on the visual.

## ConsoleIconTray push icon descendant draw

The [authenticated Tray carriers](sqwt-icon-control-textures.md#direct-consoleicontray-design-data-binding)
declare ten collapsed Push Buttons at form lines 64-73, selecting
ControlTemplate_Button_PushIcon. The unnamed LAB_null Label at
template line 11 has RenderTransformOrigin `0.5,0.5`. Its slot at
lines 13-14 declares ScaleX and ScaleY `1.25` and CenterX and CenterY
`0.5`. The nested IconControl at line 12 declares width and height 24
and VisualOpacity 0. A Button-level transform and the animated Label
sibling at lines 17-20 are separate. Under the checked
[default transform route](#default-transform-of-an-element), the
Label's center is half its own RenderSize on each axis. The icon's
declared dimensions do not specify the Label's RenderSize.

STATIC. LLVM objdump 21.0.0 disassembly and original PE tables
establish a visual matrix transfer. The changed VisualTransform
write can set the dirty bit at `0x0094130B` and copy the matrix at
`0x00941324`. The admitted callback sends Visual+`0x38` to drawing
context `+0x30` at `0x0093FE6A`. Context acquisition also sends
the dirty retained matrix at `0x0093F8D2..0x0093F8E7`.
DrawingToolContext cell `0x00FA2914 -> 0x00559FB0` copies the
nonnull holder matrix to pending state. The publisher submits its
0x40 bytes as local operation 3 at `0x0054EB57`. Admitted
WindowActor dispatch reaches WindowContext at `0x007FA8FF`.
The matrix storage branch writes local matrix `+0x20` through
`0x008CD0F8 -> 0x008CF4F0`.

The nested icon can reach the Label's Content lane. Its markup
passes the icon to parent markup `+0x10` at `0x009468B9`. The
checked Label subobject `+0x194` cell `0x01073778 -> 0x00935160`
calls `0x00934F40` at `0x00935178` when ContentTemplate is null.
That path requests object Content. On the checked changed Content
callback, holder+`0x10` mask 4 at `0x00935AD7` gates the call to
`0x009349B0`. Its empty name resolves to the default Content
name at `0x0092B3BA`. A matching ContentPresenter and the object
flag admit the setter call at `0x009349D0`. With retained content
nonnull and Presenter+`0x2BC` mask 8 clear, the callback can attach
the supplied visual at `0x0099D108 -> 0x00940B30`. The actual
LAB_null presenter and visual ancestry remain conditional. On admitted context parent
insertion, operation `0x21` reaches `0x008CE440`, adding the
child context to the parent's vector and storing parent at child+`0x70`.

WindowContext update `0x008CD700` recomputes the matrix under its
transform-dirty gates. With an incoming parent matrix,
`0x008CD90D` multiplies context translation by that matrix and
stores the result at context+`0x24`. If local matrix `+0x20` is
present, `0x008CDA43` prepends it to that result. With the
[row-vector convention](#matrix), this is
`M_visual * T_context * M_parent` before later prepass changes.
A descendant can inherit the parent matrix without its own local
matrix. The recursion at `0x008CDD49..0x008CDDAE` can pass the
accumulated matrix through the retained child contexts. A renderer
prepass can adjust the matrix before recursion. RenderIcon's
`+0x14` cell `0x010668B8 -> 0x008DF490` returns false for its own
prepass offset, while intermediate renderers remain separate.

On the admitted draw pass, `0x008CE7D0` can traverse those child
contexts at `0x008CEB47..0x008CEB85`. A visited icon with retained
matrix state installs its accumulated `+0x24` through
`0x008CE9F1 -> 0x00CA3B20`. That conditionally flushes the prior mode, sets mode 8
and copies the matrix to `0x013764A8`. The selected
[RenderIcon path](sqwt-icon-control-textures.md#observed-ordinary-control-to-renderer)
requires a drawable context and active icon texture. It calls the
textured quad at `0x008E1D30`. Its setup uses the matrix global at
`0x00CA56A6` or `0x00CA56E0` and supplies four float4 shader
constants through `0x00CA2FC1`. The lower call can reuse matching
constants. This is a static transfer from the Label's matrix to
an admitted descendant draw. Actual presenter selection, local
size, first visible texture, intermediate matrix adjustment,
Measure, Arrange, hit testing and the final screen footprint remain
UNKNOWN.

## IsAnchorWithin and IsMouseOver

Both are boolean properties held in one byte each in the `UIElement` state
block. The constructor clears the low six bits of the byte, then sets bit 1 from
bit 7 and bit 0 from bit 5 of the descriptor metadata word. Bit 5 is the value,
and bits 0 and 1 gate the refresh. STATIC.

| Property | Name literal | Descriptor | Thunk | Index | Metadata word | Byte | Constructed at |
|---|---|---|---|---|---|---|---|
| `IsMouseOver` | `0x0106BF74` | `0x0135968C` | `0x00F22AC0` | `0x0F` at `0x013596A0` | `0x80` at `0x013596A4` | element `+0x15F` | `0x0093CA75..0x0093CAC7` |
| `IsAnchorWithin` | `0x0106BF80` | `0x013594FC` | `0x00F22AF0` | `0x10` at `0x01359510` | `0x80` at `0x01359514` | element `+0x160` | `0x0093CACD..0x0093CB21` |

Both metadata words are `0x80`, so each constructor leaves the value bit clear
and the gate bit 1 set. Both properties are therefore constructed false. STATIC.
The decompiler shows these reads as `param_1 + 0x58` and similar word indices,
which are 4-byte units and not byte offsets.

`IsMouseOver` is set by `0x0093A410`, which sets bit 5 (`OR AL,0x24`) when the
gate bits are nonzero, bit 5 is clear and the word at `+0xC` of its argument is
the element. `0x0093A480` clears bit 5 under the mirror conditions. Both are
entered with the address of the element's `+0xB4` part. What raises those two
calls is not traced. STATIC for the stores.

`IsAnchorWithin` is recomputed by `0x0093D490`. It calls slot `+0x48` of the
table at element `+0xB4` and retains the full returned byte r. After conditional
helper processing, it reloads current state byte b. It stores only when b&3 is
nonzero at `0x0093D4D0` and q=(b>>5)&1 differs from r at `0x0093D4DC`.
The comparison normalizes q, not r. Equality skips the explicit store and
notification, including q=0,r=0 even with nonzero admission bits. Other helper
or callback effects are not excluded.

The changed arm replaces bit 5 with bit 0 of r shifted to that position, sets
mask 4 and calls `0x0099C680` at `0x0093D4FE`. Its immediate ECX is the
field address element `+0x160`. Explicit arguments supply that address, the
complete element, callback `0x0093B700`, zero adjustment, descriptor
`0x013594FC` and notification 1. Helper `0x0099C680` constructs a property
event at `0x0099C6F6` and synchronously calls the supplied callback with the
complete element at `0x0099C714`. This does not guarantee descriptor or state
preservation after callbacks. STATIC.

 `0x0093D510`, `0x0093D5A0` and
`0x0093D630` are three more entries to the same body for a caller that passes the
`+0xB4` part (`LEA EDI,[EBP-0xB4]`), each calling slot `+0x48` through that
part's own table. They continue into `0x0091CA60`, `0x0091CA90` and
`0x0091CCA0`. STATIC.

The `UIElement` constructor stores the table `0x0106C2A4` at element `+0xB4`
(`0x0093C7D7`). Its slot `+0x48` is `0x0091C590`, which returns false unless
`0x0091B7B0` is true, and otherwise tail-calls slot `+0x14` of the table, which
is `0x0091C4B0`. The gate `0x0091B7B0` is true when the global `0x01356F34` is
non-null and its byte `+0x158` is `2`. Slot `+0x14` returns true when the part is
the object held in the global `0x01357020` or an ancestor of it, following the
parent word at `+0x60`, and false otherwise. The global holds the current keyboard-focus interface.
The anchor routed-event recipient is stored separately at `0x01356FCC`.
Window interface table `0x01069F94`, cell `0x01069FDC`, also selects
`0x0091C590` at `+0x48`. Its RTTI locator `0x01179078` identifies offset
`+0xB4` of `.?AVWindow@Sqwt@@`, descriptor `0x01269E20`. Other derived
tables remain outside this finding. The
[Window attachment and conditional focus notifications](sqwt-window-trigger-order.md#attachment-and-conditional-focus-notifications)
records the focus store, anchor Visibility gate and changed-value guards.

The value is therefore derived and not a constant. It reads false while
`0x01357020` is null or the gate fails, and it can be true only when the global
names the element or a descendant. The writes to `0x01357020` are at `0x0091DC8D`
in `0x0091DBD0` and at `0x0091E75C` in `0x0091E3E0`, and the writes to
`0x01356F34` are at `0x0091BFA5` in `0x0091BF40` and at `0x0091BEE9` in
`0x0091BE10`. The runtime Visibility state and the remaining writer conditions are open.

## Surviving gaps

- Layout and final pixels under the [conditional descendant draw](#consoleicontray-push-icon-descendant-draw).
- Hit testing under a transform.
- The full caller and notification conditions for the separate centre update at `0x0093BBFF`.
- The events that run the `IsMouseOver` setters `0x0093A410` and `0x0093A480`.
- The runtime Visibility state of the AnchorElement and the remaining caller
  conditions for the keyboard-focus and AnchorElement pointer writers.
- Whether a derived `UIElement` class overrides slot `+0x48` of the `+0xB4` table.
- First property trigger evaluation for the Window's selected Style.
  [ActionMenu materialization](sqwt-window-trigger-order.md#ordinary-actionmenu-property-elements)
  and [event-pass order](sqwt-window-trigger-order.md#conditional-event-passes)
  have separate conditions.
