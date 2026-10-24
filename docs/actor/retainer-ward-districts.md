# Retainer market ward and district selection

The retail 1.23b client labels market districts with `xtx/placeName` keys.
`MarketEntrance.eventPushStepPrvMarket` returns the selected place key, rather
than its button ordinal. The bell's separate
`RetainerFurniture.eventTalkSelectBazaarStreet` returns two Lua numbers:
`(region, placeKey)`. Its three city branches select regions `202`, `204`,
and `205` and district-key bases `1261`, `2261`, and `3261` respectively.
Neither result is a world position or a zone identifier.

## Evidence identity and method

Script and sheet observations use extraction `2012.09.19.0001`.
Canonical script identities are keyed by `relativePath` in
`xivl-client-scripts:manifests/scripts.json`. Original LPB identities are keyed
by `decodedScriptPath` in
`xivl-client-scripts:manifests/retail_lua_coverage.json`. The selected original
LPBs, decoded payloads, and canonical Lua members matched those manifests.
The decoded payload SHA-256 values are listed below.

Original LPBs were decoded with
`xivl-client-scripts:tools/retail_script.py:decode_lpb` and inspected with
`xivl-client-scripts:tools/build_quest_contracts.py:ChunkReader` and
`instruction`. Prototype `0.N` means root child N. PCs are zero-based, and
byte offsets refer to the decoded Lua chunk. Bytecode establishes joined
conditions that the decompiled Lua does not render reliably.

| Member below `xivl-client-scripts:lua/scripts/` | Decoded payload SHA-256 |
| --- | --- |
| `chara/npc/object/marketentrance.lua` | `f2d08b15cadf12a03fc180898a2e63efa2f1343a991379561586b93c4f586100` |
| `widget/ask/marketselectwidget.lua` | `6a8fd4b2e830a4a22ff4f9150bf18cf40c7e9e49133a66a11b598417331f6158` |
| `widget/desktopwidget_connector.lua` | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/ask/askbaseclass.lua` | `a1d4129ca5779c3b2598da418909f057bd9b6c6733701855fd70bf2a9dcb1230` |
| `chara/npc/object/retainerfurniture.lua` | `4a8af6a781c1e364dbdba23eda8824845f4d05b2b755a30402f5b3e18b03cee7` |
| `chara/npc/object/retainermeetingspace.lua` | `d0a67ecaca6e77cf8579331321e7f76133a77815d0262778c159d7bc22f0f351` |
| `area/privatearea/privateareamasterbranch.lua` | `53662e5ac62aa6584ac16dcc84a4a45e030f584f8bc3006a69af301fa138866a` |
| `area/privatearea/privateareamastermarket.lua` | `da102dbfc5fbd60e2e2cc8b01d47994298977059377dc47c519af2719781224a` |

Native observations use retail `ffxivgame.exe`, PE32 x86, size `15996808`,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
preferred image base `0x00400000`. Addresses below are VAs. Direct file-backed
reads used pefile `2024.8.26` and Capstone `5.0.7` in x86-32 mode. These are
static observations, not target execution or a retail server exchange.

## Selector text and local result

The desktop wrapper opens `Ask/MarketSelectWidget` with widget kind `4`, calls
its `ask` method, yields for selection, reads `getAskResult` only when
`selectWidgetYield` returns boolean `true`, and closes the widget. It starts
with result `0`, which remains the result if the widget is absent or the
selection wait does not return `true`. See connector prototype `0.240`, PCs
`0-30`, offsets `0x1C48B-0x1C503`.

Both MarketEntrance calls supply this context:

```text
desktopWidget:askMarketSelectWidget(
    self, self, worldMaster, defaultAnswer, true,
    60002, answerTextKeys, unpack(answerPlaceKeys))
