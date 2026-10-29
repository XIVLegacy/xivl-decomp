# DamagePlate numeric presentation

[Documentation index](../README.md) | [Combat result path](../net/combat-result-element.md#damage-plate)

This page records a bounded static numeric branch and declarations from the identified DamagePlate assets. It does not establish displayed pixels, a concrete projection, or a live animation lifecycle.

## Identity and method

The executable is `ffxivgame.exe`, 15996808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32 machine `0x014C`, optional-header magic `0x010B`, image base `0x00400000`. All executable locators below are VAs. Subtract the image base for RVAs.

The method is fresh ghidra-cli disassembly from the identified executable, independent mapping of every inspected instruction to file-backed PE section bytes, direct little-endian data reads, and control-flow and argument tracing. Local decompiler renderings were interpretation aids. Claims below rest on instructions and original data, not inferred class inheritance.

Asset paths are relative to the supplied install. All four named Sqwt inputs below carry the complete SQEX signature. Filename-keyed decoding uses `xivl-tools:tools/blowfish.py`, 4324 bytes, SHA-256 `7c4208f5d3b7acf20bfb67ddcba706cbb10bf4d1c6c1677b4b11d8685774f90e`, with the recipe in `xivl-tools:docs/formats/sqex.md`. Each decoded document passed strict UTF-8 and namespace-aware XML parsing. Re-encryption reproduced every input byte, including each partial-block tail. Neither the original inputs nor decoded documents are distributed.

| Path | Input bytes | Input SHA-256 | Decoded bytes | Decoded SHA-256 |
|---|---:|---|---:|---|
| `client/sqwt/system/ingame/DamagePlateWidget.form` | 2062 | `b9dd21df18d840730938131090f323319f8bb4ff667853ac919ab36204d6e206` | 2054 | `830ac577f2ed980bdfb1a938a15846c806b11fad8a9583ea97825b24175fb204` |
| `client/sqwt/system/ingame/DamagePlateWidget.tpl` | 10071 | `c3f492929f4009cfd6192ea2067f86d96a07ad532304269f03e3ed860c6adc75` | 10063 | `29becbdcd76890d1d225dc1f5ed3c689a9efecdda812893edb765e95c4e8f22f` |
| `client/sqwt/common/default.style` | 353783 | `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` | 353775 | `33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6` |
| `client/sqwt/common/default.skin` | 141967 | `6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640` | 141959 | `929e65e9d5f191292d9b044e3e659ba115f18f6b3d8a82894046a7c75cbcad5a` |

STATIC denotes inspected instructions, original data or identified asset declarations. INFERRED denotes their bounded interpretation. UNKNOWN retains an unresolved edge. A declaration does not establish a native default or successful runtime selection.

## Numeric branch and reused roles

At `0x007956D4..0x00795729`, the row effect ID is reloaded. A zero ID exits. The kind is `(effect >> 27) & 15`. The kind table at `0x007967CC` selects `0x00795A27` for kind 1. That branch uses a second selector `(effect >> 9) & 63`. Subcases 0, 1 and 5 call the digit routine with -1. Other kind-1 subcases pass the row numeric dword to the digit routine. The ordinary branch at `0x00795B22` tests `effect & 3`: value 3 requests an additional text lookup with ID `0x7D1`. A bounded numeric-only selection therefore excludes subcases 0, 1 and 5 and low tag 3. This does not assign retail visual meanings to those selectors. STATIC.

The cache-name pointers at `0x012C41B8..0x012C41C8` identify `Label_DamageDisplay:TextBlock_Number_5` through `TextBlock_Number_1`, in that order. The lookup loop at `0x00795800..0x00795871` stores these five pointers for the selected slot. The digit routine `0x00795280..0x00795408` iterates exactly five entries. A null entry skips all work, including quotient advancement. On a non-null entry, a negative current value requests Visibility 0 and makes no Text call. A nonnegative value requests Visibility 2, computes signed quotient and remainder by ten at `0x007952F9..0x00795317`, passes the remainder through the integer text formatter, and calls the Text setter at `0x0079536D`. After that call it advances to the quotient and replaces a zero quotient with -1. STATIC.

For five non-null entries and admitted property stores, zero requests one visible zero in Number_5 and collapses the other four roles. The value 100000 requests five visible zero digits. A negative value collapses all five roles without clearing their cached Text. Thus a blank role and a stored zero are distinct. The low-five-decimal-digit interpretation agrees with the [combat result digit contract](../net/combat-result-element.md#damage-plate). INFERRED from the arithmetic, role order and integer text path.

These are setter requests, not unconditional writes. `0x0093C2A0` supplies the Visibility descriptor `0x013596BC` and cell `+0x158` to `0x009BCB50`. That store can reject a value according to existing property flags and caller arguments, and equal values do not notify. `0x00914580` maps Collapsed to 0 and Hidden to 1, with Visible represented by 2. The Text wrapper `0x009556A0` supplies descriptor `0x0135A300` and cell `+0x33C` to `0x009BA4E0`, whose reference branch can return without an immediate text store. The initializer `0x00F23800` names that descriptor Text. STATIC. The selected roles' runtime property states are UNKNOWN.

`0x00795734` skips slot allocation and name lookup when its cached wrapper already exists, then `0x0079590B` loads the retained role pointers. The low-nibble slot index advances and wraps after 4 at `0x00795931..0x00795952`. This establishes five reused slots. It does not establish an animation clock reset. Destruction at `0x00794E80..0x00794ED5` deletes non-null wrappers in all five slots and deletes the shared form owner when its reference count reaches zero. This is list destruction, not per-popup completion. STATIC.

## Palette requests

`0x00794BB0` returns classification 0 for the selected container element, 1 for a nonselected element whose tested status byte is nonzero or tested halfword differs from `0xFFFF`, and 2 otherwise, including a null element. The classification is derived from the other element selected by `0x00794F30`. For kind 1 that is the record source. STATIC. Names such as friendly or hostile are not established by those tests.

The ordinary kind-1 branch calls `0x00794C00` with final selector 0. Classification 0 selects eight float lanes at `0x012C41D4`, classification 1 selects `0x012C4234`, and classification 2 selects `0x012C4294`. The first four lanes go to `0x00938020` and the next four to `0x00938050`, for each non-null cached digit and both non-null text roles. The property initializers `0x00F22540` and `0x00F22570` identify those descriptors as Foreground and FontEdgeColor. STATIC.

The tuples below give each original float as `float32(n / 255)` in native lane order. They do not assign a framebuffer channel order.

| Classification | Foreground numerators | FontEdgeColor numerators |
|---:|---|---|
| 0 | `(255,255,245,180)` | `(255,66,46,20)` |
| 1 | `(255,255,214,222)` | `(255,120,16,16)` |
| 2 | `(255,255,255,255)` | `(255,26,26,26)` |

The palette routine has a separate selector-2/default palette at `0x012C4214`. The ordinary kind-1 call above does not select it. Final colors remain conditional on property admission, style and rendering behavior. UNKNOWN.

## Identified HD declarations and conditional font source

Construction at `0x0079516E..0x007951D0` supplies the UTF-16 literal `\system\ingame\DamagePlateWidget.form` at `0x00FE1888` to the named-file reader at `0x009104A0`, then passes the returned buffer to the document parser at `0x00DC2C60`. That reader resolves the supplied named path through `0x0090FBC0` and uses its file-read branch. This is a static request path. Successful open and the concrete selected document are not observed here. STATIC.

The identified form imports `common/default.style` and `system/ingame/DamagePlateWidget.tpl`. It selects Window template `TemplateControl_DamagePlate`. The form declares Label `Label_DamageDisplay`, height 88, with template `ControlTemplate_Label_DamageDisplay` and style `LAB_null`. In the identified template, that key has literal TargetType `Label`, and its visual subtree consists of a horizontal StackPanel containing five Canvas/TextBlock pairs. It contains no IconControl. Its TextBlocks Number_1 through Number_5 each declare FontSize 42, FontStyle Italic, FontType 4, FontEdge True, Canvas.Top 32, and style `TBL_parameterNormal`. STATIC declarations.

The form's icon and two text controls are sibling roles outside that digit subtree. The icon initially declares IconID 30. Excluding these siblings from a digit-only interpretation does not prove that native form construction makes no initial icon request. `LAB_null` and `TBL_parameterNormal` have Normal Frame rows selecting literal Skin `NULL`. The identified skin dictionary has no named `NULL` entry. These declarations do not settle native `NULL` semantics, clipping, baseline, italic treatment, control widths, property inheritance or exact glyph placement. UNKNOWN.

The [FontType bridge](sqwt-font-type-property-bridge.md) and [bitmap source request](bitmap-font-source-request.md#fr-02-bounded-source-tables-and-conditional-spellings) retain the broader conditional path. Independently inspected instructions at `0x00937B17`, `0x00987E10`, `0x00987910`, `0x00554560`, `0x004B2C90`, `0x00C97390`, `0x00C97258`, and `0x00C96D90` preserve a low-byte FontType selector into the module's second argument and source lookup on the constructor-selected bitmap path. The module wrapper changes exactly selector 3 to 0, leaving 4 unchanged. Construction at `0x004B2BA8..0x004B2BF9` chooses the five-entry source table `0x00F8D61C` when its third argument's low byte is zero. Entry 4 is raw key `0x000C0004`. The bitmap branch is a separate constructor argument. STATIC, conditional on that object and table path.

Under the published nonzero numeric resource-path mode, the key spells `data/00/0C/00/04.DAT`. The exact supplied file is 410724 bytes, SHA-256 `fc1f64c16825749c22d7a32c8275759856aa6b8118f10e8dbbb7f8c2614af5cd`. Its identity is established without decoding its pixels. This does not prove active table selection, successful font creation, selected glyph records, source dimensions, italic metrics, or a live font request. UNKNOWN.

## Popup and digit animation declarations

The identified Window template's `DamagePlateAnime.Popup` trigger declares transform origin `(0.5,0.5)`, zero-duration Top and VisualOpacity initialization, and the following DoubleAnimation segments. It also declares `NumberAnime.Start` targeting `Label_DamageDisplay`. STATIC declarations.

| Property | From | To | Begin seconds | Duration seconds |
|---|---:|---:|---:|---:|
| VisualOpacity | 0 | 1 | 0 | 0.2 |
| ScaleX and ScaleY | 1.75 | 1 | 0 | 0.15 |
| ScaleX and ScaleY | 1.5 | 1 | 0.15 | 0.15 |
| Top | 0 | -40 | 0.8 | 0.2 |
| VisualOpacity | 1 | 0 | 0.8 | 0.2 |

For Number_i, i from 1 to 5, the digit trigger declares start offset `0.05 * (i - 1)` seconds. Relative to that offset, Canvas.Top has three segments: 32 to 0 for 0.15 seconds at 0, 0 to 42 for 0.15 seconds at 0.15, then 42 to 32 for 0.10 seconds at 0.30. The first two use FromAccelType SpeedUp and ToAccelType SlowDown. The third reverses those types. No acceleration strength is authored on those segments. STATIC declarations.

The native caller selects the `DamagePlateAnime.Popup` name in the ordinary numeric branch at `0x00795B6A`. It requests the show path through `0x0096DE30` at `0x00796719`, resolves the command through `0x00928280` at `0x00796726`, and, when found, directly calls the returned object's virtual offset 8 at `0x00796761` with the selected Window subobject as target. This establishes the native show and dispatch requests. It does not prove command handling or storyboard execution. STATIC.

## Conditional native DoubleAnimation sample

Constructor `0x009B5440` installs vtable `0x0107DCD4`. Its preceding locator is `0x01181098`, complete-object offset zero, TypeDescriptor `0x012E8704`, named `.?AVDoubleAnimation@Animation@Media@Sqwt@@`. The constructor associates From/To and acceleration descriptors with receiver fields. Descriptor initializers `0x00F29930..0x00F299A9` name FromAccelType, FromAccelValue, ToAccelType and ToAccelValue. The reader `0x00916120` and table `0x012DFF38` map Liner to 0, SpeedUp to 1 and SlowDown to 2. STATIC.

At `0x009B4680..0x009B4861`, each strength equal to sentinel 10000000.0 is replaced with the float32 value `0.30000001192092896` from `0x00F9D4B0`. For strength s, From SpeedUp creates control point `(s,0)` and From SlowDown `(0,s)`. To SpeedUp creates `(1,1-s)` and To SlowDown `(1-s,1)`. Nonlinear mode is set for these nonzero types. STATIC.

In the admitted evaluator at `0x009B4A70`, call-record qword `+8` is Q and retained receiver qword `+0x190` is D. Q equal to D stores the retained endpoint at receiver `+0x180`, after the To offset. Otherwise, with nonlinear mode selected and D nonzero, the argument passed to `0x009B4530` is a float32 conversion of `floor(low64(1000 * Q) / D) / 1000`, using unsigned 64-bit division. The multiply helper is `0x009D5840`, and the divide helper is `0x009D5880`. This formula describes these instruction inputs. It does not identify their clock epoch, units, scheduler or admitted range. STATIC.

`0x009B4530` evaluates the cubic from `(0,0)` through the two stored controls to `(1,1)`. The caller consumes its Y result directly at `0x009B4B88..0x009B4BA9` as `From + (To - From) * cubicY(t)`. It does not invert cubic X in this branch. For a conditional 32 to 0 segment with From SpeedUp, To SlowDown, Q=1 and D=4, t is 0.25. Cubic Y is 0.15625 and the computed value is 27. This is an arithmetic consequence of the inspected evaluator, not a timed client sample. INFERRED.

Generic clock creation, Q admission and range, restart epoch, fill behavior, competing animation precedence and completion visibility or destruction are UNKNOWN. The declarations do not establish them, and the five-slot cache does not close them.

## Anchors and surviving gaps

For kind 1, `0x00794EE0` resolves the row target as anchor owner, while `0x00794F30` resolves the record source as the other element. The identified form declares StringData.Value0 `EID_BODY_DYN`. The native property query at `0x0079642A..0x00796482` substitutes `EID_NAME` only when its returned string is empty. STATIC. Actual attachment resolution and fallback are UNKNOWN.

`0x00796487..0x0079649B` calls `0x0054AF00` with the retained owner key, returned selector, null optional offset, and mask `0x1A0`. The helper performs dynamic-cast and virtual dispatch. Related setter bodies `0x0055A540`, `0x0055A560`, `0x0055A580`, `0x0055A5A0`, and `0x0055A5F0` store a primary key at record `+0x36C`, secondary key at `+0x37C`, mask at `+0x35C`, bounded selector at `+0x388`, and a four-float offset at `+0x398`. Those generic bodies do not prove this plate's concrete DrawingToolContext or WindowActor instance, dispatch table, association or projection. UNKNOWN.

## Conditional scatter offset request

For the selected DamagePlateAnime.Popup name, the path at `0x00796590..0x007966E6` also reads pair data at `0x00FE17D8`, list index `+0xC8`, rotation `+0xCC`, and a numeric conversion before updating the offset and advancing the index. The preceding comparisons exclude CurePlateAnime.Popup and StatusPlateAnime.Popup, using the not-equal string result from `0x00445DB0`. This is not an exhaustive classification of all other command names.

The scatter branch at `0x0079659B..0x007966E6` reads index i from list `+0xC8` and theta from list `+0xCC`. For i in 0 through 19, `(p,q)` is the float32 pair at `0x00FE17D8 + 8*i`. The caller computes `a = float32(double(p) - 0.5)` and `b = float32(double(q) - 0.5)`. It passes theta to `0x009D63B0` and `0x009D6280`, retaining each returned x87 value through a float32 store. Call those retained values S and C. Their inspected fallback paths contain FSIN and FCOS, and their error-name records identify sin and cos. Separately selected alternate paths remain outside this arithmetic interpretation. STATIC.

At `0x007956DE..0x007956E1`, the caller retains the row numeric dword at record `+0x3C + 20*row`. At `0x00796635`, FILD loads that dword as a signed 32-bit integer. The caller invokes `0x009DB130`, stores its result as float32, reloads that value, and calls conversion helper `0x009D6600`. The error-name record at `0x012EADD0` identifies log10. The inspected positive-finite fallback contains FLDLG2, FXCH and FYL2X. Let k be the signed low 32 bits returned by the conversion helper. STATIC.

The signed comparison at `0x0079664B..0x0079664E` chooses exact float32 1 for `k <= 1`. Otherwise it computes `m = float32(double(float32(k - 1)) * 0.30000001192092896 + 1.0)`. The double multiplier is stored at `0x00FB7DE0`. Using the retained values above, the final caller expressions are `X = float32((double(C)*a - double(S)*b)*double(m))` and `Y = float32(double(C)*b + double(S)*a)`. Z remains float32 zero and W remains float32 one. Only X receives m. The double arithmetic operations and float conversions follow the active floating environment. This evidence does not select a rounding mode. STATIC.

The original pair table is exactly represented by `float32((n/100,r/100))`, in order, for `(n,r)` equal to `(50,50), (38,22), (65,55), (42,72), (55,32), (38,48), (31,68), (48,35), (54,70), (61,40), (38,64), (52,28), (32,55), (55,68), (35,32), (57,62), (40,75), (64,38), (39,57), (46,28)`. All 40 original lanes agree with those float32 values. The table read has no local index range guard. The advance at `0x00796677..0x00796680` forms the 32-bit sum `i+1`, sign-extends it, and takes signed IDIV remainder by 20. For i in 0 through 19 this advances modulo 20. Current index admission is UNKNOWN. STATIC arithmetic.

The admitted scatter branch has no positive or nonzero numeric guard. Zero reaches the log10 helper and conversion path. The inspected x87 zero branch at `0x009DB206..0x009DB239` selects an error path using the negative-infinity extended constant at `0x012EBE6A`. Complete alternate-provider, error-handler and exceptional conversion results remain UNKNOWN. No final k, successful offset request or offset value is assigned here for zero, negative input or another exceptional helper result. Construction stores theta zero at `0x0079510F`. Current theta and an exhaustive later-writer set remain UNKNOWN.

When the arithmetic returns normally, the caller passes the four-float result to `0x005484D0` at `0x007966F2`. That helper obtains the retained Visual's peer, dynamically casts it to DrawingToolContext, and conditionally calls virtual offset `0xC0`. In the original DrawingToolContext table `0x00FA28E4`, that entry is `0x0055A5F0`. Its body obtains a cached record and copies the four floats to record `+0x398`. These conditional dispatch and store bodies do not establish the concrete DamagePlate peer, its WindowActor association, attachment lookup or world-to-screen projection. UNKNOWN.

Therefore an assumed fixed nameplate height or permanently zero final offset is unsupported. No historical runtime observation of another label closes these plate-specific gaps.
