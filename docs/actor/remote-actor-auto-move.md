# Remote actor auto-move

A remote actor (an NPC or another player) is moved by two server records.
Map clientbound `0x00D0` fills a per-actor speed table. Map clientbound
`0x00CF` supplies a gait level, a target position and a rotation. The client
does not place the actor at the `0x00CF` position. A chase step moves the actor
toward the target each timestep at the table speed of the gait level, and the
actor decelerates once it is within an arrival radius. This page records that
contract, its constants and the points that remain open. The
[locomotion selection finding](locomotion-selection.md) owns the animation
level and state machine that the chase feeds. The
[wire census](../net/move-actor-wire-census.md) owns the capture counts.

Labels. STATIC means the cited instructions and data state it. OBSERVED means
it was counted in the retail captures. INFERRED is an interpretation of STATIC
evidence. OPEN is not established.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All addresses are VA.

Producing tools are `ghidra` CLI decompile and cross-reference queries and
Capstone 5.0.7 x86-32 disassembly of raw PE reads. Decompiler output was
treated as a lead and every cited branch was read from the disassembly. The
bytes and constants this page relies on are pinned as `staticChecks` rows in
`config/remote_actor_auto_move.json`.
`python tools/verify_remote_actor_auto_move.py --exe <ffxivgame.exe>`
authenticates the executable identity and every row against a supplied copy.
No live session was observed.

Roles. `C` is the CharaActor. `H = C+0xBF0` is the movement helper. `E` is the
CharaElement. `T` is the `0x00CF` or `0x00D0` wrapped actor record, counted
from its first byte (the u16 opcode is at offset 2). A step is one unit of the
timestep argument that the slot-142 update passes to `0x007B6C20`.

## Verdicts on the working assumptions

| Assumption | Verdict |
|---|---|
| `0x00D0` fills a four-slot table, wire value divided by 30 per step | The table has 16 slots. The wire carries slots 0 to 3. The divisor is STATIC. The wire value being per second is INFERRED. |
| Nonzero `0x00CF` state sets the auto-move level and the first vector is the target | STATIC. A zero state keeps the previous level, and the target is still replaced. |
| The actor advances toward the target at the table speed and stops by arrival | STATIC for the chase and the arrival test, behind a gate whose producer is OPEN. |
| The gait handed to the pose layer is the moving level | STATIC. The chase requests level `H+0x80` every step and the arrival requests 0. |
| The second vector often carries `(0,h,0)` with `h` near 1.1 to 1.7 | Refined. OBSERVED in 416 of 15100 records. Its `x` and `z` are always 0 and the client consumes only `y`, as a height above the ground, never as a position. |

## `0x00D0` speed list

Receiver `0x0058D052` (row `speed table receiver`) reads the count byte at
`T+0x90` and copies records `i` below the count, at most 16, as a float at
`T+0x10+8i` and a u16 slot at `T+0x14+8i`, into `E+0x188`. It writes slot
`0xFFFF` after the last record and sends element command `0x10` with `0x88`
bytes. The command arm (table below) is `0x0065CE50`. For each record it calls
`0x007A4750(slot, float / 30.0)` with the double `30.0` at `0x00FA7B18`.

