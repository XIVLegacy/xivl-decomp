# Guildleve actor initialization route

The ordinary native `0x00CC` path performs a prefix read and five Boolean
control reads before constructing the Lua initializer's value vector. The
prefix helper consumes the retained string through its NUL; the ordinary
non-current-player arm leaves `(20005,12487,6,0,0,0)` and its first false
control selects deferred initialization. The append, descriptor-copy and
argument-order proofs below preserve all six values on this static route.
Allocation, request and lifetime conditions still prevent treating it as
proof of a completed historical call.

The captured record and complete tagged values belong to
`xivl-captures:studies/regional-guildleve-publisher-contract/README.md`,
Retail client Director binding for 12487. Its main connection 0, s2c frame
50 / subevent 17 has source `0x45100D44`, class `PrivateGLBattleOrbNormal`,
and a 264-byte application with SHA-256
`6d8094109a3c70cbf8252ef6dba9c457ccb67b112e189b6c6681c7055e933491`.
The Lua formal consumers belong to
`xivl-client-scripts:docs/guildleve-initializer-arguments.md`.

## Six initializer arguments

Positions below exclude `self`. The native path and five Boolean controls
precede the values on the wire but are not arguments to `_onInit`.

| Position | 12487 value | Client consumer | Source established here |
| ---: | ---: | --- | --- |
| 1 | 20005 | `DirectorBaseClass._onInit` stores `directorWork.directorId`, then forwards the remaining arguments to `self:init(...)` | First integer in the received initializer. No inspected client producer derives it from the leve ID. |
| 2 | 12487 | `GuildleveBaseClass.init` stores `guildleveWork.guildleveId` and uses it as the `guildleveSheet` key | Second received integer, the Guildleve row ID. |
| 3 | 6 | The same method stores `guildleveWork.aetheryteLocation` | Third received integer, an aetheryte location selector. It is neither the aetheryte row ID nor the Guildleve sheet's separate `location` field. |
| 4 | 0 | The same method stores `guildleveWork.exMarkerX` | Received extra-marker X coordinate. |
| 5 | 0 | The same method stores `guildleveWork.exMarkerY` | Received extra-marker Y coordinate. |
| 6 | 0 | The same method stores `guildleveWork.exMarkerZ` | Received extra-marker Z coordinate. |

The first value is therefore the client field named `directorId`, distinct
from the transport actor ID `0x45100D44` and the Guildleve ID 12487.
The name alone does not prove a battle-area identifier or a rule for choosing
the value for another leve.

### Director ID and the sheet's border field

The static field named `borderId` is position 8 in
`xivl-client-data:tools/mappings/guildleve.py`. Here and below CSV positions
exclude the row ID and are zero-based. For 12487 this field is 20005, equal
to the first captured integer. It also equals `guildleve_UI.csv` position
77 for every one of the 624 Guildleve rows. This is a sheet relationship,
not proof that the network producer copies that field into `directorId`.

There is a concrete presentation consumer for this sheet value.
`GuildleveExecutionWidget.init` reads `guildleve_UI` position 77 for the
Guildleve ID and passes it as `setCard`'s second value. `setCard` supplies
that value to `setIcon("IconControl_Plate", value)`. This establishes a
card/plate icon use. The inspected route supplies no battle-area use.

| Guildleve row | `guildleve[8]` / `guildleve_UI[77]` | `guildleve[6]` aetheryte row | That aetheryte's field 2 |
| ---: | ---: | ---: | ---: |
| 12487 | 20005 | 1280067 | 6 |
| 10824 | 20004 | 1280002 | 6 |

The second row checks a different border value against the same client UI
consumer. It does not supply a second observed initializer. In particular,
`D = guildleve[G][8]` remains an unproved network-producer rule. The joined
value `aetheryte[guildleve[G][6]][2]` likewise matches L=6 for 12487, but
the initializer does not perform that lookup. Its L is supplied on the wire.
Do not substitute either table join for an observed producer when deriving
another leve's initializer.

