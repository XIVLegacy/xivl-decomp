# Observer forwarding diagnostic

`observer_diagnostic_check` exercises three x86 call-through adapters using
fake originals and injected memory, error and handle-identity readers. It
exports synthetic service-selection and context-write rows with one entry/exit
sequence source. The executable has no attachment, DLL loading, hook
installation or retail mode. `observer_runtime_check` additionally exercises
the raw-event bridge, an injected installation transaction, the
[publication protocol](observer-publication-protocol.md) and the
[recovery model](observer-recovery.md), including its snapshot adapter, with
fake backends.
It also has no live execution mode.

The supported execution profile is `synthetic-forwarding-profile`. A checked
query output means the fake reader copied its returned interface, vtable and
slot `+0x10`; it does not bind a loaded implementation. Failed, null and unread
outputs remain unqualified. The serializer always reports live coverage as
`incomplete`.

## Build and asset-free checks

Use CMake 3.25 or newer, Visual Studio 2022 MSVC, Windows SDK and a fresh
absolute build directory outside the tracked tree:

```powershell
cmake -S tools/windows -B C:\scratch\observer-build -G "Visual Studio 17 2022" -A Win32
cmake --build C:\scratch\observer-build --config Release --target observer_diagnostic_check observer_runtime_check
C:\scratch\observer-build\Release\observer_diagnostic_check.exe --self-test
C:\scratch\observer-build\Release\observer_runtime_check.exe --self-test
C:\scratch\observer-build\Release\observer_runtime_check.exe --bridge-output C:\scratch\observer-bridge.json
C:\scratch\observer-build\Release\observer_diagnostic_check.exe --synthetic-output C:\scratch\observer-synthetic.json
python tools/windows/validate_observer_diagnostic.py --trace C:\scratch\observer-synthetic.json
python tools/windows/validate_observer_diagnostic.py --trace C:\scratch\observer-synthetic.json --strict-provenance
python tools/windows/test_validate_observer_diagnostic.py --trace C:\scratch\observer-synthetic.json
python tools/windows/test_observer_hook_plan.py
```

The output parent must exist. Existing destinations and relative paths are
refused. A failed write leaves its partial file for inspection. Synthetic
addresses, handles and event identities come from the test harness.

The C++ checks exercise Win32 arguments and stack balance, unchanged results
and LastError/LastStatus pairs, nested lookup/query identities, adjusted
interface pointers, real x86 CONTEXT field offsets, refused reads, exceptions,
overflow, reentry and concurrent forwarding. They do not run a native context API. The offline
validator rejects incomplete samples and malformed event joins. Validator
acceptance establishes this synthetic record contract only.

## Callback identity reader

`CallbackIdentityReader` in
[`observer_callback_identity.h`](observer_callback_identity.h) accepts a
borrowed SDK `IUnknown`, obtains `IDebugSystemObjects` with the Windows SDK
declaration, and records the actual `QueryInterface` result plus the six
selected and event ID getters. It initializes each output to its documented
unknown sentinel, preserves the HRESULT and output-known flag, and releases a
successful non-null interface reference exactly once. The configured error-pair
callbacks record and restore the injected LastError/LastStatus values; missing
or failed callbacks keep binding ineligible.

`Recorder::begin_callback`, `begin_callback_acquisition`,
`finish_callback_acquisition` and `end_callback` use the recorder sequence
source for callback entry, SDK acquisition and callback exit. The acquisition
row retains both raw-key snapshots, owner authority and lifetime evidence, the
cached raw lifecycle tuple (debug object, raw PID/TID and raw generation), all
SDK method results, and the explicit binding outcome. A callback entry always
starts with raw identity; it is not retrofitted with an engine generation.
`RawEventBridge::bind_engine_event` receives the linked callback and
acquisition IDs only after the raw key, owner witness, SDK reads and error-pair
restoration are eligible. A missing callback exit remains an incomplete row.
Preflight and owner refusals retain the acquisition caller's observer thread and
canonical not-attempted slots for all six SDK methods; an accepted acquisition
must retain the callback-entry observer thread.

