# Ambient Garlean fleet visibility boundary

Five retail air layouts name an Imperial airship family through `sgrp_bg_emp`,
`grp_air_emp1/2/3`, `w_air_fuji_l_body1/2/3`, `sdef_teikoku_hikutei`,
`emp_stt0`, and `time_bg_emp_stt0`. Their assignment to the observed ambient
Garlean fleet is supported by the live resource join below. Retail code
directly selects `emp_stt0` and applies a clock-derived timeline phase.
An independent server control remains unresolved.
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
playback has a direct binary consumer below. A server enable/disable value
remains unresolved for every row.

The producing method was a Python 3 binary read: walk the `0x30`-byte
RegionResourceData records from `0x40` using parent count at `0x24` and child
count at record `+0x0C`, read the key at `+0x08` and token at `+0x10`, then
scan and hash the five corresponding DATs. A key `0xAABBCCDD` resolves to
`data/AA/BB/CC/DD.DAT`. No asset bytes are retained in this finding.

A prior external lead is
`docs/airship_ferry_decomp_open_questions_2026-06-21.md:331-358,664-686`,
inspected at revision `d5fb880e17f07f31ea7f8380ff813e5d71b6b678`.
The same source's earlier revision
`dca944d3e0cfadd4495b45f2c41d8aba9cd49390` was also inspected, including
`tools/build_citystate_scheduler_decomp_atlas.py:170-213`.
Its `art_s0/art_r0/art_f0/art_w0` airship-over result remains a bounded
transport-resource lead, not a fleet-control finding. The resource rows and
literals above were re-read from retail artifacts; the imported reports are
not additional independent retail evidence.

## Placed group and authored visibility

All offsets in this table are physical offsets within the hash-matched DAT
above. Each placed `RefObjects/InstanceObject` targets the named
`RefObjects/UnitTree/UnitTreeObject` `sgrp_bg_emp` (serialized GID 1157).
That tree has one direct child, alias `emp_stt0`, targeting the timeline
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
record bounds. The method follows the same external source's
`docs/ifrit-animation-decomp-2026-08-02/EXHAUSTIVE_BATTLEFIELD_RESOURCE_COVERAGE.md:51-56`
at revision `dca944d` above, with direct retail byte verification.

### Numeric actor keys and declared child targets

