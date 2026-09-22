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

Twenty-two legacy PWIB envelopes declare exactly 12 bytes beyond their
physical files. Padding those envelopes in parser memory yields structurally
complete resources; no installed file is modified. Other size mismatches are
not accepted by this rule.

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

## Evidence boundary

The embedded graph proves client resource availability, model-slot ownership,
internal child selection, and the scheduler resolver ABI. It does not prove an
appearance-load invocation, world placement, actor-class association, or
server selector. No root is labeled automatically invoked: there is no exact
appearance-load-to-request edge and no recovered script names an embedded
scheduler. Indirect or data-driven invocation remains possible, but a class
file, root name, or numeric vocabulary entry cannot substitute for that missing
edge.
