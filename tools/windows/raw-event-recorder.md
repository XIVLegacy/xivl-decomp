# Raw wait and continue recorder

The raw recorder is a fixture-only tool for observing the native wait and
continue boundary used by the pinned x86 DbgEng profile. It keeps a reserved
1024-entry per-class budget for the supported raw `--seconds 1..3` fixture
window in process-resident storage and forwards the original call with
unchanged arguments, return status, and both x86 current-thread error fields.
An overflow is a coverage gap; the budget does not establish stream completeness.
The integration path installs only for `--fixture --raw-recorder` with an
explicit absolute `--ntdll` path. Retail startup and `initial_threads_only`
remain unchanged.

## Build and checks

Build the Windows tools with Visual Studio 2022, the Win32 generator, and a
Windows SDK containing `DbgEng.h`:

```powershell
cmake -S tools/windows -B C:\scratch\xivl-decomp-raw-build -G "Visual Studio 17 2022" -A Win32
cmake --build C:\scratch\xivl-decomp-raw-build --config Release
C:\scratch\xivl-decomp-raw-build\Release\raw_recorder_check.exe --self-test
```

The self-test uses synthetic originals and injectable context APIs. It checks
argument and error-pair forwarding, strict wait admission, timeout and failure
payload refusal, lifecycle generations, header-only module events, exception
copying, callback and continuation correlation, fresh context-handle
ownership, context flag validation, resident late-call forwarding, CFG
function IDs, the measured process CFG mitigation query, resident ntdll
identity, and the actual install/restore transition through an injected
inspection/protection/CAS backend, including refusal of modified or unowned
helper pages. It does not claim a live engine call or event coverage.

The no-target profile check accepts explicit absolute paths to the pinned
images:

```powershell
C:\scratch\xivl-decomp-raw-build\Release\raw_recorder_check.exe `
    --install-check --engine C:\Windows\SysWOW64\dbgeng.dll `
    --ntdll C:\Windows\SysWOW64\ntdll.dll `
    --output C:\scratch\raw-recorder-install-check.json
```

This mode hashes and inspects the files, calls `DebugCreate` once, and reads
the descriptor, marker, native slots, lease count, saved protection, and this
observer's process CFG mitigation query and CFG function IDs without
requesting slot writes or protection changes. The JSON reports
`process_cfg_query_ok`, the immediate `process_cfg_query_error`,
`process_cfg_enabled`, and separate wait and continue wrapper FID results.
`DebugCreate` may perform its own normal engine initialization. The check
reports the loaded base addresses observed in that process. A return value of
`1` means the static profile is eligible for the later fixture-only install
path; it still does not attach a target or call native wait or continue.
The resident install routine repeats the process CFG query and both wrapper FID
checks immediately before it can enter the shared lease transition.

The synthetic transition check exercises the same bounded install and restore
ordering through an injected in-memory inspection/protection/CAS backend. It
does not load DbgEng or change native memory:

```powershell
C:\scratch\xivl-decomp-raw-build\Release\raw_recorder_check.exe `
    --transition-check
```

`--install-restore-check` is an equivalent command name.

It verifies that installation returns to read-only/count-zero with owned wait
and continue wrappers, and that restore returns to read-only/count-zero with
all three native values. Its result is labeled
`synthetic_transition_check`; it is not evidence of a native round trip.

The no-target native install and restore check reserves its output
file before it performs any native mutation, creates one own-process DbgEng
client, validates the pinned loaded image, measured process CFG policy, and
both wrapper FIDs, installs the resident wrappers, restores all three native
slots, and records before, installed, and restored snapshots:

```powershell
C:\scratch\xivl-decomp-raw-build\Release\raw_recorder_check.exe `
    --native-install-restore-check `
    --engine C:\Windows\SysWOW64\dbgeng.dll `
    --ntdll C:\Windows\SysWOW64\ntdll.dll `
    --output C:\scratch\raw-recorder-native-install-restore.json
```

The command never attaches a target and never calls native wait, continue, or
converter functions. A zero return proves the exact install and restore
snapshots plus idle read-only/count-zero cleanup. Return `2` means a profile,
precondition, clean transition, or output failure without ambiguous cleanup;
return `3` means cleanup became unconfirmed after mutation. If receipt writing
fails, the command reports that failure on stderr and preserves return `3`
when cleanup is ambiguous. The output then records the resident interface
retention and install reports. An output failure leaves no receipt proving
the round trip.

