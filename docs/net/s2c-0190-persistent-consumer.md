# s2c 0x0190 persistent consumer

The retail client persists each `0x0190` record as an
`Application::Lua::Script::Client::Command::Item::ServerOrderUpdateWorkCommand`.
That native RTTI supports an item-command interpretation. It does not establish
the imported modifier noun, an equipment change, or a concrete batch type.

## Owner and lifetime

The central dispatcher `0x004dc690` supplies
`RaptureElementContainer+0x510` to the `0x018F`, `0x0190`, and `0x0191`
wrappers. Each wrapper loads the same manager pointer from route state +0x24,
which is container +0x534. `0x00CC9320` only constructs the scoped two-dword
key holder; it is not a lock, and `0x00CC9330` is an empty scope exit.

The route-state constructor `0x00577FD0` clears the pointer. Initializer
`0x0057A3C0` allocates the manager through `0x0076B8F0`, replaces any prior
manager, and invokes `0x00769320` before freeing the old allocation. The
manager owns two ordered maps at +0x4 and +0x10. `0x0076B950` finds or creates
the persistent per-key state in the +0x10 map. The manager therefore survives
individual packets and is destroyed or replaced with the route state, not by
`0x0191`.

`0x018F` reaches `0x0076BE30` and enqueues the start-side command for the
scoped key. `0x0191` reaches `0x0076BF10`, enqueues work-end and update-end
commands, obtains the secondary +0x4-map state, moves the primary per-key
state into it through `0x00768B10`, and erases the primary-map entry. This is
a staged start/work/end protocol, but no native `Batch` class or literal was
found.

## Record projection and queue

`0x0076BE60` obtains the primary per-key state and allocates a 0x20-byte
record. Constructor `0x00768C40` writes application dwords +0x00 and +0x04 to
record +0x08 and +0x0C. It constructs a vector object at record +0x10,
allocates 0x40 bytes, and copies exactly sixteen dwords from application
+0x08 through +0x47 into the buffer reached through record +0x14.

The [immutable application-size contract](https://github.com/XIVLegacy/xivl-client-structs/blob/25c9d48d776135eeca8f32314fa90fb9faf9fca4/manifests/unmapped_payload_decoding.json)
records a 0x68-byte application payload. The writer consumes 0x48 bytes and
does not retain the packet pointer, so application +0x48 through +0x67 is an
unread 0x20-byte tail on this route. A 48-byte tail results from treating the
larger body framing as the application payload; the native writer does not
support that interpretation.

`0x00766920` moves the record into the command constructed by `0x0089B8D0`
and enqueues it in the persistent per-key state. The command's tracked RTTI
vtable at RVA 0xC57238 resolves execution slot 9 to `0x0089B9E0`.

## Static reader census and runtime boundary

`0x0089B9E0` is the only static value-bearing command reader found. It obtains
the record, passes record +0x08 as a two-dword lookup key to `0x00765CF0`, and
passes the vector object at record +0x10 onward. The lookup selects an entry
and calls its virtual method at offset +0x2C. The exact call is
`0x00765D49: CALL EDX`, with the 16-dword vector argument on the stack.

No statically resolved instruction after construction loads either header
value or any copied vector value. The lookup and record destructors read map
or vector metadata only. The selected receiver class and final mutation are
therefore lost behind runtime dispatch. This is the durable static boundary,
not evidence that the fields are inert.

For runtime closure, break at 0x00765D49 and record EDX as the target, ECX as
the selected receiver, and the pushed vector address. Watch record +0x08,
record +0x0C, and the 0x40-byte allocation reached through record +0x14. A
break at `0x0089B9E0` preserves the owning command and record pointer before
the lookup.

## Semantic verdict

The `Item::ServerOrderUpdateWorkCommand` class is a direct native edge to the
item command subsystem. No direct edge reaches the retail equipment-change
path, and the cluster does not call the known inventory-record modifier
deserializer. The code supplies no native `modifier`, `MassSetItemModifier`,
or concrete `batch` noun. Imported packet labels remain candidates only.

The focused machine contract is
[`config/s2c_0190_persistent_consumer.json`](../../config/s2c_0190_persistent_consumer.json).
It is checked by `tools/verify_s2c_0190_persistent_consumer.py` and the
repository validation. Public evidence names only retail addresses and committed
RTTI or vtable catalogs.
