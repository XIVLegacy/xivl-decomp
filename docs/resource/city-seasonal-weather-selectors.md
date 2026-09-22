# City seasonal weather selectors

The installed city layouts and recovered 2011 patch payloads preserve
weather-conditioned decoration selectors. Their layouts identify client-side
visibility conditions, not retail server timing or a live event replay.

## Six city layouts

Typed scans of each city's public and interior `MapLayoutResourceData` find
paired named show/hide schedulers and weather-selector vectors. Across these
six DATs, 60 Halloween-named scheduler pairs reconcile by layout count with
60 sparse masks selecting weather 8027 alone:

| City | Public layout ID / DAT key | Public pairs / masks | Interior layout ID / DAT key | Interior pairs / masks |
| --- | --- | ---: | --- | ---: |
| Gridania | 321 / `0x29B00001` | 11 / 11 | 331 / `0x29B00002` | 10 / 10 |
| Limsa Lominsa | 121 / `0x29D90001` | 8 / 8 | 131 / `0x29D90002` | 14 / 14 |
| Ul'dah | 421 / `0x615A0001` | 10 / 10 | 431 / `0x615A0003` | 7 / 7 |

The layout counts support weather 8027 as a client-side condition for these
resident Halloween groups. They do not prove that each named pair has a uniquely
identified mask without a structural pointer join, nor does it prove the
historical server's event timing or a visible result in every scene. The
weather packet path is described in
[Weather transition runtime](../net/weather-transition-runtime.md); the
separate player work packet is in
[Seasonal event work control plane](../event/seasonal-control-plane.md).

Five interior Starlight-named pairs instead reconcile with empty sparse show
masks in this final snapshot: one in Gridania, two in Limsa Lominsa, and two
in Ul'dah. The [historical layout delta](#historical-layout-delta) identifies
their earlier 8032 selectors. That historical binding does not make weather
8032 a Starlight switch in the final 1.23b layout. No sparse city mask
contains weather 8029, so the final city's named scheduler layer does not
establish a summer decoration switch for that weather ID.

## Historical layout delta

The extracted `D2011.10.04.0000.patch` payloads retain the same six city
layout keys listed above. Typed parsing finds 60 Halloween-named show/hide
pairs and 60 8027-only masks: Gridania 21, Limsa Lominsa 22, Ul'dah 17.
This is direct evidence in the October layout version, independent of the
final-installation counts. It remains a layout-level reconciliation, not a
per-pair pointer or runtime rendering trace.

Comparing the extracted `D2011.12.14.0000.patch` interior layouts to their
October predecessors finds five added paired controls and five added
8032-only masks:

| City / interior DAT | Added paired controls | Added 8032-only masks | Added winter-post group |
| --- | --- | ---: | --- |
| Gridania / `0x29B00002` | `time_bg_crs1` | 1 | `sgrp_w_itm0_pstr1_h` |
| Limsa Lominsa / `0x29D90002` | `time_bg_itm0_pstr1_h`, `time_bg_itm0_pstr3_h` | 2 | post variants 1 and 3 |
| Ul'dah / `0x615A0003` | `time_bg_xmas1`, `time_bg_xmas2` | 2 | post variants 1 and 2 |

