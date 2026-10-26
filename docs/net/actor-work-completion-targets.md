# Actor work completion targets

The actor `0x0137` completion route attempts its Lua work notification before
a conditional updater transfer. Its initialization, owner and readiness guards
are separate from the Group `0x017A` parser's state and pending-map behavior.
The [SyncWriter finding](sync-writer.md) owns typed property application.

## Selected target and record

`0x00775A30` looks up its second argument in the manager map. A found context
C with nonzero `C+0x24` reaches actor parser `0x00775180`; missing or inactive
contexts enter a separate caching path whose replay is not established here.
The caller's first argument O is forwarded unchanged through the parser and
record processor to the callback helper.

At `0x00775282-0x007752B0`, the parser uses initialized thresholds `0x60`,
`0x82`, `0xA4` from VAs `0x00FE059B`, `0x00FE05A0`, `0x00FE05A1`.
Tags `0x82..0xA3` describe completion targets of length `tag-0x82`.
Thus `0x9B` passes exactly 25 bytes to `0x00CCAC60` at `0x007752D2`, then
advances the cursor by 25. `charaWork/battleParameter` has that ASCII length.
The common decoder's established contract makes it an unindexed two-string
target; no NUL is included in this count. A following byte is another record
tag. The unopened decoder's malformed/embedded-NUL behavior is not established.

The common decoder, target assignment and Lua argument builders are reused
from `xivl-client-structs:docs/linkshell-rank-work-contract.md`, sections
`Native 0x017A receiver` and `Continuation and completion`. The actor calls
those exact common helpers; this does not import Group updater semantics.

The ordinary completion path creates a `0xB8`-byte record through
`0x007765B0`, with property-prefix count at `+0`, target at `+4`, completion
byte 1 at `+0xB4` and callback byte 1 at `+0xB5`. Only after the stream loop
does `0x007756F2` process the records through `0x00774220`. The parsed
properties' slot-1 setter calls at `0x00775652` therefore precede the target's
callback attempt.

## Actor state and updater

For a non-init target, `0x00774220` requires `C+0x24==2`.
The empty/`_init` exception can change state 1 to 2; its record has flags 1/0
and suppresses the ordinary Lua notification. It is a separate branch.

The record processor looks up the target in `C+0x18`, removes a found updater
for reuse, or obtains another updater through its separate reserved-key or
constructor paths. It supplies the selected property-pointer range, then a
nonzero record completion byte reaches `0x00762D20` at `0x00774486-0x0077449D`.
The helper assigns the target at updater U's `+0x14`, copies record `+0xB5`
to `U+0xC4`, and clears readiness cells `U+0xC6/+0xC7`. The completion byte
at `+0xB4` and callback byte at `+0xB5` are distinct.

At `0x007744AE` the processor first calls `0x00773F10(U,&status,O)`. Only
after that attempt does it compare status with the symbolic global at
`0x0134BB04`; inequality selects the later transfer call. Runtime contents
of that global are not established by file-backed bytes.

## SyncMemory wrapper inputs

