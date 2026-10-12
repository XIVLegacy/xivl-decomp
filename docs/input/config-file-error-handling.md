# Config file and selection error handling

This page records file and selection failure paths in the retail 1.23b
`ffxivconfig.exe`. The input is 3,471,240 bytes with SHA-256
`e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977` and
image base `0x00400000`. All addresses below are VAs; subtract the image base
for RVAs. LLVM 22.1.4 instruction ranges were checked against original bytes
and independent Capstone decoding. PE imports were checked with pefile.

[Utility settings](config-utility-settings.md) owns control captions, offered
options, producer defaults and normal save/cancel timing.
[Controller mapping](controller-mapping.md) owns signed source assignments,
presets, reset and the bounded game consumers. These static error-path
observations do not establish OS API success or actual behavior under Wine.

## File admission differs between readers

The three readers request `CreateFileW` with read access `0x80000000`, share
mode 1, disposition 3 (`OPEN_EXISTING`), attributes `0x80`, and null security,
template and overlapped pointers. Each checks the invalid-handle sentinel -1.
Their read-result checks differ:

| File / reader | Request and admission | Useful instruction locators |
|---|---|---|
| `config.pad`, `FUN_00432b10` | Requests `0x148` bytes into `0x0045c148`; ignores ReadFile BOOL, requires returned count at least `0x148` and magic `0x20100211` | Read call `0x00432b79`, unsigned count branch `0x00432b87`, magic compare `0x00432b89` |
| Primary `config.lng`, `FUN_00433660` | Requests 8 bytes into `0x00459124`; ignores BOOL, requires count exactly 8 and magic `0x20100706`; does not bound the index byte | Read call `0x00433824`, count branch `0x0043382e`, magic compare `0x00433830` |
| `config.sys`, `FUN_004428d0` | Passes the low GetFileSize result directly to ReadFile into `0x0045cc78`; does not test size sentinel, BOOL or returned count before magic inspection | Size call `0x004429dd`, read call `0x004429f0`, magic load `0x004429f6` |

The pad reader has no file-size query: larger files are not rejected, but the
request remains `0x148` bytes. A missing file, inadequate returned count or
wrong magic clears that entire block, writes the known defaults and copies
`0x52` DWORDs to the current block. The accepted path also copies the whole
block, including its opaque tail. These checks do not validate source indexes
or selectors inside an admitted file.

The sys read destination is in the zero-fill part of PE `.data` (VA
`0x00456000`, virtual size `0xfdfc`, raw size `0x2000`). Neither those section
bounds nor the reader's field accesses establish the destination object's
capacity. No allocation or capacity guard precedes its read request. A short
or failing read is not itself a fallback condition: the code inspects the
dword left at `0x0045cc78`, which may be stale or partially produced. An
unrecognized magic or failed open retains the explicitly initialized scalar
defaults. Actual API bytes and any overwrite or process outcome are unobserved.

## Sys conversion and opaque bytes

The legacy `0x20100706` selector uses the six-DWORD table `[7,9,2,0,4,1]`.
At VA `0x00442a7a` it loads `[ESP+0x14 + selector*4]` without a bound check.
A selector outside `0..5` therefore reads outside that stack table; the value
and its later effect are unresolved.

The `0x20101020` UTF-16 copy at `0x00442c20..0x00442c33` and the
`0x20120419` copies at `0x00442d50..0x00442d63` and
`0x00442d70..0x00442d83` stop only at a source NUL. They have no source-size
or destination-capacity comparisons. The legacy path copies no strings.
The reader initializes scalar offsets `0x00..0x5c`; it does not clear the
string regions or final opaque bytes in that initialization sequence.

