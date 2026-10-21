# Package-5 item presentation boundary

The inspected stock 1.23b item-list route does not establish pending, lotted
and passed meanings for values `90`, `91` and `92`. Its Move, Transfer and
Waste actions are ordinary item command submissions. No inspected branch
joins those actions to a shared lot, a roll or a pass result.

The command tuples, eligibility and conditional `0x012D` representation remain
canonical in `xivl-client-scripts:docs/loot-item-ui-contract.md`, using
`xivl-client-structs:manifests/player_command_wire_contract.json`. This finding
adds the bounded presentation check, rather than another serializer contract.

## Native item information

The binary is retail `ffxivgame.exe`, build `2012.09.19.0001`, size
`15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. Addresses here are VAs.

The existing native contract distinguishes item tags from the incoming
record's mode and value bytes:
`xivl-client-structs:docs/bazaar-native-wire-contract.md#dealing-fields-read-by-lua`.
The focused original-byte recheck used pefile `2024.8.26` section mapping and
Capstone `5.0.7` x86-32 decoding:

| Reader and exact coverage | Observation |
| --- | --- |
| `_getDealingInfo`, `[0x00701E90,0x00701F20)` | Resolves the item, passes item `+0x8C` to `0x00763460`, then returns the resulting byte as a number or returns nil. |
| `[0x00763460,0x00763472)` | Supplies the literal key `0xC9` to `0x007633D0`. |
| `[0x007633D0,0x00763457)` | Walks byte keys and parallel byte values. An equal key copies its associated byte and returns true; exhaustion returns false. No `90/91/92` interpretation occurs. |
| `_getLockingInfo`, `[0x00701CB0,0x00701DD0)` | Uses the caller-supplied byte key in the same lookup. It has a separate key-3 fallback involving item `+0x89`; it does not classify `90/91/92` as lot states. |

These reads establish an item-info value domain, not the meaning of every
possible value. In particular, a tag-value byte must not be substituted for
wire `dealingValue` at record `+0x12` or `dealingMode` at `+0x13`.

## Lua consumers and actions

`ItemListWidget.makeDropItemList` selects package `5` and calls
`setItemToXmlLight` (`widget/itemlistwidget.lua:2905-3031`). That helper passes
the selected item to `DesktopWidget.setItemToXml` and `setItemDetailToXml`,
with row kind `4` for listbox `5`
(`widget/itemlistwidget.lua:1714-1909`).

The three complete widget chunks contain no numeric constant `90`, `91` or
`92` in their decoded Lua bytecode. The inspected renderer and item helpers
do not supply the missing lot-state binding:

| Resource and source locator | Observation |
| --- | --- |
| `widget/desktopwidget_itemdetail.lua:4-1394,1397-2299`, `setItemToXml` and `setItemDetailToXml` | Row kind `4` uses generic item presentation. Neither method reads dealing info or classifies `90/91/92`. The detail method derives `ex` from `isExclusiveItem`. |
| `widget/desktopwidget_connector.lua:18095-18159`, `getItemDealDataLocal` | Calls `_getDealingInfo()` and returns that scalar unchanged as its first result. It does not assign pending/lotted/passed meanings to `90/91/92`. |
| `item/itembaseclass_u.lua:114-131` | Declares the native dealing-info and attachment wrappers. |
| `item/itembaseclass_common.lua:4454-4508` | `isExclusiveItem` requests locking key `3`; `getWasteConfirmLevel` adds one when that predicate is true. This is a discard-confirmation level, not a recovered lot-state reader. |
| `widget/itemlistwidget.lua:4680-4722`, `operateGetDrop` | Resolves package-5 item and stack count, then calls `executePlayerItemMovePackage(5, slot, properPackage, count)`. |
| `widget/itemsubwidget.lua:336-362` | Get All, Give All and Trash buttons call the parent's `operateGetDrop`, `operateShareDrop` and `operateTrash`. |
| `widget/itemsharewidget.lua:203-384` | Self selection moves the item; another member transfers it; Breakup calls `executePlayerItemWaste(5, slot)`; Leave calls the move helper. Successful local submission changes or closes the widget. |

The shared helper routes are Move `24223`, Transfer `24225` and Waste `24226`.
A true helper return is not a recovered server settlement or lot result.
Trash first opens `ItemEditWidget`; its conditional waste call is already
covered by the canonical loot-item contract.

Decoded prototype locators use zero-based PCs in unluac's disassembly:
`ItemListWidget` `main/f29` assigns row kind `4` at PC `78` and calls the
two renderers at PCs `107/118`; `main/f51` calls Move at PC `21`.
`ItemShareWidget` `main/f4` calls self Move, Transfer, Waste and Leave Move
at PCs `83/104/145/198`. The renderer methods are
`desktopwidget_itemdetail` `main/f0` and `main/f1`; the dealing-info call is
connector `main/f410`, PC `16`. An unrelated literal `91` in the renderer
resource's sort-key method (`:3142`) tests a sort selector, not an item-info
value or lot state.

## Resource identity and method

Resource paths below are relative to retail `client/script`. Their raw size
and SHA-256, decoded payload identity and canonical source identity are pinned
by `xivl-client-scripts:manifests/retail_lua_coverage.json` and
`xivl-client-scripts:manifests/scripts.json` at revision
`39927f86fd8614102d4144cd98fe0eeab01e1e3a`. Each selected original, decoded
payload and reproduced source matched those pins.

| Script | Original LPB | Bytes |
| --- | --- | ---: |
| `widget/itemlistwidget.lua` | `n1635q/1q5xy1rqn1635q.le.lpb` | 51910 |
| `widget/itemsubwidget.lua` | `n1635q/1q5xrp8n1635q.le.lpb` | 3987 |
| `widget/itemsharewidget.lua` | `n1635q/1q5xr29s5n1635q.le.lpb` | 8315 |
| `widget/desktopwidget_connector.lua` | `n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 |
| `widget/desktopwidget_itemdetail.lua` | `n1635q/65rzqvun1635q_1q5x65q91y.le.lpb` | 68039 |
| `item/itembaseclass_common.lua` | `1q5x/1q5x89r57y9rr_7vxxvw.le.lpb` | 41430 |
| `item/itembaseclass_u.lua` | `1q5x/1q5x89r57y9rr_p.le.lpb` | 2426 |

Reproduction used `xivl-client-scripts:tools/retail_script.py:decode_lpb`
and the pinned `unluac_2025_12_23.jar`, SHA-256
`98be0fa84ac73ca66dce2842a2e4512226f4c611b6500dc96415571fc5538fcc`.
The numeric-constant check used its decoded-bytecode disassembly. Source
locators above use the manifest-matched canonical decompile, not original
Lua source.

## Confidence and remaining discriminator

The package and action arguments are direct static observations. The negative
result covers the three complete widget chunks, their named rendering/dealing
dependencies and the native ranges listed above. It does not prove absence
from all client code or from an unobserved runtime route.

The missing discriminator is a retail consumer or observed transaction that
joins a concrete item-info field and value to each proposed lot state and
joins Move/Waste to a lot/pass outcome. None was established in this scope.
The first-party manual's separate loot-list distribution description remains
in `xivl-captures:docs/mob-drops-party-loot-evidence.md`. Static UI branches do
not establish historical server distribution, winner selection or timeout.
