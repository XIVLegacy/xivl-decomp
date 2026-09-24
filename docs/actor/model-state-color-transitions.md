# Monster model-state and color transitions

This note separates two retail presentation mechanisms recovered from the
FFXIV 1.23b client and its resources. The executable evidence uses the
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

In the pinned executable, both helpers follow the loaded model at
`actor+0x2b10`, its `+0x04` object and `+0x40` metadata interface, and that
interface's virtual `+0x04` getter. They return zero unless metadata flag
`+0x03` has bit `0x80`; otherwise `0x0065be50` returns byte `+0x35` and
`0x0065c550` returns byte `+0x3b`. At `0x007a834d..0x007a839e`, the selector
converts count 0 through 8 to lower-bit masks `0x00, 0x01, 0x03, 0x07,
0x0f, 0x1f, 0x3f, 0x7f, 0xff` and reads the supported-bit byte. It can
process lower bit N only when that bit changed, N is within the count mask,
and the supported mask includes it. The active resource root at
`actor+0x12f0` must also be nonnull (`0x007a83bd..0x007a83c5`). The
formatted `init_msb%u_1` and `init_msb%u_0` scheduler names are rooted in
the executable strings at `0x00fe75bc` and `0x00fe75cc`. These are static
gates, not proof of a particular encounter's queued mode or kick timing.

For m049, the metadata partitions the low three bits of payload byte `+0x04`
as an `init_msnNNN` ordinal. The BID and `e001/top_tex1` banks expose
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

The m034 BID bank has `init_msb4_1` and `init_msb4_0` schedulers
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
These identities were checked against the 1.23b files. The scheduler and
motion joins come from `monster-action-scheduler-contract-20260810`
`scheduler_manifest.csv`, rows for m034 BID and WSS101/201/501.

## m037 ogre limb-effect state

`client/chara/mon/m037/skl/0001` (SHA-256
`34296f469e58b5ad29e0d18d64dc214bf670bb7f0f3fb29fb244cb03817037d5`)
has `info_m037` metadata with ordinal split 4 and supported mask `0xf0`.
The BID at `act/emp_emp/bid/base/0000` (SHA-256
`d22bc300c6043d21cda6a6eb5af087d6e7e110aa5ce28f98e4adb999a40dfbc3`)
provides `init_msb7_1` and `init_msb7_0` for supported bit 7 (`0x80`).
The on scheduler's ActionClip at payload offset `0x274` names ACB
`msb_7_1`; that ACB references VINS `1UGmYVvleafinst`, which has four
typed references to leaf `1D30TRmsb_7_1`. The VINS attachment names are
`EID_L_FOOT`, `EID_L_HAND`, `EID_R_FOOT`, and `EID_R_HAND`. The leaf
references VEFF `4leD72ogr_hokb6`, whose embedded authoring name includes
`ogre_m037/berserk_loop/ogr_hokb6t.veffbin`. The off scheduler and `dead`
each cancel `init_msb7_1` at native start value zero. The selected on SCB
and VEFF payloads have SHA-256
`fcdf2b82d7a8837103399bcfe0b3c4df5af5ad5afd4caded63b9a497929e1524`
and `3c36dc6d93fd3a0981709987f9c515395d5150ecaf68e6b73ac04f37e59ce803`.
This is an authored four-limb effect route; it does not recover the retail
health threshold, state packet, or visible onset.

## m054 gargoyle weapon-effect state

