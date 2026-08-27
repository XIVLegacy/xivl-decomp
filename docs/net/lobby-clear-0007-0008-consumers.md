# Lobby clear types 0x0007 and 0x0008

## Verdict

The two clear cases are a small connection-control lane, separate from the
type `0x000A` assigned-connection acknowledgement. Receive type `0x0007`
atomically sets connection `+0x38` to one. A later eligible send-selection
pass atomically clears that field and, when the old value was nonzero, builds
one type `0x0008` record. Receive type `0x0008` returns success without a
case-specific call, state write, or payload read.

This establishes mechanics, not protocol names. The sanitized machine-readable
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
builders make exact 24-byte records. Builder-side call context populates
payload `+0x10`, and the low dword of `__time64` populates payload `+0x14`.
Neither builder accesses the received payload.

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
`32a39d2a92f2268d64ab3586b8d791fa93ed19f1`, artifact
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

## Type-10 boundary and open questions

The closed [type-10 consumer](lobby-acknowledgement-consumer.md) requires a
nonzero decrypted u32 and writes active connection `+0x04`; later polling uses
that value to stop setup retries. Types 7 and 8 neither read nor write `+0x04`,
so they do not complete, replace, or bypass that transition.

Static evidence does not establish remote acceptance rules, whether either
clear type is a prerequisite, the meaning of either builder payload dword, or
the full consequence of malformed declared lengths. In particular, type 8 is
not an echo of type 7: its builder uses builder-side values while the receive
case discards both type-7 payload dwords.
