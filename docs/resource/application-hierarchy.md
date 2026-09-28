# Application scheduling observations

This page records bounded RTTI, vtable, and function-body observations for the
application and network update paths. The type census does not establish
object ownership, instance counts, socket ownership, or connection topology.

## Application and network type census

The local RTTI catalog contains the following relevant class identities. The
slot counts are the cataloged extents at each vtable RVA; they do not assign
behavior to unreviewed slots.

| RTTI class | Vtable RVA | Slot count |
|---|---:|---:|
| `Main` | `0xb54a24` | 1 |
| `Application::Rapture` | `0xb8cc1c` | 4 |
| `Application::Rapture` | `0xb8cc30` | 2 |
| `Application::Main::MainModule` | `0xb9142c` | 1 |
| `Application::Network::NetworkModule` | `0xb91b5c` | 1 |
| `Component::Network::IpcChannel::ChannelManagerBase` | `0xd29b5c` | 1 |
| `Application::Network::LobbyClient::RaptureChannelManager` | `0xd2869c` | 25 |
| `Application::Network::ZoneClient::RaptureChannelManager` | `0xd29094` | 25 |
| `Application::Network::ChatClient::RaptureChannelManager` | `0xd295a4` | 26 |
| `Sqex::Socket::RUDP2::RUDPImpl` | `0xd13378` | 14 |

Source: Ghidra 12.1 RTTI observations in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json). The three
`RaptureChannelManager` identities form a type census only. They do not prove
three live streams, one instance per stream, or a relationship to
`RUDP2::RUDPImpl`.

The vtable-slot catalog maps `Application::Rapture` vtable RVA `0xb8cc1c`
slot 1 to function RVA `0x000b3c50` (VA `0x004b3c50`). Its body observation
records a direct call to VA `0x004e30a0` during the reviewed update. Source:
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl) and
observation `NetworkModule_topLevelTick` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
The evidence does not establish a complete top-level lifecycle hierarchy.

## Six-state network-client switch

At VA `0x004e30a0`, the body switches on the dword at object offset `0x250`
with literal cases `0` through `5`.

| Case | Direct body observation |
|---:|---|
| `0` | Can request `0x4a8` bytes and store the result at `+0x240`. |
| `1` | Calls `0x004e2d00`; byte `+0x24c` can gate a write of state `3`. |
| `2`, `3` | Write state `4`. |
| `4` | Calls `0x004e20a0` and can write state `5`. |
| `5` | Can clear pointers at `+0x240` and `+0x300`, then calls `0x004e20a0`. |

The same body references a pointer at `+0x234`. The local observation assigns
no semantic name, class identity, or vtable slot to that field. Source:
observation `NetworkClientModule_tick` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

## Packet-loop and tree observations

At VA `0x004e20a0`, the reviewed body repeatedly obtains inbound packets,
invokes the dispatcher at `0x004dc690`, and calls `0x004e5ff0`. This statement
is bounded to the reviewed body and does not establish channel or transport
ownership. Source: observation `ZoneClient_mainLoopTick` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

At VA `0x004e5ff0`, the u32 at input offset `0x1c` is compared with literal
`0x1c11`. Values below it call the tree lookup-or-insert helper at
`0x004e5ca0`; values at or above it invoke virtual slot 0 on the input object
with argument `1`. The helper at `0x004e5ca0` compares the supplied u32 with
node field `+0x0c`, returns an equal node, or calls `0x004e4dd0` to insert a
new keyed node. The observations assign neither a packet-sequence meaning to
the key nor a channel identity to the owning object. Source: observations
`ZoneClient_packetDispatch_treeOrDestroy_threshold_0x1c11` and
`ZoneClient_treeLookupOrInsert_u32Key` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

## Ordered application scheduling calls

