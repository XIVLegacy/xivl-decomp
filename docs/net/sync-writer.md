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
`FUN_006C9BB0`. Its slot 27 resolves the sync member and byte offset, checks
the destination's starting offset, and copies the changed bytes to
`[member_record+0x24] + resolved_offset` with `memcpy` at `0x006C9C77`. This is
the first proven buffer write in that concrete implementation.

The backing pointer is the Group `SharedWork` object on the retained
construction path:

```text
FUN_006CB4C0 constructs Group SharedWork
  -> FUN_006CBC90
  -> FUN_006C8CF0 argument 3
  -> FUN_006F6D60 argument 3
  -> FUN_006E2510 argument 4
  -> FUN_00CC8440 stack argument 6
  -> FUN_00CD9360 stack argument 5
  -> FUN_00CEAF60 argument 2
  -> FUN_00CE64A0 second stack argument
  -> FUN_00CCB920 backing argument
  -> FUN_00CFD1E0 backing parameter
  -> SharedSyncAccess+0x08
```

`FUN_006CBC90` carries the new object in a one-word argument copy.
`FUN_006C8CF0` and `FUN_006F6D60` preserve that value while building their
other local arguments. `FUN_006E2510` receives it as argument 4 and places it
in the last stack argument to `FUN_00CC8440`. That wrapper copies stack
argument 6 into the slot used as `FUN_00CD9360` stack argument 5.
`FUN_00CD9360` copies the same value into `FUN_00CEAF60` argument 2, which
becomes the second stack argument to `FUN_00CE64A0`. The latter stores it at
operator object `+0x08` before `FUN_00CCB920` forwards it into
`FUN_00CFD1E0`.

This instruction-level flow binds the RTTI-backed Group `SharedWork` object to
the `SharedSyncAccess` backing field for this construction path. The slot 11
through 13 adapter calls therefore reach the Group implementation's slots 21,
24, and 27 on this path.

### Member storage and pointer consumers

The value buffer belongs to a separate storage record, called `member_record`
here. In `FUN_006C97A0`,
instructions `0x006C97A4..0x006C97AB` load the lookup receiver from
`SharedWork+0x04` and pass `&SharedWork+0x08` to `FUN_006C1510`. Its returned
entry becomes the receiver for `FUN_006C8AC0`, or for `FUN_006C8C20` when the
first lookup fails and creation is allowed.

| Lookup | Container receiver relative to returned entry | Iterator base checked | Record storage |
|---|---:|---:|---|
| `FUN_006C8AC0` | `+0xA4` | `+0xA8` | Uses the selected entry from the `+0x84/+0x88` array to obtain a key; returns or allocates the member record. |
| `FUN_006C8C20` | `+0xCC` | `+0xD0` | Looks up the supplied index; returns or allocates the member record. |

The container and iterator offsets are distinct: `0x006C8B4D` and
`0x006C8C52` form the lookup receivers, while `0x006C8B61` and `0x006C8C66`
form the iterator bases. Neither pair is relative to SharedWork itself.
Both allocation paths request `0x30` bytes and call `FUN_006C4930`, which
clears three begin/end/capacity triples at record `+0x04/+0x08/+0x0C`,
`+0x14/+0x18/+0x1C`, and `+0x24/+0x28/+0x2C`. `FUN_006C49B0` grows these
byte vectors as needed. `FUN_006C97A0` takes each requested growth count from
the first entry's `+0x0C` in SharedWork's separate 16-byte-stride tables.
Slot 27 selects a table entry by index and passes it as the receiver to
`FUN_006C1DD0` to resolve the byte offset. The table entries are distinct
from the member value bytes.

The slot-27 assembly compares the resolved starting offset against the third
vector's byte count at `0x006C9C56..0x006C9C64`, then passes the supplied
length unchanged to `memcpy` at `0x006C9C77`. This body does not establish a
check of `offset + length`. Slot 24 reads from the same third vector.

The inspected container consumers extend the ownership trace:

| Function VA | Observed consumption |
|---|---|
| `0x006CA8B0` | Transfers a stored record pointer when the selected member key changes. |
| `0x006C8E40` | Transfers stored record pointers between entry containers, clearing the source value on the inspected transfer branches. |
| `0x006CCD00` | Transfers indexed record pointers to a keyed or indexed destination, then clears the source indexed container. |
| `0x006CD160` | Tests key and record presence while preparing member notifications. |

These bodies manipulate record ownership or presence. They do not establish
serialization of the record's third byte vector. The concrete byte reader
remains slot 24; an outbound caller of that reader is not established.

### Group updater apply and notification

The separately retained s2c `0x017A` path reaches the Group updater:

```text
FUN_005763B0 -> FUN_006C9910 -> FUN_006C96B0 -> FUN_006C84A0
  -> selected handler-pointer range -> FUN_006C7020
  -> Group WorkSyncUpdater
```

The opcode-to-`FUN_005763B0` edge is recorded in
[`ffxivgame.protocol_evidence.json`](../../config/ffxivgame.protocol_evidence.json),
`observations` entry `opcode_hex=0x017a`. `FUN_006C96B0` copies the
declared input bytes into a local vector. `FUN_006C84A0` resolves a property
handler, calls its slot 1 at `0x006C88C2`, and collects that handler pointer
at `0x006C88DF`. `FUN_006C7020` supplies a selected pointer range to
`FUN_006C56D0`, which appends pointers not already present in updater
`+0x2C/+0x30`. The creation branch calls `FUN_006C5620`, which installs the
Group `WorkSyncUpdater` vtable.

| Updater operation | Retained behavior |
|---|---|
| Slot 8, `FUN_006C12C0` | Tests child vtable slot 3 while walking `+0x2C/+0x30`; the completed branch sets byte `+0xEC`. |
| Slot 12, `FUN_006C1FE0` | On its guarded path, traverses the same pointer range through `FUN_006D08A0` with callback `FUN_00B241C0` and zero receiver adjustment. |
| Callback `FUN_00B241C0` | Jumps through the child's vtable byte offset `+0x10`, which is slot 4. |
| `FUN_006C0140 -> FUN_00700CC0` | After the child traversal and further state checks, constructs `_onUpdateWork` and calls the Lua invocation helper `FUN_00CC7A90`. |

This resolves the updater's child dispatch and notification path. It does not
bind an individual child to a particular SharedSyncAccess instance. The known
scalar slot-4 callback behavior above must not be assigned to every child
without its concrete registration and vtable evidence. Neither the updater
name nor this inbound path proves a network writer.

No opcode, packet framing, or network-buffer write is established downstream
of the SharedWork slot-27 copy. In particular, the local copy must not be
treated as proof of an outbound `0x0137` packet builder.

The two literal `PUSH 0x137` sites at `0x00476A26` and `0x0047A591` belong to
diagnostic calls and are not packet-construction evidence.

## Evidence

The member-storage and Group-updater observations use the retained retail
1.23b `ffxivgame.exe`, image base `0x00400000`, analyzed read-only with Ghidra
12.1.3. Function VAs and instruction locators are given above. Producing tools:
`xivl-client-structs:ghidra/DumpVAs.java` and
`xivl-client-structs:ghidra/DumpFunctionListing.java`, invoked through
`xivl-client-structs:tools/ghidra/run-headless.ps1` with `-ReadOnly`,
`-ScriptPath @('ghidra')`, and the relevant VAs in `XIVL_TARGET_VAS`.

The client-structure citations below are pinned at commit
`a52da3a3daec72431224fa7ce321aa9ee27b2c3b`.

- `xivl-client-structs:manifests/property_stream_hash_catalog.json#applyStorageBoundary`
- `xivl-client-structs:manifests/cast_chant_presentation.json#activeCastGauge.wireCarrier`
- `config/ffxivgame.rtti.json`
- `config/ffxivgame.vtable_slots.jsonl`
