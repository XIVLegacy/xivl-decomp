# Model LightSet construction and draw lists

The single-entry Drawable construction path conditionally populates the
LightSet array indexed by the [MINS byte list](floor-model-render-path.md#mins-head-byte-list).
The selected LightSet pointers reach the component list exposed by two
MeshDrawTag getters. A published PixelLight source contract can consume that
list; the separately selected VertexLight path has a different checked source.
Actual loading, current objects/tables, allocation, driver membership and
lifetime remain conditions throughout.

Source: retail `ffxivgame.exe` 1.23b, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. Addresses are VA. Capstone 5.0.7
x86-32 and pefile 2024.8.26 checked original instructions, normal control
flow, original RTTI and virtual cells. No client execution or asset/row bytes
were used for these findings.

## Connected construction and distinct arrays

The [single-entry construction](floor-model-render-path.md#primary-resource-and-concrete-receivers)
and [ownership profile](../actor/body-mesh-pass-admission.md#ownership-before-registration)
connect `0x00C50FE0`'s one-collected-model branch to `0x00C4F8E0` and
the Drawable constructor call at `0x00C4F9BF`. The caller retains an output
pointer L and stores that constructor's return through L at `0x00C4F9DD`.
Later `ptr32(L)` reloads identify the same D only under owner/storage
continuity. One candidate does not prove one collected model or a live load.

Three vector headers and their fields are distinct:

| Receiver/header | Begin | End | Capacity end | Role on this path |
|---|---|---|---|---|
| Drawable D+0x338 | D+0x33C | D+0x340 | D+0x344 | Member-pointer source array |
| Drawable D+0x34C | D+0x350 | D+0x354 | D+0x358 | Constructed LightSet pointer array |
| LightSet W+0x20 | W+0x24 | W+0x28 | W+0x2C | Selected member pointers |

These are native pointer/value arrays, without a serialized record-layout
claim. Their count arithmetic uses 32-bit differences shifted right by two.
Valid ordered, aligned, nonwrapping spans remain premises.

## LightSet source selection and construction

The current native resource S is loaded at `0x00C4FAAB` and retained in a
local at `0x00C4FAB2`. The LightSet loop reads S+0x88/+0x8C at
`0x00C4FDFE`, skips a signed-nonpositive count, and advances ordinal j.
At `0x00C4FE2A`, `0x00C51900(S,j)` returns the selected source word
S_j from that pointer array on its valid-index path. Its callbacks and
`0x009D22B4` failure path remain opaque. After such calls it reloads the
current array; no failure rejection or old-array stability is assumed.
S_j's concrete resource/payload type remains unbound.

For nonnull storage A requested with size 0x34, the caller invokes
`0x00C306D0(A,S_j,I)` at `0x00C4FEA6`, where I is the freshly reloaded
owner's embedded `D+0x2E0` interface address. An owner-null branch supplies
I=0 at `0x00C4FEB3`; this does not prove null-safe initialization. Allocation
failure bypasses the initializer and supplies zero to the append setup.

The initializer installs table `0x010D4E94` at `0x00C306FE`. Its original
COL `0x01199B30`, offset zero, and TypeDescriptor `0x013092A8` name
`.?AVLightSet@Renderer@Dw@Engine@CDev@SQEX@@`. This anchors native
LightSet identity directly, independently of ILightContainer terminology.
The body returns its saved receiver at `0x00C30801` with RET 8, subject to
ordinary call/control and saved-storage preservation.

It initializes name/string-like state at W+4 and calls `0x0043F4B0`
with `(S_j+4,0,-1)` at `0x00C30744`. It zeros the member vector's three
pointer fields and calls opaque `0x00C30D60(W+0x20,sourceCount)` at
`0x00C3075F`. No capacity, successful copy or encoding guarantee follows
from these opaque calls alone.

The caller later reloads the current owner and targets D+0x34C. On its
nonzero-begin, unsigned size-below-capacity branch, `0x00C4FEFD` stores
the initializer's actual return W, or allocation-null zero, at the old end;
`0x00C4FF02` advances D+0x354 by four. The other branch calls opaque
`0x00C4A6C0` at `0x00C4FF24`. Initial state, growth, order and same-owner
conditions are required to equate source ordinal j with final insertion
ordinal. Neither a universally successful append nor an all-nonnull runtime
LightSet array is established.

## LightSet member lookup and stores

The initializer reads a four-byte index list through S_j+0x24/+0x28.
Call `0x00C30731` obtains a limit through I virtual +4. A source value
signed-less-than that cached limit can reach a fresh I virtual +8 call at
`0x00C307E7` with the value as its index. Negative inputs have a separate
opaque diagnostic path; normal continuation does not prove their rejection.

The [nominal ILightContainer profile](floor-model-render-path.md#mins-head-byte-list)
has original cells `0x010D63BC=0x00C397E0` and
`0x010D63C0=0x00C39800`. Under actual selection of those cells, the
count/getter pair reads full Drawable D+0x33C/+0x340 and selects a
four-byte member value. This is a different array from the D+0x350/+0x354
LightSet array used by the MINS byte list. Owner, table, cached limit and
current array state require their own continuity across calls.

The selected value p is saved in a reused argument slot; its address is
passed to `0x00C31230` with receiver W+0x20 at `0x00C307F4`.
On that helper's nonzero-begin, unsigned size-below-capacity fast path,
`0x00C31262` copies p to the old member end and `0x00C31267` advances
W+0x28 by four. The alternative calls opaque `0x00C31100`. The preceding
zero initialization and opaque preparation do not prove which append path
executes or establish final member order.

The connected prefix supplies D+0x33C/+0x340 values through another inline
append at `0x00C4FDBE..0x00C4FDC3`. Its input P was read from an output
slot returned by `0x00C51610`; the caller zeros that slot after reading it
at `0x00C4FD03..0x00C4FD05`. The factory consumes an explicit output-slot
argument, requests 0x100 bytes, calls `0x00C2F690` on nonnull storage at
`0x00C5167C`, and stores the actual returned P or allocation-null zero
through that slot at `0x00C51689`. It returns the slot's address at
`0x00C5168B`, rather than the stored value. It is an output-slot factory,
without an ordinal/resource-vector selection contract.

The called `0x00C2F690` constructor saves its entry allocation A in ESI
at `0x00C2F69A`, makes one opaque base call to `0x00A42AA0`, and returns
ESI through EAX at `0x00C2F7A9`. There is no own ESI redefinition before
its plain RET at `0x00C2F7AF`; thus P=A under the base call's normal
physical/stack/state preservation. It installs main table `0x010D4DE0` at
`0x00C2F6A4`. Original COL `0x01199AF4`, offset zero, TypeDescriptor
`0x012C5F10` names
`.?AVLightNode@Renderer@Dw@Engine@CDev@SQEX@@`.
The [Light record](zone-lighting-source-and-time-of-day.md#light-record)
owns that constructor's existing field-role findings.

This anchors the constructed P as Renderer::LightNode on the factory/member
path. Later virtual/direct initialization, current table retention, successful
allocation and owner/array preservation remain conditions. It does not type
every current array entry by proximity or establish a loaded LightSet's
complete live membership, parameters or serialized source identity.

## Component list and conditional PixelLight read

The [MINS lookup/store](floor-model-render-path.md#mins-head-byte-list)
selects a value from the current Drawable's LightSet array and passes it to
`0x00C32C80`. Valid initialized component counter values 0..3 place those
values at C+0x14/+0x18/+0x1C/+0x20. The signed guard alone does not
establish counter initialization or a valid lower bound.

The [constructed tag profile](../actor/weapon-visual-row-attributes.md#constructed-tag-and-mesh-profile)
supplies T+0x118=C. Original MeshDrawTag table `0x010D7A04` exposes:

| Slot/cell | Target | Complete normal body result with ECX=T |
|---|---|---|
| 4, `0x010D7A14` | `0x00C452F0` | EAX=`dword[C+0x10]` |
| 5, `0x010D7A18` | `0x00C45300` | EAX=`C+0x14`, the inline-array address |

Each getter is ten bytes, three straight-line instructions and a plain RET,
with no explicit stack input or null/count/span guard. The count's full
stored bits are returned; the second getter does not dereference C+0x14.
Nominal COL `0x01199FBC`, TypeDescriptor `0x01309684` and actual current
table/owner retention distinguish the static profile from an arbitrary object.

Ordinary MeshDrawTag `0x00C439C0` stores T at current render context
K+0x1A0 at `0x00C43A14`. The published
[PixelLightShaderDriver contract](zone-lighting-source-and-time-of-day.md#draw-time-light-list-and-rules)
reads that context field as a LightSet list through virtual +0x10/+0x14.
Under that contract, same K, current K+0x1A0=T, actual nominal getter
selection and valid retained C list, the consumer sees this component list.
The exact C22DD0 virtual-call instruction ABI was not newly decoded here.

Reaching a tag draw does not establish accepted PixelLightShaderDriver
membership or its invocation with that K. The
[selected body profile](../actor/body-shader-constant-join.md#accepted-driver-registers)
establishes accepted VertexLight, which is a separate driver. Actual material/
candidate/driver dispatch, current table/context and lifetime joins remain
conditions. No GPU submission or visible light contribution is established.

## Selected VertexLight source

For the published body profile's zero lightCount/light0bit0 handles,
VertexLight slot 2 `0x00C22A90` selects `0x00C21A60`. Let V be the
driver, K the wrapper's first explicit context argument and Q its second
shader-container argument. Call `0x00C22D2E` supplies ECX=V and explicit
arguments `(K,Q)` to the helper. Both bodies return with RET 8.

The wrapper clears cached count V+8 at `0x00C22C20` and loads the
separate per-node block L=`ptr32(K+0x308)` at `0x00C22C26`. Its source
loops read L+0x150, traverse pointer slots from L+0x30 with stride 0x24,
filter the nodes and write accepted pointers to V+0x0C+4*k while advancing
V+8. It also reads the block's first sixteen bytes. The selected helper
reads that cache and context tints at K+0xC04..K+0xC20.

Complete own-body checks of this wrapper and selected helper found no
attributable direct K+0x1A0 memory/address access. Aligned local-stack
stores/reloads and wider memory overlaps were included. Whole-context calls
to `0x00C483D0/0x00C483F0/0x00C48340/0x00C48350/0x00C48370`
remain opaque, as do node-data helpers `0x00C20A70/0x00C20B80`.
Their descendants and unknown aliases are not excluded by this bounded
negative. The selected positive source is K+0x308; the MINS LightSet-list
bridge cannot substitute for it. The alternate VertexLight helper and
PixelLight membership remain separate questions.

## Coverage and source limits

The connected new prefix `[0x00C4F8E0,0x00C50058)` contains 546 reached
instructions/1912 bytes. It exits to `0x00C5025E`, `0x00C501F8` or
the separately qualified MINS continuation. No constructor RET was reached.
This is prefix coverage, without complete constructor, exception or descendant
coverage. Additional complete normal bodies were:

| Half-open VA span | Role |
|---|---|
| `0x00C51900..0x00C51995` | Native source-entry selector |
| `0x00C306D0..0x00C3081B` | LightSet initializer |
| `0x00C397E0..0x00C397F3` | Nominal member count |
| `0x00C39800..0x00C39833` | Nominal indexed member |
| `0x00C31230..0x00C31299` | Member append fast store |
| `0x00C51610..0x00C5169D` | Output-slot factory |
| `0x00C2F690..0x00C2F7B0` | Constructed LightNode return/type join |
| `0x00C452F0..0x00C452FA` | Component LightSet count getter |
| `0x00C45300..0x00C4530A` | Component LightSet array getter |
| `0x00C22A90..0x00C22DD0` | VertexLight wrapper |
| `0x00C21A60..0x00C21F3B` | Selected VertexLight helper |

Source S_j's payload identity, preparation/growth behavior, actual loaded
owner/table state and selected driver/context identity remain explicit limits.
The component counter initializer is a supplied callback `0x00C33B20` in
the call at `0x00C4FFF4`; neither that callback nor its iterator was read.
No initial count is inferred from its presence. The component+0x24 and
attribute-byte-buffer reader questions are distinct from this LightSet path.
