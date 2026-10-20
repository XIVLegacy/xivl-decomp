# Grand Company approval and state delivery

The officer approves only when its supplied `a3` is Lua boolean true. The
recovered officer methods do not compute it from seals, rank or quest work.
The generic s2c `0x0130` named-function route can deliver that argument. The
selecting GC invocation and authoritative approval producer remain unbound.
Numeric `1` does not satisfy the officer's `a3 == true` check.

The separate s2c `0x0194` route writes allegiance and three rank bytes into
the same `PlayerBase` fields read by the native company getters. It supplies
neither the approval boolean nor a seal balance. The available client
surfaces establish how to publish state and present a result; they do not
establish an original server transaction or its packet order.

## Evidence grades and identity

- **A-native:** instructions or data from the identified retail executable.
- **A-Lua:** authenticated retail script consumers, conditional on invocation.
- **A-data:** literal static client rows, without a grant or debit claim.
- **A-wire:** an identified retained packet observation.
- **I:** inferred client intent, without a recovered producer.
- **U:** an unbound producer, historical invocation or server rule.

Native input is Windows 1.23b `ffxivgame.exe`, extraction
`2012.09.19.0001`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses below are VAs. Original PE section reads
used pefile `2024.8.26` and Capstone `5.0.7` in x86-32 mode. Retained Ghidra
assembly was compared byte-for-byte with the PE for the receiver, setter and
registration bodies. The getter ranges were decoded directly from that PE.
These observations are static, not a live execution or retail server trace.

The script identities, method instructions and existing promotion consumers
are owned by:

- `xivl-client-scripts:docs/gc-promotion-client-contract.md:14-128`.
- `xivl-client-scripts:docs/gc-enlistment-promotion-evidence.md:30-132`.
- `xivl-client-scripts:docs/job-grand-company-event-contracts.md:467-520`.

## Incoming approval argument

