# Summoned retainer item storage

The 1.23b retainer requests Lua packages `1`, `100`, and `8`, encoded in
c2s `0x0131` as package bytes `0`, `99`, and `7`. An open
`RetainerTradeWidget` consumes published item changes for both the player
and the retainer. A successful event notice does not replace those updates.
The traced client selects an item on the retainer for result `31` and on
the player for result `32`: their source directions are retrieve and entrust,
respectively. Server operation names are not established by those UI codes.

## Evidence identity and method

Native observations below use retail `ffxivgame.exe`, version
`2012.09.19.0001`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. All native addresses are VAs; subtract
that base for RVAs. Direct file-backed instruction reads used pefile
`2024.8.26` for section mapping and Capstone `5.0.7` in x86-32 mode.
These are static observations, not a live server/client acceptance test.

The shared native item queue and serializer findings are canonical in
`xivl-client-structs:docs/bazaar-native-wire-contract.md`, sections
"Package and slot numbering" and "Incoming package operations and
publication", and
`xivl-client-structs:manifests/player_command_wire_contract.json#serializer`.
Those records supply the Ghidra recipes and their limits. The instruction
locators below were also read directly from the identified PE.

Lua observations use the original installed LPBs decoded by
`xivl-client-scripts:tools/retail_script.py:decode_lpb`, then inspected with
`tools/build_quest_contracts.py:ChunkReader` and `instruction` in that
repository. Each LPB and decoded payload matched its
`manifests/retail_lua_coverage.json` entry. The decompiled members matched
`manifests/scripts.json`. `0.N` below means root child N; PCs are zero-based,
and offsets are bytes in the decoded Lua chunk. This avoids relying on
unluac's misleading rendering of some joined conditions.

| Member below `lua/scripts/` | Decoded payload SHA-256 |
| --- | --- |
| `chara/npc/retainer/retainerbaseclass.lua` | `f75aed434e40af97f5c865dc9803acdbd8d997b185d2cf10c9c5e73e6635deb3` |
| `chara/npc/retainer/ordinaryretainer.lua` | `3b458d8430d91dc358921b5e0ae35e879e6c1e502124a62a1af6a9211861d25a` |
| `widget/retainertradewidget.lua` | `247e6de3e9e86f19e5206f6110745228c96df80e9391aebeab2e4e16b8ebf278` |
| `widget/desktopwidget_connector.lua` | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `chara/charabaseclass.lua` | `101cafa86ebcb362b502e47dd33e262ede31b4939cc7fe945ab571e1afd0d3a9` |
| `group/communitygroup/retainergroup.lua` | `01ae225296a6324cb3e7dd29c5b6e141b16e8284fe471d15d88dac3770d717bd` |
| `widget/retainerlistwidget.lua` | `fe3e8dd4d738b61564443b6c1e06d28804d19ff6293eb49fb01f8e4a6c1d7ce8` |

## Package request

`RetainerBaseClass.updateRetainerItemPackage` calls `_updateItemPackage`
three times, in this order:

| Storage | Lua package | Wire/native package |
| --- | ---: | ---: |
| Ordinary items | `1` | `0x00` |
| Crystals and gil | `100` | `0x63` |
| Bazaar | `8` | `0x07` |

The direct calls are prototype `0.3`, PCs `8-16`, offsets
`0x33D-0x35D`, with constants `1,100,8`. Prototype `0.4`, the
`retryRequest` timer handler, repeats them. The first method limits requests
using `serverTime - lastUpdateItemPackage >= 1`, otherwise scheduling the
remaining interval if no retry timer exists. This is a request throttle,
not a timeout waiting for three replies. The code uses `_getServerTime`
units; no conversion to seconds is established here.

`OrdinaryRetainer.eventTalkRetainerItemTrade(1)` requests packages before
opening the widget; mode `2` requests them before selecting another result.
Prototype `0.6` and canonical Lua lines `286-325` establish these calls.
The Bazaar-only package does not supply the ordinary transfer rows.

The native route is `_updateItemPackage` at `0x006E79D0`, call
`0x006E7A22` to `0x006E5AD0`, conversion call `0x006E5B2F` to
`0x0078BC90`, then builder call `0x006E5B6C` to `0x0075EA50`.
`0x0078BC90` subtracts exactly one from the Lua package number.

