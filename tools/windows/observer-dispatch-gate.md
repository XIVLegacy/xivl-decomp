# Offline observer mutation dispatch

The [dispatch API](observer_dispatch_gate.h) orders individual injected
mutations against permanent observer abort. The
[transaction](observer-diagnostic.md#injected-installation-transaction),
[publication protocol](observer-publication-protocol.md) and
[recovery coordinator](observer-recovery.md) share one attempt's gate.
`observer_runtime_check --self-test` exercises the integration with CPU fakes.

## Attempt wiring

For a gated attempt, the caller supplies a non-null `dispatch_gate` to the
recovery callbacks, hook backend and publication transport. All three must
refer to the same attempt state, including adapters reached through aggregate
callbacks. A null gate preserves the existing ungated injected interface;
the adapters do not validate missing or mismatched wiring.

The pointed-to gates must outlive their adapters, coordinator and outstanding
callbacks. Gate copies share attempt state, but callers must not move from,
replace or reassign a participating gate while the attempt is active. Mutable
component state remains externally serialized. The atomic admission state
does not make those component operations safe to race.

## Admission and completion

Admission establishes which operation won before abort. An admitted operation
may remain outstanding when abort wins. Its eventual result must be recorded
as the actual result of that operation. Abort cannot interrupt its callback
or establish that a blocked native call returned.

Abort permanently prevents later normal mutation and code-bearing cleanup
admissions. It must not wait on a transaction/publication lock, a blocked
callback or a ledger writer. Completion of admitted work does not reopen the
gate or authorize another operation after abort. Normal attempt completion
must compete with abort through the same authority.

## Operation boundaries

The integration covers these injected mutation families:

| Family | Operations |
|---|---|
| Module and hold ownership | Retain/release module, acquire/release quiescence |
| Executable storage | Reserve/free storage |
| Code state | Change/restore protection, write bytes, flush instruction cache |
| Indirect-call targets | Register/revoke CFG |
| Publication | Each record-field write, claim/release controller ownership |

Aggregate publication and clearing may write multiple record fields. Admission
of the outer callback does not authorize all those field writes. Each inner
write needs its own admission under the same attempt authority. Protection
repair and cleanup also remain subject to abort, even when they would otherwise
be attempted after failure.

A refused admission means that callback was not dispatched. It does not prove
that earlier work had no effects or that retained resources are safe to release.
Existing uncertainty and protection failures remain latched. Results and
resource flags for work admitted before abort retain their original meaning.
An installation result can therefore describe completed installation even
when abort won during its last admitted callback. Attempt success remains a
separate decision through the gate; that result does not authorize restoration
or cleanup after abort.

## Scope

Callers still serialize mutable transaction/controller state and establish
creation-owned identity, current hold evidence and all five
[quiescence attestations](observer-diagnostic.md#quiescence-and-resident-module-contract).
The gate does not acquire those proofs. Fixture cleanup, recorder coverage,
creator exit acknowledgment and retained-handle exit confirmation remain
separate recovery outcomes.

The fake checks establish admission ordering and injected-result handling.
Native backend callbacks must expose every mutation that needs an individual
abort boundary. Hidden work inside one admitted callback has no additional
gate supplied by this API. Native creation, execution exclusion, remote
ordering, blocking calls and supervisor/crash behavior require qualification.
No native controller/backend or execution command is provided.