The sheet identities and column conventions are in
`xivl-client-data:docs/guildleve-selected-row-values.md`, Source identity,
and `xivl-client-data:manifests/tables.json`. Direct Python CSV reads matched
those complete-file pins: `guildleve.csv` SHA-256
`74ee8061449bf538d504443017c776f994c06fd1bcd2fd008bbee18284d93b2d`,
`guildleve_UI.csv` SHA-256
`956267ede9d4e488ee8b29e78e200a83536d3f9056e51e98090f2b1313e47af2`,
and `aetheryte.csv` SHA-256
`7f0cdeecf05f10bdcf4b917238b0c3a5495dca61de3c4d491886e9d123bf6692`.

### Extra-marker use

`getPosByMapMarkerExtra` returns the stored X/Y/Z in order. The base
`processMapOpenMessage` and `setMiniMapMarkerForGL` read `signal` and,
when it is positive, submit these coordinates to
`desktopWidget:setMapNavigationWidgetMarkerData` or
`desktopWidget:setMiniMapWidgetMarkerData`. They obtain the extra marker's
size with marker selector 4. These branches do not test the extra
coordinates for zero. Three zeroes alone therefore do not disable the
extra-marker call.

The base size selector calls `getAetheryteLocation`: 6 returns `normal`,
other values return `small`. This confirms a concrete use of the third
argument without assigning a geographic name to 6.
`PrivateGLFreeGatherTown` also supplies the extra position to its map and
minimap achievement hooks. The getter's complete result tail reaches the
widget call in both hooks.

The last three values are coordinate inputs, not difficulty or party-state
inputs in these consumers. They are nonzero when the packet producer supplies
nonzero coordinates. The client initialization examined here neither computes
them nor selects the producer's reason for doing so. Nonzero retail examples,
coordinate units and a leve-to-position generation rule require separate
evidence. The positive-signal base branch and achievement overrides establish
uses, not that the captured all-zero vector rendered a marker.

### Retained-capture cross-check

All 54 restored members matched the sizes and SHA-256 pins in
`xivl-captures:sources/pcap-1.23b/manifest.yaml` at revision
`526954241b592aefec1d57f3697796114466753c`. Iterating `_decode_capture` from
`tools/extractors/extract_00da_00e1_comparison.py` at that revision and
selecting complete s2c `0x00CC` applications with a
`/Director/Guildleve/` path found only the 12487 record above. The other 25
applications containing `Guildleve` were Chara object paths. There was no
second regional Director initializer, alternate D, or nonzero XYZ example
in the admitted corpus. The different-border row 10824 above is therefore
a static-data/UI cross-check, not a captured argument substitution.
A search of the 2,671 manifest-authenticated recovered Lua texts found no
literal 10824, and no literal 20004 in Director scripts. It supplies no
alternate initializer for that row either. This is a text-search boundary,
not a claim about every native caller or omitted decompiler behavior.

The canonical decoder reported zero admitted unparsed bytes, subevent
truncations, short wrapped headers and inflation failures. It discarded 228
reconstructed s2c trailing bytes outside complete frames. The selected
initializer was complete. `tools/validate_framing.py` also reported zero
compression-flag mismatches for all 54 objects. This absence covers the
decoder's complete clear game frames, not uncaptured sessions or the
discarded tails.

### Lua binary consumers

These are decoded Lua 5.1 payload offsets, not executable VAs. Children are
zero-based beneath each root and PCs are one-based. A direct Python operand
parser checked the original retail LPBs after XOR-0x73 decoding from byte
13, authenticated payloads against
`xivl-client-scripts:manifests/retail_lua_coverage.json`, and authenticated
the corresponding recovered text against
`xivl-client-scripts:manifests/scripts.json`. The observation is the encoded
register/constant transfer, not a decompiler-assigned parameter name.

