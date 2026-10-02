# Native map selection trace

`trace_map_selection` records native SetMap input, region construction and
RegionInfo lookup for a bounded, owner-operated selection. It uses four
hardware execute breakpoints and writes private JSONL records. It does not
select a zone or allocate an identifier.

Attachment and breakpoint handling pause the target's threads. The observer
changes debugger state and debug registers; it does not patch client files,
application fields or instructions. Its timing includes debugger effects.

## Binary and native observation contract

The supported retail profile is `ffxivgame.exe`, build `2012.09.19.0001`,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 I386 loaded at VA `0x00400000`. Retail addresses are fixed. The observer
rejects other images, rebasing and mismatched resident instruction spans.

The four observations use DbgEng processor breakpoints with one-byte execute
access and no thread restriction. The
[match-thread contract](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugbreakpoint-setmatchthreadid)
allows any thread to trigger an unrestricted breakpoint. The engine owns
breakpoint installation and
[removal](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol-removebreakpoint);
the fixture verifies coverage and removal on the supported Windows profile.

The supported retail thread policy is `initial_threads_only`. A new
application thread ends the interval with a failed record and cleanup before
it resumes with observation breakpoints. Such an interval is not an accepted
control. Debugger attachment and interrupt helpers require a separately
verified identity before their breakpoint exceptions can be handled as
observer-owned events.

The synthetic fixture can exercise `global_all_threads`; this policy is not
available for the retail image. An unmapped single-step exception can occur
at an observation site before an ordinary breakpoint callback. A matching
fixture handler record can establish application delivery for that event;
it does not establish its origin or support retail use of that policy.
Thread creation and exit records distinguish debugger engine IDs from Windows
thread IDs. A lifecycle generation separates reused identities and expires
pending lookup entries when their thread exits.

The following VAs and arguments were recovered with Ghidra
`tools/ghidra_scripts/DumpFunctions.java` and exact-address x86-32
disassembly of the pinned executable. Register and stack offsets refer to the
instruction being observed, before it executes.

| Observation VA | Actual bases and values |
|---|---|
| `0x0059CED0` | Receiver entry: ECX is MapLayoutElement; `[ESP+0x04]` is the GameMessageHeader pointer; `[ESP]` is the return address. Opcode is the word at that buffer `+0x02`. |
| `0x00626DF0` | Region construction entry: ECX is the scene; `[ESP+0x04]` is the region dword; `[ESP+0x08]` is retained as a raw argument. Scene `+0x190` and `+0x17C` still contain their prior values. |
| `0x0079B380` | Lookup entry: ECX is the RegionInfo table; `[ESP+0x04]` is the full-dword query; `[ESP]` is its actual caller return address. Table `+0x0C/+0x10` bounds its four-byte root-pointer span. |
| `0x0064E87C` | Manager lookup result, before storing EAX at `[EBP+0x10]`: EBP is the manager and EAX is the matched RegionInfo or null. Manager `+0x14` is its region dword. |

For opcode `0x0005`, receiver reads at `0x0059CEF9`, `0x0059CEFD` and
`0x0059CF0E` establish zone at game-header base `+0x14`, region at
`+0x10`, and mode byte at `+0x18`. The integration SetMap contract uses a
48-byte complete subpacket: 16-byte outer header, 16-byte game header and
16-byte application payload, as specified in
`xivl-client-structs:docs/map-layout-selector.md`, "SetMap framing and native base".
The [header declarations](../../include/net/lobby_proto_channel.h) own those
two header layouts. The actual native argument points to the game header,
so the observer reads 32 bytes from that pointer and records the application
base at pointer `+0x10`. Application dwords `+0/+4` remain region/zone;
all 16 application bytes are retained. The outer header's address is not
established by this hook and is not inferred by subtracting 16. Generated
structure offsets rebased to another header are not application offsets.
The hook does not measure a complete retail subpacket's wire length; the
framing sizes in its records describe the integration contract.

Construction writes the region to scene `+0x190` at `0x00626E25` and the
new manager to scene `+0x17C` at `0x00626E57`. These stores are static
locators; the construction-entry observation precedes both. The result hook
also records the scene obtained from global `0x0133DEF4`, its stored region
and manager, and its RegionInfo table at `+0x154`. Their values are observed
at that later hook, rather than inferred from entry arguments.
Lookup can run inside `0x0064F900` before the caller reaches the manager
assignment at `0x00626E57`; scene `+0x17C` can therefore still hold the
previous manager while EBP identifies the manager being constructed.

The manager call to lookup is at `0x0064E875`, with return address
`0x0064E87A`. It loads the query from manager `+0x14` at `0x0064E869`.
For that caller, EBP identifies the manager at lookup entry; the observer
checks its vtable `0x00FBFA88` before reading its fields. The matched root's
`+0xB0` key and `+0xB8` auxiliary value are recorded at the result hook.
A null result remains a null result. Other callers to `0x0079B380` remain
distinguishable and do not acquire this manager interpretation.

