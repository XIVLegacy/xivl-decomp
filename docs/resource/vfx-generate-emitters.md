# Qix emitters and generated motion

A veff effect spawns child units with the Generate controls. This page records
what the installed effects use, where the random draws come from, how the
Genesis graphs turn time into events, what the spawn handlers do with an event,
how the area shapes and the receiver controls turn the spawn payload into
motion, and the particle batch controls of the ManyGenerate family. It ends with
the math a renderer needs.

The tables the controls belong to are on the
[control module registry](vfx-control-module-registry.md) page, the arrays they
read are on the [effect data body](vfx-effect-data-body.md) page and the frame
update is on the [effect runtime](vfx-effect-runtime.md) page. The
[DrawResource page](vfx-drawresource-and-shaders.md) owns what a child unit
draws. The machine-checked form of every STATIC fact is the `generate` section
and the `install.emitters` census of
[`vfx_effect_runtime.json`](../../config/vfx_effect_runtime.json), checked by
`tools/verify_vfx_effect_runtime.py`. The parser is `tools/vfx_emitter_body.py`
and the reference math is `tools/vfx_emitter_math.py`. The identity and method
section of the [container page](vfx-qix-resource-container.md#identity-and-method)
applies here unchanged, and so do its tier labels. Addresses are virtual
addresses of the pinned `ffxivgame.exe`.

## What the install uses

The census counts control nodes and the effects that hold at least one. The
install holds 1,069 effects of version `0x1d` and 722 of them hold an emitter
control.

| Control | Nodes | Effects |
|---|---:|---:|
| `GenerateMaster` | 1,086 | 600 |
| `ManyGenerateFormSphere` | 191 | 145 |
| `ManyGenerateUnitTime` | 191 | 145 |
| `ManyGenerateMotionEmission` | 189 | 143 |
| `ManyGenerateDrawLine` | 183 | 140 |
| `ContinualPolygonNormal` | 96 | 86 |
| `GenerateTimeArea` | 85 | 62 |
| `GenerateTime` | 40 | 39 |
| `GenerateTimeVertex` | 20 | 18 |
| `ManyGenerateContinualPolygonNormalCylinder` | 4 | 3 |
| `ManyGenerateDrawObject` | 4 | 3 |
| `ManyGenerateDrawSimpleContinualPolygon` | 3 | 3 |
| `ManyGenerateMotionSphereRotation` | 2 | 2 |
| `ManyGenerateDrawPoint` | 1 | 1 |

The receivers that a spawn feeds are `Position3DGenerated` in the coordinate
variants Root (574 nodes), Local (352) and World (218), `Angle3DGenerated` in
Root (165), Local (28), World (16) and PureWorld (2), `Scale3DGenerated` in
Local (7) and Root (12), and `Position3DMapBindGenerated` Root (2). The registry
page counts declarations, so it lists 605 `GenerateMaster` effects. The census
counts nodes.

Every spawner holds a Genesis slot. The Genesis values belong to
`GenerateMaster` (1,086), `ManyGenerateUnitTime` (191), `GenerateTimeArea` (85),
`GenerateTime` (40) and `GenerateTimeVertex` (20), which is one value per node.
INFERRED from the install.

## Random source

STATIC. All emitter draws come from one MT19937 object. Its pointer sits at
`0x0137b780` and every draw loads it into `ecx` before calling `0x00e3f1b0`.

| Part | Address | Fact |
|---|---|---|
| Seed | `0x00bc3aa0` | The DefaultContext constructor calls `clock()` and passes the result to the generator constructor `0x00bc3980`. |
| Counter | `0x00bc39c4` | The constructor stores the address of the counter at `+0x9c4` into `+0x9c0`. |
| Fill | `0x00e3f040` | `state[0] = seed` and `state[i] = 1812433253 * (state[i-1] ^ (state[i-1] >> 30)) + i`. |
| Refill | `0x00e3f080` | The standard twist with matrix constant `0x9908b0df` and offset 397. Then it stores `0x26f` in the counter that `+0x9c0` points to. |
| Output | `0x00e3f1b0` | Decrements the counter, refills when the old value was -1, then tempers `state[0x270 - old]`. The masks are `0xff3a58ad` shifted left 7 and `0xffffdf8c` shifted left 15, which are the standard `0x9d2c5680` and `0xefc60000`. |

The counter makes the output index differ from the textbook order. After a refill
the first output is word 1, the 624th output tempers the pointer word at `+0x9c0`
(the address of the counter at `+0x9c4`) and the next call refills and outputs
word 0. A reimplementation may use a standard MT19937 because the seed is the
clock.

Draw conversions, STATIC from the constants and the instructions around
`0x00d9a741`:

| Draw | Rule | Constant |
|---|---|---|
| Unit | output word divided by 4294967295, stored as float. Range 0.0 to 1.0 | `0x010c6268` |
| Signed | output word as a signed integer times 2^-31, stored as float. Range -1.0 up to just below 1.0 | `0x010cfd40` |
| Integer up to n | `0x00e3f240` masks each output to the next power of two minus one and retries until `value - 1 <= n` as unsigned numbers. For n above 0 the value lies in 1 to n + 1 and a caller takes it modulo n + 1, which gives 0 to n | none |

## Graph time and the frame stage

STATIC. The graph update `0x00be4550` evaluates each polynomial binding at
`x = (unit time - segment start) / 5000`. The unit time is the tick word at
UnitNode `+0x8c`. It grows by the frame delta, which is scaled by the float at
`+0x94` when bit 6 of the flags at `+0x98` is set (`0x00bce8a4`). A new unit starts
at 0. One frame is 5000 ticks.

The walk starts at the segment the last update ended in and moves on while the
unit time has reached the next start tick. The start ticks compare as signed
integers, so the start `4294963412` of one installed binding is negative (a
one-off observation outside the census). At each boundary the module slot `+0x24`
runs and at the end the evaluator slot `+0x20` runs with `x`. For a polynomial
module `+0x24` is `0x00bec470`. It writes the constant term of the entered segment
plus the carried value. No traced path changes the carried value, because the
routine behind module slot `+0x28` (`0x00bec710`) has no traced caller. The
evaluator `0x00bec2c0` is the one on the [data body
page](vfx-effect-data-body.md#graph-records-and-polynomial-segments). A binding
whose start ticks do not rise walks one start at a time, so a segment is entered
only after every earlier start has passed.

Per unit node and frame the stage `0x00bc6690` runs a zero delta pre-pass and then
the graph update with the unit delta and time, the unit time advance with child
creation, the control calc pass and last the event pass `0x00bcc830` when bit 14 of
`+0x98` is set. STATIC. The root unit node takes `0x00bc6e30` instead. Which pass
first updates a unit created by a spawn is UNKNOWN.

A unit ends when its time reaches the end tick at unit entry `+0x28`. With bit 1
of the instance flags at `+0x0f` set, a time at or past the loop end
`+0x2c + +0x30` becomes `(time - loop end) % +0x30 + +0x2c` while the loops done
stay below the count at `+0x34`. STATIC at `0x00bce8a4` to `0x00bcea0e`.

The count byte is `0xff` in every installed unit. The check against it runs
first. On the `0xff` path the code stores no count and calls `0x00bb35d0` with
mask 1 on the object at UnitNode `+0x00`. The routine returns whether bit 0 of the
dword at `+0x28` of that object is set, and a true result ends the loop
(`0x00bce950` to `0x00bce96a`). The meaning of that bit is UNKNOWN, so whether a
looping instance repeats forever is UNKNOWN. The census counts 70 instances with
the loop bit (`loopInstances`). The end ticks of the 1,075 resolved
`GenerateMaster` children run from 25,000 (5 frames) through 1,135,000 (227
frames) with a median of 230,000 (`childEndTicks`). INFERRED from the install.

## Genesis graphs

A Genesis graph is a unit graph of module `StandardPolynomialGenesis` or
`StandardPolynomialSureGenesis`. The polynomial is the cumulative number of
events and not a rate. The value object keeps the constant term at `+0x00` and the
number already emitted at `+0x0c`. STATIC, `0x00becfa0` and `0x00bed1b0`.

| Rule | Where | Fact |
|---|---|---|
| Gate | `0x00becfdd`, `0x00bed1bb` | A nonzero dword at `[[value +0x04] +0x10]` makes the burst routine store the constant term and the elapse routine store `N(x)` into `+0x00`, and both emit no event. The meaning of that dword is UNKNOWN, and this page and the math below take it as zero. |
| First update | `0x00becfc0` | A degree 0 first segment emits the truncated constant term. |
| Entering a segment | `0x00becfc0` | A degree 0 segment emits the truncated constant term minus the number emitted. A segment of degree 1 or 2 emits nothing here. |
| Degree 1 | `0x00bed5c9`, `0x00bed774` | The peak count is `c0 + c1 * x` for `c1 > 0` and `c0 + c1 * max(x - span, 0)` otherwise. |
| Degree 2 without a quadratic term | `0x00bed2c2` | The peak is `c0 + c1 * x` for `c1 >= 0` and `c0 + c1 * (x - span)` otherwise. |
| Degree 2 | `0x00bed2fe` | With `h = c2 / 2` the peak is `N(vertex)` when the vertex `-c1 / (2h)` lies in `[x - span, x]` and is at least `N(x - span)`. Else it is the larger of `N(x - span)` and `N(x)`. |
| Emission | `0x00bed3fb` | No event when the peak is below the number emitted. Otherwise `n = trunc(peak - emitted)` events and the emitted count grows by `n`. |

`x` is the segment time of the sub-interval end and `span` is the length of the
sub-interval in frames. A step that crosses a segment start splits into one
sub-interval per segment. The coefficients use the layout of the polynomial
segments, so `N(t) = c0 + c1 * t + 0.5 * c2 * t^2`.

The event count of a step stays `n`. For a degree 2 segment with `c2 > 0` the
routine evaluates `N(x)` through `0x00beca90` and queues the first
`m = trunc(N(x) - emitted)` of the `n` events with one shared age
(`0x00bed44c`). The loop at `0x00bed4f0` then solves the ages of events `m + 1`
to `n` with the root solver `0x00becda0`. With `c2 > 0` the vertex is the
minimum of `N`, so the peak is the larger of `N(x - span)` and `N(x)` and `m`
does not exceed `n` apart from float rounding. The one exception is a vertex
that lies exactly on `x - span`: the peak is then `N(x - span)`, the `m` events
are still queued and the emitted count grows by `n` only. The reference math
does not model that case. The census counts 28 degree 2 segments with `c2 > 0`
(`risingQuadraticSegments`). A one-off check outside the census found no `c1 = 0`
among them. STATIC for the reads, INFERRED for the bound on `m`. The age is the
number of ticks between the event and the end of the step. No handler traced here
reads it, so only the event counts matter.

The installed forms, counted in bindings and segments:

| Module | Bindings | Segments | Degree 0 | Degree 1 | Degree 2 |
|---|---:|---:|---:|---:|---:|
| `StandardPolynomialSureGenesis` | 1,231 | 2,307 | 155 | 1,835 | 317 |
| `StandardPolynomialGenesis` | 191 | 358 | 30 | 322 | 6 |

The common shapes are `t0:d1[1, r] t(k):d1[n, 0]`, a straight count that reaches
`n` after `k` ticks, and `t0:d2[1, a, b] t(k):d2[n, 0, 0]` with a bent count. The
second segment holds the exact total, so the handoff emits the last whole event
that the float arithmetic left short. A sample is `t0:d2[1, 1.4478, -0.0807]
t40000:d2[10, 0, 0]`, which emits 10 events in 8 frames. The sure and the plain
form differ in the buffer that holds the events. The two buffers belong to the
GenesisManager `0x00bd4cf0` and the module id bit `0x10` selects the sure one.
INFERRED. The capacity of the plain buffer is UNKNOWN. The random graph variants
`RandomPolynomialGenesis` (1 binding) and `RandomPolynomialSureGenesis` (4
bindings) use other routines that are not read.

## Event handlers

STATIC. The enqueue `0x00bd48a0` stores the event with its age in one of the two
buffers of the effect. The event pass calls descriptor slot `+0x9c` of the control
node that owns the Genesis slot once per event, with the work buffer, the control
parameter block, a flag and the age (`0x00bdb780`). The handlers of the emitters:

| Control | Slot | Address | Role |
|---|---|---|---|
| `GenerateMaster` | `+0x9c` | `0x00d6ad10` | Spawn one child when the chance passes. |
| `GenerateTime` | `+0x9c` | `0x00d6b380` | Spawn one child at the emitter. |
| `GenerateTimeArea` | `+0x9c` | `0x00d6b540` | Spawn one child on a random shell. |
| `GenerateTimeVertex` | `+0x9c` | `0x00d6b890` | Spawn one child at a model vertex. INFERRED. |
| `ManyGenerateUnitTime` | `+0x9c` | `0x00d79bb0` | Count one particle with a chance. |

None of the handlers uses the age. A handler of a node that has no child unit
does nothing.

## GenerateMaster

The block of a `GenerateMaster` control holds a pointer to the record and a kind
byte at `+0x04`. The kind is 1 in all 1,086 installed blocks and the calc body
`0x00d6ad50` acts only on kind 2, so the calc does nothing. The record, STATIC:

| Offset | Width | Field |
|---|---:|---|
| `+0x00` | 12 | Spawn step. The handler adds it to the three floats at node work `+0x30` after every call. |
| `+0x0c` | 4 | Base chance, f32. 1.0 in 1,074 nodes. |
| `+0x10` | 12 | Camera gate: near, far and the chance beyond far. |
| `+0x1c` | 8 | Child reference: a pointer and a kind byte at `+0x20`. |
| `+0x21` | 1 | Bit 0 enables the camera gate. Set in 49 nodes. |
| `+0x24` | 8 | Area reference: a pointer to the area record and a kind byte at `+0x28`. |
| `+0x2c` | 4 | Chance spread, f32. 0.0 in 1,083 nodes. |

A child reference of kind 1 points to a cell that holds the payload offset of a
unit entry. 1,075 of the 1,076 kind 1 references land on a unit entry. Kind 2
points to a list of weights and child pointers and picks one by a unit draw times
the weight sum (`0x00d819f0`). 10 nodes use it and their pointer cells are not
resolved by the parser. UNKNOWN.

One event runs these steps in `0x00d9a710`. STATIC unless stated.

1. Draw a signed word `s`, then a unit word `u`. The chance is
   `p = base + spread * s`. The handler stops when `u > p`.
2. When the gate bit is set, the spawner position and the camera position give a
   distance `d`. The camera position is the translation of the inverse of the
   matrix that the effect context returns, INFERRED. With a far-chance of 1.0 or a
   `d` at or below `near` the spawn stays. Otherwise the chance is `far-chance` for
   `d >= far` and `1 - (d - near) / (far - near) * (1 - far-chance)` inside, and
   one more unit draw above that chance drops the spawn (`0x00d99da0`).
3. Resolve the child unit and create it (`0x00e3a3c0`). Without a child the
   handler logs an error and stops.
4. Sample the area record (below) into the payload.
5. Send the payload to the receivers of group 0, 1 and 2 (below).
6. After the call returns, the spawn step is added to the sum at node work
   `+0x30`. This also happens when the chance or the gate stopped the spawn.

The census of the 1,086 nodes: area kinds 0 (199), 1 (616), 2 (26), 3 (30), 4
(215), the surface bit in 231 spheres, 2 boxes and 22 cylinders. 1,004 of the
1,075 resolved children contain a DrawResource node. INFERRED from the install.
The child holds its own color, matrix and DrawResource nodes and the spawn adds no
other link, so a particle is drawn by the unit-local links of
[the DrawResource page](vfx-drawresource-and-shaders.md#the-drawresource-module).

## Area record and shapes

The area record starts with slot references. A slot reference is two s16 values,
the graph index in the unit list and the binding index in that graph. The area
reader `0x00d9a5c0` resolves the speed scalar at `+0x00` and the acceleration
scalar at `+0x04` for the payload. The reference at `+0x08` gives the drag factor.
The references at `+0x0c`, `+0x10`, `+0x14` and `+0x18` give the angular speed,
the angular acceleration, the scale speed and the scale acceleration. STATIC.

| Offset | Width | Field |
|---|---:|---|
| `+0x1c` | 1 | Distance scale flag. A nonzero value scales the sampled position by a camera dependent factor. The factor is not decoded. |
| `+0x1e` | 1 | Bit 0 selects the surface. Bits 1 to 6 are three pairs for the x, y and z axis. |
| `+0x20` | 4 | Sphere and cylinder: radius slot reference. Box: first spread float. |
| `+0x24` | 4 | Sphere and cylinder: radius jitter float. Box: second spread float. |
| `+0x28` | 4 | Cylinder: height slot reference. Box: third spread float. |
| `+0x2c` | 4 | Cylinder: height jitter float. Box: half extent slot reference. |

The sampler `0x00d99f50` draws the three axis values first, in the order x, y, z.
A flag byte with no bit in `0x7e` returns a zero position and direction. The pair
`(a, b)` for an axis gives a unit draw `u` as `2u - 1` for `(1, 1)`, `u` for
`(1, 0)`, `-u` for `(0, 1)` and 0 for `(0, 0)` (`0x00d999e0`). Each axis consumes a
draw even when its pair is `(0, 0)`.

| Kind | Name | Rule after the axis draws `v` |
|---|---|---|
| 0 | point | Position and direction stay zero. |
| 1 | sphere | Draw `j`. `r = radius + jitter * j`. Without the surface bit draw `k` and `r = r * k`. Position `normalize(v) * r`. |
| 2 | box | Find the axis of the largest value by comparing `abs(v[i]) >= best`, where `best` is the signed value kept so far. The surface bit sets that component to +1, or to -1 when it is not above zero. Then draw one unit word per axis. Component `i` is `(draw_i * spread_i + extent_i) * v[i]`. |
| 3 | cylinder | `angle = atan2(v.x, v.z)`. Draw `a` and `h` and set `r = a * radius` and `H = height + height_jitter * h`. With the surface bit the sample is a cap when the horizontal length of `v` is below `abs(v.y)`. The cap sets `y = +1` or `-1` by the sign of `v.y`. Otherwise `r = radius`. Draw `j` and add `radius_jitter * j`. Position `(cos(angle) * r, H * y, sin(angle) * r)`. |
| 4 | model vertex | Calls `0x00d996b0` with the model resource of node work `+0x2c`. INFERRED. The vertex choice is UNKNOWN. |
| 5 | unchanged | Position and direction stay zero. |

The direction is `normalize(position)` for the sphere and the box. A vector of
zero length stays zero. The cylinder direction is `normalize((x, 0, z))`. The
draw order inside kind 3 is `a`, `h`, `j`. The compare of the box takes the signed
best value, so a negative value found early makes any later axis win. The shapes
are STATIC at `0x00d9a164` to `0x00d9a50f`.

## Payload and receivers

A spawn builds one payload. The offsets are bytes. STATIC from the builder and the
three receivers.

| Offset | Width | Group 0 | Group 1 | Group 2 |
|---|---:|---|---|---|
| `+0x00` | 16 | position | euler angles | scale |
| `+0x10` | 16 | direction | unused | unused |
| `+0x20` | 16 | unused | speed | speed |
| `+0x30` | 16 | unused | acceleration | acceleration |
| `+0x40` | 4 | speed scalar | unused | unused |
| `+0x44` | 4 | acceleration or drag | unused | unused |
| `+0x48` | 1 | bit 0 drag mode | unused | unused |

Group 0 goes to the `Position3DGenerated` nodes of the child, group 1 to the
`Angle3DGenerated` nodes and group 2 to the `Scale3DGenerated` nodes. The control
entry holds a list per group at `+0x08` as 8 byte pairs of a pointer and a count.
A list holds u16 indices into the node order of the child unit. The census confirms
the module of every listed node, with 939 position, 185 angle and 19 scale
receivers among the resolved `GenerateMaster` children.

The group 0 payload carries the area sample (`+0x00`, `+0x10`) and the scalars
(`+0x40` is the speed reference value and `+0x44` the acceleration reference
value). When the drag reference is not 1.0 the builder sets bit 0 of `+0x48` and
stores the drag factor in `+0x44`. It then transforms position and direction by the
coordinate id of the first group 0 receiver, read from descriptor `+0x58`
(`0x00e3a360`, `0x00d98f00`):

| Id | Coordinate | Transform |
|---:|---|---|
| 0 | World | Position by the spawner matrix, direction by its rotation part. |
| 1 | Root | The same, then by the matrix returned by `0x00420200` on the effect root. That routine is a 4x4 inverse. INFERRED. |
| 2 | Local | None. |
| 3 | PureWorld | None. |

Descriptor `+0x50` is 1 for every `Generated` receiver. The manifest lists the 16
receivers and the verifier compares both words with the image.

The group 1 payload holds the euler angles of the spawner matrix plus the spawn
step sum, the angular speed and the angular acceleration. The group 2 payload
holds the lengths of the first three matrix rows, the scale speed and the scale
acceleration. The matrix to angle conversion is `0x00823740`. INFERRED.

Each receiver keeps the payload at message type 2 and counts ticks from its own
construction. With `t = ticks / 5000`:

| Receiver | Output |
|---|---|
| `Position3DGenerated` | `parent + origin + dir * speed * t + dir * accel * t^2 / 2` |
| `Angle3DGenerated` | `parent + origin + speed * t + accel * t^2 / 2` |
| `Scale3DGenerated` | `parent * (origin + speed * t + accel * t^2 / 2)` |

The parent is the vector that the static link at work `+0x60` gives. The position
output has a fourth value of 1.0. STATIC at `0x00ba5260`, `0x00ba5cb0` and
`0x00ba6320`. In the drag mode the position receiver keeps the line
`L = origin + dir * speed * t`, a running sum and a factor that starts at 1.0. Each
update adds `(L - previous L) * factor` to the sum, then multiplies the factor by
the drag value and the output is `parent + sum`. The damping therefore applies once
per update and not per tick. STATIC at `0x00ba52a2` and INFERRED for the reading.

## Child units

A spawned child is a full unit node. `0x00bce540` creates it through the UnitNode
constructor `0x00bcd8c0`, which clears the time. Its own polynomial graphs, such as
the color, scale and alpha curves, run at that time by the rules above, so the
curves of a particle start at the segment 0 of each binding at the moment of the
spawn. STATIC for the creation and the time. The color, matrix and DrawResource
nodes of the child read the receiver outputs through unit-local links, and 1,004
of the 1,075 resolved `GenerateMaster` children hold at least one DrawResource
node. INFERRED from the install. The child ends at its end tick, see the section
on graph time.

## GenerateTime and GenerateTimeArea

`GenerateTime` has the record fields child pointer at `+0x0c`, gate flags at
`+0x10` and the camera gate floats at `+0x00`. Its handler tests the density factor
of the manager (`0x00e3a100`), then the camera gate, creates the child and sends a
zero payload to group 0 after the transform of the spawner matrix. The child starts
at the spawner position with zero speed and acceleration. STATIC. The density
factor is the float at manager `+0x2c`. It keeps a spawn when it is 1.0 or more and
otherwise with that chance. Its default and its writers are UNKNOWN.
`GenerateMaster` does not test it.

`GenerateTimeArea` reads a center at `+0x20`, a base radius at `+0x2c`, euler
angles at `+0x30` and a radius spread at `+0x3c`. Its routine `0x00d990c0` draws
three signed values times pi as angles and one signed draw for the radius spread
and composes a transform with the record angles and center. The spawned point and
direction come from that transform. INFERRED. The shell composition is not decoded
further. The census counts 84 of 85 nodes with a resolved child and all of them
use group 0 only.

## ManyGenerate batches

The census finds 181 units that carry `ManyGenerateFormSphere`,
`ManyGenerateMotionEmission`, `ManyGenerateUnitTime` and `ManyGenerateDrawLine`
together. INFERRED from the install.

`ManyGenerateUnitTime` is the trigger. Its construct `0x00d79b70` zeroes the
pending count and sets the mode byte at work `+0x18` to 1. Its event handler
`0x00d79bb0` draws a unit word and adds 1 to the pending count when the draw is
not above the float of its second slot. Its calc
`0x00d79b90` moves the pending count to the per-frame particle count at work
`+0x14` when the delta is nonzero. STATIC. The spawn engine `0x00d92650` reads that
count in mode 1 and creates that many particles per frame. Mode 2 spaces particles
along the travel distance of the emitter and no installed effect uses it.
INFERRED for the mode names.

Each particle gets its start state from two routines:

* `ManyGenerateFormSphere` (`0x00d8ff8d`): three draws of `0.5 * s` give a cube
  vector, the record center is added, bit 0 of `+0x1d` takes the absolute value of
  the y axis, the direction is the normalized vector and the radius is
  `rec[0x0c] + rec[0x10] * s`. The transform of the position into the effect is
  UNKNOWN.
* `ManyGenerateMotionEmission` (`0x00d8f410`): `speed = rec[0x0c] + rec[0x1c] * s`
  and `drag = rec[0x2c] + rec[0x3c] * s`. Then each velocity axis `i` is
  `rec[4i] + rec[0x10 + 4i] * (s * 0.5)` with a sign flip when a further signed draw
  times 3 is below 1, and each acceleration axis follows the same rule with the
  records at `+0x20` and `+0x30`. When the drag is exactly 1.0 the direction times
  the speed is added to the velocity. The draw order is speed, drag, then a jitter
  and a sign draw for each of the six axes. The record offsets follow from the work
  buffer offsets the routine reads and from the values 0.075, 0.025 and 1.0 that
  one installed record holds (a one-off observation outside the census). INFERRED,
  because that the work buffer is the veff record is not shown.

The step of every particle is `0x00d942a0`. With `dt = ticks / 5000` the position
becomes `p + (v + a * dt / 2) * dt` and the velocity `v + a * dt`. STATIC.

`ManyGenerateDrawLine` sets the lifetime of a line in `0x00d72fb0`. The lifetime
is `(rec[0x2c] + n) * 5000` ticks with `n` an integer in 0 to `rec[0x50]`. The
history length is `rec[0x0d] + m` with `m` in 0 to `rec[0x54]`. One installed
record holds 16 and 8 for the lifetime and 2 and 2 for the history (a one-off
observation outside the census), which gives 16 to 24 frames. The routine is
STATIC and the record offsets are INFERRED like those above. How the line is
drawn, the form transform, the clamps of the motion record at `+0x40` and the
other draw controls are UNKNOWN.

## Math for bahamut-client

1. Seed one MT19937 with the clock or any value. Convert a word to a unit draw by
   `word / 4294967295` and to a signed draw by `int32(word) * 2^-31`.
2. Keep one unit time per unit in ticks, starting at 0. Advance it by the frame
   delta times the unit time scale and evaluate every graph at
   `x = (time - segment start) / 5000`. Walk the segments one start at a time with
   signed starts.
3. For every Genesis binding of a unit keep a segment index and an emitted count.
   Emit nothing while the gate dword at `[[value +0x04] +0x10]` is nonzero (its
   meaning is UNKNOWN, treat it as zero). Each update emits
   `trunc(peak - emitted)` events per sub-interval with the peak rules above, and
   `trunc(c0 - emitted)` when a degree 0 segment is entered or first read. Deliver
   each event once to the control that owns the slot.
4. On a `GenerateMaster` event run steps 1 to 6 of the section above. Draw order
   matters only when a replay of the client is needed.
5. Sample the area kind with the three axis draws first. Build the payload and
   apply the coordinate id of the first position receiver.
6. Create the child unit. Its position, angle and scale come from the receiver
   formulas with `t` counted from the child's creation. Its color, matrix and
   DrawResource nodes read those outputs through their unit-local links.
7. End the child when its time reaches the end tick of its unit entry, or loop it
   by the loop fields when the instance sets the loop bit. Whether the loop
   repeats depends on the UNKNOWN flag test of the graph time section.
8. For `ManyGenerate` units count one particle per accepted event, give each
   particle the form and motion start state, step it by `p + (v + a * dt / 2) * dt`
   and end it after its line lifetime.

The functions of `tools/vfx_emitter_math.py` restate the arithmetic of steps 1 to 6
and 8 over plain numbers and the test `tools/test_vfx_effect_runtime.py` pins them
with literal values.

## Boundaries

* No effect was played back. Nothing here shows a rendered frame or a spawn
  sequence of the retail client.
* The frame delta, the unit time scale and the order in which a spawned child is
  first updated are UNKNOWN. The same holds for the random time scale that the
  UnitNode constructor `0x00bcd8c0` applies in some modes.
* The model vertex area, the weighted child list, the distance scale of the area,
  the `GenerateTimeVertex` handler, the matrix to angle conversion and the shell of
  `GenerateTimeArea` are not decoded.
* The distance based calc of `GenerateMaster` (block kind 2) is not used by the
  install and not decoded.
* The calc of `ContinualPolygonNormal`, the matrix controls that compose the
  generated position, angle and scale and the draw side of the ManyGenerate family
  are not read.
* The plain event buffer capacity and the density factor defaults are UNKNOWN and
  a value for them needs a CALIBRATION ruling.
* The census describes one 1.23b installation. It does not show which effects the
  client plays.

## Verification

```text
python tools/verify_vfx_effect_runtime.py --exe orig/ffxivgame.exe
python tools/verify_vfx_effect_runtime.py --install <game-root>
python tools/test_vfx_effect_runtime.py
```

`--exe` compares the byte checks and the coordinate ids of the receivers with
the pinned executable. `--install` recounts the emitter census, which includes the
loop-bit instances, the child end ticks and the rising quadratic segments. The
tests build a synthetic effect with a spawner, a Genesis graph and a child unit,
and pin the reference math against literal values.
