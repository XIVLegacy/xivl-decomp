# Float and mask instruction sites

This note records bounded instruction sites from the retail
1.23b Windows `ffxivgame.exe`. The pinned executable has image base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Capstone 5.0.7 in 32-bit x86 mode, disassembling PE
bytes at each virtual address (VA). The double constant is read from the PE
and interpreted as a little-endian IEEE-754 value. Each relative virtual
address (RVA) is the VA minus the image base.

## Float writes and mask arguments

| RVA | VA | PE bytes and disassembly | Narrow interpretation and ambiguity |
| --- | --- | --- | --- |
| `0x848000` | `0x00C48000` | `F3 0F 10 44 24 04 F3 0F 11 81 14 07 00 00 C2 04 00` - loads the stack float, stores it at `[ecx+0x714]`, and returns. | Writes one float to receiver offset `+0x714`; its semantic label is not established by this instruction. |
| `0x848020` | `0x00C48020` | `F3 0F 10 44 24 04 F3 0F 11 81 18 07 00 00 C2 04 00` - loads the stack float, stores it at `[ecx+0x718]`, and returns. | Writes one float to receiver offset `+0x718`; its semantic label is not established by this instruction. |
| `0x847DC0` | `0x00C47DC0` | `8B 44 24 04 F3 0F 10 44 24 08 F3 0F 11 84 81 34 06 00 00 C2 08 00` - reads an integer and float from the stack, then stores the float at `[ecx+eax*4+0x634]`. | An indexed float write into receiver-relative storage. The valid index range and meaning of the indexed values are not established here. |
| `0x847FC0` | `0x00C47FC0` | `F3 0F 10 44 24 04 F3 0F 11 81 0C 07 00 00 C2 04 00` - loads the stack float, stores it at `[ecx+0x70C]`, and returns. | Writes one float to receiver offset `+0x70C`; its semantic label is not established by this instruction. |
| `0x8272FD` | `0x00C272FD` | `6A 11 8D 48 30 E8 C9 17 01 00` - pushes literal `0x11`, sets `ecx` from `eax+0x30`, then calls VA `0x00C38AD0`. | The literal argument is `0x11`. Its bit definitions and the called routine's selection behavior are not established here. |
| `0x8278D7` | `0x00C278D7` | `6A 11 8B CB E8 A0 EC FF FF` - pushes literal `0x11`, sets `ecx` from `ebx`, then calls VA `0x00C26580`. | The literal argument is `0x11`. Its bit definitions and the called routine's selection behavior are not established here. |
| `0x88BA57` | `0x00C8BA57` | `83 7D 0C 01` compares a stack argument with `1`. After the two qword copies described below, `75 28` at RVA `0x88BA76` (VA `0x00C8BA76`) branches when it differs. The equal path at RVA `0x88BA78` (VA `0x00C8BA78`) uses `F3 0F 10 8C 24 DC 00 00 00 F3 0F 10 83 44 01 00 00 0F 5A C0 0F 5A C9 F2 0F 59 C1 66 0F 5A C0 F3 0F 11 84 24 DC 00 00 00` to load the local float at `+0xDC`, multiply it by `[ebx+0x144]`, and store the product back at `+0xDC`. | In the argument-equals-1 branch, one float at offset `+0xC` in a copied 16-byte local block is multiplied by another object-relative float at `[ebx+0x144]`. The block's component meanings and the identity of the object at `ebx` are not established here. |

At RVA `0x88BA5B` (VA `0x00C8BA5B`), bytes `F3 0F 7E 07` and
`66 0F D6 84 24 D0 00 00 00` copy one qword from an input pointer into the
local block. At RVA `0x88BA68` (VA `0x00C8BA68`), bytes `F3 0F 7E 47 08` and
`66 0F D6 84 24 D8 00 00 00` copy the second qword. After the conditional
multiply, bytes `E8 3C 52 79 FF` at RVA `0x88BAAF` (VA `0x00C8BAAF`) call VA
`0x00420CF0` with the block. This establishes a conditional component
transform in the function, not the component's color channel, shader meaning,
or visible effect.

## Other parameter candidates

| RVA | VA | PE bytes and disassembly | Narrow interpretation and ambiguity |
| --- | --- | --- | --- |
| `0x20B58C` | `0x0060B58C` | `C6 44 24 04 01 F3 0F 7E 44 24 04 66 0F D6 00` - writes byte `1` to a stack local, loads eight bytes beginning there, and stores those bytes to `[eax]`. The preceding instruction at RVA `0x20B581` (VA `0x0060B581`), `05 3C 01 00 00`, adds `0x13C` to `eax`. | The low byte of a local eight-byte value is set to `1` before the value is copied to the adjusted address. The structure, field semantics, producer role, and effect are unresolved. |
| `0x6580F0` | `0x00A580F0` | `8B 41 08 8B 48 08 F3 0F 10 44 24 04 F3 0F 11 41 50 C2 04 00` - follows pointers through `[ecx+8]` and `[eax+8]`, then stores the input float at `[ecx+0x50]`. | A two-step pointer-based float setter to offset `+0x50`. The field is not independently identified as ambient-occlusion strength. |
| `0x1CD00` | `0x0041CD00` | `0F BE 44 24 14 F2 0F 2A C0 F2 0F 59 05 A8 98 F5 00` - sign-extends a byte argument, converts it to double, and multiplies it by the double at absolute VA `0x00F598A8` (RVA `0xB598A8`). That constant's PE bytes are `00 00 00 00 00 00 C0 3F`, which encode `0.125`. | The signed-byte argument is scaled by `0.125` and converted through the observed floating-point path. This establishes a scaled floating-point intermediate; downstream API and sampler-state meaning are not identified here. |

## Gameplay-depth preset tail

The complete 57-byte block at RVA `0x20B58C` (VA `0x0060B58C`) is:

```text
C6 44 24 04 01 F3 0F 7E 44 24 04 66 0F D6 00
F3 0F 7E 44 24 0C 66 0F D6 40 08
F3 0F 7E 44 24 14 66 0F D6 40 10
F3 0F 7E 44 24 1C 66 0F D6 40 18
83 49 2C 04 5E 83 C4 20 C3
```

It sets the low byte of the first eight-byte stack value to `1`, copies four
stack qwords at offsets `+0x04`, `+0x0C`, `+0x14`, and `+0x1C` to the
destinations at `[eax]`, `[eax+8]`, `[eax+0x10]`, and `[eax+0x18]`, then ORs
`4` into `[ecx+0x2C]` and returns. The copied structure, flag meaning,
producer role, and effect remain unresolved.

## Evidence boundary

These sites establish specific field writes, literal call arguments, a
conditional float transform, and a signed-byte scaling operation. Their
instructions alone do not establish setting names, activation conditions,
selected objects, visual quality, performance, or other runtime outcomes.
Such semantic and custom-effect claims require separate evidence.
