# BG-object embedded schedulers

Standalone `cmn` and `lib` action banks are not the complete BG-object
presentation corpus. A recursive decode of all 236 installed FFXIV 1.23b
BG-object model binaries found 2,972 resources across 93 families and 224
appearance variants.

The model payloads contain 122 scheduler instances in 47 families and 104
embedded VEFF instances in 40 families. Twenty-nine scheduler-bearing families
and 23 VEFF-bearing families have no standalone action bank. Their
presentations must therefore be inventoried from the model slot itself rather
than classified as missing from the client.

Portable seasonal model identities and their unresolved placement boundary are
recorded in [Seasonal BG-object assets](seasonal-bgobj-assets.md).

Twenty-two legacy PWIB envelopes declare exactly 12 bytes beyond their
physical files. Padding those envelopes in parser memory yields structurally
complete resources; no installed file is modified. Other size mismatches are
not accepted by this rule.

## Model slots and appearance fields

The complete installed BG-object model atlas contains 93 `b###` families, 224
`e###` asset directories, and 236 model binaries. The extra model slots come
from variants that carry more than one of `top_mdl`, `met_mdl`, and `sho_mdl`.
Those slots join to the official appearance table through `body`, `head`, and
`feet`, respectively, using the exact serialized variant value: e001 is 1024,
e002 is 2048, and later variants continue in 1024 increments.

The client DAT export and the actor-appearance SQL contain the same 586 BG
appearance rows with no differing IDs. They reduce to 227 distinct equipment
and model signatures. Of the 236 installed model slots, 226 have at least one
official appearance binding and ten do not:

| Model slot | Required appearance value |
| --- | ---: |
| `b900/e001/top_mdl` | `body=1024` |
| `b930/e002/top_mdl` | `body=2048` |
| `b931/e002/top_mdl` | `body=2048` |
| `b932/e002/top_mdl` | `body=2048` |
| `b933/e001/sho_mdl` | `feet=1024` |
| `b934/e001/sho_mdl` | `feet=1024` |
| `b935/e001/sho_mdl` | `feet=1024` |
| `b964/e002/top_mdl` | `body=2048` |
| `b998/e005/top_mdl` | `body=5120` |
| `b998/e007/top_mdl` | `body=7168` |

Family b900 exists in the installed model tree without an official appearance
base. Conversely, official appearance base b956 has no installed client model
family. Synthetic appearance rows can exercise an unbound installed slot, but
they are test inputs and must not be described as official or retail mappings.

## Scheduler graph

The 122 instances reduce to 115 unique scheduler hashes. Parsing every
authored block produces 143 blocks and 569 clips:

| Clip class | Count |
| --- | ---: |
| `BindActorClip` | 122 |
| `RaptureChantSyncClip` | 113 |
| `ActionClip` | 89 |
| `RaptureCharaNodeGroupMaskClip` | 72 |
| `RaptureSoundClip` | 62 |
| `KillClip` | 26 |
| `MotionClip` | 25 |
| `ClipSyncClip` | 21 |
| `RaptureEffectEndClip` | 16 |
| `RaptureGetParamClip` | 12 |
| `IfClip` | 4 |
| `RaptureCasterManagedSchClip` | 4 |
| `RaptureCharaColorFadeClip` | 3 |

All 89 ActionClips resolve uniquely through ACB, VINS, and a leaf resource.
Those leaves produce 101 VEFF edges: 77 actions have one effect and 12 have
two. All 86 unique embedded VEFF hashes are reached, although three duplicate
VEFF instances are not selected by an action edge. Twenty-five MotionClips
resolve both BCM and same-ID BTM companions. All 16 effect-end clips select
one ActionClip, and the four managed-scheduler clips select exact sibling SCBs.

The scheduler entry ABI is `<HBBI>`. The high byte identifies the CACT actor;
it is not a track-flags byte. Treating only the first authored block as active
would omit 21 blocks and 87 clips.

## Aetherial-gate fixture

Model slot `b925/e001/top_mdl/0001` contains scheduler `initf_idle`, SHA-256
`b5a9ed639bd342290dc4a9ffd87090d9c4f6bc972208c8f815378bd9bc36a13b`.
Its `ProxyActor_0021` graph binds action `etl0_1`, chant and sound
synchronization, and VEFF `2zWElCetl0_1`. The 30,540-byte VEFF has SHA-256
`758160331f93f18bb80879792098c1d33deeb71a5ac48c1d1f51963328864e08`.

This is exact model-owned presentation capability. It explains why the
aetherial gate does not need a separate action-bank file, but it does not show
which actor construction or server event starts the root scheduler.

## Root and child topology

Of the 122 scheduler instances, 118 are model-slot roots: 100
`initf_idle` resources in top or met slots and 18 `2nitf_idle` resources in
shoe slots. The other four are b992/e001 child schedulers named `ex128`,
`ex64`, `ex32`, and `ex16`; compiled managed-scheduler clips select those
children by sibling resource ID.

