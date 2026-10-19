# Gridania and Ul'dah public Halloween pointer joins

The final 1.23b public layouts directly connect all 11 Gridania and all
10 Ul'dah Halloween-named show/hide pairs to 8027-only show selectors
and complementary hide selectors. Every pair has one declared UnitTree
owner with three direct children: a BGParts resident and the two timelines.
These owners have zero group reference and zero group count at +0x24/+0x28.
Their serialized owner structure differs from the selected Limsa example's
separately declared resident group.

## Original resources and method

| City / layout | Original DAT | Bytes | SHA-256 | SEDBlyb / payload file base |
| --- | --- | ---: | --- | --- |
| Gridania / 321 | `data/29/B0/00/01.DAT` | 1300928 | `abc13bd6f096b0fe9703b2092ca4a7f819ff2b88e4d08ae222a9db447f618312` | `0xD580` / `0xD5B0` |
| Ul'dah / 421 | `data/61/5A/00/01.DAT` | 1036816 | `69c6ff8488ed9c407a28705cf6cb844d55845f7eed3ad58b31845b11a8331c4c` | `0x9080` / `0x90B0` |

Offsets below are absolute file offsets. A serialized reference P resolves
to its city's payload base plus P. Python struct and
`xivl-tools:tools/research/census_layout.py`, `parse_layout`, at revision
`75fe7414bf3f1ecd779825083d477adc4ed5f319` (script SHA-256
`5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`),
followed declared node extents and descriptors.
The common field interpretation is documented in
`xivl-tools:docs/formats/world-layout.md`, Node declarations, Declared shapes
and Fields consumed by the zone reader. The joins below follow references,
rather than adjacent names or matching counts. Only the two selected direct
resident resource leaves were opened after exact resource-table joins.

Each selected TimeLine node has declared size 0x28 with descriptors
`(0x04,0), (0x08,2), (0x14,0), (0x1C,0), (0x20,2)`. Its +0x1C
reference reaches a 0x18-byte property named `wprp_0001` with descriptors
`(0x04,0), (0x08,2), (0x10,0)`. Property +0x10 references a run
of 23 declared 8-byte entries; +0x14 is its count. The vector declaration
has no reference descriptors. Every run lies inside its declared extent.
The selected declarations' table rows, descriptor lists and extent records
are located below. Every node used here matches its declared shape.

The entries enumerate IDs 8001-8017 and 8027-8032 in that order. Each
entry's first u32 has the numeric weather ID in its low half. A selected
entry has high half 0xFFFF and second u32 0xFFFFFFFF. An unselected entry
has high half zero and second u32 zero. Every show vector selects only
8027. Every hide vector selects the other 22 IDs. None describes a new ID.

| City | Type | Table row | Extent record | Descriptors | Node run start / count / size |
| --- | --- | --- | --- | --- | --- |
| Gridania | timeline | `0x153B4` | `0x15940` | `0x156C0` | `0xFDF5C` / 109 / `0x28` |
| Gridania | property | `0x15414` | `0x15988` | `0x1570C` | `0x10D4A0` / 47 / `0x18` |
| Gridania | vector | `0x15424` | `0x15994` | `0x15718` | `0x10D908` / 1039 / `0x8` |
| Gridania | owner_tree | `0x15164` | `0x15784` | `0x154B0` | `0x4BA10` / 345 / `0x50` |
| Gridania | owner_child | `0x15174` | `0x15790` | `0x154CC` | `0x525E0` / 1060 / `0x30` |
| Gridania | bgparts | `0x152A4` | `0x15874` | `0x155E4` | `0xEFC98` / 579 / `0x54` |
| Ul'dah | timeline | `0xEAF4` | `0xF054` | `0xEDEC` | `0xC9900` / 95 / `0x28` |
| Ul'dah | property | `0xEB54` | `0xF09C` | `0xEE38` | `0xD4520` / 30 / `0x18` |
| Ul'dah | vector | `0xEB64` | `0xF0A8` | `0xEE44` | `0xD47F0` / 682 / `0x8` |
| Ul'dah | owner_tree | `0xE8C4` | `0xEEB0` | `0xEBF0` | `0x3D800` / 350 / `0x50` |
| Ul'dah | owner_child | `0xE8D4` | `0xEEBC` | `0xEC0C` | `0x44560` / 939 / `0x30` |
| Ul'dah | bgparts | `0xE9B4` | `0xEF64` | `0xECDC` | `0xBC334` / 521 / `0x54` |

