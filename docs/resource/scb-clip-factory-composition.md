# SCB resource parsing and clip construction

The resource parser calls the clip builder synchronously on the nonzero
actor/track parser-result path. The selected call path is
`0x00A26380 -> 0x00A1C7B0 -> 0x00A28510`. Actor and track parsing prepare
collections before that invocation. They are not themselves the clip builder.
The local builder selects a clip factory by its class-name key. The selected
Limsa `vtp5` wrapper supplies `LayRaptureCallSchedulerClip`, and a registration
path supplies factory `0x00837220` for that exact key. The scheduler's
constructor derives its registry object from the complete manager's retained
registry pointer. Factory selection still requires the same retained object
value, the parsed key mapping and matching node state. Complete wrapper
initialization and owning UnitTree context at clip creation remain separate
conditions.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, image base `0x00400000`,
size 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra listings were checked against the original PE with pefile
`2024.8.26` and Capstone `5.0.7`, x86-32 mode. Listing addresses are RVAs.
The checks covered every listed instruction, the enclosing address spans,
and the operands and calls below. The exporter
`tools/ghidra_scripts/DumpFunctions.java` computes `# size` by summing
function-body address-range lengths. The builder's listed instructions total
`0x26C` bytes within an enclosing `0x26F`-byte span. The separate three-byte
alignment gap at `0x00A2854D` was independently decoded as `LEA ECX,[ECX]`.
The final listed jump at `0x00A2877A` was checked through its complete end
at `0x00A2877F`.

The existing [Limsa scheduler binding](limsa-fireworks-scheduler-bindings.md)
and [SCB layout](cut-scheduler-clip-layout.md) findings supply the resource
entry path, serialized records and wrapper receiver contract.

The constructor, string lookup and registration observations below used
Python `3.12.0`, pefile `2024.8.26` and Capstone `5.0.7` in x86-32 mode.
Each retained Ghidra instruction in the selected bodies was compared with its
original PE bytes and independently decoded. The reused builder excerpt was
limited to `0x00A28610..0x00A28641`. Literal pointers and vtable cells were
read directly from the PE. This was static original-byte verification, not
runtime observation or a new Ghidra import.

The registry-source trace checked the complete base-constructor body
`[0x00A1EBE0,0x00A1EE17)`, whose instructions cover all `0x237` bytes
without gaps. Reused constructor/argument excerpts were limited to
`[0x00A78DAB,0x00A78DAD)`, `[0x00A78DDF,0x00A78E0E)`,
`[0x00A84EE0,0x00A84F37)`, `[0x00A78D30,0x00A78D58)`,
`[0x00A25E40,0x00A25E62)`, `[0x00A72D2E,0x00A72D30)` and
`[0x00A72E01,0x00A72E2E)`. Each half-open range was compared in full
against original PE bytes and independently decoded with the same Python,
pefile and Capstone versions. Instruction-byte unions exactly covered these
ranges. The reused excerpts cover only the selected field/argument dataflow
below.

To reproduce the byte checks, verify the PE identity above, map each VA with
`pefile.PE.get_offset_from_rva(VA-0x00400000)`, and decode its complete
range using `capstone.Cs(CS_ARCH_X86,CS_MODE_32)`. Compare each retained
instruction's bytes and length with the original and compare the union of
instruction addresses with the stated ranges. Follow the stack adjustments
through each selected call to establish the argument positions below.

## Actor and track preparation

`0x00A298D0` takes `(chunk, factory, arena)` as three dword stack arguments
and returns with `RET 0x0C`. It sign-extends `s16(chunk+0x0A)` for the
actor count and starts actor records at `chunk+0x10`. It reserves `0x14`
arena bytes for a collection and calls `0x00A21DE0` with the count and
arena. That constructor reserves `4*count` pointer bytes, retains the base
at collection `+8`, the count at `+0x0C`, and initializes the append index
at `+0x10` to zero.

For each positive-count record, the actor helper zero-extends byte `+2`
and selects an eight-byte key entry. At `0x00A2993B..0x00A29945`, it calls
`0x00A1F9C0` with `(keyEntry, record, factory, arena)`. That helper loads
a selected node's function pointer from node `+0x10` and calls it at
`0x00A1FA1F` with five dwords `(record, factory, keyEntry, arena, 0)`.
It restores those 20 bytes itself. Its registry lookup `0x00A1FC00`
uses the exact string comparison described below. A node equal to the
retained end node returns zero.
The result is appended unconditionally through `0x00A21D10`, including
null results. The next record adds the sign-extended word at record `+0`.

