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
