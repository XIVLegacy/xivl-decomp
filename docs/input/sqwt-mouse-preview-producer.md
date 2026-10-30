# Sqwt delivered mouse preview producer

A delivered Sqwt event with raw id `0x54` can enter a DesktopWindow mouse
producer. Results from device virtual calls drive generic preview events. Target
selection precedes routing. A later exact mask test can select the named
left-button previews. These are conditional static paths. OS mouse arrival,
the actual current device receiver and current UI ownership are UNKNOWN.

[Button mouse source](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
owns the concrete Button ingress and Click gates. This page owns the delivered
producer and target selection observations. The
[concrete RaptureMouseDevice provider](sqwt-rapture-mouse-device.md) owns the
selected device construction, raw report creator and stored-state methods.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Method | Ghidra CLI disassembly and xrefs, checked against file-backed PE32 bytes, encoded relative destinations, RTTI and referenced tables and names |

All addresses are VA. STATIC marks instruction or data observations.
INFERRED marks a stated join or interpretation. UNKNOWN marks an unsettled
fact. This is static evidence and does not establish client execution.
The public [RTTI](../../config/ffxivgame.rtti.json) and
[vtable-slot catalog](../../config/ffxivgame.vtable_slots.jsonl) provide type
and slot anchors.

## Delivered input and argument production

Let D be the complete `Sqwt::DesktopWindow` and I=D+`0xB4`. Vtable
`0x01069224` and Complete Object Locator `0x01178D7C` identify
`.?AVDesktopWindow@Sqwt@@` with offset `0xB4`. Interface slot 13 is
`0x0091AC80`. Slot 22, byte offset `+0x58`, is `0x0091AC40`. STATIC.

At `0x0091AC80..0x0091AD9C`, the bridge reads the descriptor pointer32 at
incoming R+`0x04` and its unsigned id byte at descriptor+`0x05`.
The selector at `0x0091ADDC` and jump table at `0x0091ADC0` select
`0x0091AD8C` only for raw id `0x54`. That branch copies dword
R+`0x20` to I+`0x3B8`, passes D=I-`0xB4`, and calls producer
`0x0091A6B0` with R. The scalar R+`0x20` has no established semantic
name. Raw id `0x54` is not identified as an OS message. STATIC.

The producer uses pointer32(R+`0x2C`) as a virtual receiver without a local
null check before its first helper call. Coordinate helper
`0x00919BB0..0x00919BFA` calls receiver virtual `+0x28` and dereferences
its result for two dwords. It stores the pair at receiver+`0x10`/
`0x14` and in the output pair. Starting at D, it subtracts float32
object+`0x24`/`0x28` from the output through the parent pointer32 at
object+`0xA4`. The arithmetic uses x87 float loads and stores. STATIC.
Actual current receiver identity, units, coordinate origin and physical
sampling are UNKNOWN. The concrete-provider page identifies one selected
construction path without identifying every incoming receiver.

At `0x0091A748..0x0091A795`, virtual slots `+0x14`, `+0x18` and
`+0x1C` each receive by-value dword `0xFF`. The `+0x14` result drives
PreviewMouseDown production. The `+0x18` result drives PreviewMouseUp.
The producer initializes dword masks `1,2,4,8,16` at
`0x01356EB0..0x01356EC0` under flag mask `0x01` at `0x01356EC4`
(`0x0091A7B6..0x0091A7EE`). Each down/up loop tests the full result
dword against each mask. STATIC.

For each matching mask, constructor `0x00919C70..0x00919CFA` receives the
device pointer, dword R+`0x20`, mask dword and a pressed byte. Down passes
`1` and up passes `0` (`0x0091AA30..0x0091AA4D`,
`0x0091AB74..0x0091AB9C`). Let A be the constructed argument object.

| Storage | Width | Constructor write |
|---|---|---|
| A+`0x3C` | byte | Pressed argument |
| A+`0x40` | dword | Mask argument |
| A+`0x44` | dword | Device virtual `+0x10(mask)` result |
| A+`0x48` | byte | Nonzero test of device virtual `+0x1C(mask)` result |

The constructor installs vtable `0x010691D0`, identified by RTTI as
`Sqwt::Input::MouseButtonEventArgs`. It first calls base constructor
`0x00545B50`. The concrete-provider page describes that base construction
and one selected provider's stored-state return contracts. Other incoming
provider implementations and actual current receiver/state remain UNKNOWN.
The listed writes do not establish total initialization or a physical-button
enumeration. STATIC for the writes and RTTI.

## Preview production and specialization

The initial calls to `0x0091E980` pass I, A, the point pointer and a null
explicit target.

| Path | Descriptor | Id | Name | Call site |
|---|---|---|---|---|
| Down | `0x013596EC` | `0x07` | PreviewMouseDown | `0x0091AA79` |
| Up | `0x013595F0` | `0x17` | PreviewMouseUp | `0x0091ABCE` |

Initializers `0x00F22BE0`/`0x00F22C10` forward those names, ids and
routing argument byte `2`. Their shared constructor body is uninspected.
A nonzero first result causes a second call using the same A storage, with
that result as explicit target and descriptor `0x0135942C` for down or
`0x01359608` for up. A zero result skips the paired call
(`0x0091AA7E..0x0091AA97`, `0x0091ABD3..0x0091ABF2`). STATIC.
Handlers can mutate A. The second call can also select a changed preferred
target before consulting its explicit target.

Generic dispatcher `0x0091D320` uses selector `0x0091D818` and jump
table `0x0091D760`. Its generic down arm `0x0091D436` first calls
receiver virtual `+0xA4`. The up arm `0x0091D47A` first calls
`+0xA8`. Only after normal return does each read dword A+`0x40`.
Exact value `1` writes the named left-preview descriptor to A+`0x04`
and invokes receiver virtual `+0x34`. Exact value `4` selects another
descriptor and also calls `+0x34`. STATIC.

| Arm | Exact-value-1 descriptor | Named preview | Id | Initializer |
|---|---|---|---|---|
| Down | `0x01359674` | PreviewMouseLeftButtonDown | `0x0D` | `0x00F22CA0` |
| Up | `0x013593CC` | PreviewMouseLeftButtonUp | `0x0F` | `0x00F22CD0` |

Those initializers forward routing argument byte `2`. On the exact-value-1
or exact-value-4 paths, normal return from virtual `+0x34` is followed by
restoring the generic descriptor at `0x0091D46E`/`0x0091D4B2`.
Every other post-handler value jumps to `0x0091D759` and bypasses both
specialization and that restoration write. No local handled-byte test occurs
between the generic virtual call and mask selection. STATIC.
An earlier handler can change both the mask and descriptor.

For an argument produced with mask `1` from the delivered raw-`0x54`
branch, entry to the generic preview arm and dword A+`0x40` still exactly
`1` after virtual `+0xA4`/`+0xA8` returns normally establish the
named descriptor write and virtual `+0x34` call. The named dispatcher arms
`0x0091D4BE`/`0x0091D4D0` select slots 43/44, offsets
`+0xAC`/`+0xB0`. Joining this path to the
[Button ingress](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
is INFERRED and additionally requires the concrete dispatch path, recipient
type and route state. Provider mask `1` alone does not establish that join,
a physical OS button mapping, a Click or an installed Click EventTrigger.

## Target selection and route result

At `0x0091E9AF..0x0091EA45`, `0x0091E980` selects these in order:

1. Nonnull pointer32 at global `0x01357024`.
2. Nonnull explicit target argument.
3. Receiver virtual `+0x58`, passed the point pointer, a byte-output
   address, zero, zero and scratch `PointHitTestResult` storage.

The scratch vtable `0x010691C8` has RTTI identity
`Sqwt::Media::PointHitTestResult`. The query's byte-output local is not
initialized before the virtual call at this site. A zero query result returns
null through `0x0091EA18`. A selected nonzero T is used directly as an
interface pointer, without adding `0xB4`, and is stored at
`0x01357028` before route work. STATIC. These local operations do not
validate lifetime, ownership, visibility, enabled state or geometry.

The DesktopWindow query thunk `0x0091AC40..0x0091AC7C` subtracts
`0xB4` and forwards five arguments to `0x0091A4E0`. It retains the
result across a call to `0x0092ABC0(0)` when dword `+0x08` of the
fifth argument object is zero. That helper's behavior is UNKNOWN.

At `0x0091A4E0..0x0091A5BB`, the query iterates the container at
D+`0x490`/`0x494` through iterator `0x004E4020`. Each visited node
supplies pointer32(node+`0x10`) as an interface receiver for virtual
`+0x58`. A local child byte output starts at zero. After the call, exact
byte output `1` returns null before testing the returned pointer.
Otherwise the first nonzero pointer wins. Zero continues iteration.
Only exhaustion with nonnull D falls back to D+`0xB4`. STATIC.
Iterator order, child validity, complete geometric/property predicates and
assertion helper `0x009D22B4` behavior are UNKNOWN. The scratch RTTI name
does not establish a complete hit-test policy or general fallback contract.

For selected T, target virtual `+0x0C` returning AL exactly `1` makes
bytes A+`0x19`, A+`0x14` and A+`0x15` zero. Any other AL makes
them one. The predicate has no established semantic name here.
The helper clears source dwords A+`0x08` and A+`0x0C`, then calls
`0x0091E1C0(A,T,1,1,1)` (`0x0091EBBB..0x0091EBF2`). STATIC.

After that call, nonzero byte A+`0x17` returns null. This stop/result byte
is distinct from the handled byte behind pointer A+`0x10` at Button
ingress. Otherwise normal completion returns T at `0x0091ED5B`, after
conditional ancestor transition work. Helper `0x0091C5C0..0x0091C5EA`
searches parent+`0x60` chains for a common pointer or null. STATIC.
The full route/transition helper contracts and exception outcomes are not
established by these observations. A null result can follow prior routing and
side effects. It does not mean no route occurred.

## Preferred pointer and unresolved boundary

Setter `0x0091C640` stores ECX as dword global `0x01357024` and
returns AL=`1` without a local eligibility check or OS API call.
Helper `0x0091C650` unconditionally clears that dword.
`0x0091C4E0` tests receiver equality with the preferred pointer.
`0x0091C4F0` tests against it, or against `0x01357028` when the
preferred pointer is zero. STATIC.

The branch at `0x0091D9D0` requires nonnull child C and ECX equal to
pointer32(C+`0x60`). At `0x0091DAC2..0x0091DADD`, it follows
parent+`0x60` from the preferred pointer and clears the slot if C occurs
in that ancestry. STATIC for this local branch. Other caller preconditions,
parent overrides and complete cleanup remain UNKNOWN.

### Selected Button arming and preference

For this path, let E be the complete Button, B=E+`0xB4` its input interface,
and A the delivered argument. The
[Button mouse source](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
owns the concrete type, table construction and later pressed/Click gates.
Button table `0x01073B84` and ButtonBase table `0x0107603C` supply these
selected input slots. STATIC.

| Slot | Byte offset | Target |
|---|---|---|
| 15 | `+0x3C` | Setter `0x0091C640` |
| 16 | `+0x40` | Clearer `0x0091C650` |
| 32 | `+0x80` | Control method `0x0093A020` |

Let G(E) be AL from `0x0093F060`. It calls byte reader `0x00974240`.
A nonzero byte global `0x0135C2B0` yields AL zero. Otherwise G(E) is
exactly whether dword E+`0xA0` is nonzero
(`0x0093F063..0x0093F079`). STATIC. Enabled/disabled, ownership and device
semantics for this raw predicate remain UNKNOWN.

Button down `0x0097EFF0` calls `0x0092EB90` at `0x0097EFFE`, which
first calls Control down base `0x009398D0`. That base first calls
`0x0091C840`. After normal return, arming requires the handled byte behind
A+`0x10` zero, G(E)==1, and nonzero `0x0095F020` results for both
descriptors below (`0x0093991A..0x00939962`). Values are read through
`0x0095EE10` and converted through `0x00988A30`. STATIC.

| Name | Descriptor | Initializer | Literal |
|---|---|---|---|
| DragDrop.Id | `0x013603B8` | `0x00F27BD0` | `0x01078514` |
| DragDrop.SourceCategory | `0x01360400` | `0x00F27C30` | `0x01078520` |

Only a nonzero first converted result admits the block at
`0x009399DE..0x00939A27`. After `0x0067AB40(A,E)` returns, it ORs
mask `0x10` into byte B+`0xDC` (E+`0x190`) at `0x00939A08`.
This is an arming write, distinct from preferred-pointer publication. STATIC.
Provider admission, other state writers, actual registration and default
DragDrop values remain UNKNOWN.

PreviewMouseMove initializer `0x00F22E20` supplies descriptor `0x013595FC`,
id `0x11`, routing byte `2` and literal `0x0106C0AC`. Dispatcher
`0x0091D320` subtracts one from the id before indexing `0x0091D818`.
This entry selects `0x0091D394` through `0x0091D760` and calls virtual
`+0x80` at `0x0091D39F`. The selected Button table joins it to
`0x0093A020`. That method first calls `0x0091C6C0` at `0x0093A065`,
then tests B+`0xDC` mask `0x10` and G(E)==1 before calling B virtual
`+0x3C` at `0x0093A093`. Under the selected table, the reached setter
publishes B. STATIC for dispatch and calls, INFERRED for the selected-table
join.

Button up `0x0097F080` first calls Control up base `0x00939E90` at
`0x0097F08C`. The base first calls `0x0091C870` at `0x00939ED5`,
then tests B+`0xDC` mask `0x10` and G(E)==1 before calling B virtual
`+0x40` at `0x00939F03`. The selected table reaches the clearer. No
local handled-byte or preferred-owner comparison intervenes between that
forwarding-helper return and this call. The clearer does not compare its
receiver with the preferred slot. Only a reached clear is unconditional.
STATIC.

Each initial forwarding helper `0x0091C840`, `0x0091C6C0` and
`0x0091C870` requires B+`0x64` nonzero, A+`0x17` zero and B+`0x4C`
nonzero before invoking its stored handler. Those gates govern forwarding,
not the later B+`0xDC`/G(E) tests. STATIC. Callbacks can change state,
arguments, tables, preference or lifetime.

If the selected up base reaches its clear and returns normally, that clear
precedes Button's later handled/IsHitTestVisible checks, old pressed-bit
snapshot and Click gates through `0x0097F11E`. INFERRED from call order.
The linked Button study owns those gates and the separate
[Click destruction notification](../resource/sqwt-command-parameter-consumer.md#click-destruction-notification).
An ordinary Button down is not thereby an unconditional preference acquisition,
and a clear does not verify that B still owns the slot.

Joining separate down, move and up deliveries requires the intended same live
E and B, selected current tables, readable event/state objects, ordinary
callee-saved returns, the intended same delivered device and owner, and no
intervening lifetime break. These are conditions, not actual state or device
enforcement by the scalar slot. Actual visitation and final state remain
UNKNOWN. Reaching a Click helper does not prove attachment, command success,
completed Click or object survival.

### Selected retirement and subtree cleanup

The linked Button destruction path reaches body `0x0097EC70`. Its selected
continuation has these transfers. Complete E is retained under ordinary
callee-saved returns until the final explicit interface adjustment. STATIC.

| Caller body | Transfer site | Destination and receiver |
|---|---|---|
| `0x0097EC70` | `0x0097EC97`, tail JMP | `0x00954660`, E |
| `0x00954660` | `0x0095478A`, CALL | `0x00935660`, E |
| `0x00935660` | `0x009357EF`, CALL | `0x00936C70`, E |
| `0x00936C70` | `0x00936D8D`, CALL | `0x009314E0`, E |
| `0x009314E0` | `0x00931707`, CALL | `0x0093AC10`, E |
| `0x0093AC10` | `0x0093ACB3`, CALL | `0x0091DBD0`, B=E+`0xB4` |

The final adjustment is explicit at `0x0093AC3B` and `0x0093ACAC`.
InputElement retirement `0x0091DBD0` installs table `0x01069A6C`.
COL `0x01178F70` and type descriptor `0x01269D3C` identify
`.?AVInputElement@Sqwt@@`. It reads B+`0x60` at `0x0091DC32`.
A nonnull parent P receives virtual `+0x54` with child B at `0x0091DC3F`.
The null-parent branch instead calls `0x0091D930` at `0x0091DC45`.
It also directly calls `0x0091D9D0` on selected contained children at
`0x0091DC7C`. STATIC. Earlier resource-release and indirect calls are not
assumed harmless or exception-free.

If the parent's selected slot reaches the cleanup above, its child/parent
preconditions hold and the ancestry remains live and readable, the preferred
pointer can be cleared when it is B or lies in B's removed subtree. INFERRED.
That clear at `0x0091DADD` precedes the later C+`0x60` clear at
`0x0091DB96`. Parent-null effects, other parent overrides, destruction
callers and complete retirement coverage remain UNKNOWN. Opaque callees and
other cleanup are not closed. No universal stale-pointer prevention or actual
failure result follows.

### Preferred-pointer reproduction

The lifecycle observations use `ghidra-cli 0.2.2` against the identified
executable. Subtract image base `0x00400000` from a VA for RVA. Focused
recipes include:

```sh
ghidra disasm 0x0091C4E0 -n 120 --json
ghidra disasm 0x0093F060 -n 15 --json
ghidra disasm 0x00974240 -n 3 --json
ghidra disasm 0x009398D0 -n 135 --json
ghidra disasm 0x0093A020 -n 140 --json
ghidra disasm 0x00939E90 -n 140 --json
ghidra disasm 0x0097EFF0 -n 105 --json
ghidra disasm 0x0097F109 -n 16 --json
ghidra disasm 0x0091D9D0 -n 160 --json
ghidra disasm 0x0091DBD0 -n 170 --json
ghidra disasm 0x0093AC10 -n 65 --json
```

Read the selected slot dwords, descriptor initializer operands and literals,
and the cited transfer sites with their receiver setup. For RTTI, follow
table[-1] to COL, COL+`0x0C` to TypeDescriptor and TypeDescriptor+8 to the
NUL-terminated name. Compare instructions and referenced data with PE section
raw-file mapping. Virtual extent beyond a section's raw size does not supply
file bytes or current global values. Keep executable bytes, bodies, listings
and Ghidra state outside the tracked tree. Other indirect users and memory
writers are not exhausted by these selected paths.

A capture-like interpretation of the preferred slot is INFERRED from its
precedence, setter and ancestry cleanup. OS SetCapture/ReleaseCapture,
physical capture, focus policy and complete lifecycle are UNKNOWN.
Also UNKNOWN are physical OS arrival and complete delivery to raw id `0x54`,
the actual current device receiver and state, current DesktopWindow or
application owner, tree contents, handler attachment and widget lifetime.
These facts do not establish any particular Button, installed Click trigger,
safe malformed-input admission, universal input profile, default state,
rendering effect or live result.