The builder supplies opcode `0x0131` and internal message size `0x18`.
Its application extent is eight bytes after the internal 16-byte header:

| Application offset | Width | Consumer-visible construction |
| --- | ---: | --- |
| `+0x00` | u32 little-endian | Target actor identifier, copied at `0x0075EA6D/0x0075EA74`. |
| `+0x04` | u8 | Converted package, read at `0x0075EA78` and stored at `0x0075EA92`. |
| `+0x05..+0x07` | 3 bytes | No semantic write established in this builder. |

The package is not a dword merely because its native wrapper occupies one.
The caller supplies the target key from input context `+0x0C` at
`0x006E5B57/0x006E5B68`; `0x0075D460` separately prepares the local send
context. The request is not implicitly for the local player's package. The internal size
does not by itself prove a captured outer-frame or sub-event length.

## Silence versus a framed empty package

This comparison keeps the retainer actor, relation and connection valid and
omits only the package reply, without another update changing that package.

At this request boundary there is no response latch or yielding wait:
`0x006E5AD0` submits the packet and returns. The three RetainerBase calls
discard their return values and update the local request timestamp. Silence
therefore does not execute a refusal callback or populate an empty package.
Previously cached contents remain the available input. For an absent package,
`DesktopWidget.getItemPackageCapacityLocal` returns `0` after
`_hasItemPackage == false`; the retainer capacity getter delegates to it.
See connector lines `17868-17919`, original prototype `0.400`.

There is no separate package-refusal message in this comparison. A normal
zero-item load means `016D(mode=2),0146(actor,capacity,package),016E`, with
no item records between PackageStart and End. Mode `2` matters when replacing
an existing package: an empty delta clones and preserves its old items.

| Reply | Native package state after processing | Retainer widget consequence |
| --- | --- | --- |
| Silence, no prior package | No package is created; `_hasItemPackage` is false and the capacity helper reports `0`. | Initialization reads zero capacity and enumerates no rows. The traced opening path does not wait for a package acknowledgement or run a package-response timeout. |
| Silence, prior package exists | Cached package and rows are retained. | Old rows/capacity can remain displayed. No callback follows from silence. |
| Fresh framed empty load | A package exists with the advertised capacity and null item entries. | A widget initialized afterward enumerates an empty package. Capacity zero means zero slots; positive capacity means an empty container with that many slots. |
| Empty mode-2 replacement, preserving capacity | The old items are replaced; old occupied slots are marked dirty. | Their callbacks remove the old displayed rows after publication. |
| Empty replacement shrunk below an old occupied slot | Old dirty indices can be outside the replacement vector. | The traced publication path can raise its vector-subscript failure; this is not a safe empty-load recipe. |

The distinction is visible in the binary. `0x006F41A0` creates a missing
package at `0x006F4220-0x006F42BD`; constructor `0x006EF0D0` stores
capacity and initializes the slot vector with null entries through
`0x007240E0`. Zero capacity skips vector allocation at `0x0072411E-0x00724120`.
Publication `0x006F7880` installs even a package with no item records.
`_hasItemPackage` (`0x00708B60 -> 0x00702880 -> 0x006E5BA0`) tests
whether that package pointer exists, without requiring an item or nonzero
capacity. Capacity access at `0x007027BC` reads package `+8`.

For a mode-2 replacement, `0x006F43E1` calls `0x006F40E0` on the old
package. Its loop at `0x006F4114-0x006F418B` inserts occupied/resolvable
old slots into the dirty set through `0x007213F0`; null entries are skipped
at `0x006F4158-0x006F415A`. Before the ordinary callback,
`0x00703659` calls `0x006EE550` against the replacement package.
That helper checks each dirty index against the new vector: a null vector
at `0x006EE5B8-0x006EE5BA`, or an index beyond its extent at
`0x006EE5BC-0x006EE5C8`, reaches `0x006EE62E -> 0x005CF390`, the
`invalid vector<T> subscript` exception path. Replacing populated storage
with capacity zero is therefore unsafe on this static path. Preserve its
capacity when clearing items; this finding does not validate a general
capacity-shrinking protocol.

