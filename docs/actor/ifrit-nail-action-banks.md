# Ifrit and Infernal Nail action banks

The installed `client/chara/mon/m852/act` tree has 26 action files:
FID 1110, BID 0000, BTL 0001, MGC 0001-0004, and 19 WSS banks.
WSS 0006, 0009, and 0011 are absent from this m852 path. The
installed m524 Nail tree has only BID 0000 and WSS 0001. These are
path-level inventories, not a complete runtime command selection map.

Selected m852 banks expose distinct authored motion and effect content:

| WSS | Bank SHA-256 | Decoded motion evidence |
| --- | --- | --- |
| 0007 | `45f0c0abde7221e053a1b693ed6ca643676ca8871f3bfe40b0dfc2d044b6eaa7` | Five-frame `cbbm_sp_b02`, with a large local hara-Z change; separately contains `m852_0007_fire` |
| 0008 | `b2ba7c44797e50ee6085f9d4ed0b647ce67d56445f2eccc537602db789878217` | 32-frame `cbbm_sp_b04` complementary body curve |
| 0015 | `1132c275c97bf3af4a8ef0e25b2d166dade14ebb768850c22b9f91a6cea97a98` | 40-frame `cbbm_sp_03` and `cbxs_st0to4` |
| 0016 | `4fbf6d5b77a776f395c27fe5a69e52031c813bcdd5f5434d7fcb5949db73d414` | 30-frame `cbbm_sp_04` and reverse `cbxs_st4to0` |
| 0018 | `05ac1317b3ae797466e9e323927ec51799ebfde1dbd0ec048c04af9575a4d469` | Five-frame `cbbm_sp_05` with upward local hara-Y change |
| 0019 | `8aa13bcd9a161966f6aee5a1b6914e86ebae9dcc43378d120cb7443bb231f4e8` | 30-frame `cbbm_sp_06` with downward local hara-Y change |

The m524 BID 0000 (SHA-256
`d2b2761da954a4704145e7ed644e31d39b20d6742b499d76eba9adfbf5df8c17`)
contains named activation, deactivation, idle, death, held-death,
and state-4 motion/controller resources. The m524 WSS 0001
(SHA-256 `c64e37d06b0cf34df4e5c77d8de4b3fb9c6aad4851af223b61502a07c36e35df`)
contains `cbbm_sp_01`, `cbxs_st0to1`, and actor-bound effect resources.
The `cbbm_sp_01` transform is 90 frames at 30 fps. Its authored motion
length is distinct from the WSS scheduler's active block and any
server-side visibility delay.

All eight listed action-file identities were checked against the
installed client. The contributor's
`docs/ifrit-animation-decomp-2026-08-02/IFRIT_CLIENT_ANIMATION_BANKS.md`
(SHA-256 `852de41825e101b0d875d930343832fcb8a2fdefb090f8e08c53dbe657719fdd`)
and `INFERNAL_NAIL_ANIMATIONS.md` in the same directory
(SHA-256 `df3b6720640eed0dc916481c92407042910fde4c529c01c80ed96fcce4783b84`)
provide bank, nested-resource, and curve locators. The broader
`monster-action-scheduler-contract-20260810/scheduler_manifest.csv`
also indexes the WSS scheduler/resource joins.

Installed banks establish client capability. Bank numbers, command IDs,
effect names, and mechanics are separate namespaces. No retained retail
selector here proves when a battle used these banks, how flames attached,
whether a Nail's death resources played automatically, or how either
presentation was reconstructed for late joiners. Motion frame counts are
not server cast, travel, telegraph, or action-lock durations.
