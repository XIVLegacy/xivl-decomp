# Lobby clear types 0x0007 and 0x0008

## Verdict

The two clear cases are a small connection-control lane, separate from the
type `0x000A` assigned-connection acknowledgement. Receive type `0x0007`
atomically sets connection `+0x38` to one. A later eligible send-selection
pass atomically clears that field and, when the old value was nonzero, builds
one type `0x0008` record. Receive type `0x0008` returns success without a
case-specific call, state write, or payload read.

The type-8 builder contract is closed through its terminal transport dispatch.
It emits a 24-byte clear subrecord inside a 16-byte outer frame header, for a
constructed 40-byte one-record frame. This establishes client mechanics, not
a protocol name or any remote acceptance rule. The sanitized machine-readable
contract is
[`lobby_clear_0007_0008_consumers.json`](../../config/lobby_clear_0007_0008_consumers.json).

## Receive route

`FUN_00DA2330` copies the common 16-byte subrecord header before its type
switch. Types 7 and 8 share the branch at `0x00DA2491`; declared length
`0x18` is required for the complete 24-byte subrecord copy. That comparison is
not a case-specific acceptance gate: after generic bounds permit the record,
the dispatcher can still use the already-copied type header when the full copy
was skipped. Malformed cursor behavior outside this bounded observation is not
closed.

`FUN_00DA25D0` dispatches type 7 to `0x00DA27B0`. The branch calls the PE import
`KERNEL32.InterlockedExchange` with value one and destination connection
`+0x38`, ignores the old value, and returns true. It does not read the copied
payload. Type 8 targets the common true-return block at `0x00DA2678`; it has no
case-specific call or state access and also ignores the payload.

Four direct dispatcher callers at `0x00DA1B7E`, `0x00DAC6DF`, `0x00DAC8A9`,
and `0x00DACA8B` test that boolean result. A true result accepts the current
connection or node for the caller; false continues its search or iteration.

## State and ordering

The `ConsumerConnection` base construction route initializes `+0x38` to zero
through `FUN_00452A40`. `FUN_00DB3300` reaches `FUN_00DB3020` through
`FUN_00DB3280`. When the connection `+0x14` eligibility gate is zero,
`FUN_00DB3020` exchanges `+0x38` with zero:

- old nonzero -> call the type-8 builder `FUN_00DB8090`;
- old zero and the half-interval time test passes -> call the type-7 builder
  `FUN_00DA1D70`; and
- after either build -> store the current low dword of `__time64` at connection
  `+0x30`.

Repeated received type-7 cases before that exchange remain the single value
one, so they coalesce into one type-8 selection. If the `+0x14` gate is
nonzero, the exchange is skipped and the pending value remains set. Both
builders make exact 24-byte records. Neither accesses the received payload.
The type-8 selection passes the current connection-map node key to
`FUN_00DB8090`; the builder obtains its other dynamic u32 directly from the
low dword of `__time64`.

## Type-8 builder and frame

`FUN_00DB3020` only selects a builder when the connection buffer length at
`+0x14` is zero. `FUN_00DB8090` independently requires the sent cursor at
`+0x16` to be zero and checks that cursor plus `0x28` does not exceed the
buffer capacity. Construction through `FUN_00DA1EA0` gives each up-buffer a
`0x1000` capacity, so a normally constructed empty buffer satisfies the
40-byte requirement. The caller does not test the builder return value.

For that empty-buffer route, the builder first zeros the complete 16-byte
outer header. It initializes outer length `+0x04` to 16 and subrecord count
`+0x06` to zero, then appends the record and increments them by 24 and one.
The resulting frame declares length 40 and one subrecord. The record manifest
is relative to the subrecord start:

| Offset | Width | Static source and classification |
|---:|---:|---|
| `+0x00` | 2 | constant declared length `0x18` |
| `+0x02` | 2 | constant clear type selector `0x0008` |
| `+0x04` | 4 | constant zero, common-header field |
| `+0x08` | 4 | constant zero, common-header field |
| `+0x0C` | 4 | uninitialized builder stack slot, indeterminate common-header field |
| `+0x10` | 4 | connection-map node key, dynamic builder input |
| `+0x14` | 4 | low dword returned by builder-local `__time64`, dynamic time source |

The `+0x0C` store at `0x00DB8118` is not fed by an initializing write in the
function. It must not be generalized as a static zero merely because both
retained records contain zero there. The builder advances its cursor by 24,
increments the outer declared length and count, and returns one on success.

## Terminal send path

