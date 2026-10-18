# StartServerOrderEventFunction receiver

`StartServerOrderEventFunctionReceiver` carries an incoming named Lua function
request. Its slot 2 checks parameter-reference dependencies; slot 3 hands the
record to local execution preparation. The
[native delegate producer](../script/delegate-command-producer.md) owns the
complete payload-to-Lua-frame contract, artifact identity and reproduction.
The generic path can serve dialogue, commands and other supplied functions.

## Vtable and receiver storage

The pinned receiver vtable is VA `0x010574C8`, RVA `0x00C574C8`.
Function addresses below are VAs in the pinned Windows 1.23b `ffxivgame.exe`.
Original PE table cells and retained Ghidra instructions were checked through
pefile 2024.8.26 and focused Capstone 5.0.7 decoding.

| Slot | Cell VA | Target VA | Established role |
| --- | --- | --- | --- |
| 0 | `0x010574C8` | `0x008A1BB0` | Scalar deleting destructor. |
| 1 | `0x010574CC` | `0x0089F430` | Allocates and copy-constructs a heap receiver. |
| 2 | `0x010574D0` | `0x0089EB20` | Advances `this` by `0xCC` and calls dependency gate `0x0089E8E0`. |
| 3 | `0x010574D4` | `0x0089E260` | Resolves the trigger and hands stored function/input data to `0x006E1140`. |
| 4 | `0x010574D8` | `0x0089E060` | Compares the trigger key with the separate sentinel value. |

`0x0089EDB0` constructs the receiver. The stored function string is at
`+0x10`, event string at `+0x64`, trigger/owner keys at `+0x08/+0x0C`,
and event-type byte at `+0xB8`. The copied parameter vector has bounds
`+0xC0/+0xC4`; the separate dependency vector has bounds `+0xD0/+0xD4`
inside the subobject at `+0xCC`. Their purposes and strides differ.

The key wrappers copy u32 values through `0x00CC9320`; they are not native
object pointers. The string constructor uses NUL-terminated input, with
`dword[0x00F67298]=0xFFFFFFFF`; it does not enforce the two 32-byte payload
field widths. Successful construction, allocation and input termination
remain conditions.

## Parameter-reference dependency gate

The dependency collector `0x0089E3F0 -> 0x0089E320` runs the typed parser
through visitor table `0x01057438`. Its slot 5, `0x008A2710`, handles the
relevant actor-key references. It applies the `0x00CC7190` classifier,
performs registration bookkeeping, and attempts key resolution. When that
lookup remains zero, it appends an eight-byte dependency item: u32 key at
`+0`, zero byte at `+4`. The other bytes in that second word have no
established semantic role. The item is not a trigger/function pair.

`0x0089E8E0` operates on that dependency vector:

1. An empty or unallocated range returns admission byte 1.
2. The forward pass resolves each key through `0x00CC7A50`. On a miss it
   tests membership through `0x00CC7180`; if absent there too, it invokes
   `0x00CC78C0`.
3. The reverse pass resolves trailing keys and reads the registry-entry flag
   through `0x00CC72A0`. A null resolution or zero flag stops removal.
   Passing entries are destroyed and removed from the back at eight-byte stride.
4. Admission is 1 only when the remaining range is empty; otherwise it is 0.

This is LIFO removal within one dependency vector. The receiver pointer queue
is FIFO and has four-byte elements. `0x008A1370` tests dependency-range
emptiness. Removal helper `0x0077A210` destroys an eight-byte range through
`0x00CC9330`, whose body is `RET`. None of these operations invokes the
requested Lua callback.

## Native helper entry points

| Helper VA | Byte-backed observation |
| --- | --- |
| `0x00CC70B0` | A separate predicate used by typed actor conversion; its broader classification is not established here. |
| `0x00CC7180` | Navigates through LuaControl `+0x1C8` via `0x00CD80E0` to circular-list membership `0x00D132B0`, comparing the u32 key at node `+8`. |
| `0x00CC7190` | Navigates through LuaControl `+0x1C4` via `0x00CD80F0` to `0x00D035D0`, which compares the first returned metadata byte with `byte[0x0130D426]=0x0F`. |
| `0x00CC72A0` | Resolves a Lua registry entry through `0x00CD7A30` and reads entry byte `+0x7D`. This is not an actor-owned byte or callback dispatch. |
| `0x00CC78C0` | Calls `0x00CDDE20(key,0)` and returns `*entry` when nonnull. |
| `0x00CC7A50` | Resolves a u32 binding key through the LuaControl wrapper and its partitioned lookup. Reached resolver branches reject sentinel `0xE0000000`. |