`0x007A4750` stores `H+0x10C+4*slot` without a bound, then sets the live speed
scalar from the entry of the applied level `H+0xE0`. The scalar is `H+0xF4`, or
`H+0xEC` when the ramp length `H+0x100` is positive (the ramp is in the
[level change ramp](#level-change-ramp) section). A second 16-slot table at
`H+0x14C` is cleared and initialised like the first, and no traced setter
writes it.

The float comes first and its slot follows in the same record. The payloads
agree. In the on-foot records the u16 values 0, 1, 2, 3 follow the floats 0.0,
2.0, 5.0, 5.0 (see the [wire census](../net/move-actor-wire-census.md#0x00d0-speed-list)).
The `xivl-captures` study `movement-speed-band-observations` reads the same
bytes as `(slot, float)` and so pairs every value with the previous slot. The
client reading is the one that applies.

### Defaults before any `0x00D0`

The helper constructor (`0x007B82B0`, rows `constructor speed defaults` and the
two float rows) clears both tables, then stores slot 0 as 0.0, slot 1 as the
float `0x3D888889` at `0x00FE7FC4` (2.0 / 30) and slots 2 and 3 as the float
`0x3E2AAAAB` at `0x00FE818C` (5.0 / 30). The default list is therefore
0, 2, 5, 5 per second, equal to the most common retail list. The mounted list
in the captures is 0, 3.6, 9, 9.

### Which slot is used

The speed is the entry of the applied level. Levels are 0 idle, 1 walk, 2 run
and 3 battle move (see the selection finding). The chase requests level
`H+0x80`, the last nonzero `0x00CF` state, so wire state 1, 2 and 3 select
slots 1, 2 and 3.

## `0x00CF` fan-out

The `0x00CF` case body is at `0x0058CEF7` (row `s2c 0x00CF case body`). The
local actor takes a signal branch (clear `E+0x26D` bit 0, call `0x00586F40`
with `0x4FD`). A remote actor stores the u16 at `T+0x28` to `E+0x210`. Both then
run the following.

| Step | Source in `T` | Function | Commands sent |
|---|---|---|---|
| First vector | floats at `+0x18`, `+0x1C`, `+0x20`, rotation at `+0x24`, state u16 at `+0x28` | `0x0058F240` | `0x36` (state), `0x34` (rotation), `0x35` (16-byte vector with 1.0 in the fourth lane) |
| Second vector | floats at `+0x30`, `+0x34`, `+0x38` | `0x0058F160` | `0x26` (16-byte vector, fourth lane 1.0) |

`0x0058F240` and `0x0058F160` also keep copies in the element
(`E+0x184`, `E+0x214`, `E+0x21C` and `E+0xB0`, `E+0xB8`). Both send only when
their broadcast argument is nonzero, which the `0x00CF` case always passes as 1.

The actor command dispatcher is CharaActor vtable slot 157 (`0x00662D30`, cell
`0x00FC0FA8`). It maps a command through the byte table at `0x00663C08` and
then the arm table at `0x00663AE8`.

| Command | Arm | Effect |
|---|---|---|
| `0x10` | `0x00663844` | speed list, see above |
| `0x26` | `0x00663074` | `0x0065CC60` stores the vector `y` (below) |
| `0x34` | `0x00662E7C` | `0x0065CD10` calls `0x007A46B0` with the rotation |
| `0x35` | `0x00662E9D` | `0x0065CCA0` calls `0x007A76C0`, the target setter |
| `0x36` | `0x00662ECF` | `0x007A4700`, the level setter |

The arms are STATIC. The step from `0x004D7980` (it builds a scene packet with
the command plus `0x15`) to this dispatcher with the unshifted command was not
traced. The command identities rest on the matching argument shapes of
commands `0x34`, `0x35` and `0x36` and on the `0x26` and `0x10` arms reading the
shapes that the senders write. That link is INFERRED.

### Level setter `0x007A4700`

It stores the argument at `H+0x80` only when the argument is nonzero and
`C+0x2B78` bit 0 is clear. A state of 0 therefore never lowers the level. The
role of `C+0x2B78` bit 0 is OPEN. When it is set, the level setter, the target
setter and the chase are all skipped.

### Target setter `0x007A76C0`

With `C+0x2B78` bit 0 clear it shifts the target history and stores the new
target. `H+0xA0` takes `H+0x90`, `H+0x90` takes `H+0x50`, and `H+0x50` takes
the new vector with 1.0 in the fourth lane. `H+0xB4` takes `H+0xB0` and
`H+0xB0` is zeroed. The history has no reader among the traced code. Every
`0x00CF` therefore replaces the target, including records with state 0.

It then measures the flat distance, over x and z only, from `H+0x10` to the new
target. When that distance exceeds the double 30.0, `C+0xE0` is not
`0x7FFFFFFF`, predicate `0x0065A580` is false and `C+0x2B78` bit 0 is clear, it
stores the float 30.0 at `H+0x1E0` (see catch-up below). The role of
`0x0065A580` is OPEN. It reads the object at `u32(C+0x118)+0x68`.

### Second vector

Command `0x26` reaches `0x0065CC60`, which reads only the `y` float of the
vector and calls `0x007A42B0` on `C+0x590`. That stores `y` at offset `+0x588`
and, when flag bit `0x200` of `+0x5B4` is set, also at `+0x58C` and clears the
flag. The only reader found, `+0x58C` through `0x007A42F0`, is the CharaActor
position method at vtable cell `0x00FC0FCC` (slot 166, `0x00660F70`). When its
ground query reports a hit it replaces the actor `y` with the hit `y` plus
`+0x58C`. Without a hit the actor keeps its own `y`. INFERRED: the second
vector `y` is a height above the ground, which the server sends as 0 for
ordinary ground actors. How `+0x588` is smoothed into `+0x58C` is OPEN. The vector `x`, `z` and the element copy at `E+0xB0` have no traced reader.
`xivl-client-structs:manifests/0x00CF_moveactor_pipeline.json` describes the
`0x0058F160` write as an interpolation helper for the current position. The
instructions cited here show a height store for command `0x26` and no reader of
the element copy, so that description is not supported.

## The chase

### Gate

`0x007B6C20(H, dt)` runs once per step. It first calls `0x0065DFD0` on the
actor (row `catch-up early return gate`). When that is true the function copies
the float at `+0x46C` from the object of `0x0065E020` to the actor (when the
object exists and its virtual slot `+0x268` returns 0) and returns, which skips
the catch-up ramp and the chase. The role of `0x0065DFD0` is OPEN. Otherwise,
for the selected actor (`0x0065A500`) it fixes the multiplier `H+0x1E4` at 1.0,
clears the window `H+0x1E0` and takes no ramp. With `H+0x1EC` equal to 0 it calls
`0x007B5510` only when `C+0x1104` equals 4, the actor is not the selected actor
(`0x0065A500`), `C+0x2B78` bit 0 is clear and `0x0065A580` is false (row
`remote mover gate`). Any other `C+0x1104` calls `0x007B4D10`, which does not
chase `H+0x50`.

OPEN and load-bearing. The only store to `C+0x1104` found in the executable is
the value 3 at `0x007B534C`. A scan of the code for 32-bit displacement stores,
indexed stores, vector stores and `H`-relative forms found no store of 4. The
producer of 4 is unknown, so whether a given retail actor takes the chase was
not shown statically.

### Step

`0x007B5510` keeps a state at `H+0x84`: 3 hold (the constructor value), 0 chase,
1 deceleration playback, 2 release. It forms the vector from `H+0x10` to the
target `H+0x50`. When predicate `0x0065BA90` (bit 3 of `C+0x2AF0`) is set the
`y` component is zeroed, so the distance is flat. The chase is state 0.

The chase continues while the arrival radius `H+0x1DC` is below the distance or
the applied level `H+0xE0` is 0 (row `arrival test`). The step then, in order
(row `chase step`):

1. Stores the unit direction at `H+0xD0` and passes it to `0x007AA690`.
2. When `H+4` bit 3 is clear and the object at `H+0x2D4` grants it, writes
   `H+0x1A0 = H+0x80` (the requested level).
3. Calls `0x007AFD90` to apply the level and the direction.
4. Adds the delta of `0x007A7E50(dt)` to `H+0x10`. The delta is the direction
   times `H+0xF4`, `H+0x1E8` (1.0 after reset), `H+0xF8` and `dt`. The delta is
   also multiplied by the double 0.0 at `0x00F62F70` (row `constant 0.0 double`)
   when the object at `*(C+0x118)+0x78` is nonzero and `0x006245D0` is true, and
   again when `C+0x2B78` bit 0 is set. Under either condition a chasing actor
   moves 0 per step. The role of `0x006245D0` is OPEN.
5. Copies `H+0xF4` to `H+0x74`. When `H+0x80` was 0 it sets `H+0x80 = 1` and
   `H+0x84 = 3`.

There is no clamp to the remaining distance. The last item means a first
record with state 0 moves one step with a requested level of 0 and then uses
level 1.

### Catch-up multiplier

`H+0xF8` takes `H+0x1E4` every step for a non-selected actor (row
`catch-up multiplier ramp`). While the timer `H+0x1E0` is positive it falls by
`dt` and `H+0x1E4` rises by `0.08 * dt` up to 1.2. Once the timer is not
positive `H+0x1E4` falls by `0.08 * dt` down to 1.0. The constants are the
double `0.08` at `0x00FE8180` and the float 1.2 at `0x00FE8188`. A far target
(flat distance above 30.0, see the target setter) therefore starts a window of
30 steps with a speed multiplier that reaches 1.2 after 2.5 steps and returns
to 1.0 after the window. For the selected actor the multiplier is fixed at 1.0.

### Arrival radius

The radius `H+0x1DC` is 0.0 after reset (row `helper reset clears arrival
radius`). Its only writer is `0x007A45A0`, called by the state machine
functor `0x008BF790` and four sibling functions. At the call site of
`0x008BF790` (row `arrival radius sum`) it fetches the metadata record of the
stop clip through `0x007B1F00`. From the dword at record offset `+0x20` it takes
`n`, the signed byte 0, and an easing value, signed byte 1 divided by the double
10.0. It then sums, for `i` from 0 to `n` inclusive, `0x007A4DB0` of the speed
`table[H+0x80]` toward 0.0. `0x007A4DB0(a, b, n, i, ease)` returns
`a + (i / n) * (b - a)` when `ease` is 0.0, and a curve from `0x009DB280`
otherwise. The radius is the distance the stop clip covers while the speed ramps
to zero. A clip with `n` steps and ease 0 gives `speed * (n + 1) / 2`.

The metadata record is read from a map keyed by motion name at `H+0x228`. Its
source resource and the `n` and ease values per stop clip were not decoded. The
radius is therefore unknown in value and stays 0.0 until the first stop clip
evaluation.

### Deceleration

When the distance is within the radius and the level is nonzero, the step
requests level 0 and builds a path of points that ends exactly on `H+0x50`
(state 1). Two cases follow, selected by the live speed.

- Live speed `H+0xF4` below half of `table[H+0x80]` (the double 0.5 at
  `0x00F59898`): the point count is `int(distance * 15.0)`, raised to 15 when
  smaller.
- Otherwise: the point count is the integer from `H+0x100 + 0.5`, converted with
  `cvttsd2si` at `0x007B5C54`. Outside a ramp `H+0x100` is -1.0, so the count
  is 0 and the path holds only the target.

Playback (state 1) advances an index by `dt` per step with linear interpolation
between points and, at the end, writes the last point and enters state 2. INFERRED
from the count of 0: with no ramp running at arrival the actor reaches the
target on the next playback step instead of easing in. State 2 and then state 3
(hold) follow. Hold first sets `y` to the target `y` when `0x007A47A0` accepts
the difference. With `H+4` bit 4 clear it then calls `0x007B0740`, which
re-enters the chase (`H+0x84 = 0`) when the actor is farther than the double
0.1 from the target (row `hold re-engage`). With the bit set it skips that call
and instead moves the actor toward the target by a fraction `0.1 * dt` of the
offset, no more than 1/15 per step. The meaning of the bit is OPEN.

### Level change ramp

`0x007AFD90` handles a change of the requested level. It starts a ramp of the
live speed scalar from the previous value `H+0xF0` to the new entry `H+0xEC` over
`H+0x100` steps with easing `H+0x104`, advanced by `0x007A7BD0` (row `speed ramp
tick`). In the branch taken when the clip metadata gives no ramp the length is
the float 30.0 at `0x00F932E4` and the easing is 0.5. Other branches use 10.0,
half the signed byte at `H+0x254`, or -1.0 for no ramp, which is the reset
value. The condition that
chooses a branch reads clip metadata bytes at `H+0x254` to `H+0x256` and
was not resolved. The curve `0x009DB280` was not traced.

## Timestep unit

The chase moves a distance of `H+0xF4 * dt` per call and the slot-142 update
splits its timestep into calls of at most 1.0. The wire value divided by 30
makes `H+0xF4` a distance per unit of timestep. INFERRED: one unit is one thirtieth
of a second, because the local player's `0x00CA` reports average 4.92 distance
units per second against the on-foot run entry 5.0
(`xivl-captures:studies/c2s-00ca-position-report`). The source of the timestep
argument (the frame clock) was not traced.

## Consumer notes for bahamut-client

A client may implement as STATIC or OBSERVED:

- Parse `0x00D0` with the count byte at record offset 144 and, for each record,
  the float at `16+8i` and the u16 slot at `20+8i`. Store `float / 30` per step
  at the slot, with at most 16 records applied. Slots 0 to 3 are the observed
  range.
- Start from the default list 0, 2, 5, 5 per second (0, 1/15, 1/6, 1/6 per
  step) before any `0x00D0`.
- On `0x00CF`, keep the last nonzero state `s` as the gait level, which is also
  the speed slot. Replace the target with the first vector every record, even for
  state 0. Apply the rotation float at offset 36.
- Chase: each step move the actor along the unit vector to the target (flat
  when the actor is ground-locked) by `table[s] * multiplier * dt` without
  clamping. Request level `s` for the pose layer while chasing and level 0 on
  arrival.
- Catch-up: when a new target is more than 30.0 flat units from the current
  position, run a 30-step window with the speed multiplier rising from 1.0 to
  1.2 by 0.08 per step and falling back by the same rate afterward.
- Read only the `y` of the second vector, as a height added above the ground
  height. Do not use it as a position. Its `x` and `z` are 0 in every capture.
- The count byte may be 1, 3 or 4. Slots not listed keep their previous value.

Keep as AUTHORED or CALIBRATION:

- The wall-clock length of a step (CALIBRATION; 1/30 s is INFERRED only) and
  the frame clock that supplies `dt`.
- The arrival radius. It depends on stop-clip metadata that is not decoded, so
  it is CALIBRATION, with 0.0 as the pre-evaluation value.
- The stop profile. The common retail case INFERRED to land on the target on the
  next playback step. An eased stop is AUTHORED.
- Level change ramps (30 steps, 10 steps, easing 0.5) are INFERRED branch
  constants. Use them as CALIBRATION until the condition that selects them is
  settled.
- The gate `C+0x1104 == 4`. Treat every NPC as chasing as an AUTHORED
  assumption until the producer of 4 is found.
- The meaning of `C+0x2B78` bit 0, predicates `0x0065A580`, `0x0065DFD0` and
  `0x006245D0`, and the hold bit `H+4` bit 4. Two of them stop the chase
  (`0x0065DFD0` skips it, `0x006245D0` or `C+0x2B78` bit 0 zeroes the step), so a
  client that always chases is an AUTHORED simplification.

## Open

- The producer of `C+0x1104 == 4`.
- The roles of `0x0065DFD0` (early return before the ramp and the chase) and
  `0x006245D0` (zero displacement factor).
- The frame clock that supplies `dt` and the link from the scene packet to the
  dispatcher command numbers.
- The stop-clip metadata record: its source resource and its values.
- The easing curve `0x009DB280` and the conditions of the level change ramp.
- The role of the second 16-slot table at `H+0x14C`.
- The smoothing from `+0x588` to `+0x58C` and the reader of the `E+0xB0` copy.
- No live retail session was observed.
