# Bowl of Embers fire-ring layout

The layout `data/61/5A/00/08.DAT` contains a named fire-ring
unit in `wil_w0_fld05` / `wil0Field05a`. Client instance
`isgrp_016280`, internal layout node 949, references
`sgrp_vfx_ifring` at `(2526.596924, 248.343002, 2208.061035)`.
The adjacent `isgrp_016281` is a separate terrain unit. The suffix
`016280` is a client instance name, not a decoded server actor, layout,
or map-object ID.

The ring unit contains `vfx_ifuring1_001`, attributes
`attr_w0f0_ifu_ring_a` and `attr_w0f0_ifu_ring_b`, sound definition
`sdef_ifrit_circle`, and collision `coll_ifu_ring`. Five group-owned
timelines map to these short aliases:

| Alias | Scheduler | Direct clip result |
| --- | --- | --- |
| `show` | `time_vfx_if_ring_a_show` | collision on value 1 |
| `hide` | `time_vfx_if_ring_a_hide` | collision on value 0 |
| `vtp1` | `time_vfx_if_ring_vtp1` | sound and VFX clips |
| `sho1` | `time_vfx_if_ring_b_show` | collision on value 1 |
| `hid1` | `time_vfx_if_ring_b_hide` | collision on value 0 |

The nearby `time_vfx_fire_vtp1` scheduler is not a member of this ring
unit. The alias and clip data establish authored layout capability and
the collision clip values. They do not establish the initial visible
state, runtime show/hide order, retail server caller, late-join replay,
or a link from this ring to an Eruption or Plume combat selector.

The same layout has 225 decoded `RefObjects/InstanceObject`
records. A structural scan of the Bowl floor tile's X/Z bounds
`2496..2560` / `2176..2240` finds six instances:

| Instance | World XYZ | Referenced object |
| --- | --- | --- |
| `isgrp_000396` | `(2528, 248, 2208)` | regional floor/collision group |
| `isgrp_016281` | `(2528, 248, 2208)` | Bowl floor chip |
| `isgrp_016280` | `(2526.597, 248.343, 2208.061)` | `sgrp_vfx_ifring` |
| `w0f5_bbr1_Boss` | `(2516, 246.919, 2212)` | `pomk_0006` position marker |
| `isgrp_007349` | `(2520.917, 170.147, 2225.860)` | point light below the floor |
| `isgrp_007144` | `(2551.625, 247.880, 2236.699)` | bridge-lamp group |

These are the complete decoded instance set inside those bounds, not an
encounter spawn catalog. In particular, the one named `Boss` marker is a
client layout marker, not a proven retail Ifrit spawn coordinate. The
set does not contain a repeated placed Plume or Nail pattern; numeric,
script-created, or otherwise non-instance placements remain outside it.
Layout extraction reproduced the six saved instance rows
field-for-field; their node and physical offsets are in
`arena_tile_instances.csv` from `extract_ifrit_bowl_layout_neighborhood.py`.

The layout is 1,245,056 bytes, SHA-256
`56b24e6aca53911810848baf7be254a2c20038d8c127bcc0c8603ba6b0614e7c`.
The related `f0ifuring1.veff` resource at `data/89/84/00/74.DAT`
is 11,676 bytes, SHA-256
`f399fa88a3654a81ee8c00744c5994a063dcc785ddc9ccc868841ee5c323be39`.
Both identities were checked against the 1.23b client files. The node,
instance, and clip identifiers above describe the parsed static records;
they do not establish a retail ring trigger or visible timing.
