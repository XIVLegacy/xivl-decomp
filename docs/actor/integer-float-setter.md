# Integer-to-float setter candidate

This note records an integer-to-float setter candidate in the retail 1.23b
Windows `ffxivgame.exe`. The pinned executable has image base `0x00400000` and
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observation comes from Capstone 5.0.7 in 32-bit x86 mode, disassembling PE
bytes at the stated virtual address (VA). The relative virtual address (RVA)
is the VA minus the image base.

| RVA | VA | PE bytes and disassembly | Narrow interpretation and ambiguity |
| --- | --- | --- | --- |
| `0x3B0700` | `0x007B0700` | `F3 0F 2A 44 24 04` converts the stack integer at `[esp+4]` to a float; `F3 0F 11 81 CC 01 00 00` stores it at `[ecx+0x1CC]`; `E8 7D F6 FF FF` calls VA `0x007AFD90`; `C2 04 00` returns while removing four argument bytes. | The instructions support an object-method-shaped entry that converts one integer argument and stores a float before a helper call. They do not establish the receiver type, the field's semantic name or units, or that the value is an action movement-stop duration. |

Identifying the action caller, movement behavior, timebase, and runtime effect
requires evidence beyond these instructions.
