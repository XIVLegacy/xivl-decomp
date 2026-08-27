# s2c 0x018D client consumer

The retail `0x018D` receive route synchronously reaches a named UI class after
projecting its fixed 0x298-byte application buffer into a larger client-owned
record store. `FUN_0055CF70` finishes the projection, obtains the registered
`MapScreenControl`, and calls `0x00671400`. That method reads the projected
records and creates or updates the `MapMarkerParty` presentation rows in
`group_marker_data`. This is the first concrete outward operation found.

The opcode remains neutral `_0x018D`. Its exact wire layout is owned by the
[immutable xivl-opcodes contract](https://github.com/XIVLegacy/xivl-opcodes/blob/67b709d5ffd90b8dc10a699e608fa1216e40660d/data/s2c_018d_wire_layout.json).
The sixteen physical wire rows and sixteen constructed storage records are
capacities, not a safe runtime count: the retail apply loop sign-extends the
count byte and performs no clamp.

## Owner and lifetime

`Application::Main::RaptureElementContainer+0x4d8` holds a nullable pointer to
a 0x838-byte `ClientWorkElement`. Its constructor `0x0055f8b0` installs the
class RTTI vtables and constructs a 0x7a0-byte `ClientWorkStorage` member at
+0x98. `0x0055f830` initializes the storage header, constructs sixteen
0x78-byte records beginning at storage +0x18, and clears storage +0x798.

The `ClientWorkElement` deleting path reaches `0x0055d100`; it invokes storage
destruction at `0x0055cf20`, which vector-destructs the sixteen records, then
continues through the base destructor. The selector factory at `0x005334c0`
is the only recorded factory-table reference. Construction, registered
container ownership, record lifetime, and teardown are therefore closed for
the direct topology.

The dispatcher alias is explicit in the generated retail instructions:
`0x004dd167` loads the +0x4d8 pointer, `0x004dd18f` adds +0x98 to that pointee,
`0x004dd1a6` pushes the separate wire application address, and `0x004dd1a7`
places the adjusted storage owner in `ECX` before the call at `0x004dd1a9`.
This register flow is authoritative where the untyped decompiler call syntax
does not display the hidden `this` argument.

`MapScreenControl` separately registers itself at Main +0x17858 through
`0x00672690` and clears that registration in `0x00678640`. The apply routine
uses `0x004d7620` to retrieve that exact registered instance before calling
the UI consumer.

## Wire-to-storage projection

The application buffer and client storage are distinct layouts. The fixed
wire application is 0x298 bytes with 0x28-byte records at +0x10. The client
storage records have stride 0x78 and begin at storage +0x18.

| Wire application | ClientWorkStorage | Operation |
|---:|---:|---|
| +0x00 | +0x08 | copy dword |
| +0x04 | +0x0c | copy dword |
| +0x08 | +0x10 | copy dword |
| +0x290 | +0x14 | sign-extend count byte |

For each iterated row:

| Wire record | Storage record | Operation |
|---:|---:|---|
| +0x00 | +0x00 | copy dword |
| +0x08 | +0x08 | copy dword |
| +0x0c | +0x0c | copy dword |
| +0x14 | +0x10 | copy dword |
| +0x18 | +0x14 | copy dword |
| +0x1c | +0x18 | copy dword |

`0x00575550` prepares a temporary lookup context. `0x00573fc0` uses it with
record +0x00 and +0x08 inputs and fills helper-owned string-like state at
record +0x20 plus a scalar at +0x74. `0x00573f70` destroys the temporary after
apply. No RTTI class name is proven for that temporary context.

## Complete direct reader census

| Storage location | Reader | First direct consumer |
|---:|---:|---:|
| header +0x08, +0x0c, +0x10 | none found | none found |
| count +0x14 | 0055d020 | 00671400 |
| count +0x14 | 0055d0d0 | 00691f30 deferred gate |
| record +0x00 | 0055d090 | 00671400 |
| record +0x08, +0x0c | 0055d0b0 | 00671400 |
| record +0x10, +0x18 | 0055d050 | 00671400 |
| record +0x20 | 0055d030 | 00671400 |
| record +0x74 | 0055d070 | 00671400 |
| storage +0x798 | 0055d0d0 | 00691f30 deferred gate |

The first UI consumer does not separately load projected record +0x14. No
additional first-consumer load was found in the remaining record tail outside
the helper state at +0x20 and scalar at +0x74. Each listed record accessor has
exactly one recorded direct caller, `0x00671400`. The three header dwords have
no outward reader in the exact direct, data, field, vtable, and generated-call
census.

## First outward operation

`0x00671400` calls the count and record accessors, iterates the stored count,
and uses the exact retail strings `MapScreenControl`, `group_marker_data`,
`MapMarkerParty`, and `Update`. Its UI calls create or update presentation
rows. The apply route calls this method synchronously before returning, so it
precedes the separate state gate.

The apply routine latches +0x798 when it was clear and the new count exceeds
one. `PcSearchWidgetOperator` vtable slot 29 (`0x00691f30`) later tests that
byte together with count equal to one through `0x0055d0d0`, calls
`0x00691e80`, and clears the byte through `0x0055d0f0`. That operation receives
no storage or record pointer. It is a later refresh gate, not the first
outward consumer, and its higher-level purpose remains unresolved.

## Verdict and boundary

The direct route proves a local UI presentation update. It contains no direct
Lua/N-API consumer and no network builder or emission. Numeric record values
do not by themselves establish coordinate nouns, party policy, membership,
permissions, or server behavior. The physical capacity of sixteen records
does not make sixteen a safe count.

The negative census covers Ghidra-recorded direct and data references, resolved
field and affine aliases, vtable entries, and the generated direct-call corpus
for the traced objects and methods. Unresolved computed or dynamic indirect
readers, runtime-only consumers, numeric field meanings, the temporary
helper's class, and server behavior remain outside the result.

The focused machine contract is
[`config/s2c_018d_client_consumer.json`](../../config/s2c_018d_client_consumer.json).
It is checked by `tools/verify_s2c_018d_client_consumer.py` and the repository
gate. Public evidence names only retail addresses, committed catalog classes,
and the immutable upstream wire contract.