A first empty package has no old occupied slots and no item operations to
dirty new ones. The callback at
`0x00702E30` then has no nonempty dirty-slot set to deliver. A widget already
open with cached capacity zero is therefore not guaranteed to refresh merely
because a first empty package was installed. Publish the initial package
state before initializing the storage widget, or recreate/reinitialize its
lists afterward. Wire receipt alone does not establish either execution order.

Use the framed empty snapshot when the intended client-visible state is an
empty or unavailable container. It gives the client a definite package state
and clears old rows when replacement is intended. Silence does neither.
Advertise the real capacity for an available empty container, preserve the
existing capacity when clearing cached rows, and do not invent item records
as an acknowledgement. Zero capacity is suitable only for a first empty
package without old dirty slots in the traced case. This recommendation is an inference
from the consumer, not a recovered retail refusal policy.

Neither option is a reliable prohibition on transfer attempts. In particular,
the normal-item entrust branch of `canTransfer` checks whether
`getRetainerItem(1,retainerBagCapacity)` is nil and treats nil as space
(widget `0.31`, PCs `18-27`, `0x4B79-0x4B9D`). The connector validates
the slot range before native lookup and returns nil for slot zero
(`desktopwidget_connector.lua:13481-13540`). Thus zero advertised capacity
does not prove that every entrust attempt is disabled. No crash, disconnect,
or automatic close is established for this well-formed empty reply. There is
no controlled live replay of these cases in this finding.

There is an explicit *transfer* result consumer:
`OrdinaryRetainer.eventReturnResult(code, result)` forwards codes `31/32`
to `DesktopWidget.noticeRetainerTradeResult`, then to
`RetainerTradeWidget.noticeItemTransferResult`. Original OrdinaryRetainer
prototype `0.9`, PCs `0-20` at `0x198B-0x19DB`, and widget prototype
`0.52`, PCs `0-12` at `0x73B8-0x73E8`, prove this behavior:

- Every notice records `_getServerTime()` in `noticeGetTime`.
- When the second argument is boolean `true`, both update waits retain
  their existing values.
- Otherwise both waits are cleared. This does not apply any inventory data.

Numeric `1` is not boolean `true` in this comparison. This known refusal
behavior belongs to the transfer event. An empty package load does not clear
both transfer waits as this false notice does.
The generic s2c named-function consumer can deliver such a method through
`0x0130`; its decoder at `0x0076C220` and argument/dependency limits are
documented in [StartServerOrderEventFunction](../event/start-event-fn-receiver.md).
No retainer-specific captured invocation is established.

## Transfer result values and widths

`RetainerTradeWidget.getAskResult` returns `(code, package, slot, count)`.
When `canTransfer` returns true it sets both `waitplayerupdate` and
`waitretainerupdate` before returning. Widget prototype `0.50`, PCs
`22-34` (`0x717E-0x71AE`) and `51-63` (`0x71F2-0x7222`) prove the
two branches.

| Code | Source item owner | Ordinary tuple | Money-edit tuple |
| --- | --- | --- | --- |
| `31` | Retainer, widget owner `4` | `(31, retainerItemPackage, retainerItem, chosenCount)` | `(31,100,retainerMoneyIndex,chosenCount)` |
| `32` | Player, widget owner `1` | `(32, playerItemPackage, playerItem, chosenCount)` | `(32,100,playerMoneyIndex,chosenCount)` |

The source ownership is independently fixed by OrdinaryRetainer prototype
`0.6`: PC `75` (`0xA1A`) resolves `_getItem` on self for `31`; PC `100`
(`0xA7E`) resolves it on the player for `32`. After liveness checks it
returns `(code, itemObject, package, count, catalog, ...nameIndex)`.
The numeric slot has become an item object at this boundary.

Keep three different representations separate:

| Representation | Slot | Package | Count |
| --- | --- | --- | --- |
| Widget work declaration | `integer32` for chosen/player/retainer item indices | `integer8` | `integer32` |
| Widget's local Lua result | Lua number | Lua number | Lua number |
| Generic typed serialization of the actor caller's return | Item reference contains native u16 slot | Item reference contains native u8 package; the separate numeric package operand uses tag `00` plus four bytes | Ordinary integral operand uses tag `00` plus four bytes |

