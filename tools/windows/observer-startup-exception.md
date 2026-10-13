# Startup exception classification

The [fail-stop profile](observer-candidate.md#observer-exception-profile)
refuses observer exception events before ordinary continuation. A startup
allowance requires a separate identity and origin proof. These file and source
findings narrow that proof; they do not classify a native event.

## Retained x86 file candidates

The analyzed `ntdll.dll` is PE32/I386, version `10.0.26100.9444`, 1,822,400
bytes, SHA-256
`7e15bd30890e9bf93b47fc894a68b2445618ee7528eb39584a263b21e112f9df`.
Its preferred image base is `0x4B280000` and image extent is `0x1BF000`.
These identify a retained file, without establishing a loaded mapping.
All addresses below are RVAs within that exact file.

The producing method used Python 3.12, pefile 2024.8.26 and Capstone 5.0.7
in x86-32 mode. It parsed PE metadata and decoded bounded linear windows and
rooted control-flow graphs, with raw rel32 call/jump candidates in executable
sections and preferred-address byte candidates in file-backed sections.
Calls were allowed to return and both
conditional outcomes were explored. Cropped endpoints, indirect branches and
outside-range paths remained unknown; the graphs did not establish complete
function bounds or executed paths. The retained target was parsed as file bytes.

| Candidate | File observation | Limit |
| --- | --- | --- |
| `LdrInitializeThunk`, `0x34230` | Bounded rooted control flow reaches calls to the helper at `0x118B5D`; its conditional breakpoint is the inline `CC` at `0x118B88`. | The paths and branch operands do not establish which route a created process takes. |
| `DbgBreakPoint`, `0x7BE10` | Exported breakpoint body has its own `CC`. `DbgUiRemoteBreakin`, `0xCE9B0`, calls it at `0xCE9E4`. | The separate remote-break path prevents identifying this export with the inline loader candidate. |
| `DbgUserBreakPoint`, `0x7BE00` | Separate exported breakpoint body; the executable-section scan found no direct rel32 call candidate. | That bounded result does not exclude other producers or global callers. |

The retained loader route includes conditional transfers through `0x342A0`
and `0xC9344`. Calls at `0xCAA79` and `0x344BC` reach `0x118B5D`.
The helper calls `NtQueryInformationThread` with class `0x11` and
pseudo-handle `-2`, then skips its inline breakpoint on negative status or
a nonzero output byte. These are conditional encoded routes, not a complete
native loader history or an exhaustive indirect-caller boundary.

The byte following the inline breakpoint is `0x118B89`. A later native
observation must preserve the delivered exception address and thread context
separately; file instruction bytes cannot determine Windows context adjustment.
The x86 file supplies no identity or producer proof for native Ntdll or WOW64
components. The [WOW64 implementation contract](https://learn.microsoft.com/en-us/windows/win32/winprog64/wow64-implementation-details)
describes those separate architecture layers.

## Creation and phase joins

[ObserverDebugOwner](observer_live_runtime.cpp) retains both the creator's
controller thread ID and the original child thread ID returned by
`CreateProcessW`. Event admission requires the created process ID and a
nonzero event thread ID. Exact-owner continuation checks the controller thread
and authoritative held event key. It does not require an exception's target
thread to equal the original child thread. The exception refusal records the
controller and event thread IDs; the creation ledger records the original
child thread separately.

The controller provisions activation under the creation event, continues that
event and then waits for bootstrap readiness. Child entry and composition
preparation can precede readiness. Therefore, missing bootstrap, options or
hook-ready state does not prove that observer entry, initialization or dynamic
loading has not run. The documented [initial breakpoint](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/initial-breakpoint)
for a created target precedes DLL initialization after main/static DLL loading;
that event-class contract cannot identify an arbitrary pre-bootstrap exception.

| Required join | Source/file finding | Evidence needed for an allowance |
| --- | --- | --- |
| Created process | Creation-owned process handle and PID are retained. | Bind the authoritative held event to that same created instance. |
| Original child thread | Its ID is retained separately from controller and event thread IDs. | Establish the event's target-thread identity and its required relation to the original child thread. |
| Resident module | The retained file has an exact byte identity and candidate RVAs. | Join the loaded base/extent and resident file identity to the exception and producer. |
| Producer and phase | Multiple conditional breakpoint routes exist; pre-bootstrap is only a controller state. | Preserve actual context and caller/route evidence establishing the intended startup event before forbidden initialization or hooks. |
| Single use | The creation-attempt latch limits creation; raw generation/index identify held events. | A reviewed generation-bound consumed state must refuse reuse, duplicate or stale startup admission. |

First chance is a notification property of an individual exception, according
to [EXCEPTION_DEBUG_INFO](https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-exception_debug_info).
Exception code, chance, address, first-seen order or a familiar export name
cannot supply the missing joins. The current source has no startup classifier
or startup-consumption state; all held observer exceptions remain refused.

## Acquisition boundary

The [supporting identity capture](observer_supporting_identity.cpp) records
the creation-bound `CREATE_PROCESS_DEBUG_EVENT` and `LOAD_DLL_DEBUG_EVENT`
file handle, module base, path, size and hash. Its accepted load records do not
contain an image extent or resident instruction bytes. The
[held inventory helper](observer_live_runtime.cpp) can take an exception
address from a held event and reconcile module and allocation ranges, but
its production call occurs after bootstrap binding and the initial hold
worker. That later acquisition does not classify an earlier startup exception.

The [exception refusal record](observer_exception_policy.h) contains the
held exception's code, flags, chance and address with creation PID,
controller/event TIDs, session and event generation/index. Context, stack,
original child TID, activation attempt, module identity and phase are not
fields of that record. Creation handles, the original child TID and activation
identity are retained separately. A startup classification would need an
explicit join to those sources while the authoritative event remains held.

A conservative classifier would require one event-bound record joining that
creation instance, thread lifetime, architecture-specific context, resident
mapping and producer/phase witness before candidate continuation. The two
encoded helper callers share the same inline breakpoint, so the loaded RVA
alone cannot distinguish their routes. Missing or contradictory joins must
refuse. Any reviewed one-use state would have to consume the exact
attempt/process/thread/phase admission before continuation dispatch and remain
consumed after a failed or unknown result. These acquisition requirements
establish neither an allowance nor a current source defect.

## Qualification boundary

The file trace and source audit supply an offline classification proposal.
They supply no observed initial exception, loaded-module identity, context,
caller history or successful continuation. The default descriptor refuses
native activation. A selected allowance still requires reviewed event-origin
evidence, source implementation and a fresh operating source/build/product
binding. The hook entrant and neighboring-byte coverage requirements remain
separate in the [candidate contract](observer-candidate.md#conditional-native-qualification).
