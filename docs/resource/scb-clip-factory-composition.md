# SCB resource parsing and clip construction

The resource parser calls the clip builder synchronously on the nonzero
actor/track parser-result path. The selected call path is
`0x00A26380 -> 0x00A1C7B0 -> 0x00A28510`. Actor and track parsing prepare
collections before that invocation. They are not themselves the clip builder.
The actual clip class still depends on the selected registry entry, and the
owning UnitTree context at clip creation remains a condition.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, image base `0x00400000`,
size 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra listings were checked against the original PE with pefile
`2024.8.26` and Capstone `5.0.7`, x86-32 mode. Listing addresses are RVAs.
The checks covered every listed instruction, the enclosing address spans,
and the operands and calls below. The exporter
`tools/ghidra_scripts/DumpFunctions.java` computes `# size` by summing
function-body address-range lengths. The builder's listed instructions total
`0x26C` bytes within an enclosing `0x26F`-byte span. The separate three-byte
alignment gap at `0x00A2854D` was independently decoded as `LEA ECX,[ECX]`.
The final listed jump at `0x00A2877A` was checked through its complete end
at `0x00A2877F`.

The existing [Limsa scheduler binding](limsa-fireworks-scheduler-bindings.md)
and [SCB layout](cut-scheduler-clip-layout.md) findings supply the resource
entry path, serialized records and wrapper receiver contract.

## Actor and track preparation

`0x00A298D0` takes `(chunk, factory, arena)` as three dword stack arguments
and returns with `RET 0x0C`. It sign-extends `s16(chunk+0x0A)` for the
actor count and starts actor records at `chunk+0x10`. It reserves `0x14`
arena bytes for a collection and calls `0x00A21DE0` with the count and
arena. That constructor reserves `4*count` pointer bytes, retains the base
at collection `+8`, the count at `+0x0C`, and initializes the append index
at `+0x10` to zero.

For each positive-count record, the actor helper zero-extends byte `+2`
and selects an eight-byte key entry. At `0x00A2993B..0x00A29945`, it calls
`0x00A1F9C0` with `(keyEntry, record, factory, arena)`. That helper loads
a selected node's function pointer from node `+0x10` and calls it at
`0x00A1FA1F` with five dwords `(record, factory, keyEntry, arena, 0)`.
It restores those 20 bytes itself. Its registry lookup `0x00A1FC00`
remains uninspected; a node equal to the retained end node returns zero.
The result is appended unconditionally through `0x00A21D10`, including
null results. The next record adds the sign-extended word at record `+0`.

`0x00A26850` takes `(trackTable, arena)` as dword stack arguments and
returns with plain `RET`; its caller removes eight bytes at `0x00A25CA8`.
It reserves `0x18` arena bytes for a collection, then `0x00A26CA0`
reserves its pointer array and allocates `0x14` bytes per entry through
the uninspected constructor `0x00A2A720`. The count is the track table's
first dword, tested as signed for the positive-count loops.

Each track record begins after that dword. Word `+0` equal to `0xFFFF`
skips `0x00A2A780`; otherwise that word is sign-extended for indexing the
collection and the helper is called with the current entry. Its semantics
remain uninspected. `0x00A2A7D0` retains the serialized record pointer at
current-entry `+0x10`. Record advance is
`(2*(u8(record+3)&0x1F)+7)&~3`. These selected track bodies do not call
the clip builder.

## Synchronous builder invocation

Let `S` be the new object passed into `0x00A26380`, and let
`P = u32(S+0x50)`. After its existing resource, allocation and version
guards, the factory stores the result of `0x00A25DC0` at `S+0x74` and
calls the actor/track parser `0x00A25B90`. The instructions at
`0x00A26582..0x00A2658E` test that parser's dword result and call
`0x00A1C7B0` with `ECX=S` only when it is nonzero. The call happens
before the factory compares used arena bytes with its planned size at
`0x00A26593..0x00A2659B`.

`0x00A1C7B0` constructs a local builder through `0x00A28170` at
`0x00A1C7DE`, with `(u32(S+0x7C), u32(P+0x58), P)`. That constructor's
field mapping and its expected 12-byte callee cleanup remain uninspected.
At `0x00A1C7E3..0x00A1C7F8`, it calls `0x00A28510` with two dwords
`(u32(P+0x3C), S+0x60)`. The builder removes eight argument bytes with
`RET 8`. Its ECX is the caller's stack pointer after those pushes plus
`0x20`; this is the constructor's local region if its expected cleanup
holds. The returned collection is retained at `S+0x34`.

As established by the SCB layout finding, `0x00A28510` selects an eight-byte
clip key entry using `u8(D+2)` and calls a selected registry node's
`+0x10` function pointer at `0x00A28695`. The argument pushes at
`0x00A28690..0x00A28694` pass `(D, keyEntry, arena, 0)` as four dwords;
the caller removes 16 bytes at `0x00A28699`. The selected node must differ
from the retained end node at `0x00A28661..0x00A28665`. A nonnull factory
return is separate from successful initialization or playback.

If that selected pointer is `0x00837220`, the final zero chooses its
construction branch rather than its size-query branch. The factory reserves
`0x40` arena bytes and, when the original cursor is nonnull, calls
`0x00836B80` with `(D,keyEntry)`. That constructor calls `0x00836660`
and installs primary vtable `0x0103B54C`. Its slot-1 cell at
`0x0103B550` contains receiver-selection method `0x00836730`.
The selected local builder's registry setup and `0x00A1FC00` lookup are
not joined here to the `0x00837220` pointer. That remains the exact
factory-selection condition.

## Owning-context limit

`0x00836660` loads global `0x013657B4` at `0x008366AE` into clip `+0x38`
and global `0x013657B8` at `0x008366B7` into clip `+0x3C`. These are
the actual global values at those instructions. The latter is the `U`
used by the existing wrapper receiver-selection finding.

The synchronous call chain does not prove those globals retained their
values from UnitTree publication. The uninspected builder constructor,
registry lookup, actor constructors, intervening virtual calls and
`0x00836EF0` before the final capture can affect context. No selected
read joins the instance's retained `+0xFC` field to this capture. The next
bounded static edges are `0x00A28170` at the local-builder construction
and `0x00A1FC00` at its registry lookup, followed by context preservation
across the selected constructor chain.

This composition establishes conditional construction calls. It does not
establish allocation success, complete initialization, child registration,
rendering, an authenticated controller actor, a binding or scheduler request
observed in retail, a historical sender, a calendar, or effect placements.
