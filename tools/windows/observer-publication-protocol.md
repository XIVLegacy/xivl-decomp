# Observer publication protocol

[The protocol API](observer_publication_protocol.h) connects the transaction's
per-entry publication callbacks to one aggregate forwarding record through an
injected byte transport. `observer_runtime_check --self-test` exercises it with
CPU fakes. No native transport or controller factory is provided.

## Record and ownership

[`ObserverPublicationRecordV1`](observer_diagnostic.h) is the canonical shared
state. Its fixed-width fields contain the version, bound/published flags,
observer and module identities, controller owner, profile and executable
digest, wrapper addresses, original targets, publication generation and
forwarding counters.
Compile-time assertions fix its size, alignment and offsets. The record
contains no mutex, function pointer object or `std::atomic` object. Target
bridges use local `std::atomic_ref` access to its targets, flags and counters.
Controller addresses and targets are 32-bit x86 values. Recursive-target
checks use the bound observer wrapper addresses.

Before the held mutation phase, the observer establishes its binding and
claims exclusive controller ownership under the local publication mutex.
Ordinary local publication and clearing then refuse mutation, including a
call that was waiting for that mutex when ownership was claimed. Binding
requires quiescence; it refuses active calls, published targets and incompatible
partial records. Rebinding a released record preserves its generation and
unlogged count. Initialization and ownership callbacks execute outside the
controller's hold.

The controller's transport supplies reads, writes, hold evidence and ownership
callbacks. A real transport must establish the observer mapping, record
address, retained module and address-space identity. Supplying matching
numbers or a file digest establishes none of those native facts. Calls and
state inspection for a controller must be externally serialized.

## Held publication

Every record read and field mutation requires matching observer, owner-thread,
event, session, lease, controller-owner and module-pin identities. The event
must remain outstanding and the lease held. All five
[quiescence attestations](observer-diagnostic.md#quiescence-and-resident-module-contract)
must pass, including exclusion before new threads execute user-mode code.
Both the hold evidence and record must report zero active forwarding calls.

Publication follows the fixed Lookup, Query, ContextWrite order. Each callback
writes its original target while the aggregate published flag remains clear.
Before each field write, the controller compares the whole current record
with the expected preceding record. It confirms the written value by reading
the record again. The third callback confirms all targets, preserves any prior
unlogged count in controller state, resets that count, advances the generation
and sets the published flag last. A generation cannot wrap. The installation
transaction publishes all originals before any entry redirect becomes visible.

An earlier successful read is insufficient for a later write. Hold evidence,
binding, expected bytes and active-call count are checked again at each step.
An exception from a transport callback is classified as ambiguous.

The [dispatch gate](observer-dispatch-gate.md) orders each injected field write
and ownership operation against the shared attempt's abort decision. An
aggregate publication callback does not supply admission for all its writes.

## Clearing and retention

The clear callback receives the live `HookInstallState` before its per-entry
publication flag is cleared. It refuses while any entry has a visible or
possibly visible redirect, and verifies the entry, trampoline, generation
and expected target set. Once no redirect can remain reachable, it clears
aggregate publication before zeroing the targets. Subsequent per-entry clears
confirm that aggregate clearing completed. Trampolines remain owned until
publication clearing has succeeded.

Counter history belongs to a publication generation. Clearing leaves the
generation and unlogged count available for inspection. The next publication
records the prior count before confirming its reset. Ownership release is a
separate operation after successful clearing and outside the held mutation
phase. A known empty cancellation before the first publication may also release
ownership. The target-local release callback confirms the owner, unpublished
state, zero targets and zero active calls under the local mutex.

A write with an uncertain outcome, unconfirmed post-write readback or failure
after an earlier confirmed mutation in the same callback latches
`unknown_side_effects`. An ambiguous ownership claim also latches uncertainty.
Further publication, clearing and ownership release refuse recovery of that
controller. The enclosing transaction conservatively classifies every failed
mutating publication or clear callback as unknown and retains its code-bearing
resources and lease. No reconciliation API or automatic cleanup is provided.

## Verification boundary

The fake tests cover binding refusal, event and owner matching, all five
attestations, active calls, ordered publication, generation and counter
handling, expected-old comparison, write/readback failures, ownership and
aggregate transaction cleanup. Protocol tests inspect the canonical local
record through snapshots; diagnostic tests exercise forwarding bridges that
use the same record.

These checks establish byte values and callback ordering. A native design
still needs continuous Windows hold evidence, resident-module binding,
context exclusions, remote write visibility and ordering relative to
target-local atomic accesses, retained storage and coordinated release.
The protocol supplies no native qualification or execution command. The
[native mechanism constraints](observer-diagnostic.md#documented-native-mechanisms)
and transaction's retained-state rules continue to apply.
