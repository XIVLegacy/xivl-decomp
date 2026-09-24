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
