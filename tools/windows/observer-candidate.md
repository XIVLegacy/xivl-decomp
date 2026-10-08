# Query output identity observer

An accepted `query-output-identity-v1` observation identifies the adjusted translation
interface returned for a uniquely bound new event, its vtable, setter target
at slot `+0x10`, and that target's resident module. It uses the three planned
x86 entry redirects described by the [diagnostic contract](observer-diagnostic.md#offline-entry-plan).
It preserves unrelated, failed, null and unread queries as raw evidence.

| Entry RVA | Collected value | Qualification boundary |
|---|---|---|
| `0x467F13` | Manager, service GUID, returned record and record `+0x10` | The return snapshot does not establish the later internal QueryInterface receiver. |
| `0x468B10` | Service GUID, IID, output slot, HRESULT, adjusted interface, vtable and setter target | Query-time mapping, module binding and a unique event interval are required. |
| `0x3D049D` | Handle, context pointer, flags, debug registers, EIP/EFLAGS and wrapper result | Wrapper forwarding does not establish every native/WOW64 context-write path. |

The selected service GUID is `AE987DC0-7D24-4C33-A5A6-312D96192C8E` and IID is
`BE5E232C-1D4B-4983-A520-383DA865DA1C`. The pinned DbgEng file is PE32/I386,
6,097,408 bytes, SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631`.
The [translation reference finding](breakpoint-context-path.md#translation-guid-reference-boundary)
owns the identifier and setter-slot evidence.

| Required evidence | Recorder rows and fields | Acceptance check |
|---|---|---|
| Raw event association | `pending_event`, `raw_debug_object`, `event_pid`, `event_tid`, `raw_generation`, `event_index` | The complete raw key stays fixed through admission, binding and closure. |
| Engine lifecycle association | `callback_entry`, `callback_acquisition`, `engine_binding`, `engine_generation` | Retained cache facts and selected SDK reads agree independently of raw PID/TID. |
| Selected record snapshot | `lookup`, `manager`, `service_guid`, `returned_record`, `record_service_address`, `record_service` | Read statuses and the actual original-call result remain explicit. |
| Adjusted QueryService result | `query`, `service_guid`, `iid`, `result`, `output_slot`, `returned_interface`, `vtable`, `slot_plus_10_target` | The selected ordinary query has readable nonzero values and the expected slot address. |
| Query-time module evidence | Query `provenance`, mapping geometry, resident identity, backing file identity, lifetime and coherence fields | The collector binds the same event, operation and returned values; path or file hash alone cannot qualify them. |
| Wrapper forwarding | `context_write`, handle/target identity, context registers, result and error fields | Arguments, ordinary result and error state survive the wrapper. |
| Ordering | Shared `sequence` and `exit_sequence`, provenance acquisition interval, binding and pending closure | Binding finishes before query entry; the query finishes before closure. |
| Collection and cleanup | Overflow/unlogged counters and separate installation, publication, recovery, fixture and observer outcomes | Missing or unknown evidence cannot be repaired by successful cleanup. |

## Input identity

The unchanged ordinary fixture control is PE32/I386, 17,920 bytes, SHA-256
`254a9916e383fbaed00b433132250a4b5b1c8bd9f7243d0caf4169ae66019379`.
Its [source](map_selection_fixture.cpp) has SHA-256
`37edc0b5ca1217c308e0ccd671b8ad58dc9d60713a8409cad33a42d9c2511946`.
The controller and observer executable identities, supporting module files,
source manifest and output paths must also be explicit. Record actual loaded
bases separately from file RVAs and file identity separately from resident
mapping evidence. A rebuilt fixture requires its own digest and cannot be
described as this same binary control.

Native operation is limited to a newly created isolated observer and its
separate ordinary fixture. There is no retail or arbitrary-PID profile.

## Source composition

| Component | Source | Responsibility |
|---|---|---|
| Entry and input contract | [observer_candidate.cpp](observer_candidate.cpp) | Parse the explicit request and apply the current authority gate. |
| External controller and transport | [observer_live_runtime.cpp](observer_live_runtime.cpp) | Own observer creation, debug-event holds, supervisor deadlines, resident inspection, remote installation and cleanup. |
| Observer child | [observer_live_bootstrap.cpp](observer_live_bootstrap.cpp) | Publish target-local control records and connect the retained child session to DbgEng and the ordinary fixture. |
| Production callback owner | [trace_map_observer_callbacks.cpp](trace_map_observer_callbacks.cpp) | Connect raw admission, Events, retained lifecycle acquisition and the shared Recorder. |
| CPU qualification | [observer_controller.cpp](observer_controller.cpp) and [observer_native_backend.cpp](observer_native_backend.cpp) | Exercise that callback and forwarding composition with injected authority, memory and results. |
| Receipt acceptance | [validate_observer_candidate.py](validate_observer_candidate.py) | Check the positive trace and persisted causal failure artifacts. |

Compiling these sources establishes buildability. CPU qualification checks the
injected graph. Native source review does not establish runtime effects.

## Offline qualification

Build the `observer_candidate` target with MSVC for Win32 in a fresh build
directory. Preserve the source revision, source manifest, compiler identity,
build log and executable SHA-256 with the qualification receipt. Native tool
products and fixture products are compile-only under the preparation policy.

The candidate's `--offline-qualify` mode uses CPU-injected memory, identities,
original calls, clocks and operation results. Supply an absolute fresh receipt
path, the declared profile, a positive session identity, a row cap and all seven
positive finite limits. The limits count injected steps in this mode; they
are not selected native deadlines. For supplied PowerShell variables:

```powershell
& $candidate --offline-qualify --output $receipt `
  --profile query-output-identity-v1 --session $session --row-cap $rowCap `
  --hold-ticks $hold --known-cleanup-ticks $cleanup `
  --responsiveness-ticks $responsiveness --owner-exit-ticks $ownerExit `
  --termination-ticks $termination --acknowledgement-ticks $acknowledgement `
  --exit-confirmation-ticks $exitConfirmation
python tools/windows/validate_observer_candidate.py --receipt $receipt
python tools/windows/test_validate_observer_candidate.py --receipt $receipt
```

The receipt embeds the Recorder trace. Its summary cannot replace the raw
association, production callback acquisition, engine binding, forwarding
intervals or query-time provenance. The validator checks those rows with the
diagnostic validators, matches their session and counts to the receipt, and
requires separate restoration, publication and cleanup outcomes. The tamper
checks reject changed query targets, missing binding, unlogged calls, altered
limits, contradictory summaries and native qualification claims.
The total trace row count must fit the supplied cap. All causal fault results
must be present, and the linked trace and failure artifacts must be readable
and consistent with the receipt. A persistence flag cannot substitute for an
output file.

Passing this command establishes the recorded CPU scenario. It does not
establish native callback delivery, SDK alias exclusivity, resident identity,
execution exclusion or fixture survival. The current capture allowance and
runtime identity prerequisites prohibit native activation.

## Conditional native qualification

The native command accepts the same profile and seven limits, plus absolute
observer, engine and fixture paths, explicit command lines, three file sizes
and SHA-256 digests, the full 40-hex source revision, and a positive provenance
hash byte cap. The external build manifest binds that declared revision to the
executable bytes. The observer
executable digest is supplied externally after building; it is not embedded
as its own expected digest. Native limits use Windows milliseconds and reject
zero and `INFINITE`. CPU step limits cannot qualify these deadlines.

The following is a proposed command shape for a separately authorized run.
Every variable must be recorded with the pinned source and executable
manifest. This preparation does not select native deadline values or execute
the command:

```powershell
& $candidate --native --profile query-output-identity-v1 `
  --source-revision $sourceRevision `
  --observer $observer --observer-command-line $observerCommandLine `
  --dbgeng $dbgeng --fixture $fixture --fixture-command-line $fixtureCommandLine `
  --output $nativeTrace --session $session --row-cap $rowCap `
  --provenance-hash-cap $hashCap `
  --observer-size $observerSize --observer-sha256 $observerSha256 `
  --dbgeng-size 6097408 --dbgeng-sha256 d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631 `
  --fixture-size 17920 --fixture-sha256 254a9916e383fbaed00b433132250a4b5b1c8bd9f7243d0caf4169ae66019379 `
  --hold-ticks $hold --known-cleanup-ticks $cleanup `
  --responsiveness-ticks $responsiveness --owner-exit-ticks $ownerExit `
  --termination-ticks $termination --acknowledgement-ticks $acknowledgement `
  --exit-confirmation-ticks $exitConfirmation
```

The observer command line selects `--native-child` and supplies `--profile`,
`--dbgeng`, `--fixture`, `--fixture-command-line`, `--output`, `--session`,
`--row-cap`, `--provenance-hash-cap` and a positive finite `--timeout-ticks`.
Its output is the fresh native trace path. The external controller writes a
separate `.failure-ledger` sidecar at that path before activation and records
failures or completion there. Preserve both artifacts; a nonempty trace file
alone cannot establish an accepted observation.

Both native entries retain a hard authority refusal before native APIs. No
command-line flag grants authority. A future run needs a reviewed policy
revision, revised capture allowance and qualified runtime identities, as well
as supplied finite bounds. The CPU receipt validator accepts only its offline
schema; a native result requires review of the raw trace, resident evidence,
controller ledger and separate cleanup outcomes against this contract.

## Evidence acceptance

An identity finding requires the exact copied GUID/IID, an ordinary successful
query, readable nonzero interface/vtable/slot values, and a demonstrated
executable image mapping for the target. The query's provenance must bind the
same returned values, event and operation. A familiar path or an on-disk digest
alone cannot establish the resident mapping.

The raw event identity and separately observed engine lifecycle token must
form a unique join. Binding must finish before query entry, and query exit must
precede continuation closure. A later token cannot repair an earlier query.
Grouped output order cannot replace the Recorder's shared sequence values.
Missing exits, refused reads, changed ownership, overflow, unlogged calls and
unconfirmed persistence make the observation incomplete.

Restoration, publication release, fixture cleanup and observer exit are
separate outcomes. Observer death cannot establish restored code or successful
fixture cleanup. A failed collection remains failed after successful cleanup.

This profile does not identify the actual internal QueryInterface receiver,
consumer, staging/commit, service-change flush, breakpoint selection or
pre-engine exception producer. It does not explain the retained
constructor/receiver mismatch by itself. The
[broader context path](breakpoint-context-path.md#concrete-missing-edge)
owns those remaining research boundaries.

## Controller requirements

The controller creates only its isolated observer with
`DEBUG_ONLY_THIS_PROCESS`. Its creator thread owns waiting and continuation.
An outstanding observer-process debug event holds that process's threads
until continuation, and a thread-creation event precedes the new thread's
user-mode execution. These are the documented
[Windows event boundaries](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events).
An event in the separate fixture session supplies no observer-process hold.

Every mutation and code-bearing cleanup requires all five
[quiescence attestations](observer-diagnostic.md#quiescence-and-resident-module-contract)
under one continuous observer hold. Held inspection and mutation use external
operations without executing observer callbacks or waiting on its locks.
Module retention and publication ownership handshakes run outside that hold.
Context exclusions cover entry interiors, wrappers and every owned trampoline.
The candidate places each bridge in a separate compiler-owned executable
section and uses in-section forwarding counters. An exception crossing a
bridge retains its active-call state and prevents code cleanup.

The supervisor retains independently owned creation handles. Permanent abort
prevents later success, targets only the created observer, and preserves
unknown transaction state. The creator acknowledges an exit event before
waiting for final process shutdown. Supplied positive finite limits govern
hold acquisition, known cleanup, responsiveness, owner exit, termination,
exit acknowledgment and exit confirmation; there are no run timeout defaults.
The [recovery contract](observer-recovery.md) owns classification and outcome
rules.

The native ledger records source and executable identities, the created
instance, owner/event/session/lease/module/publication identities, operation
intents and actual results, retained resources and uncertainty flags. A
termination request and confirmed process death are separate observations.
If the supplied observation limits expire with an unconfirmed abort, the CLI
reports unknown state and keeps the controller alive for owner intervention
using finite polling intervals. It cannot return through normal process exit
while that retained state is still required.
