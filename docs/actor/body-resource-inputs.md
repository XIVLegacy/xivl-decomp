# Scene body resource inputs to ModelObject

The pinned Scene CharaActor/CharaVisual route conditionally joins appearance
selection, loaded resource records and ModelObject slot 1. On completion state
3, the exact packed pointer list and maintained pointer count reach that slot
as `list, count, 1, layer, 0, 0`. The successful initializer's zero enable
argument clears Drawable mask 1 before registration. These native edges are
PROVEN within the static and synthetic scope below. Admission of the intended
scene actor into the [zero-override body profile](body-shader-constant-join.md)
remains INCOMPLETE.

The first missing identity edge is the intended scene actor's actual appearance
and resolved request/root association with the pinned body bytes. The list's
count consistency, successful loading, subsequent draw enable, active mesh/pass
and stable zero override also remain conditions before submission. A formatted
path and a completed request list do not establish those conditions.

| Contract | Verdict | Boundary |
| --- | --- | --- |
| Synthetic appearance to top bank 5 | PROVEN | B=1 and nonzero dword 14 with bits 19:10=3; successful resolver assumed |
| Completion result to packed pointer domain | PROVEN conditionally | Module-returned resource root, not request handle or identified model child |
| Visual list/count to Model slot 1 | PROVEN for pinned state-3 route | Pointers and maintained count forwarded unchanged; count is not recomputed |
| Body initializer enable argument | PROVEN zero on this route | Mask-1 collection needs a subsequent enable transition |
| Intended scene body/profile admission before submission | INCOMPLETE | Actual resource identity, count invariant, current flags/pass, profile and lifetime |

## Identity and method

