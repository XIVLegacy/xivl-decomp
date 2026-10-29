# Cut scheduler BG action clip

`RaptureBgActionClip` consumes two name indices, one u32, a Boolean byte and
a separate mode byte. The Boolean selects two different target-resolution
APIs. The mode maps to native dispatch values `0x7F` through `0x82`.
These are static consumer relationships. Historical playback, concrete target
binding and any resulting collision/reset behavior require separate evidence.

## Source and ownership

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs. Python 3.12 and Capstone 5.0.7 in x86-32 mode
recovered entry-seeded local control flow from the original PE. Input size
and digest matched `xivl-client-structs:manifests/retail_inputs.json`, input
`ffxivgame-1.23b`. No retail execution is implied.

[The slot catalog](../../config/ffxivgame.vtable_slots.jsonl), record line
32503, identifies `0x00816EA0` as slot 1 of concrete
`Application::Scene::Cut::Clip::RaptureBgActionClip` vtable `0x01002CC4`.
The original four-byte pointer cell at `0x01002CC8` matched that entry,
SHA-256 `091485fcc7e78631811c84cec20e5bf63156e7731be98a7ea945af9bae37cef2`.
The owning RTTI records are in
[ffxivgame.rtti.json](../../config/ffxivgame.rtti.json), lines 1881-1882.
Their producer was `tools/ghidra_scripts/DumpRtti.java`, Ghidra 12.1, for
the same binary digest. Concrete slots 2-41 match the typed base table.
This does not independently establish their complete lifecycle effects.

## Begin context and record fields