## Timeline, property, vector and embedded scheduler joins

Each table cell lists show / hide file offsets. Timeline names are
`time_bg_<suffix>_show` and `time_bg_<suffix>_hide`.

### Gridania

| Suffix | Timeline | Property | Vector | SCB |
| --- | --- | --- | --- | --- |
| `hlw1a` | `0xFEC2C` / `0xFEC54` | `0x10D6F8` / `0x10D710` | `0x10E9B0` / `0x10EA68` | `0x137D70` / `0x1380A0` |
| `hlw1b` | `0xFEC7C` / `0xFECA4` | `0x10D728` / `0x10D740` | `0x10EB20` / `0x10EBD8` | `0x1383D0` / `0x138700` |
| `hlw1c` | `0xFECCC` / `0xFECF4` | `0x10D758` / `0x10D770` | `0x10EC90` / `0x10ED48` | `0x138A30` / `0x138D60` |
| `hlw1d` | `0xFED1C` / `0xFED44` | `0x10D788` / `0x10D7A0` | `0x10EE00` / `0x10EEB8` | `0x139090` / `0x1393C0` |
| `hlw1e` | `0xFED6C` / `0xFED94` | `0x10D7B8` / `0x10D7D0` | `0x10EF70` / `0x10F028` | `0x1396F0` / `0x139A20` |
| `hlw1f` | `0xFEDBC` / `0xFEDE4` | `0x10D7E8` / `0x10D800` | `0x10F0E0` / `0x10F198` | `0x139D50` / `0x13A080` |
| `hlw1g` | `0xFEE0C` / `0xFEE34` | `0x10D818` / `0x10D830` | `0x10F250` / `0x10F308` | `0x13A3B0` / `0x13A6E0` |
| `hlw1h` | `0xFEE5C` / `0xFEE84` | `0x10D848` / `0x10D860` | `0x10F3C0` / `0x10F478` | `0x13AA10` / `0x13AD40` |
| `hlw1i` | `0xFEEAC` / `0xFEED4` | `0x10D878` / `0x10D890` | `0x10F530` / `0x10F5E8` | `0x13B070` / `0x13B3A0` |
| `hlw2a` | `0xFEEFC` / `0xFEF24` | `0x10D8A8` / `0x10D8C0` | `0x10F6A0` / `0x10F758` | `0x13B6D0` / `0x13BA00` |
| `hlw2b` | `0xFEF4C` / `0xFEF74` | `0x10D8D8` / `0x10D8F0` | `0x10F810` / `0x10F8C8` | `0x13BD30` / `0x13C060` |

### Ul'dah

