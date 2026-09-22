# Seasonal event work control plane

This note covers the installed 2012.09.19.0001 FFXIV 1.23b client. The
`ffxivgame.exe` image base is `0x00400000`; its SHA-256 is
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The findings derive from pinned x86 disassembly and decoded client Lua. They
describe static client behavior, not a reconstructed historical server schedule.

## Player event work

Opcode `0x0196` has a `0x38`-byte packet. Its handler at `0x00576050`
expands payload byte `+0x01` into eight Boolean work fields (indices 1-8),
then reads eight little-endian `uint16` fields at payload `+0x02..+0x10`
(indices 9-16). The bulk setter is at `0x0075D2D0`. The getter registered at
`0x00707CC0` dispatches to byte helper `0x0075D390` for object offsets
`+0x84..+0x8B` and word helper `0x0075D3A0` for offsets `+0x8C..+0x9A`.
Thus `SpecialEventWork[9]` is the first server-supplied word, not a local
event detector.

The recovered Lua consumers use index 9; no recovered consumer requests
another index. Their explicit comparisons and effects are:

| Value | Decoded Lua locator | Supported effect |
| ---: | --- | --- |
| 8 | `chara/npc/populace/populacecompanyshop.lua:481` | Sets Grand Company shop `eventFlag` to 8. |
| 11 | `chara/npc/populace/populacecompanyshop.lua:483` | Sets that shop flag to 11. |
| 18 | `command/system/emotestandardcommand.lua:16`; `widget/emotelistwidget.lua:294` | Permits and lists emote 156 (Fire Dance). |
| 20 | `command/system/teleportcommand.lua:348`; `quest/scenario/etc/etc304.lua:107` | Changes teleport filtering and selects cutscene music ID 29. |

The company catalog applies `required <= eventFlag`: at 8 the Maelstrom,
Twin Adder, and Immortal Flames shops expose Storm Tracer (3020601), Serpent
Tracer (3020603), and Flame Tracer (3020602), respectively. At 11 they retain
those rows and each adds Patriot's Choker (9040018). These are catalog and
script effects, not evidence that a particular mode selected area weather.

## Separate area weather lane

Opcode `0x000D` controls area weather. Its native receiver and transition
fields are documented in [Weather transition runtime](../net/weather-transition-runtime.md).
The Lua `_setWeather` virtual thunk is at `0x0071E410`. Unlike player event
work, area weather can select DAT-authored decoration masks; see
[City seasonal weather selectors](../resource/city-seasonal-weather-selectors.md).

No recovered index-9 consumer calls the weather setter or a city background
scheduler. Mode 18 and summer weather 8029 are separate companion states:
the emote gate does not establish a weather trigger. The static sources
used here do not establish which combinations, timing, or initial
states the historical retail server sent to a particular player and area.

## Evidence and limits

The packet and getter claims come from the pinned executable at the addresses
above. The index-9 effects come from the decoded Lua locators above, cross-
checked against the decoded Grand Company catalog rows 102001/102002,
202001/202002, and 302001/302002. The source extraction produced the
`seasonal-control-plane-decomp-atlas-20260711` packet, work-layout, consumer,
and shop-row tables; its output is an aid to review, not independent runtime
evidence. Neither static lane proves that the retail server activated any
specific seasonal combination at a particular time.