`0x00A26850` takes `(trackTable, arena)` as dword stack arguments and
returns with plain `RET`; its caller removes eight bytes at `0x00A25CA8`.
It reserves `0x18` arena bytes for a collection, then `0x00A26CA0`
reserves its pointer array and allocates `0x14` bytes per entry through
the uninspected constructor `0x00A2A720`. The count is the track table's
first dword, tested as signed for the positive-count loops.

Each track record begins after that dword. Word `+0` equal to `0xFFFF`
skips `0x00A2A780`; otherwise that word is sign-extended for indexing the
collection and the helper is called with the current entry. Its semantics
remain uninspected. `0x00A2A7D0` retains the serialized record pointer at
current-entry `+0x10`. Record advance is
`(2*(u8(record+3)&0x1F)+7)&~3`. These selected track bodies do not call
the clip builder.

## Synchronous builder invocation

Let `S` be the new object passed into `0x00A26380`, and let
`P = u32(S+0x50)`. After its existing resource, allocation and version
guards, the factory stores the result of `0x00A25DC0` at `S+0x74` and
calls the actor/track parser `0x00A25B90`. The instructions at
`0x00A26582..0x00A2658E` test that parser's dword result and call
`0x00A1C7B0` with `ECX=S` only when it is nonzero. The call happens
before the factory compares used arena bytes with its planned size at
`0x00A26593..0x00A2659B`.

`0x00A1C7B0` constructs local builder `B` through `0x00A28170` at
`0x00A1C7DE`, with `(u32(S+0x7C), u32(P+0x58), P)`. The constructor
stores these arguments at `B+8`, `B+0x0C` and `B+0x10`, respectively,
installs vtable `0x01098650` and returns with `RET 0x0C` at
`0x00A2818D`.
At `0x00A1C7E3..0x00A1C7F8`, it calls `0x00A28510` with two dwords
`(u32(P+0x3C), S+0x60)`. The builder removes eight argument bytes with
`RET 8`. Its ECX is the caller's stack pointer after those pushes plus
`0x20`, the same local region initialized by the constructor. The returned
collection is retained at `S+0x34`.

As established by the SCB layout finding, `0x00A28510` selects an eight-byte
clip key entry using `u8(D+2)` and calls a selected registry node's
`+0x10` function pointer at `0x00A28695`. The argument pushes at
`0x00A28690..0x00A28694` pass `(D, keyEntry, arena, 0)` as four dwords;
the caller removes 16 bytes at `0x00A28699`. The selected node must differ
from the retained end node at `0x00A28661..0x00A28665`. A nonnull factory
return is separate from successful initialization or playback.

If that selected pointer is `0x00837220`, the final zero chooses its
construction branch rather than its size-query branch. The factory reserves
`0x40` arena bytes and, when the original cursor is nonnull, calls
`0x00836B80` with `(D,keyEntry)`. That constructor calls `0x00836660`
and installs primary vtable `0x0103B54C`. Its slot-1 cell at
`0x0103B550` contains receiver-selection method `0x00836730`.
The selection contract below identifies the matching registered key. A
registered pointer and a nonnull construction result do not establish a
fully initialized scheduler wrapper.

## Class key and registry selection

Let `G = u32(B+8) = u32(S+0x7C)` and
`K = u32(B+0x0C) = u32(P+0x58)`. At `0x00A28617..0x00A28620`, the
builder computes `keyEntry = K + 8*u8(D+2)`. At
`0x00A2862A..0x00A2863C`, it calls `0x00A1FC00` with registry `G+8`
as ECX and that entry as its lookup key. The entry's first dword is the
class-name pointer.

`0x00A1FBA0` walks the registry's binary tree using unsigned bytewise
NUL-terminated string ordering. `0x00A1FC00` checks the resulting candidate
against `u32(keyEntry)` with the same string ordering at
`0x00A1FC2D..0x00A1FC59`. Equality returns that node. A miss returns the
registry's end node from registry `+4`. Node `+0x0C` holds the class-name
pointer and node `+0x10` the factory pointer. Selection therefore depends
on exact class-name bytes, not pointer identity or the serialized actor
ordinal.

Registration helper `0x00836E60` reads `0x012CF81C`, whose original PE
value points to `LayRaptureCallSchedulerClip` at `0x0103A5DC`. It pairs
that key with `0x00837220` at `0x00836E7C` and calls `0x0063F710` on
the supplied object `+8` at `0x00836E84`. The insertion path
`0x0063F710 -> 0x0063F240 -> 0x0063E800 -> 0x0061EEC0` compares the
same bytewise keys, allocates and links a node when absent, and stores
the pair at node `+0x0C/+0x10`. An existing equal key returns its node
without replacing its stored factory. Thus reaching this registration call
alone does not prove that an earlier equal entry held the same pointer.

