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

The child loads the absolute DbgEng path with
`LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32`, restricting
the directory search for that load's dependencies to the engine directory and
system directory under the
[LoadLibraryExW contract](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw).
This does not enforce a supporting-file allowlist or control executable startup
imports, already loaded modules, API-set or side-by-side selection, or later
delay/dynamic loads. Those bindings require separate evidence and review before
activation. A supporting-file mismatch or unknown binding cannot qualify an
observation; changing directory flags supplies no resident identity proof.

## Supporting-file identity

The controller requires an absolute `--supporting-file-set` path, its exact
`--supporting-file-set-size` and `--supporting-file-set-sha256`. The set pins
the current observer executable, the selected engine and supporting file
candidates by lowercase module name, byte size and SHA-256. Its format is
ASCII with LF line endings and a final newline:

```text
schema=xivl-observer-supporting-file-set-v1
profile=query-output-identity-v1
count=N
lowercase_module_name|decimal_size|64_lowercase_hex_sha256
```

Entries must be sorted and unique. The byte pin covers the exact file,
including its headers, decimal spelling and line endings. Missing, changed,
unlisted or unreadable identities cannot be added from an observed load.
The file set and its command-line pin supply no native authority.

For a newly created observer, delivered process-creation and DLL-load events
supply the process, event sequence, module base and actual debug-event file
handle. The controller retains each file identity decision outside the
observer before ordinary continuation. Native qualification also requires a
complete held inventory of the supported x86 image mappings, reconciled with
those accepted load identities before code-bearing installation and fixture
creation. Unknown mappings, failed or truncated acquisition, stale membership
and failed persistence must refuse progress. Unload/reload changes must not
reuse an old accepted base or readiness result.

A refusal stops diagnostic admission as incomplete, retains the failed event
and uses the existing permanent-abort recovery path. An exit event has its
separate acknowledgment path. File identity acceptance does not establish
resident byte coherence, active API-set or side-by-side selection, dynamic
closure, or coverage outside the supported x86 image inventory. Those native
facts require observed evidence; file pins and CPU checks cannot supply it.

## Source composition

| Component | Source | Responsibility |
|---|---|---|
| Entry and input contract | [observer_candidate.cpp](observer_candidate.cpp) | Parse the explicit request and apply the current authority gate. |
| External controller and transport | [observer_live_runtime.cpp](observer_live_runtime.cpp) | Own observer creation, debug-event holds, supervisor deadlines, resident inspection, remote installation and cleanup. |
| Supporting-file checks | [observer_supporting_identity.cpp](observer_supporting_identity.cpp) | Validate the explicit file set, reconcile held module membership and gate persisted identity decisions before continuation. |
| Observer child | [observer_live_bootstrap.cpp](observer_live_bootstrap.cpp) | Publish target-local control records and connect the retained child session to DbgEng and the ordinary fixture. |
| Fixture evidence retention | [observer_fixture_evidence.cpp](observer_fixture_evidence.cpp) | Retain creation-bound handles, raw event and continuation receipts, separate stdout/stderr/state channels, release results and post-detach exit evidence. |
| Collection boundary | [observer_collection.cpp](observer_collection.cpp) | Close Recorder admission at one immutable monotonic deadline, independently of child completion and cleanup. |
| Production callback owner | [trace_map_observer_callbacks.cpp](trace_map_observer_callbacks.cpp) | Connect raw admission, Events, retained lifecycle acquisition and the shared Recorder. Active detach keeps the raw recorder and cached originals resident until the final release boundary. |
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
execution exclusion or fixture survival. Native activation requires separate
owner authorization and a reviewed run contract. An identity-gathering exception
permits collection of unresolved identities; it cannot mark them qualified.

## Observer exception profile

The `fail-stop-unclassified-v1` profile refuses every held observer
`EXCEPTION_DEBUG_EVENT` before `ContinueDebugEvent`. The debug owner uses its
authoritative pending event after checking creation identity and continuation
authority. It retains the event, leaves the continuation-dispatched result false
and requests permanent observer abort whether refusal evidence is written or
the bounded write fails. The controller records the created process ID,
controller and event thread IDs, session, raw generation/event index,
exception code, flags, first chance and address. Failed evidence storage also
marks the ledger incomplete.

