# Raise and warp offer timeout evidence

The retained 1.23b capture corpus supplied no identified ConfirmRaise or
ConfirmWarp offer, clear, or offer-to-clear interval on the queried `0x0137`
property route. It therefore supplied no unanswered expiry witness or timeout
duration. Original Lua bound work-driven question closure, but no offer
countdown or timer-driven answer/closure in the selected widget paths.

The [publication contract](confirm-work-publication.md) owns the nine work
declarations, derived hashes and conditional incoming-writer composition.
`xivl-client-scripts:docs/confirm-prompt-client-contracts.md` owns prompt
construction and the outgoing answer tuple. Neither a pending getter nor an
answered offer establishes a timeout.

## Original capture query

The input was the complete 54-member `pcap-1.23b-objects.zip`, 2622720 bytes,
SHA-256 `20a78b9f40ff2393037c9a160c957783cf590b4f01797493d22fcd2039e9cbff`.
`xivl-captures:config/retail_inputs.json` pins that archive;
`xivl-captures:sources/pcap-1.23b/manifest.yaml` pins every member. Archive
membership, all member lengths/digests and cache equality were checked before
decoding. Tool revision was `xivl-captures` commit
`82f1783f98fd03db656d193a6394439a3e1b32f5`.

The producing query used these existing Python functions, with each capture,
TCP connection and direction kept separate:

1. `xivl-captures:tools/validate_framing.py`, `check`, checked compression
   flags before decoding. `tools/extractors/extract_streams.py`,
   `reconstruct_lanes`, `parse_outer_frames` and `maybe_inflate`, reconstructed
   port-54992 game lanes and decoded complete outer bodies. All sub-event
   boundaries were consumed exactly. TLS and port-54994 lobby traffic were
   outside this game-route query, including in `login.pcapng`.
2. Every s2c wrapped-actor `0x0137` sub-event had size 168, with its 136-byte
   application block at sub-event offset 32. `parse_records` in
   `tools/extractors/extract_property_stream_catalog.py` and
   `parse_property_block` in `tools/extractors/extract_gam_keys.py` agreed on
   each declared region, hash, value width and raw value. The declared region
   was fully consumed; remaining padding was zero.
3. Exact equality against all nine hashes in the publication contract found
   zero records. `xivl-client-structs:tools/verify_murmur2.py` recomputed the
   full `playerWork.<field>` hashes and passed all six embedded vectors.
   A second query searched every complete decoded game body, both directions,
   for each four-byte hash in little- and big-endian order: zero matches.

| Query scope | Observation |
| --- | ---: |
| Original captures / admitted game connections | 54 / 84 |
| Complete outer frames, both directions | 19760 |
| s2c `0x0137` packets / typed property records | 2014 / 9307 |
| Distinct property hashes / contributing captures | 263 / 37 |
| Nine confirmation hashes, typed-record matches | 0 |
| Nine confirmation hashes, raw LE/BE matches | 0 |

`login.pcapng` contributed 22 packets and 200 properties; the other 53
captures contributed 1992 packets and 9107 properties. The input list came
from the 54-member source manifest, not a scenario-name filter or the older
53-capture scope description in the property study.

## Pairing and timestamp boundary

| Offer kind | Identified offers / clears | Offer frame and UTC | Clear frame and UTC | Interval | Cause |
| --- | --- | --- | --- | --- | --- |
| ConfirmRaise | 0 / 0 | N/A | N/A | N/A | Unclassified: no endpoints |
| ConfirmWarp | 0 / 0 | N/A | N/A | N/A | Unclassified: no endpoints |

There were no identified endpoints to pair with an answer, timeout, zone
change or state change. A capture ending, an unrelated property update or a
teleport scenario name was not treated as a clear. The negative depends on
the publication contract's derived identifiers and the retained complete
game-route bytes; it does not authenticate all possible incoming encodings.

The following are coverage controls for the four previously queried travel
captures and login, **not offer-to-clear intervals**. Each tuple is
`outer-frame/sub-event/body-offset; capture-packet`. Frame and sub-event
indices were zero-based within connection 0, main lane, s2c; packet numbers
were one-based within the named original capture. Paths below are beneath
`xivl-captures:sources/pcap-1.23b/objects/` and use its manifest's member pins.

