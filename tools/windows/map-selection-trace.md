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

The supported thread policy is `initial_threads_only`. The four breakpoint
objects apply to the threads present when capture is armed. Creation of a new
application thread ends the interval with a failed record and cleanup; that
interval is not an accepted control. A fresh login or selection that creates
threads may therefore exceed this profile. A broader thread profile requires
separate debugger and cleanup verification.

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
failure cleanup and cancellation. The thread-creation refusal checks replay of
all four sites after detach on both initial application threads and two later workers.
Every attached fixture must resume after detach, lose its debugger and exit
normally; tests never terminate it to make cleanup pass.

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

An explicit duration of 1 through 30 seconds is required. A 10,000-observation
limit and Ctrl+C / Ctrl+Break also end capture through cleanup. Closing the
console is not a tested cancellation path. An already debugged target, invalid profile or
existing output is refused. Raw process records and executable/build digests
stay private and outside tracked trees.

## Records

Format 1 begins with an `identity` row containing the target and engine
digests and the retail/fixture distinction. `profile` rows retain the resident
locator checks. Observation rows have `sequence`, `tid`, `utc_filetime`,
`elapsed_ms`, `hook_eip`, integer registers and `stack_slots`.
`utc_filetime` is the Windows FILETIME tick value, not a Unix timestamp;
elapsed time uses a separate steady clock. Stack slots begin at the recorded
ESP and are raw dwords, rather than universally named function arguments.

| Observation kind | Meaning |
|---|---|
| `setmap` | Actual game/application bases, retained bytes, region, zone and mode. |
| `receiver_ignored` | Another opcode at the shared receiver; its application bytes are not decoded. |
| `region_constructor` | Scene and entry arguments, with prior scene region/manager values. |
| `region_lookup` | Table, full-dword query, pointer-span bounds and actual caller. Manager fields require the known caller. |
| `lookup_result` | Returned/null root, lookup correlation and guarded manager/scene fields. |
| `partial_error` | Raw observation context retained when an interpreted read failed. |

Terminal `detached` or `failed` records contain observation/hit counts and
detach status, `thread_policy` and `initial_thread_count`. Observation rows are
buffered while attached and written after cleanup, preserving order. A count
includes filtered receiver observations, so it is not a count of SetMaps.
Host observation times cannot measure total
target pause or natural loading latency.
After attachment starts, `detach_confirmed: false` leaves target state
unresolved even if best-effort cleanup was attempted.

## Interpretation and remaining evidence

Order observations by sequence and retain thread IDs, hook EIP, registers,
stack slots and pointer bases. A manager lookup entry/result pair requires
the entry's actual return address `0x0064E87A`, the same thread and manager,
and result ESP equal to entry ESP plus eight, accounting for `RET 4`.
Retain the observed query and matched root key even when they differ.
Incomplete, interleaved or ambiguous records do not establish a pair.

SetMap, construction and lookup can be separated by scene queues. Equal
region values and nearby timestamps alone do not prove they belong to one
selection. The four observation points do not capture every queue operation
or the constructor's return. Keep unmatched calls and prior/current scene
manager values visible when reporting a correlation.

A zero-observation capture establishes only that no accepted site was seen
in that instrumented interval. A failed record or unconfirmed detach is not
an accepted control. Neither a lookup hit nor a shared retail resource set
accepts an independent authored scene. Ordered ResourceModule requests,
Resource-correlated DAT opens and downstream completion remain required by
the [request boundary](../../docs/resource/map-layout-request-boundary.md)
and [auxiliary completion contract](../../docs/resource/region-auxiliary-resource.md).
Rendering, collision, walking and ZoneMaster/script initialization require
their own acceptance evidence.
