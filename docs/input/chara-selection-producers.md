# Desktop and CharaElement selection producers

The RaptureElementContainer construction path writes its receiver W to
desktop global `0x01356E10`. A fresh CharaElement's interface I = E+8 has
admission bit0 cleared. Interface selection through slot14 is a separate
operation: it tests bit0 and slot23 before it can write global selected
interface `0x01357020`. The selected CharaElement slot23 returns true, but
that does not supply bit0.

These are retained-file observations from retail 1.23b `ffxivgame.exe`,
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs with image base `0x00400000`; RVA = VA - image base.
The method matched selected retained Ghidra assembly instructions to the
PE's raw-backed bytes and decoded those windows with Capstone 5.0.7.
RTTI complete-object locators and vtable cells were read directly from the PE.

## Desktop creation and lifetime

`0x004B2DF0` requests `0x17D58` bytes at `0x004B3656..0x004B3663`.
If allocation returns nonnull, `0x004B3697` calls `0x004DC3A0` on that
allocation. Its null arm supplies zero instead. In the constructor,
`0x004DC401..0x004DC414` passes outer object+`0x10` as the receiver of
`0x004DBF40`. Call this embedded receiver W.

`0x004DBF40` preserves W in ESI and calls `0x0091B020` on W at
`0x004DBF6D`. The base constructor preserves the same receiver in ESI and
writes it to `0x01356E10` at `0x0091B2F3`. Its `0x01069224` interface
table has RTTI name `.?AVDesktopWindow@Sqwt@@` and complete-object offset
`0xB4`. After that call returns, `0x004DBFB1` installs `0x00F911BC` at
W+`0xB4`. That table's complete-object locator at `0x01141AE4` identifies
`.?AVRaptureElementContainer@Main@Application@@`, also at offset `0xB4`.
Thus W is the desktop-global value on this construction store path.

The [constructor attachment](chara-interface-attachment.md) uses this W
as the type-8 factory receiver and the parent of the E+8 interface. If W
still equals the nonnull desktop global at enrollment, its direct parent
W+`0xB4` is the desktop sentinel used by `0x0091C370`. This is a concrete
construction relation. It does not establish persistence through every
intervening call or the current global's equality to a live W.

Teardown is a separate producer. `0x004DBDEE` calls `0x0091AE80` on W.
After its cleanup calls, `0x0091AEFB` zeros EBX and `0x0091AEFD` writes EBX
to `0x01356E10`. That clear has no comparison against the current global
value or the destructor receiver. This differs from the selected-interface
clear at `0x0091DC83..0x0091DC8D`, which writes zero to `0x01357020` only
when the current value equals the interface being destroyed.

## CharaElement admission bit

The CharaElement constructor `0x0058B4E0` installs interface table
`0x00FA7B2C` at E+8 at `0x0058B542`. Its complete-object locator
`0x01148A7C` identifies
`.?AVCharaElement@Chara@Element@Main@Application@@` at offset 8.

| Interface cell | Target | Observed operation |
| --- | --- | --- |
| `0x00FA7B30`, slot1 | `0x0091C3D0` | Return I+`0x70` bit0 |
| `0x00FA7B34`, slot2 | `0x0091C3E0` | Assign I+`0x70` bit0 from argument bit0 |
| `0x00FA7B64`, slot14 | `0x0091E3E0` | Conditional interface selection |
| `0x00FA7B88`, slot23 | `0x0091CDC0` | Exact `MOV AL,1; RET` |

At `0x0091C3E0..0x0091C3E9`, the setter computes
`I[0x70] ^= (I[0x70] ^ argument_byte) & 1`. Other bits are preserved.
`0x0058BA08..0x0058BA46` pushes the constructor's zero EBX, supplies E+8
as ECX, and calls this setter. The base RaptureElement constructor also
passes zero at `0x004DABEB..0x004DABEE`.

The literal direct-CALL search in the retained assembly found these setter
call sites. The selected argument-setup windows were matched to the PE.
All supply zero in the inspected constructor paths.

| Caller | Setter CALL VA |
| --- | --- |
| `0x004DAB50` | `0x004DABEE` |
| `0x00564F80` | `0x00565035` |
| `0x00566380` | `0x00566570` |
| `0x00566880` | `0x005669DE` |
| `0x0058B4E0` | `0x0058BA46` |
| `0x0098A240` | `0x0098A342` |

This search covers literal direct calls in retained listings. It does not
exclude virtual slot2 callers, inline writes or writes through aliases.
A concrete later caller that writes one to
the selected CharaElement interface remains unjoined. The exact bounded
edge is the E+8 receiver at `0x00FA7B34` -> `0x0091C3E0`, with argument
bit0 equal to one. Desktop construction and a nonnull desktop global do
not supply that interface argument.

## Returned interface aliases

The constructor installs final E-base table `0x00FA7C50` at
`0x0058B53C`, immediately before the E+8 table store. Its complete-object
locator `0x01148980` identifies the same CharaElement at offset zero.
The following slots operate on E, rather than on I.

