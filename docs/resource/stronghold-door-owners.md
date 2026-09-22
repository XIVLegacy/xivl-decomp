# Stronghold door owners

The installed FFXIV 1.23b layouts for Natalan, Zahar'ak, U'Ghamaro Mines,
and Castrum Novum contain 46 placed unit-tree instances with compiled
open/close door timelines. This inventory uses serialized placement ownership,
not proximity between names in the resource.

## Exact owner families

| Stronghold | Zone | Layout | Unit-tree owner | Instance IDs | Count |
| --- | ---: | ---: | --- | --- | ---: |
| Natalan | 143 | 201 | `sgrp_bg_gate_ixal` | 4001-4009 | 9 |
| Zahar'ak | 174 | 405 | `sgrp_w0f0_br_dor1_h` | 15834-15840 | 7 |
| Zahar'ak | 174 | 405 | `sgrp_w0f0_br_dor2_h` | 5841-5843 | 3 |
| U'Ghamaro Mines | 137 | 116 | `sgrp_bg_d6_door_a1` | 487, 488, 490, 492, 1315, 1333 | 6 |
| U'Ghamaro Mines | 137 | 116 | `sgrp_bg_d6_door_b1` | 493-500, 1114, 1116, 1124, 1132, 1280 | 13 |
| U'Ghamaro Mines | 137 | 116 | `sgrp_bg_d6_door_c1` | 501, 1095, 1585 | 3 |
| Castrum Novum | 190 | 501 | `sgrp_teikokutobira_A` | 19111, 19269-19272 | 5 |

The corresponding scheduler pairs are:

| Owner | Open timeline | Close timeline |
| --- | --- | --- |
| `sgrp_bg_gate_ixal` | `time_gate_ixal_open` | `time_gate_ixal_clos` |
| `sgrp_w0f0_br_dor1_h` | `time_bg_door_c1_open` | `time_bg_door_c1_clos` |
| `sgrp_w0f0_br_dor2_h` | `time_bg_door_c2_open` | `time_bg_door_c2_clos` |
| `sgrp_bg_d6_door_a1` | `time_bg_d6_door_a1_open` | `time_bg_d6_door_a1_clos` |
| `sgrp_bg_d6_door_b1` | `time_bg_d6_door_b1_open` | `time_bg_d6_door_b1_clos` |
| `sgrp_bg_d6_door_c1` | `time_bg_d6_door_c1_open` | `time_bg_d6_door_c1_clos` |
| `sgrp_teikokutobira_A` | `time_bg_gate_open` | `time_bg_gate_clos` |

These owners expose the four-byte `open` alias used by the map-object
animation contract. The matching close alias is serialized as `clos`,
consistent with the four-byte name field documented in
[Ferry door schedulers](ferry-door-schedulers.md).

U'Ghamaro also gives the A1 and C1 owners `stt0` and `end0` sequences, and
contains a separate `sgrp_0135` machine-door family. Those resources are not
part of the 22 ordinary A1/B1/C1 placements above. Natalan, Zahar'ak, and
Castrum similarly contain collision, lighting, VFX, or event-room timelines
that are not door-owner evidence merely because they appear in the same DAT.

## Evidence identity

| Stronghold | Resource | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Natalan | `data/28/D9/00/01.DAT` | 1,902,592 | `bd7966e0aaafc96862384e2e52b53f48e8be90000fd61a1885450f25d75a6d2e` |
| Zahar'ak | `data/61/5A/00/08.DAT` | 1,245,056 | `56b24e6aca53911810848baf7be254a2c20038d8c127bcc0c8603ba6b0614e7c` |
| U'Ghamaro Mines | `data/29/D9/00/0D.DAT` | 601,920 | `d8ff9874f392367fa111a75997915257a465bf8fc4c613cafa035cd12f6e733a` |
| Castrum Novum | `data/03/E7/00/01.DAT` | 2,028,000 | `85b5f27689ac1cb3f750b4c72c976a171b32557b1732c2a0b6936ea2e160ca9c` |

Across the four resources, a broad name-based scan finds 43 door-like
timelines, 171 placements beneath their owners, and 60 lexical placement
candidates. Those larger populations are review sets, not additional doors.
The 46 rows above are the placements directly owned by the seven compiled
open/close families.

## Evidence boundary

The layout graph proves resource ownership, instance identity, authored
placement, and animation availability. It does not assign a server actor
class, proximity radius, lock policy, encounter gate, or initial state. A
class file or a successful binding elsewhere cannot supply those semantics.
Server implementations may adopt these layout/instance pairs, but runtime
policy requires separate client-script, server-content, or historical capture
evidence.
