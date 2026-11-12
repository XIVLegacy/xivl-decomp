# Sqwt NumberInputBox button frames

The stock quantity arrows and limit buttons use ordinary Button Frame
skins. The selected skin supplies both a layout minimum and paint overhang.
Pointer containment uses the arranged control box and Visual clip.

## Input and method

Target is FINAL FANTASY XIV 1.23b build 2012.09.19.0001, `ffxivgame.exe`,
PE32 i386, base `0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Evidence IDs NI-FRAME-MEASURE, NI-FRAME-PUBLISH,
NI-FRAME-DRAW, NI-FRAME-POINT and NI-FRAME-CLIP are STATIC.
NI-FRAME-ASSET is decoded
client data. Ghidra CLI and Apple LLVM 21.0.0 provide the checked
instructions. PE section-backed reads provide the table cells and strings.
The asset inputs and digests are in [the digit grid and assets](sqwt-number-input-digit-grid.md).
Recheck exact SQEX re-encryption and inspect only the cited metadata.
The executable was not run.

## Layout minimum

The Button constructor at `0x0097444E..0x0097445E` installs primary
vtable `0x01073CAC` and InputElement table `0x01073B84`. RepeatButton
at `0x00998902..0x00998912` installs `0x01079924` and `0x010797FC`.
Both inherit FrameworkElement. Its constructor sets bit `0x20` at
`+0x250` (`0x0092FA0D..0x0092FA0F`). The selected NumberInputBox
constructors set local names and use Button as the style group for
both RepeatButton arrows. See the digit grid page for their placement.

A successful `0x0092A65E` style lookup reaches primary virtual `+0xEC`
at `0x0092A66B..0x0092A67A`. Both tables select `0x00938740`
(cells `0x01073D98` and `0x01079A10`). That callback reads the resolved
skin's BorderThickness separately and calls `0x00931720` at
`0x009388E4`. `0x00931778` tests the initialized `+0x250` bit before
iterating up to 14 populated style slots. The first nonzero HitMargin
tuple is retained at `0x009317A8..0x0093183A`, and a size accumulator
receives the resolved skin via `0x0093184D`. The admitted 1x1 cell
branch at `0x0092C4D7..0x0092C4F4` computes source right minus left and
bottom minus top. `0x0092C506..0x0092C51F` passes the result through
`0x009297C0`, which takes the maximum separately for each dimension.

At `0x00931869` and `0x00931875`, the accumulator width/height and the
HitMargin tuple go to `0x00929C40` and `0x00929D00`. They retain raw
source size at `+0x280/+0x284` and raw margins at `+0x270..+0x27C`.
`0x00929C75..0x00929CEC` and `0x00929D24..0x00929D97` store the
following float32 pair at `+0x288/+0x28C` using the virtual `+0xC8`
scale `S`:

```text
((sourceWidth - L - R) * S, (sourceHeight - T - B) * S)
```

For both buttons, `+0xC8` is `0x0092B600`, which delegates to its
logical parent when present. The parentless fallback at `0x012E035C`
is float32 `0.800000011920929`. The layout scale for a particular live
window is outside this trace. The measure wrapper `0x0092B880` takes
`+0x9C` at `0x0092BA07..0x0092BA15`. Both button tables supply
`0x008E92C0`, which returns the `+0x288` pair for the lower-bound
calculation. The bound enters measure and the arrange wrapper `0x0092FD20`. The
arranged box also depends on the available size and alignments.

At scale 1 the installed Frame styles all use 20x20 sources.

| Button style in `common/default.style` | Lines | HitMargin | Layout minimum |
| --- | --- | --- | --- |
| `NIB_basis_buttonUp` | 1361-1370 | `2,4,2,2` | 16x14 |
| `NIB_basis_buttonDown` | 1373-1382 | `2,2,2,4` | 16x14 |
| `NIB_basis_buttonMax` | 1385-1394 | `2` | 16x16 |
| `NIB_basis_buttonMin` | 1397-1406 | `2` | 16x16 |

The Normal and Disabled skin names append `_N` to the style. The Hover,
Focused and Active names append `_H`. The scalar HitMargin path
`0x009A7D0F..0x009A7D2A` copies the parsed number to all four lanes. The eight N/H variants at
`common/default.skin` lines 703-725 omit SkinMargin. Its parser
initializes the separate four lanes to zero at `0x009A7D41..0x009A7D62`.

## Conditional Frame publication

The [ordinary control peer factory](sqwt-icon-control-textures.md#observed-ordinary-control-to-renderer)
selects the same DrawingToolContext for Button and RepeatButton when its
allocation, interface and window-context conditions hold. Both button
primary tables use `0x0093EF90` at `+0x54`. On an existing admitted peer,
FrameworkElement property callback `0x0092DAB0` sends the inherited style
file, style name and explicit style group through distinct slots:

| Property | Native callback locators | DrawingToolContext target | Stage and local operation |
| --- | --- | --- | --- |
| SqwtStyleFile | `0x0092DE1E..0x0092DE2E` | `+0x54 -> 0x0055A170` | `+0x150`, 7 at `0x0054ED11` |
| SqwtStyle | `0x0092DDB6..0x0092DDCD` | `+0x58 -> 0x0055A1C0` | `+0x1B4`, 10 at `0x0054ED41` |
| SqwtStyleControlName | `0x0092DD1E..0x0092DD35` | `+0x5C -> 0x0055A210` | `+0xEC`, 6 at `0x0054ECE1` |

WindowContext dispatcher table `0x008CF3B8` maps operation 7 to
`0x008CEF4D`, operation 10 to `0x008CEFF0`, and operation 6 to
`0x008CEDF6`. A successful nonempty style-file result at
`0x008CEF8D..0x008CEFD4` selects `0x008CCF90` and supplies the style file
through `0x008E1080`. The factory installs RenderSkin (table
`0x01066934`) in main renderer slot `+0xC4` at `0x008CD092..0x008CD098`.
Operations 10 and 6 retain the style name and group, forwarding to a
nonnull renderer. `0x008E2A50` calls the same style resolver
`0x00934210` at `0x008E2B54`. This establishes an admitted ordinary
Frame path. It leaves initial peer synchronization order and particular
live instance admission open.

The ActualWidth/ActualHeight change branch builds a local rectangle
with zero origin at `0x0092DBB5..0x0092DC01`. Setter `0x00940560`
writes its property wrapper at Visual `+0x88`. The peer forwards this
rectangle through `0x0093FEA9..0x0093FEBB` or its dirty-state
initialization `0x0093F999..0x0093F9B4`. DrawingToolContext slot
`+0x50` is `0x0055A120`, which stages its four floats at `+0x80`. Local operation 2
at `0x0054ECB1` carries that record to `0x008CED9B`. `0x008CC2D0`
truncates the finite floats and stores them at WindowContext
`+0xF0..+0xFC`. Visual position is a separate peer property (`+0x34`
through `0x0093FE45..0x0093FE54`).

## Explicit Label Frame condition

`SqwtSkinStatus` supplies the ordinary Frame's condition index. In the same
pinned PE, descriptor initializer `0x00F22050` names it through
`0x0106A9C4` and sets descriptor `0x013589A0` ordinal `0x12`.
The attribute arm `0x009308B1..0x009308D2` passes the holder at
`+0x210` to `0x0095CB50`. The ordinary literal path calls
`0x00988A30` for integer parsing and `0x00926E80` for storage.
A changed integer sets holder flag `0x04` at `+0x214`.
On an existing peer, case `0x12` of `0x0092DAB0` forwards the
integer through the peer's `+0x78` (`0x0092DC9E..0x0092DCC7`).

Label RTTI `.?AVLabel@Controls@Sqwt@@` at `0x0126D200` has a complete
object locator referenced at `0x010738B0`. The constructor installs
primary table `0x010738B4` at `0x009742DE`. Peer acquisition uses
`+0x54 -> 0x0093EF90` at `0x0093F8B3..0x0093F8BC`.
The new-peer replay at `0x0093FADD` calls Label slot
`+0x50 -> 0x00937120`. It reaches `0x0092A440`, where a nonempty
style control name and holder flag `0x04` admit the retained
`+0x210` integer to peer `+0x78` at `0x0092A57B..0x0092A591`.

DrawingToolContext table `0x00FA28E4` resolves that slot to `0x0055A3A0`.
It stages the dword at cached record `+0xE0` and sets the dirty bytes
`+0xE8` and `+0x5C`. The subrecord at `+0xDC` is initialized for local
operation 9 at `0x0054D424..0x0054D437`. Flush `0x0054EBCE..0x0054EBF7`
calls its table `0x00FA2C74` slot `+4 -> 0x005563E0`, which sends the dirty
dword through the ordinary command helper at `0x00556401`.
WindowContext operation 9 (table cell `0x008CF3DC`) retains the value at
`+0x124` and forwards it to an existing RenderSkin (`0x008CEE63..0x008CEE79`).
The RenderSkin factory replays the same retained value at
`0x008CD03C..0x008CD049`. Setter `0x008E1880` stores condition at `+0xE8`.
Style group is a separate value at `+0xE4`. With an admitted style row, value
1 selects slot 1, SkinActive. SkinHover is slot 4.
[RenderSkin row selection](action-menu-timer-icon-style.md)

Installed `client/sqwt/widget/ItemUseWidget.form` line 96 assigns
`SqwtSkinStatus=1` and `SqwtStyle=LAB_winBasis` to `Label_ItemUseWindow`.
The form root selects `common/default.style`, where the Label style's Frame
maps SkinActive to `WIN_basis_H` and SkinNormal to `WIN_basis_N` (lines
1989-1993). In `common/default.skin` lines 2083-2089, the active merged skin
references gradient `WIN_basis_bg1_H`, tiled `WIN_basis_bg2_N`, and 3x3
`WIN_basis_frame_H`, in that order. The installed form is 10627 bytes,
SHA-256 `35f88233e405f4fd6faa199f2bbd95106c224375091812c5fe7bf4d03390b7fd`.
The decoded document is 10619 bytes, SHA-256
`0fc0f34f2124dd3fff6743ae1a65de5502dfa4061f847dffd9d336a816b83a12`.
The pinned style and skin identities are in the installed assets table on the
[digit grid page](sqwt-number-input-digit-grid.md#installed-assets).

Use LLVM objdump 21.0.0 and PE section-backed dword reads at the cited VAs.
Decode the three named assets with `xivl extract-resource <input> --output
<scratch> --materialize-payloads` and verify the SQEX round trip.
This establishes the selected Frame for the retained literal on an admitted
peer. Automatic status selection at `0x00930C20` can write the same
holder under its own conditions. Later effective status and live paint
timing remain unverified.

## Painted dimensions and point containment

WindowContext update copies `+0xF0..+0xFC` at
`0x008CDBCB..0x008CDBF6`, then calls each active renderer's `+0x0C`
with that record at `0x008CDC20..0x008CDC4D`. RenderSkin table cell
`0x01066940` selects `0x008E2CA0`, whose nonnull selected row reaches
`0x008E1540` at `0x008E2CFE`. This preparation stores the incoming
rectangle at renderer `+0x48..+0x54`. It reads the selected skin record's
HitMargin (`+0x04..+0x10`) at `0x008E15C7..0x008E15D3`, subtracts L/T
from x/y and adds L+R/T+B to width/height at
`0x008E15D5..0x008E1652`. It uses signed style offsets at
`0x008E1657..0x008E169E` in the separate origin transform, with
renderer `+0xC8/+0xCC` origin terms. The Active NIB style requests
`(1,1)`, while the Normal row carries no offset declaration. After
storing that transform it clears the renderer's local x/y at
`0x008E1741..0x008E174B`.

The active draw path at `0x008CEA7C..0x008CEAA8` calls renderer `+4`.
RenderSkin `+4` is `0x008E2670`, a thunk to `0x008E2450` that applies
the stored transform and calls the cell renderer `0x008E20F0` at
`0x008E261B` when drawing is enabled. `0x008E2151..0x008E21C3` adds
SkinMargin left/top and subtracts its two summed dimensions. Both
sums are zero for the eight selected skins. The 1x1 branch at
`0x008E240F..0x008E2427` calls `0x008E0930`. The cell parser identifies
Stretch string `0x0107C4F4` with mode zero (`0x009A84A4..0x009A84B9`).
Mode zero keeps the destination width/height in `0x008E0930`, with
the source bounds passed separately to `0x008E00F0`. This is the
stretched draw route through `0x00CA58A0`. Mode 2 replaces those
destination dimensions with the source span.

For a WindowContext rectangle of size `w,h` after the float-to-integer
truncation, the selected stock frame
therefore submits a rectangle of size `(w + L + R, h + T + B)` before
transform and clipping, with an overhang starting L pixels left and T
pixels above the box before the separate origin terms and style offset.
If arranged at the skin minimum at scale 1, each of the four stock
frames submits 20x20. This is a conditional result from the asset and
instructions. A larger arranged box stretches the selected source.

Pointer selection is distinct. Both InputElement tables' `+0x58`
entries select `0x00939B10` (cells `0x01073BDC`, `0x01079854`). It
localizes the point by Visual offsets and calls `0x0097E1A0` at
`0x00939B64`. The descent at `0x0097DE40` carries the Visual clip and
calls the leaf's primary `+0x38`. Both button tables select
`0x0092B640` (cells `0x01073CE4`, `0x0107995C`). At
`0x0092B70A..0x0092B79A` it takes local origin zero, actual width
`+0x1B4`, actual height `+0x1AC`, and intersects the positive Visual
clip from `+0x68` before the point predicate `0x00926D40`. The predicate
has inclusive left/top and exclusive right/bottom for finite ordinary
rectangles. This local hit branch uses the arranged box without the
paint overhang. HitMargin still participates indirectly through its
layout minimum. Higher-level target, capture, visibility, repeat,
and activation state remain separate.

## Hand-in viewport evidence

`widget/EventItemSelectWidget.form` lines 35-37 wrap the row StackPanel
in a visible ScrollViewer with Height `490`, Margin `0,0,2,2`, and
vertical mode Auto. The 16 row declarations include 32x32 icon boxes
at lines 47-48 through 332-333. This bounds the logical viewport and
shows why a stack can exceed it. The [ScrollContentPresenter layout](sqwt-tab-scroll-text-parts.md#layout-rules)
retains separate viewport and extent values.

The ScrollViewer constructs its embedded ScrollContentPresenter at
`+0x760` (`0x009733F6..0x00973404`). Its base ContentPresenter
constructor is called at `0x009C327A` and requests ClipToBounds true
at `0x0099D7D0..0x0099D7D8`. The setter `0x0093C660` supplies
`+0x15E` and descriptor `0x01359620`, whose initializer at
`0x00F22AA0..0x00F22AAF` names ClipToBounds. For a RenderSize change,
`0x0093BBA5..0x0093BBD8` passes the current pair at `+0x14C/+0x150`
to virtual `+0x8C`. ScrollContentPresenter table cell `0x01082A30`
reaches `0x00939BD0` through `0x0099C880` and `0x0093A3A0`.
With the ClipToBounds bit set it builds a local clip of `(0,0,max(1,w),
max(1,h))` and calls `0x00940440`, whose wrapper is Visual `+0x68`.

An admitted peer forwards that Clip through `0x0093FE6E..0x0093FE86`
to DrawingToolContext slot `+0x4C` (`0x0055A0E0`). Its staging at
`+0x44..+0x50` flushes via local operation 4 (`0x0054EBB1`). The
WindowContext arm at `0x008CEDBB` forwards to `0x008CD6D0`, which
passes its four lanes to `0x008CD110` for the local clip state.
WindowContext draw conditionally sends its computed clip at `+0x58`
through `0x008CBEF0` (`0x008CE832..0x008CE838`), with a scissor
request at `0x008CBF15..0x008CBF2C` for the enabled bounded case.
This identifies a native viewport clip producer and scissor consumer.
The exact initial retail row cut, computed scissor for this live
form, screen positions, render order against other children and
output at non-unit scale remain unverified.
