# VFX leaf colour distance ramps, cull bounds and lerp object

A ColorRGBALeafEx node `N` scales its alpha by a distance ramp, can skip
drawing past a cull distance and can scale by a lerp object. This page
records the set `P+0x50 -> N+0x80`, the ramp value routine, the cull test,
the sibling set builders and the lerp object. The construct, the evaluator
and the kind jump table of `0x00D59450` are in
[VFX LeafLife and ColorRGBALeafEx](vfx-leaf-life-and-color.md).

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly
and memory reads through the `ghidra` CLI supplied the instructions and
constants. STATIC means read from instructions or data. INFERRED means
concluded from a STATIC read without tracing the use.

## Distance set values

STATIC. The set at `N+0x80` has its kind byte at `+0x02`. The routine
`0x00D81780(set)` is called from the construct `0x00D5AEB0` (`0x00D5B01F`) and
from `0x00D59350` (`0x00D5935C`). It stores `+0x10 = 1.0 / (+0x10 - +0x0C)`
and `+0x1C = -1.0 / (+0x1C - +0x20)`, and 0 for a zero difference. The
binary64 constants are `1.0` at `0x00F598A0` and `-1.0` at `0x00F599D0`
(`0x00D81780..0x00D81818`).

STATIC. For the kinds 2, 4 and 5 of `0x00D59450` the builder defaults
(`1.0`, `2.0`, `50.0`, `26.0`, `63.0`, `37.0`) are in the LeafLife page. After
`0x00D81780` they hold `+0x0C` 0, `+0x10` 0.5, `+0x14` 50.0, `+0x18` 26.0,
`+0x1C` -1/26 and `+0x20` 37.0.

In node terms the four ramp values are `N+0x8C` (a, the set `+0x0C`),
`N+0x90` (b, `+0x10`), `N+0xA0` (c, `+0x20`) and `N+0x9C` (e, `+0x1C`).

## Ramp value routine

STATIC. `0x00D839E0(N+0x10, vec4, point)` returns an x87 float:

- It returns 1.0 when b and a are both zero, and when c and e are both zero
  (`0x00D839F3..0x00D83A34`). Any other combination goes on to the ramp.
- `L` is the length of the four lanes of `vec4` (`0x00D83A69..0x00D83A8C`).
- `d` is the float `0x0080A270(point)` returns. The byte at `N+0x42` selects
  it: bit 0 adds `d` to the near term and bit 1 adds `d` to the far term.
  A clear bit uses 0.0 (`0x00D83AAB..0x00D83ACE`).
- `near = (L + d0 - a) * b` and `far = (L + d1 - c) * e + 1.0`, in binary64
  with the constant at `0x00F598A0` (`0x00D83AD0..0x00D83B2B`).
- Two substitution tests follow: when b and a are both zero `near` takes the
  `far` value, and when c and e are both zero `far` takes the `near` value
  (`0x00D83B33..0x00D83B60`). The early return above already answered for
  both conditions, so the tests are not reached with those values
  (INFERRED).
- The answer is the smaller of the two, clamped to 0 through 1.0 (the
  constant `0x00F54F70`) (`0x00D83B63..0x00D83BB4`).

STATIC. The evaluator `0x00D5A400` calls the routine only while the alpha so
far is nonzero, when the kind byte `N+0x82` is nonzero and the floats
`N+0x94` and `N+0x84` are both nonzero (`0x00D5A714..0x00D5A768`). The result
multiplies the alpha.

STATIC. `0x0080A270(point)` reads the global `0x0133DEF4` through
`0x00623C60`, answers 0.0 when it is null, and otherwise calls `0x00627050`.
That routine answers 0.0 when the pointer at `[scene+0x164]` or the object
at `[[scene+0x118]+0x64]` is null or the two are equal. Otherwise it forms
`x = |point - A| - |point - B|` over three lanes, where A is the position
`0x006246C0` returns for the object at `[[scene+0x118]+0x64]` and B is the
position virtual `+0x88` of `[scene+0x164]` returns, and answers `-x` when
`x` is below 0.0 and `x` otherwise (`0x00627050..0x0062717B`). The global is
the MapLayoutActor scene root of
[layout block streaming](layout-block-streaming.md).

