# City seasonal root effects

The three installed capital root layouts retain separate egg and hanabi
visual families. Their names identify authored resource families; the layouts
alone do not identify a retail actor owner or an activation schedule.

| City | Root DAT key | Egg groups / timelines | Hanabi groups / named timelines | Root sparse weather masks |
| --- | --- | ---: | ---: | ---: |
| Gridania | `0x29B00000` | 5 / 15 | 9 / 0 | 2 |
| Limsa Lominsa | `0x29D90000` | 5 / 15 | 9 / 10 | 5 |
| Ul'dah | `0x615A0000` | 5 / 15 | 9 / 10 | 3 |

Each root has `vfx_egg_001..005` and the fifteen
`time_vfx_egg_v_[lcr][1-5]` names. The three roots also have
`vfx_hanabi1..9`. Limsa and Ul'dah additionally have two five-member
`time_vfx_hanabi*_vtp1..5` name families. Gridania lacks those expanded
hanabi timeline names in the installed DAT scan; this does not mean its
hanabi assets or compiled scheduler bodies are absent. All ten sparse weather
masks in these roots select 8030 and 8032, not 8027 or 8029. The separate
city layouts that contain weather-conditioned decorations are described in
[City seasonal weather selectors](city-seasonal-weather-selectors.md).

## Script and activation boundary

Recovered `MapObjFireworks._onLoop` reads server time, checks Hydaelyn night,
and submits five `_runBgScheduler` requests with short names `v_l#`, `v_c#`,
or `v_r#`; `getFireworksSchedulor` chooses `#` from 1 through 5. These short
names match the suffix set of the egg timelines in all three roots. The script
does not name `time_vfx_egg_*`, `vtp1..5`, weather 8029, or a hanabi timeline.
Suffix agreement makes the egg family a plausible target but is not a
verified name-resolution, actor-owner, or retail invocation join. It cannot
be used to classify this script as a proven Hatching-tide controller or a
Moonfire controller.

The recovered Lua and compiled Lua inventories each contain 2,517 files,
with no `vtp1..5` caller found by the examined literal scan. Native BG
scheduler dispatch can accept server-supplied names, so that negative Lua
scan does not rule out retail hanabi playback. No authenticated retail owner
binding, trigger, or orchestration timing is recovered for either family.
The installed root layouts also do not show a direct 8029 weather-selector
mask for the hanabi groups; weather and timeline activation must not be
conflated.

## Provenance

The installed `MapLayoutResourceData` root DATs are 664,720, 529,376, and
438,960 bytes respectively, with SHA-256 digests:

| DAT key | SHA-256 |
| --- | --- |
| `0x29B00000` | `b5ae5559d9e1eaea1bdb94a545b861ee5d751faaa5e9e451415edcac66bf184b` |
| `0x29D90000` | `b61cf798a02fc1bf72f1445a01b6a94a2e58793e7dd82d0d654f1d422bbceff3` |
| `0x615A0000` | `f7ce6e6db1f5862248bc6108f149c69f4a5981b057eb4e2707b3297fc5fc9434` |

The layout counts and names were checked against
`build_decoration_only_hatching_tide_atlas.py` outputs
`hatching_tide_city_root_summary.csv` and
`hatching_tide_scheduler_tokens.csv`, and
`build_moonfire_hanabi_residue_atlas.py` outputs
`moonfire_hanabi_city_root_summary.csv` and
`moonfire_hanabi_scheduler_tokens.csv` in the contributor's July 2026
atlases. The script call path is in recovered
`lua/chara/npc/mapobj/mapobjfireworks.lua`, methods `_onLoop` and
`getFireworksSchedulor`. The absence of a Lua `vtp` caller comes from the
residue atlas's `moonfire_hanabi_controller_gap.csv` and scan summary; it
is a corpus limit, not a universal absence claim.
