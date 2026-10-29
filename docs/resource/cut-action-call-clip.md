# Cut scheduler ActionCall clip

`RaptureActionCallClip` copies u32(D+`0x14`) into a local request Q+`0x10`
and forwards that request through two wrappers to a component at the selected
actor pointer R+`0x2858`. A constructor returns O before type dispatch.
A conditional type-5 branch then makes a virtual call and forwards O to an
indexed destination. The constructor-context join identifies Q+`0x10` as
the packed playback-selector input field of the existing visual-copy route.
Preservation of the original dword through uninspected gates remains
conditional. No concrete game-command ID or model-qualified WSS/BID bank
is assigned.

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

Let B be the normal local-frame ESP. Q is the request's base address,
B+`0xD0`, not the address of a cell holding that pointer. The selected stores are:

| Q field | Store / locator |
| --- | --- |
| u32 `+0x00` | 0 at `0x0081D40F` |
| u32 `+0x04` | 5 at `0x0081D416` |
| u32 `+0x0C` | u32(R+`0xDC`) at `0x0081D3FF` / `0x0081D421` |
| u32 `+0x10` | u32(D+`0x14`) at `0x0081D405` / `0x0081D428` |
| u16 `+0x30` | Accepted target count, narrowed at `0x0081D4E8` / `0x0081D4F8` |
| Area `+0x38` | Starts at `0x0081D44C`, stride `0x14` at `0x0081D4B9`; an accepted entry's first dword is u32(slot23_result+`0xDC`), stores `0x0081D4AC` / `0x0081D4B7` |

The `0xDC` source words are not given actor-ID semantics from their offsets.
The two target-selection helpers `0x0081CFA0` and `0x0081D120` have the
caller tuples below. Their effects, full selection predicates, output sizes
and other Q fields remain untraced. This table is a partial field
relationship, not a complete request schema.

At `0x0081D4ED` the caller forms Q. Pushes at `0x0081D4F4` and
`0x0081D4F5` supply Q then R. The call at `0x0081D500` is
`0x0065EAC0`, ECX=R+`0x11B4`, with nearest-first arguments `[R,Q]`.
The receiver comes from `0x0081D3DA` / `0x0081D3E2`, restored at
`0x0081D4E0`. Normal x86 call/return and callee-saved-register conventions
are assumed.

### Target-helper caller tuples

Let B be steady caller-local ESP, P the entry clip ECX, and X the original
entry stack word. P is stored at B+`0x20` at `0x0081D25D`; X is loaded
from B+`0x280` at `0x0081D373`/`0x0081D46C`. These are pointer/raw-word
roles, not actor identities. The cursor cell B+`0x14` initially receives
D+`0x18` at `0x0081D27F`/`0x0081D286`.

| Helper callsite | Receiver / nearest-first supplied arguments | Return use |
| --- | --- | --- |
| `0x0081D291` -> `0x0081D120` | &cell(B+`0x14`), `[B+0x7C]` | EAX becomes receiver of `0x00445E00` at `0x0081D2A8`. |
| `0x0081D2F5` -> `0x0081D120` | &cell(B+`0x14`), `[B+0x7C]` | EAX becomes receiver of `0x00445D20` at `0x0081D305`. |
| `0x0081D363` -> `0x0081D120` | &cell(B+`0x14`), `[B+0x28]` | EAX becomes receiver of `0x00445210` at `0x0081D37D`. |
| `0x0081D45C` -> `0x0081D120` | &cell(B+`0x14`), `[B+0x28]` | EAX becomes receiver of `0x00445210` at `0x0081D476`. |
| `0x0081D387` -> `0x0081CFA0` | P, `[W,X]` | EAX is retained in ESI at `0x0081D38C`. |
| `0x0081D480` -> `0x0081CFA0` | P, `[W,X]` | EAX is retained in ESI at `0x0081D485`. |

