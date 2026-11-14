# Talk condition receiver

The `SetTalkEventConditionReceiver` copies two payload bytes and a name into a
retained receiver. Its receive slot casts the target to `NpcBase` and calls a
setter that appends a TalkBlock or a TalkNonBlock condition to the NPC talk
vector at `NpcBase+0xE8`. The second payload byte selects the leaf. The first
payload byte is the value the TalkBlock constructor copies.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses are VAs. The observations come from
ghidra-cli disassembly listings, cross-reference queries and memory reads of
that program. Stack argument positions were counted from the pushes in each
prologue. Static inspection did not execute the client or observe a packet.

## Receiver construction

NullActorImpl vtable cell `0x00FE0374` (slot 50 of the table at `0x00FE02AC`)
selects `0x00761B90`. That function allocates `0x5C` bytes at `0x00761BC1`
and constructs the receiver at `0x00761C1A`. Let `P` be the payload pointer it
loads at `0x00761BD7`. The constructor receives three stack arguments, in
order:

| Argument | Source | Conversion |
| --- | --- | --- |
| 1 | `P+2`, formed at `0x00761C14` | name pointer |
| 2 | byte `P[0]`, loaded at `0x00761BFE` | `0x0075AEE0` stores the byte, or the byte at `0x01355296` when it is zero |
| 3 | byte `P[1]`, loaded at `0x00761BDB` | `0x0078DDE0` stores the byte unchanged |

The constructor `0x0089DE70` writes vftable `0x010573DC` at `0x0089DEAA`,
builds the name at receiver `+0x04` through `0x00447260` at `0x0089DEBA`,
stores the argument 2 byte at `+0x58` (`0x0089DECE`) and the argument 3 byte
at `+0x59` (`0x0089DED3`). The finished receiver is passed to `0x007945A0`
on NullActorImpl `+0x08` at `0x00761C58`, as in the retained pattern B of
the [actor-implementation dispatch](../net/actorimpl-receiver-dispatch.md#two-dispatch-patterns).
The walker that later calls the receive slot and the LuaActorImpl slot 50
body were not traced here.
The `xivl-opcodes` catalog binds this receiver class to s2c `0x012E`
`SetTalkEventCondition`.

## Receive and setter

Vftable cell `0x010573E0` selects `0x0089DF60`. It calls `__RTDynamicCast`
`0x009DA6CC` from `ActorBase` (`0x01270964`) to `NpcBase` (`0x012709E4`) and
then calls `0x006F29B0` with ECX set to the cast result. The stack arguments
are receiver `+0x04`, `+0x58` and `+0x59`, pushed right to left at
`0x0089DF7E..0x0089DF89`. The receive slot has no null check of the cast
result.

The setter compares the byte at its third argument (receiver `+0x59`) with
mutable cell `0x0134C3FE` at `0x006F29E5`:

| Comparison | Allocation | Constructor call | Arguments |
| --- | --- | --- | --- |
| Equal | `0x60` | `0x006F2A10 -> 0x00892D00` (TalkBlock) | name, pointer to receiver `+0x58` |
| Unequal | `0x5C` | `0x006F2A35 -> 0x00892E30` (TalkNonBlock) | name |

Either constructed condition is appended through `0x00725ED0` with ECX set
to `NpcBase+0xE8` at `0x006F2A62`. The setter has the shape of the emote
setter in [NPC emote conditions](npc-emote-event-dispatch.md) and of the
notice setter in [status-condition receivers](status-condition-receivers.md).

The TalkBlock constructor copies its byte from the pointer argument
(`0x00892D46`). Its slot8 returns the byte at condition `+0x5C`, the priority
the [event-base admission](event-base-callback-arguments.md#manager-admission-and-replacement)
compares. TalkNonBlock slot8 returns the byte at `0x01355296`. Both
callbacks are listed in the
[callback vectors](event-base-callback-arguments.md): `_onTalkEvent` for the
TalkBlock leaf and `_onTalkRequest` for the TalkNonBlock leaf.

## Consequence and limits

INFERRED. Payload byte 1 selects the TalkBlock leaf when it equals the
current byte at `0x0134C3FE` and the TalkNonBlock leaf otherwise. Payload
byte 0 is the TalkBlock priority, and a zero byte is replaced by the byte at
`0x01355296`. Both cells lie in zero-fill storage, so an unwritten cell reads
0. The run-time writers of `0x0134C3FE` and `0x01355296` are UNKNOWN.

UNKNOWN: the writers of `NpcBase+0x5C` and `+0x5D`, the effect of condition
slot 9 that the [SetEventStatus receiver](status-condition-receivers.md)
calls on a talk entry, and the route that fires
`/Command/System/TalkCommand` on a confirmed target.

## Reproduction

Disassemble `0x00761B90`, `0x0078DDE0`, `0x0075AEE0`, `0x0089DE70`,
`0x0089DF60` and `0x006F29B0` with `ghidra disasm`, and confirm the
callsites above. `ghidra x-ref to 0x00761B90` returns the data reference
`0x00FE0374`. `ghidra memory read 0x010573DC 8` returns the two vftable cells
`0x008A1280` and `0x0089DF60`. `ghidra memory read 0x0134C3FC 4` reads zero.
