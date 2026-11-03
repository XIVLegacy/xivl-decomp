# Sqwt render transform

A UI element holds a `RenderTransform` pointer, a `RenderTransformOrigin` pair
and a `RenderSize` pair. The markup forms that name a `ScaleTransform` write
one cached default transform of the element, whose centre is the origin times
the render size and whose factors default to one. The markup attributes
`CenterX` and `CenterY` have no writer. This page records those joins, the
matrix the transform builds, and the `IsAnchorWithin` default. It does not
establish how a drawn element composes the matrix with its layout offset, nor
whether the matrix reaches hit testing.

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

The defaults are the `FLDZ` stores at `0x0093C865` and the zero pushes that
precede each constructor call. STATIC. The origin is a pair of floats with no
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

The code at `0x0093BAE2..0x0093BB8B` and `0x0093BBFF..` multiplies the same pair
again into the cached transforms' centres when it runs, and then re-applies the
transform pointer at `+0x138` through `0x00940260` when it is non-null
(`0x0093BB76..0x0093BB8B`). Which event runs this block is not traced, so the
claim that the centre follows a size change is INFERRED from the block's use of
`+0x14C` and `+0x150`.

## Markup routes

| Route | Evidence |
|---|---|
| The string value `ScaleTransform` written to `RenderTransform` selects the element's cached transform. The value test is `0x00445D60` against the literal at `0x0106C218` (`0x0093D1D2..0x0093D1D9`), then `0x00939CB0` and the `RenderTransform` setter `0x0093C540` with the cached object (`0x0093D1E4..0x0093D1F0`). The literal `RotateTransform` at `0x0106C228` selects the rotate cache the same way | STATIC |
| An element whose name ends in `.RenderTransform` (the suffix literal `0x0106D4F4`, tested at `0x0094BB3F..0x0094BB56` in `0x0094AD90`) with a non-null owner constructs a `Sqwt::Markup::RenderTransformMarkupObject` (vtable `0x0106DBCC`) through `0x0094F530`. Its slot 2 `0x0094F750` writes its input value to the `RenderTransform` descriptor through the generic setter `0x00912850`, then constructs a `TransformMarkupObject` (`0x0094F5B0`) that holds the owner element at `+0x74` | STATIC |
| An attribute of that child, such as `ScaleX`, goes to slot 3 of the `TransformMarkupObject` vtable `0x0106DBF0`, the handler at `0x0094F640` (the data word `0x0106DBFC`). The `MarkupObject` base takes its attribute handler from the same slot (see [Sqwt qualified property lookup](sqwt-qualified-property-lookup.md)). It prefixes the attribute name with `RenderTransform.` (literal `0x0106DC10`) and calls the generic setter on the owner element | STATIC |
| The qualified names `RenderTransform.ScaleX`, `RenderTransform.ScaleY` and `RenderTransform.AngleZ` (descriptors `0x0135FD88`, `0x0135FDD0`, `0x0135FE18`, see [Sqwt qualified property lookup](sqwt-qualified-property-lookup.md)) are handled by `0x0093BD80`. For ScaleX it reads the value, calls `0x00939CB0` for the cached transform and writes `0x00990B10`. For ScaleY it writes `0x00990B40`. For AngleZ it calls `0x00939D70` and writes `0x00990C10`. It then re-applies `+0x138` through `0x00940260` when that pointer is non-null (`0x0093BE44`, `0x0093BF66`, `0x0093BFC9`) | STATIC |
| `CenterX` and `CenterY` have no writer. The executable holds no `CenterX` or `CenterY` literal, ASCII or UTF-16, by a byte search over the whole image, and `ScaleX`, `ScaleY` and `AngleZ` occur only inside the three qualified literals. The centre setters `0x00990B70`, `0x00990BA0`, `0x00990C40` and `0x00990C70` have direct callers only in `0x00939CB0`, `0x00939D70` and the block at `0x0093BAE2` | STATIC for the search results. That a markup `CenterX` attribute leaves the centre unchanged is INFERRED from the missing registration, since a key built by concatenation would need the literal |

The authored `CenterX="0.5"` and `CenterY="0.5"` of an installed template
therefore do not set the centre. The centre of such a transform is
`RenderTransformOrigin * RenderSize`. INFERRED.

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
centre. The only multipliers of the matrix type in the `0x0090xxxx..0x009Fxxxx`
range are the two transform builders (`0x00990CA0`, `0x00990E20`), so the
consumers are outside the UI layer. UNKNOWN.

## IsAnchorWithin

`IsAnchorWithin` is registered by the thunk `0x00F22AF0` (name literal
`0x0106BF80`, descriptor `0x013594FC`, index `0x10` at `0x01359510`). Its value
is bit 5 of the element byte at `+0x160`. The `UIElement` constructor clears
that byte's low six bits before setting two flag bits from the descriptor
metadata (`param_1 + 0x58` in `0x0093C750`), so the value starts false. STATIC.

The refresh `0x0093D490` calls the virtual at slot `+0x48` of the vftable at
element `+0xB4`, and when its byte differs from bit 5 it stores the new value
and notifies the property system (`0x0093D4D2..`). The same refresh is
entered at `0x0093D510`, `0x0093D5A0` and `0x0093D630` with the element address
recovered by `LEA EDI,[EBP-0xB4]`, so the virtual is one slot of the vftable at
element `+0` and of the one at `+0xB4`. What the virtual returns for an
element in a session with no anchor, and when the refresh runs, are UNKNOWN.

## Surviving gaps

- The reader of `VisualTransform` and the composition of the matrix with the
  element's layout position.
- Hit testing under a transform.
- The event that runs the size-driven centre update at `0x0093BAE2`.
- The default of `IsMouseOver` (its registration thunk is `0x00F22AC0`, name
  literal `0x0106BF74`) and the value of the `IsAnchorWithin` virtual in a
  session with no anchor.
- The relative order of the trigger sets of a `Window.Template` and a
  `Window.Style` on one root.