`client/chara/mon/m054/skl/0001` (SHA-256
`1aa51fd820700ad31be4fdd06999e4a421453a713d1ea69106ab321d82948f61`)
has `info_m054` metadata with ordinal split 4 and supported bits 4 and 5
(`0x30`). The BID at `act/emp_emp/bid/base/0000` (SHA-256
`386bdd68a6f5b05b835970a23d4d1665bfee26d44283ee540a34e89ddbc4f561`)
contains `init_msb4_1` with two typed ActionClips at payload offsets
`0x2a0` and `0x2c8`. They name ACBs `m054_aura_r` and `m054_aura_l`;
their VINS/leaf branches converge on VEFF `1lciMXggl_sklh5` and its
embedded `gargoyle_054/weapon_aura` authoring name. The branches use
attachment ports `EID_SUBT_EFF2` and `EID_SUBT_EFF3`, respectively;
those port names alone do not identify anatomical bones.
`init_msb4_0` cancels the on scheduler at native start value zero.
Bit 5 selects separate motion/chant schedulers, not these aura ACBs.
The on SCB and shared VEFF payloads have SHA-256
`25b1616abfe77f2eac6965726e2a31ff6c6dbb53733ffd2ea4ee68e461f8cba7`
and `1fc1f56053fd3729ed6879c124c98e98c685349fe9e48a3203bf7fa430d1e307`.
The same BID contains a `dead` resource in `outer/RES:s_bid00`, with path
`chr\sch\mon\m054\emp_emp\dead\bin\dead`. It is a 1,344-byte SEDBSCB at
file offset `0x59428`, SHA-256
`31228e756e8fb5e374ddc1e721740afed6454d6e05506a6c3aef952d8ab6ee82`.
Its second `@CBLK` begins at payload offset `0x230` and declares five
records. The type-name strings at payload offsets `0x474` through `0x4CC`
are `ProxyActor`, `BindActorClip`,
`RaptureServerMoveStopClip`, `RaptureClientMoveStopClip`, `MotionClip`, and
`RaptureCharaActionSoundClip`; this list has no
`RaptureCancelChantSyncClip` or `RaptureEffectEndClip` entry. This bounds
one named scheduler and does not show when it runs or whether another client
route clears the effect.

No original empowerment threshold, north-terminal control rule, complete
death-time effect-cleanup policy, or retail state delivery follows from
these resources.

## m505 immediate alpha state

`client/chara/mon/m505/skl/0001` (SHA-256
`65aabb1f35ab859757292df66cff7f99f449f9ddfbe14f6ec0ebfa62892dc25e`)
advertises model-state bit 4 (`0x10`) in `info_m505`. Its BID at
`act/emp_emp/bid/base/0000` (SHA-256
`eba7345fa9cdfa9a0330fa5caae2f653a4f2c0c0135e128c6f888b382f317aef`)
contains `init_msb4_1` and `init_msb4_0`, each with one
`RaptureCharaColorFadeClip` at SCB payload offset `0x264`.

The complete 60-byte hide record has transition duration zero at
full-record `+0x14`, component mask `0x00000008` at `+0x18`, and target
RGBA `(1, 1, 1, 0)`. The show record has duration zero, mask
`0x80000008`, and target `(1, 1, 1, 1)`; its high mask bit forces the
start value. Their record SHA-256 values are
`3875e113fa62257f8abc3081368349c48c09b1abcb95d670e62799bbdeae4d19`
and `9f6c51275a49c2bfd63ba75daac58fc5f495d08d75922011c093829eb90d8523`.
The pinned executable's color-fade consumer at `0x008262a0` reads
duration from `+0x14` and mask from `+0x18`; downstream `0x008422e0`
uses mask bit 8 for alpha and directly assigns the target when duration
is zero. Thus the value 8 is a channel mask, not an eight-frame fade;
the SCB block's 40,000-unit length is not the opacity-transition duration.
This proves an immediate alpha control in the m505 model. It does not
prove the retail Chain Bearer state selector, relocation mode, hidden
interval, targetability, or encounter timing.

## Shared color-fade storage

