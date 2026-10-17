# Floor model bounds and visibility inputs

The selected four-placement floor fits the native expanded layout block
bounds. Its authored model has matching local mesh, model and compression
bounds. These offline checks do not determine the selected camera, active
blocks, distance ranges or pixel clipping state.

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

## Distance culling and pixel clipping

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

The remaining static boundary is the connection from the DrawableObject
union to the culling receiver's `+0xA0/+0xB0`, including the effective
placement matrix. The file and consumer observations above do not close
that connection. Runtime evidence must also establish selected
block activation, camera/point identity, effective range/clip values and
draw acceptance. No visibility, standing or walking result follows from
the offline containment proof.
