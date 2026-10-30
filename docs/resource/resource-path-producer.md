# Resource path producer

This page traces the exact-build numeric resource-id producer to the previously
verified DAT-open boundary. It proves the canonical static chain and an
exact-build hook signature. It does not promote the candidate id from the one
observed successful open beyond inference.

## Verdict

`Component::Resource::ResourceModule` vtable slot 1 at `0x00c99130` accepts a
u32 resource id. In numeric mode it formats that id through `0x0044b3a0`,
deep-copies the resulting path into a newly allocated Resource at `+0x04`,
stores the same id at `+0x58`, sets state `+0xb0` to 1, and queues the Resource
on its FileThread. The FileThread consumer at `0x00c96850` later passes
`Resource+0x04` to the LocalFile open member at `0x00453c00`.

The complete canonical edge is:

```text
ResourceModule slot 1 (0x00c99130)
  -> numeric formatter (0x0044b3a0)
  -> Resource constructor (0x00caedd0)
  -> FileThread queue producer (0x008edda0 / 0x008edbf0)
  -> FileThread consumer (0x00c96850)
  -> LocalFile open (0x00453c00)
```

The observed path `data\2A\08\00\17.DAT` is consistent with input
`0x2a080017`, but that request was not captured at the producer or formatter.
The executable contains no immediate occurrence of that id. Its correlation
therefore remains `inferred`, matching the pinned prior open-boundary record in
`config/resource_path_producer.json`.

## Formatting contract

The formatter has two modes selected by byte `0x01266b64`. When that byte is
nonzero, it uses:

```text
%cdata%c%02X%c%02X%c%02X%c%02X.DAT
```

Every `%c` is backslash and the four byte groups are supplied from most to
least significant. Hex digits are uppercase. Thus numeric input `0x2a080017`
produces `\data\2A\08\00\17.DAT`. The formatter joins that relative value to
the root wrapper at `0x0132cb98`; `0x004b2d30` supplies the root. No
machine-local root is retained here.

