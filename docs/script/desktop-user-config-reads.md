# DesktopWidget user config reads

The DesktopWidget connector reaches the game user config store through a table
that maps 68 work indices to a Lua group and key. Its init method applies the
log font size, the UI lock, the popup help and the action help from that store,
and the log windows read their tab enable flags and channel colours from it.
The connector reads the font size itself and hands it to the log windows. The
installed scripts treat a record that is not the number 1 as off, and the log
font size treats any value but 1 or 2 as the small size. This page records
those reads. The store contents and defaults are owned by
[the user config store contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/user-config-store-contract.md).

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe`, as recorded on the cited tracked page |
| INFERRED | A role or ordering taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The method is the one of
[Work update callback consumers](work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f484` name the direct children of the chunk root in listing order.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `widget/logwidget` | `client/script/n1635q/yv3n1635q.le.lpb` | 19090 | `d99be24e3e64e9d105b25b722d119ee349c5f9244ddddf4e2e1f048c32355ad7` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `widget/desktopwidget_connector` | 213474 | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/logwidget` | 19077 | `f40fdae854b956352b045242defcfbedf964944bd2219dc621f9f802600b9a91` |

The tracked call-site index lists 11 `_getUserConfig` call sites in three
scripts, the connector, `widget/logwidget` and `widget/logcolorsettingwidget`
(`xivl-client-scripts:lua/napi_index.json`, API `_getUserConfig`).

## Work index map

OBSERVED. `getConfigCategory`, `main/f484` of the connector, is a chain of
equality tests on its argument. Each match returns a Lua group and key for
`_getUserConfig(group, key)`. The native subtracts 1 from each, so the pair is
the Lua-facing id of the store contract. The chain holds 68 entries. An index
outside them returns nil, and the setters and getters that call it return early
on that nil (`setConfigWork`, `main/f485`, and `getConfigWork`, `main/f488`).

| Lua group | Work index to Lua key |
|---:|---|
| 1 | The index equals the key for 1, 4 to 7 and 9 to 19 |
| 3 | 20 to 27 map to keys 1 to 8, 28 to 32 to keys 12, 15, 16, 17, 18, and 33 to 43 to keys 21 to 31 |
| 4 | 50 to 56 map to keys 1 to 7 |
| 5 | 60 to 63 map to keys 1 to 4 and 64 to key 9 |
| 7 | 70, 71 and 72 map to keys 1, 2 and 17, 73 to 80 to keys 4 to 11, 81 to 84 to keys 12 to 15, and 85 to key 16 |

Group 2 (channel colours) and group 6 (tab filters, titles and enable flags)
are addressed directly by key and are not in the table.

## Reads at desktop init

OBSERVED. `DesktopWidget.init`, `main/f0`, calls `updateLogFontSize` at PC 326
and `updateConfigWork` for work index 37 (PCs 328 to 330), 33 (PCs 331 to 333)
and 71 (PCs 334 to 336).

| Work index | Lua group and key | Store name | Consumer in `updateConfigWork`, `main/f486` |
|---:|---|---|---|
| 33 | 3, 21 | `popup_help` | `getStaticWidget(15):updatePopupHelp()` |
| 37 | 3, 25 | `ui_lock` | Sets the window property `IsDragResize` to the inverse of `getConfigFlag(37)`, then `getStaticWidget(15):updateUILock()` |
| 39 and 40 | 3, 27 and 3, 28 | `log_font_size1` and `log_font_size2` | `updateLogFontSize(39)` and `updateLogFontSize(40)` |
| 71 | 7, 2 | `action_help` | `getStaticWidget(15):updateActionHelp()` |

`getConfigFlag`, `main/f491`, returns true only when the work value equals the
number 1 (PCs 4 to 7). A nil or 0 value is false.

## Log font size

OBSERVED. `updateLogFontSize`, `main/f139`, calls `getLogFontSize(39)` and
writes the result to `getStaticWidget(2):setLogFontSize(size)`, and calls
`getLogFontSize(40)` for `getStaticWidget(3)`. `getLogFontSize`, `main/f140`,
starts with the size 12 (PC 0). A work value of 1 selects 14 (PC 6) and a value
of 2 selects 18 (PC 11). A nil or any other value keeps 12. `LogWidget.setLogFontSize`,
`main/f25` of `widget/logwidget`, passes the size to `setFontSize` for
`LogControl_Main_1` and `LogControl_Main_2`.

The store registers both font size records with default 1 (STATIC, store
contract, "Group 2, Lua group 3", Lua keys 27 and 28). A store at its
constructor defaults therefore selects size 14, and a read that returns nil
selects 12. INFERRED. The init reads the constructor defaults when no character
file has overlaid them, because the file overlay completes on a later read
callback (store contract, "Override order"). What a retail character file held
is NOT TRACED.

## Log tabs and channel colours

OBSERVED. `getConfigLogEnable`, `main/f503` of the connector, returns true only
when `_getUserConfig(6, 17 + index)` equals the number 1 (PCs 4 to 12), where
`index` comes from `getConfigLogIndex`. The log widget reads it at `main/f30`
PC 56 and `main/f31` PC 11 of `widget/logwidget`. The store registers the eight
enable records with defaults 1, 1, 0, 0, 0, 0, 0 and 0 (store contract, "Group
5, Lua group 6"). `setConfigLogEnable` writes 1 or 0 to the same keys.

OBSERVED. `LogWidget.setChatColor`, `main/f57` of `widget/logwidget`, builds the
text `#ff` followed by `_string.format("%x", desktopWidget:_getUserConfig(2,
channel))` (PCs 7 to 17), where `channel` is argument 1, or argument 1 plus
argument 2 minus 1 when argument 1 is 5 and argument 2 is not nil (PCs 0 to 6).
It writes that text to the `Foreground` of `TextBox_ChatInput` and
`TextBlock_Name`. The store registers all 32 channel colour records (STATIC,
store contract, "Group 1, Lua group 2"), so the retail read returns a number.
INFERRED. A nil from an unregistered record makes `_string.format` raise a Lua
error at this call, because `_string` holds the renamed standard `string`
library ([Lua bootstrap chunks](lua-bootstrap-chunks.md)) and the standard
`%x` conversion rejects a nil argument.

## Selected configuration controls

OBSERVED. The following widget contracts own the control operands, predicates,
call widths and immediate Lua consumers that complement the map above:

| Widget | Owning request contract |
|---|---|
| Nameplate | [Nameplate controls](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/config-nameplate-widget-contract.md#local-control-contract) records the work-index requests, toggle/selection writes, conditional slider writes and initialization reads. |
| Sound | [Sound configuration](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/config-sound-widget-contract.md) records the seven sound controls, conversions and configuration requests. |
| Camera | [Camera configuration](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/config-camera-widget-contract.md) records the camera and lock-on controls, conditional writes and initialization reads. |
| System | [System configuration](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/config-system-widget-contract.md) records nine toggle work-index requests and the direct index-64 selection read/write. |

These controls request user-work group 1 on the control. That group is separate
from the Lua configuration group returned by `getConfigCategory` and from the
native store's zero-based group/key coordinates. A later
`getControlUserWorkInt(1, control)` result supplies the work index used by the
getter or setter. The earlier setter request does not establish that native
getter result. The system contract's passing fixture deliberately returns 901
after requesting control work 1; the following request uses 901.

Conditional on that returned index resolving through the published map, the
map supplies the Lua store group/key. The widget contracts establish local
getter/setter requests and immediate control requests. They do not establish
native application, rendered or audio output, readback, or disk persistence.
The store's registration, coercion, defaults and overlay behavior remain owned
by the linked store contract.

## Open items

- NOT TRACED. Consumers beyond the selected widget request contracts above,
  and the native control-work getter result needed to join a requested
  control index to the store map in an actual invocation. Effective native
  application of those widget requests remains separate.
- NOT TRACED. Whether the character file overlay completes before the DesktopWidget
  init reads the store.
- NOT TRACED. The contents of a retail character file.

## Reproduction

Unwrap and list the two resources as in
[Work update callback consumers](work-update-callbacks.md#reproduction), check
the decoded sizes and hashes above, and read the prototypes named in the text.
The work index map is the equality chain of `main/f484`, where each tested
constant is followed by two `loadk` instructions that hold the group and key.
