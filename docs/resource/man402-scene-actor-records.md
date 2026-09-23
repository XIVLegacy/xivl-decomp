# Man402 cutscene actor records

The installed client is stamped `2012.09.19.0001` by `game.ver`. A read-only
structural scan of four `client/cut/<scene>/<scene>` files used the
actor-dictionary signature and spatial record fields described in
[Man308 cutscene actor records](man308-scene-actor-records.md). The
`decompile_man402_cutscene_setup.py` scene selector had SHA-256
`c27991d0d0a81c0e9201646889b61c857eb07e973b27fdf7de4992628018cd98`;
its shared structural reader had SHA-256
`cd37fc01ebd25ed794eb688d8461d1002e3f8d8dacae58f9a8abef54ecd00995`.
Expected values checked decoded records after discovery rather than finding
them.

| Scene | Bytes | SHA-256 | Dictionary records | Selected spatial opcode |
| --- | ---: | --- | ---: | ---: |
| `man40200` | 31,216 | `b3d1beb49e162709a234f8166efbc82591269403e55aa8cf3797419e2330b97e` | 8 | 2 |
| `man40210` | 287,456 | `a91e33d499882953a0fe844f615ee7ddb2ddc6ca3035fe575cc53ab6d59b7e67` | 17 | 7 |
| `man40220` | 20,976 | `5ca195d839a1c89ea103543e0a418c6b54cc8cced998f7b8252770ed8664fbdd` | 6 | 7 |
| `man40230` | 67,648 | `19be89e42a316aedab3d7e8c019147bbfd8677ba0dd0e2d730381f097e172f80` | 8 | 2 |

Selected actor-dictionary records carry these numeric tokens at record
`+0x30`; offsets locate record starts within the named files. The two
`mon_haiena` labels have the same numeric token in the scene, not a proved
runtime battle-class identity.

| Scene | Authored label and numeric token at record offset |
| --- | --- |
| `man40200` | `TATARU` 1001046 at `0x230`; `MINFILIA` 1000843 at `0x2A8` |
| `man40210` | `aramigo_rrm` 1000480 at `0x30498`; `aramigo_hf` 1000478 at `0x304D4`; `aramigo_mc` 1001239 at `0x306D4`; `mon_haiena1` 1001370 at `0x30600`; `mon_haiena2` 1001370 at `0x3063C` |
| `man40220` | `aramigo_jyujyut` 1001239 at `0x2D4`; `sNPC` 0 at `0x25C` |
| `man40230` | `TATARU` 1001046 at `0x230` |

Selected `0x40`-byte spatial records hold three little-endian float32
coordinates at record `+0x10` and rotation at `+0x20`. Values are rounded to
three decimals; offsets, opcodes, and joined actor indexes identify the
underlying bytes.

| Scene / actor | Spatial offset / opcode | Position `(x, y, z)`, rotation |
| --- | --- | --- |
| `man40200` / `TATARU` index 4 | `0xAB8` / 2 | `(-38.994, 0.000, -2.514)`, `-0.808` |
| `man40210` / `PC` index 3 | `0x33E20` / 7 | `(1690.881, 20.171, -857.553)`, `2.539` |
| `man40210` / `sNPC` index 4 | `0x31870` / 7 | `(1694.418, 19.997, -861.227)`, `-2.319` |
| `man40210` / `aramigo_mc` index 15 | `0x31C7C` / 7 | `(1984.491, 31.996, -1680.115)`, `-0.302` |
| `man40210` / `mon_haiena1` index 11 | `0x310E8` / 7 | `(1987.319, 32.733, -1713.693)`, `-0.339` |
| `man40210` / `mon_haiena2` index 12 | `0x310A8` / 7 | `(1989.393, 32.656, -1714.478)`, `-0.339` |
| `man40220` / `PC` index 4 | `0x828` / 7 | `(1870.347, 19.690, -1731.395)`, `1.457` |
| `man40230` / `TATARU` index 4 | `0xAA8` / 2 | `(-38.994, 0.000, -2.514)`, `-0.808` |

None of these four scene dictionaries contains numeric token `2201417`.
That bounded absence does not refute a separately selected battle actor.
Scene-local appearances and transforms do not establish persistent spawns,
escort waypoints or leash timing, a two-kill objective, party policy,
rewards, or historical retail scene selection. A zero `sNPC` token does
not identify the companion selected at runtime.
