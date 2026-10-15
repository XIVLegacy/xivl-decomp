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
