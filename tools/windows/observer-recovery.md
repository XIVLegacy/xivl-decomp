# Offline controller recovery

The [recovery API](observer_recovery.h) coordinates cancellation and exit
decisions around the [installation transaction](observer-diagnostic.md#injected-installation-transaction)
and [publication protocol](observer-publication-protocol.md). It uses injected
identities, state observations and operation results. The runtime check's
`--self-test` exercises the model with fakes. No native recovery backend or
execution command is provided.

## Identity and ownership

Bind recovery to one newly created observer instance, its executable identity,
the retained process-handle identity and its dedicated event-owner thread.
An observed PID, reopened process or replacement owner cannot substitute for
that binding. The model's opaque handle identity represents a creation-owned
handle; it does not obtain or validate a native handle.

Supply positive finite step limits for reaching a hold, known cleanup, owner
responsiveness, owner exit, termination, event acknowledgment and exit
confirmation. These count caller-supplied steps, not elapsed time. The model
selects no native timeout defaults. Expiry changes the proposed disposition;
it cannot establish that a blocked native call returned or that a process exited.

Owner operations and snapshots of `HookInstallState` and
`ObserverPublicationController` must be serialized. The supervisor may request
abort without inspecting those mutable objects or waiting on the owner.
Abort remains permanent for the attempt. Old normal-work completions cannot
authorize further cleanup or report normal success after abort wins. Registered
abort operations and the required exit acknowledgment may still complete and
advance abort recovery. An operation already dispatched remains an in-flight
operation until its actual result is known.

Action and ledger callbacks run within the coordinator's serialization domain
and must return promptly. Action callbacks submit outstanding work as `InFlight`
and retain its operation ID for completion. Ledger submission must also be
nonblocking; an unconfirmed write leaves evidence incomplete.
Callbacks may request atomic abort and use the documented observation/getter
APIs; mutating step reentry is refused. A blocked callback prevents serialized
steps from progressing, so this contract does not establish a native supervisor
deadline for an arbitrary blocking callback.

The [action adapter](observer-recovery-actions.md) queues action work for
execution outside this serialization domain and retains exact operation IDs
for completion. Owner and supervisor work use separate execution domains.
It supplements the prompt action/ledger submission contract.

The [snapshot adapter](observer_recovery_snapshot.h) derives the hook and
publication components from `HookInstallState`,
`ObserverPublicationController` and an injected copy of the publication
record. Supply the expected publication binding independently, the record's
read result and the transaction disposition. The adapter performs no read,
write or cleanup operation and does not create an identity or hold proof.

The hook component copies the transaction state, uncertainty/protection flags
and lease/pin flags. Its code-bearing summary includes every entry with held
storage, CFG registration, a published original target or a visible or
possibly visible redirect. `Retained` alone does not establish that code remains.
The publication component copies ownership, aggregate/clear state and
uncertainty, then checks the copied record's header, binding, owner, generation
and targets against the expected binding, controller state and live hook
publication state. A fresh controller may inspect an empty record with
preserved generation history before its first publication. Cleared historical
targets do not stand for live code after transaction resources are released.
A local controller flag cannot replace the record check. Missing or refused
record input cannot produce a complete publication component or authorize
cleanup.

The caller must copy all inputs within its serialization domain and supply
the remaining `RecoveryStateSnapshot` fields separately, including its
generation, synchronization, creation/connection state and current hold or
typed exit-event evidence. The adapter does not establish continuous native
exclusion, remote read coherence or creation-owned identity. It preserves
existing uncertainty flags without creating a mutation-uncertainty latch for
a read-only refusal. The coordinator retains observed safety failures across
later observations.

`update_observation` supplies a synchronized current copy. The original copy
and classification remain historical evidence. Each action callback must
revalidate its live transaction entries, publication record and required hold
before reporting a postcondition. Operation IDs distinguish unresolved and
late completions; a deadline does not discard their identities.

Observation generations must advance. A later copy cannot clear an observed
uncertainty, protection or binding failure. Passing proofs within the declared
limit can enable the single known-state restoration attempt; an expired
operation remains a separate unresolved operation.

The [dispatch gate](observer-dispatch-gate.md) supplies offline ordering for
individual injected mutations inside composite transaction/publication calls.
The native caller must preserve that granularity and share the attempt's abort
authority. An existing admitted operation cannot be revoked or interrupted;
its actual result remains separate from the attempt's failed disposition.

## Cleanup decisions

Both uncertainty latches, unverified protection, changed binding and loss of a
required hold take precedence over resource flags or a successful transaction
report. Recovery never clears those latches or adopts unexpected bytes.

| State | Decision |
|---|---|
| Confirmed creation failure with no observer instance or connection | Close only owned local preparation resources. |
| Known state with no code-bearing resource | Complete owned lease/pin cleanup and checked empty publication-owner release. |
| Known installed or retained state with a verified hold and all five proofs | Attempt restoration once for that cancellation. |
| Refused proof with a verified hold | Preserve the hold and failure evidence until the declared limit, then abort. |
| Unknown effects, unverified protection, changed binding or lost required hold | Refuse further transaction cleanup and select observer abort. |

The five [quiescence attestations](observer-diagnostic.md#quiescence-and-resident-module-contract)
remain required for code-bearing restoration. A local lease flag cannot prove
continued execution exclusion. The transaction's result does not establish
that publication ownership or fixture cleanup completed. Observer callbacks
for coordinated pin/owner release belong outside the held mutation phase,
after reachable code has been removed.

## Abort and exit

For a responsive owner with a verified ordinary event, abort selects owner
exit with kill-on-exit established. It does not select detach or ordinary
continuation. [DebugSetProcessKillOnExit](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-debugsetprocesskillonexit)
requires a debug connection and affects that thread's current and future
debuggees. A native owner must therefore connect only to the isolated observer.

An unresponsive or dead owner permits at most one termination request through
the retained observer handle. [TerminateProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess)
is asynchronous for another process. A successful request, timeout or failed
wait cannot stand for completed shutdown.

A delivered `EXIT_PROCESS_DEBUG_EVENT` requires a separate transition:
the creator records a separate binding for that later event and acknowledges
it before the final wait on its independently retained process handle. An
initial snapshot that already reports exit delivery must supply the explicit
typed `delivered_exit_identity`; a generic expected event cannot supply it. The
newer exit observation invalidates ordinary-hold authority even if its exit
identity is refused. Unbound delivery remains unresolved and blocks the final
wait. The attempt fails if acknowledgment cannot complete within the declared
limit. A correctly bound, explicitly delivered later exit observation may
resolve the unbound delivery. Each acknowledgment operation retains its
dispatched event binding; a completion for a superseded event remains
historical evidence. The
[exit-event contract](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events)
requires acknowledgment before kernel shutdown can finish.
[Continuation](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-continuedebugevent)
belongs to the creator thread, and acknowledgment closes system-managed event
handles. A supervisor cannot take over continuation.

Only a signaled retained process handle confirms observer exit. The
[wait result](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject)
and exit-code collection are distinct: a missing exit code leaves that field
unknown without revoking independently confirmed death.
Late signaling evidence also confirms death while preserving the attempt's
failed disposition and its unresolved operations.

Abort remains failed after confirmed observer death. Restoration, publication
ownership release, recorder coverage and fixture cleanup retain their own
results. Normal completion requires those declared cleanup checks and orderly
observer exit. Cancellation success records completed resource cleanup;
normal completion and exit retain their separate evidence. Failed ledger
writes preserve incomplete evidence and cannot block abort indefinitely.
If cancellation already has a delivered exit event, creator acknowledgment
and retained-handle exit confirmation precede terminal cancellation success.
Cancellation without exit delivery does not require observer shutdown.

## Verification boundary

Fake checks exercise identity refusal, uncertainty precedence, cleanup
eligibility, permanent abort, action/completion ordering, event acknowledgment
and separate termination/exit outcomes. They preserve the existing forwarding,
bridge, transaction and publication suites.

These checks establish model decisions and injected-result handling. Native
process creation, hold evidence, dispatch synchronization, debug-thread exit,
supervision, remote mutation and the separate fixture failure contract still
require qualification. Observer death alone supplies no fixture cleanup
evidence.
