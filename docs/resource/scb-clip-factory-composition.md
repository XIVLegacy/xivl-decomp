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

## Actor-factory construction

The actor-factory constructor `0x00A27450` retains four supplied dwords
without making a call. Its caller `0x00A25DC0` receives F in ECX and
two stack arguments `(A,X)`. The checked caller prefix reads A from entry
ESP `+4` after `0x10` bytes of stack setup, at `0x00A25DD6`.
After an additional saved-ESI push, it reads X from entry ESP `+8`, then
current ESP `+0x1C`, at `0x00A25DF7`. F remains in ECX until
`0x00A25E06` loads its `+0x1C` value for the first constructor
argument. `0x00A25E0B` then selects the constructor receiver.

The caller reserves `0x14` bytes by advancing `u32(A+8)`. On a nonzero
old cursor H, its call at `0x00A25E0D` supplies H as ECX and four
dword arguments in this order:

```text
(u32(F+0x1C), u32(F+8), u32(u32(X+0x50)+0x38), u32(F+0x0C))
```

These are the values loaded for the selected call. The
[outer-factory input relation](../actor/lay-scheduler-member-publication.md#outer-factory-input-and-scheduler-handoff)
identifies X with S on the selected outer path, subject to its ABI and field
conditions. This prefix ends at the constructor call's complete return address,
before any subsequent caller stores or return handling.

With valid receiver and argument storage that do not overlap, the constructor
copies ECX to EAX and performs the following stores:

| Object offset | Stored value | Input relation |
| --- | --- | --- |
| `+0` | `0x01098604` | Installed vtable |
| `+4` | Argument 2 | `u32(F+8)` |
| `+8` | Argument 4 | `u32(F+0x0C)` |
| `+0x0C` | Argument 1 | `u32(F+0x1C)` |
| `+0x10` | Argument 3 | `u32(u32(X+0x50)+0x38)` |

It returns the original receiver in EAX with `RET 0x10`. Its complete body
reads only the four stack arguments and writes the receiver's five dwords.
It has no `CALL` or `JMP` and no absolute global-memory operand. Its own
stores are receiver-relative. The body performs no explicit registry lookup
or node insertion. These instructions establish retained factory inputs and
a vtable installation. They do not establish the semantic types or registry
roles of the retained values, or identify the constructed object as an owning
UnitTree or clip-initialization context.
One of the supplied values could already refer to such state, but these
stores do not authenticate that identity. Later actor lookup, wrapper
construction and context preservation remain separate contracts.

### Retained-input source and actor lookup

Base factory constructor `0x00A25E40` stores its sixth dword argument
at F `+0x1C`. The prefix through `0x00A25E5B` does not adjust ESP.
Its load from ESP `+0x18` at `0x00A25E62` therefore reads entry
argument 6, which it stores at `0x00A25E6F`. On the published derived
factory forwarding path, `0x00A78D30` loads its own entry argument 6
from ESP `+0x18` and pushes that value at `0x00A78D39` for the base
constructor. If F `+0x1C` retains that constructed value until H's
construction, the sixth input is copied onward to H `+0x0C` under the
relation above. Its semantic type remains unqualified.

The actor lookup uses a separately constructed stack-local object R.
At `0x00A26558` and `0x00A2657B`, the caller supplies the retained
EBX value as ECX to `0x00A25DC0` and `0x00A25B90`, respectively.
Under normal register preservation across the intervening calls, both
therefore receive the same F. The resource parser saves its incoming F
in EBX at `0x00A25BB0`.

On the selected actor branch, `0x00A25C32` loads `u32(F+0x10)`
as argument 1 to `0x00A298A0`. Argument 2 is the value loaded at
`0x00A25C2B..0x00A25C2E`, `u32(u32(EBP+0x50)+0x50)`.
The constructor installs vtable `0x010989A0`, copies argument 1 to R
`+4` and argument 2 to R `+8`, and returns R with `RET 8`.
Its complete body has no call or jump. Its own stores are receiver-relative.
Valid receiver and argument storage that do not overlap remain conditions.

The constructor's receiver is ESP `+0x30` after two argument pushes.
After its `RET 8`, the actor-parser call's receiver at `0x00A25C56`
is ESP `+0x34` after three argument pushes. With normal stack cleanup,
these expressions address the same local R. The value read from
`u32(EBP+0x74)` at `0x00A25C3F` is supplied as an explicit stack
argument, separately from ECX. The instruction at
`0x00A298EC` copies R to EBX. At `0x00A29930..0x00A29944`,
the actor loop reads its key-table base from `R+8`, selects an eight-byte
entry using the record's byte `+2`, and loads `u32(R+4)` into ECX for
`0x00A1F9C0`. With the relevant storage and register state preserved,
the selected operands implement:

```text
keyEntry = u32(R+8) + 8*u8(record+2)
lookupReceiver = u32(R+4) + 8
```

The helper prefix computes that receiver at `0x00A1F9CB`, forwards
keyEntry as the query argument and calls the published
[string-key lookup](#class-key-and-registry-selection) at `0x00A1F9D6`.
Thus the lookup receiver derives from `u32(F+0x10)`, copied through R
`+4`. The checked base constructor stores its third argument at F `+0x10`.
The sixth input follows the separate F `+0x1C` to H `+0x0C` retention
path above. This identifies the fields and argument roles, without proving
equality or inequality between their stored values. Their producers before
these constructor arguments are distinct questions. The third input's
manager source is traced below. Preservation until the later field reads,
actual manager/registry identity and owning UnitTree or clip-initialization
context remain separate bindings.

#### Actor-registry producer and registration inputs

Let M be the complete manager and V its retained actor-registry object.
In manager construction, `0x00A72F6B` binds ESI to M. On the selected
construction branch, `0x00A72FCD..0x00A72FD6` supplies the current EAX
value as ECX to `0x00A1FB50` and retains its return in EDI. The alternate
branch selects zero. After conditional cleanup of the old field value,
`0x00A72FF2` stores EDI at M `+0x0C`. The cleanup's indirect effects
remain unqualified. Normal register preservation and valid object storage
are conditions on this retention relation.

Constructor `0x00A1FB50` returns its incoming receiver V. It installs
vtable `0x01097AF8` and calls `0x00A3CAA0` for a pointer E. It stores
E at V `+0x0C`, sets byte E `+0x15` to one, links E `+0/+4/+8` to E
itself, and zeros V `+0x10`. These stores establish the empty-tree shape
at V `+8` used by the published string lookup. They require a valid E
and preservation across the called helper. `0x00A3CAA0` supplies E through
the return of its call to `0x0040E110`, made with the explicit size operand
`0x18`, then initializes node fields. Its allocator-related callees remain
unqualified. The constructor does not establish successful allocation or
complete manager initialization.

At factory creation, `0x00A72D2E` binds ESI to the complete M.
`0x00A72E10` reads `u32(M+0x0C)` into EBX before the virtual call at
`0x00A72E19`. Under normal EBX preservation across that call, the push
at `0x00A72E24` supplies this value as argument 3 to `0x00A78D30`.
Using the published derived/base forwarding contract, that argument becomes
F `+0x10`, then R `+4`, and finally the actor lookup's receiver V `+8`.
This identifies the producer and registry field role. It does not prove
that the retained value survives unchanged until each later read.

The complete manager vtable `0x0109E860` has slot 3 cell `0x0109E86C`
pointing to `0x00A72BB0`. That thunk tail-dispatches the same receiver's
slot 11. With that vtable retained, cell `0x0109E88C` selects
`0x00A72E50`. For a nonzero M `+0x0C` value, the first sequence at
`0x00A72E54..0x00A72E6D`
supplies that value as ECX and these two dword arguments to `0x00A1FAC0`:

```text
(u32(0x012F1EC0) at the getter load, 0x00A73090)
```

Getter `0x00A78650` reads the mutable data cell `0x012F1EC0`, whose
original PE value points to `LayUnitMemberActor\0` at `0x0109F644`, as
also documented in
the [parsed class-name comparison](timeline-weather-record-view.md#synchronous-scheduler-callback).
If the cell retains that original value and the pointed-to bytes are
preserved, the supplied key is `LayUnitMemberActor`. The getter returns
the cell's value at its load; the static PE value alone does not establish
that runtime key.
The already-pushed `0x00A73090` remains the second argument because the
getter returns with plain `RET`. Helper `0x00A1FAC0` copies the two
arguments to a local pair, adjusts ECX by eight, and supplies a second local
stack address and the input pair address to `0x00A1FC90` at `0x00A1FADE`.
This establishes the supplied value pair for the same M `+0x0C` registry
role. Its `LayUnitMemberActor` key remains conditional on the cell and
literal preservation above. The insertion behavior of `0x00A1FC90` is
described below. Reaching the call alone does not establish a node's stored
function pointer.

Registration inputs and actor lookup address the same V `+8` only when
their complete manager and retained object values agree and remain
preserved through F `+0x10` and R `+4`. The separate clip-factory registry
uses M `+8`, F `+0x14` and S `+0x7C`. The sixth factory argument retained
at F `+0x1C` and H `+0x0C` remains a separate role. These field relations
do not establish pointer equality or inequality, owning context, scheduling,
or actor and clip initialization effects.

#### Actor-registry insertion and equal keys

Helper `0x00A1FC90` receives the tree at V `+8` in ECX, an output
address as argument 1 and the supplied two-dword pair's address as
argument 2. At `0x00A1FCAD..0x00A1FCB3`, it reads the pair's first
dword and node `+0x0C` as string pointers. Its comparisons use unsigned
NUL-terminated byte ordering, as does the published actor lookup. The
walk follows node `+0` for a lesser input key and node `+8` otherwise,
stopping at a node whose byte `+0x15` is nonzero. On the normal path in
a valid ordered tree, it checks the candidate or its predecessor, obtained
through `0x004E4020`, before selecting insertion or an existing equal key.

On the absent-key branch, `0x00A1FD14..0x00A1FD23` or
`0x00A1FD89..0x00A1FD90` supplies the original pair to `0x00A43D10`.
That helper calls `0x00C514D0` at `0x00A43D94` with five arguments:
`(endNode,parent,endNode,pair,0)`. The node producer calls `0x0040E110`
with explicit size `0x18`. On its nonzero return N, the producer copies
`u32(pair)` to N `+0x0C` at `0x00C5151B..0x00C5151D` and
`u32(pair+4)` to N `+0x10` at `0x00C51520..0x00C51527`. It copies
the supplied links to N `+0/+4/+8`, sets byte N `+0x14` to zero and
byte N `+0x15` to zero. The key and factory pointer values are copied;
these instructions do not copy the pointed-to string bytes.

On normal return with valid node storage, `0x00A43D10` increments the
tree count at tree `+8`, links N as the root or the selected parent's
left/right child, and updates the end node's extremal links when needed.
Its rebalance code and direct rotation helpers `0x006CEEF0` and
`0x00B9F240` write node links and colour fields without replacing the
stored key/factory pair. The insertion helper's successful output contains
the tree receiver at output `+0`, N at `+4` and byte one at `+8`.

For an equal key, `0x00A1FD85..0x00A1FDA4` instead returns the existing
node with the same tree receiver and byte zero at output `+8`. This branch
does not call the node producer or link helper and does not replace node
`+0x10`. Thus duplicate registration preserves the earlier factory pointer,
even if the supplied factory differs. The existing predecessor helper
changes the output candidate's node address, not its stored pair.

This closes the insertion-to-lookup storage relation: with the supplied pair
preserved across the allocation-related calls, an absent-key normal insertion
retains the supplied name pointer and `0x00A73090` at the exact node offsets
read by the published lookup. With the name-cell and literal
preservation conditions above, the key is `LayUnitMemberActor`. A later
equal lookup can select that factory only if this insertion supplied the
matching node, or an earlier equal node already held the same factory,
and the manager, registry and node values remain preserved. The checked
instructions do not establish those live conditions. Allocation-related
callees, exceptional paths and storage validity remain unqualified. In
particular, the producer's zero-return branch does not prove usable node
storage for the caller's following writes.

#### Actor-registry reproduction

The [registered actor product](../actor/lay-scheduler-member-construction.md)
binds factory `0x00A73090` to its constructor chain, concrete primary and
secondary interfaces and initial binding field. Actual parsed selection and
publication into a city's scheduler collection remain separate conditions.

Use the original PE identity and Python, pefile and Capstone versions in
[Evidence identity](#evidence-identity). This trace used retained Ghidra
listings with original-PE pefile mapping and independent Capstone x86-32
decoding. It did not use a new native-provider analysis or observe execution.
Compare every retained instruction with the original bytes and verify its
complete boundary. The actual instruction-address unions are:

| Purpose | Instruction union | Instruction bytes |
| --- | --- | --- |
| Actor-registry constructor | `[0x00A1FB50,0x00A1FB88)` | `0x38` |
| End-node producer | `[0x00A3CAA0,0x00A3CAFF)` | `0x5F` |
| Complete-manager slot-3 thunk | `[0x00A72BB0,0x00A72BB7)` | `0x07` |
| Registration input sequences | `[0x00A72E50,0x00A72E9A)` | `0x4A` |
| Registration pair forwarding | `[0x00A1FAC0,0x00A1FAE9)` | `0x29` |
| Published actor-name getter | `[0x00A78650,0x00A78656)` | `0x06` |

The reused old operands are `[0x00A72E01,0x00A72E2E)`,
`[0x00A72FC0,0x00A72FF5)`, `[0x00A72D2E,0x00A72D30)`,
`[0x00A72F6B,0x00A72F6D)` and `[0x00A72F8F,0x00A72F92)`.
Each union above is contiguous and was checked through its exclusive end.
Additional complete method checks covered manager setters
`[0x00A72CE0,0x00A72D03)` and `[0x00A73220,0x00A73243)`, each `0x23`
instruction bytes. They write M `+8` and M `+0x10`, respectively, and
do not supply the actor-registry producer. The checks do not qualify their
indirect cleanup callees. Read the two selected manager vtable cells, the
actor-name pointer cell and the complete NUL-terminated literal directly
from the same PE. Reuse the published factory forwarding, stack-local
construction and string-query/output-pair lookup contracts without reopening
their complete bodies.

The insertion trace uses the same retained-listing/original-PE static method.
Its complete instruction unions are contiguous through these exclusive ends:

| Purpose | Instruction union | Instruction bytes |
| --- | --- | --- |
| Actor-registry insertion | `[0x00A1FC90,0x00A1FDAD)` | `0x11D` |
| New-node link and rebalance | `[0x00A43D10,0x00A43EFC)` | `0x1EC` |
| Candidate predecessor | `[0x004E4020,0x004E40A9)` | `0x89` |
| New-node producer and pair stores | `[0x00C514D0,0x00C51537)` | `0x67` |
| Left rotation | `[0x006CEEF0,0x006CEF3E)` | `0x4E` |
| Right rotation | `[0x00B9F240,0x00B9F292)` | `0x52` |

Follow stack adjustments across both calls to distinguish the output address,
insertion side, parent and original pair. Verify the pair loads/stores, both
output flags and the complete rotation write sets against independently
decoded original bytes. Reuse the published input-pair forwarding and lookup
contracts above. No additional literal, vtable or asset read is needed for
this insertion relation. These static checks do not observe a registration
or lookup executing.

#### Retained-input reproduction

The following reused operands were checked with the original PE identity,
Python, pefile and Capstone versions in [Evidence identity](#evidence-identity).
Every selected retained instruction matched original bytes and independently
decoded to its complete boundary. The actual instruction-address unions
covered these half-open ranges without gaps:

| Purpose | Instruction union | Instruction bytes |
| --- | --- | --- |
| Base argument ABI prefix | `[0x00A25E40,0x00A25E5F)` | `0x1F` |
| Base retained-input tail | `[0x00A25E5F,0x00A25E79)` | `0x1A` |
| Derived sixth-argument forwarding prefix | `[0x00A78D30,0x00A78D4E)` | `0x1E` |
| Derived final pushes and base call | `[0x00A78D4E,0x00A78D58)` | `0x0A` |
| Resource parser F binding | `[0x00A25BB0,0x00A25BB2)` | `0x02` |
| Stack-local constructor inputs | `[0x00A25C20,0x00A25C42)` | `0x22` |
| Actor parser call receiver | `[0x00A25C42,0x00A25C5F)` | `0x1D` |
| Actor helper incoming receiver binding | `[0x00A298EC,0x00A298EE)` | `0x02` |
| Actor-loop lookup operands | `[0x00A29930,0x00A29945)` | `0x15` |
| Generic actor lookup prefix | `[0x00A1F9C0,0x00A1F9E1)` | `0x21` |
| Outer factory calls and H retention | `[0x00A26556,0x00A26582)` | `0x2C` |
| Complete stack-local constructor | `[0x00A298A0,0x00A298B9)` | `0x19` |

The final base-tail instruction starts at `0x00A25E72` and ends at
`0x00A25E79`. The derived prefix's final instruction starts at
`0x00A78D4A` and ends at `0x00A78D4E`. The generic helper's final
instruction starts at `0x00A1F9DF` and ends at `0x00A1F9E1`.
Include those complete ends when deriving the instruction unions. Reuse
the constructor and lookup contracts already published here. These checks
do not extend to later callers or unqualified callee and alias effects.

### Actor-factory reproduction

Use the original PE identity and Python, pefile and Capstone versions in
[Evidence identity](#evidence-identity). The retained Ghidra instruction
rows were compared with original PE bytes and independently decoded in
x86-32 mode, including each complete final instruction. The actual
instruction-address unions were contiguous and covered exactly:

| Evidence | Instruction union | Instruction bytes |
| --- | --- | --- |
| Complete constructor | `[0x00A27450,0x00A27477)` | `0x27` |
| Caller input and allocation prefix | `[0x00A25DC0,0x00A25E12)` | `0x52` |

Derive the unions from instruction byte addresses rather than adding an
exported body size to its start. The constructor's final `RET 0x10`
starts at `0x00A27474` and ends at `0x00A27477`. The caller range
includes only its prefix through the call at `0x00A25E0D`, whose complete
end is `0x00A25E12`. Follow its stack changes to distinguish entry
argument positions from current ESP offsets. No asset input or client
execution was used.

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

### Actor collection field handoff

On the actor-parser return path, the instructions at
`0x00A25C5F..0x00A25C66` push EAX, copy the current EBP to ECX and call
`0x00A1BB30`. The immediately preceding call at `0x00A25C5A` targets
`0x00A298D0`, whose actor-collection result and `RET 0x0C` are established
above. No instruction between that return and the setter call replaces EAX
or the pushed value.

The complete ten-byte helper `[0x00A1BB30,0x00A1BB3A)` loads its first
stack argument into EAX, writes EAX to receiver `+0x70`, and returns with
`RET 4`. It performs no null check, allocation or additional call. Let
`W` be the current EBP at `0x00A25C60` and `P` the parser's returned value.
With valid writable receiver storage, the resulting explicit store is:

```text
u32(W+0x70) = P
```

The helper also stores a supplied zero without checking it. It does not
check the collection's count, entries or later lifetime. The published parser
contract independently permits null actor entries within a constructed
collection; those entries are distinct from a zero collection pointer.

The [member-publication contract](../actor/lay-scheduler-member-publication.md#parser-receiver-abi)
assigns that EBP load to parser argument 1 using the complete normal parser
body and iterator cleanup. Its outer-factory input proof joins the supplied
EDI to constructed scheduler S on the selected path, under normal-call and
register-preservation conditions. Thus this branch stores P at S `+0x70`.

If this S equals the scheduler retained at context `+0x0C`, and its field
retains P, this store supplies the collection consumed by the
[indexed actor path](uldah-halloween-material.md#active-parameter-receivers)
and [ShowHideClip operation vector](showhide-clip-dispatch.md#operation-argument-vectors).
Those consumers use `u32(u32(C+0x0C)+0x70)`. The parser's argument identity
does not establish that separate context pointer equality, actual city delivery,
an indexed actor's identity or the operation receiver Q.

The following half-open instruction unions were compared against the original
PE and independently decoded with the pefile/Capstone method and versions
specified under [evidence identity](#evidence-identity). Their complete
instruction coverage totals 221 bytes; no table or string payload is needed
for the field-store claim.

| Role | Instruction union |
| --- | --- |
| Parser entry and initial stack setup | `[0x00A25B90,0x00A25BB7)` |
| Iterator calls and local setup | `[0x00A25BB7,0x00A25BEF)` |
| Receiver load and actor-branch comparison | `[0x00A25BEF,0x00A25C2B)` |
| Actor parser return and setter call | `[0x00A25C3F,0x00A25C67)` |
| Caller argument pushes | `[0x00A26572,0x00A26582)` |
| Complete field setter | `[0x00A1BB30,0x00A1BB3A)` |

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

## Post-builder owner and auxiliary state

Let `C = u32(S+0x34)` be the built clip collection and `A = S+0x60`
the arena. Getter `0x00A21460` returns `u32(C+0x10)`. Accessor
`0x00A21430` takes a dword index i and returns zero when that count is
zero, i is negative or i is greater than or equal to the count using signed
comparisons. Otherwise it returns `u32(u32(C+8)+4*i)` and removes its argument
with `RET 4`.

The caller snapshots that count at `0x00A1C802..0x00A1C809`. If the
snapshot is positive under a signed comparison, it calls the accessor in a
loop for indices starting at zero.
For each nonzero returned clip M, `0x00A1C81D` stores S at M `+0x34`.
With valid collection storage and the count, array and loop state preserved
across the calls, this assigns an owner back-pointer to each nonnull built
entry. The store itself does not supply a clip-initialization context.

At `0x00A1C82A..0x00A1C83A`, the caller obtains the count again and
calls `0x00A281A0` with `(count,A)`, then retains its return at `S+0x30`.
The helper advances `u32(A+8)` by `0x1C`. A nonzero old cursor becomes
the receiver of `0x00A213B0` with the same arguments; a zero old cursor
returns zero. The constructor installs vtable `0x01097F58`, stores A at
receiver `+0x18` and byte one at `+0x14`, then reserves `4*count` bytes
from the arena. It stores that old cursor at receiver `+8`, the supplied
count at `+0x0C` and zero at `+0x10`. Its own instructions do not populate
or clear the pointer slots. Thus the normal result is a secondary collection
with capacity count and append count zero, conditional on valid storage and
32-bit allocation arithmetic.

The next caller interval advances the arena by `0x14`. On a nonzero old
cursor H, `0x00A1C85F` calls `0x00A1C0D0` with receiver H and
`(u32(u32(S+0x50)+0x3C),A)`. A zero old cursor selects zero instead.
The result is retained at `S+0xBC` at `0x00A1C86C`.

Let L be that constructor's first argument. It stores L at `H+4` and
zero at `H+0x0C/+0x10`. Getter `0x00A26980` returns `u32(L+0x14)`.
For a returned count n that is positive under a signed comparison, the
constructor loops from zero to n-1. Accessor `0x00A26990` returns zero when
`u32(L+0x14)` is zero;
otherwise it reads `u32(u32(L+0x0C)+4*i)` without an index bounds check.
At `0x00A1C0FA..0x00A1C103`, the constructor directly dereferences
each result, calls its primary vtable `+0x10` without pushing a context
argument, then adds EAX to `u32(H+0x10)` with 32-bit arithmetic. Valid
entry storage and the relevant collection and loop state remain conditions.
The indirect method's meaning and effects are unqualified.

After that loop, a signed comparison selects H `+8` as the buffer when
`u32(H+0x10)` is less than or equal to four. The greater-than-four
branch selects the old arena cursor and advances it by
`(u32(H+0x10)+3)&0xFFFFFFFC`, using 32-bit arithmetic. At
`0x00A1C12A..0x00A1C139`, the body stores the selected buffer at
`H+0x0C` and calls `0x009D2110` with
`(buffer,0,u32(H+0x10))`. This records the supplied arguments, without
qualifying that callee's writes or preservation effects. The inline storage
interpretation requires a nonnegative length no larger than four. External
storage requires a valid arena region and an adequate nonwrapping reservation.
The checked body does not establish those input conditions.

The normal constructor return is H. The local builder's final call to
`0x00A28190` at `0x00A1C87A` only writes vtable `0x01098650` at
its receiver and returns. None of these selected direct calls passes the
published material initializer's explicit `C_init` argument. Indirect calls
and unqualified helper or alias effects prevent a whole-path absence claim.
The earlier owner store also does not identify S with `C_init` or `C_app`.
The published [base initializer](uldah-halloween-material.md#initializer-context)
later writes `u32(C_init+0x0C)` to M `+0x34`, so the two writes alone do
not establish the context's identity. Complete wrapper initialization and
the owning UnitTree at clip capture remain separate bindings.

### Post-builder reproduction

Use the original PE identity and Python, pefile and Capstone versions in
[Evidence identity](#evidence-identity). Compare every retained Ghidra
instruction against original PE bytes, independently decode its boundary
in x86-32 mode, and compare actual instruction-address unions. The complete
bodies cover these half-open ranges without gaps:

| Purpose | Instruction union | Instruction bytes |
| --- | --- | --- |
| Built-collection count | `[0x00A21460,0x00A21464)` | `0x04` |
| Built-collection accessor | `[0x00A21430,0x00A21451)` | `0x21` |
| Secondary collection allocation | `[0x00A281A0,0x00A28201)` | `0x61` |
| Secondary collection constructor | `[0x00A213B0,0x00A213ED)` | `0x3D` |
| Auxiliary state construction | `[0x00A1C0D0,0x00A1C145)` | `0x75` |
| Parsed collection count | `[0x00A26980,0x00A26984)` | `0x04` |
| Parsed collection accessor | `[0x00A26990,0x00A269A8)` | `0x18` |
| Local builder cleanup | `[0x00A28190,0x00A28197)` | `0x07` |

The reused caller unions are `[0x00A1C7CB,0x00A1C7CD)`,
`[0x00A1C7E9,0x00A1C7EC)`, `[0x00A1C7ED,0x00A1C7EF)` and
`[0x00A1C7F8,0x00A1C892)`. The first three bind S to ESI, A to
`S+0x60` in EBX and the initial index to zero in EDI. The tail's final
instruction starts at `0x00A1C891`; include its complete end
`0x00A1C892`. Track stack changes and distinguish the built collection
at `S+0x34` from L at `u32(u32(S+0x50)+0x3C)`. These checks consumed
no asset input and observed no execution.

## Secondary collection callers and callback context

The [clip-index contract](cut-scheduler-clip-timing.md#clip-index-word) owns
the built-list lookup and secondary append performed by `0x00A28210`.
The caller paths below supply its actual arguments and retain the incoming
receiver through population and the published callback-context producer.
They do not establish delivery of a constructed combat Scheduler to that
later updater entry.

Let A=`u32(S+0x30)`, B=`u32(S+0x34)`, Q=`u32(S+0x50)` and
G=`u32(S+0x7C)`. The preceding construction finding owns those fields.
Here S names the incoming receiver, with retained-field identity as a condition.

### Population arguments and pointer transfer

`0x00A1CB20` retains its receiver in ESI, stores its first argument U at
S+0xB4, and clears S+0x94 and A+0x10. Getter `0x00A27BD0` returns
k=`u8(u32(U+4))` with plain `RET`. G pushed before that getter therefore
survives, and `0x00A1CB4D` calls `0x00A1BA90(S,k,G)`, with S in ECX.
The callee's `RET 8` consumes k and G. Valid U and a supported k remain
conditions; the field store alone does not select a valid block.

`0x00A1BA90` builds a local receiver from `(G,u32(Q+0x58),Q)` through
`0x00A28170`, whose `RET 12` consumes those constructor arguments.
It pushes B and then k before calling `0x00A26990` at `0x00A1BAD8`,
with ECX=`u32(Q+0x3C)`. The getter removes only k with `RET 4`, leaving
B on the stack. The following pushes supply its returned V and A, so
`0x00A1BAE6` invokes `0x00A28210` with exactly `(A,V,B)`.
That method's `RET 12` removes all three arguments. Its ECX is the local
builder, not S. The block getter has no independent bounds check for k.

For entry ESP R of `0x00A1BA90`, its local/saved-register setup gives
ESP=R-0x24. B is at R-0x28 and k at R-0x2C before the getter call.
After `RET 4`, pushing V and A gives R-0x30; `0x00A28210` enters at
R-0x34 with A/V/B at entry +4/+8/+0x0C. Its `RET 12` restores R-0x24.
Treating the intermediate getter as plain `RET` would change this ABI.

The block's virtual +0x1C must supply a valid iterator with the expected
cleanup. `0x00A28289` sign-extends record+0x0C to built-list index i;
`0x00A28295` queries B through `0x00A21430`. A nonzero result M is
passed unchanged to the appender on A at `0x00A282A6`. The appender
`0x00A21520` increments A+0x10 without a capacity check. A zero lookup
takes the diagnostic branch and is not appended. Valid count, capacity,
iterator and retained array storage remain necessary.

This relates B[i] to a resulting A[j]; it does not equate i and j.
Secondary positions depend on iterator order and successful nonnull appends.
The earlier count reset defines a fresh sequence only if intervening calls
preserve the required fields and storage.

### Same incoming receiver at initialization dispatch

`0x00A1E710` retains incoming ECX in ESI. Its first argument b is passed
to `0x00A28090` on `u32(Q+0x40)`, producing U. That getter has `RET 4`
and no independent b bounds check. The nonzero-U branch calls
`0x00A1BD20` with `(k,b)` and `RET 8`, retains U at S+0xB4, resets
S+0x94 and A+0x10, then calls `0x00A1BA90(S,k,G)` at `0x00A1E76B`.
The zero-U path does not establish this population.

After population, `0x00A1E788` calls `0x00A1E540` with ECX=S, then
`0x00A1E797` explicitly supplies that same S to `0x00A1E380`.
The [scheduler-time contract](cut-scheduler-clip-timing.md#scheduler-time)
owns the latter's conditional call to `0x00A1D330` at `0x00A1E419`.
The receiver is retained in EBP and reloaded into ECX on that call.
Expected callee-saved register and required-field preservation remain conditions.

The [stack-context producer](uldah-halloween-material.md#stack-context-producer-and-initialization-dispatch)
constructs local C from that incoming S and supplies C to slot 1 of a
nonzero secondary entry M on its qualified branch. Under its existing
retention, storage, state and table conditions, the relation is:

```text
B[i] -> A[j] -> M slot1(C)
u32(C+0x0C) = S
```

If M retains the [RaptureMotionClip table](combat-motion-name-provider.md#factory-identity),
that slot reaches its published initializer and RIDTBL name consumer.
This is conditional invocation capability, not an observed selected clip.

`0x00A1E820` also retains its incoming receiver and forwards it to
`0x00A1E710` at `0x00A1EA27` on its state-dependent nonnegative-selection
path. `0x00A1F3C0` retains its incoming receiver across its calls to
`0x00A1E820` and `0x00A1E380`. No checked caller supplies the constructed
combat S to those updater entries. That owner/state edge, selected block,
iterator target, clip indices and retained storage remain separate bindings.

### Caller reproduction

The original PE identity and pefile/Capstone method above apply. Complete
instruction ranges checked for the caller extension are half-open:

| Function | Instruction union |
|---|---|
| 0x00A1CB20 | [0x00A1CB20,0x00A1CB5D) |
| 0x00A1BA90 | [0x00A1BA90,0x00A1BB0E) |
| 0x00A28210 | [0x00A28210,0x00A28349) |
| 0x00A1E710 | [0x00A1E710,0x00A1E814) |
| 0x00A1E820 | [0x00A1E820,0x00A1EA3F) |
| 0x00A1F3C0 | [0x00A1F3C0,0x00A1F560) |
| 0x00A1BD20 | [0x00A1BD20,0x00A1BD88) |
| 0x00A28090 | [0x00A28090,0x00A280A8) |
| 0x00A26990 | [0x00A26990,0x00A269A8) |
| 0x00A27BD0 | [0x00A27BD0,0x00A27BD7) |

The reused construction, collection getter/appender and wrapper bodies were
also checked. The `0x00A1E540` union is
`[0x00A1E540,0x00A1E677)` and `[0x00A1E680,0x00A1E705)`; its gap
is not instruction coverage. The published local-context producer retains
its own instruction union and intervening-call qualifications. No runtime
entry, selected bank, clip invocation or playback was observed.

## ActionClip child scheduler delivery

The additional direct caller of `0x00A1F3C0` supplies an ActionClip-owned
child AccScheduler. It does not supply the root combat Acc/Mcc instance.
This closes a conditional child construction-to-update relation while leaving
root ingress to the updater unproved.

### ActionClip identity and construction

Original primary table `0x01096F6C` has slot 1 `0x00A18170`, slot 2
`0x00A18480`, slot 4 `0x00A17CB0` and slot 49 `0x00A17A30`.
Its COL at `0x01185158` points to type descriptor `0x012CE820`, with
decorated name `.?AVActionClip@Plugins@Cut@Engine@CDev@SQEX@@`.
Constructor `0x00A179C0` installs this table at `0x00A179E5` and
clears clip+0x3C at `0x00A179F9`.

Let L be that ActionClip, C0 its supplied context, P=`u32(C0+0x0C)`
and N the child below. On successful state/resource branches, slot 1 calls
slot 49 at `0x00A18364` or `0x00A183B7` with four arguments:
`(C0,resourceValue,retainedEDI,D)`, where D comes from L+0x14.
The resource and state checks remain conditions, rather than observed success.

Slot 49 constructs a local AccSchedulerFactory F through `0x00A19B60`
at `0x00A17A7E`, then calls `0x00A1A470(F,O,0)` at `0x00A17A96`.
O reuses slot 49's incoming first-argument stack cell at entry ESP+4;
C0 was retained separately in EDI at `0x00A17A4C`. F is distinct from O.
On its nonzero allocation
branch, `0x00A1A470` supplies N as ECX to `0x00A1A670` at
`0x00A1A4FF`. That constructor calls the base Scheduler constructor,
installs Acc primary/secondary tables `0x0109765C/0x0109764C` at
`0x00A1A6CB/0x00A1A6D1`, and returns N with `RET 0x18`.
Allocation failure supplies zero.

The factory writes N to *O at `0x00A1A50C`, invokes initializer
`0x00A1A1E0(F,N)` without testing its success return, and returns O
with `RET 8`. Immediately after that return, `0x00A17A30` loads
N=*O at `0x00A17A9B`, clears *O, and stores N at L+0x3C at
`0x00A17AC1`. Between the load and store, an unequal nonnull previous
L+0x3C can receive a deleting slot-0 call. Normal ABI, nonaliasing storage
and retention through those calls remain necessary. Slot 49 later stores
one at L+0x40 at `0x00A17C7D`.

The local factory inherits inputs from P. With R=`u32(P+0x74)`,
the five arguments supplied to `0x00A19B60` are
`(resourceValue,u32(R+4),u32(P+0x80),u32(P+0x7C),P)`.
Its direct base call `0x00A25E80` stores argument 4 at F+0x14,
`u32(P+0x84)` at F+0x18, and `u32(u32(P+0x74)+0x0C)` at F+0x1C.
This qualifies a copied registry input from P+0x7C. Equality with the
[global root registry](combat-motion-name-provider.md#global-registration-and-root-construction)
still requires P's identity and retained fields.

### Retained child into update

ActionClip slot 2 at `0x00A18480` retains L in ESI. Its base update and
state/resource branches must return normally, and L+0x40 must be nonzero
after `0x00A184BD`. At `0x00A185D1`, it loads ECX=`u32(L+0x3C)`
and calls `0x00A1F3C0` at `0x00A185D4` with no adjustment or explicit
stack argument. If the same L retains the nonzero constructed N, this supplies
that same N to the [existing updater/context chain](#secondary-collection-callers-and-callback-context),
subject to all its collection, iterator, table, state and field conditions.

L slot 4 at `0x00A17CB0` can invoke N virtual +0x2C and deleting slot 0,
then clear L+0x3C at `0x00A17CE2`. Thus neither child lifetime nor pointer
retention from construction to update follows solely from the constructor.
This path already needs P active enough to invoke L and creates a separate N.
It supplies no equality N=rootS, owner registration for rootS, or first updater
entry for P. It does not establish selected invocation or playback.

### Other checked entries and reproduction

Acc secondary table `0x0109764C` slot 1 and Mcc secondary table
`0x01097148` slot 1 both select `0x00A19140`. With incoming ECX=S+4,
this wrapper subtracts four and calls complete S primary slot 19, then returns
with `RET 0x0C`. Acc selects `0x00A1A700`; Mcc selects `0x00A18FC0`.
Their checked bodies construct/populate resource collections and have no direct
call or jump to `0x00A1F3C0`, `0x00A1E820`, `0x00A1E710` or
`0x00A1E380`. Their external and indirect callee effects remain separate.

The additional tail entry `0x00A1EA40` retains incoming S, calls
`0x00A1C6E0` with &S+0x38, restores ECX=S at `0x00A1EA4C` and
jumps to `0x00A1E820` at `0x00A1EA4F`. No entry in the vtable-slot
catalog targets it, and a direct CALL/JMP search of retained `0061*.s` and
`0062*.s` found no caller to it. That module-limited absence does not establish
whole-program absence. The supplier of rootS to an updater remains unproved.

Using the PE identity and byte/union procedure above, check complete bodies
`0x00A179C0`, `0x00A17A30`, `0x00A17CB0`, `0x00A18170`,
`0x00A18480`, `0x00A19B60`, `0x00A1A470`, `0x00A1A1E0`,
`0x00A1A670`, `0x00A25E80`, getters `0x00A20040/50/60/70`,
`0x00A1B950/60`, `0x00A19140`, `0x00A18FC0`, `0x00A1A700`
and `0x00A1EA40`, plus the cited table/COL/type-descriptor cells.
Follow output addresses and `RET` cleanup separately from object
receivers. The checked twenty bodies have contiguous instruction unions.

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
