# Sun and moon node orientation

The SunMoon clip update moves a sky body along its orbit and then orients the
body node. This page records the orientation function `0x008D3400`, the moon
phase index it computes and the roll it applies. The clip records and the orbit
position are in the
[sky celestial layout census](sky-celestial-layout-census.md#sunmoon-clip-v-0x01060f4c-vtable-enter-0x008d2a40-update-0x008d3990).

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Instruction listings from ghidra-cli 12.1.4
were treated as leads and decoded again with Capstone 5.0.7 over the file-backed
PE sections. The sequences cited here are the `staticChecks` rows of
[`config/sun_moon_orientation.json`](../../config/sun_moon_orientation.json),
and `tools/verify_sun_moon_orientation.py --exe` compares each one with the
pinned bytes.

STATIC means read from instructions or data in the pinned executable. INFERRED
means the evidence is consistent with the claim and does not close it.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| The function runs after Move on every update and skips for clip mode 4, for a record word `+0x28` of zero, and without a camera actor | STATIC | [Gates](#gates) |
| A record with word `+0x28` of zero, which the census gives to the sun, receives no rotation from this function | STATIC for the gate, the record value is from the census | [Gates](#gates) |
| The camera actor is the type `0x0C` cache of the actor container, read at container `+0x64` | STATIC | [Camera actor](#camera-actor) |
| The moon phase index is `(((T + 43200) / 86400) >> 2) & 7` for the Eorzea clock `T`, unless an override cell is nonnegative | STATIC | [Moon phase index](#moon-phase-index) |
| The override is used unmasked, defaults to `0xFFFFFFFF` and is set by scene message 116 | STATIC | [Moon phase index](#moon-phase-index) |
| The index enters only as a roll about Y of index times 45 degrees | STATIC | [Roll](#roll) |
| The remaining rotation is a look-at basis from the node toward the camera with up `(0, 1, 0, 0)` | INFERRED | [Look-at basis](#look-at-basis) |
| The clip mode is the type id of the bound instance property, and mode 4 is a light | STATIC for the ids, INFERRED for the binding | [Clip mode](#clip-mode) |
| A mode 4 clip orients its node from the orbit position alone, and the light shines from that position toward the orbit centre | STATIC for the calls and outputs, INFERRED for the direction | [Mode 4 light path](#mode-4-light-path) |

## Gates

The update at `0x008D3990` calls Move `0x008D37A0`, then `0x008D3170`, then the
orientation function at `0x008D3A09`. The function returns without a rotation
in three cases.

- The clip mode dword at clip `+0x40` equals 4 (`0x008D3425`).
- The dword at record `+0x28` is zero (`0x008D3439`).
- The actor container lookup or the camera slot read gives null
  (`0x008D3448` to `0x008D3464`).

The census lists the record word at `+0x28` as zero in the sun record and one in
the moon and light records. With that value the sun disc is never rotated here
and keeps whatever rotation its node already holds, while the moon and the two
light records are rotated every update. The [clip mode](#clip-mode) section
gives the source of the mode dword at `+0x40`.

## Camera actor

The function takes the container from the callback input through the getters
`0x00A20030` and `0x0060B360` (the
[typed chain](attribute-resource-and-scene-binding.md) of the actor container)
and reads the word at container `+0x64`. The container dispatcher sits at
container `+0x54`, and the type `0x0C` store in `0x007A02D0` writes dispatcher
`+0x10` at `0x007A0345`, which is container `+0x64`. Callback index 1 of the
registration at `0x007A0450` holds the factory `0x0079F7E0`, and that factory
calls the CameraActor constructor `0x0061B5A0`. The slot is therefore the
cached CameraActor. The function reads a vector from camera vtable `+0x88`.
The [camera object inputs](../actor/camera-object-inputs.md) page describes
the 16-byte vector getter at that slot. Treating that vector as the camera
position is INFERRED.

## Moon phase index

The function reads the weather manager through `0x007E02F0`, which returns the
word at `+0x158` of the global at `0x0133DEF4`. The override getter `0x007E06E0`
returns manager `+0x130`. The constructor stores `0xFFFFFFFF` there at
`0x007E8C8B`. A nonnegative value is converted to float as is. A value
above 7 is not masked.

Otherwise the function reads the 64-bit clock T through `0x007E0430`, which
selects manager `+0x108` or, when manager `+0xF4` is nonzero, `+0x120`. The
[time-of-day finding](zone-lighting-source-and-time-of-day.md#time-of-day)
owns the interpretation of those clocks as Eorzea seconds. The function adds
`43200` to T and divides the unsigned 64-bit sum by 60, 60 and 24 with the
helper `0x009D5880`. It then keeps `(quotient >> 2) & 7`.

```text
day   = (T + 43200) / 86400
index = (day >> 2) & 7
```

The index changes at Eorzea 12:00 and advances every fourth Eorzea day, so the
eight values repeat every 32 Eorzea days. Scene message 116 reaches the override
setter `0x007E06D0` through the switch at `0x0062CDA0` (byte table entry `0x14`,
jump entry `0x0062CF56`). The sender of that message and the values it uses are
not traced.

| T (Eorzea seconds) | Index |
| ---: | ---: |
| 0 | 0 |
| 302399 | 0 |
| 302400 | 1 |
| 2376000 | 7 |
| 2721600 | 0 |

## Roll

The function converts the index to float and multiplies it by `45.0` in double
precision. It rounds to float, multiplies by pi, divides by `180.0` and rounds to
float again. It passes that angle to
`0x0042FDC0`, which writes the row-major matrix

```text
( cos  0  -sin  0 )
(  0   1    0   0 )
( sin  0   cos  0 )
(  0   0    0   1 )
```

using the FCOS routine `0x009D6280` and the FSIN routine `0x009D63B0`. This is a
rotation about the Y axis by index times 45 degrees. The index is not used
anywhere else in this function, and index 0 gives the identity matrix. Whether a
different writer picks a moon texture region is not traced.

## Look-at basis

The function reads the node position through target vtable `+0x30` and adds the
position of the object at clip `+0x3C` when that object exists. It subtracts the
sum from the camera vector, normalizes the difference through `0x007D8DC0`
(an `RSQRTPS` refinement, INFERRED to be a normalization) and calls the basis
builder `0x008D2C90` with that vector as fourth argument, a zero vector as fifth
and `(0, 1, 0, 0)` as sixth. The three outputs are packed into matrix rows by
`0x008D30A0`, multiplied with the roll matrix by `0x0061D050`, converted to
angles by `0x007DD840` and set through target vtable `+0x3C`. `0x007DD840`
calls the CRT dispatcher `0x009DAEBA` three times and `0x00652200` once, and
tests a matrix lane against 0.0 (`0x01086650`) to pick 1.0 or -1.0
(`0x007DD8D8..0x007DD9F0`). Reading the three results as Euler angles from
`atan2` is INFERRED. The same call
chain serves the moon and the directional light records. What the rotation does
to a light direction is not traced.

The call arguments are STATIC. Reading the builder as a look-at toward the
camera with world up is INFERRED. The axis role of each output row, the
multiplication order and the handedness are not established here.

## Clip mode

STATIC. The enter `0x008D2A40` finds the object of the list at actor
container `+0x78` whose `+4` sub-object is the clip's `+0x44`, keeps it at
clip `+0x48` (`0x008D2C05`), and calls the binder `0x008D2870` at
`0x008D2C26`. The binder calls virtual `+4` of the sub-object at `+0xE0`
with the type ids 1, 2, `0xF` and 4 in that order (`0x008D288D`,
`0x008D28AA`, `0x008D28C7`, `0x008D28E4`). It stores the first non-null
answer at clip `+0x3C` and its type id at clip `+0x40`, or -1 at `+0x40`
when the last query also fails (`NEG`, `SBB`, `AND 5`, `ADD -1` at
`0x008D28EC..0x008D28F3`). The enter then overwrites clip `+0x40` with
virtual `+4` of the object that virtual `+0xBC` of the object that virtual
`+0x64` of the enter's input object returns, when each is non-null
(`0x008D2C42..0x008D2C6A`).

STATIC. Slot 1 of four instance property vftables returns a constant:

| Class | vftable | Slot 1 | Value |
| --- | --- | --- | --- |
| `RaptureBGChipInstanceObjectProperty` | `0x00FF242C` | `0x007254A0` | 1 |
| `RaptureBGPartsInstanceObjectProperty` | `0x00FF2534` | `0x00B8D680` | 2 |
| `RaptureLightInstanceObjectProperty` | `0x00FF2A9C` | `0x00602600` | 4 |
| `RaptureVfxInstanceObjectProperty` | `0x00FF2E84` | `0x005F0660` | `0xF` |

The class names come from the RTTI complete object locator at vftable -4.

INFERRED. The binder's ids are the BGChip, BGParts, Vfx and Light property
types, and a clip bound to a light instance runs in mode 4. That virtual
`+0xBC` of the instance returns its property object is not traced.

## Mode 4 light path

STATIC. The update calls `0x008D3170` at `0x008D39FB` before the
orientation function. `0x008D3170` acts only for clip mode 4
(`0x008D31B8`) and reads no record word `+0x28`. It calls `0x008D3010`
(`0x008D31D5`), which reads the node position P through target vtable
`+0x30` and calls the basis builder `0x008D2C90` with eye P, at
`(0, 0, 0, 0)` and up `(0, 1, 0, 0)` (`0x008D302C..0x008D3087`).

STATIC. The builder `0x008D2C90(out1, out2, out3, eye, at, up)` forms
`at - eye` (`SUBPS` at `0x008D2D2B`) and normalizes it through
`0x0042EAD0` into Z. It forms two further vectors through the cross product
routine `0x0042E930` and `0x0042EAD0` (`0x008D2E9D..0x008D2ECC`). It writes
the negated first of them to out1 (`0x008D2F0A..0x008D2F58`), the second to
out2 (`0x008D2F5D..0x008D2F6D`) and the negated Z to out3
(`0x008D2FAB..0x008D2FF3`).

STATIC. `0x008D3170` packs the three outputs as matrix rows 0, 1 and 2 with
row 3 `(0, 0, 0, 1)`, applies no roll, and passes the matrix to
`0x007DD840` (`0x008D3207..0x008D32B2`). It then sets the target's position
through vtable `+0x34` from the static `0x013553B8`, its angles through
vtable `+0x3C` and its scale through vtable `+0x44` from the static
`0x013553A8`. The orientation
function returns for mode 4 (`0x008D3425`), so the camera vector and the
roll never reach a light.

STATIC. `0x00C2F8F0` multiplies the node matrices `[node+0xD8]+0x30` and
`[node+0xD4]+0x10` through `0x0061D050` and builds the vector
`(0, 0, -1.0, 0)` with -1.0 the float at `0x00F62F80`
(`0x00C2F91E..0x00C2F987`).

INFERRED. With eye P and at the origin, out3 is `p = normalize(P)`, the unit
orbit position, so row 2 of a light node is p. The light direction, `(0, 0,
-1, 0)` times the node matrix, is `-p`: the light travels from its orbit
position toward the orbit centre with no camera term. The cross product
order of `0x0042E930`, and so the sign of rows 0 and 1, is not established
here.

## Consumer implications

- Compute the phase index with the formula above from the Eorzea clock. A
  model that has no override should treat the override as `0xFFFFFFFF`.
- Apply the index only as a rotation of the moon node about Y by index times 45
  degrees. This function selects no texture region.
- Do not rotate the sun disc from this function, per the census record value
  of word `+0x28` (zero for the sun), which is decoded sky data and not read
  from the executable bytes. This function applies a rotation to the moon node
  and to the light nodes whose record word is nonzero on each update. The
  toward-camera facing of that rotation is INFERRED and a stand-in until the
  axis roles are decoded.
- Keep the facing and roll composition as a stand-in until the axis roles and
  the multiplication order are decoded.
- Orient a lane light from its orbit position alone and take its direction
  as minus the unit orbit position. Do not gate a light on record word
  `+0x28`, which the mode 4 path does not read.

## Not established

- The axis roles of the look-at outputs, the multiplication order and the
  handedness of the final rotation.
- Whether any other writer selects a moon texture region.
- The instance accessor behind the mode overwrite of the enter and the
  composition of the angle setter at target vtable `+0x3C`.
- The sender of scene message 116 and its value range.

## Reproduction

Use the pinned executable. Run from the repository root:

```text
python tools/verify_sun_moon_orientation.py
python tools/verify_sun_moon_orientation.py --exe orig/ffxivgame.exe
python tools/test_promoted_static_checks.py
python tools/validate_repo.py
```

The first form checks the manifest and its mutations. `--exe` checks the pinned
size and SHA-256 and every `staticChecks` row against the executable bytes. The
`--exe` form is a local retail-input gate that needs the pinned executable, so
CI runs the manifest, mutation and model tests only.