At VA `0x004da680`, a nonzero byte at object offset `0x504` returns early. If
`+0x4a8` is zero while `+0x17444` and `+0x174dc` are nonzero, the body writes
one to `+0x4a8`. Under a separate `+0x54b` and helper-result gate, it walks a
list rooted at `+0x17808`, invokes virtual offset `0x18` on non-null entries,
and scans the dword range bounded by `+0x17828` and `+0x1782c`. A comparison
derived from `0x00443e40` controls the direct call to `0x00578970`. Source:
observation `Function_004da680_gateListsAndCall00578970` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The complete body at `0x00578970` has four direct setup or guard calls before
the contiguous sequence below and one direct cleanup call after it. The
sequence contains 13 fixed direct targets followed by one virtual call through
the object pointer at `+0x34`, vtable offset `8` (slot 2). This is exhaustive
for top-level calls in the reviewed body. The runtime target of the virtual
call, and all direct, indirect, or virtual calls made inside the listed
callees, are outside the bound. Source: observation
`Function_00578970_orderedCallSequence` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

| Order | Function | Direct body observation |
|---:|---|---|
| 1 | `FUN_00766f00` | Processes two lists; one path is gated by `+0x16c == 10`, and completed entries can set resolved-object byte `+0x5c` and be erased. |
| 2 | `FUN_0076f6f0` | Walks a list, branches on entry-object bytes `+0x140` and `+0x141`, invokes virtual callbacks, and erases completed entries. |
| 3 | `FUN_007700b0` | Walks a list, passes literal `0xde` to `0x0076fb10`, forwards successful results, releases owned objects, and erases entries. |
| 4 | `FUN_0076a9c0` | Walks four data-category ranges, including the literal names `command`, `achievement`, and `hamletDefScore`. |
| 5 | `FUN_006cdf20` | Branches on fields `+0x0c`, `+0x20`, and nested `+0x38`, then conditionally calls `0x006c5f40`, `0x006cdd20`, or `0x006c2200`. |
| 6 | `FUN_00583440` | Processes a ring-backed pointer queue and stops after literal `0x20` completed entries; deferred entries do not increment that counter. |
| 7 | `FUN_005836d0` | Performs the parallel ring-backed queue path with a distinct handler and the same literal `0x20` completed-entry limit. |
| 8 | `FUN_007694d0` | Walks a list, resolves entries, dynamically casts from `LuaControl` to `CharaBase`, checks resolved-object byte `+0x5c`, and conditionally erases entries. |
| 9 | `FUN_00770c00` | Removes at most one ring entry after a helper succeeds, selects a handler from object byte `+0x114`, releases the object, and frees the entry. |
| 10 | `FUN_0076dab0` | Calls `0x0075cea0` and `0x0076a490` with the same argument. |
| 11 | `FUN_00765340` | Walks a list and, when two helper checks succeed, invokes virtual slot 0 on the referenced object and erases the entry. |
| 12 | `FUN_0075d120` | Reads binding ids `0xc0000024`, `0x7a121`, `0x7a122`, and `0x7a123`; changed values dispatch `(((value_7a121 & 3) << 5) | (value_7a123 & 0x1f)) * 2 | 1`, while a cleared `0x7a122` path can dispatch zero. |
| 13 | `FUN_00764fd0` | Walks flagged pairs, resolves each object, waits on byte `+0x5c`, dynamically casts a root binding from `LuaControl` to `DesktopWidget`, and removes completed pairs. |
| dynamic | virtual slot 2 | Invoked through the object pointer at `+0x34`; the target and role are not established. |

Sources: observations `OrderedCall_01_state0x16c_listSweep` through
`OrderedCall_13_desktopWidgetCastListSweep` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

## Cross-references

- `docs/net/packet-dispatch-router.md` - analysis of `FUN_004e20a0` and
  `FUN_004e5ff0`.
- `docs/net/actorimpl-receiver-dispatch.md` - LuaActorImpl receiver-slot
  analysis.
- `docs/net/network-dispatch-paths.md` - analysis of the callback path through
  `FUN_00dae520` and `FUN_004e20a0`.
- `docs/net/wire-protocol.md` - bounded socket, packet-buffer, and routing
  observations.
- `docs/resource/dynamic-cast-hierarchies.md` - RTTI walk and
  class-hierarchy evidence.
