# Actor appearance path decoder

The retail 1.23b client turns the 28 appearance dwords of an actor into
resource directory paths in three steps. `0x006B7840` splits dword 0 into a
model category and number. Small lane helpers split each packed dword into
bit fields. `0x006B5E40` and `0x006B7A40` then format directory paths from
those numbers and fixed literals. This page records each step, the literal
tables it reads, and a worked example from a retail `0x00D6` sample.

Native addresses are VAs in `ffxivgame.exe`, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, size
15996808. Observations come from `ghidra-cli` 0.2.2 driving Ghidra 12.1.4
decompile, disassembly, memory-read and cross-reference queries on that
binary. Wire sample values come from `xivl-opcodes:data/vendor/captures/payload_samples.json`,
key `0x00d6`, the first sample (capture `change_bodyarmor.pcapng`).

The upstream chain from the packet to the model helper is in
[Appearance-dirty model apply path](appearance-dirty-apply.md). The literal
callsites are in [Appearance resource literals](appearance-resource-literals.md).
The record transport is in [Native item-appearance boundary](item-appearance-boundary.md).

## Tier labels

| Tier | Meaning |
|---|---|
| `direct-pe-byte-read` | Literal text or pointer table read from the pinned image |
| `local-data-flow` | Instructions in the cited function move the stated value from source to use |
| `local-bitfield-data-flow` | As above, and the shift and mask constants are read from instructions |
| `decompile-and-instruction-derived` | Decompiler rendering checked against the cited instructions |
| `direct-xref-set` | Complete direct reference list returned by the Ghidra reference query |
| `wire-sample-arithmetic` | Lane arithmetic applied to a retail capture value |
| `installed-file-existence` | A directory entry exists in a contributor-supplied retail install. No bytes or hashes are recorded |
| `UNKNOWN` | Not established by the cited evidence |

## Wire index to helper field

| Step | Location | Observation | Tier |
|---|---|---|---|
| Packet read | `0x0058D0C6..0x0058D0FC` (case `0xD6` of `0x0058CCA0`) | The count is the byte at game message offset `+0x110` (`0xD7` uses `+0x90`), measured from the start of the message including its 16-byte header, the base of the `bytes` field in the xivl-opcodes sample. Entry `i` starts at message offset `+0x10 + 8*i`, with a dword value at `+0` and a byte index at `+4`. Each entry calls `0x00586870(index, value)` | `local-data-flow` |
| Actor store | `0x00586870` | For index `< 0x1C`, stores the value at `actor + 0xAAC + 4*index` and sets byte `actor + 0xB20` | `local-data-flow` |
| Helper copy | `0x006B7840` (CharaVisual vtable slot 25, entry at `0x00FD3F38`) | Copies record dwords `0..0xC` to helper `+0x5C..+0x8C` and dwords `0xD..0x1B` to helper `+0x98..+0xD0`. Dword `0x1C` is the flag word | `local-data-flow` |

The wire index therefore equals the record dword index. The record dwords that
this page decodes are:

| Record dword | Helper field | Consumer |
|---:|---|---|
| 0 | `+0x5C` | `0x006B7840` base-model split |
| 2 | `+0x64` | `0x006B6850` three 10-bit lanes |
| 3 | `+0x68` | Face word |
| 4 | `+0x6C` | Hair word |
| `13 + k`, k in 0..5 | `+0x98 + 4*k` | Gear slot `k` word |

Gear slot order is fixed by the name table read in `0x006B5E40`, listed below:
slot 0 `met`, 1 `top`, 2 `dwn`, 3 `glv`, 4 `sho`, 5 `pch`. Server logical
index 13 (BODYGEAR) is record dword 14, which is gear slot 1 `top`.

## Base-model split

`0x006B7840` (`local-data-flow`, instructions `0x006B7976..0x006B79CE`) reads
record dword 0 as `B`, stores `B` at helper `+0x5C`, then writes helper
`+0x54` (category) and `+0x58` (number):

