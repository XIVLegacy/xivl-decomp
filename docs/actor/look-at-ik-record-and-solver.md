# LookAtIKObject record, goal and step

The look-at record path of the
[look-at consumers page](chara-look-at-consumers.md#ik-constraints) ends in
IK objects of type 2, `SQEX::CDev::Engine::Phieg::IKDynamics::LookAtIKObject`
(vtable `0x010A92E4`). This page records where those objects get their joints
and limits, how they turn a target position into two goal angles, how the
per-frame step approaches the goal and how the result becomes a joint
rotation. The skeleton file bytes that fill the objects are recorded in
`xivl-client-data:docs/skeleton-cib-and-ik-members.md`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. The instructions were read
with Capstone `5.0.7` over the file-backed sections and with the `ghidra` CLI
`disasm`. Decompiler output was read for orientation only. Float constants
were read from the file-backed data. STATIC means read from instructions or
data. INFERRED means concluded from a STATIC read without tracing the use.

## Record to object field map

STATIC. The loader `0x00AED060` reads one definition record R and fills the
object O. Its fields, with the record offset first:

| R | O | Value in `pc/c001` | Use |
| --- | --- | --- | --- |
| `+0x14` | `+0x38` | `j_kao` | end joint, by name through virtual `+0xC` of the skeleton interface |
| `+0x70` | `+0x3C` | `n_hara` | root joint, a zero offset names no joint |
| `+0x18..+0x20` | `+0x100` | 0, 0, 0 | offset vector times the loader's scale argument, w 1.0 |
| `+0x24..+0x30` | `+0x8C..+0x98` | -6, 6, -10, 10 | dead zone, vertical pair then horizontal pair, degrees |
| `+0x34..+0x40` | `+0xA4..+0xB0` | -20, 20, -30, 30 | slow zone |
| `+0x44..+0x50` | `+0xB4..+0xC0` | -50, 50, -80, 80 | range |
| `+0x54` | `+0x64` | 500 | maximum speed |
| `+0x58` | `+0xC8` | 10 | stored, no reader traced |
| `+0x5C` | `+0x58` | 2500 | acceleration |
| `+0x60`, `+0x64` | `+0x40`, `+0x44` | 1, 1 | goal interval bounds, integers |
| `+0x68` | `+0x68` | 135 | minimum speed |
| `+0x6C` | `+0x9C` | 0.3 | stored, no reader traced |
| `+0x74`, `+0x78` | `+0xD8`, `+0xDC` | 0, 0 | goal offsets added after the goal is computed |
| `+0x7C` | `+0x54` | 2 | axis mode |
| `+0x80`, `+0x84` | `+0xE4`, `+0xE0` | 0, 0 | corner thresholds |

STATIC. With byte `R+0x0A` bit 0 set, the loader also reads the flag byte at
`R+0x88`:

- bit 0 stores `R+0x8C` at `O+0xA0` (0.3)
- bit 1 sets bit 0 of the object flag byte `O+0x30` and stores `R+0x90`,
  `R+0x94` and `R+0x98` at `O+0x5C`, `O+0x74` and `O+0x70` (30, 90, 10)
- bit 2 sets bit 1 of `O+0x30`, and bit 3 sets bit 2
- bit 4 sets bit 3 of `O+0x30`, stores the byte `R+0x9C` at `O+0x78` (1),
  `R+0xA0` at `O+0x7C` (150) and the absolute value of `R+0xA4` at `O+0x80`
  (0.7). The absolute value masks the sign with the constant at `0x010A8F00`

STATIC. The chain table is at `R+0x0C` and its count at `R+0x10`. Each entry
is self-relative. For each entry the loader looks its name (`E+0x0C`) up
through virtual `+0xC` of the skeleton interface, creates a runtime joint
entry through `0x00AECEE0`, stores `E+0x70` at runtime `+0x04` (the delay) and
copies eight vec3 rows with w 1.0:

| Entry offset | Runtime offset |
| --- | --- |
| `+0x34` | `+0x10` |
| `+0x40` | `+0x20` |
| `+0x4C` | `+0x30` |
| `+0x10` | `+0x40` |
| `+0x1C` | `+0x50` |
| `+0x28` | `+0x60` |
| `+0x58` | `+0x70` |
| `+0x64` | `+0x80` |

`0x00AF5950` converts the rows with the degree to radian constant at
`0x010A90FC` (0.017453292). The chain of `pc/c001` is `j_sebo_c` with delay 1,
`j_kubi`, `j_kao` and `j_kami_a`, each with delay 0. The `j_kao` rows read, at
entry `+0x1C`, 4.71, 75.6, 11.6, at `+0x40` -1.11, -60.26, 0.93, at `+0x58`
-48.41, 0, 0 and at `+0x64` 41.54, 0, 0. The remaining rows of `j_kao` are
zero.

STATIC. The object setup `0x00AED900` multiplies every limit of `O+0x8C` to
`O+0xC0` and `O+0xC8` by 0.017453292. A magnitude not above 0.01
(`0x00FB7DEC`) becomes 0.01 or -0.01 (`0x010A97A0`). It multiplies the speeds
`+0x58`, `+0x64`, `+0x68`, `+0x5C`, `+0x70`, `+0x74`, `+0x7C`, `+0xD8` and
`+0xDC` by the same constant, and `+0x80` only for axis mode 2. It then
computes the first goal through `0x00AE9FF0`.

## Goal angles

STATIC. `0x00AE9FF0`, with axis mode 2 in `O+0x54`, takes the rotation rows of
the root joint matrix, normalizes them and builds three axes. P1 is row 1, P2
is row 0 negated and P3 is row 2. The direction is the target point at
`O+0xF0` minus the end joint position plus the offset `O+0x100` rotated by the
root. It calls `0x00AF8BA0` and then `0x00AF2810`, and adds `O+0xD8` to the
vertical result and `O+0xDC` to the horizontal result.

STATIC. `0x00AF2810(P1, P2, P3, d, &vertical, &horizontal)` computes
`a = d . P1`, `b = d . P2` and `c = d . P3`. The horizontal angle is
`-0.0 - atan2(b, c)` and the vertical angle is
`-0.0 - atan2(a, sqrt(b*b + c*c))`, with the SSE2 `atan2` at `0x00DC4762`
taking y in `xmm0` and x in `xmm1`. Each angle is wrapped by
`x - trunc((x +/- pi) * 0.15915494) * 6.2831855` (`0x010A9700`,
`0x010A96FC`). The vertical goal is stored at `O+0x144` and the horizontal
goal at `O+0x148`.

INFERRED. With these axes and the `pc/c001` rows, a target on the +X side of
the root gives a positive horizontal goal and a target above gives a negative
vertical goal.

## Per-frame step

STATIC. LookAtIKObject slot 2 installs the active step `0x00AED610` at
`O+0x28` and sets byte `O+0x31`. The release step is `0x00AED730`.

STATIC. The active step `0x00AED610`:

- With the weight `O+0xD4` below 0.00001 (`0x00FEE2B4`) it takes a goal
  through `0x00AE9FF0`, calls `0x00AE99C0`, sets the current angles
  `O+0xCC` and `O+0xD0` to the goal and clears the speed `O+0x60`.
- Otherwise it calls `0x00AE99C0` every frame. When the counter `O+0x50`
  has reached `O+0x48`, it first resets the counter, draws `O+0x48` as a
  random integer between `O+0x44` and `O+0x40` through `0x00414D00`, saves the
  old goal on the stack, takes a new goal and calls `0x00AE99C0`. When byte
  `O+0x33` is set after that call, the saved goal is written back. Otherwise
  byte `O+0x32` is cleared. Flag bit `O+0x30 & 4` then calls `0x00AECBB0`.
- It then re-tests the weight, calls the solve step `0x00AEA570` with the frame time, then
  `0x00AE9420` and `0x00AED520`.

STATIC. The release step `0x00AED730`, while `O+0x31` is set, zeroes both goals.
With the weight below 0.00001 it zeroes `O+0xCC`, `O+0xD0` and `O+0x60` and
clears `O+0x31`. Otherwise it runs the solve step and clears `O+0x31` when the
speed `O+0x60` equals 0 afterwards. It then calls `0x00AE9420` and
`0x00AED520`.

STATIC. The solve step `0x00AEA570(dt)` reads the current angles `O+0xCC`
(vertical) and `O+0xD0` (horizontal) and the goals `O+0x144` and `O+0x148`. It
computes:

- `k = O+0xD4 * O+0x1C`
- the acceleration `O+0x58`, blended with `O+0x5C` as
  `(1 - t) * O+0x5C + t * O+0x58` when flag `O+0x30 & 1` is set, where `t` is
  `(|d| - O+0x70) / |O+0x74 - O+0x70|` clamped to 0..1
- the reach as the smaller of `|d| / dt` and `O+0x64 * k`, and the floor as
  `O+0x68 * k` when the reach is above it and 0 otherwise
- the slow zone limits `O+0xA4..O+0xB0` selected by the sign of each delta,
  scaled through `0x00AE9830` when flag `O+0x30 & 2` is set. The scale routine
  takes register arguments and was not read

STATIC. The branch on byte `O+0x32` copies `O+0x60` to `O+0x6C` when it sets
the byte, reads `O+0x6C` while it is set, calls `0x00AE9310` for the flag
`O+0x30 & 8` case and adds the increments to `O+0xCC` and `O+0xD0`.

INFERRED. While the angle ratio against the slow zone is at least 1 and byte
`O+0x32` is clear, the speed rises by the acceleration times `k` and `dt` up to
the reach and not below the floor. Inside the slow zone the speed starts from
`O+0x6C` scaled by the ratio. The branch bodies other than those listed here
were read for structure only.

## Weights

STATIC. `0x00AE9BF0(O, out, vertical, horizontal)` turns the goal angles
into weights in `out`:

- bit 0 of `out+0` is set when the horizontal angle is negative and bit 1
  when the vertical angle is negative
- `out+8` is the horizontal angle over the range limit of its side (`O+0xBC`
  for negative, `O+0xC0` otherwise) and `out+4` is the vertical angle over
  `O+0xB4` or `O+0xB8`. Each is clamped to 0..1
- `out+0xC` is `(wv - O+0xE4) * (wh - O+0xE0) / ((1 - O+0xE4) * (1 - O+0xE0))`
  when both differences are positive, else 0, and the corner weight is
  clamped to 1
- `wh` is then limited to `|cos(atan2(vertical, horizontal))|` and `wv` to
  `|sin(...)|`, each clamped to 0..1

## Joint turn

STATIC. `0x00AF75E0(out)` reads those weights:

- with bit 0 set the horizontal row H is runtime `+0x20`, otherwise `+0x50`
- with bit 1 set the vertical row V is runtime `+0x70`, otherwise `+0x80`
- the corner row C is `+0x10` (bits 0 and 1), `+0x30` (bit 0 only), `+0x40`
  (bit 1 only) or `+0x60` (neither)
- the angles are `wv * V + wh * H + wc * C` minus the runtime `+0x90` of every
  chain joint listed at `O+0xA4..O+0xA8`, which `0x00AED900` fills with the chain
  joints that are skeleton ancestors of the joint

The angles go through the matrix builder `0x00AE15C0`, an inverse of the
joint parent matrix and the root joint rotation, and the result is written
through the skeleton interface. The builder, the Euler order and the final
composition were not read.

INFERRED. In `pc/c001` the `j_kao` row at entry `+0x1C` is the horizontal
row for a positive horizontal goal (about 75.6 degrees on the Y axis), the row
at `+0x40` the negative side, `+0x58` the vertical row for a negative goal
(about -48.4 degrees on X) and `+0x64` the positive side.

## Debug draw

STATIC. Slot 12 of the vtable, `0x00AEAB00`, is the debug draw of the object.
It formats the labels `R:%.2f`, `L:%.2f`, `U:%.2f` and `D:%.2f` from
`0x010A9728`, `0x010A9730`, `0x010A9738` and `0x010A9740` (pushed at
`0x00AEBE00`, `0x00AEBE9A`, `0x00AEBF34` and `0x00AEBFCE`) and draws through
the interface from `0x00414CD0`. It is not the solver. The look-at page names
it the likely solver, and the step and solve routines above are the solver.

## Open

- The scale routine `0x00AE9830` and `0x00AE98F0`, the user of `O+0xC8`, the
  source of `O+0x1C`, `0x00AE99C0`, `0x00AECBB0`, `0x00AE9420` with
  `0x00AF52B0`, the blend `0x00AF8270` gated by runtime `+0x110` and
  `+0x114`, and the delay ring of `0x00AED520`.
- Whether the release step clears `O+0x32` when the goal drops to 0.
- The matrix builder `0x00AE15C0` and the final joint composition of
  `0x00AF75E0`.

## Reproduction

```text
ghidra disasm 0x00AED060 -n 400
ghidra disasm 0x00AED900 -n 120
ghidra disasm 0x00AE9FF0 -n 200
ghidra disasm 0x00AF2810 -n 80
ghidra disasm 0x00AED610 -n 100
ghidra disasm 0x00AED730 -n 40
ghidra disasm 0x00AEA570 -n 400
ghidra disasm 0x00AE9BF0 -n 150
ghidra disasm 0x00AF75E0 -n 80
ghidra disasm 0x00AEAB00 -n 30
```

Read the float constants at `0x010A90FC`, `0x00FB7DEC`, `0x010A97A0`,
`0x010A9700`, `0x010A96FC`, `0x010A97B8`, `0x00FEE2B4` and the strings at
`0x010A9728..0x010A9740`.
