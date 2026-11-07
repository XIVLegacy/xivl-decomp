# Cut actor-class appearance requests

A nonzero class word from `RaptureCharaActorClassIdClip` becomes a queued
appearance-template request for the selected Scene actor ID. The main-thread
consumer resolves an `actorclass_graphic` row, posts its 29-dword appearance
record back to the Scene, and stages it on the actor resolved by that ID.
The concrete mutation is an in-place appearance-bank write. Delivery, object
retention, row resolution and the later load/apply conditions remain inputs.

## Evidence and interpretation

Source: retail FFXIV 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
PE32 x86, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses are VAs. Original file-backed instructions and cells
were checked with Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 in
x86-32 mode. Retained instruction listings were authenticated against the
original bytes; enclosing gaps were not counted as instructions.

The [clip and integer-parameter finding](cut-caption-actor-bindings.md#actor-class-clip)
owns actor-ordinal selection, the typed context and the class-value input.
The selected `man50250` clip addresses actor 5 and integer index 1. Its
record and actor 9's separate dictionary entry do not select the same target.
The contracts below require valid storage, normal ABI/returns, the named
tables, retained pointers/IDs and no harmful intervening mutation. They do
not establish actual callback reachability or rendered appearance.

## Scene target and emitted request

Delegate `0x00613960` rejects class word `0xFFFFFFFF` or a null first
explicit actor pointer, then calls `0x006121A0` with that actor as receiver
and `(constructedName,classWord)` (`0x00613960..0x00613981`). Let M be
this CutManagerActor. `0x006121AB..0x006121BD` obtains the name bytes
through `0x00445210` and looks up the name through `0x007D2930` on
`u32(M+0x118)+0xA4`.

That lookup walks the actor list and compares the supplied bytes with each
actor's virtual slot-13 name, byte offset `0x34`. The comparison is exact
and case-sensitive; the first match returns its actor pointer T, and exhaustion
returns zero (`0x007D2930..0x007D29C5`). A dictionary label, actor ordinal,
runtime actor ID and constructed lookup name have distinct roles. The
[ProxyActor name contract](cut-proxy-actor-names.md) joins the selected
`man50250` record to its adjusted getter and copied lookup bytes.

For nonzero T, `0x006121D1` calls `0x0060F130` with
`(T,constructedName,classWord)`. The latter forms this 12-byte Q and calls
`0x007CE7B0(M,7,Q,0xC)` at `0x0060F172`:

| Q offset | Source | Use |
| --- | --- | --- |
| `0x00` | `u32(T+0xDC)` at `0x0060F14E` | Destination Scene actor ID |
| `0x04` | Decimal-converted name at `0x0060F149/15B` | Selector for the zero-class alternatives |
| `0x08` | Original class word at `0x0060F154/16E` | Appearance row key on the nonzero-class path |

Name conversion uses the `_atol` thunk `0x009D6FDD -> 0x009D6FCC`;
the latter supplies base 10 and a null end pointer to `0x009D8B23`.
The [local transport](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport)
owns the element-key lookup, selector adjustment and payload copy. A normal
producer return does not acknowledge receipt or appearance application.

After the request call, `0x006121A0` still returns T. If T is zero it skips
the request and Scene flag changes below. It separately converts the name;
a result of 10000 stores T at M+`0x220`, even when T is zero
(`0x00612209..0x00612222`). That field store is distinct from the class word.

## Immediate Scene bookkeeping

For the nominal CharaActor table `0x00FC0D34`, the calls after
`0x0060F130` affect T's `+0x2B70` flags as follows:

| Operation | Original cell / target | Effect |
| --- | --- | --- |
| Slot 150, `+0x258` | `0x00FC0F8C` / `0x006694E0` | True only when bit `0x20` is set and the low nibble is zero |
| If true, slot 172 with 1 | `0x00FC0FE4` / `0x00669510` | Sets bit `0x400` |
| Slot 149 with 0 | `0x00FC0F88` / `0x006694C0` | Clears bit `0x20` |
| `0x0065DA00` | Direct call at `0x00612204` | Sets bit `0x40000` on its first admitted pass and requests appearance-store removal by T's ID |
| Its slot 182 with 0 | `0x00FC100C` / `0x00669600` | Clears bit `0x20000` |

The removal call is `0x007D1C60` on `u32(T+0x118)+0xD8`; it forwards
T+`0xDC` to `0x007D1BE0`. A matching store entry is cleared by writing
`0xC0000000` at entry+8 and zero at entry+4. A missing entry has no such
write. The [existing appearance-store contract](attribute-resource-and-scene-binding.md#group-flag-setters)
owns its loading implications. These Scene-side changes follow the producer
call, without waiting for its queued request's success.

## Main-thread class queue

The nominal CutManagerElement table `0x00FA4DB8` selects event handler
`0x00566740` at slot 8. Selector 7 uses table cell `0x00566860`,
target `0x005667A5`, which passes Q to `0x005656E0` at
`0x005667AA`. The arm has no selected local payload-length check.

For nonzero Q+8, `0x005656EC` bypasses the name alternatives and appends
the request to the pointer ring at element E+`0xB4` through `0x00569280`.
That append copies all three words into a 12-byte record and increments the
count. Its allocation/growth helpers must complete normally and leave valid
stored records. Q+4 does not select this branch's target or row.

The table's slot 6 selects drain `0x00564D80`. It takes the front record,
initializes a local appearance block, and supplies Q+8 as row key to
`0x0055D2B0` (`0x00564E62..0x00564E6D`). The
[catalog resolver](../actor/item-appearance-boundary.md#catalog-resolution-result)
lazily opens `actorclass_graphic` and builds its 29-dword result. A null
sheet handle or unresolved row returns AL=0. Only AL==1 at `0x00564E72`
permits the send/pop path; another result leaves the front request queued.

After successful resolution, Q+4 equal to 12000 also selects
`0x00564E7A..0x00564EA5`: call `0x006B6F40` with the first appearance
dword, pass whether its result equals 2 to `0x004D7470` on
`u32(E+0x7C)`, then call `0x00575520` on that call's returned receiver.
These callees' wider effects remain opaque here; their normal returns are
required before the following packet construction.

The accepted path calls `0x004EC080` with
`(Q0,0x1D,appearance,0x74)` at `0x00564EBD`, then posts through
`0x004D6780` at `0x00564ED3`. The record is copied inline because
`0x74 <= 0x78`. The drain advances its head and decrements its count after
the post call returns. It does not wait for Scene receipt or model loading.
The selected channel at E+`0x84`, its slot 3 and eventual delivery remain
conditions of the [published ScenePacket route](../actor/item-appearance-boundary.md#native-consumption-chain).

## Resolved actor appearance

Scene consumer `0x007C93C0` resolves Q0 through its actor registries.
With a nonnull selected actor A, type `0x1D` becomes selector 8 and reaches
its virtual slot 157, byte offset `0x274` (`0x007C95D3..0x007C95F2`).
Preserving T's ID-to-object association is required to identify A with the
earlier T; equal IDs alone do not exclude removal or reuse between queues.

The CharaActor cell `0x00FC0FA8` selects `0x00662D30`. Its slot-154
predicate must return zero. Selector byte `0x00663C10` is 7, selecting
cell `0x00663B04 -> 0x00663808`, which calls `0x006623F0` on A.
That function copies exactly 29 dwords to A+`0x13C8`, then calls
`0x0065D730` on A. The latter backfills previously zero base-bank fields
and sets the low nibble of A+`0x2B70` to phase 1.

The [bank submit/readiness/apply contract](../actor/appearance-dirty-apply.md#event-to-renderer-handoff)
owns the continuation through `0x00666720` and `0x00665E40`.
An admitted phase countdown offers the requested bank to the appearance
store. A missing entry can use the synchronous apply fallback; a retained
entry requires later readiness. Resource batches and activation remain
separate from staging. The inspected route establishes writes to A's
appearance banks, not a completed load or visible model.

## Zero-class alternatives

Q+8 equal to zero selects these alternatives in `0x005656E0`:

| Decimal name word Q+4 | Selected request |
| --- | --- |
| 10000 | Store Q0 at E+`0x11C`; `0x0055FB90` requests appearance from the nonnull source at `u32(u32(E+0x7C)+0x17838)` |
| 10001 through 10066 | `0x00565410` uses index Q4-10001; success returns directly, while failure writes fallback row key `0x00632EB4` into Q8 and appends |
| 12000 | `0x005637F0` uses the `0x004D7430` predicate to select a special fallback or a source-copy path. The latter sends only when `0x00563750` returns a nonnull source; null returns without sending. |
| Other values | Append with row key zero |

The first branch's TEST/JA sequence admits exactly 10000, not a range below
it. These alternative sources do not establish an actual `man50250`
argument. The [ProxyActor name contract](cut-proxy-actor-names.md) supplies
name word 12000 for actor 5 under its formatting and retention conditions.
For name word 12000, the null-source exit is `0x00563872 -> 0x005638F2`,
before the shared packet construction at `0x0056389C`.

## Reproduction and limits

Authenticate the PE above, decode the delegate/lookup/request entries
`0x00613960`, `0x006121A0`, `0x0060F130`, `0x007D2930`, the named
flag helpers, selector consumer/drain and appearance entries, and compare
the original cells and selector byte. The existing catalog and bank pages
own their full layouts and continuation. Calls not separately inspected
remain opaque; normal returns and preserved storage are conditions.

A fresh read-only Ghidra 12.1.3/JDK 21 run used `DecompileToText.java`,
SHA-256 `83a31add99e86bea0128470e4cc52b193e373ebc342cf060861bd3c17339668a`,
for the delegate, lookup, producer, consumer/drain, row resolver, packet
dispatch, Scene bookkeeping and bank-stage/apply entries. All 25 selected
decompilations completed without analysis timeout. No target execution
or runtime observation was used. Exact supplied values, lookup success,
allocation, queue delivery, current table/object identity and completed
resource loading remain unobserved.