The D120 receiver and explicit word are local addresses, not loads from
those cells. The two CFA0 calls load their receiver from a stack cell and
push W without dereferencing it. W is the immediately preceding
`0x00445210` return word.

The X pushes at `0x0081D37A`/`0x0081D473` precede the accessor calls.
The exact accessor at `0x00445210` reads u32(ECX+0) into EAX and returns
at `0x00445212` with no immediate argument cleanup. It has no call, object
write or stack-argument read. Thus X remains live across it, followed by
the W pushes at `0x0081D386`/`0x0081D47F`. Under normal balanced-call
and consistent-local-frame conventions, receiver loads
`0x0081D382`/`0x0081D47B` select P, and CFA0 receives `[W,X]` with
eight-byte cleanup expected by its caller. This is caller evidence; CFA0's
implementation/formal signature remains uninspected. The
[existing accessor use](sqwt-markup-font-numeric-properties.md)
provides a separate string-wrapper context, not a type proof for this W.

The second D120 call requires nonzero AL from `0x00445E00`, retained in
BL and tested at `0x0081D2C3`/`0x0081D2C5`. The third requires zero AL
from `0x00445D20` at `0x0081D331`/`0x0081D333`; its nonzero alternative
bypasses that call and the first CFA0 call. No meaning is assigned to
these opaque predicates. The loop repeats the fourth D120 and second CFA0
calls after the Q fields are initialized. It rereads and sign-extends
D+`0x11` at `0x0081D4CD`, rather than snapshotting that loop bound.

Caller cursor progression reads the current cursor's first byte and adds
8 when nonzero or `0x24` when zero, storing the result at B+`0x14`.
The initial progression is `0x0081D42F-0x0081D43E`; loop progression is
`0x0081D4BC-0x0081D4D6`. Helper effects on that cursor and D remain opaque.
The loop's nonzero CFA0 result must also have a nonzero slot23 result
before its returned +`0xDC` word is appended to the accepted target area.

Q=B+`0xD0` and its row area starts at B+`0x108`. The supplied local
addresses differ from those bases; neither helper directly receives Q or
its row base. Unknown output extents and pointers reachable through the
cursor, receiver, W or X can still alias Q or rows. Caller tuples alone
therefore do not prove unconditional selector preservation, target identity,
successful playback, command IDs or encounter/variant membership.

## Forwarding and guards

`0x0065EAC0` tests explicit R at `0x0065EAC5` / `0x0065EAC7` and exits
for zero. It calls R primary-vtable byte offset `0x268`, slot 154, at
`0x0065EAD3`, with no explicit arguments; a nonzero result also exits.
Otherwise `0x0065EAE4` calls `0x00846080`, ECX=R+`0x2858`, with `[Q]`.
The incoming ECX=R+`0x11B4` is unused before being overwritten. This
wrapper has no Q-field read or data store and returns with `RET 8`.

`0x00846080` forwards the same receiver and `[Q]` to `0x00845E80`
at `0x00846085`. It converts the returned EAX to AL=(EAX!=0) and uses
`RET 4`. It has no field read, data store, record-number or length operand.
Neither wrapper proves a copy of Q or a final numeric-ID interpretation.

## Conditional type-5 branch

Let C=R+`0x2858`, the entry receiver of `0x00845E80`. Its dispatcher
reloads Q at `0x00845FC6`, reads u32(Q+4) at `0x00845FCA`, adds -5
at `0x00845FCD`, and bounds the result unsigned <=4 at
`0x00845FD0` / `0x00845FD3`. Only index-0 cell `0x0084603C` was read.
It points to `0x00846001`, cell SHA-256
`3da246598bbf31bba03cec0e5b436209891e31fa828aece274af5d4bed580bc6`.
Four other cells and their arms remain unexamined.

The caller initialized Q+4=5, but earlier uninspected helpers/guards can
affect Q. This selected branch therefore remains conditional on Q+4 still
being 5 at that dispatch read. It does not establish unconditional type-5
delivery.

