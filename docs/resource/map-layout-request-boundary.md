# MapLayout numeric request boundary

The normal layout-manager path can reach the numeric ResourceModule producer
and its asynchronous DAT-open chain. A manager submission, a numeric request,
and a completed file open are three separate observations. This finding does
not establish acceptance of an authored scene.

## Binary and method

These are VAs in retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
image base `0x00400000`, size 15,996,808, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The retained function inventory and assembly were produced by
`tools/ghidra_scripts/DumpFunctions.java`. Exact-address disassembly with
`llvm-objdump 22.1.4` independently checked the manager, request, initialization,
and getter instructions below against that executable. Use
`llvm-objdump --disassemble --start-address=<VA> --stop-address=<end-VA>`
with the owning function ranges in `config/ffxivgame.symbols.json`.

The ResourceModule vtable identity and slot are recorded in
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`:
`Component::Resource::ResourceModule`, vtable RVA `0x00D08C3C`, slot 1,
function RVA `0x00899130`.

## Conditional manager request

`0x00643020` reads the manager's numeric key at `+0x158`. It obtains a name
through `0x0079B890` and copies that name to manager `+0x17C`. The helper
resolves the key through `0x0079B420`; a key without a matching name is not
therefore equivalent to an ordinary named retail layout.

At `0x00643146`, the manager supplies the literal lookup bytes `byl\0`
(`0x006C7962`) to `0x00A3DF70`. The zero result at `0x00643159` branches to
`0x0064326C`, which submits the numeric request through `0x0079C890` at
`0x0064328E`, returning to `0x00643293`. The other branch attempts a named
resource lookup through `0x0079C720` and can search existing managers by
their `+0x17C` names at `0x006431DB` through `0x00643260`. Its successful
reuse moves the existing manager's resource pointer at `+0x14C` into the
new manager. The literal key is retained here without assuming a reversal
or normalization rule.

An additive resource must consequently have a distinct resource key and a
non-aliasing name. A new selector value with an existing resource set, or a
new numeric key that reuses a retail manager's name, is insufficient evidence
of an independent scene. Observe the actual branch and requested resources.

## Bridge to ResourceModule

`0x0079C890` first consults its existing request collection through
`0x00654420`. A hit increments an existing record rather than issuing another
ResourceModule call. For a miss, `0x0079C90B/0x0079C90E` admit the request
only when the unsigned dword at request-object `+0x10` is at most `0x40`.
This is an observed admission predicate, not a zone-ID limit or a guarantee
of arbitrary regional composition size.

The admitted branch reads the context at request-object `+0x04`, calls
`0x0060B250`, then invokes the returned object's virtual slot `+0x04` at
`0x0079C931`. The getter is exactly a load from context `+0x50`.
Normal context initialization constructs a 0x44-byte ResourceModule at
`0x004B33B6` and stores its returned pointer at context `+0x50` at
`0x004B33C8`. Its constructor `0x00C99090` installs vtable VA `0x01108C3C`
at `0x00C990C3`, whose slot `+0x04` is `0x00C99130`. These instructions
close the ordinary concrete virtual target. An observation probe must still
record the runtime pointer/vtable rather than assuming every context instance
has that target.

The [resource path producer](resource-path-producer.md) owns the remainder:
`0x00C99130` formats the resource key with `0x0044B3A0`, constructs the
Resource, and queues it. FileThread `0x00C96850` later passes the Resource's
owned path wrapper at `+0x04` to LocalFile `0x00453C00`. Numeric byte-group
paths apply only when formatter mode byte `0x01266B64` is nonzero; the zero
mode uses a different table-based path. Request caching, pending work and
already loaded objects can all prevent a fresh open for a repeated selector.

## Observation ABI and causal join

Offsets below use entry ESP, before the callee's prologue. Preserve every
argument and the original return value; generated packet-body offsets do not
describe any of these native argument lists.

| Seam | Actual base and arguments |
|---|---|
| `0x0079C890` | `ECX` is the request object; `[ESP+0x04]` is the manager key, followed by three argument slots; callee returns with `RET 0x10` |
| `0x00C99130` | `ECX` is ResourceModule; `[ESP+0x04]` is the `u32` resource key; nine argument slots total, `RET 0x24`; `EAX` returns the Resource pointer |
| `0x00C9697F` callsite | `ESI` has been advanced to Resource `+0x04`; `ECX` is the selected FileThread-owned LocalFile record; arguments are that path wrapper, `rb`, and retry count zero; return VA `0x00C96984` |
| `0x00453C00` entry | `ECX` is LocalFile; `[ESP+0x04]` is the borrowed narrow path wrapper, `[ESP+0x08]` the mode, `[ESP+0x0C]` the retry count; return and LocalFile stream state establish the result |

The numeric request's return Resource pointer supplies the stable join key
across threads. At the FileThread callsite, recover the Resource pointer as
the path-wrapper argument minus four, and record its key at `Resource+0x58`.
Join by live Resource identity and request epoch, preserving multiple uses
and lifetime boundaries; a numeric ID or matching time alone is correlation,
not proof of the same request. A nested same-thread stack at the producer
cannot account for the later FileThread open.

A complete record includes a monotonic sequence, thread, map epoch, caller
return VA, object and path-wrapper identities, every original argument,
numeric key, formatter mode, unchanged original path, actual opened path,
open return/stream result, and a terminal dropped-record/output-failure
summary. Bound logging and terminate the owner-operated run after the named
selection completes or the declared timeout expires. Ambiguous or missing
joins remain explicit; they are not successful resource acceptance.

## Verification boundary

`python tools/verify_resource_path_producer.py --exe <explicit-executable>`
passed the pinned-build producer/open signature and deliberate mutation
checks. The new manager and initialization locators were checked directly
with exact-address disassembly. No live selector input or resource overlay was
changed for this finding. Distinct authored selection, DAT-open results and
preserved retail control selections require the bounded experiment defined
by the selector's consumer contract.
