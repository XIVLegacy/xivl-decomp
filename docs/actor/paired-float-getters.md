# Paired float getters

This note records two range-returning sites in the retail 1.23b Windows
`ffxivgame.exe`. The pinned executable has image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Capstone 5.0.7 in 32-bit x86 mode, disassembling the PE
bytes at each virtual address (VA). The relative virtual address (RVA) is the
VA minus the image base.

| RVA | VA | PE bytes and disassembly | Narrow interpretation and ambiguity |
| --- | --- | --- | --- |
| `0x2576D0` | `0x006576D0` | `D9 81 F4 00 00 00 C3` - `fld dword ptr [ecx+0xF4]; ret` | Returns the float stored at the receiver's `+0xF4`. The field's semantic name is not established here. |
| `0x4002C0` | `0x008002C0` | Reads `[ecx+0xF8]` and `[ecx+0xFC]` with `F3 0F 10 81 F8 00 00 00` and `F3 0F 10 89 FC 00 00 00`, compares them with `66 0F 2F DA`, then branches with `76 0A`. The paths store `xmm0` or `xmm1` to the stack using `F3 0F 11 04 24` or `F3 0F 11 0C 24`, then return the stack float with `D9 04 24 59 C3`. | For ordered finite values, returns the smaller field value. The comparison and branch do not establish a general-purpose minimum for unordered values, nor do they identify either field's meaning. |

At RVA `0x224890` (VA `0x00624890`), the boundary helper first tests
`[ecx+0x1BC]`. A nonzero byte returns the float at `[ecx+0x1C8]`.
Otherwise, it reads the first stack argument at `[esp+4]`; if that pointer is
null, it checks `[ecx+0x168]`. If both are null, it returns the global float
at VA `0x00FB6E84`. A non-null pointer is advanced by four bytes, and the
helper calls vtable slot `+0x188` on that receiver at `0x006248BF`. The
field roles, receiver types, and global fallback meaning are not established
by these instructions.

The two getter paths meet in a bounds check at `0x007FEAC0`. Its branch at
`0x007FEB68` either calls `0x00624890` (which dispatches through receiver
vtable slot `+0x188` at `0x006248BF`) or directly calls `0x008002C0` at
`0x007FEB76`. Four vtables in `config/ffxivgame.vtable_slots.jsonl`
(rows 20314, 42162, 42320, and 42478)
place `0x006576D0` in slot `+0x188`; the receiver at this call is not
individually typed by that catalog.
Both branches add the float at `[ebx+0x1D0]` to the returned value at
`0x007FEB7F`-`0x007FEB9B`. The result reaches `0x00628250` through the call
at `0x007FEBE1`-`0x007FEBEF`; that callee compares squared point-to-box
distance with the squared supplied radius. `0x007FF140` calls this bounds
path with both branch modes at `0x007FF167` and `0x007FF1A2`.

The virtual getter path is also called from `0x007FB470` at `0x007FB525`
and `0x007FB55E`, from `0x00800F60` at `0x0080100D`, from `0x00801050`
at `0x0080108D`, and from `0x008010D0` at `0x00801101`. These calls
establish uses of the indirect getter path in range and bounds code. They do
not establish a model or LOD selection rule, which layouts use either value,
or a resulting visual change. The sites do not replace the separate
background visibility and pixel-clipping evidence.
