# MiniMap direction properties and view cone

The `MapScreenControl` of the [MiniMap base tiles](minimap-base-tiles.md)
page carries a north flag, a north label template and four fixed images.
With the north flag set the control writes no rotation. After the base
tiles it draws a view cone image turned by a camera angle and offset by
the camera distance. Control offsets on this page are offsets of that
control object.

## Inputs and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI disassembly
supplied the instructions. Reads of the file-backed PE sections supplied
the string, f32 and binary64 constants and one displacement scan. The
asset checks use `xivl inspect` and `xivl extract-resource` on the same
build. No target process ran.

The decoded markup inputs are `client/sqwt/widget/MiniMapWidget.form`,
2943 bytes, SHA-256
`83428fa59fa9d26951c1c646d40b87fbbfd9ea0b537ae30e3af4b1ff36560360`, and
`client/sqwt/widget/MiniMapWidget.tpl`, 760 bytes, SHA-256
`f0d692c82247d8c646c111ba8b3859f61ef02d6d99ba0dff5d8ad092ce7c335f`.

## Direction properties

STATIC. Each initializer pushes the name literal, loads the descriptor and
stores the tag in the descriptor's dword at `+0x14`.

| Property | Name literal | Initializer | Descriptor | Tag | Holder and setter arm |
| --- | --- | --- | --- | --- | --- |
| `IsDisplayGrid` | `0x00FC25C0` | `0x00F11B10` | `0x0133FF7C` | `0x09` | byte `+0x48C`, arm `0x00673C57..0x00673C78` |
| `IsNorth` | `0x00FC25D0` | `0x00F11B40` | `0x0133FFA0` | `0x0A` | byte `+0x48D`, arm `0x00673C81..0x00673CA2` |
| `MarkerCenterControlName` | `0x00FC2744` | `0x00F11DE0` | `0x01340270` | `0x1E` | `+0x528`, arm `0x0067405D..0x0067407E` |
| `NorthTemplateName` | `0x00FC2784` | `0x00F11E40` | `0x013402DC` | `0x21` | `+0x534`, arm `0x006740DB..0x006740FC` |
| `DirectionRadiusRatio` | `0x00FC27D4` | `0x00F11EC0` | `0x0134036C` | `0x25` | `+0x554`, constructor `0x0067A61A..0x0067A638` |

The two Boolean arms pass their holder byte to `0x00596A80`. That routine
reads the held value as bit `0x20` of the byte at
`0x00596ABC..0x00596AC6`. The two string arms pass their holder to
`0x00684EA0`.

The constructor `0x00679ED0` clears the low five bits of byte `+0x48D` and
sets bit `0x20` at `0x0067A1A1..0x0067A1AC`, before it merges the
descriptor metadata bits into that byte. `IsNorth` therefore starts true.
The constructor loads the f32 at `0x00FB9310`, which is 0.699999988079071,
as the initial `DirectionRadiusRatio`.

MEASURED. The form declares at line 19 `IsNorth="True"`,
`NorthTemplateName="ControlTemplate_ContentControl_NorthText"`,
`DirectionRadiusRatio="0.7"` and
`MarkerCenterControlName="SparkleControl_MapMarker"`.

The consumer of `DirectionRadiusRatio` was not traced.

## North flag and the two rotations

STATIC. The update `0x00679940`, called from `0x00679EC0`, first calls
`0x006780D0`. At `0x0067994B..0x00679952` it tests bit `0x20` of byte
`+0x48D`. With the bit set it skips to `0x00679962`. With the bit clear it
calls `0x006729A0` and then `0x006728A0`.

`0x006729A0` turns the control:

