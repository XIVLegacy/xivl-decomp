# Battle posture state setter and end-of-clip callback

The battle posture machine requests its clips through one setter,
`0x008ACB90`. This page records the setter's admission and failure
paths, the state values its callers pass and the end-of-clip callback
`0x008AB8B0` that chooses the next state. The subphase handlers that install
and release the machine are in
[Locomotion animation selection](locomotion-selection.md#battle-activation-and-deactivation-phases).

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations come from Ghidra 12.1.4 through ghidra-cli disassembly of
the cited functions and PE section reads of the two tables.

## Setter 0x008ACB90

`0x008ACB90` is `__thiscall` with five stack arguments in the order state,
force, blend, phase source and phase add. The receiver is the machine and
`[receiver]` is the owner. STATIC.

- It reads owner `+0x2B74` and keeps whether bit `0x40` is set
  (`0x008ACBCE..0x008ACBDA`).
- When the requested state equals receiver `+0x2C` and the force byte is zero
  it returns without a request (`0x008ACBDD..0x008ACBEE`).
- Otherwise it stores the previous state at `+0x30` and the new one at `+0x2C`
  (`0x008ACC52`, `0x008ACC55`), builds the clip name through `0x008A8DD0`
  and requests it through `0x007B9030` (`0x008ACE9B`). Playback arguments are
  in [Locomotion animation selection](locomotion-selection.md).
- With the owner flag set, the names `cbbp_u_activ` and `cbbp_u_deact` go
  through `0x007BF750` instead (`0x008ACDDA`, `0x008ACE1A`). The meaning of the
  flag is not traced.
- The loop byte is 1 for state 1 and for states `0x11` through `0x14`
  (`0x008ACD29..0x008ACD3F`) and 0 otherwise.

### Callers and state values

| Site | Call | Note |
| --- | --- | --- |
| `0x008C0581` | state `0x1D`, force 1, blend 0 | constructor `0x008C03C0`, first flag byte zero and second nonzero |
| `0x008C062C` | state 1, force 1, blend 0 or 10 | same constructor, blend 0 when the first flag byte is nonzero (`0x008C056A`) and 10 when both are zero (`0x008C0624`) |
| `0x008BF07E` | state `0x1E`, force 1, blend 10 | leave handler `0x008BF040` |
| `0x008AB985..0x008AB9DE` | states `0x11` to `0x14`, force 0, blend 10, phase from the callback | hit-return cases of the callback below |
| `0x008AB91D` | state 1, force 0, blend 0 | draw end, only with the owner flag clear |
| `0x008AB9EA` | state 1, force 0, blend 10, phase 0 | default case of the callback |

After the `0x1D` request the constructor may call state 1 with force 1 and
blend 0 at `0x008C05A2` after storing 1 at receiver `+0x2C`. The two local
bytes that gate it (`0x008C0586`, `0x008C058C`) were not traced. STATIC.

## Failed playback

`0x007B9030` returns zero when it cannot start a clip. The setter then calls
`0x007B1AA0` with the clip name on the object at `owner+0xBF0`
(`0x008ACEBB`). The alias request that follows is skipped for states `0x1D` and
`0x1E` (`0x008ACEC8..0x008ACED3` jumps to `0x008ACF2B`). When there is no alias,
or the alias request fails, `0x008ACF2B` calls the notify only when the loop
byte is 0. The notify is virtual slot `+4` of the callback object built at
`0x008ACD1A` by `0x008C45A0` with `0x008AB8B0`, with arguments
`(0x007A4A90(owner+0xBF0) + 0x10, new state, 0)`. A missing clip therefore
ends at once through the callback below. STATIC.

The field stop handler `0x008B2970` follows the same pattern. It builds its
callback with `0x008C4AE0` and `0x008ABDF0` (`0x008B2ABC..0x008B2AC7`), requests
through `0x007B9030` (`0x008B2B48`), looks up an alias through `0x007B1AA0`
(`0x008B2B67`) and, with none, calls the same slot at `0x008B358C..0x008B35AF`.

## End-of-clip callback 0x008AB8B0

The callback reads the finished state from its second stack argument and
dispatches on `state - 2` through the byte map at `0x008ABA24` and the jump
table at `0x008ABA04` (`0x008AB8EB..0x008AB8FE`). A state above `0x1E` takes
`0x008AB9FB`, which returns. STATIC.

| Finished state | Handler | Request or action |
| --- | --- | --- |
| 2 to 7 and `0x15` to `0x1C` | `0x008AB9EA` | setter with state 1, force 0, blend 10, phase 0 |
| 8 to `0x0D` | `0x008AB97E` | setter with state `0x11`, force 0, blend 10, the callback's phase argument |
| `0x0E` | `0x008AB999` | state `0x12`, same arguments |
| `0x0F` | `0x008AB9B4` | state `0x13`, same arguments |
| `0x10` | `0x008AB9CF` | state `0x14`, same arguments |
| `0x11` to `0x14` | `0x008AB9FB` | returns without a request |
| `0x1D` | `0x008AB905` | calls `0x0065B190` and `0x007A3040` on the owner, then state 1, force 0, blend 0 only when the owner flag is clear, then virtual slot `+4` of the object at receiver `+4` |
| `0x1E` | `0x008AB947` | calls `0x007A3040`, virtual slot `+4` of the object at receiver `+0x18`, and `0x007A4120(owner+0x590)` when the owner flag is clear |

A finished draw therefore cuts to the idle with blend 0 and a finished
sheathe issues no setter request. Every finished stop returns to state 1 with
blend 10.

## Stop foot predicate

The stop handler `0x008B2970` branches on `0x007A4A10` at `0x008B29D5`. That
function calls `0x007D0B70` on the object at `[owner+0xBF0]+0x2130` and
returns 1 when the result is not negative. `0x007D0B70` returns 1 when byte
`+0x68` of that object has bit 2 set and -1 otherwise. STATIC. The writer of
that bit is not traced.

## Boundary

The unit of the blend count, the weight law and whether the outgoing clip keeps
advancing are not read here. The upper layer behind owner flag `0x40`, the
names inside the `0x008A8DD0` table and the writer of the foot bit are open.
No live retail session was observed.

## Reproduction

Disassemble `0x008ACB90`, `0x008AB8B0`, `0x008C03C0`, `0x008BF040` and
`0x007A4A10` and match the locators. Read 32 bytes at `0x008ABA04` and 32
bytes at `0x008ABA24` through the PE section table, then apply the
`state - 2` indexing above.
