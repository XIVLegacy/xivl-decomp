# Sqwt Track and ScrollBar rectangle requests

The native Track arrange override computes a Thumb rectangle from supplied
Track fields, a final size and the child's desired size. The desired-size
branch recomputes the rectangle. This page records those requests and the
ScrollBar and selected float Slider routes that precede them. Final layout
and rendering remain UNKNOWN.

The [Sqwt parts study](sqwt-tab-scroll-text-parts.md) owns the Control layout
rules, style parts and unresolved-background questions. STATIC below means an
instruction, string or RTTI observation. Property roles inferred from descriptor
and name associations are identified separately.

## Executable and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32 image
base `0x00400000`. All addresses on this page are VA.

The producing instruments are Ghidra through `ghidra-cli` for focused
disassembly, cross-references and decompiler leads, and Node Buffer reads for
independent PE section mapping, instruction-byte comparison, relative-target
calculation and RTTI walks. The pointer path uses `llvm-objdump -d` and
little-endian table reads through the same PE section mapping. The
observations below derive from instructions
and data. Decompiled types and expressions are leads, not original source.
These are static observations, not target execution or live layout results.

| Function | Inclusive VA range |
|---|---|
| Track arrange | `0x0099BD90..0x0099BFD9` |
| Float Slider viewport refresh | `0x00997030..0x00997091` |
| Float Slider horizontal style route | `0x0099759C..0x00997680` |
| ScrollBar measure | `0x00989D60..0x00989EE4` |
| ScrollBar arrange | `0x00989EF0..0x0098A0AD` |
| Vertical part name assignment | `0x00970B30..0x00970D18` |
| Track constructor | `0x0099C460..0x0099C678` |
| Track inherited measure | `0x0092A250..0x0092A2F4` |
| Arrange request helper | `0x0093A870..0x0093AAA1` |

The complete Track arrange body is `586` bytes with SHA-256
`ad91719bb6767be6375a26cda8a3e11332e0ce95c0541d1882c3b9e139c043fb`.

## Classes and vertical part path

The offset-0 vftables have the layout slots below. STATIC. The RTTI catalogs
use RVA, so add the image base before comparing them with these VAs:
[RTTI](../../config/ffxivgame.rtti.json) and
[vtable slots](../../config/ffxivgame.vtable_slots.jsonl).

| Class | Offset-0 vftable | Arrange slot 54 | Measure slot 55 |
|---|---|---|---|
| ScrollBar | `0x0107798C` | `0x00989EF0` | `0x00989D60` |
| Track | `0x0107A54C` | `0x0099BD90` | `0x0092A250` |
| Thumb | `0x0108488C` | `0x00937480` | `0x00937310` |

The type descriptor names are
`.?AVScrollBar@Primitives@Controls@Sqwt@@`,
`.?AVTrack@Primitives@Controls@Sqwt@@` and
`.?AVThumb@Primitives@Controls@Sqwt@@`. Their type descriptors are
`0x012E60D0`, `0x012E7934` and `0x012EA47C`, respectively. The COL offset
is zero for each table above. Other subobject tables do not supply the layout
identities in this table.

The name assignments in `0x00970B30..0x00970D18` identify these members.
STATIC.

| Owner | Member | Assigned name | Name string VA |
|---|---|---|---|
| ScrollViewer | `+0xC78` | PART_VerticalScrollBar | `0x010730AC` |
| ScrollBar | `+0x384` | PART_UpButton | `0x010730C4` |
| ScrollBar | `+0x388` | PART_DownButton | `0x010730D4` |
| ScrollBar | `+0x380` | PART_Track | `0x010730E4` |
| Track | `+0x2C8` | PART_Thumb | `0x010730F0` |

The UpButton, DownButton and Track assignments guard null members. The Thumb
assignment has no separate null test. These names establish this object path,
not its current owner, lifetime or live state.

## Track input associations

The constructor associates the following fields with descriptors. The
initializer ranges associate those descriptors with the named strings. The
addresses and associations are STATIC. Interpreting the fields as the
corresponding properties follows those associations.