| E-base cell | Target | Observed operation |
| --- | --- | --- |
| `0x00FA7C7C`, slot11 | `0x004D67B0` | Return E+8 for nonnull E, otherwise null |
| `0x00FA7C80`, slot12 | `0x004D67C0` | Exact `XOR AL,AL; RET 4` |
| `0x00FA7C84`, slot13 | `0x004D67D0` | Call slot12 with the supplied argument; return E+8 only for nonzero AL |

With this final table, slot13 takes its null-result arm
(`0x004D67D9..0x004D67EF`). Slot11 provides a returned E+8 alias that a
search limited to literal caller-side `LEA ECX,[E+8]` would miss. Neither
getter assigns the interface's admission bit. These conclusions depend on
dispatch through the selected final table; they do not fix another table's
slot12 result or establish a later setter call.

The attachment helper `0x004DA9A0` demonstrates a slot11 result consumer:
`0x004DA9CE..0x004DA9D9` calls E-base slot11, passes its returned pointer
to `0x0091C490`, and tests the interface's parent field at I+`0x60`.
Its following parent-attachment arm obtains the interface again through
slot11 (`0x004DA9E2..0x004DA9FE`). That use supplies an alias-tracing
pattern, without supplying a positive bit0 assignment.

## Selected interface requests and event listeners

Final CharaElement interface table `0x00FA7B2C` supplies slot25
`0x0091CD60` and slot26 `0x0091CD90`. Slot25 calls slot26 at
`0x0091CD72` and tests its result at `0x0091CD74`. The selected slot26
body is exactly `XOR EAX,EAX; RET 8`, so this dispatch takes the false
arm at `0x0091CD84`. Its subsequent slot14 call at `0x0091CD7F` is not
reached under this table. This request route does not assign admission bit0.

Interface slot13 `0x0091D320` dispatches events using descriptor byte5
minus one, bounded to `0..0x52`. The byte map at `0x0091D818` selects
targets from the table at `0x0091D760`. The switch tail
`0x0091D514..0x0091D75E` (exclusive end) was decoded directly from
raw-backed PE bytes with Capstone 5.0.7. Those rows are absent from the
primary retained listing. The map and jump table were read as data.

Its fallback at `0x0091D73A` requires nonnull sender [I+`0x64`], unhandled
event byte `+0x17 == 0`, and nonnull dispatcher [I+`0x4C`]. It then calls
the dispatcher's slot1 with `(sender, event)`. Construction through
`0x004DAB50 -> 0x0091E090` clears I+`0x4C` at `0x0091E0F7` and
stores E at I+`0x64`/`0x68` at `0x0091E11A/0x0091E11D`. Sender is E
if that construction store persists.

Helper `0x0091F620` lazily supplies dispatcher B at I+`0x4C`. On successful
allocation of `0x24` bytes, it installs table `0x00FA6788` at B and stores
B at I+`0x4C` (`0x0091F680/0x0091F686`). That table's slot1 cell
`0x00FA678C` points to `0x00571520`. This dispatcher obtains listener R
from its registration vector at `0x005715FD`, then calls R's vtable cell
`+8` at `0x0057160A` with two arguments `(sender, event)`.

The listener call and I.slot2's one-argument admission setter have different
call tuples. A shared numerical vtable offset does not establish the same
receiver or operation. The registration producing R, its concrete callback
table and any forwarding from that callback to selected I remain unjoined.
Dispatcher allocation alone does not establish a registered listener or
a positive bit0 assignment.

## Listener registration and stored callback owners

The final Chara interface table supplies these registration methods:

| Interface cell | Target | Observed operation |
| --- | --- | --- |
| `0x00FA7B50`, slot9 | `0x0091F6F0` | Set R+`0x18`, then call `0x00571BB0` on B+8 with `(R,0)` |
| `0x00FA7B54`, slot10 | `0x0091F6D0` | Set R+`0x18`, then call `0x00571D10` on B with R |
| `0x00FA7B5C`, slot12 | `0x0091F710` | Pass R to (B+8).slot1 |

At `0x0091F6DD`, slot10 stores its first argument, the event descriptor,
at R+`0x18`. Helper `0x00571D10` passes the address of R to vector helper
`0x00813050` on B+`0x0C` at `0x00571D2C`. It sets R+`0x16` bit0 at
`0x00571D31` and records B+8 through the reciprocal vector at R+4
(`0x00571D41`). The vector helper's capacity-available arm writes the
supplied pointer at its end and advances that end by four
(`0x00813080..0x00813087`). Its growth arm delegates to `0x008CFC80`;
the deeper allocation behavior is outside this observation. Slot9's
`0x00571BB0` path clears R+`0x16` bit0 at `0x00571BD7/0x00571BDA`.
These listener-flag operations are distinct from I+`0x70` admission bit0.

