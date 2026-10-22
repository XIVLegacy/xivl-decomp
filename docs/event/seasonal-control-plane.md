# Seasonal event work control plane

Player special-event work and area weather use separate client control paths.
The decoded Lua uses the former for content gates; city layouts use the latter
for weather-conditioned decoration visibility.

## Player event work

Opcode `0x0196` has a `0x38`-byte packet. Its handler at `0x00576050`
expands payload byte `+0x01` into eight Boolean work fields (indices 1-8),
then reads eight little-endian `uint16` fields at payload `+0x02..+0x10`
(indices 9-16). The bulk setter is at `0x0075D2D0`. The getter registered at
`0x00707CC0` dispatches to byte helper `0x0075D390` for object offsets
`+0x84..+0x8B` and word helper `0x0075D3A0` for offsets `+0x8C..+0x9A`.
Thus `SpecialEventWork[9]` is the first server-supplied word, not a local
event detector. The
[player profile work accessors](player-profile-work-accessors.md) page records
the flag bit order, the getter value tags and its missing range check.

The recovered Lua consumers use index 9; no recovered consumer requests
another index. Their explicit comparisons and effects are:

| Value | Decoded Lua locator | Supported effect |
| ---: | --- | --- |
| 8 | `chara/npc/populace/populacecompanyshop.lua:1084` | Sets Grand Company shop `eventFlag` to 8. |
| 11 | `chara/npc/populace/populacecompanyshop.lua:1093`; `widget/ask/questdetailwidget.lua:1062-1072`; `widget/ask/questrewardwidget.lua:645-655` | Sets that shop flag to 11; both widget `isFestival` methods return true for 11. |
| 18 | `command/system/emotestandardcommand.lua:40`; `widget/emotelistwidget.lua:948` | Permits and lists emote 156 (Fire Dance). |
| 20 | `command/system/teleportcommand.lua:541`; `quest/scenario/etc/etc304.lua:446` | Changes teleport filtering and selects cutscene music ID 29. |

The company catalog applies `required <= eventFlag`: at 8 the Maelstrom,
Twin Adder, and Immortal Flames shops expose Storm Tracer (3020601), Serpent
Tracer (3020603), and Flame Tracer (3020602), respectively. At 11 they retain
those rows and each adds Patriot's Choker (9040018). These are catalog and
script effects, not evidence that a particular mode selected area weather.

## Separate area weather lane

Opcode `0x000D` controls area weather. Its native receiver and transition
fields are documented in [Weather transition runtime](../net/weather-transition-runtime.md).
The [Lua `_setWeather` bridge](../net/weather-transition-runtime.md#lua-weather-id-setter)
dispatches through thunk `0x0071E410` to MyPlayer's ID setter. Unlike player event
work, area weather can select DAT-authored decoration masks; see
[City seasonal weather selectors](../resource/city-seasonal-weather-selectors.md).

No recovered index-9 consumer calls the weather setter or a city background
scheduler. Mode 18 and summer weather 8029 are separate companion states:
the emote gate does not establish a weather trigger. The static sources
used here do not establish which combinations, timing, or initial
states the historical retail server sent to a particular player and area.

## Server orchestration boundary

The recovered inputs are compatible with a server-owned event controller.
A calendar window and a settings override that forces an event on or off
would select server policy, then supply the applicable client inputs:

| Client input | Established control | Remaining content boundary |
| --- | --- | --- |
| Player event work, `0x0196` | Supplies the flags and words consumed by recovered Lua. | A mode does not select area weather or prove a complete event state. |
| Area weather, `0x000D` | Selects a region weather resource and transition duration. | Decoration conditions and atmosphere depend on the installed DAT version. |
| Map-object binding and BG scheduler requests, `0x00D8/0x00D9` | Address a layout instance and request a short scheduler name through the [native binding path](../resource/limsa-fireworks-scheduler-bindings.md#native-binding-and-request-path). | Limsa's static path does not identify a historical retail controller or activation policy. |

Organizing that policy in one event-named server Lua module is a server
design choice. The client paths do not prescribe a script directory,
calendar dates or settings format. They provide separate controls that the
server would coordinate; there is no recovered universal event switch.

Invoking a control already authored in the installed resources does not
require replacing its DAT. This does not establish complete seasonal events
from server scripts alone. In the final client, the resident Halloween
layout conditions select weather 8027, while historical Starlight payloads
and selectors changed across patches. Scheduling those inputs does not
restore an overwritten atmosphere or recover missing placements and retail
activation records. Those remain event-specific evidence or content gaps.
The resource-specific evidence and limits are in
[City seasonal weather selectors](../resource/city-seasonal-weather-selectors.md)
and [Seasonal BG-object assets](../resource/seasonal-bgobj-assets.md).
The [stock component bindings](../resource/stock-seasonal-component-bindings.md)
separately qualify manual decoration controls and possible reuse of existing
weather layouts; missing historical whole files do not prove missing leaves.

## Evidence and limits

The packet and getter claims come from `ffxivgame.exe` (image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`)
at the addresses above. The index-9 effects come from the decoded Lua
locators above in the pinned `xivl-client-scripts` corpus
(`manifests/private_lua_corpus.json`, archive SHA-256
`0e8f902f7a2f592fc1220d41b89a3f35ec395cfb261806d4bd590a530099ae31`;
member hashes in `manifests/scripts.json`), cross-checked against the decoded
Grand Company catalog rows 102001/102002, 202001/202002, and
302001/302002. Neither static lane proves that the retail
server activated any specific seasonal combination at a particular time.