| Track field | Name | Descriptor | Name string VA | Initializer range |
|---|---|---|---|---|
| `+0x2A4` | Maximum | `0x01360EE4` | `0x0107A388` | `0x00F284A0..0x00F284C3` |
| `+0x2AC` | Minimum | `0x01360F50` | `0x0107A390` | `0x00F284D0..0x00F284F3` |
| `+0x2B4` | Value | `0x01360F74` | `0x0107A3A4` | `0x00F28530..0x00F28553` |
| `+0x2BC` | ViewportSize | `0x01360F08` | `0x0107A3AC` | `0x00F28560..0x00F28583` |
| `+0x2C4` | Orientation byte | `0x01360F2C` | `0x0107A398` | `0x00F28500..0x00F28523` |

The constructor passes `1.0` for Maximum, `0.0` for Minimum, Value and
ViewportSize, and byte `1` for Orientation to the respective property helpers.
These helper arguments do not establish universal effective defaults. It sets
`+0x2C8` to `this+0x2CC` and calls `0x009CA650` at that member. STATIC.

Arrange selects width-axis arithmetic for Orientation byte zero and
height-axis arithmetic for any nonzero byte. The flag at `+0x2C6` bit
`0x20` is described by its formula below. Its property name is not established.

## Height-axis Track request

The following inputs describe the height-axis branch of
`0x0099BD90..0x0099BFD9`:

- Maximum `M`, Minimum `m`, Value `v` and ViewportSize `V`.
- Nonzero Orientation and flag `b = (+0x2C6 & 0x20) != 0`.
- Supplied final size `(W,L)`.
- Child pointer `p = [Track+0x2C8]` and desired height
  `h = float[p+0x12C]`.

The ordered finite algebra below summarizes the observed arithmetic. It is
not a bit-exact floating-point model.

1. Select `q = M-v` when `b` is set, otherwise `q = v`.
2. Compute `R = M-m` and store `D = V+R` as a float.
3. Start `r` at zero. If stored `D` is not ordered-equal to zero, recompute
   `V/(V+R)` and store it as float `r`.
4. Start `T` at `L`. If the float-stored difference `r*L-h` is negative,
   recompute `T = (L-h)/(R/D)` and store it as a float.
5. Start `y` at zero. If stored `D` is not ordered-equal to zero, compute
   `T*((q-m)/D)` and store it as float `y`.
6. Submit rectangle `(0,y,W,T*r)` to the child through `0x0093A870`.
   Copy `(W,L)` unchanged to the output size.

STATIC for the reads, arithmetic, stores and branches. The reversed
`q=M-v` is float-stored before the later subtraction of `m`. The formula
is not `M+m-v`. `R` remains an x87 intermediate. The ratio recomputes
`V+R` rather than simply reusing the float-stored `D`. The desired-height
difference is float-stored before comparison. Adjusted `T` and `y` are
float-stored, and the requested height uses `T*r`. Those stores and x87
intermediates prevent treating the algebra as exact floating-point equivalence.

The desired-height branch is a recomputation, not a conventional minimum clamp.
As conditional finite symbolic arithmetic, `M=80`, `m=0`, `V=20`,
`L=100` and `h=30` give `T=87.5` and requested height `17.5`.
This is an unexecuted algebra example, not a runtime result. Replacing that
branch with `max(h,proportionalHeight)` does not describe these instructions.

### Comparison and refusal limits

The guards at `0x0099BDFD..0x0099BE06` and
`0x0099BF5A..0x0099BF63` use FUCOM, FNSTSW, a test of AH against
`0x44`, and JNP. Ordered zero skips division. Unordered takes division
if execution proceeds with masked floating exceptions. STATIC.

At `0x0099BF21..0x0099BF2A`, FCOM compares zero with the float-stored
difference `r*L-h`. When execution proceeds with masked floating exceptions,
FNSTSW, a test of AH against `0x41`, and JNZ skip recomputation for equal,
positive or unordered stored difference. A negative stored difference takes
the recomputation. The floating-control environment is UNKNOWN.

There is no additional `R==0` or `D==0` guard inside the recomputation,
no finite-value rejection and no general clamp of Value, range, viewport,
position or length in this body. The child pointer is dereferenced without a
null check. These absences establish no safe domain, exception guarantee or
downstream acceptance of negative or nonfinite rectangles.

