# CameraActor transform input path

`CameraActor` update code at `0x006195D0` selects a context producer, moves
context endpoint fields through `0x0060E4C0`, and builds transform fields
through `0x00617A80` and `0x0061D050`. The traced sources and unresolved
boundaries are listed below.

The [CharaActor object input wrappers](camera-object-inputs.md) trace the
three indirect object methods and their conditional receiver fields.
The [key-2 endpoint calculation](camera-endpoint-calculation.md) follows
its arithmetic and guarded query updates.

## Evidence identity

- Binary: `orig/ffxivgame.exe`, image base `0x00400000`, 15,996,808 bytes,
  SHA256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- Static disassembly: Capstone 5.0.7 x86-32, pefile 2024.8.26, Python
  3.12.0. The instruction and data VAs below are from this exact image.
- Context catalog: `config/ffxivgame.vtable_slots.jsonl`, 26,203,595 bytes,
  SHA256 `b776f19827f3002b6fc7fd522812f23d851b9a6065d47620e54f01bd0ae5732f`.
  The vtable addresses below are image VAs derived from its RVA records.

The field names in this note are byte offsets proven by instruction operands.
`MOVAPS` transfers 16 bytes; `SUBPS`, `MULPS`, and `ADDPS` prove floating-point
lane arithmetic. RTTI class names identify catalog rows only. These facts do
not assign endpoint meaning, an eye point, a fourth-word meaning, or user
target locking.

## Selector and working dataflow

The update entry is `0x006195D0`. It tests camera `+0x448` bit 0 at
`0x0061964C` and branches to the normal path at `0x00619653`. The bit-set path
copies context `+0x330` to working `+0x310` at `0x006196C5-0x006196CC` and
`+0x340` to `+0x320` at `0x006196D3-0x006196DA`, clears `+0x460/+0x464/+0x468`,
and calls `0x00617A80` at `0x00619702`.

The normal path loads the selected key from `+0x434` at `0x0061970C`, pushes
that value at `0x00619751`, and calls `0x00618650` at `0x00619754`.
`0x00618650` tests `+0x448` bit `0x10` at `0x00618653`, uses `0x00616F90`
at `0x00618685`, and resolves `+0x440` through `0x00616310` at
`0x006186F0`. It compares `+0x440` with `0xC0000000` at `0x006186E6` and
falls back through camera `+0x118`, then `[+0xD0]`, at
`0x006186F9-0x006186FF`. This is a pointer and selection edge.

The selected context is then dispatched by vtable byte offsets. Default keys
call offset `+0x04` at `0x006198F3`; the catalog maps the relevant rows to:

- key 0, vtable `0x00FF10BC` (`CameraActorContextTPS`),
  `0x007F3BA0`;
- key 4, vtable `0x00FF110C` (`CameraActorContextTPS_Lock`),
  `0x007F7260`.

Special-key state calls use offsets `+0x1C` and `+0x28` at `0x006197F3` and
`0x006198D3`. The native dispatch and constructor anchors associate the
endpoint-producing `+0x28` rows with key 2 `0x007F56B0` and key 6
`0x007F7410`. The key-7 candidate is vtable `0x00FF11B4`, whose catalog class
is `CameraActorContextWowFPS_Lock` and whose `+0x28` method is `0x007F5310`;
constructor `0x007F1EC0` writes that vtable at `0x007F1EF8`. The key-7
association remains conditional on the selected object carrying that vtable.
Camera constructor `0x0061B5A0` calls `0x007F1EC0` at `0x0061BB3A`, stores
the returned context pointer at `0x0061BB4C`, writes key `7` at
`0x0061BB58`, and inserts it through `0x00621BE0` at `0x0061BB60`. These
registration facts do not establish live object state or lifetime.
Their preceding `+0x1C` rows are `0x007F5450`, `0x007F5CF0`, and
`0x007F5280` respectively.

After the selected context method returns, the entry calls endpoint
interpolator `0x0060E4C0` at `0x00619A92`, then transform builder
`0x00617A80` at `0x00619A9B`.

`0x0060E4C0` reads context `+0x340` and working `+0x320` at
`0x0060E50C-0x0060E513`, performs `SUBPS`/`MULPS`/`ADDPS` at
`0x0060E525-0x0060E537`, and writes working `+0x320` at `0x0060E53A`. It
then reads `+0x330` and `+0x310` at `0x0060E541-0x0060E548`, performs the
same lane operations at `0x0060E54F-0x0060E555`, and writes `+0x310` at
`0x0060E558`. A `COMISS` of `+0x43C` against zero is at `0x0060E4D3`,
and `JBE` at `0x0060E4D6` enters the direct-copy block at `0x0060E565`;
that condition includes unordered as well as `<= 0`. It copies `+0x340` to
`+0x320` at `0x0060E56C` and `+0x330` to `+0x310` at
`0x0060E573-0x0060E57A`.

