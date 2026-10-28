# City airship scene sequence

Recovered `DftSrt.eventDeparture` calls
`startFadeOutCutSceneDefault`, then `startNQCutScene(departure, 1)`, then
`startNQCutScene(arrival, 1)` when an arrival key is present, and finally
`startFadeInCutSceneAfterWarp`. The calls are ordered in the client script;
the method itself does not encode a six-route table or world landing XYZ.
`DirectorBaseClass.delegateEvent` forwards the invoking owner after the
player argument when it calls the delegated method. This is why the scene
keys are the later arguments to `DftSrt.eventDeparture`, rather than fields
on a `PopulaceFlyingShip` actor.

## Playback return and event correlation

`QuestBaseClass.startNQCutScene` creates a CutScene with the quest/scenario
owner, calls `startCutScene(1, 61, selector, ...)`, receives two results,
deletes the scene only when the first result is exactly `true`, and returns
the second result. For `eventDeparture`, the selector is the literal `1`.
The common CutScene method calls `_play` in play mode 1 and returns its
result pair after its local UI/map-load handling. The playing actor is also
the target of an accepted skip prompt. The
`xivl-client-scripts:docs/cutscene-replay-skip-contract.md`, section
`Skip actor binding`, owns that UI contract.

The original LPB bytecode establishes where the return values stop:

| Method | Original decoded-chunk locator, zero-based PC | Observation |
| --- | --- | --- |
| `DftSrt.eventDeparture`, `root/proto2` | PC 9, `0x217`; PC 15, `0x22F` | Both scene calls use `CALL A=6 B=4 C=1`, requesting no results. |
| Same method | PC 19, `0x23F` | `RETURN A=0 B=1` returns no values after the fade-in helper. |
| `QuestBaseClass.startNQCutScene`, `root/proto1` | PC 10, `0x79E`; PCs 11-18, `0x7A2`-`0x7BE` | Receives the pair, tests the first against `true`, conditionally deletes, and returns the second. |
| `CutScene.startCutScene`, `root/proto8` | PC 235, `0x20BD`; PC 287, `0x218D` | `_play` supplies two values; the helper returns the pair. |
| Skip-widget callback, `root/proto2` | PCs 0-7, `0x2DC`-`0x2F8` | Ask result 1 and a nonnil bound actor reach that actor's `_skip()` call. |