| Locator | Operation |
| --- | --- |
| `0x006729A5..0x006729BD` | Read the f32 at `+0x124` of the object at control `+0x9DC`. Multiply by the binary64 180.0 at `0x00F62F40`, divide by the binary64 3.1415927410125732 at `0x00F62F48` and store f32. |
| `0x006729C1..0x006729D0` | Pass that value and the binary64 360.0 at `0x00FC3300` to the x87 helper `0x009DAFBA`. |
| `0x006729E6..0x006729F1` | Add 360.0 when the helper result is below zero. |
| `0x00672A01..0x00672A1B` | Add 180.0 and pass the f32 result to `0x00990C10` on the object at control `+0x5B8`. |
| `0x00672A20..0x00672A63` | Pass control `+0x14C` times the binary64 0.5 at `0x00F59898` to `0x00990C40`, and control `+0x150` times 0.5 to `0x00990C70`, on the same object. |
| `0x00672A68..0x00672A6F` | Call `0x00940260` on the control with that object, 0 and 1. |

The three setters store their argument at `+0x0C`, `+0x10` and `+0x14` of
the object and clear its byte `+0x18` on a change. Those are the AngleZ,
CenterX and CenterY fields and the valid byte of the
[RotateTransform](sqwt-render-transform.md#rotatetransform-matrix).

`0x006728A0` turns a child element back. It returns at once when the
pointer at control `+0x9E4` is null (`0x006728A5..0x006728AD`). Otherwise
it computes the same degrees, including the 180.0 addition, and subtracts
them from the binary64 negative zero at `0x00F63008`
(`0x00672907..0x0067292B`). It passes that negated angle to `0x00990C10`
on the object at control `+0x5D4`, passes the element's own `+0x14C` and
`+0x150` times 0.5 to `0x00990C40` and `0x00990C70`
(`0x0067293B..0x0067297E`), and calls `0x00940260` on the element
(`0x00672983..0x0067298E`).

INFERRED. With `IsNorth` false the control turns about its centre with the
angle at `+0x124` of the object at `+0x9DC`, and the element at `+0x9E4`
turns by the opposite angle about its own centre, so it stays upright.
`0x009DAFBA` is read as a floating remainder from its operands and the
sign correction that follows. With `IsNorth` true neither transform is
written. The stock form sets it true, so the stock map stays north up
unless a later writer clears the property. No such writer was looked for
in the shipped scripts.

## North label element

STATIC. The activation routine `0x00672690` tests the string holder
`+0x534` at `0x00672774..0x00672788` and calls `0x006713A0` when it holds a
nonempty string.

`0x006713A0` passes the held name, or the literal at `0x01266B10` when the
holder is null, to the control's virtual `+0xB0` (`0x006713A3..0x006713BC`)
and passes the result to `0x0096B500`. On a nonnull element it calls
`0x00940B30` on the control with the element, sets mask `0x02` in the
element byte `+0xB0` at `0x006713D5` and calls `0x009313C0` on the control
with the element. On a null element it stores 0 at control `+0x9E4` at
`0x006713E8`.

A scan of the code section for the displacement bytes `E4 09 00 00` finds
four instructions that address control `+0x9E4`: the constructor's zero
store at `0x0067A99D`, the zero store at `0x006713E8` and the two reads
in `0x006728A0`. The two other matches are a branch displacement at
`0x009EAA17` and a stack local at `0x00D8C4B9`. No instruction found by
this scan stores a nonnull element at `+0x9E4`.

MEASURED. The template file holds one template,
`ControlTemplate_ContentControl_NorthText`, for a `ContentControl`. It
sets `Focusable` and `IsHitTestVisible` false and `HorizontalAlignment`
`Center`, and holds one `TextBlock` named `TextBlock_North` with
`Text="N"`, `FontSize="16"`, `FontStyle="Italic"`,
`HorizontalContentAlignment="Center"`, `FontEdge="True"`,
`FontEdgeColor="#ff1a1a1a"`, `Foreground="#ffe57a45"`,
`SqwtStyle="TBL_mapNorth"`, `Margin="0,40,0,0"` and
`VerticalAlignment="Top"`.

INFERRED. The named template is instantiated and attached to the control
as a child. The arranged rectangle of that child is UNKNOWN. A child that
fills the control would place the letter 40 units below the control's top
edge, centred on its width. Since the scan finds no nonnull store at
`+0x9E4`, the counter rotation of `0x006728A0` has no traced input, and a
writer that reaches the field another way is not excluded.

## Fixed images

STATIC. `0x00672690` calls `0x00991090` on four embedded images with fixed
numeric IDs at `0x006726B4..0x006726EF`.

| Image holder | ID | Asset (MEASURED) |
| --- | --- | --- |
| `+0x790` | `0x61C10003` | `data/61/C1/00/03.DAT`, 81968 bytes, 128 by 128 GTEX, two mip levels |
| `+0x67C` | `0x61C10005` | the [fallback tile](minimap-base-tiles.md#viewport-and-draw) |
| `+0x6D8` | `0x61C10007` | `data/61/C1/00/07.DAT`, 262176 bytes, 256 by 256 GTEX, one mip level |
| `+0x734` | `0x61C10008` | `data/61/C1/00/08.DAT`, 262176 bytes, 256 by 256 GTEX, one mip level |

SHA-256 of `03`, `07` and `08`:
`c574e7b31018cf64d3ab839de1c59ce36ef37bfe819830e4c8f9e4fe0a8a0b83`,
`03b8896b4d895aa146d1d6e8d00a00cf95c3a53ed1052950fdb4903e6454062c` and
`51e5694791afe70b7a75d98b02fd3dbae4b0ba76429b4877e9aff6522356fcd1`. All
three report format index 4.

MEASURED on the PNG previews that `xivl extract-resource --preview-png`
writes. In `07` and `08` the texels with alpha above 8 lie inside columns
117 to 232 and rows 23 to 138. The shape is a quarter fan that opens from
its lower left corner toward larger x and smaller y. `07` is red and `08`
is yellow.

## View cone draw

STATIC. The draw `0x00676B00` pops its second transform through context
slot `+0x18` at `0x0067709E..0x006770A5`, after the base tile loop. At
`0x00677DCF..0x00677DE4` it calls `0x00672060` with the drawing context
when `Function` (`+0x448`) is nonzero and the actor pointer at `+0x9E0`
is nonnull. That call is the only reference to `0x00672060`. The transform
stack then holds the first push of the draw, the
[translation by half the control size](minimap-base-tiles.md#base-tile-local-transform-requests).

In `0x00672060`:

| Locator | Operation |
| --- | --- |
| `0x00672094..0x006720CB` | From the object at control `+0x9DC`, read the f32 at `+0x124` and the two qwords at `+0x110` and `+0x118`, a four lane f32 vector. |
| `0x006720D1` | Call the [world-position getter](../net/target-cycle-and-pick.md#range) `0x004CFE30` with selector 1 on the actor at `+0x9E0`. |
| `0x006720D6..0x0067213A` | `d` is the f32 square root of the squared differences of lanes 0 and 2 between that vector and the actor position. |
| `0x0067212A..0x0067215F` | When the binary64 0.20000000298023224 at `0x00FC3368` is above `d`, `d` becomes the f32 at `0x00FC24C8`, the same value. Otherwise `d` above the f32 10.0 at `0x00FC24C0` becomes 10.0. |
| `0x00672162..0x006721AA` | `t = f32((d - 1.5) / 9.799999997019768) * 20.0`, `a = f32(t + 138.0)`, `b = f32(118.0 - t)`. The binary64 constants are at `0x00F62F98`, `0x00FC3360`, `0x00FB6E90`, `0x00FC3358` and `0x00FC3350`. |
| `0x006721AE..0x006721F6` | Build the translation by `(-a, -b, 0)` with `0x00430430`. |
| `0x006721FB..0x0067224C` | Build the translation by `(a, b, 0)` and multiply with `0x0042EDB0`, the first matrix as receiver. |
| `0x00672255..0x006722F0` | Build the Z rotation by `f32(2.35619455575943 - angle)` with `0x0042FEB0` and multiply it in. The binary64 constant is at `0x00FC3348` and `angle` is the `+0x124` value. |
| `0x0067235B..0x006723AC` | Build the translation by `(-a, -b, 0)` again and multiply it in. |
| `0x00672473..0x00672485` | Push the result through context slot `+0x14`. |
| `0x00672487..0x006724D7` | Build two rectangles from 0 and the f32 256.0 at `0x00FC3340`. |
| `0x006724DF..0x0067255D` | A fraction `f` is the 64 bit tick from `0x00555B20` minus the tick at control `+0xA60/+0xA64`, over the f32 500.0 at `0x012BAAFC`. It is 1.0 when 500.0 is not above the difference. |
| `0x00672564..0x00672631` | With bit `0x40` of byte `+0xA69` set, draw the image at `+0x6D8` through context slot `+0x1C` with `f`, then, while 1.0 is above `f`, the image at `+0x734` with `1 - f`. With the bit clear the first draw uses `+0x734` and the second `+0x6D8`. |
| `0x00672633..0x0067263A` | Pop the transform through context slot `+0x18`. |

The [multiplication helper](vfx-root-matrix-mode-zero.md#conditional-equation-and-simd-boundary)
computes its stack argument times its receiver, and the
[rotation builder](sqwt-render-transform.md#rotatetransform-matrix) writes
rows `(cos, sin, 0, 0)` and `(-sin, cos, 0, 0)`. In the row-vector
convention of those pages the pushed matrix is
`T(-a,-b) * R * T(a,b) * T(-a,-b)`, which equals `T(-a,-b) * R`.

The update at `0x00679BAB..0x00679BF9` owns bit `0x40` of `+0xA69`. With
nonzero `Function` and a nonnull actor at `+0x9E0`, `0x00585400` returns
the actor dword at `+0xF0`. A value of 2 with the bit clear sets the bit.
Any other value with the bit set clears it. Each change stores the tick
from `0x00555B20` at `+0xA60/+0xA64`. The constructor stores the result of
`0x004D7320`, the dword at `+0x4C4` of its argument object, at control
`+0x9DC` (`0x0067A97E..0x0067A991`).

INFERRED:

- An image point `p` lands on `R(p - (a, b))` from the control centre. The
  pivot `(a, b)` runs from about `(135, 121)` at the smallest distance to
  about `(155, 101)` at the largest, along the diagonal the fan opens on.
- The object at `+0x9DC` is the active camera record, with a position at
  `+0x110` and a yaw at `+0x124`, and `d` is the horizontal distance from
  the camera to the player actor.
- The image is drawn with a 500 millisecond cross fade between the red and
  the yellow fan when the actor dword at `+0xF0` enters or leaves 2.
- The draw is the unmasked `DrawImage` request inside the viewport clip
  that the draw pushed at `0x00676C71`.

UNKNOWN: the meaning of actor `+0xF0` value 2, the zero direction and sign
of the `+0x124` angle, and the screen direction the fan takes for a given
yaw. The tick unit of `0x00555B20` is not established on this page.

## Grid pass

STATIC. After the pop at `0x006770A5` the draw tests bit `0x20` of byte
`+0x48C`, the `IsDisplayGrid` holder, at `0x006770A7..0x006770AE` and
skips to `0x00677D99` when it is clear. The gated pass loads the image at
`+0x790` at `0x006771A6` and formats labels with the `%d` literals at
`0x00FC29B4` and `0x00FC29B8`. The pass body was not walked further. The
stock form declares no `IsDisplayGrid` value.

## Consumer implications

- A client may keep the stock MiniMap north up and write no control
  rotation while `IsNorth` is true.
- The north label may use the measured template values. Its placement
  inside the control is CALIBRATION.
- The view cone may use images `0x61C10008` and `0x61C10007`, the pivot
  and angle formulas above and the 0.2 to 10.0 distance clamp. The mapping
  of a client camera to the `+0x110` vector and the `+0x124` angle, and
  the condition for the red image, are CALIBRATION.
