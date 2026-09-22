# Guildleve transformation presentation

Twenty FFXIV 1.23b guildleves have recovered objective text that explicitly
describes a disguised group transforming. Their mob slots join to 17 monster
families and the c005 humanoid family. The installed resources and pinned
executable establish a state-driven BID transition; they do not establish a
packed animation selector or recover the missing encounter choreography.

Native addresses below refer to the pinned executable with image base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Guildleve coverage

Six transformation leves have local encounter scripts and use the native
state edge:

| ID | Title |
| ---: | --- |
| 10866 | An Imp in Sheep's Clothing |
| 10867 | Fiend in the Flock |
| 10868 | Escape from Cell B17 |
| 11666 | Just One of the Dodos |
| 11667 | The Lady's Bite |
| 11668 | The Devilet Inside |

Fourteen source-backed rows have no local encounter script:

| ID | Title | ID-tagged placement or recovered Lua hit |
| ---: | --- | --- |
| 10845 | Escape Artist | none |
| 10905 | Escape from Cell E05 | none |
| 10925 | Unholy Moley | none |
| 11426 | Escape from Cell D72 | none |
| 11705 | Secrets of the Sultanate | none |
| 11725 | A Devilet's Best Friend | none |
| 12226 | Corpus Adamance | none |
| 12466 | Impish Intentions | none |
| 12467 | Hidden Behind a Hide | none |
| 12468 | Leaders of the Pack | none |
| 12485 | Underneath the Shell | none |
| 12505 | What the Devilet Dons | none |
| 12525 | Out of its Shell | none |
| 13026 | My Fey Lady | none |

Their objective descriptions and 52 populated mob slots are source-backed.
Numeric coordinate-search collisions are not actor-ID joins and do not recover
positions, search-circle layouts, spawn grouping, or reveal order.

## Installed BID resources

Every joined family has `cbbm_activ` and `cbbm_deact` MTB, MCB, and CIBT
resources under `emp_emp/bid/base/0000`. The complete decoded set contains 114
resources: 108 battle-main resources plus six c005 upper-body resources.

| Family | BID bytes | BID SHA-256 |
| --- | ---: | --- |
| m002 | 319,936 | `6d29dd2b5ef43338a1c65840b60b075899c7b6e224f1e4bd903905aa4ec32d8c` |
| m004 | 459,744 | `9f99cd6972a682794b81a00ec0b8661c1837eb4f5321cfa05b59bd9cd6bf63fc` |
| m006 | 460,032 | `cc51dccc63285f8220760516a58ae19ef23bf8ea9dff37a4ecfea31efb6baff2` |
| m013 | 313,408 | `1d53e0812754c8ad5eba870da822b536b774777e738634e18f97338b0fb68ed7` |
| m028 | 623,408 | `41a7bc29295e59925ffa5644202610c81f9c6fd88adfa3ec87c9c215ff2eeb05` |
| m029 | 825,008 | `54106d7a46b1fdf14e975354a50255e2d9a3a149bb9681e0c0ba854e3068e8bb` |
| m032 | 313,056 | `a9f6ac418e365b5e03bdd848beacbb3ba932af282bdbc3ee617a8bbe5b0ac17a` |
| m033 | 417,424 | `4a7b2b1af0ae10642d6f3e73d042c9e00595aa77312a9b5172171df6c95086a6` |
| m035 | 407,664 | `3040a5c1009abd279c9e88f7abf33ef0775394ae1bce79b18ecbad0486739b9a` |
| m038 | 382,944 | `3bb82ae75845f67c91fda1b780839d58c25c4d29dc4091a790c823f0bced64ba` |
| m039 | 444,800 | `6c0db0c0992706967d51e00e46bbac062299fa50ecd49b37213ebd4b1825395c` |
| m501 | 144,048 | `252eb0ada59033d3d59ac32a2d666910cfec82987abf1a18f7ec74c975a68348` |
| m502 | 150,560 | `becc2e288749edb55e6b7afe35a76bd853c550e11fa9c95a54e7f30418adfced` |
| m508 | 182,480 | `07ca77410e589eb4f8901d9bd4935105c68e9787f0abbc7a9176052858b32ee9` |
| m513 | 354,704 | `428a0433bc369c26e7c34f18269a33d85341bbce6c2f64a3bb82381fae0721fd` |
| m515 | 73,312 | `847dc97bbeb0ad52e31609cf731f5fe1142ca23b294f6f92b5103815b3e4dc73` |
| m518 | 932,304 | `e63750e38ecec1d4ed0fc350a3ac6963868f9d22a55ab2c43059ea6cd049702a` |
| c005 | 690,800 | `e32666b101dc8fda64c20b572793e1900b9f07b2f133bc11f2f8d4d7ba1fc000` |

The monster families use the battle-main transition lane. c005 additionally
contains `cbbp_u_activ` and `cbbp_u_deact`; actor flag `0x40` selects that
upper-body lane. The common CIBT profiles are exact resource content:

| Profile | Bytes | Activate terminal | Deactivate terminal |
| --- | ---: | --- | --- |
| battle-main small | 64 | `cbnm_id0` | `cbbm_id0` |
| battle-main middle | 64 | `cbnm_id0` | `cbbm_id0` |
| c005 upper body | 44 | no terminal motion | no terminal motion |

Motion duration is family-specific and comes from each MTB header. The CIBT
profile size or terminal name is not a universal presentation duration.

## Native state bridge

Opcode `0x0134` dispatches at `0x007C0E10`. A main-state transition from
PASSIVE 0 to ACTIVE 2 enters phase 4 at `0x007BCC80`. The normal route probes
`cbbm_activ` at `0x007BCDE3` and falls back to `cbbm_id0`; actor flag `0x40`
selects `cbbp_u_activ` instead.

The reverse ACTIVE 2 to PASSIVE 0 transition enters phase 5 at `0x007BD120`,
requests transition ID `0x1E` through `0x007AC400`, and maps it at `0x008A8DD0`
to `cbbm_deact`. Actor flag `0x40` selects `cbbp_u_deact` instead.

A later controller call through vtable `0x00FD3ED4`, slot `0x6C`, target
`0x006B5710`, supplies bank selector 1 (`bid/0001`) during activation or 0
(`cmn/fid`) during deactivation. That controller selection is a separate stage
from the earlier transition-motion mapping and does not turn a packed common
library selector into a BID request.

## Packed-selector correction

`0x04000000` decodes as category 4, `cmn/lib`, middle bank 0000, low selector
0. None of the 18 joined families has `cmn/lib/0000`, and no recovered Lua
caller requests decimal 67108864 for monster activation. The value therefore
cannot select the installed `emp_emp/bid/base/0000` resources merely because
both paths contain the text `0000`.

Guildleve reveal presentation must use the actor-state transition bridge. A
direct `0x04000000` animation request is not a retail-backed substitute.

## Evidence boundary

The evidence establishes the 20 transformation objectives, mob and appearance
joins, physical BID resources, activation/deactivation motion names, actor-flag
alternate, and native state edges. It does not establish retail spawn
coordinates, target selection, search-circle geometry, inherited-hate order,
the delay between disguise and ACTIVE publication, or late-join and reconnect
replay. Those missing historical behaviors cannot be recovered by injecting a
packed selector or by assigning numeric coordinate collisions to a leve.
