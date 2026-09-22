# Bowl of Embers fire-ring layout

The installed layout `data/61/5A/00/08.DAT` contains a named fire-ring
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

The layout is 1,245,056 bytes, SHA-256
`56b24e6aca53911810848baf7be254a2c20038d8c127bcc0c8603ba6b0614e7c`.
The related `f0ifuring1.veff` resource at `data/89/84/00/74.DAT`
is 11,676 bytes, SHA-256
`f399fa88a3654a81ee8c00744c5994a063dcc785ddc9ccc868841ee5c323be39`.
Both identities were checked against installed files. The decoded
instance and clip locators are in the contributor's
`tools/outputs/ifrit-bowl-layout-neighborhood-20260805/summary.json`,
`arena_tile_instances.csv`, and `ring_scheduler_clips.csv`; the
interpretation is bounded in
`docs/ifrit-animation-decomp-2026-08-02/COMPLETE_COVERAGE_MATRIX.md`,
Battlefield fire-ring correction.