### Conditional extent substitution

The upstream extent/viewport/offset binding is not traced here. As conditional
ordered finite algebra only, suppose Orientation is nonzero, `+0x2C6` bit
`0x20` is clear, `m=0`, `M=extent-viewport`, `V=viewport`,
`v=offset`, extent is nonzero, and the desired-height branch leaves `T=L`.
The requested thumb height then reduces to `L*viewport/extent` and `y`
to `L*offset/extent`. With that flag set instead, the same other assumptions
give `y=L*(extent-viewport-offset)/extent`. These substitutions are not a
bit-exact floating-point model or evidence of an actual upstream state binding.

## Width-axis Track request

When the Orientation byte is zero, `0x0099BE2F..0x0099BEEE` uses
the width-axis version of the same finite algebra. For supplied final size
`(L,H)`, read the child's desired width `h` at `+0x128` in
`0x0099BE35`. Compute `D`, `R`, `r` and `q` as above. At
`0x0099BE64` the negative stored difference `r*L-h` takes the
recomputation `T = (L-h)/(R/D)`. Otherwise `T=L`. With a
nonzero `D`, `0x0099BE9F..0x0099BEB5` stores
`x = T*((q-m)/D)`. The rectangle request at `0x0099BFB4` is
`(x,0,T*r,H)`. The copy of the supplied final size is unchanged.
The guards and x87 limitations above apply to the width branch as well.

## Float Slider and its sound configuration styles

The standard install's SQEX containers were decoded with basename-keyed
Blowfish and their XML element and attribute locations checked. The table
identifies the carriers used below. Decoded markup is outside this page.

| Carrier under `client/sqwt` | Bytes | Container SHA-256 |
| --- | ---: | --- |
| `widget/ConfigSoundWidget.form` | 17417 | `c8f1bccce73ac9be0658a061bbbee94bccc70d07ff906ece6f78eae4fc542441` |
| `widget/ConfigSoundWidget.tpl` | 802 | `1481a37ce413c31bf1cdc478c5a77cfa2606f703558d58acd60bd119d139de4e` |
| `common/default.style` | 353783 | `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` |
| `common/default.skin` | 141967 | `6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640` |

At form lines 59, 76, 97, 118 and 139, the five volume Sliders use
Maximum 100, SmallChange 1 and LargeChange 10. The sound-position Slider at
line 175 uses 80, 10 and 20 respectively. All six specify width 160,
`SqwtStyle=SLD_basis` and select `Style_Slider`. They omit Minimum,
Value, Orientation and direction reversal. Each has four self-targeted
repeat bindings. Left and pad LLeft request `Slider.DecreaseSmall`,
and Right and pad LRight request `Slider.IncreaseSmall`. The pad
attribute has one trailing ASCII space in each row. The visible Dolby
control at line 208 is a separate `TOG_config` ToggleButton.

