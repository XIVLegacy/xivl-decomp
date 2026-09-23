# Ifrit and Infernal Nail action banks

The installed `client/chara/mon/m852/act` tree has 26 action files:
FID 1110, BID 0000, BTL 0001, MGC 0001-0004, and 19 WSS banks.
WSS 0006, 0009, and 0011 are absent from this m852 path. The
installed m524 Nail tree has only BID 0000 and WSS 0001. These are
path-level inventories, not a complete runtime command selection map.

The complete installed model roots also include non-action resources:

| Root | Action files | Equipment/model files | Skeleton files | Total bytes |
| --- | ---: | ---: | ---: | ---: |
| `client/chara/mon/m852` | 26 | 6 | 2 | 21,080,376 |
| `client/chara/mon/m524` | 2 | 6 | 1 | 2,889,204 |

All 43 direct-root paths, lengths, and SHA-256 digests were checked
against the installed files and the direct-file manifest in
`EXHAUSTIVE_CLIENT_ASSET_COVERAGE.md`, section "Complete direct file
manifest." The 15 equipment/model and skeleton files are not extra WSS
banks. This root census does not exclude dependencies reached by hashed,
numeric, or executable-generated references outside those paths.

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

The nested `cbbm_sp_01` MTB (1,879 bytes, SHA-256
`dd9ce2e2a0f918a3839101d40a9cb21fe9f7a12d84563727c2dcb3df138d9fd3`)
has a decoded bone-1 `n_hara` local-Y track at payload `+0x100`.
Frames 53, 54, 55, and 90 decode to `+0.082535`, `+0.023412`,
`-0.000071`, and `-0.000071`, respectively; frames 55 and 90 have the
same raw quantized value 31877. The rise curve holds near zero through
its final authored frame rather than containing a terminal descent.
The separate BID `cbbm_id0` MTB (SHA-256
`f6570a520c987a0718234d6ada56ae083ddfbc919d273e25a6b20ece01567011`)
has one frame and a constant bone-1 local Y of `-5.999999523` at payload
`+0x108`. This resource contrast can explain a return to a buried idle
pose if WSS1 releases without another state owning the pose; it does not
prove that this handoff occurred in a historical retail encounter or fix
a server timing value.

Two installed m852 banks reuse the nested m999 impact payloads. Recursive
PWIB/SEDB comparison found 39 of 40 embedded resources byte-identical
between m852 WSS0021 (SHA-256
`d365c2f62241323971e73880bddd3b1908bfa8ffea269d17fa3dd158f25ecd34`)
and m999 WSS0002 (SHA-256
`495d76a562dbe2bebfb2698468dd114cd2aa024e6f22004b9fd0520ddd72cfc1`).
For m852 WSS0022 (SHA-256
`035e5346203e2d9a715b8c89cee14d8f0001fc38d7b43bc1bf74c44db24e78e3`)
and m999 WSS0003 (SHA-256
`0ddecf22508dcd151e302c7484981932b52db02f8ca926c3a114c65168046ac6`),
42 of 43 match. In each pair the one different embedded entry is the
outer `SEDBSCB` action scheduler; nested effect payloads are shared.
This is an asset-equivalence join, not proof that the historical encounter
selected either m852 wrapper or its m999 counterpart. The reproducible
comparison is in `build_ifrit_ground_vfx_decomp.py` and its
`pair_equivalence_detail.json` output.

## Native death-scheduler route

The pinned executable's main-state transition function at VA
`0x007C0E10` calls `0x007AC7F0` for its death-scheduler setup. The
per-frame state machine at VA `0x007BADE0` (RVA `0x003BADE0`) probes
the literal SCB names `dead1`, `dead2`, then `dead`. The strings at
`0x00FE73B8`, `0x00FE73C0`, and `0x00FE73C8`, and their references in
that function, were checked directly in the installed PE with
`pefile` and Capstone x86-32. Thus a normal main-state death can
request an active `dead` scheduler without a separate defeat WSS.

The installed m524/e002 model package at
`client/chara/mon/m524/equ/e002/met_mdl/0001` is 352,064 bytes,
SHA-256 `5a5a4414c7327ca5dd0f04d78677e76827d535126b8db3edd5633a7ea24784ff`.
Its nested `dead` SCB is 1,888 bytes, SHA-256
`5b4b3631be6ac08e27f99efabc685893fe93e11e8146d2b73fabab1e03ea63f6`.
The decoded 12-entry scheduler cancels `init_msb4_1`, includes motion
and move-stop clips, and launches `m524_ded`. The nested VEFF
`151rmjanc_dead1` has SHA-256
`85745f0569c3b2f1aaa0ecf01267f586c86a2f4507b2aa64fa8e051fbc6a5278`.
These nested locators are in
`tools/outputs/ifrit-model-state-decomp-20260805/{resources.csv,scheduler_graph.csv}`;
the native route is in
`IFRIT_GROUND_STATE_AND_NAIL_DEATH_CLOSURE_2026-08-05.md`, section 3.
The whole model package hash was checked against the installed file.

This closes the static death-selection mechanism and the e002 asset's
availability, not live root precedence when more than one model
resource is active, actual visual playback, or a retail corpse lifetime.

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
which Nail death resource root won in a live actor, or how either
presentation was reconstructed for late joiners. Motion frame counts are
not server cast, travel, telegraph, or action-lock durations.