| Capture | `0x0137` packets / records | First property packet: locator; UTC | Last property packet: locator; UTC |
| --- | ---: | --- | --- |
| `teleport_to_camp_nine_ivies.pcapng` | 26 / 253 | `81/18/2152; 315`; `2012-12-31T02:27:04.337938Z` | `87/18/1896; 323`; `2012-12-31T02:27:04.804172Z` |
| `teleport_to_camp_tranquil.pcapng` | 29 / 244 | `45/1/48; 211`; `2012-12-31T01:32:07.769846Z` | `62/30/3136; 232`; `2012-12-31T01:32:08.380085Z` |
| `teleport_to_gridania.pcapng` | 24 / 218 | `22/17/1752; 91`; `2012-12-31T01:37:35.564989Z` | `72/1/296; 198`; `2012-12-31T01:37:47.624207Z` |
| `return_to_inn.pcapng` | 21 / 203 | `47/19/2488; 147`; `2012-12-31T01:44:01.726228Z` | `51/14/1472; 153`; `2012-12-31T01:44:02.085730Z` |
| `login.pcapng` | 22 / 200 | `17/20/2528; 970`; `2012-12-31T01:19:52.564675Z` | `88/0/0; 1276`; `2012-12-31T01:20:32.504831Z` |

Producing tools were `_packet_spans` in
`xivl-captures:tools/extractors/extract_login_0002_appearance_order.py` and
`_frame_completion` in
`xivl-captures:tools/extractors/extract_0193_clock_contract.py`, joined to the
frame/sub-event walk above. UTC came from the first capture packet completing
all bytes of the outer frame, using IP-declared TCP extents. The opaque outer
timestamp was not interpreted as an offer clock. Separate captures do not
form continuous observation time.

## Static question teardown

All locators below refer to original Lua payload disassembly and the
reproduced `lua/scripts/` source. Prototype indices are zero-based immediate
children of the root; code offsets are decimal bytes in the decoded chunk,
and PCs are zero-based instructions. The identity table below fixes the
original LPBs. Producing tools were `xivl-client-scripts:tools/retail_script.py`
and pinned unluac source/disassembly, rather than a live client trace.

| Original script and locator | Observation |
| --- | --- |
| `chara/player/playerbaseclass.lua:1289-1315`, `root/proto30`, code `12067`, CALL PCs `6/12`; `widget/desktopwidget_connector.lua:6930-6944/7014-7028`, `root/proto113/119`, code `76426/77074` | `confirmWarpCommand` / `confirmRaiseCommand` work notifications call the corresponding desktop refresh, which calls `updateMainMenuWidget`. |
| `widget/desktopwidget_connector.lua:9595-9618`, `root/proto170`, code `95002` | Refresh calls `updateReadyCommand` and updates static widgets 1 (MainMenu) and 15 (ConsoleIconTray). |
| `widget/mainmenuwidget.lua:940-946/1799-1833`, `root/proto7/15`, code `7687/13375`, CALL PCs `1/28`; `widget/consoleicontraywidget.lua:724-744`, `root/proto8`, code `7408`, CALL PC `3` | MainMenu `update` calls `updateMainMenu`, which calls `updateSystemCommand`. Tray `update` calls `updatePushButton`. These are the refresh-to-close-consumer edges. |
| `widget/mainmenuwidget.lua:1274-1707`, `root/proto12`, code `9821`, PCs `4-35` | Nil Raise variation **or 40004**, stored command `24306`, and an existing CommonAsk child lead to `closeChildWidget` (CALL PC `33`) and stored-command zero (PC `35`). |
| Same MainMenu prototype, PCs `113-142` | When reached, nil Warp variation, stored command `24304`, and an existing CommonAsk child lead to `closeChildWidget` (CALL PC `140`) and stored-command zero (PC `142`). |
| Same MainMenu prototype, PCs `36-57` | `isMyPlayerDead()` true plus stored command greater than zero closes an existing CommonAsk child (CALL PC `54`), clears the stored command and returns. This exact state predicate is separate from the variation tests. |
| `widget/consoleicontraywidget.lua:915-1341`, `root/proto12`, code `8943`, PCs `338-388` | Existing CommonAsk plus false `helpFlag[askType]` leads to `closeWidgetDirect(child)` (CALL PC `387`). Warp uses ask type 3; Raise uses 5. The same prototype excludes nil Raise / Raise 40004, and only populates Warp in the non-dead branch. |
| `widget/commonaskwidget.lua:117-133`, `root/proto3`, code `1388`, CALL PCs `4/8` | The separate answer/cancel path calls the parent's `processAskResult(answer)`, then `closeWidgetDirect(self)`. |

The variation-zero getter behavior from the publication contract composes
with these refresh/close branches: a delivered work clear can request closure
of the matching open question. The MainMenu and tray update branches above
do not call the answer handler or submit a synthetic answer in their Lua.
This is static compatible teardown, not an observed historical clear, expiry
or native widget-close callback trace. Stored-command zero is conditional on
the existing child in the cited MainMenu branches.

