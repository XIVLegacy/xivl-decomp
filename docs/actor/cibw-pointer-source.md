# Cibw receiver and source pointer

The selected caller supplies type key `0x63696277` to an incoming receiver's
virtual `+0x188` lookup, then installs its returned node in Cibw storage.
The actual lookup receiver's table and the node's backing bytes remain
unbound. This does not authenticate a member named `weponindex` or a
class/model-to-motion-family rule.

## Source and caller

Source: retail 1.23b `ffxivgame.exe`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses are VAs. Original-PE section mapping
with pefile 2024.8.26 and Capstone 5.0.7 x86-32 decoding covered complete
entry-rooted syntactic normal CFGs `[0x008A53A0,0x008A542C)` and
`[0x00852450,0x00852492)`, respectively 44 and 24 instructions. Calls
retain normal fallthrough; exceptions, actual call preservation and lifetime
are not established.

Let S be incoming ECX of `0x008A53A0`. It is saved in EDI at
`0x008A53C5`. The target selected at `0x008A53C7`/`0x008A53C9` is
`[dword[S]+0x188]`. Call `0x008A53DE` uses ECX=S and a pointer to a
local dword set to `0x63696277` at `0x008A53D6`; returned EAX is A.
No producer or cast in this body identifies S's dynamic table.

The reached call at `0x008A5A18` in `0x008A5900` supplies the retained
ESI as ECX at `0x008A5A16`. That caller initially saves its incoming
ECX=W in ESI at `0x008A5916`, with no later local ESI definition before
the call. Thus S=W if its ten preceding opaque callsites, including a looped
call, preserve the retained register and expected stack/control state.
The caller installs no table or typed producer for W. Its complete
entry-rooted normal span `[0x008A5900,0x008A5A52)` covers 104 instructions,
SHA-256 `58cdb82f7b4806f82d49d8dfc2c432742b6b23bc0ca563a056831eb86c816798`.

An upstream call at `0x008DC3BD` in `0x008DC350` supplies the nonzero
full EAX returned from `0x009DA6CC` at `0x008DC3AF` directly as W.
The source pointer is loaded from actual ESI+8 at `0x008DC39D`; the
five supplied cast arguments are that pointer, zero, source TypeDescriptor
`0x012B7AB4` (CDevActor), target TypeDescriptor `0x012B77A8`
(WeaponActor), and zero. The [RTDynamicCast finding](../resource/dynamic-cast-hierarchies.md)
owns the helper identity. Under that contract, nonzero W is a WeaponActor
subobject, potentially within a derived complete object. The zero-result
branch skips the call. No opaque call intervenes between the nonzero return
and receiver copy at `0x008DC3BB`. Equating the source field's actual ESI
with this caller's entry receiver still requires physical preservation across
its earlier calls. The complete normal span `[0x008DC350,0x008DC3F9)`
covers 62 instructions, SHA-256
`d6259f967b405d79a802d25f536eb7e65b250332fdde7a54dbb662f7d7f8daf5`.

Zero A bypasses allocation, installation and local publication. Nonzero A
causes an eight-byte request through `0x009D1B35`. Nonzero storage M is
supplied as ECX to `0x00852450` at `0x008A5403`, with A as its argument.
At `0x008A5414`, the local store is `[EDI+0x230]=EAX`; allocation failure
supplies zero. Identifying this as `[S+0x230]=M` requires preservation of
the retained physical registers and expected stack/control state across the
opaque calls. This is a local store, without a final-state or ownership proof.

## Installed type and backing interface

Initializer `0x00852450` writes table `0x01040A3C` at M at
`0x00852459` and A at M+4 at `0x0085245F`. Original locator cell
`0x01040A38` points to `0x0116E858`, whose type descriptor is
`0x012D12C8`, identifying
`.?AVCibw@Cib@Chara@Actor@Scene@Application@@`.
Slot 0 cell `0x01040A3C` contains `0x008525E0`. This types M; it does
not type S, A or A's interface.

