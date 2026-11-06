# Sqwt TimerIcon Percent

`Sqwt::Controls::TimerIcon` forwards its `Percent` property to a renderer
through local command `0x16`. On the `RenderTimerIcon` path, the value
controls the geometry of a textured overlay drawn after the ordinary icon.

## Input and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86 little endian, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`.

Read-only ghidra-cli disassembly, cross-references, and data queries identify
the instruction and table locators. Selected instructions and table entries
match the original PE section mappings. LLVM objdump 21.0.0 independently
decodes the property comparison at `0x009BF513..0x009BF548` and the renderer
arithmetic at `0x008E51E5..0x008E5223`.

Original RTTI names the control `.?AVTimerIcon@Controls@Sqwt@@` at
`0x012E8F50` and the renderer
`.?AVRenderTimerIcon@Renderer@Window@Actor@Scene@Application@@` at
`0x012DDC00`. The renderer constructor installs vtable `0x01066AC4` at
`0x008E491D`. Its slot 1 is `0x008E5090` and slot 8 is `0x008E4A10`.

## OBSERVED property and renderer handoff

| Locator | Observation |
| --- | --- |
| `0x00F2A570..0x00F2A589`, `0x009C0E17..0x009C0E33` | Registration uses the string `Percent` at `0x010821E8` and descriptor `0x01362F70` with property tag 1. The control constructor binds it to float `+0x2C4` with default zero. |
| `0x009C03A2`, `0x009BF4F0..0x009BF548` | Property callback tag 1 calls `0x009BF4F0`. That routine requires the predicate at `0x0093F060`, a non-null drawing context from `0x0093F880`, and mask `0x04` at control `+0x2C8`. It calls context slot `+0x94` with the property value, substituting `0.0` for exactly `100.0`. |
| `0x00FA2978`, `0x0055A480..0x0055A49B` | DrawingToolContext vtable `0x00FA28E4` at `+0x94` selects `0x0055A480`. The routine stores the float at drawing state `+0x40C`, sets mask `0x02` at `+0x5A`, and sets byte `+0x5C` to 1. |
| `0x0054EF03..0x0054EF35` | With mask `0x02` set and the command producer available, the drawing-state producer emits local command `0x16` with the four-byte float body. |
| `0x008CEC66..0x008CEC84`, `0x008CF410` | The WindowContext dispatcher loads the local command, checks it against unsigned maximum `0x28`, and indexes the dword jump table at `0x008CF3B8`. Index `0x16` selects cell `0x008CF410`, which holds `0x008CF1EE`. |
| `0x008CF1EE..0x008CF219` | The arm requests renderer kind `0x15` from `0x008CC150`. For a non-null result, it sets renderer byte `+4` to 1 and calls vtable offset `+0x20` with the body float. |
| `0x008CC173..0x008CC1E3`, `0x008E4A10..0x008E4A1D` | The factory constructs `RenderTimerIcon` for kind `0x15` when context cache `+0xDC` is empty. A populated cache is reused. On the `RenderTimerIcon` vtable, offset `+0x20` selects slot 8, which stores the float at renderer `+0x20`. |

The control predicate requires global byte `0x0135C2B0` to be zero and
control `+0xA0` to be non-null. See the
[Sqwt mouse preview path](../input/sqwt-mouse-preview-producer.md#selected-button-arming-and-preference).

## OBSERVED draw gates and geometry

WindowContext walks three renderer pointers starting at context `+0xC0`
at `0x008CEA7C..0x008CEAB0`. Each non-null pointer with renderer byte
`+4` nonzero receives a call through vtable offset `+4`.

`RenderTimerIcon` slot 1 calls `0x008E1C90` at `0x008E515E` before checking
the timer overlay. See the
[ordinary RenderIcon draw routine](sqwt-icon-control-textures.md#observed-local-image-geometry).

At `0x008E5163..0x008E519B`, the timer routine uses the nonzero word at
renderer `+0x0C` as a one-based index into the ID array pointed to by
renderer `+0x08`. It proceeds when the selected ID is nonzero and the
texture resolver returns a non-null handle. At `0x008E51B4..0x008E51DA`, it
converts the texture dimensions at `+0x18` and `+0x1C` to floats.

At `0x008E51E5..0x008E5223`, slot 1 subtracts renderer float `+0x20` from
the double constant `100.0` at `0x00F55AD0`, rounds the result to float,
and clamps negative results to zero. The overlay path runs only when this
result, called `p` here, is below `100.0`. Within that path it calls
`0x008E4A40` in the following order, passing `p` and the texture dimensions
on each call.

| Call site | Region ID | Condition within the overlay path | Local rectangle |
| --- | --- | --- | --- |
| `0x008E525B` | 1 | `p < 25.0` | Top left |
| `0x008E5298` | 3 | `p < 50.0` | Bottom left |
| `0x008E52D0` | 2 | `p < 75.0` | Bottom right |
| `0x008E52F5` | 0 | Always | Top right |

The thresholds `25.0` and `50.0` are doubles at `0x00FA0DF8` and
`0x00FC1040`. The threshold `75.0` is a float at `0x01066AF0`.
The region rectangles follow from the offsets
at `0x008E4F98..0x008E5058`. `0x008E4A40` halves the texture dimensions,
selects the region through the table at `0x008E507C`, and constructs four
coordinate pairs. The partial-region branches use an angle of `-p / 100`
times the constant `6.2831854820251465` at `0x00F62FE0`. The sine and
cosine paths include `FSIN` at `0x009D641D` and `FCOS` at `0x009D62ED`.
The angle-dependent point uses the texture rectangle's center and
half-diagonal. Within each quarter, a comparison selects between two
ways to construct the region's coordinates.

| Region ID | Comparison site | Constant address | Threshold | Type |
| --- | --- | --- | --- | --- |
| 1 | `0x008E4B57` | `0x01066B08` | `12.5` | Double |
| 3 | `0x008E4CB6` | `0x01066B00` | `37.5` | Float |
| 2 | `0x008E4DE9` | `0x01066AF8` | `62.5` | Double |
| 0 | `0x008E4F09` | `0x01066AEC` | `87.5` | Float |

The coordinate pairs reach `0x00CA5E80` with the texture and a separate
four-float argument, for example at `0x008E4BFC..0x008E4C0D`.

## Interpretation and limits

In local coordinates with the origin at the top left, increasing `p`
removes the overlay counterclockwise from the top center. The `Percent`
substitution at exactly `100.0` sends renderer input zero, which makes
`p` equal `100.0` and suppresses the overlay after the ordinary icon call.
The texture and four-float draw argument travel separately from the
percentage-dependent geometry.

The static path leaves the ActionMenu's live control binding, overlay
texture and appearance, and ancestor visibility behavior unresolved.
