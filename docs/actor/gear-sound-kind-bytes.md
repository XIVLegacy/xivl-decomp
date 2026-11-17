# Gear sound kind bytes

The appearance helper stores one sound kind byte per gear part in the six
bytes at `+0x37A` of its object. This page records how `0x006B6C60` decodes
them from the sheet and which gear part each byte belongs to. The sound
consumers of the bytes and the meaning of each kind are not recorded here.
The sheet column is in the
[var_equip sound kind column](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/var-equip-sound-kind-column.md).

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Disassembly with Ghidra 12.1.4 through ghidra-cli covers `0x006B6C60`,
`0x006B6B30`, `0x006B6CD0` and `0x00C9A480`. A PE section read of the original
file supplies the slot table.

## Row accessor

`0x00C9A480(n)` returns `*(object+8) + 4 * byte[*(*(object+0xC)+0x14) + n]`
(`0x00C9A480..0x00C9A494`). It is the cell accessor that the
[texture number path](appearance-path-decoder.md#texture-number) already uses
with key `0x6D`. `0x006B6B30` reads key `0x6C` of the same row objects at
`0x006B6B4C..0x006B6B53`, and `0x006B6C60` reads key `0x6E` at `0x006B6C84`.
STATIC. INFERRED that all three reads address the `var_equip` row of the gear
word, because they take the same row objects.

## Part order

`0x006B6B30` walks six row objects of its argument array and skips index 4
(`CMP ESI,4` at `0x006B6B40`). The array order is the kind order of
[Row keys requested by 0x006B6850](appearance-path-decoder.md#row-keys-requested-by-0x006b6850):
`top`, `dwn`, `glv`, `sho`, `pch`, `met`. INFERRED for the array, since the
shared loop shape and the skipped index 4 agree with the page that records
`pch` as the part with no redirect.

The slot table at `0x00FD3E80` holds the six bytes 5, 0, 1, 2, 3 and 4. STATIC,
read from the PE file. `0x006B6C60` iterates `i` from 0 to 5 and reads array
element `table[i]` (`0x006B6C70..0x006B6C80`). The result is stored at
`+0x37A + i`. So the six bytes are, in address order:

| Byte | Array element | Part |
| --- | ---: | --- |
| `+0x37A` | 5 | `met` |
| `+0x37B` | 0 | `top` |
| `+0x37C` | 1 | `dwn` |
| `+0x37D` | 2 | `glv` |
| `+0x37E` | 3 | `sho` |
| `+0x37F` | 4 | `pch` |

The weights that `0x006B6CD0` adds per byte are `0x0B`, `0x20`, `0x12`, `0x15`,
`0x10` and `0x17` for `+0x37A` through `+0x37F` (`0x006B6CFA..0x006B6D47`).
Those weights then belong to `met`, `top`, `dwn`, `glv`, `sho` and `pch`. The
byte at `+0x37E` is therefore the `sho` (feet) byte. INFERRED from the array
order above. The footstep entry in
[sound-effect-triggers.md](../resource/sound-effect-triggers.md#footsteps) does
not name these bytes.

## Decode

For each of the six iterations `0x006B6C60` takes the row object. STATIC:

- A null array element stores 6 (`CMP [EBP+EAX*4],0` and `JZ` at
  `0x006B6C77..0x006B6C80`, target `0x006B6C9C`).
- Otherwise it reads the signed cell `v` (`0x006B6C8B`).
  - `v` below 0 replaces the low three bits of the helper dword at `+0x48`
    with `(-v) & 7` (`0x006B6C91..0x006B6C99`) and stores 6.
  - `v` from 1 to 7 stores `v - 1` (`0x006B6CB7..0x006B6CC1`).
  - `v` of 0 and `v` of 8 or more store 6.
- After the loop it ORs 1 into the byte at `+0x44` (`0x006B6CAA`).

So a stored kind is 0 to 5 for cell values 1 to 6 and 6 for every other
case. The installed sheet holds no cell value of 7 or more. The low three bits
of `+0x48` are not decoded here.

## Boundary

The names of kinds 0 to 5, the consumer that maps a kind to a sound resource
and whether the `pch` array element is ever a non-null row object are open.

## Reproduction

Disassemble `0x006B6C60` and `0x006B6CD0` and match the locators. Read six
bytes at `0x00FD3E80` through the PE section table. Count the cell values with
the recipe of the linked sheet page.
