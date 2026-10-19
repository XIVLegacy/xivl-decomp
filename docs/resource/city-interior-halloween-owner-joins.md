# City interior Halloween selector and owner joins

The final 1.23b interiors contain 31 Halloween-named show/hide scheduler
pairs: 10 in Gridania, 14 in Limsa Lominsa and 7 in Ul'dah. All 62
timelines have exact serialized weather-property joins: show selects only
8027, and hide selects its complement among the 23 authored IDs. Every
timeline has one direct UnitTree owner, and both timelines in each pair
share that owner. The owners connect to 50 distinct resident BGParts,
whose resource keys each match one outer `brt` row.

These are 31 named scheduler pairs. Ul'dah `hwa1` has a material-clip
scheduler on its hide side, so the data does not support calling all 31
pairs symmetric `ShowHideClip` bodies.

## Original resources and method

| City / layout | Original DAT | Bytes | SHA-256 | SEDBlyb / payload file base |
| --- | --- | ---: | --- | --- |
| Gridania / 331 | `data/29/B0/00/02.DAT` | 460368 | `f2088d1feba5fe71fbc4c462229f1bd1970371d6f023622a7a83d6aa8d450876` | `0x6280` / `0x62B0` |
| Limsa Lominsa / 131 | `data/29/D9/00/02.DAT` | 728976 | `c6d86bfea2692e612464e7e08f6604df4f26fc1ea99da16727e824c920a693d8` | `0x8940` / `0x8970` |
| Ul'dah / 431 | `data/61/5A/00/03.DAT` | 529616 | `d969e982a55d5c986e7bea24ae5af1bf23bfe7ed18854f7dd7c4f21a11d2353f` | `0x60C0` / `0x60F0` |

All named offsets below are absolute file offsets. A serialized layout
reference P resolves to the city's payload base plus P. SCB offsets marked
as chunk offsets start at the selected embedded SCB; ends are exclusive.
Python `struct` and `xivl-tools:tools/research/census_layout.py`,
`parse_layout`, revision `75fe7414bf3f1ecd779825083d477adc4ed5f319`,
SHA-256 `5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`,
followed the declared type extents and field descriptors. The format
references are `xivl-tools:docs/formats/world-layout.md`, Node declarations,
Declared shapes and Fields consumed by the zone reader, and
`xivl-tools:docs/formats/sedb-res.md`, Resource ID to DAT path.
Only the three original layout DATs were read; resource-table targets were
identified without opening the resident resources.

## Typed selector and owner contract

Each selected TimeLine has size `0x28` and descriptors
`(4,0),(8,2),(0x14,0),(0x1C,0),(0x20,2)`. Its `+0x1C` reference
reaches a `0x18`-byte property named `wprp_0001`, with descriptors
`(4,0),(8,2),(0x10,0)`. Property `+0x10/+0x14` is a reference/count
pair for 23 declared eight-byte vector entries without reference descriptors.
Every selected vector run fits its complete declared extent.

The vector order is 8001-8017, then 8027-8032. The first u32's low
half is the ID. Selected entries have high half `0xFFFF` and second
u32 `0xFFFFFFFF`; unselected entries have both fields zero. Each show
selects 8027 and each hide selects the other 22 IDs.

Each owner is a declared `0x50`-byte UnitTree; each child is the declared
`0x30`-byte five-descriptor child shape. UnitTree `+0x30/+0x34` gives
its direct child run, and child `+0x18` reaches the declared base object.
Gridania's owners have three or four direct children and zero group
reference/count. Ul'dah's owners have three direct children and zero group
reference/count. Every Limsa owner has two direct timeline children and
one separately declared five-descriptor `0x30`-byte group. Its
`+0x20/+0x24` run contains two resident children; its subgroup run is null.
All resident bases are declared `0x54`-byte BGParts. BGParts `+0x20`
references the key string matched to an outer resource-table row.

The declaration table rows below locate the exact shapes. Each row's
`+0/+4` gives its extent-record/descriptor references; each extent record
declares start, count and stride. All these complete extents are bounded
by their original layout payload.

