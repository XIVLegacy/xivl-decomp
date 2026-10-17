# Floor model bounds and visibility inputs

The selected four-placement floor fits the native expanded layout block
bounds. Its authored model has matching local mesh, model and compression
bounds. These offline checks do not determine the selected camera, active
blocks, distance ranges or pixel clipping state.

The BGChip layout rejection path uses its owning BlockAABB. The
DrawableObject union and property `+0xA0/+0xB0` rejection path belong to
different native branches; connecting those boxes does not establish the
selected BGChip's layout rejection inputs.

## Evidence and selected bytes

Native observations use retail 1.23b `ffxivgame.exe`, 15,996,808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Native addresses below are VAs; subtract
`0x00400000` for RVAs. Ghidra `tools/ghidra_scripts/DumpFunctions.java`
produced the retained instruction listings. PE section mapping checked the
listed instruction bytes against the executable, and Capstone 5.0.7 checked
their x86 instruction lengths. Python `struct` decoded the selected file
fields, referenced strings, constants and vtable entries. No client ran.

| Input | Size | SHA-256 |
|---|---|---|
| Layout `data/7F/1F/00/00.DAT` | `9816` | `c0a0e6128d36946ba32f8980ad80d39fd3aec990f3fc8a18742bbd8851a76b48` |
| Model `data/7F/1C/00/00.DAT` | `41088` | `1af91b0e1e5e34f676f1af2884f82eddb63062e62bee058b0260e9646dfb7d0f` |
| Authored model | `41088` | `c567537529a31cbd1688102477972c8449e8a10cd3df29850c1e5fac9c95e160` |

The selected layout LYB payload begins at file `0x190`. The
[BGChip binding](bgchip-model-binding.md) and
[instance setup](bgchip-instance-resource-setup.md) own its native resource
and scene handoff. File offsets in this page include all resource headers.

## Placement and block bounds

Each selected Instance's `+0x3C` points to UnitTree file `0x1840`.
Its child at file `0x1890` points through `+0x18` to BGChip file `0x1E60`.
The Instance translation is at `+0x20`; rotation and scale pool references
are at `+0x2C/+0x30`. The zone at file `0x14C0`, the UnitTree and this
child have zero translation/rotation and unit scale. The four Instances
also have zero rotation and unit scale.

The folder's `+0x10/+0x14` array/count pair reaches wrappers whose first
dword is an Instance pointer. The following table records that membership
and the bounds of the authored local tile `[-16,0,-16]..[16,0,16]` after
the unchanged translations.

| Instance file offset | Translation | DivideMap folder file offset | Authored world bounds |
|---|---|---|---|
| `0x1960` | `(16,0,-16)` | `0x2460` | `(0,0,-32)..(32,0,0)` |
| `0x19A0` | `(16,0,16)` | `0x24D0` | `(0,0,0)..(32,0,32)` |
| `0x19E0` | `(-16,0,16)` | `0x2540` | `(-32,0,0)..(0,0,32)` |
| `0x1A20` | `(-16,0,-16)` | `0x25B0` | `(-32,0,-32)..(0,0,0)` |

These four folders have byte `+0x62` bit 0 clear. Their raw `+0x30/+0x40`
minimum/maximum vectors contain small seam deviations: a nominal zero
minimum can be `9.5367431640625e-7`, and a nominal zero maximum can be
`-1.9073486328125e-6`. Consequently the exact authored seam vertices are
outside the raw serialized boxes. For example, `(0,0,0)` is outside the
box at file `0x24D0` because its X/Z minima are positive.

At `0x006426D1-0x0064277F`, `0x006426C0` reads those serialized vectors,
subtracts/adds the double at `0x00FBF9D0`, then stores float vectors at
BlockAABB `+0x20/+0x30`. The double is `0.009999999776482582`.
Replaying these operations with float storage contains every authored
corner in its own expanded folder box, including the zero-height Y axis.
The union of the four placed tiles is `[-32,0,-32]..[32,0,32]`.