The fake COM objects in `observer_callback_identity_tests.cpp` exercise this
same SDK reader path, including failed getters, refused `QueryInterface`,
cleanup exceptions, deferred binding, nested forwarding and continuation
closure. Build the Win32 target and emit its fresh synthetic callback record
with:

```powershell
cmake --build C:\scratch\observer-build --config Release --target observer_runtime_check
C:\scratch\observer-build\Release\observer_runtime_check.exe --self-test
C:\scratch\observer-build\Release\observer_runtime_check.exe --callback-output C:\scratch\observer-callback.json
python tools/windows/validate_observer_diagnostic.py --trace C:\scratch\observer-callback.json
```

This reader is compiled against the Windows SDK but does not load DbgEng, call
`DebugCreate`, attach to a process, or receive a native callback. The
`--callback-output` record is fake-only evidence and keeps `live_coverage` as
`incomplete`; native callback entry and lifecycle ownership still require a
separate integration.

## SDK callback dispatch wrapper

`ObserverCallbackDispatch` in
[`observer_callback_dispatch.h`](observer_callback_dispatch.h) is a concrete
`IDebugEventCallbacks` implementation. It retains a borrowed delegate and
borrowed SDK client and the existing recorder, reader and bridge, then forwards
every SDK method exactly once; `GetInterestMask` and all methods other than
`Breakpoint` and `CreateThread` are direct calls. The wrapper owns no delegate,
client, recorder, bridge, provider or SDK storage. Its COM count starts at one,
`QueryInterface` supports `IUnknown` and `IDebugEventCallbacks`, and `Release`
never deletes caller-owned storage. The caller must keep every configured object
alive and establish quiescence before destruction.

The wrapper calls `Recorder::begin_callback` before reading the callback owner,
SDK identity or callback arguments. `Breakpoint` obtains lifecycle-owner
evidence and performs the existing six-getter `CallbackIdentityReader` capture
before entering the delegate. `CreateThread` stamps the delegate interval first
and obtains owner evidence and capture after it returns, so owner allocation in
the delegate remains visible as a post-delegate operation. The recorder gives
the entry, delegate begin/end, acquisition, binding and exit rows one sequence
clock. The validator requires the whole interval to be enclosed by callback
entry/exit and checks the phase-specific acquisition order from those stamps.

The owner provider receives the callback kind, phase and exact raw identity from
callback entry. Its result is lifecycle-owner evidence and is never synthesized
from SDK numbers, addresses or a matching token. Missing, changed or ended raw
keys, missing owner evidence, provider exceptions, foreign-thread delivery,
reentry, invalid configuration and capacity refusal leave incomplete coverage
while the delegate still receives its original arguments once. Provider and
reader failures are retained separately from the delegate result. A completed
negative delegate `HRESULT` remains the returned result, and injected
LastError/LastStatus values are restored around instrumentation while the
delegate's returned pair is preserved. A failed prerequisite restore remains
sticky and prevents SDK reads and binding. If a restore fails after the reader
has produced a real acquisition and binding, those rows remain factual; the
typed late-failure evidence keeps the dispatch row incomplete.
With no raw bridge configured, an eligible reader can still record an Accepted
acquisition with a refused status and zero binding receipt; the dispatch row
stays incomplete and makes no binding claim.

`instrumentation_thread_id` is a caller-selected owner thread ID. Zero remains
unknown and refuses instrumentation without marking delivery as foreign; the
delegate still runs once and the refused reader attempt remains incomplete when
capacity permits.

