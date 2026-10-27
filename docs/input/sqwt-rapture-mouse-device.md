# Sqwt RaptureMouseDevice report provider

`Application::Main::MainModule` embeds a concrete
`Application::Main::SqwtInterface::RaptureMouseDevice`. Its report creator
constructs `Sqwt::Input::MouseEventArgs`, assigns the device directly to
R+`0x2C`, and can deliver raw descriptor id `0x54` through the embedded
`RaptureElementContainer` interface to the DesktopWindow bridge. These are
selected static construction and call paths. Actual current receivers,
actual physical button-state creation and complete OS input delivery are UNKNOWN.

This page owns the concrete provider and report construction facts.
The [delivered mouse preview producer](sqwt-mouse-preview-producer.md) owns
generic preview production, target selection and routing. The
[Button mouse source](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
owns Button ingress and Click gates. The
[numeric callback and repeat state](sqex-mouse-numeric-callback.md) page owns
the selected Sqex writer and callback submission path.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Format and image base | PE32, VA `0x00400000` |
| Method | Ghidra CLI disassembly and xrefs, compared with file-backed PE bytes and encoded relative destinations, plus bounded manual descriptor-header, RTTI, vtable and PE import interpretation |

All addresses are VA. STATIC marks instruction or data observations.
INFERRED marks an explicit conditional join. UNKNOWN marks an unsettled fact.
The [RTTI catalog](../../config/ffxivgame.rtti.json) and
[vtable-slot catalog](../../config/ffxivgame.vtable_slots.jsonl) provide
type and slot anchors. Static construction writes do not establish current
contents, universal defaults, whole-object initialization or execution.

## Object notation and construction

| Symbol | Meaning on the selected path |
|---|---|
| O | Complete `Application::Main::MainModule` |
| D | `RaptureElementContainer` at O+`0x10` |
| P | `RaptureMouseDevice` at O+`0x17944` |
| I | Stored receiver interface, D+`0xB4` on the nonzero construction branch |
| R | Raw `MouseEventArgs` constructed in local storage by `0x00547E10` |
| U | Input-state pointer supplied to `0x00547ED0`, with a 32-byte copied range |
| F | Scalar argument copied to R+`0x20` |
| A | Distinct downstream `MouseButtonEventArgs` in the generic producer |

Constructor `0x004DC3A0..0x004DC4A0` installs MainModule table
`0x00F9142C`. Its RTTI names `.?AVMainModule@Main@Application@@`
at complete-object offset zero. It constructs D through `0x004DBF40`,
then constructs P through `0x00547D20` at `0x004DC4A0`. The provider
argument is O+`0xC4` when the computed D pointer is nonzero, otherwise
null. Thus the nonzero argument is I=D+`0xB4`. STATIC.

Receiver constructor `0x004DBF40` calls uninspected base `0x0091B020`.
It installs primary table `0x00F912E4` at D and interface table
`0x00F911BC` at D+`0xB4` (`0x004DBFAB..0x004DBFB1`). Both
RTTI chains name `.?AVRaptureElementContainer@Main@Application@@`.
The interface Complete Object Locator has offset `0xB4`. Its hierarchy
places `Sqwt::DesktopWindow` at nonvirtual displacement zero and
`Sqwt::InputElement`/`Sqwt::IInputElement` at nonvirtual displacement
`0xB4`. Interface slot 13 is `0x004D71C0`, a direct jump to
`0x0091AC80` without another receiver adjustment. STATIC.

Provider constructor `0x00547D20..0x00547DB3` calls uninspected base
`0x00919B50`, installs primary table `0x00FA2AF4`, and stores its
pointer argument at P+`0x04`. Complete Object Locator `0x01145F4C`
has offset zero and type
`.?AVRaptureMouseDevice@SqwtInterface@Main@Application@@`. Its RTTI
base chain places `Sqwt::Input::MouseDevice` and
`Sqwt::Input::InputDevice` at nonvirtual displacement zero. STATIC.

| P storage | Width | Explicit provider-constructor operation |
|---|---|---|
| `+0x00` | pointer32 | Install table `0x00FA2AF4` |
| `+0x04` | pointer32 | Store receiver argument |
| `+0x18`, `+0x1C`, `+0x20`, `+0x24` | float32 each | Store zero |
| `+0x28` | helper receiver | Call uninspected `0x00D35280` |
| `+0x48`, `+0x4C`, `+0x50`, `+0x54`, `+0x58`, `+0x5C` | dword each | Store zero |
| `+0x60`, `+0x61` | byte each | Store zero |

Other base/helper writes, total object size, allocation success and lifetime
are UNKNOWN. The explicit null argument branch is not a universal nonnull
receiver guarantee. Static embedded construction does not identify a running
instance or prove persistence of its stored fields.

## Raw descriptor and report gates

Initializer `0x00F20780..0x00F207A3` calls descriptor constructor
`0x0097F6C0` with ECX=`0x01356E1C`, routing byte `1`, id byte
`0x54` and a null third pointer. The bounded manually decoded header at
`0x0097F6ED..0x0097F712` stores the first argument byte at
descriptor+`0x04`, the second argument byte at descriptor+`0x05`, and
the third pointer32 at descriptor+`0x08`. The prologue stack delta is
`-0xD0`, placing those arguments at ESP+`0xD4`/`0xD8`/`0xDC`
at the loads. STATIC. Full constructor registration effects, descriptor
current contents and any OS message identity for raw id `0x54` are UNKNOWN.

P table slot 1, byte offset `+0x04`, is `0x00547E10`. Its local
branches at `0x00547E39..0x00547EB3` apply these gates. STATIC.

| Condition or site | Local effect |
|---|---|
| Byte P+`0x61` exactly `1` | Call opaque predicate `0x0091B7B0` |
| That predicate returns AL zero | Store byte P+`0x60`=`1` |
| Byte P+`0x60` exactly `1` | Enter report branch |
| Stored pointer32(P+`0x04`) nonnull within that branch | Construct local R through `0x00545B50(P,F,byte(P+0x61))` |
| `0x00547E75` | Assign descriptor pointer `0x01356E1C` to R+`0x04` |
| Creator second argument low byte exactly `1` | Store byte `1` to R+`0x14`, R+`0x15` and R+`0x19` |
| `0x00547E9C` | Call stored receiver pointer virtual slot 13, offset `+0x34`, with R |
| Normal return from that call | Invoke destructor `0x00545C20` on local R |
| Entered report branch reaches `0x00547EAF` | Clear byte P+`0x60`, including the null stored-target branch |
| Ordinary return through `0x00547EB3` | Clear byte P+`0x61` |

The target is the pointer stored at P+`0x04`, not the address P+`0x04`.
This call adds no `0xB4` adjustment. Other creator second-argument low-byte
values do not take those flag writes. Predicate semantics, exceptions,
callback retention/escape and the complete event destruction contract are
UNKNOWN. Local stack storage does not establish a safe-lifetime policy.

## MouseEventArgs construction fields

Constructor `0x00545B50..0x00545C10` receives P as its first pointer
argument, F as its second dword and a third byte argument. It calls
`0x005457D0(P,F)`, then installs table `0x00FA2640`, whose RTTI
names `.?AVMouseEventArgs@Input@Sqwt@@`. The base installs
InputEventArgs table `0x00FA25F4`, with RTTI
`.?AUInputEventArgs@Input@Sqwt@@`. STATIC.

| R storage | Width | Selected constructor or creator write |
|---|---|---|
| `+0x04` | pointer32 | Base clears, creator assigns `0x01356E1C` |
| `+0x08`, `+0x0C` | dword each | Base clears source fields |
| `+0x10` | pointer32 | Base assigns byte address R+`0x19` |
| `+0x14`, `+0x15` | byte each | Base clears, creator conditionally writes `1` |
| `+0x16`, `+0x17` | byte each | Base clears |
| `+0x18` | byte | Base writes `1` |
| `+0x19` | byte | Base clears, creator conditionally writes `1` |
| `+0x1C` | pointer32 | Base stores P |
| `+0x20` | dword | Base stores F |
| `+0x24` through `+0x28` | five bytes | Copy bytes P+`0x08` through P+`0x0C` |
| `+0x2C` | pointer32 | Store P directly at `0x00545BBC` |
| `+0x30` | dword | Store device virtual `+0x0C(0xFF)` result |
| `+0x34` | dword | Store device virtual `+0x1C(0xFF)` result |
| `+0x38` | byte | Store third byte argument |

Base writes are at `0x00545802..0x00545835`. Mouse field writes are at
`0x00545B96..0x00545BF2`. R+`0x2C` receives the same P pointer
with no object or interface displacement. R+`0x10` initially points to
R+`0x19`. The separate stop byte is R+`0x17`. These are entry
construction facts, not a stable snapshot after callbacks. Downstream A is a
different argument object even though its base constructor is also
`0x00545B50`.

## Concrete stored-state virtual methods

These methods belong to constructed P table `0x00FA2AF4`. A mask
parameter is a by-value dword. STATIC.

| Table byte offset | Method VA | Operation |
|---|---|---|
| `+0x08` | `0x00548060` | Return dword P+`0x4C` |
| `+0x0C` | `0x00547E00` | Return dword P+`0x48` AND mask |
| `+0x10` | `0x00548070` | Return dword `1` if dword P+`0x50` AND mask is zero, otherwise `2` |
| `+0x14` | `0x00548090` | Return dword P+`0x58` AND mask |
| `+0x18` | `0x005480A0` | Return dword P+`0x5C` AND mask |
| `+0x1C` | `0x005480B0` | Return dword P+`0x54` AND mask |
| `+0x20` | `0x005480C0` | Store byte P+`0x61`=`1` |
| `+0x28` | `0x00547DE0` | Return pointer P+`0x18` |
| `+0x2C` | `0x00547DF0` | Return pointer P+`0x20` |

The generic DesktopWindow producer calls `+0x14`/`+0x18`/`+0x1C`
with `0xFF` and calls `+0x28` for coordinates. These method bodies
settle one concrete implementation of those calls. They do not assign
physical-button names, semantic enum names for `1`/`2`, current returns or
universal interface contracts.

## State feed and mutation order

Function `0x00547ED0..0x00548053` takes U as its first pointer
argument. Before its first predicate call it clears byte P+`0x60`.
It calls `0x00D352A0` with receiver U and argument P+`0x28`.
If AL is zero it calls `0x00D352D0` with the same pointer roles.
Either the first nonzero AL or a nonzero second AL admits the update body.
Two zero results branch to `0x00548035` and skip its stores
(`0x00547F10..0x00547F29`). The helper meanings are UNKNOWN. STATIC
for these calls and branches.

On the update body, these stores occur. STATIC.

| Source or condition | P storage | Width and operation |
|---|---|---|
| U+`0x00`, U+`0x04` | `+0x18`, `+0x1C` | Convert signed int32 with CVTSI2SS and store float32 |
| Sampled U+`0x08` differs from P+`0x4C` | `+0x4C` | Store dword sample, including zero |
| U+`0x0C` | `+0x48` | Store dword AND `7` |
| U+`0x1C` | `+0x54` | Store dword AND `7` |
| U+`0x10` | `+0x58` | Store dword AND `7` |
| U+`0x18` | `+0x5C` | Store dword AND `7` |
| U+`0x14` | `+0x50` | Store dword AND `7` |
| Bits 0/1/2 of P+`0x48` | `+0x08`, `+0x09`, `+0x0A` | Store individual bytes |
| Update body | `+0x0B`, `+0x0C` | Store zero bytes |
| U+`0x00` through U+`0x1F` | `+0x28` through `+0x47` | Four eight-byte copies |
| After those copies | `+0x60` | Store byte `1` |

The coordinate conversions precede the U+`0x08` comparison. Equality skips
its store. Inequality stores the sampled dword at `0x00547F4C`, even if
zero. Only a changed nonzero sample takes an additional event path through
the [MouseWheelEventArgs constructor](sqex-mouse-numeric-callback.md#signed-quotient-and-wheel-scalar-interpretation)
at `0x00546040` and route helper `0x0091E980`, using
descriptors `0x01359798` and `0x013593C0`. Those route calls use stored
P+`0x04` without an intervening local null check. A changed zero sample
stores zero and skips the extra event path. STATIC.

The additional event calls occur before the later mask reads and copies
(`0x00547F42..0x00548031`). They may mutate U or device state.
The predicates are also uninspected. The fields and copied record are not
proved to be an atomic snapshot. Numeric units, valid source domains, SSE
rounding environment, extra-event semantics and other writers are UNKNOWN.
The AND `7` operations do not establish a physical-button domain or a safe
null/fallback contract.

## Selected MainModule service and scalar sample

At `0x004D6570..0x004D65D6`, MainModule calls state feed
`0x00547ED0` on P=O+`0x17944` with the second input pointer U and
a sampled dword O+`0x08`. It later calls `0x00552DF0` with arguments
(third input, P, O+`0x17C80`, D=O+`0x10`, F). F is specifically
the dword O+`0x08` sampled at `0x004D65B8`, passed as the fifth
argument. Separate O+`0x08` loads occur at `0x004D658C` and
`0x004D65A2`. STATIC.

In `0x00552DF0`, P is saved and restored across other work
(`0x00552E32..0x00552E5F`, `0x005530D6`). After opaque helper
`0x0091A1C0`, the tail `0x005530F9..0x00553103` calls P slot 1
with the saved fifth scalar argument and a second argument whose low byte is
that helper AL result. No meaning is assigned to its upper bits or the
helper result. STATIC.

After `0x00552DF0` returns normally, `0x004D65CE` adds `1` to
dword O+`0x08`. MainModule constructor `0x004DC40D` explicitly
initializes that dword to zero. These are 32-bit stores and arithmetic.
When the report gates admit construction, F is copied to R+`0x20` before
that later increment on this selected path.
Intervening helpers and callbacks may mutate state, so the separate samples
and the value immediately before ADD need not be identical. No time unit,
cadence, OS scheduling or live input-frame guarantee is established.

## Conditional upstream copied state

Let C be complete `Application::Rapture`. Constructor-like routine
`0x004B3B50` installs primary table `0x00F8CC30` at C and interface
table `0x00F8CC1C` at C+`0x04`
(`0x004B3B96..0x004B3B9C`). Both RTTI chains name
`.?AVRapture@Application@@`. Complete Object Locators `0x01140B1C`
and `0x01140BA4` give offsets zero and `0x04`. Interface table slots
0/1/2 select initialization `0x004B2DF0`, update `0x004B3C50` and
cleanup `0x004B3830`. STATIC.

Let B be EDI in initialization and S the interface receiver of update.
These methods receive the interface C+`0x04`. The initialization body
`0x004B2DF0..0x004B37DE` sets EDI=entry ECX at `0x004B2E39`.
It has no subsequent caller instruction writing EDI until epilogue POP EDI
at `0x004B37C6`, and no implicit string-register write to EDI.
B therefore remains that interface base under ordinary x86 callee-saved
call/return behavior. STATIC for the instructions, INFERRED for continuity
across callees under that convention. Initialization is separate from
complete-object construction.

The selected initialization window `0x004B35F4..0x004B36AD`
requests `0x18` from opaque helper
`0x009D1B35`, calls `0x00D31F10` only on a nonzero result, and
stores that result or zero at B+`0x44`. It also requests `0x17D58`,
conditionally calls MainModule constructor `0x004DC3A0`, and stores the
result or zero at B+`0x64`. Thus B+`0x44` is C+`0x48` and
B+`0x64` is C+`0x68`. The update destination S+`0xC4` is C+`0xC8`
when S is that same interface. STATIC for the offsets.

Let M be the constructed `Sqex::Input::MouseDevice`.
Constructor `0x00D31F10..0x00D31F7B` installs table `0x01110504`
at M and `0x011104FC` at M+`0x04`, stores its pointer argument at
M+`0x14`, and calls the
[RepeatCounter initialization helper](sqex-mouse-numeric-callback.md#repeatcounter-output)
`0x00D428D0` on M+`0x04`.
Table `0x01110504` has RTTI name `.?AVMouseDevice@Input@Sqex@@`
and complete-object offset zero. STATIC.

Caller `0x004B3D34..0x004B3DCC` tests stored S+`0x44`. On the
nonnull path it calls virtual `+0x08`, reloads the stored provider and calls
virtual `+0x10`, copies 32 bytes from the returned pointer to S+`0xC4`,
and calls virtual `+0x14`. It passes S+`0xC4` as U to MainModule
pointer32(S+`0x64`) through `0x004D6570`. A null S+`0x44` skips
copy/clear but still forwards existing S+`0xC4`. There is no local null
check of S+`0x64` at its call site. STATIC.

If S=B and the constructed provider fields remain intact, table
`0x01110504` selects `+0x08`=`0x00D31DE0`,
`+0x10`=`0x00D32080` and `+0x14`=`0x00D31E50`.
The getter returns global address `0x013783A0`. The clear method writes
zero dwords at `0x013783B0`, `0x013783B8`, `0x013783B4` and
`0x013783A8` (`0x00D31E50..0x00D31E66`). INFERRED for the
conditional construction-to-caller join, STATIC for the method bodies.

Actual same-instance identity S=B, field persistence, allocation
semantics/success, initialization completion, current provider and lifetime
remain UNKNOWN. The local interface layout and register continuity do not
prove those properties across callbacks, cleanup or arbitrary later calls.
The supplied M+`0x14` pointer type is UNKNOWN.
The skipped-copy path does not establish fresh input or safe fallback.

## Conditional USER32 coordinate calls

For table `0x01110504`, update method `0x00D31DE0..0x00D31E47`
enters its coordinate path only when byte global `0x0137839C` equals
`1` and the GetActiveWindow result equals dword
[pointer32(M+`0x14`)+`0x04`]. The enabled path dereferences M+`0x14`
without an intervening local null check. STATIC.

| PE import identity | IAT VA | Call site |
|---|---|---|
| `USER32.dll!GetActiveWindow` | `0x00F3E458` | `0x00D31DEF` |
| `USER32.dll!GetCursorPos` | `0x00F3E3F8` | `0x00D31E02` |
| `USER32.dll!ScreenToClient` | `0x00F3E3FC` | `0x00D31E14` |

It passes a local two-dword buffer to GetCursorPos, then the same buffer
and stored handle dword to ScreenToClient (`0x00D31DFD..0x00D31E14`).
It reads the buffer dwords into globals `0x013783A0`/`0x013783A4`.
No API return-value check occurs between those coordinate calls and buffer
reads. It calls the
[RepeatCounter helper](sqex-mouse-numeric-callback.md#repeatcounter-output)
`0x00D42940` on M+`0x04` with dword `0x013783AC` and zero,
storing its result to `0x013783BC`. STATIC.
API success, current active window, coordinate units and failure policy are
UNKNOWN.

The [callback submission and wrapper return](sqex-mouse-numeric-callback.md#callback-address-submission-and-wrapper-return)
contract distinguishes inner `0x00D31C30` returning zero from wrapper
`0x00D31F80` returning its ordinal-`413` delegate result. Other selected
methods pass callback address
`0x00D31F80` through `0x00DCD3CE`/`0x00DCD3D4` under their local
[pointer32(M+`0x14`)+`0x0C`] test
(`0x00D31FC0..0x00D31FD9`, `0x00D32060..0x00D3207D`).
The indirect jump thunks and PE import lookup records establish only these
identities. STATIC.

| Thunk VA | IAT VA | PE import identity |
|---|---|---|
| `0x00DCD3C8` | `0x00F3E024` | `COMCTL32.dll` ordinal `413` |
| `0x00DCD3CE` | `0x00F3E028` | `COMCTL32.dll` ordinal `412` |
| `0x00DCD3D4` | `0x00F3E02C` | `COMCTL32.dll` ordinal `410` |

No semantic API names are assigned to those ordinal imports. The
[numeric callback](sqex-mouse-numeric-callback.md#numeric-case-mapping)
establishes selected message cases and current/transition writers.
It writes byte `0x0137839C` at entry, before any window/message test
(`0x00D31C38`). This does not exhaust writers or establish its initial
value. The numeric page separates its conditional platform-constant
correspondence from binary observations. Actual physical delivery, successful
callback installation and complete registration/lifecycle remain UNKNOWN.

## Conditional join and unresolved boundary

On the constructed MainModule/RaptureElementContainer/RaptureMouseDevice
path, ordinary delivery through stored P+`0x04` slot 13 reaches
`0x004D71C0` then `0x0091AC80`. Raw descriptor id `0x54` and
R+`0x2C`=P hold before intervening recipient mutation. INFERRED for the
join to the [generic producer](sqwt-mouse-preview-producer.md). This does
not identify every incoming raw-`0x54` record or current device receiver.

R and downstream A are distinct. Reaching a named left preview still requires
entry to the generic arm and exact dword A+`0x40`=`1` after generic
virtual `+0xA4`/`+0xA8` returns normally. Reaching Button additionally
requires concrete dispatch, recipient type and route state. Descriptor
restoration follows normal virtual `+0x34` return only for exact
post-handler values `1`/`4`. Other values bypass specialization and
restoration. Preferred targets can be reselected between paired calls.
The stop byte at `+0x17` remains distinct from the handled byte behind
pointer `+0x10`.

Uninspected helpers include `0x00919B50`, `0x0091B020`,
`0x00D35280`/`0x00D352A0`/`0x00D352D0`, `0x0091B7B0`,
`0x0091A1C0`, and RepeatCounter storage helpers `0x009D04AC`/
`0x009D2110`. Their meanings must
not be assigned from call shape alone. Other writers and complete exception,
null, allocation and lifecycle contracts are UNKNOWN.

These facts do not establish physical OS input reachability, a currently
installed provider or UI owner, successful API output, current global state,
focus/capture policy, a particular widget, installed Click trigger, Click
execution, complete hit testing, rendering, safe malformed-input admission
or a live result.
