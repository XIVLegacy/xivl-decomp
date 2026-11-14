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

## Handler arguments and stored words

The dispatcher arm at `0x004DD36F` pushes three message words and calls
`0x00578C90` with the route state as receiver: the subopcode from message
`+0x10`, the value from `+0x14` and a clock word from `+0x08`. Message `+0x08`
is inside the 16-byte game-message header
([wire protocol](wire-protocol.md#world-chat-selected-group-route) records the
same header offset base). Every claim in this section is STATIC, read from the
instructions in the `staticChecks` rows of the manifest.

| Subopcode | Stored word | Destination |
|---|---|---|
| 0x00..0x0f | sentinel: 0, otherwise clock + value (mod 2^32) | vector entry at the subopcode, writer `0x0075F3E0` |
| 0x10, 0x11, 0x12, 0x16 | sentinel: 0, otherwise clock + value (mod 2^32) | state `+0x10`, `+0x14`, `+0x18`, `+0x1c` |
| 0x13 | sentinel: nothing, otherwise the raw value | terminal object `+0x38` |
| 0x14, 0x15 | nothing | user-control flag, see below |
| above 0x16 | nothing | the handler returns |

The sentinel is `0xFFFFFFFF`. The word is selected once at `0x00578CBD`: when
the value equals the sentinel the selected word is zero, otherwise it is the
clock plus the value with 32-bit wraparound. Every store arm passes that
selected word to its writer, so a sentinel on a store subopcode stores zero and
does not leave the slot unchanged. The four scalar readers in the previous section
return the stored word unchanged.

Subopcode 0x13 uses the value directly. The sentinel arm at `0x00578D5A` reads
the word at terminal object `+0x38` and runs the diagnostic path through
`0x0075BC90` without a store. Any other value takes the branch to
`0x00578E94`, which stores the raw value argument at terminal object `+0x38`
and then runs the same diagnostic call. The clock is not part of that store. The
ActionCheck consumers below read that word as signed.

Subopcodes 0x14 and 0x15 share one value exclusion set, 0, 0x15 and 0x16, and
one flag byte at route state `+0x3a`. Subopcode 0x14 acts when the byte is
clear: it calls the setup-all method `0x0075B300` and sets the byte. Subopcode
0x15 forwards the value to `0x00576020`, which returns when the value is
excluded or the byte is clear, and otherwise calls teardown-all `0x0075B360`
and clears the byte. The two arms therefore alternate setup and teardown, and a
repeated 0x14 or 0x15 changes nothing. The four scalar word writers are plain
stores. The vector writer calls `0x009D22B4` when the index is at or past the
vector size and then stores anyway. Whether any subopcode below 0x10 can reach
that call depends on the vector size, which these bodies do not show.

Not established: the initial values the constructors give the timer words and
the ActionCheck word, the vector size, and the meaning of any stored word.

### Consumer implications

A client model of this handler may rely on the following.

- A store subopcode with value `0xFFFFFFFF` stores zero. Readers return the
  stored word unchanged, and what zero means is not established.
- A non-sentinel store is `(clock + value) mod 2^32`, taking the clock from the
  game-message header word at `+0x08`.
- Subopcode 0x13 stores the raw value and ignores the clock. The sentinel is a
  query that stores nothing.
- Apply the 0x14 and 0x15 value exclusions and the flag byte as above. The
  0x15 value set equals the 0x14 value set.

## Warp end release

STATIC. Ghidra CLI cross references and disassembly of retail
`ffxivgame.exe` 1.23b (15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, VAs at
image base `0x00400000`). C is a CharaElement. Its state word is the s16 at
`C+0xE4`, and bits 1 to 5 of the word at `C+0xE8` are the tick phase.

The guard `0x00576020` has two direct callers: the 0x15 arm at `0x00578FBB`
and `0x0058907F` inside `0x00589030`.

| Routine | Observation |
| --- | --- |
| `0x00589000` | Returns when bit `0x40` of the byte at `C+0xE8` is set. Otherwise passes the sign-extended state word to `0x00574ED0` with ECX `[C+0x80]` (`0x0058900C..0x0058901A`) and sets the bit (`0x0058901F`) |
| `0x00589030` | Returns when bit `0x40` is clear (`0x0058904D..0x00589054`). Otherwise passes the sign-extended state word to `0x00574EE0` with ECX `[C+0x80]` (`0x00589068`), takes the route state from `0x004D7470` on `[C+0x7C]` and calls `0x00576020` on it with the same state word (`0x0058906D..0x0058907F`), then clears the bit (`0x0058908A`) |
| `0x0058A090` | The tick. `0x004D86B0` on `[C+0x7C]` is compared with C and the result kept in BL (`0x0058A0E2..0x0058A0F3`). Phase 1 calls `0x00589000` only with BL set (`0x0058A1DD..0x0058A1E7`). Phase `0xC` stores `0x14` at byte `C+0xEA` (`0x0058A684`). Phase `0xD` decrements that byte while it is nonzero and then calls `0x00589030` (`0x0058A6A0..0x0058A6BC`). Phase `0xF` stores `0xFFFF` in the state word (`0x0058A737`) and, with BL set, calls `0x00586FB0` (`0x0058A778`) |
| `0x0058A7F0` | Called from the state setter `0x0058ADC0` (`0x0058AE62`) and from `0x0058E860`. When the state word is not `0xFFFF` it stores 1 at `0x01338D28`, stores `0xFFFF` in the state word (`0x0058A80D`) and posts internal message `0x27` with `{2, 0}`. With C equal to the `0x004D86B0` result it then calls `0x00589030` (`0x0058A8C6`), which therefore passes `0xFFFFFFFF`. It ends by clearing the phase bits (`0x0058A8CB`) |
| `0x008A4410` | After the `_onPreWarp` firer `0x006FEDE0` it calls setup-all `0x0075B300` (`0x008A44B3`). That call is not conditional on the firer branch |
| `0x008A44D0` | Calls teardown-all `0x0075B360` first (`0x008A450E`). When byte `+0x166` of the object at receiver `+0xC` is set it calls `0x0075B330` and clears bytes `+0x166` and `+0x165` (`0x008A4516..0x008A452E`). It then calls the `_onPostWarp` firer `0x006FEF10` and `0x00703F60`. For state 4 or `0x10` it calls `0x0075B4C0` with 0 (`0x008A4597..0x008A45A7`) |

`0x00586FB0` is the c2s `0x0007` emitter in
`xivl-client-structs:manifests/c2s_emit_sites.json`. The slot 41 and 42
thunks `0x00574ED0` and `0x00574EE0` and the two gates are
`xivl-client-structs:manifests/symbols.json` rows
`OnPreWarpSlot41Dispatcher_FUN_00574ED0`,
`OnPostWarpSlot42Dispatcher_FUN_00574EE0`,
`PreWarpEngineGate_DesktopWidget_FUN_008A4410` and
`PostWarpEngineGate_DesktopWidget_FUN_008A44D0`.

Direct callers of the three group helpers:

| Helper | Callers |
| --- | --- |
| setup-all `0x0075B300` | `0x008A4410`, `0x008A45D0`, `0x006E3260`, the 0x14 arm at `0x00578FAD` |
| `0x0075B330` | `0x008A44D0`, `0x008A4720`, `0x006E32A0` |
| teardown-all `0x0075B360` | `0x00576020`, `0x008A44D0` |

INFERRED. The value of subopcodes 0x14 and 0x15 lies in the domain of the
state word. With the flag byte set by a 0x14, the end routine `0x00589030`
of a held warp state releases it through the same guard as 0x15 unless the
state word is 0, 0x15 or 0x16. The phase-1 call to `0x00589000` runs before
the phase-`0xF` call to the `0x0007` emitter, and so does the phase-`0xD`
release. The pre and post warp gates pair one setup-all with one
teardown-all around the warp.

Not traced: the receiver class at `[C+0x80]` and which implementation its
slots 41 and 42 reach at runtime, the conditions that move the tick from
phase 1 to phase `0xC`, the object at receiver `+0xC` of `0x008A44D0`, and
the meaning of `0x0075B330` and of the `0x0075B4C0` call.

Reproduction:

```text
ghidra x-ref to 0x00576020
ghidra x-ref to 0x00589000
ghidra x-ref to 0x00589030
ghidra disasm 0x00589000 -n 40
ghidra disasm 0x0058A090 -n 80
ghidra disasm 0x0058A600 -n 140
ghidra disasm 0x0058A7F0 -n 80
ghidra disasm 0x008A4410 -n 50
ghidra disasm 0x008A44D0 -n 80
ghidra x-ref to 0x0075B300
ghidra x-ref to 0x0075B330
ghidra x-ref to 0x0075B360
```

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
gate. Run `python tools/verify_s2c_0193_native_state.py --exe orig/ffxivgame.exe`
to compare each `staticChecks` row with the pinned executable bytes. That form is
a local retail-input gate, and CI runs the manifest, mutation and model tests
only. RTTI and vtable identities come from
[`config/ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`config/ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
This finding does not name opcode 0x0193, assign server behavior, import a
packet type, infer timer units or eligibility policy, import SetControlState
semantics, or invent names for the four groups or ActionCheck container.