The pinned executable (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`)
routes `RaptureCharaColorFadeClip` updates through `0x008262a0` and
`0x0065ef60`. The bridge requires a nonnull actor and a zero return from its
vfunc at `+0x268`. On that branch, `0x0065ef8d` passes lane 1 and the
renderer `+0x1560` color state to `0x008422e0`. Arrival's initial hide also
calls `0x008422e0` for lane 1 with alpha mask `0x8` and a zero alpha target
(`0x00661990`). The m505 hide/show records and the POP-7 scheduler records
therefore target the same native color-fade lane; they are not independent
opacity layers. See
[animation bank routing](animation-bank-routing.md#arrival-type-and-revival-motions)
for the POP-7 resource and arrival path.

This shared destination establishes a competing-write hazard, but not which
effect or clip ran last for any historical encounter, nor the actor, selector,
timing, or runtime state that chose it.

## m526 rock section masks

The m526 BID bank at
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

The `client/chara/mon/m999/equ/e001/met_mdl/0001` resource
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

The pinned `ffxivgame.exe` resolver at VA `0x00bdbf10` reads a serialized
32-bit static-link key from a control node. Its top nibble selects a path:
class 0 uses bits 16..27 as a producer-instance index with 0x34 stride
and bits 0..15 as an output slot with 0x24 stride. In the ordinary-input
branch, class 1 uses a context-owned 0xe8-stride control-module record
and provider/fallback lookup; class 2 searches an external/context list
before falling back to class 1; class 3 takes an unsupported-code path.
Thus `0x100a0000`
selects runtime module index 10, not VEFF serialized class-table row 10.
That module's identity requires the context's template append order.

Re-extracting the two VEFFs' paired 0x24-byte control and
0x1c-byte link records yielded both serialized link vectors. State 4 has
44 head (`+0x00/+0x04`) and 53 tail (`+0x14/+0x18`) keys; state 5 has
21 head and 32 tail keys. State-5 `Position3DMapBind:CoordRoot` paired
records 6 and 18 have no head vector. Their tail inputs are:

| Paired record | Tail key 0 | Tail key 1 |
| ---: | --- | --- |
| 6 | `0x10000000` | `0x10000000` |
| 18 | `0x100a0000` | `0x00010000` |

The last key selects producer instance 1, output slot 0. The first three
have class-1 registry-selector encoding; they do not name a target,
terrain normal, or actor transform. The static loader pairs control/link
records by index, but the source head/tail vectors have not been joined
to the resolver's two runtime destination arrays. A separate self-local
resolver branch can write null for a class-1 key. These key joins do not
recover the historical owner or active encounter invocation.

The two state-5 MapBind controls have a native terrain-query path.
`Position3DMapBind:CoordRoot` evaluation at VA `0x00d7e370` reads its
linked input through `+0x30` and, when dirty, adds the double constant
2.0 at `0x00f63028` to the sample Y before calling `0x00d85800`
(`0x00d7e49b..0x00d7e4b3`). On a miss it adds 2.0 and calls again
(`0x00d7e4bf..0x00d7e4e4`); a hit stores the height adjustment at
control `+0x24`. The query holds X/Z, constructs a vertical segment
from supplied Y + 0.01 to Y - 200.0, and passes byte tag `0x1e`
(`0x00d85800..0x00d85a20`). The executable constants are 0.01 at
`0x00fb7dec` and -200.0 at `0x0109d91c`. This is a client terrain
conformance operation, not evidence that a historical server placed a
helper at any particular world coordinate.

The state-5 VEFF's six 0x38-byte layer records carry three raw words at
indices 8-10. Re-extraction from the m999 model gives selected
rows below; the serializer's semantic field-name join remains absent:

| Layer label | Raw word 8 | Raw word 9 | Raw word 10 |
| --- | ---: | ---: | ---: |
| Ground crack (additive) loop | 310,000 | 80,000 | 20,000 |
| Telegraph-spawn distortion | 255,000 | 0 | 300,000 |
| Crack (subtractive) | 115,000 | 0 | 115,000 |

At 100,000 units per second these numbers are timing-like, but they are
not independently identified as start, duration, and end fields. They
cannot determine a retail cast length, active mode sequence, or helper
cleanup policy.

The m999 WSS4 and WSS5 wrappers have different whole-file
hashes (`769514e370b57a5024e1f646fbe7ab05563f802c615e2f32890c51895d7a9423`
and `d5f262f0d06fe1fa8f1f990df3333cc8093a1c72fea22aedc507aba16baaec72`),
but both embed the same `main` SCB
`7da7c8b13a2096a23de59d6719c2e2cad385cb8e6e295974273b33e7a08c0422`
and `mon_main` SCB
`e7075d9e96134471ae4532f71e37d88489f5cfd660f248bf633a3bad05d54e14`.
Each `mon_main` contains a `RaptureActionSubStatusSchKickClip`.
The functional scheduler payloads do not themselves distinguish state 4
from state 5; the queued mode and compatible model resource do.

The resource file hashes were checked directly. Nested SCB and effect
locators are in the contributor's
`tools/outputs/ifrit-model-state-decomp-20260805/{state_resources.csv,resources.csv}`
and `IFRIT_GROUND_STATE_AND_NAIL_DEATH_CLOSURE_2026-08-05.md`.
No historical helper owner, command, world coordinate, trigger order,
or visible lifetime is recovered by these asset joins.

## m508 Spirit of the Wood route

The Spirit of the Wood transition is a separate scenario effect, not an m049
model-state transition and not a BODYGEAR interpolation. The
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