The native Instance getters preserve the distinction between serialized
placement and an effective render matrix. Instance vtable `0x00FF279C`
slots 24, 27 and 30 select `0x00A9ACC0`, `0x00A9AE30` and
`0x00A9AFB0`. They obtain the original Instance through slot 12 and read
translation `+0x20`, the rotation pointer at `+0x2C`, and the scale
pointer at `+0x30`. Their fallback branches obtain a child record through
slot 62, using child translation `+0x00` and rotation/scale pointers
`+0x0C/+0x10`. This corroborates the file-field interpretation above.

The matrix builder in slot 31, `0x00A9B0A0`, uses slots 28, 25 and 22
for scale, rotation and translation, calls `0x00652200`, and composes
matrices through `0x0061D050`. Those getters can select controller values:
`0x00A9AC30`, `0x00A9ADA0` and `0x00A9AF20` test slot 35,
`0x008003D0`, which checks whether the corresponding signed word at
instance interface `+0x24/+0x26/+0x28` is nonnegative. The zero rotations
and unit scales in the file therefore do not by themselves prove the
effective matrices passed to render setup or later updates.

### Placement matrix handoff

There is a connected conditional path from those file-backed getters to
the render update. `0x0064D5E0` obtains manager slot 91's layout-settings
translation at `0x0064D631`, constructs a translation matrix, and supplies
it to `0x007FF6D0` at `0x0064D6A9`. That routine calls instance slot 64
before base-object setup at `0x007FF738`, and again after setup at
`0x007FF7E6`. Slot 64 is `0x00A9AA60`. At `0x00A9AA88-0x00A9AA8C`,
it forwards the supplied parent matrix and instance interface to property
slot 21. The post-setup call can therefore update a newly created display
wrapper; the earlier call alone cannot demonstrate that wrapper exists.

For BGChip property vtable `0x00FF242C`, slot 21 is thunk
`0x007FB260`, which reaches `0x00A97050`. The property copies the
supplied parent matrix into its retained transform state, obtains instance
slot 31 at `0x00A970CD`, and composes the matrices through `0x0061D050`
at `0x00A970EC`. With a non-null display wrapper, its slot 5 receives
the result at `0x00A97101`. The concrete LayDefaultModelObject interface
vtable `0x00FBAD6C` maps slot 5 to `0x00625B20`. In the ordinary branch,
model slot 26 is `0x008DEC70`, which stores the matrix at model
`+0x50..+0x80`. The indexed instance branch instead uses
`0x00C398A0` and drawable `+0x374` state. These are conditional update
routes, not an observed choice for the selected floor.

The UnitTree path retains the parent composition. For base-object type
`0x12`, `0x007FF6D0` obtains the parent's instance slot 31 at
`0x007FF79B`, combines it with the supplied parent matrix at
`0x007FF7C2`, and calls `0x007FF5A0`. That routine obtains UnitInfo
slot 13, `0x00A9DF60`, and composes the UnitTree-local matrix at
`0x007FF5DE`. It supplies the result to child base-object setup slot 18
at `0x007FF650`, and recurses over nested UnitInfo records.
`0x00A9DF60` builds its SRT matrix from slots 10, 7 and 4. Their
file-backed branches read the normalized UnitTree transform at
translation `+0x00`, rotation pointer `+0x0C`, and scale pointer
`+0x10`; normalization added `0x10` to the original UnitTree address.

Later propagation is also connected. `0x00A9AA60` combines the parent
matrix with instance slot 31 at `0x00A9AAB8` and dispatches UnitInfo
slot 62 at `0x00A9AAD8`. That slot is `0x00A9E390`, which composes
UnitInfo slot 13 and dispatches each child instance's slot 64 at
`0x00A9E433`. A BGChip child consequently reaches the same property
slot 21, local SRT composition and display slot 5 described above.

On the file-backed branches, the selected zero ancestor translations,
zero rotations and unit scales reduce this composition to each Instance's
listed translation. This is a conditional static placement result.
Controller selectors, later mutations, allocation and setup admission,
and selection of the ordinary or indexed update route still require an
observation of the selected load.

