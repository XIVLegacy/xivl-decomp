# Three-city weather resource selection

The retained RegionResourceData supports exact resource selections for supplied
Limsa Lominsa, Gridania and Ul'dah region inputs. Weather IDs 8001 and 8007
have type-zero child rows and distinct available payloads in all three roots.
These are conditional client selections, not historical active-zone observations,
ordinary-weather eligibility, probabilities or rendered-weather results.

## Inputs and method

The original `data/03/C0/00/00.DAT` is 52,336 bytes, SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
The [loader finding](../resource/region-weather-resource-rows.md#native-loader-boundary)
owns its native constructors. Python `struct` and `hashlib` checked the
`RegionResourceData` / `1.1.0` header, declared size at `+0x20`, root count
at `+0x24`, and all declared root/child runs from `0x40` in `0x30`-byte steps.
The 61 roots and their children consume 1,089 rows and end exactly at EOF
`0xCC70`. Parentage below follows each root's child count, not nearby tokens.

The native selection observations use retained Ghidra instruction listings
checked against `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
pefile `2024.8.26` mapped the original PE; Capstone `5.0.7` independently
decoded the listed x86 instructions and selected semantic anchors. Addresses
below are VAs. No client process ran.

## Declared region membership

| Supplied region input | Root token / file offset | All children | Type-zero children | Type-zero IDs, in row order |
| --- | --- | ---: | ---: | --- |
| Limsa Lominsa: 101 | `sea_s0` / `0x40` | 30 | 13 | 8001, 8002, 8003, 8004, 8007, 8006, 8065, 8066, 8032, 8031, 8030, 8029, 8027 |
| Gridania: 103 | `fst_f0` / `0xAF0` | 32 | 16 | 8001, 8002, 8003, 8004, 8007, 8010, 8065, 8066, 8067, 8032, 8031, 8030, 8029, 8028, 8068, 8027 |
| Ul'dah: 104 | `wil_w0` / `0x1120` | 36 | 18 | 8001, 8002, 8003, 8004, 8007, 8012, 8065, 8066, 8067, 8068, 8032, 8031, 8069, 8028, 8030, 8029, 8027, 8014 |

Each root's raw `+0x20` is zero. Type-zero membership includes event and
other specialized resources. It is not a list of ordinary random outcomes.
The supplied region is an invocation input; these rows do not identify a
current zone, city instance or surrounding field area.

## Selected rows and conditional queue records

For these rows, child `+0/+4/+8/+0x0C` contains weather ID, type, resource
key and a stored dword. All types below are zero. Tokens at child `+0x10`
are `wtr_fine` for 8001 and `wtr_rain` for 8007.

| Region | Weather ID | Child file offset | Resource key | Raw child `+0x0C` |
| ---: | ---: | --- | --- | --- |
| 101 | 8001 | `0x70` | `0x29D9000F` | `0x2` |
| 101 | 8007 | `0x130` | `0x29D90013` | `0x2` |
| 103 | 8001 | `0xB20` | `0x29B0000F` | `0x102` |
| 103 | 8007 | `0xBE0` | `0x29B00013` | `0x102` |
| 104 | 8001 | `0x1150` | `0x615A0010` | `0x102` |
| 104 | 8007 | `0x1210` | `0x615A0014` | `0x102` |

The stored dword is preserved without assigning eligibility or climate meaning.
The shared [weather selection path](weather-transition-runtime.md#limsa-rain-selection)
applies to each supplied pair under its existing actor, receiver and provider
guards:

1. `0x00624180` uses actor `+0x190` when its second argument is zero.
   `0x0079B380` resolves the supplied region against RegionInfo `+0xB0`.
   `0x0079B190` normalizes through type-zero children and `0x0079B050`
   selects the corresponding key. Both selected IDs match exactly. A miss
   in the normalizer falls back to 8001 for each of these roots.
2. The actor stores the normalized ID at `+0x19C` and key at `+0x1A4`.
   Its `+0x17C` branch chooses `0x007E8DB0` or `0x007E8DF0`; each stores
   the supplied region at weather-owner `+0x64` before key submission to
   `0x007E87F0`. The alternate type-one Sky submission remains separate.
3. `0x007E87F0` suppresses an already-queued resource key. On its admitted
   construction path, `0x0079B6C0` finds the first root containing that key
   and supplies its `+0xB8` value, derived from root `+0x20`. For all six
   selected keys that first owner is the supplied root, and the value is zero.
4. `0x007E80F0` uses `0x0079B540`, preferred-root helper `0x0079B310`
   and reverse-key lookup `0x0079ADF0` to recover the same weather ID.
   The append record contains key at `+0`, recovered ID at `+4` and the
   constructed complete-object pointer at `+8`; the pointer remains a runtime
   input. This reverse lookup and first-owner search do not filter child type.

Actor enable/state/current-ID guards, queue-processing readiness, concrete
world/provider aliases and construction success remain necessary inputs.
In particular, an exact row and an available DAT do not bypass these guards.

## Retained payload identities

In the [numeric formatter mode](../resource/resource-path-producer.md#formatting-contract),
the keys produce the relative paths below. Every selected file begins with
`MapLayoutResourceData` at `+0`, version `1.1.0` at `+0x18`. Header and file
identity checks do not establish dependency closure, actual opens or readiness.
The [direct resource-table inventory](../resource/city-weather-direct-inputs.md)
records those six payloads' typed nonzero targets and retained file identities,
with empty-tag zero rows, aliases and transitive/runtime limits kept separate.

| Resource key / relative path | Bytes | SHA-256 |
| --- | ---: | --- |
| `0x29D9000F` / `data/29/D9/00/0F.DAT` | 117,952 | `6fbd4ce9183fcb9ced230c3edba489b9b5803a939f0f85ceb9653c4eaec7942e` |
| `0x29D90013` / `data/29/D9/00/13.DAT` | 113,024 | `11b2735ee161d302ad212a27041c84060a5b4f61da438c15c9f4bec982d83012` |
| `0x29B0000F` / `data/29/B0/00/0F.DAT` | 129,856 | `4c2627bd34f53d445d0834677e5064fcecaffd0d13b07dae8fbb698b323a98a6` |
| `0x29B00013` / `data/29/B0/00/13.DAT` | 118,240 | `a12f5341bbe4b64da8251fe9f8452b19cb12a13437b0f62b394698601a2d48bb` |
| `0x615A0010` / `data/61/5A/00/10.DAT` | 124,160 | `f46543a5f6e8d4002c96b1585faea4c26b064bf23b491674e19f068e9b412477` |
| `0x615A0014` / `data/61/5A/00/14.DAT` | 139,392 | `83feba5a16b649fb3acfb66168107085f61c336e4c66c7ab4701cfdda44d928a` |

## Reproduction and boundary

Use the pinned table, executable and six payload identities. Read the root
count and each root's `+0x0C` child count, following complete declared runs.
Select roots by their `+0x00` IDs, then filter their children by `+0x04 == 0`.
Check the six exact row fields above and reproduce the native normalization,
key lookup, preferred-root reverse lookup and first-owner lookup at the cited
VAs. Preserve file order and all matching types in the two reverse searches.
Then check the numeric paths, header labels, lengths and SHA-256 values.

This establishes the selected conditional resource route for all three inputs.
Historical city/field schedules, probabilities, ordinary eligibility, visible
rain and every subarea's inheritance remain separate evidence questions.
The [city seasonal study](../resource/city-seasonal-weather-selectors.md)
owns the distinct seasonal payload/version and resident-decoration evidence.