Each of the ten compiled `SEDBSCB` bodies is 816 bytes and targets a
`LayUnitMemberActor` with `ShowHideClip`. Direct comparison of each show/hide
pair finds exactly one different byte at chunk `+0x1F0`: show has 1, hide
has 0. The five 8032 mask offsets in those December interior DATs are
`0x686C0` (Gridania), `0x94170` and `0x942E0` (Limsa), and `0x6ECD8` and
`0x6EE48` (Ul'dah). The patch delta, winter-post resources, and selector
counts authenticate a Starlight decoration control family. They do not
recover the historical server's switch timing, per-instance XYZ transforms,
or an independent one-to-one pointer from each named scheduler to a mask.

The supplied extracted payloads were checked against their inventory sizes
and SHA-256 digests. These nine city layout files support the counts and
October-to-December delta (key is the eight-hex-digit DAT filename):

| Patch | DAT key | SHA-256 |
| --- | --- | --- |
| 2011.10.04 | `29B00001` | `7c9b042025e1b882b2e5bce26f75c8b10984219234d7f34a06665f19a8bb5d41` |
| 2011.10.04 | `29B00002` | `2ad724c4cacc530cf30eb4afe53e893177f2e5494d1bbe33891566e8d17919cc` |
| 2011.10.04 | `29D90001` | `43f6cc4323f2f1c811a0ad646955b69b0e384b89bc68e7bff00147627df441c5` |
| 2011.10.04 | `29D90002` | `5243e9d67a56b2e8b84a42c2246cdd9132fb1f1c656cf6b2e0250a578c1bdd5d` |
| 2011.10.04 | `615A0001` | `eb8fb982cfc8240003a3aadd4bfbbf3631a2321826fa88619d35a8e2305240df` |
| 2011.10.04 | `615A0003` | `e90b948a36bd72fa8f2eb0b78757571ebed6d0d9e9d252df4ddf9216e9793f20` |
| 2011.12.14 | `29B00002` | `26a9cb144325ec1664982c1d6eecb988422624dea776cbf001a042a4f227aba9` |
| 2011.12.14 | `29D90002` | `9720349a2c8da8e58a86455cd267e647b306a619ac33cfe9deb5fcf1bb37ac67` |
| 2011.12.14 | `615A0003` | `10270b05d6724c514aefc8b588a2ab74dcb740c70a7784c7e9152640a3f63821` |

## Weather payload reuse

The supplied `D2012.07.21.0000.patch` payloads replace the marker families
at existing weather-resource keys without changing their legacy `wtr_xmas`
or `wtr_hall` tokens. The decoded December 2011 payloads at the same keys
provide the direct comparison:

| Weather / city / DAT key | December 2011 marker | July 2012 marker | July payload SHA-256 |
| --- | --- | --- | --- |
| 8032 / Gridania / `0x29B0001A` | `cbind_xmas`, `vfx_cam_xmas` | `vfx_lastwtr`, `vfx_tunder1` | `d5091df5591691482a712c606ba8b733bd9341db361beb99cc27226ac3eb6405` |
| 8032 / Limsa Lominsa / `0x29D9001A` | `cbind_xmas`, `vfx_cam_xmas` | `vfx_lastwtr`, `vfx_tunder1` | `1c85433b1194c39680b122706d36b1debd3adbf25a417b7b06f934e6690103f3` |
| 8032 / Ul'dah / `0x615A001D` | `cbind_xmas`, `vfx_cam_xmas` | `vfx_lastwtr`, `vfx_tunder1` | `9ae218a998bc965c2e973eaa4a303864a227bb175a8cba7fc66126f63d1d452a` |
| 8027 / Gridania / `0x29B00020` | `sdef_hallo_imp` | `cbind_xmas`, `vfx_cam_xmas` | `44cff7e75f20f8c0735f4ecd91bc1e7c2b9c28551806d5ee3fa3e5b1b1a122f1` |

All four July files match their recorded byte lengths and hashes, and the
listed markers were checked in the extracted bytes. The supplied
`D2012.09.19.0001.patch` updates Ul'dah's 8032 DAT again (SHA-256
`fd1d2aa16538dd7203dbb7e3d0077b32082b830e525a9af138c08a851e4ecc7b`);
that later payload still contains `vfx_lastwtr` and `vfx_tunder1`, not the
December `cbind_xmas` or `vfx_cam_xmas` markers.

The final installed layout scan still finds 8027-only masks alongside
Halloween-named scheduler pairs in all three cities. In Gridania, that
layout condition coexists with an Xmas-marked 8027 weather payload. These
static resources do not prove the resulting rendered combination or a retail
server event schedule. In particular, historical Starlight 8032 is not a
safe Starlight command for the final client, and 8027 does not denote one
uniform atmosphere across the three cities.

## Starlight component residue

The direct-dependency graph of the 287 installed layouts identifies 55
Starlight-named structured component DATs. Thirty-nine have a current layout
owner: seven in Gridania's 8027 payload and 32 in retained last-weather or
thunder groups. Sixteen have no direct owner: four Gridania, six Limsa, and
six Ul'dah files. Internal resource identifiers join the orphan camera,
cloud, snow-effect, model, and texture leaves into city-local atmosphere
chains; nine additional non-Xmas-named cloud support DATs lack current
ownership. These are retained components, not an authenticated final-layout
wrapper or retail placement.

Two old Gridania effect keys are exact byte duplicates of effects used by
its final 8027 payload:

| Old key | Final key | Bytes | Shared SHA-256 |
| --- | --- | ---: | --- |
| `0x5D210080` | `0x5D2100AA` | 6,940 | `df4386c42ed3fce40763e1a10b571882930924eb891565733579480c7b60a638` |
| `0x5D210081` | `0x5D2100AB` | 12,136 | `d9d21cdabce67e272e86941f3fb58a6235a969ef8ef86114115729ae50f1412d` |

The duplicate bytes prove preservation under different dependency keys, not
the exact historical event transition. None of the 203 examined installed
`SEDBvins` controllers references the six orphan historical Xmas leaves.
The missing Limsa/Ul'dah controller, layout wrapper, selector, and placed
transforms cannot be inferred from the surviving component graph.

## Corpus boundary

The typed scan covered 287 DATs referenced by recovered
`RegionResourceData` rows. Eleven have validated selector vectors, yielding
97 sparse masks: 60 select only 8027; 25 select 8030 and 8032; five select
no weather; three select only 8028; two select 8014 and 8028; one selects only
8014; and one selects 8001 and 8002. None selects 8029. The trial-area
8014/8028 masks show that this is a general weather-conditioned layout
mechanism, not a seasonal-only byte pattern.

The final 2012.09.19.0001 installation counts come from the typed selector
and scheduler parser that produced `city-seasonal-weather-selector-atlas-20260711`
(`layout_weather_selector_summary.csv` and
`region_layout_weather_selector_inventory.csv`). The historical counts and
offsets come from `build_historical_seasonal_layout_timeline.py` output
`historical-seasonal-layout-timeline-20260712`
(`patch_city_layout_inventory.csv`, `historical_event_scheduler_pairs.csv`,
`historical_event_weather_selectors.csv`,
`december_starlight_layout_delta.csv`, and
`december_starlight_compiled_scheduler_chunks.csv`). The weather-payload
comparison uses `build_late_seasonal_patch_timeline.py` output
`late-seasonal-patch-timeline-20260712/late_event_payload_identity.csv`.
The supplied extracted DAT bytes, sizes, hashes, markers, and show/hide
differences were locally checked. The original patch archive envelope was
not independently re-verified in this checkout. Names and matching counts
support family-level interpretation, not direct visual captures or
historical server state.

The component-owner counts, identifier edges, and VINS scan derive from
`build_seasonal_component_residue_atlas.py` outputs
`seasonal_component_ownership.csv`,
`starlight_resource_identifier_edges.csv`,
`starlight_cloud_support_ownership.csv`, and
`starlight_vins_controller_scan.csv`. The four Gridania DAT sizes and
SHA-256 digests in the table were checked directly against the installed
files.
