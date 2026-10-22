# Camera input chain

How the 1.23b client turns wheel, mouse drag and camera commands into
`CameraActor` state: which context key a fresh session runs, which float the
wheel writes, the clamp and rate constants, the lens and the blend. The
[indexed float storage](indexed-float-clamp.md) page records the getter and
setter. [Camera context registration](camera-context-selection.md) records
keys 0, 2, 4 and 6. The key table below covers all eleven keys.

The manifest is [`config/camera_input_chain.json`](../../config/camera_input_chain.json).
[`tools/verify_camera_input_chain.py`](../../tools/verify_camera_input_chain.py)
checks every signature and table below against the retail executable and
carries the numeric model that the tests exercise.

## Evidence identity

- Binary: `ffxivgame.exe` retail 1.23b, image base `0x00400000`, size
  `15996808`, SHA-256
  `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
  Addresses are VAs.
- Tools: Capstone 5.0.7 in x86 32-bit mode over the file-backed PE sections,
  with the section table parsed by hand. Ghidra 12.1.4 decompiled
  `0x007F7EF0`, `0x007F8200`, `0x007F7AF0`, `0x007F7D40`, `0x007F7940` and
  `0x0055CDB0` as an independent reading. Its output agreed with the
  instruction reading in every row it covers.
- Tiers: STATIC is read directly from instructions and data. INFERRED needs
  the assumption stated in the row. Anything else is listed as unresolved.
- Notation: `+0xNNN` on its own is a `CameraActor` offset. `ctx` is the
  selected context object. The camera is the argument the context methods
  receive.

## Context keys

The constructor `0x0061B5A0` allocates, constructs and inserts eleven contexts
through the registry call `0x00621BE0`. Each row is checked at its constructor
call, its key store and insert call, and its derived vtable store.

| Key | Class | Constructor | Vtable | Update arm |
|---:|---|---|---|---|
| 0 | `CameraActorContextTPS` | `0x007F3AD0` | `0x00FF10BC` | single |
| 1 | `CameraActorContextFPS` | `0x007F1730` | `0x00FF101C` | single |
| 2 | `CameraActorContextWowTPS` | `0x007F1FC0` | `0x00FF1204` | tick |
| 3 | `CameraActorContextWowFPS` | `0x007F1DC0` | `0x00FF1164` | tick |
| 4 | `CameraActorContextTPS_Lock` | `0x007F4A40` | `0x00FF110C` | single |
| 5 | `CameraActorContextFPS_Lock` | `0x007F1840` | `0x00FF106C` | single |
| 6 | `CameraActorContextWowTPS_Lock` | `0x007F5C10` | `0x00FF125C` | tick |
| 7 | `CameraActorContextWowFPS_Lock` | `0x007F1EC0` | `0x00FF11B4` | tick |
| 8 | `CameraActorContextCharaSelect` | `0x007F2400` | `0x00FF0F24` | single |
| 9 | `CameraActorContextCharaCreate` | `0x007F26D0` | `0x00FF0F74` | single |
| 10 | `CameraActorContextCharaFocus` | `0x007F2C70` | `0x00FF0FCC` | tick |

STATIC. The constructor stores key 11 (`0x0B`) at `+0x438` as the previous-key
sentinel and registers no context under it. Keys 1, 3, 5 and 7 are the
first-person classes.

For keys below 8, `key & 3` names the family: 0 TPS, 1 FPS, 2 WowTPS, 3
WowFPS. Keys 4 to 7 are the same families with the `_Lock` classes. The setter
`0x00618650` moves a requested key into the lock range when `0x00616F90`
returns zero and into the plain range when it returns nonzero. The tables are
at `0x006187BC` (4 to 7 map to 0 to 3) and `0x006187CC` (0 to 3 map to 4 to 7).
When `0x007A3A60` holds for the object at `+0x590` of the pointer that `+0x440`
or `[[+0x118]+0xD0]` resolves to, a second table
(`0x006187E4`, arms at `0x006187DC`) maps 0, 1, 4, 5 to 0 and 2, 3, 6, 7 to 2,
so that predicate removes the first-person keys. With `+0x448` bit `0x10` set,
requests for keys 1, 3, 5 and 7 return before any change. The user-facing
meaning of both predicates is unresolved.

What bahamut-client can change. A client that models only key 0 can name the
other keys from this table. It needs no behavior for keys 4 to 7 until the
predicate in `0x00616F90` is resolved.

## Initial state

STATIC. The constructor writes the following.

| Field | Value | Site |
|---|---|---|
| `+0x3F4`, `+0x3F8`, `+0x3FC`, `+0x400`, `+0x3F0` | float `4.5` from `0x00FB70B8` | `0x0061BC32..0x0061BC63` |
| `+0x434` | literal `1`, replaced by the selection below | `0x0061B7A9` |
| `+0x438` | `0x0B` | `0x0061B7B3` |
| `+0x428` | `0.75` from `0x00F5988C` | `0x0061B791` |
| `+0x1D8` | `1.0` | `0x0061B684` |
| `+0x1C4` | byte `0x12` | `0x0061B663` |
| `+0x1C6` | low nibble cleared | `0x0061B635` |

The selection at `0x0061BC63..0x0061BCD7` starts at key 0 and takes key 4 when
`0x00616F90` returns zero. When the `0x007A3A60` remap holds it turns key 4 into
key 0. A fresh session therefore selects key 0 or key 4 and never a Wow or
first-person key. Because the literal 1 differs, the change block runs and
stores `30.0` to `+0x43C` (see Blend). The slot read by key 0 is slot 0 and the
slot read by key 4 is slot 1, so both start at 4.5 units of eye distance before
the guarded query adjustments of the endpoint producers.

Command 3 (`0x006179B0`) restores the four slots to 4.5 and, for keys 1 and 5,
`+0x428` to 0.75. The camera element sends event 3 from its default-position
routine, see Pointer path.

What bahamut-client can change. The initial chase radius 4.5 replaces the
authored 6.0. Its look-at height is not part of this page, see Open questions.

## Update pipeline and command receiver

STATIC. The update `0x006195D0` computes `dt = (float)elapsed * 30.0 /
300000.0` from the first dword of its argument (`0x006195F1..0x006195F9`
with the doubles at `0x00FA7B18` and `0x00F67020`). The unit of `elapsed` is
unresolved. The key then selects one of two arms through the byte table
`0x00619E64` and the table `0x00619E5C`.

| Arm | Keys | Calls |
|---|---|---|
| tick | 2, 3, 6, 7, 10 | ctx `+0x1C` with `dt`, ctx `+0x20` per queue A item, ctx `+0x24` per queue B item, then ctx `+0x28` |
| single | 0, 1, 4, 5, 8, 9 and any other key | ctx `+0x04` with `dt` |

After the arm the update runs the interpolator `0x0060E4C0` and the transform
builder `0x00617A80`. The queues are the containers at `+0x1DC` (A, items are
pointers to a command payload) and `+0x1F4` (B, items are the first 16-bit word
of a payload). Their readers are `0x007F89F0` and `0x007F8A50`.

`CameraActor` vtable slot 157 at `0x00FB92E0` is the command receiver
`0x0061A6C0`. Ids 0 to 0x30 index the table at `0x0061AE48`. The rows below
are the ones traced to a store or a call.

| Id | Effect |
|---:|---|
| 1 | copies 12 payload bytes to `+0x1BC` (4 words) and `+0x1C4` (dword), then requests key 0 (families 0, 2) or key 1 (families 1, 3) when either of the first two words is nonzero |
| 3 | reset, see Initial state |
| 4, 5, 6, 7 | float to `+0x40C`, `+0x1D8`, `+0x42C`, `+0x430` |
| 16 | set context key |
| 17 | toggle the TPS and FPS family |
| 20, 23 | float to `+0x404`, `+0x424` |
| 40 | queue A push |
| 42 | queue B push |
| 41 | key request, payload handled by `0x00618CB0` and not traced |
| 43, 45 | `+0x448` bit `0x04`, bit `0x10` set from the payload |
| 44 | `0x00618B40` with the payload flag |
| 47 | optional key request, slot 0 from payload float 0 through the clamp setter, then `0x00619F80` with float 1 |
| 48 | `0x00619FE0` with two floats |
| 0x2E | sent by the update itself through `0x007CE7B0` (`0x00619A22`) for keys 0 and 2 with 8 bytes, two floats from `0x00619010` and `0x006190E0`. The consumer is not traced |

Commands 40, 41 and 42 return while the first or second word stored by command
1 is nonzero. The pushes `0x00618BF0`, `0x00618C50` and `0x00618CB0` request key
2 for families 0 and 2 and key 3 for families 1 and 3 before they store the
item, through the tables at `0x00618C38`, `0x00618C98` and `0x00618FFC`.

What bahamut-client can change. A pointer drag or a wheel notch while in key 0
or key 4 first switches to the Wow family (key 2 or 3, normalized by the lock
rule), then applies its effect. A camera-move command with a nonzero payload
switches back to key 0 or 1. The client can keep one orbit state and treat the
switch as the point where mouse look starts.

## Pointer path

`CameraElement` (`Application::Main::Element::System::CameraElement`,
constructor `0x00566380`) is a UI element, not a camera object. Its constructor
subscribes four handlers at `0x005665D0..0x005666F2`.

| Event | Id and strategy | Handler |
|---|---|---|
| `PreviewMouseMove` | `0x11`, 2 | `0x0055C9B0` |
| `PreviewMouseUp` | `0x17`, 2 | `0x0055CB60` |
| `PreviewMouseDown` | `0x07`, 2 | `0x0055CC90` |
| `MouseWheel` | `0x1E`, 0 | `0x0055CDB0` |

STATIC. The mouse-down handler (before each button branch), the wheel handler
and the mouse-move handler (two sites, `0x0055CA61` and `0x0055CB1A`) first
call `0x004D7580` on `[esi+0x7C]`, then a vtable method of the returned object
(slot `+0x30`, and `+0x0C` at `0x0055CA6A`), and return without sending a
command when the result is nonzero. The mouse-up handler has no such call.
The meaning of the predicate (UI capture or input blocking) is unresolved, so
the chain below is not unconditional.

STATIC. The handlers raise events through `0x004D7980` whose ids equal the
camera command ids of the previous section, with matching payload sizes (10
bytes for 0x28, 2 for 0x2A, 12 for 1, 4 for 0x17, 8 for 0x2F). INFERRED. The
forwarder between the element and slot 157 is not traced, so the identity of
ids and sizes is the evidence that links them.

Buttons. The Win32 handler stores bit 1 for `WM_LBUTTONDOWN`, 4 for
`WM_RBUTTONDOWN` and 2 for `WM_MBUTTONDOWN` (table `0x00D31D9C`, stores at
`0x00D31C6E`, `0x00D31CA3`, `0x00D31C86`). The device state conversion at
`0x00547FBA` maps mask bits 0, 1, 2 to state bytes `+8`, `+9`, `+0xA`.
`PreviewMouseDown` tests byte `+0xA` first and starts the drag that sends
command `0x28`, then byte `+9` for the drag that sends `0x29`. INFERRED. The
right button therefore drives the orbit and the middle button drives command
0x29. The left button is not used by the element. The hop from the Win32
statics to the event's state object is not traced.

Drag payload. The 10-byte payload of command `0x28` is five 16-bit words:
pressed flag, `dx`, `dy`, `x`, `y`. `dx` and `dy` are the cursor change since
the previous move in the event's float coordinates truncated to integers
(`0x0055CA83..0x0055CB57`). The consumers read words 1 and 2.

Wheel. The Win32 handler at `0x00D31D25` adds the signed high word of
`wParam` to an accumulator and divides by 120 keeping the remainder, so the
wheel value is whole notches. The remainder accumulator is the dword at
`0x01378398` and the notch result is stored at `0x013783A8`. STATIC.
`0x0055CDB0` multiplies the notch count by 200, truncates to 16 bits and sends
command `0x2A` with those 2 bytes. STATIC. The device word the handler reads is
loaded from offset 8 of a record at `0x00547F42`. INFERRED. That record is the
statics block around `0x013783A0..0x013783AC` (x, y, wheel, button mask), and
the record handed to the loader is not traced.

Commands. `0x00566D10` handles the `RaptureCommands` objects (names at
`0x00FA7EA0..0x00FA8020`). `MoveCamera` stores its two 16-bit arguments at
sub-object `+0x8C` and `+0x8E`. `ForwardCameraOn` and `BackwardCameraOn` store
-1.0 and 1.0 at `+0x16C` and send event `0x17` with that float, and the `Off`
commands send 0.0. `ZoomCamera` stores its first argument as a float at
`+0x170` and, while `+0x16C` is zero, sends event `0x17` with the second
argument times 0.015625. `ChangeCameraMode` sends event `0x11` and then runs the
default-position routine `0x005636B0`. Every call ends by sending event 1 with
the 12 bytes at `+0x8C` (`0x0056701A`). `DefaultCameraPosition` runs `0x005636B0`,
which sends event `0x2F` with a radius and an angle read from the config blob,
then event 3, then `0x2F` again. `TPSCameraInitialize` first writes 4.5 and 0.0
through the config writers and then sends `0x2F` (`0x00566CA0`). The element
constructor stores 4.5 at `+0x128` and `+0x12C` and the byte `0x12` at `+0x9C`.
The gating of these commands by the RaptureUserControl counts, which the server
drives through s2c 0x0193 subopcodes 0x14 and 0x15, is described in
[s2c 0x0193 native state](../net/s2c-0193-native-state.md) and is not traced
here.

What bahamut-client can change. Wheel input should be quantized to whole notches
before it reaches the camera model. Rotation should use the cursor change per
move. Right drag orbits. The left button does not orbit. The middle-button drag
(command 0x29) is not traced.

## Wheel zoom

STATIC. Queue B items are 16-bit wheel payloads `p = 200 * notches`. The
context slot 9 methods process one item each.

| Key | Method | Rule |
|---:|---|---|
| 2 | `0x007F7EF0` | if `p > 0` and slot 0 is at most 1.5, set `+0x448` bit `0x02`. Then slot 0 = clamp(slot 0 - p / 500.0, 1.5, 10.0) through `0x00611350` |
| 6 | `0x007F8200` | the same rule on slot 1 |
| 3 | `0x007F7AF0` | if `p < 0` and `+0x428` is at least 1.0, set the flag. Then `+0x428` = clamp(`+0x428` - p / 1000.0, 0, 1) |

One notch therefore moves the eye distance by 0.4 toward the focus in third
person. From the default 4.5 the eighth notch reaches the 1.5 floor and the
ninth raises the flag. The update tests the flag at `0x0061973C..0x0061974A`,
clears it and calls the family toggle `0x006187F0`. The toggle
table is at `0x00618ACC`. Family 0 requests key 1, family 1 requests key 0,
family 2 requests key 3 and family 3 requests key 2. When `0x00616F90` returns
zero the request is the lock key instead (5, 4, 7, 6), see the arm checks in
the manifest.

First person uses `+0x428` as a zoom in 0 to 1. One notch moves it by 0.2, and
a backward notch at zoom 1.0 returns to third person.

Continuous axis. Command 23 stores a float at `+0x424`. The tick methods add
`+0x424 * dt / 10.0` to slot 0 through the clamp (`0x007F5450` at
`0x007F566E..0x007F5692`). The same `+0x424` read appears in the key 0 update
at `0x007F4061..0x007F40CE`.

The clamp bounds 1.5 and 10.0 are the setter constants documented on the
[indexed float page](indexed-float-clamp.md). They are the radius limits of
slots 0 and 1. Slots 2 and 3 are not written by the traced handlers.

What bahamut-client can change. Set the wheel step to 0.4 per notch and the
third-person radius limits to 1.5 and 10.0 from the authored 0.1 and 2.0 to
20.0. Zoom in past 1.5 toggles to first person. Wheel input in first person
changes a 0 to 1 zoom that maps to the field of view below.

## Drag rotation

STATIC for the formulas and the flag gate, INFERRED for the roles of the two
angles and for what the flag means. Queue A
items are read at words `+2` (`dx`) and `+4` (`dy`). The invert byte `+0x1C6`
negates `dx` for bit 8 and `dy` for bit 4 in third person. In first person bit
2 negates `dx` and bit 1 negates `dy`.

| Key | Method | Rule |
|---:|---|---|
| 2 | `0x007F7D40` | ctx `+0x8` += `dy / 100.0 * 0.2 * s` limited to +-0.7853982 (pi/4). ctx `+0xC` += `dx / 100.0 * 0.2 * s`, wrapped to -pi..pi. `s` is `+0x1D8` |
| 3 | `0x007F7940` | ctx `+0x4` += `dy / 100.0 * 0.15 * s` limited to +-1.3962635 (80 degrees). ctx `+0x8` += `dx / 100.0 * 0.15 * s`, wrapped |

When the follow predicate `0x007A4610` holds, key 3 turns the actor by
`dx / 100.0 * 0.1 * s` through `0x007AA5B0` and leaves its yaw alone. Key 2
tests its context byte `+0x10` at `0x007F7E17` after the pitch update. While
the byte is nonzero it turns the actor by `dx / 100.0 * 0.1` and does not
change ctx `+0xC`. The yaw orbit above therefore applies only while `+0x10` is
zero. The follow tick below writes the byte every tick, 1 at `0x007F5653` and 0
at `0x007F565C`, so it mirrors the predicate. INFERRED. Whether the predicate
holds during ordinary walking is unresolved, so which of the two key 2 drag
behaviors a walking player gets is unresolved.

The key 2 endpoint producer `0x007F56B0` builds a rotation about X from ctx
`+0x8` with `0x0061F250` and a rotation about Y from ctx `+0xC` with
`0x0060D290` (rows `(1,0,0,0)` and `(0,1,0,0)` respectively). The eye is the
object vector plus slot 0 times the product applied to `(0,0,1,0)`, see the
[endpoint page](camera-endpoint-calculation.md). Pitch is therefore limited to
45 degrees up or down in third person and yaw is unbounded. The sign
convention of the resulting screen motion depends on the rotation order and
the object vector's basis and is not derived here.

With `s = 1.0` the third-person rates are 0.002 radians per cursor unit and the
first-person rates 0.0015.

Key 0 consumes the camera-move payload instead. Its yaw step is
`dx / 100.0 * 0.0625 * dt * s` applied through `0x0060D290`
(`0x007F3FC5..0x007F4000`). The values `MoveCamera` carries for a held key are
unresolved.

What bahamut-client can change. Right drag rates and limits can use these
constants: pitch +-45 degrees and 0.002 radians per cursor unit in third person.
Yaw orbit applies only while the key 2 byte `+0x10` is zero. Retail turns the
actor instead while it is set, and which state applies during walking is
unresolved, so a client should keep orbit as the default and label the actor
turn path CALIBRATION. The pitch limits -1.2 to 0.2 in the authored chase are
not retail.

## Follow tick

INFERRED. The key 2 tick `0x007F5450` runs when the actor's helper predicate
`0x007A4610` holds, which reads bit 3 of the helper dword at `+4`. The tick
reads a heading lane from the actor (`+0x94` method, lane at `+4`), subtracts
pi and wraps to -pi..pi. It blends ctx yaw toward it as the vectors
`(cos, sin)` with weight `min(1.0, 0.2 * dt)`, then stores `atan2` of the
result and sets ctx `+0x10` to 1 (`0x007F5653`). When the actor pointer at
`+0x4` is null or the predicate fails, the tick sets `+0x10` to 0
(`0x007F565C`) and skips the blend. In both cases it then adds `+0x424 * dt /
10.0` to slot 0. The predicate is the one named `ctl` on the
[locomotion selection](locomotion-selection.md) page. Whether it holds during
ordinary walking is unresolved, so the auto-follow condition is unresolved.

What bahamut-client can change. Nothing yet beyond the drag gate above. Only
the blend form and rate are known.

## Lens

STATIC for the formulas. The post-update arms (`0x00619922`, `0x00619A2D`,
`0x00619A61`, table `0x00619E70`, index `0x00619E7C`) store `+0x40C` per key.

| Keys | `+0x40C` |
|---|---|
| 0, 2, 4, 6, 10 | `2 * atan(15.0876 / (2 f))`, `f` = byte `+0x1C4` limited to 18..28 |
| 1, 3, 5, 7 | `0.5235988 + 0.4363323 * (+0x428)` |
| 8, 9 | `+0x404`, with `-0.3` stored at `+0x414` |

The update calls `0x00610F00` with argument 0 at `0x00619904`, before the
per-key arm stores `+0x40C` (`0x006199E8`), so the projection of a frame uses
the lens value of the previous frame. `0x00610F00` passes `+0x40C` to
`0x0060E860`, which stores `+0x15C = +0x40C + 0.5235988`, near
`0.25` at `+0x154` and far `10000.0` at `+0x158`, and builds the projection
block with the aspect at `+0x150` (viewport width over height). The projection
arm `0x00A5AEE9` passes field of view, aspect, near and far to `0x007DF8C0`.
INFERRED. `+0x15C` is the vertical field of view in radians. With that reading:

| Case | `+0x40C` | `+0x15C` |
|---|---:|---:|
| third person, `f = 18` | 0.7937 | 1.3173 (75.48 degrees) |
| third person, `f = 28` | 0.5263 | 1.0499 (60.16 degrees) |
| first person, zoom 0 | 0.5236 | 1.0472 (60 degrees) |
| first person, zoom 1 | 0.9599 | 1.4835 (85 degrees) |
| first person, zoom 0.75 | 0.8508 | 1.3744 (78.75 degrees) |

The `atan` routine is `0x009DAC80`, identified by its error name at
`0x012EAD50`. The offset `(+0x1C5 - 4) / 25.0` is stored at `+0x41C`. The
branch of `0x00610F00` taken with a nonzero argument copies `+0x40C` to `+0x408`
and `+0x15C` with near `0.01` and far `1000.0`. The update never takes it. The
producers of `+0x1C4` and `+0x1C5` are the camera-move payload (command 1) and,
upstream, the camera config widget. That upstream link is unresolved.

What bahamut-client can change. Near 0.25 and far 10000.0 replace the authored
lens planes, with the aspect taken from the viewport. The field of view
1.3173 radians (`f = 18`) rests on the INFERRED reading of `+0x15C` above. It
adds 30 degrees to a value that is already `2 * atan(sensor / (2 f))`, which is
unproven, so the default stays a CALIBRATION value until the consumer of
`+0x15C` in the projection builder is confirmed. Lens changes take effect one
frame late.

## Blend

STATIC. A context change stores `30.0` to `+0x43C` and zero to `+0x1D4`
(`0x0061876B`, `0x006188D3`, `0x00618A7C`, `0x0061BD06`). Each call of the
interpolator `0x0060E4C0` subtracts `dt` from `+0x43C`, floors it at zero and
replaces each working endpoint with `target + (working - target) * w`, where
`w = (+0x43C) / 30.0`. The remaining distance is multiplied by a factor that
falls linearly from 1 to 0 while the timer runs, so the approach is faster than
a straight line. At zero the working endpoints equal the targets. The timer runs
for `30 / dt` updates. INFERRED. With `dt = elapsed * 30 / 300000` the timer
ends after 300000 elapsed units. The unit is unresolved, and the
[screen environment page](../resource/screen-env-glare-latitude.md) leaves the
same unit open.

What bahamut-client can change. After every key change, replace the eye and
look-at each frame with `target + (current - target) * w` and let `w` fall
linearly from 1 to 0 over the timer. The duration in seconds stays a
CALIBRATION value until the elapsed unit is known.

## Collision and look-at

The key 2 endpoint producer's guarded query steps, the 0.5 threshold at
`0x00FF12D0` and the -0.2 offset at `0x00FF12E8` are on the
[endpoint page](camera-endpoint-calculation.md). This page adds no collision
constant. The query helpers call the geometry producer `0x00AF94D0`, so the pull
distance depends on runtime geometry. The look-at is the object vector from
`0x00664100`, built from the actor's `+0x88`, `+0x94` and `+0x9C` methods, and
its height is not recoverable from the instructions alone.

What bahamut-client can change. Keep clearance and look-at height as
CALIBRATION values.

## Script natives and packets

No opcode in the tracked catalogs carries a camera name. The client script
natives `_lockCameraControl`, `_unlockCameraControl`, `_isCameraControlEnabled`
and `_forceCameraTPSMode` (`PlayerBaseClass`) and `_aimCameraTutorial`,
`_cancelAimCameraTutorial`, `_waitForCameraTutorial`, `_aimCameraChocobo` and
`_cancelAimCameraChocobo` are listed in
`xivl-client-scripts:lua/napi_index.json`. Their C++ stubs start at
`0x00732E20`, `0x00732F70`, `0x007330C0`, `0x00735970`, `0x0073B7F0`,
`0x0073B940`, `0x0073A830`, `0x0073BD30` and `0x0073BE80`. Their effect on the
camera flags is unresolved. The server can already gate the camera commands
through the user-control groups of the s2c 0x0193 page cited above.

What bahamut-client can change. Nothing until the natives' effect is resolved.

## Open questions

The manifest lists these as `unresolved`.

- The unit of the update's elapsed integer.
- The look-at object vector and the collision margin.
- The follow logic of keys 0 and 4 beyond the radius and rotation reads above.
- The payload and effect of command 0x29 and the lock-key drag methods.
- The producers of the focal, offset and invert bytes.
- The user-level meaning of `0x00616F90`, `0x007A3A60` and the pointer-handler
  predicate `0x004D7580`.
- The camera effect of the script natives above.
- The argument values of `MoveCamera` for held keys or a pad, and the key 0
  rates.
- Whether the predicate `0x007A4610` holds during ordinary walking.
- The consumers of commands 0x2E, 0x2F and 0x30. The lens arm sends command
  0x2E with two floats for keys 0 and 2 (`0x00619A22`).

## Reproduction

Hash the executable and map each VA through the section table. Decode the
ranges named in the manifest's `staticChecks` and compare the bytes. The
verifier does this with `python3 tools/verify_camera_input_chain.py --exe
<path>`. The tests in `tools/test_promoted_static_checks.py` build a synthetic
image from the rows, flip one byte per row and shift the addresses to show each
row is enforced, and exercise the numeric model for the wheel, drag, lens,
blend and key rules. The model encodes the disassembly reading and is checked
only against the constant rows. It is not executed against the binary, so a
comparison direction or a branch order in it is not verified by the byte rows.