Deactivation first switches cached wrappers to terminal forward-only mode and
waits for the single in-flight admission reservation and any coverage-gap
publication to finish before rows are serialized. Cleanup uses the same
terminal step before retaining state after a detach or restore ambiguity.

The trace command accepts `--raw-recorder` only with `--fixture` and an
explicit absolute `--ntdll` path. It performs the same no-target profile check
before opening the target, installs the resident wait and continue wrappers
after `DebugCreate` and before `AttachProcess`, captures raw rows, detaches,
and restores the exact native slots before normal teardown. Raw rows are
serialized after detach with `raw_` kinds, including copied exception bytes,
context snapshots, and named coverage-gap rows. A restore ambiguity retains
the resident originals, module pins, and detached DbgEng interface
references and reports failure.
Raw mode accepts `--seconds 1..3`; the existing non-raw fixture and retail
profiles retain their 1..30 duration range.

Failed captures retain admitted raw rows after the restore guard has completed
and recording has stopped. `raw_cleanup` records restoration, remaining slot
ownership, ambiguity, error and loaded bases separately from data coverage.
A true `raw_summary.coverage` does not qualify a failed hook observation or
unconfirmed cleanup. Serialization is attempted once so a later output error
cannot append duplicate raw rows.

The accepted file identities are PE32 I386 DbgEng SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631` and ntdll
SHA-256 `7e15bd30890e9bf93b47fc894a68b2445618ee7528eb39584a263b21e112f9df`.
Preferred image bases are checked as file identity only; slot addresses use
the loaded module bases. The shared lease profile covers `.mrdata` RVA
`0x5A8000`, length `0x1BE4`, and its two-page footprint.
Every lease snapshot preserves raw `VirtualQuery` protection values. It
requires both `.mrdata` pages to be exactly `PAGE_READONLY` at idle or exactly
`PAGE_READWRITE` during the tiny lease, with no `PAGE_NOCACHE`, write-combine,
or other modifiers. The SRW lock at `0x597020`, count at `0x597014`, and saved
protection at `0x59702C` are separately queried as committed `MEM_IMAGE` pages
whose allocation base is the loaded engine base; each must be exactly
`PAGE_READWRITE` before any lock or interlocked mutation. Read-only and native
profile JSON report their measured raw protections and ownership result.

## ABI and admission

The recorder uses the observed x86 stdcall shapes:

```text
wait     LONG(debug_object, alertable, timeout_pointer, state_pointer)
continue LONG(debug_object, client_id_pointer, continuation_status)
```

Each wrapper saves `FS:[0x34]` and `FS:[0xBF4]`, restores the incoming pair
before one original call, copies the returned status and error pair immediately
after that call, and restores the returned pair after bounded diagnostics. A
null original is never called. A disabled wrapper keeps resident originals and
forwards cached late calls. Nested or concurrent calls fail admission and
forward safely while marking coverage false. The in-flight admission value is
a nonblocking reservation; no SRW, recorder mutex, protection lock, heap
operation, log, or DbgEng call spans the original native call. Admitted wait
and continue rows also record the thunk return address and argument-stack base
from _ReturnAddress and _AddressOfReturnAddress at the wrapper boundary.

Only status exactly zero admits raw bytes. Timeout `0x102`, negative status,
and other positive status values retain the wait result without reading the
state pointer. Recognized lifecycle and module states copy the first 12 bytes
only. States 6, 7, and 8 copy the demonstrated `0x60`-byte ordinary
exception extent, then require parameter count at most 15, chance 0 or 1,
nonzero PID and TID, and an exception code other than
`0x40010006`, `0x40010007`, or `0x4001000A`. Unknown states, short or
unreadable buffers, and excluded codes set coverage false and do not borrow a
CREATE-event handle.

## Identity, context, and correlation

Lifecycle CREATE records allocate a raw generation keyed by the raw debug
object, PID, and TID. EXIT records retire only a matching active identity.
The recorder admits one immutable raw debug object for an observation. A
changed object is forwarded once, records a `debug_object_mismatch` coverage
gap, and cannot inherit another object's generation. An exception before a
matching CREATE retains an explicit unknown generation and cannot be promoted
to a qualified join. Header-only module load and unload events inherit the
uniquely matching active generation without interpreting their union; an
unmatched module lifetime remains unknown. A fresh `OpenThread` call uses
`THREAD_GET_CONTEXT |
THREAD_QUERY_LIMITED_INFORMATION` (`0x808`) and a non-inheritable handle. The
recorder requires matching nonzero `GetThreadId` and
`GetProcessIdOfThread`, records `GetThreadTimes` creation as an observation
rather than a unique identity, and closes every owned handle.
`GetThreadContext` starts with `CONTEXT_CONTROL |
CONTEXT_DEBUG_REGISTERS` and requires API success plus the complete returned
mask. Zero DR0-DR3, DR6, or DR7 values remain valid data. EXIT and module
events never trigger a context read.

Wait returns, continue entries, and continue results have separate entry-time
attempt IDs. Continuation rows retain the raw client-ID pointer plus its
decoded PID/TID, and include the debug object in their pending key. A negative
continue retains a pending event only when the pre-call key uniquely matches;
the result retains the event index found before the original call. An
unmatched or unknown negative continuation has no retained event and sets
coverage false. A nonnegative result clears only one pending event whose debug
object, PID, TID, and known lifecycle generation match the client ID. Mismatch,
duplicate, unsupported, or unknown-generation joins keep any existing pending
record and set coverage false. Callback records use the actual
Windows PID/TID and raw pending key while retaining the DbgEng engine
generation in a separate field; the two generations are never equated.
Callback rows carry the pending raw-event index explicitly. Address or
adjacency alone never establishes ownership.

Every attempted exception context read receives a bounded row, including
OpenThread, identity, GetThreadContext, and CloseHandle failures. Context
snapshots describe one fresh OS-visible read at the recorder boundary
while the returned raw state remains valid. They do not establish the producer
of an exception, a processor trap frame, WOW64 bypass coverage, or persistence
after an engine write.

## Slot protection model and coverage boundary

The slot lease requires descriptor and initializer readiness, count zero, saved
and current read-only protection, and all three native expected slot values.
The actual fixture path holds the shared SRW lock only for each complete tiny
two-page `.mrdata` transition, rechecks the exact three slots under the lock,
changes the page to read-write/count-one, publishes only wait and continue
wrappers, then returns immediately to read-only/count-zero before observation.
Restore starts from fresh read-only/count-zero owned wrappers, enters a second
tiny read-write/count-one interval, restores all three native values, and
returns to read-only/count-zero. Protection failure, wrong old state, foreign
slot values, and ambiguous count retain the resident originals, module pins, or
unknown state for later inspection.

The retained static evidence identifies the converter slot at DbgEng RVA
`0x5A85D8`, native continue at `0x5A85F8`, native wait at `0x5A8630`, and
the native exports at ntdll RVAs `0xCE640`, `0x7A910`, and `0x7B9E0`. The
public map-selection profile also establishes the actual wait call at RVA
`0x3DE6B6` with return at `0x3DE6B8` and conversion at `0x3DE6E6`, plus
continue at `0x3DE7F8` and return at `0x3DE7FA`; see the
[raw event ownership contract](map-selection-trace.md#raw-event-ownership-boundary).
Concurrent writers, bounded-storage overflow, context or identity failures,
callback joins, and late cleanup ambiguity set coverage false and are reported
as raw coverage gaps.

## Fixture evidence and remaining limits

The bounded synthetic fixture observations on 2026-10-03 used the pinned
DbgEng and ntdll images above. The TF calibration recorded actual wait and
continue wrapper return addresses at engine RVAs `0x3DE6B8` and `0x3DE7FA`,
admitted ordinary exception bytes, successful fresh owned-handle context reads,
and uniquely matched continuation results. It recorded no breakpoint callback,
so it did not establish a complete callback chain or an exception producer.

The unchanged ordinary and VEH continue-search fixtures both failed hook
validation: a callback named the constructor breakpoint while the selected EIP
named the receiver. The VEH failure retained raw rows and confirmed native
slot restoration. Earlier callbacks joined uniquely, but the final callback
could not bind to a compatible pending raw event and marked coverage false.
The VEH first-chance exception was continued as not handled. The unchanged
handler logged continue-search at the same thread and address. The matching
second-chance event was continued as handled. Both fixtures exited normally after the
observer detached. Survival after the handled second chance does not qualify
the failed observation or survival after an unhandled second chance. The
ordinary failure produced no raw rows and cannot supply native event evidence.

The callback-ID/EIP disagreement and exception producer remain unresolved.
The context rows are Windows API snapshots at the wait boundary, rather than
processor trap frames. They do not cover later engine context writes or
WOW64/native bypasses. These fixture results do not qualify broader retail
thread support, a SetMap selection, resource requests, DAT opens or game
collision.