The retained literal-call search found eight calls to `0x0091F6D0` in
`0x0069D140`, `0x0086BE80`, `0x0088A750` and `0x008DA6A0`.
Their receiver setup uses a UI owner or loaded child pointer plus `0xB4`.
On their allocation-success branches, listener constructors install tables
`0x00FCB6B4`, `0x00FCB6C4`, `0x00FCB6D4`, `0x0104C8D0`,
`0x01055338` or `0x01064AD4`. Their RTTI identifies Sqwt UI event
handlers; these calls do not identify a listener registered on selected I.

Those tables' callback targets are `0x0088AB00`, `0x00966990` and
`0x0066FCC0`. The first loads ECX from `[R+0x20]+[R+0x2C]` and
tail-jumps through `[R+0x28]`. The second adds an optional event-field
filter before the same forwarding. The third loads ECX from `[R+0x20]`
and tail-jumps through `[R+0x24]`. The inspected constructors store the
supplied callback owner and target in those fields, with zero adjustment
for the first two forms. Sender and event remain stack arguments.

These thunks distinguish the stored callback owner from the sender.
They do not establish owner=selected I or transform sender E into E+8.
The downstream callback bodies, other virtual registration paths and
possible aliases remain separate joins. The bounded registration trace
supplies no positive selected I.slot2 argument or ordering before slot14.

## MoveCharacter service and selected interface

Caller `0x0054B440` obtains reporting selection through `0x004D72E0`,
casts it to CharaElement and rejects a null result. It forms I=E+8 at
`0x0054B507`. The call at `0x0054B521` uses service S as ECX and passes
`(temporary, I, 0)` to S.slot2. S is the return from `0x00928F90` with
argument `0x013370B0`, a string-key object initialized from
`RaptureCommands.MoveCharacter` at `0x0054B4CD..0x0054B4D7`.
The raw string is at `0x00FA22D8`.

The lookup searches stored objects by their key at S+`0x24` through
`0x00928280`. Its successful existing-object arm returns the match without
checking its table. On a miss and successful allocation, `0x00928C90`
constructs S and installs table `0x0106A78C` at `0x00928CFC`.
Its complete-object locator `0x01179454` identifies
`.?AVRoutedCommand@Input@Sqwt@@` at offset zero. Slot2 cell `0x0106A794`
points to `0x00927FA0`. The following dispatch describes this constructed
table; lookup success alone does not establish an existing object's table
or later table persistence.

That method preserves argument2 I and calls `0x0091F590` with an event
and I at `0x00928059`, then conditionally at `0x00928124`. The event
constructor `0x00929100` stores S at event+`0x1C`/`0x20`. Initializers
`0x00F21C80` and `0x00F21CB0` populate the event keys as follows:

| Key | Route byte | ID byte | Name |
| --- | --- | --- | --- |
| `0x0135872C` | 2 | `0x29` | `PreviewExecuted` |
| `0x01358738` | 0 | `0x2A` | `Executed` |

These initializer windows were decoded from raw PE bytes; no retained owner
listing covers them. Their called constructor `0x0097F6C0` stores the route
at key+4, ID at key+5 and name pointer at key+8. Event identity depends on
those initialization stores persisting. The second event is skipped when
the first event's handled byte becomes one.

`0x0091F590` forwards `(event, I, 1, 1, 1)` to `0x0091E1C0`.
After its event-key and sender checks, that helper adds I and its parent
chain to a local route list. `0x0091D210` dispatches the stored recipients
through slot13. For selected I's table, `0x0091D320` maps these event IDs
to slot64 `0x0091CC10` and slot65 `0x00589490`, through the raw-backed
branches at `0x0091D6AA` and `0x0091D6BC`. This route list is separate
from the persistent listener registration described above.

The Executed handler's MoveCharacter arm requires event S to equal
`0x013390D8` at `0x0058950A`, and the effective-selection getter on
[I+`0x74`] to return I-8 at `0x0058951B..0x00589520`. Constructor
`0x00F0D350` initializes that global command object with the same string
key. Key equality does not prove that a lookup returned that exact object.

On this guarded arm, event+`0x7C`/`0x80` supplies the new movement bytes
at I+`0x25C`/`0x25D`. The original caller's temporary supplies zero for
both. Nonzero override words at I+`0x274`/`0x276` replace those values
and are cleared. The handler sets I+`0x278` and event handled state.
Its delegated `0x0091D870` helper uses I+`0x70` bit4 to qualify a
command-membership test for `(I, S)`. Those visible stores and the membership
test do not assign admission bit0. Conditional listener calls and other
indirect effects remain separate from the inspected direct writes.

When the movement bytes change and I+`0xE8` is `0x0B` or `0x0D`,
`0x00589556` first calls `0x00574D70` with ECX=[I+`0x78`]. The thunk
loads D from that pointer and tail-jumps at `0x00574D77` through
D's table+`0x5C`, slot23, adding no stack arguments. The field holds an
actor-implementation handle L=[I+`0x78`]=[E+`0x80`], with D=[L]. Its
construction and conditional replacement select the targets below.

## Selected actor-implementation handle and slot23

