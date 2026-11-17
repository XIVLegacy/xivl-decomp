# Face expression stack, blink hold and lip object

The CharaActor expression object at actor `+0x1BC` queues a named face
expression, pushes it on an eight entry stack and holds the blink while it
plays. A second object plays one of six lip motions. This page records the
request, the push, the blink hold routines and the lip object. The default
motions and the blink step are in
[face default motions and the blink](face-default-and-blink.md).

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly,
cross references and memory reads through the `ghidra` CLI supplied the
instructions and data. STATIC means read from instructions or data. INFERRED
means concluded from a STATIC read without tracing the use.

## Name literals

STATIC. `0x00FE6F00` holds `cfxf_default` and `0x00FE711C` holds `off`. The
pointer list at `0x012C4C50` holds 21 names, zero terminated: `cfxf_laugh`,
`cfxf_laugh_st`, `cfxf_grin`, `cfxf_cry`, `cfxf_sigh`, `cfxf_spewing`,
`cfxf_frustrated`, `cfxf_sulk`, `cfxf_bow`, `cfxf_think`, `cfxf_distress`,
`cfxf_ouch`, `cfxf_ouch_st`, `cfxf_sleep`, `cfxf_die`, `cfxf_pant`,
`cfxf_f_distrust`, `cfxf_f_scornful`, `cfxf_f_puzzled`, `cfxf_f_hate` and
`cfxf_f_relaxed`. The list at `0x012C4CA8` holds 18 names, zero terminated:
`cfxf_praise`, `cfxf_grin`, `cfxf_laugh`, `cfxf_laugh_st`, `cfxf_angry_st`,
`cfxf_angry`, `cfxf_wrysmile`, `cfxf_shock`, `cfxf_surprised`, `cfxf_flash`,
`cfxf_fear`, `cfxf_ouch_st`, `cfxf_hu`, `cfxf_pant`, `cfxf_cheer`, `cfxf_ho`,
`cfxf_f_upset` and `cfxf_f_relaxed`.

## Expression request

STATIC. `0x007A6210(name, u16 flags, int fade, int hold)` runs on the
expression object (`this` is actor `+0x1BC`, `[this]` is the actor). Its
callers by cross reference are `0x00663109`, `0x0065E900` in `0x0065E8D0`,
`0x0065E943` in `0x0065E910` and `0x007A9D59` in `0x007A9CE0`.

- The name `off` (a 4 byte compare against `0x00FE711C` at `0x007A6228`)
  copies `cfxf_default` (0x10 bytes) to `+0x190`, stores the flags in the
  word at `+0x17C` with bit 16 set, stores `(float)fade` at `+0x174` and at
  `+0x178`, and returns (`0x007A622C..0x007A6272`).
- Any other name is compared with `cfxf_default`. A name that is not
  `cfxf_default` must resolve through `0x00662720(0x6D6362, name)` and
  `0x00662720(0x6D7462, name)` on `[this]` (`0x007A62BF..0x007A62E5`), and the
  request returns when either answer is zero.
- With `hold` above 0 the request walks the stack entries from index 7 down
  to the first used entry and returns when bit 0 of that entry's flag byte is
  set (`0x007A62EF..0x007A6316`). The stack layout is in the push below.
- Otherwise it copies the name to `+0x190`, stores the flags in `+0x17C` with
  bit 16 set, `(float)fade` at `+0x174` and `(float)hold` at `+0x178`, and for
  a name other than `cfxf_default` stores the `mtb` handle at `+0x170` and the
  `mcb` handle at `+0x16C` (`0x007A6318..0x007A6385`).

STATIC. `0x007A9CE0(name)` has no direct caller. With a null name it copies
`cfxf_default` to `+0x180` and calls the push below with `a2` 0, `a3` the float
at `0x00FA95A0` and `a4` -1.0. With a name it copies the name to `+0x180` and
calls `0x007A6210(name, 0, 10, -1)` (`0x007A9CE3..0x007A9D59`).

INFERRED. The request stages one pending expression at `+0x16C..+0x190`.
The routine that consumes the pending slot was not traced.

## Push

STATIC. `0x007A63A0(name, int a2, float a3, float a4, byte lock, key*)` has
callers `0x007BA53E`, `0x007AC3B6`, `0x007A9CC6`, `0x007A9D31` and
`0x007B6A2B`. `key` points at two dwords.

- It resolves the motion. For `cfxf_default` the motion is the dword at
  `0x00FC0188`, otherwise it is `0x0065DB60(first dword of key)` on `[this]`.
  A null motion ends the push (`0x007A63A9..0x007A63F3`).
- It then calls `0x0065C2F0(0)` and `0x0065C300(0)` on `[this]`. They add
  `0x390` and `0x35C` to the actor and jump to the blink hold `0x007A33B0` and
  to `0x007A3900` (`0x0065C2F0`, `0x0065C300`).
- A name found in the list `0x012C4C50` calls `0x0065C2F0((int)a4)`. A name
  found in the list `0x012C4CA8` calls `0x0065C300((int)a4)`
  (`0x007A6459`, `0x007A64AD`).
- The stack is eight entries of `0x28` bytes. Entry `i` starts at
  `this + 4 + 0x28 * i`, and an entry is in use when its dword at `+0x08` (the
  motion) is nonzero (`0x007A64BB..0x007A64D1`). With all eight in use every
  entry moves down one slot through `0x0065DBA0` and a block of copies, and
  the new entry takes index 7 (`0x007A64DE..0x007A6715`). Otherwise it takes
  the first unused index.
