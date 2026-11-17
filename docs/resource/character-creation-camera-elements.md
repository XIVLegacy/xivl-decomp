# Character creation camera elements, swatches and world marks

The character creation scene looks at one of three points of the created
actor, and its dialogs read colour entries and a world population word. This
page records the CharaActor camera elements and the look-at helpers that read
them, the yaw and distance update of the creation camera context, the senders
of the creation view state, the colour swatch writers and the world crowding
marks. The camera commands, the view state routine `0x006AAC60` and the
creation window table are in
[character select status block](character-select-status-block.md#creation-camera-context).

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32
image base `0x00400000`. Addresses are VAs. The instructions were read with
Capstone `5.0.7` over the file-backed sections and with the `ghidra` CLI
`disasm` and `x-ref`. Float and string constants were read from the
file-backed data. STATIC means read from instructions or data. INFERRED means
concluded from a STATIC read without tracing the use. The install reads are
in `xivl-client-data:docs/skeleton-cib-and-ik-members.md`.

## CharaActor camera elements

STATIC. The CharaActor initializer `0x00664890` looks up ten element names
through virtual `+0x54` of the actor and stores the ids at `+0x1690`
(arm `0x00664D34..0x00664DED`):

| Name | String | Id at |
| --- | --- | --- |
| `EID_CAMINT_HI` | `0x00FC01F4` | `+0x1690` |
| `EID_CAMINT_LOW` | `0x00FC0204` | `+0x1694` |
| `EID_CAM_HEAD` | `0x00FC0214` | `+0x1698` |
| `EID_CAM_FACE` | `0x00FC0224` | `+0x169C` |
| `EID_CAM` | `0x00FC0234` | `+0x16A0` |
| `EID_CAM_SIT` | `0x00FC023C` | `+0x16A4` |
| `EID_CAM_CFT` | `0x00FC0248` | `+0x16A8` |
| `EID_CAM_GRD` | `0x00FC0254` | `+0x16AC` |
| `EID_CAM_CHO` | `0x00FC0260` | `+0x16B0` |
| `EID_CAM_GB` | `0x00FC026C` | `+0x16B4` |

STATIC. It then caches the matrix of each element, through virtual `+0x60`,
in 0x40 byte slots (`0x00664E14..0x00665054`): `+0x16C0` CAMINT_HI, `+0x1700`
CAMINT_LOW, `+0x1740` CAM_HEAD, `+0x1780` CAM_FACE, `+0x17C0` CAM, `+0x1800`
CAM_SIT, `+0x1840` CAM_GRD, `+0x1880` CAM_CHO, `+0x18C0` CAM_GB and `+0x1900`
CAM_CFT. It stores the actor position (virtual `+0x88`) added to the CAMINT_HI
translation row at `+0x1940` (`0x006650A7`).

MEASURED. All nine `client/chara/pc/c00N/skl/0001` skeletons hold the ten
names. `client/chara/pc/cmn/0001` holds `EID_CAMINT_HI` and `EID_CAMINT_LOW`
only.

## Look-at helpers

STATIC. `0x00664100(out, t)` returns the cached `+0x1940` when bit 9 of
`+0x2B70` is clear (`0x00664121`). Otherwise it builds the actor basis from
virtual `+0x9C` (scale) and `+0x94` (rotation, through `0x00652200`), reads the
byte at `+0x1350` (`0x0066420B`) and switches on it through the byte table
`0x00664404` and the jump table `0x006643F0`:

- values 1, 3 and 13 take the CAM_GRD matrix (`+0x1840`)
- 11 takes CAM_SIT (`+0x1800`)
- 15 and 91 take CAM_CHO (`+0x1880`) when the dword `+0x2B9C` is 0 and
  CAM_GB (`+0x18C0`) otherwise
- 30, 31 and 32 take CAM_CFT (`+0x1900`)
- every other value blends the CAMINT_HI and CAMINT_LOW translations by `t`,
  with `t` limited to 0..1

In each case the translation is carried through the actor basis and added to
the position, and the result is stored at `+0x1940` (`0x006643C4`). With
`t = 0.5` (`0x00F62F60`) the blend is the midpoint of the two translations.

STATIC. `0x00660090(out)` takes the CAM_FACE translation (`+0x1780`) through
the actor basis and adds the position. `0x006601D0(out)` takes the vector
`(0, 0.6, 0, 0)` through the actor basis and adds the position. The vector is
the static at `0x0133F060`, filled once under the guard byte `0x0133F070` from
the float 0.6 at `0x00F62F78` (`0x00660235..0x0066025A`).

STATIC. The creation context update `0x007F27B0` calls them by the mode at
`+0x50`: 0 calls `0x00664100` with 0.5, 1 calls `0x00660090` and 2 calls
`0x006601D0` (`0x007F28D3..0x007F2912`). The result is stored at `+0x30`.

INFERRED. View state 2, the body view, looks at the midpoint of the two
CAMINT elements. View state 3, the close view, looks at the CAM_FACE joint.
View state 4 looks 0.6 above the actor origin.

## Yaw and distance update

STATIC. The handlers `0x007F12B0` (view state 2), `0x007F1170` (3) and
`0x007F13F0` (4) take the component 1 of the actor rotation (virtual `+0x94`
of the actor from `0x00616310`), multiply it by 180.0 (`0x00F62F40`) over pi
(`0x00F62F48`) and store it as the yaw goal at `+0x0C`. States 2 and 3 add
180.0 and state 4 adds nothing. The goal is wrapped into -180..180 with the
constants `0x00FF0FC0` (-180), `0x00FF0FBC` (180) and 360 (`0x00FC3300`). A
negative current yaw `+0x08` with a positive goal then gains 360, and a
positive yaw with a negative goal loses 360 (`0x007F139A..0x007F13CC`). With no
actor the goal is left as it was.

STATIC. The update `0x007F27B0` in yaw mode `+0x54` equal to 0:

- the step share is `dt * 0.3` (double `0x00FB7DE0`), taken as 1.0 when it
  exceeds 1.0 (`0x00F598A0`), applied to `goal - yaw`
- the step is limited to `dt * 15.0` (`0x00F62FD8`)
- when the result is within 1.0 of the goal, the mode `+0x54` becomes 1 and
  the yaw is set to the goal (`0x007F2A9B..0x007F2AA7`)
- the yaw is wrapped into -180..180 and turned to radians for the rotation
  `0x0060D290`, which uses the cosine helper `0x009D6280` and the sine helper
  `0x009D63B0` and gives the rows `(c, 0, -s)` and `(s, 0, c)`

In yaw mode 1 the yaw gains the drag `+0x1BC / 100` (negated when bit 3 of the
byte `+0x1C6` is set) times `dt` times 3.5 (`0x00FF12A8`).

STATIC. The distance goal `+0x14` gains `+0x1C2 / 2000` (`0x00FF12B0`) times
`dt`, unless bit 0 of the actor byte `+0x2B78` is set, and `+0x1BE / 100 * 0.1`
(`0x00F55AD0`, `0x00FBAD50`) times `dt`. It is limited to 1.0 at the low end
(`0x00F54F70`) and 5.0 (`0x00FA7B20`) at the high end.

STATIC. The ramp `+0x18` gains 1/60 (`0x00FEE2B8`) a frame while it is below
0.5. The current distance `+0x10` closes on `+0x14` by `+0x18 * dt`, limited to
1.0. The look-at point `+0x40` closes on the goal `+0x30` by the same share.
The rotation of the yaw is applied to `(0, 0, -distance)` through `0x0060CE50`
with the distance before this update. The eye, stored at camera `+0x330`, is
the look-at point before this update plus that offset. The look-at point after
the update is stored at camera `+0x340`.

## View state senders

STATIC. A scan of every `E8` and `E9` opcode in the code section and every
dword occurrence of `0x006AAC60` in the file finds 12 calls and no other
references. 10 push 2 or 3 as listed in the
[view state table](character-select-status-block.md#view-state-b3e94). The
other two are in `0x0085E840`, which loads the dword `+0x29C` of its Button
argument, subtracts 3 and, for the result 0 to 5, picks a jump table entry
`0x0085E888` through the bytes at `0x0085E890`. The values 3, 4, 5, 7 and 8
send state 3 (`0x0085E86C`) and any other value, including 6, sends state 2
(`0x0085E880`). A scan finds no call, jump or absolute reference to
`0x0085E840`.

STATIC. No call of `0x006AAC60` pushes 4. Camera event `0x1E` (view state 4)
is sent by `0x0055C990`, which the BootupManager update command 34 reaches.
The command switch `0x006B0683` reads `[esi+0x44]`, subtracts 1 and indexes
the table `0x006B25EC`, whose entry 33 is `0x006B1505`. That arm reads the
payload dword at `[[esi+0x48]]`. The values 2, 3 and 4 call `0x0055C980`
(`0x1D`), `0x0055C970` (`0x1C`) and `0x0055C990` (`0x1E`). The value 1 calls
`0x0055F250`, the value 5 is the focus arm already recorded, and 0 calls
`0x004D6750`, `0x0055F1B0` and `0x0055F270`. The poster of command 34 was not
traced.

STATIC. The BootupManager initializer calls `0x0055C980` at `0x006AF4EE` and
`0x006AF592`, so the scene starts in view state 2 once the background exists.

## Colour swatches

STATIC. `0x00863830` reads the window id at `+0x1C0`, finds the row of the
table `0x012D1738`, takes the colour cell list of dword 3 through
`0x006AA030` and, for each of 32 slots whose element `+0x3F0 + 4 * i` is
bound, stores the entry `i` at `+0x1CC + 4 * i` and calls `0x0085EAE0(element,
entry, 0)`. The slot whose Button value equals `+0x1C4` also gets virtual
`+0xB4` slot `+0x38` of the Button. `0x0086D0D2..0x0086D166` binds the slot
elements by the name `Border_CaraMakeColorChip_%d` (`0x0104A234`), 1 to 32.

STATIC. `0x0085EAE0(element, colour, custom)`:

- colour `0xFFFFFFFF` writes the style `Border_CharaMakeColorPaletNot`
  (`0x0104A4B0`) through the `SqwtStyle` descriptor `0x01358958` and sets the
  three channel writers to 1.0 (`0x0085EB73..0x0085EBAC`)
- any other colour writes `Border_CharaMakeColorPaletBase` (`0x0104A4EC`) and
  sets `VisualOpacityRed`, `VisualOpacityGreen` and `VisualOpacityBlue`
  (`0x0106C4EC`, `0x0106C500`, `0x0106C514`) to byte 0, byte 1 and byte 2 of
  the colour divided by 255.0 (`0x00F598B0`) through `0x00940320`,
  `0x00940380` and `0x009403E0`
- a nonzero `custom` takes `Border_CustomColorChipNot` (`0x0104A4D0`) and
  `Border_CustomColorChipBase` (`0x0104A50C`) instead

STATIC. The hex reader `0x006A8F50(text)` returns `0xFFFFFFFF` when the first
or second character is NUL. Otherwise it reads up to six pairs of digits. A
digit is `0x30..0x39` minus 0x30, `0x61..0x7A` minus 0x57 or `0x41..0x5A` minus
0x37, so any letter is a digit up to 35 (`0x006A8F80..0x006A8FEB`). Each pair
adds `(high * 16 + low) * m` where `m` starts at 1 and shifts left 8 after
each pair (`0x006A8FF5..0x006A8FFD`), so only the first four pairs contribute.
The loop ends after six pairs or at a NUL read before a pair
(`0x006A9000..0x006A900D`). A character in none of the three ranges leaves the
previous digit in the register, which was not modelled.

INFERRED. The text `b49a8b` reads as red `0xB4`, green `0x9A` and blue
`0x8B`, because the first pair lands in the lowest byte and `0x0085EAE0`
divides byte 0 for red. The one character entry `0` of the highlight list is
no colour.

## World crowding marks

STATIC. `Border_WorldCrowded` (`0x0104BD94`) is bound for each world row at
`0x0086CBE8`. `0x00863680(element, value)` walks the table `0x0104BC74` of
`(threshold, style)` pairs from the entry at `0x0104BC84` down to the first
entry, and takes the first whose threshold is at or below `value` as an
unsigned compare (`0x008636AD..0x008636C1`):

| Threshold | Style | String at |
| ---: | --- | --- |
| 0 | `Border_WorldSelectCrowded1` | `0x0104BC20` |
| 60 | `Border_WorldSelectCrowded2` | `0x0104BC3C` |
| 80 | `Border_WorldSelectCrowded3` | `0x0104BC58` |

It writes the style through the `SqwtStyle` descriptor `0x0134F0C8`.

STATIC. The row fill `0x0086CE88..0x0086D072` tests byte `+3` of each world
record, the high byte of the list position word, against the masks 1 at
`0x012D171A`, 2 at `0x012D171B` and 4 at `0x012D171E`. With none set it calls
`0x00863680` with the record dword `+4` (the population word), writes 1.0
through `0x009402C0` and the Button style `Button_WorldSelect` (`0x0104BDE4`).
With any set it writes 0.5 (`0x00F62F60`), writes 1 through `0x0093C2A0` and
sets the style `Button_WorldSelect_disable` (`0x0104BDC8`). The elements that
`0x009402C0` and `0x0093C2A0` reach were not read.

## Open

- The poster of BootupManager command 34 and what selects camera key 9.
- The creation slot offsets `B+0x194 + 4 * mode`.
- The elements reached by `0x009402C0` and `0x0093C2A0` in the world row fill.

## Reproduction

```text
ghidra disasm 0x00664D34 -n 100
ghidra disasm 0x00664E14 -n 140
ghidra disasm 0x00664100 -n 200
ghidra disasm 0x00660090 -n 80
ghidra disasm 0x006601D0 -n 60
ghidra disasm 0x007F12B0 -n 90
ghidra disasm 0x007F27B0 -n 340
ghidra x-ref to 0x006AAC60
ghidra disasm 0x0085E840 -n 20
ghidra disasm 0x006B1505 -n 40
ghidra disasm 0x00863830 -n 80
ghidra disasm 0x0085EAE0 -n 120
ghidra disasm 0x006A8F50 -n 60
ghidra disasm 0x00863680 -n 20
ghidra disasm 0x0086CE88 -n 120
```

Read the strings at `0x00FC01F4..0x00FC026C`, `0x0104A234`, `0x0104A4B0..0x0104A50C`,
`0x0104BC20..0x0104BC58`, `0x0104BD94..0x0104BDF8` and `0x0106C4EC..0x0106C514`,
the bytes at `0x006643F0..0x0066445F`, `0x0085E888..0x0085E895`, the dwords at
`0x0104BC74..0x0104BC8C` and the bytes at `0x012D171A..0x012D171E`.