The [generic named-function decoder](start-event-fn-receiver.md#trade-routing-through-generic-runeventfunction)
publishes the s2c Map `0x0130` route:
`0x004D8860 -> 0x00575040 -> LuaActorImpl slot 57 -> 0x0076C220`.
Offsets refer to the application payload, after the internal message prefix:

| Application offset | Width | Consumer |
| --- | --- | --- |
| `+0x00` | u32 | Trigger binding key |
| `+0x04` | u32 | Owner binding key |
| `+0x08` | u8 | Event type |
| `+0x09` | 32 bytes | Event-name field |
| `+0x29` | 32 bytes | Requested function-name field |
| `+0x49` | 64 bytes | Typed parameter input |

The [native supplied-frame contract](../script/delegate-command-producer.md#function-binding-value-and-supplied-frame)
establishes the conditional direct call frame `(F,O,D0,D1,...)`: requested
function `F`, selected binding's Lua value `O`, then materialized input values
in order. It does not silently insert a player argument, event name or event
type into that parameter vector. Nil occupies a position. Binding resolution,
readiness, typed conversion, callable lookup and coroutine state remain
conditions; receiving the packet alone does not prove execution.

For a direct request of the officer method, the positional mapping is:

| Function name | Explicit values after `self=O` | Approval position |
| --- | --- | --- |
| `eventTalkJoined` | `D0=a1,D1=a2,D2=a3,D3=a4` | Third supplied value, `D2`; forwarded unchanged to the local choice helper |
| `eventRankUpChoice` | `D0=a1,D1=a2,D2=a3` | Third supplied value, `D2` |
| `eventDoRankUp` | `D0=a1,D1=a2` | No approval argument consumed |
| `eventRankUpDone` | `D0=a1,D1=a2` | No approval argument consumed |

This is an **A-native/A-Lua conditional mapping**, not an observed GC packet.
An outer `delegateCommand` request has a different inner argument mapping;
its method key is itself a supplied value. Do not apply the direct table to
such an envelope without resolving its selected inner callable.

The [typed decoder](../script/delegate-command-producer.md#typed-values-and-bounds)
uses one-byte wire tag `03` for Lua true and `04` for false. Tag `00` plus
four bytes yields a number, so a numeric `1` is distinct. Tag `05` supplies
nil; `0F` ends the stream without adding an argument. The third value has no
universal byte offset inside the 64-byte area because earlier typed values
can have different encoded lengths. A sender must encode an ordered typed
vector, not write a fixed boolean field at `+0x4B`.

`eventTalkJoined` calls `eventRankUpChoice(a1,a2,a3)` on menu result 1.
The choice returns true only for macro result 1 and boolean `a3 == true`.
It reads belonging company and the company's seal stack for prompt formatting;
it does not compare that stack with a cost or read a prerequisite quest to
produce the boolean. **A-Lua.** The upstream meaning of `a3` remains **U**:
affordability, quest completion and aggregate eligibility are not interchangeable
facts established by the local check.

Refusal rows 39/40/41 explicitly describe insufficient company seals. That
supports seal sufficiency as the intended meaning of the affirmative-choice
flag (**I**), rather than a demonstrated prerequisite-quest flag. It still
does not recover a cost comparison, caller assignment or authoritative rule;
an upstream combined check could produce the same value. The authenticated
`populaceCompanyOfficer.csv` is 79,229 bytes, SHA-256
`dc88d368c2595796df15fe07629815ab381b0968b9b43b8a1c765c8f52f0fe36`,
at `xivl-client-data:manifests/tables.json:4395-4400`. English is data position
1 after the row ID; physical lines 40-42 hold these three refusal rows.

c2s Map `0x012D` EventStart and `0x012E` EventUpdate are client requests and
continuations, not s2c carriers for this input. The generic c2s continuation
is `0x00894AB0 -> 0x00894090 -> 0x0075E670`; the builder sets `0x012E`
at `0x0075E700`. See [the generic continuation](../net/retainer-item-storage.md#transfer-result-values-and-widths)
and `xivl-opcodes:opcodes.json`, `MapServerbound` entries `0x012D/0x012E`.
No officer-specific event name, event type, binding pair or returned packet
has been established merely by this generic transport.

## Allegiance and rank publication

**A-native.** The receiver binding is
`xivl-opcodes:data/client_receivers.json:329-369`: s2c Map `0x0194`,
`GrandCompanyReceiver`, central dispatcher `0x004DC690`, LuaActorImpl slot 71.
The slot target `0x0075A400` reads four successive application bytes at
`0x0075A42A-0x0075A43C`, constructs the receiver through `0x0089CC90`, and
calls `0x0089CD60` at `0x0075A45A`.

Constructor `0x0089CC90` preserves the four-byte order: stack inputs at its
adjusted `ESP+0x18/+0x1C/+0x20/+0x24` are stored at receiver `+4/+5/+6/+7`
by `0x0089CCCD/0x0089CCD4/0x0089CCE1/0x0089CCE4`. It installs receiver
vtable `0x01057318` at `0x0089CCDB`. The adapter's reverse pushes and these
four stores establish application bytes 0-3 at receiver bytes 4-7; the layout
does not rely on the constructor name alone.

`0x0089CD60` casts the actor control from `ActorBase` to `PlayerBase`, loads
receiver bytes `+4..+7`, and dispatches through the resulting object's vtable
offset `+0xA4` at `0x0089CD81/0x0089CDA1`. This is virtual slot 41, not an
object field at `+0xA4`. The original `PlayerBase` vtable cell `0x00FD5EA8`
contains `0x006DEB00`; the catalog independently records that slot in
`config/ffxivgame.vtable_slots.jsonl`, class
`Application::Lua::Script::Client::Control::PlayerBase`, slot 41.

The setter's four stores and the getter bindings give this application layout:

| Application byte | `PlayerBase` destination | Getter consumer |
| --- | --- | --- |
| `+0` | `+0xED`, store `0x006DEB08` | `_getBelongGrandCompany()` |
| `+1` | `+0xEE`, store `0x006DEB12` | `_getGrandCompanyRank(1)` |
| `+2` | `+0xEF`, store `0x006DEB1C` | `_getGrandCompanyRank(2)` |
| `+3` | `+0xF0`, store `0x006DEB22` | `_getGrandCompanyRank(3)` |

The allegiance getter at `0x00706CC0` sign-extends `byte[this+0xED]` at
`0x00706CC1`. Its registration body `0x0074F380` supplies that target at
`0x0074F3B7` and the name string `_getBelongGrandCompany` at `0x00FD7F20`,
referenced by `0x0074F469`; registration reaches `0x00CCCAD0` at
`0x0074F499`.

The rank getter at `0x00706CF0` selects input company 1/2/3 through
`0x00706D2A-0x00706D37`, then sign-extends fields `+0xEE/+0xEF/+0xF0`
at `0x00706D4B/0x00706D42/0x00706D39`. Other selector values reach the
epilogue without that value-output call. Registration `0x0074F4D0` supplies
the target at `0x0074F507`, the `_getGrandCompanyRank` name at
`0x0074F5B9` (string `0x00FD7F38`), and calls registration at `0x0074F5E9`.
The Lua wrapper's sentinel handling remains owned by
`xivl-client-scripts:docs/gc-promotion-client-contract.md:66-77`.

No bit in these four bytes is identified as officer approval. The setter
writes the full bytes and does not debit inventory, invoke an officer method
or refresh a widget. This establishes client state storage, not the historical
time of a rank change. A publication can also replay existing state during
login or area setup. The packet replaces all four fields; a sender needs a
complete desired snapshot, including the other companies' ranks, rather than
a rank delta or an assumed all-zero unselected state.

### Reproducible native ranges

These half-open code ranges were read from the pinned executable. The
registration bodies and receiver/setter also matched retained Ghidra assembly.

| Entry / role | Code range | Bytes |
| --- | --- | ---: |
| `0x0075A400`, incoming four-byte adapter | `[0x0075A400,0x0075A483)` | 131 |
| `0x0089CC90`, ordered receiver construction | `[0x0089CC90,0x0089CD01)` | 113 |
| `0x0089CD60`, receiver dispatch | `[0x0089CD60,0x0089CDA7)` | 71 |
| `0x006DEB00`, state stores | `[0x006DEB00,0x006DEB2B)` | 43 |
| `0x0074F380`, allegiance getter registration | `[0x0074F380,0x0074F4CA)` | 330 |
| `0x0074F4D0`, rank getter registration | `[0x0074F4D0,0x0074F61A)` | 330 |
| `0x00706CC0`, allegiance getter | `[0x00706CC0,0x00706CE5)` | 37 |
| `0x00706CF0`, rank getter | `[0x00706CF0,0x00706D6F)` | 127 |

The dynamic-cast type descriptors at `0x012BFA48` and `0x01270964` carry
`.?AVPlayerBase@Control@Client@Script@Lua@Application@@` and
`.?AVActorBase@Control@Client@Script@Lua@Application@@` at descriptor `+8`.
`config/ffxivgame.rtti.json:1525` pins the PlayerBase vtable identity;
`config/ffxivgame.vtable_slots.jsonl` owns its slot targets. A derived object's
actual virtual target remains conditional on its runtime vtable.

## Seals and post-approval presentation

The seal balance is an item count, separate from `0x0194`:

| Company | Seal catalog ID | Lua package / native package |
| ---: | ---: | --- |
| 1 | 1000201 | `100 / 99` |
| 2 | 1000202 | `100 / 99` |
| 3 | 1000203 | `100 / 99` |

The seal reader scans Lua package 100 and returns the matching stack count;
the [published presentation](grand-company-promotion.md#seal-reader) and
`xivl-client-scripts:docs/gc-promotion-client-contract.md:66-98` own those
consumers. Package conversion subtracts one, as documented in
[the package contract](../net/retainer-item-storage.md#package-request).

The existing [inventory publication contract](../net/retainer-item-storage.md#refresh-and-packet-ordering)
provides `0x016D`, package selector `0x0146`, full item records
`0x0148..0x014C` and/or removals `0x0152..0x0156`, then `0x016E`.
A changed seal stack belongs to that item publication path; a rank byte or
status-widget points setter cannot replace it. This is a client-compatible
surface, not a witnessed GC debit sequence. No dedicated seal-debit opcode or
promotion-specific ordering is established here.

The officer's `eventDoRankUp(a1,a2)` presents its supplied values, effect and
status surface; `eventRankUpDone(a1,a2)` sets displayed join status and points,
salutes and closes it. The exact dialogue row groups and effect calls are in
`xivl-client-scripts:docs/gc-promotion-client-contract.md:14-64`.

| Actor class / company | `eventDoRankUp` rows | Approval-refusal row / other-answer row |
| --- | --- | --- |
| `1500199 / 1` | `33,34` | `39 / 42` |
| `1500200 / 2` | `35,36` | `40 / 43` |
| `1500198 / 3` | `37,38` | `41 / 44` |

These are client text-sheet row IDs in bank `6797`,
`populaceCompanyOfficer`, rather than global system-message or packet IDs.
They do not identify a seal-debit log message. The done method has no dialogue
row. In the rank-up presentation, `a2` supplies the effect's rank value and
`a1` supplies the status setter; in completion, `a1` supplies the status
setter and `a2` supplies displayed points. Their authoritative values and
producer remain unbound.

`0x0194` does not itself call either method. The server-side choice of method,
the point balance passed to completion, and their order relative to inventory
publication remain **U**.

## Official enlistment

The separate `Com0l7`, `Com0g7` and `Com0u7` client paths open the official
join widget with company 1, 2 or 3. The accepted branch presents that company
and literal rank 11, then updates the status surface. These are presentation
calls, separate from the native state-publication route. **A-Lua.** The
authoritative join,
initial-rank write and seal grant remain **U** without their selecting event
and state publications.

The two widget results are `(selectionSucceeded,answer)`. Acceptance requires
the first to equal boolean true and the second to equal numeric 1. The client
then continues its local presentation and returns 1 from the start method;
failed selection or another answer follows the other branch and returns 0.
This is not a separate server approval tuple. The selected method's eventual
generic event continuation remains distinct from that local widget result.

`DesktopWidget.askEventModeWidgetYield` forwards to
`askWidgetYield(self,4,widgetName,resultCount,...)`. For these calls,
`resultCount=1` and the init vararg is the company number. The helper selects
the widget, collects its result only after selection returns exact true,
closes it and returns `true` plus that one answer, or false on failure.
The official join widget's operate handler stores its supplied command value
unchanged through `AskBaseClass`; its cancel handler focuses `Button_No`.
The inspected Lua bodies establish local result propagation, without a named
membership transaction or a separate GC request at widget acceptance.

| Company / start method | Widget CALL PC / decoded offset | Two-result tests | Rank-11 effect CALL |
| --- | --- | --- | --- |
| 1, `Com0l7.processEventGuincamStart` | `94 / 0x318` | `95,97 / 0x31C,0x324` | `110 / 0x358` |
| 2, `Com0g7.processEventFulkeStart` | `99 / 0x328` | `100,102 / 0x32C,0x334` | `115 / 0x368` |
| 3, `Com0u7.processEventAubreyStart` | `86 / 0x2F6` | `87,89 / 0x2FA,0x302` | `102 / 0x336` |

Each is root child 1 in its decoded Lua 5.1 chunk; PCs are zero-based and
offsets count bytes. The source identities are pinned at
`xivl-client-scripts:docs/job-grand-company-event-contracts.md:483-495`.

`xivl-client-data:docs/grand-company-reward-rows.md:1-25` records seal item
and count-shaped values 1000 at quest rows `111407/111607/111807`. The
separate tutorial rows `111402/111602/111802` contain 250. These are
**A-data**, not observed grants after widget acceptance. Neither a displayed
rank 11 nor the reward row supplies the missing enlistment transaction.

### Widget consumer identities and locators

The tracked source identities are in `xivl-client-scripts:manifests/scripts.json`
and `xivl-client-scripts:manifests/retail_lua_coverage.json`. Each original LPB,
decoded payload and canonical member matched those records. The canonical
paths below identify reproduction products; raw decompiled bodies remain
untracked. This contract promotes the specific consumers and instruction
locators rather than citing a private source tree.

| Member below `lua/scripts/`; manifest row | Original LPB resource | Decoded payload SHA-256 |
| --- | --- | --- |
| `widget/desktopwidget_connector.lua`; scripts 15399, coverage 25454 | `n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/ask/grandcompanyofficialjoinwidget.lua`; scripts 14925, coverage 26399 | `n1635q/9rz/3s9w67vxu9wlv441719y0v1wn1635q.le.lpb` | `ad9878a8813c925dd88fc63a4022fbc33d215902a549438924e427f69bd9a485` |
| `widget/ask/askbaseclass.lua`; scripts 14877, coverage 26519 | `n1635q/9rz/9rz89r57y9rr.le.lpb` | `a1d4129ca5779c3b2598da418909f057bd9b6c6733701855fd70bf2a9dcb1230` |

Prototypes below are direct root children. PCs are zero-based; decoded byte
ranges are half-open. The seven complete method bodies contain 72 words.

| Method | Canonical lines | Root child / PCs | Decoded byte range |
| --- | --- | --- | --- |
| `DesktopWidget.askEventModeWidgetYield` | 25790-25799 | `541 / 0-7` | `[208441,208473)` |
| `DesktopWidget.askWidgetYield` | 25805-25854 | `542 / 0-37` | `[208542,208694)` |
| `GrandCompanyOfficialJoinWidget.processUICommandOperate` | 74-82 | `2 / 0-3` | `[858,874)` |
| `GrandCompanyOfficialJoinWidget.processUICommandCancel` | 85-93 | `3 / 0-3` | `[936,952)` |
| `AskBaseClass.getAskResult` | 79-86 | `5 / 0-3` | `[1062,1078)` |
| `AskBaseClass.setBaseAskResult` | 98-112 | `7 / 0-9` | `[1227,1267)` |
| `AskBaseClass.getBaseAskResult` | 132-139 | `9 / 0-3` | `[1540,1556)` |

## Retained wire boundary

**A-wire.** The source universe is all 54 members of
`xivl-captures:sources/pcap-1.23b/manifest.yaml`, SHA-256
`baaa15f99f7f5d865bf2ec0dbcc1e454d034382b0559edccb0085c57c4061cf5`.
Every file's size and hash matched its member record. The ordered member
digest is `1293490808399dd5e1015d921a388f0fe4931dfa3ef678f30d790f5296025e6a`:
sort member filenames, then hash each ASCII filename, NUL and the 32 decoded
SHA-256 bytes in sequence. This pins the source set rather than a server rule.

The admitted game streams contain 200 complete s2c `0x0130` applications,
each 144 bytes, across 34 direct function names. Exact-name checks found zero
of the following in the function field `[0x29,0x49)`, typed area
`[0x49,0x89)`, or whole application, including possible inner delegate names:

- `eventTalkJoined`, `eventRankUpChoice`, `eventInformationRankUp`,
  `eventDoRankUp`, `eventRankUpDone`.
- `processEventGuincamStart`, `processEventGuincamEnd`.
- `processEventFulkeStart`, `processEventFulkeEnd`.
- `processEventAubreyStart`, `processEventAubreyEnd`.

The three quest pairs' exact spelling is recorded at
`xivl-client-scripts:lua/registry.json:23432-23435,23608-23611,23795-23798`.
Raw whole-string matching in the typed area remains valid for this exact-name
question despite 42 records with unsupported tags in the auxiliary typed-value
parser. Computed method selection, other event paths, excluded protocols and
unrecorded sessions are outside this negative.

There are 14 complete s2c `0x0194` observations. All are main-lane wrapped
sub-events of size 40, with 24-byte inner bodies. The first 16 inner bytes
are the message prefix; application bytes 0-3 contain the state below. The
following four application bytes are zero in these records, without an
additional semantic assignment.

State **A** is `(allegiance,rank1,rank2,rank3)=(2,127,11,127)`, with transport
source/target `0x029B2941 -> 0x029B2941`. State **B** is `(2,127,27,127)`,
with transport source/target `0x029B27D3 -> 0x029B2941`. These are different
actor-pair observations, not a same-player transition from 11 to 27.
Packet indexes below are one-based completing capture packets. Frame/subevent
indexes are zero-based within the admitted connection and s2c direction.
Manifest line spans locate each member's size and SHA-256 in the source above.

| Capture | Packet | Frame / subevent | State | Source manifest lines |
| --- | ---: | --- | --- | --- |
| `from_gridania_to_blackshroud.pcapng` | 768 | `245 / 8` | A | 99-101 |
| `gridania_to_coerthas.pcapng` | 602 | `184 / 8` | A | 114-116 |
| Same | 2865 | `854 / 8` | A | 114-116 |
| Same | 4019 | `1328 / 8` | A | 114-116 |
| `login.pcapng` | 970 | `17 / 8` | A | 135-137 |
| `move_out_of_room.pcapng` | 95 | `40 / 8` | A | 144-146 |
| `moving_around_gridania.pcapng` | 475 | `211 / 8` | A | 147-149 |
| Same | 3574 | `1259 / 8` | A | 147-149 |
| `party_battle_leve.pcapng` | 5445 | `3453 / 32` | B | 153-155 |
| `return_to_inn.pcapng` | 147 | `47 / 8` | A | 165-167 |
| `teleport_to_camp_nine_ivies.pcapng` | 315 | `81 / 8` | A | 177-179 |
| `teleport_to_camp_tranquil.pcapng` | 221 | `55 / 8` | A | 180-182 |
| `teleport_to_gridania.pcapng` | 189 | `67 / 10` | A | 183-185 |
| `war_quest_update2.pcapng` | 841 | `639 / 29` | B | 189-191 |

Grouping by capture, admitted connection and transport actor pair yields no
rank change. The 12 state-A and two state-B packets are partial GC state
evidence; none witnesses the named enlistment widget, officer approval,
seal debit or a GC promotion transaction.
`xivl-opcodes:data/vendor/captures/payload_layouts.json:25499-25612` records
13 samples, and `xivl-opcodes:opcodes.json:4893-4917` omits login from its
`observedIn` list. This 54-member census records 14; the source coverage of
those counts remains explicit.

### Capture reproduction and limits

The producing parser entry points are
`xivl-captures:tools/validate_framing.py:46`,
`xivl-captures:tools/extractors/extract_00da_00e1_comparison.py:185,276-349`
and `xivl-captures:tools/extractors/extract_local_guildleve_dialogue.py:109`.
Authenticate and sort the 54 manifest members; run `validate_framing.check`
before `_decode_capture`; inspect `application_bytes(event)` for the selected
s2c opcodes. For `0x0130`, compare the NUL-terminated function field and
whole-string ASCII tokens in the two byte ranges above. For `0x0194`, read
the first four application bytes, retaining connection, transport actor pair,
completing packet and direction-local frame/subevent identity. Do not join
different captures into a promotion sequence.

All 54 framing checks passed with no compression-flag mismatches. Coverage is
84 admitted game lanes and 10,790 s2c frames. The parser discarded 228 trailing
s2c stream bytes at the canonical frame boundary. Those bytes and excluded
lobby/TLS protocols are not a complete-message negative. No concrete GC name
selected a neighboring c2s `0x012D` follow-up. No new retail capture is implied.

## Consumer promotion and missing discriminator

The client-backed contract supports separate operations: typed callback
delivery, allegiance/rank state publication, seal-stack publication and UI
presentation. Only boolean true can satisfy the supplied approval input.
The state and presentation surfaces do not establish an atomic transaction.

The unresolved discriminator is a selecting invocation or original server
producer that binds the officer object, function name and argument vector to
the approval decision, then identifies its continuation. To distinguish seal
sufficiency from quest eligibility, that evidence must show the tested inputs
and resulting boolean; an incidental rank snapshot or refusal text alone
cannot do so. The enlistment boundary additionally needs the accepted widget
result's continuation and corresponding membership/rank and inventory writes.