Widget initialization prototype `0.1`, PCs `3-17` and `35-49`, offsets
`0x878-0x8B0` and `0x8F8-0x930`, declares the work widths.
Money indices are separately declared `integer16`; their use does not narrow
the ordinary item-index fields or the numeric Lua arguments.

Native item serialization at `0x006F5260` gathers the resolved item owner,
package and slot and calls `0x00790BA0` at `0x006F5307`.
The single-item writer `0x00791870` reads package with a byte load at
`0x00791875` and slot with a word load at `0x00791878`, producing
`07 binding:u32be slot:u16be package:u8`. The dword is the resolved owner
binding reference; this serializer alone does not prove equality to a raw
actor ID. The ordinary integer writer
`0x007915E0` emits tag `00` and four bytes. Lua/native package and slot
conversion subtracts one; the return callback conversion adds one.
These serializer facts do not establish a fixed retainer packet struct or
an observed event envelope. The actual typed argument stream must be decoded;
it is not a packed `(u8 package,u16 slot,u32 count)` tuple at every layer.

The generic EventUpdate continuation independently confirms the numeric
width: `0x00894AB0` prepares a 64-byte typed area and calls
`0x00790CE0` at `0x00894B44`. Its internal-tag-0 branch writes wire tag
`00` at `0x00790DC1` and four value bytes at `0x00790DE4-0x00790DF5`.
The send route is `0x00894B65 -> 0x00894090 -> 0x0075E670`, which
sets opcode `0x012E` at `0x0075E700` and copies that area at
`0x0075E723`. This is a generic continuation layout, not a captured retainer
request with fixed argument offsets.

The incoming full item record also has a u16 slot at record `+0x10`
and a four-byte quantity at `+0x08`: decoder `0x0076B760` loads them at
`0x0076B7AE` and `0x0076B79C`. A single removal reads its u16 slot at
`0x00769099`. These fields belong to the package-update records, rather than
the widget's Lua result arguments.

## Refresh and packet ordering

The native queue preserves item-command order. The client-compatible
publication dependency, for every affected actor/package, is:

```text
0x016D UpdateStart(mode)
  0x0146 PackageStart(actor:u32, capacity:u16, package:u8)
    0x0148..0x014C full item records and/or 0x0152..0x0156 removals
  [next 0x0146 and its records, if applicable]
0x016E UpdateEnd
```

These payload fields are native little-endian scalars. In the `0x0146`
decoder, `0x0076BC4C/0x0076BC4E/0x0076BC52` read the u32/u16/u8 fields.
Execution `0x0089B3F0 -> 0x006F41A0` selects the destination package.
`0x006DD8C0` retains the update mode. Existing-package mode `2` creates
fresh staged storage; the other branch clones existing storage. Do not use
mode `2` as a delta expecting unmentioned items to survive.

End execution reaches `0x00703700`. Its clear-defer branch at
`0x007037B6-0x007037C8` calls `0x006F7880`, then `0x007034E0`, then
marks publication complete. A set defer flag instead queues a continuation.
Receiving `0x016E` alone is not proof that publication has already executed.
`0x00702E30` fires `_onUpdateItemPackage` only for a nonempty dirty-slot
set, converts package and slots back to Lua indices at
`0x00702F4F/0x00702FD5`, and dispatches at `0x00703045`.
The shared native record owns the full queue trace and linked-package limits.
`0x0147` has no required publication role established here and cannot
replace `0x016E`.

Lua forwards that callback through
`CharaBaseClass._onUpdateItemPackage -> DesktopWidget.processUpdateItemInformation
-> updateRetainerListWidget -> updateActorWidget`. Connector prototype
`0.94`, PCs `5-17` (`0x11757-0x11787`), sets the retainer callback count
with `(0,numberOfSlots)` and then calls `updateItemList(package,slot)` for
each slot. Its player branch and prototype `0.97` do the same through
`updatePlayerItem`. The open widget rebuilds its normal/crystal/money
lists when the corresponding counter reaches zero.

The wait-release conditions are exact and actor-specific:

| Result | Retainer callback releases `waitretainerupdate` | Player callback releases `waitplayerupdate` |
| --- | --- | --- |
| `31` | Package and slot both equal the selected retainer source. | Package equals `retainerItemPackage`. |
| `32` | Package equals `playerItemPackage`. | Package and slot both equal the selected player source. |

Widget `0.47`, PCs `7-30` (`0x6686-0x66E2`), and `0.48`, PCs
`7-30` (`0x6B55-0x6BB1`), establish these comparisons. The UI selection
handler tests these waits before accepting another transfer
(`retainertradewidget.lua:2724-2759`). A callback for another actor/package
or an empty dirty-slot set does not satisfy the selected source test.

For a successful normal transfer, publish the changed player bag and retainer
package `0`; for crystals/gil, publish each affected package `99`.
Include the source slot's new record or removal and the destination's new or
changed record. Bazaar package `7` is a separately requested display package;
it is not a substitute for either physical transfer update.

The traced widget imposes no success-notice-before-update or
update-before-success-notice requirement. Its waits are set before the client
returns the selection, and boolean-true `eventReturnResult` leaves them
unchanged. Either callback order can satisfy the two independent conditions.
Keep the widget and retainer relation alive, publish both changes after that
selection, and preserve the native framing above. Sending the success notice
before the framed updates is one compatible sender order, not a recovered
retail requirement. Sending only a notice leaves the update waits pending;
the traced item consumers do not themselves resume the generic event sequence.
No retail trace or controlled live replay establishes the scheduler timing
between the event queue and item-command queue for this operation.

## Mid-session roster publication

The live roster has a refresh route without a relog check. A clean second
complete group publication after hire/release remains unproved. These are
different claims: the widget can reread an updated live group, but that does
not establish how the server replaces or recreates the native group.

`RetainerGroup._onUpdateMemberInformation` and `_onUpdateWork` call
`DesktopWidget.processUpdateGroupInformation`. Original prototypes `0.1`,
PCs `20-25` (`0x504-0x518`), and `0.4`, PCs `5-11`
(`0x779-0x791`), establish those calls. The connector tests group kind
`80001`, obtains the open `RetainerListWidget`, and calls its
`updateRetainerData`: prototype `0.84`, PC `119` at `0x108CA` compares
`80001`, PC `123` at `0x108DA` loads the widget name, and PCs `127-128`
at `0x108EA-0x108EE` invoke the refresh. This route is usable during a
session; it is not confined to widget initialization.

The refresh reads the current count and six current member values through
`MyPlayer._getGroup(80001)`. It treats an absent count as zero, skips
employment state `127`, rewrites rows, and hides unused rows through row
eight. RetainerListWidget prototype `0.2`, PC `2` (`0x959`), PC `14`
(`0x989`), PC `15` (`0x98D`), PC `25` (`0x9B5`), and PCs `28-39`
(`0x9C1-0x9ED`) establish those reads and row operations. Connector
prototypes `0.384` and `0.385` resolve group `80001` at
`0x25105/0x25109` and `0x251CB/0x251CF`; the six getters are at
`0x251E3,0x251F3,0x25203,0x25213,0x25223,0x25233`.
Their field meanings and canonical Lua locators are recorded in
`xivl-client-scripts:docs/bazaar-retainer-client-contracts.md`, section
"In-game retainer roster source". Native `_countMember` at `0x006FFB90` reads the
current range through `0x006C02A0`; member getters at
`0x007000A0/0x00700110` use `0x006C0D70/0x006C0D80`. None of these
inspected bodies has a login predicate.

The native group loader also has no unconditional one-shot rejection of a
later header. In the pinned PE, s2c `0x017C` consumer `0x006CC620`
guards its initial setup with `this+0x24 == 0` and input tag
`packet+0x30 == 0x2711`. An already-initialized header branches at
`0x006CC65C` directly to `0x006CC6E3` and calls `0x006CC070`.
The first setup path alone tests record byte `+0x0A` at
`0x006CC6DD-0x006CC6E1` before its immediate call; later headers bypass
that test. Its higher-level meaning is unresolved. Both record
branches in `0x006CC070` append through `0x007238B0` and store the new
record pointer through keyed-slot helper `0x006D75F0`. The former grows
and increments the append-container count; the latter searches a three-dword key and
returns the node's `+0x20` value slot. Equal keys can therefore rebind the
keyed value while the append container retains entries. This is not proof of
atomic roster replacement or removal of released members.

