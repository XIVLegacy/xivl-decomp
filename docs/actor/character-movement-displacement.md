# Character movement direction and displacement

A retained selected-character input branch reaches an intermediate position
write at VA `0x007B4DBC`. It forms a vector from four signed movement bytes,
combines that vector with camera-derived matrix rows, normalizes it into helper
direction `H+0xC0`, then scales that direction and adds the result to `H+0x10`.
The scalar state, callback results and caller admission remain conditions.
Later branches can replace the position before it reaches an actor setter.

The [selected-character command route](../resource/attribute-resource-and-scene-binding.md#selected-character-movement-commands)
owns the movement command, packed-byte delivery and actor admission evidence.
Here `C` is the actor, `T = u32(C+0x118)`, and `H = C+0xBF0`.
The helper constructor stores its owner at `H+0` (`0x007B82E1`).
CharaActor vtable cell `0x00FC0F6C`, slot 142 of `0x00FC0D34`, is
`0x006679C0`.

## Evidence identity and method

The input was the retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs with image base `0x00400000`; subtract that base
for an RVA. Retained Ghidra per-function assembly supplied instruction
boundaries. Capstone 5.0.7 in x86-32 mode independently decoded the selected
raw-backed PE ranges, and instruction bytes matched those retained boundaries.
The static replay also checked the selected switch, vtable and constant cells.
No target process or native floating-point routine was executed.

## Signed bytes and camera transform

This branch requires nonzero input, camera `K = u32(T+0x64)` non-null,
`u32(K+0x434) == 0`, and helper flag bit `0x08` clear. At
`0x00665341/0x00665348`, mode byte table `0x00665AC8` and arm table
`0x00665ABC` select `0x0066534F`. `0x007A4610` returns helper flag bit
`0x08`; its false result branches to `0x006655DC` at `0x00665358`.

`0x006652CF..0x00665335` loads `C+0x11B8..C+0x11BB` with `MOVSX`,
converts each signed integer to double, divides by double 100.0 at
`0x00F55AD0`, and stores a float. Call the four stored values `a,b,c,d`.
At `0x006655DC..0x00665639`, the branch constructs:

```text
q.x = float(-0.0 - (double(c) + double(a)))
q.y = 0
q.z = float(-0.0 - (double(d) + double(b)))
q.w = 0
```

The negative double zero is at `0x00F63008`. The sign and grouping follow
the `ADDSD`, `SUBSD`, and float stores; this is not a subtraction of the
two command pairs. The four-byte input is signed at this consumer, so a byte
`0x80` supplies -128 before division.

`0x0042F740`, called at `0x0066563F`, computes a square root of the XYZ
sum of squares. Its scalar result and the input timestep go to
`0x00661270` at `0x00665655`. That routine selects helper state and can
change `H+0x1A0` through `0x007AA570`. The subsequent trace is conditional
on its resulting state, rather than treating the state as a byte-derived speed.

The branch then reads float `K+0x420` (`0x0066565A`) and calls
`0x0042FDC0` at `0x00665683`. Let `A` and `B` be the float results of
its calls to `0x009D6280` and `0x009D63B0`, respectively. The routine
writes matrix rows `(A,0,-B,0)`, `(0,1,0,0)`, `(B,0,A,0)`, and
`(0,0,0,1)`. `0x0061CF20` calls `0x0042F3A0`, which combines the first
three rows with `q.x/q.y/q.z` using float `MULPS` and `ADDPS` in that
order, then clears the fourth result lane. Thus the result has:

```text
r.x = float(float(q.x*A) + float(q.z*B))
r.y = 0
r.z = float(float(q.x*(-B)) + float(q.z*A))
r.w = 0
```

`0x0053B750`, called at `0x00665694`, normalizes this result. Its XYZ
sum-of-squares calculation uses `RSQRTPS`, then one refinement using the
float constants 1.0 and 0.5 at `0x00F54F70/0x00F62F60`. It preserves
the fourth lane. For positive squared length `L` and reciprocal-square-root
estimate `R`, its operation structure is `R + (1 - R*L*R)*(0.5*R)`,
with rounding at each float vector instruction. Non-positive lanes retain
their original input through masks. This describes the observed operations,
not an exact replacement with mathematical `1/sqrt(L)`.

## Retained direction and state

The copied vector reaches `0x007AA690` at `0x00665A96`, followed by
`0x007AFD90` at `0x00665A9D`. The writer requires non-null `H+0x2D4`
and a nonzero output byte from that object's slot 1 before storing the
vector to `H+0x190` at `0x007AA6F5`. If `0x0065BA90` returns true,
it first sets y to zero. That predicate reads bit `0x08` of `C+0x2AF0`.
The concrete callback implementation and its output are not established here.

A direction-preserving branch of `0x007AFD90` has these conditions:

- `u32(H+0x1B8) != 1`, so its read at `0x007AFF9D` can normalize
  `H+0x190` into `H+0xC0` at `0x007AFFFB`. The vector's XYZ length
  must exceed the float at `0x00FB7AB4` (approximately 0.001).
- Helper flag bit `0x80` is clear, the slot-1 output byte remains nonzero,
  and `0x007AA710` returns false, so the early admission path does not clear
  `H+0x1A0`. That helper has additional actor and state predicates.
- `H+0x298` is null, so its callback at `0x007B0145` cannot modify the
  direction buffer passed by address.
- The resulting `H+0x1A0` equals `H+0xE0`. With bit `0x80` clear,
  `0x007B0167` skips to `0x007B047F`, preserving scalar `H+0xF4`.

The second normalizer is `0x0061CFD0`. It uses the same reciprocal-square-root
refinement and preserves the fourth lane after that lane is explicitly zeroed
by the caller. The branch therefore connects a newly accepted `H+0x190`
vector to the direction used by displacement. The retained scalar's producer
and other state-transition branches remain separate evidence edges.

## Timestep and intermediate position write

The slot-142 caller selects H at `0x00667F80`. It calls `0x007A4B60`
at `0x00667F88`, then `0x007A4AE0` at `0x00667F8F`. The latter calls
actor slot 34 and copies its returned position into `H+0x10` at
`0x007A4B4F`. The displacement baseline is this reloaded position, not
the value of `H+0x10` when input processing began.

`0x00667F9A` calls `0x007A50E0`; a true result skips the timestep loop.
Otherwise the loop requires its remaining float timestep to be positive.
It selects the smaller of that remainder and 1.0, calls `0x007A5E70`
with `C+0x1070` and `H+0x10`, then passes that selected scalar to
`0x007B6C20` at `0x0066801E`. The latter call subtracts the selected
scalar from the remainder afterward and repeats while the remainder is
positive. The [selected scene clock](#selected-scene-clock-input) gives
the conditional nominal units for that caller input.

Within `0x007B6C20`, the ordinary addition branch requires
`C+0x2B98 == 0`. Its selected-actor predicate `0x0065A500` compares C
with `u32(T+0xD0)`; equality sets `H+0x1E4` to 1.0 and `H+0x1E0`
to zero at `0x007B6C91..0x007B6C9C`. It later copies `H+0x1E4` to
`H+0xF8` (`0x007B6D82`). The branch continues when `H+0x34 <= 0`,
avoiding the selected-actor early return at `0x007B6E72`.

Calls to `0x007A7BD0`, `0x007AAEE0`, and `0x007B1BB0` precede ordinary
displacement. Float `H+0x100 < 0` skips the first helper's scalar update.
`0x007AAEE0` updates angle-related fields on this branch, while actor byte
`C+0xB3D` in `[1,13]` skips `0x007B1BB0`'s target-facing update.
These conditions preserve the retained `H+0xC0/H+0xF4` inputs.

When `H+0x1EC == 0` and `C+0x1104 != 4`, `0x007B6EC5` calls
`0x007B4D10`. With byte `C+0x1108 == 0`, that routine calls
`0x007A7E50` at `0x007B4DAD`. The displacement helper reads the stored
scalar `H+0xF4` and, for the selected actor, multiplies it by `H+0x108`.
It then multiplies by `H+0x1E8`, `H+0xF8`, and the selected timestep in
that order. Each multiplication uses double intermediates followed by a
float conversion. The optional context predicate through `T+0x78` and actor
bit `C+0x2B78 & 1` can each apply the double zero multiplier at
`0x00F62F70`. Null `T+0x78` and a clear actor bit bypass both modifiers.

At `0x007A7F2F..0x007A7F94`, the routine multiplies direction XYZ
`H+0xC0/+0xC4/+0xC8` by that scalar, explicitly sets the fourth delta
lane to zero, stores the four-lane output and returns its pointer. Its caller
loads that delta, loads current `H+0x10`, executes `ADDPS` at
`0x007B4DB9`, and stores to `H+0x10` at `0x007B4DBC`.

The write occurs before the `C+0x1104` switch at `0x007B4DC8`.
An unsigned value greater than 3 returns through `0x007B54DD`; the
enclosing route separately excludes value 4. This describes a conditional
return branch, not an observed or established producer of such a state.
Switch arms 0..3 and the earlier `C+0x1108` arm can replace or further
update `H+0x10`. Nonzero `H+0x1EC` selects a distinct interpolation
path, including its own position writes.

### Selected scene clock input

STATIC. The selected SceneThreadImpl initializer `0x004E9E60` sets
configuration rate `+4` to `30.0f`, optional scheduler cap rate `+8`
to zero and byte `+0xC` to 1 (`0x004E9EDB..0x004E9F14`). A nonzero
`+0xC` retains the quantized count in place of the fixed count. On
the successful nondecreasing counter path with positive frequency
and representable intermediate values, `0x00A4FDD0` converts the
sample difference to integer
microseconds, advances the baseline to the current sample, and caps
the measured interval at 1,000,000 microseconds
(`0x00A4FE4B..0x00A4FECC`). It then converts that interval to integer
units at 300,000 per second (`0x00A4FECF..0x00A4FEF4`).

STATIC. The scheduler adds the signed count to its retained 64-bit
carry at `0x00A49EE1..0x00A49EEF`. For a nonnegative total, the loop
chooses the first multiple of 1,000 strictly greater than the total.
It stores total minus output as the next carry. For a negative total
it emits zero and retains that total (`0x00A49EF2..0x00A49F23`).
The carry stores precede the optional cap, measured-versus-fixed
selection, zero gate, one-shot addend and scale
(`0x00A49F26..0x00A4A09F`). The zero gate and scale convert their
signed count through single precision before their multiplication
and integer conversion.

STATIC. The scheduler passes the address of the resulting count to
the callback walk (`0x00A4A0FD..0x00A4A118`). The retained
`UPDATE_AI_RAPTURE` record selects `0x004E9700` in bucket 145
(`0x004EA9F5..0x004EAA15`, `0x004EA042..0x004EA074`).
The callback reads the pointed signed count and divides by the
configured-rate divisor, 10,000 at the selected `30.0f` setting
(`0x004E9715..0x004E975F`). The actor walk forwards this normalized
float to admitted current CharaActor slot 142
(`0x004E99DC`, `0x0060C170`, `0x007C9000`, `0x007D25D0`,
`0x007CDE40`, cell `0x00FC0F6C`). It is a processed scheduler
count in nominal 1/30-second units at the retained `30.0f` setting.
The actor subdivides this input into steps of at most 1.0 after its
local gate.

DERIVED. With the selected configuration retained, zero gate clear,
zero addend, scale 1 and an established carry from -1,000 to -1,
the successful measured path emits 0 to 300,000 count units before
callback dispatch. The corresponding actor input is 0 to 30 only if
the callback reads that value and the actor route admits it. A zero
incoming carry with a maximal sample instead gives 301,000 before
retaining -1,000. This is arithmetic. Initial or reset carry,
intervening writes through the count pointer, actual callback cadence,
stall recovery, and final movement remain UNKNOWN.

## Target-facing update

STATIC, from the disassembly of `0x007B1BB0`, which the movement step calls
with the helper `H` as receiver. `C` is the owner actor at `H+0x00` and `T`
the context at `C+0x118`.

| PC | Observation |
|---|---|
| `0x007B1BC9` | Calls the selected-actor predicate `0x0065A500` on `C` and returns when it is false |
| `0x007B1BD8..0x007B1BE6` | Returns when byte `C+0xB3D` is in 1 to 13 |
| `0x007B1BEC..0x007B1BF8` | Returns when bytes `C+0xB39` and `C+0xB3A` differ |
| `0x007B1BFE` | Returns when bit `0x08` of byte `C+0xB7D` is set |
| `0x007B1C0D` | Returns when `0x007AA710(H)` answers true |
| `0x007B1C1A` | Returns when bit 0 of the flag dword `H+0x04` is set |
| `0x007B1C24..0x007B1C33` | Passes the dword `H+0x08` as the key to the lookup `0x007A0150` on `T+0x54` and returns on a null result |
| `0x007B1C40..0x007B1C5B` | Calls virtual `+0x88` of the found object for a four-float position and subtracts the helper position `H+0x10` |
| `0x007B1C60..0x007B1C68` | Loads the x lane, then the z lane, and calls the CRT entry `0x009DAEBA`, the two-operand arctangent of the [motion callback inputs](locomotion-selection.md) |
| `0x007B1C75..0x007B1CA7` | Multiplies the heading by the binary64 at `0x00FE7FA8` (-16777216.0), divides by the float at `0x012C4D20` (bits `0x40C90FDB`, two pi), truncates with `CVTTSD2SI`, subtracts the result from the negated dword `H+0x1AC` and masks with `0xFFFFFF` |
| `0x007B1CAC..0x007B1CB6` | Adds `0xFFC71C71` and returns when the unsigned result is above `0x8E38E2` |
| `0x007B1CB8..0x007B1CD4` | When the pointer at `H+0x2C0` is nonnull, calls its virtual `+0x04` with the address of a zeroed byte |
| `0x007B1CD8..0x007B1D10` | When the float `H+0x24` is above 0, adds `H+0xF8` to `H+0x20`, stores the sum, and returns while the sum is below `H+0x24` |
| `0x007B1D12..0x007B1D22` | Returns when the byte is zero. Otherwise calls `0x007B14E0(H, heading, 0)` with the heading in radians |

`0x007B14E0` calls the [turn start](chara-turn-and-field-consumers.md)
`0x007B0530(heading, mode, 0)`. When that returns nonzero it sets bit 0 of
`H+0x04` and stores the float at `0x00F62F80` (-1.0) in `H+0x20` and
`H+0x24`.

The masked value is the gap between the heading of the keyed object and the
current angle `H+0x1AC` in units of a 24-bit circle. The compare admits a gap
from `0x38E38F` to `0xC71C71`, which is 80 to 280 degrees and symmetric about
the half turn. A null pointer at `H+0x2C0` leaves the byte zero, so the turn
then never starts.

INFERRED: the selected actor starts a turn to the exact heading of the object
keyed by `H+0x08` once that heading is 80 degrees or more from its current
angle, subject to the delegate byte and the timer.

UNKNOWN: the writer of `H+0x08` and which target it names, the object at
`H+0x2C0` and what its byte answers, the writers of `H+0x24`, where bit 0 of
`H+0x04` is cleared, and the bytes `C+0xB39`, `C+0xB3A` and `C+0xB7D`.

Reproduce with `ghidra disasm 0x007B1BB0 -n 100`, `ghidra decompile
0x007B14E0`, `ghidra memory read 0x00fe7fa8 8`, `ghidra memory read
0x012c4d20 4` and `ghidra memory read 0x00f62f80 4`. The tool is `ghidra` CLI
`0.2.2` driving Ghidra `12.1.4`.

## Boundaries

The ordinary write is an intermediate helper position. Later
`0x007AAB30`, caller continuation `0x007A78E0`, actor position setters,
and conditional queries are outside this arithmetic result. The
[native query boundary](../resource/attribute-resource-and-scene-binding.md#selected-character-movement-commands)
owns those previously established call routes.

Static replay authenticated the retained instructions and constants under
normal and optimized Python. Its symbolic arithmetic examples supplied matrix
coefficients and `RSQRTPS` seeds, using round-to-nearest float stores.
The client transcendental return values, hardware estimate, x87 control word,
MXCSR modes, intermediate precision and exception behavior were not observed.
The mathematical expressions above preserve operation and store ordering but
do not claim bit-exact retail floating-point execution.

Other camera modes, helper callback identities, scalar producers and state
transitions, interpolation, final actor position, live input and selection,
world/group admission, collision acceptance, contact and rendering remain
unqualified by this branch.
