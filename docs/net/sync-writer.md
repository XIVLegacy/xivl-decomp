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

## Staging boundary

`FUN_00CFE2B0` initializes `SyncContainer+0x08` to the static
`detail::AccessInterface` object at `0x0130D414`. That interface's slots 1
through 10 are pure virtual in the retained vtable. A complete reference
export for `0x0130D414` found only its static vtable initializer and reads from
container construction/destruction; it found no concrete access-object
assignment.

No opcode, message size, actor id, or packet-buffer write is proven along this
callback path before the access-object dispatch. The first unresolved staging
edge is therefore:

- access object at `SyncContainer+0x08`, vtable slot 5 for the Boolean callback;
- access object at `SyncContainer+0x08`, vtable slot 3 for the other retained
  typed callbacks.

The two literal `PUSH 0x137` sites at `0x00476A26` and `0x0047A591` belong to
diagnostic calls and are not packet-construction evidence.

## Evidence

The client-structure citations below are pinned at commit
`a52da3a3daec72431224fa7ce321aa9ee27b2c3b`.

- `xivl-client-structs:manifests/property_stream_hash_catalog.json#applyStorageBoundary`
- `xivl-client-structs:manifests/cast_chant_presentation.json#activeCastGauge.wireCarrier`
- `config/ffxivgame.rtti.json`
- `config/ffxivgame.vtable_slots.jsonl`