INFERRED. `L` is the distance of the node from the camera: the evaluator
builds a world point from node data and transforms it through `0x0042EC50`
(`0x00D5A603`). The matrix source was not traced.

## Cull bounds

STATIC. Node `+0x122` is the byte at `P+0xC2`, `+0x12C` is `P+0xCC` (near)
and `+0x130` is `P+0xD0` (far), copied from `P+0xA0..P+0xFF` to
`N+0x100..N+0x15F`. In the evaluator at `0x00D5A608`, with `+0x122` set:

- bit 1 of the byte `+0x134` is cleared.
- bit 1 is set when `+0x130` is nonzero and `L` is at or above `+0x130`.
- when bit 0 of `+0x134` is clear, bit 1 is set when `+0x12C` is nonzero and
  `+0x12C` is at or above `L`.
- with bit 1 set the evaluator jumps to the common exit `0x00D5AE99` with the
  colour left at the zero stored first, so the node draws nothing
  (`0x00D5A608..0x00D5A70F`).

`L` is the length of the transformed world point above.

## Angle and `P+0x20` sets

STATIC. The angle set builder `0x00D59640` stores the kind at `+0x02` and
switches on `kind - 1` through the table `0x00D596F0`:

| Kind | Effect |
| --- | --- |
| 1 | copies the set |
| 2 | stores 0 at `+0x02` |
| 3 | stores nothing |
| 4, 5 | `+0x04` and `+0x08` are 90.0 (`0x0111D128`), `+0x0C` and `+0x10` are 0 |

The evaluator runs the angle ramps for a nonzero byte at `N+0xB2`
(`0x00D5A721`, `0x00D5A77B`).

STATIC. The builder `0x00D59710` for `P+0x20 -> N+0x50` switches on the kind
through the table `0x00D597D4`. Kind 2 copies `0x30` bytes. Kinds 0, 1 and 3
store the defaults `+0x10` 0.0, `+0x14` 100.0 (`0x01042F88`), `+0x18`,
`+0x20`, `+0x24` and `+0x28` 1.0 and `+0x2C` 0.0.

## Lerp object

STATIC. The object at `N+0x14C` has the vtable `0x01125950`. Slot `+0x04` is
`0x00D87720`, which clears the byte at `+0x04`. Slot `+0x0C` is `0x00D87740`.
It reads the begin and end values from virtuals `+0x68` and `+0x6C` of the
owner at `+0xE8`, stores them through `0x00633090` and `0x00633050` into the
timer objects at `+0x84` and `+0x14`, and sets the byte at `+0x04`
(`0x00D87740..0x00D877D4`). `0x00D877F0` answers 0.0 while the byte is 0.
Otherwise it divides the integer from `0x006330A0` on `+0x08` by the one on
`+0x78` as binary64 and caps the result at 1.0 (`0x00D877F0..0x00D87859`).

STATIC. The evaluator calls slot `+0x0C` at `0x00D5ACD4` whenever `N+0x14C`
is set. For an alpha above the binary64 at `0x00F62F70` it multiplies the
alpha by `N+0x144 + (N+0x148 - N+0x144) * 0x00D877F0()`
(`0x00D5ACC3..0x00D5AD44`).

## Open

- The writers of the `N+0x42` bits and what supplies the transform matrix of
  `L`.
- The clock and span of the two timer objects.
- Which effect resources set a nonzero kind in the angle, `P+0x20` and cull
  fields. That needs an install census.

## Reproduction

```text
ghidra disasm 0x00D81780 -n 50
ghidra disasm 0x00D839E0 -n 200
ghidra disasm 0x00D5A440 -n 400
ghidra disasm 0x00D59640 -n 40
ghidra disasm 0x00D59710 -n 50
ghidra disasm 0x00D596BF -n 8
ghidra disasm 0x00D5978F -n 12
ghidra disasm 0x0080A270 -n 6
ghidra disasm 0x00627050 -n 70
ghidra disasm 0x00D87740 -n 40
ghidra disasm 0x00D877F0 -n 30
ghidra memory read 0x00D5961C 24
ghidra memory read 0x00D596F0 20
ghidra memory read 0x00D597D4 16
ghidra memory read 0x01125950 24
```

Read the binary64 values at `0x00F598A0` and `0x00F599D0` and the floats at
`0x00F54F70`, `0x0111D128` and `0x01042F88`.
