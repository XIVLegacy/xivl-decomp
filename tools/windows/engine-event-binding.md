# DbgEng wait-to-selected-state binding

This finding pins the static boundary between the native raw wait and the
DbgEng state that a client can read through `IDebugSystemObjects`. It supports
capturing the raw wait result and the converted event header. It does not
establish a native byte offset or a direct native join from that tuple to the
engine-selected process, thread, or lifecycle generation.

## Pinned input and method

The input is the PE32 I386 `dbgeng.dll` at `C:\Windows\SysWOW64\dbgeng.dll`,
size `6097408` bytes, SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631`. The
image was read as file bytes only. No DbgEng library load, engine creation,
attach, process query, context query, native-memory write, or fixture run is
part of this finding.

The bounded Capstone disassembly used Capstone `5.0.7` with
`CS_ARCH_X86`/`CS_MODE_32` and instruction detail enabled. The focused
wait/continue ranges were RVA `0x3DE550:0x330` (through `<0x3DE880`) and
`0x3DE790:0x100` (through `<0x3DE890`). RVA-to-file mapping used each
file-backed section's `PointerToRawData + (RVA - VirtualAddress)` when
`VirtualAddress <= RVA < VirtualAddress + max(VirtualSize, SizeOfRawData)`.
The full raw `.text` section was disassembled with Capstone detail and
skip-data enabled;
non-text sections were scanned as little-endian DWORDs for runs of at least
three values inside the image's `.text` VA range.

The exploratory binding candidates were direct `call` operands targeting the
`.text` range, memory operands with displacements in
`{0x144,0x148,0x14C,0x15C,0x178,0x19C,0x1A0,0x1A4}`, a four-instruction
`mov eax,[ecx+field]` / `mov [edx+field],eax` / `xor|mov` / `ret` getter
sequence, and constructor-style `C7` stores whose immediate target was inside
`.text`. For those stores, immediate offsets were `+2` for `C7 01`, `+3` for
`C7 41`, and `+6` for `C7 81`. These are exploratory text-VA candidate rules,
not an exhaustive vtable or native-object scanner. An independent disassembly
used `C:\Program Files\LLVM\bin\llvm-objdump.exe`, LLVM `22.1.4`, with image
VA `0x10000000`:

```text
llvm-objdump.exe -d --x86-asm-syntax=intel --start-address=0x103DE550 --stop-address=0x103DE880 IMAGE
```

The public claims depend on the pinned image digest, PE32/I386 mode, image VA,
the exact ranges and candidate rules above, the Capstone settings, the LLVM
command, and the SDK header hashes. The SDK table was derived by enumerating
the `STDMETHOD` declarations for `IDebugSystemObjects` and
`IDebugEventCallbacks` after the three `IUnknown` slots, and by resolving
`STDMETHODCALLTYPE` from `basetyps.h`. The input image is not copied into this
repository.

## Native wait chain

The existing [map-selection trace](map-selection-trace.md#raw-event-ownership-boundary)
establishes the resolver and the wait/continue ownership boundary. The direct
instructions in the pinned image add the following bounded chain:

| RVA | Static observation | Supported meaning |
| --- | --- | --- |
| `0x3D2007` | Indexed resolver walks the name/flag descriptor at `0x5A9B90` and slots at `0x5A85C8`. | The converter, continue, and native wait slots are indices 4, 12, and 26: `0x5A85D8`, `0x5A85F8`, and `0x5A8630`. |
| `0x3DE6B6` | The wait routine calls the function currently held in slot `0x5A8630`. The four pushed values are object `+0x15C`, zero, a local timeout pointer, and a local state-buffer pointer. | The observed native wait ABI is `LONG(debug_object, alertable, timeout_pointer, state_pointer)`. The return in `EAX` is still the raw native status at `0x3DE6B8`. |
| `0x3DE6BA` and `0x3DE6C6` | The status is compared with `0x102`, then tested for a negative value. | Timeout and negative results bypass conversion. Other nonnegative results reach the converter. |
| `0x3DE6E6` | The function in converter slot `0x5A85D8` receives the local state pointer and the caller event buffer. | Conversion is a later step than the raw wait return. Its result is checked before the event buffer is consumed. |
| `0x3DE715` and `0x3DE71E` | The converted event buffer is read at `+4` and `+8`. | Those two DWORDs are copied to owner fields `+0x148` and `+0x14C` at `0x3DE718` and `0x3DE721`. The existing converter evidence identifies them as `DEBUG_EVENT.dwProcessId` and `dwThreadId`. |
| `0x3DE733`, `0x3DE74A`, `0x3DE756`, and `0x3DE768` | Branches inspect event codes `3`, `5`, and `6`, close the converted `+0xC` word on the `+0x144` path, call helpers `0x3E1790` and `0x3E17E7`, and may call the `ResumeThread` import. | These are internal event/resource branches. They do not identify an `IDebugSystemObjects` object or prove that an adjacent PID/TID is the engine-selected identity. |
| `0x3DE6FD` | A nonzero conversion/error path calls helper `0x3E16C8`, which traverses internal lists and object slots at `+0x40` and `+0x58`. | This is a bounded failure path. The inspected slots have no supported selected-process/thread binding. |
| `0x3DE7F8` | The continue slot `0x5A85F8` receives object `+0x15C`, a pointer to the two words copied from `+0x148/+0x14C`, and the caller status. | A negative result leaves pending `+0x148` intact; a nonnegative result clears it at `0x3DE864`. The helper at `0x3D0DE4` passes the pair to `ContinueDebugEvent` as PID, TID, and status. |

The smallest native observation boundary is the raw wait return at
`0x3DE6B8`, before conversion at `0x3DE6E6`. A zero status admits the
recognized state branches; `0x102`, negative, and other positive statuses are
retained as statuses and are not raw events. Converter reads establish only
the bytes consumed by each branch. They do not establish an all-state
initialized-byte guarantee or a full union extent.

The pending pair is a continuation key used by this internal object. It is
not an engine thread ID pair, an event-generation counter, or proof that the
native wait event and the later selected state are the same object.

## Selected-state ABI and virtual calls

The Windows SDK 10.0.26100.0 `um/DbgEng.h` declares `IDebugSystemObjects`
with `IUnknown` slots 0 through 2 and four-byte PE32 vtable entries.
`STDMETHODCALLTYPE` is `__stdcall` in the matching `basetyps.h`. The header
hashes used for this table are:

```text
DbgEng.h   3daa5d6aebbfca3aefcee6fd0b5b34abd2eebf2a0313facb3f5939da6ae8defa
basetyps.h 48683511f47520707530d6e660d30ff9ac3c6255d976c8b004e28d17b53b5098
```

The relevant interface slots and current source use are:

| Interface slot | SDK method | Meaning available through the public ABI | Current source use |
| --- | --- | --- | --- |
| 3 | `GetEventThread` | Engine ID of the thread on which the last event occurred. | [`read_event_thread`](trace_map_selection.cpp#L480) reads it; [`record_raw_callback`](trace_map_selection.cpp#L2649) reads it again. |
| 4 | `GetEventProcess` | Engine ID of the process on which the last event occurred. | No call in the bounded source path. A selected engine process ID is therefore missing. |
| 5 | `GetCurrentThreadId` | Current implicit engine thread ID. | [`current_thread_identity`](trace_map_selection.cpp#L452) and [`record_raw_callback`](trace_map_selection.cpp#L2649) read it. |
| 7 | `GetCurrentProcessId` | Current implicit engine process ID. | No call in the bounded source path. |
| 13 | `GetCurrentThreadDataOffset` | Offset of the current thread system data structure; in user mode this is the current TEB offset. | [`current_thread_identity`](trace_map_selection.cpp#L452) reads it. |
| 15 | `GetCurrentThreadTeb` | Offset of the current thread TEB; in user mode it is equivalent to the data offset. | [`current_thread_identity`](trace_map_selection.cpp#L452) reads it. |
| 17 | `GetCurrentThreadSystemId` | System TID for the current engine thread. | [`current_thread_identity`](trace_map_selection.cpp#L452) reads it. |
| 27 | `GetCurrentProcessSystemId` | System PID for the current engine process. | No call in the bounded source path. |

The `IDebugEventCallbacks` declaration places `Breakpoint` at slot 4,
`Exception` at slot 5, `CreateThread` at slot 6, and `ExitThread` at slot 7.
The source registers its callback object at
[`trace_map_selection.cpp:2835`](trace_map_selection.cpp#L2835), before
`AttachProcess`, and reaches the first `WaitForEvent` at
[`trace_map_selection.cpp:2840`](trace_map_selection.cpp#L2840).
The SDK header documents that event callbacks are delivered when the client
thread calls `WaitForEvent` or `DispatchCallbacks`; the header does not expose
the native storage member that carries the selected IDs.

The callback implementation obtains selected thread fields through the
`IDebugSystemObjects` calls above. `Events` now delegates its existing
`ThreadIdentity` matching and lifecycle vector to the shared
`ObserverEventLifecycleCache`. Initial registration remains at
[`trace_map_selection.cpp:1024`](trace_map_selection.cpp#L1024), while
`CreateThread` and `ExitThread` use the cache at
[`trace_map_selection.cpp:727`](trace_map_selection.cpp#L727) and
[`trace_map_selection.cpp:746`](trace_map_selection.cpp#L746). The cache
starts its observer allocation domain at zero and allocates an initial or
created lifecycle token with `++next_generation` at
[`observer_event_lifecycle.cpp:279`](observer_event_lifecycle.cpp#L279) and
[`observer_event_lifecycle.cpp:292`](observer_event_lifecycle.cpp#L292).
This is an observer allocation domain, not a known DbgEng counter or native
object offset. The source hold revalidates the exact active cache entry before
each observation or SDK access and retains its shared storage until explicit
scope release. The source association remains separate from `ThreadIdentity`;
the cache does not claim that it observed a raw PID. The native callback
registration does not activate the prepared source.

The `CreateThread` token is allocated when that callback executes, after the
raw wait notification has already run. A pre-conversion provider therefore
cannot synchronously obtain it for the raw event; the callback record still
uses `generation = 0, known = false` for its thread key and carries the
observer token separately. A later callback or post-wait record can retain
that token beside the explicit raw event index and PID/TID checks, but it
cannot retroactively change the earlier `RawEventBridge` admission.

The current loop consumes a pending lifecycle event at
[`trace_map_selection.cpp:3004`](trace_map_selection.cpp#L3004) and resumes
execution before reaching the breakpoint path that calls
`record_raw_callback` at
[`trace_map_selection.cpp:3090`](trace_map_selection.cpp#L3090). Thus a
`CreateThread` callback currently has no raw-recorder callback join in this
path; adding one would be the
smallest future observation change, and it would still need the explicit raw
event index and retained keys described below.

Under the exploratory text-VA candidate rules above, the bounded byte scan did
not recover a supported native vtable address for `IDebugSystemObjects`, an
implementation function for slots 3, 4, 5, 7, 13, 15, 17, or 27, or a member
load that can be tied to the wait owner's `+0x148/+0x14C` fields. The SDK slot
table is therefore an ABI binding for later client observation, not a claim
about a native implementation layout.

## Timing and collection boundary

The raw recorder calls the original native wait, records its return, and
decodes/notifies the raw state before the outer wait routine reaches its
converter. In [`raw_event_recorder.cpp:2289`](raw_event_recorder.cpp#L2289),
the original wait returns; zero-status decoding begins at line 2320; the
observer notification is emitted at
[`raw_event_recorder.cpp:2365`](raw_event_recorder.cpp#L2365), and the wrapper
returns at [`raw_event_recorder.cpp:2369`](raw_event_recorder.cpp#L2369). The
raw notification therefore cannot synchronously ask DbgEng for the
post-conversion selected event state and treat that answer as a native join.

The defensible collection order is:

1. At raw wait return `0x3DE6B8`, retain the attempt ID, debug object, raw
   status, recognized raw state, and raw PID/TID when the strict admission
   rules pass.
2. After the engine has dispatched its callback or after the client resumes
   from `WaitForEvent`, read `GetEventThread`, `GetCurrentThreadId`, and the
   current thread system/data/TEB fields. Read `GetEventProcess` and the
   current process methods if process selection is required.
3. Join only through an explicit raw event index and lifecycle record, with
   the debug object and PID/TID checks retained. Engine IDs are observations
   that qualify the later callback; they are not replacements for the raw
   key.

The current [`record_raw_callback`](trace_map_selection.cpp#L2649) path
illustrates the seam limitation. It sets
`callback.thread = { options.pid, identity.system_id, 0, false }` and copies
`identity.generation` separately to `callback.engine_generation` in the
[`record_raw_callback`](trace_map_selection.cpp#L2649) body. The callback thread
generation is therefore explicitly unknown;
the DbgEng generation field is not filled from a native counter. The raw
recorder then joins by pending raw debug object/PID/TID plus equal current,
event, and cached engine IDs in
[`raw_event_recorder.cpp:2867`](raw_event_recorder.cpp#L2867). That join does
not backfill the earlier `RawEventBridge::raw_identity` admission or convert
the observer lifecycle token into native identity.

The default `RawEventBridge` mode still invokes its injected provider
synchronously while the raw notification is on the pre-conversion wait path.
The current API also supplies an explicit deferred mode: it admits the raw
tuple first, then `bind_engine_event` accepts a later injected
`EngineIdentityObservation` and separate nonzero lifecycle token. The bridge
records the attempt and `Recorder::bind_pending_event` upgrades only the exact
still-pending raw tuple. Raw admission and query rows are immutable; a query
that entered before binding remains unqualified even if its original returns
after binding. This preparation API validates witness consistency only and
supplies no native token offset or provider implementation. The source-level
implementation is anchored to `xivl-decomp@dc2990361f901976db158eb78964a6fff60a71a7`
for the prior static boundary; current line locations are maintained by the
implementation files.

The explicit event index is part of the exact pending-event key. Binding
history uses the lifecycle key (debug object, PID/TID and raw generation) and
therefore permits successive event indices in one lifecycle to retain the same
observer token, rejects a token change within that lifecycle, and rejects reuse
of that token by a different lifecycle. A replay of the exact closed tuple is
stale. Binding receipts preserve every refused attempt, and any refusal keeps
bridge coverage incomplete.

The [callback acquisition and query timing finding](callback-query-timing.md)
defines the required method-result record and acquisition authority. Its
bounded static query and callback routes do not establish their event-specific
order.

## Limits

- The raw state and converted `DEBUG_EVENT` header are statically bounded;
  runtime event delivery, callback ownership, and selected-state qualification
  are not established here.
- No native process/thread member offsets, native generation counter, or
  direct wait-to-`IDebugSystemObjects` vtable edge is supported.
- `options.pid` in the callback record is the observer's target process input;
  it is not evidence of `GetEventProcess` or `GetCurrentProcessId`.
- Address proximity, equal PID/TID values, the pending pair, and internal
  helper list adjacency do not establish event identity.
- The static RVAs identify analysis locations. They are not installable hook
  contracts, and this finding does not reconstruct the October 3 mismatch.
