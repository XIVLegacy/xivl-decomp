# Man308 cutscene actor records

The installed client is stamped `2012.09.19.0001` by `game.ver`. The files
below are `client/cut/<scene>/<scene>` resources. A read-only structural scan
of their bytes found actor-dictionary records beginning with `3c 00`, with
little-endian words 5 at record `+0x20`, `0xffffffff` at `+0x28`, and 2 at
`+0x2c`. The label is at `+4`, the actor index at `+3`, and the numeric actor
token at `+0x30`. The scan then selected `0x40`-byte spatial records whose
actor index joins that dictionary, whose coordinates and rotation are finite,
and whose opcode matches the scene-specific value below. The read-only
`decompile_man308_cutscene_setup.py` and its shared structural reader
`decompile_man300_cutscene_setup.py` had SHA-256
`2f9933db549ac0e32e01b73f9b441e8cf6ee4002ad9f84173b109babe601b02d`
and `cd37fc01ebd25ed794eb688d8461d1002e3f8d8dacae58f9a8abef54ecd00995`
respectively. Their expected values check decoded records after discovery;
they are not used to locate them.

| Scene | Bytes | SHA-256 | Dictionary records | Selected spatial opcode |
| --- | ---: | --- | ---: | ---: |
| `man30800` | 89,312 | `77898b7d13b44fbe9291d55a4437256a99f84f0d68d9d98bd78a68a6b689d0ed` | 4 | 1 |
| `man30810` | 62,992 | `c6d4e7c29afa64ecbc48eba09547ee031adafe4f17274f36ad90a9298a8cd988` | 9 | 1 |
| `man30820` | 49,056 | `43a4f2da7a6d967c448f5a095f7b81d309c4187b74e5e8da341853dd9ecacf29` | 11 | 9 |
| `man30830` | 16,304 | `c4e663ff2a38cdccd5aaea3327c234eb5b4d778e3b8c9c0a0725b5dc68aab1bf` | 5 | 1 |
| `man30850` | 4,777,472 | `2bad417f5fde6110215fa5872a11d0b5522d15d70b47e4162f7c5fd682b86474` | 15 | 2 |
| `man30860` | 9,360 | `531f23a31868660db1eccd29eeb5bf22d37420cb0698db011bbdd4e6d6634bf5` | 3 | 2 |
| `man30880` | 2,358,128 | `ad3502659271a9d5a5dc384b7a8b2315a193bd693ff34ed8912077bb3c4a74bc` | 15 | 15 |
| `man30890` | 94,496 | `18742ab8972d9093dc2c4e134ae5e66aad45cacb49a45dd47c933da469b12100` | 7 | 6 |
| `man30900` | 44,960 | `eb9d6018701bd8fe90a16c091716f16270b05725b16ac61b5600328b9ca0d60f` | 7 | 8 |
| `man40640` | 9,909,200 | `992d373847cedea4ea794aaf169d5c4bde5a9d63eb3aa66cad6062108e9646c0` | 27 | 26 |

The following actor-dictionary joins are direct scene-file records. Offsets
locate the start of the record within the named file; names are authored
labels, not a resolved server actor or class-path mapping.

| Scene | Label and numeric token at record offset |
| --- | --- |
| `man30830` | `Amalja_A` 1000517 at `0x2B8`; `Amalja_B` 1000518 at `0x2F4`; `Amalja_E` 1000612 at `0x330` |
| `man30850` | `etra_F_Lal_A` 6000116 at `0x120F0C`; `etra_F_Lal_B` 6000117 at `0x1211E4`; `etra_M_Elz` 6000118 at `0x120F84`; `etra_M_Hur` 6000119 at `0x120F48`; `IFLEAT` 6000242 at `0x121184` |
| `man30860` | `Amalja_P_A` 2206513 at `0x220`; `Amalja_D` 2206506 at `0x25C`; `Amalja_E` 2206506 at `0x298` |
| `man30880` | `etra_F_Lal_A` 6000116 at `0x15E1F0`; `etra_F_Lal_B` 6000117 at `0x15E4A4`; `etra_M_Elz` 6000118 at `0x15E268`; `etra_M_Hur` 6000119 at `0x15E22C`; `IFLEAT` 6000242 at `0x15E468` |
| `man40640` | `c006a0` 6000116 at `0x5ACA78`; `c006b0` 6000117 at `0x5ACAB4`; `c003c0` 6000118 at `0x5ACAF0`; `c001d0` 6000119 at `0x5ACB2C`; `m852e0` 6000242 at `0x5ACB68` |

Selected `0x40`-byte spatial records have three little-endian float32
coordinates at record `+0x10` and rotation at `+0x20`. Values below are rounded
to three decimals; the offset, opcode, and joined actor index identify the
underlying bytes.

| Scene / actor | Record offset / opcode | Position `(x, y, z)`, rotation |
| --- | --- | --- |
| `man30810` / `PC` index 4 | `0xDBC` / 1 | `(1134.053, 312.430, 830.706)`, `-1.649` |
| `man30810` / `PC` index 4 | `0x1EE0` / 1 | `(1000.714, 308.565, 985.779)`, `1.541` |
| `man30830` / `PC` index 4 | `0xBC0` / 1 | `(995.168, 309.146, 982.116)`, `-2.519` |
| `man30830` / `Amalja_A` index 7 | `0x990` / 1 | `(990.180, 309.682, 979.995)`, `1.795` |
| `man30830` / `Amalja_B` index 8 | `0x9D0` / 1 | `(992.990, 309.931, 976.904)`, `-0.001` |
| `man30830` / `Amalja_E` index 9 | `0xC00` / 1 | `(992.674, 309.542, 979.550)`, `0.822` |
| `man30860` / `Amalja_P_A` index 5 | `0x6F0` / 2 | `(2535.898, 249.642, 2219.305)`, `-2.178` |
| `man30890` / `PC` index 4 | `0xB4E8` / 6 | `(1217.650, 311.707, 776.001)`, `-0.931` |

These are cutscene-local actor tokens and authored transforms. The scan does
not establish persistent world-spawn positions, hostility, server-side quest
ownership, private-area creation, party limits, combat sequence, or when a
historical retail client played the scenes. In particular, the three labels
in `man30860` carry only two distinct numeric tokens; they are not proof of
three distinct actor classes or a three-kill objective.
