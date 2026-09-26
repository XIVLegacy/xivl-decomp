# Four-case payload consumer

This page records the four direct branches in a native payload consumer. The
numeric values are local selector values in this function; the instructions
alone do not establish wire opcodes, UI meaning, or a retainer workflow.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for an RVA. The relevant branches were read from the tracked
`asm/ffxivgame/000d8d10_FUN_004d8d10.s` disassembly and checked against the
pinned PE bytes.

## Direct branches at `0x004D8D10`

The function reads a word at input `+2`, subtracts `0x00C8`, and accepts
indexes 0 through 3 through the four-entry table at `0x004D8F58`
(`0x004D8D51`-`0x004D8D65`). Other values reach the return path at
`0x004D8F2C`.

| Input word | Branch VA | Direct observations |
| --- | --- | --- |
| `0x00C8` | `0x004D8D6C` | Passes input `+0x10` and `+0x30` separately to `0x00447260`, each with the length read from `0x00F67298`. Calls `0x004D8560` with literal argument `3` and the two constructed locals. If that call succeeds and byte `0x0132F818` is nonzero, calls `0x004D6B40` (`0x004D8DB4`-`0x004D8DCD`). |
| `0x00C9` | `0x004D8E7B` | Passes the dword at input `+0x10` to `0x005754E0` with receiver at owner `+0x510`. A zero word result exits. Otherwise copies input `+0x14` with length `0x20` and input `+0x34` with length `0x200`, then calls `0x004D8560` with that result and the two locals (`0x004D8E7B`-`0x004D8ED7`). |
| `0x00CA` | `0x004D8EE9` | Passes input `+0x10` to `0x00447260` with the length read from `0x00F67298`, then calls `0x004C80B0` with receiver at owner `+0x950` (`0x004D8EE9`-`0x004D8F13`). |
| `0x00CB` | `0x004D8DF7` | Passes input `+0x14` and `+0x34` separately to `0x00447260`, each with the length read from `0x00F67298`. The word at input `+0x10` becomes the argument to `0x004D8560` when nonzero; zero is replaced by `3`. The same success-and-byte guard as `0x00C8` controls an optional `0x004D6B40` call (`0x004D8DF7`-`0x004D8E69`). |

The `0x00CB` branch does not clamp nonzero values to at least 3: values 1 and
2 pass through unchanged. The called helpers and copied local objects need
separate evidence before naming these branches or treating their input layout
as a retail application protocol.

## Helper boundaries

These helper observations were disassembled from the pinned PE above with
Capstone 5.0.7.

At VA `0x004D8560` (RVA `0x000D8560`), the helper calls `0x004B4ED0` with
ECX=`this+0x17430` and pushed `1`. It then calls `0x00C99E80` with
ECX=`this+0x7F8`, pushing the prior call's EAX, `1`, and a stack dword; zero
AL returns false. Next it calls `0x00445C70` with a stack dword in ECX; zero
AL also returns false. It calls `0x004C3DD0` with ECX=`this+0x950` and that
same dword pushed; nonzero AL returns false. After those gates, a null
`this+0x660` skips the last call. If nonnull, `0x004CE760` is called with that
pointer in ECX after pushing EBX, EDI, and the dword loaded from `[ESP+0x10]`;
its AL is not tested. Both paths return AL=1. These calls establish only the
local gates and forwarding; their contracts and the stack values' meanings
remain unresolved.

At VA `0x004C80B0` (RVA `0x000C80B0`), the helper loads `[ECX+0x18]` into ECX,
writes `0x40F` to `[ESP+4]`, and tail-jumps to `0x004C7710`.

At VA `0x005754E0` (RVA `0x001754E0`), a null dword at `[this]` returns AX=0
and removes one stack argument. Otherwise the helper calls `0x00575400` with
ECX=`this`; zero AL returns the same way. On the passing path it loads
`[this+0x18]` into ECX and tail-jumps to `0x006C7B80`, preserving the original
stack argument. The called routines' meanings are not established by these
instructions.

