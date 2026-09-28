# Thornmarch ring layout controls

The MapLayoutResourceData at `data/29/B0/00/06.DAT` is
1,692,336 bytes, SHA-256
`f539b71233efa49c3f869d88ca494ab88db2a55487f2a2c64543bbaf0091d7fa`.
It contains named Moogle arena ring and wall groups, plus paired
show/hide schedulers. The names and embedded records establish authored
client controls, not their retail invocation sequence.

The layout names include `f0f0_mog_ring_h`,
`attr_f0f0_mog_ring_a`, and `f0f0_mog_w001_h` through
`f0f0_mog_w004_h`. These are layout-local model/collision and prop
identifiers, not world-origin coordinates.

| Surface | Show scheduler | Embedded `SEDBSCB` offset | Hide scheduler | Embedded `SEDBSCB` offset | Bytes per SCB |
| --- | --- | ---: | --- | ---: | ---: |
| Ring | `time_mog_ring_show` | `0x19A750` | `time_mog_ring_hide` | `0x19AAD0` | 896 |
| Wall 001 | `time_mog_w001_show` | `0x19AE50` | `time_mog_w001_hide` | `0x19B180` | 816 |
| Wall 002 | `time_mog_w002_show` | `0x19B4B0` | `time_mog_w002_hide` | `0x19B7E0` | 816 |
| Wall 003 | `time_mog_w003_show` | `0x19BB10` | `time_mog_w003_hide` | `0x19BE40` | 816 |
| Wall 004 | `time_mog_w004_show` | `0x19C170` | `time_mog_w004_hide` | `0x19C4A0` | 816 |

Each listed offset begins with `SEDBSCB`; the size field at payload
`+0x10` bounds its record. Each bounded record contains a
`ShowHideClip` type literal and its corresponding ring or wall token.
The show/hide names also occur in the layout's name table. This direct
asset check does not recover the runtime dispatcher, battle-phase
conditions, initial visibility, or a world transform for the ring.

## Primal atmosphere resource

The RegionResourceData at `data/03/C0/00/00.DAT` is 52,336
bytes, SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
Its record at file offset `0xD90` pairs child ID 8028, resource key
`0x29B0001E`, and the `wtr_smmn` token. The corresponding resource at
`data/29/B0/00/1E.DAT` is 88,912 bytes, SHA-256
`1b9ca79a141a8cfd4cd6cc28d4497cd14222d5c6b79137f25beb548e1c5f468a`.
It contains 12 `SEDBSCB` and 62 `SEDBmtb` signatures, along with
`moguri01`, `sdef_mog_loop`, `sdef_mog_spot`, `cbind_mog`,
`time_wtr_00`, and `envmap_smmn` literals. These are authored
atmosphere and effect resources. The static wrapper does not establish
when the encounter selected them or what a player saw in a particular
phase.

## Serialized arena instances

The layout's `lyb` payload starts at physical file offset `0x5C50`.
Its root `LaySettingsObject` pointer is relative `0x25780`, and the
root translation is `(0, 0, 0)`. Following each
`RefObjects/InstanceObject` reference to a
`RefObjects/UnitTree/UnitTreeObject` recovers one ring instance and
24 wall instances. Each row below gives the instance's relative node
offset and its serialized translation at node `+0x20`; coordinates
are rounded to 0.001 client units. They are authored layout
placements, not enemy spawn points or proof of a runtime visibility
state.

| Instance | Node offset | X | Y | Z |
| --- | ---: | ---: | ---: | ---: |
| `isgrp_mog_ring` | `0x8BC10` | -2368.000 | -16.000 | -896.000 |
| `isgrp_mog_w1_01` | `0x8BC50` | -2386.729 | -16.034 | -845.864 |
| `isgrp_mog_w1_02` | `0x8BC90` | -2406.722 | -8.739 | -959.267 |
| `isgrp_mog_w1_03` | `0x8BCD0` | -2336.189 | -1.841 | -963.409 |
| `isgrp_mog_w1_04` | `0x8BD10` | -2289.911 | -20.795 | -876.032 |
| `isgrp_mog_w2_01` | `0x8BD50` | -2402.089 | -23.571 | -861.673 |
| `isgrp_mog_w2_02` | `0x8BD90` | -2389.818 | -23.377 | -845.553 |
| `isgrp_mog_w2_03` | `0x8BDD0` | -2327.012 | -23.808 | -871.055 |
| `isgrp_mog_w2_04` | `0x8BE10` | -2338.799 | -23.517 | -921.975 |
| `isgrp_mog_w2_05` | `0x8BE50` | -2315.430 | 0.008 | -959.433 |
| `isgrp_mog_w2_06` | `0x8BE90` | -2326.687 | -23.102 | -921.217 |
| `isgrp_mog_w2_07` | `0x8BED0` | -2371.827 | -23.603 | -939.002 |
| `isgrp_mog_w2_08` | `0x8BF10` | -2424.176 | -22.468 | -931.299 |
| `isgrp_mog_w2_09` | `0x8BF50` | -2372.180 | -23.618 | -928.936 |
| `isgrp_mog_w2_10` | `0x8BF90` | -2402.669 | -23.745 | -918.563 |
| `isgrp_mog_w2_11` | `0x8BFD0` | -2447.643 | -10.801 | -832.020 |
| `isgrp_mog_w2_12` | `0x8C010` | -2397.450 | -28.055 | -899.844 |
| `isgrp_mog_w3_01` | `0x8C050` | -2425.160 | -22.911 | -837.232 |
| `isgrp_mog_w3_02` | `0x8C090` | -2338.923 | -24.332 | -887.171 |
| `isgrp_mog_w3_03` | `0x8C0D0` | -2416.979 | -23.349 | -861.587 |
| `isgrp_mog_w3_04` | `0x8C110` | -2405.523 | -8.874 | -959.204 |
| `isgrp_mog_w4_01` | `0x8C150` | -2339.164 | -11.497 | -895.499 |
| `isgrp_mog_w4_02` | `0x8C190` | -2342.053 | -0.574 | -965.886 |
| `isgrp_mog_w4_03` | `0x8C1D0` | -2444.937 | 0.538 | -912.944 |
| `isgrp_mog_w4_04` | `0x8C210` | -2323.853 | -16.830 | -875.850 |

The matching references are `sgrp_f0f0_mog_ring_h` for the ring and
`sgrp_f0f0_mog_w001_h` through `sgrp_f0f0_mog_w004_h` for the wall
families. This explicit node/reference join is required: searching for
coordinate-shaped floats or wall labels alone can misassign a position
to the wrong wall family.