| `B` | Category | Number |
|---|---:|---|
| `< 0x100` | 0 | `B` |
| `0x100 .. 9999` | 1 | `B - 0x100` |
| `10000 .. 19999` | 1 | `B - 10000` |
| `20000 .. 39999` | 3 | `B - 20000` |
| `>= 40000` | 2 | `B - 40000` |

Category values index the tables below. The comparisons are signed. The
function then calls vtable slot `+0x58` with `(category, number)`, passes the
returned string object to `0x00447450` with `this` set to helper `+0x17C`,
and calls `0x006B6850`.

## Directory prefix

CharaVisual vtable slot 22 (`0x00630850`, entry `0x00FD3F2C`) builds the
per-model directory. It selects entries by category from two pointer tables
and formats them with `/client/chara/%s/%s%03d/` (`0x00FBB3F8`)
(`direct-pe-byte-read`).

| Category | Directory name (table `0x012B8558`) | Letter (table `0x012B8548`) | Prefix example |
|---:|---|---|---|
| 0 | `pc` | `c` | `/client/chara/pc/c003/` |
| 1 | `mon` | `m` | `/client/chara/mon/m051/` |
| 2 | `wep` | `w` | `/client/chara/wep/w<NNN>/` |
| 3 | `bgobj` | `b` | `/client/chara/bgobj/b<NNN>/` |

For a negative number the slot formats `/client/chara/%s/` (`0x00FBB3E4`)
instead. `0x006B7840` never passes a negative number.

The block at `0x00FD2CC8` also holds the joined forms `pc/c`, `mon/m`,
`wep/w` and `bgobj/b`. `0x006B7A40` indexes a four-entry local copy of them
only after it has sent categories 2 and 3 to other functions, so it uses
entries 0 and 1 only (`local-data-flow`, `0x006B7A80..0x006B7AAA`). Its
face and hair format reads table `0x012BF360`, whose first two entries are
`pc/c` and `mon/m` (`direct-pe-byte-read`).

## Lane helpers

All three helpers take the packed word `x` and store each lane through pointer
arguments (`local-bitfield-data-flow`, instructions read at the listed VAs).

| Helper | Output 1 | Output 2 | Output 3 | Output 4 |
|---|---|---|---|---|
| `0x00630660` | `x >> 30` | `(x >> 14) & 0x7FFF` | `(x >> 8) & 0x3F` | `x & 0xFF` |
| `0x006306A0` | `x >> 30` | `(x >> 10) & 0x3FF` | `x & 0x3FF` | none |
| `0x006306F0` | `x >> 30` | `(x >> 20) & 0x3FF` | `(x >> 10) & 0x3FF` | `x & 0x3FF` |

Callers select which output they keep:

| Consumer | Word | Helper | Kept value | Lane |
|---|---|---|---|---|
| `0x006B5E40` index 0, `0x006B7A40` face loop | face | `0x00630660` | Output 3 | bits 13:8 |
| `0x006B5E40` index 1, `0x006B7A40` hair loop | hair | `0x006306A0` | Output 2 | bits 19:10 |
| `0x006B5E40` index 2..7, `0x006B7A40` gear loop | gear | `0x006306A0` | Output 2 | bits 19:10 |

The keeping decision is visible in the argument order at the call sites
`0x006B5EBA`, `0x006B5ECA` and `0x006B7FB2`, `0x006B7FEB`, `0x006B7ADE`
(`local-data-flow`). This page assigns no meaning to the other lanes.

## Face, hair and gear model paths

`0x006B5E40(index, word)` (`decompile-and-instruction-derived`) does nothing
when `word == 0`. Otherwise it extracts the number lane, obtains the directory
prefix from vtable slot `+0x58`, and formats `%s%s%03d/%s` (`0x00FD2C6C`) as
directory prefix, entry prefix, number, entry name. It appends the literal
`0001` through `%s0001` (`0x00FD2C78`). Its sole caller `0x006B8680` calls it
for index 0 with helper `+0x68`, index 1 with helper `+0x6C`, and indices 2
through 7 with helper `+0x98 + 4*(index - 2)` (`direct-xref-set`, call sites
`0x006B8AC2`, `0x006B8ACF`, `0x006B8AE9`).

