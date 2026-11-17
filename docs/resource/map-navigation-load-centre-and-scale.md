# Map navigation load centre and scale

The `MapScreenControl` of the full-screen map window declares `Function="0"`
and `FocusPointColumn="15"` (see the
[form declarations](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/sqwt-map-navigation-structure.md)).
This page records which centre the control proposes after a piece result is
accepted and which scale it stores. It supplements "Viewport center and scale
inputs" of [minimap-base-tiles.md](minimap-base-tiles.md), which fixes the
property holders and the centre admission helper, and leaves the choice among
the centre arms and the `Function` 0 scale rule open.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Disassembly with Ghidra 12.1.4 through ghidra-cli covers `0x00679940`,
`0x00670210`, `0x006780D0` and the constant cells, and a PE section read of
the original file supplies the constant values.

## Load centre

After an accepted piece result the update `0x00679940` reads the piece fields
and then chooses the proposed centre that it passes to `0x00672D30`
(`0x00679D9D..0x00679E21`). Control fields are `+0x56C` `QuestMapIndex`, `+0x470`
Layout, `+0x478` Rect and `+0x53C` `FocusPointColumn`. STATIC.

| Condition, tested in this order | Centre pair passed | Locator |
| --- | --- | --- |
| `QuestMapIndex` differs from -1 | `+0x630` | `0x00679D9D`, `0x00679DCC..0x00679DD4` |
| Layout and Rect are both -1 | `+0x968`, the retained actor X and Z | `0x00679DF2..0x00679E09` |
| `FocusPointColumn` at least 0 | `+0x628` | `0x00679E0B..0x00679E15` |
| otherwise | `+0x620`, the retained map midpoint | `0x00679E18` |

When `FocusPointColumn` is at least 0 the update first copies the checker
pair at checker `+0x08` and `+0x0C` into control `+0x628` and `+0x62C`
(`0x00679DD6..0x00679DEC`). The piece materialisation `0x006780D0` repeats the
same four-way choice at `0x006783CD..0x006784CC`.

### Focus pair from the sheet

The checker `0x00670210` stores the focus column in checker `+0x14`, the
layout in `+0x18` and the page in `+0x1C` through its request `0x006712B0`.
With `F` the value at `+0x14` and `F` at least 0, it reads `mapNavi_data`
columns 3 and 4 and forms `Ox = -0.0 - col3` and `Oy = -0.0 - col4` in single
precision (`0x006703C9..0x00670423`, the `-0.0` constant at `0x00F62FA0`). It
then reads columns `F` and `F+1` (`0x00670428..0x00670445`). With `res` the
checker resolution at `+0x10`:

- When both of those columns are 0, it reads columns 10 and 11 and stores
  `f32(col10 / res * 0.5 + Ox)` and `f32(col11 / res * 0.5 + Oy)` at checker
  `+0x08` and `+0x0C`. The 0.5 is the binary64 constant at `0x00F59898`
  (`0x00670453..0x006704E5`).
- Otherwise it stores `f32(colF / res + Ox)` and `f32(colF1 / res + Oy)`
  (`0x006704EA..0x0067052E`).

The arithmetic widens to binary64 and stores single precision. `res` is the
checker value at that point, the `_layout` column 1 byte. The `mapNavi_data`
column 5 override of `+0x10` is applied after these stores
(`0x00670545..0x00670573`). STATIC.

## Load scale

In `0x006780D0` the test of `Function` (`+0x448`) at `0x0067841A` selects the
rule. A nonzero `Function` stores actual scale 1 through the constant
`0x00F54F70`, which holds f32 1.0. With `Function` 0 (`0x006784D1..0x0067858A`):

- `sx = f32(+0x1B4 / Dx)` and `sy = f32(+0x1AC / Dy)`. `Dx` and `Dy` are
  `+0x664` and `+0x668`, the `mapNavi_data` columns 10 and 11. `cover` is the
  larger of the two (`0x006784D1..0x00678529`).
- The start value is the f32 at `0x012BAAF8`, which is 1.0 in the original
  `.data` image. Every control instance shares it.
- The start is raised to `MinScale` (`+0x460`), then to `cover`, then lowered
  to `MaxScale` (`+0x458`) when above it (`0x0067852C..0x00678576`).
- The result is stored at `+0x60C` and written back to `0x012BAAF8`
  (`0x0067858A..0x00678592`). The `Scale` property then follows through the
  holder at `+0x450`.

The direct scale update `0x00673640` is the other writer of `0x012BAAF8`
(`0x00673661`). STATIC for every step above. INFERRED that `+0x1B4` and
`+0x1AC` are the arranged width and height of the control. With the
constructor values `MinScale` 0.5 and `MaxScale` 1 of the minimap page and no
zoom, a load stores scale 1 whenever `cover` is at most 1.

## Boundary

The step sizes of `Map.ScaleUp` and `Map.ScaleDown` are not decoded. The route
that writes Layout and Rect before the update runs is a script contract.

## Reproduction

Disassemble `0x00679D95..0x00679E2F`, `0x006703C9..0x00670573` and
`0x0067841A..0x00678592` and match the locators. Read f32 at `0x00F62FA0`,
`0x00F54F70` and `0x012BAAF8` and the binary64 at `0x00F59898` from the
original file through its PE section table.
