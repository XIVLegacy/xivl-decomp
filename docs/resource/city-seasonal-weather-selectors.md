# City seasonal weather selectors

This note covers the installed 2012.09.19.0001 FFXIV 1.23b client layout
DATs. It documents static weather-conditioned visibility, not a live retail
event replay. The associated executable has image base `0x00400000` and
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

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
in Ul'dah. This is a final-snapshot state, not evidence that earlier retail
patches lacked those decorations. No sparse city mask contains weather 8029,
so the final city's named scheduler layer does not establish a summer
decoration switch for that weather ID.

## Corpus boundary

The typed scan covered 287 DATs referenced by recovered
`RegionResourceData` rows. Eleven have validated selector vectors, yielding
97 sparse masks: 60 select only 8027; 25 select 8030 and 8032; five select
no weather; three select only 8028; two select 8014 and 8028; one selects only
8014; and one selects 8001 and 8002. None selects 8029. The trial-area
8014/8028 masks show that this is a general weather-conditioned layout
mechanism, not a seasonal-only byte pattern.

These counts and joins are derived from the six city layout IDs and DAT keys
above and the 287 mapped-layout inventory, using the typed selector and
scheduler parser that produced `city-seasonal-weather-selector-atlas-20260711`
(`layout_weather_selector_summary.csv`,
`seasonal_scheduler_weather_bindings.csv`, and
`region_layout_weather_selector_inventory.csv`). Names and matching counts
support the family-level interpretation; they are not direct visual captures
or proof of all patch-era layouts. Missing historical layout payloads and
server state remain explicit limits.
