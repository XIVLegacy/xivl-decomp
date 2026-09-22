# Monster model-state and color transitions

This note separates two retail presentation mechanisms recovered from the
FFXIV 1.23b client and installed resources. The executable evidence uses the
pinned image with SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`
and image base `0x00400000`.

## m049 model-state route

The `SetActorSubState` receiver at `0x00662d30` handles subtype `0x3b` at
`0x006638a4`. It passes the payload word at `+0x04` to `0x007b4440`, which
queues type 3 state on the actor component at `actor+0x1110`. This publication
does not launch the model-state scheduler by itself.

An action scheduler containing `RaptureActionSubStatusSchKickClip` drains the
queued state through `0x007bf2e0` (with an alternate drain call at
`0x007bf5f8`) and applies it through `0x007a82f0` on `actor+0x0b80`. The
selector checks the model metadata state count through `0x0065be50` and the
supported-bit mask through `0x0065c550`. Unsupported or unavailable states
therefore deliberately produce no model-state scheduler.

For m049, the metadata partitions the low three bits of payload byte `+0x04`
as an `init_msnNNN` ordinal. The installed BID and `e001/top_tex1` banks expose
states 0 through 6 and establish this mapping:

| State | Element | Transition | Steady motion | `multiDiffuseColor` |
| ---: | --- | --- | --- | --- |
| 1 | Fire | `cbxs_st0to1` | `cbxs_st1` | `(0.75, 0.237675, 0.075)` |
| 2 | Ice | `cbxs_st0to2` | `cbxs_st2` | `(1.155, 1.535205, 1.75)` |
| 3 | Lightning | `cbxs_st0to3` | `cbxs_st3` | `(0.626487, 0.441, 0.7)` |
| 4 | Wind | `cbxs_st0to4` | `cbxs_st4` | `(0.15, 0.75, 0.3523)` |
| 5 | Earth | `cbxs_st0to5` | `cbxs_st5` | `(1.1, 0.8426, 0.242)` |
| 6 | Water | `cbxs_st0to6` | `cbxs_st6` | `(0.21, 0.512833, 1.0)` |

Each neutral-to-element material motion is 15 frames at 30 fps, or 0.5
seconds. The evidence resources are:

- m049 BID: 1,426,320 bytes, SHA-256
  `cdc066b3af9f107eecbd39db3bd6457531f430420861ff80313aa00da8110c96`.
- m049 `e001/top_tex1`: 236,736 bytes, SHA-256
  `0faadc602d3dad517859d0f90d633d1e5206ad683253cf475aa54157e22dfe97`.

Payload byte `+0x05` is the reserved high byte, not the m049 ordinal. The same
low byte can also contain supported `init_msb` flags. For m049, bits 4, 5, and
6 select `init_msb4`, `init_msb5`, and `init_msb6`; their terminal
`pur_mupt1`, `pur_mupt2`, and `pur_mupt3` resources are magic-power-up VFX.
Their names and ordinals do not establish an elemental mapping.

Consequently, a compatible publisher must send the m049 state in payload byte
`+0x04`, leave `+0x05` zero, and later execute a real substatus-kick clip. The
state division is model metadata, so m049's three-bit interpretation must not
be generalized to every monster.

## m034 drake bit-4 resources

The installed m034 BID bank has `init_msb4_1` and `init_msb4_0` schedulers
whose motion references are `cbxs_st2` and `cbxs_st1`, respectively.
The m034 WSS101 and WSS201 `mon_main` schedulers reference
`cbxs_st1to2`; WSS501 references `cbxs_st2to1`. These are authored
resources for both state directions, not proof that combat entry or a
particular server status triggers them. A separate historical source
associates drake glow with Smoulder, but the client resource names alone
do not establish the exact retail command-to-state packet sequence.

The BID at `client/chara/mon/m034/act/emp_emp/bid/base/0000` is 503,328
bytes, SHA-256
`fa03315aeed7fc7553384b1a16bf6c4a410bce1ab96cb04962f58b6092c2da84`.
The WSS201 bank at `act/emp_emp/wss/base/0002` is 206,768 bytes,
SHA-256
`3530ba8206deffa6ab836095e945a6317a4a65c9abd1556f3aca466fae02d671`.
These identities were checked against installed files. The scheduler and
motion joins come from `monster-action-scheduler-contract-20260810`
`scheduler_manifest.csv`, rows for m034 BID and WSS101/201/501.

## m526 rock section masks

The installed m526 BID bank at
`client/chara/mon/m526/act/emp_emp/bid/base/0000` (SHA-256
`23e5a6fa702023a5f9d94de5d96c12ab5335e8aca2c39f44da6f61274de50a7a`)
contains `RaptureCharaNodeGroupMaskClip` entries in four state-on
schedulers. The decoded entries are:

| Scheduler | Groups cleared by its clips |
| --- | --- |
| `init_msb4_1` | 1, 2 |
| `init_msb5_1` | 3, 4 |
| `init_msb6_1` | 5, 6 |
| `init_msb7_1` | 1 through 6 |

Each listed clip has flag 1. In the pinned executable, handler VA
`0x00825B20` (RVA `0x00425B20`) reads the group and flag at clip-data
`+0x10/+0x11` and forwards them through `0x0065E5B0`. The model-mask
operation at VA `0x0065C020` (RVA `0x0025C020`) obtains the current mask,
computes `1 << group`, and clears that bit when the flag is nonzero;
flag zero instead sets it. This directly supports a section-visibility
interpretation, not a uniform scale-down.

The m526/e001 top model at
`client/chara/mon/m526/equ/e001/top_mdl/0001` (SHA-256
`eb9723b10763b589e06c17ef03eedfc3c7cdafff4cfc909c968cae7fcaa5fc9d`)
has six named groups. The contributor's decoded `model_groups.json`
places groups 1/2 highest, 3/4 in the middle, and 5/6 lowest by their
local Y bounds. The rock-state clip entries are in
`outputs/garuda-rock-state-followup-20260907/rock_state_clips.json`;
their BID identity also matches
`monster-action-scheduler-contract-20260810/scheduler_manifest.csv`.

This explains what the authored client resources can hide. It does not
recover retail rock HP, damage thresholds, server timing, placements, or
the historical packet sequence that selected each state.

## m999 ground-helper state resources

The installed `client/chara/mon/m999/equ/e001/met_mdl/0001` resource
(SHA-256 `c80a1e587b5b0e21cc19cad2baa08c75ed31482ea3fd5219e125053c00e3b105`)
contains two named on/off state pairs. Bit 4 of the queued mode word
selects `init_msb4_1` or `_0`; bit 5 selects `init_msb5_1` or `_0`
through the model-state route above. Their extracted scheduler hashes
are:

| State pair | On SCB SHA-256 | Off SCB SHA-256 |
| --- | --- | --- |
| `init_msb4` | `cef7f840816daab125531a943ded9e2b6db6bc85b191d62ac21ab785dfd982ce` | `957de5016af9321fd83e75d3ebf05d48fc875162ad9bd2a07f30630e7a76c9ec` |
| `init_msb5` | `36e2818b521fb27b258823c010605d26df41d157573d74655c54528b5499fb60` | `61b73c999e996dd87cddce4cf8546e1233e89b3246693a6b375bf340da0419f8` |

The state-4 effect graph uses actor-root position controls. The
state-5 graph includes `Position3DMapBind` and generated terrain-normal
binding. This is an authored difference in attachment behavior, not
proof of the retail Eruption or Plume command mapping.

The nested `SEDBveff` payloads identify state 4 as
`0Xv7Tfift_eish2` (28,908 bytes, SHA-256
`1e3d19959accc04b7d2ba78ad473040f7ce6353d4edfd86fc4e63ccaa31183a6`)
and state 5 as `2Jckltift_skleb` (17,952 bytes, SHA-256
`6c1767731acff97324e526a7d35891501e5d97c1619462f51738570da3dba83e`).
Parsing their little-endian `{data offset, count, stride}` descriptors
gives 152 entries at payload offsets `0x428..0xB47` for state 4 and 90 at
`0x4C0..0x8F7` for state 5. Every computed array range is inside its
payload; both tables end immediately before an `RGBA` literal. Root
objects at `0x1170` and `0x1090` join 66+66 and 35+35 primary/secondary
records, respectively, into 12 and 7 graph groups. The first two primary
records in each graph are resource/container prefixes, leaving 64 and 33
mechanically joinable control records.

The state-4 control class table includes 11 `DrawResource`, 9
`AbstractPosition3D`, 11 `AbstractAngle3D`, and 11 `AbstractScale3D`
records. State 5 includes two `Position3DMapBind:CoordRoot` records and
three `DrawResource` records. These are controls within authored effect
graphs, not counts of network-spawned helpers. The serialized 12-byte
pools also contain offsets, indices, and tagged keys; interpreting each
row as three world-coordinate floats would invent placements. Neither
graph identifies retail helper origins, actor-class ownership, or the
runtime mode/kick sequence.

The installed m999 WSS4 and WSS5 wrappers have different whole-file
hashes (`769514e370b57a5024e1f646fbe7ab05563f802c615e2f32890c51895d7a9423`
and `d5f262f0d06fe1fa8f1f990df3333cc8093a1c72fea22aedc507aba16baaec72`),
but both embed the same `main` SCB
`7da7c8b13a2096a23de59d6719c2e2cad385cb8e6e295974273b33e7a08c0422`
and `mon_main` SCB
`e7075d9e96134471ae4532f71e37d88489f5cfd660f248bf633a3bad05d54e14`.
Each `mon_main` contains a `RaptureActionSubStatusSchKickClip`.
The functional scheduler payloads do not themselves distinguish state 4
from state 5; the queued mode and compatible model resource do.

The installed file hashes were checked directly. Nested SCB and effect
locators are in the contributor's
`tools/outputs/ifrit-model-state-decomp-20260805/{state_resources.csv,resources.csv}`
and `IFRIT_GROUND_STATE_AND_NAIL_DEATH_CLOSURE_2026-08-05.md`.
No historical helper owner, command, world coordinate, trigger order,
or visible lifetime is recovered by these asset joins.

## m508 Spirit of the Wood route

The Spirit of the Wood transition is a separate scenario effect, not an m049
model-state transition and not a BODYGEAR interpolation. The installed
`man2g000` resource is 9,608,688 bytes with SHA-256
`c895ce29d8f28246cfe9f6238e364754c82f1e003c43fd15289ec616f8b7cc19`.
Its actor 24 (`m508t0`, class `6000249`) has this authored chain:

```text
man2g000 block c23
  ActionClip at 0.44 s, record 0xf0fc, RIDT index 0x210
  -> c_c23_appr01
     -> 3ZjARPvleafinst
        -> 1vgBedm508_appe
           -> 0J3FJQm508_appe
