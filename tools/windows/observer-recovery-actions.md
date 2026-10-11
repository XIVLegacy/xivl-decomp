# Offline recovery action dispatch

The [action adapter](observer_recovery_actions.h) submits work for the
[recovery coordinator](observer-recovery.md) without running transaction or
publication callbacks inside the coordinator's serialization domain.
`observer_runtime_check --self-test` exercises the adapter with CPU fakes.

## Submission and execution

The coordinator's optional `action_with_id` callback receives a copied
`RecoveryActionWork` with its operation ID, execution domain, expected and
observed identities, current state, dispatch gate and bound exit-event
identity. The existing action callback remains available when that callback
is absent. An exit acknowledgment retains its dispatched event binding in its
work and result history. A superseding event can invalidate execution
admission.

A submitted action retains its coordinator operation ID. Submission returns
`InFlight`; it does not establish the action's result. The executor runs the
action outside the coordinator lock and preserves its actual result and
receipt for completion by that same operation ID.

Immediately before execution, the adapter asks the coordinator to admit the
exact queued operation. Expired or superseded work and normal owner work
after abort are refused before executor dispatch. Admission cannot revoke an
operation already executing; its actual return remains evidence.

Owner work and supervisor work use separate execution domains. A blocked
owner operation must not occupy the supervisor executor or hold a queue lock
needed by the supervisor. Exit-event acknowledgment remains creator-owned.
The supervisor cannot take over event continuation when the owner is blocked.

The [dispatch gate](observer-dispatch-gate.md) remains the authority for each
individual transaction/publication mutation. Queuing an aggregate action does
not admit all its internal mutations. Abort operations retain their separate
recovery authorization.

## Caller setup

Use one adapter for one coordinator attempt. Establish its callback set and
attach the coordinator before pumping work. The adapter, coordinator,
executor context and shared dispatch gate must outlive queued work, active
executors and completion delivery. Do not change their configuration while
that attempt is active.

For a gated attempt, supply the same non-null gate to the coordinator,
hook backend and publication transport. The action envelope carries that
attempt's gate; it does not repair missing or mismatched inner wiring.

Supply positive owner and supervisor queue capacities. Those limits bound
queued envelopes; completed result history remains retained for the attempt.
The adapter starts no threads. The caller binds the owner domain to the
actual creator thread and runs the supervisor domain independently. A blank
thread configuration needs explicit binding before its pump can execute.
Thread affinity refuses work on another caller thread; it does not prove
native creation or handle ownership.

## Results and evidence

Abort does not cancel an operation already executing. Its eventual result
retains the original operation ID and actual effects, including unknown
effects. A late result cannot reopen normal mutation or convert an aborted
attempt into success. A termination request remains separate from confirmed
process exit, restoration, publication release and fixture cleanup.

Ledger submission must also return promptly. A failed or unconfirmed write
leaves evidence incomplete. This adapter does not make an arbitrary blocking
ledger writer safe to call inside the coordinator lock.

An executor that returns `InFlight` leaves its operation outstanding. A later
terminal result can be posted by its exact ID from the bound execution domain.
Posting completion while the executor is still running is refused. Neither
duplicate completion nor a completion for another attempt can replace the
first retained actual result.

## Qualification boundary

The fake checks establish submission, execution separation and exact-result
delivery. Callers still supply creation-owned identities, current serialized
state, continuous execution exclusion and all five required attestations.
Step limits count caller steps and supply no elapsed-time or blocked-call
completion guarantee.

Native execution, event affinity, supervisor/crash behavior, remote ordering
and fixture failure behavior require qualification. No native controller,
backend or execution command is provided.