The general group sequence has separate header, member-begin, member-batch,
member-end and property-sync consumers. `0x006CDE30` processes two
subdecoders and calls completion through `0x006CDA80` only after both
finish. Property sync `0x017A` is gated by an active lookup:
`0x006C96B0 -> 0x006C1510`; null exits before application. Its fields
cannot alone bootstrap an absent context. The complete native routes are
canonical in [Group and SharedWork](../event/group-shared-work-system.md)
and [Sync writer](sync-writer.md).

Thus the consumer supports refreshing a valid live `80001` group without
relogging. It does not justify requiring a relog, nor does it prove that
blindly replaying `017C,017D,017F..0182,017E` is a clean hire/release
republish. The wire-group-ID-to-Lua-`80001` join, context teardown/rebinding,
initial-setup state, and a same-session second-sequence witness are missing.
Acceptance of that exact server operation remains unknown.

## Capture witnesses and unknowns

`xivl-captures:derived/observations.json`, `inner_opcodes.c2s`, has no
`0x0131` entry across its 54-capture inventory. No retainer package-request,
silence-versus-empty comparison, hire/release roster refresh, or result-31/32
exchange is established by that inventory. Clientbound `0x0131` is a
different, generic EndEvent route and must not be counted as a request witness.

The shared framing does have a related retail witness:
`buy_square_maple_shield.pcapng`, SHA-256
`aab582bd9658c89cd4d56e132500481f442b9f2dc2179f1a5829ea09776bce6b`,
main s2c frame `37`, stream sub-event indices `120-127`:
`016D,0146,0148,0146,0148,0147,0147,016E`.
Frame `38`, index `129`, then carries generic `0130`. This is a shop
transaction witness, not evidence of retainer notice ordering.
`xivl-captures:tools/validate_framing.py` reported no trailing bytes or
compression-flag mismatches. Its `tools/extractors/extract_wire_order.py`
with `--direction s2c --around 0x016d --before 2 --after 30` supplied the
locators. Source identity is canonical in
`xivl-captures:sources/pcap-1.23b/manifest.yaml`.

Unresolved: retail capacities and a live silence-versus-empty comparison;
the exact retainer event receipt sequence and cross-queue timing; clean
hire/release group replacement and its wire-to-`80001` mapping; server
authorization, atomicity and persistence. The client-side representation and
publication dependencies do not establish those server policies.

## Reproduction

Use the binary identity above with the existing Ghidra read-only exporter
recipe in `xivl-client-structs:ghidra/README.md#dumpvasjava`, selecting these
additional empty-package consumer entries:

```text
0x006F41A0,0x006EF0D0,0x007240E0,0x006F40E0,0x006F7880,0x006EE550,
0x00702880,0x006E5BA0,0x00702740,0x005CF390
```

For an original-file instruction check, read an explicit bounded range from
the section-mapped PE and decode it with the tool versions above. For example,
`Binary` is the identified local executable, not an analysis database:

```python
import hashlib
from pathlib import Path

import pefile
from capstone import CS_ARCH_X86, CS_MODE_32, Cs

data = Path(Binary).read_bytes()
assert hashlib.sha256(data).hexdigest() == (
    "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
)
pe = pefile.PE(data=data)
va, size = 0x006EE550, 237
offset = pe.get_offset_from_rva(va - pe.OPTIONAL_HEADER.ImageBase)
for insn in Cs(CS_ARCH_X86, CS_MODE_32).disasm(data[offset:offset + size], va):
    print(hex(insn.address), insn.mnemonic, insn.op_str)
```

Keep generated listings local. For the Lua checks, select each coverage row's
original `resourcePath`, verify its `sha256`, decode it, verify
`wrapper.decodedPayloadSha256`, and inspect the prototypes/PCs cited above.
The named corpus reader records each prototype's code offset and instruction
words, so the locators do not depend on the decompiled text's variable names.