| Index | Entry prefix (table `0x012BF2A8`) | Entry name (table `0x012BF2C8`) |
|---:|---|---|
| 0 | `kao/f` (`0x00FD2C04`) | `fac_mdl/` (`0x00FD2C60`) |
| 1 | `kao/h` (`0x00FD2BFC`) | `hir_mdl/` (`0x00FD2C54`) |
| 2 | `equ/e` | `met_mdl/` (`0x00FD2C48`) |
| 3 | `equ/e` | `top_mdl/` (`0x00FD2C3C`) |
| 4 | `equ/e` | `dwn_mdl/` (`0x00FD2C30`) |
| 5 | `equ/e` | `glv_mdl/` (`0x00FD2C24`) |
| 6 | `equ/e` | `sho_mdl/` (`0x00FD2C18`) |
| 7 | `equ/e` | `pch_mdl/` (`0x00FD2C0C`) |

Both tables are `direct-pe-byte-read`. The resulting shapes are:

```text
/client/chara/pc/c003/kao/f001/fac_mdl/0001
/client/chara/pc/c003/kao/h001/hir_mdl/0001
/client/chara/pc/c003/equ/e009/dwn_mdl/0001
```

The function formats `%s9999` (`0x00FD2C88`) from the same directory for every
index, and looks the file up only for index 0 (9999 and 9998) and index 1
(9999), the indices the table at `0x00FD3E60` admits. For index 0 it also formats
`%s9998` (`0x00FD2C98`) when helper `+0x40` bit `0x4000` is set. A `.bin` literal (`0x00FD2C80`, `0x00FD2C90`,
`0x00FD2CA0`, `0x00FD2CA8`) accompanies each path string in the call to
`0x00D39290`. Whether `.bin` is a file-name suffix is not decoded here.

## Texture directory paths

The tracked symbol catalog names `0x006B7A40` as an equipment, face and hair
path builder. That name covers the texture-bank directory builder only. The
model paths above come from `0x006B5E40`.

`0x006B7A40` (`decompile-and-instruction-derived`) handles category 3 through
`0x006B6280` and category 2 through `0x006B6480`. For categories 0 and 1 it
formats texture directory paths with three formats.

| Use | Format | Address |
|---|---|---|
| Gear | `/client/chara/%s%03d/equ/e%03d/%s%s/%04d` | `0x00FD2CE8` |
| Face and hair | `/client/chara/%s%03d/kao/%s%s%04d` | `0x00FD2DAC` |
| Face and hair name part | `f%03d/fac`, `h%03d/hir` | `0x00FD2D94`, `0x00FD2DA0` |

The `%s%s` pair after the equipment id is a part name followed by a texture
bank suffix. The gear part names are the literals at `0x00FD2CB0` (`top`),
`0x00FD2CB4` (`dwn`), `0x00FD2CB8` (`glv`), `0x00FD2CBC` (`sho`),
`0x00FD2CC0` (`pch`) and `0x00FD2CC4` (`met`). The loop visits them in that
order, reading gear slots 1, 2, 3, 4, 5 and 0. The suffix comes from
CharaVisual vtable slot 23 (`0x00630910`, entry `0x00FD3F30`), which returns
`_tex1/` (`0x00FBB41C`) for argument 0 and `_tex2/` (`0x00FBB414`) for 1.
Arguments above 2 fall back to entry 0, and the callers pass 0 or 1
(`direct-pe-byte-read` for the strings).

The argument is helper `+0x40` bit 13. `0x006B7840` loads that bit from record
dword `0x1C` bit 0 (`local-bitfield-data-flow`, `0x006B7866..0x006B7879` and
`0x006B7C30`).

The shapes are:

```text
/client/chara/pc/c003/kao/f001/fac_tex1/NNNN
/client/chara/pc/c003/kao/h001/hir_tex1/NNNN
/client/chara/pc/c003/equ/e009/top_tex1//NNNN
```