Dispatch rows extend `CallbackEntryRow` with a typed marker, phase, delegate
identity, callback arguments, delegate completion/HRESULT and sequence stamps,
owner/acquisition/binding links, refusal state and error restoration evidence.
Legacy caller-invoked callback rows keep `dispatch_marker` false and remain
compatible with traces that omit the new fields. Strict validation accepts a
dispatch row only when its typed fields, shared-clock interval, exact
acquisition link and binding receipt agree; a forged complete success is
refused. `--callback-dispatch-output` emits a fresh fake delegate/client/
provider profile by invoking the wrapper's actual SDK methods. It performs no
registration or `SetEventCallbacks` call, engine creation, attach or live
coverage operation, and keeps `live_coverage` as `incomplete`.

## Query-bound provenance

`QueryRow` can carry evidence from an injected
`QueryProvenanceCollector`. The collector runs after a successful query has
returned and the interface, vtable and slot `+0x10` reads have completed. It
receives query return observations and records the exact session, operation,
event, returned interface, vtable and slot target. Its acquisition interval
uses the recorder's common sequence source and records the observed lifetime
and read coherence. The recorder assigns the authoritative exit stamp and
restores the returned error pair after collection, so the callback cannot
qualify the final row itself. `output_complete` means that the collector
completed its own evidence capture; it does not report row persistence or
writer success.

The evidence preserves mapping observations for the returned interface, vtable
and setter target. Interface and vtable mappings may remain explicitly unknown
when no mapping reader was used, or may be reported as allocations. A selected
target requires an executable image mapping, resident base and extent, path,
supported PE32/I386 architecture, backing file size and nonzero SHA-256, plus
explicit evidence binding that file to the resident image. The binding carries
an authority identifier and mechanism, and a lifetime identifier that ties the
binding to the resident/file tuple for the acquisition interval. A path or
matching hash without that binding is refused. Unread, changed, mismatched or
incomplete evidence stays in the query row and cannot qualify.

The fake provider uses the synthetic absolute path
`C:\synthetic\provider.dll` as injected evidence; it is never opened or
resolved by this component. The authority and lifetime fields describe that
injected witness only and do not establish native identity.

The strict validator accepts only the exact translation service GUID
`AE987DC0-7D24-4C33-A5A6-312D96192C8E` and IID
`BE5E232C-1D4B-4983-A520-383DA865DA1C`. Other queries may be retained as raw
observations, but they do not satisfy this selected profile. The collector is
optional, so existing forwarding remains available with unknown provenance
when no callback is installed. Callback refusal or exception leaves the row
incomplete and does not change the forwarded result or error pair.

`--strict-provenance` checks this injected evidence and still reports native
qualification as unsupported. Synthetic output always keeps
`live_coverage` set to `incomplete`; no native reader, engine load, process or
context query, live command or capture is provided by this profile.

## Adapter contract

The public ABI and static selection finding is
[the breakpoint context path](breakpoint-context-path.md).
[The header](observer_diagnostic.h) exposes `Recorder`, typed originals,
injected callbacks and thread-local `BridgeScope` binding. Each original must
remain valid through every active call. `publish_passthrough` publishes all
three originals under one generation. Calls without a matching thread-local
logging binding forward to those originals and increment `unlogged_calls`.
The validator refuses a trace with any such call. Without publication or a
binding, bridges return sentinel results.

Publication and removal require externally established thread quiescence.
`clear_passthrough` checks the generation, original targets and active-call
count, but that count cannot prevent another thread from entering. A backend
must publish all three targets before making any redirect reachable and must
restore every redirect before clearing the targets. Original lifetimes extend
through all calls. The passthrough counter belongs to its publication
generation and remains visible after removal; use a new publication or process
for a new observation interval.

Readers must refuse inaccessible ranges and must not fault. C++ callback
exceptions become refused reads. `/EHsc` supplies no SEH recovery. Error-pair
read/write callbacks must themselves preserve the pair they observe or restore.
Missing or failed error callbacks make preservation unproved. LastStatus is
serialized as signed 32-bit NTSTATUS bits. Caller-owned quiescence is required
before snapshots or serialization; no original executes
under the recorder lock.

Complete traces require admission to finish before forwarding begins and all
related calls to finish before closure begins. Forwarding may run concurrently
within that fixed event interval. Raced transition rows remain available for
inspection; the validator refuses operations crossing admission or closure.

