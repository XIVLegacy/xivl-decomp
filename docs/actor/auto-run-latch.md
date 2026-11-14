# Auto-run flag and latch

Actor command `0x20` toggles a movement helper flag that keeps a character
moving with no held direction. A latch byte on the actor decides when a
held direction ends that state. The flag is the predicate `ctl` of the
[locomotion selection](locomotion-selection.md#selector-0x00661270) page and the
follow predicate of the [camera input chain](camera-input-chain.md#follow-tick).

`C` is the `CharaActor` and `H` is its movement helper at `C+0xBF0`.

## Evidence identity and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI disassembly
supplied the command arms and the helper routines. The Ghidra project
holds no instructions at the update arm `0x0066534F`, so Capstone 5.0.7 in
x86-32 mode decoded `0x0066534F..0x006656F8` from the file-backed bytes.
Reads of the file-backed PE sections supplied the two jump tables and the
constants. No target process ran.

## Command names

The image holds these command name literals:

| Literal | Addresses |
| --- | --- |
| `RaptureCommands.MoveCharacterAutoRun` | `0x00FA2098`, `0x00FA7EBC` |
| `RaptureCommands.ChangeMoveCharacterMode` | `0x00FA7E78` |
| `RaptureCommands.ChangeCameraMode` | `0x00FA2204`, `0x00FA7EE4` |

The default key rows of those commands are recorded in
[Speed toggle, auto-run and camera mode rows](https://github.com/XIVLegacy/xivl-client-data/blob/ed860d7658e72c259ac3e3238cfcf29a16623926/docs/movement-binding-rows.md#speed-toggle-auto-run-and-camera-mode-rows).
The tracked
[auto-run finding](https://github.com/XIVLegacy/xivl-client-structs/blob/79b7404cf5852f7c35fd26ae2b34762089357f7a/docs/client-architecture.md#L701-L728)
records that `MoveCharacterAutoRun` reaches the input listener `0x00589490`,
which raises scene operation `0x20` toward the dispatcher below. The
listener arm of `ChangeMoveCharacterMode` was not read here. That it raises
command `0x21` is INFERRED by elimination.

## Flag routines

| Routine | Operation |
| --- | --- |
| `0x007A4610` | Returns bit 3 of the dword at `H+0x04`. |
| `0x007B04A0` | With a nonzero byte argument it sets bit 3 of `H+0x04` and returns. With zero it clears the bit, and when the object at `H+0x2D4` is nonnull and its slot 1 writes a nonzero answer byte it stores 0 at `H+0x1A0`, the requested level (`0x007B04BC..0x007B04DE`). It then calls `0x007AFD90`. |
| `0x007A4860` | Returns the dword at `H+0xE0`, the applied level. |
| `0x0061F990` | Returns 1 when the camera dword at `+0x434` is 1, 3, 5 or 7, else 0. |

## Command arms

STATIC. The actor command dispatcher `0x00662D30` indexes the byte table
`0x00663C08` with the command and jumps through the table `0x00663AE8`.

| Command | Arm | Operation |
| ---: | --- | --- |
| `0x1E` | `0x00662D78` | Stores the argument dword at `C+0x11B8`, the four signed input bytes. |
| `0x20` | `0x00662D94` | The auto-run toggle below. |
| `0x21` | `0x00662E45` | Flips the byte `C+0x168`. |
| `0x22` | `0x00662E1D` | Stores 1 at the latch byte `C+0x169` and calls `0x007B04A0` with 0 (`0x00662E1D..0x00662E2C`). |

The `0x20` arm calls `0x007A4610`.

- Flag set, `0x00662DA7..0x00662DB0`: store 1 at `C+0x169` and call
  `0x007B04A0` with 0.
- Flag clear, `0x00662DC9..0x00662E04`: return with no change when
  `0x007A4860` returns 0. Otherwise read the camera `K` at `+0x64` of the
  object at `C+0x118`. When `K` is nonnull, `0x0061F990` returns nonzero
  for it and the signed byte `C+0x11B9` is above 0, return with no change.
  Otherwise store 0 at `C+0x169` and call `0x007B04A0` with 1.

So the command starts auto-run only while the applied level is not 0, that
is while the character is already moving, and it clears the latch when it
does.

## Per-frame update

STATIC. The update `0x00665220` takes one float argument `dt`. At
`0x00665247..0x00665286`, with the flag clear and all four bytes
`C+0x11B8..0x11BB` zero, it calls `0x00661270` with 0.0 and `dt` and goes
to the common tail. Otherwise it reads the camera `K` as above and returns
when it is null. It divides each signed byte by the binary64 100.0 at
`0x00F55AD0` into the floats `a`, `b`, `c` and `d`, in address order
(`0x006652CF..0x00665335`). A camera mode `u32(K+0x434)` above 10 goes to
the tail. The byte table `0x00665AC8` and the arm table `0x00665ABC` send
modes 0, 2, 8, 9 and 10 to `0x0066534F`, modes 1 and 3 to `0x006656F8` and
modes 4 to 7 to `0x006658A5`.

The arm `0x0066534F` calls `0x007A4610`. With the flag clear it takes the
[camera-relative displacement branch](character-movement-displacement.md#signed-bytes-and-camera-transform)
at `0x006655DC`. With the flag set:

| Locator | Operation |
| --- | --- |
| `0x0066535E..0x006653DE` | Build the vector `(0, 0, 1, 0)`, read the f32 at `+4` of the result of the actor's virtual `+0x94`, and pass both through `0x0042FDC0`, `0x0061CF20` and `0x0053B750`. Keep the result as `F`. |
| `0x006653E7..0x00665461` | `L = double(c) + double(a)`. Build `q = (f32(-0.0 - L), 0, f32(-0.0 - (double(d) + double(b))), 0)` with the binary64 negative zero at `0x00F63008`. |
| `0x00665467` | `n` is the result of `0x0042F740` on `q`, the length the displacement page records. |
| `0x0066547A..0x006654A6` | `x` is the result of `0x0042F650` on `q` and the vector `(1, 0, 0, 0)`. That routine sums the products of lanes 0, 1 and 2 (`0x0042F6BB..0x0042F6CC`), so `x` is `q.x`. |
| `0x006654AB..0x006654C8` | Call the selector `0x00661270` with `n` and `dt`. |
| `0x006654CD..0x0066553B` | Pass `q` and the camera f32 `K+0x420` through the same three helpers, then `s` is the result of `0x0042F650` on that vector and `F`. |
| `0x00665540..0x00665581` | Take the release path only when `n` is above the binary64 0.800000011920929 at `0x00FA55D0`, the absolute value of `x` is below the f32 0.699999988079071 at `0x00FB9310` and `s` is below the f32 -0.699999988079071 at `0x00FC1058`. Any failed test goes to the keep path. |
| `0x00665583..0x006655A0` | Release path. With the latch `C+0x169` zero, go to the tail with no change. Otherwise store 1 at the latch and call `0x007B04A0` with 0. |
| `0x006655A5..0x006655D7` | Keep path. Pass `f32(L / -20.0 * dt)` to the turn routine `0x007AA5B0`, with the binary64 -20.0 at `0x00FC1050`, then store 1 at the latch. |

The flag-set branch makes no call to the direction store `0x007AA690`. Its
calls are `0x007A4610`, the actor's virtual `+0x94`, `0x0042FDC0`,
`0x0061CF20`, `0x0053B750`, `0x0042F740`, `0x0042F650`, `0x00661270`,
`0x007AA5B0` and `0x007B04A0`. The tail at `0x00665A9B` calls
`0x007AFD90`.

## Reading

INFERRED from the arms and the update.

- The flag keeps the helper's retained direction, since the flag-set
  branch stores none. The
  [direction-preserving branch](character-movement-displacement.md#retained-direction-and-state)
  of `0x007AFD90` then keeps the character moving.
- While the flag is set, the lateral input sum `L` turns the character
  through `0x007AA5B0` on every update that takes the keep path.
- The latch is 0 from the key press until the first update on the keep
  path, and 1 afterwards. The release test only ends auto-run once the
  latch is 1. So a direction held when the key is pressed does not end
  auto-run until it has left the release cone once.
- The release cone is an input longer than 0.8 whose lateral part is under
  0.7 and whose camera-rotated direction has a product under -0.7 with
  `F`. `F` is read as the actor forward, a unit Z turned by the actor
  heading.

UNKNOWN: the unit of `dt` and so the turn rate per second, the sign of the
turn, the screen direction of the release cone, and the arms
`0x006656F8` and `0x006658A5` of the other camera modes.

## Consumer implications

- Auto-run may start only from a moving state, keep the heading it had and
  clear on a second key press or on actor command `0x22`.
- The release thresholds 0.8, 0.7 and -0.7 and the latch order are retail
  values. The turn rate per second and the direction of the release cone
  are CALIBRATION.
