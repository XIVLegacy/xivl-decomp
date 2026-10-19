# Three-city clear and rain resource-table inputs

The six selected Limsa Lominsa, Gridania and Ul'dah clear/rain payloads
contained 214 outer resource-table rows: 202 typed nonzero occurrences naming
146 distinct nonzero resource IDs, and 12 empty-tag zero rows. All 146 selected
numeric target paths were available in the supplied stock install. This is a
direct table inventory and retained file-identity check.

## Sources and table framing

The [three-city selection finding](../net/city-weather-selection.md#retained-payload-identities)
owns the supplied region/weather selections and source payload pins. Weather
8001 has token `wtr_fine`; weather 8007 has token `wtr_rain`.

The source paths, lengths and SHA-256 pins remain in that owning record.

Extraction followed `XIVLegacy/xivl-tools:tools/research/census_layout.py`,
revision `75fe7414bf3f1ecd779825083d477adc4ed5f319`, SHA-256
`5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`,
`parse_layout` outer-table fields. Python `struct` independently reproduced
each name, tag and ID. Only the explicit six sources and their 146 distinct
nonzero numeric target paths were read. No directory scan or transitive
target traversal was used.

Every source had signature `MapLayoutResourceData` at `+0` and version
`1.1.0` at `+0x18`. The scene offset at `+0x20` and row count at `+0x24`
framed rows from `0x40` in `0x20`-byte steps. The table ended exactly at the
declared scene offset in all six files. A row contained a 16-byte name at
`+0`, 4-byte tag at `+0x10` and u32 resource ID at `+0x14`.

| Region | Weather | Source ID | Rows | Typed nonzero occurrences | Unique nonzero IDs | Table end / scene |
| ---: | ---: | --- | ---: | ---: | ---: | --- |
| 101 | 8001 | `0x29D9000F` | 31 | 30 | 30 | `0x420` |
| 101 | 8007 | `0x29D90013` | 45 | 42 | 41 | `0x5E0` |
| 103 | 8001 | `0x29B0000F` | 28 | 27 | 27 | `0x3C0` |
| 103 | 8007 | `0x29B00013` | 40 | 37 | 36 | `0x540` |
| 104 | 8001 | `0x615A0010` | 29 | 28 | 28 | `0x3E0` |
| 104 | 8007 | `0x615A0014` | 41 | 38 | 37 | `0x560` |

## Empty-tag rows and multiplicity

Each source row zero was `DEF_BLK`, empty tag, ID zero, with its ID field at
file `0x54`. Each rain table also contained empty-tag zero rows named
`vfx_cam_rain` and `vfx_cloud`:

| Source ID | `vfx_cam_rain` row / ID field | `vfx_cloud` row / ID field |
| --- | --- | --- |
| `0x29D90013` | 28 / `0x3D4` | 34 / `0x494` |
| `0x29B00013` | 26 / `0x394` | 32 / `0x454` |
| `0x615A0014` | 24 / `0x354` | 30 / `0x414` |

These twelve zero rows were excluded from file lookup and were not missing
targets. No empty-tag nonzero-ID or nonempty-tag zero-ID row was observed.

Every rain table repeated one `sniv` ID under the same observed name:

| Source ID | Target ID / name | Row indexes / ID field offsets |
| --- | --- | --- |
| `0x29D90013` | `0x898C0011` / `1ipGk6vleafinst` | 33 / `0x474`, 44 / `0x5D4` |
| `0x29B00013` | `0x5D200010` / `46Mc9qvleafinst` | 31 / `0x434`, 39 / `0x534` |
| `0x615A0014` | `0x8985000F` / `4dQUoZvleafinst` | 29 / `0x3F4`, 40 / `0x554` |

The file-identity inventory collapsed IDs for hashing while preserving all
source occurrences and aliases. The 202 typed occurrences used tags `btm`
26 times, `bxt` 78, `fcss` 56, `xetv` 16, `ldmv` 8, `ffev` 6, `fael` 6,
and `sniv` 6. Tags alone do not establish target compatibility.

## Direct membership and retained availability

| Supplied region / city | Clear unique | Rain unique | Shared | Clear only | Rain only |
| --- | ---: | ---: | ---: | ---: | ---: |
| 101 / Limsa Lominsa | 30 | 41 | 15 | 15 | 26 |
| 103 / Gridania | 27 | 36 | 13 | 14 | 23 |
| 104 / Ul'dah | 28 | 37 | 13 | 15 | 24 |

All six tables contained `0x5CE6000D`, `0x5CE6000E`, `0x5CE6000F`,
`0x5CE60010` and `0x615F0000`. The three rain tables also shared
`0x615F0010`, observed as `4taP0cx0x_xxxx_` / `bxt`, absent from these clear
tables. These six IDs were exactly the pairwise intersections of the three
city clear/rain unions. This compares table membership only.

Each of the 146 selected target paths was checked once per unique ID for
presence, readability, exact size and SHA-256. All were present. The retained
target identities describe the supplied stock root's bytes; they do not
authenticate every target's historical patch provenance. The six exact source
payload pins remain separate from that target-identity claim.
The [target identity inventory](city-weather-direct-inputs.csv) records each
selected ID, numeric path, size, digest and all source-row occurrences.
Its row locators use zero-based indexes and absolute file offsets.

The relative paths follow the
[conditional numeric formatter contract](resource-path-producer.md#formatting-contract).
Selecting those files offline establishes neither the actual client mode and
root at a request nor open success. No Weather overlay-first policy was
assumed.

This inventory does not establish full dependency closure, every row's
consumption, transitive references or embedded FCurves, typed compatibility,
native instantiation, rendered rain, historical schedules, probabilities,
ordinary eligibility, or subarea assignment. No client or server ran, and no
target media was extracted, decoded, or played.

Reproduce by checking the six source pins, following the bounded outer rows,
preserving the twelve empty-tag zero rows separately, and deriving numeric
paths for the nonzero IDs. Hash each distinct selected target and compare its
size, digest and occurrence list with the inventory. Do not follow target
references to extend this one-step selection.
