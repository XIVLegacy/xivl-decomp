# SyncWriter property handlers

This page describes the client-side `SyncWriter` registration, property-apply,
and callback paths retained in the FFXIV 1.23b client. These paths explain how
an s2c `0x0137` property record reaches typed client storage. They do not
establish an outbound packet builder, server mutation policy, or packet
framing.

## Registered property apply

`SyncMemoryReceiver` routes the `0x0137` application payload through
`FUN_00775A30` to `FUN_00775180`. For each property record, the parser consumes
a value-width byte, a little-endian 32-bit property hash, and the declared raw
value bytes. It also recognizes target-marker entries. For a property record,
it looks up the hash in the context property map at `+0x0C`, loads the handler
pointer from map-node `+0x10`, and calls handler vtable slot 1 at `0x00775652`.

The common scalar slot-1 thunk `FUN_00D30C70` increments the 16-bit counter at
writer `+0x0C` and tail-jumps through typed slot 6. For example,
`FUN_00D2F9B0` stores a four-byte integer value at writer `+0x10`. This is a
client apply path. The capture grammar preserves raw value bytes and does not
assign a universal signedness or byte-order interpretation to them.

`ActorWorkSync` owns the hash-to-handler registry. Its population route reaches
`FUN_00CFD610`, which installs the selected writer's callback fields:

```text
writer +0x04 = SyncContainer +0x2C secondary callback interface
writer +0x08 = registration-supplied callback context
```

When the field uses a shared-work wrapper, that wrapper's slot 1 forwards to
the inner concrete writer's slot 1. The writer class is selected by the
registered Information subtype; it is not a fixed actor-structure offset.

## Common scalar layout

The retained Boolean, Integer8, Integer16, Integer24, Integer32, and Float
writers support this common prefix:

```text
+0x00  concrete writer vtable
+0x04  callback handler object
+0x08  callback context
+0x0C  16-bit counter
+0x0E  mode byte; exact semantics unresolved
+0x10  typed storage begins
```

Boolean uses bytes at `+0x10` and `+0x11`. Other concrete writers have
type-specific storage beyond the common prefix. String, actor, array, and
shared-work writers require their own layouts and must not be inferred from
the scalar prefix.

## Concrete scalar vtable

The retained concrete scalar writers use an eight-slot vtable. Related writer
interfaces and adapters can have different slot counts.

| Slot | Retained behavior |
|---:|---|
| 0 | Destructor |
| 1 | Increment `+0x0C`, then tail-call typed slot 6 |
| 2 | Shared no-op `FUN_00A72A20` |
| 3 | Common scalar body `FUN_00D30C80` returns whether `+0x0C` is nonzero; other writer families vary |
| 4 | If `+0x0C` is nonzero, call typed slot 7 and decrement the counter |
| 5 | Shared return stub `FUN_006CE2E0`; semantic name unresolved |
| 6 | Typed property apply |
| 7 | Typed callback dispatch |

Slot 4 does not copy or commit a second value in its retained body. The
retained body tests the counter before slot-7 dispatch and decrements it after
that dispatch. Its higher-level scheduling and ownership policy remain
unresolved.

## Boolean callback path

The Boolean slot-7 callback body is `FUN_00D2F8C0`, at file/RVA `0x92F8C0`.
It loads the callback handler from writer `+0x04`, the callback context from
writer `+0x08`, and the bytes at `+0x10` and `+0x11`. It calls handler vtable
slot 2.

The registration path makes that slot concrete:

```text
FUN_00D2F8C0
  -> writer +0x04
  -> SyncContainer +0x2C secondary interface, vtable slot 2
  -> FUN_00CFD5B0
  -> access object at SyncContainer +0x08, vtable slot 5
```

`FUN_00CFD5B0` forwards the callback context and both Boolean bytes together
with an opaque argument from `SyncContainer+0x10`. This establishes the exact
slot-2 target requested from the Boolean serializer. It does not establish
that either byte is an old/new pair or that the callback constructs a packet.

## Other typed callbacks

The Integer8, Integer16, Integer32, Float, String, Actor, and Array slot-7
bodies do not share the Boolean pair shape. Their retained bodies use handler
slot 1 with one value or a range. The SyncContainer secondary slot-1 target is
`FUN_00CFD580`, which forwards to the access object at `SyncContainer+0x08`,
vtable slot 3.

String, actor, array, and endian-adjusting wrapper framing remains unresolved.
The class names and shared bodies alone do not prove ownership, byte swapping,
or an on-wire representation.