This profile can reject a first-chance C++ exception that the unhooked program
would handle internally. It supplies fail-stop behavior, rather than exception
transparency through the inserted forwarding frames. Recorder's C++ restoration,
the forwarding counters and the `/EHsc` model retain their separate contracts;
see the [forwarding exception boundary](observer-diagnostic.md#forwarding-exception-boundary).

There is no startup-breakpoint allowance. Such an allowance requires separate
proof of the exception's origin, module, creation-bound thread, pre-hook phase
and single use. Code, first chance or event order alone cannot classify it.
The [startup classification](observer-startup-exception.md) distinguishes the
retained x86 loader and remote-break candidates and the required
original-child-thread, resident-module, producer, phase and single-use joins.
The default descriptor continues to refuse native activation. A source change
requires a new reviewed source/build/product binding before selecting an
operating descriptor; an earlier executable pin does not identify this code.

## Conditional native qualification

The default source policy refuses both native entries. A separately reviewed
private policy header may be supplied as the explicit CMake build input
`XIVL_OBSERVER_POLICY_HEADER`. It supplies purpose, grant, profile, finite
limits and canonical absolute review-record/reservation locators. It contains
no credential and belongs to the pinned source snapshot. A command-line value
or external record cannot enable that policy.

`observer_activation_binding` separates `GatherQueryOutputIdentities`
permission from runtime identity qualification. Preparation conveys no native
permission. It compares the complete bounded ASCII run record, source/build
file associations, actual controller/observer/engine/fixture/supporting-set
bytes, exact command lines and argv, limits, fresh outputs and cleanup contract.
The record uses sorted unique `key=value` rows, LF endings and a final newline.
Unknown, duplicate, missing or changed fields refuse admission.

Immediately before the first controller worker dispatch, the factory reserves
the policy's one canonical file with `CREATE_NEW` and flushes the immutable
grant/attempt/session, request/record digests and controller creation identity.
Existing or partial markers and uncertain creation/write/flush results prevent
dispatch and automatic retry. Reservation intent and actual dispatch results
remain separate evidence. The capability permits one observer creation and
same-attempt cleanup under the existing resource and exclusion checks.

Under the exact outstanding `CREATE_PROCESS` event, the controller fills the
exported data-only `xivl_observer_activation_binding_v1` record and duplicates
only a read-only reservation handle and a controller identity/synchronization
handle into the created observer. It persists the complete provision readback
before continuation. The child independently validates the same record, file
pins, effective inputs and creation identities before loading DbgEng. Versioned
attempt/digest acknowledgments bind bootstrap, options readiness, holds and
fixture transfer. Direct child invocation supplies no such provision.

The `.activation-record`, `.activation-attempt` and `.activation-provision`
sidecars retain exact reviewed bytes, dispatch/creation outcomes and the
provision readback. Reservation reads use explicit file offsets. These source
checks and CPU receipts do not establish observed native effects.

The native command accepts the same profile and seven controller limits, plus
explicit `--collection-ticks` and `--child-completion-ticks`, absolute
observer, engine, fixture and supporting-file-set paths, explicit command
lines, four file sizes and SHA-256 digests, the full 40-hex source revision, and
a positive provenance
hash byte cap. The external build manifest binds that declared revision to the
executable bytes. The observer
executable digest is supplied externally after building; it is not embedded
as its own expected digest. Native limits use Windows milliseconds and reject
zero and `INFINITE`. Collection must be at most 3000 ms. Child completion is
separate from responsiveness, collection and known cleanup. CPU step limits
cannot qualify these deadlines.

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
  --supporting-file-set $supportingSet --supporting-file-set-size $supportingSetSize `
  --supporting-file-set-sha256 $supportingSetSha256 `
  --observer-size $observerSize --observer-sha256 $observerSha256 `
  --dbgeng-size 6097408 --dbgeng-sha256 d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631 `
  --fixture-size 17920 --fixture-sha256 254a9916e383fbaed00b433132250a4b5b1c8bd9f7243d0caf4169ae66019379 `
  --hold-ticks $hold --known-cleanup-ticks $cleanup `
  --responsiveness-ticks $responsiveness --owner-exit-ticks $ownerExit `
  --termination-ticks $termination --acknowledgement-ticks $acknowledgement `
  --exit-confirmation-ticks $exitConfirmation `
  --collection-ticks $collection --child-completion-ticks $childCompletion
```

The observer command line selects `--native-child` and supplies `--profile`,
`--dbgeng`, `--fixture`, `--fixture-command-line`, `--output`, `--session`,
`--row-cap`, `--provenance-hash-cap`, `--collection-ticks`,
`--child-completion-ticks` and `--cleanup-ticks`. Its output is the fresh native
trace path. Every limit is explicit, positive and finite. The collection deadline
cannot be refreshed by startup, overlapping originals or cleanup. A deadline
stop and evidence completeness are separate facts; unfinished admitted intervals
remain incomplete. Post-stop originals still forward, while diagnostic row
publication stays closed. Natural fixture exit and its separately known exit code
cannot extend collection or repair incomplete evidence.

The ordinary fixture command line is exactly the quoted pinned fixture
executable with no arguments. The controller first starts the isolated
observer and the child acquires `DEBUG_PROCESS_DETACH_ON_EXIT` on its retained
DbgEng client. It records the raw requested mask, both HRESULTs, every returned
bit and the observer/session readiness sequence in controller-owned storage.
The controller creates the unchanged fixture with `CREATE_SUSPENDED` only
after that receipt is externally validated. It retains the fixture process and
primary thread handles, records the process ID and creation time in a separate
`.fixture.state` stream, duplicates those creation-bound handles plus the
stdout, stderr and state channels into the same observer process, and requires
the target-local identity acknowledgment before attach. The observer never
reopens the fixture by PID.

The child calls `AttachProcess` with `DEBUG_ATTACH_DEFAULT`, records the
HRESULT and flags, resumes the transferred creation-bound primary thread, records the
raw `ResumeThread` prior suspend count (`DWORD(-1)` is failure), and waits for
the first available DbgEng event before treating attachment as complete. A
successful `AttachProcess` HRESULT alone is insufficient. Collection stops at
the immutable collection deadline. The controller starts one absolute cleanup
deadline before reading pre-release output from the retained channels, publishes
that same deadline and obtains a cleanup hold. While that hold is active it
restores the diagnostic hooks and records the terminal raw-tail boundary. The
raw slots, raw originals, callback state and retained module stay resident for
active detach. The child records the active-detach intent and
`DEBUG_END_ACTIVE_DETACH` result, then records the actual post-detach event,
binding and continuation tail. Before publishing `Detached`, the child copies
that persisted post-detach sequence into its shared control record. Each raw
event, binding and continuation republishes the live counts, last indices,
pending state, coverage and persistence witness. The controller reads that
live witness again under a second fresh typed hold and refuses before any raw
slot write when the acknowledgement is stale, changed, pending, incomplete or
unpersisted. Every raw gap, overflow, owner or evidence sink refusal,
exception, and publication failure also publishes a sticky invalid witness
through the unheld child route; later successful raw activity cannot heal that
witness. It joins the exact unchanged tail to the raw wrapper extents, the raw
forwarding counter and current held identities, persists the accepted
second-hold evidence and restoration boundary, then restores raw slots. It
sets the restoration latch and only then releases the child, callbacks and
retained resources. The
controller keeps polling the retained output channels and creation-bound
fixture handle within the same finite cleanup bound, records the final output
after release, requires a source-conditioned post-detach marker, and reads the
process exit code only from the retained creation handle. An `alive 0` line
alone is not a post-detach marker. Missing output, continuation, release,
marker, wait or code evidence remains unknown.

Native activation still requires a reviewed fixture failure contract covering
creation-bound external retention, actual event/continuation tail, session
release, post-release state and natural fixture exit after collection stops.
Source timing checks and file pins cannot satisfy that contract.
The external controller writes a
separate `.failure-ledger` sidecar at that path before activation and records
failures or completion there. Preserve both artifacts; a nonempty trace file
alone cannot establish an accepted observation.

Both native entries retain a hard authority refusal before native APIs. No
command-line flag grants authority. A run needs a reviewed policy revision,
an available attempt allowance, supplied finite bounds and qualified runtime
identities or an explicit exception permitting their collection. That exception
cannot substitute for observation, exclusion or fixture failure evidence.
The CPU receipt validator accepts only its offline
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

The fixture is a separate controller-created process. Its inherited handle
list is limited to stdin, stdout and stderr. After readiness, the observer
receives duplicates of the creation-bound fixture process/thread handles and
the three controller-owned evidence channels through the reviewed control
transfer. The controller retains the original fixture handles and evidence
storage independently of the observer. It keeps
them through observer loss or any unknown fixture outcome until a finite
owner-intervention poll establishes a separate result.

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