| Retail class / method | Child, PC and payload offset | Observation |
| --- | --- | --- |
| `DirectorBaseClass._onInit` | 5, PC56 `0x5B6`; PCs57-59 `0x5BA..0x5C2` | `SETTABLE` writes parameter R1 to `directorId`; open `VARARG` and `CALL` forward its tail. |
| `GuildleveBaseClass.init` | 17, PCs112/114/120/122/124 `0x112C/0x1134/0x114C/0x1154/0x115C` | Stores its R1/R2/R3/R4/R5 as G/L/X/Y/Z. PCs106-110 `0x1114..0x1124` read sheet column 21 with G as key. |
| `GuildleveBaseClass.getPosByMapMarkerExtra` | 16, PCs1-7 `0xEE8..0xF00` | Reads X/Y/Z and returns all three. |
| `GuildleveBaseClass.processMapOpenMessage` | 48, PCs55-73 `0x2D0F..0x2D57` | Tests `0 < signal`, then passes extra X/Y/Z to the map widget. |
| `GuildleveBaseClass.setMiniMapMarkerForGL` | 50, PCs60-78 `0x30CE..0x3116` | Same positive-signal test and XYZ transfer to the minimap widget. |
| `GuildleveBaseClass.getAetheryteLocation` / `processSetMapMarkerSize` | 19, PCs1-3 `0x14B2..0x14BA`; 53, PCs2-10 `0x3477..0x3497` | Returns L, compares it to 6, returns `normal` or `small`. |
| `GuildleveExecutionWidget.init` / `setCard` | 0, PCs75-91 `0x32C..0x36C`; 6, PCs26-29 `0x902..0x90E` | Reads sheet column 77, forwards it as the second card value, and sets `IconControl_Plate`. |
| `PrivateGLFreeGatherTown` achievement map / minimap hooks | 5, PCs6-8 `0x742..0x74A`; 6, PCs6-8 `0x806..0x80E` | Calls the extra-position getter with open results, then the widget with open arguments. |

Director and Guildleve LPB identities are pinned in the cited initializer
argument finding. The widget resource is
`client/script/n1635q/3p1y6y5o55m57pq1vwn1635q.le.lpb`, original SHA-256
`8eed436e9058cba2bc7ea92603a1a493cf10be7ace8b676ea66b5f384326c8cd`,
payload SHA-256
`58812bfccc48e5d651569f55382de88c55a365ec91d0a9171ffe93e751efce13`.
The achievement override resource is
`client/script/61s57qvs/3p1y6y5o5/us1o9q53y4s5539q25sqvnw.le.lpb`,
original SHA-256
`ac3790a105226867ab29edbae997af6388ab2a84f0a660b9932cae41250e4292`,
payload SHA-256
`79f4a253dd051c94d7e739c5da164467cfba92b57e9876df275362f8ad301208`.

## Native body and prefix

