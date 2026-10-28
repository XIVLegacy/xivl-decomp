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
The native producers and local transport below establish where the result
code and dword come from. A callback with a false boolean still sends
selector 2 when the skip byte is clear. Selector 2 therefore cannot be used
as a distinct completion or cancellation discriminator.
After an accepted skip prompt, selector 1 is conditional on the result
callback being reached with the same actor's skip byte still set. Neither
the widget nor the `_skip` adapter proves that callback occurred.

The second constructor also has a conditional `0x00D0` send at
`0x00764085`, after the `0x00CE` send. Its predicate is a separate byte
obtained by `0x006FBC9E -> 0x00763060`, not the result boolean. That branch
does not add fields to the `0x00CE` body or establish event correlation.

## Native result producers and local transport

These records are local messages, distinct from serverbound Map `0x00CE`.
`0x004D7980` adds `0x15` to its selector at `0x004D79D2`, then constructs
a ScenePacket through `0x004EC080`. Scene dispatch `0x007C93C0` subtracts
`0x15` and calls actor virtual slot `+0x274`
(`0x007C95D3`-`0x007C95F2`). The CutManagerActor vtable at `0x00FB8DA4`
holds `0x0061C6C0` in that slot. Its table at `0x0061C780` binds:

| ScenePacket type | Local selector | Reached consumer |
| --- | ---: | --- |
| `0x15` | 0 | `0x0061C4D0`, copies request fields and calls `0x00611C30`. |
| `0x16` | 1 | `0x0060F440`, requires the stored name to match and actor `+0x1ED` to be set before applying arguments. |
| `0x17` | 2 | `0x0060F3F0`, raises actor `+0x1E8` to the supplied dword if larger and requests a stop through controller slot `+0x0C`. |

`0x00563C40(3)` puts dword 3 in the `0x17` payload
(`0x00563D48`-`0x00563D57`). The admitted start clears actor `+0x1E8`
at `0x00611C79`. These are separate from CutScene `+0xC1` and from the
scheduler notification field at CutManagerActor `+0x1E4`.

Producer `0x0061A0B0` requires actor `+0x180` nonzero and the first byte
of its name at `+0x181` different from `0x2A`. It copies the 32-byte name
to result storage at actor `+0x228` and copies actor `+0x1E8` to result
`+0x20` (`0x0061A0F5`-`0x0061A135`). It obtains a dword from virtual
slot `+0x14` of the controller at actor `+0x174`, passing actor `+0x224`
(`0x0061A14A`-`0x0061A15C`). It appends that dword to a temporary vector
through `0x00559540`; `0x00620010` writes the vector count at result
`+0x24` and its first dword at `+0x28`. It emits selector 4, size `0x2C`,
at `0x0061A17C`-`0x0061A183`.

The CutManagerActorSchedulerListener vtable `0x00FEF314` has
`0x00611810` and `0x00611850` in slots 1 and 7. Those methods set actor
`+0x1E4` to 1 or 2 respectively, only when it is zero. The actor update
at `0x0061C790` calls `0x0061A0B0` at `0x0061C7B8` or `0x0061C812`
when that field is nonzero, subject to its intervening actor-state tests.
Thus the notification selects when to publish; the published result code
comes from `+0x1E8`. If it remains zero after start, the consumer returns
true with code 0. A delivered stop operand 3 can instead reach the same
true callback with code 3. The callback boolean alone cannot distinguish
those paths, and the static route does not prove a requested stop was delivered.

A separate producer `0x0061A1C0` requires an active named actor and
`+0x1ED` clear. It calls `0x0061FF70` with code 1 at `0x0061A257`, emits
the same selector 4 and size `0x2C` at `0x0061A263`, then sets `+0x1ED`.
Its direct caller is `0x0061B000`, reached by
RapturePendingStopClip method `0x008280D0` at `0x00828114`. The clip
supplies its signed byte at data `+0x14` to the controller result lookup.
This is an authored pending-stop route to the false callback, not evidence
that every code-1 result denotes user cancellation or failed initialization.

Both producers call `0x007CE7B0`. It uses actor `+0xDC` as a lookup key,
adds 3 to the selector, constructs the envelope with `0x004D65E0`, and
queues it at the resolved main object `+0x17928`
(`0x007CE806`-`0x007CE82E`). The constructor copies the supplied bytes
to envelope `+0x20`; the queue copies its record through `0x004D66C0`
at `0x007CF845`. Consumer `0x004DAA10` subtracts 3, resolves the key and
invokes element slot `+0x20` with envelope `+0x20` and its size
(`0x004DAA95`-`0x004DAAC4`). CutManagerElement vtable `0x00FA4DB8`
binds that slot to `0x00566740`; its table `0x00566854` maps selector 4
to the call of `0x00563D70` at `0x00566762`. This closes the local
producer-to-result-handler route conditionally on the key resolving to that
element. It supplies no originating event or scene-invocation token.

## Enclosing request completion

The generic request listener also has an authenticated return path.
`0x00CCF290` invokes the pending checker's result publisher through slot
`+8` at `0x00CCF332`. When the Lua thread is no longer yielded, the
branch at `0x00CCF55F` permits function-end result collection into request
`+0xEC` through `0x00CFAAE0` at `0x00CCF64C`, then calls `0x00CCE8E0`
at `0x00CCF6EA`. An empty operator/result range can bypass collection and
still reach that call. `0x00CCE8E0` dispatches phase 0 at `0x00CCE9B5`
when the request's `0x3C`-entry range at `+0xDC/+0xE0` is empty.
Phase 0 invokes listener slot 1 at `0x00CCDA75`.