Every conversion/default exit joins the `REP MOVSD` at VA `0x00442d97`:
`0xab` DWORDs (`0x2ac` bytes) move from current `0x0045c600` to snapshot
`0x0045c8b0`. Thus offsets `0x2a8..0x2ab` participate in snapshot, compare
and save, although no individual field decode or child consumer is established.
Malformed terminators and unknown bytes must not be treated as validated
settings. The normal conversion map remains in
[utility settings](config-utility-settings.md#dialog-handlers-and-non-controller-settings).

## Language fallback result and resource-table bound

On a failed primary open, count mismatch or magic mismatch, startup calls
`FUN_0043fb30` at VA `0x0043383c`. The next instructions discard its EAX
result and copy `0x00459124/0x00459128` into active
`0x0045912c/0x00459130` at `0x00433844..0x0043385c`. The fallback's candidate
file/registry/LANGID results use local buffers/output slots; they do not
install a candidate into these global language records. The primary-buffer
bytes copied afterward may be untouched, partial or prior bytes. This
establishes the discarded return, not an observed default language.

The fallback first tries its runtime-selected root's `config.lng`. It accepts
a low GetFileSize result at least 5, requests exactly 5 bytes into a local
buffer and requires returned count exactly 5. It ignores BOOL and performs no
magic comparison. At `0x0043fc21` it reads the fifth byte and accepts values
below 6. On failure it queries HKLM using `FUN_0043f700`, whose successful
query checks status and `REG_DWORD` type, or calls `FUN_0043f5f0` for
GetUserDefaultLangID. Candidate 5 can also arise from LANGIDs `0x0404/0x0c04`.
The registry output pointer belongs to the fallback's own stack frame.

`FUN_0040cc20` installs only five resource IDs at `0x00457f04`.
The loader `FUN_0040e350` forms its resource ID from
`*(ushort *)(0x00457f04 + index*4)` at `0x0040e401` without an index guard.
Index 5 reads one DWORD beyond the five-ID table; it is not a sixth decoded
language. The primary record's index is also unbounded. The loader's later
module/resource/PEX checks do not guard this preceding table access. Neither
the fallback's discarded result nor this table bound proves an exercised
invalid-language path, a particular resource ID or a runtime failure.

## Save completion does not establish persistence

All three confirmed save paths request write access `0x40000000`, share 1,
disposition 2 (`CREATE_ALWAYS`), attributes `0x80`, and null security/template
arguments. A valid handle leads to these requests:

| File | Source and byte count | Write call / caller |
|---|---|---|
| `config.pad` | `0x0045c290`, `0x148` | `0x00431a57` in `FUN_004318b0` |
| `config.sys` | Snapshot `0x0045c8b0`, `0x2ac` | `0x0044192f` in `FUN_00441910`, invoked at `0x004423a5` |
| `config.lng` | `0x0045912c`, 8 | `0x004334b1` in `FUN_004332a0` |

The direct writers ignore BOOL and byte count. The sys wrapper ignores BOOL
and returns the output-count slot at `0x00441935`; its caller ignores EAX.
There is no retry or write-failure branch in these handlers. A failed create
skips WriteFile, but the confirmed save path still reaches dialog closure.
Only valid handles are passed to CloseHandle; its result is ignored.
`CREATE_ALWAYS` is a create/truncate request, and dialog closure is not proof
of truncation, a complete write or durable disk success. The utility's prompts
cannot serve as a persistence acknowledgement for a future settings writer.

## Font selection is captured while disabled

`FUN_00435e20` stores the Windows-font checkbox at `config.sys+0x54`, then
unconditionally reads the quality, thickness and selected face. There is no
branch on that boolean around the face lookup. Disabling the three combos
therefore does not exclude them from root save/close capture.

At VA `0x00435ea3` the face-list base is loaded from `0x0045cbf8`. A null
base reaches error call `0x00435ece`. For a nonnull base the code computes
the `0x1c`-stride record count, compares the selected index with that count
at `0x00435eca`, and uses unsigned `JB` at `0x00435ecc` to skip the error
only for an index below the count. With a valid nonnegative list count,
`CB_GETCURSEL` result -1 fails this comparison.

The call at `0x00435ece` invokes `FUN_0041d2db`. If it returns, the code
reloads the base at `0x00435ed3`, forms `base + index*0x1c` and reads
record `+0x18` at `0x00435ee2` without another null or bounds check.
This is a conditional unchecked continuation, not a guaranteed process fault.
The available names, selected index and error callback are runtime inputs.

## Error callback and termination request

`FUN_0041d2db` pushes five zeros, calls `FUN_0041d2b7` at `0x0041d2e2`,
cleans the 20-byte argument list at `0x0041d2e7` and returns at
`0x0041d2ea`. The callee reads global `0x004581b4`, passes it through
pointer decoder `FUN_004217d0`, and tests the result. A nonnull result reaches
the tail `JMP EAX` at `0x0041d2cb`. Its callback installation and return
behavior were not observed.

The null-decoded-pointer branch calls `FUN_004216e5`, which clears dword
`0x00465df4`, then jumps to `FUN_0041d1bb`. That routine builds an exception
record with code `0xc000000d`, queries `IsDebuggerPresent`, requests
`SetUnhandledExceptionFilter(NULL)`, and calls `UnhandledExceptionFilter`.
It ultimately requests `TerminateProcess(GetCurrentProcess(),0xc000000d)`
at VA `0x0041d29b`. The code still has cleanup and a `RET` at `0x0041d2b6`
if execution returns from those API calls. Neither branch proves a universal
non-return contract.

The pointer decoder has cached/platform paths and a fallback
`GetModuleHandleA`/`GetProcAddress` lookup for `KERNEL32.DLL` and
`DecodePointer`, whose strings are at VAs `0x0044b754`/`0x0044b764`.
It invokes a resolved decoder when available and otherwise returns its
original argument. The observed complete range is
`0x004217d0..0x0042183e`, exclusive end. Actual decoder/callback state
remains unverified.

## Selected-face copy capacity and result

Font capture calls `FUN_0041dad1` at VA `0x00435f03` with destination
`block+0x60`, count `0x20` and the selected name pointer. It cleans the
three arguments and returns without inspecting the copy result.

The copy routine's complete instruction range is
`0x0041dad1..0x0041db3d`, exclusive end:

| Condition | Observed branch, conditional on an accessible caller buffer |
|---|---|
| Null destination or zero count | Writes `0x16` to the slot returned by `FUN_0042091f`, calls the five-argument error dispatcher and returns `0x16` if that dispatcher returns |
| Null source with valid destination/count | Clears destination's first WCHAR at `0x0041db09`, then takes the same `0x16` path |
| Source NUL before remaining count reaches zero | Returns 0 at `0x0041db37` |
| Count exhausted before source NUL | Clears only destination's first WCHAR at `0x0041db26`, writes `0x22`, calls the same dispatcher and returns `0x22` if it returns |

The caller's 32-WCHAR capacity admits a terminated name of at most 31 WCHARs.
The exhausted-count branch does not zero the remaining destination units.
Its conditional return does not protect the preceding list indexing, and the
caller does not check the return code. Malformed source accessibility, callback
behavior, actual API outcomes and any resulting process/display state remain
unobserved.