| Suffix | Timeline | Property | Vector | SCB |
| --- | --- | --- | --- | --- |
| `hw0` | `0xCA670` / `0xCA648` | `0xD4718` / `0xD4700` | `0xD56C8` / `0xD5610` | `0xFB560` / `0xFB230` |
| `hw1` | `0xCA4E0` / `0xCA4B8` | `0xD4628` / `0xD4610` | `0xD4F98` / `0xD4EE0` | `0xF9580` / `0xF9250` |
| `hw2` | `0xCA698` / `0xCA508` | `0xD4730` / `0xD4640` | `0xD5780` / `0xD5050` | `0xFB890` / `0xF98B0` |
| `hw3` | `0xCA6C0` / `0xCA530` | `0xD4748` / `0xD4658` | `0xD5838` / `0xD5108` | `0xFBBC0` / `0xF9BE0` |
| `hw4` | `0xCA6E8` / `0xCA558` | `0xD4760` / `0xD4670` | `0xD58F0` / `0xD51C0` | `0xFBEF0` / `0xF9F10` |
| `hw5` | `0xCA710` / `0xCA580` | `0xD4778` / `0xD4688` | `0xD59A8` / `0xD5278` | `0xFC220` / `0xFA240` |
| `hw6` | `0xCA738` / `0xCA5A8` | `0xD4790` / `0xD46A0` | `0xD5A60` / `0xD5330` | `0xFC550` / `0xFA570` |
| `hw7` | `0xCA760` / `0xCA5D0` | `0xD47A8` / `0xD46B8` | `0xD5B18` / `0xD53E8` | `0xFC880` / `0xFA8A0` |
| `hw8` | `0xCA788` / `0xCA5F8` | `0xD47C0` / `0xD46D0` | `0xD5BD0` / `0xD54A0` | `0xFCBB0` / `0xFABD0` |
| `hw9` | `0xCA7B0` / `0xCA620` | `0xD47D8` / `0xD46E8` | `0xD5C88` / `0xD5558` | `0xFCEE0` / `0xFAF00` |

Timeline +0x14 references the embedded SCB and +0x18 declares its byte
size. All 42 spans are bounded within their lyb payloads. Each SCB has
magic `SEDBSCB`, header size 0x30 and total size 0x330, equal to the
timeline's size field. Each contains null-terminated `LayUnitMemberActor`
at chunk +0x2D4 and `ShowHideClip` at +0x2E7. Each show/hide pair differs
in exactly one byte, chunk +0x1F0: show is 1 and hide is 0. Selected SCB
SHA-256 values are listed with their resident resource examples below.

## Direct resident owners

Every owner is a declared `RefObjects/UnitTree/UnitTreeObject` of size
0x50. Its +0x30/+0x34 gives a three-record child run. Each child is the
declared 0x30-byte child shape and its +0x18 is the base reference.
Two children named `show` and `hide` reference the corresponding timelines
above. The remaining child references a declared 0x54-byte
`BaseObjects/BG/BGPartsBaseObject`. Names below are observations after the
pointer joins. They were not used to infer ownership.

### Gridania

| Suffix | UnitTree / name | Resident child / name | BGParts / name | Timeline children show / hide |
| --- | --- | --- | --- | --- |
| `hlw1a` | `0x52270` / `sgrp_f0t0_a0_hlw1a_h` | `0x5E670` / `f0t0_a0_hlw1a_h_001` | `0xFB6A4` / `f0t0_a0_hlw1a_h` | `0x5E6A0` / `0x5E6D0` |
| `hlw1b` | `0x522C0` / `sgrp_f0t0_a0_hlw1b_h` | `0x5E700` / `f0t0_a0_hlw1b_h_001` | `0xFB6F8` / `f0t0_a0_hlw1b_h` | `0x5E730` / `0x5E760` |
| `hlw1c` | `0x52310` / `sgrp_f0t0_a0_hlw1c_h` | `0x5E790` / `f0t0_a0_hlw1c_h_001` | `0xFB74C` / `f0t0_a0_hlw1c_h` | `0x5E7C0` / `0x5E7F0` |
| `hlw1d` | `0x52360` / `sgrp_f0t0_a0_hlw1d_h` | `0x5E820` / `f0t0_a0_hlw1d_h_001` | `0xFB7A0` / `f0t0_a0_hlw1d_h` | `0x5E850` / `0x5E880` |
| `hlw1e` | `0x523B0` / `sgrp_f0t0_a0_hlw1e_h` | `0x5E8B0` / `f0t0_a0_hlw1e_h_001` | `0xFB7F4` / `f0t0_a0_hlw1e_h` | `0x5E8E0` / `0x5E910` |
| `hlw1f` | `0x52400` / `sgrp_f0t0_a0_hlw1f_h` | `0x5E940` / `f0t0_a0_hlw1f_h_001` | `0xFB848` / `f0t0_a0_hlw1f_h` | `0x5E970` / `0x5E9A0` |
| `hlw1g` | `0x52450` / `sgrp_f0t0_a0_hlw1g_h` | `0x5E9D0` / `f0t0_a0_hlw1g_h_001` | `0xFB89C` / `f0t0_a0_hlw1g_h` | `0x5EA00` / `0x5EA30` |
| `hlw1h` | `0x524A0` / `sgrp_f0t0_a0_hlw1h_h` | `0x5EA60` / `f0t0_a0_hlw1h_h_001` | `0xFB8F0` / `f0t0_a0_hlw1h_h` | `0x5EA90` / `0x5EAC0` |
| `hlw1i` | `0x524F0` / `sgrp_f0t0_a0_hlw1i_h` | `0x5EAF0` / `f0t0_a0_hlw1i_h_001` | `0xFB944` / `f0t0_a0_hlw1i_h` | `0x5EB20` / `0x5EB50` |
| `hlw2a` | `0x52540` / `sgrp_f0t0_a0_hlw2a_h` | `0x5EB80` / `f0t0_a0_hlw2a_h_001` | `0xFB998` / `f0t0_a0_hlw2a_h` | `0x5EBB0` / `0x5EBE0` |
| `hlw2b` | `0x52590` / `sgrp_f0t0_a0_hlw2b_h` | `0x5EC10` / `f0t0_a0_hlw2b_h_001` | `0xFB9EC` / `f0t0_a0_hlw2b_h` | `0x5EC40` / `0x5EC70` |

