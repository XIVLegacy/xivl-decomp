# Ferry scene inventory

The installed client is stamped `2012.09.19.0001` by `game.ver`.
The read-only `tools/decompile_ferry_scenes.py` extractor (SHA-256
`134b27fec603154cf83b216369560e3aa6cf49b666d79fc6024aea1e5c3952a8`)
hash-checks two installed movie resources, reads their actor dictionaries,
and decodes the contained SEDBSCB timeline blocks. A fresh build and check
of its seven output artifacts agreed. The installed `client/cut/` tree has
exactly two immediate directory names beginning `vsl`.

| Scene resource | Bytes | SHA-256 | Actor records | Timeline clips |
| --- | ---: | --- | ---: | ---: |
| `client/cut/vsl0l010/vsl0l010` | 514,224 | `6df2130a46e3683e94b1522b4d1bce4399464b7fca28698d1e1e90c4f5ccf083` | 15 | 173 |
| `client/cut/vsl0u010/vsl0u010` | 517,936 | `fef60bfdfa00ca8c26d697569cdcaefdb1192d66858f1f7ab0d35a369bd06b80` | 16 | 181 |

Both actor dictionaries include a `Ship` record with actor class ID
1200091 and a player binding. Both scenes have six timeline blocks,
including a zero-clip folder block. `vsl0l010` has named `board`, `deck`,
`send off`, and `voyage` blocks; its decoded background-action clips name
`sea_s0_lin01` and target `isgrp_000456`. The corresponding clips in
`vsl0u010` name `wil_w0_lin01` and the same target. The actor and clip
counts describe authored scene resources, not server spawn rosters or
voyage duration.

The labels and background references are consistent with origin-side
boarding sequences. The two-resource `vsl*` directory inventory supplies
no separately named arrival movie in that family. It does not prove which
scene a historical server selected, whether another family supplied an
arrival sequence, or the retail timing of zone transfer. Raw SEDBSCB time
units and scene-local actor positions must not be treated as travel
seconds or persistent world coordinates.
