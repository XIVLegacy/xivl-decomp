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

Before projection, `0x00575550` obtains the pointer stored at
`RaptureElementContainer+0x18` and `0x006c1570` builds a 0x18-byte value
context. The original application +0x00/+0x04 pair selects the source record:
a nonzero pair is looked up in the source owner's +0x14 map, while the zero
pair takes a default path that tests a nested +0x04 value against `0x2711`.
A pointer to original application +0x08 is forwarded into the builder but is
not consumed there. `0x00573970` stores the source owner at context +0x00 and
copies the 0x10-byte selector state to context +0x08. No RTTI class is proven
for the context or the container +0x18 pointee. `0x00573f70` destroys the
context immediately after apply.

`0x00573fc0` resets the in-place `Sqex::Misc::Utf8String` at record +0x20 to
empty, clears record +0x74 to zero, validates the context, looks up record
+0x00, and retries with record +0x08 only when the first lookup returns signed
-1. This -1 is an index miss sentinel, not a wire-value sentinel. Record +0x0c
is not a lookup key. Failure leaves the outputs empty and zero.
Only records iterated by the current projection are reset and resolved;
uniterated storage slots are not touched and are outside the consumer count.

`0x006c3320` scans 0x10-byte tagged entries between owner +0x84 and +0x88.
`0x006d3fe0` follows tag-specific indirection and extracts the candidate key
from the referent +0x04. After a match, `0x006c09e0` supplies the string and
`0x006c08a0` supplies the scalar. The tag-0 string path dynamically casts
`Component::Lua::GameEngine::LuaControl` to
`Application::Lua::Script::Client::Control::CharaBase` and copies its +0x60
`Utf8String`; the tag-3 path copies an object +0x08 `Utf8String`. The scalar
resolver copies referent +0x00 to record +0x74. The semantic field names and
remaining tagged-owner types are not proven.

## Key domains and selected object

The three wire dwords remain neutral selectors, with different native uses:

| Storage record | Native rule | Stability boundary |
|---:|---|---|
| +0x00 | primary tagged-referent +0x04 lookup key; zero is still looked up, while nonzero controls eligibility; compared with selected object +0x88 | no wire-value miss sentinel; overwritten on every wire update; compatibility with the native registry key does not prove a stable wire identity |
| +0x08 | fallback lookup key only after a signed -1 index miss; also participates in the +0x08 OR +0x0c eligibility test | zero is allowed and is not the miss sentinel; no stable identity is proven |
| +0x0c | participates only in the +0x08 OR +0x0c eligibility test | never a lookup key; no stable identity is proven |

`0x004d86b0` selects `RaptureElementContainer+0x17838`, falling back to
+0x17834 when null. These slots reference objects in the container's ordered
`Application::Main::RaptureElement` registry. `0x004d9910` resolves a caller
key to a registered object, and the removal path clears selection slots that
still reference the removed element. `0x004dab50` stores a constructor
argument at base `RaptureElement+0x88`; that value is therefore stable for the
native object's registered lifetime. The insertion path at `0x004da9a0` uses
that same dword as the registry key. Removal parses the key as a
`0xc0000000`-tagged value with tag below 3 and a low-24-bit payload;
`0xc0000000` also clears or represents invalid active selection. The semantic
tag and payload domains are unresolved, and this native selection sentinel is
not proven to be a wire sentinel.

`0x004d8f70` independently applies the same selection rule and dynamically
casts the selected `RaptureElement` to
`Application::Main::Element::Chara::CharaElement`. The 0x018D consumer itself
does not require that derived cast; it reads the base +0x88 field through
`0x004d6750`. Base-constructor callers span multiple RaptureElement families,
so the concrete subtype is runtime-selected. This proves a RaptureElement
registry identity domain and that the selection may admit CharaElement, not
that record +0x00 is an actor ID or that every selected object is CharaElement.

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

## Per-field presentation contract

`0x00671400` lazily asks its vtable +0x1c method for the exact resource name
`group_marker_data`, dynamically casts the result from
`Sqwt::ResourceDictionary` to `Sqwt::Data::SqwtXmlDataMaker`, and caches the
result at `MapScreenControl+0x9e8`. A failed lookup returns without reading
records or changing rows. The same class's property handler stores the exact
path `debug/pc_mark_sample.le.spk` at +0xa00, but the direct lookup does not
prove that `group_marker_data` came from that package.

The consumer accepts a source row only when all three conditions hold:

1. The byte at `MapScreenControl+0x57c` equals 2.
2. Record +0x00 is nonzero, or the bitwise OR of record +0x08 and +0x0c is
   nonzero.
3. Record +0x00 differs from the stable `RaptureElement+0x88` registry key of
   the object selected through Main +0x17838 with +0x17834 as a null fallback.

These facts do not name any of the three dwords. Accepted rows are compacted
into dense zero-based UI indexes; the source key is not the presentation
index. There is no separate create branch: the same seven `SetAttr` groups
target each accepted dense index whether that index is new or already present.

