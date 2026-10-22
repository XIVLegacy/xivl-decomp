# Music IDs 13 and 57 resource mapping

Music IDs 13 and 57 produce request keys whose numeric-mode paths exist as
distinct resources in the retained 1.23b installation. The retained
construction path binds the sound object's resource interface to
`Component::Resource::ResourceModule`. The pathname join remains conditional
on that binding remaining in force at the request and on numeric formatter
mode. File presence does not authenticate reported titles, audible playback
or historical zone usage.

## Retained identities

The installation's `game.ver` reads `2012.09.19.0001`. Its `ffxivgame.exe`
is 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
with image base `0x00400000`. Resource paths below are relative to that
installation. Their sizes and SHA-256 values identify the inspected bytes;
neither filenames nor the version label establish a title.

| Song ID | Requested resource ID | Numeric-mode path | Bytes | SHA-256 |
| ---: | --- | --- | ---: | --- |
| 13 | `0x07FE000D` | `data/07/FE/00/0D.DAT` | 3,244,271 | `1bfec9b74401244af9c1a3cd32b3f0a72729643b1695d0181916550a582ca4b6` |
| 57 | `0x07FE0039` | `data/07/FE/00/39.DAT` | 2,803,090 | `3d36a533f009c5c4ad8dc3bb2ae8aab2414fb9ffc4da11c68abfcc21c72ca817` |

Both files begin with ASCII `SEDBSSCF`. Their little-endian u32 at file
`+0x08` is 3, and the u32 at `+0x10` equals the complete file size. These
are observed header values, not a complete container schema or playback test.

## Request and pathname boundary

The native observations below used retained Ghidra instruction listings,
checked against the pinned PE with pefile `2024.8.26` and independently decoded
with Capstone `5.0.7`. All executable addresses are VAs. The check established
bytes, instruction boundaries and selected virtual-table entries, without
executing the client.