All native locators are VAs in retail `ffxivgame.exe` 1.23b, build
`2012.09.19.0001`, x86, base `0x00400000`, size 15996808, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.
The installed body file and first SHD/program identities remain canonical in
the [profile identity](body-shader-constant-join.md#profile-and-identity).
Installed bytes are not evidence of a historical actor's loaded bytes.

Ghidra flat assembly supplied search leads. Fresh pefile 2024.8.26 and
Capstone 5.0.7 comparisons checked 2742 exported instructions in 34 request/
resource functions and 488 in the visual/actor bridge, with overlapping
functions. These checks do not establish whole-function export coverage or
all indirect callers. The initializer's unexported fallthrough was decoded
directly from the PE, as in the [ownership trace](body-mesh-pass-admission.md).

The [focused verifier](../../tools/verify_visual_model_inputs.py) pins the PE,
checks selected instruction/table/string anchors and executes separate native
fragments with Unicorn 2.1.4. No client process or live actor was observed.

## Appearance, request and published root

Let S be a Scene CharaActor with table `0x00FC0D34`, V its CharaVisual with
table `0x00FD3ED4`, and M its ModelObject. The established
[owner producers](body-mesh-pass-admission.md#ownership-before-registration)
bind V+8=S, S+0x2B5C=V and S+0x0C=M. Identifying the intended S through
the [scene lookup](window-actor-anchor-producer.md#target-production) remains
an input, not a consequence of these offsets.

`0x00665EF0..0x00665F0D` copies 29 appearance dwords and calls visual
slot 25=`0x006B7840`. That method copies appearance dword 0 to V+0x5C
at `0x006B7898`, and dword 14 to V+0x9C at `0x006B78F8/0x006B78FB`.
Its base split at `0x006B7976..0x006B79CE` maps B=1 to category 0,
number 1; slot 22=`0x00630850` formats `/client/chara/pc/c001/`.
The equipment loop at `0x006B8AD4..0x006B8AF7` supplies V+0x98+4*s
and index 2+s to `0x006B5E40`. For s=1, index 3 is top.

The real lane helper `0x006306A0`, called at `0x006B5ECA`, extracts
`E=(word>>10)&0x3FF`. Tables `0x012BF2A8/0x012BF2C8` select `equ/e`
and `top_mdl/`; formatting at `0x006B5F1E/0x006B5F54` yields
`/client/chara/pc/c001/equ/e003/top_mdl/0001` when E=3 and the word
is nonzero. Bits 31:30 and 9:0 do not alter this model path. A zero word
skips this request. On resolver success, `0x006B5FF5..0x006B6013`
supplies bank 5 and flag 2 to `0x006320C0`.

That string is the resolver input. `0x00D392A9` forwards it, global root
`0x0132CB98`, suffix `.bin` and flag zero to `0x00D38E50`. Nonzero
byte `0x01266B64` selects an identity-copy/existence-check branch at
`0x00D38EC6/0x00D38EE6`. The other branch enumerates wildcard candidates
through FindFirstFileW/FindNextFileW at `0x00D39022/0x00D39152` and can
replace the output at `0x00D391B0`. Neither mode nor returned path for an
actual actor was observed.

The [load-record contract](appearance-resource-slots.md#load-record) owns the
field layout and request semantics. In particular, `0x00445DB0` tests string
inequality: equal stored/incoming paths skip at `0x006320E1`. A changed
path replaces the pending request without clearing the old loaded root in
that prefix. Pending record+0x58 and published record+0x5C are distinct.

On the ordinary callback branch (`0x01376038==0`), `0x00631C70` requires
nonnull request slot-6 data at `0x00631D65..0x00631D74`. Visual slot
4=`0x00632760` calls `0x006324E0`; flag 2 selects owner slot
103=`0x0061FF60`, returning [S+0x120]. At `0x00631DEB`, the module
at [S+0x114]+8 receives that object, request data, request slot-10 result
and path/name inputs through its vtable+0x58 factory. The nonnull check at
`0x00631DED/0x00631DEF` guards passing its result unchanged to visual
slot 6=`0x00632440` at `0x00631DFE`.

For a nonnull result, `0x00632471` retains through the owner, and
`0x00632489` releases a prior root when present. `0x0063249B` publishes
the result unchanged. The module's concrete factory and actual returned
resource bytes remain unjoined. Downstream `0x00C52532..0x00C52539`
supplies each root to `0x00C52260`, then `0x00A44BF0` traverses its
child/sibling links. The published root is not itself proved to be the WRB
ModelResourceNode or a Drawable, and one root can contain multiple models.

## Packing, count and exact Model call

Construction at `0x006B7636 -> 0x00631EF0` supplies capacity 0x39 (57).
The constructor stores capacity at V+0x14, records at V+0x18, initializes
V+0x1C to zero and allocates four bytes per capacity entry at V+0x24.

```text
V+18 -> 57 records, stride 6C
  record+58: pending request handle
  record+5C: module-returned resource root
    631320, or ready 6323D0: pack nonnull roots in record order
      V+24 -> DWORD pointer array; V+1C -> maintained count
      visual slot12 6315C0 -> slot13 6315D0 -> slot16 6BE3D0
        completion state 3 -> 6BB080 prefix
          [V+8] actor slot108 65B9F0 -> 7CD5B0 -> A5F6B0
            [S+C] Model slot1 A61DF0(list, count, 1, layer, 0, 0)
              C510F0 -> resource finder -> C502B0
```

`0x00631330..0x0063134D` and `0x00632400..0x0063241D` copy every
nonnull record+0x5C into successive DWORD cells without conversion or
deduplication. The latter compactor first requires all record+0x58 values
zero; otherwise `0x00632436` sets V+0x20=1 and defers. A completion at
`0x0063249E..0x006324C7` can clear that marker and call `0x00631320`
after the same pending-handle scan. This permits omitted null roots; it does
not require every bank to publish a resource or prove successful body loading.

The count is read from V+0x1C at `0x00631351/0x00632420`, not derived
from the compactor's cursor. A nonnull result replacing a null root increments
it at `0x00632497`; clearing at `0x00632390..0x006323AE` resets and
recounts retained roots. The null-result branch at `0x00632453/0x00632455`
skips to the write at `0x0063249B` without decrementing it or releasing
the prior root. The callback's separate failure branch checks its first
argument at `0x00631C84`: zero reaches slot 3 at `0x00631CAE` to clear
the pending handle, then slot 6 with an explicit null result at `0x00631CBE`.
With an old root and count one, native replay leaves root zero and count one.
The success-path factory null guard above does not guard this failure branch.
Whether an actual changed request fails with an old root retained is unobserved;
the native path admits that conditional transition. Correct count equals the
number of nonnull roots therefore still needs a producer/caller invariant.
Compaction and Model dispatch do not repair or independently admit an
inconsistent count. With V+0x20=1, the same synthetic failure transition runs
the real pending scan and compactor and reaches Model slot 1 with count one
after packing no roots. This is not proof of a live failure or of how the
downstream loader handles that input.

Slot 12=`0x006315C0` sets V+0x35 bit 0, then slot 13=`0x006315D0`
requires readiness slot 14 and equality of V+0x35/V+0x34. The pinned slot
14=`0x00630930` returns true. Slot 13 passes V+8, array, count, V+0x30
and V+0x38 to slot 16=`0x006BE3D0`. `0x00632540` owns this completion
mask/state-pointer protocol; state 3 reaches `0x006BE445 -> 0x006BB080`.
The latter's `0x006BB125..0x006BB135` reads S afresh from V+8 and
invokes actor slot 108 with the unchanged list/count. This load precedes its
later material/node adjustment and guards.

Slot 108=`0x0065B9F0` marks actor+0x2B70 bit 0x10 through slot 170,
then calls `0x007CD5B0 -> 0x00A5F6B0`. Slot 104=`0x0065B950`
obtains the layer container through S+0x114 slot 6 and looks up S+0x2B79
through `0x00C269F0`; it maps -1 to zero. Its ordinary RET leaves the zero
pushed at `0x007CD5BB` on the stack as wrapper argument 4. It becomes
initializer argument 5, the enable argument. No fixed layer is established.

`0x00A5F6B0` releases optional S+0x18 and invokes actor slot 20 before
reading M=[S+0x0C] at `0x00A5F6D7`. The call at `0x00A5F6F3` receives:

| Model argument | Native source |
| --- | --- |
| 1, list | Exact V+0x24 array pointer |
| 2, count | Exact V+0x1C pointer-entry count |
| 3, loader flag | Constant 1 |
| 4, layer | Actor slot 104 result |
| 5, enable | Retained constant 0 from `0x007CD5BB` |
| 6 | Constant 0 at `0x00A5F6DF` |

Both ModelObject table `0x0109CE2C` and RaptureModelObject table
`0x010653A4` have slot 1=`0x00A61DF0`. Argument 3=1 selects
`0x00C510F0` at `0x00A61FA5` and hence `0x00C502B0` after resource
finding, even with one top-level pointer. Loader success and the exact model
descendants remain conditions. The replay stops before this loader executes.

## Enable and pre-submission admission

On the successful initializer path, `0x00A62207` sets M+0x29C bits 1/2.
The zero fifth argument reaches Model slot 27(0) at `0x00A62222`, then
`0x00A62224` clears model bit 2. The common slot 27(1) call at
`0x00A62234` only saves model bit 3 once bit 2 is clear. Native slot
27=`0x008DE860` and getter slot 82=`0x006576B0` therefore leave loaded
D+0xA0 bit 0 clear before the ownership copy at `0x00A62246` and
registration at `0x00A622A6`. A positive-control fifth argument of one
leaves that bit set when D+0xA0 bit 8 is clear.

The [mask-1 collection](body-mesh-pass-admission.md#reached-pass-and-context)
therefore requires a subsequent enable transition for this newly initialized
Drawable. The zero argument alone does not prove an entire actor group is
quiescent, or exclude other phases, callbacks or existing attachments. The
later enable writer/current flags and reached layer/bucket remain unjoined.

If this root actually loads the pinned body and normal material construction
selects the [matching mesh](body-mesh-pass-admission.md#exact-body-mesh-and-material),
the established material/default/driver register profile applies under its
own conditions. It still needs the first matching live material, active tags,
valid mutable indices and stable context+0xBA4=0. The
[native draw prefix](body-mesh-pass-admission.md#admission-before-the-first-request)
does not enforce those predicates before `0x00C44F07` requests c0..c15.
This trace supplies no complete group preflight/fallback, successor-isolation
or lifetime barrier.

## Reproduction and limits

Run from the repository with pefile, Capstone and Unicorn 2.1.4 installed:

```powershell
python tools/verify_visual_model_inputs.py --exe <pinned-ffxivgame.exe> --output <new-result.json>
```

The checker rejects optimized Python and existing output paths. Its five
selector cases execute `0x006B5E40` and the real lane helper, with prefix,
String runtime, formatter, successful identity-copy resolver and cookie stubs.
They include zero/no request and E=4 controls. Five real `0x00445DB0`
comparisons distinguish equality from emptiness. Three native `0x00632440`
cases check first publication, replacement and null replacement with mock
owner retain/release and V+0x20=0.

Two additional cases execute the real `0x00631C70` callback failure prefix
and real visual slots 3/6 with no stubs. They start with a pending request,
either a null or nonnull old root and its matching initial count, and
V+0x20=0. Both clear the pending handle and root; the nonnull-old-root case
retains count one. Execution stops at `0x00631CC0` before the request's
follow-up call. This is a conditional failure transition, not evidence of an
actual request failure or execution through later list completion.
One combined failure case sets V+0x20=1 with 57 records, only bank 5 pending
and a previous bank-5 root/count one. It executes the failure callback, real
slots 3/6, readiness scan, compactor and bridge to actual Model slot 1 with
no roots packed and count one. Its bridge uses the same three stubs below;
it stops before loading or the callback's later request follow-up.

Twenty forwarding cases execute both compactors, native visual slots,
state-3 dispatcher, initial `0x006BB080` load, actor/wrapper chain and
layer-result normalization to actual `0x00A61DF0` entry. Empty, all-null,
one, sparse and 57 duplicate inputs each use lookup results -1 and 5.
Additional cases demonstrate pending deferral and unchanged forwarding of
a deliberately inconsistent count. Context getter, layer lookup and Model
slot-3 teardown are stubs; optional cleanup objects are null. No resource
pointer is dereferenced by the downstream Model loader in these cases.

Two separate initializer-tail cases start at `0x00A62211` with the preceding
OR-6 modeled, execute real slot 27/getter calls and stop at `0x00A62236`
before ownership/registration. They use synthetic nonnull D with bit 8 clear.
The fragments do not compose a native scene load. No resolver, concrete
resource factory, asset-byte association, whole pass, submission cache/queue,
draw, GPU, client process or live instrumentation executes.

The smallest remaining identity edge is intended S -> actual appearance ->
resolved bank-5 request -> published root's loaded bytes containing the pinned
SHD and WRB mesh. Count consistency, successful construction and the later
enable/pass/override qualifications remain independent admission obligations.