The gear format places a `/` after a suffix that already ends in `/`, so the
formatted gear string contains `//`. Slot `pch` (loop index 4) always formats
`NNNN` as `0000` (`local-data-flow`, `0x006B7B20..0x006B7BC2`).

The number `NNNN` is not a lane of any record dword. It comes from a sheet
row. See [Texture number and row keys](#texture-number-and-row-keys).

## Worked example

Record values are the first `0x00D6` sample. Results are
`wire-sample-arithmetic` over the lane table above.

| Record dword | Value | Lane used | Result |
|---:|---|---|---|
| 0 | `0x00000003` | `B < 0x100` | category 0, number 3, `pc/c003` |
| 3 | `0x14D00100` | `(x >> 8) & 0x3F` = 1 | `f001` |
| 4 | `0x00000400` | `(x >> 10) & 0x3FF` = 1 | `h001` |
| 13 | `0x00003880` | `(x >> 10) & 0x3FF` = 14 | `e014`, `met` |
| 14 | `0x00001020` | 4 | `e004`, `top` |
| 15 | `0x00002440` | 9 | `e009`, `dwn` |
| 16 | `0x00002CC0` | 11 | `e011`, `glv` |
| 17 | `0x0000548A` | 21 | `e021`, `sho` |
| 18 | `0x0002F800` | 190 | `e190`, `pch` |

Word `0x2440` gives `e009` and `0x2F800` gives `e190` as expected.

A contributor-supplied retail 1.23b client tree contains the directory
entries `client/chara/pc/c003/kao/f001/fac_mdl`, `fac_tex1`, `fac_tex2`,
`kao/h001/hir_mdl`, `hir_tex1`, `hir_tex2`, and the file `0001` for all eight
`_mdl` paths derived above (`installed-file-existence`). `equ/e190` has
`pch_tex1/0000` and `pch_tex2/0000` only, which matches the fixed `0000` for
slot `pch`. This corroborates the path shapes. It is not a live client
observation.

## Resource registration lanes in 0x006B6850

`0x006B6850` (`decompile-and-instruction-derived`) is called once by
`0x006B7840` at `0x006B7A0E` (`direct-xref-set`). It reads the same words and
submits numeric IDs to `0x00CA7D70(kind, id)`. Category 3 returns without
submitting. The kinds and their inputs are:

| Kind | Word | Lanes read |
|---:|---|---|
| 0 (category 2 only) | helper `+0x9C` | `0x006306F0` outputs 3 and 4 |
| 0..5 (categories 0 and 1) | gear words in order slot 1, 2, 3, 4, 5, 0 | bits 19:10 (must be above 0), bits 9:5, bits 4:0 |
| 6 | face, helper `+0x68` | bits 13:8 (must be above 0), bits 7:5, bits 4:0 |
| 7 | hair, helper `+0x6C` | bits 19:10 (must be above 0), bits 9:5, bits 4:0 |
| 8, 9, 10 | helper `+0x64` | bits 9:0, bits 29:20, bits 19:10 |

Kind 4 submits ID 0 without reading a word. The ID arithmetic and the table
that receives each ID are in
[Texture number and row keys](#texture-number-and-row-keys). The server-side
name COLOR for record dword 2 is not established by these lanes.

## Texture number and row keys

The texture number `NNNN` and the directory redirect come from three Excel
sheets that `0x006B7600` registers on the CharaVisual object. The sheet names
are literals read from the image (`direct-pe-byte-read`). Each registration is
`0x006314D0(tableIndex, name, slotCount)` (`local-data-flow`, pushes at
`0x006B76C0..0x006B76C7`, `0x006B76F2..0x006B76F9`, `0x006B7725..0x006B772C`).

| Table index | Sheet name | Literal | Slots |
|---:|---|---|---:|
| 0 | `var_equip` | `0x00FD2B18` | 11 |
| 1 | `var_tex_path` | `0x00FD2B24` | 6 |
| 2 | `var_wep` | `0x00FD2B34` | 1 |

`0x006314D0` builds one `CDevActorExcelWaiter` per table (constructor
`0x00632B30`, vftable `0x00FBBF80`, base constructor `0x00CA7F50` with vftable
`Component::Excel::ExcelSheetWaiter::vftable`) and stores it at
`[visual + 0x2C] + 4*tableIndex`. The constructor stores the owner at waiter
`+0x40` and the table index at waiter `+0x9C` (`local-data-flow`,
`0x00632B74` and `0x00632B99`).

### Request and delivery

`0x00CA7D70(slot, key)` on a waiter table stores `key` as the row key for
`slot`. When every requested slot has its row, the base waiter calls its vtable
slot 1 with the slot count, a per-slot row pointer array (waiter `+0x30`) and a
cookie (`0x00CA7B40`, `0x00CA7CD0`). The derived waiter's slot 1
(`0x00632C40`, entry `0x00FBBF84`) forwards the table index and the row pointer
array to CharaVisual vtable slot 17 (`0x00631590`, entry `0x00FD3F18`). That
method stores the array at `[visual + 0x30] + 4*tableIndex`, sets bit
`2 << tableIndex` of byte `visual + 0x35`, and calls slot 13 (`0x006315D0`,
entry `0x00FD3F08`). When byte `+0x35` equals byte `+0x34`, slot 13 calls slot 16
(`0x006BE3D0`, entry `0x00FD3F14`) with `[visual + 0x30]` as the fourth stack
argument and the dword at `visual + 0x38` as the fifth (`local-data-flow`).

The fifth argument is a pointer to the mode dword, which slot 16 reads. Mode 0 runs `0x006B6B30`
when the first row array is present (call `0x006BE3F3`) and `0x006B8680`
otherwise (call `0x006BE3FE`). Mode 1 runs `0x006B8680` (call `0x006BE412`).
`0x006B8680` passes its argument unchanged to `0x006B7A40` (call `0x006B8B1E`).
The `param_2` of `0x006B7A40` is therefore a pointer to three row pointer
arrays:

| Offset | Array | Index |
|---:|---|---|
| `+0x00` | `var_equip` rows | kind 0..10 |
| `+0x04` | `var_tex_path` rows | gear kind 0..5 |
| `+0x08` | `var_wep` rows | 0 |

A row is the object passed as `this` to `0x00C9A480(column)`. The accessor
returns the address of a four byte cell, `[row + 8] + 4 * map[column]`, where
`map` is the byte array at `[[row + 0x0C] + 0x14]` (`local-data-flow`,
`0x00C9A480..0x00C9A494`). Every column number below is the argument to this
accessor, a sheet column index.

### Row keys requested by 0x006B6850

`0x006B6850` submits the keys for table 0 and table 2 (`0x00CA7D70` calls at
`0x006B68E4`, `0x006B69D2`, `0x006B6A4A`, `0x006B6AB1`, `0x006B6B0B`). Below,
`n` is helper `+0x58` (the model number), `c` is helper `+0x54` (the category)
and `k` is the kind, which is also the slot index in the table
(`decompile-and-instruction-derived`, arithmetic checked at the listed
instructions).

Gear kinds 0..5 read the gear word with index `{1, 2, 3, 4, 5, 0}[k]`, so
kinds 0..5 are `top`, `dwn`, `glv`, `sho`, `pch`, `met`. With `w` that word,
`E = (w >> 10) & 0x3FF`, `P = (w >> 5) & 0x1F`, `Q = w & 0x1F`. A key is
submitted only when `E > 0`, and kind 4 submits key 0 without reading a word.

| Case | Key | Instructions |
|---|---|---|
| Gear, `c = 0` | `((k + 10001 + 10*E) * 100 + P) * 100 + Q` | `0x006B69B7..0x006B69CB` |
| Gear, `c = 1` | `j = (k == 0) ? 0 : k + 1`, `((((n + 1000) * 1000 + E) * 10 + j) * 10 + P) * 10 + Q` | `0x006B6979..0x006B69B1` |
| Face, kind 6 | with `w` = helper `+0x68`, `F = (w >> 8) & 0x3F`, `R = w & 0xFF`: `(((n + 200) * 1000 + F) * 10 + (R >> 5)) * 100 + (R & 0x1F)`, when `n >= 0` and `F > 0` | `0x006B69E3..0x006B6A4A` |
| Hair, kind 7 | with `w` = helper `+0x6C`, `H = (w >> 10) & 0x3FF`, `S = w & 0x3FF`: `(((n + 300) * 1000 + H) * 10 + (S >> 5)) * 100 + (S & 0x1F)`, when `n >= 0` and `H > 0` | `0x006B6A4F..0x006B6AB1` |
| Kinds 8, 9, 10 | with `v` = helper `+0x64` and lane `L` = `v & 0x3FF`, `(v >> 20) & 0x3FF`, `(v >> 10) & 0x3FF` for kind 8, 9, 10: `(n + 1000 * (kind - 7)) * 100 + L`, when `n >= 0` | `0x006B6AB6..0x006B6B0B` |
| Category 2, table 2 slot 0 | with `w` = helper `+0x9C`, `E2 = (w >> 10) & 0x3FF`, `G = w & 0x3FF`: `((n + 1000) * 1000 + E2) * 1000 + G`, when `n > 0`, `E2 > 0` and the key is not negative | `0x006B686E..0x006B68E4` |

Category 3 submits nothing (`0x006B685C`). The sheet row that each key selects
and the column set of each sheet are decoded retail data. This page records
no row values. Row values are read from the installed sheets at run time.

### Texture number

`0x006B7A40` reads `NNNN` for gear parts, face and hair as the dword in the cell
returned by `0x00C9A480(0x6D)` on the `var_equip` row of the part.

| Part | Row | Read | Tier |
|---|---|---|---|
| Gear kind 0..3 and 5 | `var_equip` row at array 0, index `k` | `0x006B7C02..0x006B7C0F` (call `0x006B7C08`) | `local-data-flow` |
| Gear kind 4 (`pch`) | none | `NNNN` is 0 (`0x006B7BBE`, jump to `0x006B7C13`) | `local-data-flow` |
| Face | `var_equip` row at array 0, index 6 | `0x006B800F..0x006B801A` (call `0x006B8015`) | `local-data-flow` |
| Hair | `var_equip` row at array 0, index 7 | same instructions, loop index 1 | `local-data-flow` |

A gear kind whose row pointer is null is skipped and no path is formatted
(`0x006B7BC8`). Kind 4 skips even that test. The value is formatted with
`%04d`, so the cell is treated as signed. The gear format passes it as the last
variadic argument of `_sprintf_s` through an unbalanced push. The caller pushes
`NNNN` and then the texture bank flag, and vtable slot 23 (`0x00630910`, `RET 4`)
pops only the flag. The decompiler rendering of the call omits `NNNN`, and the
instructions at `0x006B7C16..0x006B7C87` carry it.

### Directory redirect

For gear parts, `0x006B7A40` replaces the model number in the
`/client/chara/%s%03d/` part of the texture directory (`local-data-flow`,
`0x006B7BD0..0x006B7C02`):

1. It reads the dword in the cell at column `0x6C` of the `var_equip` row.
2. If the dword is 0, the number is helper `+0x58` unchanged.
3. Otherwise the `var_tex_path` row at array 1, index `k`, must be non-null. A
   null row skips the part.
4. The number becomes the signed 16-bit value `MOVSX word ptr` read from the
   cell at column `n - 1` of that `var_tex_path` row, where `n` is helper `+0x58`
   (`0x006B7BEF..0x006B7BFB`).

The replacement applies to categories 0 and 1 alike, since both take this
code. The face and hair format reads helper `+0x58` directly and has no
redirect (`0x006B8030`).

`0x006B6B30` requests the rows (`local-data-flow`). For gear kinds 0..5 except
4, when the `var_equip` row is non-null and the dword at column `0x6C` is above
0, it calls `0x00CA7D70(kind, value)` on table 1 (`0x006B6B61`), so the column
`0x6C` dword is the `var_tex_path` row key. It then sets `visual + 0x178` to 1
and calls `0x00632540(0xF, visual + 0x178)`. Its only caller is slot 16 in
mode 0.

Install cross-check (`installed-file-existence`). In a contributor-supplied
retail tree, `chara/pc/c001/equ/e004` has `top_tex1` and `top_tex2` and
`chara/pc/c003/equ/e004` has no `top_tex1` or `top_tex2` directory, only
`pch_tex1` and `pch_tex2`. Every `kao/*/*_tex*` directory under `c001`, `c002`
and `c003` holds only the entry `0000`. This is consistent with a redirect and
with `NNNN` being zero for face and hair. It does not prove which rows produce
either result.

## Category 2, category 3 and accessory paths

`0x006B7A40` sends categories 3 and 2 to `0x006B6280` and `0x006B6480`.
`0x006B8680` calls `0x006B6690` for each accessory word. The path strings come
from `_sprintf_s` calls whose arguments the decompiler drops, so the shapes
below follow the pushes at the listed instructions
(`decompile-and-instruction-derived`). Each prefix is the directory string that
vtable slot 22 returns for the given category and number, such as
`/client/chara/wep/w<nnn>/`. Each suffix is the vtable slot 23 string
(`_tex1/` or `_tex2/`).

| Use | Function | Format | Arguments | Shape |
|---|---|---|---|---|
| Category 2 texture | `0x006B6480` | `%sequ/e%03d/%s%s%04d` (`0x00FD2E14`) | prefix for (2, `n`), `E2`, part pointer at `0x012BF300` (`top`), suffix, dword at column `0xCB` of the `var_wep` row | `/client/chara/wep/w<nnn>/equ/e<EEE>/top_tex1/<NNNN>` |
| Category 3 texture | `0x006B6280` | `%sequ/e%03d/%s%s0000` (`0x00FD2DF0`) | prefix for (`c`, `n`), `E` of gear word `s`, part `s` of table `0x012BF2E8`, suffix | `/client/chara/bgobj/b<nnn>/equ/e<EEE>/top_tex1/0000` |
| Accessory model | `0x006B6690` | `%sacc/a%03d/%s0001` (`0x00FD2EA0`) | prefix for (`c`, `n`), `A`, part `i` of table `0x012BF304` | `/client/chara/pc/c003/acc/a<AAA>/nek_mdl/0001` |

Category 2 runs only when the array 2 pointer and its first row pointer are
non-null (`0x006B64C2..0x006B64D3`). `E2` is output 3 of `0x006306F0` on helper
`+0x9C` (the lane `(w >> 10) & 0x3FF`). The `NNNN` value is the dword at column
`0xCB` of that row (`0x006B6511..0x006B6523`). The part pointer at `0x012BF300`
reads `top` (`0x00FD2E10`).

Category 3 loops over gear slots `s` in 0..5 on helper `+0x98 + 4*s`. It takes
`E` as the lane `(w >> 10) & 0x3FF` through `0x006306A0` output 2 without a zero
test, and prints a literal `0000` in place of a number. Table `0x012BF2E8` lists
`met`, `top`, `dwn`, `glv`, `sho`, `pch` (`direct-pe-byte-read`), which is gear
slot order.

The accessory function takes the word of accessory index `i` in 0..8 from
helper `+0xB0 + 4*i` (the call at `0x006B8B07`) and does nothing when the word
is 0. `A` is output 2 of `0x006306D0`, `(w >> 10) & 0x3FF`, and output 1 is
`w >> 30` (`local-bitfield-data-flow`, `0x006306D0..0x006306ED`). Table
`0x012BF304` lists nine entries in order: `nek_mdl/`, `ear_mdl/`, `eal_mdl/`,
`wrr_mdl/`, `wrl_mdl/`, `idr_mdl/`, `idl_mdl/`, `rir_mdl/`, `ril_mdl/`
(`direct-pe-byte-read`).

Install cross-check (`installed-file-existence`). A contributor-supplied tree
holds `chara/wep/w002/equ/e036/top_tex1/0000`,
`chara/bgobj/b001/equ/e001/top_tex1/0000` and
`chara/pc/c001/acc/a001/nek_mdl/0001`, and the `acc/a001` directory also holds
the other eight part names. This corroborates the shapes. It is not a live
client observation.

Every path built by these functions is handed to `0x006320C0` with a bank
number. The bank numbers are in
[Appearance resource slots](appearance-resource-slots.md).

## Open items

- The meaning of the sheet columns read here, `0x6C` and `0x6D` of
  `var_equip`, `0xCB` of `var_wep` and the `var_tex_path` columns, is `UNKNOWN`
  beyond the reads recorded above. The sheet column layouts are decoded retail
  data and are not published here.
- What happens when no row is found for a requested key is `UNKNOWN`. Whether
  a gear kind with `E = 0` keeps an earlier row in its slot is not traced.
- The `_snd` sibling path branch for the `top` slot in `0x006B7A40` is not
  recorded here. Its format strings are at `0x00FD2D1C` and `0x00FD2D38`.
- The role of the companion `9999` and `9998` files is `UNKNOWN`. This page
  makes no skeleton or PWIB claim.
- Weapon words (record dwords 6..12) are not decoded here.
- Record dwords 1 and 5, the face lane bits 29:14 and 7:0 outside the key
  formulas, the gear lane bits 9:0 outside the key formulas, and the bits 31:30
  lane have no path role established here.
- The directory prefix is formed before any file lookup. Whether a given
  directory exists for a given actor, and any substitution when it does not,
  is `UNKNOWN`.
- No live retail session was observed. Every path above is derived from static
  evidence.

## Reproduction

Open the pinned executable with `ghidra-cli` and run these read-only queries.

```text
ghidra decompile 0x006B7840
ghidra disasm 0x006B7840 -n 200
ghidra decompile 0x006B6850
ghidra decompile 0x006B7A40
ghidra disasm 0x006B7A40 -n 700
ghidra decompile 0x006B5E40
ghidra disasm 0x006B5E40 -n 400
ghidra decompile 0x00630850
ghidra decompile 0x00630910
ghidra disasm 0x00630660 -n 30
ghidra disasm 0x006306A0 -n 30
ghidra disasm 0x006306F0 -n 30
ghidra disasm 0x0058CCA0 -n 900
ghidra disasm 0x00586870 -n 30
ghidra x-ref to 0x006B5E40
ghidra x-ref to 0x006B7840
ghidra x-ref to 0x006B7A40
ghidra decompile 0x006B7600
ghidra decompile 0x006B6B30
ghidra decompile 0x006B6280
ghidra decompile 0x006B6480
ghidra decompile 0x006B6690
ghidra disasm 0x006B6850 -n 170
ghidra disasm 0x006B6A4F -n 120
ghidra disasm 0x006B6480 -n 130
ghidra disasm 0x006B6280 -n 140
ghidra disasm 0x00C9A480 -n 8
ghidra disasm 0x00632C40 -n 12
ghidra disasm 0x00631590 -n 20
ghidra disasm 0x006315D0 -n 30
ghidra disasm 0x006BE3D0 -n 40
ghidra disasm 0x006306D0 -n 8
ghidra memory read 0x012BF2E8 24
ghidra memory read 0x012BF300 36
ghidra memory read 0x00FD3ED4 100
ghidra memory read 0x012B8548 16
ghidra memory read 0x012B8558 16
ghidra memory read 0x012B8568 12
ghidra memory read 0x012BF2A8 96
ghidra memory read 0x012BF360 16
ghidra memory read 0x00FD2C00 160
ghidra memory read 0x00FD2CB0 128
ghidra memory read 0x00FD2D90 96
```

Dereference each pointer-table entry with a further `ghidra memory read` to
read its string. Apply the lane table to the sample dwords by hand or with a
short script. Stop each disassembly at the first plain return of the function
under test.
