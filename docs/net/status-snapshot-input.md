# Map 0x0179 snapshot input

The admitted CharaElement case for Map clientbound game-message `0x0179`
reads twenty u16 values from the 40-byte application in order. It submits
every index 0..19 to the same local staging helper before forwarding that
local record. Zero-valued observations do not make the consumed array padding.

## Input and selected route

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA 6.4.0 with Ghidra 12.1.3 supplied bounded reads. pefile 2024.8.26 and
Capstone 5.0.7 x86-32 checked original PE raw backing, byte equality and
complete decoding.

The [published CharaElement route](e2-selector-state.md#input-and-routing)
owns the receiver and packet-view binding. Its entry predicate must admit
this opcode; `0x0179` is absent from the restricted-state whitelist.

At `0x0058CD21`, the dispatcher reads the opcode from game-message `+2`.
The comparison with `0x0134` selects the high block at `0x0058D330`.
That block subtracts `0x0135`, admits unsigned indexes through `0x72`,
reads a byte from table base `0x0058D994` and indexes the dword table
at base `0x0058D930`.

For `0x0179`, the index is `0x44`. Byte `[0x0058D9D8]` is `0x08`, and
dword `[0x0058D950]` is `0x0058D488`. These are selected table entries;
neither address is the base of its table.

## Ordered application reads

Application offsets exclude the 16-byte game-message header.

| Application offset | Type | Count | Consumed order |
| ---: | --- | ---: | --- |
| `0x00` | u16 | 20 | Index i receives the value at byte offset `2*i`, for i=0..19. |

Let S be ESP at case entry. The case calls `0x0058EAF0` with local record
`S+0x34`, zeros EBX and advances the game-message pointer by `0x10`.
Each iteration zero-extends the next u16, pushes that value and the index,
and calls `0x0058EAB0` with the same local record. Its `ESP+0x3C`
calculation follows two pushes, so the record address remains `S+0x34`.
The index and input pointer advance by one and two respectively; unsigned
`JB` after comparison with `0x14` produces exactly twenty iterations.

After normal helper returns, the case passes the local record to
`0x00588D20`, using its retained EDI as receiver. The selected case does
not inspect the packet's declared size. Its largest read ends at game-message
offset `0x38`, requiring 56 game-message or 40 application bytes. This fixed
read extent is distinct from a capture observation.

The downstream status construction, reference storage and packed-reader
contract is already recorded in
`xivl-opcodes@347e2cfbed8814043dfd38bd18e6af8bfa38ab79:data/client_opcode_semantics.json#s2c-0179`.
The [System receiver's fields](../event/actor-substat-receiver.md) are offsets
within a constructed receiver object; its byte at `+0x14` is not a direct
wire-payload field. The
[status wire translation](../actor/status-wire-translation.md) records how a
wire value becomes the receiver's key and the single-slot opcode `0x0177` that
feeds the same path. This selected case does not establish the meaning of
individual identifiers, status causality, server policy or visible effects.

## Retained observations and reproduction

The catalog's pinned `data/vendor/captures/payload_samples.json`, record
`samples.s2c.0x0179`, retains 55 records across 16 captures. Every record is
a 72-byte subevent containing a 56-byte game message and 40 application
bytes. Fifty-four applications are zero; the remaining record from
`return_to_inn.pcapng` has `0x5ADF` at index zero and zero at the other
nineteen indexes. This does not identify the resolved status record or its
packed-reader values. The older proposal's 30 selected zero samples remain
a different denominator.

Reproduce only these half-open original code ranges:

| Range | Bytes | Instructions |
| --- | ---: | ---: |
| `[0x0058CD21,0x0058CD39)` | 24 | 5 |
| `[0x0058D330,0x0058D34C)` | 28 | 5 |
| `[0x0058D488,0x0058D4BB)` | 51 | 17 |

The union is 103 code bytes and 27 instructions. Supporting typed reads
are exactly the byte at `0x0058D9D8` and the dword at `0x0058D950`.
Map each VA through the original PE sections, require complete raw backing
and compare those bytes before x86-32 decoding. The root was not read as a
full body, and the reset, staging, stream, constructor and callback bodies
are outside this reproduction.

Existing layout and helper records are
`xivl-client-structs@28489fa109a5da74f670c9a51afcc8412ee94429:manifests/s2c_payload_decoding_lua_bound.json#payloadStructs.s2c_0x0179`,
`manifests/fun_0058cca0_walk.json`, case `0x179`, and
`manifests/symbols.json#BCS-Y-1069`, BCS-Y-1070 and BCS-Y-0643.