`0x00617A80` reads working `+0x310` and `+0x320` at `0x00617A9E` and
`0x00617AA5`, then subtracts them at `0x00617AB1`. It first writes a
four-word block to `+0x250` at `0x00617F0F`. A later matrix call to
`0x0061D050` at `0x00618058` writes all four rows to the same `+0x250`
output through `0x00618060`, `0x00618067`, `0x0061806F`, and `0x00618081`.
The builder writes `+0x2D0` at `0x006180D1` after its dynamic call at
`0x0061809D`.

The final matrix call is `0x0061D050` at `0x00618197`. The builder forms
`+0x250` and takes its address at `0x00617F28`; it pushes that pointer at
`0x00618181`, computes an output buffer at `0x00618182`, and pushes that
buffer at `0x00618189`. It then sets `ECX` to camera `+0x290` at
`0x0061818A`. The helper loads its second input from `[EBP+0x0C]` at
`0x0061D059`, its output pointer from `[EBP+0x08]` at `0x0061D076`, reads
the `ECX` input rows at `0x0061D05F`, `0x0061D063`, `0x0061D066`, and
`0x0061D072`, and reads the second-input rows at `0x0061D05C`,
`0x0061D06A`, `0x0061D06E`, and `0x0061D099`. It writes result rows at
`0x0061D0BC`, `0x0061D0F5`, `0x0061D158`, and `0x0061D15F`, which the builder
copies to `+0x210`, `+0x220`, `+0x230`, and `+0x240` at `0x0061819F`,
`0x006181AA`, `0x006181B5`, and `0x006181C0`.

## Retail endpoint writers

These are the exact context methods reached by the catalog dispatch. The
pointer checks and indirect calls are instruction facts; their runtime object
types and callee identities remain unresolved.

- Key 0 `0x007F3BA0` calls `0x00616380` at `0x007F3BD5`, checks the selected
  object and its vtable `+0x258` at `0x007F3BF0-0x007F3BFE`, loads object
  vtable `+0x88` at `0x007F3C0D`, and calls it at `0x007F3C1B`. Its vector
  arithmetic ends with context stores `+0x340` at `0x007F466E` and `+0x330`
  at `0x007F4679`.
- Key 4 `0x007F7260` calls `0x00616380` at `0x007F728C` and
  `0x00610D30` at `0x007F7296`, checks both results at
  `0x007F72A3-0x007F72A9`, loads and calls object vtable `+0x88` at
  `0x007F72D7/0x007F72E2` and `0x007F7304/0x007F730F`, and calls vector
  helpers `0x00663F80` and `0x00663CE0` at `0x007F72F3` and `0x007F7320`.
  It stores `+0x340` at `0x007F73E7` and `+0x330` at `0x007F73F2`; missing
  pointers branch to `0x007F73F9` before a new endpoint store.
- Key 2 `0x007F56B0` checks its selected object at
  `0x007F5700-0x007F5719`, calls `0x0061F250` at `0x007F5765`,
  `0x0060D290` at `0x007F5785`, `0x0061D050` at `0x007F578F`, and
  `0x0060CE50` at `0x007F5796`, then calls `0x00613880` at
  `0x007F57E7` and `0x007F5977`. It stores `+0x340` and `+0x330` at
  `0x007F5996` and `0x007F599D`.
- Key 6 `0x007F7410` calls the local helper `0x007F6F60` at
  `0x007F75CB` and `0x00613880` at `0x007F7634` and `0x007F767A`. It
  interpolates existing `+0x340` and a context vector at
  `0x007F77C4-0x007F77E4`, then existing `+0x330` at
  `0x007F77EB-0x007F77FF`, writing both context endpoints.
- Key 7 `0x007F5310` checks both objects and their vtable `+0x258` status at
  `0x007F533F-0x007F5378`, calls `0x00663F80` and `0x00663CE0` at
  `0x007F5385` and `0x007F5391`, then calls `0x0061CFD0` and `0x0060CE50`
  at `0x007F53B1` and `0x007F53DC`. It stores `+0x330` at `0x007F541C`
  and `+0x340` at `0x007F542F`. Its slot-7 method `0x007F5280` updates
  camera `+0x428` and has no endpoint store in the bounded instructions.