## Timer consumers and limits

| Original script and locator | Timer observation |
| --- | --- |
| `widget/commonaskwidget.lua:11-114`, `root/proto0/1/2`, code `319/840/1173`; complete script 133 lines | Init installs question, answer list, default answer, modal/cancel conditions. Operate parses an answer; Cancel supplies 0; after-show restores focus. No offer age/deadline, timer start, timer callback or delayed default answer occurs. |
| `widget/widgetbaseclass.lua:57-193/393-415`, `root/proto0/16/17/18`, code `828/3832/3917/3987` | Base schema declares `widgetWork._temp.commonTimer`; setter writes `widgetWork.commonTimer`. `_onTimer` forwards to empty `processTimer`. No selected confirmation caller arms it. The apparent `_setLoopInterval(0.1)` at proto0 PCs `26-28` is skipped by the initial jump. |
| `widget/consoleicontraywidget.lua:581-614/915-1341`, `root/proto4/12`, code `6168/8943` | `TimerFinish` sends balloon-help window/arrow OFF. `TimerStart` in `updatePushButton` follows balloon-help display. Neither timer branch answers or closes CommonAsk. |
| `widget/mainmenuwidget.lua:63-169/776-814/1212-1271`, `root/proto0/3/11`, code `958/6234/9347` | `RaptureCommands.TimerEnd` and `setDezionTimer` operate on MainMenu item 15; the latter reads `getWarpRecastTime()` and writes recast display fields. This is a warp-recast consumer, with no CommonAsk lifetime edge in these methods. |
| `widget/consoleicontraywidget.lua:2057-2079`, `root/proto29`, code `17829` | `openAskWidget` stores ask type and opens the CommonAsk child if absent; it starts no timer. |

No countdown duration was bound to these offers by the selected Lua. This
does not prove that any historical expiry was server-only, exclude a native
form behavior, or turn generic timer plumbing into an offer deadline.

## Original script identity

All seven LPBs matched sizes/digests and their decoded payload/source
identities in `xivl-client-scripts:manifests/retail_lua_coverage.json` and
`xivl-client-scripts:manifests/scripts.json` at commit
`91b13060e4fd3a016bde576833babf14775fae37`. Source reproduction and raw
disassembly used `unluac_2025_12_23.jar`, SHA-256
`98be0fa84ac73ca66dce2842a2e4512226f4c611b6500dc96415571fc5538fcc`.
LPB paths are relative to `client/script/`.

| Script basename | Original LPB | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `playerbaseclass` | `729s9/uy9l5s/uy9l5s89r57y9rr.le.lpb` | 25846 | `32182274f3886d0cdf6329f8a82e6592790c9b3038e00bebe5b2b38702bbd53e` |
| `playerbaseclass_cliprog` | `729s9/uy9l5s/uy9l5s89r57y9rr_7y1usv3.le.lpb` | 2058 | `1d41560f5750be6c9a6903c489cb535f05296674a08548afda44437c83b06f53` |
| `commonaskwidget` | `n1635q/7vxxvw9rzn1635q.le.lpb` | 1559 | `cad77ceafd51dfc6b59da750b7beb01d2d25f3641ba2e52ded2d198964c56b9c` |
| `widgetbaseclass` | `n1635q/n1635q89r57y9rr.le.lpb` | 4080 | `aad807a42296e93301582defc8f15e4003b8093af672794282cd54950cc556ad` |
| `mainmenuwidget` | `n1635q/x91wx5wpn1635q.le.lpb` | 19491 | `c0e1782a3ca183fcaa520a16320946f62d3fc861c74978cde19ea20fcd064b0a` |
| `consoleicontraywidget` | `n1635q/7vwrvy517vwqs9ln1635q.le.lpb` | 18579 | `892ac23eb20269dc847572c7a9877f31fb7766137d76b32f1a7e6ca671bd2af5` |
| `desktopwidget_connector` | `n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |

## Remaining discriminator

A retained retail transaction must identify the receiving actor and nonzero
Raise or Warp variation, preserve continuous bidirectional traffic while the
offer remains unanswered, and include its eventual clear/withdrawal plus
enough contemporaneous UI/state evidence to attribute teardown. Keep the
recipient in the same zone and relevant state, record no answer/cancel,
disconnect or offer replacement, and retain packet timestamps through and
beyond the disappearance. A separate answered control should bind its answer
and clear. A server zero publication and client question closure would then
be distinguishable from an answer, zone/state change or capture ending.

Until that witness exists, expiry duration, countdown presentation and the
server's withdrawal policy remain unestablished.