The [music control contract](../net/music-control-runtime.md#wire-fields)
owns the u16 song-ID input and `0x07FE0000 + songID` calculation. Applying
that calculation gives the two resource IDs above.

`0x004B7650` routes a replacement request through receiver slot 4 or 5.
Table `0x00F90F9C` supplies `0x00441DC0` and `0x00441F00` at those slots.
On their admitted new-request paths, both forward the unchanged request ID
through slot 1 of the interface at sound-object `+0x04`:

| Wrapper | Load interface / load slot-1 target | Push ID / indirect call |
| --- | --- | --- |
| `0x00441DC0` | `0x00441EC5` / `0x00441ECA` | `0x00441EDC` / `0x00441EDD` |
| `0x00441F00` | `0x0044200D` / `0x00442012` | `0x00442027` / `0x00442028` |

The new-call branch follows a nonzero return from `0x00B8E7B0` and zero AL
from helper `0x00441BF0`. Allocation-failure handling and the backend were
not characterized. These branches do not guarantee that each music request
creates a resource open.

Constructor `0x00C99090` installs table `0x01108C3C` at `0x00C990C3` and
returns its original object pointer at `0x00C9910E`. Initializer
`0x004B2DF0` calls it at `0x004B33B6`, stores its result at owner `+0x50`
at `0x004B33C8`, and reloads that pointer at `0x004B368F` for argument 2
of outer constructor `0x004DC3A0`, called at `0x004B3697`. The first outer
argument is the separately supplied value held in ESI. A zero ResourceModule
allocation skips its constructor and stores zero through the same field.
A separate zero outer-object allocation skips the outer constructor. The
binding below describes the normal constructor path, rather than construction
observed at runtime.

At `0x004DC3F6`, outer constructor `0x004DC3A0` loads `[ESP+0x88]` after
two pushes. Accounting for its prologue and those pushes identifies outer
argument 2. It forwards that pointer unchanged as argument 1 of dispatcher
constructor `0x004DBF40` at `0x004DC414`. The dispatcher receiver is outer
`+0x10`. The dispatcher loads its argument at `0x004DC0C1` and passes it
to sound constructor `0x004DD790` at `0x004DC0CD`, constructing member
`+0x928`. `0x004DD790` forwards the argument to `0x00442580`, which stores
it at sound-object `+0x04` at `0x004425CF`. These receiver adjustments
place the sound member at outer `+0x938`; they do not adjust the resource
pointer. `0x004DD7CE` installs sound table `0x00F90F9C`.

The original ResourceModule table entry at `0x01108C40` is slot 1 target
`0x00C99130`. Its complete-object locator at `0x0119C364` has offset zero
and points to TypeDescriptor `0x0130B648`, whose mangled name is
`.?AVResourceModule@Resource@Component@@`. This agrees with
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`.
The normal construction path supplies a concrete ResourceModule binding.

The inspected initializer has no direct overwrite of owner `+0x50` between
the store and reload. The inspected outer and dispatcher constructors have
no direct overwrite of the corresponding sound `+0x04` field, and
`0x004DD790` does not overwrite that field after its base call. The later
explicit sound-member call at `0x004DC2C4` targets `0x004419D0`, whose
complete body only returns AL=1. ResourceModule's later member constructor
`0x00C96C10`, called with object `+0x28`, writes its member's own table
and fields, with no direct write to the parent vptr. These checks cover the
named bodies' direct stores. Transitive callees and later code remain outside
that write coverage.

The [resource path producer](resource-path-producer.md#formatting-contract)
owns the conditional numeric formatter. When byte `0x01266B64` is nonzero,
the formatter at VA `0x0044B3A0` renders the ID as uppercase, most-significant
byte first, under `data/`. When the byte is zero, it uses a table route.
The paths in the table identify the retained numeric-mode files. Static
presence does not establish the formatter mode or a successful resource open
in a historical session.

The [initialization predicate](resource-path-producer.md#initialization-predicate)
identifies the indexed-record getter and conditional caller stores that
select numeric versus table mode. Its early-failure path reaches neither
caller store. The actual record value and request-time mode remain unproved.

If that pointer and ResourceModule vptr remain in force when the wrappers
load sound `+0x04`, their slot-1 target is `0x00C99130`. This is conditional
static dispatch reasoning, rather than an observed call or successful open.
The remaining boundary is call-time pointer/vtable state, request admission
and resource creation, numeric formatter mode, and open outcome. The matching
files supply no additional title or historical selection proof.

## Bounded metadata check

Python's standard-library byte reads and `struct.unpack_from` identified a
`03 76 6F 72 62 69 73` Vorbis comment marker at file `+0x320` in each file.
Reading its little-endian vendor length, vendor, comment count and
length-prefixed comments yielded vendor `Xiph.Org libVorbis I 20090709` and
two comments:

| Song ID | Comments | End after framing byte, exclusive |
| ---: | --- | --- |
| 13 | `LoopStart=1671169`, `LoopEnd=6963168` | `0x375` |
| 57 | `LoopStart=440956`, `LoopEnd=7055955` | `0x374` |

The following framing byte was 1 in both packets. Neither inspected comment
packet carries a title. This is a bounded metadata observation, not an
absence claim over every byte or possible metadata field. No audio stream
was extracted, decoded or played, and no duration or audible-loop behavior
is inferred from the comment values.

## Title confidence and zone usage

`xivl-captures:studies/classic-music-reference/derived/music-crosswalk.csv`,
rows `music_id=13` and `music_id=57`, reports The Forest's Pulse and Whisper
of the Land. Both have `secondary-named` title confidence. The owning study's
`derived/evidence-map.md`, Result and source, identifies the retained wiki
revision and digest. Resource presence and the metadata above do not upgrade
those labels to authenticated official titles.

The [song-ID resource table](music-song-table.md) covers the full song ID span
and the missing-file path.

The same rows report normal Black Shroud battles for 13 and Black Shroud
field use from patch 1.18 onward for 57, with secondary usage confidence.
`xivl-captures:studies/music-000c-wire/derived/map-context.md` owns the
retained `13/1 -> 57/4` packet context: missing stream coverage prevented a
confirmed active-map/zone join. The resource mapping supplies no new zone,
subarea, combat/restoration policy or accepted zone-data value.

## Reproduction

Use the retained installation and exact identities above. Do not substitute
a newly acquired soundtrack or a file selected by a reported title.

1. Read `game.ver`, then check the executable and the two DAT sizes/SHA-256
   values with `Get-Item` and `Get-FileHash -Algorithm SHA256`.
   Check the selected constructor/call instructions against that PE. Follow
   the original pointer through owner `+0x50`, outer argument 2 and dispatcher
   argument 1 to sound `+0x04`, accounting for moving ESP at `0x004DC3F6`.
   Read ResourceModule table slot 1 at `0x01108C40`, its complete-object
   locator and TypeDescriptor. Keep constructor-time binding distinct from
   call-time state, formatter mode and resource-open success.
2. Compute `0x07FE0000 + songID` for 13 and 57. Render its four bytes as
   uppercase hexadecimal in most-significant-first order for numeric mode.
3. Read only the selected headers and metadata. Check `SEDBSSCF`, the u32
   values at `+0x08/+0x10`, and the marker at `+0x320`. Parse the vendor and
   each comment using its u32 little-endian length, with every extent bounded
   by the file. Check the comment count, framing byte and exclusive ends above.
4. Keep file presence, request/open success, audible identity and historical
   selection as separate claims. This inspection establishes only the selected
   conditional pathname mapping and retained resource presence.