The ordinary arm of `0x004D8860` adds `0x10` to its packet header pointer at
`0x004D88FD` and passes that body to `0x00574780` at `0x004D8902`.
The wrapper supplies the same pointer as argument 2 to `0x00774AD0` at
`0x005747FB`. The [mutable-holder contract](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
owns the preliminary branch and implementation selection conditions.

The native body's `+0x24` class and `+0x44` stream positions match the
published application layout. Offsets below use this body/application
identification; no historical native buffer was recorded. The dispatcher
header's `0x10` is not added again to application offsets.

`0x00774C1C` constructs the class string from body `+0x24`.
At `0x00774C36`, reader constructor `0x0078F810` receives body `+0x44`
and length `0xC0`, retaining cursor and remaining length. This bounds the
native stream at application `+0x104` exclusive. The four application bytes
at `+0x104..+0x107` are outside it. The reader must pass the separate
`0x0078FA80` guard, whose validation behavior is not established here.

Wrapper `0x0078F8D0` delegates to prefix helper `0x00791CA0` and stores its
returned EAX as the reader cursor. The helper skips one tag byte, calls string
assignment `0x004489C0`, then explicitly scans to NUL at
`0x00791CC0..0x00791CC9`. It advances past NUL and subtracts the counted
length plus two from remaining at `0x00791CCF..0x00791CDF`. For this record,
44 path bytes therefore consume 46 stream bytes: cursor `+0x44 -> +0x72`,
remaining `0xC0 -> 0x92`. This assumes string assignment returns normally.
The helper checks neither the skipped tag nor the scan against remaining;
the retained input supplies tag `0x02` and an in-bound NUL at `+0x71`.

| Application offset | Retained input | Consumer and branch |
| --- | --- | --- |
| `0x44..0x71` | String tag, full Director path, NUL | `0x00774CC7 -> 0x0078F8D0 -> 0x00791CA0`; returned output/cursor |
| `0x72` | False | `0x00774CCE -> 0x0078F840`; passed as argument 8 to request submission |
| `0x73` | False | `0x00774CD5 -> 0x0078F840`; skips an optional six-read bundle |
| `0x74` | False | `0x00774DD1 -> 0x0078F840`; skips `0x0078F900` and a native `+0x4C` update |
| `0x75` | False | `0x00774E40 -> 0x0078F840`; skips a read and native `+0xB6` write |
| `0x76` | False | `0x00774E8D -> 0x0078F840`; skips another read |

`0x0078F840` reads one byte, writes true only for tag `0x03`, advances the
cursor by one and decrements remaining length. It does not append a Lua
value. The five retained `0x04` bytes are therefore controls, leaving cursor
`+0x77` and remaining `0x8D` for the typed remainder.

At `0x00774B52`, equality with the key returned by `0x004D7490` selects a
separate current-player path. That arm consumes sixteen plus four integers
before the general visitor. The six-value remainder below requires that arm
not to be selected. The historical key comparison was not observed.

## Vector, deferred descriptor and request

The caller initializes V through `0x00747E20` at `0x00774F8B`.
Taking S as caller ESP before `0x00774F82`, the post-push
`LEA ECX,[ESP+0x64]` at `0x00774F87` passes `V = S+0x60`.
The initializer returns with `RET 4`. The later `LEA ECX,[ESP+0x60]`
at `0x00774FDA` and push at `0x00774FDE` supply that same V as the
visitor's third argument.

The initializer zeroes outer begin/end/capacity at
`0x00747E55..0x00747E5B` and middle begin/end/capacity at
`0x00747E74..0x00747E7A`. It reserves `0x80` entries through
`0x00582AA0` at `0x00747E9B` and `0x00584C50` at `0x00747EA7`.
On successful allocation, outer stores at `0x00582B84/0x00582B8A/0x00582B8D`
set capacity/end/begin using the old logical count zero. Middle stores at
`0x00584D41/0x00584D49/0x00584D4C` do the same with 16-byte records.
Both ranges therefore remain empty, with capacity for `0x80` entries.
The six integer appends fit their spare-capacity arms on this successful
initialization path. The growth proof below also covers their fallback arms.

`0x00774FE9` calls `0x00790AA0` with the remaining reader and output vector V.
The reused [typed-value contract](../script/delegate-command-producer.md#typed-values-and-bounds)
supplies one Lua value per covered token and
no value for END. With the default big-endian interpretation, integer tags at
application `0x77,0x7C,0x81,0x86,0x8B,0x90` yield
`(20005,12487,6,0,0,0)` in that order. END is at `0x95`. The historical
endian selector `0x0134B368` remains unobserved. The caller does not test the
visitor's return value before continuing, so conversion is not an invocation
success check.

At `0x007750B0`, submission `0x00774530` receives
`(key-ref,name,class,path,V,callback,owner,first-control)`.
For this first control's false value, the immediate arm is rejected at
`0x007745B0..0x007745B8`. Constructor `0x00773960` receives the vector
through argument 5. At `0x007739DC` it passes that argument to
`0x0078ADB0`, with destination descriptor `D+0x100`.

The constructor also constructs name/class/path strings at `D+4`, `D+0x58`
and `D+0xAC`, and stores callback/owner/flag at `D+0x130/0x134/0x138`.
Helper `0x0078ADB0` constructs three containers at vector offsets
`+0/+0x10/+0x20`. The outer two use `0x005831A0`, which requests a buffer
for DWORD elements; the middle uses `0x0078AC70 -> 0x0078AA30`, which
requests a buffer for 16-byte records. Insertion `0x0078B190` requests a
`0x144`-byte node and passes node+8 to `0x0078B080 -> 0x0078AEA0`.
The latter constructs matching key/string fields, calls `0x0078ADB0` again
for the retained descriptor's `+0x100`, and copies callback/owner/flag.
It constructs a second descriptor rather than retaining the temporary's
address. Valid nonoverlapping allocation returns give the containers separate
buffers; allocation and construction must return normally.

The middle-record copy preserves physical slot order and type-0 scalar
payloads as detailed below. Iterator helpers
`0x0071CB30/0x006CFBB0` supply source end/begin, guarded by begin <= end.
The outer containers use `0x0058F650 -> 0x009D186E`; the latter checks
pointer/capacity preconditions for nonzero count and forwards
`(destination,source,byte-count)` to `0x009D5110` at `0x009D18BB`.
The original byte-copy instructions and jump-table words establish movement
for the aligned, nonoverlapping outer ranges used here. For DWORD counts
0..6, the table at `0x009D5218` selects, respectively,
`0x009D527B/0x009D5268/0x009D5260/0x009D5258/0x009D5250/0x009D5248/0x009D5240`.
Those bodies copy matching source/destination offsets. The tail table at
`0x009D5284` selects `0x009D5294/0x009D529C/0x009D52A8/0x009D52BC`
for 0..3 remaining bytes. Thus zero through six outer entries retain their
bit values and order through both descriptor copies. This proof does not
cover the large/SIMD or reverse-overlap branches of the general byte copier.

Consumer `0x00772C80` passes `D+0x100` as argument 4 to `0x007687F0` at
`0x00772DF4`. Its selected class normally comes from `D+0x58`; a separate
current-player arm can append `MyPlayer`. The saved path at `D+0xAC` is not
an initializer value in this call. At `0x00768923`, `0x007687F0` forwards
its vector unchanged as argument 5 to `0x0078BBB0`:

```text
(state/handle, name, class, key-ref, vector, initializer-callback)
```

After binding and the optional callback, `0x0078BBB0` loads this vector at
`0x0078BC3E`, then passes it to `0x00CD0910` at `0x0078BC50` with method
name `_onInit` from `0x00FE04F0`. These forwarding callers insert no path
or controls and remove no numeric value. The existing
[request and completion contract](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion)
owns engine, instance, method-lookup, request, callback and lifetime conditions.

With valid vector ranges and normal allocation/copy returns, the deferred
transfer preserves V. The ordered scalar consumer below supplies
`_onInit(self,20005,12487,6,0,0,0)` if the selected request succeeds.
The values reach the client fields in the six-argument table above.
Historical execution remains unobserved.

## Scalar record copy and cleanup

`0x0078AAE5 -> 0x0078A9D0 -> 0x0078A790` populates the middle buffer.
The loop compares source to end at `0x0078A7D0`, constructs a destination
record through `0x005847D0` at `0x0078A7D9`, then advances both pointers by
`0x10`. For valid aligned endpoints and normal returns, source slot i goes
to destination slot i. This proves physical middle-slot order, separately
from logical Lua argument order.

Placement constructor `0x005847D0` initializes a record and calls
`0x00584670`. For source type byte `+0xC == 0`, assignment takes the
payload address from source `+8` and calls `0x00583D90` at `0x0058469B`.
That helper reads a DWORD through the address at `0x00583DFA`, writes it
at destination `+0`, stores destination `+8 = destination` at `0x00583E06`,
and sets type `+0xC = 0` at `0x00583E09`. Thus a valid type-0 record's
inline numeric cell is copied and its payload pointer rebased to the new
record. Two successful middle-buffer constructions preserve these cells
without retaining pointers into the source records.

Temporary descriptor cleanup `0x007724D0 -> 0x00744270` releases its outer
buffers and invokes `0x005843E0 -> 0x00584370` for the middle range.
Only type 3 with a nonnull payload reaches release call `0x004D1670` at
`0x005843A0`. Type 0 skips that call and clears its own pointer/type fields.
Destroying the temporary's type-0 records therefore does not release the
retained copy's separate inline cells. Other value types and exception paths
are outside this scalar contract.

## Producer append boundary

The integer visitor `0x00793430 -> 0x00748E00` passes the integer address
to `0x00584F70` through a stack proxy whose `+4` points to vector `+0x10`.
At `0x00584FB6`, it constructs a local type-0 record through `0x00583D90`,
then submits it to `0x00584EE0` at `0x00584FCB` before local cleanup.
That callee reads the middle container's begin, end and capacity-end pointers
at `+4/+8/+0xC`. With valid aligned pointers, a nonnull begin and spare
capacity select the call to `0x00584840` at `0x00584F25`. It supplies the
current end, count 1 and the source record. `0x00584840` invokes the
already-established placement constructor `0x005847D0` at `0x0058488B`
and advances its destination by `0x10`. After normal return,
`0x00584EE0:0x00584F2D..0x00584F30` advances the container end by `0x10`.
This arm appends a copied type-0 scalar whose inline pointer is rebased,
rather than retaining a pointer into the temporary source record.

A null begin or exhausted capacity takes the other arm. `0x00584D80`, called at
`0x00584F55`, derives the insertion index from the old end, supplies count 1
and the source to `0x00584970` at `0x00584DD0`, then reconstructs an iterator
against the updated begin. Within `0x00584970`, the capacity-growth branch
calls `0x0044D500` with `(chosen-capacity * 0x10,0,0x15)` at `0x00584A66`.
It passes the old prefix to `0x0078AA00` at `0x00584A85`, the copied local
record and insertion count to `0x00584930` at `0x00584A95`, then the old
suffix to `0x0078AA00` at `0x00584AA8`. After those calls return normally,
a nonnull old begin selects cleanup through `0x00584370` and the call to
`0x0044D350`. It then updates the three container pointers at `0x00584AFB`,
`0x00584B03` and `0x00584B06`.
`0x0078AA00` reaches the ordered record-copy loop `0x0078A790` at
`0x0078AA1E`. `0x00584930` reaches the one-record constructor loop
`0x00584840` at `0x00584952`. The same type-0 placement/assignment helpers
therefore preserve the old prefix, new scalar and old suffix, including
rebasing each inline payload pointer. Both the spare and growth arms append
one retained scalar. Induction from empty containers covers all six successful
integer appends without assuming spare capacity.

The matching outer entry comes from `0x00748E00`. Its call to singleton
accessor `0x00CCA210` is at `0x00748E07`; `0x00748E24..0x00748E27`
load the singleton's vector begin and its first entry pointer. The constructor
`0x00CC9D60` initializes the vector, supplies entry zero's callback object
with vtable `0x0110DF38` at `0x00CC9DC9`, stores the callback object at
entry `+4` at `0x00CC9DCF`, and appends the entry at `0x00CC9DEC`.
Original PE vtable data binds slot `+0x08` to `0x00CF9940` and selector
`+0x18` to `0x00CF8880`. This binds an actual callback identity, not just
an uninterpreted DWORD or an inferred type name.

At `0x00748E34`, `0x0078DD00` appends that entry pointer to the outer
container. Its spare arm stores the DWORD at the current end at
`0x0078DD32`. Its growth call at `0x0078DD5C` reaches `0x0078DC70`,
which computes the old DWORD count and calls `0x0078DA00` with one insertion
at `0x0078DCC0`. The latter allocates capacity times four through
`0x0044D500` at `0x0078DADB`, copies prefix and suffix through
`0x00DA8EA0` at `0x0078DAFA/0x0078DB17`, inserts the pointer through
`0x0080F500` at `0x0078DB0A`, releases the old buffer, and updates
begin/end/capacity at `0x0078DB5C..0x0078DB68`. The relocation helper
reaches the byte copier proved above. With valid aligned ranges and normal
returns, six integer visits produce six ordered scalar records and six
copies of the same entry-zero pointer. The two `0x0078ADB0` descriptor
constructions preserve both ranges.

### Ordered scalar consumption

The existing [supplied-frame contract](../script/delegate-command-producer.md#function-binding-value-and-supplied-frame)
owns binding/function lookup, one-value conversion and the Lua call boundary.
The initializer uses its same input path:

```text
0x00CD0910 -> 0x00CCF9B0 -> 0x00CCEE30
             -> 0x00CCFFE0 -> 0x00CCFA70 -> 0x00CCFF60 -> 0x00CF9630
```

The direct calls to `0x00CCEE30` and `0x00CCFFE0` are at
`0x00CCF9F4` and `0x00CCF042`. `0x00CD003D` supplies callback literal
`0x00CCFF60`, and `0x00CD004D` calls iterator `0x00CCFA70`.
The iterator derives the outer count from `(end-begin)>>2`, initializes
one shared middle cursor to zero at `0x00CCFA8D`, forms
`begin + index*4` at `0x00CCFAAF..0x00CCFAB2`, and passes the cursor's
address from `0x00CCFAB9` through the callback call at `0x00CCFAC0`.
Its outer index increments at `0x00CCFAC4`.

Callback `0x00CCFF60` passes the entry's callback object, the middle
context and that same cursor pointer to `0x00CF9630` at `0x00CCFF7C`.
For `cursor < (middle_end-middle_begin)>>4`, `0x00CF9630` reaches vtable
slot `+0x08` at `0x00CF97A0/0x00CF97A8`. Entry zero binds this to
`0x00CF9940`. Its type-0 selector is `0x00CF8880:0x00CF8886`.
The converter reads the shared cursor at `0x00CF9983`, addresses
`middle_begin + cursor*0x10` at `0x00CF9999..0x00CF999C`, and inspects
the copied scalar record. For type 0 it loads the payload pointer at
`0x00CF99F6`, executes `FILD dword ptr [ECX]` at `0x00CF99FA`, hands
the number to `0x00CF3300` at `0x00CF9A07`, and increments the cursor
at `0x00CF9A0C`.

Consequently the six integer entries consume middle slots 0..5 as
`D,G,L,X,Y,Z`. The supplied-frame contract places each materialized value
after the binding value `self`, so the physical-copy and logical-order
proofs join without assuming that an ordered callback list alone implies
ordered scalar selection. Producer append and outer ordering are closed
for this six-integer route under the stated normal-return conditions.

## Remaining unknowns

The client evidence does not establish a general producer rule equating D
with the sheet's border value, the producer of L, or which leves receive
nonzero extra-marker coordinates and how those coordinates are generated.
The static row joins above remain correlations. Coordinate units and an
alternate captured regional initializer are also unestablished.

Historical branch/endian state, reader validation, allocation success,
request admission, binding lookup, callback completion and object lifetime
remain runtime conditions. Non-integer vectors, malformed ranges and
exception paths are outside this scalar proof.

## Binary evidence

The input is retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VAs. Retained Ghidra instruction listings were checked
against original PE file bytes with pefile `2024.8.26` and Capstone `5.0.7`
in x86-32 mode.

Checked entries were `0x004D8860`, `0x00574780`, `0x00774AD0`,
`0x00774530`, `0x007687F0`, `0x00772C80`, `0x0078F810`,
`0x0078F8D0`, `0x0078F840`, `0x00790AA0`, `0x0078BBB0` and
`0x00773960`. Their instruction union was 4,926 bytes. The listing gaps
`[0x007750D2,0x007750D5)` and `[0x007688F2,0x007688F5)` remain unfilled;
enclosing function spans are not continuous decoded coverage.
The separate eight-byte `_onInit` string at `0x00FE04F0` matched original
bytes. A controlled call-byte mutation at `0x005747FB` failed authentication.

Prefix and retention evidence additionally checked `0x00791CA0`,
`0x0078ADB0`, `0x0078B190`, `0x007724D0`, `0x005831A0`,
`0x0078AC70`, `0x0078B080`, `0x00744270`, `0x0078AA30`,
`0x0078AEA0`, `0x0058F650` and `0x005843E0`: 1,570 instruction bytes,
with no typed-data reads. The six-byte listing gap
`[0x00791CBA,0x00791CC0)` remains unfilled. Forty-eight focused operands
matched, and six controlled instruction mutations failed authentication.

Scalar copy and cleanup evidence checked `0x0078A9D0`, `0x00584370`,
`0x0071CB30`, `0x006CFBB0`, `0x009D186E`, `0x0078A790`,
`0x005847D0`, `0x00584670`, `0x00583D90`, `0x00793430`,
`0x00748E00` and `0x00584F70`: 1,040 instruction bytes, no typed data.
This includes the 16-byte integer-builder revisit. Forty-five focused operands
matched, and ten controlled instruction mutations failed authentication.

Producer append evidence used Ghidra `12.1.3` through REA `6.4.0` and the
same original-byte decoder versions. It checked complete entries
`0x00584EE0` (130 bytes), `0x00584840` (119 bytes) and `0x00584D80`
(139 bytes), plus these noncontiguous `0x00584970` body ranges:
`[0x00584970,0x00584B0E)`, `[0x00584B35,0x00584B9C)` and
`[0x00584BBF,0x00584C4C)`. All 410 instruction starts matched the original
decoder across 1,046 instruction bytes. Eleven call targets and eight focused
operands matched. The 74 bytes between those last ranges were not inspected;
their enclosing span is not continuous instruction coverage.

The storage/order verification additionally authenticated 43 retained
Ghidra listings against original PE bytes using the same pefile and Capstone
versions: 3,366 listed instruction starts covering 10,145 distinct bytes,
including revisits of earlier helpers. It checked 65 focused instruction
sites, their expected direct targets or callback literals, and rejected
controlled one-byte mutations. A separate 42-byte original slice at
`0x00CCFF60` covered the 40-byte callback body and two padding bytes.
Original data reads covered 40 bytes at `0x0110DF38`, 32 bytes at
`0x009D5218`, and 16 bytes at `0x009D5284`; their callback and small-copy
dispatch words matched, and controlled table mutations were rejected.

The additional retained listings for `0x00CCA210`, `0x009D5110`,
`0x00CCF290` and `0x00CCFFE0` contain gaps. The instruction counts above
are listed-byte coverage, not continuous enclosing-span coverage. The
`0x00CCF290` sibling prepass is not needed for the initializer proof.
The relevant small-copy table words and actual input callback path were
checked separately as described above. The observations establish the
static storage and consumption route, not runtime execution.

The base-case check separately decoded 38 focused original instruction sites
for `0x00747E20`, outer reserve `0x00582AA0`, middle reserve `0x00584C50`,
and the caller/visitor argument transfers. Direct targets, reserve immediates
and the stack-relative V identity matched; controlled instruction mutations
were rejected. The helper extents are `[0x00747E20,0x00747ECA)`,
`[0x00582AA0,0x00582BA4)` and `[0x00584C50,0x00584D63)`.
Those extents identify the entries, not an assertion that all their bytes
belong to the 38 focused checks. The empty-state postcondition requires
successful nonnull allocations and normal helper returns.
