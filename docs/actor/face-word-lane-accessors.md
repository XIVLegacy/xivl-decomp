# Face word lane accessors

The appearance record carries a face word that the CharaActor reads and
writes by lane id. This page records the lane table the reader switches on
and the actor words the writer updates.

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly
and memory reads through the `ghidra` CLI supplied the instructions and the
jump table. STATIC means read from instructions or data.

## Reader

STATIC. `0x0065EA20(actor, id)` returns -1 for a null actor or when virtual
`+0x268` of the actor answers nonzero. Otherwise it calls
`0x006B8170(id)` on the object at actor `+0x2B5C` (`0x0065EA20..0x0065EA51`).

STATIC. `0x006B8170(id)` resolves the id through `0x006B72B0` and the
word through `0x006B6E50`. A failed resolve returns a local it stored first. Otherwise it jumps through the table `0x006B828C` at index `id - 8`
(`0x006B81BF..0x006B81CB`, 16 entries) and returns one lane of the word:

| Id | Lane | Mask |
| ---: | --- | --- |
| 8 and 21 | bits 19:10 | `0x3FF` |
| 9 | bits 4:0 | `0x1F` |
| 10 | bits 9:5 | `0x1F` |
| 11 | bits 13:8 | `0x3F` |
| 12 | bits 4:0 | `0x1F` |
| 13 | bits 7:5 | `0x7` |
| 14 | bits 29:27 | `0x7` |
| 15 | bit 26 | `0x1` |
| 16 | bits 25:23 | `0x7` |
| 17 | bits 22:20 | `0x7` |
| 18 | bits 19:18 | `0x3` |
| 19 | bits 17:16 | `0x3` |
| 20 | bits 15:14 | `0x3` |
| 22 | bits 9:0 | `0x3FF` |
| 23 | bits 29:20 | `0x3FF` |

An id below 8 or above 23 returns the unmasked word through `0x006B8283`.

## Writer

STATIC. `0x00662100(actor, id, value)` resolves the id through `0x006B72B0`
and switches on the result, which is at most 3, through the table
`0x006623A0`. The first arm reads the current word from
`actor + index * 4 + 0x13C8` and, when `value` is negative, the replacement
from `actor + index * 4 + 0x1354`. It switches on `id - 8` through the table
`0x006623B0`, for example `0x00662166` merges the low 10 bits, and stores the
merged word back at `actor + index * 4 + 0x13C8`. It then calls
`0x0065D730` on the actor (`0x00662100..0x0066217F`).

## Open

- Which model, texture or skeleton path reads each lane. The path decoder
  reading only bits 13:8, 7:5 and 4:0 and a sweep for other shifts of the
  word were not reproduced as a tracked result.
- The meaning of ids 8, 9, 10, 21, 22 and 23.

## Reproduction

```text
ghidra disasm 0x0065EA20 -n 14
ghidra disasm 0x006B8170 -n 70
ghidra disasm 0x006B81D2 -n 60
ghidra disasm 0x00662100 -n 40
ghidra memory read 0x006B828C 64
ghidra memory read 0x006623A0 16
ghidra memory read 0x006623B0 64
```

Read the 16 dwords at `0x006B828C` as code addresses and decode the
shift and mask at each target.