`0x00641C10` takes BlockAABB in ECX and a point pointer on the stack,
returns squared point-to-box distance in x87 ST0, and pops four bytes.
With non-null BlockAABB `+0x08`, it first transforms the point through
that object's matrix at `+0x30` using `0x0060CE50` and uses its boxes at
`+0x10/+0x20`. Otherwise it uses BlockAABB `+0x20/+0x30` directly.
`0x0064C44E` calls this helper while selecting registered blocks. The
[block activation finding](layout-block-activation.md) owns its count,
lookup, queue and state guards. Containment does not prove selection.

## Model and mesh bounds

The selected WRB contains three six-float big-endian bounds records.
Each data body begins at chunk file offset plus `0x10`.

| Chunk file offset | Structural role | Authored minimum / maximum |
|---|---|---|
| `0x9CFC` | `MDL/MESH/AABB/AABB` leaf | `(-16,0,-16) / (16,0,16)` |
| `0x9D4C` | `MDL/AABB/AABB` leaf | `(-16,0,-16) / (16,0,16)` |
| `0x9D7C` | `MDL/AABB/COMP` | `(-16,0,-16) / (16,0,16)` |

All three original records had the same X/Z limits and minimum Y
`-3.552713678800501e-15`. The importer replaces all three with the
authored geometry bounds. This is a file observation; the `COMP` role in
position decoding does not establish it as an independent culling box.

The MESH resource builder at `0x00C8DC10` selects `MESH` through
`0x00C8F690`. At `0x00C8DE19-0x00C8DE41` it selects nested `AABB/AABB`,
passes the leaf body to `0x00C8F880`, and copies the decoded vectors to
that resource's `+0x10/+0x20` at `0x00C8DEC5/0x00C8DEC9`.
`0x00C8F880` byte-swaps six floats into two aligned vectors with W=1.
This is a resource-side box, distinct from the DrawableObject receiver.

The MeshContainer builder `0x00C8F160` separately selects `AABB/AABB`
at `0x00C8F22B-0x00C8F25B`, decodes it with `0x00C8F880` and stores
the vectors at container `+0x40/+0x50` at `0x00C8F2C3/0x00C8F2CE`.
It selects `COMP` and stores its decoded vectors at `+0x80/+0x90`
at `0x00C8F34F/0x00C8F356`. Constructor `0x00C8F470` installs
vtable `0x010F9568`, whose PE RTTI type is
`.?AVMeshContainer@Renderer@Dw@Engine@CDev@SQEX@@`.
At `0x00C75945-0x00C75993`, the caller creates that container and
stores it in its resource's `+0x4C` pointer vector.

On the runtime side, `0x00C39D90` iterates DrawableObject count `+0x368`
and the `0xB0`-stride records at `+0x36C`, obtaining each box through
`0x006BE8B0` and `0x00C337A0`. The latter reads a box at
`[record+0x7C]+0x40/+0x50`. At `0x00C337BD/0x00C337CB`, it loads the
matrix pointer stored at `[record+0x88]+0x10`, or uses an identity
fallback, and calls `0x0062E180`. That helper transforms
all eight AABB corners and takes componentwise minima/maxima. The union
is written to DrawableObject `+0x2F0/+0x300`; `0x00C39F50` also copies
it to `+0x70/+0x80`. Additional non-null `+0x374` state can transform
the union. `0x00C5013F-0x00C50147` obtains the model resource's
`+0x4C` vector entry through `0x00C517E0` and passes it to
`0x00C341D0`, which stores it at record `+0x7C` at `0x00C34206`.
This supplies the MeshContainer box to the union, rather than substituting
the raw MESH resource's `+0x10/+0x20` box.

### Component hierarchy matrix

