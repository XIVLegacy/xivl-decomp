# Man304 cutscene actor records

The 1.23b client is stamped `2012.09.19.0001` by `game.ver`. The four
`client/cut/<scene>/<scene>` files below contain actor dictionaries and
spatial records. A read-only structural scan used the record signature,
actor-index join, coordinate fields, and float checks described in
[Man308 cutscene actor records](man308-scene-actor-records.md). The
`decompile_man304_cutscene_setup.py` scene selector had SHA-256
`b92586cace957f93e2ff34f26b51bbffa088676a1b73dd24eaa26957619c50ee`;
its shared structural reader had SHA-256
`cd37fc01ebd25ed794eb688d8461d1002e3f8d8dacae58f9a8abef54ecd00995`.
The expected scene sizes, actor counts, tokens, and PC transforms checked
decoded results after discovery rather than locating records.

| Scene | Bytes | SHA-256 | Dictionary records | Selected spatial opcode |
| --- | ---: | --- | ---: | ---: |
| `man30400` | 294,992 | `eed20480ee59234fb342ac1d891d66d19cd7747cf608702de9ba7d8ebc0fcd01` | 11 | 7 |
| `man30410` | 55,104 | `83905139eefdc59b3a51390a34542f95f7d2c70aede057652d1e25633d8bbaa6` | 8 | 7 |
| `man30420` | 70,832 | `4be7383a94189685f636361c022663e25f5cd510eb23afc90c0b11038ceac124` | 12 | 5 |
| `man30430` | 116,160 | `91fec6d12f54b30b52e8dbc864406097faca04be31e6128ec766ba03512924ae` | 4 | 5 |

The numeric tokens below are little-endian words at actor-dictionary record
`+0x30`. Offsets locate the beginning of each record in the named scene.

| Scene | Selected authored labels and numeric tokens |
| --- | --- |
| `man30400` | `MINFILIA` 1000843 at `0x258C4`; `HEDYN` 1001047 at `0x25A2C`; `Sylph_a` 1001085 at `0x259B4`; `Sylph_b` 1001086 at `0x259F0`; `Sylph_NYXIA` 1001385 at `0x25AC4`; `Sylph_GONAXIA` 1001386 at `0x25B00`; `Sylph_PANNIXIA` 1001388 at `0x25B3C` |
| `man30410` | `Sylph_a` 1001085 at `0x2E4`; `Sylph_b` 1001086 at `0x320`; `Sylph_NYXIA` 1001385 at `0x35C`; `Sylph_GONAXIA` 1001386 at `0x398`; `Sylph_PANNIXIA` 1001388 at `0x3D4` |
| `man30420` | `MINFILIA` 1000843 at `0x240`; `snpc` 0 at `0x354` |
| `man30430` | `MINFILIA` 1000843 at `0x230`; `snpc` 0 at `0x2A8` |

The following selected `0x40`-byte records store three little-endian float32
coordinates at record `+0x10` and rotation at `+0x20`. The displayed values
are rounded to three decimals; each offset and actor index identifies the
underlying bytes.

| Scene / actor | Spatial offset / opcode | Position `(x, y, z)`, rotation |
| --- | --- | --- |
| `man30400` / `PC` index 5 | `0x261BC` / 7 | `(-39.280, -0.006, -1.616)`, `-3.142` |
| `man30410` / `PC` index 4 | `0xA50` / 7 | `(-33.488, -1.954, -36.533)`, `2.883` |
| `man30420` / `PC` index 5 | `0xC9C` / 5 | `(-28.064, -3.000, -48.384)`, `-2.117` |
| `man30430` / `PC` index 5 | `0x1088` / 5 | `(35.392, 1.204, -0.899)`, `1.561` |

These are authored cutscene actor records and scene-local transforms. They
do not establish persistent world placements, actor-class paths, quest-marker
ownership, server event dispatch, content-instance creation, reward delivery,
or the historical order in which retail selected these scenes. A zero `snpc`
token does not identify the companion selected at runtime.