[RegionInfo selection](../../docs/resource/map-layout-selector.md) owns
lookup widths, child eligibility, transport narrowing and resource selection.
This observer does not enumerate children or capture resource requests/opens.

## Build and fixture verification

Use CMake 3.25 or newer, Visual Studio 2022 MSVC and a Windows SDK containing
`DbgEng.h`. Select Win32 and an explicit new output directory outside the
tracked tree:

```powershell
cmake -S tools/windows -B C:\scratch\map-selection-build -G "Visual Studio 17 2022" -A Win32
cmake --build C:\scratch\map-selection-build --config Release
python tools/windows/test_map_selection.py `
    --build-directory C:\scratch\map-selection-build\Release `
    --engine C:\Windows\SysWOW64\dbgeng.dll `
    --output-directory C:\scratch\map-selection-tests
```

The engine must be an explicit trusted x86 Microsoft DbgEng DLL. The tool
records its digest and performs no download or installation. Its separate
synthetic fixture supplies independent instruction sites and contains no
client assets. Fixture verification checks actual register/stack and buffer
bases, matched/null lookup values, bounded idle capture, input refusals,
failure cleanup, cancellation and the total-record cap. The retail-policy
fixture checks new-thread refusal and cleanup. The separate synthetic global
policy exercises workers created during capture, worker exit and a handled
exception. Surviving initial and later workers replay all four sites after
detach; these checks do not resolve the unmapped single-step's provenance.
Ordinary surviving fixtures must resume after detach, lose their debugger and
exit normally. A separate case checks a normal target exit during capture.
The deliberate unhandled TF controls below retain their exception exit codes;
tests never terminate a fixture to make cleanup pass.

### Exception delivery comparisons

Run the deliberate delivery calibrations with explicit build, engine and
output paths:

```powershell
python tools/windows/test_map_selection.py `
    --build-directory C:\scratch\map-selection-build\Release `
    --engine C:\Windows\SysWOW64\dbgeng.dll `
    --output-directory C:\scratch\map-exception-tests `
    --case exception-delivery
```

These controls deliberately set the CPU trap flag or raise an application
breakpoint. Their known triggers calibrate delivery instrumentation.

Run a separate bounded comparison of unchanged fixture cycles and a
handler-only variant:

```powershell
python tools/windows/test_map_selection.py `
    --build-directory C:\scratch\map-selection-build\Release `
    --engine C:\Windows\SysWOW64\dbgeng.dll `
    --output-directory C:\scratch\map-natural-comparison `
    --case natural-comparison
```

Each variant runs for three seconds. The handler logs delivery and returns
`EXCEPTION_CONTINUE_SEARCH`. It neither changes the exception context nor
swallows the exception. The default verification matrix does not run these
comparisons. Keep the deliberate TF controls separate from spontaneous
single-step observations, and do not repeat comparisons until an event occurs.

The [vectored-handler contract](https://learn.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling)
places handler invocation after the debugger's first-chance notification.
Pair debugger and fixture records using actual thread identities and instruction
addresses. Keep unmatched records and actual fixture exit codes. A passing
calibration or a comparison with no spontaneous event leaves that event's
origin unresolved. These comparisons use synthetic processes and do not
extend the supported retail thread policy.

## Capture one unchanged selection

Choose the intended PID explicitly. Keep all client files and the server's
selection unchanged. The owner launches, logs in and operates the game.
Create the private output directory beforehand and use a new filename:

```powershell
C:\scratch\map-selection-build\Release\trace_map_selection.exe `
    --pid 12345 --seconds 30 `
    --dbgeng C:\Windows\SysWOW64\dbgeng.dll `
    --output C:\scratch\retail-selection.jsonl
```

1. Wait for `Recording armed` before making the agreed selection normally.
2. Record the displayed area and the action performed, including whether
   a fresh SetMap could have occurred during the interval.
3. Wait for the observer to exit, inspect its terminal `detach_confirmed`
   value and record whether the client continued normally. A successful
   capture also prints `Detached`.

An explicit duration of 1 through 30 seconds is required. A 10,000-record
limit and Ctrl+C / Ctrl+Break also end capture through cleanup. Closing the
console is not a tested cancellation path. An already debugged target, invalid profile or
existing output is refused. Raw process records and executable/build digests
stay private and outside tracked trees.

## Records

Format 2 begins with an `identity` row containing the target and engine
digests and the retail/fixture distinction. `profile` rows retain the resident
locator checks. Observation rows have `sequence`, `tid`, `utc_filetime`,
`elapsed_ms`, `hook_eip`, integer registers and `stack_slots`.
In fixture identities, `image_base` and `retail_sha256` describe the retail
reference profile; `image_base` is not the fixture's observed module base.
Fixture site addresses come from the actual fixture launch.
`utc_filetime` is the Windows FILETIME tick value, not a Unix timestamp;
elapsed time uses a separate steady clock. Stack slots begin at the recorded
ESP and are raw dwords, rather than universally named function arguments.
Observations also retain `engine_tid`, `thread_data_offset`, `thread_teb_offset`
and `lifecycle_generation` to identify the thread lifetime used for a lookup
join. Engine IDs and Windows thread IDs are separate namespaces.

