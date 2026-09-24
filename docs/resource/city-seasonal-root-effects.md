# City seasonal root effects

The three 1.23b capital root layouts retain separate egg and hanabi
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
hanabi timeline names in the 1.23b DAT scan; this does not mean its
hanabi assets or compiled scheduler bodies are absent. All ten sparse weather
masks in these roots select 8030 and 8032, not 8027 or 8029. The separate
city layouts that contain weather-conditioned decorations are described in
[City seasonal weather selectors](city-seasonal-weather-selectors.md).

Gridania's root DAT also contains the pooled strings `vtp1..vtp5` once each,
at offsets `0x6D10`, `0x6D15`, `0x6D1A`, `0x6D1F`, and `0x6D24` in
`0x29B00000` (SHA-256
`b5ae5559d9e1eaea1bdb94a545b861ee5d751faaa5e9e451415edcac66bf184b`). No
expanded `time_vfx_hanabi*_vtp#` string occurs in that DAT. This distinguishes
the resident short keys from timeline references; it does not link a key to a
primitive, object group, or activation.
The source inventory is `moonfire_hanabi_executable_atlas_20260712/hanabi_short_key_string_reuse.csv`, rows 2-6; these offsets and the negative
expanded-name scan were checked against the DAT bytes.

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
The root layouts also do not show a direct 8029 weather-selector
mask for the hanabi groups; weather and timeline activation must not be
conflated.

## Compiled hanabi bodies and owner candidates

Each root contains eight hanabi-bearing `SEDBSCB` chunks, four in each of
two banks. All three cities have the same eight chunk sizes (928, 1024,
1024, 1152, 1184, 1184, 1216, and 1232 bytes) and controlled-actor-count
fingerprint (1, 2, 2, 4, 4, 4, 4, 5), while all 24 raw chunk hashes are
distinct. Gridania therefore retains compiled bodies even without the
expanded `time_vfx_hanabi*_vtp#` names. Each corresponding primitive has
matching timing words across the cities, including 9,000,000- and
6,000,000-unit block fields. Their unit interpretation and the higher
`vtp#`-to-primitive selection are not established by those bytes alone.
The u32 at `+0x28` in the second `@CBLK` header is `3 | 5 | 5 | 6` for bank
one and `7 | 7 | 8 | 7` for bank two, with each sequence matching across the
three city DATs. These encoded values do not establish visible effect counts,
elapsed seconds, or retail activation.

The bank markers immediately follow serialized `isgrp` names in six root
layout neighborhoods. Their candidate instance IDs are 41 and 62 in
Gridania, 199 and 222 in Limsa, and 251 and 265 in Ul'dah. Strict 0x20-byte
transform records encode the four Limsa/Ul'dah IDs in their final word as
`(instance_id << 8) | slot`; 82 such records decode. Gridania uses a
different transform representation here, so neither of its candidate IDs
has that second join. These are structural owner candidates, not verified
retail map-object spawn rows. No actor-class-to-owner or activation join
follows from the neighborhoods or transforms.

### Serialized hanabi component references

Each root's nine `vfx_hanabi*` groups contain 104 serialized component-reference
entries and 40 distinct dependency DAT keys:

| City | Component-reference entries | Distinct dependency DAT keys |
| --- | ---: | ---: |
| Gridania | 104 | 40 |
| Limsa Lominsa | 104 | 40 |
| Ul'dah | 104 | 40 |

Direct parsing of the hash-matched root DATs yields 312 serialized entries and 120 city-scoped
distinct-key observations; the latter is the sum of each city's distinct-key
count, not a claim that all 120 keys differ globally. Each parsed entry has
an index, name, extension, dependency key, and flags. The root DAT hashes
below identify the source layouts. These are serialized references, not proof that every
dependency was loaded, displayed, or activated in retail.

## Provenance

The three `MapLayoutResourceData` root DATs are 664,720, 529,376, and
438,960 bytes respectively, with SHA-256 digests:

| DAT key | SHA-256 |
| --- | --- |
| `0x29B00000` | `b5ae5559d9e1eaea1bdb94a545b861ee5d751faaa5e9e451415edcac66bf184b` |
| `0x29D90000` | `b61cf798a02fc1bf72f1445a01b6a94a2e58793e7dd82d0d654f1d422bbceff3` |
| `0x615A0000` | `f7ce6e6db1f5862248bc6108f149c69f4a5981b057eb4e2707b3297fc5fc9434` |

The root DAT hashes above identify the layouts used for the direct group,
scheduler, and component scans. The compiled SCB records and local transforms
support structural neighborhoods, not active owners. The recovered
`chara/npc/mapobj/mapobjfireworks.lua` methods `_onLoop` and
`getFireworksSchedulor` identify the inspected script path. A literal scan
of the pinned `xivl-client-scripts:manifests/private_lua_corpus.json` corpus
found no Lua `vtp` caller; this is a corpus limit, not a universal absence
claim.