Before dispatch, `0x00845F9B` calls constructor `0x00843B50`. Its receiver is the
result of `0x009D1B35` at `0x00845F7C`; its explicit argument is &S,
where S=ESP_at_`0x00845F94`+`0x1C`. Returned O is retained at
`0x00845FA0` and tested for nonnull at `0x00845FAE` / `0x00845FB0`.
The constructor and the relevant S fields are joined below.

The selected arm calls O slot 1 at `0x0084600A`, receiver O, argument
`[1]`. The concrete entry selects `0x00846240`, whose published setter
sets entry flag bit 2 from that Boolean. This is a queue-entry flag,
not a model substate. The caller then rereads u32(Q+4) at `0x0084600C`.
The direct call to
`0x005692F0` at `0x0084601F` has receiver C+4+20*u32(Q+4) and the
single argument &localHoldingO. This local is ESP_before_`0x00846016`+
`0x4C`; O is stored there at `0x0084601B`, after the argument push.
The receiver simplifies to C+`0x68` when that reread is 5. The existing
[queue insertion contract](../actor/action-queue.md#proven-entry-population-and-insertion)
owns the concrete entry and pointer-insertion behavior. It does not prove
animation completion.

## Constructor context and packed-selector input

The selected context setup `[0x00845F1F,0x00845F94)` uses normal ESP B_S
and S=B_S+`0x1C`. Let A=u32(C+`0x228`), separate from the selected R
until their owner relation is established. At `0x00845F52`, S+0 receives A.
At `0x00845F64`, S+4 receives the input request pointer Q from
B_S+`0x4C`. These are pointers, not numeric actor identities.

`0x00843B50` installs primary table `0x0103E428` at O+0 and secondary
table `0x0103E3F4` at O+8. Original COL cells `0x0103E424` /
`0x0103E3F0` contain `0x0116E42C` / `0x0116E498`; the RTTI catalog's
rows 2377-2378 identify `Application::Scene::Actor::Chara::CharaActionQue`.
Primary slot-1 cell `0x0103E42C` contains `0x00846240`, independently
agreeing with the existing queue contract.

On the nonnull `0x33C` allocation branch, the call at `0x00843BE3` is
`0x00798AA0`, with that allocation's result as receiver and nearest-first
arguments `[u32(S+0),u32(S+4)]`, now `[A,Q]`. The returned visual pointer
is stored at O+`0x14`; the null-allocation branch stores zero there.
The [concrete visual construction](../actor/action-queue.md#concrete-visual-dispatch)
already establishes the final CharaActionVisual type and base-constructor
copy. That engine contract is reused here.

The [staged-record copy](../actor/action-queue.md#staged-type-and-packed-selector-domains)
copies `0x38 + row_count*0x14` bytes to visual+`0x2C`. Consequently,
this producer's Q+`0x10` is the packed-selector input copied to
visual+`0x3C`. `0x00798470` consumes that copied field with the published
packed split. No concrete serialized selector value or model-bank
assignment is supplied by this producer join.

The pointer wrapper `0x007A0F70` stores Q at its receiver T+0 and returns
T. It makes no call or Q dereference. Under ordinary distinct local/request
storage, that wrapper does not alter Q's bytes. The two Q-taking predicates
below preserve the selected fields on the ordinary type-5 path. Original
u32(D+`0x14`) is supplied to the packed-selector input field; its equality
with the later visual selector still depends on upstream target helpers,
aliases and compatible copying. Q+4's separate source/reread conditions
still apply to type dispatch and bucket selection.

## Two admission predicates and record preservation

At `0x00845EF6`, `0x0065DF30` receives ECX=u32(C+`0x228`) and `[1]`.
Zero AL skips Gate A. Otherwise `0x00845F02` calls `0x008440A0`, ECX=C,
with `[Q]`; zero Gate-A AL branches to `0x00845FB6`, while nonzero AL
continues. Gate B, `0x00844FF0`, receives the same `[Q]` at `0x00845F12`.
Its nonzero AL selects `0x00845FB6`; zero continues to `0x00845F1F`.
The initial actor predicate remains untraced and is not assigned a status name.

Gate A reads only u32(Q+`0x10`) at `0x008440B1`. At `0x008440D2`, the
existing scalar decomposer `0x00798370` receives that copied dword and three
distinct local output-word addresses. Q is not an explicit argument. Gate A
returns AL=1 exactly when the outputs, in argument order, are `0x7F`, 0 and
unsigned 100..199. It writes only its local frame. These are compiled
predicate constants, not observed serialized selector values or game-command
IDs, and are distinct from the BG clip's native dispatch selector `0x7F`.

Gate B's wrapper T is its incoming argument cell, entry ESP+4, containing Q.
It is not the request base. `0x007A0F70` writes the same pointer into that
cell. Existing `0x007A14E0` reads the selector high byte and tests it against
`0x19`; `0x007A0F90`, called at `0x00845040`, is one RET with no memory
access or call. Gate B contains no direct Q or controller-field write.

False high-byte predicate returns zero. Otherwise Gate B requires
`u32(type-6)<=1`, so an unchanged ActionCall type 5 returns zero before the later
controller reads/call. For types 6/7, AL=1 additionally requires all of:

- Nonzero u32(C+20*type+20), the indexed bucket count.
- Ordered f32(C+`0x22C`)>0.0; NaN and nonpositive values return zero.
- Nonzero `0x00844B60(C,[type])` result.

The exact threshold is binary32 `[0x01086650,0x01086654)`, SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
`0x00844B60` remains uninspected and is unreachable for unchanged type 5.
No timer, phase, collision or reset meaning is assigned to C+`0x22C`.

Under ordinary disjoint record/controller/argument/local storage, normal
x86 ABI and the published scalar-helper contracts, these two gates preserve
Q+4 and Q+`0x10` on this type-5 path. That is not an unconditional equality
between original D+`0x14` and the later visual selector through all upstream
target helpers or aliases.

## Remaining joins

The source-qualified staged-record copy and packed-selector input role are
established under their guards. The upstream target-selection helpers
`0x0081CFA0` and `0x0081D120`, aliases and actual source values remain
qualified. Resource compatibility, successful playback, completion/settlement and
encounter actor/add/variant membership remain unestablished. A packed
playback selector is distinct from a game-command ID and a model-qualified
WSS/BID bank. Historical actor binding does not prevent this static join.

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
| `0x00843B50` | `[0x00843B50,0x00843CE9)` | `d81fbb356614a547dc1cd9bbe3c578b3c63eb84c807c396a9d7f4b045c7fcacd` |
| `0x005692F0` | `[0x005692F0,0x00569365)` | `baaf7459d908c2bb963526dd7862ec361d520dfdaffbd59ede6dc26fb7ab80e9` |
| `0x007A0F70` | `[0x007A0F70,0x007A0F7B)` | `0d9d4797ad8c81da5b9f78b6e3eba5066720c9a41fbd098a5dac1e8581c3c29c` |
| `0x008440A0` | `[0x008440A0,0x00844103)` | `f946ed4f8cec8273cf19caea158340414b2207c777b6466f4e17a57bddcafaf0` |
| `0x00844FF0` | `[0x00844FF0,0x008450A9)` | `363c9d0c85b5f92d847d5bf14d4c2113da9e4ae5cf8eed6a63c8e0f834760f01` |
| `0x007A0F90` | `[0x007A0F90,0x007A0F91)` | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` |
| `0x00445210` | `[0x00445210,0x00445213)` | `19d200194d49b34530548575f37a1bb4203ee60f18d42f0084e86b7a8c899faa` |