At `0x00637707..0x00637715`, `0x006375A0` obtains the complete
`LaySchedulerManager` and calls its slot 4. The original complete-object
vtable `0x0109E860` selects `0x00A732C0`, whose nonnull branch returns
the object retained at manager `+8`. That returned object is supplied to
`0x00836E60` at
`0x00637735..0x00637736`. This establishes where the registration path
obtains its registry object, conditional on the initialization path reaching
these calls.

### Registry-object source

Let `M` be the complete manager passed to `0x00A72D10` and `F` its
constructed `LaySchedulerFactory`. Dword argument numbers below exclude
the ECX object pointer.

| Native locator | Registry-pointer dataflow |
| --- | --- |
| `0x00A72E0D..0x00A72E29` | Loads `u32(M+8)` into EDI and pushes it as argument 4 to `0x00A78D30`. |
| `0x00A78D34..0x00A78D53` | Forwards argument 4 unchanged to base factory constructor `0x00A25E40`. |
| `0x00A25E54..0x00A25E5F` | Loads argument 4 and stores it at `F+0x14`. |
| `0x00A78DF6..0x00A78E09` | Loads `u32(F+0x14)` and pushes it as argument 3 to scheduler constructor `0x00A84EE0`. |
| `0x00A84EFC..0x00A84F32` | Forwards argument 3 unchanged to base scheduler constructor `0x00A1EBE0`. |
| `0x00A1EC8D..0x00A1EC9F` | Loads argument 3 and stores it at `S+0x7C`. |

The base constructor saves EBX before setting `EBX=ESP` at
`0x00A1EBE0..0x00A1EBE1`, so its `[EBX+0x10]` load is the third
argument. ESI holds its incoming ECX object pointer from `0x00A1EC10`.
The derived constructor's SEH saves and pushes are included in the verified
forwarding range. Their stack adjustments preserve the third argument's
position. The selected factory method's other field loads and moved `+0x20`
value do not supply `S+0x7C`.

Thus this construction path initializes `G` from the registry-object value
retained at `M+8`, through `F+0x14`. It does not use the address `M+8`
as `G`. Registration's complete-manager slot 4 also returns the value at
`M+8`. When registration and factory creation use the same complete manager
and retained registry-object value, they therefore address the same registry:
both registration and builder lookup apply the `+8` adjustment to that
object. This closes the missing constructor-field source, not the runtime
conditions that the initialization calls execute or that the retained value
remains the same between them. The observed loads and stores do not establish
the live values at either call or rule out later field replacement.

### Selected Limsa wrapper

The selected owner-200 `vtp5` span is file
`[0x778D0,0x77D40)` in `data/29/D9/00/00.DAT`, size 1,136 bytes,
SHA-256
`a675e4aa2f1f43da508b91add20ababa093783ec6995ef756e293abaa75a7afc`.
The containing DAT is 529,376 bytes, SHA-256
`b61cf798a02fc1bf72f1445a01b6a94a2e58793e7dd82d0d654f1d422bbceff3`.
Python `struct` parsing using the linked SCB layout found one `@CCPT`
entry: ordinal 0 at file `0x77980`, string token 1, resolving to
`LayRaptureCallSchedulerClip` at file `0x77CE3`. The first clip at
`D = 0x77B20` has `u8(D+2) = 0` and stride `0x2C`.

For that clip, the valid parsed key table selects entry `K`. Under the
manager/registry value identity contract above, if its exact matching node
holds `0x00837220`, the builder selects that factory and supplies
construction flag zero. This is lookup eligibility and conditional factory
selection. The input-to-runtime mapping of `P+0x58`, the actual retained
manager/registry values and the matching node's stored pointer remain
conditions. The remaining discriminator for registry identity is evidence
that registration's returned object value equals the value preserved through
the selected factory/scheduler fields to builder creation. The same complete
manager with an unchanged retained registry pointer is sufficient for the two
manager reads to agree. The constructor dataflow alone does not establish
actual activation.

## Owning-context limit

`0x00836660` loads global `0x013657B4` at `0x008366AE` into clip `+0x38`
and global `0x013657B8` at `0x008366B7` into clip `+0x3C`. These are
the actual global values at those instructions. The latter is the `U`
used by the existing wrapper receiver-selection finding.

The synchronous call chain does not prove those globals retained their
values from UnitTree publication. Actor constructors, intervening virtual
calls and `0x00836EF0` before the final capture remain uninspected context
edges. No selected read joins the instance's retained `+0xFC` field to this
capture. The registry-source constructor dataflow leaves context preservation
across the selected constructor chain separate.

This composition establishes conditional construction calls. It does not
establish allocation success, complete initialization, child registration,
rendering, an authenticated controller actor, a binding or scheduler request
observed in retail, a historical sender, a calendar, or effect placements.
