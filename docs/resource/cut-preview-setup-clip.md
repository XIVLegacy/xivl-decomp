# Cut scheduler preview setup clip

The nominal `RapturePreviewSetupClip` begin handler copies 32 payload bytes
and passes them with the clip pointer to a delegate. When its guard permits
the request, that delegate packs an internal selector-9 event containing a
zero dword, 32 bytes from its reloaded first argument and its reloaded second
argument. These equal the supplied owner buffer and clip pointer when the
preservation conditions below hold. The conditional
[event consumer](cut-preview-event-consumer.md) owns the subsequent string
input and callback-argument path.

## Source and nominal handler

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VAs. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7
in x86-32 mode recovered the original data and entry-seeded local flow.
Calls were not followed; existing getter, base-begin, queue and cookie
contracts are reused below. No retail execution is implied.

[The slot catalog](../../config/ffxivgame.vtable_slots.jsonl), line 31299,
names primary table `0x00FF9BF4` slot 1 as `0x008132D0`. The original
12 bytes at `0x00FF9BF0` contain COL `0x011657B4`, slot 0 `0x00813370`
and slot 1 `0x008132D0`, SHA-256
`62a176dca222cf99b62a439a03f6cb0e5e9da9eaa032d2a61d44f47b2a84844c`.
The COL's 20-byte SHA-256 is
`3bd9af6bb397838fed61fab8180cdc8c75243c5abb5791ffbd86cf1a846872dd`.
Its signature, complete-object offset and constructor displacement are zero.
Type descriptor `0x012C9120` holds
`.?AVRapturePreviewSetupClip@Clip@Cut@Scene@Application@@`.
This authenticates the nominal table, not a current clip or its factory.

The finite inspection caps were `[0x008132D0,0x00813360)` and
`[0x00813190,0x00813200)`. Catalog sizes were not used as function ends.
The reached normal bodies ended at the following return instructions:

| Entry | Recovered interval, end exclusive | Exit | SHA-256 |
| --- | --- | --- | --- |
| `0x008132D0` | `[0x008132D0,0x00813360)` | `RET 4` at `0x0081335D` | `843eae79dcada782abec431c7f4cc87b2939e5c01fd9aa7b90aca5d2f0a1c4b2` |
| `0x00813190` | `[0x00813190,0x008131F5)` | `RET 8` at `0x008131F2` | `04fa380de3ebaeb0b672dacd6426b0752f4e9379b4cd79f9d67a0cbbcbf0f9a8` |

## Begin context and fixed copy

Let C be the incoming clip receiver and F its explicit begin-context argument.
At `0x008132E7`, the handler calls base begin `0x00A20F30` with C and F.
The [scheduler timing contract](cut-scheduler-clip-timing.md#begin) owns its
phase and elapsed-time behavior.

The handler calls `0x00A20030` on F at `0x008132EE`, then `0x0060B360`
on its nonzero result at `0x008132F9`. Each zero result skips the payload
copy. The existing
[typed context chain](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
conditionally identifies the second result K as the RaptureActorContainer.
The handler reads M=`u32(K+0x68)` at `0x00813302` and requires M nonzero.
On that typed route, M is the cached CutManagerActor.

`0x00813309` forms M+`0x164`; `0x0081330F` tests that computed address
for zero without dereferencing it. This is a separate wrapping 32-bit
address test. It does not change ECX from M to M+`0x164`.

On the remaining path, `0x00813313` freshly loads D=`u32(C+0x14)`.
Four qword reads at `0x00813316`, `0x00813321`, `0x0081332C` and
`0x0081333C` copy D+`0x14`, `0x1C`, `0x24` and `0x2C` into one
contiguous 32-byte stack buffer B. The local code does not test D, scan for
a terminator or interpret those bytes as a variable-length string.

At `0x00813348`, `0x00813190` receives ECX=M and nearest-first explicit
arguments `[B,C]`. The callback-context and actor pointers are not substituted
for B. The [authored preview records](grand-company-cut-preview-owners.md)
place a NUL-terminated owner string at D+`0x14`, followed by zeros, in
52-byte records. The fixed copy covers those 32 trailing bytes if the current
D is one of those records; the nominal handler alone does not bind it.

## Delegate and internal event payload

`0x00813190` first calls opaque guard `0x0060F3A0` with receiver M and
no explicit argument at `0x00813196`. AL=0 skips the request. On AL!=0,
the delegate reads its first argument after the guard and copies all 32 bytes
into a new stack payload Q. It reloads its second argument after the guard.

| Q offset | Width | Value | Local write sites |
| --- | --- | --- | --- |
| `0x00` | 4 | Literal zero | `0x008131D7` |
| `0x04` | 32 | Bytes actually read through the reloaded first-argument pointer after the guard | `0x008131A7`, `0x008131B2`, `0x008131C3`, `0x008131DF` |
| `0x24` | 4 | Dword reloaded from the second-argument cell; C if that cell retains its supplied value | `0x008131E5` |

At `0x008131E9`, the delegate calls `0x007CE7B0` with receiver M and
nearest-first arguments `[9,Q,0x28]`. It does not test that call's result.
The [published queue contract](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport)
adds 3 to the internal selector, copies the supplied payload into an envelope
and queues a copy; its consumer subtracts 3 before element dispatch. The new
tuple therefore supplies selector 9 and 40 bytes to that existing mechanism.
Selector 9 is an internal engine event value, not a retail packet opcode.
Actual enqueue success and later receiver selection remain conditions.

Under ordinary argument cleanup, callee-saved-register preservation and
valid distinct stack/object storage, the composed payload contains the
caller's fixed copy and C when the reloaded argument cells hold B and C at
their respective reads and B's bytes remain unchanged through the copy.
On that composition the payload includes C as a pointer. Copying its four
bytes does not retain the clip object or establish its lifetime until
asynchronous consumption.

## Remaining consumer boundary

The guard's predicate and the clip pointer's later use remain untraced here.
The separately qualified [selector-9 consumer](cut-preview-event-consumer.md)
does not establish actual key-to-element selection. There is no local owner-name lookup, quest selection,
text-bank load, zone binding or authoritative quest-state mutation. These
bodies alone do not prove successful preview setup or historical playback.

The begin handler's epilogue calls `0x009D20F4` at `0x00813355` before
its return. The [same-binary cookie contract](skeleton-output-float4-submission.md#complete-direct-closure)
permits return on cookie equality and leaves its mismatch target opaque.
Neither body assigns a uniform successful return value. Their normal local
return instructions do not establish that their opaque calls or cookie check
returned in a particular execution.
