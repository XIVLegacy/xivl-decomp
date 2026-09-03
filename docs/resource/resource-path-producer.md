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
call. Function `0x004b2df0` owns the observed writes to the gate, including a
constant-one write at `0x004b2eca` and a conditional byte write at
`0x004b3191`.

The Resource constructor also has five direct callers at `0x00c98e40`,
`0x00c98f80`, `0x00c992d0`, `0x00c99480`, and `0x00c99670`. They accept or
build paths by other routes. Among those direct Resource-constructor callers,
only the slot-1 producer also calls the numeric formatter. Four additional
direct formatter callers are outside this Resource-constructor set, so
formatter reachability alone does not identify a Resource producer and not
every Resource path has numeric-producer provenance.

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