`CharaActionQue` vtable slot 7 points to the resolver at `0x00844660` in the
pinned executable. It accepts direct SCB names and expands these aliases:

| Input | Selected scheduler |
| --- | --- |
| `@0` | `initf_idle` |
| `@1`, non-`emp` marker | `initb_idle` |
| `@1`, `emp` marker | `initf_idle` |
| `@2` | `initp_idle` |

The 37-entry native numeric vocabulary also maps 1206 to `@1`, 1208 to
`initf_idle`, 1226 to `@2`, and 1228 to `@0`. These entries prove accepted
request vocabulary, not that model loading submits the request.

The queue constructor has one direct owner, the generic request function at
`0x00845e80`. Eleven sites call its generic wrapper and one site, reached from
opcode `0x0144` cases 5 and 6, calls the direct-name wrapper. All twelve sites
forward actor fields, caller values, or packet payload. None fixes `@0`,
`initf_idle`, `2nitf_idle`, 1208, or 1228, and the executable contains no
`2nitf_idle` literal.

## Static owner coverage

A hash-locked join of the embedded-only families against official appearance
fields, actor-class rows, the checked-in server placement table, and local and
recovered class scripts covers 82 installed variants and 86 model slots. Those
slots contain 62 embedded scheduler instances and 53 VEFF instances from the
families without standalone banks.

Eighty-three of the 86 slots have at least one official appearance-field
owner. The three exceptions are the `sho_mdl` slots for `b933/e001`,
`b934/e001`, and `b935/e001`. Their body slots have appearance owners, but no
official appearance field equals the value required to select those shoe
slots. The shoe assets therefore remain installed capabilities without an
official slot binding.

The 83 mapped slots join to 226 actor-class rows:

| Client class path | Actor classes | Placed classes | Placement rows |
| --- | ---: | ---: | ---: |
| `/Chara/Npc/Object/Aetheryte/AetheryteChild` | 81 | 63 | 67 |
| `/Chara/Npc/Object/Aetheryte/AetheryteParent` | 37 | 35 | 37 |
| `/Chara/Npc/Object/MiningPoint` | 6 | 1 | 1 |
| `/Chara/Npc/Object/RaidDungeonLight` | 1 | 1 | 16 |
| `/Chara/Npc/Populace/PopulaceHamletPushEvent` | 1 | 0 | 0 |
| `/Chara/Npc/Populace/PopulaceStandard` | 3 | 3 | 23 |
| empty class path | 97 | 0 | 0 |

All 129 nonempty-path rows resolve both a local class script and a recovered
retail class script. The 97 empty-path rows cannot be joined to script behavior
from family or appearance alone. Overall, 103 actor classes have 144 checked-in
placement rows and 123 actor classes are static-only.

The placement counts describe the examined server data, not original retail
world ownership. Appearance plus actor class proves a valid static client
owner. A placement row proves that the project constructs that owner. Neither
one proves which embedded scheduler the retail client starts.

The joined script corpus contains 531 actor-ID literal references, but a
literal reference is not receiver ownership. Only three class scripts expose
an animation-call surface in this scope, and all three arguments are dynamic:
`AetheryteChild` and `AetheryteParent` call `PlayAnimation` with a guildleve
sheet result, while recovered `PopulaceHamletPushEvent` forwards a variable to
`_runCharaScheduler`. No literal call matches an embedded scheduler ID.

The residual report has 58 family/variant rows covering all 62 scheduler
instances, and none has an exact script selector. Four b992 child schedulers do
have exact incoming managed-scheduler edges, but an internal edge is not a
script selector. This absence is preserved as an invocation gap, not filled
with `initf_idle` merely because that name is present in the model.

### b925 ownership detail

The b925/e001 model slot joins to 83 actor classes. Eighty-one use
`/Chara/Npc/Object/Aetheryte/AetheryteChild`; 63 of those have checked-in
placements and 18 do not. Four unplaced child classes - 1280023, 1280056,
1280087, and 1280116 - still occur as numeric references in recovered retail
scripts. The other 14 lack both a placement and a static script reference in
the examined corpora. Two additional b925 actor rows have empty class paths
and no placement.

These joins establish that the b925 model is shared by the AetheryteChild
class family. They still do not establish an automatic invocation edge from
class construction to the embedded `initf_idle` scheduler.

## Evidence boundary

The embedded graph proves client resource availability, model-slot ownership,
internal child selection, appearance and class associations where explicitly
joined, and the scheduler resolver ABI. It does not prove an appearance-load
invocation, historical world placement, or server selector. No root is labeled
automatically invoked: there is no exact appearance-load-to-request edge and
no recovered script names an embedded scheduler. Indirect or data-driven
invocation remains possible, but a class file, root name, appearance match, or
numeric vocabulary entry cannot substitute for that missing edge.