### Ul'dah

| Suffix | UnitTree / name | Resident child / name | BGParts / name | Timeline children show / hide |
| --- | --- | --- | --- | --- |
| `hw0` | `0x444C0` / `sgrp_w0t0_h0_hlw0p_h` | `0x4F480` / `w0t0_h0_hlw0p_h_001` | `0xC6D2C` / `w0t0_h0_hlw0p_h` | `0x4F4E0` / `0x4F4B0` |
| `hw1` | `0x441F0` / `sgrp_w0t0_h0_hlw1s_h` | `0x4EF70` / `w0t0_h0_hlw1s_h_001` | `0xC6A38` / `w0t0_h0_hlw1s_h` | `0x4EFD0` / `0x4EFA0` |
| `hw2` | `0x44240` / `sgrp_w0t0_h0_hlw2m_h` | `0x4F000` / `w0t0_h0_hlw2m_h_001` | `0xC6A8C` / `w0t0_h0_hlw2m_h` | `0x4F060` / `0x4F030` |
| `hw3` | `0x44290` / `sgrp_w0t0_h0_hlw3l_h` | `0x4F090` / `w0t0_h0_hlw3l_h_001` | `0xC6AE0` / `w0t0_h0_hlw3l_h` | `0x4F0F0` / `0x4F0C0` |
| `hw4` | `0x442E0` / `sgrp_w0t0_h0_hlw4l_h` | `0x4F120` / `w0t0_h0_hlw4l_h_001` | `0xC6B34` / `w0t0_h0_hlw4l_h` | `0x4F180` / `0x4F150` |
| `hw5` | `0x44330` / `sgrp_w0t0_h0_hlw5bg_h` | `0x4F1B0` / `w0t0_h0_hlw5bg_h_001` | `0xC6B88` / `w0t0_h0_hlw5bg_h` | `0x4F210` / `0x4F1E0` |
| `hw6` | `0x44380` / `sgrp_w0t0_h0_hlw6k_h` | `0x4F240` / `w0t0_h0_hlw6k_h_001` | `0xC6BDC` / `w0t0_h0_hlw6k_h` | `0x4F2A0` / `0x4F270` |
| `hw7` | `0x443D0` / `sgrp_w0t0_h0_hlw7t_h` | `0x4F2D0` / `w0t0_h0_hlw7t_h_001` | `0xC6C30` / `w0t0_h0_hlw7t_h` | `0x4F330` / `0x4F300` |
| `hw8` | `0x44420` / `sgrp_w0t0_h0_hlw8_sp` | `0x4F360` / `w0t0_h0_hlw8_sp_001` | `0xC6C84` / `w0t0_h0_hlw8_sp` | `0x4F3C0` / `0x4F390` |
| `hw9` | `0x44470` / `sgrp_w0t0_h0_hlw9_sp` | `0x4F3F0` / `w0t0_h0_hlw9_sp_001` | `0xC6CD8` / `w0t0_h0_hlw9_sp` | `0x4F450` / `0x4F420` |

