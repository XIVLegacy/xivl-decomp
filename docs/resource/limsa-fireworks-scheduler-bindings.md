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
The [native receiver lookup](#wrapper-receiver-selection) gives these values
numeric child-table semantics, conditional on the clip retaining the
respective tree's context. The sequences describe serialized clip records,
not an observed order of executed calls.

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

## Wrapper receiver selection

`LayRaptureCallSchedulerClip` factory `0x00837220` constructs a `0x40`-byte
object through `0x00836B80/0x00836660`. Its primary vtable is `0x0103B54C`.
The original RTTI type descriptor at `0x012CFA78` names
`.?AVLayRaptureCallSchedulerClip@Clip@Cut@Scene@Application@@`.
Primary slot 1 is `0x00836730`, the receiver-selection method below.

Base constructor `0x00A21260` retains serialized record `D` at clip `+0x14`.
`0x008366AE..0x008366BC` copies globals `0x013657B4/B8` into clip
`+0x38/3C`. Use `U` for the layout-unit interface captured at `+0x3C`.
This is construction context, separate from the serialized `LaySystemActor`
records.

### UnitTree child-key table

The kind-`0x12` branch of `0x007FE900` initializes a UnitTree's layout-unit
context. At `0x007FEA78..0x007FEAA9`, it obtains instance slot 61, publishes
that pointer as `0x013657B8`, calls its slot 1 and clears the global.
On `RaptureLayoutUnitInfo` interface vtable `0x00FF298C`, slot 1 is
`0x007FBF50`. It forwards to complete-object vtable `0x00FF2A90`, slot 1
`0x007FD830`. The layout-unit interface is at complete object `+4`.

`0x007FD872..0x007FD88A` obtains the incoming instance's slot-61 context.
The child loop passes it to each created child's slot 59 at
`0x007FDA52..0x007FDA61`. That setter, `0x008004B0`, stores it at child
interface `+0x1C`; getter slot 61, `0x0060DE70`, returns it.
At `0x007FDB48..0x007FDB5D`, the loop registers the original child record's
`+0x1C` dword and the created child interface with that context's slot 39.
The original records have stride `0x30`.

Slot 39, `0x00A9D230`, inserts through `0x00A9ED60` into the context's
`UnitTreeInstanceObjectTable`. Its original vtable is `0x010A4790`, with
RTTI type descriptor `0x012F6F88` named
`.?AVUnitTreeInstanceObjectTable@Layout@Stella@Lay@Engine@CDev@SQEX@@`.
The numeric key is stored in tree-node `+0x0C` and the interface in `+0x10`.
Thus the declared child IDs have a native registration path within the
containing tree's context.

### Shared UnitInfo key receiver

For UnitInfo main interface U retaining table `0x00FF298C`, original cells
`0x00FF2A28/0x00FF2A54` select slots 39/50 at
`0x00A9D230/0x00A9D330`. Both begin by comparing `u32(U+0x84)` with
zero. Their checked nonzero fallthrough preserves incoming ECX=U, then
loads L=`u32(U+0x84)`. Slot 39 tail-jumps to L slot 2 with the argument
stack unchanged. Slot 50 calls L slot 5 with its original first stack
argument. Their zero-branch destinations and return tails are not covered
by this receiver check.

For L retaining the `UnitTreeInstanceObjectTable` table `0x010A4790`,
original cells `0x010A4798/0x010A47A4` select insertion `0x00A9ED60`
at slot 2 and unsigned numeric-key lookup `0x00A9E8F0` at slot 5.
The [TimeLine actor callback's slot 37](timeline-weather-record-view.md#unitinfo-callback-lookup-boundary)
also loads L from U+`0x84` and tail-forwards its key to L slot 5. Thus it
reaches this same numeric-key lookup under the constructed-table condition.
The lookup returns the stored interface or zero on an ordinary miss under
its existing valid-tree conditions.

This is a shared field relation. Equality across different invocations needs
the same U and retained field/table values, including retention between each
comparison and load. The actual selected city's U, L/table, key and entry
state remain unestablished. No duplicate-key rule, actor RTTI or visible
activation is added. L is not identified with the separate ShowHideClip
manager's operation-provider field.

The original receiver unions are `[0x00A9D239,0x00A9D246)` and
`[0x00A9D339,0x00A9D34B)`; the separately checked prefixes are
`[0x00A9D230,0x00A9D239)` and `[0x00A9D330,0x00A9D339)`.
They cover 49 unique instruction bytes across four old excerpt candidates.
Read the four listed method cells as separate four-byte values, 16 bytes
total. Python 3.12.0 and Capstone 5.0.7 x86-32 checked the pinned executable,
PE raw backing, exact values and complete instruction boundaries. Reuse the
insertion, lookup, RTTI and slot-37 contracts without rereading their bodies
or data. Preserve the uninspected null/tail and actual-state boundaries.

### Clip lookup and scheduler interface

At `0x00836740..0x00836756`, clip slot 1 reads `u32(D+0x14)` and calls
captured `U` slot 50 with `(key,1)`. The inspected implementation,
`0x00A9D330`, ignores the second argument and calls table slot 5,
`0x00A9E8F0`. It uses `0x00A8A730` to find the exact unsigned key and
returns its stored interface, or zero on a miss.

The clip calls the selected instance's slots 10 and 47. Slot 47,
`0x00C4BAE0`, returns the property interface at instance-interface
`+0x14`. The timeline property vtable `0x00FF2D2C` has RTTI name
`.?AVRaptureTimeLineInstanceObjectProperty@Layout@Map@Actor@Scene@Application@@`.
Its slot 30, `0x00CB1C80`, returns the retained scheduler interface from
property-interface `+0x0C`. The clip then invokes that scheduler's slot 3
at `0x0083682F..0x0083683B`.

This method does not read actor ordinal `D+3`, the `LaySystemActor` name,
or its other serialized fields to select the receiver. With `U` equal to
the corresponding hanabi tree's context, the thirteen numeric requests
therefore select the local aliases listed above. A missing numeric instance
is dereferenced at `0x00836758`; the later scheduler-null result is guarded
at `0x00836783..0x00836791`. The path requires successful construction,
registration and property creation.

### Construction-context boundary

The TimeLine initialization path is base slot 18, `0x008D51B0`, through
`0x00AB35C0` to base slot 42, `0x00AB3730`. That helper obtains resource
`R` through `0x00AB38B0` and calls the `LayExternalObjectManager` at global
`0x01365B08`, slot 4. The returned object receives resource `R` through
its slot 1 at `0x00AB3759`.

The [TimeLine source/result contract](timeline-scb-resource-binding.md)
qualifies R's attached-record inputs, cache-hit boundary, copy/view branches
and SCB payload handoff to the parser.

Initialization `0x00A77A30` constructs a `LayExternalManagerConnector`
through `0x00A841D0`. At `0x00A77C4F..0x00A77C64`, it supplies the
connector's complete object `+4` and ownership argument 1 to manager slot 1,
`0x00ABB900`. That setter retains the interface at manager-interface `+4`
and sets its ownership bit. Manager slot 4, `0x00ABB930`, loads this
delegate and calls its slot 1, `0x00A84180`.

The connector calls `0x00A72AF0` and returns complete object `+4` from
global `0x01365688` when nonnull. The same initialization path constructs
that `LaySchedulerManager` through `0x00A72F50` and installs it through
`0x00A72AC0` at `0x00A77AF1`. Its interface vtable `0x0109E850` selects
`0x00A72B00` for the resource-R slot-1 call. These are conditional
initialization and allocation paths, not an observed installation.

`0x00A72B00` rejects zero `R`, constructs `LaySchedulerManipulator`
through `0x00A78180`, and calls its interface slot 1, `0x00A78420`, with
`R` and the complete manager. It returns the manipulator interface without
checking that initialization's Boolean result. The TimeLine path
`0x00AB3600..0x00AB3616` stores that result through property slot 31,
`0x00AA26D0`, at property-interface `+0x0C`. Getter `0x00CB1C80` returns
the same field. On this creation path, the clip's scheduler receiver is
`LaySchedulerManipulator`. A nonnull interface alone does not prove
successful construction of its underlying scheduler.

The original RTTI anchors for these objects are:

| Vtable | Type descriptor | Mangled type name |
| --- | --- | --- |
| `0x010A8AC0` | `0x012F8978` | `.?AVLayExternalObjectManager@External@Stella@Lay@Engine@CDev@SQEX@@` |
| `0x010A026C` | `0x012F29F8` | `.?AVLayExternalManagerConnector@External@Default@Lay@Engine@CDev@SQEX@@` |
| `0x0109E850` | `0x012F0C98` | `.?AVLaySchedulerManager@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@` |
| `0x0109F484` | `0x012D0CF0` | `.?AVLaySchedulerManipulator@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@` |
| `0x0109F814` | `0x012F20D8` | `.?AVLaySchedulerFactory@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@` |

The factory has a complete-object vtable. The other listed interfaces are
at complete object `+4`.

### Resource-to-builder boundary

Manipulator initialization `0x00A78420` retains `R` at interface `+0x0C`
and calls complete manager slot 9, `0x00A72D10`. That method constructs
`LaySchedulerFactory` through `0x00A78D30/0x00A25E40`, retaining `R` at
factory `+0x0C`. On a nonnull factory result, the manipulator invokes
`0x00A02F20`, whose factory slot-1 call at `0x00A02FFB` selects
`0x00A78D80`.

That method constructs an object through `0x00A84EE0` and passes it to
`0x00A26380` at `0x00A78E27`. The composition finding below owns the
constructor and parsed class-key mapping.
After its resource, allocation and size guards, `0x00A26380` calls
`0x00A25DC0` at `0x00A2655A` and `0x00A25B90` at `0x00A2657D`.

The first advances the supplied arena cursor by `0x14`, calls
`0x00A27450` at `0x00A25E0D`, and returns the object retained by its
caller at new-object `+0x74`. The second has serialized `@CACT` and
`@CTRK` branches. Their original comparison strings are at `0x010984D4`
and `0x0109821C`. Its `@CACT` branch calls `0x00A298D0` at
`0x00A25C5A`, with the parsed record pointer, the object at `+0x74` and
arena `new-object+0x60`. Its `@CTRK` branch calls `0x00A26850` at
`0x00A25CA0` with a parsed pointer and the same arena. The
[SCB composition finding](scb-clip-factory-composition.md) owns their actor
collection/track preparation and the guarded synchronous
`0x00A26380 -> 0x00A1C7B0 -> 0x00A28510` call path. The local builder's
constructor, exact class-key lookup, matching `0x00837220` registration and
manager-to-scheduler registry-pointer dataflow are established there. The
selected parsed table resolves its class token to the registered name.
Equal retained registry-object values at registration and factory creation,
valid parsed storage and matching node state still condition factory selection.

The established child-context inheritance, numeric registration and clip
constructor's global capture do not establish the owning context at that
conditional factory call. Constructor
`0x008102B0` also retains global `0x013657B8` at complete-object `+0xFC`
at `0x00810319..0x0081032D`; no read of that retained field is connected
to the selected builder path. Uninspected intervening calls can change
context. The owning `U` at either wrapper's actual clip creation
therefore remains an input condition, not a result inferred from call-stack
proximity. The linked composition finding owns the constructed key mapping,
registry-object value conditions and remaining pre-capture context edges.

Instance helper `0x007FBAB0` also publishes slot-61 context around property
slot 24, `0x00AA2A80`. That method submits transform data and does not
establish the missing clip-construction call chain.

## Remaining activation boundary

The static result identifies the layout row, instance-name namespace,
bank-local aliases, timeline bodies and conditional native invocation path.
It supplies no authenticated actor ID/class or spawn row, historical sender,
event calendar, bank-selection policy or visible effect. Those inputs cannot
be inferred from the aliases or the asset names. The full `LaySystemActor`
state was not reconstructed by the receiver lookup.

Weather `8029` and the recovered `MapObjFireworks` script do not supply this
missing retail activation record. Their separate evidence and limits remain
in [City seasonal root effects](city-seasonal-root-effects.md#script-and-activation-boundary).
The constructed parsed key table and registry-pointer source establish
conditional factory selection. Complete wrapper initialization and the owning
tree's context at actual clip capture remain unjoined. The
[parser-to-builder call path](scb-clip-factory-composition.md#synchronous-builder-invocation)
is synchronous under its stated guards. Historical orchestration requires an
authenticated dispatch or observation that identifies the actor, binding and
submitted names together. A calendar and settings controller is a separate
[server policy choice](../event/seasonal-control-plane.md#server-orchestration-boundary).
