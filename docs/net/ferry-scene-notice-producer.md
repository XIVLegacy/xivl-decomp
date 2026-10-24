# Ferry scene notice constructors

This is a static outbound-message finding for the pinned retail 1.23b
`ffxivgame.exe` (image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`).
The locators below are VAs; subtract the image base for RVAs. Observations
come from the repository's local per-function x86 disassembly exported
by `tools/ghidra_scripts/DumpFunctions.java` from Ghidra 12.1. The
constructor and caller locators below establish only static client message
construction; they do not establish a retail server transaction.

The cited retained instructions were checked individually against the
15,996,808-byte PE with pefile `2024.8.26` and Capstone `5.0.7` in x86
32-bit mode. The short `_skip` adapter at `0x006E1970` was decoded directly
from its registered entry in the PE. No client execution was used.

## Selector producers and local results

`_play` firer `0x0074F810` registers adapter `0x006FB830` at
`0x0074F847`; the string at `0x00FD7F68` is `_play`. The adapter passes
the CutScene's string at `this+0x6C` to `0x0076D610`, then creates a
PlayingResumeChecker, stores it at `this+0x64` and clears the skip byte at
`this+0xC1` (`0x006FB90F`-`0x006FB917`). It installs the checker through
`0x00CD2860` at `0x006FB948`.

At VA `0x0076D79C`, `FUN_0076D610` writes code `0x00CE` to the local
message at offset 0 and `0x38` at offset 4. At VA `0x0076D7AC`, it writes
zero at offset `0x18` before calling `FUN_004D6D10` at `0x0076D7B9`.
The caller `FUN_006FB830` reaches this constructor at VA `0x006FB8E8`.

The second caller, `FUN_006FBC50`, reaches `FUN_00763DC0` at VA
`0x006FBCB2`. At VAs `0x00763E92`, `0x00763E9D`, and `0x00763EA8`,
that constructor writes the same code and size and then a dword at
message offset `0x18`. Its byte argument at `[EBP+0x0C]` is transformed
by `NEG DL; SBB EDX, EDX; ADD EDX, 2` at VAs `0x00763E49` through
`0x00763E7C`: a nonzero byte produces 1, and zero produces 2. It calls
`FUN_004D6D10` at VA `0x00763EB8`.

The byte supplied to that second constructor is specifically CutScene
`this+0xC1`, loaded at `0x006FBCA3`, not the callback's result boolean.
The accepted skip-widget call reaches `_skip` firer `0x007362A0`, which
registers adapter `0x006E1970` at `0x007362D7` with string `_skip` at
`0x00FD6DB8`. The adapter writes `1` to CutScene `+0xC1` at
`0x006E1974` and calls `0x0075B200`. That wrapper reaches
`0x005645F0 -> 0x00563C40(3)`. The skip call sets a flag and requests
a stop. It does not itself call either `0x00CE` constructor.

| Reached path | `0x00CE` application `+0x00` | Name source | Local result |
| --- | ---: | --- | --- |
| `_play` adapter `0x006FB830` | 0 | CutScene `+0x6C` | Installs a waiter and clears `+0xC1`. |
| Result callback `0x006FBC50`, skip byte zero | 2 | Same CutScene `+0x6C` | Stores the callback boolean and dword separately. |
| Same callback, skip byte nonzero | 1 | Same CutScene `+0x6C` | Stores the same separate result operands. |

The result path is actor-bound: scene-result handler `0x00563D70` reads a
result-record code at `+0x20` and dword at `+0x28`. Table `0x00563E48`
maps codes 0 and 3 to `0x00563DB6`, which calls `0x00574E50(1, dword)`
at `0x00563DCC`. Code 1 maps to `0x00563DE9`, which calls
`0x00574E50(0, dword)` at `0x00563E2B`. Code 2 takes the no-callback
tail. `0x00574E50` dispatches virtual slot `+0x8C`; the LuaActorImpl
table at `0x00FDFB2C` contains `0x0075D950` in that slot.
`0x0075D950 -> 0x008A3910` uses the CutScene RTTI descriptor
`0x012C17AC`, then reaches `0x006FBC50` for that resolved actor.

At `0x006FBC79`-`0x006FBC85`, the callback stores the dword at checker
`+4`, its boolean byte at checker `+8`, and a separate readiness value
from `dword[0x01377ECC]` at checker `+0x0C`. It then clears the actor's
checker pointer. Constructor `0x00713030` installs checker vtable
`0x00FD51AC`. Its `0x00713090` method reads `+0x0C`; result publisher
`0x007491F0` reads `+8` and `+4` through `0x00748870` and `0x00584F70`.
Those helpers use byte/tag-2 and dword/tag-0 values, respectively
(`0x00583E96`-`0x00583EA9`, `0x00583DF6`-`0x00583E09`). This is the
local pair consumed by the common CutScene Lua helper.

This distinguishes operands without assigning an unsupported cancel label.
The inspected result-record consumer does not establish which producer
creates each code for natural completion, cancellation, failure or a
continuation. That semantic edge stops at the record supplied to
`0x00563D70`, specifically `+0x20/+0x28`. A callback with a false boolean
still sends selector 2 when the skip byte is clear. Selector 2 therefore
cannot be used as a distinct completion or cancellation discriminator.
After an accepted skip prompt, selector 1 is conditional on the result
callback being reached with the same actor's skip byte still set. Neither
the widget nor the `_skip` adapter proves that callback occurred.

The second constructor also has a conditional `0x00D0` send at
`0x00764085`, after the `0x00CE` send. Its predicate is a separate byte
obtained by `0x006FBC9E -> 0x00763060`, not the result boolean. That branch
does not add fields to the `0x00CE` body or establish event correlation.

## Complete copied span and field boundary

`FUN_004D6D10` at VA `0x004D6D10` conditionally forwards to
`FUN_004E0240`, which reaches `FUN_00DAE010`. In the latter, VAs
`0x00DAE0A9`-`0x00DAE0B8` and `0x00DAE154`-`0x00DAE163` copy from
message offset `0x18` for declared size minus `0x10`. Thus `0x38` is
the constructor's total message size, not a 56-byte application body;
the copied body is 40 bytes. Internal record offsets and application offsets
are different domains:

| Application offset | Internal record offset | Bytes | Proven source |
| --- | --- | ---: | --- |
| `0x00` | `0x18` | 4 | Selector 0, 1 or 2 on the paths above. |
| `0x04` | `0x1C` | 32 | Zero-filled name buffer, overwritten with at most 31 bytes of the CutScene string. |
| `0x24` | `0x3C` | 4 | Copied stack storage with no explicit assignment in either constructor before this send. Value and semantic provenance remain unbound. |

The first constructor zeros the 32-byte temporary at
`0x0076D706`-`0x0076D715`, obtains string length at `0x0076D71E`, clamps
to `0x1F`, and copies at `0x0076D74F`. Four eight-byte stores at
`0x0076D768`, `0x0076D778`, `0x0076D78C` and `0x0076D7B0` fill record
`+0x1C..+0x3B`. The second constructor performs the equivalent operations
at `0x00763DE6`-`0x00763E33`, with name stores at `0x00763E4B`,
`0x00763E5F`, `0x00763E7F` and `0x00763EAF`.

For a stack-offset check, let `S` be ESP at `0x0076D706`.
The first record begins at `S+0x44`, selector at `S+0x5C`, name
at `S+0x60..S+0x7F`, and copied tail at `S+0x80..S+0x83`. In the second
constructor, let `S` be ESP at `0x00763DE6`. The corresponding offsets
are `S+0x130`, `S+0x148`, `S+0x14C..S+0x16B` and
`S+0x16C..S+0x16F`. Account for each intervening PUSH when comparing the
instruction operands. Neither name copy covers the tail. The full span is
located, but a fully assigned 40-byte semantic payload is not established.

No traced `0x00CE` field is an authenticated starting-event identifier,
actor key or sequence/context token. The local callback dword is not copied
into this body. The [paired airship script](../event/airship-scene-sequence.md#playback-return-and-event-correlation)
owns the first discarded-return edge and the separate incoming-event
endpoint. These observations do not verify a particular runtime scene name,
historical route, server acceptance or visible playback.

## Reproduction

Use an authorized matching executable. Save this decoder outside the tracked
tree and run `python <private-output>/scene_notice_decode.py
<ffxivgame.exe>`. It checks the identity and decodes only the listed
instruction ranges. It does not fill omitted regions of a retained listing.

```python
import hashlib
from pathlib import Path
import sys
import capstone
import pefile

raw = Path(sys.argv[1]).read_bytes()
assert len(raw) == 15996808
assert hashlib.sha256(raw).hexdigest() == (
    "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9"
)
pe = pefile.PE(data=raw)
assert pe.OPTIONAL_HEADER.ImageBase == 0x400000
decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
ranges = (
    (0x0074F83E, 0x51), (0x007362CE, 0x51), (0x006E1970, 0x23),
    (0x006FB8D1, 0x81), (0x006FBC50, 0x6D),
    (0x0076D706, 0xB8), (0x00763DE6, 0xD7),
    (0x00563D9A, 0xAE), (0x00574E50, 0x13),
    (0x0075D950, 0x2D), (0x008A3910, 0x38),
    (0x00713030, 0x60), (0x00713090, 0x0C), (0x007491F0, 0x2D),
    (0x00DAE0A9, 0x14), (0x00DAE154, 0x14),
)
for va, size in ranges:
    offset = pe.get_offset_from_rva(va - 0x400000)
    for insn in decoder.disasm(raw[offset:offset + size], va):
        print(f"{insn.address:08x} {insn.mnemonic} {insn.op_str}")
```

Read table `0x00563E48`, vtable cells `0x00FDFBB8` and
`0x00FD51AC..0x00FD51B4`, and strings `0x00FD7F68/0x00FD6DB8` with the
same PE translation. The retained constructor has an unrelated cleanup gap
`[0x0076D7E8,0x0076D7EB)`, and the adapter has a cleanup gap
`[0x006FB8CE,0x006FB8D1)`. Neither was filled or used to infer the payload.
