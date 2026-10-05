# Observer forwarding diagnostic

`observer_diagnostic_check` exercises three x86 call-through adapters using
fake originals and injected memory, error and handle-identity readers. It
exports synthetic service-selection and context-write rows with one entry/exit
sequence source. The executable has no attachment, DLL loading, hook
installation or retail mode.

The supported execution profile is `synthetic-forwarding-profile`. A checked
query output means the fake reader copied its returned interface, vtable and
slot `+0x10`; it does not bind a loaded implementation. Failed, null and unread
outputs remain unqualified. The serializer always reports live coverage as
`incomplete`.

## Build and asset-free checks

Use CMake 3.25 or newer, Visual Studio 2022 MSVC, Windows SDK and a fresh
absolute build directory outside the tracked tree. Build only this target:

```powershell
cmake -S tools/windows -B C:\scratch\observer-build -G "Visual Studio 17 2022" -A Win32
cmake --build C:\scratch\observer-build --config Release --target observer_diagnostic_check
C:\scratch\observer-build\Release\observer_diagnostic_check.exe --self-test
C:\scratch\observer-build\Release\observer_diagnostic_check.exe --synthetic-output C:\scratch\observer-synthetic.json
python tools/windows/validate_observer_diagnostic.py --trace C:\scratch\observer-synthetic.json
python tools/windows/test_validate_observer_diagnostic.py --trace C:\scratch\observer-synthetic.json
python tools/windows/test_observer_hook_plan.py
```

The output parent must exist. Existing destinations and relative paths are
refused. A failed write leaves its partial file for inspection. Synthetic
addresses, handles and event identities come from the test harness.

The C++ checks exercise Win32 arguments and stack balance, unchanged results
and error pairs, nested lookup/query identities, adjusted interface pointers,
real x86 CONTEXT field offsets, refused reads, exceptions, overflow, reentry
and concurrent forwarding. They do not run a native context API. The offline
validator rejects incomplete samples and malformed event joins. Validator
acceptance establishes this synthetic record contract only.

## Adapter contract

The public ABI and static selection finding is
[the breakpoint context path](breakpoint-context-path.md).
[The header](observer_diagnostic.h) exposes `Recorder`, typed originals,
injected callbacks and thread-local `BridgeScope` binding. Each original must
remain valid through every active call. Each participating thread needs its
own binding. Unbound bridges are unsupported and return sentinel results;
they cannot be installed as transparent hooks.

Readers must refuse inaccessible ranges and must not fault. C++ callback
exceptions become refused reads. `/EHsc` supplies no SEH recovery. Error-pair
read/write callbacks must themselves preserve the pair they observe or restore.
Missing or failed error callbacks make preservation unproved. Caller-owned
quiescence is required before snapshots or serialization; no original executes
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
raw generation and event index remain distinct from the engine generation.
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

## Remaining runtime requirements

There is no live installer or capture command for these adapters. Entry
redirect installation still needs demonstrated thread quiescence, resident
module/digest binding, relocation verification, CFG-valid trampoline calls,
owned protection transitions, lifetime and restoration. A copied entry's
exception/unwind behavior also needs review before use in the engine.

Runtime coverage additionally requires the raw-event admission bridge,
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
