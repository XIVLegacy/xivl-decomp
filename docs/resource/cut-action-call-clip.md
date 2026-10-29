# Cut scheduler ActionCall clip

`RaptureActionCallClip` copies u32(D+`0x14`) into a local request Q+`0x10`
and forwards that request through two wrappers to a component at the selected
actor pointer R+`0x2858`. An untraced helper returns O before type dispatch.
A conditional type-5 branch then makes a virtual call and forwards O to an
indexed destination. The serialized dword's
numeric domain remains unestablished: this is not an assignment of a
game-command ID, packed playback selector or model-qualified WSS/BID bank.

## Source and ownership

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VAs. Python 3.12 and Capstone 5.0.7 x86-32 recovered
entry-seeded local control flow from the original PE, authenticated against
`xivl-client-structs:manifests/retail_inputs.json`, input
`ffxivgame-1.23b`. No runtime or exceptional-unwind behavior is inferred.

Exact class `Application::Scene::Cut::Clip::RaptureActionCallClip` is in
[the RTTI catalog](../../config/ffxivgame.rtti.json), lines 1985-1986.
[The slot catalog](../../config/ffxivgame.vtable_slots.jsonl), line 34739,
maps table `0x010100FC` slot 1 to `0x0081D230`. Their producer was
`tools/ghidra_scripts/DumpRtti.java`, Ghidra 12.1. Original pointer cell
`0x01010100` independently matched that entry, four-byte SHA-256
`05a88fc440bba8c24bf8cd66d1c03a63b8d546b08e4dc6e413e61d07f469e45e`.
Slot 2 at `0x0081CF90` was not decoded by this contract.

## Payload fields and local request

D is loaded from entry ECX+`0x14` at `0x0081D261`. R denotes the selected
pointer retained through the caller's cast branches; its numeric actor
identity, concrete dynamic class and historical use remain unbound.

| D input | Width / signedness | Observation |
| --- | --- | --- |
| `+0x10` | u8 | Compared with 2 at `0x0081D264`; inequality returns through `0x0081D268`. |
| `+0x03` | u8, zero-extended | Supplied to `0x00A1FFA0` at `0x0081D27A`. |
| `+0x14` | u32 | Read at `0x0081D405`, copied unchanged to Q+`0x10` at `0x0081D428`. |
| `+0x11` | s8, sign-extended | Loop bound at `0x0081D4CD`; the initial signed <=0 branch skips the loop at `0x0081D446`. |

Let B be the normal local-frame ESP and Q=B+`0xD0`. The selected stores are:

| Q field | Store / locator |
| --- | --- |
| u32 `+0x00` | 0 at `0x0081D40F` |
| u32 `+0x04` | 5 at `0x0081D416` |
| u32 `+0x0C` | u32(R+`0xDC`) at `0x0081D3FF` / `0x0081D421` |
| u32 `+0x10` | u32(D+`0x14`) at `0x0081D405` / `0x0081D428` |
| u16 `+0x30` | Accepted target count, narrowed at `0x0081D4E8` / `0x0081D4F8` |
| Area `+0x38` | Starts at `0x0081D44C`, stride `0x14` at `0x0081D4B9`; an accepted entry's first dword is u32(slot23_result+`0xDC`), stores `0x0081D4AC` / `0x0081D4B7` |

The `0xDC` source words are not given actor-ID semantics from their offsets.
The two target-selection helpers `0x0081CFA0` and `0x0081D120`, their full
selection predicates and other Q fields remain untraced. This table is a
partial field relationship, not a complete request schema.

At `0x0081D4ED` the caller forms &Q. Pushes at `0x0081D4F4` and
`0x0081D4F5` supply &Q then R. The call at `0x0081D500` is
`0x0065EAC0`, ECX=R+`0x11B4`, with nearest-first arguments `[R,&Q]`.
The receiver comes from `0x0081D3DA` / `0x0081D3E2`, restored at
`0x0081D4E0`. Normal x86 call/return and callee-saved-register conventions
are assumed.

## Forwarding and guards