The incoming event executor registers its PlayerManager listener through
`0x00CD0940` at `0x00897199`; vtable `0x0105706C` binds slot 1 to
`0x00896090`. That listener reads the request's word at `+0x14` through
`0x00CD2BC0 -> 0x00CD08F0`, compares it first with word `0x0134BC18`,
then with word `0x0134BC1C`, and only the second match reaches
`0x00894AB0` (`0x00896135`-`0x00896161`). The second argument is
specifically the same request's result container at `+0xEC`, returned by
`0x00CD2C70` at `0x00896152`.

The two words have no file-backed image values, but the PE contains
initializer routines: `0x00F18620` supplies `0xC9` and address
`0x0134BC18`, while `0x00F18640` supplies `0xCA` and address
`0x0134BC1C`. Both call `0x00CD0A10`, whose store at `0x00CD0A17`
writes the supplied 16-bit value. The executor selects the second cell
through `0x006DE1E0` at `0x00897179`. These initializer and comparison
instructions establish the static `0xC9/0xCA` contract; they do not observe
the cells' values in a historical invocation.

`0x00894AB0` serializes that container into its 64-byte buffer and sends
it through `0x00894090` (`0x00894B44`-`0x00894B65`). The wrapper obtains
the reply context through `0x0075D460` and `0x00CC73B0`, then calls
serverbound Map `0x012E` builder `0x0075E670` at `0x0089410A`.
Application `+4` is the context dword copied from input `+0x14` by
`0x0075D460`; its domain meaning is unresolved. No event name is added to
this reply. The full operand layout is owned by
`xivl-client-structs:structs/network/event-update.md`; the typed result
encoding and error route are owned by the
[generic executor finding](../script/delegate-command-producer.md#completion-reply-serializer).

This joins a generic request's return container to its conditional reply,
not the airship pair's two local values to a successful server continuation.
The paired helper discards those values, and the error path can also reach
the phase-0 listener. Neither `0x00CE` selector 2 nor a terminated or empty
`0x012E` result block certifies natural completion, cancellation, or success.

The first remaining invocation edge is the binding of a particular incoming
event request and Lua thread to the two created CutScene actors and to the
resolved CutManagerActor/element keys. The remaining dword-value edge is the
controller slot `+0x14` called at `0x0061A15C` or `0x0061A237`; its result
is not an event identifier merely because it reaches Lua. The smallest local
comparison records those request/thread/actor identities and generations,
the `0x15/0x16/0x17` delivery, both producer code/dword pairs, and the
same request at `0x00896152` and `0x0089410A`, with exact `0x00CE/0x012E`
operands. Natural play, accepted skip, and an authored pending-stop case
must remain separate observations. This is contributor-owned local
instrumentation, not a future retail-server capture or proof of historical
route policy.

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
    (0x004D79C0, 0x38), (0x004EC080, 0x6A),
    (0x007C95CB, 0x29), (0x0061C6C0, 0x71),
    (0x00563D48, 0x14), (0x0060F3F0, 0x47),
    (0x00611C30, 0x59), (0x0061A0B0, 0x110),
    (0x00620010, 0x62), (0x00611810, 0x38), (0x00611850, 0x38),
    (0x0061C790, 0x9C), (0x0061A1C0, 0x102),
    (0x0061FF70, 0x95), (0x008280D0, 0x5B), (0x0061B000, 0x1A),
    (0x007CE7B0, 0xBF), (0x004D65E0, 0x3A), (0x004D66C0, 0x30),
    (0x007CF7F0, 0x88), (0x004DAA95, 0x31), (0x00566740, 0x2E),
    (0x00CCF55D, 0x192), (0x00CCE909, 0xB1),
    (0x00CCDA53, 0x24), (0x00897171, 0x2D),
    (0x008960C2, 0xA4), (0x00CD2BC0, 0x1D), (0x00CD08F0, 0x17),
    (0x00CD2C70, 8), (0x00F18620, 0x3B), (0x00CD0A10, 0x0D),
    (0x00894AE5, 0x85), (0x008940B4, 0x5B), (0x0075D460, 0x17),
)
for va, size in ranges:
    offset = pe.get_offset_from_rva(va - 0x400000)
    instructions = list(decoder.disasm(raw[offset:offset + size], va))
    assert sum(insn.size for insn in instructions) == size, hex(va)
    for insn in instructions:
        print(f"{insn.address:08x} {insn.mnemonic} {insn.op_str}")
```

Read table `0x00563E48`, vtable cells `0x00FDFBB8` and
`0x00FD51AC..0x00FD51B4`, and strings `0x00FD7F68/0x00FD6DB8` with the
same PE translation. Also read jump tables `0x0061C780` and `0x00566854`,
and the cited cells of vtables `0x00FB8DA4`, `0x00FEF314`, `0x00FA4DB8`,
`0x01028284` and `0x0105706C`. Their RTTI descriptors are respectively
`0x012B7894`, `0x012C6970`, `0x0126F668`, `0x012CD680` and
`0x012D7BA8`; read the mangled names at descriptor `+8`.
The generic completion checks are also reproduced by
`python tools/verify_run_event_function.py --exe <matching-executable>`.
The retained constructor has an unrelated cleanup gap
`[0x0076D7E8,0x0076D7EB)`, and the adapter has a cleanup gap
`[0x006FB8CE,0x006FB8D1)`. Neither was filled or used to infer the payload.
