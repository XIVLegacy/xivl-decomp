# Observer forwarding diagnostic

`observer_diagnostic_check` exercises three x86 call-through adapters using
fake originals and injected memory, error and handle-identity readers. It
exports synthetic service-selection and context-write rows with one entry/exit
sequence source. The executable has no attachment, DLL loading, hook
installation or retail mode. `observer_runtime_check` additionally exercises
the raw-event bridge and an injected installation transaction with fake
backends. It also has no live execution mode.

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
entries and pinned file profile as the offline planner. An injected backend
reports module identity, relocation evidence, range ownership and protection,
and attests thread quiescence. The transaction itself requires the module
handle to equal the reported base, compares the resident span bytes and checks
the arithmetic of every range. Everything else is a backend report, as the
[contract](#quiescence-and-module-binding-contract) states.

Trampoline storage must already be executable and not writable. The
transaction writes each trampoline and redirect through a temporary Read|Write
window, verifies the bytes, flushes instruction caches, restores the earlier
protection and inspects it again. It registers CFG targets and publishes
originals before any redirect becomes visible. Before each redirect or
restoration write it compares the live bytes with the expected pre-image, which
detects a changed site and does not exclude a writer. It checks nonwrapping x86
ranges and full wrapper/trampoline overlap. Restoration reads and compares the
owned bytes before changing them.

The transaction revalidates the lease before each lease-gated callback and again
after each write, verification and flush. The
[contract](#quiescence-and-module-binding-contract) names the lease-gated
callbacks. Each revalidation samples backend attestations and does not exclude
other threads. The lease token reaches only revalidation and release.
Successful installation releases the lease. The first restoration acquires a
new one.

Failures follow these rules:

- A failure inside a write window sets unknown side effects and makes one
  attempt to restore the saved protection without revalidating, because the
  revalidation may be the failure and a range left without Execute can fault
  where NX is enforced. The attempt can expose torn instruction bytes. A
  restore that fails, or that inspection does not confirm, sets
  `protection_unverified` in the state and the report. A protection change with
  an ambiguous result or an invalid echo is not undone and sets the same flag.
- Unknown side effects block subsequent cleanup. No reconciliation API is
  provided, and a failed callback result cannot establish resource ownership.
  Backend exceptions are ambiguous results, not evidence that a mutation did
  not occur. An ambiguous revalidation always sets unknown side effects.
- A refused revalidation before the first mutating callback of its step
  changes nothing. A refused first revalidation of an installation rejects it
  and releases the pin and lease. A later one retains the state without unknown
  side effects, and restoration may retry. A retry reuses the held lease and
  needs a passing revalidation. When the state holds no CFG registration,
  publication, trampoline storage or redirect that may be visible, restoration
  instead releases the lease and pin after a refused revalidation. Restoration
  reports `Restored` after an installation that completed and `RolledBack`
  after one that did not.
- Both reports carry `unknown_side_effects` and `protection_unverified` from
  the state when they retain it, so retry guidance reads from the report.
  `restored_entries` counts one call, and an entry whose redirect never
  completed is not counted.
- There is no destructor cleanup or automatic retry.

The backend owns native module, mapping, protection, CFG, publication and
thread-proof semantics. No Windows backend factory is provided. Fake-backend
tests establish transaction ordering, argument coherence and failure handling
only. Calls and state inspection for one transaction must be externally
serialized. The transient preparation state rejects callback reentry into
installation or restoration. It does not synchronize competing threads.

## Quiescence and module binding contract

Whether a native lease can hold the conditions below for the pinned engine is
not established. The transaction calls a lease-gated callback only after the
backend attests every condition at a revalidation, apart from the one protection
repair after a failure. It verifies none of them. A default proof refuses.

| Proof field | Attestation |
|---|---|
| `zero_active_calls` | At the call, no call that entered a wrapper is still in progress. The transaction never reads the recorder's active-call count. That count starts inside each bridge, so it cannot see a thread that has taken a redirect but not yet entered the bridge. |
| `other_threads_held` | From acquisition to release, no thread of the process that hosts the module, other than the caller, can execute user-mode code. This covers every thread of the process, not those present at acquisition. |
| `thread_creation_barred` | Over the same interval, no thread can start in that process. |
| `no_context_in_span_interiors` | At the call, no held thread has an instruction pointer inside a span interior, in its current context or in a saved APC or exception context. A pointer at the entry byte or at the first byte after the span is outside. The interiors are RVAs `0x467F14` to `0x467F17`, `0x468B11` to `0x468B16` and `0x3D049E` to `0x3D04A2`. |
| `no_context_in_wrapper_or_trampoline` | At the call, no held thread has an instruction pointer inside a wrapper extent or a trampoline, in its current context or in a saved APC or exception context. A thread inside a wrapper that has not yet entered the bridge would otherwise call a published original after restoration frees it. |

The transaction cannot check these obligations:

- The lease-gated callbacks are `reserve_executable`, `free_executable`,
  `change_protection`, `write_bytes`, `flush_instruction_cache`, `register_cfg`,
  `revoke_cfg`, `publish_original` and `clear_original`. Each must refuse and
  change nothing when the backend's own lease is not held, because the lease
  token is not passed to it. `retain_module` and `acquire_quiescence` run before
  a lease exists and `release_module` runs after it is released.
  `release_quiescence` receives the token. The repair attempt calls
  `restore_protection` without a revalidation, so the backend must accept it
  after a lost lease.
- The transaction asks for no write atomicity. Exclusion of executors rests on
  the hold.
- `verify_bytes` returns Success only when the bytes are equal and Refused
  when they differ. `read_bytes` returns the bytes at the address, and the
  resident comparison trusts it.
- The backend derives the ranges its attestations cover: interiors from the
  loaded base and the span table, wrappers from the transaction's own
  `inspect_range` calls or from a configuration identical to the request, and
  trampolines from its own reservations. No other callback receives a wrapper
  extent. A backend that changes protection by page removes Execute from all
  code on those pages for the window, so the hold must exclude every thread,
  not only those near the span.
- The hold lasts through release. Release revokes CFG registration before it
  clears publication and frees storage.
- During installation, module inspection, the first range inspections and the
  resident reads run while the lease is held and before the first
  revalidation. Every other callback also runs while the hold lasts, including
  later range inspections, byte reads and compares and the protection repair.
  A backend whose hold suspends its own helper threads must serve all of them
  from the calling thread.

The transaction requires the module handle to equal the reported loaded base.
It compares the 5, 7 and 6 resident span bytes with the file bytes after
rebasing the one HIGHLOW operand. It requires each code range to lie inside one
inspected region that is owned, executable and not writable. File size,
SHA-256, preferred base, image size and relocation evidence are backend reports
that the transaction compares only with the pinned constants. The reported
loaded base is compared with the module handle and used for range arithmetic.
Ownership, protection and pin semantics are backend reports as well. A file
hash, a reported path or a module handle does not bind resident bytes. The
bytes at each resume address and the rest of each function are not compared,
and nothing shows that the mapping is the image section of the pinned file.

### Edges not established

| Edge | Locator, file read or reason none exists |
|---|---|
| Threads that can reach each entry, including engine-internal threads, and the thread that installs | The call sites in [the breakpoint context path](breakpoint-context-path.md) establish presence, not execution or the executing thread. The installing thread depends on the host and has no static locator. From the pinned file: import references to thread-creation APIs with their call sites, DllMain and TLS callbacks. |
| A hold and creation barrier mechanism | No static locator exists, because the mechanism depends on the host and OS. A hold that suspends threads can stop one that owns a loader or heap lock the caller needs. An x86 observer sees [a WOW64 representation, not a trap frame](map-selection-trace.md#raw-event-ownership-boundary), so saved APC and exception contexts may not be readable. |
| Branches into span interiors | No scan exists, and the planner checks only relocations that overlap a span. From the pinned file: relative branch targets and HIGHLOW operands equal to the interior RVAs above. |
| A hot-patch alternative | The bytes before each entry are not recorded. From the pinned file: the 16 bytes before RVA `0x467F13`, `0x468B10` and `0x3D049D`, which are `0x467F03` to `0x467F12`, `0x468B00` to `0x468B0F` and `0x3D048D` to `0x3D049C`, located with the section mapping in `observer_hook_plan.py`. |
| Full resident comparison | From the pinned file: section table with VirtualSize and flags, relocation directory, DllCharacteristics and load-configuration directory. A comparison must exclude `.mrdata` at RVA `0x5A8000`, length `0x1BE4`, whose protection and slot contents the engine changes at run time ([recorder preflight](recorder-preflight.md)). The inspection structures carry no allocation base, memory type or backing identity. |
| Pin and installed interval | No static locator exists, because pin semantics belong to the backend. In `observer_hook_install.cpp`, `install_hook_transaction` calls `inspect_module` once and `restore_entries_and_resources` calls it never, so module identity, base and size are not rechecked while installed or during restoration, and restoration rechecks only each site range, protection and bytes. |
| Write exposure | Write width and order and NX enforcement in the hosting process have no recorded locator, because they depend on the host. From the pinned file: the other code on the pages of the three entries, which are RVA `0x467000` to `0x467FFF`, `0x468000` to `0x468FFF` and `0x3D0000` to `0x3D0FFF`, is not recorded. |
| Release and retry | In `observer_hook_install.cpp`, `restore_entries_and_resources` returns at once when unknown side effects are set, so no call releases a held lease after them. A refused restoration that holds a code resource returns through `retained_restore` with its lease held, so a retry can pass only if the backend proof can change while the lease is held. A thread held inside a wrapper cannot leave while the hold lasts. |
| Copied-entry behavior | From the pinned file: the code at the resume addresses RVA `0x467F18`, `0x468B17` and `0x3D04A3` and the callee frames reached from them, for unwind and exception behavior. CFG validity of the wrapper-to-trampoline call has no static locator in the pinned file, because the call originates in the observer module and depends on that module's CFG instrumentation, the host process policy and the registered targets. The remaining requirements below state the open items. |

## Synchronous raw-event bridge

[The bridge API](observer_event_bridge.h) attaches an optional sink to
`RawRecorder` while inactive and quiescent. Supported events are offered after
raw decoding, lifecycle handling and context snapshot capture, before wait
returns. Continuation observations include the actual result before return.
Failed continuation retains the pending identity; successful continuation
closes only a uniquely matched event. Wait/continue originals, arguments,
results and LastError/LastStatus are preserved.

The engine-generation provider receives the exact debug object, PID/TID, raw
generation and raw index. It must establish a separate engine binding. Missing
or failed bindings remain unknown; the bridge never derives one from raw
generation, addresses or neighboring rows. Missing identities, overlap,
overflow and sink failures make coverage incomplete. Snapshot readers require
quiescence. The bridge, recorder, callbacks and callback data must outlive all
notifications; detach the sink only after deactivation and quiescence.

`--bridge-output` emits a fresh synthetic receipt from fake wait, context and
continuation calls. Its raw bytes and event identities belong to that harness.
It does not qualify engine identities or complete native write coverage.

## Remaining runtime requirements

There is no live backend or capture command for these adapters. Native entry
redirect installation still needs demonstrated thread quiescence, resident
module/digest binding, relocation verification, CFG-valid trampoline calls,
owned protection transitions, lifetime and restoration. A copied entry's
exception/unwind behavior also needs review before use in the engine. The
[contract](#quiescence-and-module-binding-contract) lists the attestations and
the edges not established.

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
