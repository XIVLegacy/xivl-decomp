# Armoire stored-item state

The retail client rebuilds a categorized catalog-ID cache from received bits.
Its stored-item Lua readers count and enumerate that cache with one-based
category and item arguments. The visible eligibility wrapper requires a
nonnegative helper result and matching category; no storage transaction was
established.

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
SetPlayerItemStorage to worker `0x006EFBD0`. The wrapper and packet framing
were not retraced here; the following observations start at that worker.

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
zero above that range. Complete packet extraction, unused-tail handling and
the complete mapping table remain separate from this worker contract.

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
The eligibility helper `0x00714440` remains uninspected, so its complete
acceptance domain is not recovered here. The inspected reader instructions
prepare results from the cache; no successful inventory or storage mutation
was established.

The actor and widget result contract is owned by
`xivl-client-scripts:docs/item-storage-widget-contracts.md`. In particular,
the selected item returned by a widget does not prove successful storage.

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

The first three ranges include their trailing alignment bytes. Selected
typed data totaled 49 bytes: the three vtable cells, four category-switch
pointers, one mapper pointer, one compact selector byte and four classifier
pointers. Other jump-table entries were not inspected. Static disassembly
does not establish a complete whitelist, item-name/category labels, server
transactions, persistence, ordering or historical runtime state.