| City | TimeLine / property / vector rows | UnitTree / child / BGParts rows | Group row |
| --- | --- | --- | --- |
| Gridania | `0x8EAC` / `0x8F0C` / `0x8F1C` | `0x8C4C` / `0x8C5C` / `0x8D4C` | none selected |
| Limsa Lominsa | `0xC924` / `0xC984` / `0xC994` | `0xC6D4` / `0xC6E4` / `0xC814` | `0xC754` |
| Ul'dah | `0x92A4` / `0x9304` / `0x9314` | `0x9074` / `0x9084` / `0x91A4` | none selected |

## Pair coverage

Timeline names are `time_bg_<suffix>_show` and `time_bg_<suffix>_hide`.
SCB sizes list show / hide. All 31 rows have one shared direct owner and
exact 8027/complement vectors. Resident counts describe pointer membership,
not the number of objects affected at runtime.

| City | Suffix | UnitTree | Residents | SCB bytes show / hide |
| --- | --- | --- | ---: | --- |
| Gridania | `hwa1` | `0x2A4C0` | 1 | `0x330` / `0x330` |
| Gridania | `hwf1` | `0x2A510` | 2 | `0x370` / `0x370` |
| Gridania | `hwf2` | `0x2A560` | 2 | `0x370` / `0x370` |
| Gridania | `hww1` | `0x2A470` | 2 | `0x370` / `0x370` |
| Gridania | `hww2` | `0x2A420` | 2 | `0x370` / `0x370` |
| Gridania | `hww3` | `0x2A3D0` | 1 | `0x330` / `0x330` |
| Gridania | `hws1` | `0x2A5B0` | 1 | `0x330` / `0x330` |
| Gridania | `hws2` | `0x2A600` | 1 | `0x330` / `0x330` |
| Gridania | `hws3` | `0x2A650` | 2 | `0x370` / `0x370` |
| Gridania | `hws4` | `0x2A6A0` | 1 | `0x330` / `0x330` |
| Limsa Lominsa | `hw_aq1_h` | `0x345A0` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_ast_h` | `0x345F0` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_f1_h` | `0x34640` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_f2_h` | `0x34690` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_f3_h` | `0x346E0` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_hjt_h` | `0x34730` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_kos_h` | `0x34780` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_main_h` | `0x347D0` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_sp1_h` | `0x34820` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_sp2_h` | `0x34870` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_tik_h` | `0x348C0` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_ws1_h` | `0x34910` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_ws2_h` | `0x34960` | 2 | `0x370` / `0x370` |
| Limsa Lominsa | `hw_obj8_h` | `0x349B0` | 2 | `0x370` / `0x370` |
| Ul'dah | `hwa1` | `0x27A80` | 1 | `0x330` / `0x560` |
| Ul'dah | `hwf3` | `0x27940` | 1 | `0x330` / `0x330` |
| Ul'dah | `hwf2` | `0x27990` | 1 | `0x330` / `0x330` |
| Ul'dah | `hwf1` | `0x279E0` | 1 | `0x330` / `0x330` |
| Ul'dah | `hwg1` | `0x27A30` | 1 | `0x330` / `0x330` |
| Ul'dah | `hwe1` | `0x27AD0` | 1 | `0x330` / `0x330` |
| Ul'dah | `hwe2` | `0x27B20` | 1 | `0x330` / `0x330` |

## Representative exact chains

Gridania `hwa1`: TimeLines `0x64A80/0x64AA8` reference properties
`0x67690/0x676A8`, vectors `0x678A0/0x67958`, and SCBs
`0x6BDB0/0x6C0E0`. UnitTree `0x2A4C0`,
`sgrp_f0i0_a1_hlw01_h`, has a three-child run at `0x2F690`.
Children `0x2F6C0/0x2F6F0` reach the two timelines; child `0x2F690`
reaches BGParts `0x623C4`, `f0i0_a1_hlw01_h`. BGParts field
`0x623E4` references the key `2YD1pef0i0_a1_h` at `0x17AE2`.
Outer `brt` row `0x3860` binds resource `0x72E901D4`.