An independent Python 3.12.0 read of the five hash-pinned DATs above joined
all 35 serialized `LayUnitMemberActor` keys to unique children within the
exact selected placement's declared UnitTree/group closure. There were no
missing or ambiguous member keys. The member
[key contract](../actor/lay-scheduler-member-publication.md#serialized-lookup-key)
reads K at actor record+0x20; the
[child registration contract](limsa-fireworks-scheduler-bindings.md#unittree-child-key-table)
uses child+0x1C in the containing UnitInfo context. These are conditional
native contracts composed with original static resource fields. The CATT
labels do not themselves authenticate a runtime factory or bind the serialized
actor to `LaySchedulerUnitMemberActor`; the numeric equality is a static join.

Each closure includes one direct timeline child and four group records:
the outer group holds the sound child and three subgroups each hold one body
and one VFX child. The body aliases reference the same BGParts base node
through three distinct children. Offsets below are physical DAT offsets.
Each CACT record is 0x24 bytes; ordinal equals raw actor index. Raw CATT kind
0 is `LayUnitActor` and kind 1 is `LayUnitMemberActor`, each with descriptor
0. The ordinal-0 raw +0x20 value is recorded separately because the member
key getter does not type that group's field.

| Layout | Ordinal | CACT physical | Raw kind/index | K | Child alias / status | Child / parent | Target declaration / offset / GID |
| --- | --- | --- | --- | --- | --- | --- | --- |
| sea_s0_air01 | 0 | 0xA310 | 0 / 0 | 10 raw | separate | - | - |
| sea_s0_air01 | 1 | 0xA334 | 1 / 1 | 4 | w_air_fuji_l_body1 | 0x3470 / 0x2F60 | B 0x40D4 / 1258 |
| sea_s0_air01 | 2 | 0xA358 | 1 / 2 | 5 | w_air_fuji_l_body2 | 0x34D0 / 0x2F90 | B 0x40D4 / 1258 |
| sea_s0_air01 | 3 | 0xA37C | 1 / 3 | 6 | w_air_fuji_l_body3 | 0x3530 / 0x2FC0 | B 0x40D4 / 1258 |
| sea_s0_air01 | 4 | 0xA3A0 | 1 / 4 | 7 | vfx_hiku002_007 | 0x34A0 / 0x2F60 | V 0x45D8 / 1014 |
| sea_s0_air01 | 5 | 0xA3C4 | 1 / 5 | 8 | vfx_hiku002_008 | 0x3500 / 0x2F90 | V 0x45D8 / 1014 |
| sea_s0_air01 | 6 | 0xA3E8 | 1 / 6 | 9 | vfx_hiku002_009 | 0x3560 / 0x2FC0 | V 0x45D8 / 1014 |
| sea_s0_air01 | 7 | 0xA40C | 1 / 7 | 21 | sdef_teikoku_hikutei | 0x3590 / 0x2F30 | S 0x4564 / 707 |
| roc_r0_air01 | 0 | 0x3F60 | 0 / 0 | 11 raw | separate | - | - |
| roc_r0_air01 | 1 | 0x3F84 | 1 / 1 | 4 | w_air_fuji_l_body1 | 0x1D60 / 0x1BE0 | B 0x2294 / 1258 |
| roc_r0_air01 | 2 | 0x3FA8 | 1 / 2 | 5 | w_air_fuji_l_body2 | 0x1DC0 / 0x1C10 | B 0x2294 / 1258 |
| roc_r0_air01 | 3 | 0x3FCC | 1 / 3 | 6 | w_air_fuji_l_body3 | 0x1E20 / 0x1C40 | B 0x2294 / 1258 |
| roc_r0_air01 | 4 | 0x3FF0 | 1 / 4 | 8 | vfx_hiku002_008 | 0x1D90 / 0x1BE0 | V 0x2580 / 1014 |
| roc_r0_air01 | 5 | 0x4014 | 1 / 5 | 9 | vfx_hiku002_009 | 0x1DF0 / 0x1C10 | V 0x2580 / 1014 |
| roc_r0_air01 | 6 | 0x4038 | 1 / 6 | 22 | sdef_teikoku_hikutei | 0x1E80 / 0x1BB0 | S 0x2534 / 707 |
| roc_r0_air01 | 7 | 0x405C | 1 / 7 | 10 | vfx_hiku002_010 | 0x1E50 / 0x1C40 | V 0x2580 / 1014 |
| fst_f0_air01 | 0 | 0x9A40 | 0 / 0 | 10 raw | separate | - | - |
| fst_f0_air01 | 1 | 0x9A64 | 1 / 1 | 4 | w_air_fuji_l_body1 | 0x3460 / 0x2F50 | B 0x4094 / 1258 |
| fst_f0_air01 | 2 | 0x9A88 | 1 / 2 | 5 | w_air_fuji_l_body2 | 0x34C0 / 0x2F80 | B 0x4094 / 1258 |
| fst_f0_air01 | 3 | 0x9AAC | 1 / 3 | 6 | w_air_fuji_l_body3 | 0x3520 / 0x2FB0 | B 0x4094 / 1258 |
| fst_f0_air01 | 4 | 0x9AD0 | 1 / 4 | 7 | vfx_hiku002_007 | 0x3490 / 0x2F50 | V 0x4598 / 1014 |
| fst_f0_air01 | 5 | 0x9AF4 | 1 / 5 | 8 | vfx_hiku002_008 | 0x34F0 / 0x2F80 | V 0x4598 / 1014 |
| fst_f0_air01 | 6 | 0x9B18 | 1 / 6 | 9 | vfx_hiku002_009 | 0x3550 / 0x2FB0 | V 0x4598 / 1014 |
| fst_f0_air01 | 7 | 0x9B3C | 1 / 7 | 21 | sdef_teikoku_hikutei | 0x3580 / 0x2F20 | S 0x4524 / 706 |
| wil_w0_air01 | 0 | 0xA2D0 | 0 / 0 | 15 raw | separate | - | - |
| wil_w0_air01 | 1 | 0xA2F4 | 1 / 1 | 2 | w_air_fuji_l_body1 | 0x3490 / 0x2F80 | B 0x40F4 / 1258 |
| wil_w0_air01 | 2 | 0xA318 | 1 / 2 | 6 | w_air_fuji_l_body2 | 0x34F0 / 0x2FB0 | B 0x40F4 / 1258 |
| wil_w0_air01 | 3 | 0xA33C | 1 / 3 | 5 | w_air_fuji_l_body3 | 0x3550 / 0x2FE0 | B 0x40F4 / 1258 |
| wil_w0_air01 | 4 | 0xA360 | 1 / 4 | 11 | vfx_hiku002_011 | 0x34C0 / 0x2F80 | V 0x45D4 / 1014 |
| wil_w0_air01 | 5 | 0xA384 | 1 / 5 | 12 | vfx_hiku002_012 | 0x3520 / 0x2FB0 | V 0x45D4 / 1014 |
| wil_w0_air01 | 6 | 0xA3A8 | 1 / 6 | 13 | vfx_hiku002_013 | 0x3580 / 0x2FE0 | V 0x45D4 / 1014 |
| wil_w0_air01 | 7 | 0xA3CC | 1 / 7 | 28 | sdef_teikoku_hikutei | 0x35B0 / 0x2F50 | S 0x4560 / 706 |
| lak_l0_air01 | 0 | 0x2140 | 0 / 0 | 14 raw | separate | - | - |
| lak_l0_air01 | 1 | 0x2164 | 1 / 1 | 4 | w_air_fuji_l_body1 | 0x17B0 / 0x1720 | B 0x1C88 / 1258 |
| lak_l0_air01 | 2 | 0x2188 | 1 / 2 | 5 | w_air_fuji_l_body2 | 0x1810 / 0x1750 | B 0x1C88 / 1258 |
| lak_l0_air01 | 3 | 0x21AC | 1 / 3 | 6 | w_air_fuji_l_body3 | 0x1870 / 0x1780 | B 0x1C88 / 1258 |
| lak_l0_air01 | 4 | 0x21D0 | 1 / 4 | 10 | vfx_hiku002_010 | 0x17E0 / 0x1720 | V 0x1F08 / 1014 |
| lak_l0_air01 | 5 | 0x21F4 | 1 / 5 | 11 | vfx_hiku002_011 | 0x1840 / 0x1750 | V 0x1F08 / 1014 |
| lak_l0_air01 | 6 | 0x2218 | 1 / 6 | 12 | vfx_hiku002_012 | 0x18A0 / 0x1780 | V 0x1F08 / 1014 |
| lak_l0_air01 | 7 | 0x223C | 1 / 7 | 24 | sdef_teikoku_hikutei | 0x18D0 / 0x16F0 | S 0x1EE4 / 707 |

B means declared `BaseObjects/BG/BGPartsBaseObject` (record size 0x54), V means
`BaseObjects/VFX/VFXBaseObject` (record size 0x28), and S means
`BaseObjects/SE/SEBaseObject` (record size 0x24). In Coerthas, ordinal 6 is the sound
member and ordinal 7 is the third VFX member. Ul'dah's body keys are 2, 6, 5.
Neither actor order nor the truncated body label establishes those joins.

The decoder checked all five complete file sizes/hashes before interpreting
any payload, authenticated the five embedded SCBs, and followed the exact
placement pointer through original declared reference shapes, counts and
extents with EOF and cycle guards. It read 40 bounded CACT records across
12,256 SCB bytes and reused the existing clip classification. Declaration
framing follows `xivl-tools:docs/formats/world-layout.md`, Node declarations,
Declared shapes and Fields consumed by the zone reader. The local version-1
decoder SHA-256 is `8bee37afa37578312b2efe9e4ce53bb626959a0c83586aa2a2b72b3197419a21`.
Focused positive and negative checks covered context, bounds, cycles,
missing keys and duplicate preservation.

Confidence is high for these static identities. An actual retained U/L table,
successful callback lookup and result T remain unobserved. The declarations
do not bind A to E/T, identify the concrete +0x44/+0x48 receivers, establish
polarity or recursive body/VFX/sound effects, or close the server selector and
lifecycle edges. No additional receiver construction/table/caller identity
follows from a declared BGParts, VFX or SE class alone.

### Exact folder and conditional creation context

Each of the five exact fleet placements has one direct pointer membership
in `blk_0000`, serialized GID 600. The folder's declared class chain is
`RefObjects/SharedFolder/DivideMap/DivideMapFolderObject` ->
`RefObjects/SharedFolder/SharedFolderObject` -> `Root/RootObject`.
This is a folder-list pointer join, independent of names or spatial proximity.
All offsets below are physical DAT offsets. Indices are zero-based.

| Layout | LYB base | Fleet placement / GID | Direct folder | Folder array / count / selected index | Selected array slot -> wrapper -> placement |
| --- | --- | --- | --- | --- | --- |
| `sea_s0_air01` | `0x830` | `0x3A50` / 935 | `0x47E0` | `0x3D04` / 5 / 1 | `0x3D08` -> `0x4854` -> `0x3A50` |
| `roc_r0_air01` | `0x3F0` | `0x20A0` / 934 | `0x2650` | `0x21F0` / 3 / 0 | `0x21F0` -> `0x26C0` -> `0x20A0` |
| `fst_f0_air01` | `0x830` | `0x3A10` / 937 | `0x47A0` | `0x3CC4` / 5 / 1 | `0x3CC8` -> `0x4814` -> `0x3A10` |
| `wil_w0_air01` | `0x830` | `0x3A70` / 935 | `0x47E0` | `0x3D24` / 5 / 1 | `0x3D28` -> `0x4854` -> `0x3A70` |
| `lak_l0_air01` | `0x270` | `0x1AA0` / 934 | `0x1F60` | `0x1BF0` / 3 / 0 | `0x1BF0` -> `0x1FD0` -> `0x1AA0` |

Each selected Instance byte `+0x14` is `0x05`, so its automatic creation
bit 1 is clear. Each owning DivideMap folder byte `+0x62` is `0x00`, so its
resident bit 0 is clear. The four binary32 lanes at each folder's `+0x30`
and `+0x40` are all exactly `0x00000000` (positive zero). The first three
lanes supply each serialized low/high corner; the fourth lane is preserved
without assigning it a coordinate meaning. The low/high field starts are
respectively `0x4810/0x4820`, `0x2680/0x2690`, `0x47D0/0x47E0`,
`0x4810/0x4820`, and `0x1F90/0x1FA0` in table order.

Under the published [folder registration](layout-block-activation.md#folder-registration)
contract, all five therefore take deferred AABB registration. The separate
[automatic Instance path](bgchip-model-binding.md#unittree-child-creation-and-admission-source)
does not create these placements from their clear bit 1. If selection,
allocation and state guards let the block reach its
[state-1 callback](layout-block-activation.md#state-1-placement-callback),
that callback can create the missing exact Instance without testing bit 1.
This establishes conditional deferred creation for every row. It does not
establish immediate creation or that any block actually reached that state.

The existing resource-parent rows above remain candidate region-submission
contexts. [Region construction](map-layout-selector.md#normal-construction-and-name-resolution)
submits eligible child rows without reading a zone value. Classification
`0x4000..0x4002` then uses the
[radius query](layout-block-streaming.md#per-frame-entry), subject to that
page's manager and scene guards. These serialized zero corners do not prove
universal coverage: the loader derives an expanded AABB, and the query uses
scene `+0x1D0` plus the manager radius. The latter uses manager-root
`+0x3C` or the published absence fallback, then retained scene multiplier
`+0x338`. Neither the selected
manager-root identity nor a retained multiplier was established here, so no
numeric runtime radius follows. Submitted resources, folder registration
and constructor eligibility establish neither ready instances nor resource
completion, playback, body/VFX/sound effects or authenticated server zones.

The producing method was Python 3.12.0 standard-library `struct` and
SHA-256 on Windows 11. Every complete input passed the size/hash pins above
before any field interpretation. Declaration framing followed
`xivl-tools:docs/formats/world-layout.md`, sections "Node declarations",
"Objects and classes" and "Graph cross-checks", at revision
`e33378e5f2b429f49ff21a9450e787664227adca`, document SHA-256
`3bdb36c1592b4f30e54658e22e931927844280003c520fbb196c34c2a608ab85`.
The parse checked each class-node parent chain, the exact `0x40` Instance
and `0x70` DivideMap descriptor sets, and `0x04` list/slot nodes with
descriptor `(0,0)`. Each list stayed inside its declared extent and the
same input file. All inventory SharedFolder-derived lists were checked for
the exact five pointers, preserving duplicate matches; only the direct
owning contexts are reported.

The focused decoder `parse_load_context.py` had SHA-256
`3ec0dffbff59a05f6f1b9054a0e18184ad6b43bd5f63f36cc7aaf01d17901b0f`.
The verifier `verify_load_context.py` had SHA-256
`0a7fec94bacbcb55758621855a243a8d657ca073bbc9d0f26023340da1c9dcd7`.
A separate direct-byte declaration/membership replay agreed for all five
inputs. In-memory controls rejected malformed size/hash, count, wrapper and
declaration framing, excluded null and nearby nonidentical target pointers,
and preserved duplicate membership. Confidence is high for these static
bytes and pointer joins; the loading interpretation consumes the published
native contracts without new native analysis or client execution.

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

## Direct fleet clock consumer

VA `0x00627180` directly joins the placed fleet to timeline playback.
It selects the first stored layout whose `+0x150` classification is
`0x4000` through `0x00626940`, looks up kind 7 and literal `emp_stt0`
(`0x00FB9860`) through layout `+0xE0`, and accepts only placement owners
`isgrp_000002`, `isgrp_000003`, or `isgrp_000005`. The owner pointer table
is at `0x012B82DC`; its strings are at `0x00FB9850`, `0x00FB9840`, and
`0x00FB9830`. All five placements in the DAT table match these names.
The selector itself does not test layout readiness.

At `0x00627301..0x00627333`, instance vtable `+0xBC` supplies the timeline
property and its secondary interface vtable `+0x78` supplies the manipulator.
The constructor at `0x00A78180` authenticates
`SQEX::CDev::Engine::Lay::Default::External::Cut::Scheduler::LaySchedulerManipulator`,
with secondary vtable `0x0109F484`. Its `+0x10` method (`0x00A77D60`)
clears flag bits with mask `0xF9` and invokes child cleanup. Its `+0x0C`
method (`0x00A77D10`) forwards the first argument to child vtable `+0x84`,
calls child `+0x2C`, then dispatches manipulator `+0x20`. The child phase
setter at `0x00A1B8B0` stores the argument at `+0x1CC` and clears three
evaluation fields. This supports a reset followed by start/seek at a phase,
with high confidence. The second argument `-1` bypasses a positive-only
branch in `0x00A77D10`; it is not a fleet enable value.

The phase comes from the unsigned 64-bit clock at `WeatherManager+0x108`,
returned by `0x007E0450`. RTTI at vtable `0x00FF05CC` and constructor
`0x007E8AE0` authenticate that owner. Native CRT helpers at
`0x009D5880`, `0x009D9970`, and `0x009D9AB0` derive
`floor(clock / 60) % 1440`. The signed boundary table at `0x00FBAC28` is
`[-180, 180, 540, 900, 1260, 1620]`, forming six-clock-hour windows.
The initial cursor at `MapLayoutActor+0x330` selects the current window.
After a successful dispatch, the cursor advances and wraps to 1 at 5.
Subsequent dispatches occur within windows beginning at 03:00, 09:00,
15:00, and 21:00, rather than requiring a call at those exact times.

For nonnegative minute offset `delta` within a window, the native phase is
the truncation of `float32(floor(delta * 4200000 / 1440) / 1000) * 300000`.
The relevant instructions are `0x0062722E..0x00627245` and
`0x00627353..0x00627384`. These clock phases and the authored SCB values
describe client playback. They do not establish historical retail activation
dates or a separate server policy.

| Control or lifecycle boundary | Retail observation / locator | Remaining limit |
| --- | --- | --- |
| Scope | First stored classification `0x4000` layout, `emp_stt0`, and the three placement names above | No per-player or independent server Boolean established |
| Entry gate | `0x00627186` rejects `MapLayoutActor+0x1F0 == 3`; null `+0x158` also exits | Meaning of state 3 unresolved |
| Readiness | Caller `0x00629890` requires any one of `+0x17C/+0x180/+0x184`, nonnull `+0x164`, and `0x00623E20` success before call at `0x006298E2` | Readiness is for the selected main/other layout, not proof that every fleet target is ready |
| Initialization | Constructor `0x0062D520` writes cursor 0 at `0x0062D7C1`; it creates the clock owner and supplies clock 36000 through `0x007E1600` | Actual login ordering and later synchronization unresolved |
| Reset | `0x0062D1C0` writes cursor 0 at `0x0062D403`; scene operation `0x14` in `0x0062CDA0` writes it at `0x0062CE22` | Resource-miss/requeue paths bypass the first write; ordinary transfer and seamless crossing are not separately authenticated |
| Retry | Cursor selection precedes object/owner/manipulator lookup; advancement follows the dispatch at `0x00627384` | A nonzero cursor retries only in its selected window; failure in initial interval 0 leaves the zero sentinel and permits another window search |
| Clock production | `0x007E1600` writes the shared clock; scene operation `0x64` reaches it at `0x0062CEBF`. Operation `0x66` reaches alternate writer `0x007E1820`. Per-frame `0x007E91D0` calls clock advancement `0x007E0BC0` | Incoming wire field and complete clock override ordering unresolved |
| Exact values | SCB body show/hide words are 1/0; the direct fleet path supplies phase and `-1` to playback | No producer enable/disable command values established |

These observations were read directly from the pinned executable with Python
3.12, Capstone 5.0.7, and pefile 2024.8.26. The native constants, CRT return
registers, cursor writes, RTTI, and vtable destinations were checked against
the bytes. No decompiled function body is retained here.

## Independent suppression boundary

The generic named visibility helper has an authenticated scene producer.
`Application::Scene::Cut::Clip::RaptureBgShowHideClip`, vtable
`0x0100226C`, dispatches VA `0x00816B50`; its call at `0x00816C7F` reaches
`0x00617000`, then `0x00616530`, the named resolution helpers above, and
`0x00629F20`. These instructions and RTTI were checked with the same native
tools and executable pin. They establish a scene clip path, not a wire opcode
or fleet policy binding.

### Clip data producer

The clip supplies resource-backed names and a Boolean at its execution entry.
At `0x00816BD6`, clip instance
`+0x14` supplies the current data record. Its u32 fields `+0x14/+0x1C`
are indices passed at `0x00816C04/0x00816C35` to `0x00A20080`, then
`0x00A26F00`. The latter adds a signed 16-bit table entry to the table base.
`0x00A26FBE..0x00A26FC1` binds that table from the resource's `String`
chunk, identified by the pointer at `0x01097FAC` to literal `0x00FAB6E8`.
The byte at data `+0x25` is normalized with `SETNE` at `0x00816C56`.
Data `+0x24` selects the resolver branch. Data `+0x18` supplies the kind,
with a conditional `0x11` to `0x12` remap at `0x00816C13..0x00816C2A`.
Data `+0x20` supplies the additional resolver selector.

Registration at `0x00635C23..0x00635C33` associates literal
`RaptureBgShowHideClip` (`0x00FBC914`) with factory `0x00639090`.
That factory calls constructor `0x00816A90` at `0x006390F7`, through
`0x008169B0` to base constructor `0x00A21260`. The base stores the supplied
data pointer at clip instance `+0x14` (`0x00A21275`). Scheduler method
`0x00A1CCF0` also calls `0x00A28420`, which rebinds that field at
`0x00A284B7` to the current record returned by `0x00A29D80`.
`0x00A29D90` advances records by a signed relative 16-bit offset.
These are fields of the selected resource record. In an authored SCB, its
record and `String` table fix the names, selectors and Boolean. No traced
server input replaces those fields independently.
The fleet SCB registry above names the engine `ShowHideClip`. It does not
join that authored playback to the application `RaptureBgShowHideClip`.

The searched producer set comprises that registration, factory, constructor,
record binding and string lookup, the direct caller chain of `0x00617000`
and `0x00616530`, both named resolvers, and the scene-operation dispatcher
`0x0062CDA0`. Catalog-anchored native call inspection found the clip call at
`0x00816C7F` as the only direct caller of `0x00617000` among the
catalog-decoded functions.
This is not an exhaustive indirect-call or script-API search.
The dispatch byte table at `0x0062D0E0`, indexed by operation minus 4,
and pointer table at `0x0062D078` join operation `0x15` to `0x0062CE38`.
It passes a byte and three string spans to `0x00623CF0`, then requests a
layout scheduler through `0x0064C000`. That request has no authenticated
join to a fleet-only `RaptureBgShowHideClip` or its data `+0x25`.
The resource-data entry at `0x00A2519A` is a virtual call through provider
`+0x40`, slot `+0x04`; its selected input was not joined to a server API.
On this traced clip path, the first unresolved producer edge is a
server-reachable selection of an authored record containing the fleet-only
targets and required value.

These producer observations use the executable and native tool versions
above. The direct-call search linearly decoded only function ranges in
`config/ffxivgame.symbols.json` at
`1944d844fbf8bb441aec1224d34b1fa92a538b3a`, then checked the selected
calls, registration pointers and dispatch tables against the executable.
Fresh read-only Ghidra runs `fleet-clip-producer-20261001-02` and
`fleet-clip-data-20261001-02` used the same committed
`DecompileToText.java` digest and runner as the earlier runs, Ghidra 12.1.3,
JDK `21.0.12+8`, memory `8G`, and analysis timeout `0`. Both completed
without an analysis timeout or selected-function failure. Their respective
`DECOMP_VAS` sets were:

- `0x00816B50,0x00816A20,0x00816CE0,0x00816CF0,0x0080FA00,0x00A21040,0x00A21220,0x00617000,0x00616530,0x0062BC40,0x0062A170,0x0062CDA0`
- `0x00635760,0x00639090,0x00816A90,0x008169B0,0x00A21260,0x00A20080,0x00A26F00,0x00A26F70,0x00A25170,0x00A22C30,0x00A23DD0,0x00A25FA0,0x00A26260,0x00A26910,0x00A2A7D0,0x00A2A8A0,0x00A02360,0x00A01540`

The current-record rebinding at `0x00A284B7` was checked by native
disassembly. Resolver argument forwarding follows the native stack reads,
not the decompiler's incomplete inferred signatures. The exports and
temporary projects remain local-only.

### Instance flag consumer

At instance vtable `+0x98`, VA `0x00A99DB0` takes the low byte of its first
argument, compares it with bit 1 at instance `+0x2A`, updates only that bit,
and notifies receivers at `+0x44/+0x48`. Inputs 0 and 1 clear and set
that bit. Direct stack tracing joins normalized clip data `+0x25` to that
argument. Clip data `+0x24` selects the named resolution branch instead.
The wrapper forwards its stack arguments 2 through 7, and each resolver
passes its original argument 2 as `0x00629F20` argument 3. This is an exact
generic flag boundary, with high confidence.

The catalog places that setter at slot 38 of both `RaptureLayoutInstanceObject`
vtable `0x00FF279C` and engine `LayoutInstanceObject` vtable `0x010A4024`.
On a change, it calls the receiver stored at instance `+0x44` through vtable
`+0x1D0` at `0x00A99DDF`, then the receiver at `+0x48` through `+0x9C`
at `0x00A99DF0`, without passing the value as an explicit argument.
The getter at `0x00A99DA0` returns bit 1. The separate setter at `0x00A99D50`
changes bit 0, not bit 1. The first unresolved downstream edge is the concrete
fleet receiver of those notifications and its render/audio consumer. The
inspected setter and getter do not establish hide/show polarity, descendant
coverage, or a sound mute.

The searched consumer set comprised these instance methods and their vtable
references, the named resolution/fanout methods, layout lookup and traversal,
and selected body/VFX/sound clip entries. `0x00629F20` calls each resolved
element's `+0x98`; this is vector fanout, not proof of recursive group effects.
The inspected engine `ShowHideClip` entry at `0x00DE8130`, slot 1 of
vtable `0x01134304`, resolves a dynamic receiver at `0x00DE8147` and calls
its `+0x24` at `0x00DE818D` or `+0x20` at `0x00DE81B9`.
It does not directly call instance `+0x98`. The
[fallback listener path](showhide-clip-dispatch.md#fallback-operations-and-indexed-actor-branches)
conditionally joins those calls through a member actor to the same setter;
the selected fleet receiver and rendered effect remain unbound.
Application `LayVFXClip` (`0x010380E4`, entry `0x00833400`) and
`LaySEClip` (`0x01038EAC`, entry `0x00833E90`) each call a resolved
target's `+0x98` with literal 1 at `0x008334E7/0x00833F5A`.
The engine clip classes have separate RTTI/vtables. Neither the fleet SCB
registry-to-constructor mapping nor these resolved targets was authenticated
to those application classes and `0x00A99DB0`. Consequently those literal-1
calls do not prove that the fleet's clock-driven playback overwrites bit 1.
They also do not prove that clearing it suppresses the complete body/VFX/sound
family. No verified server operation supplies a fleet-only name and value
through this path, so suppression initialization, reset and reapplication
ordering remain unresolved separately for login, ordinary transfer and
seamless crossing.

Fresh read-only runs `fleet-instance-effect-20261001-02/decompile` and
`fleet-instance-effect-20261001-02/callers` used the pinned executable,
the same Ghidra/JDK versions and memory above, and analysis timeout `2700`.
Both completed without a timeout or selected-function failure. The committed
`DecompileToText.java` digest is given above; `FindCallers.java` had SHA-256
`b553bdef091a047a6ec1639724fd5cf94775d5be05b5263bc9662dbf9071614f`.
Both selected this VA set for decompilation or reference lookup:

- `0x00A99C00,0x00A99D50,0x00A99DA0,0x00A99DB0,0x00629F20,0x0062A170,0x0062BC40,0x0062A1F0,0x0062A250,0x00DE8130,0x00833400,0x00833E90,0x00A20070,0x00A1FFA0,0x00A20F30,0x00624B30,0x00626710,0x00629E40`

Native disassembly checked the flag writes and virtual call sites; catalog
RTTI/vtable rows checked the application/engine identities. The reference
export found only the two vtable data references to `0x00A99DB0`; it does not
enumerate polymorphic calls or prove the absence of another writer. Raw
exports and temporary projects remain local-only.

The clock path is client-owned and targets the fleet timeline independently
of SetDalamud. Changing its shared clock would also change other clock users.
Hiding the entire air layout would couple the separate airship trees listed
above. Neither action satisfies an isolated fleet switch. The available
evidence supports the fleet group as a candidate boundary, but does not yet
establish a server-only switch or prove that a client change is necessary.

### Conditional constructed member and property

The [constructed member-target finding](showhide-clip-dispatch.md#constructed-member-target-and-bgparts-property)
at revision `7ab9456ffe4b15d2de474630d8c203af393826be` narrows the
conditional target types. Its child factory
`0x006410A0 -> 0x007FB770` returns complete+4 with table `0x00FF279C`.
When that original creation/registration path supplies the retained key
table and lookup result, it therefore supplies the Rapture instance table
used above. The static fleet key matches do not demonstrate those runtime
conditions or authenticate the CATT factory path.

For a selected BGParts category-2 base, the published property factory
`0x008382A0 -> 0x007FD210` returns complete+`0x4C` with table
`0x00FF2534`, stored as T+`0x14`. Property slot 12,
`0x00A97C40`, forwards through nonnull property+`0x0C` to that receiver's
slot 37. The constructor clears that optional pointer; its later concrete
writer and receiver remain unbound. This is a conditional property path,
separate from the flag-change notifications at T+`0x44/+0x48`.
The constructed Rapture binding override does not directly store those notification
fields, and the base constructor initially clears them. Substituting the
engine binding method that does write them would join the wrong table.

The [matching unit-target branch](showhide-clip-dispatch.md#unit-actor-with-a-matching-unitinfo-key)
also identifies conditional UnitInfo flag/fanout operations and member
slot-73 forwarding through the optional property. It does not identify the
fleet's actual unit key, member/child vector entries or their retained
tables. The ordinal-0 CACT raw value is not a proven UnitInfo scalar join.
Neither conditional path supplies rendered polarity, complete descendant
or audio effects, or an independent server producer.

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

The following chain separates the client mechanisms from the missing
independent control. An operation number, resource name or flag value is
usable only after its producer, object domain and effect are joined.

| Causal link | Established evidence | Missing evidence |
| --- | --- | --- |
| Server input to operation or authored selection | Scene operation `0x15` supplies a scheduler request; operations `0x64/0x66` reach shared clock writers | An attributable packet/script producer selecting a fleet-only target and independent value. The shared clock is not such a value. |
| Authored record to named target | The application clip reads names, kinds, selectors and a normalized Boolean from its current resource record | A selected fleet-only application record, its actual selection path and retained target context. The engine fleet SCB and application clip are distinct. |
| Exact fleet resource to constructed instance | The five placed chains, declaration-backed member-key joins and qualified live Ul'dah bindings identify the resources and static child candidates | Actual selected runtime instance, operation receiver and retained construction/lookup identity for an independent control |
| Value to state change | The generic instance method changes bit 1 and notifies its two stored receivers; the fleet clock path resets/starts/seeks its timeline | A fleet-bound disable/enable interpretation. Bit clear/set is not render or audio polarity. |
| State change to presentation | The fleet SCBs contain body, VFX and sound actors; a conditional BGParts property forwarder is identified separately from flag notifications | Actual fleet notification receivers and the property's later receiver producer, rendered polarity, descendant effects, independently scheduled VFX/sound and interaction with playback |
| Presentation state across arrival and reset | The direct clock consumer has the initialization, resource reset, scene reset, readiness and retry conditions listed above | An independent suppression state, its lifetime and its ordered reapplication for each actual arrival/reset owner |

No authenticated operation/target/value tuple can yet be supplied as an
independent server contract. The first unbound edge on the traced application
path is server-reachable selection of a fleet-only authored record. The first
unbound downstream effect is the concrete fleet instance's notification
receiver and its render/audio consequence. Establishing one does not close
the other. Failure to establish the server path does not prove that a client
change is required.

### Suppression and restoration are separate requirements

For a caller that needs an independent enable/disable control, these are
requirements to prove, rather than recovered operation semantics:

| Requested result | Required control evidence | Current boundary |
| --- | --- | --- |
| Suppress an initially visible or already playing fleet | Fleet-only target, proven hide/mute polarity, complete body/VFX/sound coverage and handling of independently active effects | Neither omission of an enable operation nor input 0 to the generic setter proves this result. Stopping a scheduler alone does not prove cleanup or hiding of its current outputs. |
| Permit authored playback after suppression | Proven release or restoration operation, with the appropriate current scheduler state and no permanent forced-visible override | Input 1 to the generic setter is not a proven restore command. Reset/start/seek is established playback behavior, but its release relationship to an independent suppression is unbound. |
| Preserve the selected result after later playback | Identified competing writers or a proven precedence rule and reapplication boundary | The application VFX/sound literal-1 calls are not authenticated to the selected fleet targets. They establish neither an overwrite nor protection from one. |
| Preserve unrelated presentation | Fleet-only receiver/instance scope with transport scenes, other airship trees, unrelated members, weather, seasonal content and meteor controls excluded | The whole air layout and shared clock are broader than the fleet. Neither is an isolated suppression boundary. |

The evidence therefore does not establish whether suppressing an already
playing fleet and restoring a previously suppressed fleet use the same
operation, opposite values, different operations or different ordering.
The authored body words and generic bit inputs cannot decide that question.

### Arrival, readiness and reapplication

| Lifecycle case | Available static or retained fact | Required evidence before a suppression guarantee |
| --- | --- | --- |
| First login | Constructor initializes the fleet cursor and clock; the caller checks a selected layout's readiness | Join actual login/layout creation to the selected fleet instance and control readiness, then prove suppression of an initially visible fleet. |
| Ordinary transfer | Resource-transition reset can clear the cursor, subject to the documented bypass paths | Bind the actual transfer owner, retained/recreated fleet state and ordered suppression application. |
| Seamless crossing | Resource submission and per-frame block residency are separate; the generic resource reset is not a seamless-specific trace | Bind the crossing's layout/instance reuse or replacement and its own readiness/reapplication boundary. |
| Re-entry with retained state | Loaded resource identity and the historical Ul'dah member join are available; instance pointers are session-local | Identify retained suppression/playback state and prove both false-after-true and true-after-false transitions. |
| Layout reconstruction | The static creation contracts constrain when a placement can be constructed | Bind replacement instance/context identity and any loss of the independent state. Reacquire targets after unload. |
| Later clock or scheduler reset | Direct fleet clock dispatch and the documented cursor reset paths can request playback | Join the independent control to those actual writers and establish precedence or an ordered reapplication point. |

No numeric delay, resend interval or packet ordering follows from these
facts. The selected clock consumer's readiness check does not prove readiness
of every fleet target, nor does resource submission prove construction or
visible residency.

### Verification cases for a future control

These cases are unexecuted acceptance requirements, not observed behavior:

1. Exercise each authenticated operation/value against all five fleet
   resource/placement mappings, with an unaffected-layout control.
2. Suppress an initially visible fleet and an already playing fleet, checking
   all bodies, VFX and sound rather than body visibility alone.
3. Release prior suppression and permit the current authored playback,
   including authored hiding at its normal phase.
4. Repeat both transitions through first login, ordinary arrival, seamless
   crossing, retained-state re-entry, reconstruction and each proven later
   reset owner, using its authenticated readiness boundary.
5. Verify that the shared clock, sibling airship trees, transport/cutscene
   actors, unrelated layout members, weather, seasonal content and Dalamud
   retain their original behavior.

No historical retail activation schedule follows from these static sources.
The live observation above does not establish which activation occurred in
that session. The offline consumer establishes a retail activation mechanism.

## Activation probe boundary

The initial probe used an authorized Ul'dah session. The fleet appeared
before its start call was captured. A conditional hardware execution
breakpoint at VA `0x0064C000` produced no matching fleet request during the
retained observation. This candidate path was not established as exhaustive;
the non-hit proves neither automatic playback nor absence of a server update.
No activation stack or exact SCB runtime instance was obtained. The debugger
was detached and the client remained running.

The offline findings above do not require another runtime probe. The traced
application-clip candidate still needs a concrete fleet-only authored record
and a server-reachable selection path; neither is authenticated here. The
historical non-hit does not close that edge.