| Adapter | Native argument shape | Observation |
|---|---|---|
| Lookup | ECX manager, one stack GUID, `ret 4`; bridge ignores EDX | Returned record and checked record `+0x10` |
| QueryService | stdcall manager, service GUID, IID, output slot; `ret 16` | HRESULT, exact output-slot pointer, adjusted interface, vtable and slot `+0x10` |
| Context write | fastcall handle in ECX, native CONTEXT pointer in EDX | Input flags, EIP/EFLAGS, DR0-DR3/DR6/DR7, fake target identity and result |

Pending event admission is explicit. Raw debug object, Windows PID/TID,
raw generation and the zero-based event index remain distinct from the engine
generation.
Duplicate, changed, unknown or missing identities produce incomplete records.
Lookup/query joins use actual nested operation IDs, manager, GUID and event
identity. Serialization preserves attempted pending identities separately from
the active event snapshot.

## Offline entry plan

`observer_hook_plan.py` reads bytes without loading them. It admits only PE32
I386 DbgEng of size `6097408` and SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631`.
It checks executable section membership and HIGHLOW relocation coverage for
these complete instruction spans:

| Boundary RVA | File bytes | Copied span | Relocation |
|---|---|---|---|
| `0x467F13` | `8b ff 55 8b ec` | 5 | None |
| `0x468B10` | `6a 04 b8 ef 5e 4e 10` | 7 | HIGHLOW at span `+3` |
| `0x3D049D` | `8b ff 56 57 8b f9` | 6 | None |

Static locators and ABI come from the linked disassembly finding. The file
profile check directly verifies the spans and relocations. Five bytes would
split the last MOV at the context-write entry. The QueryService immediate
must be rebased before copying; its later relative call remains at the
original resume address.

```powershell
python tools/windows/observer_hook_plan.py --engine C:\Windows\SysWOW64\dbgeng.dll --output C:\scratch\observer-entry-profile.json
```

The command writes a fresh `pinned-x86-entry-plan-v1` receipt. An unsupported
digest, architecture, section span or relocation refuses the profile.
`plan_redirect(name, loaded_base, image_size, wrapper, wrapper_size, trampoline)`
is an importable byte planner: caller-assigned addresses and full wrapper
extent must form nonzero, nonwrapping, disjoint x86 ranges. It computes an
E9 redirect padded to the copied span, relocated original bytes and an E9
return to the original resume address. It neither allocates nor validates
resident mappings and never writes executable memory. Receipts state
`installed: false` and `live_coverage: false`.

## Injected installation transaction

[The transaction API](observer_hook_install.h) admits the same fixed three
entries and pinned file profile as the offline planner. Its injected backend
owns native module retention, resident-image binding, relocation evidence,
range inspection, protection, CFG, publication and thread exclusion. The
transaction checks their reported values and operation ordering. No Windows
backend factory is provided.

### Quiescence and resident-module contract

A lease must exclude entry throughout each mutation and code-bearing cleanup
step. Its revalidation callback returns `HookQuiescenceAttestation` and must
attest all five conditions:

1. No call through any of the three forwarding adapters is active.
2. Every other thread in the observer process is held against execution.
3. Every new thread in that process is prevented from executing user-mode code
   while the lease is held.
4. No held instruction context is inside an overwritten entry span, excluding
   its first byte and original resume address.
5. No instruction context is in a wrapper or trampoline that the transaction
   may publish, overwrite or release.

The mutating thread must remain outside the engine and these call-through
paths, including during backend callbacks. These are backend attestations,
not independently established thread facts. The transaction does not enumerate
threads, inspect their contexts or implement new-thread execution exclusion.
An idle-call count or a thread snapshot cannot supply that exclusion contract.

Microsoft's API contracts supply constraints for a native implementation:

- A [Tool Help snapshot](https://learn.microsoft.com/en-us/windows/win32/toolhelp/snapshots-of-the-system)
  copies current lists. It does not describe a barrier against later thread
  creation.
- [SuspendThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread)
  can suspend a thread that owns a lock needed by the caller. Its documented
  deadlock risk prevents assuming that an arbitrary freeze is a usable lease.
- [GetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext)
  requires a suspended thread for a valid context and does not return a valid
  context for the calling thread.

A native implementation must establish a continuous hold, new-thread execution
exclusion and safe mutator placement.

The module handle value must equal the reported loaded base. The transaction
checks the pinned architecture, file size/digest, preferred base, image extent
and relocation descriptions returned by the backend. It independently reads
and compares only the three copied entry spans, totaling 18 resident bytes.
The backend must still establish that the retained mapping belongs to that
file and stays valid for every reachable original, wrapper and trampoline.
A file digest, reported path or equal handle/base alone does not establish
resident-image identity.

Successful installation releases the lease. Restoration acquires a new lease
when code-bearing state remains and no lease is held. A native implementation
must establish all five conditions for both phases, rather than assuming that
an installation lease supplies restoration coverage.

#### Documented native mechanisms

The `new_threads_prevented_from_executing` attestation requires continuous
exclusion before a new observer thread executes user-mode code. Thread creation
may proceed. The backend must establish exclusion for the whole held lease.

| Mechanism | Documented boundary | Transaction limitation |
|---|---|---|
| Cooperative entry gate | Excludes callers that obey the gate. | It does not establish that every process thread is held or that new threads cannot execute unrelated code. |
| Thread snapshot followed by suspension | Enumerates a copied thread list and suspends the selected threads. | It does not provide continuous exclusion of newly created threads; held threads may own locks needed by a mutating callback. |
| External debugger with an outstanding observer-process event | The system holds all threads in the affected process until the event is continued. | This requires a controller outside the held process and does not establish the other attestations or backend operations. |

Microsoft's [debugging-event contract](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events)
places `CREATE_THREAD_DEBUG_EVENT` after creation and before user-mode execution.
Its [debugger loop](https://learn.microsoft.com/en-us/windows/win32/debug/writing-the-debugger-s-main-loop)
allows a separate debugger to inspect and change the stopped process. These
contracts support investigating an external controller with continuous
exclusion before thread execution. The backend still needs evidence for each
attestation and the native mutation, publication and cleanup operations.

An outstanding event in the fixture stops the fixture, not the observer that
hosts DbgEng and these adapters. An external controller would need to hold an
event for the observer process itself. Its debug session must be scoped to that
observer. The [process-creation flags](https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags)
provide `DEBUG_ONLY_THIS_PROCESS` for that scope. A controller that follows
descendant processes can acquire fixture events intended for DbgEng. Running
a publication callback in a held observer would require releasing the hold
and establishing exclusion again.

The `publish_passthrough`, `clear_passthrough` and `passthrough_snapshot`
functions execute locally and take a mutex. The injected
[publication protocol](observer-publication-protocol.md) instead operates on a
versioned shared record and integrates per-entry callbacks with aggregate
forwarding state. Its ownership handshake runs before the hold. A native
transport must still establish remote write ordering, address and lifetime
bindings, and access without executing held observer code or waiting for its
locks.

The [GetModuleHandleEx contract](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexa)
acquires a reference in the calling process. A controller's module or file
handle cannot substitute for retention of the observer's mapping. The
observer's reference acquisition and release must fit the same native
lifetime design.

For an external debugger, keeping an event outstanding is a process hold,
not a complete recovery policy. [DebugSetProcessKillOnExit](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-debugsetprocesskillonexit)
defaults to terminating connected debuggees when the controller thread exits;
the alternate setting detaches them. Neither behavior confirms restoration
or preserves a held lease for an unknown transaction state. A native design
must select controller lifetime, failure disposition and recovery before it
can promise safe lease release.

### Mutation, repair and retained state

The [dispatch gate](observer-dispatch-gate.md) orders individual injected
mutation and cleanup operations against permanent abort. It supplements the
required lease and ownership checks.

The transaction prepares relocated trampolines, verifies their bytes, changes
them from writable to executable, flushes instruction caches, registers CFG
targets and publishes originals before redirects become visible. It checks
nonwrapping x86 ranges and full wrapper/trampoline overlap. It compares current
site bytes with the expected resident or redirect bytes immediately before
each redirect or restore write, in addition to preparation and restore
preflight. Earlier accepted bytes do not establish later ownership.

After a known writable protection transition, a failure inside that window
gets one attempt to restore the recorded protection and confirm it through
range inspection. This repair also runs when revalidation refuses inside the
window. It does not write more bytes, complete a failed cache flush or prove
that the code is safe to resume. An already attempted protection restore is
not retried. `protection_unverified` records an unconfirmed repair.

A refused revalidation before a step's first mutation leaves known state
restorable. Refusal after a writable transition, uncertain writes, changed
ownership and failed mutating callbacks retain unknown side effects.
Backend exceptions are ambiguous results, not evidence that a mutation did
not occur. Both operation reports expose `unknown_side_effects`; a false
value is a transaction classification, not a native safety attestation.
Unknown side effects block subsequent cleanup. No reconciliation API is
provided.

When no reachable redirect or code-bearing resource remains, known lease and
module retention can be released without another passing attestation poll.
Otherwise, release and cleanup still require the lease conditions. A retained
freeze after a latched failure has no automatic release policy. There is no
destructor cleanup or automatic retry. The installation history survives a
refused restoration attempt so a later successful restoration reports
`Restored`. `restored_entries` counts completed redirect restorations in that
attempt, excluding entries already containing original bytes at preflight.

Fake-backend tests establish transaction ordering and failure handling only.
Calls and state inspection for one transaction must be externally serialized.
The transient preparation state rejects callback reentry into installation
or restoration; it does not synchronize competing threads.

### Edges not established

All engine RVAs below refer to the pinned image in the offline entry plan.
The locators identify required boundaries; they do not establish a native
lease or qualify live installation.

| Required edge | Static locator or absence | Missing evidence |
|---|---|---|
| Exclude other threads and newly created entrants | No hold or new-thread execution-exclusion implementation is established. Entry RVAs are `0x467F13`, `0x468B10` and `0x3D049D`. | A continuous hold and exclusion before new threads execute user-mode code, covering the observer process, with a mutator that can finish while other threads are held. |
| Exclude instruction contexts from overwritten interiors | Copied spans are 5, 7 and 6 bytes at those entry RVAs. | Native context checks under the same hold, including the mutator's placement. |
| Exclude wrapper and trampoline contexts | Wrapper extents are request values; trampoline addresses are backend allocation results. No fixed engine locator exists. | Demonstrated context exclusion and lifetime through publication, forwarding, restoration and release. |
| Bind the retained resident image to the pinned file | The three copied spans and QueryService HIGHLOW relocation at span `+3`; handle/base equality is enforced. | A retained native mapping and reproducible file-to-resident binding beyond the 18 compared bytes. |
| Own protection, cache and CFG transitions | Redirect and trampoline ranges come from the entry plan; no native callback implementation exists. | Native ownership, transition receipts, cache effects and valid indirect-call targets. |
| Preserve copied-entry exception and unwind behavior | QueryService begins at `0x468B10`; copied spans and resume addresses are in the entry plan. | A supported exception/unwind contract for relocated prologues and the actual wrappers. |
| Recover after refusal or a latched failure | The [offline recovery model](observer-recovery.md) supplies injected failure decisions; no native recovery backend exists. | Native hold, retained-handle ownership, dispatch synchronization and independently confirmed shutdown under the same recovery policy. |

## Raw-event bridge and delayed binding

[The bridge API](observer_event_bridge.h) attaches an optional sink to
`RawRecorder` while inactive and quiescent. Supported events are offered after
raw decoding, lifecycle handling and context snapshot capture, before wait
returns. Continuation observations include the actual result before return.
Failed continuation retains the pending identity; successful continuation
closes only a uniquely matched event. Wait/continue originals, arguments,
results and LastError/LastStatus are preserved.

The default synchronous mode asks an injected engine-generation provider for
the exact debug object, PID/TID, raw generation and raw index before the wait
wrapper returns. `engine_generation` is an observer-assigned engine thread
lifecycle token; it is not a counter read from DbgEng. The
[engine event binding finding](engine-event-binding.md) owns its source
semantics and native event-selection timing.

Explicit deferred mode admits a complete raw tuple with an unknown engine
token and records it as awaiting binding without adding an unknown-generation
gap. A later caller supplies the exact raw tuple and an injected
`EngineIdentityObservation`: current, event and cached thread engine IDs must
be known, equal and different from `DEBUG_ANY_ID`; current and event process
engine IDs must be known and equal; current system PID/TID must equal the raw
PID/TID; and a separate nonzero lifecycle token must be supplied. These checks
validate witness consistency only and do not qualify a native identity.

The later caller obtains the admitted tuple from the Recorder's
`pending_event()` snapshot and supplies it to `RawEventBridge::bind_engine_event`;
the bridge's event and receipt accessors are quiescent snapshot readers.
`RawEventBridge::bind_engine_event` records every bounded attempt in an
immutable receipt and asks `Recorder::bind_pending_event` to upgrade only the
exact still-pending raw tuple. Missing, changed, stale, already-bound,
conflicting, duplicate, incomplete and overflow attempts remain refusal
evidence. The raw admission row and event evidence are never rewritten. A
query that entered before binding remains unqualified even when its original
returns after binding; only a query whose complete interval starts after the
binding row can qualify. A query crossing the binding transition is
incomplete. Failed continuation retains its identity, while successful
continuation closes the exact pending interval and rejects later binding. The
exact event index joins one pending tuple, while the lifecycle key (debug
object, PID/TID and raw generation) keeps one observer token consistent across
successive event indices. A changed token within that lifecycle, token reuse
by another lifecycle, and exact closed-tuple replay are refused. The bridge's
`coverage()` is raw-event bridge coverage; it latches false after any refused
binding and does not claim that the Recorder's forwarding rows are globally
complete. The synthetic output keeps `live_coverage` incomplete, including
when early query rows remain raw-only.

Raw notifications, delayed binding and continuation require explicit external
serialization. Query calls use the Recorder's existing interval and
concurrency contract. Snapshot readers require quiescence. The bridge,
recorder, callbacks and callback data must outlive all notifications; detach
the sink only after deactivation and quiescence. Unresolved or refused
bindings keep coverage incomplete, and the bridge never derives a token from
raw generation, addresses or neighboring rows.

`--bridge-output` emits a fresh synthetic receipt from fake wait, delayed
binding, context and continuation calls. Its raw bytes and event identities
belong to that harness. The binding receipts are injected witness checks and
do not qualify native engine identities or complete native write coverage.

## Remaining runtime requirements

There is no live backend or capture command for these adapters. Native entry
redirect installation still needs demonstrated thread quiescence, resident
module/digest binding, relocation verification, CFG-valid trampoline calls,
owned protection transitions, lifetime and restoration. A copied entry's
exception/unwind behavior also needs review before use in the engine.

Runtime coverage additionally requires demonstrated engine-generation binding,
selected owner/backend/inner and execution-unit identities, module bindings
for returned interfaces and resolved context API destinations, ordered buffer
staging/commit, service-change flush, slot-vector selection and native callback
entry. Native/WOW64 bypasses and a pre-engine exception-producer discriminator
remain unproved. The three adapters alone cannot explain the retained mismatch
or establish complete context-write coverage.

Any future bounded fixture observation must specify its unchanged control,
source/binary identities, exact command, output members and cleanup checks.
It remains separate from retail SetMap selection, distinct authored resources,
rendering and walking acceptance.