The component matrix is distinct from layout placement. The model builder
selects `MICT` at `0x00C769B0-0x00C769B7`, copies it and retains its
pointer at resource `+0x98` at `0x00C76A03`. Drawable construction reads
that pointer at `0x00C4FF3A`. `0x00C885A0` byte-swaps its `+0x10`
component count; `0x00C88560` selects the indexed `MINS` child and
`0x00C88530` selects its `HEAD` body. In both original and authored
models, `MICT` at file `0x9DEC` has one component, `MINS` is at
`0x9E0C`, and its `HEAD` body at `0x9E3C` names node zero and mesh zero.
The MINS count of two at `+0x10` counts its `HEAD` and `NAME` children,
not two drawable components.

At `0x00C50069-0x00C50091`, construction byte-swaps that node index and
obtains a `0x1C`-byte Joint through drawable `+0x334`, its `+0x08`
skeleton pointer `S`, then `S+0x5C`. For an admitted non-null Joint,
`0x00C5012E` calls `0x00C33A50`, which stores it at component record
`+0x88`. The [Joint association](skeleton-joint-access.md) owns the
general constructor equations: `J_i = ptr32(S+0x5C) + 0x1C*i` and
`O_i = ptr32(S+0x44) + 0xA0*i`. Here `ptr32` reads a native pointer.
At `0x00A40CBA`, the constructor writes `ptr32(J_i+0x10) = O_i`.
Thus the selected node-zero component supplies `J_0`, and
`0x00C337CB` obtains `O_0` through its `+0x10` pointer. Its first
`0x40` bytes supply the component AABB transform. This is a hierarchy
output matrix; the pointer association does not use an RSTP carrier or
the layout placement matrix.

