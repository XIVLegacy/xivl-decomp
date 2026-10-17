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

The number `NNNN` is not decoded by any lane. See the open items.

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

Kind 4 submits ID 0 without reading a word. The ID arithmetic and the
meaning of `0x00CA7D70` are `UNKNOWN`. The server-side name COLOR for record
dword 2 is not established by these lanes.

## Open items

- The texture number `NNNN` is `UNKNOWN`. `0x006B7A40` reads it as a dword
  from a per-part request object through `0x00C9A480(object, 0x6D)`. For
  gear, the same routine reads index `0x6C` of that object and, when the
  dword is nonzero, replaces the model number with a signed 16-bit value read
  at index `number - 1` of the object in a second parallel array. The producer
  of the two arrays (the `param_2` of `0x006B7A40`, passed down from
  `0x006B8680`) is not traced.
- The role of the companion `9999` and `9998` files, and of the bank slots
  `0x0B`, `0x0C` and `0x0E` passed to `0x006320C0`, is `UNKNOWN`. This page
  makes no skeleton or PWIB claim.
- Categories 2 and 3 are decoded by `0x006B6480` and `0x006B6280`. Their path
  shapes are not recorded here.
- Weapon words (record dwords 6..12) and the nine words at helper `+0xB0`
  (record dwords 19..27, passed to `0x006B6690`) are not decoded here.
- Record dwords 1 and 5, the face lane bits 29:14 and 7:0, the gear lane bits
  9:0, and the bits 31:30 lane have no path role established here.
- A `_snd` sibling path branch for the `top` slot exists in `0x006B7A40` and
  is not recorded here.
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
