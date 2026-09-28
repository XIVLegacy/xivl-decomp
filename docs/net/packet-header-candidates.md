# Network consumer vtables and packet flow

This page records bounded vtable, dispatch, queue, and producer observations
from the retail 1.23b `ffxivgame.exe`. The function-body observations came from
existing Ghidra decompilation records; the class and slot identities come from
the tracked RTTI and vtable catalogs. These observations do not identify the
C++ type of the dword at `packet+0x8`, the owner or library type of the u32-keyed
tree reached by `FUN_004e5ca0`, or a multiple-inheritance relationship.

## Dispatch and queue observations

At VA `0x004e20a0`, the default dispatch path loads the dword at packet offset
`0x8` into `ECX`, passes the packet pointer on the stack, and calls VA
`0x004e5ff0`. The same body obtains an inbound packet through VA `0x00dae520`.
The reviewed body at VA `0x00dae520` calls VA `0x00db1960`; that callee copies
the dword at queue-node offset `0x8` to the output packet pointer's offset
`0x8`.

The tracked evidence for VA `0x004e20a0` and VA `0x004e5ff0` is in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
[`packet-dispatch-router.md`](packet-dispatch-router.md) records the
`packet+0x8` load and call to VA `0x004e5ff0`, while
[`network-dispatch-paths.md`](network-dispatch-paths.md) records the
VA `0x00dae520` to VA `0x00db1960` dequeue path.

## Zone manager vtables

The RTTI rows in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) identify both
four-slot vtables. The slot rows in
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl)
record the target RVAs and function names. Stored target VAs below use the
retail image base `0x00400000`.

### ConnectionManagerTmpl

The vtable at VA `0x01129754` (RVA `0xd29754`) belongs to
`Component::Network::IpcChannel::ConnectionManagerTmpl<Application::Network::ZoneProtoChannel::ZoneProtoUp,Application::Network::ZoneProtoChannel::ZoneProtoDown>`.

| Slot | Stored target VA | Function RVA | Catalog name |
|---:|---:|---:|---|
| 0 | `0x00db83f0` | `0x9b83f0` | `FUN_00db83f0` |
| 1 | `0x009d364d` | `0x5d364d` | `__purecall` |
| 2 | `0x00776340` | `0x376340` | `FUN_00776340` |
| 3 | `0x00776340` | `0x376340` | `FUN_00776340` |

The cataloged `__purecall` target at slot 1 establishes that this
`ConnectionManagerTmpl` vtable contains a pure virtual slot and the class is
abstract.

### ServiceConsumerConnectionManager

The vtable at VA `0x01129768` (RVA `0xd29768`) belongs to
`Application::Network::ZoneProtoChannel::ServiceConsumerConnectionManager`.

| Slot | Stored target VA | Function RVA | Catalog name |
|---:|---:|---:|---|
| 0 | `0x00db8410` | `0x9b8410` | `FUN_00db8410` |
| 1 | `0x00db7e50` | `0x9b7e50` | `FUN_00db7e50` |
| 2 | `0x00776340` | `0x376340` | `FUN_00776340` |
| 3 | `0x00db7150` | `0x9b7150` | `FUN_00db7150` |

## ConsumerConnection vtables

The tracked RTTI catalog contains separate five-slot rows for the full Lobby
and Zone class names. The corresponding slot catalog supplies all five target
values for each row.

| Class | Vtable VA | Vtable RVA | Slot target VAs 0 through 4 |
|---|---:|---:|---|
| `Application::Network::LobbyProtoChannel::ServiceConsumerConnectionManager::ConsumerConnection` | `0x011276e8` | `0xd276e8` | `0x00da2100`, `0x00da1480`, `0x00da0c90`, `0x00da0c80`, `0x00da0c70` |
| `Application::Network::ZoneProtoChannel::ServiceConsumerConnectionManager::ConsumerConnection` | `0x0112973c` | `0xd2973c` | `0x00db8270`, `0x00db7d10`, `0x00db7440`, `0x00db7420`, `0x00db73c0` |

These rows establish the cataloged class identities and vtable contents. They
do not establish that the catalog enumerates every vtable in either complete
object or prove a single- or multiple-inheritance layout.

## Producer-chain observations

The existing instruction-level trace records this sequence:

1. SCCM slot 1 resolves to VA `0x00db7e50` in the tracked vtable catalog.
2. The reviewed body at VA `0x00db7e50` calls VA `0x00dafa30`.
3. VA `0x00dafa30` prepares a local output object, calls VA `0x00daf5b0`,
   and returns the local object's address through the dword at argument offset
   `0x8`.
4. Within VA `0x00daf5b0`, the instruction pair `MOV EDX,[EAX+ECX]` and
   `MOV [EDI+0x8],EDX` copies one source dword to destination offset `0x8`.
5. The caller later loads the dword at packet offset `0x8` into `ECX` and calls
   VA `0x004e5ff0`.

This trace establishes a copy and call-dataflow path for the dword at offset
`0x8`. It does not establish that the value is a C++ subobject pointer or assign
it an `IpcChannel` class identity.

## Related bounded records

- [`channel-dispatch-tree.md`](channel-dispatch-tree.md) records only the
  u32-keyed tree control flow at VA `0x004e5ca0` and the bounded RTTI census.
- [`packet-dispatch-router.md`](packet-dispatch-router.md) records the
  VA `0x004e20a0` -> VA `0x004e5ff0` dispatch path.
- [`network-dispatch-paths.md`](network-dispatch-paths.md) records the
  VA `0x00dae520` dequeue wrapper and its call to VA `0x00db1960`.
