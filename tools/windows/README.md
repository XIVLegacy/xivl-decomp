# Virtual-item predicate diagnostic

`sample_virtual_item_state` records the two native completion results,
loader flags, a selected sheet-binding lookup and visited identifier checks
identified in [virtual-item creation](../../docs/script/virtual-item-creation.md).
It accepts only the pinned retail 1.23b executable and fixed observation
addresses, apart from its separate synthetic fixture mode.

This is a debugger diagnostic, not a latency benchmark. Attachment and
each breakpoint pause the target's threads. Hardware execute breakpoints
change debugger state and debug registers; the tool does not patch game
files or application fields. Instrumented elapsed times include debugger
effects and cannot establish the natural shop-opening duration.

## Build and fixture verification

Build with CMake 3.25 or newer, Visual Studio 2022 MSVC, and a Windows SDK
containing `DbgEng.h`. Select Win32 even on a 64-bit Windows host. Use an
explicit new build directory outside the tracked tree:

```powershell
cmake -S tools/windows -B C:\scratch\item-diagnostic-build -G "Visual Studio 17 2022" -A Win32
cmake --build C:\scratch\item-diagnostic-build --config Release
python tools/windows/test_item_predicate.py `
    --build-directory C:\scratch\item-diagnostic-build\Release `
    --engine C:\Windows\SysWOW64\dbgeng.dll `
    --output-directory C:\scratch\item-diagnostic-tests
```

The debugger DLL must be an explicitly supplied, trusted x86 Microsoft
DbgEng installation. The system engine `10.0.26100.1` was exercised on
Windows 11. A Store-packaged engine can reject loading from an external
executable. No debugger download or installation is performed by this tool.

The asset-free fixture has independent instruction sites and known state.
Tests check both phases and both boolean values, the raw builder fields,
loader state transitions, matching/different/null binding collections,
identifier results and fixed field reads, the pre-store latch, idle capture,
invalid checker/loader cleanup, identity and
existing-output refusal, and Ctrl+Break cancellation. Each test requires
that the fixture is no longer debugged, produces a heartbeat after detach,
and subsequently exits normally. Fixtures are never terminated to pass
cleanup. The tests need an interactive Windows session for the hidden
console used to deliver Ctrl+Break.

## Capture one opening

Use a disposable session and keep its weapon-category selector open.
Choose the intended client PID explicitly. Close any other debugger first.
Create the output directory beforehand and use a new output filename:

```powershell
C:\scratch\item-diagnostic-build\Release\sample_virtual_item_state.exe `
    --pid 12345 --seconds 12 `
    --dbgeng C:\Windows\SysWOW64\dbgeng.dll `
    --output C:\scratch\item-predicates.jsonl
```

1. Wait for `Recording armed` in the console.
2. Return to the game and select one weapon category, then wait for its item list.
3. Wait for `Detached` and record whether the list appeared and interaction continued.

The duration accepts 1 through 30 seconds and defaults to 12. Capture also
ends at 10,000 observations or on Ctrl+C / Ctrl+Break. These keys request
cleanup, including during attachment; closing the console is not a tested
cancellation path. Do not close it while attached.

The tool holds the queried process handle to prevent PID reuse, rejects
an already debugged target, verifies the executable digest and loaded site
signatures, and validates checker/loader vtables and known boolean fields
at predicate samples. It uses four hardware execute observation points.
It removes its breakpoints and resumes through the debugger engine before
detaching. A timer interrupts the blocking event wait; it is joined before
cleanup. This avoids losing available target context through a timed-out
capture wait. Microsoft documents [the execution model](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/debugging-session-and-execution-model),
[interrupts](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol-setinterrupt),
and [detach behavior](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugclient-detachprocesses).

## Record interpretation

The JSONL identity row records PID, executable/engine SHA-256 and
`format_version: 2`. Observation rows are buffered during attachment and
written after cleanup. All have `tid`, `utc_filetime` and `elapsed_ms`.
The record contains three kinds of observation:

| Field | Meaning |
|---|---|
| `kind=predicate` | Returned result at either original observation point. |
| `phase` | 1: result from `0x006e2d30`; 2: result from `0x006ed9e0`. |
| `result` | The returned `AL` boolean; 0 is pending and 1 is complete for that stage. |
| `checker`, `builder` | Observed 32-bit addresses, not permanent operation IDs. |
| `builder_18` | Raw dword at builder `+0x18`, without a semantic catalog-ID claim. |
| `builder_10`, `builder_14` | Raw builder dwords, without an operation-ID claim. |
| `first_done_before_store` | Checker byte `+0x21` at the observation point. |
| `loader`, `loader_08` through `loader_0c` | Loader pointer and state bytes; `0b` remains uninterpreted. `08=1` with `0a=0` on a pending first query indicates waiting for its callback. |
| `collection`, `collection_08` | Checker-owned collection pointer and its raw `+0x08` field. |
| `tid` | Windows thread ID at the breakpoint. |
| `utc_filetime`, `elapsed_ms` | Host observation time in FILETIME ticks and milliseconds since arming. |

`kind=binding` at `0x006f5461` records `item`, `key_low`, `key_high`,
`identifier`, `owner`, `container`, `context`, the returned `collection`,
and `collection_08_before_insert` (null for a null returned collection).
It observes only the selected native helper's conditional lookup path,
before insertion. Compare its collection pointer with a predicate's
checker-owned collection within the same lifetime; do not assume they match.
A zero collection is still recorded, not replaced or repaired.

`kind=identifier` at `0x006eda5a` records `collection`,
`collection_08_before_erase`, `node`, `identifier`, `result` and `manager`.
Result zero leaves that collection query pending; result one proceeds to
conditional removal. The query stops on its first pending identifier,
so these rows are the visited prefix, not a complete membership list.
The [native observation contract](../../docs/script/virtual-item-creation.md#loader-and-collection-observation-contract)
owns each register and field locator. No tree traversal or target function
invocation occurs.

Format 2 retains the original predicate fields but adds two row kinds.
Analysis written for format 1 must filter by `kind` and understand these
additional records; treating all observations as predicates is invalid.

A `detached` row reports observation count, stop reason, and
`host_snapshot_ms`: summed host time spent reading and buffering snapshots.
That metric excludes event delivery, actual resume, attach/detach and other
debugger work. It is not total target pause or observer overhead.

On an error, previously acquired rows are retained followed by a `failed`
row. `detach_confirmed: false` after `attachment_started: true` requires
inspection of the target state; it is not a successful capture.
Failed debugger cleanup does not guarantee that the target remains attached
or continues normally after the diagnostic exits. An engine failure is an
unresolved target state, even if best-effort cleanup was attempted. A zero
observation capture establishes only that no accepted observation point
was hit within that instrumented interval. Archive records locally with
the tool revision/build digest and the corresponding packet-capture identity;
do not publish raw process records or client binaries.