At VA `0x004D6B40`, the body calls through slot `+0x18` of the vtable reached
from `[ESI+0x174F8]`, then compares dword `[EAX]` with `4` and `5`
(`0x004D6B67`-`0x004D6B89`). The single stack argument becomes ECX for
`0x00445220` when neither value is `4` nor `5`; either value selects calls to
`0x00445470` and `0x00447A40`, with the same argument used as ECX for both.
Both paths then call through slot `+8` of the object at owner `+0x664`
(`0x004D6B8F`-`0x004D6C0E`).
These are direct call and branch observations; the indirect routines' contracts
remain unresolved.

## Direct call paths into `0x00C99B80`

The FF14-Memory candidate lead is
`tools/outputs/lpb/native_retainer_constructor_submit_deeper_20260618/target_instruction_decode.csv`
(SHA-256
`78d60986d3c65186d642be50adaa0c9a8a4d36f065ab3ac4bda1723c54b13bbf`). Its
`0x00C99C00` comparison is `cmp al,0x37` followed by an unsigned `jae` to
`0x00C99E1A`; the branch condition does not assign a meaning to the threshold.

The pinned executable contains direct calls to `0x00C99B80` at
`0x00C99E63` and `0x00C99EE2`. In `0x00C99E40`, the caller saves its incoming
ECX, forwards a value produced by `0x00445210`, tests the returned AL, and
normalizes it before `ret 0x14`. In `0x00C99E80`, the caller likewise restores
its incoming ECX for the call and tests the result before separate cleanup
paths. One outer path is `0x004D8560`: it calls `0x00C99E80` with ECX equal to
its incoming `this+0x7F8`, and `0x00C99E80` forwards that same receiver to
`0x00C99B80`. These bytes establish a caller boundary and receiver offset;
they do not identify the receiver type, the candidate's contract, or a
retainer-specific role. The `0x37` cutoff remains unexplained.

## Repeated call path at `0x004DAD80`

FF14-Memory's
`tools/outputs/lpb/native_retainer_setup_submit_next_20260618/target_instruction_decode.csv`
(SHA-256
`b7944d06c9085e008a75f3b3e0f7ffcb9fce46f339ce7c8314bb0ac6023653ad`,
rows 523-529) records a repeated call to `0x004D6B40`. In the pinned
executable, `0x004DAE7B` forms `EDI+EBP` and pushes it, `0x004DAE7F` sets ECX
from EBX, and `0x004DAE81` calls the helper. Afterward, the loop increments
ESI, advances EBP by `0x54`, and branches back. This establishes successive
call arguments separated by `0x54` bytes; it does not identify the pointed-to
data or assign a field layout.

`FUN_004DB800` conditionally calls `0x004DAD80` at `0x004DBABF` when its local
byte at `[ESP+0x24]` is nonzero, passing ECX=`ESI-0x4A4`. The tracked
`config/ffxivgame.vtable_slots.jsonl` (SHA-256
`b776f19827f3002b6fc7fd522812f23d851b9a6065d47620e54f01bd0ae5732f`, row
759) maps `FUN_004DB800` to slot 1 of `Application::Main::RaptureElementContainer`.
That caller label does not name the adjusted receiver or the loop arguments.
The caller and repeated-step facts do not establish a retainer workflow or
application-level meaning.

The tracked `config/ffxivgame.symbols.json` (SHA-256
`0639fc4a84a0778e67dc2f781f36f2132e83c3bb66ac6b6d0bf0eba9ebf33f81`, rows
4022-4023) reports `FUN_004DAD80` as size `0x170`, ending at `0x004DAEF0`.
Pinned bytes at `0x004DAEEC` contain `add esp,0xC4`, followed by `ret` at
`0x004DAEF2`; the next catalog symbol starts at `0x004DAF00`. The facts above
use the earlier loop and caller bytes only and do not treat the reported size
as an exact epilogue boundary.
