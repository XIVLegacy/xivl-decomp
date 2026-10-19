# SCB resource parsing and clip construction

The resource parser calls the clip builder synchronously on the nonzero
actor/track parser-result path. The selected call path is
`0x00A26380 -> 0x00A1C7B0 -> 0x00A28510`. Actor and track parsing prepare
collections before that invocation. They are not themselves the clip builder.
The local builder selects a clip factory by its class-name key. The selected
Limsa `vtp5` wrapper supplies `LayRaptureCallSchedulerClip`, and a registration
path supplies factory `0x00837220` for that exact key. The scheduler's
constructor derives its registry object from the complete manager's retained
registry pointer. On the checked SCB parsing path, the parsed clip-key table
resolves the selected wrapper's class token to that exact name. Factory
selection still requires equal retained registry-object values and matching
node state. Complete wrapper initialization and owning UnitTree context at
clip creation remain separate conditions.

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

The class-key producer observations used the same original PE and tool
versions. The [material directory finding](uldah-halloween-material.md#directory-construction-and-descriptor-selection)
supplies the generic directory construction and lookup contracts.
The class-key trace reused only the caller, constructor-field and getter
excerpts listed in [reproduction](#class-key-reproduction). Every selected
instruction and complete stated range was checked against original PE bytes.
The promoted bodies' instruction unions cover their stated ranges without
gaps. Selected Limsa SCB structure metadata was read directly with Python
`struct`; no client execution was observed.

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
constructor `0x00A2A720`. The count is the track table's
first dword, tested as signed for the positive-count loops.

Each track record begins after that dword. Word `+0` equal to `0xFFFF`
skips `0x00A2A780`; otherwise that word is sign-extended for indexing the
collection. The selected entry is the link receiver, and the current entry
is its dword stack argument. The helper supplies the directed parent and
child-list stores described below. `0x00A2A7D0` then retains the serialized
record pointer at current-entry `+0x10`. Record advance is
`(2*(u8(record+3)&0x1F)+7)&~3`. These selected track bodies do not call
the clip builder.

### Track entry construction and parent links

`0x00A2A720` takes the allocated entry address in ECX, copies it to EAX,
and writes the following initial state before its plain `RET`:

| Entry offset | Explicit constructor store | Link interpretation |
| --- | --- | --- |
| `+0` | Dword zero | Parent pointer |
| `+4` | Dword zero | Head of child list |
| `+8` | Dword zero | Next sibling in that list |
| `+0x0C` | Byte one | Meaning unestablished |
| `+0x10` | Dword zero | Serialized record pointer subsequently retained by `0x00A2A7D0` |

The constructor does not write bytes `+0x0D..+0x0F`, install a vtable or
call another function. Its returned EAX is the original entry address.
The allocation excerpt at `0x00A26CF0..0x00A26D26` advances the arena
cursor by `0x14`, compares the old cursor with EBP, and calls the
constructor on the unequal branch. It then appends either that constructor
return or the skipped branch's zero to the collection pointer array and
increments the append count. The constructor itself has no null guard.

Let P be the referenced entry supplied as ECX to `0x00A2A780`, and C the
current entry supplied at `[ESP+4]`. Its complete body performs these stores
in order, then returns with `RET 4` and C still in EAX:

```text
u32(C+0) = P
previousHead = u32(P+4)
u32(C+8) = previousHead
u32(P+4) = C
```

For valid, distinct entries with disjoint storage, this inserts C at the
head of P's child list and records P as C's parent. C's `+8` points to the
previous head. The helper's only stores are the three shown above, and it
calls no other function. With that storage condition, it leaves C's own
child-list head, byte `+0x0C` and record-pointer field unchanged. The
initialized pointer fields and these stores establish a structural
parent/child representation, without assigning execution or scheduling order
to that representation.

At `0x00A268B7..0x00A268CA`, the caller skips only word `0xFFFF` and
otherwise uses the sign-extended word directly as a pointer-array index.
The selected caller excerpt has no bounds check before that indexed load,
and the link body directly dereferences the supplied entries. Valid storage
and parent indices remain input conditions. The subsequent record-pointer
store does not establish that later code preserves or consumes these links.
This construction and link contract supplies no complete scheduler-wrapper
initialization, owning UnitTree context at clip capture or observed playback.

### Track construction reproduction

The track checks used the original PE identity and Python `3.12.0`, pefile
`2024.8.26` and Capstone `5.0.7`, x86-32 mode, described above. Retained
Ghidra instructions served as locators. Each selected instruction's bytes
were compared with the original PE and independently decoded, with matching
instruction boundaries. The actual instruction-address unions were contiguous
and exactly covered these half-open VA ranges:

| Evidence | Instruction union and complete final end | Instruction bytes |
| --- | --- | --- |
| Entry constructor, complete body | `[0x00A2A720,0x00A2A734)` | `0x14` |
| Parent link, complete body | `[0x00A2A780,0x00A2A792)` | `0x12` |
| Reused entry allocation and append operand | `[0x00A26CF0,0x00A26D26)` | `0x36` |
| Reused parent-index, receiver and record operand | `[0x00A268B0,0x00A268E2)` | `0x32` |

The final instruction starts at `0x00A26D1E` in the allocation excerpt
and at `0x00A268DE` in the link excerpt. Include their complete ends
`0x00A26D26` and `0x00A268E2` when reproducing the checks. Map each VA
with `pefile.PE.get_offset_from_rva(VA-0x00400000)` and decode the full
stated range with `capstone.Cs(CS_ARCH_X86,CS_MODE_32)`. Compare the union
of instruction byte addresses, rather than treating an enclosing span or
exported `# size` as body coverage. The checks read no DAT input and
observed no client execution.

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

## Parsed clip-key table

Let F be the selected `LaySchedulerFactory`. The parsed object P is the Q
constructed by `0x00A24E70` on this fireworks factory path. At
`0x00A78DDF..0x00A78DE7`, the factory loads its resource pointer from
`F+0x0C`, calls `0x00A24E70` and pushes the
returned value as argument 6 to scheduler constructor `0x00A84EE0`.
The latter loads incoming argument 6 at `0x00A84EF8` and pushes it as
base argument 6 at `0x00A84F02`. Base constructor `0x00A1EBE0` reads
that argument from `[EBX+0x1C]` at `0x00A1EC25` and stores it at
`S+0x50` at `0x00A1EC6F`. Its saved-EBX stack frame makes that address
the sixth argument. Thus the builder's P follows the parser's returned Q
through an explicit field store. This does not identify P for a separate
material initializer context.

On its accepted construction branch, `0x00A24E70` retains Q in ESI at
`0x00A24F60` and returns it at `0x00A2514D`. It constructs the
published directory at `Q+8`, calls `0x00A27080` on Q at
`0x00A2509C`, then supplies `(Q,J[0],arena)` to `0x00A24400` at
`0x00A250A5..0x00A250B3`. Here J[0] is the first serialized directory
entry. Getter `0x00A21F60` computes entry j as
`u32(u32(E+8)+4*j)+u32(E+0x0C)` for directory receiver `E=Q+8`.
The caller supplies ordinal zero at `0x00A250A6`.

The selected part of `0x00A27080` constructs the string-table reference.
At `0x00A2713E..0x00A27146`, it reads key pointer `0x00FAB6E8` from
cell `0x01097FAC` and calls the published directory lookup on `Q+8`.
The original eight key bytes are `String\0\0`. On a nonzero return T and
nonnull arena cursor, `0x00A2714F..0x00A27169` reserves eight arena
bytes, stores T in their first dword and zero in their second, and retains
their address at `Q+0x5C`. T is the matched directory entry plus `0x20`,
the string-offset table described in the SCB layout.

`0x00A24400` reads child count `s8(J[0]+0x1C)` and iterates children
beginning at `J[0]+16*u16(J[0]+6)`. Getter `0x00A272A0` returns the
current pointer only for a positive remaining count; `0x00A272C0` returns
it unconditionally. Advancer `0x00A27320` adds `16*u16(child+6)` to
that pointer and decrements the count. `0x00A27340` tests whether the
remaining count is positive. At `0x00A244C1..0x00A244DA`, the parser
compares six bytes of the child header with `@CCPT\0` at original PE literal
`0x01098234`. Equality reaches `0x00A244DC..0x00A244F2`, which
supplies `(s16(child+0x0A), child+0x10, arena)` to `0x00A27210` with
ECX=Q. The caller explicitly sign-extends the count at `0x00A244EC`.

`0x00A27210` stores the count at `P+0x54`, reserves `8*count` arena
bytes and stores the old cursor as K at `P+0x58`. For each positive-count
entry i, its instructions at `0x00A27234..0x00A27258` implement:

```text
serializedEntry = child + 0x10 + 4*i
token = u16(serializedEntry)
T = u32(u32(P+0x5C))
u32(K+8*i) = T + s16(T+2*token)
u8(K+8*i+4) = u8(serializedEntry+2)
```

The name pointer is therefore resolved from the `@CCPT` entry's token,
rather than copied from the serialized entry. The runtime key entry has
eight-byte stride; the serialized class entry has four-byte stride.
This mapping requires the selected SCB to reach the checked parser branch,
valid counts and token offsets, nonnull arena storage and preservation of
the constructed fields until the builder reads them. It does not establish
successful wrapper initialization.

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

For this selected span, the `@CRST` chunk C is file `0x77900` and J[0]
is file `0x77930`. The `String` directory entry at file `0x77CB0`
supplies `T = 0x77CD0`. The `@CCPT` child header is file `0x77970`,
with signed count 1. Its first serialized entry's token 1 selects the
signed word `0x13` at T+2. With W denoting the loaded span's start, T is
`W+0x400`, and the native population formula stores `W+0x413` at K.
That pointer addresses the class text at file `0x77CE3`. Conditional on
this input and the parsed storage above, the first clip's class byte zero
therefore selects runtime entry K for `LayRaptureCallSchedulerClip`.

Under the manager/registry value identity contract above, if its exact
matching node holds `0x00837220`, the builder selects that factory and supplies
construction flag zero. This is lookup eligibility and conditional factory
selection. The actual retained manager/registry values and the matching
node's stored pointer remain conditions. The remaining discriminator for
registry identity is evidence that registration's returned object value
equals the value preserved through the selected factory/scheduler fields
to builder creation. The same complete
manager with an unchanged retained registry pointer is sufficient for the two
manager reads to agree. The constructor dataflow alone does not establish
actual activation.

## Owning-context limit

`0x00836660` loads global `0x013657B4` at `0x008366AE` into clip `+0x38`
and global `0x013657B8` at `0x008366B7` into clip `+0x3C`. These are
the actual global values at those instructions. The latter is the `U`
used by the existing wrapper receiver-selection finding.

The synchronous call chain does not prove those globals retained their
values from UnitTree publication. The immediate pre-capture helper has the
narrow preservation contract below. Earlier actor constructors and
intervening virtual calls remain uninspected context edges. No selected read
joins the instance's retained `+0xFC` field to this capture. The registry-source
constructor dataflow leaves those earlier context conditions separate.

### Immediate pre-capture helper

At `0x00836682..0x00836694`, the constructor retains its incoming receiver
in ESI and forwards its two incoming stack arguments to `0x00836EF0`
with the same receiver. The helper's `0x18` bytes of stack setup place
incoming argument 1 at `[ESP+0x1C]` and argument 2 at `[ESP+0x20]`.
At `0x00836F18..0x00836F24`, it pushes those arguments in reverse order
and calls `0x00A21260` with the original receiver. This preserves the
existing `(D,keyEntry)` construction arguments.

The generic constructor has no call or jump. Its memory writes are confined
to receiver-relative dwords through `+0x34`, including the published D and
key-entry stores at `+0x14/+0x18`. The helper's only call is this constructor.
Its remaining writes install vtables `0x0103B0FC/0x0103B0F4` at receiver
`+0/+8` and maintain stack and FS exception state. It returns the same
receiver and removes the two arguments with `RET 8` at `0x00836F58`.
Neither body accesses global `0x013657B4` or `0x013657B8` directly.

The checked caller interval after the helper returns has no further call
before the captures. It writes stack exception state and receiver vtables
at `0x00836699..0x008366A7`, then performs the two global loads and
receiver stores at `0x008366AE..0x008366BC`. For normal construction with
valid receiver, stack and exception storage distinct from the global cells,
these instructions do not change either global. If no external writer intervenes,
the captures therefore retain the values present on entry to `0x00836EF0`.
This establishes preservation across this local interval, without identifying
those values as the owning UnitTree's context or proving preservation from
its earlier publication.

This composition establishes conditional construction calls. It does not
establish allocation success, complete initialization, child registration,
rendering, an authenticated controller actor, a binding or scheduler request
observed in retail, a historical sender, a calendar, or effect placements.

## Class-key reproduction

Check the PE identity above and the selected DAT and SCB identities in
[Selected Limsa wrapper](#selected-limsa-wrapper). Follow the linked SCB
directory and child framing, then read only the selected span's String
entry, offset table, `@CCPT` count/entry and first clip class byte. Apply the
native population formula above before the builder's eight-byte indexing.

Check the complete original-PE ranges below with the same byte, instruction
boundary and union-coverage procedure described in Evidence identity. Ends
are exclusive.

| Purpose | Start VA | Stop VA |
| --- | --- | --- |
| Directory entry getter | `0x00A21F60` | `0x00A21F70` |
| Root child parser | `0x00A24400` | `0x00A2461D` |
| Clip class-key population | `0x00A27210` | `0x00A27262` |
| Current child or null | `0x00A272A0` | `0x00A272AD` |
| Current child | `0x00A272C0` | `0x00A272C4` |
| Child advance | `0x00A27320` | `0x00A2733F` |
| Positive child count | `0x00A27340` | `0x00A27348` |

The reused field/caller excerpts are `[0x00A1EBE0,0x00A1EBE3)`,
`[0x00A1EC10,0x00A1EC13)`, `[0x00A1EC25,0x00A1EC72)`,
`[0x00A24E85,0x00A24E99)`, `[0x00A24F33,0x00A24F7C)`,
`[0x00A24FCB,0x00A25053)`, `[0x00A25091,0x00A250BC)`,
`[0x00A25139,0x00A2514F)`, `[0x00A27080,0x00A270AD)`,
`[0x00A27124,0x00A27128)`, `[0x00A2713E,0x00A2716C)`,
`[0x00A27270,0x00A27286)`, `[0x00A272B0,0x00A272C0)` and
`[0x00A272F0,0x00A272F8)`, together with the previously listed
`0x00A78D80/0x00A84EE0` forwarding excerpts. These cover only the
selected object/argument, String-reference and child-field reads. Reuse the
published generic directory construction and lookup contracts without
assuming that every consumer's P is this Q.

## Pre-capture helper reproduction

Use the PE identity and Python, pefile and Capstone versions in
[Evidence identity](#evidence-identity). Check every retained Ghidra
instruction against the original PE bytes and decode each instruction in
x86-32 mode. The instruction unions cover the following exclusive ranges
without gaps:

| Purpose | Start VA | Stop VA | Instruction bytes |
| --- | --- | --- | --- |
| Pre-capture helper | `0x00836EF0` | `0x00836F5B` | `0x6B` |
| Generic constructor | `0x00A21260` | `0x00A212A8` | `0x48` |
| Caller forwarding through global capture | `0x00836682` | `0x008366BF` | `0x3D` |

The generic constructor's full range is the existing
[generic D-retention evidence](uldah-halloween-material.md#reproduction).
Its complete write and call set supports the additional preservation relation
above. The caller range is limited to receiver/argument forwarding, the helper
call and the immediate interval through both capture stores. Follow the stack
adjustments at each call and distinguish receiver-relative stores from the
absolute global operands. These static checks establish the normal local
instruction path, not live global values or exceptional execution.
