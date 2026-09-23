# Man300 cutscene actor records

The 1.23b client is stamped `2012.09.19.0001` by `game.ver`. A read-only
scan of the seven `client/cut/<scene>/<scene>` resources below decoded
actor-dictionary records using the signature and field offsets in
[Man308 cutscene actor records](man308-scene-actor-records.md). The
`decompile_man300_cutscene_setup.py` reader had SHA-256
`cd37fc01ebd25ed794eb688d8461d1002e3f8d8dacae58f9a8abef54ecd00995`.
It reads sequential `setup` records for the smaller scenes and scans the
whole composite files for `0x40`-byte spatial records with a joined actor
index, finite coordinates, and a scene-selected opcode. Expected values check
decoded results after discovery; they do not locate the records.

| Scene | Bytes | SHA-256 | Dictionary records | Spatial read |
| --- | ---: | --- | ---: | --- |
| `man30000` | 105,600 | `536d860ef5604f655589b913b4f8bcf2982ab449bb78d83f08359c0f0f9d9b26` | 3 | `setup` stream |
| `man30010` | 105,488 | `7cd9c52466747be2bd46d40cad9fbd3554931282d65517783e107db3815a5ff9` | 11 | `setup` stream |
| `man30020` | 130,736 | `40a260dc9264aea9079940296d2d57b947d33a58182f4ac5a4b1ba1e4567707e` | 8 | `setup` stream |
| `man30030` | 2,239,504 | `d6e7df9bfd79e83b37c98afe7ebf04e533f5b0f417b2f4b47acf6a87dd2d83a9` | 17 | whole-file opcode 6 |
| `man30040` | 76,560 | `c8b78c0cba6772e5309fc9d0bda474e623f72aa205400f4c7d64fc0140473e07` | 10 | whole-file opcode 4 |
| `man30050` | 819,872 | `97757ba4dea05f96b2455c3836c75af173a7d2cc975fe7b2999e77023bcd9135` | 15 | whole-file opcode 16 |
| `man30060` | 38,752 | `6eb5d233cad0b38be1c742fd065755612c90c1d99489c10d84c5fdf11609ef1c` | 5 | `setup` stream |

Selected actor-dictionary records carry these numeric tokens at record
`+0x30`. The offsets locate the starts of records in the named scene files.
The authored labels are not server class-path mappings or proof of interactive
actor ownership.

| Scene | Label and numeric token at record offset |
| --- | --- |
| `man30000` | `MINFILIA` 1000843 at `0xEF10` |
| `man30010` | `HEDYN` 1001047 at `0x13F14` |
| `man30020` | `SHANGA_MESHANGA` 1001048 at `0x230`; `Sylph_Troxia` 1001049 at `0x320`; `HEDYN` 1001047 at `0x398` |
| `man30030` | `nananobi` 1001050 at `0x1B8EB8`; `S_ALMXIO` 1001085 at `0x1B8EF4`; `S_ZOXIO` 1001086 at `0x1B8F30`; `I_ixal03` 1001095 at `0x1B9098`; `A_amarujya03` 1001098 at `0x1B8FE4` |
| `man30040` | `S_ALMXIO` 1001085 at `0xF240`; `S_ZOXIO` 1001086 at `0xF27C`; `SCALELIZARD_FIR` 1001099 at `0xF2B8` |
| `man30050` | `NANABINO` 1001050 at `0x65B8C`; `Asien` 1001174 at `0x65E40`; `Crystal` 6500003 at `0x65E7C` |
| `man30060` | `HEDYN` 1001047 at `0x2A8`; `NANANOBY` 1001050 at `0x304` |

Selected `0x40`-byte spatial records hold three little-endian float32
coordinates at record `+0x10` and rotation at `+0x20`. Values are rounded to
three decimals; offsets, opcodes, and actor indexes identify the source bytes.

| Scene / actor | Spatial offset / opcode | Position `(x, y, z)`, rotation |
| --- | --- | --- |
| `man30000` / `MINFILIA` index 4 | `0xF37C` / 3 | `(39.330, 1.205, 0.022)`, `-1.571` |
| `man30020` / `HEDYN` index 10 | `0x9C0` / 7 | `(-39.317, -0.006, -2.861)`, `0.000` |
| `man30030` / `PC` index 0 | `0x1BAC84` / 6 | `(998.925, 253.076, -280.176)`, `3.124` |
| `man30030` / `S_ALMXIO` index 7 | `0x1BA884` / 6 | `(969.014, 250.484, -317.854)`, `-2.192` |
| `man30030` / `S_ZOXIO` index 8 | `0x1BABC4` / 6 | `(968.186, 250.635, -316.680)`, `-2.534` |
| `man30040` / `PC` index 0 | `0xF850` / 4 | `(1016.166, 250.998, -270.654)`, `3.124` |
| `man30050` / `PC` index 4 | `0x66BF4` / 16 | `(1054.047, 251.661, -282.217)`, `2.381` |
| `man30060` / `PC` index 4 | `0x6CC` / 5 | `(-39.317, -0.006, -1.520)`, `3.131` |

These are authored scene-local actor records and transforms, not persistent
world placements. They do not establish a four-target combat objective,
Parley alternatives, nonlethal HP floor, private-area entry, party rules,
reward timing, or historical retail scene invocation. Later in-game
positions and contributor-server routing must not be substituted for these
cutscene bytes.