`Style_Slider` at template lines 2-15 targets Slider, has an empty
`SqwtIsAutoSetSkin` and has no setters. A ValueChanged trigger requests
`UILuaCommands.SliderValueChanged` without a declared parameter. The
three command-trigger sound requests use numbers 20, 57 and 69 for
`SE_SYS_TestSound`, `SE_TestSound` and `SE_VOICE_TestSound` respectively.
The [Lua controls](https://github.com/XIVLegacy/xivl-client-scripts/blob/69b204159fcef6d69ae7eb247db48ce87a41c960/docs/config-sound-widget-contract.md#local-contract)
supply the config consumers. Native delivery of the value to this command
is outside this style trace.

### Sound preview release declarations

Three standard form Sliders add five targetless bindings each after their
four adjustment bindings. MEASURED from the standard form pinned above.

| Owner | Form lines | Command |
| --- | --- | --- |
| `Slider_SystemSE` | 82-86 | `SE_SYS_TestSound` |
| `Slider_SE` | 103-107 | `SE_TestSound` |
| `Slider_Voice` | 124-128 | `SE_VOICE_TestSound` |

Each group declares KeyBinding Left and Right with `IsUp=True`, PadBinding
LLeft and LRight with `IsReleased=True`, and MouseBinding LeftUp with
`HandledEventsToo=True`. Five of the six PadAction strings end in one
ASCII space. The exception is `Slider_SystemSE` LLeft. None declares a
command parameter, target or repeat override. The other three Sliders omit
these cue bindings.

STATIC. The [ordinary key construction](sqwt-command-parameter-consumer.md#actionmenu-keyboard-aliases)
passes IsUp to the gesture at `0x0094755D -> 0x0068C550`.
The matcher `0x0068C5D0` compares the event phase at +0x2B with the
gesture phase at +0x20 before key and modifier checks. The
[pad reader and construction](sqwt-command-parameter-consumer.md#actionmenu-padbinding-source)
pass IsReleased through `0x009488B6..0x009488C5` and
`0x009477A5 -> 0x0068C730`. The pad mask reader uses
`0x005557A0 -> 0x0097FC70`, which searches for each table name
case insensitively and returns on the end of that name. The trailing
space therefore leaves the matching LLeft or LRight bit available.

STATIC. The MouseBinding reader `0x00948720` tests the release
table at `0x00948853 -> 0x0094C1D0`. LeftUp maps to zero in
that table, and a successful match sets markup +0x168 to 1.
The common attribute reader at `0x009483B5..0x00948455` names
the separate HandledEventsToo byte +0x16A. Current event recipient,
actual matcher admission and cue playback remain open.

### Horizontal style parts and construction

The `SLD_basis` parts at style lines 5649-5652 refer to
`Button:BTN_basis_slidebarSlidingH` for HorizontalSlidingThumb and
`Window:SLD_basis_bgH` for HorizontalBackground. The background Frame
at lines 507-517 uses its N skin for Normal and Disabled and its H skin
for Hover, Active and Focused. Their three stretch cells at skin lines
1032-1041 have widths 18, 4 and 18, height 32 and HitMargin 4,0,4,0.
The Thumb's Frame at style lines 1565-1575 uses the same skin for all five
conditions. Its one stretch cell at skin lines 1029-1031 is 10 by 32 with
no HitMargin. Neither part declares a SkinMargin.

The float Slider constructor calls float RangeBase at
`0x009983CC -> 0x00967850`. Its x87 loads and calls to `0x009C1FD0`
initialize the following float properties. The descriptor initializer
range `0x00F23FA0..0x00F24083` associates the descriptors with the
property-name strings. STATIC.

| Property | Descriptor | Constructor call | Initial value |
| --- | --- | --- | --- |
| LargeChange | `0x0135AE1C` | `0x009678C0` | 1.0 |
| Maximum | `0x0135ADC4` | `0x009678E3` | 1.0 |
| Minimum | `0x0135ADA0` | `0x00967906` | 0.0 |
| SmallChange | `0x0135AD7C` | `0x0096792B` | float bits `0x3DCCCCCD` at `0x00FA4968` |
| Value | `0x0135ADF4` | `0x0096794E` | 0.0 |

These constructor values precede markup and script property writes.

The float Slider's default template constructor `0x00997C50` calls
`0x00996BD0`, which constructs a Track and names it `PART_Track` at
`0x00996C28`. The Track constructor above constructs its Thumb.
For Slider byte E+0x375 zero, `0x009975AF..0x009975D4` assigns
`Slider:HorizontalBackground` to E. At `0x009975F4..0x00997635`
the same helper receives target `PART_Track:PART_Thumb` and style
part `Slider:HorizontalSlidingThumb`. The nonzero branch requests
the vertical names. This joins the named styles to distinct owners.

The float Slider's template attachment callback at table cell
`0x010792F8 -> 0x00997390` stores the resolved Track at E+0x378.
On the admitted path it writes Track ViewportSize 1.0 at
`0x009974D6` and requests Thumb size (8.0, 8.0) at
`0x00997519 -> 0x00929C40`. The latter call writes the raw
size pair at Thumb+0x280 and +0x284.

The horizontal refresh `0x0099703E` uses the Slider scalar
at E+0x1B4 for the divisor. It reads range fields E+0x344 and
E+0x34C, subtracts them and requests ViewportSize
`(8.0 / divisor) * (maximum - minimum)` at `0x00997089`
via `0x0099C1D0`, descriptor `0x01360F08`. The double
constant is at `0x00F57E48`. The nonzero orientation branch uses
E+0x1AC. The style route at `0x00997793` and table cell
`0x01079278 -> 0x00997370` both reach the refresh.

Thumb cells `0x01084928`, `0x01084954` and `0x01084978` supply
the size accessor `0x008E92C0`, scale getter `0x0092B600` and
resolved-skin callback `0x00938740`. They share the
[skin-derived size path](sqwt-number-input-button-frames.md) with
Button. The callback can replace the raw size pair after a successful
style lookup. Desired width at Track arrangement also passes through
[FrameworkElement measurement](damage-plate-form-rules.md#canvas-measurement).
The final ordering between the attachment write, style publication
and measurement, effective scale, first rectangle and first draw
remain unobserved for this installed form.

### Focus from the float Slider thumb

STATIC. The float Slider, Track and Thumb input tables at `0x010790C4`,
`0x0107A424` and `0x01084764` all select `0x0092EB90` in input slot
`+0xAC`. It calls the base at `0x0092EB99`, then requires an unhandled
event and the complete receiver equal to event original source at
`0x0092EB9E..0x0092EBAF`. The admitted preview left-down calls
`0x0092EA10` at `0x0092EBB7`.

The Thumb constructor requests Focusable false at
`0x009CA754..0x009CA75C -> 0x0093C3C0`, descriptor `0x0135975C`.
Track construction at `0x0099C48C` takes the FrameworkElement
base path and retains the
[UIElement default](sqwt-qualified-property-lookup.md#actionmenu-stackpanel-focus-declarations).
The float RangeBase constructor uses Control, which requests IsTabStop
true at `0x00938F49..0x00938F53` and Focusable true at `0x00939063`.
The installed BGM Slider at form line 59 has no local Focusable or
IsTabStop attribute.

Track inserts its Thumb at `0x0099C65F -> 0x009313C0`. The float
Slider requests its default Track template at
`0x00997CBB -> 0x00938170`. The admitted current-template branch
at `0x00936EB9..0x00936ED7` calls the same attachment helper on
the template root. The helper stores the complete logical parent
at child `+0x240`, including at `0x0093145B`. With those links
retained, the chain is Thumb, Track, Slider.

When the Thumb fails the non-Window receiver checks in `0x0092EA10`,
the branch at `0x0092EB0F` searches logical parents through
`0x0092B150`. On the default chain, an eligible Slider can receive
the input focus request at `0x0092EB25`. The
[ancestor checks](sqwt-number-input-pointer-edit.md#focus-before-the-quantity-step)
and subsequent focus admission still apply. This path requests focus
during preview down.

Thumb input slot `+0xC4` selects MouseDown handler `0x009CA380`.
On the unhandled path it requests capture through input slot `+0x3C`
at `0x009CA420`. The Slider's MouseDown subscriber `0x00996EA0`,
bound at `0x00998478..0x0099848D`, requires original source equal
to the current Track at `0x00996EF0..0x00996EF9`. Only that path
can select the large adjustment commands at `0x00996F83..0x00996F94`.
A Thumb original source fails the comparison. Drag deltas use the
separate callback `0x00996C90`. Pointer containment, retained focus
and actual delivery remain unobserved.

### Float adjustment and ValueChanged

STATIC. Float Slider input table `0x010790C4`, installed at
`0x009983FB`, selects Executed handler `0x010791C8 -> 0x00996DA0`.
The handler calls the base event store at `0x00996DA9` and requires
the handled byte to be clear at `0x00996DB1`. The command initializers
`0x00F27DA0` and `0x00F27E20` pair Slider.IncreaseSmall and
Slider.DecreaseSmall with `0x01360710` and `0x013607D0`.
The matching branches add or subtract SmallChange from current effective
Value at `0x00996DCF..0x00996DDB` and `0x00996DE4..0x00996DF0`.

Both branches store the proposed float and mark Executed handled. The
admitted path divides that proposal by SmallChange, calls the x87
conversion helper `0x009D6600`, multiplies the returned integer by
SmallChange and requests typed Value through
`0x00996E55..0x00996E87 -> 0x00966C00`. The conversion helper has an
SSE truncation path and an x87 path with a residual correction. On the
stock whole-number ranges and small steps above, an exact integral
quotient within the integer range is retained. The full nonintegral and
exceptional conversion boundary is outside this join.

Float Value setter `0x00966C00` supplies descriptor `0x0135ADF4`
and callback `0x00966FF0` to `0x009B9320`. Callback slot 4 copies
the request from E+0x35C to E+0x370 at `0x0096701C..0x00967028`
and calls `0x00966CF0`. For finite comparable values, that path caps
the request at Maximum, then floors it at Minimum. The prior effective
value at E+0x36C gates the event. On a changed admitted value,
`0x00966E43..0x00966EA1` updates the snapshot and routes descriptor
`0x0135ADE8` through `0x0091F1E0` with the Slider as source and
original source. The event holds both new and old floats. The descriptor
initializer at `0x00F23F70` names it ValueChanged.

The selected Style above requests a separate command without a
declared parameter. The [inherited Lua
dispatcher](https://github.com/XIVLegacy/xivl-client-scripts/blob/c0b3540cfd57f586ce0fbbb10868bac202851f4d/docs/config-sound-widget-contract.md#inherited-sound-control-dispatch)
reads and floors the sender's effective Value before delivering it to
ConfigSoundWidget. The routed event's floats are separate from that Lua
read. Actual focused input, initial delivery and audio application remain
conditional.

## Configuration Sound opener

MEASURED. The standard `widget/ConfigWidget.form` container is 2930
bytes with SHA-256
`2029f43c19dd567ad11946553ba09520fe8f4860bfa85c57ae0f00409d972cbf`.
The basename-keyed SQEX decode is 2922 bytes with SHA-256
`6114add8a5e292997ae903575e7c73a044275826ad83cf2275b51f970381ad45`.
Its root is Window_ConfigWidget. Lines 5 and 10 select
`common/WINBasisWithTitle.tpl` and `ControlTemplate_Window_WINBasisWithTitle`.

The childless Button_Sound at line 21 declares UILuaCommands.Operate,
Content @1310 and SqwtStyle BTN_basis. It has no local CommandTarget,
CommandParameter or Template. The [ConfigWidget
script](https://github.com/XIVLegacy/xivl-client-scripts/blob/ad36e3b559f4bebf3e3d62a70f4ccc598818b5f3/docs/config-sound-widget-contract.md#configuration-menu-opener)
installs a confirm condition on that name and requests the sound child
when the matching Operate arrives. The [ordinary Button Click
path](sqwt-command-parameter-consumer.md#button-mouse-source) qualifies
the native command request. Actual pointer delivery and child lifetime
remain conditional.

## Sound Configuration Dolby toggle

MEASURED. The standard ConfigSoundWidget form at line 208 declares the
childless `ToggleButton_DOLBY`, IsChecked True, command
`UILuaCommands.ToggleButton`, SqwtStyle `TOG_config`, Width 120,
Margin 0,2,0,2 and HorizontalAlignment Left. There is no local CommandTarget.
The carrier hashes are in the [sound configuration styles](#float-slider-and-its-sound-configuration-styles)
table above.

In `default.style` lines 3448-3451, `TOG_config` redirects CheckedButton
to Button style `TOG_config_leftOn` and UncheckedButton to Button style
`TOG_config_rightOn`. Their Frame rows are at lines 1577-1600. For each
prefix, Normal uses its N skin, Disabled its D skin, and Hover, Active,
Anchored and Reserved use its H skin. Focused and Droppable use NULL.
There is no SkinMargin or SkinOffset on either Frame.

In `default.skin` lines 1042-1061 and 1584-1593, the six referenced
CellSkins use a 3-by-1 grid. Their stretch cells are 24, 2 and 24 pixels
wide, each 26 pixels high. HitMargin is 0,0,0,2.

STATIC. The native [ToggleButton own-key and render selection](action-menu-timer-icon-style.md#conditional-initial-toggle-skin)
uses UncheckedButton:Frame as group zero and CheckedButton:Frame as
group one. Its [checked-state callback](../input/game-controller-startup.md#checked-state-callback-changes-the-image-part-index)
requests the corresponding image-part index. With the current frame
resources admitted, that selects one Frame row for the owner. It joins
the named checked and unchecked art to the control without a separate
visual child. Effective layout after HitMargin, first paint and routed
Dolby command admission remain open.

## ScrollBar measure and Track request

The custom branch of ScrollBar measure `0x00989D60..0x00989EE4` and
arrange `0x00989EF0..0x0098A0AD` requires `+0x38C` bit `0` to be set.
When that bit is clear, the functions delegate to the Control pair described
in the [parts study](sqwt-tab-scroll-text-parts.md#layout-rules). STATIC.

Custom measure passes the same supplied available size to each non-null child
at `+0x384`, `+0x388` and `+0x380` through `0x0093CD90`. Absent
children contribute zero. For nonzero Orientation at `+0x37C`, ordered
child comparisons select the maximum desired width and the result height is
the sum of desired heights. Those components are read at `+0x128/+0x12C`.
This finite algebra does not define nonfinite inputs as ordinary maxima.

Custom arrange submits `(0,0,W,H)` to each non-null end button at
`+0x384` and `+0x388` through `0x0093A870`. After each request it
reads `+0x1B4` and `+0x1AC` from that child. With nonzero Orientation,
it submits `(0,a,W,H-a-b)` to the non-null Track at `+0x380`, where
`a` and `b` are the first and second `+0x1AC` post-request scalar
reads. Missing child contributions remain zero. It returns `(W,H)` unchanged.
STATIC. There is no clamp of `H-a-b` in this body.

The scalar reads are inputs to this request. Their broader semantic identity
and the end buttons' final dimensions and positions remain UNKNOWN. The
full-rectangle requests do not establish overlapping final button placement.

Track's inherited measure at `0x0092A250..0x0092A2F4` seeds its result
from virtual slot `39` at `+0x9C`, enumerates children through `+0x254`,
and measures each with the supplied available size through `0x0093CD90`.
Each result component is raised only when the child desired component at
`+0x128/+0x12C` is greater. STATIC. The seed and child desired sizes
remain inputs. Thumb inherits Control arrange and measure, with no separate
geometry override at slots `54/55`.

## Arrange request boundary

The helper `0x0093A870..0x0093AAA1` substitutes a shared zero rectangle
when Visibility byte `+0x158` is zero. It checks pending layout and cached
rectangle fields. Its core path stores the supplied components into
`+0x168/+0x16C/+0x170/+0x174` when visibility is nonzero and dispatches
virtual slot `26` at `+0x68`. A cache path can skip that dispatch when
dimensions are unchanged and update position through `0x00940110`.
The helper itself has no finite or positive clamp. STATIC.

The downstream slot-26 core and `0x00940110` behavior remain UNKNOWN.
Effective constraints, margins, alignment, pixel rounding, final geometry,
current owner and lifetime, live state and rendering are not established by
these rectangle requests.

The [parts study's unresolved-part question](sqwt-tab-scroll-text-parts.md#open-questions)
also remains open. The missing VerticalBackground resolver/default-record/draw
consumer join and the eight float roles and three pointer roles in the default
records are UNKNOWN. This page establishes no fallback or no-draw result.

## Reproduction

Use an executable with the stated size and SHA-256. Read its PE32 section
headers and map each cited VA by subtracting `0x00400000` and locating the
corresponding section's raw data. Walk each vftable's preceding COL, confirm
its zero offset and type descriptor, and compare slots `54/55` with the
public RTTI and slot catalogs.

Use focused Ghidra disassembly of the complete inclusive ranges above.
For example, `ghidra disasm 0x0099BD90 -n 250 --json` includes the full
Track arrange body. Limit interpretation to that body's end at
`0x0099BFD9`. Independently compare instruction bytes and relative branch
destinations with the mapped executable. Read the five property-name strings
and follow their initializer descriptor associations into the constructor.
Preserve the x87 store and comparison order when evaluating the conditional
algebra. Final layout requires separate evidence from the downstream paths.
