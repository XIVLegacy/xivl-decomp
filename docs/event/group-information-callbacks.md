# Group information callback fire sites

The client raises the Lua callbacks `_onUpdateMemberInformation`,
`_onUpdateGroupInformation` and `_onUpdateGroupCurrent` from group
bookkeeping code. This page records where each name literal is used and which
native routines reach it. It does not establish when a server drives those
routines. The group store itself is in
[group-shared-work-system.md](group-shared-work-system.md) and the callback
argument convention of the event callbacks is in
[event-base-callback-arguments.md](event-base-callback-arguments.md).

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Ghidra 12.1.4 through ghidra-cli: string reads of the
literals, `x-ref to` on the literals and on the fire routines, and
disassembly of `0x00700760`.

## Fire routines

| Literal | Address | Only referenced from | Callers of that routine |
| --- | --- | --- | --- |
| `_onUpdateMemberInformation` | `0x00FD48DC` | `0x0070082B` in `0x00700760` | `0x006C95B5` in `0x006C8E40`, `0x006C34A9` in `0x006C33F0`, `0x006C37B1` in `0x006C3720`, `0x006C7A5C` in `0x006C7870` |
| `_onUpdateGroupInformation` | `0x00FD48C0` | `0x00709E1D` in `0x00709CA0` | `0x006CD8BF` in `0x006CD7D0`, `0x006C34BF` in `0x006C33F0`, `0x006C012E` in `0x006C0080` |
| `_onUpdateGroupCurrent` | `0x00FD4938` and `0x00FD4950` | `0x00700F73` in `0x00700E70` and `0x007010C9` in `0x00700FF0` | both routines are called only from `0x006C2200` (`0x006C22F5` and `0x006C228C`) |

STATIC for every address above. The caller of `0x006C2200` is `0x006CDF20` at
`0x006CE005`, the routine that `group-shared-work-system.md` describes.

## Member information index

`0x00700760` takes three stack arguments. It resolves a group object from the
first two through `0x006E23B0` (`0x0070079B..0x007007A0`) and builds an operand
list whose single entry is the third argument plus one (`0x00700802..0x0070081F`,
`ADD EAX,1` at `0x00700807`). It then dispatches through `0x00CC7A90` with the
literal at `0x00FD48DC`, the group object as the receiver `ECX` (`0x00700855`)
and that operand list. So the slot number that a script receives for
`_onUpdateMemberInformation` is the native member index plus 1. STATIC for the
store of the sum, INFERRED for the single-entry shape of the operand list. The
loop caller `0x006C3720` takes the index from `0x006C3320`.

## Boundary

The guards in `0x006C33F0` and `0x006C0080` that select between the
information and member routines are not decoded here. The walk of `0x006C2200` and the
identity that receives `_onUpdateGroupCurrent` are in
[group-work-request-and-current-consumer.md](../net/group-work-request-and-current-consumer.md).
Which packet reaches `0x006C2200` is not established by this page.

## Reproduction

Run `ghidra find string "_onUpdateMemberInformation"` and the same for the two
other literals, then `ghidra x-ref to` each literal address and each routine
address in the table. Disassemble `0x00700760` and match the locators.