## Key-0 endpoint queries and smoothing

The `CameraActorContextTPS` key-0 worker `0x007F3BA0` requires a
selected target and a nonzero target slot `+0x258` result at
`0x007F3BD5..0x007F3C1B`. A changed target refreshes the cached
binding through context slot `+0x10`. The selected binding helper
`0x007F6170` stores the target at `0x007F6194`. For a nonnull
binding, it seeds the horizontal radius from CameraActor slot 0,
the rate with binary32 0.3 and the vertical offset with binary32
0.5 at `0x007F61BE`, `0x007F61CF` and `0x007F61E4`.
The enter helper `0x007F46A0` can instead seed from existing pose
fields (STATIC).

Let `F` be the candidate focus vector and `E` the candidate eye
vector. The horizontal eye offset uses retained direction at
context `+0x10` and radius at `+0x84`. The vertical eye offset
uses context `+0x78`. A prior downward-probe latch at context `+0x88`
reduces the horizontal radius by `0.20000000298023224 * dt` and clears
at `0x007F4026..0x007F4059`. The ordered positive direction-dot arm
at `0x007F41B9..0x007F426B` clamps the finite radius between the
greater of `R - 1` and 1.5 and the requested slot-0 radius `R`.
A nonpositive or unordered dot retains the old direction, stores
binary32 0.1 in the radius and disables the queries at
`0x007F42B3` (STATIC).

On the query-eligible path, the worker applies the following in order.

1. `0x007F4318` queries from `E` to `E + (0, -0.2, 0)`.
   On a hit it uses the hit point minus that offset for `E`.
   It stores `E.y - F.y` to context `+0x78`. The latch becomes
   the result of comparing the signed input scalar against
   `0.10000000149011612` at `0x007F4348..0x007F4361`. The
   scalar comes from CameraActor word `+0x1BE`, with the sign
   selected by `+0x1C6` mask `0x04`, divided by 100 and rounded
   to binary32 at `0x007F3DA1..0x007F3DE0`.
2. `0x007F4380` queries from `F` to `E`. On a hit it replaces
   `E` with the returned point.
3. When `d = length_xyz(E - F)` exceeds `0.5000000074505806`,
   the worker invokes the radius query at `0x007F443B`. It uses
   `r = 0.20000000298023224` and an endpoint
   `F + normalize_xyz(E - F) * (d - r)`. On success, with hit
   point `H` and normal `N`, it replaces `E` with
   `F + normalize_xyz(E - F) * length_xyz(H + r*N - F)`
   at `0x007F4444..0x007F44AA`. A failed radius query keeps
   the existing `E`.

The 0.5 comparison selects the radius query. Helper `0x006141E0`
reaches `0x00613710`, which passes the radius with the segment
direction and length to the query descriptor at
`0x006137AC..0x006137BE`. The formulas describe the XYZ
lanes. The worker's `normalize_xyz` is the SSE helper
`0x0061CFD0`, with reciprocal-square-root refinement (STATIC).

The worker stores the candidates at context `+0x30` and `+0x50`
at `0x007F44BC..0x007F44C0`. The base rate is binary32 0.3 when
both CameraActor input words `+0x1BC` and `+0x1BE` are zero,
and binary32 0.5 otherwise. If queries were eligible, the rate
target is `1 + (base - 1) * clamp(length_xz(previousEye -
previousFocus) - 1.5, 0, 1)`. This selection uses eligibility
even if each query misses. The other path keeps the base rate
(`0x007F44C4..0x007F4599`). The previous vectors are the
retained values at context `+0x20` and `+0x40` at this block.
The earlier discontinuity path at `0x007F3C79..0x007F3D0C` can
refresh those values before interpolation.

At `0x007F45A7..0x007F4679` the worker moves context rate
`+0x7C` halfway to that target. For finite inputs the interpolation
factor is `min(rate * dt, 1)`. It interpolates the previous eye
and focus at context `+0x20` and `+0x40` toward the corresponding
candidate. It forwards these vectors to CameraActor `+0x330`
and `+0x340`. The caller subsequently performs the separate
working-endpoint interpolation described above (STATIC).