| Wire record | Storage record | Accessor | Local operation | UI argument |
|---:|---:|---:|---|---|
| +0x00 | +0x00 | 0055d090 | eligibility and selected-object comparison | none |
| +0x08 | +0x08 | 0055d0b0 | low dword of zero-sentinel OR | none |
| +0x0c | +0x0c | 0055d0b0 | high dword of zero-sentinel OR | none |
| +0x14 | +0x10 | 0055d050 | binary32 `CVTTSS2SI`, truncate toward zero to signed int32 | `X:Int` |
| +0x18 | +0x14 | none | not read by this consumer | none |
| +0x1c | +0x18 | 0055d050 | binary32 `CVTTSS2SI`, truncate toward zero to signed int32 | `Z:Int` |
| helper | +0x20 | 0055d030 | copy `Utf8String`; construct literal `!!!` through 00866010 and erase every occurrence through 0067ac00 | `Text:String` |
| helper | +0x74 | 0055d070 | raw dword, zero on lookup failure | `Layout:Int` |

The float conversion uses the x86 `CVTTSS2SI` instruction. It does not round
to nearest or preserve a fraction; invalid or out-of-range inputs follow the
instruction's integer-indefinite behavior. `X` and `Z` are exact UI property
names after this transform, not proof that the wire fields use world
coordinates.

Each accepted row receives seven exact `SetAttr` groups:

| Property | Type | Value source |
|---|---|---|
| `X` | `Int` | converted storage record +0x10 |
| `Z` | `Int` | converted storage record +0x18 |
| `Layout` | `Int` | helper dword at record +0x74 |
| `Text` | `String` | sanitized helper string at record +0x20 |
| `Visibility` | `String` | literal `Visible` |
| `SparkleSequence` | `String` | literal `m00002` |
| `Template` | `String` | literal `MapMarkerParty` |

When at least one row was accepted, the method dispatches `Update` exactly
once after the per-row writes. If the accepted count is below the existing
row count, `0x00942db0` removes the inclusive stale suffix
`[accepted_count, existing_count - 1]` in descending order with
`RemoveIndex`. Zero accepted rows therefore remove every existing row without
an `Update` call. Dense indexes are reused on the next invocation; there is no
per-source-key deletion branch.

The count path has two different comparisons. Projection sign-extends the
wire byte, skips its loop only for zero, and uses an unsigned back-edge
comparison, so a negative nonzero count is unsafe before presentation is
reached. The presentation consumer skips signed counts less than or equal to
zero and uses a signed less-than loop for positive counts. Neither path clamps
to the sixteen physical rows, and a positive count above sixteen reads beyond
the constructed storage records.

## Deferred +0x798 gate

The apply routine latches +0x798 when it was clear and the new count exceeds
one. `PcSearchWidgetOperator` vtable slot 29 (`0x00691f30`) later tests that
byte together with count equal to one through `0x0055d0d0`, calls
`0x00691e80`, and clears the byte through `0x0055d0f0`. That operation receives
no storage or record pointer. It is a later refresh gate, not the first
outward consumer, and its higher-level purpose remains unresolved.

## Verdict and boundary

The direct route proves a native UI property update. It contains no direct
Lua/N-API consumer and no network builder or emission. The network boundary is
the already proven inbound receive and projection chain; `0x00671400` only
reads the client storage and dispatches UI properties.

The copied header dwords at storage +0x08, +0x0c, and +0x10, the unused middle
record float at +0x14, and storage +0x798 do not influence the presentation
consumer. This does not make the original application header inert:
application +0x00/+0x04 selects the helper context before projection, while a
pointer to application +0x08 is forwarded but unused. The +0x798 state remains
confined to the separate deferred `PcSearchWidgetOperator` gate described
above.

No evidence names record +0x00 as an actor ID or +0x08/+0x0c as a marker type
or map ID. The unused middle float is not proven to be Y. No radius, rotation,
icon, label, or color field is proven, and the constant visibility, sequence,
and template values must not be reassigned to wire fields.

`group_marker_data` and `MapMarkerParty` are direct resource and template
literals at the UI boundary. They do not prove a relationship between the
wire keys and group-marker data, party state, actor state, or map state.
Likewise, `Text` and `Layout` are presentation property names, not native
semantic names for record +0x20 or +0x74.

The only direct chara-domain edge is the tag-0 string resolver's conditional
`LuaControl` to `CharaBase` cast and +0x60 `Utf8String` copy. That is a helper
text source; it does not establish an `Application::Scene::Actor` state edge
or make either lookup selector an actor identity.

The negative census covers Ghidra-recorded direct and data references, resolved
field and affine aliases, vtable entries, and the generated direct-call corpus
for the traced objects and methods. Unresolved computed or dynamic indirect
readers, runtime-only consumers, the semantic names and cross-update stability
of the three wire key dwords, the unused middle float, RTTI for the
`RaptureElementContainer+0x18` pointee, tagged owners outside the proven
`CharaBase` and tag-3 string paths, the source package for
`group_marker_data`, and server policy remain outside the result.

The focused machine contract is
[`config/s2c_018d_client_consumer.json`](../../config/s2c_018d_client_consumer.json).
It is checked by `tools/verify_s2c_018d_client_consumer.py` and the repository
gate. Public evidence names only retail addresses, committed catalog classes,
and the immutable upstream wire contract.
