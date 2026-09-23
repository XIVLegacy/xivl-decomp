# City seasonal weather selectors

The 1.23b city layouts and recovered 2011 patch payloads preserve
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

The extracted `D2010.12.13.0000.patch` binding table directly maps weather
8032 / `wtr_xmas` to Limsa `0x29D9001A`, Gridania `0x29B0001A`, and
Ul'dah `0x615A001D`. All three extracted weather payloads contain
`cbind_xmas` and `cam_xmas`, establishing an all-city Starlight atmosphere
in that patch version. The binding-table DAT `0x03C00000` is 20,464 bytes
with SHA-256
`f1579303ae88efc2ec2f50f90e5b44ad4a5aaccd1be026076f2112a3e14780c8`.
The three payload SHA-256 digests, in the same city order, are
`d2377abcb114e4e23cac6cedece40cdbc67225efa6ee3b51afeb0f02241d4196`,
`5a19f20d5b3b9052a098bce3ae49efdfb7386342106310a59048051467a976b0`,
and `fc984c755fdc3e55c79a977cdb5c5a7dd0737452e93554441d03beced5cc91db`.
This historical binding does not transfer to the final payloads at the same
keys; see [Weather payload reuse](#weather-payload-reuse).

The same December table binds 8031 / `wtr_chry` in all three cities. Its
extracted payloads have `cbind_chry`, `vfx_cam_chry`, and `sky0_star`, but no
Hina or blossom marker. A later Little Ladies' Day association for 8031 is
therefore a candidate, not a verified event control.

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

The final layout scan still finds 8027-only masks alongside
Halloween-named scheduler pairs in all three cities. In Gridania, that
layout condition coexists with an Xmas-marked 8027 weather payload. These
static resources do not prove the resulting rendered combination or a retail
server event schedule. In particular, historical Starlight 8032 is not a
safe Starlight command for the final client, and 8027 does not denote one
uniform atmosphere across the three cities.

## Starlight component residue

The direct-dependency graph of the 287 layouts identifies 55
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
the exact historical event transition. None of the 203 examined
`SEDBvins` controllers references the six orphan historical Xmas leaves.
The missing Limsa/Ul'dah controller, layout wrapper, selector, and placed
transforms cannot be inferred from the surviving component graph.

## Other city-layout show/hide names

The 1.23b city DATs also contain these 24 name-paired show/hide strings.
Each listed string was checked at its byte offset. The pair is based only on
the shared base name and `_show`/`_hide` suffix; it does not prove a common
owner, event, or invocation.

| Layout | DAT key | Show string @ offset | Hide string @ offset |
| ---: | --- | --- | --- |
| 321 | `0x29B00001` | `time_bg_coll_a1_show` @ `0x2F465` | `time_bg_coll_a1_hide` @ `0x2F47A` |
| 321 | `0x29B00001` | `time_bg_comp_show` @ `0x2F5AD` | `time_bg_comp_hide` @ `0x2F5BF` |
| 321 | `0x29B00001` | `time_bg_flag1_show` @ `0x2F561` | `time_bg_flag1_hide` @ `0x2F574` |
| 321 | `0x29B00001` | `time_bg_flag2_show` @ `0x2F587` | `time_bg_flag2_hide` @ `0x2F59A` |
| 391 | `0x29B00018` | `time_bg_air_show` @ `0x227A` | `time_bg_air_hide` @ `0x228B` |
| 121 | `0x29D90001` | `time_bg_coll_a1_show` @ `0x25DF4` | `time_bg_coll_a1_hide` @ `0x25E09` |
| 121 | `0x29D90001` | `time_bg_flg1_show` @ `0x25F06` | `time_bg_flg1_hide` @ `0x25F18` |
| 121 | `0x29D90001` | `time_bg_flg2_show` @ `0x25F2A` | `time_bg_flg2_hide` @ `0x25F3C` |
| 121 | `0x29D90001` | `time_bg_flg3_show` @ `0x25F4E` | `time_bg_flg3_hide` @ `0x25F60` |
| 121 | `0x29D90001` | `time_bg_gci1_show` @ `0x25F72` | `time_bg_gci1_hide` @ `0x25F84` |
| 121 | `0x29D90001` | `time_bg_hdi1_show` @ `0x25FBA` | `time_bg_hdi1_hide` @ `0x25FCC` |
| 121 | `0x29D90001` | `time_bg_kki2_show` @ `0x25F96` | `time_bg_kki2_hide` @ `0x25FA8` |
| 121 | `0x29D90001` | `time_bg_lamp1_show` @ `0x25FDE` | `time_bg_lamp1_hide` @ `0x25FF1` |
| 131 | `0x29D90002` | `time_bg_coll_a1_show` @ `0x1DBFB` | `time_bg_coll_a1_hide` @ `0x1DC10` |
| 131 | `0x29D90002` | `time_bg_tutrl01_show` @ `0x1DCA7` | `time_bg_tutrl01_hide` @ `0x1DCBC` |
| 196 | `0x29D90017` | `time_bg_lin_ex_show` @ `0x47E2` | `time_bg_lin_ex_hide` @ `0x47F6` |
| 196 | `0x29D90017` | `time_bg_lin_in_show` @ `0x480A` | `time_bg_lin_in_hide` @ `0x481E` |
| 196 | `0x29D90017` | `time_bg_objc_l0_wl_show` @ `0x4703` | `time_bg_objc_l0_wl_hide` @ `0x471B` |
| 421 | `0x615A0001` | `time_bg_coll_a1_show` @ `0x228BE` | `time_bg_coll_a1_hide` @ `0x228D3` |
| 421 | `0x615A0001` | `time_bg_flag1_show` @ `0x22A25` | `time_bg_flag1_hide` @ `0x22A38` |
| 421 | `0x615A0001` | `time_bg_flag2_show` @ `0x22A82` | `time_bg_flag2_hide` @ `0x22A95` |
| 421 | `0x615A0001` | `time_bg_wall1_show` @ `0x22A4B` | `time_bg_wall1_hide` @ `0x22A5E` |
| 421 | `0x615A0001` | `time_bg_wall2_show` @ `0x22AA8` | `time_bg_wall2_hide` @ `0x22ABB` |
| 431 | `0x615A0003` | `time_bg_coll_a1_show` @ `0x156CD` | `time_bg_coll_a1_hide` @ `0x156E2` |

The 24 entries are the rows outside the Halloween- and Starlight-named groups
in `city_layout_paired_schedulers.csv` rows 2-90. Each literal and offset was
checked against the corresponding DAT bytes.

The resource files are pinned independently of the names:

| DAT key | SHA-256 |
| --- | --- |
| `0x29B00001` | `abc13bd6f096b0fe9703b2092ca4a7f819ff2b88e4d08ae222a9db447f618312` |
| `0x29B00018` | `c9e8dd231cf675cb6cf7ed9239ff1f6889e9f1e2bbc8099d78ab956a9d41d44c` |
| `0x29D90001` | `bc40d7ff57ab8998978532cce4564e2488ed81d6d10b8f7ede3e1586b0b26523` |
| `0x29D90002` | `c6d86bfea2692e612464e7e08f6604df4f26fc1ea99da16727e824c920a693d8` |
| `0x29D90017` | `35f5df6d3138398f8b6fcba0025b4a870eb8370cbaaabad423ae8da4f17fa4db` |
| `0x615A0001` | `69c6ff8488ed9c407a28705cf6cb844d55845f7eed3ad58b31845b11a8331c4c` |
| `0x615A0003` | `d969e982a55d5c986e7bea24ae5af1bf23bfe7ed18854f7dd7c4f21a11d2353f` |

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
The December 2010 bindings and markers come from
`build_historical_seasonal_patch_recovery.py` outputs
`historical_weather_bindings.csv`, `historical_chry_payload_audit.csv`,
and the extracted `0x03C00000` and three city weather DATs. Their listed
sizes and SHA-256 digests were checked against the extracted bytes.
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
SHA-256 digests in the table were checked directly against the 1.23b client
files.
