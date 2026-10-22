# Map 0x00E3 state index

Map clientbound game-message `0x00E3` supplies a u32 state index to
`0x00588A20`. It accepts unsigned values 0 through 5 and ignores an index
already stored in CharaElement's five-bit state field. A changed valid index
updates that field, emits a two-byte internal scene record and forwards a
table-selected u32 through the inline NamePlate interface. The wire index,
scene flag and forwarded value are separate quantities.

## Input and field

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA 6.4.0 with Ghidra 12.1.3 supplied the bounded wrapper bytes, extent and
selected table. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32
checked original raw backing and instruction decoding.

The existing case mapping is
`xivl-client-structs:manifests/fun_0058cca0_walk.json#case_table`, opcode
`0xE3`, and symbol `BCS-Y-0633`. The selected call site at
`0x0058CDF3` loads a dword from game-message `+0x10`, pushes it, and
calls `0x00588A20` with the CharaElement receiver. This is application
`+0`, after the 16-byte game-message header. The caller imposes no length
check or byte mask; it requires four application bytes.

Let C be CharaElement and I the incoming u32. The wrapper compares I with
6 using unsigned `JAE`. Thus every bit pattern outside 0..5, including
negative int32 representations, returns with no wrapper write or helper
call. It also returns without effects when
`(([C+0x270] >> 6) & 0x1F) == I`.

For a changed valid index it replaces only bits 6 through 10:
`new = old ^ ((old ^ (I << 6)) & 0x7C0)`.
Other bits are preserved. These are client input and duplicate-state
constraints, not server quest eligibility or authorization policy.

## Table and ordered effects

The wrapper indexes six eight-byte entries at `0x00FA7AE8`:

| I | Flag byte at entry `+0` | Forwarded u32 at entry `+4` |
| ---: | ---: | ---: |
| 0 | 1 | `0xFFFFFFFF` |
| 1 | 0 | `0xFFFFFFFF` |
| 2 | 1 | `0x00000204` |
| 3 | 0 | `0xFFFFFFFF` |
| 4 | 1 | `0x00000215` |
| 5 | 1 | `0xFFFFFFFF` |

Bytes `+1..+3` are zero in each entry and are not consumed by this wrapper.
The complete 48-byte table has SHA-256
`5f8241c9995d62efa98adcbb6366b3c1773b9f8de3a04666193877dd23c549bb`.

After storing the index, the wrapper constructs the two bytes `{1,flag}`
and calls `0x004D7980` with record selector `0x14`, their address and size
2. The scene body is not the incoming index. The reused broadcaster
contract is `xivl-client-structs:manifests/symbols.json#BCS-Y-1083`;
`manifests/scene_record_choke_point.json`, caller `0x00588A20`, records
the corresponding internal scene-packet ID `0x29`.

For flag zero only, the wrapper calls `0x004D7310` on `[C+0x7C]`.
If its returned pointer is nonnull, it calls `0x00563920` with that receiver
and C as its argument. This is the CharaElement pointer, without an actor-ID
load at this call site. These helper bodies were not inspected; their
complete behavior is not inferred from the pointer argument.

Finally, it loads the table's u32 at entry `+4`, replaces the incoming
stack argument with that value and tail-jumps through virtual slot 7 at
`+0x1C`, with receiver `C+0xBA0`. Invalid or unchanged indices do not
reach this dispatch. Normal storage and helper returns are required.

The existing NamePlate construction and actor offset are owned by
`xivl-client-structs:docs/bazaar-appearance-contract.md`, Native NamePlate
consumer, and `docs/client-architecture.md`, Player model and NamePlate
render boundary. Constructor `0x006A3C00` installs `0x00FCF98C` at
`0x006A3C47`. Under that retained vtable, slot 7 selects `0x006A2E10`
(`config/ffxivgame.vtable_slots.jsonl`, NamePlate slot 7). The
[player nameplate icon contract](../actor/player-nameplate-icons.md#other-controls-and-native-state-icons)
binds this method to secondary-right IconDatas IDs 516 and 533 and their
original exclamation-marker resources. The index's server meaning and
runtime display remain separate evidence boundaries.

## Retained framing and limits

The pinned product
`xivl-opcodes:data/vendor/captures/payload_samples.json#samples.s2c.0x00e3`
retains 12 samples across seven captures: I=2 nine times and I=0 three
times. Each has a 40-byte subevent, 24-byte game message and eight-byte
application. Application `+4..+7` is zero in these samples and is not
consumed by the selected case. The retained sample count is not an
exhaustive packet count. No original capture was decoded here.

Reproduce only these original, half-open instruction ranges:

| Range | Bytes | Purpose |
| --- | ---: | --- |
| `[0x0058CDF3,0x0058CE03)` | 16 | Existing selected u32 load, call and exit branch. |
| `[0x00588A20,0x00588AB1)` | 145 | Complete state wrapper. |

The union is 161 code bytes and 54 instructions. Read only the selected
48-byte table `[0x00FA7AE8,0x00FA7B18)` as typed data. Map the VAs through
the original PE sections, require complete raw backing, compare the REA
bytes and decode complete x86-32 instructions. The first range reuses the
already authenticated caller excerpt; the parent dispatcher was not reopened
as a full body. The wrapper's helpers, NamePlate constructor and slot-7
method were not read.

The existing catalog name `SetActorQuestGraphicPacket` remains a retained
label. This finding proves the range, state field and conditional native
path, without assigning a quest status to every index, identifying the
forwarded resource values, or proving rendering, server policy or client
acceptance.
