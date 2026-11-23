# Footstep detector and ground material

The [footstep pair](sound-effect-triggers.md#footsteps) `0x004B7790` plays one
sound from `data/08/07` and one from `data/1A/46`. This page records the
CharaActor member that calls it, the foot contact detector, the source of
each of its nine arguments, and the ground material word that selects the
first argument group.

## Inputs and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI disassembly and
cross references supplied the instructions. Reads of the file-backed PE
sections supplied strings, pointer cells and float constants. The ground
material census reads every `SEDBPHB` file of the same install's `data/`
tree. No target process ran.

C is a CharaActor, H its movement helper at `C+0xBF0` and V its CharaVisual
at `C+0x2B5C`, as on the
[locomotion selection](../actor/locomotion-selection.md) page.

## Footstep member

STATIC. The member F sits at `C+0x2130`. Actor setup `0x00664890` calls its
resolver `0x007D0560` with ECX `C+0x2130` (`0x00664A6D..0x00664A73`). The
actor update `0x006679C0` (CharaActor slot 142) loads `C+0x2130` into EDI
(`0x006681F0`), passes the H getter `0x007A4A60` result to `0x007D0750`
(`0x00668201`) and calls the member update `0x007D0FF0` (`0x0066820F`).
The CharaSoundController update `0x008501A0` runs just before, with ECX
`C+0x21D4` (`0x006681DF..0x006681E5`).

Fields read by the routines below: nine element indices at F`+0x00..+0x20`
(negative when unresolved), nine signed side bytes at F`+0x24`, nine f32
timers at F`+0x30..+0x50`, the last foot at F`+0x54`, the H scalar at F`+0x70`,
the actor at F`+0x74` and the sound module at F`+0x78`.

The pointer cell `0x012C5DF8` begins nine name pointers: `EID_SE_FOOT_R`,
`EID_SE_FOOT_L`, `EID_SE_FOOT_R2`, `EID_SE_FOOT_L2`, `EID_SE_FOOT_R3`,
`EID_SE_FOOT_L3`, `EID_SE_FOOT_R4`, `EID_SE_FOOT_L4` and
`EID_SE_FOOT_BODY_DYN`. The cell `0x012C5DF0` begins two more,
`EID_MOT_FOOT_R` and `EID_MOT_FOOT_L`.

Direct callers:

| Callee | Callers |
| --- | --- |
| `0x007D0FF0` member update | `0x0066820F` |
| `0x007D0AB0` detector | `0x007D1150` in `0x007D0FF0` |
| `0x007D0D70` play | `0x007D11B5` in `0x007D0FF0`, `0x007D126E` in `0x007D11E0` |
| `0x007D11E0` explicit trigger | `0x0065F00E` in `0x0065EFE0` |
| `0x0065EFE0` | `0x0081C6C8` in `0x0081C650` |

`0x0081C650` is slot 1 of the `RaptureFootstepTriggerClip` table, cell
`0x0100EC50` ([slot catalog](../../config/ffxivgame.vtable_slots.jsonl)).
`0x007D0D70` is the only direct caller of `0x004B7790` (`0x007D0EA3`).

In `0x007D0FF0`, a detected foot plays through `0x007D0D70` with the kind from
`0x0065C190`, which forwards V to `0x006B6FA0`, unless that kind is 0 or `0x12`
or `0x0065C1F0`, the low three bits of V`+0x48`, is nonzero
(`0x007D1190..0x007D11B5`). Those cases take another path that was not read.

## Detector `0x007D0AB0`

STATIC. When the gate `0x0065B1A0` on the actor fails the detector returns 9,
no foot (`0x007D0AC8..0x007D0AD2`, `0x007D0B2A`). Otherwise, for slot i from 0
to 8 with a resolved index and a timer not above 12.0 (f32 `0x00FEB198`), it
calls the actor's virtual `+0x60` with the index for a 4 by 4 matrix and tests
its f32 at `+0x34` against 0 (`0x007D0AD6..0x007D0B04`):

- at or above 0, the side byte becomes 1 (`0x007D0B40`)
- below 0, with the side byte above 0 and either i different from the last
  foot or the timer at or below 0, the timer becomes 20.0 (f32 `0x00FEB194`),
  the side byte -1 and the last foot i, and i is returned
  (`0x007D0B47..0x007D0B62`)
- below 0 otherwise, the side byte becomes -1

INFERRED. The `+0x34` float is the y of the element's translation row, so a
contact is the first frame a foot element passes below the actor origin
after it was at or above it.

## Play arguments

STATIC. `0x007D0D70(ground, foot, kind)` pushes the nine arguments of
`0x004B7790` at `0x007D0E62..0x007D0EA2`. `0x007D09A0` returns without
popping its argument, which stays on the stack as A8.

| Arg | Source | Values |
| --- | --- | --- |
| A1 | `0x007D09F0` | position of the foot element |
| A2 | `u32(0x00FEA8D8 + 4 * (material / 10 - 1))` | 9, 1, 3, 7, 8, 2, 10, 5, 0, 11 |
| A3 | `0x006B82D0` on V | 0 for body class 1, 1 for body class 2 |
| A4 | `0x006B6E40`, byte V`+0x37E` | 0 when 1 or 4, else 1 |
| A5 | `0x007D0900(ground)` | 0 when the string at ground `+0x64` contains `_00` (`0x00FEB188`) and the dword at `0x012C5EC0` is 8005 or 8007 to 8010, else 1 |
| A6 | F`+0x70` against the binary64 0.1 at `0x00FBAD50` | 6 when below, else 3 |
| A7 | `0x007D09A0` | 0 to 4 |
| A8 | the same compare | 7 when below, else 4 |
| A9 | `0x007D0BB0(kind)` | f32, below |

- The material is the dword at ground `+0x9C`. A value whose `material / 10 - 1`
  is outside 0 to 9 returns before any play (`0x007D0D77..0x007D0D9E`).
- `0x006B82D0` returns 3 when V`+0x54` is nonzero, else `0x006B6F40` of V`+0x58`.
  That table gives class 1 for 1, 3, 5, 7, 9, 12, 21 and 24, class 2 for 2, 4, 6,
  8, 10, 11, 22, 23 and 25, and 0 otherwise. Any class other than 1 or 2 returns
  before any play (`0x007D0DAD..0x007D0DC4`).
- A7 maps the `0x006B6CD0` result through the table at `0x007D09D8`: 1 to 3,
  2 to 0, 3 to 1, 4 to 4, and every other value to 2.
- `0x006B6FA0` maps V`+0x58`, when V`+0x54` is 0, as 1 to 7 to themselves, 8, 9
  and 10 to 10, 11 and 12, 11 to 8, 12 to 9 and 21 to 25 to 13 to 17.
- `0x007D0BB0` loads entry `kind` of a 19-entry stack table built from 0.0,
  1.0 (`0x00F54F70`) and the f32 values 1.1, 0.73, 1.45, 1.38 and 0.87 at
  `0x00FEB19C..0x00FEB1AC`. By the unmapped V`+0x58` value, A9 is 1.0 for 1, 2,
  8, 9, 10 and 12, 0.87 for 3, 4 and 23, 1.38 for 5 and 21, 1.45 for 6 and 22,
  0.73 for 7 and 11 and 1.1 for 24 and 25.

With the sums on the [footstep pair](sound-effect-triggers.md#footsteps)
page, the first play is `0x08070002 + A6 + 8*A5 + 16*A4 + 32*A3 + 64*A2`
and the second `0x1A460000 + 9*A7 + A8`. INFERRED from the values: A9 follows
the body sizes of the player rigs and reads as a pitch.

## Ground material

STATIC. The ground object's probe `0x007D8840` calls `0x007D5DF0`, which
builds a ray from the f32 1.0 (`0x00F54F70`) and -30.0 (`0x00FEE2A8`) on the
up axis, and passes the hit to `0x007D4130` (`0x007D8924..0x007D8936`). When
the dword at object `+0x9C` is then 1 or `0x66` it probes again through
`0x007D5D50` and reads that hit the same way (`0x007D8941..0x007D8986`).

`0x007D4130(hit, out, ...)` asks the hit shape for the named data
`RaptureAttribute` (`0x00FED90C`) through its virtual `+0xC0`
(`0x007D41FC..0x007D4211`). It requires the data size to be at least twice
the hit's triangle index, read at hit `+0x20` (`0x007D4217..0x007D4220`). At
the u16 at data `+ 2 * index` it stores the low seven bits at out `+0x08`
(`0x007D4228..0x007D422E`) and bit 12 through a further output pointer
argument when it is nonnull (`0x007D4231..0x007D4242`),
and passes bits 7 to 11 to shape virtual `+0x198` and then `+0x1A0`
(`0x007D4244..0x007D426C`). The probe passes the record at object `+0x94` as
out, so the material lands at object `+0x9C`.

MEASURED over every file below `data/` that begins `SEDBPHB`. GBD offsets
follow `xivl-tools:docs/formats/phb-collision.md`, with GBD at file `0xC0`.

| Observation | Count |
| --- | ---: |
| PHB files | 5203 |
| GBD `+0x6C` equal to 1 | 4201 |
| GBD `+0x6C` equal to 0 | 1002 |
| files holding the string `RaptureAttribute` | 4201, exactly those with `+0x6C` equal to 1 |

In each of the 4201, let x be the dword at GBD `+0x78` and y the dword at GBD
`+x`. GBD `+y` begins a block tagged `LATB` whose dwords are the tag, the block
size, `0x101`, 1, `0x40`, `0x60` and `0x80`. At block `+0x40` four dwords give
the data offset `0x80`, the name offset `0x60`, the data size and 1. The name is
`RaptureAttribute` and the data size is twice the triangle count at GBD
`+0x64`. The dword that GBD `+0x74` points to equals the block size, and the
one GBD `+0x70` points to is 30.

Over the 3326695 u16 words, the low seven bits take only 0, 1, 2, 10, 14, 20,
24, 40, 42, 44, 50, 60, 64, 70, 74, 80, 84, 90, 100, 101 and 102. The most
frequent are 40 (1609063) and 0 (776323). Bits 7 to 11 take 0, 1, 10, 11, 13,
14, 20, 21 and 22. Bit 12 is set in 614855 words. Bits 13 to 15 are never set.

INFERRED. The low seven bits are the ground material of the footstep play, so
its tens digit less one indexes the A2 table and a word of 0 to 9 plays no
footstep.

## Surface record, ground name and weather id

STATIC. The array at ground object `+0x94` has a `0x30` byte stride and is
built by `0x007D49F0` (`ADD EAX,0x30` at `0x007D4AB2`). The probe
`0x007D4130` fills each record it is given: dword `+0x04` from the hit
(`0x007D41D0`), the low seven bits of the word at `+0x08` (`0x007D422B`),
bytes at `+0x0C` and `+0x0D` (`0x007D41B6`, `0x007D41C3`) and a name copied to
`+0x0E` (`LEA ECX,[EDI+0xE]` at `0x007D4183`). Bit 12 goes through the
output pointer argument. The caller `0x007D8840` forwards the record dword
at `+0x00` to the actor through `0x00662440` (`0x007D8A80`) and the two bytes
through `0x00662510` and `0x00662540` (`0x007D8A8F`, `0x007D8A9E`). Those
three setters store their value at actor `+0x2B88`, `+0x2B8C` and `+0x2B90`
when it changed and raise actor messages `0x5B`, `0x5D` and `0x5E` through
`0x007CE7B0` (`0x0066252C`, `0x00662554`). What the message handlers do with
the values is not traced.

The ground name read by A5 is written by `0x007D85D0`. It looks up the surface
entry at `[this+0x6D8]` through `0x007D3B70` (`0x007D86AA`, `0x007D86B3`),
reads a name through `0x00C1F430` (`0x007D8739`) and copies it into ground
`+0x64` with a sixteen byte bound (`0x007D8750..0x007D8753`). The same routine
copies the global at `0x012C5EB8` into ground `+0x74` with the same bound
(`0x007D8731`, `0x007D8744..0x007D8747`), and the use of that string is not
traced. INFERRED: the name is the DrawEnv name that the
[weather path](weather-sky-fog-environment-path.md) resolves from bits 7 to 11,
so which entries hold `_00` follows from the DrawEnv names.

The dword at `0x012C5EC0` is written at `0x007D8CE9` in `0x007D8AF0` from
`0x007E2480` on the weather manager returned by `0x007E02F0`, and at
`0x007D8375` in `0x007D8360` from a register value that is not traced.
`0x007E2480` divides the byte span `[manager+0x70, manager+0x74)` by 12 to get
an entry count. One entry answers the dword at entry `+0x04`. Two entries
answer that dword when the fraction returned by `0x007E2330` is below the f32
0.5 at `0x00F62F60` and the dword at entry `+0x10` otherwise. Any other count
answers -1. So the A5 test of `0x007D0900` compares the key of the current
weather resource entry, not a client-side weather name.

## Reproduction

```text
ghidra disasm 0x00664A60 -n 8
ghidra disasm 0x006681E0 -n 12
ghidra disasm 0x007D0AB0 -n 80
ghidra disasm 0x007D0D70 -n 100
ghidra disasm 0x007D09A0 -n 20
ghidra disasm 0x007D0900 -n 30
ghidra disasm 0x007D0BB0 -n 40
ghidra disasm 0x006B6FA0 -n 40
ghidra disasm 0x006B6F40 -n 10
ghidra disasm 0x007D1180 -n 20
ghidra disasm 0x007D4130 -n 120
ghidra disasm 0x007D8840 -n 120
ghidra disasm 0x007D85D0 -n 120
ghidra disasm 0x007E2480 -n 80
ghidra disasm 0x00662440 -n 14
ghidra x-ref to 0x012C5EC0
ghidra x-ref to 0x007D0D70
ghidra x-ref to 0x007D11E0
ghidra x-ref to 0x0065EFE0
ghidra x-ref to 0x00FED90C
```

Read the cells at `0x012C5DF0`, `0x012C5DF8`, `0x00FEA8D8`, `0x007D09D8`,
`0x006B7030` and `0x006B6F7C` and the floats at `0x00FEB194..0x00FEB1AC`. For
the census, walk `data/`, keep files whose first seven bytes are `SEDBPHB`,
read GBD `+0x6C`, follow GBD `+0x78` to the block, check the tag and the
record, and tally the words by mask.

## Open

- What handle slot `+0x40` does with A9, and the unit of the update argument.
- The writer of V`+0x37E` and the resource property behind it.
- The surface entry name field that `0x00C1F430` reads, and which names hold `_00`.
- The actor handlers of messages `0x5B`, `0x5D` and `0x5E` and the use of ground `+0x74`.
- The second probe `0x007D5D50` and the masks of both probes.
- The gate `0x0065B1A0` and the path for kinds 0 and `0x12`.
