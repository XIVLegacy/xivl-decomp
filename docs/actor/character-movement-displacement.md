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
positive. The scalar is in caller timestep units; this trace does not assign
those units a real-time duration.

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
