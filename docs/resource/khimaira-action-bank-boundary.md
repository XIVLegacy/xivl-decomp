# Khimaira action-bank boundary

The installed client is stamped `2012.09.19.0001` by `game.ver`. It has
WSS banks 0010 through 0020 under
`client/chara/mon/m047/act/emp_emp/wss/base/`. The table records exact
file SHA-256 values and selected visible ASCII literals from a raw-byte
scan. These are resource observations, not a command-to-bank join.

| WSS bank | Bytes | SHA-256 | Visible motion literal |
| ---: | ---: | --- | --- |
| 10 | 3,296 | `06d7ea552d2de1391f55128048e8fd979bdfd0e4fe29092e43872bb854e22380` | No `cbbm_` literal found |
| 11 | 35,824 | `c3d7569b61c7e7f6fe80efc2ed2a874d703c2fa695e1d091834d618d5e926ac2` | `cbbm_sp_02` |
| 12 | 36,240 | `88fb21dc1367ec5f255c3d2bb4abb49bbcf2ae5cddf26a002af01fee51ff2b51` | `cbbm_sp_03` |
| 13 | 28,112 | `004b1c0de58acd7fada25e89b7166630c72f9393cb29efce8993a1462c63ab3d` | `cbbm_sp_04` |
| 14 | 28,112 | `004b1c0de58acd7fada25e89b7166630c72f9393cb29efce8993a1462c63ab3d` | `cbbm_sp_04` |
| 15 | 26,656 | `99e600a3b23200c0b446e2f6c8a09b516d06558a1a2922ae24ca1a1a7e433c2a` | `cbbm_sp_05` |
| 16 | 26,656 | `99e600a3b23200c0b446e2f6c8a09b516d06558a1a2922ae24ca1a1a7e433c2a` | `cbbm_sp_05` |
| 17 | 26,976 | `fb240d77a7e752769255f075cb64fcb1ca9ac23cec79f64c8272ac737a9394dd` | `cbbm_sp_06` |
| 18 | 26,976 | `fb240d77a7e752769255f075cb64fcb1ca9ac23cec79f64c8272ac737a9394dd` | `cbbm_sp_06` |
| 19 | 26,272 | `fab54e2c65d58b2275de298695b1091d84f7a61079e1b537f972ad2aac913611` | `cbbm_pb04_dmg` |
| 20 | 27,008 | `a0460bb00a6cda7158cbd6160775498f1ea0cc506bde17dc4e696d70632c97b9` | `cbbm_sp_01` |

Banks 13/14, 15/16, and 17/18 are byte-identical pairs. Their separate
bank numbers do not by themselves establish distinct motions or which head
or mode each represents. The `pb04` name in bank 19 is an authored literal;
it does not independently identify part 4 as legs or specify a part-HP
formula.

The contributor's Cutter's Cry report proposes a sequential mapping from
commands 23465-23475 to WSS10-20. The installed files above establish the
bank sequence, but do not serialize those command IDs or a retail caller.
The report's cited `KhimairaNormalStandard.lua` part identifiers are in its
server script; the independently recovered client class only declares
inheritance. Thus command mapping, part-ID semantics, breakage bit masks,
timing, and retail selection remain unverified here.