These are proven discarded returns: neither scene result becomes an
explicit return operand from `DftSrt.eventDeparture`. The
[native completion path](../net/ferry-scene-notice-producer.md#enclosing-request-completion)
can forward a generic request's result container to `0x012E`, including an
empty result. Its association with a particular incoming airship invocation
and the two created scene actors remains unbound. The method's empty return
does not distinguish natural completion, accepted skip or interruption.

The [native scene notice](../net/ferry-scene-notice-producer.md) establishes
an actor-local `_play` waiter, skip flag and result callback. Its `0x00CE`
body has a lifecycle selector and scene name, with four copied bytes whose
value is unbound. No traced field is established as a starting server event
ID, event name, trigger/owner actor ID, sequence number or invocation token.
In particular, the callback's result-record `+0x20` code and `+0x28` dword
are local result operands, not authenticated event or sequence identifiers.

The [StartServerOrderEventFunction receiver](start-event-fn-receiver.md)
owns the incoming trigger/owner keys, event type/name, function name and
parameter vector. Its generic native frame is a separate endpoint. The
inspected scene/result path does not bind those fields to this pair or to
the notice's four trailing bytes. The
[server-order talk admission](server-order-talk-admission.md) exchange also
does not supply that missing correlation.

No client-side stale/duplicate filter for a booking reply was established.
The local request/result association does not prove a booking generation,
and the reply context dword's domain remains unresolved. The scene name
and unassigned notice tail are not established invocation discriminators.
Server-side correlation may use retained request/session state,
but no particular algorithm or generation scheme is recovered here.

`PopulaceFlyingShip` has separate attendant-menu returns, detailed below.
These are menu answers. They are not departure/arrival playback completion,
cancellation or skip reports. No inspected retail caller joins them to an
exact ordered pair of city scene keys.

## Attendant menu and callback operands

The recovered `PopulaceFlyingShip` body initializes text source
`(7620, "populaceFlyingShip")`. Its `eventIn` signature has five named
parameters including the receiver: `(attendant, player, arg2, arg3, arg4)`.
It replaces `arg3` with `player:getMoneyOnHand()` before opening the menu.
Below, `money` denotes that result. The English fare text identifies `arg4`
as the supplied fare. This method does not compare the money with that fare,
check an inventory, remove a ticket or deduct gil.

The method starts `attendant:startCliantTalkTurn(2, player)`, obtains
`attendant:getActorClassId()`, initializes three destination flags to true,
and disables one flag for each of these actor classes (`root/proto1`
PCs 0-30, `0x219`-`0x291`):

| Attendant actor class | Choice 1: Limsa Lominsa | Choice 2: Gridania | Choice 3: Ul'dah |
| --- | --- | --- | --- |
| `1500003` | false | true | true |
| `1500055` | true | false | true |
| `1500208` | true | true | false |
| Any other value | true | true | true |

It says row 2, then enters the outer
`worldMaster:ask(attendant, attendant, 3, 3)` menu. Calls below use Lua
colon notation: the receiver before the colon is implicit and every
argument inside the parentheses is explicit. The English rows 4-6 label
the outer choices as booking passage, learning about services, and doing
nothing. Destination rows 12-15 label choices 1-4 as Limsa Lominsa,
Gridania, Ul'dah, and nowhere. `WorldMaster.askRestrictChoices` builds rows
as the supplied base row plus the original flag index and maps the selected
visible index back to that original index
(`xivl-client-scripts:lua/scripts/world/worldmaster_event.lua:159-233`).
The mask therefore preserves city choice numbers when the current city's
entry is omitted.

| Outer ask result | Reached client branch |
| --- | --- |
| `1`, with `arg2 == true` | `attendant:say(attendant, 30, 0)` acknowledges an unused ticket, then destination selection. |
| `1`, with `arg2 == nil` | `attendant:say(attendant, 29, 0, money, arg4)` presents insufficient funds, then finish the talk turn and return nil. |
| `1`, with any other `arg2` | `attendant:say(attendant, 10, 0, money, arg4)` asks the destination, then destination selection with a gil-fare confirmation. |
| `2` | Say rows 7 and 8 about the service and special passes. Only actor class `1500208` then calls `_runCharaScheduler(70795264)` and says row 9 about the mythril pass. Repeat the outer menu. |
| Any other result, including nil | Finish the talk turn and return nil. |

The exact true and nil comparisons are at PCs 44 and 52. The other branch
includes false and every nonnil value other than true. The money and
`arg4` operands support presentation of those supplied values, not a local
payment decision. Row 30 acknowledges an unused ticket, while row 8
separately describes special passes for free travel on specified routes.
The text does not turn the `arg2 == true` branch into a check for such a
pass. Ticket ownership, the caller's classification of this argument and
payment acceptance remain outside this method.

Destination selection calls
`worldMaster:askRestrictChoices(attendant, attendant, 11, flag1, flag2, flag3, true)`
at PC 79 (`0x355`) and saves the result as `choice`. Choice 4 or nil returns
to the outer menu (PCs 80-83). Every other returned value reaches the
confirmation presentation. The method does not independently restrict that
value to 1, 2 or 3 after the ask call.

For `arg2 == true` it says `(attendant, 31, 0, choice)`. Otherwise it says
`(attendant, 16, 0, choice, arg4, money)`. Both branches then call this exact
confirmation tuple at PC 116 (`0x3E9`):

```text
worldMaster:askMultipleTextMacro(attendant, attendant,
    1, 17, 3, 1, true, true, true, choice, 0, 0, 0)
```

| Confirmation result | Reached client branch |
| --- | --- |
| `1` (row 18, yes) | Say `(attendant, 21, 0)` directing the player to the landing area, finish the talk turn, and return the saved destination `choice`. |
| `3` (row 20, no) | Repeat the outer menu. |
| Any other result, including `2` (row 19, change destination) and nil | Repeat destination selection. |

The original instructions distinguish the two answers: destination selection
is saved in R8, confirmation is saved in R9, and `RETURN A=8 B=2` at PC 126
(`0x411`) returns the former after the confirmation equals 1. The nil tail
finishes the talk turn and returns one nil value at PCs 160-163
(`0x499`-`0x4A5`). The method contains no departure timer, scheduled wait,
boarding invitation callback, scene call or warp call. Its sole scheduler
call is the actor-specific outer-menu-2 branch above. This does not determine
whether a called native yields or whether the server schedules a later event.

### Exit callback and selector 30010

`eventOut` has signature `(attendant, player, arg2)` and starts the same
talk-turn call. The original `EQ` at `root/proto2` PC 4 (`0x667`) compares
`arg2` with the literal `30010`, so the special branch's Lua argument shape
is `(attendant, player, 30010)`. This is a selector comparison within the
callback, not a returned scene result or an authenticated event ID.

| Reached condition | Ordered calls in colon notation |
| --- | --- |
| `arg2 == 30010` | `attendant:say(attendant, 23, 0)`; `worldMaster:say(attendant, 24)` |
| Any other `arg2` | `attendant:say(attendant, 32, 0)` |
| Both branches | `worldMaster:ask(attendant, attendant, 25, 2)` |
| Ask result `1` | `attendant:say(attendant, 28, 0)`; `attendant:finishCliantTalkTurn()`; return true |
| Any other ask result, including nil | `attendant:finishCliantTalkTurn()`; return nil |

English row 23 asks whether the player wishes to defer travel and leave the
landing area. Row 24 says purchased tickets are not refunded but remain valid
for later use. Rows 25-27 ask whether to leave, with yes/no answers. Row 32
also asks whether to leave. These are voluntary-exit text branches. They do
not describe missing a scheduled departure or establish the server's refund
or ticket-state implementation.

The special branch's `WorldMaster.say` receives exactly three function
arguments including its receiver: `(worldMaster, attendant, 24)`.
There is no additional explicit attendant or zero operand on this call
(PC 15, `0x693`, `CALL A=3 B=4 C=1`). The shared ask receives the full tuple
`(worldMaster, attendant, attendant, 25, 2)` and keeps one answer
(PC 28, `0x6C7`, `CALL A=3 B=6 C=2`). PC 29 compares that answer exactly
with 1. The two explicit returns are true at PC 39 (`0x6F3`) and nil at
PC 44 (`0x707`), both with `RETURN A=4 B=2`.

The separate `eventNG(attendant, player)` starts the talk turn, says
`(attendant, 22, 0)`, finishes it and returns no values (`root/proto3`
PCs 0-11, `0x7DE`-`0x80A`). Neither exit method establishes a world landing
position, a route to a scene pair or a scheduled boarding event.

These observations were recovered from the authenticated script and its
original decoded chunk using
`xivl-client-scripts:tools/retail_script.py:decode_lpb` and
`xivl-client-scripts:tools/build_quest_contracts.py:ChunkReader,instruction`.
The promoted
`xivl-client-scripts:docs/quest-event-client-contracts.md`, section
`PopulaceFlyingShip event-in return`, independently preserves the saved-choice
return distinction. Text labels come from
`xivl-client-data:manifests/tables.json`, member
`csv/populaceFlyingShip.csv`, English column 1, rows 2-32, SHA-256
`38fec76b161a97d5f09857df51b8d385a57df95b5204d5792414593b6bf15928`.
The row key is the first CSV column; the numbered language columns follow it.
The text was read with Python `csv.reader` from the archive pinned by
`xivl-client-data:manifests/private_csv_corpus.json`. These labels identify
the displayed service and ticket/fare branches, not a hardcoded fare amount.
No retail caller supplying the menu arguments was authenticated.

### Source identity and reproduction

The selected Lua files matched canonical LF bytes in
`xivl-client-scripts:manifests/scripts.json`. The original LPBs and decoded
chunks used for the bytecode locators matched
`xivl-client-scripts:manifests/retail_lua_coverage.json`.
These are static observations of extraction `2012.09.19.0001`.

| Script under `lua/scripts/` | Canonical SHA-256 |
| --- | --- |
| `quest/scenario/defaulttalk/dftsrt.lua` | `a8ed70ef14ae6eebce5c403c4982c72b3fbe84f0916409ca052274e97f685d48` |
| `chara/npc/populace/populaceflyingship.lua` | `02e39950c60bc96b1b6996aecf2c6230dafb8bc13b17ff6b857a74733f14e61f` |
| `quest/questbaseclass_common.lua` | `cf61a5687dbc7b6accd26c8cd3a138decc0372fb72518ac6f2054f125bc1465e` |
| `director/directorbaseclass.lua` | `45c87f548a82e81681433d6ad89c6de6fc62f313cead66e72bf8b8cf7aa55a82` |
| `gamedata/cutscene_common.lua` | `f6db9559b3f805d2738c073c7db9dd102fd5c33a056e789ff2fb2f8aff395bd1` |
| `widget/cutsceneskipwidget.lua` | `0f570ae13cca80944ecbb625e29be056799b046869357f6d8d44e8ac5eab136e` |
| `world/worldmaster_event.lua` | `744487a7caaa94e1c4d2056cc07686a894c6ba336bbd021bbffeebceef2ad099` |

For example, the DftSrt carrier
`tp5rq/r75w9s1v/6549pyqq9yz/64qrsq.le.lpb` has 757 bytes and SHA-256
`989D8F897DBE769090EF7D90EB35FBE4B061EF2314BBA24BE23D85C4BEA729B6`.
Its decoded 744-byte chunk has SHA-256
`72762BFBE567A34AF198BCF342F42581E2CE83ABB65A46A034A5472FE35A17E7`.
The attendant carrier
`729s9/wu7/uvupy975/uvupy9754yl1w3r21u.le.lpb` has 2,194 bytes and SHA-256
`371f2397e2dca81f6de424f568c66dd35e7c09a44e899addd5c5a02a962c4b43`.
Its decoded 2,181-byte chunk has SHA-256
`702d5b7f4b3f4f5674fc6e525a442c5843b1f8edbaa30295a80ec28776a62aea`.
In that chunk `eventIn`, `eventOut` and `eventNG` begin at `0x219`,
`0x657` and `0x7DE`, with 165, 46 and 12 code words respectively.
From an authorized `xivl-client-scripts` checkout, reproduce the decoded
chunk and canonical source with explicit private output paths:

```console
python tools/retail_script.py --input <client-script-root>/tp5rq/r75w9s1v/6549pyqq9yz/64qrsq.le.lpb --jar tools/vendor/unluac/unluac_2025_12_23.jar --decoded-out <private-output>/dftsrt.luac --script-out <private-output>/dftsrt.lua
```

Inspect the decoded chunk with the `ChunkReader` and `instruction` decoder
in `xivl-client-scripts:tools/build_quest_contracts.py`. Each listed offset
is `prototype["offset"] + 4 * PC`. For DftSrt, `root/proto2` starts at
`0x1F3` and has 20 code words. Check the original call/result operands,
rather than inferring returns from a decompiler's temporary variable names.

## Waits, removal and connection failures

The live invocation contains scene and map-load waits. It does not contain
a durable trip record or a reconnect handler. This is a bounded observation
of these methods, not a client-wide persistence or timeout claim.

In the authenticated `gamedata/cutscene_common.lua`, lines 1247-1314,
`_play` returns a boolean and an int. On the true-result path, lines
1301-1310 call `_waitForMapLoaded(nil)` when the mode is not 64. The airship
helper supplies mode 61. Native `0x006E5EB0` reads the optional argument
but does not use it as a deadline. It calls `0x0075C820` to read a resolved
object's byte `+0xB9` and yields while it is nonzero. Poll `0x0071C7F0`
continues waiting while that byte is nonzero and otherwise completes.
Neither this poll nor the scene poll `0x00713090` has a local timer.
The ordinary object construction and selected loading-flag writers are
traced below. Its identity relative to a particular request generation
remains unproved.

`DftSrt.eventDeparture` has no retry or error menu around either scene
helper. Each helper must return normally before execution proceeds to the
next call, even though its result is discarded. A false local scene result,
a missing scene callback and a completed scene waiting for map load are
different conditions. The scene checker does not itself test a server
acknowledgement. Missing server traffic alone therefore does not establish
which wait is pending.

### Map loading state and writers

`0x0075C820` resolves parent `V`, calls `0x004D7390`, and reads the
returned object's byte `+0xB9`. The getter returns `[V+0x4E0]`.
Parent construction at `0x004DBFA5` passes `V+0x4AC` to factory-table
constructor `0x0053B230`. The factory registers callback `0x005335C0`
at index 9. The reached initialization branch at `0x004D91E2` supplies
selector 9 and actor key `0xC000001E`; factory `0x00537620` stores its
callback result at factory `+0x34`, hence `V+0x4E0`. The callback constructs
`0x208` bytes through `0x0059EE50`, which installs MapLayoutElement
vtable `0x00FAAD88`. This establishes the ordinary static pointer/type
relationship, not a particular historical instance or its reset lifetime.

| Reached writer | Effect on MapLayoutElement `+0xB9` |
| --- | --- |
| Constructor at `0x0059EF07` | Initialize to zero. |
| Reset `0x0059C8F0`, store `0x0059C93B` | Clear to zero and post local scene selector 4. |
| Setter `0x0059CEB0`, store `0x0059CEB4` | Copy its argument byte and post that byte with local selector `0x2C`, size 1. |
| Request publisher `0x0059E3C0`, store `0x0059E485` | Set to 1 and post the same one-byte selector `0x2C`. This tail also runs when the first argument is zero. |
| Packet staging at `0x0059CF66` | Set to 1 on the admitted nonzero-input branch after a separate state/equality test. This is not a recovered request-generation check. |
| Scene receiver `0x0059E660`, store `0x0059E69C` | Copy selector-`0x2C` payload byte directly, without comparing a request or generation in this branch. |

The scene-result handler itself raises the byte before a true callback:
for result codes 0 and 3, `0x00563DB6`-`0x00563DC2` resolves the parent
MapLayoutElement and calls `0x0059CEB0(1)`. The true callback follows at
`0x00563DCC`. Thus the later Lua map wait can follow state raised by local
scene completion; it does not itself prove a required post-scene server
acknowledgement.

There is a conditional local clear producer. MapLayoutActor vtable
`0x00FBAE3C`, slot 137, contains `0x0062AC20`. When actor `+0x1F9` is
nonzero and predicate `0x00626A80` returns false, that method clears
`+0x1F9` and sends byte 0, selector `0x2C`, size 1 through `0x007CE7B0`
at `0x0062AC4E`. The predicate returns true while actor `+0x188` is
nonzero or any reached object in its two pointer ranges has nonzero byte
`+0x1B0`. It has no elapsed-time test in the inspected body. The reached
objects' types and terminal failure states remain untraced.

The [local envelope transport](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport)
routes through actor `+0xDC`. If that key resolves to this MapLayoutElement,
its slot-8 cell `0x00FAADA8` reaches `0x0059E660` and copies the zero byte.
The payload contains no transfer identifier, and that consumer branch does
not compare one. The key's actual target, instance lifetime and relation to
the Lua poll's currently resolved parent pointer remain runtime identities.
This is a conditional clear path, not proof against a stale queued clear.

Reset `0x004DAF00` separately resolves `V+0x4E0` and calls the clear
routine at `0x004DAFA2` when nonnull. Admitted raw command `0x000E` and
`0x0011` branches in the main dispatcher call that reset at
`0x004DCC73/0x004DCCA0`. Callback `0x004DB5B0` also reaches it for raw
byte values 0, 1 and 2. Their command/selector meanings and connection-loss
delivery were not established. These paths prove a reset clear exists,
not automatic timeout, disconnect recovery or return to the origin.

### Generic CutScene removal

The CutScene vtable cell at `0x00FD5990` contains `0x006FCAA0`, slot 5.
Forwarder `0x00CCC9E0` loads the control's vtable `+0x14` and tail-jumps
to it. Removal routine `0x00CDAEB0` calls `0x00CE24C0` at `0x00CDAF72`.
That callee compares record `+8` with the mutable values at
`0x0130D4FC/0x0130D500/0x0130D504`. Its admitted branch loads the control
from record `+0`, supplies record `+0x70` and an engine handle, and calls
the slot-5 forwarder. A matching record containing a CutScene can therefore
reach the override. Those enum values and admission meanings remain unknown.

An additional call at `0x00CDB6AC` reaches the same forwarder while iterating
script-state controls. The surrounding path later releases and zeros an
engine-owned pointer at `0x00CDB775/0x00CDB77D`. Wrapper `0x00CC9650`
calls that path at `0x00CC965A` before further release. These effects support
a generic reset/release interpretation. They do not identify a logout,
disconnect or map-server restart entrypoint.

When CutScene `+0xC0` is nonzero or `+0x64` is nonnull, `0x006FCAA0`
calls its finalization helpers, clears `+0xC0` at `0x006FCB25` and drops
the checker pointer at `0x006FCB2C`, then calls the base routine. This body
does not read the checker or publish its boolean, int or ready result.
The generic raisers extend the
`xivl-client-structs:docs/cutscene-native-contract.md`, section
`Slot 5 teardown routine`, without establishing what causes removal for a
particular airship invocation. Subsequent thread destruction and opaque
callee effects remain outside this observation.

### Failure-case evidence

| Case | Established client behavior | Remaining boundary |
| --- | --- | --- |
| Server never answers after the scene | A local callback publishes the scene checker result independently of a server acknowledgement. A successful scene helper can then wait for map load. | No same-request post-scene server-answer dependency, automatic retry or return to menu was established. An unchanged live checker stays waiting, which does not prove a session hangs forever. |
| Slow zone transfer | The mode-61 true-result path polls shared MapLayoutElement loading byte `+0xB9` until it is zero. Conditional scene and reset clear paths are traced above. | No local deadline, transfer-generation identity or bound on waiting is established. Other engine/connection timers are untraced. |
| Failed zone transfer | This load checker exposes waiting versus done, with no distinct failure result. The paired Lua method has no recovery menu. | The local clear predicate's terminal resource states and reset inputs' failure meanings are unbound. Neither guaranteed timeout nor permanent hang is established. |
| Drop during boarding | The default fade-out helper precedes `_play`; a pending CutScene can lose its checker association through the generic removal path if reached. | The connection-loss-to-removal/fade-clear edge is unbound. |
| Drop during departure or flight | The first scene helper must return before the optional arrival helper starts. Its successful result can include a map-load wait. | No separate durable client flight phase or reconnect resume point was recovered. |
| Drop during arrival | The optional second helper has its own scene lifecycle, followed by the final fade helper after normal return. | No persisted scene-pair PC or arrival recovery reader was joined to the next session. |
| Reconnect | The inspected removal code supplies no trip reconstruction. | Login, fresh menu/event selection, scene replay and fare/destination restoration after reconnect remain unproved. `xivl-client-structs:docs/login-state-boundaries.md` separately retains world-ready and disconnect-invalidation limits. |
| Map-server restart | The inspected scene/checker/removal inputs contain no identified map-restart token. | The restart-to-transport/event/removal edge is unbound. This cannot validate an admission barrier, grant release or durable booking recovery algorithm. |

No inspected method writes a persistent booking, fare, origin or
Booked/Pending/Arrived record. This does not establish that the client has
no trip persistence elsewhere. The
[EndEvent receiver](end-order-event-receiver.md#implications-for-client)
dispatches without a null-actor gate in its slot-3 body, but delivery,
per-type cleanup, resumed waiters and a non-hanging session do not follow.

### Native evidence and reproduction

The native observations above use retail 1.23b `ffxivgame.exe`, 15,996,808
bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Each listed span was read from the original PE with
pefile `2024.8.26` and decoded with Capstone `5.0.7`, x86 32-bit mode.
Every span ended on an instruction boundary. Retained Ghidra listings were
navigation aids; these claims use the fresh PE decode. No client execution
or connection-failure experiment was performed.

| Start VA and byte count | Observation |
| --- | --- |
| `0x006FB8D1 +0x81`, `0x00713030 +0x60`, `0x00713090 +0x0C` | Scene checker installation, initial waiting value and poll. |
| `0x006FBC50 +0x6D` | Callback result and ready publication. |
| `0x006E5EB0 +0x11A`, `0x0075C820 +0x0E`, `0x0071C7F0 +0x36` | Map-load wait, flag reader and poll. |
| `0x004D7390 +7`, `0x004DBFA5 +0x4F`, `0x0053B255 +0x30`, `0x0053B3D3 +0x23` | Parent getter, factory construction and callback registration. |
| `0x004D90C0 +0x4F`, `0x004D911C +0xD4`, `0x00537620 +0x15A`, `0x005335C0 +0x75`, `0x0059EE90 +0x7D` | Selector-9 initialization, result store and MapLayoutElement construction. |
| `0x0059C8F0 +0x54`, `0x0059CEB0 +0x1F`, `0x0059CED0 +0x110`, `0x0059E3C0 +0x109`, `0x0059E660 +0x45`, `0x00563D70 +0xD8` | Selected loading-byte writers and scene-result raise. |
| `0x0062AC20 +0x7D`, `0x00626A80 +0xD9`, `0x007CE7EA +0x49`, `0x004DAA95 +0x31`, `0x004D65E0 +0x3A`, `0x004D66C0 +0x30` | Conditional scene clear predicate, producer and envelope delivery. |
| `0x004DAF33 +0x74`, `0x004DB5B0 +0xE1`, `0x004DC6D0 +0x80`, `0x004DCC65 +0x5A` | Parent reset, raw callback and selected dispatcher paths. |
| `0x006FCAA0 +0xD1` | CutScene slot-5 clearing and base call. |
| `0x00CCC9E0 +7`, `0x00CE24C0 +0x98`, `0x00CDAEB0 +0xF5` | Slot-5 forwarder, guarded raiser and removal caller. |
| `0x00CDB5E6 +0xF8`, `0x00CDB6F9 +0x0F`, `0x00CDB760 +0x20`, `0x00CC9650 +0x2E` | Control iteration and engine release spans. |

Use the PE translation and Capstone decoder in the
[scene-notice reproduction](../net/ferry-scene-notice-producer.md#reproduction),
substituting these ranges. Read the four-byte vtable cell `0x00FD5990`
through the same translation. These finite ranges do not authenticate
unlisted callees or a whole-session lifecycle. Script identities are in
the source table above; the source-line observations use the hash-matched
canonical Lua, with the paired return operands independently established
by its original bytecode.

For the loading-state construction and dispatch joins, read factory jump
table `0x0053777C` (16 dwords), scene jump table `0x0059E6A8` (5 dwords)
and selector bytes `0x0059E6BC` (11 bytes), and main-dispatch jump table
`0x004DD3D0` (17 dwords) and selector bytes `0x004DD414` (228 bytes).
The corresponding selected vtable cells are `0x00FAADA8/0x00FAADAC`,
`0x00FBB060` and `0x00F9119C`. The two type checks follow each primary
vtable's preceding dword to its MSVC complete-object locator, descriptor
at locator `+0x0C`, then mangled name at descriptor `+8`. Tables
`0x00FAAD88` and `0x00FBAE3C` identify
`.?AVMapLayoutElement@Map@Element@Main@Application@@` and
`.?AVMapLayoutActor@Map@Actor@Scene@Application@@` respectively.

## Scene assets

The scene assets have city-specific departure (`*000`) and arrival
(`*010`) names. Their byte identities were checked directly:

| City | Scene | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Gridania | `zep0g000` | 111,520 | `6dc88355de9859a5ec0ad4ee856086eb37ebea6916acaf657d84649e1d11e893` |
| Gridania | `zep0g010` | 115,488 | `1958c9fadf4f147177f19d779ebf3e2c3df2bf86e0c01c4479e804e83eeaadbb` |
| Limsa Lominsa | `zep0l000` | 117,408 | `7b2009d319ebce90d41e9d7091a7e4bb4d0e10e8783b74c013b4a71b72a52197` |
| Limsa Lominsa | `zep0l010` | 159,648 | `5ab93053dfa7f8d0d71d27241a0c00ae0f89f803af9dd912345af135fa5715d6` |
| Ul'dah | `zep0u000` | 129,424 | `71634fb82d460530bbda822dea9bb5ea7b5e8154559788611a2cd376fdc6966f` |
| Ul'dah | `zep0u010` | 149,824 | `5923855356423f238a412bf578470e8d26d04cb034e5e1a201a52768cc205ee5` |

A direct parse of the six hash-matched scene assets finds actor dictionaries
and cutscene transforms. Those transforms are cinematic placements,
not authenticated public-zone spawns or arrival coordinates. A current
server route's choice of a departure/arrival pair is an implementation
decision unless joined to retail route evidence; asset presence alone does
not authenticate that choice.

## Parsed actor and scheduler boundary

A direct parse of the six hash-matched files yields 77 actor-dictionary records and
74 structurally plausible spatial records in total. The setup
stream accounts for 24 of those spatial records; the wider scan also finds
later timeline placements. These are parser counts, not a count of actors
simultaneously visible or proof of world-spawn locations.

| Scene | SCB file offset | Actor records | Setup spatial | Wider spatial | Inner block span sum |
| --- | ---: | ---: | ---: | ---: | ---: |
| `zep0g000` | `0x16498` | 8 | 5 | 16 | 8,800,000 |
| `zep0g010` | `0x19050` | 11 | 3 | 8 | 12,050,000 |
| `zep0l000` | `0x18840` | 14 | 6 | 14 | 8,100,000 |
| `zep0l010` | `0x22658` | 14 | 3 | 10 | 11,100,000 |
| `zep0u000` | `0x1BAA8` | 15 | 4 | 15 | 8,700,000 |
| `zep0u010` | `0x203D8` | 15 | 3 | 11 | 10,800,000 |

Each embedded `SEDBSCB` has a first `@CBLK` with raw span `9,000,000` and
zero clips. The last column sums the other `@CBLK` span words, read at
block offset `+0x24`; the clip count is at `+0x28`. These sums are not
playback durations: branch order, waits, and scene completion are not
reconstructed by this header read. In particular, the common outer
`9,000,000` value does not prove that each movie plays for nine seconds.
The actor and spatial counts use `decompile_airship_cutscene_setup.py`
against those six scene files. The block fields were read directly from
each hash-matched SCB.

## Scene-local actor dictionaries

The six hash-pinned scene assets contain 77 scene-local kind-1 dictionary
records. Each stores an index, a NUL-terminated label, and a four-byte value
at record offset +0x30. The table preserves the literal fields and offsets
read from those assets. These entries do not establish persistent world-actor
identities, route assignment, world placement, or playback.

| Scene | Index | Stored label | Stored value | Record offset |
| --- | ---: | --- | ---: | ---: |
| `zep0g000` | 4 | `PC` | 0 | `0x16628` |
| `zep0g000` | 5 | `Airship` | 1200090 | `0x16664` |
| `zep0g000` | 6 | `Memama` | 1001706 | `0x166A0` |
| `zep0g000` | 7 | `Pfarahr` | 1001707 | `0x166DC` |
| `zep0g000` | 8 | `Npc_hyuranosu` | 1000299 | `0x16718` |
| `zep0g000` | 9 | `Npc_Mikotte` | 1000786 | `0x16754` |
| `zep0g000` | 13 | `ZamuQ` | 1001711 | `0x16808` |
| `zep0g000` | 14 | `sentyou` | 1001781 | `0x16844` |
| `zep0g010` | 4 | `PC` | 0 | `0x191E0` |
| `zep0g010` | 5 | `Airship` | 1200090 | `0x1921C` |
| `zep0g010` | 6 | `Lionnellais` | 1500055 | `0x19258` |
| `zep0g010` | 7 | `Hida` | 1500056 | `0x19294` |
| `zep0g010` | 8 | `Memama` | 1001706 | `0x192D0` |
| `zep0g010` | 9 | `Pfarahr` | 1001707 | `0x1930C` |
| `zep0g010` | 10 | `Beaudonet` | 1001708 | `0x19348` |
| `zep0g010` | 11 | `Fryswyde` | 1001709 | `0x19384` |
| `zep0g010` | 12 | `Willielmus` | 1001710 | `0x193C0` |
| `zep0g010` | 13 | `QZamqo` | 1001711 | `0x193FC` |
| `zep0g010` | 17 | `sentyou` | 1001781 | `0x194A0` |
| `zep0l000` | 4 | `PC` | 0 | `0x189D0` |
| `zep0l000` | 5 | `Airship` | 1200090 | `0x18A0C` |
| `zep0l000` | 6 | `Ajin_Zukajin` | 1001700 | `0x18A48` |
| `zep0l000` | 7 | `Raplulu` | 1001701 | `0x18A84` |
| `zep0l000` | 8 | `G_Zentsa_Rhof` | 1001702 | `0x18AC0` |
| `zep0l000` | 9 | `Aldyet` | 1001703 | `0x18AFC` |
| `zep0l000` | 10 | `Murlskylt` | 1001704 | `0x18B38` |
| `zep0l000` | 11 | `Aenore` | 1001705 | `0x18B74` |
| `zep0l000` | 12 | `T_Noggya_Bham` | 1600093 | `0x18BB0` |
| `zep0l000` | 13 | `L_Nophlo` | 1500206 | `0x18BEC` |
| `zep0l000` | 14 | `Wineburg` | 1500207 | `0x18C28` |
| `zep0l000` | 15 | `Merchant023` | 1001493 | `0x18C64` |
| `zep0l000` | 16 | `Ruga` | 1000678 | `0x18CA0` |
| `zep0l000` | 19 | `sentyou` | 1001781 | `0x18D20` |
| `zep0l010` | 4 | `PC` | 0 | `0x227E8` |
| `zep0l010` | 5 | `Airship` | 1200090 | `0x22824` |
| `zep0l010` | 6 | `Ajin_Zukajin` | 1001700 | `0x22860` |
| `zep0l010` | 7 | `Raplulu` | 1001701 | `0x2289C` |
| `zep0l010` | 8 | `G_Zentsa_Rhof` | 1001702 | `0x228D8` |
| `zep0l010` | 9 | `Aldyet` | 1001703 | `0x22914` |
| `zep0l010` | 10 | `Murlskylt` | 1001704 | `0x22950` |
| `zep0l010` | 11 | `Aenore` | 1001705 | `0x2298C` |
| `zep0l010` | 12 | `T_Noggya_Bham` | 1600093 | `0x229C8` |
| `zep0l010` | 13 | `L_Nophlo` | 1500206 | `0x22A04` |
| `zep0l010` | 14 | `Wineburg` | 1500207 | `0x22A40` |
| `zep0l010` | 15 | `Merchant023` | 1001493 | `0x22A7C` |
| `zep0l010` | 16 | `Lady031` | 1001226 | `0x22AB8` |
| `zep0l010` | 20 | `sentyou` | 1001781 | `0x22B5C` |
| `zep0u000` | 4 | `PC` | 0 | `0x1BC38` |
| `zep0u000` | 5 | `Airship` | 1200090 | `0x1BC74` |
| `zep0u000` | 6 | `STANGYTH` | 1500208 | `0x1BCB0` |
| `zep0u000` | 7 | `LUNNIE` | 1500209 | `0x1BCEC` |
| `zep0u000` | 8 | `GUILLESTET` | 1001712 | `0x1BD28` |
| `zep0u000` | 9 | `H_CIDJAA` | 1001713 | `0x1BD64` |
| `zep0u000` | 10 | `AUTGAR` | 1001714 | `0x1BDA0` |
| `zep0u000` | 11 | `AHLDBYRT` | 1001715 | `0x1BDDC` |
| `zep0u000` | 12 | `NEYMI_FUNOMI` | 1001716 | `0x1BE18` |
| `zep0u000` | 13 | `GOODIFE` | 1001717 | `0x1BE54` |
| `zep0u000` | 14 | `LADY030` | 1001215 | `0x1BE90` |
| `zep0u000` | 15 | `PEDESTRIANU006` | 1001119 | `0x1BECC` |
| `zep0u000` | 16 | `PEDESTRIANU009` | 1001122 | `0x1BF08` |
| `zep0u000` | 17 | `ADVENTURER088` | 1001062 | `0x1BF44` |
| `zep0u000` | 20 | `sentyou` | 1001781 | `0x1BFC4` |
| `zep0u010` | 4 | `PC` | 0 | `0x20558` |
| `zep0u010` | 5 | `Airship` | 1200090 | `0x20594` |
| `zep0u010` | 6 | `STANGYTH` | 1500208 | `0x205D0` |
| `zep0u010` | 7 | `LUNNIE` | 1500209 | `0x2060C` |
| `zep0u010` | 8 | `GUILLESTET` | 1001712 | `0x20648` |
| `zep0u010` | 9 | `H_CIDJAA` | 1001713 | `0x20684` |
| `zep0u010` | 10 | `AUTGAR` | 1001714 | `0x206C0` |
| `zep0u010` | 11 | `AHLDBYRT` | 1001715 | `0x206FC` |
| `zep0u010` | 12 | `NEYMI_FUNOMI` | 1001716 | `0x20738` |
| `zep0u010` | 13 | `GOODIFE` | 1001717 | `0x20774` |
| `zep0u010` | 14 | `LADY030` | 1001215 | `0x207B0` |
| `zep0u010` | 15 | `PEDESTRIANU006` | 1001119 | `0x207EC` |
| `zep0u010` | 16 | `PEDESTRIANU009` | 1001122 | `0x20828` |
| `zep0u010` | 17 | `ADVENTURER088` | 1001062 | `0x20864` |
| `zep0u010` | 20 | `sentyou` | 1001781 | `0x208E4` |

Provenance: recovered
`quest/scenario/defaulttalk/dftsrt.lua:eventDeparture`,
`director/directorbaseclass.lua:delegateEvent`, and
`quest/questbaseclass_common.lua:startNQCutScene` establish the script
sequence. `client/cut/zep0*/zep0*` files supply the six sizes and
hashes and SCB offsets above identify the source scenes;
`decompile_airship_cutscene_setup.py` produced the parsed roles and
dictionaries from those hash-matched files.
No retail runtime probe or
new capture was used.