Let C be the clip, F its stack begin-context argument and `D = u32(C+0x14)`.
`0x00816EE5` calls base begin `0x00A20F30` with C and F. The generic
scheduler context/lifecycle contract is documented in
[Cut scheduler timing](cut-scheduler-clip-timing.md).
`0x00816EEC` calls `0x00A20030(F)` and `0x00816EFB` passes its result to
`0x0060B360`. Each null result exits. The method reads
`A = u32(container+0x68)` at `0x00816F08` and exits if A is null.
It forms A+`0x164` and tests that computed address for zero at
`0x00816F19`, without another dereference. The conditional cached
CutManagerActor route is independently described in
[Attribute resource and scene binding](attribute-resource-and-scene-binding.md#character-factory-and-world-provider).

| Record read | Width | Native site | Use |
| --- | ---: | --- | --- |
| D+`0x14` | 4 | `0x00816F29` | First name index |
| D+`0x18` | 4 | `0x00816F50` | Second name index |
| D+`0x1C` | 4 | `0x00816F72` | Numeric argument N |
| D+`0x20` | 1 | `0x00816F6E` | B = byte nonzero, normalized at `0x00816F75` |
| D+`0x21` | 1 | `0x00816F7C` | Unsigned mode Q |

The method does not read D+`0x22` or D+`0x23`. It does not consume the whole
dword at D+`0x20` as one numeric operand. No direct D-null, actor ordinal,
status or collision test occurs here.

At `0x00816F30` and `0x00816F57`, `0x00A20080`, receiver F, receives the
respective index as its sole stack argument. Each returned name pointer and
the length loaded from `0x00F67298` feed string constructor `0x00447260`.
The length was pushed before the index lookup for that later constructor;
it is not another lookup argument. The constructor ABI is reused from
[SQWT form locale resolution](sqwt-form-locale-resolution.md), rather than
inferred from that push order alone.

The unsigned comparison/branch at `0x00816F80` / `0x00816F88` selects the
four-entry table `[0x0081700C,0x0081701C)`, SHA-256
`83e96f1e66078a95080030f5aab721d3686b0cafde56b946980cdffc06e7e299`.

| Q | Branch target | Native value P |
| --- | --- | --- |
| 0 | `0x00816FA4` | `0x7F` |
| 1 | `0x00816F91` | `0x80` |
| 2 | `0x00816F98` | `0x81` |
| 3 | `0x00816F9F` | `0x82` |
| >3 | Default `0x00816FA4` | `0x7F` |

`0x00816FB8 -> 0x00617040` uses ECX=A+`0x164` and nearest-first stack
arguments `[A, firstString*, secondString*, B-slot, N, P]`. Only B-slot's
low byte is normalized; the downstream reader uses only that byte. The
method destroys both temporary strings and returns with `RET 4`.

## Target-resolution branches

`0x00617040` ignores incoming ECX and checks A at `0x00617046`. Null A
returns with `RET 24`. Otherwise `0x00617067 -> 0x006165F0` uses ECX=A+
`0x340` and `[firstString*, secondString*, B-slot, N, P]`.
The latter method initializes a result through `0x00621200`, reads
u8(B-slot) at `0x00616629` and branches at `0x00616638`. Both names are
read through string accessor `0x00445210`. Define `M = u32(A+0x340)`.
M's constructor/vtable identity is not independently established here.

| B | Direct call | ECX | Nearest-first arguments |
| --- | --- | --- | --- |
| Nonzero | `0x0061665B -> 0x0062A090` | M | `[result*, P, firstName, secondName, N, 0, float32(1.0)]` |
| Zero | `0x0061667E -> 0x0062A100` | M | `[result*, P, firstName, secondName, 0, float32(1.0)]` |

The zero branch omits N. It does not submit numeric zero in N's position.
Both paths destroy the result through `0x00621260` and return with
`RET 20`, without testing the downstream return value. The local result
constructor/destructor bodies remain uninspected.

Both branch helpers call `0x00626710(M, firstName)` at `0x0062A098` /
`0x0062A108`. Null layout lookup exits. Its layout-name walk is reused from
[Ambient Garlean fleet](ambient-garlean-fleet.md). For nonnull result R,
define V=R+`0xE0`. The nonzero branch calls V vtable +`0x14`, zero-based
slot 5, at `0x0062A0CB` with `[result*, secondName, 7, N]`. The zero branch
calls V vtable +`0x1C`, slot 7, at `0x0062A136` with
`[result*, secondName, 7]`. The actual virtual targets and their operation
names remain UNESTABLISHED. These observations do not name slot 7
cancellation or N a command ID.

Both skip the shared helper when `u32(result+0x0C) == 0x00FFFFFF`.
Otherwise `0x0062A0EB` / `0x0062A156` calls `0x00629E40` with ECX=M and
`[P, result*, 0, float32(1.0)]`. The branch helpers return with `RET 28`
and `RET 24`, respectively.

## Per-element dispatch boundary

`0x00629E40` reads count `K = u32(result+0x0C) & 0x00FFFFFF` and base
`L = u32(result+0x08)`. K=0 returns EAX=8. Each iteration passes
`u32(L+4*i)` to `0x008013E0` at `0x00629E7B`. A zero conversion result
returns EAX=10 immediately, potentially after earlier elements were
dispatched. For nonzero T, `0x00629EA0 -> 0x00624B30` uses ECX=M and
`[P, T, 0, float32(1.0)]`. The loop returns its last dispatcher result,
with `RET 16`. This shared helper forwards P without packing a queue record.

When `u32(M+0x1F0) == 3`, the iteration also queries the original element's
vtable +`0x18` and +`0x24` at `0x00629EB9` / `0x00629ECD`. It adjusts
nonnull results by -4 and calls `0x00805470` through `u32(M+0x350)` at
`0x00629EEE`. Those side effects remain opaque. Value 3 is not established
as an actor state or encounter phase.

### Element conversion

`0x008013E0` dereferences input E without an explicit null guard. At
`0x008013EE`, E slot 21 receives integer 7 and must return nonzero AL.
E slot 47 at `0x00801402` supplies P, also without an explicit P-null check.
P slot 1 at `0x0080140D` must return 7, and P slot 5 at `0x0080141B` must
return nonzero AL. The accepted path restores its stack and tail-dispatches
P slot 30 at `0x00801429`, ECX=P, with no stack arguments. Each failed
predicate returns zero through `0x008013F7`. The slot-30 result itself may
be null, which the per-element caller rejects as described above.

This identifies T as the accepted kind-7 object's slot-30 result. The
published kind-7 property/TimeLine path in
[Timeline weather record view](timeline-weather-record-view.md) and
[Seasonal decoration update order](seasonal-decoration-update-order.md)
has the same getter role. Its concrete TimeLine and H/Scheduler bindings
remain conditions on reusing those operation semantics for this clip.

### Selected final virtual calls

The dispatcher at `0x00624B30` compares `P-0x7F` unsigned with `0x0C`.
Values outside `0x7F..0x8B` return EAX=5. Its two-level table at
`0x00624C40` / `0x00624C24` gives these four BG-reachable cases. Other
operation numbers were not inspected.

| P | Branch | Calls on converted T for the clip's supplied 0 and float32(1.0) |
| --- | --- | --- |
| `0x7F` | `0x00624B4F` | Slot 4 at `0x00624B5A`; slot 17 with float32(1.0) at `0x00624B6B`; slot 3 with `[0,-1]` at `0x00624BA5` |
| `0x80` | `0x00624BAD` | Slot 4 |
| `0x81` | `0x00624BBE` | Slot 5 |
| `0x82` | `0x00624BCF` | Slot 6 |

All four selected cases return EAX=0 with `RET 16`. This function does not
test those virtual-call results. No direct `0x008024C0` call or queue-record
packing occurs in these selected paths. The concrete type and virtual targets
of T remain unbound. A shared slot shape alone does not identify it with the
previously studied TimeLine manipulator or assign start/cancel/pause meanings.

The recovered reachable ranges for the complete begin/conversion methods
were `[0x00816EA0,0x0081700C)` and `[0x008013E0,0x0080142B)`, SHA-256
`3caebc767078a69e0ea0868131fabb49d1c5d07530f2da68e214a555412f2133`
and `f10b513548632fdafa844f5e8b2e7a34e0e4b6c9391ec5aaf9f83e8140ab4676`.
The selected dispatcher paths occupied `[0x00624B30,0x00624BE0)` and
`[0x00624C1B,0x00624C24)`, SHA-256
`6076c2e9edfbd4d010a862108db4ec4530877cd7e5baff32f8ce9c888d20f607`
and `80bdb21ac54b5204fcf91b50a5945682ba08023d6f71f8b948443406aeed36b3`.
Ends are exclusive. These instruction unions were recovered separately from
catalog size totals. Operations `0x83..0x8B` remain outside the selected paths.

## Authored Dzemael records

[Raid cutscene actor records](raid-cutscene-actor-records.md) identifies
the two `rad0r102` records at payload `0xB50` / `0xDA8`, with layout
`roc_r0_dun01`, target `isgrp_001406` and trailing u32 words `[16,1]`.
The two `rad0r103` records at `0xA38` / `0xD9C` name the same layout,
target `time_door_a1_open` and words `[18,0]`. Six doors match that target.
Their original resource and layout digests remain in that canonical finding.

Generic construction stores supplied D at clip+`0x14`, and the scheduler
can rebind it to the current record, as documented in
[Cut scheduler clip layout](cut-scheduler-clip-layout.md) and
[Ambient Garlean fleet](ambient-garlean-fleet.md). This observation does not
independently trace the concrete BG clip factory, constructor or current D
writer. Composition remains conditional on the concrete instance receiving
the authored 36-byte D record.

Under that condition, `[16,1]` supplies N=16, B=1, Q=0 and P=`0x7F`.
Its target-resolution request uses slot 5 with `isgrp_001406`, kind 7 and
N=16. `[18,0]` supplies N=18, B=0, Q=0 and P=`0x7F`; its slot-7 request
uses `time_door_a1_open` and kind 7, omitting N=18. This does not establish
resolved target identity, successful dispatch or historical viewing policy.

These native selectors are separate from command IDs, packed playback
selectors and model-qualified WSS/BID banks. No numeric boss substate,
collision setter, Attribute control polarity or defeat/wipe/retry/exit
restoration follows from the record fields alone.
