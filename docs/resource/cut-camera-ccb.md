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
`tools/verify_cut_camera.py` and held in `config/cut_camera.json`), INFERRED
and UNKNOWN. The tool measures with double precision arithmetic, so a fit
median can differ in the last digit from a single precision reader.

## MTB property names

The Curve property selector `0x00a30110` tests the bits of the section mask
in the order `0x1`, `0x2`, `0x4`, `0x2000`, `0x4000`, `0x8000`, `0x10`, `0x20`,
`0x40`, `0x100`, `0x200`, `0x400`, `0x800`, `0x1000`, clears the first one that
is set and returns a string from a table of 14 `std::string` objects at
`0x01365160` with stride `0x1c`. The first set bit in that order is not the
numerically lowest one: `0x2000` is tested before `0x10`, and `0x100` before
`0x200`. The table is empty in the file image. The initializer at `0x00a2fe50`
(guard byte `0x01365158`) fills it from the 14 string literal addresses it
stores at `[ESP+0x2c]` through `[ESP+0x60]`, in table order. The literal bytes
were read at the addresses below. The
[weather light page](weather-light-curve-container-shape.md#section-roles-and-property-names-v)
lists the same names by mask, and the
[framing page](mtb-container-framing.md#curve-record-header) leaves the names to
those two pages. The rows below are in table slot order.

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
| `C+0x30` | | chunk `@FCHD`, `0x30` bytes. Its 32 payload bytes at `C+0x40` are zero in 14,260 containers and nonzero in 535 (46 distinct contents). No reader of them was found |
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
| +0x30 | s8 | node count of chain A | `0x00a114bd` |
| +0x31 | s8 | node count of chain B | `0x00a11f19` |
| +0x32 | s8 | node count of chain C | `0x00a11a43` |
| +0x34 | u32 | chain A list offset from the `@FCPA` chunk | `0x00a11423` |
| +0x38 | u32 | chain B list offset | `0x00a11433` |
| +0x3c | u32 | chain C list offset | `0x00a1143f` |
| +0x40 | u16 | group index of the focal length group | `0x00a113f0` |
| +0x42 | u16 | group index of the roll group | `0x00a11405` |

The counts are compared as signed bytes: each chain starts with `CMP byte ptr
[EDI+0x3n], 0` and `JLE` past the loop (`0x00a114bd`, `0x00a11f19`,
`0x00a11a43`), and the chain A loop bound is `MOVSX` of the byte
(`0x00a11991`). A count of 0x80 to 0xFF therefore means no nodes. The install
maximum is 4. The list offsets are added to CameraObject +0x14 (`0x00a1142a`)
and the lists are runs of u16 group indices. The code that stores +0x14 was not read.
OBSERVED: measured from the `@FCPA` chunk start, every list of every record
lies inside the `@FCIL` chunk. The names of the film, near and far roles
are INFERRED from the values (the pair is a 16 to 9 gate in millimetres, the
near values are 0.1 or smaller). The evaluator copies the four floats and the
rest of the pipeline was not traced, so how a consumer builds a projection from
them is UNKNOWN.

## Evaluation at a frame

`0x00a112e0` is a CameraObject method that updates the pose. It is not a slot of
the CameraObject vtable, which is at VA `0x01096354` (stored by the constructor
`0x00a125c0` at `0x00a125e6`) and holds `0x00a12880`, `0x00a12870` and
`0x00a12850`. The LocalCameraClip update `0x00a0bfd0` (slot 2) calls it directly
at `0x00a0c119` on the object at clip +0x3c, and so does `0x00a0a9a0` at
`0x00a0aaef`. Its argument is a pointer to the elapsed ticks as a signed 32 bit
integer. In `0x00a0bfd0` that integer is the result of the clip's own vtable
slot at offset +0x24 (`CALL EAX` at `0x00a0c10b`), stored at `0x00a0c115`, which
the [timing page](cut-scheduler-clip-timing.md#clip-start-and-end) calls the
elapsed ticks. STATIC:

- Frame. `frame = f32(f64(f32(ticks)) / 10000.0)`, then the frame offset is
  added in double and the sum is rounded to f32 (`0x00a112fb` to `0x00a11326`).
  The first instruction is `CVTSI2SS`, so the integer tick count is rounded to
  single precision before it is widened, and a count above 16,777,216 loses
  low bits. The divisor is the double at `0x00fe0570`. The clock therefore
  counts 10000 ticks per frame, as the
  [timing page](cut-scheduler-clip-timing.md#scheduler-time) observes for the
  clip starts.
- Group lookup. A group index g selects the g-th Curve section of clip 0 of the
  embedded MTB (`0x00a2b6f0`, arguments clip 0 and the u16 index). OBSERVED:
  the Curve sections run in group order in every container, so the section
  ordinal is also the group-name index. The named curve is then found by name
  (`0x00810fe0`) and sampled at the frame through its slot 3 (`CALL EAX` with
  the vtable slot at offset +0xc, `0x00a113b6`). The lookup `0x00a2b6f0` returns
  0 when the clip number is not below the byte at +0x3c of the object, or when
  the index is not below the entry count of that clip. The evaluator then logs
  an error text starting `[ERROR] Camera:Switc` (`0x00a11345` to `0x00a11389`)
  and returns without updating the object.
- Camera selection. The selector group's `tx` curve is sampled and converted to
  an integer by `0x009d6600`. That helper truncates toward zero: it is the
  float to integer conversion of the C runtime, `CVTTSD2SI` when the dword at
  `0x0137b8e8` is nonzero and otherwise an `FISTP` sequence
  (`0x009d6636` to `0x009d66a9`) that corrects the rounded result back to the
  truncated one. Nothing is added before the call, so a sample of 0.6 or 0.9
  selects record 0 and a sample of -0.7 also selects record 0. The result
  indexes the record array: `record = array + 0x50 * index` (`0x00a113bd` to
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
- Roll. The `rx` curve of the roll group, multiplied by the double at
  `0x00fef0c0` (`FMUL` at `0x00a1149f`), or 0 when absent. Stored at
  CameraObject +0x38. The 8 bytes at `0x00fef0c0` are `00 00 00 a0 46 df 91 3f`,
  which is 0.01745329238474369: the single precision value of pi/180 widened to
  a double, and not the double nearest pi/180 (0.017453292519943295). The
  products at `0x00a11593`, `0x00a115bd` and `0x00a115e7` use the same
  operand.
- Nodes. For each group index of a chain the evaluator reads the curves by name
  and builds matrices with `0x00430430` (translation in the fourth row of a
  4 by 4 row-vector matrix), `0x00430350` (diagonal scale), and `0x0062ddc0` (rotation
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
point relative to the eye.

OBSERVED actor-position fit, `fit_medians` of `config/cut_camera.json`. It takes
frame 0 of 786 LocalCameraClip shots (the schedulers at even positions among the
sorted entries of `client/cut`, paired as described below, whose chain A
rotates on two or more axes) and measures the horizontal distance from a point
of the rig to the nearest [SetPosClip](cut-scheduler-clip-payloads.md#setposclip)
position (vector A) of the same block. The median is the upper middle value of the
sorted distances. The world units of the positions and the rig are taken to agree.

| Point and variant | Median |
|---|---:|
| Interest point, six axis orders | 2.45 to 2.55 (x then y then z: 2.455) |
| Interest point, angles negated, six orders | 4.99 to 5.14 |
| Interest point, no rotation at all | 5.20 |
| Chain A result alone (no chain B or C offsets) | 2.65 |
| Eye point | 2.95 |
| Interest point against the preceding block's positions (643 shots have one) | 3.19 |

The interest point is nearer the actors than the eye in 508 of the 786 shots.
The fit therefore supports the sign and size of the chain A rotation (about 2.5
against about 5.2 without it or with the opposite sign). It does not tell the
eye from the interest point, and it separates the interest point only weakly
from the chain A result alone and from the positions of the wrong block. The
role names above rest on the names of the groups and on the value ranges, not
on this fit.

UNKNOWN or open:

- The rotation order of `0x0062ddc0`. It is built from a vectorised sine and
  cosine routine (`0x00622fd0`) whose lane layout was not decoded. The fit
  above does not separate the six axis orders (medians 2.45 to 2.55). A
  single-axis rotation, as the rigs mostly use, is not affected.
- The two translation vectors added to the chains at CameraObject +0x17c and
  +0x18c (`0x00a11e04` onwards), and what the pose is copied to afterwards
  (`0x00833160`). STATIC: the constructor `0x00a125c0` sets +0x17c to +0x184
  and +0x18c to +0x194 to 0.0 and +0x188 and +0x198 to the float 1.0 at
  `0x00f54f70` (`0x00a1264c` to `0x00a12684`). Whether any other code writes
  them was not searched.
- How a consumer maps the focal length and the film gate to a field of view,
  and how it applies the roll and the up point.
- Whether chain C is parented to the chain A result alone (taken here) or to
  the result of chain B.

## LocalCameraClip selection

The begin method `0x00a0c740` reads `u16(D+0x14)` of the clip record and fetches
a resource with `0x00a20090`, then creates a camera resource node of type `ccb`
(literal `0x636362`) from the bytes at the resource's data pointer
(`0x00a196a0`). The resource table behind `0x00a20090` was not traced.

OBSERVED (`pairing` and `durations` of the manifest): counting the distinct
`u16(D+0x14)` values of a scheduler and sorting them ascending gives the file order
of its CCB containers one to one in 657 of the 663 schedulers that hold a
LocalCameraClip, so the value is a resource index whose camera entries are the
CCB containers in file order. Of the 657, 651 hold more than one container and 6
hold one. The other six of the 663 have more containers than distinct values and
are not paired. The pairing is a count equality of the distinct values and the
containers. Its order is checked only by a duration cross-check, which passes
when a clip's end word minus its start, or the end word itself, equals the mapped
container's duration. Over the clips of the 651 paired multi-container
schedulers:

| Clips | Count | End minus start equals the duration | End equals the duration | Neither, another container of the file matches | Neither, no container matches |
|---|---:|---:|---:|---:|---:|
| Start above 0 | 1,538 | 879 | 61 | 117 | 481 |
| Start 0 | 12,227 | 8,890 | 8,890 | 549 | 2,788 |

For a start of 0 the two rules are the same, so the 8,890 count once. The
cross-check therefore passes for 940 of 1,538 clips with a start above 0 and 8,890
of 12,227 with a start of 0, and it fails for 598 and 3,337. The ordering is
supported but not proven. The `CameraClip` class (rank 27 in the
[census](cut-scheduler-clip-class-census.md)) has a 36 byte record, read by the
[layout page](cut-scheduler-clip-layout.md#cameraclip). Its update evaluates the
same CameraObject at the elapsed ticks, and whether its resource index selects a
container of the same file was not measured.

## Census

`tools/verify_cut_camera.py --install <game-root>` parses every CCB under
`client/cut`, pairs them with the schedulers of the same files, runs the fit and
compares the totals with `config/cut_camera.json`. `tools/cut_camera_rig.py`
holds the curve samples and the rig evaluation that the fit uses.

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
| Containers whose Curve section count equals the group name count | 14,795 |
| Distinct film gate pairs | 2 |
| Camera records | 14,841 |
| Selector group index | 1 in 14,795 (a `tx` step curve alone) |
| Selector keys per container, 1, 2, 3 and 4 | 14,770, 15, 5, 5 |
| Selector key values 0, 1, 2 and 3 | 14,791, 28, 11, 5 |
| Focal `ty` curves, constant, type 2, type 4 | 12,851, 1,863, 127 |
| Focal constant range | 10.0 to 300.0 |
| Film gate at three decimals | 14.003 by 24.892 in every record |
| Near values | 0.0001, 0.001, 0.01 and 0.1 |
| Duration differing from 10000 times the Header f32 | 0 |
| `@FCMT` length differing from the end of the last name string | 0 |
| `@FCPA` not at the next 16 byte boundary after the MTB | 0 |

The most common node counts are 2, 3 and 2 for chains A, B and C
(13,576 records), then 2, 2 and 1 (409). No container holds a curve type other
than 0, 2, 4 and 7, and every property is named by a mask bit.

## Verification

```
python tools/test_cut_camera.py
python tools/verify_cut_camera.py --install <game-root>
```

The first builds a synthetic container and checks the layout, the name table,
the curve samples, the refusals and the manifest comparison. The second reads
the install, reproduces every OBSERVED figure on this page and compares it with
the manifest. Neither runs the client, so the evaluation order above is a
static reading plus the actor fit.
