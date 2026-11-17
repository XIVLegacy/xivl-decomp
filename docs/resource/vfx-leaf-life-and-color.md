# VFX LeafLife and ColorRGBALeafEx

Nearly every effect leaf and single in the effect banks carries a `LeafLife`
control and a `ColorRGBALeafEx` control. This page records the descriptors of
the two Rapture controls, the `LeafLife` construct, calc and value routines,
and the `ColorRGBALeafEx` construct and evaluator. The descriptor table and the
declaration counts are in the
[control module registry](vfx-control-module-registry.md). The leaf matrix
control is in [VFX LeafMatrixCib leaf scale](vfx-leaf-matrix-scale.md).

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32
image base `0x00400000`. Addresses are VAs. The instructions were read with
Capstone `5.0.7` over the file-backed sections and with the `ghidra` CLI.
Constants were read from the file-backed data. In the lists, P is the
parameter block of a control and N its node. STATIC means read from
instructions or data. INFERRED means concluded from a STATIC read without
tracing the use.

## Descriptors

STATIC. The Rapture descriptors name their controls through `+0x5C` and their
parent through `+0x64`:

| Descriptor | Name | Parent | Revision `+0x60` | Calc `+0x78` | Construct `+0x98` |
| --- | --- | --- | ---: | --- | --- |
| `0x0130F970` | `LeafLifeEx` | `LeafLife` | `0x3C9` | `0x00D589F0` | `0x00D58D10` |
| `0x0130FA58` | `ColorRGBALeafEx` | `ColorRGBALeaf` | `0x334` | `0x00D599D0` | `0x00D5AEB0` |

The names are under `Application/Scene/Vfx/RaptureQixControl/QixControl/Controls/`
and the parents under `SQEX/CDev/Engine/Vfx/QixControl/Controls/`. The slots
after the construct are:

- `0x0130F970`: `+0xA8` `0x00D58F10`, `+0xB4` `0x00D58900` and `+0xB8`
  `0x00D58E60`. `+0xB0` is empty
- `0x0130FA58`: `+0xA8` `0x00D5B3C0`, `+0xB0` `0x00D5B390`, `+0xB4`
  `0x00D59E90` and `+0xB8` `0x00D59A40`

## LeafLife construct

STATIC. `0x00D58D10(P, N)`:

- stores 1.0 (`0x00F54F70`) at `N+0x10` and 0 at the byte `N+0x1D`
- stores the dword `+0x44` of `[owner+0x10]` at `N+0x14`, where the owner is
  the result of `0x00E3A460(N)`, and `0x249F0` (150000) at `N+0x18`
- stores the byte `P+8` at `N+0x1C` and switches on it through the jump table
  `0x00D58E3C`, eight entries. A value above 7 logs an error and stores 0
- mode 0 stores 1 at `N+0x1C`. Modes 2, 4 and 5 store the dword `P+0` at
  `N+0x14`. Mode 3 stores `P+0` at `N+0x14` and `P+4` at `N+0x18`. Modes 1, 6
  and 7 store nothing more
- passes `P+0x0C` through `0x00D58B70` into `N+0x20` and `P+0x2C` through
  `0x00D58C40` into `N+0x40`. These two were not followed

## LeafLife calc and value

STATIC. The calc `0x00D589F0` reads the clock at `[owner+0xC4]`
(`0x00BB3BD0`). While bit 0 of `N+0x1D` is clear it stores the clock at
`N+0x14` every frame (`0x00D58A1F`). The states of `N+0x1C`:

- state 5 calls virtual `+0x40` of the owner with 1, sets the bit and goes to
  state 1
- state 6, with bit 0 of the owner flag `+0x28` set (`0x00BB35D0(1)`), sets the
  bit, stores the clock at `N+0x14` and goes to state 2
- state 7, with the owner flag set (`0x00BB35E0(1)`), clears it, sets the
  bit, stores the clock at `N+0x14` and goes to state 3

Every path stores the result of `0x00D58910` at `N+0x10`.

STATIC. `0x00D58910` takes the clock in EAX and switches on `N+0x1C` through
the jump table `0x00D589C4`:

| State | Value |
| ---: | --- |
| 0 | 0 |
| 1 | 1.0 while the clock is below the dword `+0x44` of `[owner+0x10]`, otherwise 1.0 when `0x00BCCB50` on `[owner+0x14]` answers true and 0 when it does not |
| 2 | 1.0 while the clock is below `N+0x14`, otherwise 0 |
| 3, 4 | `d = clock - N+0x14`. 1.0 for `d <= 0`, 0 for `N+0x18` equal to 0 or `d / N+0x18` of at least 1, otherwise `1 - d / N+0x18` |
| 5, 6, 7 | 1.0 |

INFERRED. With the clock stored at `N+0x14` until the latch, mode 2 answers 0,
modes 3, 4 and 5 answer 1.0, mode 6 cuts to 0 after the stop flag and mode 7
fades linearly over 150000 ticks once the stop flag is up. The dword `+0x44` of
`[owner+0x10]` and `0x00BCCB50` read as the span and the running state of the
effect, and the flag `+0x28` as the clip end. None was traced.

## ColorRGBALeafEx construct

STATIC. `0x00D5AEB0(P, N)`:

- stores `P+0x00..P+0x0F` at `N+0x20..N+0x2F` (the base RGBA), 1.0 in the four
  lanes `N+0x30..N+0x3F` (the tint) and 1.0 at `N+0x1C`, and clears the low two
  bits of the byte `N+0x42`
