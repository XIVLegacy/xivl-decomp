# Face default motions and the blink

The CharaActor initializer arms the default motion of each face part and a
blink object, and the actor update steps the blink once per update. This
page records the default motion names, the blink object fields, its step
and its restart. The face variant of a body motion request is in
[motion name lookup and native request](../resource/motion-resource-request.md).

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly
and cross references through the `ghidra` CLI supplied the instructions.
Strings and float constants were read from the file-backed sections. The
CRT identity of `0x009D576E` as `_rand` is Ghidra's library match. STATIC
means read from instructions or data. INFERRED means concluded from a
STATIC read without tracing the use.

## Initializer calls

STATIC. The CharaActor initializer `0x00664890` calls, each with ECX set by
`LEA ECX,[ESI+offset]`:

| Call | Callee | Object |
| --- | --- | --- |
| `0x00664C93` | `0x007A2F00` | actor `+0x438` |
| `0x00664CBF` | `0x007A3440` | actor `+0x1BC` |
| `0x00664CD5` | `0x007A3290` | actor `+0x390`, the blink object |

## Default motions

STATIC. `0x007A2F00` walks the four slots `+0x60..+0x6C` of its object with
the names `chxm_default` (`0x00FE6DDC`), `cfxm_default` (`0x00FE6DEC`),
`cmxm_default` (`0x00FE6DFC`) and `ctxm_default` (`0x00FE6E0C`)
(`0x007A2F08..0x007A2F83`). For an empty slot it resolves the name through
`0x00662760` on `[this]` and stores the result. For a resolved motion it
calls its virtuals `+0x7C(0)`, `+0x30(1)`, `+0x38(1)` and `+0x88(1.0)`
(`0x007A2F47..0x007A2F78`).

INFERRED. The four slots are the default motions of the four face parts,
started at frame 0, looping, at speed 1.0.

## Blink object

STATIC. `0x007A3290` resolves `cfxb_blink1` (`0x00FE6EF4`) through
`0x00662760` into `+0x04` and stores the float at `0x00FB7DD4` (-10.0) at
`+0x08`, 0 at `+0x0C`, 2 at `+0x10`, 1 at `+0x14` and byte 1 at `+0x18`
(`0x007A3290..0x007A32C5`). The reset `0x007A32D0`, whose caller is
`0x00664590` in `0x00664460`, writes the same values with a null motion at
`+0x04`.

STATIC. `0x007A3330(mode)` stores the mode at `+0x10` and 1 at `+0x0C`, then,
with a motion, calls its virtuals `+0x40(1.0)`, `+0x88(1.0)`, `+0x38(0)`,
`+0x0C` with a zero frame and `+0x30(1)` (`0x007A3330..0x007A3381`).

STATIC. `0x007A33B0(0)` copies `+0x14` to `+0x10` and stores 0.0 at `+0x08`
(`0x007A33B0..0x007A33C6`). Its nonzero arguments were not read.

## Blink step

STATIC. The step `0x007A9A00(dt)` has one caller, the actor update
`0x006679C0` at `0x006680B4`:

- with `+0x08` above 0 it subtracts `dt`. A result of 0 or less stores -10.0
  at `+0x08` and copies `+0x14` to `+0x10` (`0x007A9A0A..0x007A9A42`)
- with `+0x08` not equal to -1.0 (`0x00F62F80`), at most 0, and byte `+0x18`
  set, it clears the byte and calls `0x007A3330(+0x14)`
  (`0x007A9A45..0x007A9A6C`)
- with no motion at `+0x04` it returns. Otherwise it switches on `+0x10`
  (`0x007A9A71..0x007A9A90`)
- mode 1, not playing (`+0x0C` zero): two `_rand` calls, the product shifted
  right by 16 and taken modulo `0x5A`. A zero remainder stores 1 at `+0x0C`
  and calls the motion's virtuals `+0x40(1.0)`, `+0x88(1.0)`, `+0x38(0)`,
  `+0x0C` with a zero frame and `+0x30(1)` (`0x007A9B1E..0x007A9B95`)
- modes 1 and 2, playing: when the current frame reaches the end frame it
  stores 0 at `+0x0C` and calls `+0x30(2)` (`0x007A9A96..0x007A9B02`,
  `0x007A9C0B..0x007A9C18`)
- mode 0, playing: at the end frame it also stores 2 at `+0x10`
  (`0x007A9BA0..0x007A9C04`)

STATIC. A frame is the motion's tick value times 30.0 (binary64
`0x00FA7B18`) over 300000.0 (binary64 `0x00F67020`), truncated
(`0x007A9AAF..0x007A9AF6`). The current and end values come from the
motion's virtuals `+0x10` and `+0x14`.

INFERRED. After the initializer the first step restarts the blink once in
mode 1, after which a stopped blink starts again when
`(rand() * rand()) >> 16` is a multiple of 90, and a playing one stops at its
last frame. Virtual `+0x30` reads as the play state (1 play, 2 stop).

## Open

- The nonzero arguments of `0x007A33B0`, its callers, and the other writers
  of `+0x14`.
- The expression object at actor `+0x1BC` and its initializer `0x007A3440`.
- The rate at which the actor update runs, which sets how often the random
  test is drawn.

## Reproduction

```text
ghidra disasm 0x00664C6F -n 12
ghidra disasm 0x00664CB9 -n 2
ghidra disasm 0x00664CCF -n 2
ghidra disasm 0x007A2F00 -n 50
ghidra disasm 0x007A3290 -n 20
ghidra disasm 0x007A32D0 -n 16
ghidra disasm 0x007A3330 -n 30
ghidra disasm 0x007A33B0 -n 8
ghidra disasm 0x007A9A00 -n 180
ghidra x-ref to 0x007A9A00
ghidra x-ref to 0x007A32D0
```

Read the strings at `0x00FE6DDC`, `0x00FE6DEC`, `0x00FE6DFC`, `0x00FE6E0C`
and `0x00FE6EF4`, the floats at `0x00FB7DD4` and `0x00F62F80` and the
binary64 values at `0x00FA7B18` and `0x00F67020`.
