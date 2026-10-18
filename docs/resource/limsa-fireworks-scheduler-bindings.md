# Limsa fireworks scheduler bindings

Limsa's root layout declares two hanabi UnitTrees with five public scheduler
aliases each. Exact references connect them to `isgrp_000200` and
`isgrp_000223`. The native map-object binder uses these name suffixes, rather
than the serialized node IDs. The inspected client paths support a conditional
binding and scheduler request. They do not recover a retail controller actor,
event trigger or visible fireworks.

## Artifacts and offsets

The root file `data/29/D9/00/00.DAT` is 529,376 bytes, SHA-256
`b61cf798a02fc1bf72f1445a01b6a94a2e58793e7dd82d0d654f1d422bbceff3`.
Its `SEDBlyb` scene begins at file `0x4C00`, with a `0x30`-byte header.
Serialized references P resolve to file `0x4C30 + P`, not relative to the
reference field. All offsets in the tables below are file offsets.

The declaration-driven parse used Python `struct`, following
`xivl-tools:docs/formats/world-layout.md`, sections "Node declarations",
"Declared shapes" and "Fields consumed by the zone reader". Instance nodes,
their target UnitTrees, child records and timeline records were checked against
the declared object runs. Embedded schedulers were checked against their
`SEDBSCB` signatures, byte sizes and class directories using
[the SCB format](cut-scheduler-clip-layout.md).

## Instance and owner references

Each listed instance is a declared `0x40`-byte `RefObjects/InstanceObject`.
Its generic-reference field at `+0x3C` points to the UnitTree. Each target is
a declared `0x50`-byte `RefObjects/UnitTree/UnitTreeObject`.

| Instance name | Node ID | Instance | Reference field | Referenced UnitTree | UnitTree |
| --- | ---: | --- | --- | --- | --- |
| `isgrp_000200` | 934 | `0x27980` | `0x279BC` | `sgrp_vfx_hanabi` | `0x21E20` |
| `isgrp_000223` | 939 | `0x27F40` | `0x27F7C` | `sgrp_vfx_hanabi2` | `0x22000` |

The nearby names `isgrp_000199` and `isgrp_000222` do not reference hanabi
trees. Their instances at `0x27940` and `0x27F00` reference
`sgrp_bg_bln1_b1` at `0x21DD0` and `sgrp_bg_bln1_d1` at `0x21EC0`.
String adjacency and a transform word containing an instance suffix do not
establish what that instance references.

## Public aliases and compiled bodies

The first UnitTree's child-list pointer/count fields are at
`0x21E50/0x21E54`, with count 13. The second's are at `0x22030/0x22034`,
with count 14. Both lists contain declared `0x30`-byte child records.
Child `+0x14` references its alias string and `+0x18` references its target.
The targets below are declared `0x28`-byte
`BaseObjects/TimeLine/TimeLineBaseObject` nodes. Timeline `+0x14/+0x18`
supplies the embedded SCB reference and byte size.

| Owner | Alias | Child | Timeline name | Timeline | SCB | Bytes |
| --- | --- | --- | --- | --- | --- | ---: |
| 200 | `vtp1` | `0x22D90` | `time_vfx_hanabi_vtp1` | `0x6ABC4` | `0x77D40` | 928 |
| 200 | `vtp2` | `0x22DC0` | `time_vfx_hanabi_vtp2` | `0x6ABEC` | `0x780E0` | 1024 |
| 200 | `vtp3` | `0x22DF0` | `time_vfx_hanabi_vtp3` | `0x6AC14` | `0x784E0` | 1024 |
| 200 | `vtp4` | `0x22E20` | `time_vfx_hanabi_vtp4` | `0x6AC3C` | `0x788E0` | 1152 |
| 200 | `vtp5` | `0x22D60` | `time_vfx_hanabi_vtp5` | `0x6AB9C` | `0x778D0` | 1136 |
| 223 | `vtp1` | `0x233F0` | `time_vfx_hanabi2_vtp1` | `0x6AE94` | `0x7C820` | 1136 |
| 223 | `vtp2` | `0x23420` | `time_vfx_hanabi2_vtp2` | `0x6AEBC` | `0x7CC90` | 1184 |
| 223 | `vtp3` | `0x23450` | `time_vfx_hanabi2_vtp3` | `0x6AEE4` | `0x7D130` | 1184 |
| 223 | `vtp4` | `0x23480` | `time_vfx_hanabi2_vtp4` | `0x6AF0C` | `0x7D5D0` | 1216 |
| 223 | `vtp5` | `0x234B0` | `time_vfx_hanabi2_vtp5` | `0x6AF34` | `0x7DA90` | 1232 |

The alias strings are pooled. For example, both trees reference `vtp1` at
file `0x643F`. Its first serialized location does not identify its owners.

The eight bodies with `LayUnitMemberActor` records use `LayVFXClip` and
`LaySEClip`. Two further bodies use `LaySystemActor` and
`LayRaptureCallSchedulerClip`, completing the ten declared aliases:

| Wrapper | SCB SHA-256 |
| --- | --- |
| Owner 200, `vtp5` | `a675e4aa2f1f43da508b91add20ababa093783ec6995ef756e293abaa75a7afc` |
| Owner 223, `vtp1` | `b56292a95d1dae285336142ecd649de0d6648d02dcbc9dc053ee6cb0dc55f5da` |

Both wrappers have empty RIDT/RIDI resource tables. Their clip dword at
`D+0x14` uniquely matches a child `+0x1C` value in the same UnitTree:
owner 200 has `6,6,7,8,9`, matching aliases `vtp1,vtp1,vtp2,vtp3,vtp4`;
owner 223 has `2,4,3,4,2,4,3,5`, matching
`vtp2,vtp4,vtp3,vtp4,vtp2,vtp4,vtp3,vtp5`.
The first clip records are at `0x77B20` and `0x7C9F0`, with stride `0x2C`.
This is a structural field match. The `LaySystemActor` context and native
receiver scope remain unresolved, so these sequences are not asserted as
executed calls between the local aliases.

## Native binding and request path

Addresses below are VAs in `ffxivgame.exe`, image base `0x00400000`, size
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra listings were checked instruction-by-instruction against
original PE bytes, independently decoded with pefile `2024.8.26` and
Capstone `5.0.7` in x86-32 mode. Jump-table targets were read from the PE.

The actor packet switch at `0x0058CCA0` maps `0x00D8` to `0x0058D2F2`.
That arm reads an unsigned instance value at game-packet `+0x10` and an
unsigned 16-bit layout selector at `+0x14`, then calls `0x0058BF60`.
It creates a binding only when actor `+0xB2C` is null. Constructor
`0x00590230` forwards the layout and instance values through `0x0059F2A0`.
The layout-name helper `0x0059DF90` uses the current region and
`0x0079ACF0`, with a cross-root fallback through `0x0079B660`.

The region resource table is
`data/03/C0/00/00.DAT`, SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
Under region root 101, child row `0x310` contains ID 181, type `0x1000`,
resource `0x29D90000` and token `sea_s0_low`. Child row `0x340` instead
contains ID 101, type `0x2000`, resource `0x29D90003` and token
`sea_s0_fld01`. Region key 101 therefore must not be substituted for the
layout selector naming the checked root DAT. The table's grouping and
constructors are documented in [Region weather resource rows](region-weather-resource-rows.md#native-loader-boundary).

`0x0059F190` publishes a binding record with selector `0x23` through
`0x004D7980`. On the documented
[element-to-scene path](../net/weather-transition-runtime.md#packet-publication-and-scene-dispatch),
MapLayoutActor receiver `0x0062CDA0` forwards selectors `0x23` and `0x28`
at `0x0062D041` to its retained map-object manager.
Manager `0x00806AC0` routes binding selector `0x23` to `0x00806950`,
which constructs `Application::Scene::Actor::Map::Instance::MapObject`
at `0x00805790`.

That constructor stores the instance argument at object `+0x68` and formats
`isgrp_%06d` into `+0x48` for a nonzero value. Thus values 200 and 223
address the serialized names above. Values 934 and 939 are separate node IDs.
The ready-layout branch at `0x00806A52..0x00806A8F` requires state at least
5 and performs a kind-`0x12` lookup using the formatted name. Deferred
binding paths at `0x00806550/0x00806670` also use that name. Their readiness,
region and retained-object conditions are not an observed successful bind.

Opcode `0x00D9` maps to `0x0058D307`, which passes the four name bytes at
game-packet `+0x10` to `0x00585320`. That helper requires the retained
binding, copies four bytes and terminates them locally. For `vtp1..5`,
`0x00794690` takes its alphanumeric-fourth-character branch into
`0x0059D590`, which submits selector `0x28`. The Lua midstream bridge
at `0x006F3BA0` reaches the same helper through `0x0075C8E0`.

Manager selector `0x28` reaches `0x008064A0` and `0x00806320`.
The latter obtains an alias record through the exact string comparison in
`0x00802020`, resolves a kind-7 resource interface through `0x008012F0`,
checks its slot-11 result and calls `0x008024C0`. For action value `0x7F`,
the jump table selects `0x00802502`, reaching the interface's slot 3 at
`0x0080256B`. This joins the owner-local name lookup to a conditional
scheduler invocation. It does not establish that either Limsa owner was
bound, ready or invoked in retail.

## Remaining activation boundary

The static result identifies the layout row, instance-name namespace,
bank-local aliases, timeline bodies and conditional native invocation path.
It supplies no authenticated actor ID/class or spawn row, historical sender,
event calendar, bank-selection policy, wrapper receiver context or visible
effect. Those inputs cannot be inferred from the aliases or the asset names.

Weather `8029` and the recovered `MapObjFireworks` script do not supply this
missing retail activation record. Their separate evidence and limits remain
in [City seasonal root effects](city-seasonal-root-effects.md#script-and-activation-boundary).
The specific remaining static question is how the two `LaySystemActor`
wrapper contexts select their scheduler receivers. Historical orchestration
requires an authenticated dispatch or observation that identifies the actor,
binding and submitted names together.
