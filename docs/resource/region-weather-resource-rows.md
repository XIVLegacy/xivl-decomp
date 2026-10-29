# Region weather resource rows

The `data/03/C0/00/00.DAT` has SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
Scanning 0x30-byte little-endian records from file offset `0x40`, reading the
ID at `+0x00`, DAT key at `+0x08`, and 16-byte ASCII token at `+0x10`,
finds 1,089 token-bearing records. The nearest preceding non-`wtr_*`,
non-`sky*` token supplies the parent label used below. This is a sequential
resource grouping, not a demonstrated runtime area or weather selector.

There are 454 `wtr_*` records under 26 parent tokens: 450 positive-ID
records and four zero-ID records. A separate 53 `sky*` records are resource
siblings, not weather commands. Repeated ID/DAT-key rows in different
parts of the table remain separate records in these counts.

| Parent token | `wtr_*` records | Distinct positive IDs |
| --- | ---: | --- |
| `000_10` | 1 | 8001 |
| `_sample` | 1 | 8001 |
| `_test_btl` | 1 | none |
| `_test_new` | 1 | 8002 |
| `_test_r00` | 3 | 8002, 8010 |
| `_test_wtr` | 4 | 8001, 8002 |
| `art_f0` | 6 | 8001-8004, 8007, 8010 |
| `art_r0` | 6 | 8001-8004, 8006, 8007 |
| `art_s0` | 6 | 8001-8004, 8006, 8007 |
| `art_w0` | 6 | 8001-8004, 8007, 8012 |
| `fst_f0` | 80 | 8001-8004, 8007, 8010, 8027-8032, 8065-8068 |
| `lak_l0` | 20 | 8001-8004, 8007, 8017, 8030-8032, 8065 |
| `ocn_o0` | 3 | 8010, 8065-8066 |
| `ocn_o1` | 8 | 8001-8004, 8010, 8030-8032 |
| `ocn_o2` | 9 | 8001-8004, 8006-8007, 8010, 8017, 8081 |
| `prv_00` | 2 | 8001, 8007 |
| `prv_f0` | 1 | 8001 |
| `prv_i0` | 1 | 8001 |
| `prv_s0` | 1 | 8001 |
| `prv_w0` | 1 | 8001 |
| `roc_r0` | 55 | 8001-8004, 8006-8007, 8028, 8030-8032, 8065 |
| `roc_r1` | 9 | 8001-8002, 8028 |
| `sea_s0` | 52 | 8001-8004, 8006-8007, 8027, 8029-8032, 8065-8066 |
| `sea_s1` | 24 | 8001-8004, 8007, 8030-8032 |
| `srt_o0` | 9 | 8001-8004, 8010, 8029-8032 |
| `wil_w0` | 144 | 8001-8004, 8007, 8012, 8014, 8027-8032, 8065-8069 |

The 22 distinct positive IDs are 8001-8004, 8006-8007, 8010, 8012,
8014, 8017, 8027-8032, 8065-8069, and 8081. No record has ID 8015
or 8016; this only bounds this table, not client protocol support.
Each positive ID has one token spelling in this table:

| IDs | Resource tokens, in ID order |
| --- | --- |
| 8001-8004 | `wtr_fine`, `wtr_suny`, `wtr_clod`, `wtr_mist` |
| 8006-8007 | `wtr_stom`, `wtr_rain` |
| 8010, 8012, 8014, 8017 | `wtr_bolt`, `wtr_sand`, `wtr_heat`, `wtr_fogd` |
| 8027-8032 | `wtr_hall`, `wtr_smmn`, `wtr_smmr`, `wtr_comp`, `wtr_chry`, `wtr_xmas` |
| 8065-8069, 8081 | `wtr_h001`-`wtr_h005`, `wtr_e001` |

Selected rows expose why the token is not an event label: `sea_s0`
8027/`wtr_hall` is at `0x2B0` with DAT key `0x29D9001F`; `fst_f0`
8027/`wtr_hall` is at `0xDF0` with key `0x29B00020`; `wil_w0`
8027/`wtr_hall` is at `0x1450` with key `0x615A0023`. The three
8032/`wtr_xmas` rows are at `0x1F0`/`0x29D9001A`,
`0xCD0`/`0x29B0001A`, and `0x1330`/`0x615A001D`, respectively.
The 8029/`wtr_smmr` rows are at `0x280`/`0x29D9001E` under `sea_s0`,
`0xD60`/`0x29B0001D` under `fst_f0`, and `0x1420`/`0x615A0022` under
`wil_w0`. These are resource-row bindings, not proof of a runtime weather
selection or event interpretation.
The 21 8028/`wtr_smmn` rows bind four DAT keys, first at `0x760`, `0xD90`,
`0x13C0`, and `0x2B90`. Their payloads carry different markers:

| First row | DAT key | Bytes | SHA-256 | Markers |
| --- | --- | ---: | --- | --- |
| `0x760` | `0x28D90015` | 87,024 | `db13649451a75054235319e93bc9bfd3c236eb776bdd45c6df12e96e49e2a383` | `sdef_garuda_loop` at `0x1047`, `sdef_garuda_spot` at `0x1058` |
| `0xD90` | `0x29B0001E` | - | - | See [Thornmarch ring layout](thornmarch-ring-layout.md) |
| `0x13C0` | `0x615A0020` | 103,920 | `f5be8e1a2cd596bda078735e66e96c7f342de71238fbdc1c871c142539ce0397` | `cbind_fire` at `0x3E6E`, `vfx_cam_fire` at `0x220` and `0x3E10`, `time_smmn_se` at `0x1061`, `w0f_ifu_batle` at `0x38C3` |
| `0x2B90` | `0xABF40003` | 85,008 | `1e3f35b6a5a57391c2125b7235bebf7d74250b8e33ba230dbcf8432c0ece1636` | `sdef_meteor_sound` at `0x165E` |

These markers identify authored atmosphere resources, not an arena or
encounter binding.
These are file offsets and resource bindings. The differing payloads and
historical layout selectors are in [City seasonal weather selectors](city-seasonal-weather-selectors.md).