The selected RES directory slot 2 supplies the SKL block at file `0x9E90`,
size `0x124`, under path `F02\c0_00_plt00_h`. Its reserved
`RESOURCE_TYPE[2]` at file `0x9FBC` is `0x00736B6C`. The
[packed child construction](bgchip-instance-resource-setup.md#packed-resource-child-construction)
passes that directory-selected type and payload to the registered creator.
At `0x00A3C6BD-0x00A3C6EA`, initialization registers
SkeletonResourceNodeFactory vtable `0x01099F6C` under that exact key.
Its slot 3 is `0x00A44CC0`, whose admitted raw-memory branch calls
`0x00A45B80` at `0x00A44D59`. That constructor installs
SkeletonResourceNode main vtable `0x0109A93C`, metadata interface
`0x0109A92C` at full `+0x0C`, and retains the supplied backing resource
at full `+0x48`. Original PE RTTI confirms both class identities.

Metadata interface slot 1, `0x00A45E30`, returns `0x01323848`.
`0x00C5266E-0x00C52679` compares that record's ordinal with the resource's
metadata ordinal. Equality reaches `0x00C52681`, appending the resource
to the complete ResourceFinder's vector object at `+0x50`. Its begin/end
members are complete finder `+0x54/+0x58`, because `0x00858140` reads
vector `+0x04/+0x08`. These are complete-object offsets: constructor
`0x00C524A0` installs main vtable `0x010F5734`, whose slot 1 is
`0x00C52B20`; that body forwards its same entry ECX to `0x00C52560`
at `0x00C52C02`. The separate interface at finder `+0x04` does not
supply an additional receiver adjustment on this collection route.
`0x00C4F8E0` forwards its supplied finder pointer unchanged to
`0x00C4EC80` at `0x00C4FB76-0x00C4FB7C`; that selector reads the same
complete finder begin/end members and returns their sole entry when the
count is one. This connects the selected file-backed factory and class
admission conditionally to the skeleton resource selector.
Its caller retains the result, and a non-null result reaches
`0x00E12090` at `0x00C4FC98`; the returned CharacterSkeletonNode is
stored at drawable `+0x334`. `0x00E12090` calls `0x00A3EB90`, which
obtains a hierarchy container through `0x00A45C20`, then calls
`0x00A41430` with that one container. The latter calls `0x00A40AF0`,
and `0x00A3EC26` retains the resulting skeleton at CharacterSkeletonNode
`+0x08`. `0x00A45C20` obtains resource memory and passes it through
`0x00E0DE10` to `0x00E0DD00`. The
[SKL framing](skeleton-bone-framing.md) and
[constructor-pose finding](skeleton-constructor-pose.md) own the general
record, pose and hierarchy machinery.

The selected model's embedded `SEDBSKL ` block at file `0x9E90` has
scalar 7000, byte `+0x0C` zero, one `0xB0`-byte record at `0x9EE0`,
and a counted string pool beginning at `0x9F90`. Its sole record has
translation `(0,0,0)`, quaternion `(0,0,0,1)`, scale `(1,1,1)` and
parent `-1`: identity root-pose inputs. Its flags dword is `2`.
The composition flag mask `0x10` is clear, so the constructor leaves
skeleton `+0x52` flag mask `0x02` clear and `0x00A40990` calls `0x00A3FF70` at
`0x00A40ABA`. That producer stores the hierarchy matrix rows at
`O_0+0x00/+0x10/+0x20/+0x30` at
`0x00A40420/0x00A4042E/0x00A40436/0x00A4043E`. The constructor's
root parent refers to the preceding output record, initialized to identity
before the exposed `S+0x44` base advances by `0xA0`.

The conditional path connects the selected RES entry, concrete factory,
metadata admission, sole-resource selection, constructor output pointer
and component AABB consumer. Allocation, registration, tree construction,
collection success and the sole-entry guard remain conditions; no selected
load was observed. Later skeleton or animation updates and effective
matrix values also remain unobserved. Those later choices do not change
the identified pointer association or make it a world placement matrix.

## Distance culling and pixel clipping

The selected BGChip category and its base-object type both return one.
The category selects its creator, as described in the
[binding trace](bgchip-model-binding.md). Independently, base-object
vtable `0x0106126C` slot 1 is `0x007254A0`, whose original instructions
return EAX=1. In `0x007FF140`, the type-minus-one selector at
`0x007FF1C4` and jump table at `0x007FF1B0` send type 1 to
`0x007FF163`. That branch calls `0x007FEAC0` with a true argument.
Type 2 instead reaches `0x007FF16E`, subtracts `0x4C` from the retained
property interface and tail-calls `0x007FD2E0`.

`0x007FEAC0` is a conditional layout distance rejection predicate.
`0x007FF280` and `0x007FF1E0` call the dispatcher and forward its
Boolean through instance slot 55. The latter caller additionally requires
base-object type 2, so it does not establish a BGChip call. The former
reaches the type-1 branch when its manager-vector count guard passes.
The predicate returns false without a distance test when manager
`+0x150` equals `0x1000`, its `+0x110/+0x114` vector count is at most
one, the owning UnitInfo is absent, its BlockAABB `+0x12` bit 0 is set, application
`+0x164` is null, or manager byte `+0xF4` is zero.

On the tested branch, `0x007FEB6D` obtains a far range through
`0x00624890` because the dispatcher supplied true. That helper selects
application `+0x1C8` when `+0x1BC` is set, otherwise manager slot 98,
with a fallback when no manager exists. The predicate adds application
`+0x1D0`, obtains the point from application `+0x164` slot 34, and
calls manager slot 93 at `0x007FEBD1`. For RaptureLayoutManager main
vtable `0x00FBFCEC`, slot 93 is `0x00A8BDF0`: it subtracts the
translation returned by slot 91, `0x00A8BD40`, from the point. Slot 91
reads serialized layout settings `+0x40`. The selected settings translation
is zero. At `0x007FEBEB`, the distance test `0x00628250` receives
that point, the owning BlockAABB at `+0x20/+0x30`, and the adjusted
range. `SETZ AL` at `0x007FEBF5` converts failure into rejection.
This path does not read the DrawableObject union or property
`+0xA0/+0xB0`.

### Type-2 property bounds

The [background visibility finding](../actor/background-visibility.md)
owns the four range getters and type-`0x1000` range selection.
`0x007FB470` obtains supplied ranges through interface slots `+0xC4`
and `+0xC0`, applies the `BGLOW` branch, then caps positive far and
floors positive near values through `0x006247B0/0x00624820`.
Other inputs include resource `+0x150` and application state, rather
than model file bounds alone.

`0x007FB370` uses ECX for its culling receiver and stack arguments
`(far, near, margin, point, box)`, returning AL and popping `0x14`.
For finite ordered inputs, its AL=1 rejection conditions include:

- Positive far: `0x00628250` returns false unless squared distance
  from point to box is strictly less than the square of
  `far + margin + application[+0x1D0]`.
- Positive near: effective near is
  `max(0, near - margin - application[+0x1D0])`. Rejection occurs
  when it exceeds `0x006280E0`'s result, which is closest point-to-box
  distance plus the box diagonal length.
- Receiver byte `+0xC0` enables a further predicate at `0x00624F00`.

At `0x007FD4B1`, `0x007FD2E0` calls that helper with zero margin,
the point obtained from application `+0x164` virtual slot `+0x88`,
and its own box at `+0xA0/+0xB0`. Its earlier guards include the
owning block AABB flag at `+0x12` bit 0 and resource byte `+0xF4`.
Those are conditional call inputs, not observed selected-floor values.

The type-2 box has a connected producer. At `0x006239DD`,
`0x006239C0` first calls `0x00A612C0`. Model slot 82 resolves to
`0x006576B0`, returning model `+0xD8`; `0x00C3A090` then calls
`0x00C39D90` with destination DrawableObject `+0x70/+0x80`.
Only when base-object slot 1 returns two does `0x006239F8` admit the
property copy. It obtains instance slot 47, subtracts `0x4C`, calls
`0x00A61AF0` and sends the result to `0x007FB320` at `0x00623A28`.
`0x00A619B0` supplies drawable `+0x70/+0x80` to `0x00A61AF0`,
which transforms the box with model slot 25, `0x008DE790`, before
`0x007FB320` copies it to property `+0xA0/+0xB0`. Optional model
scale and drawable instance state remain part of those calculations.

Update ordering is explicit in `0x00625B20`. Its instance branch calls
`0x00C398A0` before `0x006239C0`; its ordinary branch dispatches
model slot 26 and slot 7 before `0x006239C0`. `0x00C398A0` requires
drawable `+0x374` and forwards to `0x00C3C7E0`, which stores the
matrix in the indexed `0x30`-byte record and marks its state dirty.
These instructions close the type-2 producer-to-predicate connection.
The selected BGChip's type-one result skips this property copy.

### Pixel clipping

The cataloged `BgPlateActor` vtable VA `0x00FBA97C` slots 161/162 are
`0x0062A330/0x0062A740`. Both use slot 71 (`0x00CB1C80`) to obtain
actor `+0x0C`, then access that ModelObject's `+0xD8` DrawableObject.
Their final writes target DrawableObject `+0x2CC` for far and `+0x2C8`
for near. Embedded assertion names identify `SetZPixelClipFar` and
`SetZPixelClipNear`. Initial candidate values are `10000.0` and
`0.10000000149011612`; branches use actor `+0x124/+0x128`, application
bytes `+0x1EB/+0x1EA`, range getters, position-derived adjustment and
other application fields before those writes. They are not fixed
selected-floor distances, and these writes alone do not prove a final
pixel result.

## Authoring consequence and remaining boundary

The selected floor needs no layout bound or translation adjustment for
offline containment after the observed expansion. Preserve the existing
transforms and folder bytes. The experimental importer updates the three
model bounds records; it does not rewrite layout boxes, distance ranges
or pixel clip state. An authoring profile with different extents must
repeat the per-placement expanded-box check instead of relying on the
selected floor result.

The placement path above connects serialized inputs and parent composition
to display, model and drawable-instance update consumers. Its file-backed
branches support the listed translations, but do not establish the
selected load's controller values or effective render matrices. The
component hierarchy route also connects the selected SKL entry through
its concrete factory and constructor to the component bounds consumer,
subject to the stated construction and collection conditions. The type-2
property box is not a missing selected-BGChip edge; its layout distance
predicate uses the BlockAABB described above. Runtime evidence must also
establish selected
block activation, camera/point identity, effective range/clip values and
draw acceptance. No visibility, standing or walking result follows from
the offline containment proof.
