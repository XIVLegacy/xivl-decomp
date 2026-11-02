# Cut camera CCB container and rig evaluation

A cut scene file carries one `SEDB` container of subtype `CCB` per camera shot.
The container embeds an [MTB](mtb-container-framing.md) of per-group curves
and a small table that names which groups form the camera rig. This page owns
the container layout, the MTB property-name table that the layout depends on,
the evaluation of the rig at a frame, and how a LocalCameraClip selects a
container. The clip records are in the
[clip payloads](cut-scheduler-clip-payloads.md#localcameraclip) and the
[clip timing](cut-scheduler-clip-timing.md#clip-start-and-end) pages.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. Addresses are VA.

Producing tools are `ghidra` CLI 0.2.2 driving Ghidra 12.1.4 decompile,
disassembly, memory-read and cross-reference queries, and Python 3.14
standard-library byte reads over a contributor-supplied install whose
`game.ver` reads `2012.09.19.0001`. Decompiled text was a lead. Every offset,
constant and order below was read from the instructions or the data bytes at
the cited address. Install counts record numbers only. No resource bytes are
recorded.

The labels are STATIC (read from the executable at the cited address),
OBSERVED (a measurement over the install, reproduced by
`tools/verify_cut_camera.py`), INFERRED and UNKNOWN.

## MTB property names

The Curve property selector `0x00a30110` clears the lowest set bit of the
section mask in a fixed order and returns a string from a table of 14
`std::string` objects at `0x01365160` with stride `0x1c`. The table is empty in
the file image. The initializer at `0x00a2fe50` (guard byte `0x01365158`) fills
it from the 14 string literal addresses it stores at `[ESP+0x2c]` through
`[ESP+0x60]`, in table order. The literal bytes were read at the addresses
below. This replaces the statement on the
[framing page](mtb-container-framing.md#curve-record-header) that no bit has a
name.

| Mask bit | Table slot | Selector branch | Literal VA | Name |
|---:|---:|---|---|---|
| `0x0001` | 0 | `0x00a30113` | `0x01096320` | `tx` |
| `0x0002` | 1 | `0x00a30134` | `0x0109631c` | `ty` |
| `0x0004` | 2 | `0x00a30155` | `0x01096314` | `tz` |
| `0x2000` | 3 | `0x00a30176` | `0x01096318` | `rx` |
| `0x4000` | 4 | `0x00a2c6f0` | `0x01096310` | `ry` |
| `0x8000` | 5 | `0x00a2c710` | `0x0109630c` | `rz` |
| `0x0010` | 6 | `0x00a2c730` | `0x01096308` | `sx` |
| `0x0020` | 7 | `0x00a2c750` | `0x01096304` | `sy` |
| `0x0040` | 8 | `0x00a2c770` | `0x01096300` | `sz` |
| `0x0200` | 9 | `0x00a31b80` | `0x01099400` | `qx` |
| `0x0400` | 10 | `0x00a31ba0` | `0x010993fc` | `qy` |
| `0x0800` | 11 | `0x00a31bc0` | `0x010993f8` | `qz` |
| `0x1000` | 12 | `0x00a31be0` | `0x010993f4` | `qw` |
| `0x0100` | 13 | `0x00a31c00` | `0x010993ec` | `qrot` |

STATIC. The name read by a consumer is matched by `0x00810fe0` (a NUL-terminated
compare against each curve name of a group) and the match is sampled through
slot 3 of the curve object. The camera evaluator below asks for `tx`, `ty`,
`tz`, `rx`, `ry`, `rz`, `sx`, `sy` and `sz` with the same literals
(`0x01096320` and its neighbours), which ties the table to its use.

## StepCurve record

Property type 7 (`0x00a2e2e0`, class `StepCurve`, vtable `0x00c98d58` RVA) reads
a flag byte, a u16 key count n at +1 and then n keys of two f32 values from
+3. The loader `0x00a35e70` stores the second f32 of a key at key offset +0
and the first at +4, so the first f32 is the key time and the second is the key
value. STATIC. The scalar sample `0x00a35ce0` (slot 3) returns the first
value for an `x` at or before the first time, the last value for an `x` at or
after the last time, and otherwise the value of the key whose time is the
greatest at or below `x`. When the count has bit 31 set (the loader passes the
flag byte as the constructor's loop argument) the sample first subtracts the last
key time from `x` while `x` is not below it. The record is `4 + 8n` bytes,
matching the [framing page](mtb-container-framing.md#property-record-sizes).

## Container layout

C is the address of the `SEDB` signature. OBSERVED over every CCB of the install
(count below): all fields hold in each one, and the embedded MTB parses with
the framing page rules.

| Offset | Width | Field |
|---|---|---|
| `C+0x00` | 8 | `SEDB` then `CCB` NUL |
| `C+0x0e` | u16 | header size, `0x30` |
| `C+0x10` | u32 | container size from C, including the embedded MTB and every chunk |
| `C+0x30` | | chunk `@FCHD`, `0x30` bytes. Its payload is zero in the install |
| `C+0x60` | | chunk `@FCCO`, `0x20` bytes |
| `C+0x80` | | chunk `@FCMT`, `0x10` bytes |
| `C+0x90` | | the embedded `SEDB` `mtb` container |
| `C+0x80+u32(C+0x8c)` | | chunk `@FCPA` |
| after the camera records | | chunk `@FCIL` |

Each chunk starts with the five bytes of its `@FCxx` tag, a zero byte and a
u16 at +6. The executable holds `@FCHD` once in its data (`0x01097424`,
followed by `SEDB`) and no cross-reference to it. None of the other tags occurs
in the executable, so the reader navigates by offset and the tags are a
property of the data. The layout is therefore OBSERVED, and the fields below
are STATIC only where a read address is cited.

`@FCCO` (the object pointer at CameraObject +0x10, read at `0x00a112f0`):

| Offset in chunk | Width | Field | Read at |
|---|---|---|---|
| +0x06 | u16 | group index of the camera selector group | `0x00a112ff` |
| +0x08 | u32 | duration in ticks. OBSERVED equal to 10000 times the MTB Header payload's f32 at +4 in every container | not read by the evaluator |
| +0x0c | f32 | frame offset added to the sample time | `0x00a112f6` |

`@FCMT` holds the byte length of the embedded MTB at +8 and the offset of the
`@FCPA` chunk from the `@FCMT` chunk at +0x0c. OBSERVED in all containers: the first equals the
distance from the MTB start to the end of its last name string, and the second
reaches a 16 byte boundary after it.

`@FCPA` has the camera count n at +6 and n records of `0x50` bytes from +0x10
(the array base is stored at CameraObject +0x18). `@FCIL` follows and holds u16
group indices that the records point to. The record array index is the
selector value below.

| Record offset | Width | Field | Read at |
|---|---|---|---|
| +0x00 | 16 | camera name, NUL terminated | OBSERVED |
| +0x20 | f32 | copied to CameraObject +0x28. 14.003 in every record (film height) | `0x00a113de` |
| +0x24 | f32 | copied to CameraObject +0x24. 24.892 in every record at three decimals (film width) | `0x00a113d1` |
| +0x28 | f32 | copied to CameraObject +0x2c (near) | `0x00a113e4` |
| +0x2c | f32 | copied to CameraObject +0x30 (far) | `0x00a113ea` |
| +0x30 | u8 | node count of chain A | `0x00a114bd` |
| +0x31 | u8 | node count of chain B | `0x00a11f19` |
| +0x32 | u8 | node count of chain C | `0x00a11a43` |
| +0x34 | u32 | chain A list offset from the `@FCPA` chunk | `0x00a11423` |
| +0x38 | u32 | chain B list offset | `0x00a11433` |
| +0x3c | u32 | chain C list offset | `0x00a1143f` |
| +0x40 | u16 | group index of the focal length group | `0x00a113f0` |
| +0x42 | u16 | group index of the roll group | `0x00a11405` |

The list offsets are added to CameraObject +0x14 (`0x00a1142a`) and the lists
are runs of u16 group indices. The code that stores +0x14 was not read.
OBSERVED: measured from the `@FCPA` chunk start, every list of every record
lies inside the `@FCIL` chunk. The names of the film, near and far roles
are INFERRED from the values (the pair is a 16 to 9 gate in millimetres, the
near values are 0.1 or smaller). The evaluator copies the four floats and the
rest of the pipeline was not traced, so how a consumer builds a projection from
them is UNKNOWN.

## Evaluation at a frame

`0x00a112e0` is the per-update evaluator of the CameraObject (vtable `0x00c96354`
RVA, constructor `0x00a125c0`). Its argument is a pointer to the elapsed ticks as
a signed 32 bit integer. STATIC:

- Frame. `frame = f32(f64(ticks) / 10000.0) + frame offset` (`0x00a112fb` to
  `0x00a11326`). The divisor is the double at `0x00fe0570`. The clock therefore
  counts 10000 ticks per frame, as the
  [timing page](cut-scheduler-clip-timing.md#scheduler-time) observes for the
  clip starts.
- Group lookup. A group index g selects the g-th Curve section of clip 0 of the
  embedded MTB (`0x00a2b6f0`, arguments clip 0 and the u16 index). OBSERVED:
  the Curve sections run in group order in every container, so the section
  ordinal is also the group-name index. The named curve is then found by name
  (`0x00810fe0`) and sampled at the frame through its slot 3 (`CALL EAX` with
  the vtable slot at offset +0xc, `0x00a113b6`).
- Camera selection. The selector group's `tx` curve is sampled and converted to
  an integer by `0x009d6600` (a float to integer rounding). The result indexes
  the record array: `record = array + 0x50 * index` (`0x00a113bd` to
  `0x00a113c3`). The bound is not checked. OBSERVED: the selector group is
  group 1 in all 14,795 containers and its only curve is a `tx` step curve. It
  has one key in 14,770 containers, two in 15, three in 5 and four in 5. Key
  values are 0 (14,791 keys), 1 (28), 2 (11) and 3 (5), and none reaches the
  camera count of its container.
- Focal length. The `ty` curve of the focal group, or 35.0 (`0x00a1147b`,
  float `0x00fa7adc`) when the group has no `ty`. Stored at CameraObject +0x34.
  OBSERVED: every focal group holds a `ty`, as a constant in 12,851 records, a
  type 2 curve in 1,863 and a type 4 curve in 127. The constants run from 10.0
  to 300.0.
- Roll. The `rx` curve of the roll group, multiplied by the double
  0.017453292519943295 at `0x00fef0c0` (`0x00a1149f`), or 0 when absent. Stored at
  CameraObject +0x38.
- Nodes. For each group index of a chain the evaluator reads the curves by name
  and builds matrices with `0x00430430` (translation in the fourth row of a
  4 by 4 row-vector matrix, `0x00430350` (diagonal scale), and `0x0062ddc0` (rotation
  from three angles in radians). The row-vector product helper `0x0042ec50`
  multiplies a row by the matrix in ECX. Chain A nodes use `tx ty tz`, `rx ry rz`
  and `sx sy sz`, and the angle curves are in degrees (the same double
  constant converts them). Chain B and C nodes use only `tx ty tz`.
- Composition. For a chain A node the matrix is `N = S * R * T` in row-vector
  form (calls at `0x00a116ff` to `0x00a1183b` multiply in that order with the
  translation matrix as the multiplicand), and the accumulator after each
  node is `acc = N * acc` (`0x00a118b3` to `0x00a11906`) from an identity
  accumulator (`0x00a11450`). The first listed node is therefore the outermost
  parent. Chain C builds translation matrices with `0x00430430` and
  multiplies them with `0x0042ec50` in the loop at `0x00a11a70`, and chain B
  does the same in the loop at `0x00a11f3c`. That both start from the chain A
  result is INFERRED from the copies of the accumulator at `0x00a119c0` to
  `0x00a11a35` and from the actor fit below, and the register-level flow of
  both loops was not traced end to end.

INFERRED from names and values and not from a read: the translation row of the
chain A result after chain B's nodes except the last is the eye, the translation
row after chain C is the interest point, and the last chain B node is an up
point relative to the eye. OBSERVED support: with this reading, the interest
point at frame 0 of 786 LocalCameraClip shots (the scene files at even
positions in name order, shots whose chain A rotates on two or more axes) lies
within 2.53 to 2.61 units (median, horizontal, over the six axis orders) of the
nearest [SetPosClip](cut-scheduler-clip-payloads.md#setposclip) actor position
of the same block, and within 5.09 to 5.23 with the rotation angles negated. The eye and interest units are the world units of the SetPosClip
positions (the two agree in the shots examined).

UNKNOWN or open:

- The rotation order of `0x0062ddc0`. It is built from a vectorised sine and
  cosine routine (`0x00622fd0`) whose lane layout was not decoded. The fit
  above does not separate the six axis orders (medians 2.53 to 2.61). A
  single-axis rotation, as the rigs mostly use, is not affected.
- The two translation vectors added to the chains at CameraObject +0x17c and
  +0x18c (`0x00a11e04` onwards), and what the pose is copied to afterwards
  (`0x00833160`). Their defaults were not read.
- How a consumer maps the focal length and the film gate to a field of view,
  and how it applies the roll and the up point.
- Whether chain C is parented to the chain A result alone (taken here) or to
  the result of chain B.

## LocalCameraClip selection

The begin method `0x00a0c740` reads `u16(D+0x14)` of the clip record and fetches
a resource with `0x00a20090`, then creates a camera resource node of type `ccb`
(literal `0x636362`) from the bytes at the resource's data pointer
(`0x00a196a0`). The resource table behind `0x00a20090` was not traced.

OBSERVED: counting the distinct `u16(D+0x14)` values of a scheduler and sorting
them ascending gives the file order of its CCB containers one to one in 657 of
the 663 schedulers that hold a LocalCameraClip, so the value is a resource index
whose camera entries are the CCB containers in file order. The other six
schedulers have more containers than distinct values. For the 1,538 clips with a start
above 0 in those schedulers, the end word minus the start equals the mapped
container's duration in 879 and the end word itself equals it in 61. The
`CameraClip` class (rank 27 in the [census](cut-scheduler-clip-class-census.md))
has a 36 byte record and was not decoded.

## Census

`tools/verify_cut_camera.py --install <game-root>` parses every CCB under
`client/cut` and compares the totals with `config/cut_camera.json`.

| Quantity | Count |
|---|---:|
| Scene files read | 689 |
| CCB containers | 14,795 |
| Containers with 1, 2, 3 and 4 camera records | 14,766, 17, 7, 5 |
| Constant curves | 518,471 |
| Type 2 (fixed) curves | 42,393 |
| Type 4 (linear 16) curves | 3,580 |
| Type 7 (step) curves | 14,795 |
| Containers with a nonzero frame offset | 337 |
| Containers whose Curve sections run in group order | 14,795 |
| Distinct film gate pairs | 2 |

The most common node counts are 2, 3 and 2 for chains A, B and C
(13,576 records), then 2, 2 and 1 (409). No container holds a curve type other
than 0, 2, 4 and 7, and every property is named by a mask bit.

## Verification

```
python tools/test_cut_camera.py
python tools/verify_cut_camera.py --install <game-root>
```

The first builds a synthetic container and checks the layout, the name table
and the refusals. The second reads the install. Neither runs the client, so
the evaluation order above is a static reading plus the actor fit.