After selection, `FUN_00DB3020` invokes slot 1 of the
`ConsumerConnection` vtable, `FUN_00DA1480`, with the manager context. That
function sends from connection buffer pointer `+0x10` plus sent cursor
`+0x16`, with remaining length `+0x14 - +0x16`. Its call at `0x00DA14C2`
reaches `FUN_00D36020`, which tail-dispatches to a configured transport
object's virtual write slot. It tests selector-object fields `+0x0C`, `+0x14`,
and `+0x1C` in that order and tail-calls virtual-table byte offsets `+0x20`,
`+0x28`, and `+0x24`, respectively. A positive result advances the sent
cursor; full coverage resets the packet buffer through `FUN_00DB7FB0`. A
result below one clears connection transport state at `+0x80`. Missing
transport state, or a sent cursor already covering the buffer, clears buffer
fields `+0x14`, `+0x16`, and `+0x18` without a transport call.

After either builder is selected, `FUN_00DB3020` also clears
`+0x14` and `+0x16` and atomically clears `+0x18` after the virtual send
returns, regardless of its result. Thus a positive partial-write count is
visible inside `FUN_00DA1480`, but this selected-builder caller does not retain
that cursor after the call.

The client producer is bounded through the transport-selection boundary.
The concrete transport selected by `FUN_00D36020` is not established here.

Within `FUN_00DA1AB0`, the send-selection pass at `0x00DA1B38` precedes the
receive dispatcher at `0x00DA1B7E`. A type 7 received through that poll cannot
affect the already completed pass; a later eligible pass observes it. This
ordering claim is bounded to that caller because the dispatcher has three
other direct callers.

Connection replacement at `FUN_00DA12A0` destroys the old object and constructs
a new one, reinitializing both `+0x38` and the distinct assigned-connection u32
at `+0x04`. No carryover route was found.

## Capture reconciliation

The full decrypted lobby census is pinned to `xivl-captures` commit
`5ec97e317c31c5f0852a518f7b64cf6a09df3286`, artifact
`studies/lobby-handshake-triage/derived/lobby-record-census.json`. That JSON has
SHA-256 `50f59c4f186be104d5d45d955560eea703c9e409ddc6e6fef4d826767bfb3d85`;
its recorded source-capture SHA-256 is
`28e06b54fe559870031f077f8549b9244caafa7e5177dbca08a7feae6c2b1b62`.
Across its two retained sessions, 16 complete frames contain 20 subrecords.

Each server-to-client sequence begins with one exact 24-byte clear type 7,
followed by type 10. Each client-to-server sequence contains one exact 24-byte
clear type 8. Type 8 occurs after the encrypted application record in one
session and before it in the other. The census therefore supports direction,
recurrence, and record shape, but not immediate causality, a protocol noun, or
a strict ordering requirement.

A restricted byte-for-byte comparison of the two complete type-8 records was
reduced to offset classes without retaining their values. Record
`+0x00..+0x0F` is invariant. `+0x10..+0x11` varies,
`+0x12..+0x13` is invariant, `+0x14` varies, and `+0x15..+0x17` is
invariant. Both records have the same zero/nonzero byte-class shape: zero spans
at `+0x01`, `+0x03..+0x0F`, and `+0x12`; all other bytes are nonzero in
both observations. Thus the captures match the builder's framing and both
dynamic sources include observed variation. They constrain observed values
only. They do not show that either dynamic u32, or any nonzero payload byte,
is required in a remotely accepted minimum record.

## Type-10 boundary and open questions

The closed [type-10 consumer](lobby-acknowledgement-consumer.md) requires a
nonzero decrypted u32 and writes active connection `+0x04`; later polling uses
that value to stop setup retries. Types 7 and 8 neither read nor write `+0x04`,
so they do not complete, replace, or bypass that transition.

Static evidence does not establish remote acceptance rules, whether either
clear type is a prerequisite, the meaning of either builder payload dword, or
the full consequence of malformed declared lengths. In particular, type 8 is
not an echo of type 7: its builder uses builder-side values while the receive
case discards both type-7 payload dwords. Type 8 is not established as an
acknowledgement, heartbeat, echo, or acceptance response, and a client builder
cannot prove server acceptance.

Keep four mechanisms separate: type 7 coalesces pending state at connection
`+0x38`; receive type 8 is a no-op; encrypted type 10 consumes a nonzero u32
into connection `+0x04`; and the assigned-u32 lifecycle controls later setup
retry behavior. None supplies a semantic name for the type-8 builder payload.
