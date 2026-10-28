# NpcSay backdrop draw

The NpcSay window shows a dark translucent panel behind its text. This page
records what that panel is in the shipped widget data, how the client turns a
textured image command into a blended quad, which states it does and does not
set, and which opacity inputs reach the panel. The window that hosts the panel
is in [NpcSay window chain](../event/npcsay-window-chain.md). The shape of the
form and template is in
`xivl-client-data:docs/sqwt-npc-dialogue-structure.md`.

## Summary

| Question | Answer |
|---|---|
| What is the backdrop? | The `Label` that holds `LogControl_NpcSay`. It is 620 by 106 with `SqwtStyle="LAB_basis_npcLinesBody"`, and its `Frame` part draws the nine cell skin of the same name for every condition |
| Does anything else draw behind the text? | No. The window root uses `WIN_null`, whose `Frame` skin is `NULL`. The `LogControl` background part is also `WIN_null`. The form sets no `Opacity`, `Alpha`, `Background` or colour attribute on any control |
| Where does the dark colour come from? | The texture. All nine body cells are black (r, g and b are 0 in every texel) and carry the translucency in the alpha channel, 0 to 166 of 255 |
| How is a cell blended? | As a textured quad. The pixel program is texel times vertex colour. In blend mode 0, the initial mode, blending is `SRCALPHA`, `INVSRCALPHA`, `ADD` with alpha test `GREATER` against 0, cull none and clamp addressing. Which mode the NpcSay draw runs under is not traced |
| What is the vertex colour? | The draw state colour times the command colour, component by component, clamped to 0..1 and truncated to 8 bits. The default for both is 1.0 |
| Is the UI pass written in sRGB? | No sRGB write state, no sRGB texture state. The client blends in the encoded value space |
| Is there a window opacity input for NpcSay? | Only `VisualOpacity` on the window, animated by the template. No configuration key, widget script or form attribute scales the backdrop |
| Does `VisualOpacity` reach the cell colour? | It reaches the drawing context, as the first of four floats to context slot 17 (tool record `+0x70`). The `DrawImage` executor reads the other record at `+0x90`, fed by slot 18. Whether `+0x70` scales a cell is not established |
| What stays unknown? | The routine that places the nine cells of a control frame, the filter modes of the UI samplers, the blend mode of the NpcSay draw, and the consumer of the `+0x70` colour record |

## Input and method

Observed against FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All code locators are VAs. Ghidra CLI navigated the code, and
Capstone 5.0.7 listings of the identified PE checked every cited instruction,
table word and literal. Decompiler output is navigation only. OBSERVED marks
instructions, data and decoded assets. INFERRED marks an interpretation of
observed facts. UNKNOWN marks what was not traced.
`tools/verify_npcsay_backdrop_draw.py` re-checks the executable rows, and
`tools/test_npcsay_backdrop_draw.py` covers it.

The install data was decoded with the basename-key SQEX recipe in
`xivl-client-data:docs/sqwt-npc-dialogue-structure.md`. The form and template
inputs equal the sizes and SHA-256 values recorded there.

| Input | Bytes | SHA-256 |
|---|---:|---|
| `client/sqwt/common/default.style` | 353783 | `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` |
| `client/sqwt/common/default.skin` | 141967 | `6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640` |
| `client/sqwt/common/default.win32.gtex` | 4194336 | `c7f11f369e21b19b79fbe174bb10f78f7312b366383db9774e873238b2d5d88f` |

The style decodes to 353775 bytes and the skin to 141959 bytes. The
verifier repeats the data checks below when it is given the install and the
`xivl-tools` Blowfish module. No XML body, texel dump or decompiled body is
needed in a published artifact.

## Controls that make the panel (data)

OBSERVED. The form `Window_NpcSayWidget` holds these controls that matter for
drawing.