## Selected direct resource identities

For one pair per city, BGParts +0x20 references a key string that
matches exactly one `brt` row in the outer resource table. The row
resource ID at +0x14 identifies the available leaf. These are direct
resource inputs, without an inspected dependency closure.

- Gridania `hlw1a`: BGParts field `0xFB6C4`
  references `0x2DB3C`, string `03WJnnf0t0_a0_h`.
  Outer `brt` row `0x5520` binds key `0x72E901A3` to
  `data/72/E9/01/A3.DAT`, 45544 bytes, SHA-256
  `572354f81bb3fc673d14994b8c22fba379e1eadaec758c40464a5b0142c4abc9`.
  Selected show SCB SHA-256: `84ad55ee220f2e727f9d34336af88090ab936bd071c32bdd8e3c1ad18c0957e1`.
  Selected hide SCB SHA-256: `86dad48fc31d3b6a7ea8add1623a64725475497a12224acf8adf3c87abc33511`.
- Ul'dah `hw1`: BGParts field `0xC6A58`
  references `0x215FE`, string `2kssIiw0t0_h0_h`.
  Outer `brt` row `0x3DE0` binds key `0x897D05C8` to
  `data/89/7D/05/C8.DAT`, 76114 bytes, SHA-256
  `82dd1c5725dc27e1a744c0b4f6c720f0ccb33b1f12f8d0adcb83abeda2e68028`.
  Selected show SCB SHA-256: `a327c4bb74ad6b0637ac07515fc2ebeb646ab72b60f40b43d5c99f353bbdc187`.
  Selected hide SCB SHA-256: `2783a3424804b0e65dca6ad554303f8eb3ba136b24c52c755347c787f2cbb8d4`.

## Evidence boundary

This establishes static serialized selector, scheduler, owner and
selected resident resource joins. The SCB class labels and pair byte
difference do not identify the active native receiver or prove that
every rendered descendant receives the clip. No mesh appearance,
portable `b###` variant, or visible playback was established.

Native activation has the separate
[condition-consumer boundary](limsa-seasonal-weather-extension.md#decoration-condition-boundary).
These serialized joins do not establish that appending another entry or
allocating another ID activates decorations.

The final Gridania 8027 weather payload remains distinct: the existing
finding in [Weather payload reuse](city-seasonal-weather-selectors.md#weather-payload-reuse)
pins its Xmas-marked payload. The 8027-only Halloween
layout join above does not change that weather-payload observation or
establish a uniform atmosphere across cities.

The [selected Limsa owner](limsa-seasonal-weather-extension.md#halloween-selector-and-resident-group)
provides the positive reference. Its separate group node and 0x370-byte
SCBs are not assumed for these cities. This finding covers the two selected
final public layouts and their direct leaf examples; it does not extend to
interiors, historical versions or runtime activation.

## Reproduction

Check the exact layout and parser identities above, then parse their declared
node extents and reference descriptors. Enumerate the selected TimeLine
records and follow their `+0x1C` property reference to the vector/count.
Check every vector entry and embedded SCB span against the fields and pair
differences above. Follow each UnitTree's declared child run, confirm the
show/hide children point to the two timelines, and confirm the remaining
child points to the declared BGParts record. Check the zero group fields.
For the two selected leaves, match the BGParts key string to its unique
outer `brt` row and verify the resulting resource file's size and SHA-256.
