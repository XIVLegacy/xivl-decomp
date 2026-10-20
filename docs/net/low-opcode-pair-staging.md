# Map low-opcode pair staging

Map clientbound game-message opcodes `0x0008..0x000B` supply separate u32
value and u8 flag arrays to a shared staging helper. Opcode `0x0006` erases
the staged range. It does not invoke a per-entry operation. These mechanics
do not identify a flag meaning, prove actor deletion, or establish the
complete sequence involving `0x0007`.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA 6.4.0 with Ghidra 12.1.3 supplied bounded byte reads and helper extents.
Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked original
file backing, byte equality and complete instruction decoding.

The [receive table](receive-dispatch-ranges.md) owns the opcode-to-case
mapping. The packet base below is the game-message header, whose opcode
word is at `+2`; application offsets subtract its 16-byte prefix. These
are game-message opcodes, distinct from an outer subpacket type with the
same integer. The existing dispatcher owner and packet-base contracts are
reused from `xivl-client-structs:manifests/symbols.json#BCS-Y-0279`.

## Consumed fields

With stable packet bytes and normal helper returns, the cases consume these
fields:

| Map clientbound opcode | Count | Application u32 value i | Application u8 flag i | Read extent |
| --- | --- | --- | --- | --- |
| `0x0008` | u8 at `+0`, reloaded after each append | `+4+4*i` | `+0x24+i` | For nonzero n, at least `max(4+4*n,0x24+n)` bytes; no array read for zero. |
| `0x0009` | Literal 16 | `+4*i` | `+0x40+i` | 80 application bytes. |
| `0x000A` | Literal 32 | `+4*i` | `+0x80+i` | 160 application bytes. |
| `0x000B` | Literal 64 | `+4*i` | `+0x100+i` | 320 application bytes. |

The selected `0x0008`, `0x0009` and `0x000B` instructions were checked
directly. The `0x000A` shape reuses
`xivl-client-structs:manifests/low_switch_remaining_cases.json#case_table`,
opcode `0xA`; its body was not reopened. Each loop supplies a local pair
whose u32 is at `+0` and u8 flag at `+4` to `0x0081F090`. Neither the
selected cases nor that helper impose a packet-length bound. The fixed
loops always attempt their literal count under normal returns. No byte
swap occurs in the selected field loads.

The `0x0008` loop tests and reloads only the low byte of the count word.
It has no local limit of 8 or 11. Its value and flag regions overlap for
counts above 8; this is not evidence of a supported larger wire capacity.
The remaining three bytes after the count are not consumed by this case.
The read extents above are native requirements, not observed packet sizes
or server validation policy.

The pinned catalog products
`xivl-opcodes:data/vendor/captures/payload_layouts.json#layouts.s2c.0x0008`
and `payload_samples.json#samples.s2c.0x0008` contain 19 records, each an
80-byte subevent with a 64-byte game message and 48-byte application.
Their count bytes range from 1 through 8, and the eight bytes in the flag
region are zero. The data supports eight separate u32 slots and eight flag
slots. It does not support a count-through-11 observation or an all-u32
array occupying the flag region. These are observations of the promoted
products; no additional original capture was decoded here.

## Shared staging and erase

At the checked case entries, let R be the current EDI value and Q be
`R+0x17810`. Opcode `0x0006` passes Q to `0x0081EB90`. The checked
`0x0008`, `0x0009` and `0x000B` loops pass the same expression to
`0x0081F090`. This is the concrete receiver expression; assigning R to
the dispatcher's entry receiver requires the retained register relation.
The 64-byte prologue checked here contains stack, SEH and cookie setup
only and does not reproduce that relation. In particular, it does not
authenticate the separate `RaptureElementContainer+0x950` partition.

For valid aligned vector storage, `0x0081F090` uses Q `+4/+8/+0xC`
as begin/end/capacity. With nonnull begin and spare capacity, it calls
`0x00556880` with the supplied pair and a count of one, then advances end
by 8. The other path calls `0x0081EE20`. Those construction and growth
helpers were not inspected. The call and end advancement alone do not
prove copied pair contents, successful growth or storage lifetime.

`0x0081EB90` loads the current begin and end and calls `0x00440940`
with an output iterator address and the owner/position pairs `(Q,begin)`
and `(Q,end)`. The latter is a range-erase operation. Its general nonempty
path copies the suffix after last over first in eight-byte steps, then
stores the shortened end at Q `+8`. For the supplied full range, last is
the current end, so the suffix is empty and the stored end is begin. An
already empty range leaves end unchanged at begin. Begin, allocation and
capacity have no explicit stores on this path.

These statements require valid ordered, stable vector and output storage,
normal ABI returns and no aliasing or external mutation. Invalid iterator
conditions call `0x009D22B4`; its effects were not inspected. The erase
body makes no other call and invokes no per-element visitor. Thus the
supported `0x0006` effect is clearing the staged range, without applying
each pair or proving any actor-state change.

## Reproduction and boundary

Read only these original, half-open instruction ranges:

| Range | Bytes | Purpose |
| --- | ---: | --- |
| `[0x004DC690,0x004DC6D0)` | 64 | Old dispatcher prologue, without the receiver binding. |
| `[0x004DC7D6,0x004DC7E6)` | 16 | `0x0006` receiver and call. |
| `[0x004DC7E6,0x004DC82D)` | 71 | `0x0008` count, array loads and loop. |
| `[0x004DC82D,0x004DC869)` | 60 | `0x0009` fixed array loop. |
| `[0x004DC89D,0x004DC8DC)` | 63 | `0x000B` fixed array loop. |
| `[0x0081EB90,0x0081EBC9)` | 57 | Complete old range-clear wrapper. |
| `[0x0081F090,0x0081F112)` | 130 | Complete old staging wrapper. |
| `[0x00440940,0x004409AA)` | 106 | Complete range erase. |

The union is 567 bytes and 198 decoded instructions, with no typed-data
read or thunk. For each VA, map `VA-0x00400000` through the PE section
table and require the whole range to have original raw backing. Compare
the REA bytes with that original range before decoding. The three helper
extents are contiguous and include their returns; the dispatcher was not
read as a full body.

`0x0007` and `0x000A` bodies, diagnostic behavior, the two construction
helpers, actual queued values and flag meanings, the absolute queue-owner
binding, subsequent actor handling and server policy remain outside this
proof. A staging receiver or an empty-vector operation does not establish
the name or ordering of a complete actor-deletion protocol.