When the mode byte is zero, the same function follows a different table path
using the high 16 bits as a group and the low 16 bits as an index. The numeric
mapping is therefore conditional, not an unconditional decoder for every
call. The [initialization predicate](#initialization-predicate) below owns
the conditional caller stores and early-exit boundary; it does not
establish a historical or request-time mode.

The Resource constructor also has five direct callers at `0x00c98e40`,
`0x00c98f80`, `0x00c992d0`, `0x00c99480`, and `0x00c99670`. They accept or
build paths by other routes. Among those direct Resource-constructor callers,
only the slot-1 producer also calls the numeric formatter. Four additional
direct formatter callers are outside this Resource-constructor set, so
formatter reachability alone does not identify a Resource producer and not
every Resource path has numeric-producer provenance.

## Catalog mode

When mode byte `0x01266B64` is zero, `0x0044B469..0x0044B48F`
passes the resource key's upper 16 bits to `0x0044B2A0`, then checks the
lower 16 bits against the returned record's unsigned count at `+0x0A`.
`0x0044B2A0` binary-searches the group collection at `0x0132CB88`, using
record `+0x08` as the group key and `0x0132CB94` as its count.

For a valid group/index, `0x0044B495..0x0044B4B9` reads the record's
prefix pointer at `+0x00`, adds the low index to its base at `+0x04`, and
looks up the resulting string entry through `0x0132CB8C`. The subsequent
joins build the root wrapper, prefix, slash literal at `0x00F672E4`, and
entry string into the result. Missing groups or out-of-range indices jump
to `0x0044B5EF` before that result copy. They do not fall through to numeric
path formatting.

The formatter receives a resource key and reads initialized global catalog
state. This branch does not read a weather ID, event ID, season or a second
bank selector. Changing the global path mechanism does not itself add a
weather registration or recover historical resource contents. The
[compiled defaults](#compiled-resource-path-defaults) identify the mode and
path-file keys. Actual retail initialization remains unbound.

These observations used the retained Ghidra listings for `0x0044B3A0`,
`0x0044B2A0` and `0x0044B690`. Python pefile `2024.8.26` mapped each listed
instruction to the pinned original PE, checked its bytes, and Capstone
`5.0.7` independently decoded the selected x86-32 ranges. No client ran.
The [seasonal selection boundary](native-seasonal-resource-selection.md)
separates native lookup from installing the resource files it consumes.

## Initialization predicate

Initializer `0x004B2DF0` supplies a conditional source for the mode byte
`0x01266B64` (RVA `0x00E66B64`). At `0x004B2E69`, it calls getter
`0x00443E40` with receiver ESI, the initializer's second argument, and
arguments `(2,0,0)`. The full EAX test at `0x004B2E6E` selects the branch;
a nonzero return reaches the constant-one mode store at `0x004B2ECA`, then
jumps to `0x004B3197`. Intervening calls must return to reach that store.

The getter computes a wrapping u32 address
`S = [receiver+8] + 0xBC * ((arg1 << 5) + (arg2 & 31))`. It returns the
u32 at S+`0x60` when S is nonzero, or argument 3 when S is zero
(`0x00443E56/59/5B/61`). For `(2,0,0)`, the normal read is the table base
plus `0x2F60`, with fallback zero. The guard tests the computed address,
rather than the table-base pointer alone. The record is the `number_file`
setting identified below. The startup defaults and root producer are
established below; its request-time value remains unobserved.

When that getter returns zero, the initializer calls table initializer
`0x0044B690` at `0x004B3141`. TEST AL and SETZ BL at
`0x004B3149/4B` preserve the inverse of the callee's byte result. After
cleanup, `0x004B3189/8B` branches on nonzero BL to the false-return path at
`0x004B35B8`, before the mode store. Zero BL falls through to
`0x004B3191`, writing mode zero, then reaches `0x004B3197`. This uses the
x86 callee-saved EBX convention across the cleanup calls. The callee's full
EAX is not the predicate: a nonzero EAX with AL zero takes the failure case.

The table initializer receives four inputs: a path-wrapper result built from
root wrapper `0x0132CB98` and getter `0x00443E70(ESI;2,3,empty-wrapper)`,
integer getter `0x00443E40(ESI;2,18,0)`, integer getter
`0x00443E40(ESI;3,0,0)`, and getter
`0x00443E70(ESI;2,30,empty-wrapper)`. The string-like getter shares the
same index formula but returns S+`0x68` or its third-argument fallback.
Tuple `(2,3)` is `path_file`; the other settings are identified below.
At `0x0044B733`, the table initializer passes its first argument, literal
`rt` at `0x00F672F4`, and retry zero to LocalFile open `0x00453C00`.
Zero AL at `0x0044B738` reaches cleanup `0x0044BF7B` and clears AL at
`0x0044BF92`. No actual open outcome was observed.

The two named mode stores are direct stores in the retained initializer;
this coverage does not exclude indirect, aliased, transitive or later
mutation. A mode store also precedes later initialization that can fail.
These instructions do not establish the actual mode at a resource request
or in a historical session.

The observation used retained Ghidra instruction listings checked against
the original `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
pefile `2024.8.26` and Capstone `5.0.7` independently checked source bytes,
listed ranges and instruction boundaries without executing the client.
The retained `0x0044B690` body has discontinuous AddressSet coverage.
The [catalog text initialization](#catalog-text-initialization) below recovers
its reachable ordinary success path without treating the catalog size as a
contiguous extent.

## Compiled resource path defaults

SystemConfig constructor `0x004B8640` registers `(2,0)` as `number_file`,
with scalar default 1, through `0x00443CF0` at `0x004B90CE`. It registers
`(2,3)` as `path_file`, with default `/client/program/include/path.dat`,
through `0x00443D50` at `0x004B91FE`. The named string/default setup is in
`0x004B90A1..0x004B91FE`; scalar helper stores are at
`0x00443D32/0x00443D35`. LLVM 22.1.4 and Python 3.12.14 checked these
instructions and literal references directly in the pinned original PE.

The other catalog inputs registered by that constructor are `(2,18)`
`readable_version` = 255 (`0x004B9600`), `(3,0)` `extra_platform` = 0
(`0x004B9955`), and `(2,30)` `priority_ext` = empty string
(`0x004B9906`). These are constructor values, not request-time observations.

## Startup root and configuration boundary

The pinned PE initializes formatter byte `0x01266B64` to 1 in its data
image, at file offset `0x00E66B64`. Main startup obtains the executable
directory through `GetModuleFileNameW` in `0x00401A00`. Call
`0x00402C25` supplies that directory to `0x004B69C0`, which overwrites
SystemConfig `(8,1)`, `platform_install_path`. Startup later supplies the
SystemConfig at Main `+0x3A0` to Rapture initialization at `0x0040354B`.

On the nonzero `number_file` branch, getter `0x004B6A60` reads `(8,1)`;
`0x004B2D30` supplies the resulting path to root wrapper `0x0132CB98`.
The inspected precedence is constructor registration, assignment of the
executable directory to `(8,1)`, then the initializer's `number_file` read
and conditional mode store. This establishes an executable-relative numeric
startup profile, conditional on successful executable-directory production,
intervening calls returning, and the settings retaining those values.
Original-PE entry-rooted checks used Python
3.12.14 and LLVM 22.1.4; independent pefile/Capstone checks confirmed the
selected root and mode transfers.

Mode zero instead reads `(2,1)`, `root_path`, through `0x00443E70` at
`0x004B2F0A`, then supplies it to the root setter at `0x004B2F10`.
The executable-directory producer therefore does not locate a mode-zero
catalog. Its selected root and catalog remain unestablished.

The full-install [auxiliary census](region-auxiliary-resource.md#full-install-numeric-path-census)
used the directory containing the authenticated executable and found no
`client/program/include/path.dat`. The inspected startup chain identifies
no selected external configuration overriding the resource-path settings.
The [binary system-store consumers](../input/system-configuration-consumers.md#direct-startup-joins)
do not establish such overrides either. No authenticated mode-zero catalog
is therefore available here for a 57-instance mapping search. This is a
qualified static startup profile; it does not establish request-time mode,
exclude another invocation or later mutation, or qualify harmless absence.
A mode-zero request still needs its selected catalog authenticated before
assigning a path or claiming absence.

## Catalog text initialization

The ordinary successful control flow of `0x0044B690` is recoverable directly
from the pinned executable. Entry-rooted x86 decoding inside
`[0x0044B690,0x0044BFC0)` reaches the return at `0x0044BFBB` through
`[0x0044B690,0x0044BBB5)` and `[0x0044BBC0,0x0044BFBC)`.
The recovered body includes `0x0044BF07..0x0044BF7A`, which the retained
Ghidra listing omitted. Its calls sort the group records, close the file and
destroy temporary wrappers, then set `AL=1` at `0x0044BF77`. The retained
size `0x8AD` was a sum over its AddressSet, not a contiguous function extent.
This establishes a static success path, conditional on called routines
returning, without establishing an actual retail initialization result.

The initializer reads the opened text stream twice. Both read calls pass
capacity `0x2000` to `0x009D26C2`, whose byte loop stops at LF, EOF or
capacity and adds NUL (`0x009D27E3..0x009D2827`). Between passes,
`0x0044BB7D` calls `_fseek(FILE*,0,0)` without checking the result.

Header helper `0x0044B110` requires a leading slash and finds a later `[`.
It treats the following bytes through `]`, NUL or the wrapper's reported
length as the group prefix and returns its length plus one. Data helper
`0x0044AFD0` separates four fields at commas or signed bytes below `0x20`.
It does not implement CSV quoting. The first three fields supply group key,
index and selector through four hex lookup digits at token offsets `+2..+5`.
The converter initializes `0-9`, `A-F` and `a-f`; it does not verify a `0x`
prefix, exact token width or the remaining lookup entries. The fourth field
supplies the full path candidate. Selector low byte `0x40` skips the row.

The first pass derives the header count and each group's index extent.
The second pass creates one 12-byte record per header and populates the
pointer table and byte-string arena. A row that increases the group's highest
index writes its group key as a u16 and `index+1` as a u16 at
`0x0044BDCF/D3`. Index `0xFFFF` wraps that stored count to zero.
Lower indices do not revalidate the group key, and missing indices retain
null string pointers. Repeated indices pass candidate selection through
`0x0044ADA0`, which depends on configuration-derived selector values,
optional substring preference and candidate path length. Selected paths are
stored after dropping the header prefix length plus one byte
(`0x0044BED4 -> 0x0044B240`), without checking prefix equality.

The success path sorts the records by their unsigned u16 group key using
`_qsort` and comparator `0x0044B280` (`0x0044BF07..0x0044BF29`).
Source group order need not be sorted. Duplicate group keys are not rejected
or merged here, and the comparator returns equality for them. The parser
therefore does not establish unique duplicate-group resolution or a safe
additive mapping policy.

These observations used pefile `2024.8.26` and Capstone `5.0.7` on the
exact executable identified above. All retained instruction bytes were checked
against that executable before extending the owned CFG. No client ran.
The selected configuration values, exact catalog member, existing assignments
and request-time mode remain unbound. An additive
mode-zero profile requires those joins and a preservation audit of the actual
catalog; static parser success alone does not provide one.

## Path ownership

The narrow path wrapper is 0x54 bytes. It holds a data pointer at `+0x00`,
capacity at `+0x04`, a byte count including the NUL at `+0x08`, and a 64-byte
inline buffer at `+0x12`. Heap-backed storage is marked by zero at `+0x11`.
Growth rounds capacity and moves the bytes; destruction frees storage only for
the heap-backed form.

The Resource constructor at `0x00caedd0` deep-copies the producer-local wrapper
into `Resource+0x04`. Slot 1 can then destroy its local wrapper after queueing
and indexing without invalidating the Resource copy. The Resource destructor
body at `0x00cae7f0` eventually destroys the owned copy. At the open boundary,
the FileThread consumer borrows `Resource+0x04`; LocalFile converts it to
temporary wide storage without mutating or destroying the Resource wrapper.

This establishes ownership of the original path through the open call. It
does not establish ownership rules for a launcher-supplied override buffer.
The FileThread owns the LocalFile array and pairs one 0x2010-byte record with
each queued Resource slot. The read service at `0x00c961f0` uses the matching
record, then directly calls LocalFile close `0x00453000` at `0x00c962f5`
after the read attempt. That close flushes a non-null `FILE*`, closes it, and
clears `LocalFile+0x04`. On the successful path this occurs before Resource
state `+0xb0` is published as 2. The same read service closes at
`0x00c962d5` on allocation failure and at `0x00c96373` when the request is
rejected before the read.

The write service at `0x00c96380` closes its matching LocalFile stream at
`0x00c96441` and tears down a temporary LocalFile at `0x00c9646a`. Owner
teardown remains an independent terminal safety path through `0x00453190`,
the LocalFile deleting destructor, and FileThread array destruction. These
edges establish client stream ownership and normal per-request read closure.
They do not assign ownership to a launcher-supplied override buffer.

| Object | Owner at the pre-open seam | Lifetime evidence |
|---|---|---|
| `Resource+0x04` narrow wrapper | Resource | Borrowed by LocalFile open; destroyed with the Resource |
| 0x2010-byte LocalFile record | FileThread | Selected from the FileThread-owned array for the queued slot |
| `LocalFile+0x04` `FILE*` | LocalFile | Open stores it; read service closes and clears it |
| Temporary UTF-16 path and mode | LocalFile open stack frame | Destroyed before LocalFile open returns |
| Substitute narrow wrapper | Launcher, if supplied | No client evidence assigns ownership; it must outlive the borrowed call |

## Exact-build signature

The pre-open setup begins at VA `0x00c96972`, file offset `0x00896972`:

```text
6A 00 68 ?? ?? ?? ?? 83 C6 04 56 8B CF E8 ?? ?? ?? ??
```

The call opcode is at pattern offset 13 and resolves from `0x00c9697f` to
`0x00453c00`. Bytes 3-6 mask the absolute address of the `rb` literal; bytes
14-17 mask the call displacement. A whole-file scan of the pinned executable
finds exactly one match. Changing retained opcode byte 11 from `8B` to `8A`
produces zero matches. This supports an exact-build signature only; it does not
claim cross-build resilience.

At the matched call, `ECX` is the FileThread-owned LocalFile record. The three
callee-clean arguments are borrowed `Resource+0x04`, the `rb` mode literal,
and retry count zero. Execution returns at `0x00c96984`; this FileThread call
site does not branch on the LocalFile open return. The existing masked pattern
does not need broadening: the executable hash gates the complete build, while
the unique pattern locates the call and its resolved target.

Exact-address disassembly with `llvm-objdump 22.1.4` established the read and
write close edges above. The executable-bearing verifier checks the close body,
including its direct `fflush` and `fclose` targets and null store, plus every
listed service call target without retaining binary bytes.

Run the tracked structural and mutation checks with:

```powershell
python tools/verify_resource_path_producer.py
python tools/verify_resource_path_producer.py --exe orig/ffxivgame.exe
python tools/validate_repo.py
```

The executable-bearing form checks size and SHA-256, unique wildcard match,
resolved call target, and deliberate stable-byte mutation. Repository validation
runs the asset-free manifest checks.

## Consumer implications

An experimental pre-open hook must identify both the pinned executable hash
and the unique signature. At that boundary, `Resource+0x04` is a borrowed
narrow path wrapper. Render a numeric id as uppercase MSB-first byte groups
only after independently establishing numeric mode. The client owns the
LocalFile record and its stream and closes the stream after the read attempt.
Any substitute path remains launcher-owned and must stay alive across the
borrowed open call; static client evidence does not supply an ownership rule
for that buffer.

Production redirect behavior remains unsupported. The observed request has not
been joined dynamically to the numeric producer; successful
redirect semantics, missing-file fallthrough, and original-path identity
forwarding are untested; override-buffer ownership is untested; and cross-build
signature stability remains unresolved.

The next runtime discriminator is one pinned-build replacement that passes an
independently owned substitute wrapper through the original LocalFile open,
observes the replacement stream complete and close at `0x00c962f5`, then
repeats a miss while proving the original `Resource+0x04` bytes were forwarded
unchanged.