- builds three curve sets with the kind in byte 2 of each: `P+0x50..P+0x7F`
  through `0x00D59450` into `N+0x80`, `P+0x80..P+0x9F` through `0x00D59640`
  into `N+0xB0` and `P+0x20..P+0x4F` through `0x00D59710` into `N+0x50`
- copies the set at `N+0x80` to `N+0xD0`, `P+0xA0..P+0xFF` to
  `N+0x100..N+0x15F` and `P+0x14..P+0x1B` to `N+0x40..N+0x47`, and stores 0 at
  `N+0x48` and `N+0x14C`
- stores `P+0x18` times 300000.0 (`0x00F67020`) over 30.0 (`0x00FE5490`) at
  `N+0x44`, which turns frames of 30 a second into ticks of 300000 a second
  (`0x00D5B253..0x00D5B2C9`)
- calls `0x00D597F0` and, when byte `N+0x142` is not 0 and the floats `N+0x144`
  and `N+0x148` differ, creates an object through `0x00E3A230` and
  `0x00D87870` into `N+0x14C`. Those two calls were not followed

STATIC. `0x00D59450(set, out)` stores the kind byte at `out+2` and switches on
it through the jump table `0x00D5961C`:

- kind 0 zeroes `out+4..out+0x23`
- kind 1 copies the set. It stores `+0x04 - 0.5 * +0x08` at `+0x0C` and
  `+0x04 + 0.5 * +0x08` at `+0x10`, or 0 at both when `+0x04` is 0. It stores
  `+0x14 + 0.5 * +0x18` at `+0x1C` and `+0x14 - 0.5 * +0x18` at `+0x20`, or 0
  at both when `+0x14` is 0 (the 0.5 is the binary64 at `0x00F59898`)
- kinds 2, 4 and 5 store the defaults 1.0 at `+0x04`, 2.0 (`0x00F62F64`) at
  `+0x08` and `+0x10`, 50.0 (`0x00FB6F34`) at `+0x14`, 26.0 (`0x00FC342C`) at
  `+0x18`, 63.0 (`0x0111D124`) at `+0x1C`, 37.0 (`0x0111D120`) at `+0x20` and
  0 at `+0x0C`
- kind 3 and a value above 5 store nothing

## ColorRGBALeafEx calc and evaluator

STATIC. The calc `0x00D599D0`, with byte `N+0x41` set and `N+0x48` below
`N+0x44`, adds the frame step `[owner+0xD0]` to `N+0x48`. Slot `+0xA8`
`0x00D5B3C0` is a switch on a message number through the jump table
`0x00D5B8C8` of 15 entries, and `+0xB0` `0x00D5B390` calls virtual `+0x10` of the object at `N+0x14C`
when it exists and then the evaluator (`0x00D5B3AC`). Slots `+0xB4` `0x00D59E90` and `+0xB8` `0x00D59A40` were not read.

STATIC. The evaluator `0x00D5A400(N)` is called by the construct (`0x00D5B36F`), by
the message 0 arm of slot `+0xA8` (`0x00D5B402`, which first copies a quad
into `N+0x30`) and by slot `+0xB0`. It starts from `a = [[N+0x160]]` and applies:

- with byte `N+0x122` set, a cull test on the floats `N+0x12C` and `N+0x130`
  that sets or clears bit 1 of byte `N+0x134` and can return early
- with the kind of `N+0x80` set and `N+0x84` and `N+0x94` nonzero, a factor
  from `0x00D839E0`, a clamp to 0..1 of two ramps of a distance. The distance
  is read through `0x0080A270` and `0x00627050`, which were not followed
- with the kind of `N+0xB0` set, two angle ramps built through `0x009DAEBA`
  whose smaller value multiplies `a`
- with `N+0x14C` not 0, `a *= lerp(N+0x144, N+0x148, 0x00D877F0())`
- with byte `N+0x41` set and `N+0x48` below `N+0x44`, `a *= N+0x48 / N+0x44`

The result `a` is stored at `N+0x1C`. `N+0x10..N+0x1F` receives the lane
product of the tint `N+0x30` and the base `N+0x20` (`0x00D5AD88..0x00D5AEAD`).

INFERRED. The leaf colour is tint times base, and its alpha is scaled by `a`.
`[N+0x160]` reads as the `LeafLife` value of the same node. Neither was traced
to its source.

## Open

- `0x00627050` and its two positions, which the distance ramps need.
- The axes of the angle ramps, `0x00D877F0` and the object at `N+0x14C`.
- `[N+0x160]` as the `LeafLife` value, and the owner fields named above.
- The set builders `0x00D59710` and `0x00D59640`, and `0x00D81780`.

## Reproduction

```text
ghidra disasm 0x00D58D10 -n 140
ghidra disasm 0x00D589F0 -n 60
ghidra disasm 0x00D58910 -n 50
ghidra disasm 0x00D599D0 -n 12
ghidra disasm 0x00D5AEB0 -n 360
ghidra disasm 0x00D59450 -n 90
ghidra disasm 0x00D5A400 -n 400
```

Read the dwords of the descriptors `0x0130F970` and `0x0130FA58` at `+0x5C`,
`+0x60`, `+0x64`, `+0x78`, `+0x98` and `+0xA8..+0xB8`, the jump tables
`0x00D58E3C`, `0x00D589C4` and `0x00D5961C`, and the float and binary64
constants named above.