- The new entry stores: the two key dwords at `+0x00` and `+0x04`, the motion
  at `+0x08`, `(a2 & 0xF) * 2 | (lock & 1)` merged into the low bits of the
  dword at `+0x0C`, weight at `+0x10` (0 when `a3` is above 0, otherwise 1.0
  from `0x00F54F70`), 0.0 at `+0x14` and `+0x20`, `a3` at `+0x18` and `+0x1C`
  and `a4` at `+0x24` (`0x007A671A..0x007A678C`).
- For a motion other than the default object it calls the motion's virtuals
  `+0x38(0)`, `+0x40(1.0)`, `+0x88(weight)` and `+0x30(1)`
  (`0x007A678F..0x007A67D2`).

STATIC. `0x007A9C30(dt)` has one caller, `0x007AC3DA` in `0x007AC300`. It adds
`dt` to the float at entry `+0x20` of each used entry. When the float at
`+0x24` is above 0, the timer has passed it, and the next entry's motion
dword is zero, it pushes `cfxf_default` with `a2 = (entry flags >> 1) & 0xF`,
`a3` the entry's float at `+0x1C` and `a4` -1.0 (`0x00F62F80`)
(`0x007A9C30..0x007A9CCE`).

## Blink hold

STATIC. `0x007A33B0(n)` on the blink object (fields as in the
[blink object](face-default-and-blink.md#blink-object)). The constant
`0x00FA4460` is 0.0 and `0x00F62F80` is -1.0.

| `n` | Effect |
| --- | --- |
| 0 | `+0x10 = +0x14`, `+0x08 = 0.0` |
| -1 | when `+0x10` is 1, `+0x14 = 1`. `+0x08 = -1.0`, `+0x10 = 2` |
| above 0 | when `+0x08` is below `n`, `+0x08 = n` and, when `+0x10` is 1, `+0x14 = 1`. `+0x10 = 2` |

STATIC. `0x007A3300(n)` first compares `+0x08` with 0.0. At or below 0.0 it
sets byte `+0x18` and stores `n` at `+0x14`. Above 0.0 it stores `n` at
`+0x14` only for `n` 1 or 2 (`0x007A3300..0x007A3329`).

STATIC. `0x007A3900(n)` has the same three cases on the second object with
its fields shifted: timer `+0x1C`, mode `+0x20`, resume `+0x28`, and a value
at `+0x24` copied to `+0x2C`. Mode 3 replaces mode 2 and 2 replaces 1. For
`n` -1 it stores 2 at `+0x28` when `+0x20` is 2, `-1.0` at `+0x1C` and 3 at
`+0x20`. For `n` above 0 and the timer below `n` it stores `n` at `+0x1C`,
and when `+0x20` is 2 it stores 2 at `+0x28` and `[+0x24]` at `+0x2C`
(`0x007A3900..0x007A3977`).

INFERRED. The second object is a mouth blink or hold. Its consumer was not
traced.

## Lip object

STATIC. `0x007A6FC0(mode, set)` has one caller, `0x0065E9A9` in
`0x0065E980`. `0x0065E980` has one caller, `0x00826B14` in `0x00826AB0`, which
is referenced from the data word at `0x010229D8`. The table at `0x00FE7200`
is six names 0x10 apart: `cfxl_lip_nor1`, `cfxl_lip_nor2`, `cfxl_lip_wis1`,
`cfxl_lip_wis2`, `cfxl_lip_sho1` and `cfxl_lip_sho2`. The row is
`(mode == 1) + 2 * set`.

- With the float at `+0x1C` at or below 0.0 it stores `mode` at `+0x28` and
  `set` at `+0x2C`, sets byte `+0x30` and stores the `mcb` handle of the row
  at `+0x10` and the `mtb` handle at `+0x14` (`0x007A6FF7..0x007A707B`).
- Above 0.0 it stores `mode` and `set` at `+0x28` and `+0x2C` only for mode
  2 or 3 and plays nothing (`0x007A6FD4..0x007A6FF4`).

## Open

- The consumer of the pending expression slot and the per entry blend of
  the motion virtuals `+0x40` and `+0x88`.
- What stops a lip motion and what reads the second blink object.
- The unit of `fade` and `hold`.

## Reproduction

```text
ghidra disasm 0x007A6210 -n 120
ghidra disasm 0x007A63A0 -n 400
ghidra disasm 0x007A9C30 -n 50
ghidra disasm 0x007A9CE0 -n 40
ghidra disasm 0x007A33B0 -n 42
ghidra disasm 0x007A3300 -n 12
ghidra disasm 0x007A3900 -n 40
ghidra disasm 0x0065C2F0 -n 2
ghidra disasm 0x0065C300 -n 2
ghidra disasm 0x007A6FC0 -n 60
ghidra x-ref to 0x007A6210
ghidra x-ref to 0x007A63A0
ghidra x-ref to 0x007A6FC0
ghidra x-ref to 0x0065E980
ghidra memory read 0x00FE6F00 32
ghidra memory read 0x00FE711C 4
ghidra memory read 0x012C4C50 88
ghidra memory read 0x012C4CA8 80
ghidra memory read 0x00FE7200 96
```

Read each name pointer of the two lists as a NUL terminated string. Read
the floats at `0x00F54F70`, `0x00F62F80` and `0x00FA4460`.
