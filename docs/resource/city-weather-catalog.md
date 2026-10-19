# Three-city authored weather catalog

All 47 authored type-zero rows under supplied Limsa Lominsa, Gridania and
Ul'dah roots name distinct nonzero payload keys. All 47 selected numeric paths
were present in the supplied stock install with header label
`MapLayoutResourceData` at `+0` and version `1.1.0` at `+0x18`. In the static
model of the published lookup, each preferred root's first resource-key match
yielded its authored weather ID.

The [complete payload catalog](city-weather-catalog.csv) preserves all 47
authored rows in file order, including tokens, raw fields, exact locators,
conditional reverse IDs, retained file sizes and SHA-256, and global key
aliases. The [three-city selection finding](../net/city-weather-selection.md)
owns the source table identity, declared membership and native guards. Its
guarded selection path remains conditional for every catalog entry.

## Scope and reproduction

Python `struct` and `hashlib` checked the pinned RegionResourceData and the
47 explicit payload paths derived from roots 101, 103 and 104 with child
`+4 == 0`. Declared root/child runs, rather than token proximity, determine
parentage. The full source walk reproduced the selection finding's 61 roots,
1,089 rows and exact EOF. Root count came from header `+0x24`, each root's
following-child count from `+0x0C`, and rows began at `0x40` in `0x30`-byte
steps. The [native loader finding](region-weather-resource-rows.md#native-loader-boundary)
owns that framing's native interpretation.

Each selected child preserved its weather ID at `+0`, type at `+4`, key at
`+8`, raw stored dword at `+0x0C`, and token at `+0x10`. Each selected root
had raw `+0x20 == 0`. The first eight Limsa rows had raw child `+0x0C == 0x2`;
the other selected rows had `0x102`. These stored values are not interpreted
as eligibility or probability fields.

| Supplied root / token | Type-zero rows | Unique payload keys | Retained payloads |
| --- | ---: | ---: | --- |
| 101 / `sea_s0` | 13 | 13 | All present, `MapLayoutResourceData` / `1.1.0` |
| 103 / `fst_f0` | 16 | 16 | All present, `MapLayoutResourceData` / `1.1.0` |
| 104 / `wil_w0` | 18 | 18 | All present, `MapLayoutResourceData` / `1.1.0` |

Only the pinned table and these 47 payload DATs were read. Each payload was
hashed once per distinct key, without interpreting its outer resource table
or dependencies. The six existing clear/rain file-identity controls matched
exactly. The other retained hashes identify the selected stock root's bytes;
they do not authenticate each payload's historical patch provenance.

The numeric paths follow the
[conditional formatter contract](resource-path-producer.md#formatting-contract).
Their offline availability establishes neither actual request-time mode/root
nor client open success. No Weather overlay-first policy was assumed.

## Preferred-root reverse IDs and global aliases

The computation reused the selection finding's published
[reverse-key and first-owner formulas](../net/city-weather-selection.md#selected-rows-and-conditional-queue-records).
For each key, the preferred root was searched in child order without a child
type filter. All matching children were retained before taking the first.
Every preferred-root search had exactly one match and returned the selected
row's authored weather ID.

A separate scan retained all equal-key children across all declared roots
and all child types, then selected the first containing root in file order.
All 47 selected keys had aliases outside their city root:

| Selected city-root keys | Equal-key roots, in file order | Matches per key |
| ---: | --- | ---: |
| 101 | 101, 117, 126, 127 | 4 |
| 103 | 103, 106, 115, 122, 123 | 5 |
| 104 | 104, 107, 116, 124, 125, 130, 213, 214 | 8 |

These searches produced 276 matching child occurrences. All were type zero
and carried the same weather ID as their selected city row. No other child
type matched these keys. Every matching root had raw `+0x20 == 0`; the first
global owner was the supplied city root for every selected key. The CSV
retains each candidate root, child index, ID, type and file offset.

These are conditional static lookup results, reusing published native
formulas without opening or revalidating native bodies. They do not establish
actual native queue construction, provider identities, active zones, or
subarea inheritance. A global alias alone does not identify an active area.

## Evidence boundary

Authored type-zero membership includes 806x and event or other specialized
resources. Neither membership, token spelling nor an available DAT defines
an ordinary weather pool, eligibility, probability, calendar or subarea policy.
Header and identity checks do not establish dependency closure, compatibility,
every row's consumption, native instantiation, or visible weather. The
[six-payload direct-input study](city-weather-direct-inputs.md) separately owns
its limited outer-resource-table inventory.

No client, server, launcher or LuaVM ran. No payload dependencies or graphs
were traversed, and no media was extracted, decoded or played.

To reproduce, verify the linked source table identity and declared runs,
select the three roots' type-zero rows, and compare their locators and raw
fields with the CSV. Retain every equal-key candidate across all child types
before evaluating the published first-match formulas. Read only the 47
explicit numeric paths and compare full lengths, SHA-256 and the named header
fields with the catalog; do not follow payload dependencies.