Limsa `hw_aq1_h`: TimeLines `0x89E64/0x89E8C` reference properties
`0x92A50/0x92A68`, vectors `0x92D50/0x92E08`, and SCBs
`0xAA740/0xAAAB0`. UnitTree `0x345A0`, `sgrp_bg_hw_aq1_h`, has
direct timeline children `0x3B700/0x3B730` and group `0x7A770`,
`grp_0199`. The group's two-child run at `0x3B6A0` reaches BGParts
`0x86768/0x867BC`, `s0i0_hw_aq1_h/s0i0_hw_aq1_s`.
Their resource-reference fields `0x86788/0x867DC` reach keys at
`0x1C361/0x1C380`, `4sVwyks0i0_hw_h/0ZBgffs0i0_hw_a`.
Outer `brt` rows `0x3A60/0x3A80` bind resources
`0x899404EA/0x899404EB`.

Ul'dah `hwa1`: TimeLines `0x6BCD8/0x6BCB0` reference properties
`0x6F240/0x6F228`, vectors `0x6F490/0x6F3D8`, and SCBs
`0x7C870/0x7C310`. UnitTree `0x27A80`, `sgrp_w0i1_a1_haro_h`,
has resident/hide/show children `0x2D270/0x2D2A0/0x2D2D0`.
The resident points to BGParts `0x6A164`, `w0i1_a1_haro_h`.
Field `0x6A184` reaches key `01L9tKw0i1_a1_h` at `0x149C4`;
outer `brt` row `0x1DC0` binds resource `0x897D0614`.

## Embedded scheduler framing and exception

The [SCB layout](cut-scheduler-clip-layout.md#resource-header-and-directory)
supplies the directory, class-key, actor and linked clip-record framing.
Every selected SCB has a `0x30`-byte SEDB header, a total size equal to
the TimeLine size field, bounded root children, class-key strings,
actor records and block/clip records. No fixed `0x330` size was assumed.

The eleven equal-length `0x330` pairs have one `LayUnitMemberActor`
record and one `ShowHideClip` targeting actor ordinal 0. Each pair
differs only at chunk `+0x1F0`, with show 1 and hide 0. The nineteen
`0x370` pairs have two `LayUnitMemberActor` records and two
`ShowHideClip` records targeting actor ordinals 1 and 0. Each differs
only at chunk `+0x220/+0x234`, with show 1 and hide 0.

Ul'dah `hwa1` show instead has the single-actor `0x330` framing at
`0x7C870`. Its hide SCB is the separate `0x560`-byte span
`0x7C310..0x7C870`. Its `@CACT` chunk at chunk `+0xC0` contains
actor 0 at `+0xD0`, size `0x24`, class `LayUnitMemberActor`, and
actor 1 at `+0xF4`, size `0x20`, class `LaySystemActor`. Its
`@CBKT` chunk at `+0x170` contains a clip record at `+0x200`
(file `0x7C510`), size `0xA8`, class `LayRaptureMaterial2Clip`,
actor ordinal 1. The `@CCPT` entry and string-table reference establish
that class label; no material operation or native effect was inferred.
The hide SCB SHA-256 is
`c231243e707bfdfe8a264309210a459fc70fe2980889af7b0099ca0a86144496`;
the show SCB SHA-256 is
`877ac5be7a0a0e922c94aaf89d2d9b20bd32697b4a184ac04c5cee3233337ec2`.

The [material dispatch finding](uldah-halloween-material.md) establishes this
native subclass's inherited dispatch and conditional factory/parameter path.
Its descriptor selection, retained record pointer and active effects remain
separate from the serialized joins here.

## Evidence boundary and reproduction

This closes the selected serialized property, vector, scheduler, direct
owner, resident-membership and resource-table joins. It does not join a
serialized actor's member token to the active native receiver, prove
that every resident or rendered descendant receives a clip, or establish
material changes, visible decoration, placement or playback.

The [native condition boundary](limsa-seasonal-weather-extension.md#decoration-condition-boundary)
remains separate from these selected masks. No runtime condition
construction, backend sender, clock, calendar, historical server policy,
8071 adoption or appended-ID activation follows. The five retained
interior Starlight pairs and their existing classification are outside
this selection. Their [final-client owner joins](city-winter-owner-joins.md)
retain empty show masks and separate historical-binding limits.

Reproduce by checking the three input identities and parser pin, then
following the named timelines through their declared reference fields.
Check all 23 encoded entries per vector, each complete embedded SCB
extent and its generic framing, every direct UnitTree child reference,
Limsa's separate group child runs, and each BGParts resource-key string
against the original outer table. Preserve all owner and row matches
before counting; these selected inputs yield one direct owner per
timeline and one `brt` row per resident key.
