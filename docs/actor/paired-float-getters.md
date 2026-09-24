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

These instructions establish only the values returned from the receiver. This
trace does not show a model or LOD selection rule, which layouts use either
value, or a resulting visual change. The sites do not replace the separate
background visibility and pixel-clipping evidence.
