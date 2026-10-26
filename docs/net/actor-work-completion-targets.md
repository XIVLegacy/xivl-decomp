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

## Owner, readiness and callback arguments

`0x00773F10` calls resolver `0x00CC7A50` with O as ECX and U as its stack
argument. The resolver uses `*O` as lookup receiver and U as input to the
opaque predicate `0x00CD80F0`, selecting `0x00CD81D0` or `0x00CD8160`.
It returns zero or the first dword of the selected lookup result. Those
three callee bodies and the concrete actor/key association remain untraced.
A zero resolver result exits before property callbacks or Lua dispatch.

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

A local discriminator must correlate the actual actor/context key, O and
resolved owner with the full `0x0137` stream; record `C+0x24`, updater flags,
property readiness/results and entry to `0x00774156`. If transferred, retain
the queue identity and actual later consumer. Widget existence, focus and
rendered timing require their own observations. These are static conditional
facts, without a runtime reproduction or server publication-policy selection.
