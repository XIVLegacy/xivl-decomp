# Callback-delayed query identity and timing

This finding defines the smallest offline acquisition record that can correlate
an adapter-collected translation-query interval with a DbgEng breakpoint
callback delivered after raw wait admission. The binding must already have
exited before query entry, and the raw event must remain open until the query
interval closes. The pinned bytes do not establish that order for the static
caller candidate at `0x1E49EB`, so no observed row can be labeled as that
callsite without a separate qualified mechanism.

The existing [engine event binding](engine-event-binding.md) owns the raw wait
boundary, selected-state ABI, observer lifecycle distinction, and deferred
binding semantics. The [observer diagnostic](observer-diagnostic.md) owns the
query output provenance and immutable interval rules. This page adds the
bounded query and callback xrefs needed to apply those contracts to the
selected RVAs.

## Input and method

The input was the PE32 I386 `C:\Windows\SysWOW64\dbgeng.dll`, 6,097,408
bytes, SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631`, with
image base `0x10000000`. The bytes were read without loading DbgEng, calling
`DebugCreate`, attaching to a process, querying a target, or running a
fixture.

A bounded Python 3.12.0 script used Capstone 5.0.7 in x86/32-bit mode with
instruction detail and `pefile` 2024.8.26. It disassembled the raw `.text`
section, retained bounded decoded direct relative `call` candidates for
`0x1E4932`, `0x170A60`, and `0x19A481`, emitted eight-instruction and
twelve-instruction caller windows,
and checked exact DWORD table pointers. LLVM `22.1.4` independently decoded
the focused query, flush, selected-breakpoint, and callback-dispatch windows.
The SDK inputs were Windows SDK `10.0.26100.0` `DbgEng.h`, SHA-256
`3daa5d6aebbfca3aefcee6fd0b5b34abd2eebf2a0313facb3f5939da6ae8defa`, and
`basetyps.h`, SHA-256
`48683511f47520707530d6e660d30ff9ac3c6255d976c8b004e28d17b53b5098`.
These versions and hashes match the ABI record in
[engine-event-binding.md](engine-event-binding.md#selected-state-abi-and-virtual-calls).

Source observations of `trace_map_selection.cpp` are pinned to host revision
`eaf38e8d1b6dae049705e4c23c72c8a679dfdd91`; the source links below use that
revision's line numbers. The current source file may carry pointer-only edits
after that baseline.

## Static route witnesses

The bounded decoded `.text` scan found four direct call candidates for the
main helper at `0x1E4932`: `0x1E4921`, `0x170A90`, `0x24EF92`, and `0x2701EA`.
It found no decoded direct-call candidate for the flush method at `0x170A60`.
That is a bounded membership result, not an exhaustive absence claim. An exact
DWORD at `0x47BC8`, which is `0x47B8C + 0x3C`, contains `0x10170A60`; this is
an indirect table candidate, not a runtime edge.

The bounded flush window provides this local order:

```text
0x170A90: call 0x101E4932
0x170A95: mov esi,eax
0x170A97: test esi,esi
0x170A99: jne 0x10170AFE
0x170AC0: call dword ptr [0x105A380C]
0x170AC8: call edi
0x170AF6: call 0x1019EB3F
```

The helper forms the service and interface arguments before the selected
translation query:

```text
0x1E49D8: push 0x1009C370
0x1E49DD: push 0x10089690
0x1E49E2: push esi
0x1E49E5: call dword ptr [0x105A380C]
0x1E49EB: call edi
0x1E49ED: test eax,eax
0x1E49EF: js 0x101E4ACC
```

The instruction labels above are RVAs. The branch operand is the Capstone VA
`0x101E4ACC`, whose RVA is `0x1E4ACC`.

The service and IID pointer identities and the later commit call are described
in [breakpoint-context-path.md](breakpoint-context-path.md#separate-commit-interfaces).
The `0x1E49EB` call is indirect through the resolved table pointer in `EDI`;
its returned `EAX` is tested and a negative result branches to the fallback at
`0x1E4ACC`. On the path that also passes the later query and buffer checks,
the helper creates the separate buffer, then calls the first returned
interface at `0x1E4AB6`. These are instruction and branch observations, not
proof of which implementation supplied the interface.

The selected-breakpoint routine at `0x19A481` has one direct call xref in the
bounded scan, at `0x1955A2`. It receives the selected object in `EDX` at
`0x19A48C`, stores table `0x4A058` in a local dispatch record at `0x19A54F`,
and calls `0x199FEA` at `0x19A559`. The first DWORD at table `0x4A058` is
`0x10199BB0`, which is a static dispatch-table witness.

The callback wrapper at `0x199BB0` loads the client callback interface from
`+0xF4` at `0x199BD1`, loads vtable slot `+0x10` at `0x199BDC`, and calls
that pointer indirectly at `0x199BE7`:

```text
0x199BD1: mov ecx,dword ptr [ecx+0xF4]
0x199BD7: push esi
0x199BD8: push edx
0x199BD9: push ecx
0x199BDA: mov eax,dword ptr [ecx]
0x199BDC: mov esi,dword ptr [eax+0x10]
0x199BDF: mov ecx,esi
0x199BE1: call dword ptr [0x105A380C]
0x199BE7: call esi
```

The SDK slot table identifies `+0x10` as `IDebugEventCallbacks::Breakpoint`
after the three `IUnknown` slots and `GetInterestMask`. The static path
therefore establishes callback-object delivery at this call site. It does not
establish that the selected callback ran for the same raw exception as the
translation query, or that the `0x4A058` table was selected at runtime. The
public callback and selected-state boundaries are also recorded in
[breakpoint-context-path.md](breakpoint-context-path.md#callback-delivery).

## Smallest acquisition record

The record uses the public SDK interfaces and the retained raw key. It does
not replace any raw field with the observer's target input or with an inferred
counter.

1. At raw wait admission, retain the exact pending key before conversion:
   debug object, raw Windows PID, raw Windows TID, raw generation, and the
   zero-based raw event index. Preserve the raw status and recognized-state
   result with their actual status values. This is the pre-conversion seam in
   [engine-event-binding.md](engine-event-binding.md#native-wait-chain).
2. A callback/session owner must serialize the selected-state operations for the
   entire acquisition, including the raw-key snapshots, SDK reads, callback
   ownership check, and binding decision. It must retain the authority
   identifier, read interval, and source lifetime identifier for the selected
   state. Equal sequential getter IDs and a raw-key recheck are consistency
   checks only; they are not an atomic snapshot or lifetime proof. The
   callback-specific cached ID, lifecycle token, and cached raw lifecycle tuple
   (debug object, raw PID, raw TID, and raw generation) must come from that
   same lifecycle owner, not from an arbitrary cache. The zero-based event
   index remains the event join key and is not part of the lifecycle tuple.
3. On the proposed native `Breakpoint` callback-entry reader, snapshot the
   pending raw key before any SDK or breakpoint-object reads. This reader is a
   proposed callback-entry mechanism; the current post-wait `raw_callback`
   record is not native callback entry. Obtain `IDebugSystemObjects` from the
   existing client and retain the actual `QueryInterface` `HRESULT` and
   output-known flag.
4. Initialize every output to its unknown sentinel before each call. Read and
   retain each `HRESULT`, raw output, and known flag for
   `GetCurrentThreadId`, `GetEventThread`, `GetCurrentProcessId`,
   `GetEventProcess`, `GetCurrentThreadSystemId`, and
   `GetCurrentProcessSystemId`. A failed `HRESULT` leaves the output unknown,
   even if the buffer contains a value. A value is known only when its call
   succeeded and its output is usable. Engine IDs equal to `DEBUG_ANY_ID` are
   invalid; system IDs equal to zero are invalid. The SDK distinguishes engine
   thread IDs from system thread IDs, so neither class may be substituted for
   the other.
5. Retain the callback-specific cached engine thread ID and its observer
   lifecycle token from the same callback/session owner and lifecycle. Accept
   the deferred witness only
   when current, event, and cached engine thread IDs are known, equal, and not
   `DEBUG_ANY_ID`; current and event process engine IDs are known, equal, and
   not `DEBUG_ANY_ID`; current system PID/TID are nonzero and exactly equal to
   the raw PID/TID; and the lifecycle token is separately supplied and
   nonzero. The token is observer ownership metadata, not a native DbgEng
   generation.
6. Recheck the pending raw key after all reads. Bind only if the key is still
   pending and exactly unchanged. Record every refused, stale, closed, or
   incomplete attempt. A changed or closed key cannot be repaired from equal
   PID/TID values, neighboring rows, or the callback object address.
7. For the selected profile, collect query entry and return through the
   existing QueryService adapter at `0x468B10`, using only the permitted
   profile boundaries `0x467F13`, `0x468B10`, and `0x3D049D`. Retain the
   adapter `HRESULT` and output-slot state. After a successful query, retain
   the returned interface, its vtable, and slot `+0x10` read results with
   their actual read status. Finish the query row only after those reads and a
   recheck that the exact raw key is still open. Leave pending raw-event
   closure exclusively to successful continuation. The
   [query-bound provenance contract](observer-diagnostic.md#query-bound-provenance)
   owns these output and resident-image status rules. RVA `0x1E49EB` is a
   static caller candidate only; the current profile has no runtime caller
   field or qualified probe that can label an observed adapter row as that
   callsite.

The immutable source observation shows why the observer token must remain
separate. At revision `eaf38e8d1b6dae049705e4c23c72c8a679dfdd91`, the
post-wait callback record reads current and event engine thread IDs at
[`record_raw_callback`](trace_map_selection.cpp#L2721), but stores
`options.pid` as its process field, leaves `debug_object` unresolved, and sets
the callback thread generation to unknown while copying the observer generation
separately. The native callback entry is [`Events::Breakpoint`](trace_map_selection.cpp#L735),
but the current `record_raw_callback` invocation at
[`wmain`](trace_map_selection.cpp#L3162) occurs after `WaitForEvent` returns;
it is not a native callback-entry reader. `CreateThread` allocates a token only
when that callback runs at [`Events::CreateThread`](trace_map_selection.cpp#L791).
The main wait loop consumes the lifecycle row at
[`wmain`](trace_map_selection.cpp#L3076) before it reaches the post-wait
breakpoint observation. That later lifecycle allocation has no current raw-
callback join. The callback/session owner must therefore supply a token already
associated with the selected callback identity and retain its authority and
lifetime evidence, or the binding remains refused.

## Offline reader and shared-clock record

The concrete offline implementation is `CallbackIdentityReader` in
`observer_callback_identity.h`. Its `read` method accepts a borrowed
`IUnknown`, queries `IDebugSystemObjects` through the Windows SDK declaration,
and records the six SDK getters in the order listed above. Its `capture` method
composes that evidence with `Recorder` callback entry, acquisition and exit
rows and may call `RawEventBridge::bind_engine_event` only after the exact raw
key recheck and the supplied owner witness pass. The recorder mutex covers the
pending-key recheck and row-capacity publication, while callers serialize the
callback, raw-event, binding and continuation operations.

The acquisition row is the bounded source record: it contains the entry-linked
callback operation, a shared-clock begin/end pair, both raw-key snapshots, the
actual `QueryInterface` result and release evidence, every SDK HRESULT/output
status, owner authority and lifetime fields, and an explicit outcome. A
successful SDK read does not supply an owner token. A failed read, changed or
missing raw key, missing owner evidence, cleanup exception or refused binding
is retained with its actual evidence and remains incomplete; preflight refusals
are recorded before any SDK read with the acquisition caller's observer thread
and canonical not-attempted slots for all six SDK methods. Callback entry
does not gain an engine generation after the fact, and a missing callback exit
remains incomplete.

`observer_runtime_check --callback-output` and the C++ fake COM tests exercise
raw admission, callback entry, all six SDK reads, deferred binding, a nested
post-bind query, a context write, callback exit and continuation closure. The
output is
synthetic forwarding evidence only. It does not load DbgEng or establish native
callback entry, lifecycle allocation, selected-state serialization or live
event identity; those remain required before any native coverage claim.

## Interval decision

Callback-delayed binding can support event correlation for a later adapter
query interval under the selected profile. It cannot qualify a query that began
before binding, even if the query returns after binding, and a query crossing
the binding transition is incomplete. Successful continuation closes the exact
pending raw key and prevents later binding; the immutable-row rules are in
[observer-diagnostic.md](observer-diagnostic.md#raw-event-bridge-and-delayed-binding).

The static evidence proves only the local call order within the flush/query
route and the separate selected-breakpoint-to-callback dispatch candidate. It
provides no edge ordering `0x1E49EB` against `0x199BE7`, no runtime event
identity, and no proof that either indirect table route executed for one event.
The missing
evidence is a runtime record carrying one exact raw key through adapter query
entry and return, native callback entry and exit, followed by the exact
continue or closure result. Until that record exists, an observed adapter row
must remain raw or unqualified under `query-output-identity-v1`; it cannot be
attributed to `0x1E49EB` from static bytes alone.

No native provider or native callback-entry reader exists in this design.
Synthetic delayed-binding checks validate interval admission and immutable-row
behavior only; they do not establish native ordering or engine identity.

## Limits

- Direct call xrefs are bounded decoded candidates, not an exhaustive absence
  scan. Exact DWORD table pointers are bounded static observations. Indirect
  calls and tables remain candidates, not runtime edges.
- The SDK methods identify selected and event IDs through the public ABI. They
  do not expose a native DbgEng generation counter or prove a raw wait to
  selected-state join.
- `options.pid` is an observer input and cannot stand in for
  `GetEventProcess` or `GetCurrentProcessId`.
- Equal addresses, PID/TID values, raw generation values, or nearby serialized
  rows do not supply the missing callback ownership or timing edge.
- No native execution, debugger attachment, target query, context query, or
  capture is part of this finding.
