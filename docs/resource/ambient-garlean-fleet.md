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
do not bind A to E/T or demonstrate the selected fleet's receiver/property
admission, recursive body/VFX/sound effects, server selector or lifecycle.
The constructed notification and BGParts contracts below supply conditional
native mechanisms; declared BGParts, VFX or SE names alone do not select them.

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
changes bit 0, not bit 1. The constructed notification contract below binds
the manager/block receivers and their later current-value propagation.
Actual fleet receiver/property admission, complete descendant coverage and
audio effects remain separate from this generic flag operation.

The searched consumer set comprised these instance methods and their vtable
references, the named resolution/fanout methods, layout lookup and traversal,
and selected body/VFX/sound clip entries. `0x00629F20` calls each resolved
element's `+0x98`; this is vector fanout, not proof of recursive group effects.
The inspected engine `ShowHideClip` entry at `0x00DE8130`, slot 1 of
vtable `0x01134304`, resolves a dynamic receiver at `0x00DE8147` and calls
its `+0x24` at `0x00DE818D` or `+0x20` at `0x00DE81B9`.
It does not directly call instance `+0x98`. The
[fallback listener path](showhide-clip-dispatch.md#fallback-operations-and-indexed-actor-branches)
conditionally joins those calls through a member actor to the same setter.
The constructed BGParts path below supplies guarded transform polarity;
the selected fleet's executed path and final rendering remain unobserved.
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

### Conditional constructed member, notifications and BGParts

The [constructed member-target finding](showhide-clip-dispatch.md#constructed-member-target-and-bgparts-property)
at revision `7ab9456ffe4b15d2de474630d8c203af393826be` narrows the
conditional target types. Its child factory
`0x006410A0 -> 0x007FB770` returns complete+4 with table `0x00FF279C`.
When that original creation/registration path supplies the retained key
table and lookup result, it therefore supplies the Rapture instance table
used above. The static fleet key matches do not demonstrate those runtime
conditions or authenticate the CATT factory path.

The [notification and display finding](bgparts-display-flag.md)
at revision `c913db63dcc854656e4757f31006b7f7d431e0e9` binds the
remaining constructed notification path. Here T is complete+4. Root descriptor
initialization `0x00A8D0B0 -> 0x007FE900` supplies manager M at
T+`0x44` and optional block B at T+`0x48`; child creation `0x007FD830`
inherits them. Their callbacks `0x00A8B0D0/0x00A937A0` set update
bits rather than immediately rendering. The manager's constructed SharedFolder
traversal and the block's state-6 consumer later call T slot 73,
`0x00A9A040`, with 1. That argument propagates T's current flag to its
property and child fanout; it is not an unconditional show. Selected fleet
tables, arrays, state and successful updates still need their own admission.

For a selected BGParts category-2 base, the property factory
`0x008382A0 -> 0x007FD210` returns complete+`0x4C` with table
`0x00FF2534`, stored as T+`0x14`. The normal nonbatch setup
`0x008D3FD0 -> 0x00AAA920 -> 0x00A98460` obtains a key-zero
display-manager product and stores its model interface at P+`0x0C`.
The registered factory supplies LayDefaultModelObject table `0x00FBAD6C`.
P slot 12, `0x00A97C40`, forwards to that model's slot 37,
`0x00626250`; P slot 47, `0x00A982E0`, releases and clears the pointer.
These are separate from T's flag-change notification fields.

On the guarded indexed-model branch, normalized 1 preserves the first three
finite Float4 transform rows, while 0 zeros them; both retain the fourth row.
The path writes the indexed drawable state and marks it dirty. The nonindexed
resource branch is separate. Early batch registration does not itself create
P+`0x0C`; the separately admitted model setup consumes the registration index
and reused backend only under its documented guards. Zero can be the first
append index, not registration failure. Successful standalone initialization
samples the current parent/member flags, but indexed-resource and nonzero
reuse branches bypass that particular sample. This conditional transform
polarity does not demonstrate an actual fleet frame, all-branch hiding,
VFX/sound mute, or persistence across reconstruction and playback.

The [matching unit-target branch](showhide-clip-dispatch.md#unit-actor-with-a-matching-unitinfo-key)
also identifies conditional UnitInfo flag/fanout operations and member
slot-73 forwarding through the optional property. It does not identify the
fleet's actual unit key, member/child vector entries or their retained
tables. The ordinal-0 CACT raw value is not a proven UnitInfo scalar join.
The constructed contracts supply the guarded BGParts transform result above.
They do not establish complete fleet descendant/audio effects or an independent
server producer.

### Conditional VFX property and backend production

The [retained VFX setup](limsa-rain-presentation.md#later-weather-placement-and-vfx-setup)
binds the kind-`0x0F` base path `0x008D4FA0 -> 0x00AB5010` to
property slot 34, given both retained resource pointers, instance admission
and the property guard. This is a generic VFX contract; it does not select
the fleet's actual instance or equate fleet resources with rain.

Creator table `0x0103C574` slot 3, original cell `0x0103C580`,
selects `0x0083A010`. Its nonnull `0xF0`-byte allocation reaches
constructor `0x007FDDB0` at `0x0083A06D`. That constructor installs
property table `0x00FF2E84` at complete+`0x4C` at `0x007FDE1D`;
the factory returns this adjusted interface P, otherwise null. The preceding
RTTI locator `0x01164224` independently reports offset `0x4C`.
Creator slot 7, `0x00AA7190`, performs case-sensitive byte equality
through NUL against `BaseObjects/VFX/VFXBaseObject`, loaded through
`0x012F5904 -> 0x010A29E4`, and returns equality in AL with `RET 4`.
The factory/constructor bridge, rather than a matching name, identifies P.

P slot 34, `0x00AA4170`, takes the two retained resource pointers A/B
and setup operand C. It adjusts `this` to Q=P-4=complete+`0x48` and
calls Q slot 1 at `0x00AA4193` with five stack arguments
`(A,B,0,C,0xFFFFFFFF)`, then returns `RET 0x0C`. The final argument is
a by-value DWORD. The constructor installs Q table `0x00FF2F48`;
its slot-1 cell `0x00FF2F4C` is `0x00AA47A0`.

The latter method takes `(A,B,O,C,I)`. With nonnull A/B and selected O=0,
it calls P slot 35, obtains R through P slot 4 and H through R slot 17.
Global `0x01365B08` slot 5 returns F; F slot 3 supplies optional E.
E slot 1 receives a pointer to the three-DWORD record
`{A,0,R.slot6()}` and must return true in AL. B is otherwise only checked
for nonnull in this complete method. After E slot 27 receives P, R slot 37
supplies AL, and `0x00AA486D` stores E at Q+`0x40`=P+`0x3C`=
complete+`0x88`. E slot 4 then receives a DWORD with that AL as its low
byte, `[H+4]`, C and I. Its upper three bytes are not established as zero.
True AL returns success. Null E or failed E slot 1/4 invokes P slot 35
and returns false; that implementation and post-failure field state
remain unjoined. The separate O!=0 branch writes through O and calls
`0x00AA4D30`; the retained three-argument wrapper selects O=0.

P slot 12, `0x00AA4140`, tests the optional backend at P+`0x3C`.
Null returns `RET 4`; nonnull tail-calls E slot 15 with the original single
stack argument unchanged. The tested DWORD is a pointer, not that argument.
This method interprets no flag polarity. The complete constructor does not
itself initialize complete+`0x88`; base-helper effects and later lifetime
writers remain separate requirements.

The [default external-manager construction](bgchip-instance-resource-setup.md#separate-attribute-physics-path)
binds `0x01365B08` to wrapper `0x010A8AC0` and its installed connector
to `0x010A026C`. Original wrapper slot-5 cell `0x010A8AD4` selects
`0x00ABB940`, which tail-calls the retained connector's slot 2 without a
null guard. Connector cell `0x010A0274` selects `0x00A84190`, returning
`[0x0136576C]+4` when nonnull, otherwise zero. Default installation,
allocation and replacement conditions remain essential. These getters do
not themselves construct the returned manager.

The retained startup producer supplies that manager: `0x00A77B8F` calls
constructor `0x00A83F90` after a `0x0C`-byte allocation and stores its
result or null in `0x0136576C` at `0x00A77BC0`. The constructor installs
table `0x010A0214` at complete+4. Its original RTTI locator `0x0118ACA0`
reports offset 4 and identifies LayEffectManager. F slot-3 cell
`0x010A0220` selects `0x00A83F70`: null F+4 returns null; otherwise
it tail-calls that retained secondary interface's slot 1. The constructor
installs secondary table `0x010A0234` at its allocation+4 and stores the
adjusted pointer at manager complete+8=F+4. Failure of this secondary
allocation can therefore leave a nonnull manager that returns no E.

Secondary slot-1 cell `0x010A0238` selects `0x00A84050`, which allocates
`0x2C` bytes, calls `0x00A87AB0` at `0x00A8409F` and returns a nonnull
result+4, otherwise zero. That constructor installs E table `0x010A1ABC`
at complete+4. Original RTTI locator `0x0118C140` reports offset 4 and
identifies LayEffectManipulator. Its slot-15 cell `0x010A1AF8` selects
`0x00A87790`, binding the current-value receiver through this default
construction path.

That method reads J at E+`0x0C`=complete+`0x10`, initially zero in the
constructor. Null J returns `RET 4` without an effect. With nonnull J,
a nonzero argument low byte calls J slot 12 with the original DWORD.
A zero low byte first calls J slot 42: true AL skips slot 12; false AL
calls it with the same original DWORD. Thus even this concrete forwarding
has a guard specific to zero; it is not a direct Boolean-to-render operation.

The two E setup calls above have original cells `0x010A1AC0/0x010A1ACC`
selecting `0x00A87480/0x00A87510`. The first consumes the three-DWORD
record and, after calling E slot 2 and its allocation path, calls `0x00A87EC0`
at `0x00A874E5`. It stores the returned W at E+8=complete+`0x0C`
and returns whether W is nonnull. The second calls E slot 5, requires W,
and tests its first argument's low byte. Nonzero calls `0x00B99E40`
at `0x00A87546` with W as `this` and the second setup DWORD (`[H+4]`)
as its argument, then stores the return as J. A null J returns false.

Zero instead jumps at `0x00A8753F` to `0x00A875C6`, skipping J creation,
the optional callback and the J slot-33/52/51 block. Both branches call
K=`u32(E+0x20)` slot 39 with C, without a K null guard. Zero then also
skips J slot 54 and E slot 6. It can return true without dereferencing J;
successful setup alone therefore does not establish a nonnull effect J.
The nonzero branch's later J calls and optional global `0x01365780`
callback have separate resource, value and index guards. These operations
do not establish the concrete J/K tables or their effect meanings.

Setup also requires valid R/H and manager F: `0x00AA47A0` adds no null
guards for R, F before slot 3, or H before `[H+4]`. Default-manager
construction does not remove those prerequisites.

#### W construction and resource-selected J production

`0x00A87EC0` calls base constructor `0x00B99D00`, then installs W table
`0x010A1A70` at `0x00A87EE6`. Its original locator `0x0118C054`
reports complete-object offset zero and identifies LayEffectManipulatorLeaf.
The constructor copies the three-DWORD record to W+`0x18/0x1C/0x20`,
calls `0x00B99D50` with A and stores A at W+`0x14`. The complete base
and setter chain, through `0x00BC32C0/0x00BC3280`, leaves W+`0x0C`=0,
W+`0x10`=A and W+8=`[A+0x54]` on normal return. This identifies W;
it does not identify J or prove preservation through later calls.

`0x00B99E40` prioritizes nonnull W+`0x10`, calling `0x00B97D30` at
`0x00B99E52`; only a null value there admits W+`0x0C` and
`0x00B9E980` at `0x00B99E68`. Both pass `(D,W)`, with D the setup
operand `[H+4]`. Each loads Z from its selected resource receiver+`0x54`;
null Z returns zero. The first calls `0x00BC0BA0`, the second
`0x00BCA850`, then both pass their product S through
`0x00B95C70 -> 0x00B95B60`. The both-null W branch selects zero only
if its unexamined helper calls return normally.

The selected resource lookups and lower product calls are:

| Branch | Lookup N, with creation argument zero | Nonnull N product call |
| --- | --- | --- |
| Primary, `D & 0x000F0000 = 0x00040000` | `0x00BBFE30` at `0x00BC0BF4` | `0x00BDF6A0` at `0x00BC0C35` |
| Primary, mask value `0x00060000` | `0x00BC0560` at `0x00BC0C74` | `0x00BDF2C0` at `0x00BC0C99` |
| Fallback | `0x00BC9DA0` at `0x00BCA896` | `0x00BD7920` at `0x00BCA8D7` |

These complete lookups check resource-state bits and use
`0x00417510 -> 0x00416AE0` on Z's `+0xC4/+0xC8/+0xA0` containers,
respectively. A found result T supplies N=`[T+4]`. Their selected creation
argument zero skips the visible on-demand population arms. A loaded layout,
found T or successful earlier W setup therefore does not establish a valid
N or an effect product. `0x00416AE0` itself relies on the container's slot-7
index/found result and configured stride; its body adds no index bound.

For the first and fallback product calls, N slot 2 must return true, and
N slot 4 returns context X; the dynamic factory is Fd=`[X+0x110]`.
A resource byte selects mode 0/1, then Fd slot 1 receives `(mode,N,W,0)`
and returns S. The group product instead requires N+4 bit 0 and treats N+0
as an owner pointer rather than a vptr, obtains Fd=`[[N]+0x110]`, and calls
its slot 2 with
`(2,[N+0x10],[N+0x0C],W,0)`. None of these bodies supplies an Fd null
guard or authenticates its concrete table.

The adapter returns zero for null S. Otherwise S slot 3 selects the
interface adjustment: kind 0 returns S+`0x204`, kind 1 S+`0x21C`, and
kind 2 S+`0xC4`. Other values reach unexamined helpers before the visible
zero-return instructions. These kind-dependent offsets do not authenticate
the returned J table. Selecting J slots 42/12 by a class name, a kind value
or a matching interface offset would bypass the required construction bridge.

The conditional population arms supply further producer constraints.
`0x00BBFB10` and `0x00BC9BB0` allocate N and call
`0x00BDF810/0x00BD7BC0`, which install tables `0x010CD7CC/0x010C8EE4`.
Their common `0x00BDF110 -> 0x00BDEEE0` path stores N+8=X, with X
obtained from a parent resource's slot 4, and N+`0x0C`=that parent.
The installed slot-4 cells select the
[shared `0x007143E0` getter](bgchip-model-binding.md#unittree-child-creation-and-admission-source),
which reads `this+8`. Direct initial stores do not prove preservation
through unexamined resource initialization and callbacks. Group constructor
`0x00BDF450` instead stores its owner at N+0 and builds a counted array;
that field is not a table pointer. The population callers pass `(D,N)` to
`0x004174B0 -> 0x00416A00 -> 0x00416810`. These bodies establish
conditional configured key/value copying, but not the actual container
configuration or selected cache contents.

The root-parent arm is also conditional: `0x00BC9DA0` calls
`0x00BC8C30` at `0x00BC9E60`, then uses the returned V's `+0x84`
pointer as the parent supplied to `0x00BC9BB0`. `0x00BC8C30` passes
a copied sixteen-byte identifier, tag DWORD `0x76656666` and third argument
1 to `0x00BBD220` at `0x00BC8C71`. The latter calls `0x00E3D890`
at `0x00BBD23D` on `[Z+0x50]`, then obtains V from its result+`0x14`.
`0x00E3D890` copies the identifier and tag to a twenty-byte global key and
calls the same `0x00416AE0` helper, returning T unchanged. The concrete
container slot-7 behavior remains unbound. A null T reaches unexamined
helpers before the subsequent dereference if control returns; a safe miss
return is not established. Argument 1 skips the separate `[Z+0x40]`
slot-3 path. That separate path obtains Rv from the dynamic receiver and
passes it to `0x00E3D8D0`, which prepares the same key and calls
`0x00416BF0` with pointers to the key and Rv. The latter helper's body and
transitive effects remain untraced. A bounded original `E8 rel32` search
for `0x00E3D8D0` found only this already checked call at `0x00BBD2EF`;
it did not enumerate indirect callers. The constructed-resource path below supplies a conditional concrete V
product, parent context, factory and J-table bridge. Actual selected cache
values and their later retention remain unobserved.

The [constructed-resource bridge](#constructed-resources-parent-context-and-effect-factory)
binds the callback receiver, conditional V/parent/context factory and actual
S/J interfaces. The [J operation result](#j-operations-and-conditional-bit-polarity)
separates the recovered state-bit polarity from the missing presentation
consumer. Actual fleet selection/cache identity, default query context,
separate K/SE construction, cleanup and lifecycle still need their own joins.

#### Constructed resources, parent context and effect factory

The baseline graph route `0x007CBE90 -> 0x00A63D10` supplies actual
factory registrations. The base constructor installs table `0x00FE942C`
on nonnull helper storage, pairs it with key `0x76656666` and calls the
[published registration wrapper](gtex-pwib-loader.md#selected-restxb-consumer)
`0x0060DAC0`. Its original RTTI identifies EffectResourceNodeFactory.
The same constructor registers tables `0x00FE93CC/0x00FE93AC` with keys
`0x6C656166/0x76696E73`, corroborated as LeafResourceNodeFactory and
LeafinsResourceNodeFactory. Actual fleet resource admission and successful
later lookup remain separate conditions.

The effect factory slots 1/3 construct through `0x00B99950/0x00B99A00`.
Slot 1 writes the returned node through its output argument and returns
that output address; slot 3 returns the node directly. Guarded allocator,
allocation and supplied-context paths do not establish successful parsing.
The instance factory's products `0x00B981E0/0x00B98280` construct A's
backend Za through `0x00B97EF0 -> 0x00BC16D0`, storing it at A+`0x54`.

Shared setup `0x00BBC830` stores the supplied A+`0x4C` interface at
Za+`0x40`. That interface's installed table `0x010B6C10`, complete-object
offset `0x4C`, selects slot 3 `0x00B932B0`. The method calls slot 4 on
H=`u32(A+0x74)` with the copied key and identifier. A null node returns
zero; a nonnull node supplies its `+0x54` without a backend null check.
The effect node independently selects the same code through its own
interface table `0x010B73C4`; this does not unify the two node objects.

The effect node constructs Ve through `0x00B99830 -> 0x00BC2B30`.
Its guarded post-construction slot 14 reaches `0x00BC30D0 -> 0x00BC2EC0`.
The lower helper `0x00BC2C40` obtains X=`u32(root+0x6C)`, constructs
Ve+`0x80`, and writes Ve+`0x84` using `0x00BD7B40` or fallback
`0x00BD7AD0`. These parent constructors install table `0x010C8EE4`;
the published slot-4 getter reads current `this+8`. Their bases
`0x00BDF0A0/0x00BDEFC0` initially bind Q+8 to X. Later opaque resource
helpers and callbacks prevent an unconditional claim that Q+8 survives.
Allocation, null-resource and global-error branches can leave or clear
Ve+`0x84`. `0x00BC2EC0` ignores the lower helper's AL before returning
true, so outer success does not establish successful parent construction.
Ve's own slots 40/42 belong to `0x010C318C` and are distinct from J's.

The root at global `0x01367F44` is constructed through
`0x00A4C0B0 -> 0x00A4BAE0`; its base initially zeros root+`0x6C`.
The selected SceneThreadImpl startup route later reaches
`0x00A4ADA0 -> 0x00B94A20(root,world)` and root slot 2. The constructed
root table `0x0109B254` selects `0x00B948C0` for that slot. If root+`0x70`
is already nonnull, it skips initialization. Otherwise it allocates M,
calls `0x00B9A600(M,root)`, stores the result at root+`0x70`, then calls
`0x00BAB7C0(root,M)`. A nonnull M is stored unchanged at root+`0x6C`;
the null-input branch has opaque diagnostics before using the existing
field. This does not guarantee safe failure or initialization of reused state.

`0x00B9A600` installs table `0x010B5DAC` on a four-byte allocation and
stores that pointer, or allocation-failure zero, at M+`0x110`. Its original
RTTI corroborates EffectFactory with complete-object offset zero. Thus the
initial parent-context chain reaches a constructed Fd on this module branch,
subject to valid backing objects and retained fields/tables through later
opaque calls. The product calls add no Fd null guard.

The actual Fd slot-1 method `0x00B95A60` forwards accepted modes 0/1 to
`0x00B9C4B0/0x00B9D5E0` with the retained caller vector `(0,0,N,W)`.
Slot 2 `0x00B95AF0` accepts mode 2 and calls `0x00B9E3C0` with
`(0,array,count,W)`. Both return the lower EAX unchanged. Invalid modes
reach opaque diagnostic helpers before their visible zero return.

H can come from a supplied context. The default in
`0x00B94020/0x00B93B80` uses `u32(M+0x10C)-8` after guarded root/module
loads. Its actual query receiver remains a separate binding from Fd;
M's base initially zeros `+0x10C`. No missing-pointer branch is treated as
safely returning if an unexamined helper precedes a later dereference.

#### J operations and conditional bit polarity

The factory's mode-specific products supply the previously missing interface
identity. Constructors `0x00B9C420/0x00B9D550/0x00B9E340` install the final
primary and J tables after their base calls, with no later constructor call.
The actually selected S slot-3 cells use the published constant bodies
`0x00AB7340/0x007254A0/0x00B8D680`, returning 0/1/2. The
[zero-return contract](bitmap-font-source-request.md) and
[one/two-return contracts](../actor/mesh-matrix-input.md) were consumed
without reopening those bodies. This authenticates the adapter adjustment:

| Factory mode | S primary table / slot-3 kind | J offset / final table | J slot 12 | J slot 42 |
| --- | --- | --- | --- | --- |
| 0 | `0x010B8484` / 0 | `+0x204` / `0x010B8394` | `0x00B9BB20` | `0x00B9C680` |
| 1 | `0x010B892C` / 1 | `+0x21C` / `0x010B883C` | `0x00B9CC50` | `0x00B9D820` |
| 2, group | `0x010B8D8C` / 2 | `+0xC4` / `0x010B8C8C` | `0x00B9DA60` | `0x00B9E750` |

The three J locators report exactly those complete-object offsets. Their
UnconformableEffect/Effect/GroupEffect names corroborate the selected stores;
the names did not select a product. Each operation is an eleven-byte thunk
that subtracts its exact J offset from ECX and preserves the stack argument.
Slot 12 reaches `0x00BB5200` on either single product, or `0x00BC74B0`
on the group. Slot 42 reaches `0x00B9BDA0/0x00B9CED0/0x00B9DCE0`.
Later callbacks, child append/setup and global-error cleanup constrain which
constructed tables and current children remain selected.

Single-product slot 12 requires byte S+`0x1C` mask `0x01` set and DWORD
S+`0x18` mask `0x04000000` clear. It passes `(inputLowByte==0)` to
`0x00BB4C10` with mask `0x100`. Only DL is assigned by the caller, but
the helper consumes only the argument's low byte, so stale upper EDX bits
do not change the result. It sets WORD S+`0x1B2` mask `0x100` for input
low byte zero and clears that mask for nonzero. The helper's recomputation
and callback branch tests `mask & 0x84`; it is skipped for this `0x100`
call. No immediate render/hide callback follows from this selected helper.

Group slot 12 requires S+8 mask `0x01`. It forwards the original DWORD to
slot 14 on each current nonnull child in the S+`0x18` array, with WORD count
at S+`0x0C`. The constructed single-child primary tables select the same
`0x00BB5200` method at slot 14. Afterwards group S+8 mask `0x10` is set
for a nonzero input low byte and cleared for zero. Predicate and setter
calls can observe different current arrays/counts after opaque callbacks.

Single-product slot 42 reaches `0x00BB38E0`. It calls
`0x00BCD2A0(u32(S+0x14),-3,0)`, then `0x00BDA530` on the returned
record. The selected signed-negative branch scans `0x24`-byte entries from
resource `+0x64`, comparing `u16(u32(entry+4)+0x14)` with `0xFFFD`.
Its count source calls `0x00BD13A0/0x00BD2420`; argument two is
unused on this branch. `0x00BDA530` initializes descriptor DWORD 4 and
value byte zero, calls `0x00BDA3E0`, then returns the descriptor byte in AL
regardless of helper status. For runtime record B, the value reader follows
`G=u32(B+4)`, `C=u32(G+8)`, `T=u32(C+8)` and calls `u32(T+0xA8)` with
`(u32(B), &descriptor)`. The type-4 value is not converted locally.
There is no null-record guard, and a null callback reaches opaque diagnostics
before reloading and calling the current pointer if they return. A missing
record or callback therefore does not establish safe false. The callback's
fallback descriptor/context producer and callback are bound below; the
versioned resource's type-name input remains conditional.

Group slot 42 reaches `0x00BC7A30`, returning true in AL when any current
nonnull child's primary slot 42 returns nonzero AL, and false AL for no such
child. The constructed single primary tables select `0x00B9BDA0/0x00B9CED0`
for those calls, joining the same single-product predicate. No group mask
`0x10` guard is used by this predicate. Upper EAX is not a normalized Boolean.

Thus the current-value path has this conditional, byte-level result:

| Current input | Dispatch condition | Single-product WORD S+`0x1B2` mask `0x100` |
| --- | --- | --- |
| Low byte nonzero | J slot 12 is called | Cleared if its state guards pass |
| Low byte zero | J slot 42 returns zero AL | Set if slot 12's state guards pass |
| Low byte zero | J slot 42 returns nonzero AL | Slot 12 is skipped; the prior bit remains |

The group additionally has its own guard, child guards and current-array
conditions. Setup with low byte zero still skips J creation and direct J
calls; it is not the current-value zero row above. This establishes bit
polarity, not a hide/mute or suppression/release interpretation. A concrete
consumer of WORD S+`0x1B2` mask `0x100` must join that state to presentation.
Actual fleet property admission, default H/cache contents, retained context,
SE construction, already-playing cleanup and arrival/reset behavior remain
separate unbound edges. No server operation/target/value contract follows.


#### J42 resource descriptor and type selection

The single-product base constructors `0x00BC5C80/0x00BC6C00` call
`0x00BB6D20`. It obtains N slot 7, calls
`0x00BCDDA0(S,0,N7,0,0)`, and stores the returned resource R at S+`0x14`.
The actual N slot-7 cells select `0x00BDF7F0/0x007CC7E0`: these return
current N+`0x24`, with the former falling through current N+`0x0C` slot 7
when its own field is null. The resource base `0x00BCD8C0` stores R+0=S,
R+8=N7 and R+`0x64`=the `0x24`-byte record array. Each record has payload DWORD at
`+0`, header G at `+4` and R at `+8`, installed by `0x00BDB9D0`.
Allocation, error and later opaque helper paths constrain successful
construction and retained identity. The zero constructor argument does not
skip the early `0x00BD1390` header-word diagnostic check; it skips later
per-record validation and registration through that argument.

The descriptor-pointer array is `u32(u32(N7+0x10)+0x0C)`, through the exact
getters `0x00BD13A0/0x00BD2430`. G+`0x14` is the signed 16-bit record key.
`0x00BD20C0` installs G+8=C, and `0x00BE7480` installs C+8=T from
`u32(M+4)+u16(syntheticRecord+0x1C)*0xE8`. That synthetic type index comes
from a successful type lookup's `T+0xE4`; it is distinct from G's negative
record key. Thus the callback at T+`0xA8` belongs to a module type record,
rather than to the resource's primary vtable.

Parent Q+`0x24` has two producer paths in `0x00BD7A50`. The fallback
`0x00BDDDB0 -> 0x00BD2F00` supplies three eight-byte input pairs:

| Signed record key | Type-name pointer | Exact NUL-terminated lookup key |
| --- | --- | --- |
| -3 | `0x010BA124` | `SQEX/CDev/Engine/Vfx/QixControl/Controls/LeafLife` |
| -1 | `0x010BA080` | `SQEX/CDev/Engine/Vfx/QixControl/Controls/MatrixLeaf` |
| -2 | `0x010BA048` | `SQEX/CDev/Engine/Vfx/QixControl/Controls/ColorRGBALeaf` |

The names establish lookup identity. Cache hits and later resource callbacks
remain conditions on the current object. New construction passes through
`0x00BD1C20 -> 0x00BD13B0`, then conditionally `0x00BD18B0`; the outer
constructor returns its allocation independently of the lower AL result.

The versioned alternative `0x00BDDE10 -> 0x00BD3010 -> 0x00BE7580`
uses the two current resource headers at Q+`0x30/+0x34`. For each positive
signed count, it reads an eight-byte entry's signed key at `+4` and unsigned
type index at `+6`, then copies the type-name pointer from the indexed
`0x18`-byte type record's `+0x10`. `0x00BD1C90` and its three-DWORD table
map -3/-1/-2 to output slots 0/1/2; other keys use an increasing ordinal
starting at 3. The original key and resource-provided name pointer survive.
Key -3 therefore does not force the LeafLife name on this path. No selected
fleet header payload was read. For positive counts the converter returns
the count's low byte in AL; nonpositive counts bypass that reload and return
the original EAX. `0x00BD3010` forwards a DWORD with only AL initialized,
while `0x00BD2F00` consumes only its low byte. Neither count validation nor
safe behavior for malformed inputs follows.

#### Fallback callback registry binding

On the constructed-module startup branch, `0x00B94A20` calls
`0x00BAECB0(M,0x010B9DA8)` at `0x00B94D9C`. Scene startup later calls
the same wrapper with `0x0111CBB8` at `0x004E888C`. The wrapper calls
`0x00BADE00` to append `0xE8`-byte type records to M+4, update count M+`0x14`
and rebuild the registry at M+`0x114`:

| Configuration | Type source / count / half-open extent | Secondary source / count |
| --- | --- | --- |
| `0x010B9DA8` | `0x01300178` / 56 / `[0x01300178,0x01303438)` | `0x010B9D30` / 5 |
| `0x0111CBB8` | `0x0130F4E8` / 162 / `[0x0130F4E8,0x013187B8)` | `0x0111CAF8` / 8 |

The secondary inputs go through `0x00BACCE0` into a distinct `0x18`-byte
array at M+8. That helper resolves existing type keys; it does not directly
append types. Their payloads were not read. These two authenticated direct
wrapper callsites do not enumerate indirect callers or prove all runtime
registrations. Reuse, allocation, opaque calls, source backing and retained
fields remain conditions on this construction composition.

`0x00BD7650` constructs registry table `0x010C8848` with hash selector
`0x01000193` at `+0x18`. Actual `0x00BD7750` therefore calls `0x00E3F9D0`:
start at `0x01000193`, then multiply by `0x01000193` and XOR each unsigned
nonzero key byte, with unsigned 32-bit wrap. Registry slot 7 `0x00416CC0`
uses slot 8 `0x00BD7730` to compare unsigned DWORD hashes, with a separate
found byte and index result. Equality is hash equality, not pointer or string
equality. Lookup `0x00BAC8F0 -> 0x00BD7810` uses that constructed registry;
the published `0x00416AE0/0x00416A00` lookup/insertion bodies were consumed
without reopening them. Capacity and nullable-construction paths do not
guarantee successful registration.

All 218 selected type-key pointers and their section-bounded NUL byte
sequences were checked, with no null, truncated or unbacked key. The sole
match for LeafLife hash `0x9B11363B` was earlier source index 15, record
`0x01300F10`, whose key pointer is `0x010BA124`. Its exact callback DWORD
at `0x01300FB8` is `0x00BA45A0`. The later 162-record append had no equal
hash. This binds the fallback key to that copied callback under the actual
registry/index/backing and retention conditions; it does not bind every
versioned key -3 header or establish the selected fleet's current resource.

The complete callback `0x00BA45A0` selects descriptor DWORD 4 through its
bounded switch. It writes descriptor byte `+4` as follows, before running
its common continuation:

| Payload before continuation | Written descriptor byte |
| --- | --- |
| Ordered `0.0 < float(payload+0x10) < 1.0` | 1 |
| Otherwise, byte `payload+0x1C` is 3 or 7 | 1 |
| Otherwise | 0 |

Unordered comparisons take the state-byte fallback if they complete normally.
The [published fixed-byte table](../actor/camera-endpoint-calculation.md)
supplies the exact zero/one operands. This is a value query with a continuation:
the callback then calls `0x00BA4370(payload,0,0)`. That helper can change
payload state and overwrites `payload+0x10` from `0x00BA42A0`'s XMM0 result.
The descriptor records the inputs before that continuation; it is not a
second query of its final payload state. No descriptor pointer is explicitly
passed to the continuation. Valid backing, native calls returning normally
and retained resource/registry identity remain conditions on composition.

The owner helper `0x00E3A460` follows `u32(payload)` to the runtime record,
then record+8 to R, then `u32(R)` to full S. `0x00BDB9D0` installs the payload
backlink at `0x00BDBA14`; later helpers must preserve it. `0x00BB3BD0` writes
DWORD S+`0xC4` into the continuation's original first stack-argument slot,
which is reloaded into EBP. ESI retains the payload pointer; neither later
zero argument is that destination. With payload+`0x1D` bit 0 clear,
states 6/7 call `0x00BB35D0(S,1)`,
which tests S+`0x28` bit 0. On nonzero AL, state 7 also calls
`0x00BB35E0(S,1)`, which
clears that bit when S+`0x1C` bit 0 permits it. Those are different fields
from S+`0x1B2` mask `0x100`. The
[published mode-1 LeafLife boundary](../actor/model-state-color-transitions.md)
was consumed for `0x00BA42A0`; its body and other modes were not reopened.
State 5 can call actual S primary slot 16 with argument 1 when
`payload+0x1D` bit 0 is clear and the signed DWORD at `payload+0x14` is
at most the signed DWORD at S+`0xC4`.
Both single tables select `0x00BB4290`. Under S+`0x1C` bit 0, it sets
S+`0x28` bit 0. Only a clear-to-set transition additionally ORs `0x2000`
into DWORDs S+`0x1C/+0x18` and zeros S+`0xC4`. It has no calls or
access to S+`0x1B2` mask `0x100`. The continuation then marks its payload
state before evaluating the float result. Other modes of the consumed
evaluator and opaque helpers are not assumed to preserve payload state.

The extra setup call J slot 54 has a separate contract. Its actual adjustors
`0x00B9BE40/0x00B9CF70/0x00B9DD80` reach `0x00BB3D20/0x00BC7B90`.
The single method searches key -2 and sends descriptor DWORD `0x0C` plus
the original value through `0x00BD9EE0 -> 0x00BD9C60` to that record's
T+`0xA8` callback. The group uses only its first child's primary slot 69,
without an array/count guard; the constructed single tables select the same
single method. This is a different key/type input from the fallback -3
predicate and supplies no authenticated consumer of S+`0x1B2` mask `0x100`.

The current-value zero input therefore first evaluates the selected
resource predicate, which can itself update payload state, and calls J12
only for zero AL. The proven guarded setter still sets mask `0x100` for
that zero call and clears it for a nonzero low byte. The fallback callback
does not make that mask a proven hide/mute or suppression/release command.
One finite query for exact direct TEST encodings of WORD `+0x1B2` mask
`0x100`, or its equivalent BYTE test, returned no candidate. Other
encodings, aggregate and indirect consumers were outside that query; its
result is not an absence or exhaustion claim.

The precise remaining presentation edge is an authenticated consumer of
that S field on the selected fleet's admitted resource/property path. The
versioned predicate also needs its actual header type-name input. The other
modes of `0x00BA42A0` remain outside the consumed published contract. Current
cache/registry contents, default H installation, retained fields/tables,
SE effects, already-playing cleanup, fleet-only server selection and
arrival/reset precedence remain separate gates. No additional resource
payload or runtime probe was used to bridge them.

#### VFX source checks

Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 x86-32 produced these
original-PE observations using the binary/build identity above. The checked
control-flow instruction unions were complete for these fifteen entries;
internal branches were followed, with callee bodies checked only at listed entries.
All intervals below are half-open VAs in raw-backed PE sections.

| Entry | Complete instruction interval | Bytes |
| --- | --- | --- |
| `0x0083A010` | `[0x0083A010,0x0083A0A7)` | 151 |
| `0x00AA7190` | `[0x00AA7190,0x00AA71DB)` | 75 |
| `0x007FDDB0` | `[0x007FDDB0,0x007FDEC3)` | 275 |
| `0x00AA4170` | `[0x00AA4170,0x00AA4198)` | 40 |
| `0x00AA4140` | `[0x00AA4140,0x00AA4153)` | 19 |
| `0x00AA47A0` | `[0x00AA47A0,0x00AA48D7)` | 311 |
| `0x00ABB940` | `[0x00ABB940,0x00ABB94A)` | 10 |
| `0x00A84190` | `[0x00A84190,0x00A841A0)` | 16 |
| `0x00A83F90` | `[0x00A83F90,0x00A84036)` | 166 |
| `0x00A83F70` | `[0x00A83F70,0x00A83F83)` | 19 |
| `0x00A84050` | `[0x00A84050,0x00A840CB)` | 123 |
| `0x00A87AB0` | `[0x00A87AB0,0x00A87AF4)` | 68 |
| `0x00A87790` | `[0x00A87790,0x00A877C3)` | 51 |
| `0x00A87480` | `[0x00A87480,0x00A87509)` | 137 |
| `0x00A87510` | `[0x00A87510,0x00A87614)` | 260 |

The 1,721 instruction bytes and the separate 89-byte startup producer window
`[0x00A77B6D,0x00A77BC6)` were checked against the original PE and the
retained startup listing. The startup result is limited to that window.
Checks also authenticated eighteen exact method cells, five selected
table/locator/type-name chains, class-ID cell `0x010A2CD8 = 0x0A5D` and
the matcher pointer/literal. Table identity, object adjustments, native stack
vectors and low-byte results were checked together. No inheritance-array,
registration or string census supplied a substitute construction edge.
Confidence is high for these static instructions and conditional domains;
no client execution, selected fleet setup or rendered effect was observed.

#### Resource-selected VFX source checks

The same original PE identity and Python 3.12.0, pefile 2024.8.26 and
Capstone 5.0.7 x86-32 produced the following additional observations.
These 38 complete normal-flow instruction unions total 5,779 bytes.
Calls were followed only at the separately listed entries; their inclusion
does not prove other callees preserve state or return normally. All
intervals are half-open original VAs in raw-backed sections.

| Entry | Complete instruction union | Bytes |
| --- | --- | --- |
| `0x00416810` | `[0x00416810,0x00416875)` | 101 |
| `0x00416A00` | `[0x00416A00,0x00416AD7)` | 215 |
| `0x00416AE0` | `[0x00416AE0,0x00416B27)` | 71 |
| `0x004174B0` | `[0x004174B0,0x004174D4)` | 36 |
| `0x00417510` | `[0x00417510,0x0041751D)` | 13 |
| `0x00A87EC0` | `[0x00A87EC0,0x00A87F28)` | 104 |
| `0x00B95B60` | `[0x00B95B60,0x00B95BDF)` | 127 |
| `0x00B95C70` | `[0x00B95C70,0x00B95C75)` | 5 |
| `0x00B97D30` | `[0x00B97D30,0x00B97D57)` | 39 |
| `0x00B99D00` | `[0x00B99D00,0x00B99D12)` | 18 |
| `0x00B99D50` | `[0x00B99D50,0x00B99D6A)` | 26 |
| `0x00B99E40` | `[0x00B99E40,0x00B99EA4)` | 100 |
| `0x00B9E980` | `[0x00B9E980,0x00B9E9A7)` | 39 |
| `0x00BBD220` | `[0x00BBD220,0x00BBD2FD)` | 221 |
| `0x00BBFB10` | `[0x00BBFB10,0x00BBFD4C)` | 572 |
| `0x00BBFE30` | `[0x00BBFE30,0x00BBFEFC)` | 204 |
| `0x00BBFF40` | `[0x00BBFF40,0x00BC013D)` + `[0x00BC0140,0x00BC01C5)` | 642 |
| `0x00BC0560` | `[0x00BC0560,0x00BC0628)` | 200 |
| `0x00BC0BA0` | `[0x00BC0BA0,0x00BC0CD6)` | 310 |
| `0x00BC3280` | `[0x00BC3280,0x00BC3291)` | 17 |
| `0x00BC32C0` | `[0x00BC32C0,0x00BC32D1)` | 17 |
| `0x00BC8C30` | `[0x00BC8C30,0x00BC8CB4)` | 132 |
| `0x00BC96D0` | `[0x00BC96D0,0x00BC9740)` | 112 |
| `0x00BC9BB0` | `[0x00BC9BB0,0x00BC9D99)` | 489 |
| `0x00BC9DA0` | `[0x00BC9DA0,0x00BC9ECC)` | 300 |
| `0x00BCA850` | `[0x00BCA850,0x00BCA92B)` | 219 |
| `0x00BD7920` | `[0x00BD7920,0x00BD79C7)` | 167 |
| `0x00BD7BC0` | `[0x00BD7BC0,0x00BD7C3B)` | 123 |
| `0x00BDDD50` | `[0x00BDDD50,0x00BDDD7B)` | 43 |
| `0x00BDEEE0` | `[0x00BDEEE0,0x00BDEFB2)` | 210 |
| `0x00BDF110` | `[0x00BDF110,0x00BDF181)` | 113 |
| `0x00BDF2C0` | `[0x00BDF2C0,0x00BDF322)` | 98 |
| `0x00BDF450` | `[0x00BDF450,0x00BDF528)` + `[0x00BDF530,0x00BDF54C)` | 244 |
| `0x00BDF6A0` | `[0x00BDF6A0,0x00BDF74D)` | 173 |
| `0x00BDF810` | `[0x00BDF810,0x00BDF89C)` | 140 |
| `0x00BDF8A0` | `[0x00BDF8A0,0x00BDF8AA)` | 10 |
| `0x00E3D890` | `[0x00E3D890,0x00E3D8CE)` | 62 |
| `0x00E3D8D0` | `[0x00E3D8D0,0x00E3D913)` | 67 |

The unions preserve the three-byte gap in `0x00BBFF40` and eight-byte
gap in `0x00BDF450`. The copy helper `0x00416810` has a separate
four-DWORD jump table at `[0x00416878,0x00416888)`: the preceding
unsigned `(EDX-1)<=3` bound selects `0x00416824`, `0x0041682A`,
`0x00416836` and `0x00416848`. Those cases and the generic copy loop are
included in its complete code union. Data cells are not counted as code.

Original checks also authenticated the three selected W/N
table/locator/type-name chains and five N-method cells. N tables
`0x010CD7CC/0x010C8EE4` have zero complete-object offsets and identify
`Qix::Data::Leafins/Leaf`. Exact cells `0x010CD7D4/0x010CD7DC/0x010CD7E0`
and `0x010C8EEC/0x010C8EF4` select the checked methods: slot 2 selects
`0x00BDDD50`, slot 4 selects
the published `0x007143E0` contract, and the first table slot 5 selects
`0x00BDF8A0`. The shared slot-4 body was consumed from the linked
finding rather than reopened. The separate construction study below supplies
the J tables and their slot-42/12 cells. Confidence is high for the static
instructions and conditional pointer domains; actual cache contents,
selected fleet resources, retained tables and rendered polarity were
not observed.

#### Constructed VFX source checks

The same pinned original PE, Python 3.12.0, pefile 2024.8.26 and Capstone
5.0.7 x86-32 supplied 106 additional complete normal-flow methods, totalling
23,277 instruction bytes. These are separate from the preceding fifteen- and
38-method studies, which were consumed without replay. Original-byte,
section/file-offset, CFG, ABI and independent source checks covered the
following half-open instruction unions; callees were checked only at listed
entries. Exception-handler bodies were not expanded. The unchanged REA import
failure supplied no Ghidra body corroboration. No client execution supported
these additional construction/operation claims.

| Entry | Complete instruction union | Bytes |
| --- | --- | --- |
| `0x007CBE90` | `[0x007CBE90,0x007CC652)` | 1986 |
| `0x00A63D10` | `[0x00A63D10,0x00A642E3)` | 1491 |
| `0x00B974A0` | `[0x00B974A0,0x00B9759E)` | 254 |
| `0x00B973D0` | `[0x00B973D0,0x00B9749B)` | 203 |
| `0x00B99950` | `[0x00B99950,0x00B999F5)` | 165 |
| `0x00B99A00` | `[0x00B99A00,0x00B99A9B)` | 155 |
| `0x00B97880` | `[0x00B97880,0x00B9797E)` | 254 |
| `0x00B977B0` | `[0x00B977B0,0x00B9787B)` | 203 |
| `0x00B97690` | `[0x00B97690,0x00B977A4)` | 276 |
| `0x00B975A0` | `[0x00B975A0,0x00B97681)` | 225 |
| `0x00B93AF0` | `[0x00B93AF0,0x00B93B78)` | 136 |
| `0x00B94020` | `[0x00B94020,0x00B9420D)` | 493 |
| `0x00B93B80` | `[0x00B93B80,0x00B93D10)` | 400 |
| `0x00B99830` | `[0x00B99830,0x00B99941)` | 273 |
| `0x00B92CC0` | `[0x00B92CC0,0x00B92CD7)` | 23 |
| `0x00B9EE60` | `[0x00B9EE60,0x00B9EF46)` | 230 |
| `0x00B9EF50` | `[0x00B9EF50,0x00B9F02C)` | 220 |
| `0x00B981E0` | `[0x00B981E0,0x00B9827E)` | 158 |
| `0x00B98280` | `[0x00B98280,0x00B98314)` | 148 |
| `0x00BC2B30` | `[0x00BC2B30,0x00BC2C35)` | 261 |
| `0x00B9EAD0` | `[0x00B9EAD0,0x00B9EBE1)` | 273 |
| `0x00B97EF0` | `[0x00B97EF0,0x00B98001)` | 273 |
| `0x00B9F190` | `[0x00B9F190,0x00B9F238)` | 168 |
| `0x00BC16D0` | `[0x00BC16D0,0x00BC1817)` | 327 |
| `0x00BC2750` | `[0x00BC2750,0x00BC280C)` | 188 |
| `0x00BBC1A0` | `[0x00BBC1A0,0x00BBC2A2)` | 258 |
| `0x00BBC830` | `[0x00BBC830,0x00BBC961)` | 305 |
| `0x00BCAE80` | `[0x00BCAE80,0x00BCAFCB)` | 331 |
| `0x00BC9B40` | `[0x00BC9B40,0x00BC9BA6)` | 102 |
| `0x00BC8E10` | `[0x00BC8E10,0x00BC9199)` | 905 |
| `0x00BC0B10` | `[0x00BC0B10,0x00BC0B93)` | 131 |
| `0x00BBBFB0` | `[0x00BBBFB0,0x00BBBFCD)` | 29 |
| `0x00BAAEA0` | `[0x00BAAEA0,0x00BAAEA6)` | 6 |
| `0x00BCA3B0` | `[0x00BCA3B0,0x00BCA42C)` | 124 |
| `0x00B93350` | `[0x00B93350,0x00B9337B)` | 43 |
| `0x00B932B0` | `[0x00B932B0,0x00B93301)` | 81 |
| `0x00BBBF00` | `[0x00BBBF00,0x00BBBF07)` | 7 |
| `0x00BC28C0` | `[0x00BC28C0,0x00BC29B9)` + `[0x00BC29C0,0x00BC2A1B)` | 340 |
| `0x00B948B0` | `[0x00B948B0,0x00B948B6)` | 6 |
| `0x00BBBBE0` | `[0x00BBBBE0,0x00BBBC1B)` | 59 |
| `0x00BBCCD0` | `[0x00BBCCD0,0x00BBCDF8)` | 296 |
| `0x00BBC7B0` | `[0x00BBC7B0,0x00BBC7C0)` | 16 |
| `0x00A4C0B0` | `[0x00A4C0B0,0x00A4C16A)` | 186 |
| `0x00BAB610` | `[0x00BAB610,0x00BAB7B6)` | 422 |
| `0x00A4BAE0` | `[0x00A4BAE0,0x00A4C0A1)` | 1473 |
| `0x00BBBC20` | `[0x00BBBC20,0x00BBBC69)` | 73 |
| `0x00BBC390` | `[0x00BBC390,0x00BBC46C)` | 220 |
| `0x00B947E0` | `[0x00B947E0,0x00B94838)` | 88 |
| `0x00BC30D0` | `[0x00BC30D0,0x00BC30ED)` | 29 |
| `0x00BABE70` | `[0x00BABE70,0x00BABF26)` | 182 |
| `0x00BBC020` | `[0x00BBC020,0x00BBC049)` | 41 |
| `0x00BC2EC0` | `[0x00BC2EC0,0x00BC2FC2)` | 258 |
| `0x00BABC10` | `[0x00BABC10,0x00BABDB1)` | 417 |
| `0x00BC2C40` | `[0x00BC2C40,0x00BC2EB3)` | 627 |
| `0x00A4C170` | `[0x00A4C170,0x00A4C1BD)` | 77 |
| `0x00BC2810` | `[0x00BC2810,0x00BC28B2)` | 162 |
| `0x00BD7B40` | `[0x00BD7B40,0x00BD7BB5)` | 117 |
| `0x00BD7AD0` | `[0x00BD7AD0,0x00BD7B35)` | 101 |
| `0x00A49810` | `[0x00A49810,0x00A49B0F)` | 767 |
| `0x00BDF0A0` | `[0x00BDF0A0,0x00BDF106)` | 102 |
| `0x00BDEFC0` | `[0x00BDEFC0,0x00BDF098)` | 216 |
| `0x00BD7A50` | `[0x00BD7A50,0x00BD7ACB)` | 123 |
| `0x00A4B630` | `[0x00A4B630,0x00A4B636)` | 6 |
| `0x004E8670` | `[0x004E8670,0x004E8675)` | 5 |
| `0x004E8680` | `[0x004E8680,0x004E8852)` | 466 |
| `0x004E8860` | `[0x004E8860,0x004E886D)` | 13 |
| `0x00A49460` | `[0x00A49460,0x00A49475)` | 21 |
| `0x004E8870` | `[0x004E8870,0x004E88F3)` | 131 |
| `0x00A49430` | `[0x00A49430,0x00A49445)` | 21 |
| `0x00A49450` | `[0x00A49450,0x00A49458)` | 8 |
| `0x00A488B0` | `[0x00A488B0,0x00A488D2)` | 34 |
| `0x00A4ADA0` | `[0x00A4ADA0,0x00A4AEAF)` | 271 |
| `0x00A48A90` | `[0x00A48A90,0x00A48A95)` | 5 |
| `0x00B94A20` | `[0x00B94A20,0x00B94DB6)` | 918 |
| `0x00BAAE60` | `[0x00BAAE60,0x00BAAE74)` | 20 |
| `0x00B948C0` | `[0x00B948C0,0x00B9493B)` | 123 |
| `0x00B9A600` | `[0x00B9A600,0x00B9A73C)` | 316 |
| `0x00BAB7C0` | `[0x00BAB7C0,0x00BAB82F)` | 111 |
| `0x00BC3AA0` | `[0x00BC3AA0,0x00BC3AF7)` | 87 |
| `0x00B95A60` | `[0x00B95A60,0x00B95AED)` | 141 |
| `0x00B95AF0` | `[0x00B95AF0,0x00B95B5F)` | 111 |
| `0x00BAD5D0` | `[0x00BAD5D0,0x00BAD952)` | 898 |
| `0x00B9C4B0` | `[0x00B9C4B0,0x00B9C588)` | 216 |
| `0x00B9D5E0` | `[0x00B9D5E0,0x00B9D6D0)` | 240 |
| `0x00B9E3C0` | `[0x00B9E3C0,0x00B9E50A)` | 330 |
| `0x00BC3980` | `[0x00BC3980,0x00BC3A4F)` | 207 |
| `0x00B9C420` | `[0x00B9C420,0x00B9C4A2)` | 130 |
| `0x00B9D550` | `[0x00B9D550,0x00B9D5D2)` | 130 |
| `0x00B9E340` | `[0x00B9E340,0x00B9E3B3)` | 115 |
| `0x00B9BB20` | `[0x00B9BB20,0x00B9BB2B)` | 11 |
| `0x00B9CC50` | `[0x00B9CC50,0x00B9CC5B)` | 11 |
| `0x00B9DA60` | `[0x00B9DA60,0x00B9DA6B)` | 11 |
| `0x00B9C680` | `[0x00B9C680,0x00B9C68B)` | 11 |
| `0x00B9D820` | `[0x00B9D820,0x00B9D82B)` | 11 |
| `0x00B9E750` | `[0x00B9E750,0x00B9E75B)` | 11 |
| `0x00BB5200` | `[0x00BB5200,0x00BB523E)` | 62 |
| `0x00BC74B0` | `[0x00BC74B0,0x00BC74FA)` | 74 |
| `0x00B9BDA0` | `[0x00B9BDA0,0x00B9BDA5)` | 5 |
| `0x00B9CED0` | `[0x00B9CED0,0x00B9CED5)` | 5 |
| `0x00B9DCE0` | `[0x00B9DCE0,0x00B9DCE5)` | 5 |
| `0x00BB4C10` | `[0x00BB4C10,0x00BB4C74)` | 100 |
| `0x00BB38E0` | `[0x00BB38E0,0x00BB38F6)` | 22 |
| `0x00BC7A30` | `[0x00BC7A30,0x00BC7A6D)` | 61 |
| `0x00BCD2A0` | `[0x00BCD2A0,0x00BCD319)` | 121 |
| `0x00BDA530` | `[0x00BDA530,0x00BDA556)` | 38 |
| `0x00BDA3E0` | `[0x00BDA3E0,0x00BDA46D)` | 141 |

The seven-byte gap `[0x00BC29B9,0x00BC29C0)` is excluded. No selected
method had an unresolved internal normal-flow successor. Thirty-seven method cells
were authenticated independently from the constructor-selected tables:

| Actual table | Selected slots / exact cells |
| --- | --- |
| `0x00FE942C` | 1/3/5: `0x00FE9430/0x00FE9438/0x00FE9440` |
| `0x00FE93CC` | 1/3/5: `0x00FE93D0/0x00FE93D8/0x00FE93E0` |
| `0x00FE93AC` | 1/3/5: `0x00FE93B0/0x00FE93B8/0x00FE93C0` |
| `0x010B6C10` | 1/3: `0x010B6C14/0x010B6C1C` |
| `0x010C318C` | 14/40/42: `0x010C31C4/0x010C322C/0x010C3234` |
| `0x010B73C4` | 3: `0x010B73D0` |
| `0x00F931D4` | 12..17: `0x00F93204/0x00F93208/0x00F9320C/0x00F93210/0x00F93214/0x00F93218` |
| `0x0109B254` | 2: `0x0109B25C` |
| `0x010B5DAC` | 1/2: `0x010B5DB0/0x010B5DB4` |
| `0x010B8484` | 3/14/42: `0x010B8490/0x010B84BC/0x010B852C` |
| `0x010B892C` | 3/14/42: `0x010B8938/0x010B8964/0x010B89D4` |
| `0x010B8D8C` | 3: `0x010B8D98` |
| `0x010B8394` | 12/42: `0x010B83C4/0x010B843C` |
| `0x010B883C` | 12/42: `0x010B886C/0x010B88E4` |
| `0x010B8C8C` | 12/42: `0x010B8CBC/0x010B8D34` |

The ten selected preceding locator/COL/type-descriptor chains were checked
through their section-bounded NUL names, without inheritance arrays:

| Table | COL / type descriptor | Complete-object offset |
| --- | --- | --- |
| `0x00FE942C` | `0x0116194C / 0x012C58A8` | 0 |
| `0x00FE93CC` | `0x01161844 / 0x012C57B8` | 0 |
| `0x00FE93AC` | `0x011617EC / 0x012C5768` | 0 |
| `0x010B6C10` | `0x01194728 / 0x012FF4A0` | `0x4C` |
| `0x010C318C` | `0x0119653C / 0x01303C58` | 0 |
| `0x010B73C4` | `0x01194A1C / 0x012FF610` | `0x4C` |
| `0x010B5DAC` | `0x01194010 / 0x012FEE40` | 0 |
| `0x010B8394` | `0x01195430 / 0x012FFB40` | `0x204` |
| `0x010B883C` | `0x01195528 / 0x012FFCA8` | `0x21C` |
| `0x010B8C8C` | `0x01195634 / 0x012FFD10` | `0xC4` |

Confidence is high for these byte-level construction, adjustment, guarded
state-write and AL-predicate contracts. The descriptor/callback study
below extends these construction checks.

#### Descriptor and callback source checks

The same original executable identity and Python 3.12.0, pefile 2024.8.26
and Capstone 5.0.7 x86-32 supplied 53 further complete normal-flow
methods, totalling 12,493 instruction bytes. These extend the preceding
106-method construction study to 159 methods / 35,770 bytes. Prior fifteen- and
38-method bodies were consumed without replay. Original-byte mapping,
section bounds, complete instruction unions, ABI, typed data and independent
source review covered these half-open original VA intervals. No runtime
execution or new asset payload supplied these claims.

| Entry | Complete instruction union | Bytes |
| --- | --- | --- |
| `0x00BC5C80` | `[0x00BC5C80,0x00BC5CE1)` | 97 |
| `0x00BC6C00` | `[0x00BC6C00,0x00BC6D1B)` | 283 |
| `0x00BB6D20` | `[0x00BB6D20,0x00BB706B)` + `[0x00BB7070,0x00BB7308)` | 1507 |
| `0x00BCDDA0` | `[0x00BCDDA0,0x00BCE534)` | 1940 |
| `0x00B9BE40` | `[0x00B9BE40,0x00B9BE57)` | 23 |
| `0x00B9CF70` | `[0x00B9CF70,0x00B9CF87)` | 23 |
| `0x00B9DD80` | `[0x00B9DD80,0x00B9DD97)` | 23 |
| `0x00BB3D20` | `[0x00BB3D20,0x00BB3D41)` | 33 |
| `0x00BC7B90` | `[0x00BC7B90,0x00BC7BAF)` | 31 |
| `0x00BDF7F0` | `[0x00BDF7F0,0x00BDF802)` | 18 |
| `0x007CC7E0` | `[0x007CC7E0,0x007CC7E4)` | 4 |
| `0x00BCD8C0` | `[0x00BCD8C0,0x00BCDBA6)` | 742 |
| `0x00BDB9D0` | `[0x00BDB9D0,0x00BDBA68)` + `[0x00BDBA70,0x00BDBC85)` | 685 |
| `0x00BD13A0` | `[0x00BD13A0,0x00BD13A4)` | 4 |
| `0x00BD2430` | `[0x00BD2430,0x00BD2434)` | 4 |
| `0x00BDDDB0` | `[0x00BDDDB0,0x00BDDE06)` | 86 |
| `0x00BDDE10` | `[0x00BDDE10,0x00BDDE24)` | 20 |
| `0x00BD9EE0` | `[0x00BD9EE0,0x00BD9F05)` | 37 |
| `0x00BD2F00` | `[0x00BD2F00,0x00BD3009)` | 265 |
| `0x00BD3010` | `[0x00BD3010,0x00BD3055)` | 69 |
| `0x00BD9C60` | `[0x00BD9C60,0x00BD9CED)` | 141 |
| `0x00BD1C20` | `[0x00BD1C20,0x00BD1C60)` | 64 |
| `0x00BD13B0` | `[0x00BD13B0,0x00BD17F8)` | 1096 |
| `0x00BD18B0` | `[0x00BD18B0,0x00BD1A84)` | 468 |
| `0x00BAC8F0` | `[0x00BAC8F0,0x00BAC90F)` | 31 |
| `0x00BAC620` | `[0x00BAC620,0x00BAC66E)` | 78 |
| `0x00BAC670` | `[0x00BAC670,0x00BAC6EF)` | 127 |
| `0x00BD7810` | `[0x00BD7810,0x00BD7831)` | 33 |
| `0x00BD20C0` | `[0x00BD20C0,0x00BD20E7)` | 39 |
| `0x00BD2660` | `[0x00BD2660,0x00BD285A)` + `[0x00BD2860,0x00BD2C0A)` | 1444 |
| `0x00BE7480` | `[0x00BE7480,0x00BE74A5)` | 37 |
| `0x00BD7750` | `[0x00BD7750,0x00BD77C2)` | 114 |
| `0x00BAECB0` | `[0x00BAECB0,0x00BAECDA)` | 42 |
| `0x00BAECE0` | `[0x00BAECE0,0x00BAED67)` | 135 |
| `0x00BADE00` | `[0x00BADE00,0x00BAE09A)` | 666 |
| `0x00BAE0A0` | `[0x00BAE0A0,0x00BAE2F2)` | 594 |
| `0x00BD7650` | `[0x00BD7650,0x00BD7681)` | 49 |
| `0x00BD77D0` | `[0x00BD77D0,0x00BD780B)` | 59 |
| `0x00E3F9D0` | `[0x00E3F9D0,0x00E3F9FF)` | 47 |
| `0x00BACCE0` | `[0x00BACCE0,0x00BACD9C)` | 188 |
| `0x00A4B540` | `[0x00A4B540,0x00A4B56B)` | 43 |
| `0x00BAB0F0` | `[0x00BAB0F0,0x00BAB0FF)` | 15 |
| `0x00416CC0` | `[0x00416CC0,0x00416D7F)` | 191 |
| `0x00BE7580` | `[0x00BE7580,0x00BE75FF)` | 127 |
| `0x00BD7730` | `[0x00BD7730,0x00BD774D)` | 29 |
| `0x00BD1C90` | `[0x00BD1C90,0x00BD1CAB)` | 27 |
| `0x00BA45A0` | `[0x00BA45A0,0x00BA473D)` | 413 |
| `0x00BA4370` | `[0x00BA4370,0x00BA442A)` | 186 |
| `0x00E3A460` | `[0x00E3A460,0x00E3A46C)` | 12 |
| `0x00BB3BD0` | `[0x00BB3BD0,0x00BB3BDF)` | 15 |
| `0x00BB35D0` | `[0x00BB35D0,0x00BB35E0)` | 16 |
| `0x00BB35E0` | `[0x00BB35E0,0x00BB35F2)` | 18 |
| `0x00BB4290` | `[0x00BB4290,0x00BB42C7)` | 55 |

Additional gaps `[0x00BB706B,0x00BB7070)`,
`[0x00BDBA68,0x00BDBA70)` and `[0x00BD285A,0x00BD2860)` are excluded.
The four-case post-initializer switch reads only DWORDs
`[0x00BD1A84,0x00BD1A94)`, under its unsigned byte-selector bound <=3.
The callback's outer unsigned descriptor bound <=5 selects six DWORDs
`[0x00BA4740,0x00BA4758)`; its subtype bound <=4 selects five DWORDs
`[0x00BA4758,0x00BA476C)`. All internal successors are included above;
switch data is excluded from instruction-byte counts.

Eleven additional selected method cells authenticate N slot 7
`0x010CD7E8/0x010C8F00`, J slot 54
`0x010B846C/0x010B8914/0x010B8D64`, single-child primary slot 69
`0x010B8598/0x010B8A40`, single-owner primary slot 16
`0x010B84C4/0x010B896C`, and constructed registry slots 7/8
`0x010C8864/0x010C8868`. No further RTTI/inheritance census supplied identity.
Typed reads also covered the six predefined key-pointer/WORD operands at
`0x01303E24/28/2C` and `0x010C70B4/B8/BC`, their three exact NUL strings,
the two four-DWORD configuration blocks, the three reserved-key DWORDs
`[0x010C70C0,0x010C70CC)`, all 218 selected type-key DWORD fields and their
NUL strings, and the single selected callback DWORD `0x01300FB8`.
Each selected type-key string terminated within 256 bytes in one raw-backed
section; all hash matches were counted. These are finite selected inputs,
not a type, string or callback census.

The additional binary32 operand `[0x01086650,0x01086654)` contains
`00000000`, positive zero.
The original zero/one operands at `0x00F62F70/0x00F54F70` were consumed
from the [fixed-byte table](../actor/camera-endpoint-calculation.md)
without rereading them. Confidence is high for the conditional resource,
registry, callback and state contracts. Actual fleet admission, authored
versioned type inputs and render/audio polarity remain outside these checks.

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
| State change to presentation | Constructed manager/block notifications cause conditional current-value propagation; BGParts has a model producer and guarded indexed transform/drawable update; VFX setup has a conditional constructed resource/parent/factory/J-table bridge and guarded slot-12 mask update, with a bound fallback predicate and owner-state continuation | Selected fleet receiver/property admission and retained cache/context, the versioned J42 type-name input, selected fleet payload/owner binding and a presentation consumer of the slot-12 state bit, SE construction/setup handoff, other model branches, final rendering, complete descendant effects and interaction with playback |
| Presentation state across arrival and reset | The direct clock consumer has the initialization, resource reset, scene reset, readiness and retry conditions listed above | An independent suppression state, its lifetime and its ordered reapplication for each actual arrival/reset owner |

No authenticated operation/target/value tuple can yet be supplied as an
independent server contract. The first unbound edge on the traced application
path is server-reachable selection of a fleet-only authored record. The first
unbound downstream effect is the selected fleet's admitted property/backend
path and complete render/audio consequence, after the conditional notification
and BGParts mechanisms above. Establishing one does not close the other.
Failure to establish the server path does not prove that a client change is
required.

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