The [camera update](camera-input-chain.md#update-pipeline-and-command-receiver)
owns the input and `dt` producer. The live target vector, selected
world query shape, elapsed unit and character visibility remain UNKNOWN.

## Key-4 endpoint queries and smoothing

Key 4's vtable `0x00FF110C` selects `0x007F7260`. When
both retained objects pass the null checks at `0x007F72A3`
and `0x007F72AB`, the worker calls `0x007F66F0` at
`0x007F7336` with its context, the CameraActor and float delta.
The constructor can select key 0 or key 4, subject to the
[initial context predicates](camera-input-chain.md#initial-state).

Let `E` be the candidate eye, `F` the context vector at
`+0x30`, and `O` the separately calculated focus candidate
that the helper stores to context `+0x80`.

1. The line call at `0x007F6CA9` uses `E` and
   `E + (0, -r, 0)`, with binary32 `r = 0.20000000298023224`.
   A hit replaces `E` by the returned point minus that offset
   and stores `(E-O).y` at context `+0x9C`.
2. The line call at `0x007F6CF7` uses `F` and current `E`.
   A hit replaces `E` with its returned point. The helper rounds
   `d = length_xyz(E-F)` to binary32 before comparing it with
   the binary64 threshold `0.5000000074505806`.
3. Ordered `d` above that threshold reaches the radius helper
   `0x006141E0` at `0x007F6DB0` with radius `r` and segment
   `F` to `F + normalize_xyz(E-F) * (d-r)`. The subtraction
   uses the binary64 representation of `r` before narrowing
   the segment scale. On success, `0x007F6DBD..0x007F6E3D`
   stores `F + normalize_xyz(E-F) * length_xyz(H+r*N-F)`
   at context `+0x60` for the returned point `H` and normal
   `N`. It stores the resulting eye's Y difference from `O`
   at `+0x9C`. A miss or bypass stores the current `E` at
   `+0x60`.

The caller moves retained vectors `+0x50` and `+0x70` toward
`+0x60` and `+0x80` at `0x007F733B..0x007F73DF`. For finite
delta the factor is binary32 `dt * 0.5` capped at 1. The caller
stores the retained eye and focus to CameraActor `+0x330` and
`+0x340` before the separate outer interpolation. The
[ordinary projection refresh](camera-input-chain.md#lens)
uses its fixed near and far inputs after context update.

Reproduce on the identified image by checking
`0x007F7325..0x007F733B`, `0x007F6C3F..0x007F6E91` and
`0x007F733B..0x007F73F9` with x86-32 disassembly. Read
the binary32 radius at `0x00FF0714` and binary64 operands
at `0x00FC3368` and `0x00FF12D0`. These calls and operands
are STATIC. Live context, query eligibility, selected geometry
and device projection remain UNKNOWN.

## Camera query filters and mesh hits

The camera line worker `0x00610D60` and radius worker
`0x00613710` read CameraActor `+0x110` and use the provider
collection at its `+4`. Both supply group mask `0x00010006`
and the optional mask and comparison pair `(1, 0)` from
`0x0103DA28`. The line descriptor initializes its `+0x30`
flags to 3 and passes exclusion 3 through
`0x00610E85 -> 0x00AF94D0 -> 0x00B016E0`.
The radius descriptor writes length at `+0x20`, radius at
`+0x40` and forces flag bits `0x05` at `0x00613800`.
Its dispatcher `0x00AF8E40` selects exclusion 3 with bit 2 set.
Other preexisting radius descriptor bits are unresolved (STATIC).

The selected scene enumerators check the same shape conditions at
`0x00B0CAF3..0x00B0CB38` for the line and
`0x00B0DF57..0x00B0DF9A` for the radius. Shape byte
`+0x4B` has no overlap with exclusion 3 and has bit `0x10`
set. The group bit indexed by the low five bits of shape
`+0x4C` intersects the query mask. The optional comparison
at `0x00B06950..0x00B06997` requires a clear low bit of
shape `+0x4E` on the checked non-type-8 path. Type 8
bypasses that outer comparison. Its mesh leaf checks the low
bit of the selected triangle's 16-bit word `+6` at
`0x00B0C707..0x00B0C752` for the line and
`0x00B0D956..0x00B0D9B1` for the radius.
See [shape flags and groups](../resource/attribute-resource-and-scene-binding.md#query-shape-flags-and-groups)
and [triangle reads](../resource/attribute-resource-and-scene-binding.md#collision-query-leaf-and-triangle-reads)
for the source structure (STATIC).

Radius enumerator `0x00B0DF0F..0x00B0E100` calls
`0x00B0DD30`. A shape with `+0x4A` equal to 8 reaches the
triangle-mesh branch `0x00B0D0E0` at `0x00B0DDE6`.
Other types reach `0x00B0B040` at `0x00B0DD68`.
The mesh branch expands triangle vertices through
`0x00A7CB20` at `0x00B0D9F8`, transforms them at
`0x00B0DA0E`, then projects the initial center onto the
triangle at `0x00B0DA27`. For initial separation at least
the radius, the solver call at `0x00B0DAD8` supplies one
radius and the center displacement to `0x00B0AD00`.
On success, `0x00B0DB1C` projects the advanced center
onto the triangle. This is the checked swept-sphere
mesh path (INFERRED from the static solver arguments).

The mesh hit stores the projected point at record `+0x10`
at `0x00B0DBB2`. It stores the normalized triangle
cross-product vector at `+0x00` through the helper
`0x00B0E730`. The shape `+0x4B` bit `0x20` branch can
orient that normal relative to the query direction
(`0x00B0DC3B..0x00B0DC76`). On the checked noninitial
mesh path the returned comparison scalar includes the
travel length plus the radius at `0x00B0DB26..0x00B0DB61`.
The outer search at `0x00B0E082..0x00B0E0D1` replaces
its result on a nonnegative, strictly smaller scalar.
Equal candidates retain the earlier hit. It copies the
record point and normal, which the camera helper returns
at `0x0061421F` and `0x00614227` (STATIC).

Live shape selection, initial overlap details and the result
semantics of other primitive types remain UNKNOWN.

### Region 103 nominal tavern segment

The installed interior layout `0x29B00002` is 460368 bytes
with SHA-256
`f2088d1feba5fe71fbc4c462229f1bd1970371d6f023622a7a83d6aa8d450876`.
Its Attribute at file `0x5AEDC` has group byte 1 at
`0x5AF04`. The `bhp` row 427 at file `0x35A0` selects
PHB `0x72E7059B`. The matching placed child has ID 19,
with byte `0x05` at file `0x55E64`. The Attribute
[placement setup](../resource/attribute-resource-and-scene-binding.md#placed-setup-flags-and-group-byte)
describes the corresponding conditional native group write.

PHB `0x72E7059B` is 238928 bytes with SHA-256
`2591e3858e7806ae21e59597da84c03d9930a03ebd77beb26d78b2cab5a61fd5`.
Hull 0 triangle 4956, indexed from zero, starts at file `0x29920`.
Its little-endian surface word at `0x29926` is 5.
The camera's type-8 mesh triangle check excludes this face
when the placed mesh reaches that native branch (STATIC).

An offline probe used the nominal focus
`F = (66.15, 5.2, -1198.49)` and endpoint
`F + (0, 4.5*sin(0.2), -4.5*cos(0.2))`.
Export each layout `0x29B00000..0x29B0000D` and `0x29B00018`
with the pinned
[`export-zones` command](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/cli.md#export-zone-collision-geometry).
Intersect the supplied segment against both sides of the exported
triangles and use the face ranges to recover resource and triangle identity.
The nearest of six intersections was triangle 4956 at
1.011330 yalms. All six had surface 5 in the same PHB.
All 24 triangle bounds overlapping the nominal segment's bounds
expanded by 0.2 also came from this PHB with surface 5.
The 0.2 downward eye segment hit no exported triangles.
These measurements use the supplied offline pose and exported mesh
geometry. Live camera endpoints and native contacts remain UNKNOWN.

## One-layer helper evidence

The vector helpers expose the remaining dynamic object inputs. In
`0x00663F80`, object vtable `+0x9C` is loaded at `0x00663F9F` and called at
`0x00663FB3`; vtable `+0x94` is loaded at `0x00663FBB` and called at
`0x00664054`; a later vtable `+0x88` load/call is at
`0x00664070/0x0066407D`. The helper uses `0x0061D050` at `0x00664069` and
writes its output at `0x006640E7`.

`0x00663CE0` has the corresponding `+0x9C` load/call at
`0x00663D00/0x00663D11`, `+0x94` load/call at `0x00663D19/0x00663DBE`,
several `0x0061D050` calls including `0x00663DD3`, and a final `+0x88`
load/call at `0x00663ED1/0x00663EDA`; it adds and stores the result at
`0x00663EEA/0x00663EEF`.

`0x00613880` calls `0x00610D60` at `0x006138AE`. The query result is tested
at `0x006138B3`; `JE` at `0x006138B5` branches to the no-store epilogue at
`0x0061392A`, which sets `AL=0` at `0x00613933` before the cookie-check call
at `0x00613935`. Even after a true query, `COMISD` at `0x006138FF` and
`JBE` at `0x00613903` can take that same no-store path, so the output stores
require the second comparison to pass. The successful path loads source
vectors from `[EDI]` and `[ESI]` at `0x006138B7` and `0x006138BA`, uses
`SUBPS`/`MULPS`/`ADDPS`, writes caller buffers at `0x0061390E` and
`0x00613911`, and returns `AL=1` at `0x00613914`.
`0x00610D60` returns false when its object `+0x110` is null at
`0x00610DA0-0x00610DB7`; it writes `0xFFFFFFFF` to local stack word
`[ESP+0xC0]` at `0x00610DE6` during initialization, not to an endpoint or
vector field. Its later vector inputs and output copies are at
`0x00610DF1-0x00610E23` and `0x00610E8D-0x00610ECF`.

The direct pointer helpers are also bounded. `0x00616380` compares `+0x440`
with `0xC0000000` at `0x00616389-0x0061638E`, calls `0x00616310` at
`0x00616391` when applicable, and falls back through `+0x118/+0xD0` at
`0x0061639A-0x006163A0`. `0x00610D30` follows `+0x118/+0x70`, calls
`0x007D2A50` at `0x00610D50`, and returns zero at `0x00610D59` when its
lookup fails.

The exact object-derived four-word values are therefore produced through
indirect `+0x88`, `+0x94`, and `+0x9C` methods. This bounded static trace does
not identify those runtime callees or their receiver object layouts.

## Constant and initialization findings

The constructor `0x0061B5A0` initializes working `+0x310` and `+0x320` at
`0x0061B6D9` and `0x0061B6FD`, then copies them to `+0x330`, `+0x340`,
`+0x350`, and `+0x360` at `0x0061B704-0x0061B735`. It writes
`+0x440 = 0xC0000000` at `0x0061B7C5` and an independent scalar
`+0x444 = 0xFFFFFFFF` at `0x0061B7CF`; this trace does not show either write
as an endpoint vector store. The same body writes `0xFFFFFFFF` to local stack
word `[ESP+0x50]` at `0x0061BD55` before restoring its exception state; this
is also outside the endpoint fields.

The assignment dispatcher `0x0061A6C0` copies the four-word result of
`0x00545620` to `+0x330/+0x310` at `0x0061A6F4-0x0061A70C`, and another case
copies a result to `+0x340/+0x320` at `0x0061A72C-0x0061A751`. The source
pointer and case meaning are unresolved.

The bounded absolute-load scan finds the image constant at `0x00F62F50` used
by `MOVSD`/`ANDPD` at `0x00617BBC/0x00617BC4`,
`0x0061852A/0x00618551`, and `0x007F5DB2/0x007F5DC1`. Its
little-endian words are `0xFFFFFFFF` and `0x7FFFFFFF`, but these uses are
mask operands for scalar absolute-value operations, not `MOVAPS` endpoint
stores.
The bounded SIMD constant loads used by the matrix/vector helpers are finite
one-hot rows at `0x00FB7A60`, `0x00FB6D80`, `0x00FB7A70`, and `0x00FBF760`.
The listed immediate operands and absolute loads do not exclude `PCMPEQD`,
other construction, or values returned by helpers.

## Static numeric limit and finding

The bit patterns `0xFFFFFFFF` and `0x7FFFFFFF` decode as IEEE-754
single-precision NaN bit patterns. This interpretation does not execute SSE
or simulate the retail path. Native `MOVAPS` copies those words, while the
listed `SUBPS`, `MULPS`, and `ADDPS` instructions perform floating-point lane
operations on their operands.

The traced boundary is: selected context methods write `+0x330/+0x340`;
`0x0060E4C0` transforms those fields into working `+0x310/+0x320`; and
`0x00617A80` consumes the working fields while building `+0x250/+0x2D0` and
passes camera `+0x290` plus `+0x250` to `0x0061D050`; the helper result is
copied to `+0x210`. This boundary does not establish
whether a nonfinite value entered through an endpoint or object return, was
produced by arithmetic from finite inputs, or came through the `0x0061A6C0`
assignment path and its `0x00545620` result. The `+0x210` chain has separate
direct inputs from camera `+0x290` and computed `+0x250`; their upstream
producers remain unresolved. The exact dynamic callee, receiver state, and
runtime execution order also remain unresolved. The constant mask and local
status writes are concrete all-ones references, but neither is established as
the endpoint invalidation source.
