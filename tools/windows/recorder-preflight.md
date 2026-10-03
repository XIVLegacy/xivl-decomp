# Raw event recorder prerequisites

`recorder_preflight` checks an own-process DbgEng profile, fake x86
wait/continue forwarding and an optional isolated protection operation.
It does not install a hook or attach to a target.
Loaded readiness and hook installation eligibility are separate results;
installation eligibility is always false.

## Supported profile and commands

The module identities and native call locators are in
[Raw event ownership boundary](map-selection-trace.md#raw-event-ownership-boundary).
The tool accepts only those pinned PE32 I386 DbgEng and ntdll images. It checks
the actual loaded module identities and bases rather than treating preferred
image bases as runtime addresses.

Build with the [Win32 MSVC configuration](map-selection-trace.md#build-and-fixture-verification)
into a fresh external output directory. This target enables `/guard:cf` in
both compilation and linking. Run the two independent modes explicitly:

```powershell
C:\scratch\recorder-build\Release\recorder_preflight.exe --self-test
C:\scratch\recorder-build\Release\recorder_preflight.exe `
    --engine C:\explicit\dbgeng.dll --ntdll C:\explicit\ntdll.dll `
    --output C:\scratch\recorder-engine.json
```

The output path must be new. Omitting `--output` writes JSON to stdout. Neither
mode creates or attaches to a target, calls the native wait/continue routines,
or writes engine slots. The plain loaded mode calls documented `DebugCreate`
once and inspects state while that client exists; it makes no direct protection
request. `DebugCreate` initialization can resolve slots and change protection
through the engine's own helpers. The optional `--lease-check` changes only its own loaded
engine's protection and shared lease fields.

The asset-free mode checks the resident ntdll file's pinned PE/hash before
writing its own thread's profile-specific error fields. It tests fake original
calls through compiled indirect
wrappers, return values, unchanged argument words and pointers, output writes,
both incoming and returned error values, and synthetic diagnostic failure
paths. Synthetic slots test expected-value publication, second-slot rollback,
owned restoration and a cached wrapper call after slot restoration. Wrapper
code, saved fake originals and state remain resident. These are isolated
mechanics tests, not engine interception or concurrent-event coverage.

The executable checks enabled process CFG and its wrapper entries in its own
FID table. Compiled metadata and successful fake indirect calls establish
those call paths only. They do not establish a future DbgEng call to a wrapper.
[Microsoft's CFG documentation](https://learn.microsoft.com/en-us/windows/win32/secbp/control-flow-guard)
distinguishes compiler/linker instrumentation from runtime call-target checks.

## Conditional stop and raw input

[Microsoft's debugging-event contract](https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events)
states that notification suspends the affected process's threads until the
debugger continues the event. Matching PE32 I386 `KernelBase.dll`, SHA-256
`85ad413ff860898f9e511f2e63c0e30f7aed1275fd1fd44fa90801211d328253`,
was inspected with LLVM `llvm-objdump 22.1.4`. Its common wait at RVA
`0x24DB3C` calls ntdll `DbgUiWaitStateChange` through IAT RVA `0x28D91C`
at `0x24DB67`. Results `0x101` and `0xC0` retry; negative results and
`0x102` fail. Other nonnegative results reach conversion. The ordinary
converter call at `0x24DBC0` uses IAT RVA `0x28D924`.

In the pinned ntdll, `DbgUiWaitStateChange` at RVA `0xCEA60` calls the
native wait at `0xCEA79` with the TEB debug object, alertable value 1,
caller timeout pointer and caller state pointer. DbgEng uses its own debug
object and alertable value 0. This matching native consumer supports a
conditional stopped-event design. It does not establish a stopped context
read in an instrumented session.

A narrow decoder can require native status exactly zero and raw states
6, 7 or 8 on the ordinary exception branch, excluding special conversion
codes `0x40010006`, `0x40010007` and `0x4001000A`. The converter's demonstrated
ordinary input extent is `0x60`; validate the first-chance word and parameter
count before interpreting it. This proposed strict-zero rule is narrower
than DbgEng's actual nonnegative, non-timeout conversion predicate.
Timeout/error buffers and unsupported state unions supply no valid event
payload. Lifecycle headers need their own proven minimum extents and join.
No decoder or raw event/context correlation is implemented by this tool.

## Dual error state and ABI

LLVM disassembly of the pinned ntdll establishes two current-thread x86
fields. `RtlGetLastNtStatus` at RVA `0x73CA0` reads `FS:0xBF4`;
`RtlGetLastWin32Error` at `0xF9580` reads `FS:0x34`.
`RtlNtStatusToDosError` at `0x61EB0` stores its input status at TEB
`+0xBF4` at `0x61EFE`. The shared `RtlRestoreLastWin32Error` /
`RtlSetLastWin32Error` body at `0x62030` stores only `+0x34` at `0x62052`
and has optional notification work. Preserving LastError alone is insufficient.

The fake wrappers save both fields before diagnostic work, restore the
incoming pair before calling the original once, capture its returned pair
immediately, and restore that pair after diagnostics. Direct FS access is a
pinned x86 profile detail, not a portable Windows ABI claim. No target TEB is
written. The four-word wait signature retains debug object, alertability,
timeout pointer and output-state pointer; continue retains debug object,
client-ID pointer and continuation status. Compiled inspection must confirm
stdcall stack cleanup, nonvolatile register preservation and the indirect
calls in addition to the behavioral self-test.

## Cache lifetime and installation boundary

LLVM `llvm-readobj` and `llvm-objdump 22.1.4` inspected the pinned DbgEng.
`DebugCreate` at RVA `0x1AFBB0` reaches `DebugCreateEx` at `0x1AFAD0` and
initializer `0x1AF0DD`. The full owned initializer path resolves descriptors
before publishing global marker RVA `0x596924` at `0x1AF5D9`;
an already-completed branch can return earlier. A successful call therefore
does not prove that this call freshly populated every slot or that later
cleanup/re-resolution is excluded.

The indexed resolver's per-descriptor state is different: it stores 1 at
`0x3D20BB` before the pointer loop. The plain slot store at `0x3D20E1`
publishes the raw `GetProcAddress` result. The native descriptor state is
RVA `0x5A9B90 + 0x14 = 0x5A9BA4`; its three relevant slot targets are
checked by the preflight against actual loaded ntdll exports.

The file places those slots in read-only initialized `.mrdata`, RVA
`0x5A8000`, virtual size `0x1BE4`. Engine helper `0x3C96C0` acquires the
SRW lock at RVA `0x597020`, increments the counter at `0x597014`, and on
the first contribution uses section helper `0x3C963E` and `VirtualProtect`
at `0x3C970D` to request `PAGE_READWRITE`, saving old protection at
`0x59702C`. Failed protection removes that contribution. It releases the
SRW lock at `0x3C9720` before returning. Release helper `0x3C9733` acquires
the same lock, decrements the count, restores the saved protection on the
last release at `0x3C977F`, and releases the lock at `0x3C978E`.

The resolver enters at `0x3D202A`, calls imported `RtlRunOnceExecuteOnce`
through IAT `0x5A37D8` at `0x3D2049`, enters again at `0x3D2054`, and
releases one contribution at `0x3D2067`. It then loads the module at
`0x3D208E`, publishes state and pointers, and releases the remaining
contribution at `0x3D2118`. Callback `0x3D2190` resolves and touches the
section without entering or releasing this lease. The counted contribution
keeps the section writable across the known main-path publication; the
SRW lock supplies no mutual exclusion for that slot loop.

Cleanup `0x3D212B` can zero slots at `0x3D215D`, release the loaded module
at `0x3D2168`, and clear descriptor state at `0x3D2173`. The inspected
failure and teardown paths do not establish a complete cleanup exclusion or
global-reset invariant for a future observer. Retaining a module reference
and resident wrapper state addresses unload/late-call memory lifetime; it
does not prove the engine retained the wrapper pointer or coverage.

The resolver's unwind registration at `0x3D2009` supplies handler
`0x4F2D6C`. Its FuncInfo at `0x51FBD4` points to unwind map `0x51FBF8`
with action `0x4F2D5F`; that action takes frame local `EBP-0x18` through
`0x3C961F` to release helper `0x3C9733`. The local receives the known
section address at `0x3D2051`. This closes that direct unwind release
edge; it does not establish every possible exceptional exit.

An independent `VirtualProtect` interval is insufficient: restoring a page
while an engine lease is active could make an engine writer fault. A possible
participant in the proven shared discipline still needs the relevant
writer's lease and global lifetime established, plus tested acquisition,
failure and balanced release.
[VirtualProtect's old-protection result](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect)
describes the first page. Restoring a whole section from one DWORD requires a
supported uniform protection profile across its complete page span; mixed,
guarded or executable pages cannot be silently flattened. The read-only
preflight reports the complete `.mrdata` page profile without changing it.

## Isolated protection check

The optional mode creates a fresh single client in its own process:

```powershell
C:\scratch\recorder-build\Release\recorder_preflight.exe `
    --engine C:\explicit\dbgeng.dll --ntdll C:\explicit\ntdll.dll `
    --lease-check --output C:\scratch\recorder-lease.json
```

It requires the pinned loaded identities, exact slot targets, descriptor and
initializer state 1, lease count zero, saved protection `PAGE_READONLY`,
flag-free writable helper pages and two uniform read-only `.mrdata` pages.
Busy locks, nonzero counts and unsupported profiles are refused before
mutation. The exact section length remains `0x1BE4`; protection affects its
two-page footprint.

The check uses documented `TryAcquireSRWLockExclusive` and holds the lock
across its complete first/last transition. Fixed snapshots and standard
memory/error primitives run under the lock; engine APIs, reentry, logging,
allocation and file output do not. It temporarily contributes count 1,
requests writable protection, then restores read-only protection and count
zero. The three slot values, descriptor, initializer and saved protection
must remain unchanged. This is an isolated operation with an idle engine;
it does not test an ongoing hook interval or overlapping engine writers.

Failure output distinguishes refusal, protection failure, unexpected state
and unconfirmed restoration. A retained contribution must be paired with
verified writable pages; client and module ownership then remain resident
through process exit. A successful API restoration with an unexpected old
protection value is a failed acceptance check, with its actual cleanup state
reported separately. Synthetic tests exercise modeled page profiles and
injected protection failures; they do not establish a real API failure in
the engine.

On 2026-10-03, the pinned Win32 build passed 576 synthetic checks with enabled
CFG and both wrapper FIDs. A fresh own-process loaded profile check succeeded.
The separate own-engine lease operation observed both pages as RO/RW/RO and
count 0/1/0, with saved protection 2, descriptor and initializer 1, and all
three native targets unchanged. Restoration was confirmed and no contribution
remained. These results establish the isolated cycle only. No real protection
failure, engine-to-wrapper call, native event or target capture was exercised.

Readiness is an observation at one instant. Future installation must preserve
unresolved engine bytes, validate every relevant loaded page and target,
coordinate protection changes with the engine's actual writer discipline,
and install/restore only expected or owned pointer values. Saved originals,
wrapper code, state, slot storage and modules must remain resident through
late calls. Loss of slot ownership or ambiguous event/continuation joins
invalidates coverage even if the observer remains memory-safe.

The isolated check does not establish ongoing protection participation, raw wait/continue
interception, event admission, lifecycle correlation, stopped context reads,
all context-write paths, or actual continuation outcomes. These remaining
edges must be demonstrated before another bounded fixture capture. Retail
thread policy remains `initial_threads_only`. Selector, resource-open,
authored-scene, rendering and walking acceptance remain separate.