```

The wrapper forwards its second caller argument, `self`, as the widget's
first `ask` argument and `worldMaster` as its text owner. The default answer
is an ordinal. Text row `60002` says "Select a destination." Row `60007`
contains an `xtx/placeName` sheet reference, followed by a period in English.
The selected place key is a text parameter, rather than a distinct answer
text row. Sources: `xivl-client-data:csv/worldMaster.csv` rows `60002` and
`60007`, English position `1`; MarketEntrance `0.1`, PCs `87-99`, and `0.2`,
PCs `16-28`.

`MarketSelectWidget.ask` takes its answer count from the answer-text array.
It retains the default answer and cancel flag and applies the corresponding
vararg text parameters to `Button_Answer_i`. Its owner-text branch calls
`_setProperty(nil, button, "Content", owner, textKey, select(...))`.
Prototype `0.2`, PCs `0-17`, `50-83`, offsets `0x3FB-0x43F` and
`0x4C3-0x547`, establishes this construction. Initialization configures
answer buttons `1..24`; this is a UI control range, not a recovered number
of currently available districts (`0.0`, PCs `20-32`).

The widget's local results are exact:

| UI event | Result |
| --- | --- |
| `UILuaCommands.Operate` on `Button_Answer_i`, within `1..askAnswerMax` | Numeric ordinal `i` |
| `UILuaCommands.Cancel` or `UILuaCommands.WidgetClose`, with `canCancel == true` | Numeric `-3` |
| The same cancel/close events with `canCancel != true` | No result write on this path |

Prototype `0.3`, PCs `0-36`, offsets `0x6A5-0x735`, proves both cancel
events join at PC `26` before the `-3` write at PCs `30-32`. The recovered
Lua's apparent isolated WidgetClose branch must not be read as excluding
Cancel. AskBaseClass declares `askWork.askResult` as `integer16`, returns it
through `getBaseAskResult`, and treats nonzero as finished: prototypes
`0.0` PCs `2-5`, `0.5` PCs `0-2`, `0.7` PCs `7-9`, and `0.9-0.10`.
The default-answer and answer-count fields are separately `integer8`.
These local work declarations do not determine the event wire widths.

## City, region, and district identities

The bell first offers city place keys `1051`, `2001`, and `3051`.
Its branch then assigns the region and district base below.
RetainerFurniture prototype `0.11`, PCs `7-15` and `34-48`, offsets
`0x184E-0x186E` and `0x18BA-0x18F2`, establishes the assignments.
PrivateAreaMasterBranch independently uses the same region-to-mercantile-house
mapping for public-information text (`0.1`, PCs `0-26`,
`0x146-0x1AE`). English names come from
`xivl-client-data:csv/xtx_placeName.csv`, position `1`, at the listed keys.

| City place key and name | Script region | First district place key | Mercantile-house place key and name |
| --- | ---: | ---: | --- |
| `1051` Limsa Lominsa | `202` | `1261` | `1519` Lominsan Mercantile House |
| `2001` Gridania | `204` | `2261` | `2534` Gridanian Mercantile House |
| `3051` Ul'dah | `205` | `3261` | `3533` Ul'dahn Mercantile House |

The following are the complete contiguous named ranges beginning at those
three script bases. Each cell gives the place key and its English text.
The bell supplies `base..base+count-1` to the selector, where `count` is its
caller argument. The source does not fix that argument to `20`; existence
of these rows does not prove all twenty were offered in a particular event.

| Index | Limsa Lominsa | Gridania | Ul'dah |
| ---: | --- | --- | --- |
| 1 | `1261` Tinkerers Ward | `2261` Tinkerers Den | `3261` Tinkerers Row |
| 2 | `1262` Battlecraft Ward | `2262` Battlecraft Den | `3262` Battlecraft Row |
| 3 | `1263` Spellcraft Ward | `2263` Spellcraft Den | `3263` Spellcraft Row |
| 4 | `1264` Fieldcraft Ward | `2264` Fieldcraft Den | `3264` Fieldcraft Row |
| 5 | `1265` Tradescraft Ward | `2265` Tradescraft Den | `3265` Tradescraft Row |
| 6 | `1266` Armorfitters Ward | `2266` Armorfitters Den | `3266` Armorfitters Row |
| 7 | `1267` Lower Tailors Ward | `2267` Lower Tailors Den | `3267` Lower Tailors Row |
| 8 | `1268` Middle Tailors Ward | `2268` Middle Tailors Den | `3268` Middle Tailors Row |
| 9 | `1269` Upper Tailors Ward | `2269` Upper Tailors Den | `3269` Upper Tailors Row |
| 10 | `1270` Jewelers Ward | `2270` Jewelers Den | `3270` Jewelers Row |
| 11 | `1271` Grocers Ward | `2271` Grocers Den | `3271` Grocers Row |
| 12 | `1272` Chirurgeons Ward | `2272` Chirurgeons Den | `3272` Chirurgeons Row |
| 13 | `1273` Ironmongers Ward | `2273` Ironmongers Den | `3273` Ironmongers Row |
| 14 | `1274` Woodcutters Ward | `2274` Woodcutters Den | `3274` Woodcutters Row |
| 15 | `1275` Tanners Ward | `2275` Tanners Den | `3275` Tanners Row |
| 16 | `1276` Masons Ward | `2276` Masons Den | `3276` Masons Row |
| 17 | `1277` Clothiers Ward | `2277` Clothiers Den | `3277` Clothiers Row |
| 18 | `1278` Crystaliers Ward | `2278` Crystaliers Den | `3278` Crystaliers Row |
| 19 | `1279` Menders Ward | `2279` Menders Den | `3279` Menders Row |
| 20 | `1280` Cobblers Ward | `2280` Cobblers Den | `3280` Cobblers Row |

Source: `xivl-client-data:csv/xtx_placeName.csv` rows `1261..1280`,
`2261..2280`, and `3261..3280`, position `1`. Other named ward rows precede
these ranges, but neither this bell method nor PrivateAreaMasterMarket uses
them as its district base.

PrivateAreaMasterMarket's public-information label uses the same city base
plus the **second** `_getAreaType()` result minus one. Prototype `0.1`,
PCs `0-32`, offsets `0x146-0x1C6`, establishes the formula. PCs `33-39`
send the computed key with WorldMaster row `60003`, another `xtx/placeName`
reference. Region `203`, like an unrecognized region, retains the initialized
Limsa base on this path. That fallback is not a fourth established city.
The first `_getAreaType` result is not used in this label calculation.

The geographic sheets provide a different static join:

| City place key | Zone-group row | Listed zone IDs, positions `1..5` | Market-zone place key |
| ---: | ---: | --- | --- |
| `1051` | `101` | `133,230,134,232,140` | Zone `134` -> `1092` Market Wards |
| `2001` | `201` | `155,206,160,234,161` | Zone `160` -> `2096` Market Wards |
| `3051` | `301` | `175,209,180,233,181` | Zone `180` -> `3096` Market Wards |

Sources: `xivl-client-data:csv/zoneGroupParam.csv` rows `101/201/301`,
positions `0..5`; `xivl-client-data:csv/_zoneParam.csv` rows `134/160/180`,
position `0`; `xtx_placeName.csv` rows `1092/2096/3096`, position `1`.
The first two zones of each group share its city place key. The group row
and script region are different identifiers: for example zone-group row
`202` contains place key `2007` and zones `150/162`, while script region
`202` selects the Limsa market labels. Zone `204` itself carries place key
`1002`, not Gridania's district base. No inspected sheet row maps an
individual district place key to a unique zone or world position.

## Entry results and bell dispatch results

`MarketEntrance.eventPushChoiceAreaOrQuest` has six arguments after self.
Its local answer construction preserves their separate roles:

| Argument | Local use |
| --- | --- |
| `a1` | Append answer `(60007,a1)` when nonzero. |
| `a2` | Append `(60007,a2)` when nonzero, then append the mercantile-house destinations. |
| `a3`, `a4` | Append `(60007,value)` for each nonzero argument. |
| `a5` | Only boolean `true` adds Item Search Counter, row `60009`, with parameter `0`. |
| `a6` | When `a5 == true` and `a6` is neither `0` nor nil, also add Stop searching, row `60010`, with parameter `a6`. |

When building the `a2` branch, a current area of class
`PrivateAreaMasterBranch` suppresses its own mercantile-house option by
comparing `_getRegion()` with `202/204/205`. Otherwise all three house
options are appended. Prototype `0.1`, PCs `8-47`, offsets
`0x17C-0x218`, establishes this predicate. The method always appends
WorldMaster row `25003`, "Quit.", and defaults to answer `2` only when
both `a1` and `a2` are nonzero (PCs `70-79`). The script does not establish
the caller's semantic names for `a1..a4`.

After selection, this method returns:

| Selected branch | Return operands |
| --- | --- |
| Place option | One Lua number: the corresponding parameter from the parallel place-key array. |
| Item Search Counter | Call `askItemSearchWidget()`, discard its return, then return numeric `-2`. |
| Stop searching | Numeric `-1`. The selected item parameter is not returned here. |
| Quit | Numeric `0`, its stored parameter. |
| Cancel/close | The widget's numeric `-3`. |

Prototype `0.1`, PCs `99-150`, offsets `0x2E8-0x3B4`, establishes these
branches, including both search call sites at PCs `118-122` and `138-142`.
If the initial menu consists only of Quit, the loop is skipped and returns
the initialized ordinal `1`; do not interpret that exceptional result as
a place key. The desktop wrapper's `0` failure result can also propagate.

`MarketEntrance.eventPushStepPrvMarket(start,count,excluded)` constructs the
inclusive numeric range `start..start+count-1`, excluding any value equal
to `excluded`. On a positive in-range ordinal it returns the corresponding
range value. Other results, including `0` and `-3`, pass through unchanged.
Prototype `0.2`, PCs `4-14` and `28-37`, offsets `0x546-0x56E` and
`0x5A6-0x5CA`, establishes this mapping. Neither `start` nor `count` is
fixed to a city in this method.

The bell's `eventTalkSelectBazaarStreet(count)` instead opens the city menu,
builds `base..base+count-1`, then asks confirmation row `6337` through
`worldMaster:ask(self,nil,6337,2)`. The English row asks whether to dispatch
the retainer to the specified market ward and says it returns home if the
area has reached capacity. This is client text, not evidence of a capacity
check in the method. Source: `xivl-client-data:csv/xtx__text_ui.csv` row
`6337`, English position `1`.

Only a confirmation whose Lua type is `"number"` and whose value equals
`1` returns `(region,selectedPlaceKey)`. The return at prototype `0.11`,
PC `94`, offset `0x19AA`, has exactly two operands. The outer exit returns
numeric `(0,0)` at PCs `104-106`, `0x19D2-0x19DA`. A cancelled district
menu returns to the city menu; cancelling the outer city menu reaches
`(0,0)`. A successful example for city ordinal `2`, district ordinal `4`,
and numeric confirmation `1` is `(204,2264)`, not `(2,4)` or `(160,4)`.

## Continuation and ownership boundary

The first unbound destination edge is after these Lua returns. The inspected
entry and bell methods select text keys and return operands. They contain
no warp position, zone-instance allocation, retainer relocation operation,
or matching server reply. Their caller-supplied counts and range starts are
also not recovered retail event invocations. In particular, the confirmed
bell pair does not prove that the retainer moved, nor identify a destination
instance. The label formula using `_getAreaType`'s second result does not
prove how a server populates that result from a returned place key.

The generic native continuation has a known numeric representation.
`0x00894AB0` calls typed serializer `0x00790CE0` at `0x00894B44`, then
calls `0x00894090` at `0x00894B65`. The latter calls builder `0x0075E670`
at `0x0089410A`. That builder supplies opcode `0x012E` at `0x0075E700`,
internal size `0x68` at `0x0075E708`, and copies a 64-byte typed area at
`0x0075E714-0x0075E723`. In serializer `0x00790CE0`, the internal-tag-zero
branch writes wire tag `00` at `0x00790DC1`, writes four value bytes at
`0x00790DE4-0x00790DF5` or `0x00790DFA`, and advances by four at
`0x00790DFC`. Thus an integral numeric operand in that continuation is not
narrowed to the widget's `integer16` work field.

These fresh PE observations corroborate the generic continuation described
in [Summoned retainer item storage](../net/retainer-item-storage.md), section
"Transfer result values and widths". They do not establish a ward-specific
packet struct, fixed operand offsets, event identity, or a captured call of
these Lua methods. The two-number bell result and one-number entry result
remain exact at their Lua boundary. A concrete event invocation and its
destination consumer are needed to bind them to a particular server
continuation and subsequent area or retainer state.

## Separate retainer operations

`RetainerMeetingSpace.eventPushStepOpenRetainerMenu` reads group `80001`,
returns `0` for a missing group or count below two, opens
`askRetainerListWidget`, and maps the chosen non-player ordinal back to a
group-member index. A successful choice returns that member index, not a district key.
Source: `xivl-client-scripts:lua/scripts/chara/npc/object/retainermeetingspace.lua`
lines `76-131`, prototype `0.2`, PCs `0-43`, offsets `0x455-0x501`.
Its separate `getRetainer` method searches group `50003` for a live non-player
member (`0.1`, PCs `0-41`, offsets `0x2C3-0x367`). Neither group number supplies a
market district identifier.

Item search has its own market/type/item selections, documented in
`xivl-client-scripts:docs/market-search-bazaar-client-contracts.md`.
The entry method's `-2` marks its return after invoking that UI; it does not
return the search market field. Hiring previews and candidate city pools
belong to [Retainer presentation](retainer-presentation.md). Ordinary item
storage and transfer codes belong to
[Summoned retainer item storage](../net/retainer-item-storage.md).
Those paths do not establish ward IDs, available counts, destination
instances, or server dispatch policy for this selector.

## Sheet pins and reproduction

The following source SHA-256 values matched
`xivl-client-data:manifests/tables.json`:

| Source below `xivl-client-data:csv/` | SHA-256 |
| --- | --- |
| `xtx_placeName.csv` | `81467ef42e8aeba82fe95f6c4249356550c02e41e6734abf9fdd194051dc1714` |
| `worldMaster.csv` | `2e2e2dd5cd9651388f6fa575b4229081ad0452165e78b725df0a5b5b7cf7c643` |
| `zoneGroupParam.csv` | `e521adb118f8edc099fcf4171e905299b5b8c83c13c90fc62d46c646e80d8b23` |
| `_zoneParam.csv` | `1e75e434217e8d99848ac1d690a9fcd93e43c9a2b00fc983e3ba7fb592d377f9` |
| `xtx__text_ui.csv` | `23762a422118d99c4144a48b3495c52511b1b1b865e4fc50fdc61a973e3c6ece` |

To reproduce the script claims, select each coverage row by its full decoded
script path, authenticate the original `resourcePath` against its byte size
and SHA-256, decode it, and compare `wrapper.decodedPayloadSha256` with the
table above. Read the named prototypes and PCs. For sheets, CSV row keys are
the leading field; position `0` starts after that key. Check source bytes
against `manifests/tables.json` before selecting rows.

For the native check, map VA minus `0x00400000` through the PE section table
and disassemble bounded file-backed ranges at `0x00790CE0`, `0x00894AB0`,
`0x00894090`, and `0x0075E670` with the named tool versions. No gap-filled
listing or inferred native parameter type is needed for the cited calls,
opcode construction, copy size, tag write, and numeric width.