The CharaElement constructor calls `0x00534090` at `0x0058B528`, which
reaches RaptureElement constructor `0x004DAB50`. Its `0x005752D0` call
at `0x004DABAC` returns L, stored at E+`0x80` at `0x004DABB1`.
With a nonnull supplied context and successful allocations, L is a
four-byte holder whose pointee is constructed through
`0x00574700 -> 0x00769FD0`. The latter installs table `0x00FE02AC`.
The [fresh case-2 finding](../resource/attribute-resource-and-scene-binding.md#case-2-appearance-ordering)
owns its separate appearance-admission effects.

Raw PE complete-object locators identify these zero-offset actor
implementation tables. Their receivers are D, separate from the selected
Chara interface I on these construction paths.

| D table and RTTI | Slot23 cell | Target and bounded operation |
| --- | --- | --- |
| `0x00FE02AC`, `.?AVNullActorImpl@Client@Script@Lua@Application@@` | `0x00FE0308` | `0x0075AC50`, exactly `RET` |
| `0x00FDFB2C`, `.?AVLuaActorImpl@Client@Script@Lua@Application@@` | `0x00FDFB88` | `0x00759920`, conditional movement script callback below |

L's pointee can change. The packet binding arm at `0x004D8902` passes
L=[E+`0x80`] to `0x00574780`. When its current pointee is not LuaActorImpl,
the NullActorImpl branch forwards L to `0x00774AD0`. Request constructor
`0x005745F0` stores L at request+8 (`0x0057462D`). The inspected immediate
submission arm requires owner state 10 and further flag/result tests; it
retains the request with its key in the owner's callback list. Other requests
are deferred. Processing at `0x00766F00` requires successful script-object
lookup and readiness before calling completion `0x00574830` at `0x0076703A`.
Submission alone does not establish completion.

For the preserved request, completion recovers L from request+8 and calls
`0x0076C8D0` with that script control object. Its successful allocation calls
`0x00759510`, which installs the LuaActorImpl table at `0x00759549` and
stores the control pointer at D+8. Completion transfers the returned D
to *L at `0x005748CE`, without replacing E/I. The inspected reset route
`0x00574970` can also replace *L through current D.slot4; both named tables'
slot4 targets construct NullActorImpl. These are concrete conditional
producers, not a census of every alias or replacement writer. The target
used at `0x00574D77` depends on successful construction or replacement
and persistence. With the fresh NullActorImpl table retained, it returns
immediately without an admission write or registration.

LuaActorImpl slot23 passes `(D+4, [D+8])` through `0x008A3E60`. That
helper casts the second pointer from ActorBase to MyPlayer, then calls
`0x006F8200` with the cast result A as ECX and D+4 as its argument.
It has no null-cast guard before this call. For a valid A with A+`0x167`
nonzero, `0x006F82C0` submits A, the literal `_onMoveAtSit` at
`0x00FD4B4C`, and a result vector through `0x00CC7A90`. That wrapper
resolves a script instance through `0x00CD7A30` and forwards the
instance/name/result tuple to `0x00CCF7D0` at `0x00CC7ABC`. The latter's
script-state and name-resolution guards precede its execution-helper call.
Exactly one type-2 result whose extracted byte is one allows `0x006F8310`
to clear A+`0x167`.

This direct flag clear belongs to the MyPlayer control object. The inspected
native path supplies no positive selected I+`0x70` bit0 assignment or
registration involving I. The movement guard's I+`0xE8` is E+`0xF0`;
the inspected writers `0x00588BA0` and `0x005870B0` copy supplied values
there without choosing D's table. Values `0x0B`/`0x0D` alone do not
establish a successful binding or script identity. The selected
`_onMoveAtSit` script instance/handler and its effects toward I remain
unjoined, as does ordering before admitted I.slot14.

## _onMoveAtSit lookup and conditional command request

At `0x00CC7ABC`, the wrapper forwards resolved instance S, the name and
results to `0x00CCF7D0`. Its guarded path calls `0x00CCCD80` for name
resolution before `0x00CCECB0` for execution. Table recovery through
`0x00CD7A50` uses S's reference keys and can fall back to the context's
`+0x1BC` object. Lookup `0x00CD7B00` performs raw table lookup and follows
the native `+0x0C` parent only for nil. A non-nil value stops that search;
the subsequent type-6 test rejects a non-function. A successful parent
lookup preserves the supplied receiver table rather than substituting the
parent as self. These instructions do not identify the actual loaded table,
override, function or receiver-table recovery result.

The extracted PlayerBaseClass `_onMoveAtSit` candidate returns the result
of global `desktopWidget:executePlayerSystemCommand(24312, nil, 1)`.
Its supplied self is unused. This route is conditional on the lookup choosing
that function and the globals/methods resolving to the inspected bodies.
The connector obtains M through `worldMaster:_getMyPlayer()`, then looks up
static command C for 24312 through `getSystemCommand` and checks it with
`checkActor`. It repeats that lookup and check. The inspected branches keep
variation nil, secondary value 1 and target nil. Exact false from its
`canCommand` check reports an error and returns false; otherwise it calls
M's `command`. That method repeats `canCommand` and requires exactly true
before calling `_executeCommand`.

These script observations use extraction `2012.09.19.0001`,
`xivl-client-scripts:manifests/scripts.json` at
`7da433d547b2b91064e1f8a534a92d7236394ea7`. Complete script bytes, hashes
and line counts were matched to their manifest rows:

- `lua/scripts/chara/player/playerbaseclass.lua`, lines 1802-1817,
  1911-2024 and 2987-3000, SHA-256
  `6226B3FA15DFDBAD279B7DBA453F8A3B76FCB8B68BAD6E14F5403D52987F76E4`.
- `lua/scripts/widget/desktopwidget_connector.lua`, lines 857-871 and
  13790-13980, SHA-256
  `9C33F21C1F70A0056147E716D53300634EFABE5B744EF6E8690114DB21613A01`.
- `lua/scripts/command/commandbaseclass.lua`, lines 4-26 and 117-125,
  SHA-256 `BF7DD5FA7A6530C0C3B683BE16A252B7FFD62492BDC268F36322977DEE4D3E31`.
- `lua/scripts/chara/charabaseclass.lua`, lines 399-496, SHA-256
  `F17B3E62A137D8F524BA88D55296E1FB064512DC803F7A85DC980DD4BCBC9625`.

The static-actor extraction maps 24312 to `/Command/System/EmoteSitCommand`:
`xivl-client-data:manifests/staticactor_class_paths.json`, row `id=24312`,
at `30d90aa0d78dce17aa2378fe244ebc9c40b8e344`, manifest SHA-256
`d612438827e5997422ab6f64a807e567ddf1b953c532e8a319d67b93c53c9db0`.
This mapping does not authenticate the actual C, loaded class or overrides.
`getCommandName` depends on C's `getPriority`, whose base implementation
requests `commandSheet:_getData(C:getCommandId(), 29)`. For a system command,
priority -1 selects `commandRequest` and priority 4 selects `commandContent`;
other values can leave the name nil. The actual sheet value and method/global
selection remain conditions; the numeric ID alone does not choose the name.

The native current-player producer is concrete. Registration `0x00752750`
associates `_getMyPlayer` with callback `0x00707970`. In its actual context X',
that callback recovers Q=[X'+`0x220`], K=[Q+4], P=[K+`0x0C`] and returns
P as an actor value. Setup `0x007634B0` wraps its second argument K through
`0x00779B50/0x00778D80` and retains the holder through
`0x00CC9820/0x00CD78E0`. The slot catalog identifies table `0x00FDFFD0`
as `Holder<CoreWork*>`. Thus the getter supplies the CoreWork's `+0x0C`
pointee; it does not prove equality of X' with [H], P with A, or M with the
original handler receiver. The globals and their contexts were not observed.

After `_executeCommand` returns, PlayerBaseClass `command` assigns
`M.playerWork.commandBurstBlocker=1`, even when the returned value is false,
and propagates that value. This is a script work-field assignment, without
an authenticated native alias to I+`0x70`. The outer native result check and
A+`0x167` clear remain the separate operation described above.

Registration `0x0073F080` installs `_executeCommand` thunk `0x006DE650`,
which loads the receiver table's `+0xA8` target. MyPlayer table `0x00FD785C`
slot42 supplies `0x0070A010`, conditional on P's actual table. That method
resolves native command object N by name through `0x0071CA50` on P+`0xFC`.
The lookup returns the first equal entry without checking its table. N and
static command actor C are distinct representations. The second argument's
type-4 actor pointer passes through `0x0071BB80` and
`0x00CC73B0/0x00CD7A30`; `0x00CC7030` copies its script instance's `+0x70`
reference R_C. These pointers and references are not identified with I.

After target/event and argument guards, `0x0070A2C1` calls `0x00898480`
on G=[P+`0xF8`] with stack arguments `(P, N, &R_C, &decoded-parameters)`. At
`0x0089848F`, exact bytes `FF D2`, ECX=N and the target is
[[N]+`0x1C`], slot7. Returned AL==1 selects `0x00897310`; other AL values
select `0x00896510`, both on G with reordered stack arguments
`(N, P, &R_C, &decoded-parameters)`. The latter generic route
links a command entry into G+`0x10` before calling `0x008963F0`. This
conditional manager-list insertion does not establish selected-I registration.

The bounded trace supplies no positive selected I+`0x70` bit0 assignment or
ordering before admitted I.slot14. The construction and manager trace below
bounds two conditional N table targets. Actual S/function/parent resolution,
global contexts, P=A, C and the derived command name remain separate
conditions. These findings use retained Ghidra
listings, raw-backed PE reads and Capstone 5.0.7 decoding; explicit listing
gaps were retained without claiming continuous instruction coverage.

## Native command construction and policy dispatch

Native producer 0x006F1C20 constructs a command object from a supplied name
and two byte pointers, then forwards it to the vector at P+0xFC through
0x00725ED0. Equality of its supplied type byte with [0x0134C3FE] selects
0x00892080, whose final table is 0x01056CC4, CommandBlock. Inequality
selects 0x008921A0, whose final table is 0x01056CF8, CommandNonBlock.
Raw complete-object locators 0x01171EEC/0x01171F40 authenticate those
zero-offset types. The type-selector global is outside the retained PE's
raw-backed extents.
Both constructors copy the supplied name to N+4 through 0x00891FB0.

0x00762CA0 supplies name/type records to that producer in 0x58-byte steps,
passing (record,record+0x54,record+0x55). 0x006F3420 calls this walk with
its receiver P. A separate dynamic producer at 0x0089D4F0 supplies another
record to the same helper. The inspected available-capacity append arm
writes at the vector end. These are conditional registration producers,
without actual matching-record, uniqueness, complete alias-writer,
lifetime or table-persistence evidence. 0x0071CA50 still returns the first
equal name in existing vector order without a table test.

While those constructed tables remain installed, their slot7 cells select
0x00B73290, exactly MOV AL,1; RET, or 0x005C5C80, exactly XOR AL,AL;
RET. Neither body writes memory. At 0x0089848F the first selects manager
0x00897310 and the second 0x00896510. The latter links a command entry
into G+0x10 before calling N.slot1. The former applies priority/state
tests and can replace G+8 and call N.slot3 then slot1, stage through
G+0x0C, or reject. Its priority and existing-entry conditions remain
separate from command-table construction.

Both tables' slot1 target 0x00897910 adds N's name to its argument object
and calls N.slot10. The concrete slot10 targets 0x008920F0/0x00892200
call 0x00CD0940 with P and the raw method names _onCommandEvent (CommandBlock) or
_onCommandRequest (CommandNonBlock). That helper forwards P to
0x00CD7A30 at 0x00CD0961, then forwards its result to 0x00CCF9B0 at
0x00CD0969. The actual loaded handler and its selected-I effects remain
unobserved.

These bounded manager and script-dispatch effects do not establish a
positive selected I+0x70 bit0 assignment, input registration, or ordering
before admitted I.slot14. The input and initialization trace below bounds
conditional record construction and ownership transfer. The selected payload
and first-match instance history remain unobserved.
This finding uses the retained 15996808-byte retail 1.23b ffxivgame.exe,
SHA-256 9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000, retained Ghidra listings and fresh Capstone 5.0.7
row/byte checks. Listing gaps were retained without claiming continuous
instruction coverage.

## Pending native command record input

The incoming message-word dispatch at 0x004D8860 maps word 0x0132 to
0x004D899B: raw map byte 0x004D8C8A is 6 and target cell 0x004D8BD8
is 0x004D899B. That arm forwards incoming object+0x10 through holder
[U+0x80] to 0x00574FC0, where U is the function's receiver. The helper
jumps through its implementation's slot49. U is not an observed selected E.
This finding does not identify an outer wire route or translated opcode.

NullActorImpl table 0x00FE02AC supplies slot49 0x0076CA90. With that
target selected, payload+2 supplies the name constructor, payload byte0
supplies B and payload byte1 supplies T. Helper 0x0075AEE0 replaces zero
B with byte[0x01355296], which is not raw-backed in the retained PE.
Helper 0x0076A3F0 reuses or conditionally constructs owner O at
implementation+0x48. Producer 0x00767B40 constructs a record containing
the name and B/T at +0x54/+0x55, then forwards it to O+0x14.
The available-capacity arm of 0x007836D0 constructs at the vector end
and advances that end by 0x58. Other growth-path ordering is not established
by this bounded trace.

The 0x00762CA0 walk consumes records of this layout, and 0x006F3420
supplies its second argument as owner and receiver P to that walk.
A conditional static handoff is concrete. 0x00774AD0 moves its incoming
implementation's +0x48 owner through 0x00774530. Its admitted immediate
arm forwards O to 0x007687F0; its deferred arm retains O at descriptor+0x134
until consumption by 0x00772C80. Successful initializer construction
0x00781750 stores O at F+8 and installs table 0x00FE029C.

Helper 0x0078BBB0 obtains P_init through 0x00CC7800 and calls F.slot1,
0x00781850. That callback clears F+8 and passes O to 0x006E0150.
The latter calls P_init.slot30 at 0x006E01AB with O as its second argument.
MyPlayer table 0x00FD785C supplies 0x006F3420, conditional on P_init's
actual table. No table test occurs at that call. These call sites alone do
not establish D_message=D_handoff or P_init=later P. The same-holder and
publication paths below supply conditional static joins.

Owner construction at 0x00767730 zeros the record-vector endpoints. Cleanup
0x00765E30 -> 0x0077C9B0 destroys the records and clears those endpoints.
These paths establish conditional creation and teardown, without an actual
payload, complete invocation order or selected lifetime.

LuaActorImpl table 0x00FDFB2C has a different slot49 target,
0x00759DE0, exactly RET 4. It makes no write and does not execute this
NullActorImpl producer while that table remains installed. The earlier
conditional implementation replacement does not authenticate the table
used for any particular incoming message.

The initializer cleanup 0x007817D0 destroys only a nonnull owner still
retained in F+8. The MyPlayer slot30 path later reaches pending-owner
cleanup through 0x006F0220 -> 0x006EECD0 at 0x006EED6B/0x006EED71.
N+4 is a separately constructed string object through 0x00447200. These
operations do not establish N lifetime or retained table at the later lookup.

The first equal name in P+0xFC still depends on actual payload/name,
D_message-to-D_handoff and P_init-to-P identity, successful phase completion,
insertion history, lifetime and installed-table persistence.
The inspected PE/listings do not supply that selected instance history.
No positive selected I+0x70 bit0 assignment, input registration or order
before admitted I.slot14 was established.

These observations use the retained retail 1.23b ffxivgame.exe,
15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000, retained Ghidra listings, raw PE data and fresh
Capstone 5.0.7 row/byte checks. Listed instruction gaps were retained;
no continuous body extent was inferred from listing headers.

## Conditional holder and current-player publication joins

The retained static callers join the pending-owner message paths
conditionally. Ordinary word 0x00CC dispatch at 0x004D88E2 uses the same
[U+0x80] holder as word 0x0132. Helper 0x00574780 rejects a successful
LuaActorImpl cast, casts the retained implementation to NullActorImpl and
calls 0x00774AD0. An earlier record producer and later admitted ordinary
0x00CC call therefore use the same implementation when U, its holder,
implementation and lifetime persist. The preliminary U+0x92 guard can select
a separate 0x00CC path. Actual selected-U identity and message order remain
conditions.

The deferred creation path 0x00772C80 compares its descriptor's first word
with [W+0x1783C], distinct from reporting selection [W+0x17834]. Its equality
arm appends "MyPlayer", constructs a MyPlayer prototype through 0x006F9140
and supplies it to 0x00CC8010. Registration and subsequent class/name lookup
must succeed. On the MyPlayer prototype arm, 0x00CDCBA0 -> 0x00CE2D50 ->
0x00CCC9B0 calls prototype.slot1, whose target 0x00749180 constructs a
separate MyPlayer. Its final table is 0x00FD785C.

Before returning that created actor, 0x00CDCBA0 calls 0x00CE2440 at
0x00CDCEB0. That helper forwards the same actor and context through
0x00CCC9C0 to actor.slot3. With the MyPlayer table installed, target
0x006E3440 calls 0x00758E40, storing its receiver at CoreWork+0x0C.
The returned instance wrapper R is dereferenced by 0x00CC7800. With [R]
retaining its created-actor pointer across the intervening calls, 0x0078BBB0
supplies that same actor to the initializer callback. Under that condition,
this successful path publishes P_init before applying the pending owner.

Setup caller 0x0057A3C0 stores the result of 0x00773270 at R_setup+4
and passes it as argument2 to 0x007634B0 at 0x0057AAB1. Setup wraps
K_setup in a holder at Q+4 and retains it at X_setup+0x220. This fixes the
argument producer without proving X_setup=X_lookup or K_setup=K_publish.

The later _getMyPlayer callback 0x00707970 reads its actual context's
+0x220 holder, the holder's +4 CoreWork pointer and CoreWork+0x0C.
P_lookup=P_init therefore requires K_lookup=K_publish, the retained
publication and the relevant lifetimes. Actual payload/name, class selection,
table persistence, insertion order, competing registrations and the first
equal P+0xFC entry remain unobserved. These static joins do not establish a
selected-I bit0 assignment, registration or order before admitted I.slot14.

These observations use retained Ghidra listings and individually checked
raw-backed PE instructions/data from retail 1.23b ffxivgame.exe, 15996808
bytes, image base 0x00400000, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
with pefile 2024.8.26 and Capstone 5.0.7. Listing gaps remain explicit.

## Container selection getters and command arguments

The container exposes reporting selection and a separate override pointer.

| Getter | Returned value |
| --- | --- |
| `0x004D72E0` | `[W+0x17834]`, reporting selection |
| `0x004D86B0`, `0x004D86D0` | Nonnull `[W+0x17838]` override, otherwise `[W+0x17834]` |
| `0x004D72F0` | `[W+0x17838]`, override only |

`0x004D9980` can clear the override for a sentinel ID or store the result
of `0x004D9910`, whose lookup returns null or the pointer stored at node+`0x10`. This does not
establish equality with the reporting-selected CharaElement.

Wrapper `0x0075C770` uses the override-only getter and passes its result Q
and value one to `0x00587020`. It supplies no reporting-selection equality
or type test. The latter method forms payload `{u32 0, u32 value}` and
calls `0x004D7980` with command `0x47` and length 8. That helper uses
Q+`0x88` as ID, adds `0x15` to form event code `0x5C` at `0x004D79D2`,
and dispatches through [Q+`0x84`] slot3 at `0x004D79F6`.
The literal one is command payload data; these instructions do not supply
it to I.slot2. Q=reporting E, Q's provider and later event-recipient effects
require their own joins.

## UIElement property callback receiver

Property callback `0x0093B700` contains a slot2 call at `0x0093B8C7`.
Its arm at `0x0093B8A5` supplies `(byte[O+0x15C] >> 5) & 1` and uses
O+`0xB4` as the interface receiver. The following construction and callback
path identifies O without an adjusting thunk.

UIElement constructor `0x0093C750` preserves its incoming O and constructs
InputElement at O+`0xB4` through `0x0091E090`
(`0x0093C791..0x0093C7A2`). It then installs base table `0x0106C3CC`
at O and interface table `0x0106C2A4` at O+`0xB4`
(`0x0093C7D1/0x0093C7D7`). That interface table's locator `0x01179BA4`
identifies `.?AVUIElement@Sqwt@@`, complete-object offset `0xB4`.
Its hierarchy gives InputElement and IInputElement the same fixed offset.

Control constructor `0x00938C80` reaches this base constructor through
`0x0092F640`, retaining O. It installs Control tables `0x0106BB54` at O
and `0x0106BA2C` at O+`0xB4` (`0x00938CE4/0x00938CEA`). The latter
table identifies `.?AVControl@Controls@Sqwt@@` at offset `0xB4`.
It then calls property setter `0x0093C3C0` with the same O and value one
(`0x0093905C..0x00939063`).

The setter passes O, callback `0x0093B700` and explicit adjustment zero
to `0x0099C780` (`0x0093C3C0..0x0093C3ED`). If the helper's admission,
changed-value and notification conditions reach its callback path,
`0x0099C7F5..0x0099C806` forwards that tuple to `0x0099C680`.
The dispatcher adds the adjustment to O at `0x0099C707` before the callback
call at `0x0099C714`. This concrete tuple therefore supplies ECX=O,
without a `-0xAC` adjustment. The
[packed property transition](../resource/sqwt-font-edge-property-bridge.md)
describes the generic helper separately.

The callback's switch arm also requires descriptor index 4. The concrete
setter's descriptor `0x0135975C` is outside the PE section's raw-backed
extent, so its live index is not established by retained file bytes.
The receiver trace does not establish that this constructor call reaches
the arm or that intervening virtual calls preserve its interface table.

For the constructed UIElement and Control interface tables, slot2 points
to `0x0091C3E0`, as it does for final CharaElement table `0x00FA7B2C`.
The shared setter target does not join the receivers. CharaElement's
interface has complete-object offset 8 and a different final table and
type. Numerical equality O+`0xB4` = E+8 would require O=E-`0xAC`.
One interface address cannot simultaneously contain these distinct table
pointers while the respective constructed tables remain installed. Thus
the authenticated path supplies a UIElement-family receiver, without
authenticating a positive write on selected Chara E+8. Other receiver
producers, table changes and overlapping or reused allocations remain
outside this construction-path conclusion.

## Enrollment and global selection

The application enrollment helper `0x004D7660` calls base-element slot10
with zero for the previous element and one for the replacement, then calls
`0x0091DFB0` on E+8 at `0x004D76CF`. Final CharaElement base table
`0x00FA7C50` slot10 at `0x00FA7C78` points to `0x00585190`, an exact
`RET 4`. That selected virtual call makes no flag write.

`0x0091DFB0` enrolls a route key/interface pair after `0x0091C370` returns
nonnull. Its retained body contains no store to `0x01357020`. The
[movement command origin](player-movement-command-origin.md#route-entry-construction-and-lifetime)
owns the vector and parent-chain behavior.

Slot14 `0x0091E3E0` calls slot1 at `0x0091E41A` and returns false at
`0x0091E8B5` if its result is zero. It then calls slot23 at `0x0091E42B`
and takes the same false return for zero. For the selected CharaElement
table, the first test reads bit0 and the second returns one.

After these tests, it snapshots `0x01357020` and obtains the parent-chain
route key. If the selected global already equals I, its earlier branch
updates the matching vector entry and returns true without replacing that
global. Otherwise a null route key returns false. The remaining path may
redirect through slot26/slot14 or return early after a routed request.
If execution reaches `0x0091E75C`, it writes I, the preserved ESI, to
`0x01357020`. Subsequent routed notifications are separate operations.

The DesktopWindow constructor itself calls `0x0091E3E0` on W+`0xB4` at
`0x0091B38F`, after its desktop-global store. That call is a concrete
selection attempt on the desktop interface. Its presence and ordering do
not establish its admission result, and do not select the CharaElement.

No live allocation, desktop pointer, interface bit, parent chain, route
vector, selected root, or successful recipient was observed. A later
CharaElement bit0 producer and admitted slot14 dispatch remain the next
concrete selection edges.