Four zero-ID weather-token rows are `wtr_indoor` at `0xB4A0` / key
`0x1C660002`, `wtr_btl` at `0xBFE0` / `0x297F0001`, `wtr_suny_wil`
at `0xCA90` / `0x29BF0008`, and `wtr_fine_wil` at `0xCAC0` /
`0x29BF0009`. None is evidence of a positive packet weather ID.
Likewise, a positive resource-row ID alone does not establish a command
alias, rendered appearance, retail event schedule, or active selector.
The packet-side boundary is in [Weather transition runtime](../net/weather-transition-runtime.md).
Its [Limsa rain selection](../net/weather-transition-runtime.md#limsa-rain-selection)
joins supplied region key `101` and weather `8007` to the selected resource
and weather queue under the documented native guards.

## Declared roots 102 and 105

The declared root/child runs in the pinned 52,336-byte table give these
type-zero memberships. Parentage follows the root's `+0x0C` child count,
rather than the nearest token. Python `struct` and `hashlib` checked the
table identity and the selected records using the loader finding's framing.

| Root ID | Root token / file offset | All children | Type-zero children | Type-zero IDs in file order |
| ---: | --- | ---: | ---: | --- |
| 102 | `roc_r0` / `0x610` | 25 | 11 | 8001, 8002, 8003, 8004, 8007, 8006, 8028, 8065, 8031, 8030, 8032 |
| 105 | `lak_l0` / `0x1810` | 15 | 10 | 8001, 8002, 8003, 8004, 8007, 8017, 8065, 8031, 8030, 8032 |

All selected children have raw `+0x0C == 0x102`; both roots have raw
`+0x20 == 0`. Root 102's selected children occupy `0x640..0x820` in
`0x30`-byte steps. Root 105's occupy `0x1840..0x19F0` in the same steps.
For example, ID 8007 binds key `0x28D90011` at `0x700` under 102 and
key `0x03E70007` at `0x1900` under 105; ID 8017 binds
`0x03E70008` at `0x1930` under 105.

The [three-city selection](../net/city-weather-selection.md#declared-region-membership)
owns the corresponding memberships for roots 101, 103 and 104. These
resource bindings include specialized weather IDs. They do not define an
ordinary lottery, weights, an active area, or city-to-field weather policy.
The raw stored dwords do not receive a probability interpretation.

## Native loader boundary

The pinned 1.23b `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32
image base `0x00400000`) was mapped with pefile 2024.8.26 and decoded with
Capstone 5.0.7 for x86-32. At VA `0x0079D900`, the loader compares the table
name `RegionResourceData` and version `1.1.0` using bounded string-comparison
calls. The name pointer is pushed by the instruction at `0x0079DA63`; its
immediate begins at `0x0079DA64`.

The image global at VA `0x012C492C` contains resource key `0x03C00000`, which
maps to the DAT named above under the
[resource path formatter](resource-path-producer.md).
That pinned DAT repeats the table name and version, declares size `0xCC70`,
and has 61 root rows. Walking those roots and their following child rows in
`0x30`-byte steps consumes 1,089 rows and ends at EOF.

The loader reads the root-row count from header `+0x24`, begins at `+0x40`,
and advances rows in `0x30`-byte steps. For a root row, `+0x0C` supplies the
number of following child rows. Root and child rows are passed to separate
constructors at `0x0079CD60` and `0x0079A280`; child objects are appended to a
container at parent `+0xBC`, and root objects to a container at loader `+0x08`.
The child constructor also receives its row's `+0x0C` value, so that offset is
only identified as a following-child count on root rows.

One direct caller at `0x0062B27D` checks the loader's begin and end fields
after loading and returns early when the root container is empty. This records
a loader handoff only. It does not establish weather selection, token or ID
meaning, travel routes, endpoint selection, or runtime behavior.

### Size and counted-row boundary

Entry-seeded x86-32 control-flow recovery of `0x0079D900` owns
`[0x0079D900,0x0079DBA6)`, SHA-256
`aee14d632f11f328ae24b3b0ec3b31aab96c48b9c71f39db18bbc0fed901bb4d`.
pefile 2024.8.26 and Capstone 5.0.7 checked the reachable instructions in
the pinned executable. Direct calls remained separate analysis boundaries.
The recovered normal control flow had no unresolved jumps. Exception
dispatch and the called constructors were outside this recovery.

At `0x0079DA56`, the loader compares the dword at header `+0x20` with
the reported extent. A mismatch takes `0x0079DA5B` and skips row
construction. The extent comes from request-record `+0x10` when its cached
data pointer at `+0x0C` is nonnull. Otherwise it comes from the Resource's
virtual slot `+0x28` at `0x0079DA50`. A null Resource supplies zero on
that branch. This enforces full 32-bit size equality without validating
the counted row span.

The name comparison uses 18 bytes at header `+0`, and the version comparison
uses five bytes at `+0x18` (`0x0079DA69/0x0079DA80`). The dword root count
at `+0x24` controls unsigned iteration
(`0x0079DA8C/0x0079DA9F/0x0079DB78`). Each root's dword `+0x0C` controls
unsigned child iteration (`0x0079DAF1/0x0079DB57`). Cursor increments at
`0x0079DAEE/0x0079DB54` consume one `0x30` row each.

The complete recovered normal loader control flow has no minimum-header-length
check, per-row remaining-length check, final cursor-versus-extent comparison,
suffix divisibility test or suffix scan. An unconsumed suffix is left unread
by this loader. Its reported extent must still equal header `+0x20`.
Matching the size word does not prove that declared counts fit the buffer or
that malformed input is safe. Called constructors and downstream acceptance
remain separate. Retain the
[root and child constraints](map-layout-selector.md#root-and-child-construction).

For the pinned no-tail 52,336-byte catalog, one appended root with one child
adds two `0x30` rows. The header consequences are root count 62 at `+0x24`
and size `0xCCD0` (52,432) at `+0x20`, provided the reported extent equals
that full length. These are count/size consequences, not a general writer
or client acceptance result. Require an exact no-tail walk for that bounded
profile as a stricter check than the native loader performs.

## Selected non-weather resource rows

Five other rows in this DAT bind these literal IDs and tokens to DAT keys:

| ID | File offset | Token | DAT key |
| ---: | ---: | --- | --- |
| 801 | `0xA9F0` | `art_s0` | `0x8B380000` |
| 802 | `0xABD0` | `art_r0` | `0x8B450000` |
| 803 | `0xADB0` | `art_f0` | `0x8B520000` |
| 804 | `0xAF90` | `art_w0` | `0x72AD0000` |
| 805 | `0xB170` | `srt_o0` | `0x89ED0000` |

These are direct resource-row values from the DAT identified above. They do
not establish runtime selection, route endpoints, or the meaning of the row
IDs.
