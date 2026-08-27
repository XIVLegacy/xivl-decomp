# s2c 0x0193 native state

The retail client routes s2c opcode `0x0193` through `0x004dc690` and then
`0x00578c90`. The second function receives a 0x3c-byte state member at
`RaptureElementContainer+0x510`. The nearby
`RaptureElementContainer+0x17758` member is instead the concrete 0x58-byte
`Application::Main::SqwtInterface::RaptureUserControl` object.

## Owner and layout

`0x004b2df0` allocates 0x17d58 bytes. `0x004dc3a0` constructs MainModule at
the allocation base and constructs RaptureElementContainer at offset 0x10.
The container constructor `0x004dbf40` initializes the route state at +0x510,
constructs RaptureUserControl at +0x17758, and starts the next member at
+0x177b0. The accessor `0x004d7580` returns the RaptureUserControl address.

The route-state constructor `0x00577fd0` covers offsets +0x00 through +0x3b.
It has no discovered vtable or RTTI class. Its pointer at +0x4 leads through
+0x10c to the timer state used by the dispatcher; that terminal state's class
also remains unresolved.

## Timer readers

| Subopcode | Storage | Reader | MyPlayer slot | Lua/N-API callback |
|---|---:|---:|---:|---|
| 0x00..0x0f | u32 vector | 0x0075f420 | 88 | `_getOccupancyContentsTime` |
| 0x10 | +0x10 | 0x0075d220 | 89 | `_getNormalBehestTime` |
| 0x11 | +0x14 | 0x0075d240 | 90 | `_getCompanyBehestTime` |
| 0x12 | +0x18 | 0x0075d260 | 91 | `_getWarpRecastTime` |
| 0x16 | +0x1c | 0x0075d280 | 132 | `_getNMRushUpdateTime` |

Each reader has exactly one direct caller, the listed MyPlayer wrapper. The
low-vector wrapper decrements its callback input before indexing. That closes
the vector's public callback but does not establish individual meanings for
indices 0x00 through 0x0f or any timer unit.

## RaptureUserControl groups

`0x0054e440` fills four adjacent 0x14-byte command records. The setup-all
caller `0x0075b300` invokes one setup method per record, while teardown-all
`0x0075b360` reads each count and invokes its paired decrement when positive.

| Group | Record | Count | Setup / decrement / reader | Command-record membership | Setup registrations |
|---:|---:|---:|---|---|---|
| 1 | +0x08 | +0x18 | 0054b440 / 0054b600 / 0054b610 | MoveCharacter, ShiftMoveCharacter, MoveCharacterAutoRun | MoveCharacter, ShiftMoveCharacter |
| 2 | +0x1c | +0x2c | 0054b620 / 0054b630 / 0054b640 | ChangeTargetNext, ChangeTargetPrev, ChangeEnemyTargetNext, ChangeEnemyTargetPrev, ChangeBattleTargetNext, ChangeBattleTargetPrev, ChangeTargetMode | none; setup only increments |
| 3 | +0x30 | +0x40 | 0054b650 / 0054b660 / 0054b670 | LockTarget, MoveCamera, ChangeCameraMode, ChangeCameraLock | none; setup only increments |
| 4 | +0x44 | +0x54 | 0054b680 / 0054b8c0 / 0054b8d0 | ForwardCameraOn, ForwardCameraOff, BackwardCameraOn, BackwardCameraOff | MoveCamera, ForwardCameraOff, BackwardCameraOff |

The retail literals are prefixed `RaptureCommands.`. The setup-registration
column is intentionally separate from command-record membership: group 4
registers one callback whose command resides in group 3. Numeric position does
not establish a higher-level group name.

## ActionCheck consumers

Subopcode 0x13 queries or writes the u32 reached through route-state +0x4,
then +0xec, then +0x38. The diagnostic path is not its only consumer:

- `0x00578390` tests the same field before calling `0x00582bc0`; its direct
  callers are `0x00587210`, `0x00587370`, `0x005873e0`, and `0x005874b0`.
- `0x005785d0` tests the same field before calling `0x00583290`; its direct
  callers are `0x00585af0` and `0x0058c220`.

No Lua/N-API consumer and no other exact +0x4 -> +0xec -> +0x38 path was found
in the bounded local retail disassembly sweep. Aliases and computed indirect
access remain outside that negative result.

## Evidence boundary

The machine-readable finding is
[`config/s2c_0193_native_state.json`](../../config/s2c_0193_native_state.json).
It is checked by `tools/verify_s2c_0193_native_state.py` and the repository
gate. RTTI and vtable identities come from
[`config/ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`config/ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
This finding does not name opcode 0x0193, assign server behavior, import a
packet type, infer timer units, or invent names for the four groups.
