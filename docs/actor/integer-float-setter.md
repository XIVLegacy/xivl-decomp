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

## Conditional caller path

The tracked vtable catalog maps RVA `0x00C28CDC`, slot 1, to
`FUN_008283D0` (`config/ffxivgame.vtable_slots.jsonl:38386`). The method-name
catalog labels it
`Application::Scene::Cut::Clip::RaptureClientMoveStopClip::vfunc1`
(`config/ffxivgame.vtable_method_names.json:43634-43638`).

The same pinned executable shows a conditional call path from that slot:

- At VA `0x0082848E`-`0x008284A9`, the code converts the dword at
  `[ebp+0x10]` to floating point, multiplies it by the float at
  `[esp+0x14]`, converts the product to an integer with truncation toward
  zero, then passes that integer and `ESI` to VA `0x0065ED30` with `EBX` in
  `ECX`.
- At VA `0x0065ED30`-`0x0065ED5A`, the helper skips the call if `ESI` is
  null. Otherwise it calls through `ESI`'s vtable at byte offset `+0x268`;
  a nonzero result also skips the call. On the zero-result path, it passes
  `ESI+0xBF0` in `ECX` and forwards the integer to VA `0x007B0700`.

These instructions establish one conditional path from the catalog-labeled
vtable slot to the setter. They do not establish that `ESI` is a character
actor, that `ESI+0xBF0` is a movement controller, or that the float at
`[ecx+0x1CC]` is a duration or uses a particular time unit. The runtime
movement effect remains unknown.
