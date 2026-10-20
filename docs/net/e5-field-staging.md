# Map 0x00E5 field staging

The selected CharaElement case for Map clientbound game-message `0x00E5`
reads one byte, two dwords and a 32-byte span from the application. Its
largest fixed read requires 56 application bytes. It constructs a narrow-byte
string from the copied span and forwards four argument words to an embedded
consumer. This read extent is not an observed packet size or a completed
countdown presentation contract.

## Input and route

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA 6.4.0 with Ghidra 12.1.3 supplied bounded code/data reads and the wrapper
extent. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked
original PE raw backing, byte equality and complete instruction decoding.

The existing CharaElement receiver and game-message packet binding are
recorded in `xivl-client-structs:manifests/symbols.json#BCS-Y-0540` and the
[E2 route](e2-selector-state.md#input-and-routing). The published entry
whitelist in `xivl-client-structs:docs/client-architecture.md` excludes
`0x00E5` when its entry predicate returns false. The case below therefore
requires an admitted CharaElement route; other Element types and restricted
states are separate.

For the main low switch, `0xE5-0x0F = 0xD6`. Original byte
`[0x0058D7E4+0xD6]` is `0x15`; dword
`[0x0058D788+4*0x15]` is `0x0058D246`. The exact incoming reference from
this root to `0x0058EB80` is the call at `0x0058D2B5`.
Only that byte and dword were read, not the complete tables.

## Fixed reads and local string

Application offsets subtract the 16-byte game-message header:

| Consumed value | Game-message offset | Application offset | Width |
| --- | ---: | ---: | ---: |
| B | `0x10` | `0x00` | u8 |
| W0 | `0x18` | `0x08` | u32 |
| W1 | `0x1C` | `0x0C` | u32 |
| Copied byte span | `0x28` | `0x18` | 32 bytes, in four qword loads |

The qword loads start at game-message `0x28/0x30/0x38/0x40`.
Their exclusive end is `0x48`, requiring 72 game-message bytes or 56
application bytes. The span starts at application hexadecimal `0x18`,
which is decimal 24. No selected instruction clamps these loads to the
packet's declared length. Shorter strings do not reduce the four fixed loads.

Let S be ESP at case entry. The span is copied contiguously into
`[S+0x34,S+0x54)`, despite two intervening pushes. At `0x0058D293`,
`0x00447260` receives this buffer, a length word loaded from `0x00F67298`,
and destination string object `S+0x74`. The pinned original length word is
`0xFFFFFFFF`.

The reused constructor contract is
`xivl-client-structs:manifests/symbols.json#BCS-Y-0311`: this is a
Sqex::Misc narrow-byte string constructor. With length `0xFFFFFFFF`, it
derives the length using `strlen`, copies that many bytes and appends NUL.
It does not impose a 32-byte limit or validate UTF-8. The selected case does
not append a local terminator to its copied span before the call. A NUL within
those 32 bytes is therefore a separate condition for keeping the scan within
the copied buffer. The constructor body was not reopened.

## Forwarded arguments and stopping point

After normal constructor return, the caller supplies four stack words to
`0x0058EB80`:

1. The word at `S+0x14`, whose low byte was set to B. Its upper 24 bits are
   not established by the selected writes; this is not a proved zero extension.
2. W0, retained in EBX across the constructor under the normal x86 ABI.
3. W1, retained at `S+0x20`.
4. A pointer to the local string object at `S+0x74`.

The complete 34-byte wrapper copies those four words in the same order,
adds `0xE60` to the CharaElement receiver, calls `0x00797C80` and returns
with `RET 0x10`. Its receiver is the address `CharaElement+0xE60`, not
a pointer loaded from that offset. This consumer was not inspected.
The two dwords' clock interpretation, first argument's consumed width, string
retention and visible presentation remain unproved by this forwarding edge.

## Reproduction and boundary

Read only these half-open original instruction ranges:

| Range | Bytes | Purpose |
| --- | ---: | --- |
| `[0x0058D246,0x0058D2BA)` | 116 | Selected case prefix through its forwarding call. |
| `[0x0058EB80,0x0058EBA2)` | 34 | Complete four-word forwarding wrapper. |

The union is 150 code bytes and 40 instructions. Typed reads are exactly
`[0x0058D8BA,0x0058D8BB)`, `[0x0058D7DC,0x0058D7E0)` and
`[0x00F67298,0x00F6729C)`, totaling nine bytes. Map each VA through the
original PE sections and require complete raw backing before comparison and
x86-32 decoding. The parent dispatcher was not read as a full body.

The existing `StartCountdownPacket` catalog label remains. No capture
witness or observed length is introduced from this static read extent.
Post-case helpers and cleanup, the string constructor, the `+0xE60`
consumer, packet production, actual delivery, time units, raid identity and
server policy remain outside this proof. It does not establish safe handling
of malformed input, successful countdown display or a raid callback carrier.
