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
also remains unresolved. The +0x4 object is a separate non-vtable aggregate
constructed by `0x00773270`. Its +0xec pointer owns the 0x3c-byte terminal
object constructed by `0x0076fc60`; that object likewise has no proven retail
class name.

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
then +0xec, then +0x38. Both native predicates interpret the field as signed
and run their gated call only when it is greater than zero. Zero and negative
values suppress the call. Their selector exclusions are exact 0x7c000062 and
the unsigned ranges 0x10000000..0x10ffffff and
0x14000000..0x14ffffff.

The terminal object's +0x18 member is an ordered key container. The two gated
operations copy a caller-supplied actor handle as the key:

| Predicate | Direct callers | Positive-field effect | Suppressed-field effect |
|---:|---|---|---|
| 0x00578390 | 00587210, 00587370, 005873e0, 005874b0 | 00582bc0 inserts the key if absent, allocating and linking one 0x14-byte node | no container mutation |
| 0x005785d0 | 00585af0, 0058c220 | 00583290 erases every equal key, freeing its nodes and reducing the count | no container mutation |

The four insert callers stage records before `0x005901d0` queues them. The two
erase callers drain queued records through `0x0058ca80`; `0x0058c220` has two
erase call sites, one for the leading record and one in its row loop. The
insert helper's returned iterator remains local and unused. The erase helper's
returned count is ignored.

The gated callees contain no packet builder, Lua/N-API call, UI operation,
animation, movement, targeting, or actor-state call. The drain callers do make
Lua-bound `Application::Lua::Script::Client::Control::CharaBase` calls, and
`0x0058c220` also contains UI and actor-state branches, but those operations
are outside the ActionCheck branch. ActionCheck therefore gates only the local
ordered-container mutation in these paths; it does not gate those externally
visible operations. No gated result reaches Lua or another public consumer,
and no network emission was found below either gated callee.

A complete Ghidra reference export found four calls to `0x00578390`, three
calls to `0x005785d0` (two within `0x0058c220`), and no data aliases to either
predicate. The generated-assembly corpus independently contains those same
direct calls. Computed or dynamic indirect access remains outside this bounded
negative result, as does the high-level purpose of the ordered container.

## Evidence boundary

The machine-readable finding is
[`config/s2c_0193_native_state.json`](../../config/s2c_0193_native_state.json).
It is checked by `tools/verify_s2c_0193_native_state.py` and the repository
gate. RTTI and vtable identities come from
[`config/ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`config/ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
This finding does not name opcode 0x0193, assign server behavior, import a
packet type, infer timer units or eligibility policy, import SetControlState
semantics, or invent names for the four groups or ActionCheck container.
