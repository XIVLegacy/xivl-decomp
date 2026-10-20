# Guildleve actor initialization route

The ordinary native `0x00CC` path performs a prefix read and five Boolean
control reads before constructing the Lua initializer's value vector. The
prefix helper consumes the retained string through its NUL; the ordinary
non-current-player arm leaves `(20005,12487,6,0,0,0)` and its first false
control selects deferred initialization. Descriptor transfer and lifetime
conditions below prevent treating this static route
as proof of a completed historical call.

The captured record and complete tagged values belong to
`xivl-captures:studies/regional-guildleve-publisher-contract/README.md`,
Retail client Director binding for 12487. Its main connection 0, s2c frame
50 / subevent 17 has source `0x45100D44`, class `PrivateGLBattleOrbNormal`,
and a 264-byte application with SHA-256
`6d8094109a3c70cbf8252ef6dba9c457ccb67b112e189b6c6681c7055e933491`.
The Lua formal consumers belong to
`xivl-client-scripts:docs/guildleve-initializer-arguments.md`.

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
payloads as detailed below. Complete vector preservation still depends on
producer growth and the outer ordering container. Iterator helpers
`0x0071CB30/0x006CFBB0` supply source end/begin, guarded by begin <= end.
The outer containers use `0x0058F650 -> 0x009D186E`; the latter checks
pointer/capacity preconditions for nonzero count and forwards
`(destination,source,count)` to uninspected `0x009D5110` at `0x009D18BB`.
Its catalog name `_memmove_s` alone does not establish byte movement here.

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

If the deferred transfer preserves V and the selected request succeeds, the
Lua input is `_onInit(self,20005,12487,6,0,0,0)`. The script finding maps those
values to the client Director ID, Guildleve ID, location and extra-marker XYZ.
This route does not prove complete producer retention, outer ordering transfer
or historical execution.

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
The prefix/suffix helper `0x0078AA00` and record-construction helper
`0x00584930` have not been inspected here. These call operands establish the
growth structure, not preservation through those helpers. The spare-capacity
proof therefore does not establish all six appends from an initially empty
container, allocation success, exception behavior or complete retention.

`0x00748E00` also operates the outer DWORD container through uninspected
`0x0078DD00` at `0x00748E34`, using a value supplied through `0x00CCA210`.
Together with `0x009D5110` and the two growth helpers above, these are the
remaining producer/order edges. The scalar and spare-capacity proofs do not
establish complete six-argument storage or remove the earlier request and
runtime conditions.

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
their enclosing span is not continuous instruction coverage. No typed data
or body of either remaining helper (`0x0078AA00`, `0x00584930`) was read
for this append finding.