The `+0x1C4` metadata lookup and `+0x1C8` membership list are distinct.
Their observations do not establish a universal actor-spawn state machine.
`0x00CDDE20` resolves class registration/name metadata and can prepare an
initialization request through `0x00CF1060 -> 0x00CCF9B0`; it is not an
insertion into this receiver's pointer queue. Its retained RVA gap
`[0x008DE182,0x008DE185)` remains unfilled, so the enclosing size is not
continuous decoded coverage or a complete initialization/lifecycle proof.
The metadata classifier's byte `0x0F`, wire END `0x0F`, and resolver sentinel
`0xE0000000` belong to separate domains.

## Submission, retry and execution

`0x007859B0` checks the receiver pointer queue count. If empty, it runs the
dependency collector and may attempt common dispatcher `0x00794250`
immediately. The dispatcher calls slot 2 at `0x00794265`; admission byte 1
allows slot 3 at `0x00794279`. Other admission leaves the record for queueing.
The image byte cells determining these outcomes and their later mutable state
are part of the conditional scheduler contract in the native producer page.

A nonempty pointer queue skips collection and the immediate attempt. The
heap clone preserves the current strings, keys, parameter bytes and dependency
vector, including an initially empty dependency vector on this skipped path.
The traced deferred consumer does not compensate with another collection
step or preparation-flag test. That path does not prove the parameter
references were dependency-checked; typed conversion later has its own gates.

Deferred consumer `0x005836D0` reads the front pointer and calls the same
dispatcher at `0x00583770`. Result 1 retains the front receiver for retry;
result 2 has a separate reinsertion branch. Other results destroy and remove
the receiver. The ordinary dispatcher does not test slot 3's return value.
Queue removal therefore does not prove that a Lua callable ran or completed.

Slot 3 resolves the stored trigger to native object `T`, then passes the
stored function name, event name, owner key, event type and parameter span to
`0x006E1140`. That thunk selects manager `[T+0xF8]` and reaches `0x00896F70`.
Normal execution requires that trigger resolution be usable; the slot-3 body
itself has no null test before the thunk dereferences `T`.
The [typed frame and call consumers](../script/delegate-command-producer.md#function-binding-value-and-supplied-frame)
then determine whether local invocation occurs. Dependency readiness alone
proves neither invocation nor an unconditional silent no-op for a missing actor.

## Cross-references

- `docs/event/kick-order-event-receiver.md` - the companion KickEvent
  receiver decomp; `+0x5c` actor flag finding is shared.

## Trade routing through generic RunEventFunction

A read-only Ghidra decompilation of the retail 1.23b `ffxivgame.exe`
(version `2012.09.19.0001`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`) traced
s2c opcode `0x0130` through `FUN_004D8860` and `FUN_00575040` to
`LuaActorImpl` vtable slot 57, `FUN_0076C220` (RVA `0x0036c220`, VA
`0x0076c220`). The read-only `DumpVAs.java` pass requested VAs `0x004D8860`,
`0x00575040`, `0x0076C220`, `0x0089F430`, `0x0089EB20`, `0x0089E8E0`,
`0x0070A010`, `0x00898480`, `0x00897310`, `0x006EE680`, `0x0075E3A0`,
`0x00776760`, `0x004D6D30`, and `0x004E0240` from the Ghidra 12.1
auto-analysis program. The dispatcher passes the application payload at packet
`+0x10`, so the decoder's field offsets are relative to that payload:

| Offset | Size | Field |
|---|---:|---|
| `0x00` | `0x04` | Trigger actor id (`u32`) |
| `0x04` | `0x04` | Owner actor id (`u32`) |
| `0x08` | `0x01` | Event type (`u8`) |
| `0x09` | `0x20` | Event name (`char[32]`) |
| `0x29` | `0x20` | Function name (`char[32]`) |
| `0x49` | `0x40` | Lua parameter input (`bytes[64]`) |

The function-name field selects the Lua method and the parameter input carries
its positional arguments. Trade therefore specializes this generic decoder at
the Lua boundary rather than through a native trade packet layout. This is the
client-side message shape, not an observation of trade values or server
sequencing.

`FUN_0076C220` contains no native trade-token or notice-code switch. Trade
function and argument selection remain data-driven at the generic Lua
boundary. This is a bounded observation of the traced decoder and route, not
an absolute negative about indirect or otherwise untraced binary-wide
producers.