`0x0065EAC0` tests explicit R at `0x0065EAC5` / `0x0065EAC7` and exits
for zero. It calls R primary-vtable byte offset `0x268`, slot 154, at
`0x0065EAD3`, with no explicit arguments; a nonzero result also exits.
Otherwise `0x0065EAE4` calls `0x00846080`, ECX=R+`0x2858`, with `[&Q]`.
The incoming ECX=R+`0x11B4` is unused before being overwritten. This
wrapper has no Q-field read or data store and returns with `RET 8`.

`0x00846080` forwards the same receiver and `[&Q]` to `0x00845E80`
at `0x00846085`. It converts the returned EAX to AL=(EAX!=0) and uses
`RET 4`. It has no field read, data store, record-number or length operand.
Neither wrapper proves a copy of Q or a final numeric-ID interpretation.

## Conditional type-5 branch

Let C=R+`0x2858`, the entry receiver of `0x00845E80`. Its dispatcher
reloads &Q at `0x00845FC6`, reads u32(Q+4) at `0x00845FCA`, adds -5
at `0x00845FCD`, and bounds the result unsigned <=4 at
`0x00845FD0` / `0x00845FD3`. Only index-0 cell `0x0084603C` was read.
It points to `0x00846001`, cell SHA-256
`3da246598bbf31bba03cec0e5b436209891e31fa828aece274af5d4bed580bc6`.
Four other cells and their arms remain unexamined.

The caller initialized Q+4=5, but earlier uninspected helpers/guards can
affect Q. This selected branch therefore remains conditional on Q+4 still
being 5 at that dispatch read. It does not establish unconditional type-5
delivery.

Before dispatch, `0x00845F9B` calls `0x00843B50`. Its receiver is the
result of `0x009D1B35` at `0x00845F7C`; its explicit argument is &S,
where S=ESP_at_`0x00845F94`+`0x1C`. Returned O is retained at
`0x00845FA0` and tested for nonnull at `0x00845FAE` / `0x00845FB0`.
The helper body and S's complete relation to Q were not traced.

The selected arm calls O slot 1 at `0x0084600A`, receiver O, argument
`[1]`. It then rereads u32(Q+4) at `0x0084600C`. The direct call to
`0x005692F0` at `0x0084601F` has receiver C+4+20*u32(Q+4) and the
single argument &localHoldingO. This local is ESP_before_`0x00846016`+
`0x4C`; O is stored there at `0x0084601B`, after the argument push.
The receiver simplifies to C+`0x68` only if Q+4 remains 5 across the
uninspected O slot-1 call. Object identity, copying, insertion behavior and
the helper's effect remain unestablished.

## Remaining joins

The exact next native boundaries are `0x00843B50` with `[&S]`,
`0x005692F0` with `[&localHoldingO]`, and initial context helper
`0x007A0F70`. None was followed by this contract. No actual numeric
game-command ID, request-copy size, completion/settlement signal, encounter
actor, add or variant membership was earned. Historical actor binding does
not prevent the static field/consumer relationships above.

The record's name does not assign model substates, actor main states,
status effects or a server encounter phase. No collision polarity,
activation/cancellation or defeat/wipe/retry/exit restoration follows.

## Recovered native ranges

Ends are exclusive. Actual instruction ranges were recovered before any
comparison with catalog sums. The `0x00845E80` coverage is partial: the
direct prefix/default paths and one selected arm total 407 decoded bytes.
It is not a complete function extent.

| Entry / selected interval | Actual range | SHA-256 |
| --- | --- | --- |
| `0x0081D230` | `[0x0081D230,0x0081D521)` | `deb7c9c6e905974559b10af0d1675d839698ff30785eb5d34ad4ffdd7702beb4` |
| `0x0065EAC0` | `[0x0065EAC0,0x0065EAED)` | `44949d1cbf2638cba108e23da4e6334b7aef18efe21e6a9901cd1bc2f7642bb4` |
| `0x00846080` | `[0x00846080,0x00846092)` | `2bc347070e8187c8a7ee1b4ec92ff31caa95136803cb87c3395913f65e3bdb65` |
| `0x00845E80` prefix | `[0x00845E80,0x00845FDC)` | `94cb30b853bb4e8d477b9db7e4af8e4b71095028486fc39107aec541b744e2d1` |
| Selected arm / common tail | `[0x00846001,0x0084603C)` | `080692f33c538f04b9db803c95cd8c1dda976e782ae9b579d398662fb4477fad` |
