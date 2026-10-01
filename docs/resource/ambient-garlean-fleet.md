# Ambient Garlean fleet visibility boundary

Five retail air layouts name an Imperial airship family through `sgrp_bg_emp`,
`grp_air_emp1/2/3`, `w_air_fuji_l_body1/2/3`, `sdef_teikoku_hikutei`,
`emp_stt0`, and `time_bg_emp_stt0`. Their assignment to the observed ambient
Garlean fleet is probable. An independent server control remains unresolved.
The meteor selector and ordinary transport scenes do not establish that control.

## Affected-layout and control candidates

The raw `RegionResourceData` at `data/03/C0/00/00.DAT` is 52,336 bytes,
SHA-256 `c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
Its child rows join the following tokens to MapL DAT keys. The row offset is
the first matching child record in that DAT. Parent IDs are resource-table
IDs, not authenticated wire/server zone IDs. Per-zone coverage and camera
visibility are unresolved; the live Ul'dah observation below is one exception
to the absence of a loading observation.

| Area family | Child / token | DAT key / bytes | SHA-256 | Region row / parent IDs | `sgrp_bg_emp` / `time_bg_emp_stt0` string offsets |
| --- | --- | --- | --- | --- | --- |
| Limsa Lominsa / La Noscea | 191 / `sea_s0_air01` | `0x29D90018` / 43,792 | `62a122b6a7480c79f5c5a61f737c350822f401775a4a3bd5fcefe11ffcc4d8a7` | `0x5B0` / 101, 117, 126, 127 | `0x13EB` / `0x229F` |
| Coerthas | 291 / `roc_r0_air01` | `0x28D90013` / 18,608 | `f780a2d054d05526a07417d525b56eba9dca69b2f8a04dcee03d7bdcbe64a452` | `0xAC0` / 102, 119, 120, 211, 212 | `0xD0C` / `0x13B5` |
| Gridania / Black Shroud | 391 / `fst_f0_air01` | `0x29B00018` / 41,504 | `c9e8dd231cf675cb6cf7ed9239ff1f6889e9f1e2bbc8099d78ab956a9d41d44c` | `0x10F0` / 103, 106, 115, 122, 123 | `0x13DC` / `0x229C` |
| Ul'dah / Thanalan | 491 / `wil_w0_air01` | `0x615A001A` / 44,064 | `b9f66677ca8807caaed2da51d748a71c5eb5e6bde425b49e74251650884c9cf8` | `0x17B0` / 104, 107, 116, 124, 125, 130, 213, 214 | `0x13EA` / `0x22C1` |
| Mor Dhona | 591 / `lak_l0_air01` | `0x03E7000F` / 10,896 | `af9b742c3a9745f10ca46a72ddbb8b987b53d8896c9e87a8c6070ba0a2b54aef` | `0x1AE0` / 105, 129 | `0xABD` / `0x10AC` |

The MapL root paths follow
`gra_rapture/bg/public/<family>/air/layout/<token>/<token>.reference.dst`.
Their string offsets in table order are `0x15F2`, `0xF0B`, `0x15EF`,
`0x15FD`, and `0xD51`. The Mor Dhona file also contains a `roc_r0_air01`
reference at `0xC4B`; it does not replace the matching `lak_l0_air01` root.

In Gridania's DAT, `grp_air_emp1/2/3` occur at `0x13F4/0x1424/0x1454`,
the three body names at `0x1401/0x1431/0x1461`,
`sdef_teikoku_hikutei` at `0x1484`, and `emp_stt0` at `0x1499`.
These are exact literal locators, with high confidence for byte presence.
The structural join below establishes a placed scheduler family. Automatic
playback and a server enable/disable value remain unresolved for every row.

The producing method was a Python 3 binary read: walk the `0x30`-byte
RegionResourceData records from `0x40` using parent count at `0x24` and child
count at record `+0x0C`, read the key at `+0x08` and token at `+0x10`, then
scan and hash the five corresponding DATs. A key `0xAABBCCDD` resolves to
`data/AA/BB/CC/DD.DAT`. No asset bytes are retained in this finding.

AuroraFlare's prior lead is
`AuroraFlare/xiv-decomp:docs/airship_ferry_decomp_open_questions_2026-06-21.md:331-358,664-686`,
previously inspected at `d5fb880e17f07f31ea7f8380ff813e5d71b6b678`.
The inspected newer source revision is
`dca944d3e0cfadd4495b45f2c41d8aba9cd49390`, including
`tools/build_citystate_scheduler_decomp_atlas.py:170-213`.
Its `art_s0/art_r0/art_f0/art_w0` airship-over result remains a bounded
transport-resource lead, not a fleet-control finding. The resource rows and
literals above were re-read from retail artifacts; the imported reports are
not additional independent retail evidence.

## Placed group and authored visibility

All offsets in this table are physical offsets within the hash-matched DAT
above. Each placed `RefObjects/InstanceObject` targets the named
`RefObjects/UnitTree/UnitTreeObject` `sgrp_bg_emp` (serialized GID 1157).
That tree has one member, alias `emp_stt0`, targeting the timeline
`time_bg_emp_stt0` (GID 1634). The GIDs are serialized node identifiers,
not authenticated server actor IDs.

| Layout | Placed instance @ offset / GID | Group / member / timeline offsets | Embedded SCB offset / bytes / SHA-256 |
| --- | --- | --- | --- |
| `sea_s0_air01` | `isgrp_000003` @ `0x3A50` / 935 | `0x2D90` / `0x35C0` / `0x47B8` | `0xA240` / 2256 / `3adb44922685793e7aba35881db83c9bcb6d4d82722015d5a52e356fdf1e22a3` |
| `roc_r0_air01` | `isgrp_000002` @ `0x20A0` / 934 | `0x1B00` / `0x1EB0` / `0x2620` | `0x3E90` / 2592 / `2ca548de4e8f5fdfcfff2b135346b69f20185c11903946a13ef7a2403d6eb929` |
| `fst_f0_air01` | `isgrp_000005` @ `0x3A10` / 937 | `0x2D80` / `0x35B0` / `0x4778` | `0x9970` / 2224 / `b50aed9eb409c0ad5d619da4ce9179e424da3519dbbf47c5e740aa780afc2856` |
| `wil_w0_air01` | `isgrp_000003` @ `0x3A70` / 935 | `0x2DB0` / `0x35E0` / `0x47B4` | `0xA200` / 2592 / `7949734110c0e51e3c106a0b748018d8a315f2ba8fbe8bf637f7cb538859affc` |
| `lak_l0_air01` | `isgrp_000002` @ `0x1AA0` / 934 | `0x16A0` / `0x1900` / `0x1F30` | `0x2070` / 2592 / `fb13527c73fa1bce855685dc326cd6a7c658321ea141e3f2f06a79da2ba0ff18` |

Each SCB has eight actor records: one group, three body slots, three VFX
slots, and one sound-definition slot. The body actor labels are the stored
16-byte `w_air_fuji_l_bo` token at indices 1, 2, and 3; the full body names
above are separate layout strings. `grp_air_emp1/2/3` are not three decoded
UnitTree children. The SCB group actor label is `sgrp_bg_emp` in Limsa and
`grp_air_emp` in the other four layouts.

The clip registry contains `LayVFXClip`, `ShowHideClip`, `LaySEClip`, and
`LayTransformClip`, with no `IfClip`. Each second `@CBLK` contains 11 clips
and a raw duration of 288,000,000. Three `ShowHideClip` records target the
body indices at start 0 with value word 1 at record `+0x10`; three target
the same indices at start 287,990,000 with value 0. For example, Limsa's
show records are at `0xA56C`, `0xA5F8`, `0xA60C` and hide records at
`0xA620`, `0xA634`, `0xA648`. The other five clips control VFX, sound, and
the group transform. These are authored show/hide values and raw timeline
units, not a wire command, wall-clock duration, or historical schedule.
The show/hide interpretation is consistent with the direct paired-SCB
comparison in [City seasonal weather selectors](city-seasonal-weather-selectors.md),
"Historical layout delta"; it does not reuse that family's weather condition.

Confidence is high for the pointer and typed-record observations, probable
for association with the visually described fleet. No decoded automatic-start
field or runtime owner join follows from the absence of `IfClip`.
The authored family includes fleet VFX and sound as well as body visibility.
Changing the body words alone is not proof of a complete fleet disable.
Four layouts also contain a separate authored airship unit tree. A whole-layout
disable would couple the fleet to that tree; it is not an isolated fleet
boundary. These physical DAT locators use the same LYB decoder and input
hashes above. They do not establish a callable server control or a historical
travel schedule.

| Layout | Other unit tree @ physical DAT offset | Member aliases |
| --- | --- | --- |
| `sea_s0_air01` | `sgrp_bg_air` @ `0x2D40` | `spin`, `spot`, `_air_wing_spin`, `_air_wing_spot`, `_air_wing_clos`, `_air_wing_open`, `_air_show`, `_air_hide` |
| `roc_r0_air01` | `sgrp_bg_air_low` @ `0x1AB0` | `stt0`, `end0`, `von1` |
| `fst_f0_air01` | `sgrp_bg_air` @ `0x2D30` | Same eight aliases as Limsa |
| `wil_w0_air01` | `sgrp_bg_air` @ `0x2D60` | Same eight aliases as Limsa |
| `lak_l0_air01` | No second unit tree in the decoded node table | Unresolved controls remain |

The separate group supplies a candidate boundary for isolation, but no
authenticated server path to that boundary establishes preservation of
travel/cutscene airships, moon, weather, or other decorations. Neither
server-only feasibility nor a need for a client change is established.

The producing tools were local Python 3 decoders `parse_fleet_structure.py`
(SHA-256 `cf7a4e7709cf1bd0a1aa48396f1855c706f0cd42c55193889194a0c0717ecc56`)
and `parse_fleet_scb.py`
(`4efc8239e55124fbb73889551a88185155f671f30fcc82ed0a6cf8f58dbfc535`).
The LYB base is the u32 at DAT `+0x20` plus `0x30`. Follow its node-pointer
table, the UnitTree array/count at node `+0x30/+0x34`, each `0x30`-byte
member's alias/target at `+0x14/+0x18`, and the timeline's SCB pointer/size
at `+0x14/+0x18`. Pointers are LYB-relative. The SCB decoder checked version
2, ABI `0x00300000`, size, String/CATT/CCPT/CACT/CBLK/CCNT envelopes and
record bounds. The method follows AuroraFlare's
`docs/ifrit-animation-decomp-2026-08-02/EXHAUSTIVE_BATTLEFIELD_RESOURCE_COVERAGE.md:51-56`
at the newer revision above, with direct retail byte verification.

## Binary evidence

The retail 1.23b `ffxivgame.exe` is PE32/x86, image base `0x00400000`,
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Its version resource contains `FileVersion` `1.0.00` and `PrivateBuild`
`46686`. Addresses below are VAs in this executable.

Direct disassembly with Capstone 5.0.7 and PE reads with pefile 2024.8.26
confirmed these bounded observations. Ghidra 12.1.3 decompilation reproduced
the same selected control flow from a fresh read-only import, with no analysis
timeout or selected-function decompilation failure. This is another analysis
of the same executable, not independent retail evidence.

| Locator | Observation | Confidence |
| --- | --- | --- |
| `0x0059CFC4..0x0059CFC9` | The SetDalamud receiver reads an unsigned byte at packet `+0x10` and stages it at `MapLayoutElement+0xF0`. | Direct instructions |
| `0x0059CA60`, especially `0x0059CA6D..0x0059CACA` | The pending selector waits while `+0xBC` is nonzero, emits eight operation `0x7E` records, then restores `0xFFFFFFFF`. | Direct instructions |
| `0x0062CDA0`, case table at `0x0062D078`/`0x0062D0E0` | Case `0x7E` reaches `0x0062CFEB`, which calls `0x0062B7B0`. | Direct table and instructions |
| `0x0062B7B0` | The operation sets or clears a bit in `MapLayoutActor+0x358`, then calls `0x0062AEB0`. | Direct instructions |
| `0x0062AEB0`, pointer table `0x012B82BC`, literals `0x00FB98A8..0x00FB9924` | The visibility loop addresses only `sgrp_meteor` and `sgrp_meteor117` through `sgrp_meteor123` under `sky_celestial`. | Direct instructions and data |

The Ghidra producing tool was `tools/ghidra_scripts/DecompileToText.java`
(SHA-256 `83a31add99e86bea0128470e4cc52b193e373ebc342cf060861bd3c17339668a`),
through `tools/ghidra/run-headless.ps1`, run
`ambient-fleet-visibility-20260930-01`, JDK `21.0.12+8`, memory `8G`,
fresh import, read-only, timeout `0`. Reproduce with an explicit hash-matched
binary, a new ignored output directory, and `DECOMP_VAS` set to
`0x0062AEB0,0x0062B7B0,0x0062CDA0,0x0059CA60,0x0059EBA0,0x0059EE50,0x0059CED0`.
Raw output and the disposable project remain local-only.

A second fresh run, `ambient-fleet-layout-consumer-20260930-01`, used the
same script and toolchain with targets
`0x0062C4D0,0x00623CF0,0x00626710,0x00629F20,0x00629F60,0x00629FF0,0x0062BC40,0x0062A170,0x00616530,0x0062B380`.
It also completed without an analysis timeout or selected-function failure.
At `0x00629F20`, the generic helper forwards an argument to vtable `+0x98`
on every object in a resolved vector. `0x0062BC40` and `0x0062A170` reach
that helper after layout/object lookup. No fleet-specific caller or binding
was established. The nearby `0x00629F60` compares 8030/8032 and calls
`0x00627940`, which updates the eight meteor masks; those weather values
are not authenticated fleet controls.

This is a bounded consumer observation, not proof that a meteor group can
contain no nested ship asset. The documented fan-out alone does not identify
the ambient fleet. Selector values and meteor lifecycle semantics have their
canonical home in
`xivl-client-structs:docs/client-architecture.md`, "SetDalamud s2c 0x0010
selects one of eight scene states".

The [ordinary transport finding](../event/airship-scene-sequence.md) identifies
the separate `zep0g000`/`zep0g010`, `zep0l000`/`zep0l010`, and
`zep0u000`/`zep0u010` scene assets. Their scene-local actor dictionaries do not
identify the persistent ambient fleet.

Player special-event work and area weather are separate producer paths;
their known boundaries are in
[Seasonal event work control plane](../event/seasonal-control-plane.md) and
[Weather transition runtime](../net/weather-transition-runtime.md).
An event name or weather label supplies no fleet join.

## Live Ul'dah observation

On 2026-10-01 UTC, a test session using the hash-matched executable above
had `wil_w0_air01` in `MapLayoutActor`'s loaded-layout array at `+0x138`.
The object had RTTI vtable VA `0x00FBFEE4` (`RaptureLayoutManager`),
resource key `0x615A001A` at `+0x158`, and name `wil_w0_air01` at `+0x17C`.
The array's end pointer was at `+0x13C`. Direct disassembly of
VA `0x00626710..0x0062681C` with Capstone 5.0.7 authenticated the pointer
array walk and name comparison. This establishes a loaded resource, with
high confidence; it does not establish an active scheduler or its producer.

The producing tools were x86 CDB 10.0.29617.1000 and Python 3.12
`ReadProcessMemory`, with no client function invocation or memory write.
The retained local layout snapshot had SHA-256
`189bdbee5ab8445376f963943237075148d0789e531be2a3bf106d2abb39693f`.
A user-supplied image showed three Imperial ship bodies overhead; its
SHA-256 was
`716cd74c3be4df920fc2404086f36a581877066efa75acdfb11d16f517e3a001`.
The user identified the area as Ul'dah privatearea, server zone 184. That
reported zone number is not a decoded retail resource ID. The image and
loaded resource strengthen the fleet association but do not isolate its
render objects. The session included launcher and navigation hooks, so this
observation does not authenticate an unmodified runtime or historical
retail activation schedule. Raw snapshots, debugger logs, and image remain
local-only.

A subsequent read-only snapshot joined the loaded Ul'dah layout to placement
`isgrp_000003`, serialized GID 935, and its `sgrp_bg_emp` base object, GID
1157. The placement's unit-info table contained the exact eight aliases
`emp_stt0`, `w_air_fuji_l_body1/2/3`, `vfx_hiku002_011/012/013`, and
`sdef_teikoku_hikutei`. The relocated `emp_stt0` binding matched DAT member
`0x35E0` and target timeline `0x47B4`, GID 1634. Each saved binding key,
alias and target matched the pinned DAT bytes. This authenticates the live
fleet resource/group join, with high confidence for those identities. It
does not show whether that scheduler was running or which caller started it.

The producing tool was Python 3.12 `ReadProcessMemory`; the snapshot SHA-256
was `49fc4c21327b7377e97f632bcd924b15da82dc129c4367890507f1031f34b498`.
To reproduce, resolve the named layout through `MapLayoutActor+0x138`, then
subtract 4 from the pointer at layout `+0x24` to obtain its block manager,
then walk the block's instance
array at `+0x20` with count at `+0x24 & 0xFFFFFF`. Match the placed node and
base-object identities against the DAT before inspecting unit-info members.
Instance `+0x1C` points to unit-info; its `+0x84` points to the member
table. Table `+0x14` points to the tree head, whose `+0x04` is the root.
Tree nodes have left/right links at `+0x00/+0x08`, key at `+0x0C`,
and member instance at `+0x10`; member instance `+0x20` points to the
serialized binding with alias/target at `+0x14/+0x18`.
The instance-name getter at VA `0x00A99C90` returns serialized node `+0x08`
when instance `+0x08` is nonnull, otherwise member `+0x14` through instance
`+0x20`. Direct Capstone disassembly authenticated those name fields. Live
pointers are session-local and must be reacquired after layout unload.
The saved snapshot omits the raw table-head and tree-node links; its member
bindings can be checked against the DAT, but the traversal itself cannot be
fully replayed from that snapshot.

## Unresolved control edges

| Edge | Status |
| --- | --- |
| Fleet resource to layout group | Static placed chain and live Ul'dah aliases/target established; exact rendered-object attribution remains unobserved |
| Visibility consumer to initialization and producer | Unresolved |
| Wire opcode/field, scene operation, actor work, or authored condition | Unresolved |
| Exact enable and disable values | Authored body show/hide words 1/0 established; producer command values unresolved |
| Player, area, layout, or global scope | Unresolved |
| Login reset and reapplication | Unresolved |
| Ordinary zone-transfer reset and reapplication | Unresolved |
| Seamless-crossing reset and reapplication | Unresolved |
| Ordering relative to layout readiness or another update | Unresolved |
| Independent server-only switch and coupled content | Unresolved |
| Need for a client change | Unresolved |

No historical retail activation schedule follows from these static sources.
The live observation above does not resolve the activation edge.

## Activation probe boundary

The initial probe used an authorized Ul'dah session. The fleet appeared
before its start call was captured. A conditional hardware execution
breakpoint at VA `0x0064C000` produced no matching fleet request during the
retained observation. This candidate path was not established as exhaustive;
the non-hit proves neither automatic playback nor absence of a server update.
No activation stack or exact SCB runtime instance was obtained. The debugger
was detached and the client remained running.

The single remaining bounded probe is to arm an authenticated fleet scheduler
start site before one ordinary Ul'dah load, then record the first matching
`time_bg_emp_stt0` activation's caller stack, owning layout/instance, requested
name and initial arguments. Establish whether it came from initialization or
an incoming update. Do not inject commands or change actor work, weather,
Dalamud, or server state. Stop after the first activation or completed load.
That follow-up has not been executed and would not by itself resolve the
later transfer/reset edges.
