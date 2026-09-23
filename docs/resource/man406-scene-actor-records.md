# Man406 cutscene actor records

The installed client is stamped `2012.09.19.0001` by `game.ver`. A read-only
structural scan of the installed `client/cut/<scene>/<scene>` files below used
the actor-dictionary signature, actor-index join, and `0x40`-byte spatial
record fields described in [Man308 cutscene actor records](man308-scene-actor-records.md).
The `decompile_man406_cutscene_setup.py` scene selector had SHA-256
`e2a92df6df4c46d03008581f1330fba05d0cc4a136c24f8a9ea4c714221b9ea7`;
its shared structural reader had SHA-256
`cd37fc01ebd25ed794eb688d8461d1002e3f8d8dacae58f9a8abef54ecd00995`.
Expected values checked the decoded output after record discovery. The
additional `man40640` scene is cataloged with its hash and actor records in
the Man308 finding; it is not a separate resource here.

| Scene | Bytes | SHA-256 | Dictionary records | Selected spatial opcode |
| --- | ---: | --- | ---: | ---: |
| `man40600` | 61,984 | `259c8f7b95cd24ccd1488643ab19706e177d6e39a5a5e34b36c0c30744ea3499` | 13 | 6 |
| `man40610` | 77,360 | `55c06dbe3466a5d4e27c5e728957e99cd12b88fbfd9a7a1f6afe5137ce8c7d38` | 12 | 5 |
| `man40615` | 85,664 | `92b5dcd81d48c3f93bbbea56753116cf21bf2fcd79261b40c717b4b59da469f1` | 13 | 5 |
| `man40620` | 197,632 | `23101c8b77d8e97a045dbe4b7e5616a2e646339142e0554c4ecf269d6a9a06b0` | 15 | 5 |
| `man40625` | 386,144 | `f7cd63423b4ce235f49d00ef7a696a38822251ea2b875967e9d483467329ca56` | 12 | 4 |
| `man40630` | 153,264 | `40d068358100efb64199efc125c7063476df25a23d32c78be40a4e782631fbae` | 14 | 5 |
| `man40635` | 16,254,448 | `99a200cb48d55f672211441ae574252a05ad96a714702d058d66f3492f78aef7` | 52 | 30 |
| `man40645` | 56,416 | `6bc039f95c160850d5d428a273b51d2629b8f036b098881bd221c19b15e22c90` | 6 | 3 |
| `man40650` | 311,760 | `7282bc078f9c96b1526a6be8fb693c9682d696ca026d3073a62d178f68669b91` | 19 | 3 |
| `man40660` | 45,296 | `71b956c8b4d9a42a4f652995326896a8586a4d9a2308e239810d04191c740b7a` | 9 | 0 |

The numeric tokens below are little-endian words at actor-dictionary record
`+0x30`; offsets locate record starts. The repeated `teikoku_*` labels carry
different tokens in the two scene files, so their names cannot be substituted
for numeric identity.

| Scene | Selected authored labels and numeric tokens |
| --- | --- |
| `man40600` | `MINFILIA` 1000843 at `0x240`; `aramigo_leader` 1000477 at `0x354` |
| `man40620` | `teikoku_elm` 1001243 at `0x23C14`; `teikoku_hm` 1001244 at `0x23B9C`; `teikoku_hf` 1001245 at `0x23BD8` |
| `man40625` | `teikoku_elm` 2207001 at `0x13B60`; `teikoku_hm` 2280003 at `0x13BD8`; `teikoku_hf` 2280006 at `0x13C14` |
| `man40645` | `Child-A_HFK` 1000957 at `0x4B40`; `Child-B_HFM` 1000958 at `0x4C30`; `Child-C_HFM` 1000959 at `0x4BF4`; `Child-D_HFK` 1000960 at `0x4BB8`; `Child-E_HFM` 1000961 at `0x4B7C` |
| `man40650` | `EmpireBoss_EM` 1500131 at `0x1B1BC` |
| `man40660` | `Tatal_LF` 1001046 at `0x1E0`; `Minfilia` 1000843 at `0x258` |

Selected spatial records have float32 position at record `+0x10` and
rotation at `+0x20`. Values below are rounded to three decimals; offsets,
opcodes, and joined actor indexes identify the underlying bytes.

| Scene / actor | Spatial offset / opcode | Position `(x, y, z)`, rotation |
| --- | --- | --- |
| `man40600` / `MINFILIA` index 4 | `0xE0C` / 6 | `(39.330, 1.205, 0.022)`, `-1.641` |
| `man40620` / `PC` index 4 | `0x24748` / 5 | `(-218.470, 18.542, -666.627)`, `-2.817` |
| `man40620` / `teikoku_hm` index 11 | `0x2435C` / 5 | `(-213.322, 18.826, -689.771)`, `-1.104` |
| `man40625` / `PC` index 4 | `0x14338` / 4 | `(-70.983, 19.746, -703.104)`, `1.449` |
| `man40625` / `teikoku_elm` index 12 | `0x15320` / 4 | `(7.057, 20.171, -704.071)`, `1.591` |
| `man40625` / `teikoku_hm` index 14 | `0x15360` / 4 | `(5.713, 20.099, -705.471)`, `1.591` |
| `man40625` / `teikoku_hf` index 15 | `0x151EC` / 4 | `(4.897, 20.049, -702.149)`, `1.449` |
| `man40645` / `PC` index 2 | `0x5478` / 3 | `(265.470, 56.408, -799.867)`, `-1.654` |
| `man40650` / `EmpireBoss_EM` index 11 | `0x1C08C` / 3 | `(226.362, 62.012, -787.956)`, `2.381` |
| `man40650` / `PC` index 4 | `0x1CD10` / 3 | `(-205.201, 18.645, -680.202)`, `2.266` |

No actor dictionary in the eleven inspected `man406*` scene files contains
numeric token `2202401`. This bounded absence does not exclude an actor
created by a separate battle system. Scene records do not establish
persistent world placements, hostility, pursuit waypoints or timing, a
four-enemy objective, party size, server quest progression, or historical
retail scene selection. The scene-local coordinates must not be promoted to
server spawn positions without separate evidence.
