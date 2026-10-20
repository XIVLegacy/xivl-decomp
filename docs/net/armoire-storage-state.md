# Armoire stored-item state

The retail client rebuilds a categorized catalog-ID cache from received bits.
Its stored-item Lua readers count and enumerate that cache with one-based
category and item arguments. The visible eligibility wrapper requires a
mapped catalog ID and matching category. The full mapping contains 130
distinct items. Storage mutation remains a separate server transaction.

The [Patch 1.22 notes](https://forum.square-enix.com/ffxiv/threads/43599),
2012-04-25, `[dev1357]`, announced the inn-room Armoire, shared storage across
the three cities, full-condition and single-copy requirements, spiritbond
reset, and a Hermes' Shoes cooldown. The [Patch 1.23 notes](https://forum.square-enix.com/ffxiv/threads/50278),
2012-07-25, `[dev1457]`, added selected items. These are documented rules,
not proof that the native readers implement persistence or all eligibility
checks.

## Receiver and cache

The existing packet attribution in
`xivl-client-structs:data/vendor/opcodes/client_receivers.json`, `0x01A5`,
and `xivl-client-structs:manifests/data_dependency_overlay.json`, receiver
`0x0089CB90`, connects
SetPlayerItemStorage to worker `0x006EFBD0`. The opcode attribution is reused
from that catalog; the fixed-length constructor and apply path were checked
below.

`0x006EFBDD` selects the embedded container at player `+0x144`.
The worker clears each existing category's nested vector, then calls
`0x007222D0` with arguments 35, 65, 26 and 4 for native categories 0 through
3. The category switch at `0x006EFC76` uses four pointers at `0x006EFE60`;
the argument selection and calls are visible at `0x006EFC7D` through
`0x006EFCBB`. That helper was not inspected; these arguments do not establish
enforced capacities or a count of allowed items.

The input is a byte-vector argument. `0x006EFCCE` through `0x006EFD18`
reads its begin/end distance and tests each byte's bits from least to most
significant. For byte index `b` and bit index `k`, it computes `8*b+k` at
`0x006EFD21`, calls mapper `0x00714E40`, then classifies the returned
catalog ID through `0x007153D0`. It appends that catalog ID to the selected
category's four-byte-element vector, then visits each category and calls
`0x00720BB0` at `0x006EFE3A`. That helper and the growth/append helper were
not inspected; the capacity-available append is directly visible at
`0x006EFD7C` through `0x006EFD84`.

This worker loops over the supplied vector length; it contains no fixed
20-byte bound. Mapper `0x00714E40` accepts indices 0 through 153 and returns
zero above that range. Its complete table contains 130 nonzero results and
24 zero entries, at indices 126 through 149. The fixed-length receiver path
below supplies 32 bytes; only bits 0-125 and 150-153 map to catalog IDs.

`0x0075A730` pushes length 32 at `0x0075A75A` and passes its incoming byte
pointer to constructor `0x0089F110`. The constructor selects receiver `+4`
and computes `[start, start+length)` at `0x0089F146` through `0x0089F15C`.
`0x00531320` forwards this range to `0x00530CD0`, which clears the vector
pointer fields and passes the range to `0x008A1C60`. That final helper's
body was not inspected. The existing owned-range-copy finding is
`xivl-client-structs:manifests/structs.json`,
`ApplicationLuaScriptClientCommandNetworkEntrustItemReceiver`, `BCS-S-0443`.
The factory calls the receiver immediately at `0x0075A77A`; its apply body
forwards receiver `+4` to the cache worker at `0x0089CBB4`.

## Lua readers and eligibility

The cataloged PlayerBase dispatch offsets are `0x1EC` for `_canStoreItem`,
`0x1F0` for `_countStoredItem` and `0x1F4` for `_getStoredItem`
(`xivl-client-structs:manifests/napi_field_access_enhanced.json`). The
MyPlayer vtable at VA `0x00FD785C`, slots 123 through 125, contains
`0x007064D0`, `0x00706580` and `0x00706620`. The pointer cells at
`0x00FD7A48` through `0x00FD7A53` were checked against the executable;
the registration/stub attribution is reused from the catalog.

| Cataloged API target | Observed contract |
| --- | --- |
| `_canStoreItem`, `0x007064D0` | Subtracts one from category; calls `0x00714440(catalogId)` and requires a nonnegative result; compares `0x007153D0(catalogId)` with category minus one. The result is false unless both tests pass. |
| `_countStoredItem`, `0x00706580` | Subtracts one from category, selects a 0x10-byte category record using player `+0x148/+0x14C`, and returns the nested vector's `(end-begin)/4`, or zero for a null begin pointer. |
| `_getStoredItem`, `0x00706620` | Subtracts one from both category and index, selects the same category record, and returns the indexed four-byte catalog ID. |

The count/get paths check category and element bounds through calls to
`0x009D22B4`; its failure behavior was not inspected. The visible eligibility
wrapper reads no item condition, spiritbond or player stored-item vector.
The eligibility helper `0x00714440` is the inverse catalog-to-bit lookup.
Every nonzero mapping below round-trips to its original bit index. Its
nonnegative return domain is exactly the 130 listed catalog IDs; all other
32-bit input values return -1. The inspected reader instructions prepare
results from the cache; no successful inventory or storage mutation was
established.

The actor and widget result contract is owned by
`xivl-client-scripts:docs/item-storage-widget-contracts.md`. In particular,
the selected item returned by a widget does not prove successful storage.

## Complete item-bit mapping

Byte offsets start at the 32-byte bitstream's first byte. Bit positions
within a byte run from 0 to 7, least significant first. Lua category is
native category plus one. The mapped ranges are:

| Bits | Lua category | Items |
| --- | ---: | ---: |
| 0-34 | 1 | 35 |
| 35-99 | 2 | 65 |
| 100-125 | 3 | 26 |
| 150-153 | 4 | 4 |

Exact item-name joins and the authored category labels are recorded in
`xivl-client-data:docs/armoire-item-rows.md`.

Bits 126-149 and 154-255 have no catalog mapping. A set unsupported bit
maps to zero; the cache worker contains no zero-ID rejection before its
append path. The table does not grant ownership of any item.

| Bit index | Byte:bit | Catalog ID | Lua category |
| ---: | --- | ---: | ---: |
| 0 | 0:0 | 8013501 | 1 |
| 1 | 0:1 | 8032701 | 1 |
| 2 | 0:2 | 8051401 | 1 |
| 3 | 0:3 | 8071401 | 1 |
| 4 | 0:4 | 8081801 | 1 |
| 5 | 0:5 | 8013502 | 1 |
| 6 | 0:6 | 8032702 | 1 |
| 7 | 0:7 | 8051402 | 1 |
| 8 | 1:0 | 8071402 | 1 |
| 9 | 1:1 | 8081802 | 1 |
| 10 | 1:2 | 8013503 | 1 |
| 11 | 1:3 | 8032703 | 1 |
| 12 | 1:4 | 8051403 | 1 |
| 13 | 1:5 | 8071403 | 1 |
| 14 | 1:6 | 8081803 | 1 |
| 15 | 1:7 | 8013504 | 1 |
| 16 | 2:0 | 8032704 | 1 |
| 17 | 2:1 | 8051404 | 1 |
| 18 | 2:2 | 8071404 | 1 |
| 19 | 2:3 | 8081804 | 1 |
| 20 | 2:4 | 8013505 | 1 |
| 21 | 2:5 | 8032705 | 1 |
| 22 | 2:6 | 8051405 | 1 |
| 23 | 2:7 | 8071405 | 1 |
| 24 | 3:0 | 8081805 | 1 |
| 25 | 3:1 | 8013506 | 1 |
| 26 | 3:2 | 8032706 | 1 |
| 27 | 3:3 | 8051406 | 1 |
| 28 | 3:4 | 8071406 | 1 |
| 29 | 3:5 | 8081806 | 1 |
| 30 | 3:6 | 8013507 | 1 |
| 31 | 3:7 | 8032707 | 1 |
| 32 | 4:0 | 8051407 | 1 |
| 33 | 4:1 | 8071407 | 1 |
| 34 | 4:2 | 8081807 | 1 |
| 35 | 4:3 | 8012604 | 2 |
| 36 | 4:4 | 8012605 | 2 |
| 37 | 4:5 | 8012606 | 2 |
| 38 | 4:6 | 8012607 | 2 |
| 39 | 4:7 | 9040019 | 2 |
| 40 | 5:0 | 9040020 | 2 |
| 41 | 5:1 | 9040021 | 2 |
| 42 | 5:2 | 9030053 | 2 |
| 43 | 5:3 | 8012801 | 2 |
| 44 | 5:4 | 8012802 | 2 |
| 45 | 5:5 | 8012803 | 2 |
| 46 | 5:6 | 8012804 | 2 |
| 47 | 5:7 | 8012805 | 2 |
| 48 | 6:0 | 9050058 | 2 |
| 49 | 6:1 | 9050061 | 2 |
| 50 | 6:2 | 9050060 | 2 |
| 51 | 6:3 | 9050059 | 2 |
| 52 | 6:4 | 9050062 | 2 |
| 53 | 6:5 | 8032401 | 2 |
| 54 | 6:6 | 8032402 | 2 |
| 55 | 6:7 | 8032403 | 2 |
| 56 | 7:0 | 8032404 | 2 |
| 57 | 7:1 | 8032405 | 2 |
| 58 | 7:2 | 8032406 | 2 |
| 59 | 7:3 | 8032407 | 2 |
| 60 | 7:4 | 8032408 | 2 |
| 61 | 7:5 | 8032409 | 2 |
| 62 | 7:6 | 8032410 | 2 |
| 63 | 7:7 | 8051301 | 2 |
| 64 | 8:0 | 8051302 | 2 |
| 65 | 8:1 | 8051303 | 2 |
| 66 | 8:2 | 8051304 | 2 |
| 67 | 8:3 | 8051305 | 2 |
| 68 | 8:4 | 8051306 | 2 |
| 69 | 8:5 | 8051307 | 2 |
| 70 | 8:6 | 8051308 | 2 |
| 71 | 8:7 | 8051309 | 2 |
| 72 | 9:0 | 8051310 | 2 |
| 73 | 9:1 | 8013301 | 2 |
| 74 | 9:2 | 8013302 | 2 |
| 75 | 9:3 | 8013303 | 2 |
| 76 | 9:4 | 8013304 | 2 |
| 77 | 9:5 | 8012502 | 2 |
| 78 | 9:6 | 8032102 | 2 |
| 79 | 9:7 | 9030020 | 2 |
| 80 | 10:0 | 9040018 | 2 |
| 81 | 10:1 | 8032834 | 2 |
| 82 | 10:2 | 8032835 | 2 |
| 83 | 10:3 | 8032836 | 2 |
| 84 | 10:4 | 8032837 | 2 |
| 85 | 10:5 | 8032838 | 2 |
| 86 | 10:6 | 8032839 | 2 |
| 87 | 10:7 | 8051521 | 2 |
| 88 | 11:0 | 8051522 | 2 |
| 89 | 11:1 | 8051523 | 2 |
| 90 | 11:2 | 8051524 | 2 |
| 91 | 11:3 | 8051525 | 2 |
| 92 | 11:4 | 8051526 | 2 |
| 93 | 11:5 | 8081921 | 2 |
| 94 | 11:6 | 8081922 | 2 |
| 95 | 11:7 | 8010932 | 2 |
| 96 | 12:0 | 8050521 | 2 |
| 97 | 12:1 | 8012102 | 2 |
| 98 | 12:2 | 8013623 | 2 |
| 99 | 12:3 | 9030063 | 2 |
| 100 | 12:4 | 8013401 | 3 |
| 101 | 12:5 | 8013402 | 3 |
| 102 | 12:6 | 8013403 | 3 |
| 103 | 12:7 | 8013404 | 3 |
| 104 | 13:0 | 6010016 | 3 |
| 105 | 13:1 | 6020015 | 3 |
| 106 | 13:2 | 6030016 | 3 |
| 107 | 13:3 | 6040016 | 3 |
| 108 | 13:4 | 6050016 | 3 |
| 109 | 13:5 | 6060015 | 3 |
| 110 | 13:6 | 6070016 | 3 |
| 111 | 13:7 | 6080016 | 3 |
| 112 | 14:0 | 7010017 | 3 |
| 113 | 14:1 | 7020016 | 3 |
| 114 | 14:2 | 7030016 | 3 |
| 115 | 14:3 | 4100801 | 3 |
| 116 | 14:4 | 4100802 | 3 |
| 117 | 14:5 | 4100803 | 3 |
| 118 | 14:6 | 9030050 | 3 |
| 119 | 14:7 | 9030051 | 3 |
| 120 | 15:0 | 9030052 | 3 |
| 121 | 15:1 | 9050055 | 3 |
| 122 | 15:2 | 9050056 | 3 |
| 123 | 15:3 | 9050057 | 3 |
| 124 | 15:4 | 8013626 | 3 |
| 125 | 15:5 | 8013635 | 3 |
| 150 | 18:6 | 8010225 | 4 |
| 151 | 18:7 | 8012101 | 4 |
| 152 | 19:0 | 8070512 | 4 |
| 153 | 19:1 | 8080512 | 4 |

## One exact bit mapping

Input byte 0, bit 0 selects mapper index 0. The table cell at `0x00715168`
points to `0x00714E56`, which returns catalog ID `8013501` (`0x007A46BD`).
The classifier takes its compact-table path at `0x00715511`: the selected
byte at `0x00715833` is 2, selecting the pointer at `0x00715764` to
`0x0071554D`, which returns native category 0. The reader therefore exposes
this catalog ID in Lua category 1. This is a bit-to-catalog/category join,
not an English item-name mapping or an observed store operation.

## Evidence and limits

The input was retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE image base `0x00400000`. Capstone 5.0.7 decoded x86-32 instructions from
original PE section bytes mapped with pefile 2024.8.26. The selected code
ranges were:

| Inclusive VA range | Bytes |
| --- | ---: |
| `0x007064D0`-`0x0070657F` | 176 |
| `0x00706580`-`0x0070661F` | 160 |
| `0x00706620`-`0x007066FF` | 224 |
| `0x006EFBD0`-`0x006EFE5E` | 655 |
| `0x00714E40`-`0x00715164` | 805 |
| `0x007153D0`-`0x00715685` | 694 |
| `0x00714440`-`0x0071499D` | 1374 |
| `0x0089CB90`-`0x0089CBBC` | 45 |
| `0x0075A730`-`0x0075A7A2` | 115 |
| `0x0089F110`-`0x0089F17D` | 110 |
| `0x00531320`-`0x00531376` | 87 |
| `0x00530CD0`-`0x00530D47` | 120 |

The first three ranges include their trailing alignment bytes. Complete
lookup-table spans were `0x007149A0`-`0x00714E3F` (inverse),
`0x00715168`-`0x007153CF` (bit mapper), and
`0x00715688`-`0x00715A5F` (classifier), including table alignment. Together
with the three vtable cells and four category-switch pointers, the inspected
typed-data union was 2,812 bytes.

The table was recovered by following each of the 154 mapper entries to its
return, evaluating the classifier for each nonzero catalog ID, and checking
the inverse lookup returned the same bit. The inverse helper's 32-bit input
domain was partitioned along its comparison and table-index boundaries.
Its only accepted partitions were the 130 singleton catalog IDs in the table;
the other inputs reached the -1 return. This establishes the lookup domain,
not item condition, ownership, or successful storage.

Static disassembly does not establish item-name/category labels, server
transactions, persistence, ordering or historical runtime state. Native
helper calls beyond the selected range-construction path retain the limits
stated above.