```

The referenced resource identities are:

| Resource | Size | SHA-256 |
| --- | ---: | --- |
| `c_c23_appr01` ACB | 1,016 | `1453dbef28fff88e4e37c87acc02f749b4dbaa6d0bc96ca0f624f8b8d15d837c` |
| `1vgBedm508_appe` leaf | 926 | `365d600ba7c504e1ad91d0f7a742b4e7668ea197bae2df7648630c62a50dfe23` |
| `0J3FJQm508_appe` VEFF | 34,428 | `e6e20cc8814c32ea74b558494cb6dd81597dd884a6ab7f8d6524006bc8ebbf59` |

The leaf contains `ActorBind`, `ColorRGBALeaf`, and `LeafLife`. The VEFF has
15 `ColorRGBABlink` graph groups and 47 serialized `0x34`-byte color-control
records. Native `ColorRGBALeaf` evaluation at `0x00ba2070` multiplies two
float4 lanes and writes effect-local RGBA through `self+0x10`. The two
`ColorRGBABlink` registrations at `0x01316db4` and `0x01316ccc` share tick
function `0x00d5f950`; it interpolates the lanes and writes the masked result
to `self+0x40`. These are QIX effect controls, not actor appearance-slot
writes.

The scenario block, ACB block, VEFF envelope, and leaf-life fields use
different clocks. Their observed values are 1.95 seconds, 2.5 seconds,
300,000 VEFF units, and a native 150,000-unit leaf fade interval. They do not
prove a delay or blend duration for the separate BODYGEAR update. In
particular, mode 1 `LeafLife` evaluation at `0x00ba42a0` and `LeafLifeEx` at
`0x00d58910` depend on the owning effect boundary rather than treating the
serialized 300,000 word as a standalone cutoff.

`RaptureCharaColorFadeClip` is registered and functional in the executable,
but neither the recovered m049 schedulers nor the m508 scenario chain
instantiate it. The m508 BID also contains no `init_msn` family. The ordinary
BODYGEAR consumer remains the atomic deferred swap described in
[Native item-appearance boundary](item-appearance-boundary.md).

## Evidence boundary

The decoded resources and native consumers prove both presentation paths and
the m049 state-to-element mapping. They do not recover the exact retail
packet/action ordering used by the Toto-Rak encounter, or a captured
live-combat BODYGEAR sequence for Spirit of the Wood. The final renderer or
material setter downstream of the QIX color output is also unresolved. Those
unknowns must not be replaced with emulator timing or inferred packet trains.