Let I be wrapper `0x00759ED0`'s incoming ECX, P its second stack argument,
and T its temporary receiver. The
[incoming-state finding](../script/job-action-acquisition.md#incoming-state-and-outgoing-work-requests)
owns the one-byte length N at P and the copied N-byte stream starting at P+1.
At `0x00759EFA-0x00759F06`, the wrapper supplies `(P+1,N)` to constructor
`0x0089F4C0` with ECX=T. The constructor stores `0x0105747C` at T+0
and requests `0x00531320(this=T+4, begin=P+1, end=P+1+N)` at
`0x0089F4FE-0x0089F50C`. The copy/storage helper's successful completion
remains a condition; this selected body does not independently validate it.

At `0x00759F13-0x00759F1F`, the wrapper supplies Receive `0x0089E550`
with ECX=T and ordered stack arguments `(O=I+4, X=dword[I+8])`.
O is an address, separately saved in ESI by Receive. It is not `dword[I+4]`.
The caller of the wrapper and the provenance of I/P remain unselected.

Receive calls `0x00CC7510` with ECX=O at `0x0089E57B`. If H is its
return, `0x0089E584-0x0089E586` load `B=dword[dword[H]+4]`.
The later manager receiver is `dword[B+0xEC]` at `0x0089E59E`.
These operands do not identify the live manager or owner.

Receive next supplies `0x00CC73B0(this=O, A, X)` at `0x0089E591`.
At `0x0089E589-0x0089E58E`, X is pushed before A is formed. The LEA
therefore addresses Receive's incoming first-argument stack word, whose O
value was already saved in ESI. A is not the X slot or an implementation-field
address.

The helper loads `dword[O]` into ECX and calls
`0x00CD7A30(this=dword[O], X)` at `0x00CC73B5-0x00CC73C1`.
Let J be the selected registry entry returned by that lookup. It loads A
into ESI at `0x00CC73C6`, then calls `0x00CC7030(this=J, A)` at
`0x00CC73CA-0x00CC73CD`. The existing
[binding-reference finding](../event/counted-lua-item-references.md#wire-shape)
and [entry-key copy finding](../script/delegate-command-producer.md#refusal-reply)
own the lookup and the dword copy from entry J+0x70 into A.
`0x00CC73D2` returns A in EAX, with `RET 8` at `0x00CC73D6`.
After normal helper completion, the call at `0x0089E5AA` is:

| Operand of `0x00775A30` | Selected source |
| --- | --- |
| ECX | `dword[B+0xEC]`, loaded at `0x0089E59E` |
| Argument 1, O | Saved ESI, pushed at `0x0089E5A9` |
| Argument 2, Kp | A returned in EAX, pushed unchanged at `0x0089E5A8` |
| Argument 3, S | Address T+4, formed/pushed at `0x0089E5A4-0x0089E5A7` |

Thus S addresses the receiver's embedded stream storage, while Kp=A
addresses the copied key `dword[J+0x70]`. X remains a lookup input; neither
`[Kp]=X`, `Kp=I+8` nor a concrete numeric key/packet actor ID is established.

On the accepted new-updater construction path below, the initial value at
newU+0 is consequently `dword[Kp]=dword[A]=dword[J+0x70]`. Existing
context/state/readiness guards, reuse paths and callback/transfer conditions
still apply. This identifies operand transport without identifying a packet
actor ID, registry entry, live binding or particular runtime actor.

The local cleanup call `0x0089E5BB` uses A after `0x00775A30` returns.
Receive returns with `RET 8` at `0x0089E5D2`; the wrapper then requests
destruction of T at `0x00759F30` and returns with `RET 8` at `0x00759F45`.
If new-updater construction occurs during this `0x00775A30` call, its copy
precedes these normal-path cleanup requests. Later key/object/registry or cached replay lifetime remains unproven.
Valid input/storage, ordinary callee-preserved registers, expected argument
cleanup and normal helper returns remain conditions. These static calls do
not establish server publication policy, rendering, historical invocation
or encounter causality.

The additional selected ranges in the same pinned original `ffxivgame.exe`
are `[0x00759ED0,0x00759F48)`, `[0x0089E550,0x0089E5D5)`,
`[0x0089F4C0,0x0089F52E)` and `[0x00CC73B0,0x00CC73D9)`, totaling
404 bytes. Retained Ghidra listing bytes were matched to original
section-backed PE bytes with pefile
`2024.8.26` and every byte was independently decoded with Capstone `5.0.7`,
x86-32. The old export run identity was not established; original-byte
comparison and complete-range decoding support these local operand facts.
There were no listing gaps, undecoded tails or interior returns;
each selected body follows a straight-line normal path through calls to its
final return. Unselected callees, unwind paths and typed static data were
not inspected. Reproduction authenticates the binary size/hash/image base
given below, maps these exact ranges through PE raw section offsets, compares
every listing byte and independently decodes their complete extents.

## New updater key construction

Let Kp be the pointer supplied as the second argument of `0x00775A30`.
The caller passes that pointer to its manager lookup at `0x00775A63`, then
forwards `(O,Kp,S)` to the parser at `0x00775ABB`. Parser `0x00775180` saves
Kp at `0x007751DA` and passes it as record processor argument 2 at
`0x007756EE-0x007756F2`. Its alternate record call also preserves Kp/O at
`0x00775774-0x0077578B`. O is a separate operand.

Both new-updater allocation paths in `0x00774220` allocate `0xC8` bytes,
then call constructor `0x00768260` at `0x0077432B` or `0x007743D8` with
ECX=U and arguments `(Kp, zero-extended C[+0x29])`. At
`0x0076828C-0x00768292`, the constructor reads the dword pointed to by Kp
and stores it at U+0. It copies the second argument to `U+0xC5` and clears
`U+0xC4/+0xC6/+0xC7`. Its initial key is therefore the copied value `[Kp]`,
rather than Kp itself or a fixed vtable pointer.

The same constructor clears `U+0x08/+0x0C/+0x10`, initializes the target
at U+0x14 and returns U with eight-byte stack cleanup. Its second argument's
nonzeroness also selects `0xA0` or `0x300` for the call at `0x007682E9`;
the selected instructions do not make that byte an actor-category selector.
It is distinct from the later callback byte at U+0xC4.

Found target/reserved-key updaters take reuse paths. These stores establish
the key for new construction only, without proving every reused updater came
from this invocation's Kp or that opaque helpers never change its key.

## Owner, readiness and callback arguments

`0x00773F10` calls resolver `0x00CC7A50` with O as ECX and U as its stack
argument. The resolver uses `*O` as lookup receiver and U as input to the
opaque predicate `0x00CD80F0`, selecting `0x00CD81D0` or `0x00CD8160`.
It returns zero or the first dword of the selected lookup result. The existing
[key classifier](../event/event-base-callback-arguments.md#runeventfunction-binding)
and `xivl-client-structs:manifests/talk_native_call_contract.json`,
`identityLookup`, own those conditional registry lookups. For a newly
constructed U, the resolver's U input points to the copied `[Kp]` key above.
The concrete numeric key, registration and live actor association remain
unestablished. A zero resolver result exits before property callbacks or Lua
dispatch; the construction byte `C+0x29` is not shown selecting the lookup.

With a resolved result, the helper tests the target strings and cached
readiness at `U+0xC6`. When uncached, every collected property's virtual
`+0x0C` check must pass before that cache is set. A conditional `U+0xC7`
check through `0x0077C440` can also return early. The passing path dispatches
the collected properties through virtual `+0x10` at `0x00774041-0x0077404D`,
before the Lua call. The selected property registrations and changed-value
semantics are not supplied by the target marker alone.

When `U+0xC4` is nonzero, two strings from `U+0x14/+0x68` are appended to
the argument vector. An indexed target additionally appends both raw u16
indices plus one. The unindexed selected target therefore supplies
`_onUpdateWork("charaWork", "battleParameter")` after the receiver. The
literal is at `0x00FD8650`; Lua dispatcher `0x00CC7A90` is called at
`0x00774156`, with O as its ECX receiver and the resolved result as its
first stack argument. This binds operand transport, without establishing
the concrete live actor or callback arrival.

The later transfer at `0x007744EF` reaches `0x007851E0`. That wrapper keeps
a local U, clears its incoming argument copy, and passes the local address
to the already established common deque append `0x007238B0` at `0x00785218`,
with its entry ECX unchanged. The later scheduler consumer and execution
timing remain unbound. The selected actor parser contains no Group-style
initial duplicate-stream compare/copy calls; this does not exclude suppression
elsewhere or prove repeated unchanged packets always invoke Lua.

## Evidence and remaining discriminator

Input: retail `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86, image base
`0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained disassembly was matched instruction by instruction to original
section-backed bytes and independently decoded with Capstone 5.0.7, x86-32.
The inspected instruction unions totalled 4168 bytes; 50 file-backed typed
bytes authenticated thresholds and selected literals. The bodies were
`0x00775180`, `0x00775A30`, `0x00774220`, `0x00773F10`, `0x007765B0`,
`0x00762D20`, `0x00CC7A50` and `0x007851E0`. The retained record-processor
gap `[0x0077450D,0x00774510)` was excluded from authentication and coverage.
Focused branch/operand checks distinguished target lengths, flags, state
admission, readiness exits, callback arguments and pre-transfer ordering.

The key-construction finding additionally authenticated constructor
`[0x00768260,0x0076830D)` and the cited caller/parser/record-processor
transport against that same original PE using Capstone 5.0.7. These were five
selected bodies totalling 3168 instruction bytes: the 173-byte constructor and
2995 bytes of previously covered code, including the 11-byte classifier.
The retained record-processor gap stayed excluded; no typed static data was
read. Exact operand checks distinguished copying `[Kp]` from storing Kp,
a fixed pointer, or the separate construction byte. The wrapper/helper
transport above qualifies the selected entry's copied binding reference;
its concrete numeric value and runtime identity remain unestablished.

A local discriminator must correlate the actual actor/context key, O and
resolved owner with the full `0x0137` stream; record `C+0x24`, updater flags,
property readiness/results and entry to `0x00774156`. If transferred, retain
the queue identity and actual later consumer. Widget existence, focus and
rendered timing require their own observations. These are static conditional
facts, without a runtime reproduction or server publication-policy selection.