For nonzero A, the initializer loads I=`[A+0x40]`, selects target
`[dword[I]+4]`, and calls it at `0x0085246C` with ECX=I and no locally
supplied stack argument. Returned EAX is P. It reloads R1=`[M+4]` at
`0x0085246E`, then compares `([R1+0x34]&0x7FFFFFFF)` unsigned with
`0x20`. On the admitted size path, `0x0085247F` reads byte `[P+3]`.
If its sign bit is clear, `0x00852486`/`0x00852489` writes that byte
OR `0x80` back. The initializer returns its held M with `RET 4` at
`0x0085248F`, subject to the retained-register conditions. R1 need not
equal A after the opaque call. No local validity checks establish I, target
or P, and no record-byte-zero or version interpretation follows.

The original nominal WeaponActor table `0x01057EE4` has locator
`0x011737E4` with offset zero, the same target TypeDescriptor `0x012B77A8`
and hierarchy descriptor `0x011737F8`. Its virtual +0x188 cell
`0x0105806C=0x007CE9F0` selects the published
[conditional nominal CharaActor lookup](cibb-pointer-and-record-flag.md#conditional-nominal-charaactor-search-path).
That lookup
can identify A as a reached node with +0x30 key `0x63696277` only if
S actually selects that implementation and its published traversal,
cookie, key-stability and return conditions hold. The cast target and nominal
table do not select S's actual current vptr or exclude a derived override.

## Constructed owner and callback profile

Original table references identify initialization body `0x008A5C80` and
cleanup body `0x008A5AB0`. The initializer's first call at `0x008A5CC2`
supplies its unchanged receiver M and `(A1,0x12,A2,A3,A4)` to
`0x006329C0`, where A1..A4 are its four stack arguments. It installs
the WeaponActor table at `0x008A5CE2`. The cleanup body writes the same
table at `0x008A5AD8`; that write does not produce backing data.
The existing [base-constructor contract](../resource/action-queue-findvins-input.md#constructor-input-identity)
and [reference-resource profile](../resource/actor-motion-resource-provider.md#construction-and-initialization)
own the inherited forwarding, initial M+8 clear and A4 source at M+0x114.
Current fields and physical registers must survive the intervening calls.

The nominal WeaponActor creation cell `0x01057F10=0x007CCC10` forwards
through virtual +0x218. Its actual nominal cell
`0x010580FC=0x008A56C0` selects the WeaponActor-specific initializer.
That method invokes virtual +0xFC at `0x008A5734`; retained nominal cell
`0x01057FE0=0x007CD2D0` therefore selects the published provider
construction/publication path. This supplies a conditional M+8 provider
profile without borrowing CharaActor's different +0x218 target.
The provider's explicit local-root vector starts empty. Its later attachment,
root ordering and selected node are not supplied by constructor state.

`0x008A56C0` also constructs `0x0080E8C0`'s already published
[RaptureSchEffectController](../resource/action-queue-scheduler-handoff.md#queue-resource-and-owner)
and publishes it at M+0x220 at `0x008A5785`. That scheduler field is
separate from the lookup provider and is not a resource-root proof.

On its nonnull-candidate path, the WeaponActor constructor's call at
`0x008A5D71` supplies separate storage Q requested with size 0x54 to
`0x008DB4F0`, with arguments M and
X=`0x0060B290([M+0x114])`, and publishes its result at M+0x22C at
`0x008A5D7F`. X's concrete meaning is not qualified here. The child
constructor forwards `(M,5,X,1)` through `0x006307D0` at `0x008DB52A`.
The [shared visual-owner path](body-mesh-pass-admission.md), whose store is
at `0x0061F582`, supplies conditional Q+8=M under its shared-base and
retention premises. The new tuple is not a branch-complete universal owner
proof; no direct Q+8 store occurs in `0x008DB4F0` itself.

The child installs primary/secondary WeaponVisual tables
`0x01064ED4`/`0x01064EB4` at `0x008DB537`/`0x008DB53D`.
Original locators `0x01176C0C`/`0x01176C70` have offsets 0/4 and
share TypeDescriptor `0x012DCC34`. Its primary cells are:

| Slot | Cell | Target |
|---:|---|---|
| 13 | `0x01064F08` | `0x006315D0` |
| 16 | `0x01064F14` | `0x008DC350` |
| 17 | `0x01064F18` | `0x00631590` |

The child registers `(table 0, var_wep, one slot)` through `0x006314D0`
at `0x008DB565`. The [published waiter/readiness route](appearance-path-decoder.md)
delivers rows through slot 17, then slot 13 and conditionally slot 16.
With this constructed primary table retained, the last target is the actual
`0x008DC350` caller above. This establishes a conditional constructed
owner/callback association. It does not identify every runtime Q, prove
completion or readiness, or establish a successful cast in a live invocation.

### Conditional root publication

WeaponVisual's original primary cells also select the published
[ordinary resource completion and publication](../resource/actor-motion-resource-provider.md#conditional-decode-and-accepted-root):

| Slot | Cell | Target |
|---:|---|---|
| 1 | `0x01064ED8` | `0x00631C70` |
| 4 | `0x01064EE4` | `0x00632760` |
| 5 | `0x01064EE8` | `0x00631390` |
| 6 | `0x01064EEC` | `0x00632440` |

This supplies a conditional publisher for the constructed Q/M profile.
It requires an actual Q resource completion with matching nonnull stored H,
nonnull delivered view, the canonical ordinary-decode predicate and successful
nonnull root R. The sampled graph must select the published graph methods;
the respective zero-mode uses require the tested request-record E[k]+0x68
to be zero. These conditions are separate from the `var_wep` row callback.

At `0x00631DFE`, Q slot 6 receives `(M,R,k)`. Its zero-mode call at
`0x00632471` uses M virtual +0x18. Original WeaponActor cell
`0x01057EFC=0x00620880` forwards `(R,0)` to the retained nonnull
M+8 provider B. Its original cell `0x01047258=0x00A6ABE0` selects
the published pointer-retention/insertion path at explicit vector position 0.
The publisher also stores R at E[k]+0x5C at `0x0063249B`, subject to
replacement, opaque-call, memory and lifetime conditions. This is an insertion
event, without a final-state or later winning-node guarantee.

Request record k is not the definition's table-0/one-slot registration, and
the provider destination is literal 0 independently of k. H, decoded R and
the later type-search node remain distinct. The completion's graph+8 decode
parent is also separate from the provider graph+0x0C fallback; equality of
the sampled and retained graph objects requires its own join. No character-bank
path, index formula or selected member is supplied by these inherited cells.
The type-only Cibw search retains its separate published traversal contract.

The additional complete syntactic normal CFGs were
`[0x008A5AB0,0x008A5B8A)`, `[0x008A5C80,0x008A5DA4)`,
`[0x008A56C0,0x008A579E)` and `[0x008DB4F0,0x008DB5A0)`.
Original-PE mapping, RTTI/cells and Capstone/pefile checks used the identity
and method above. Shared base, provider, scheduler and waiter bodies were
reused from their canonical findings. Normal call preservation, current
tables, nonnull allocation, valid storage, successful delivery and lifetime
remain conditions.

## Remaining source boundary

The [delivered-row consumer](weapon-visual-row-attributes.md) qualifies the
`0x008DC398` input as a row-pointer array under the callback profile and
traces selected attribute/ShapeWeight columns. That path does not supply a
selected resource request, root or member identity.

The constructed profile supplies a conditional root publisher. Actual runtime
instance/table continuity, request/input identity and the provider's later
root/vector state remain unobserved. The pre-lookup `0x008DC350` and
`0x008A5900` own-body shapes supply no explicit M+8-provider attachment
or authenticated root-pointer argument. Their opaque calls may have effects;
same-M receivers and delivered `var_wep` rows do not substitute for a root join.
The matched node's +0x40 producer, actual virtual +4 implementation and
association of P with authenticated member bytes therefore remain missing.
No selected member size, digest or weapon-index spelling is established.