Hook and exception rows also contain `debug_context` reads for EIP, EFLAGS
and DR0 through DR7. Each read retains `available`, `result` as an unsigned
HRESULT, and `value`. An unavailable read's value is not an observed register
value. These are the selected thread's debugger-visible values after the
event stop; they do not establish the processor state before engine handling.
Hook rows copy the actual breakpoint callback object's ID, offset, type,
flags and data-access parameters into `breakpoint`, with availability/results.
Exception rows record `breakpoint.callback: false` and do not infer an engine
breakpoint from an address match.
Before decoding hook arguments, the observer requires the callback object's
ID and offset and its captured thread identity to agree with the selected
thread and observation site. The separately read
[event engine thread ID](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugsystemobjects-geteventthread)
must also agree. A contradictory or unavailable mapping produces
`partial_error` with the raw context, without interpreted hook fields.
Exception records retain the event-thread read and its validation separately
from selected debugger context. An unavailable or contradictory event identity
does not turn the selected Windows thread ID into a proven event thread;
the unowned exception is still retained and forwarded.
`identity_validation.validated` requires agreement of event engine IDs,
Windows thread IDs and lifecycle generation. `tid_qualification` distinguishes
`validated_event` from `selected_context`. Handler comparisons consume only
first-chance candidates with available successful EIP reads, once per handler
line. Older rows without event validation retain their selected-context limit.

`--fixture-label` requires fixture mode and records the caller's comparison
label in the identity row. The label describes the requested comparison; it
does not independently establish the exception's source.

| Observation kind | Meaning |
|---|---|
| `setmap` | Actual game/application bases, retained bytes, region, zone and mode. |
| `receiver_ignored` | Another opcode at the shared receiver; its application bytes are not decoded. |
| `region_constructor` | Scene and entry arguments, with prior scene region/manager values. |
| `region_lookup` | Table, full-dword query, pointer-span bounds and actual caller. Manager fields require the known caller. |
| `lookup_result` | Returned/null root, lookup correlation and guarded manager/scene fields. |
| `partial_error` | Raw context retained when callback ownership validation or an interpreted read failed. |
| `thread_created` / `thread_exited` | Ordered lifecycle events, with identity and an exit code for exit events. |
| `target_exception` | An unowned exception for which forwarding was requested, with selected context and event-thread validation. |

Terminal `detached` or `failed` records contain observation/hit counts and
detach status, `thread_policy`, `initial_thread_count`, `thread_created_events`
and `thread_exit_events`. Captured rows are buffered while attached and written
after cleanup, preserving order. A record count includes lifecycle and
forwarded-exception rows. An observation count
includes filtered receiver observations, so it is not a count of SetMaps.
Host observation times cannot measure total target pause or natural loading latency.
An exception row's `forwarded` flag records the request to resume with
`DEBUG_STATUS_GO_NOT_HANDLED`; it does not establish the exception's origin
or prove delivery to an application handler. An address match alone cannot
assign an exception to an observation breakpoint.
The [execution-status contract](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol-setexecutionstatus)
describes a request: execution occurs on the next `WaitForEvent` call.
Failed-terminal forwarding counts cover the recording loop and omit
additional cleanup forwarding. A zero failed-terminal count therefore does
not establish that no exception was forwarded during detach.
After attachment starts, `detach_confirmed: false` leaves target state
unresolved even if best-effort cleanup was attempted.

## Interpretation and remaining evidence

Order observations by sequence and retain thread IDs, hook EIP, registers,
stack slots and pointer bases. A manager lookup entry/result pair requires
the entry's actual return address `0x0064E87A`, the same thread lifetime and
manager, and result ESP equal to entry ESP plus eight, accounting for `RET 4`.
Retain the observed query and matched root key even when they differ.
Incomplete, interleaved or ambiguous records do not establish a pair.

SetMap, construction and lookup can be separated by scene queues. Equal
region values and nearby timestamps alone do not prove they belong to one
selection. The four observation points do not capture every queue operation
or the constructor's return. Keep unmatched calls and prior/current scene
manager values visible when reporting a correlation.

A zero-observation capture establishes only that no accepted site was seen
in that instrumented interval. A failed record or unconfirmed detach is not
an accepted control. A `record_cap` stop leaves the later event stream
unobserved. Neither a lookup hit nor a shared retail resource set
accepts an independent authored scene. Ordered ResourceModule requests,
Resource-correlated DAT opens and downstream completion remain required by
the [request boundary](../../docs/resource/map-layout-request-boundary.md)
and [auxiliary completion contract](../../docs/resource/region-auxiliary-resource.md).
Rendering, collision, walking and ZoneMaster/script initialization require
their own acceptance evidence.
