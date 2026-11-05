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

## Finder and output slots

At `0x00C51026/0x00C5102E`, the wrapper constructs Finder F at its
local+0x18, passing the supplied resource list, count and fifth input to
`0x00C524A0`. It reads F+0x40/+0x44 for its model-count decision.
On the one-entry branch, `0x00C51094` supplies builder arguments
`(L,F,U)`: L is a separate local output-slot address and U is the wrapper's
fourth input. The builder reloads that same F at `0x00C4FA4C`, reads its
current F+0x40 begin at `0x00C4FA58`, and loads S from the current iterator
at `0x00C4FAAB`. The
[metadata/ancestry profile](floor-model-render-path.md#primary-resource-and-concrete-receivers)
types this collection as admitted ModelResourceNode resources. Current list,
metadata, iterator, frame and lifetime continuity remain premises; a prior
count-one decision does not freeze the array or identify S's actual creator.

The builder loads L at `0x00C4F926`, saves it at local+0x1C at
`0x00C4F932`, and reloads it on the loop exit at `0x00C50251`.
The incoming own EDI definitions at `0x00C50283`, EAX=EDI, are exactly
that initial load and saved-slot reload. Under saved-slot/frame and physical
preservation through opaque calls, its plain RET at `0x00C502A8` therefore
returns L. The wrapper reads the returned slot at `0x00C51099`, clears it at
`0x00C510A2`, and transfers the logical object word through its own supplied
output slot O at `0x00C510A4`. The empty-model branch stores zero through O.
After Finder cleanup, `0x00C510CC` sets EAX=O before plain RET at
`0x00C510E6`. Thus O is the wrapper's raw return address; *O is its logical
object/null result, subject to output storage and normal call preservation.

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

## Native LightNode property source

After construction, the prefix tests packed record J+0x14 with mask 0x7C00
against 0x1000 and derives q by arithmetic-right-shifting that word by 15.
Call `0x00C4FD30 -> 0x00C512D0` reads the current resource S's
pointer-array fields +0x74/+0x78. A signed q below its count can reach two
`0x00C51870(S,q)` selections. The first result Q1 supplies an opaque
virtual source/initializer path; the fresh second result Q2 at `0x00C4FD75`
is the actual input to `0x00C2F280(P,Q2)` at `0x00C4FD7D`.
Q1=Q2 requires current source/array stability across the calls. Neither the
signed count test nor the selector's opaque diagnostic paths prove rejection
of negative or otherwise invalid indices.

The initializer has no calls and reads Q2 directly. Its selected fields are:

| Q2 source to same P offset | Operation | Destination role from the Light record |
|---|---|---|
| +0xDC | Dword bit copy | Light type |
| +0xC0..+0xCF | Two eight-byte MOVQ bit copies | RGB and alpha |
| +0xD0 | Binary32 FLD/FSTP | Intensity |
| +0xE0/+0xEC/+0xE8/+0xF0 | Binary32 FLD/FSTP | Point extras |
| +0xE4 | Source/minimum MOVSS selection | Point extra |
| +0xF4 | Dword bit copy | Priority |
| +0xF9 | Byte bit copy | Category |

The canonical offset/role distinction remains: the offsets are verified and
the names interpreted. These native loads have no endian reversal. FLD/FSTP
are floating-point transfers, without an unconditional all-encoding bit-copy
claim for exceptional values or FP state.

The +0xE4 comparison at `0x00C2F2D8` uses the referenced binary32
constant at `0x00FB7AB4`, bits `0x3A83126F`, approximately 0.001.
Under normal COMISS completion, JB at `0x00C2F2DB` selects that constant
for less or unordered input; otherwise the source is stored. This is a
minimum clamp for ordered values. Its magnitude establishes no units.
The +0xDC copy has no local type-range validation. Packed J kind 4 and
LightNode ambient type value 4 belong to different selector domains.

Q2 is a native property source with matching field positions. That shape
does not prove its RTTI type, producer or serialized backing. Actual source/
destination spans, current P type/lifetime, selector callbacks and normal FP
completion remain conditions. No source asset, light identifier or universal
parameter invariant is established.

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
reads that context field as a LightSet list. Its actual entry ABI is driver
ECX plus explicit `(K,Q)` context/shader-container inputs, with RET 8.
At `0x00C233C7` it loads T_count from K+0x1A0 and invokes that object's
table +0x10 at `0x00C233EB`, ECX=T_count, with no newly staged explicit
argument. It freshly reloads T_array at `0x00C233ED` and similarly invokes
table +0x14 at `0x00C233FE`. The count's full 32-bit EAX is tested for a
signed value at or below zero; the full array-pointer EAX supplies W=`dword[array+4*i]`
at `0x00C23416`. The two current tag/table loads are not assumed equal
across the virtual count call. Joining these to one component list requires
same-tag/table/count/array and valid-address continuity.

For that selected W, the driver reads member begin/end at W+0x24/+0x28
and calls `0x006213C0(W,j)` at `0x00C2344F`, with ECX=W and the current
member index as its sole explicit argument. The first entered index is zero;
the API is not limited to zero. The getter checks a signed cached count,
reloads the current member array and checks an unsigned index, then returns
the selected full dword member value in EAX. It returns that value, rather
than its vector-cell address. Its diagnostics and `0x009D22B4` paths remain
opaque; ordinary continuation can reload the array and perform a read, so no
malformed-input rejection or nonreturn policy is inferred.

The Pixel caller immediately dereferences p+0xDC at `0x00C23454`, where
p is that actual EAX return. One reached branch reads p+0xC0/+0xC4/+0xC8/
+0xCC/+0xD0 at `0x00C23461..0x00C2349E`. Under the same constructed
member/current-array profile, these are the LightNode fields supplied by the
property path above. Their actual same-object, valid-span and lifetime joins
remain conditions. This trace qualifies only the tag's +0x10/+0x14 calls;
it supplies no additional tag getter or component+0x24 reader. LightSet
W+0x24 is its member begin pointer, a distinct field/object.

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

The connected prefix `[0x00C4F8E0,0x00C50058)` contains 546 reached
instructions/1912 bytes. Together with the adjoining MINS prefix/continuation
and actual exit tail `[0x00C5025E,0x00C502A9)`, the entry-rooted normal CFG
closes at 713 instructions/2505 bytes, through plain RET at `0x00C502A8`.
The complete contiguous span `[0x00C4F8E0,0x00C502A9)` has SHA-256
`89e64aae293153a79147212ee02bca3736a0df79bae96f1c45f5c201745cb149`;
the 75-byte exit tail has SHA-256
`73df9936cbe67bf93ad5e5c30ef3fcdf27fbdb9b8f603504f6ce9b90aac74a5d`.
Calls remain opaque normal-return edges, without exception or descendant
coverage. Additional complete normal bodies were:

| Half-open VA span | Role |
|---|---|
| `0x00C50FE0..0x00C510E7` | Finder input, one-entry caller and output-slot result |
| `0x00C51900..0x00C51995` | Native source-entry selector |
| `0x00C306D0..0x00C3081B` | LightSet initializer |
| `0x00C397E0..0x00C397F3` | Nominal member count |
| `0x00C39800..0x00C39833` | Nominal indexed member |
| `0x00C31230..0x00C31299` | Member append fast store |
| `0x00C51610..0x00C5169D` | Output-slot factory |
| `0x00C2F690..0x00C2F7B0` | Constructed LightNode return/type join |
| `0x00C512D0..0x00C512E3` | Native property-source count |
| `0x00C51870..0x00C518F6` | Native property-source selection |
| `0x00C2F280..0x00C2F32E` | LightNode field copies and minimum clamp |
| `0x00C452F0..0x00C452FA` | Component LightSet count getter |
| `0x00C45300..0x00C4530A` | Component LightSet array getter |
| `0x00C22A90..0x00C22DD0` | VertexLight wrapper |
| `0x00C21A60..0x00C21F3B` | Selected VertexLight helper |
| `0x00C22DD0..0x00C23A0A` | PixelLight list source/virtual-call ABI |
| `0x006213C0..0x0062142E` | Selected LightSet member getter |

Using PixelLight's catalog body count of 3120 bytes as an initial contiguous
read length stops at `0x00C23A00`, inside the reached security-cookie call at
`0x00C239FF`. The count is a
[sum of body ranges](../../tools/README.md#import-and-catalog-export), not an
enclosing extent. Independently completing that call and its ordinary
fallthrough adds ten bytes, through RET 8 at `0x00C23A07`.
The completed normal envelope is 3130 bytes, SHA-256
`b4cfbb04972712b76cf97d9504054d4c9e59ef6481da763c5eced44651a61e6b`.
Its CFG reaches 661 of 664 decoded instructions; decoded sites
`0x00C2343C`, `0x00C2388D` and `0x00C2389D` are unreachable under that
model. No nonreturn assumption hid the ordinary tail, and no neighboring
body was inspected.

Source S_j's payload identity, preparation/growth behavior, actual loaded
owner/table state and selected driver/context identity remain explicit limits.
The [component initializer](floor-model-render-path.md#component-initialization)
qualifies the actual callback's selected count/default writes and iterator
receiver assignment. Its zero count applies at callback return, with retention
to the later list store still required. The component+0x24 reader and attribute
byte buffer reader remain separate questions from this LightSet path.
