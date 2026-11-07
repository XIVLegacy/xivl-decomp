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

The [authenticated ActionMenu inputs](action-menu-timer-icon-style.md#input-and-method)
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

## Applying the transform to the visual

`0x00940260` forwards to `0x00941290` with the descriptor `0x01359868`, which is
the `VisualTransform` property (name literal at `0x0106C4CC`, registered by the
thunk at `0x00F23290`) at `Visual +0x38`. `0x00941290` calls the transform's
vtable slot 3 (`0x00941290..` `CALL [vtable+0xC]`), compares the returned matrix
with the stored one through `0x0098FBB0`, and when they differ copies it into
the property's matrix through `0x0098F8D0`. STATIC. The element's transform is
thus held as a matrix on the visual.

Not established: where the draw path reads `VisualTransform`, whether the
element's layout offset is composed inside or outside the matrix, whether the
matrix reaches hit testing, and what a parent's transform does to a child's
centre. A lead, not a finding: the callers of the matrix multiply `0x0042EDB0`
in `0x0090xxxx..0x009Fxxxx` are the two transform builders (`0x00990D91`,
`0x00990DDD`, `0x00990F0F`, `0x00990F5B`) and the references to the
`VisualTransform` descriptor are `0x0094026E`, `0x00940634`, its registration
thunk and a data word. A reader that uses other math or reads the matrix
elements directly would not appear in either list, so where the matrix is
consumed is UNKNOWN.

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
table at element `+0xB4`, and then stores only when `byte & 3` is nonzero and the
returned byte differs from bit 5 (`TEST AL,3` at `0x0093D4D0`, `SHR CL,5` at
`0x0093D4D6`), notifying the property system through `0x0099C680`
(`0x0093D4FE`). `0x0093D510`, `0x0093D5A0` and
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
parent word at `+0x60`, and false otherwise. STATIC for the instructions. That
the global holds the anchor the property name suggests is INFERRED from the walk.
The base table is the only one read. Whether a derived class overrides slot
`+0x48` is UNKNOWN.

The value is therefore derived and not a constant. It reads false while
`0x01357020` is null or the gate fails, and it can be true only when the global
names the element or a descendant. The writes to `0x01357020` are at `0x0091DC8D`
in `0x0091DBD0` and at `0x0091E75C` in `0x0091E3E0`, and the writes to
`0x01356F34` are at `0x0091BFA5` in `0x0091BF40` and at `0x0091BEE9` in
`0x0091BE10`. Which events run them, and what the byte `+0x158` of the
singleton holds in a session with no input, are UNKNOWN.

## Surviving gaps

- The reader of `VisualTransform` and the composition of the matrix with the
  element's layout position.
- Hit testing under a transform.
- The full caller and notification conditions for the separate centre update at `0x0093BBFF`.
- The events that run the `IsMouseOver` setters `0x0093A410` and `0x0093A480`.
- The writers of the anchor global `0x01357020` and of the gate global
  `0x01356F34`, and the value of the singleton byte `+0x158` in a session with
  no input.
- Whether a derived `UIElement` class overrides slot `+0x48` of the `+0xB4` table.
- The relative order of the trigger sets of a `Window.Template` and a
  `Window.Style` on one root.