## Shared sync staging

`FUN_00CFE2B0` initially writes the static abstract `AccessInterface` object at
`0x0130D414` to `SyncContainer+0x08`. The later configuration path is:

```text
FUN_00CCB920
  -> SyncContainer vtable slot 12
  -> FUN_00CFD1E0(sync selector)
  -> allocate SharedSyncAccess
  -> store it at SyncContainer+0x08
```

`FUN_00CCB920` configures three operator containers. Its third dispatch uses
the sync selector address `0x0130D423`. `FUN_00CFD1E0` recognizes the save and
temporary selectors in its first two branches; its remaining branch installs
the `SharedSyncAccess` vtable at `0x0110EDC0`. The same function stores the new
object at receiver `+0x08`. This parameter-based store explains why the older
literal-reference search for `0x0130D414` did not find the replacement.

The two callback slots now resolve as follows:

| Access slot | Target | Retained behavior |
|---:|---|---|
| 3 | `FUN_00CFE960` | Reads the current byte range, compares it with the supplied range when required by the access mode, and sends a changed range through access slot 13. |
| 5 | `FUN_00CFE1A0` | Reads one byte, updates the selected bit, and sends the changed byte through access slot 13. |

For `SharedSyncAccess`, slots 11 through 13 are adapters over the retained
`SharedWorkInterface` pointer at access object `+0x08`:

```text
SharedSyncAccess slot 11 -> FUN_00CFCC70 -> SharedWorkInterface slot 21
SharedSyncAccess slot 12 -> FUN_00CFCC90 -> SharedWorkInterface slot 24
SharedSyncAccess slot 13 -> FUN_00CFCCC0 -> SharedWorkInterface slot 27
```

The retained concrete `Application::Lua::Script::Client::Group::SharedWork`
supplies compatible slot targets at `FUN_006C2E20`, `FUN_006C9A30`, and
`FUN_006C9BB0`. Its slot 27 resolves the sync member and byte offset, validates
the destination extent, and copies the changed bytes to
`[member_record+0x24] + resolved_offset` with `memcpy` at `0x006C9C77`. This is
the first proven buffer write in that concrete implementation.

The backing pointer can be followed farther through the construction path:

```text
registration wrapper incoming ECX
  -> FUN_00CD9360 argument 5
  -> FUN_00CEAF60 argument 2
  -> FUN_00CE64A0 argument 2
  -> FUN_00CCB920 argument 1
  -> FUN_00CFD1E0 argument 2
  -> SharedSyncAccess+0x08
```

`FUN_00CEAF60` is the only direct caller of `FUN_00CE64A0`, and
`FUN_00CD9360` is its only direct caller. The seven retained registration
wrappers at `FUN_00CC80A0`, `FUN_00CC8140`, `FUN_00CC81E0`, `FUN_00CC8280`,
`FUN_00CC8340`, `FUN_00CC83C0`, and `FUN_00CC8440` all forward their original
incoming `ECX` as argument 5. The static object-flow boundary is therefore the
receiver supplied to those wrappers.

The targeted RTTI hierarchy identifies Group `SharedWork` as a concrete
`SharedWorkInterface` implementation whose constructor is `FUN_006CB4C0`.
`FUN_006CBC90` allocates that object and passes it into `FUN_006C8CF0`, but no
static edge connects that handoff to any registration-wrapper receiver. The
compatible hierarchy narrows the candidate; it does not prove the dynamic type
of the forwarded pointer.

Within the inspected recorded-reference and field-reference results, no dirty
marker or outbound consumer follows the slot-27 copy. Recorded references to
slots 24 and 27 are their vtable entries, while another observed consumer only
frees and clears the member vectors during teardown. No opcode, message size,
actor id, packet framing, or network-buffer write is established by this chain.
In particular, the local SharedWork buffer copy must not be treated as proof of
an outbound `0x0137` packet builder.

The two literal `PUSH 0x137` sites at `0x00476A26` and `0x0047A591` belong to
diagnostic calls and are not packet-construction evidence.

## Evidence

The client-structure citations below are pinned at commit
`a52da3a3daec72431224fa7ce321aa9ee27b2c3b`.

- `xivl-client-structs:manifests/property_stream_hash_catalog.json#applyStorageBoundary`
- `xivl-client-structs:manifests/cast_chant_presentation.json#activeCastGauge.wireCarrier`
- `config/ffxivgame.rtti.json`
- `config/ffxivgame.vtable_slots.jsonl`
