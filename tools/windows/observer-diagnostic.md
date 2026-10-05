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
entries and pinned file profile as the offline planner. Its injected backend
must bind the file identity to the retained resident module, prove relocation
coverage, inspect full code ranges, and supply a quiescence lease covering
every participating thread. An idle-call count alone is insufficient.
Successful installation releases the lease; restoration acquires a new one.

The transaction prepares relocated trampolines, verifies their bytes, changes
them from writable to executable, flushes instruction caches, registers CFG
targets and publishes originals before redirects become visible. It checks
nonwrapping x86 ranges and full wrapper/trampoline overlap. Restoration checks
owned bytes before changing them. Uncertain writes, changed ownership or lost
quiescence retain the state and reachable resources for inspection. There is
no destructor cleanup or automatic retry. Backend exceptions are ambiguous
results, not evidence that a mutation did not occur.
Unknown side effects block subsequent cleanup. No reconciliation API is
provided; a failed callback result cannot establish resource ownership.

The backend owns native module, mapping, protection, CFG, publication and
thread-proof semantics. No Windows backend factory is provided. Fake-backend
tests establish transaction ordering and failure handling only.
Calls and state inspection for one transaction must be externally serialized.
The transient preparation state rejects callback reentry into installation
or restoration; it does not synchronize competing threads.

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
