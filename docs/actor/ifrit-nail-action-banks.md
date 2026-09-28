# Ifrit and Infernal Nail action banks

The `client/chara/mon/m852/act` tree has 26 action files:
FID 1110, BID 0000, BTL 0001, MGC 0001-0004, and 19 WSS banks.
WSS 0006, 0009, and 0011 are absent from this m852 path. The m524 Nail tree
has only BID 0000 and WSS 0001. These are
path-level inventories, not a complete runtime command selection map.

The complete model roots also include non-action resources:

| Root | Action files | Equipment/model files | Skeleton files | Total bytes |
| --- | ---: | ---: | ---: | ---: |
| `client/chara/mon/m852` | 26 | 6 | 2 | 21,080,376 |
| `client/chara/mon/m524` | 2 | 6 | 1 | 2,889,204 |

All 43 direct-root paths, lengths, and SHA-256 digests were checked
against the 1.23b files and the direct-file manifest in
`EXHAUSTIVE_CLIENT_ASSET_COVERAGE.md`, section "Complete direct file
manifest." The 15 equipment/model and skeleton files are not extra WSS
banks. This root census does not exclude dependencies reached by hashed,
numeric, or executable-generated references outside those paths.

An independent case-sensitive binary literal scan of the
`client` tree (`rg -a -l -F -e m852 -e m524 --no-ignore --hidden`)
found seven matching files outside those two direct model roots. The
tree contained 51,772 files totaling 5,469,534,267 bytes at this
check; this count differs from the 51,111-file source report, so the
scan scope is recorded here rather than inheriting its count. Each
external hit's bytes and SHA-256 matched the source report:

| External path under `client/` | Bytes | SHA-256 | Literal-reference boundary |
| --- | ---: | --- | --- |
| `cut/sum6a000/sum6a000` | 2,563,296 | `2b2e7cfddf8effc655279e5645b7e436c80b2149ef556eb650daa3705ea1e66f` | m852 cinematic paths |
| `cut/man30880/man30880` | 2,358,128 | `ad3502659271a9d5a5dc384b7a8b2315a193bd693ff34ed8912077bb3c4a74bc` | m852 cinematic paths |
| `cut/man30850/man30850` | 4,777,472 | `2bad417f5fde6110215fa5872a11d0b5522d15d70b47e4162f7c5fd682b86474` | m852 cinematic paths |
| `cut/man40640/man40640` | 9,909,200 | `992d373847cedea4ea794aaf169d5c4bde5a9d63eb3aa66cad6062108e9646c0` | m852 cinematic paths |
| `chara/mon/m999/act/emp_emp/wss/base/0001` | 275,472 | `a849d146a606332e773e6f151a61bbbdfb22d6a15102a8f4c0c3d93881113a94` | `mon\ifrit_852\skill09` |
| `chara/mon/m999/act/emp_emp/wss/base/0006` | 490,912 | `4eaa0bf9aeba22ae4b0fffb56425d080ea12aed36a1673139b7854b43c52308b` | m852 spillover token |
| `chara/mon/m526/act/emp_emp/bid/base/0000` | 44,448 | `23e5a6fa702023a5f9d94de5d96c12ab5335e8aca2c39f44da6f61274de50a7a` | `m524e001` and `skl_m524b001` |

The cut resources are authored cinematic capabilities, not combat-bank
selection. The m526 body motions have five bones, versus the Nail's
12, so its literal reference is not a Nail motion-selection join.
The search cannot rule out dependencies encoded without either literal,
including hashed or numeric references; none of these files establishes
the historical encounter's active command or asset selector.

Parsing the 28 direct action containers' outer `SEDBRES` tables yields
251 live resource entries. A row is counted only when its type, size,
and live field are nonzero:

| Root | SCB | MCB | MTB | nested RES | CIBT | CIBC | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m852 | 57 | 54 | 54 | 22 | 36 | 1 | 224 |
| m524 | 2 | 9 | 9 | 1 | 5 | 1 | 27 |

The CIBT/CIBC type words are `0x63696274`/`0x63696263`; the other
types are identified by their `SEDB` payload tags. This is an outer-table
content census, not a complete recursive dependency graph or retail
action-selection map. The source tables and per-entry locators are in
`EXHAUSTIVE_CLIENT_ASSET_COVERAGE.md`, section "Complete outer
action-resource catalog."

Selected m852 banks expose distinct authored motion and effect content:

| WSS | Bank SHA-256 | Decoded motion evidence |
| --- | --- | --- |
| 0007 | `45f0c0abde7221e053a1b693ed6ca643676ca8871f3bfe40b0dfc2d044b6eaa7` | Five-frame `cbbm_sp_b02`, with a large local hara-Z change; separately contains `m852_0007_fire` |
| 0008 | `b2ba7c44797e50ee6085f9d4ed0b647ce67d56445f2eccc537602db789878217` | 32-frame `cbbm_sp_b04` complementary body curve |
| 0015 | `1132c275c97bf3af4a8ef0e25b2d166dade14ebb768850c22b9f91a6cea97a98` | 40-frame `cbbm_sp_03` and `cbxs_st0to4` |
| 0016 | `4fbf6d5b77a776f395c27fe5a69e52031c813bcdd5f5434d7fcb5949db73d414` | 30-frame `cbbm_sp_04` and reverse `cbxs_st4to0` |
| 0018 | `05ac1317b3ae797466e9e323927ec51799ebfde1dbd0ec048c04af9575a4d469` | Five-frame `cbbm_sp_05` with upward local hara-Y change |
| 0019 | `8aa13bcd9a161966f6aee5a1b6914e86ebae9dcc43378d120cb7443bb231f4e8` | 30-frame `cbbm_sp_06` with downward local hara-Y change |

WSS 0001 (423,680 bytes, SHA-256
`10b6aac9583319ddde403800df8b0939ab535440cb5c9178c53a72452576c32b`)
contains three forward-offset VMDL meshes in its caster-side effect
resources. The decoded `SEDBvmdl` kind-14, size-68, live-1 bounds
records give these model-local extents before VEFF transforms:

| Model ID | Nested VMDL SHA-256 | Local X/Y/Z extent |
| --- | --- | --- |
| `cy0fir01y` | `ed8a43743dade41b04addbe887f4fec37b4af129a969aae29ea0ba52688afeb8` | `17.136 / 4.118 / 8.201` |
| `ds0dis01y` | `e957565b3b63aff55c8ff94a072ebe6e7c3eda17ca13894e84aba77801d62e17` | `17.012 / 0.860 / 8.492` |
| `ds0lin01y` | `ffd7423dae717b368ff2bc1fc3c6c0f83209c79ea6579697e9e1cfe8b03be16d` | `16.480 / 0.461 / 8.210` |

The same bank also embeds `rg0fire06`, a roughly
`6.883 / 6.661 / 0.081` local-extent ring (VMDL SHA-256
`b758c7fdc8a5acfde2006c3d4a3d83f462e3fd196a975f0b748031a4467379e4`).
The forward meshes make WSS1 an Incinerate/breath candidate by authored
shape, not a verified retail command-to-bank mapping or world-space
damage volume. Raw model bounds do not include effect transforms,
attachment, emission, or scale at render time.

The comparative caster, target, rock, and generated-control resources
for WSS2/3/4/10/12-14/21/22 are in
[Ifrit ground-effect bank resources](ifrit-ground-effect-banks.md).

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

Two m852 banks reuse the nested m999 impact payloads. Recursive
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
that function, were checked directly in the pinned PE with
`pefile` and Capstone x86-32. Thus a normal main-state death can
request an active `dead` scheduler without a separate defeat WSS.

The m524/e002 model package at
`client/chara/mon/m524/equ/e002/met_mdl/0001` is 352,064 bytes,
SHA-256 `5a5a4414c7327ca5dd0f04d78677e76827d535126b8db3edd5633a7ea24784ff`.
Its nested `dead` SCB is 1,888 bytes, SHA-256
`5b4b3631be6ac08e27f99efabc685893fe93e11e8146d2b73fabab1e03ea63f6`.
The decoded 12-entry scheduler cancels `init_msb4_1`, includes motion
and move-stop clips, and launches `m524_ded`. The nested VEFF
`151rmjanc_dead1` has SHA-256
`85745f0569c3b2f1aaa0ecf01267f586c86a2f4507b2aa64fa8e051fbc6a5278`.
The resource hashes above identify the nested SCB and VEFF. The native
death-selection route is at the pinned executable VAs described above.
The whole model package hash was checked against that file.

This closes the static death-selection mechanism and the e002 asset's
availability, not live root precedence when more than one model
resource is active, actual visual playback, or a retail corpse lifetime.

All eight listed action-file identities were checked against the 1.23b
client. Their bank, nested-resource, and curve identities are bounded to
those files; the static join does not identify a live action selector.

The banks establish client capability. Bank numbers, command IDs,
effect names, and mechanics are separate namespaces. No retained retail
selector here proves when a battle used these banks, how flames attached,
which Nail death resource root won in a live actor, or how either
presentation was reconstructed for late joiners. Motion frame counts are
not server cast, travel, telegraph, or action-lock durations.