| Control | Kind | `SqwtStyle` | Notes |
|---|---|---|---|
| `Window_NpcSayWidget` | Window | `WIN_null` | `IsHitTestVisible="False"`, `FontEdge="True"`, `FontSize="13"`. Style file `common/default.style` |
| `Label_NpcName` | Label | `LAB_basis_npcLinesHeader` | Row 0 of `Grid_Main`. Holds `TextBlock_NpcName` (`TBL_subheading`) |
| unnamed body label | Label | `LAB_basis_npcLinesBody` | Row 1 of `Grid_Main`, `Width="620"`, `Height="106"`, `IsHitTestVisible="False"` |
| `LogControl_NpcSay` | LogControl | `LogControl_npcLines` | Child of the body label, `Margin="10,0,10,4"` |

OBSERVED. A walk over every attribute of the form finds no `Opacity`,
`VisualOpacity`, `VisualOpacityRed`, `VisualOpacityGreen`, `VisualOpacityBlue`,
`Alpha`, `Background` or `Foreground`. The template root is a bare
`ContentPresenter`. Its only property setter is `VisualOpacity` on the window,
set to 0 by the `Shown` trigger and animated to 1 by `NpcSayStart`
([window chain](../event/npcsay-window-chain.md#window-declaration)).

OBSERVED. In `common/default.style` the `Label` style `LAB_basis_npcLinesBody`
has one `Frame` part. Its `SkinNormal`, `SkinHover`, `SkinActive`,
`SkinDisabled` and `SkinFocused` rows all name the skin `LAB_basis_npcLinesBody`
and carry no font colour. `SkinAnchored`, `SkinReserved` and `SkinDroppable`
are `NULL`. The `Window` style `WIN_null` names `NULL` as the skin in all eight rows. The
`LogControl` style `LogControl_npcLines` takes its `Background` part from
`WIN_null`, so the text control draws no frame. Twenty shipped form files name
the body skin, among them the fishing, mining, felling, negotiation and item-use
widgets, and `NpcSayWidget.sd.form`. No shipped form names the `Window` style
`WIN_basis_blackGround`, which also uses it.

## The body skin and its texture (data)

OBSERVED. In `common/default.skin` the `CellSkin` `LAB_basis_npcLinesBody` has
`Cells="3,3"`, `HitMargin="28,10,28,10"` and `SkinBorderThickness="6"`. Its
group source is `common/default.dds`. The install has no file of that name. The
texture file next to the skin is `common/default.win32.gtex`. The nine cells
are all `DrawMode="Stretch"` with these rectangles (x, y, width, height):

| Row | Left | Centre | Right |
|---|---|---|---|
| top | 0,260,64,32 | 64,260,4,32 | 68,260,64,32 |
| middle | 0,292,64,4 | 64,292,4,4 | 68,292,64,4 |
| bottom | 0,296,64,32 | 64,296,4,32 | 68,296,64,32 |

OBSERVED. The texture is GTEX format index 4 (`D3DFMT_A8R8G8B8`, see
`xivl-tools:docs/formats/gtex-pwib.md`, GTEX contract),
1024 by 1024, one mip, surface size 4194304, source base 32. Over the nine
rectangles:

- every texel is (r, g, b) = (0, 0, 0), so the colour is black everywhere
- the centre cell (64,292,4,4) is a constant alpha of 166, which is 0.651 of 255
- the maximum alpha in any cell is 166, and the minimum is 0 at the outer corner
- the alpha of a corner cell is separable. In the top-left cell it is a
  horizontal profile of 64 values (the bottom texel row) times a vertical
  profile of 32 values (the right texel column), divided by 166, and the
  product matches every sampled texel to within 3
- the horizontal profile first reaches half the plateau (83) at texel 32 of 64,
  and the vertical profile first reaches it at texel 10 of 32

The header skin `LAB_basis_npcLinesHeader` (`Cells="3,1"`,
`HitMargin="8,4,4,4"`, `SkinBorderThickness="6,0,6,6"`) is a separate coloured
strip with alpha up to 216. It is the speaker label background, not the body.

INFERRED. The half points at texel 32 and texel 10 sit next to the `HitMargin`
of 28 and 10. The vertical value equals it, and at texel 28 the horizontal alpha
is 72, which is 43 percent of the plateau, so the margin looks derived from the
feather. A feather that spans 64 by 32 texels is made for corner cells drawn at
64 by 32 pixels, and `SkinBorderThickness="6"` is then not the drawn corner
size. That supports native-size corners with a stretched middle, but no
executable row confirms the placement (see the open items).

OBSERVED. The cell parser `0x009A81F0` reads the attribute `Rectangle`, stores
five dwords per cell (stride `0x14`) and sets the first from `DrawMode`:
`Stretch` is 0 (`0x009A84B9`), `Tiling` is 1 (`0x009A84D7`) and `ConstantSize`
is 2 (`0x009A84F5`). The other four dwords are x, x plus width, y and y plus
height. The skin file declares 1186 `Stretch` cells and 31 `Tiling` cells and
no `ConstantSize` cell.

## From element to quad (code)

OBSERVED. The element render loop `0x0093AEE0` walks a list of elements. For
each one it tests a flag and the virtual at `+0x80`, then gets the drawing
context through `0x0093F880` and calls the element virtual at `+0x88`
(`0x0093B009`) with it. Slot 34 of the primary vtables is that virtual:

| Class | Vtable | Slot 34 |
|---|---|---|
| Label | `0x010738B4` | `0x00936DB0` |
| Window | `0x0106A0BC` | `0x00936DB0` |
| Image | `0x0108040C` | `0x009B9A60` |
| Border | `0x0107E01C` | `0x00776340` |

OBSERVED. `0x00936DB0`, shared by Label and Window, reads the pointer at
control `+0x310`, which the setter `0x00938080` stores for the
property `Background` (info object `0x013591D8`, name literal `0x0106B988`,
registered at `0x00F22610`). When the first component of that brush differs from
0.0 it calls context slot 18 with four floats (`0x00936DB0` listing in the
verifier). It draws no skin. The form sets no `Background`, so this path stays
quiet for the NpcSay controls.

OBSERVED. `Image` `0x009B9A60` builds two rectangles from its size and source
rectangle properties, a colour of four floats loaded with `fld1`, and calls
context virtual `+0x20` (image with mask, `0x009B9C47`) when the mask index at
`+0x2B4` is not negative, otherwise `+0x1C`. Both need the image index at
`+0x2AC` to be non-negative.

OBSERVED. The context is `DrawingToolContext` (vtable `0x00FA28E4`, constructed
at `0x00553D90`). Slot 7 (`+0x1C`) is `0x00559F30`, which forwards to the
builder `0x0054DE10`. That builder creates the command `DrawImage` through
`0x00554190` and appends its 0x40 bytes to the staging buffer at tool `+0x64`
through `0x0054CCC0`. Slot 8 does the same for `DrawImageWithMask` (`0x50`
bytes). `DrawImage` stores two resource ids at `+4` and `+8`, a flag dword at
`+0xC`, two rectangles of four floats at `+0x10` and `+0x20`, and a colour of
four floats at `+0x30` that the builder fills with 1.0, 1.0, 1.0 and the sixth
argument of the context call. Slot 1 of each command vtable takes the draw state
record and returns the command size, and slot 2 returns the size alone. A
consumer that steps through the buffer by those sizes is INFERRED, and its loop
was not located.

| Command | Vtable | Slot 1 | Size |
|---|---|---|---|
| `DrawImage` | `0x00FA2A00` | `0x00696740` | `0x40` |
| `DrawImageWithMask` | `0x00FA2A10` | `0x00696950` | `0x50` |
| `PushColor` | `0x00FA2A20` | `0x00696DC0` | `0x14` |
| `PopColor` | `0x00FA2A30` | `0x00696D90` | `0x04` |

OBSERVED. Slot 1 of `PushColor` and `PopColor` only returns the command size and
touches no state. Context slots 17 (`0x0055A050`) and 18 (`0x0055A090`) store
four floats at tool `+0x70` and `+0x90` and set dirty bits `0x08` and `0x10`.
OBSERVED. The tool flush `0x0054EC3B..0x0054EC8E` tests dirty bit `0x08` and
sends the 16 bytes at tool `+0x70` through `0x0054B0D0` with index 0, then tests
bit `0x10` and sends the 16 bytes at tool `+0x90` with index 1. `0x0054B0D0`
adds `0x15` to the index (`0x0054B11F`) and hands the result and the payload to
the scene packet builder `0x004EC080`, so the two blocks travel as scene
packets `0x15` and `0x16`. INFERRED. The `DrawImage` state colour at `+0x90` is
the receiving copy of the `+0x90` block, because the offsets match. The scene
packet handlers were not traced, so the state side of that link is not
confirmed.

OBSERVED. `DrawImage` executor `0x00696740` resolves the texture from the first
id and falls back to the second (`0x00696746..0x00696777`). With the flag dword
at `+0xC` equal to 0 it computes the four products state `+0x90..+0x9C` times
command `+0x30..+0x3C` (`0x00696788..0x006967F1`) and calls the textured quad
routine `0x00CA5760` with the two rectangles, the product colour and the
texture (`0x0069681A`). When neither id resolves to a texture it draws nothing
(`0x00696777`). A nonzero flag dword draws only when bit `0x80` is set, and then
through `0x00CA5970` (`0x0069682F`, `0x006968D0`). The routine returns `0x40`.
The same primitive `0x00CA5760` is also called by the rectangle executor
`0x00696C00` and by the window renderer family at `0x008DF7F6..0x008E205B`.
INFERRED. Skins drawn by that family use the same states.

OBSERVED. Quad routine `0x00CBC910` takes the first rectangle as x, y, width,
height and draws the corners x, y and x plus width, y plus height with no
half-texel offset. It divides the second rectangle by the texture width and
height, read from texture `+0x18` and `+0x1C`, to get the uv rectangle. It
packs the colour with `0x00CB9640`, which clamps each component to 0..1,
multiplies by 255.0 and truncates (`cvttsd2si`).

## States and programs

OBSERVED. `0x00CA5760` selects one of two paths by the dword at `0x013764A0`.
Both set the vertex program index (1 or 5) and the pixel program index 1 through
`0x00CA2E80`, and they switch mode through `0x00CA3A70` when the previous mode
differs. `0x00CA3A70` calls `0x00CA3890`, which applies the blend setting held
in the global `0x0137649C`. The state flush `0x00CA2FD0` writes the alpha test,
cull and sampler states. All of them go through the render-state wrapper
`0x004236E0`. That wrapper takes (id, value), skips the call when an array of
0x100 cached words at the device object already holds the value
(`0x00424010`), and otherwise calls the device virtual at `+0x24`. The ids are
the Direct3D 9 numbering.

| State | Value written | Locator |
|---|---|---|
| `ALPHABLENDENABLE` (27) | 1 | `0x00CA38F1`, wrapper `0x0041C1F0` |
| `SRCBLEND` (19) | `D3DBLEND_SRCALPHA` (index 6 in table `0x00F596E0`) | call `0x00CA3925`, wrapper `0x0041D050` |
| `DESTBLEND` (20) | `D3DBLEND_INVSRCALPHA` (index 7) for mode 0, `D3DBLEND_ONE` (index 1) for mode 1 | `0x00CA3902`, `0x00CA38E1` |
| `BLENDOP` (171) | `D3DBLENDOP_ADD` (index 0 in table `0x00F5971C`) | call `0x00CA393A`, wrapper `0x0041D010` |
| `ALPHATESTENABLE` (15) | 1 | call `0x00CA34B7`, wrapper `0x0041C1D0` |
| `ALPHAFUNC` (25) | `D3DCMP_GREATER` (index 4 in table `0x00F596C0`) | call `0x00CA34D7`, wrapper `0x0041CF50` |
| `ALPHAREF` (24) | 0 (the wrapper truncates 0.0 times 255 plus 0.5) | `0x0041CF6C..0x0041CF92` |
| `CULLMODE` (22) | `D3DCULL_NONE` (index 0 in table `0x00F597C8`) | call `0x00CA34E7`, wrapper `0x0041D0E0` |

The same call writes the alpha-channel factors (207, 208) with the same indices.
The enable for separate alpha blending (206) is written once, by `0x0041C480`,
whose only caller is the device setup at `0x00A487A0`. The alpha channel of the
target therefore takes source alpha times source alpha plus destination alpha
times its inverse. That channel does not change the colour of the panel.

The blend index pair follows the dword at `0x0137649C`. Value 0 gives
(SRCALPHA, INVSRCALPHA) and value 1 gives (SRCALPHA, ONE). The 2D initialiser
`0x00CA3E60` sets the dword to 0, and it is called from the sites
`0x004E890D`, `0x004E896D`, `0x004E8AB9` and `0x00CA5AC0`. The setter `0x00CA4170` changes it. Its only callers
are in the window renderer routine `0x008E3A40`, which loads the mode from the
renderer byte at `+0x78` (`0x008E3B00`) and writes 0 again at `0x008E3C19`.
INFERRED. A `DrawImage` executed outside that routine runs
under mode 0, the straight alpha blend. Mode 1 is additive and cannot make a
dark panel.

OBSERVED. The UI pixel program is table entry 1 at `0x0110A680`, 0xE8 bytes,
decoded with `tools/d3d9_shader_decode.py`:

```
ps_2_0
texld_pp r0, t0, s0
mul_pp r0, r0, v0
mov_pp oC0, r0
```

The vertex program is entry 1 at `0x01109500`, 0x18C bytes. It transforms the
position by the world and view-projection constants, copies the vertex colour
to `oD0` and the uv to the first texture coordinate. The output colour of a
cell is therefore texel times vertex colour in all four channels, then the
blend above.

OBSERVED. The flush calls `0x00418300` for samplers 0 and 1 with the index
triple (4, 4, 4) and the value 7 (`0x00CA3513` and the call that follows). That
routine stores them and calls `0x0041CC60`, which writes sampler state types
1, 2 and 3 (the three address modes) from table `0x00F59750`. Entry 4 of the
table is `D3DTADDRESS_CLAMP`. The filter and mip bias types are written by
`0x0041CD00`, and which values the UI samplers hold when a cell is drawn was not
traced.

## Colour space

OBSERVED. A scan of all 52 direct calls of the render-state wrapper finds the
immediate ids 7, 8, 14, 15, 19, 20, 22, 23, 24, 25, 27, 52, 57, 58, 59, 154,
156, 168, 171, 174, 175, 181, 185, 190, 193, 195, 206, 207, 208 and 209, and
calls whose id comes from the stencil words at `0x00F597D4..0x00F597F0`
(ids 53 to 56 and 186 to 189). It finds no write of `SRGBWRITEENABLE` (194). A
scan of all 21 calls of the sampler wrapper finds the immediate types 1 to 10
and no write of `SRGBTEXTURE` (11). The pixel program has no gamma
instruction.

INFERRED. The client therefore blends the UI in the encoded value space, with
the texel and the framebuffer holding the same encoding and the blend equation
applied to those values directly. The final present blit
([present gamma blit](present-gamma-blit.md)) raises the colour to an exponent
that is 1.0 by default, so it does not move the UI either.

For a black cell the colour term is 0, so the output is the framebuffer times
(1 - alpha). The effect of the space shows only in how light the result is.
With alpha 166/255 over an encoded 0.8, the encoded-space blend gives 0.279. The
same blend on an sRGB target, with the framebuffer decoded and the result
encoded, gives 0.496. At 0.5 it is 0.175 against 0.303. The linear result is
lighter, but it is still dark, so the colour space alone does not remove the
panel.

## Opacity inputs

OBSERVED. The Visual constructor `0x009405C0` registers four float properties
with the default 1.0 (`fld1` at `0x00940647` and `0x00940666`): `VisualOpacity`
(literal `0x0106C4DC`, info object `0x0135988C`, id 4), `VisualOpacityRed`,
`VisualOpacityGreen` and `VisualOpacityBlue` (ids 5 to 7). The value slots are
`+0x48`, `+0x50`, `+0x58` and `+0x60` of the Visual.

OBSERVED. The executable has no string for a window opacity, window alpha or
background alpha configuration key. The strings `visualopacity_iconcontrol_*`
and `visualopacity_textblock_region*` are control-specific names. Neither set
is a window setting.

OBSERVED. A string search for `Opacity` over a local decode of the shipped Lua
bytecode finds 13 scripts (names follow `xivl-client-scripts:lua/registry.json`).
`WidgetBaseClass.setVisualOpacity`, `getVisualOpacity` and
`setColor` write and read the properties. `LogWidget.setTransparency` writes
`FloatData.Value0` on its own window and `LogWidget.setTextTransparency` sets
the opacity of its two log controls to 1 minus a percentage. None of the
NpcSay widget script, the desktop connector, the player base class, the cut
scene scripts or the debug command script contains the string. The NpcSay
widget has no per-window transparency setting.

OBSERVED. The only NpcSay opacity changes are therefore the template's: 0 on
`Shown`, then 1 over 0.2 seconds on `NpcSayStart`, and 0 over 0.2 seconds on
the hide and close triggers.

OBSERVED. The per-element context builder `0x0093F880` reads the Visual
value slots `+0x48`, `+0x50`, `+0x58` and `+0x60` of the element when any of
their dirty bytes (`+0x4C`, `+0x54`, `+0x5C`, `+0x64`) has bit 4 set, and calls
context virtual `+0x44` with the four floats in that order
(`0x0093F900..0x0093F941`). That virtual is slot 17 (`0x0055A050`) of the
`DrawingToolContext` vtable, which stores them at tool `+0x70`. INFERRED. The
context is a `DrawingToolContext`, because the element virtual `+0x54`
(`0x0093EF90`) calls slot 1 of the global object `0x0135C2AC`, and the vtable at
`0x00FA2D8C` holds `0x005522D0` in slot 1, the routine that constructs the
`DrawingToolContext`. The dynamic type of the global was not read. The same builder never calls
slot 18, so the `+0x90` record that `DrawImage` multiplies comes only from a
`Background` brush (`0x00936DB0`, which the NpcSay form does not set).

UNKNOWN. What consumes the scene packet `0x15` block (tool `+0x70`, opacity and
three colour scales) on the render side, and the initial value of the `+0x90`
state colour. The reading that the window opacity multiplies every cell colour,
as it multiplies text, is therefore UNKNOWN rather than inferred: the only
multiplier in `DrawImage` is the `+0x90` colour, and by offset the opacity
travels in the other record. The template fade (a `VisualOpacity` fade is what
makes a window appear) argues that a window with opacity 0 shows nothing, which
is INFERRED from the template alone.

## What a client has to do

OBSERVED unless marked.

1. Draw each of the nine cells as a textured quad from the skin texture, with
   the cell rectangle as the source and the placed rectangle as the target.
2. Use the colour (state colour times command colour) as the vertex colour. The
   command colour the `Image` control passes is (1, 1, 1, 1). The default state
   colour was not read. A client that keeps it at white matches the observed
   multiplication, and whether the window opacity also scales the cell is
   UNKNOWN (see Opacity inputs).
3. Output texel times vertex colour, in all four channels, with the texture
   treated as straight (not premultiplied) alpha. The body texels are black
   with the alpha in the texture, so no tint or colour constant is needed.
4. Blend with source factor source alpha, destination factor one minus source
   alpha and the add operation. Discard texels with alpha 0. Do not cull. Clamp
   the texture coordinates.
5. Blend the encoded values directly (INFERRED from the absent sRGB states). On
   a surface that is sRGB, draw the UI pass through a non-sRGB view of the same
   surface, or the dark panel comes out about 0.2 lighter than retail at a
   bright background.
6. Place the corner cells at their texel size and let the middle row and column
   take the rest (INFERRED from the feather width). For the 620 by 106 label
   that is columns of 64, 492 and 64, and rows of 32, 42 and 32.
7. Draw the panel under its content. The label frame has to be drawn before the
   `LogControl` text inside it. The retail order is UNKNOWN, because the element
   render loop `0x0093AEE0` visits a list whose order was not traced.
8. INFERRED. Apply the window opacity to the panel the same way as to the text.
   If the window opacity is still 0 when the text is drawn, a client that scales
   the text by its own alpha and the panel by the window opacity shows text and
   no panel. Retail sets the opacity to 0 on `Shown` and to 1 on `NpcSayStart`
   (OBSERVED in the template). That retail itself hides the panel at opacity 0
   is not established.

## Open items

- The routine that places the cells of a control frame. `0x00936DB0` draws no
  skin. The parsed cell record (stride `0x14`), the style lookup `0x00933E70`
  (seven condition slots, in the order recorded in
  [TabControl, ScrollViewer and TextBox parts](sqwt-tab-scroll-text-parts.md))
  and the skin set lookup `0x009A0BF0` were found, but no reader of the cell
  mode that ends in a draw call. The window renderer family around
  `0x008E0000` resolves style, part and skin by name (`0x008E2A50` calls
  `0x00934210`) and calls the same quad primitive. Whether it draws the frame of
  a Sqwt control is UNKNOWN.
- Whether `SkinBorderThickness="6"` and `SkinMargin` change the drawn rectangle.
  `0x008E13A0` and `0x008E1540` widen a rectangle by four floats of the skin
  record, and which record field they read was not matched to the attributes.
- The filter and mip states of the UI samplers.
- Whether the projection built for modes 0 to 3 in `0x00CA2FD0` carries a
  half-pixel offset. It adds the quadword constant `0x00FC1060` (0.25 as a
  double) divided by the target size, and the quad routine itself adds none.
- The render-side consumer of the scene packet `0x15` block (the Visual opacity
  and colour scales) and the initial value of the `+0x90` state colour, which
  settle whether the opacity and the three colour properties scale the panel.
- Which blend mode index (`0x0137649C`) the NpcSay draw runs under.

## Reproduction

Use the identified executable in Ghidra or any x86-32 disassembler and read the
cited ranges. Check the executor `0x00696740`, the quad routine `0x00CBC910`,
the colour packer `0x00CB9640`, the begin routine `0x00CA3890`, the flush
`0x00CA2FD0`, the wrappers `0x0041C1F0`, `0x0041D010`, `0x0041D050`,
`0x0041C1D0` and `0x0041CF50`, and the cell parser `0x009A81F0`. Decode the
programs at `0x0110A680` and `0x01109500` with `tools/d3d9_shader_decode.py`.
Run `python tools/verify_npcsay_backdrop_draw.py --exe <ffxivgame.exe>` for the
executable rows. Add `--install <client/sqwt> --blowfish <xivl-tools/tools/blowfish.py>`
to re-read the form, the style rows, the skin rectangles and the texture alpha.

## Consumer notes for bahamut-client

A client may implement these parts of the contract.

- Treat the panel as nine textured quads of the body skin, black texels with
  alpha 0 to 166, drawn with straight alpha blending and white vertex colour.
  Scaling the vertex colour by the window opacity is INFERRED only.
- INFERRED. Blend in the encoded space. On an sRGB surface use a non-sRGB view
  for the UI pass.
- INFERRED. Keep corner cells at texel size and stretch the middle row and
  column.
- Check the window `VisualOpacity` at draw time before suspecting the texture or
  the blend. Retail holds it at 0 from `Shown` until `NpcSayStart` (OBSERVED in
  the template).
- Do not add a window transparency setting. The NpcSay widget has none.

These stay CALIBRATION until traced: the cell placement rule (including native
size corners), the draw order of the label frame and its text, the sampler
filters, the effect of the opacity and the three colour properties on the
panel, the blend mode of the NpcSay draw, and the drawn rectangle when
`SkinBorderThickness` or `SkinMargin` is set.
