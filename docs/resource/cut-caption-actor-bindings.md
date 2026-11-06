# Cutscene caption and actor-class inputs

The nominal `RaptureCaptionClip` evaluator posts an internal selector-`0xB`
event containing caption kind 2, the record's actor-class word, its text
operand and two argument vectors. The selected CutManagerElement consumer
forwards that payload toward implementation slot 30. The LuaActorImpl route
requests `_onShowUIClip`, with a tag constructed from the literal `Caption`.
These are conditional native input contracts, not observed caption display.

The separate `RaptureCharaActorClassIdClip` uses its record's actor ordinal
and either a literal class value or a supplied integer parameter. In the
selected `man50250` asset it addresses actor 5, while the `m854t0` dictionary
record is actor 9. That clip does not supply a class override for actor 9.

## Source and interpretation

Source: retail FFXIV 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes,
image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses are VAs. Python 3.12, pefile 2024.8.26 and Capstone
5.0.7 in x86-32 mode read original file-backed data and entry-seeded local
instructions. Explicit inspection caps bounded the bodies below. Direct calls
were treated as opaque unless a separate body or published contract is cited.

The [slot catalog](../../config/ffxivgame.vtable_slots.jsonl) identifies the
nominal classes and slots. The selected original cells agree:

| Class | Table / slot | Cell / target |
| --- | --- | --- |
| `RaptureCaptionClip` | `0x00FFBAFC` / 1 | `0x00FFBB00` / `0x00814230` |
| `RaptureCharaActorClassIdClip` | `0x01020074` / 1 | `0x01020078` / `0x00825FA0` |
| `LuaActorImpl` | `0x00FDFB2C` / 30 | `0x00FDFBA4` / `0x0075D890` |

This authenticates nominal dispatch cells. It does not select the current
runtime object, installed table or callback definition.

## Caption record to internal event

Let C be the clip receiver, F its begin context and D=`u32(C+0x14)` the
serialized record. The evaluator at `0x00814230` calls base begin
`0x00A20F30`, then obtains a nonzero context through `0x00A20030` and
`0x0060B360`. On the
[published typed context route](attribute-resource-and-scene-binding.md#character-factory-and-world-provider),
the second result is the RaptureActorContainer and its `+0x68` word is the
CutManagerActor M. The evaluator requires M and wrapping address M+`0x164`
to be nonzero. It does not dereference M+`0x164` at that guard.

After initializing its integer and string vectors, it reads byte `+4` of
the key entry at `u32(C+0x18)`:

| Key-entry byte | Local input handling |
| ---: | --- |
| 0 | Copies `u32(D+0x1C)` and `u32(D+0x18)` |
| 1 | Copies the same words and reads the argument mask at `D+0x24` |
| Other | Returns without the selected post |

The two copies are at `0x008142D4..0x008142DE` and
`0x008142EF..0x008142F9`. On byte 1, bits 1-7 of the mask select integer
entries through scheduler parameter slot 17; bits 20-24 select strings
through slot 29. A zero mask skips both argument-appending bodies. This
does not decode parameter values supplied at runtime.

At `0x0081438D..0x008143C6`, the evaluator builds Q and calls
`0x007CE7B0` with ECX=M and nearest-first words `[0xB,Q,0x14]`:

| Q offset | Supplied value |
| ---: | --- |
| `0x00` | Literal kind 2 |
| `0x04` | Actor-class input, `u32(D+0x1C)` |
| `0x08` | Text operand, `u32(D+0x18)` |
| `0x0C` | Pointer to the evaluator's integer vector |
| `0x10` | Pointer to its string vector |

The [queue contract](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport)
owns selector adjustment, payload copying and later element dispatch.
Copying pointer words does not establish their pointee lifetime, queue
success or an eventual receiver. The selector is an internal event code.

## Conditional callback route

For the nominal CutManagerElement interface, `0x00566740` subtracts 4
from the selector. The original cell at `0x00566870`, index `0xB-4`,
contains `0x005667FE`. That arm copies Q's five words into a local block
and calls `0x0055FCB0` at `0x00566825`. No local payload-length check
was recovered in the selected arm.

`0x0055FCB0` requires `u32(I+0x80)` nonzero, then forwards the five words
to `0x00578740`. The
[holder contract](../input/chara-selection-producers.md#selected-actor-implementation-handle-and-slot23)
conditionally identifies that pointer as the implementation holder.
`0x00578740` copies both argument vectors and invokes current implementation
slot 30 with `(kind,actorClass,textOperand,integerVector,stringVector)`.
Its allocation/assertion helpers and their normal returns remain conditions.

With the nominal LuaActorImpl table retained, `0x0075D890` forwards the
three scalar inputs and copied vectors through `0x008A39E0`. A successful
cast in that bridge calls `0x006FC080` with the script control read from
implementation `+8` and object handle at implementation `+4`.

The latter body appends the tag object at `0x0134B564 + kind*0x54`,
then the actor-class and text operands through `0x00585020`. It appends
the supplied integer and string vector entries in their stored order. The
integer helper ends in the published integer appender `0x00584F70`;
the string appender is `0x00748C80`. The
[operator-vector contract](../event/event-base-callback-arguments.md#operator-vector)
owns their Lua conversion and the request bridge.

Initializer `0x00F17DE0` constructs tag entry 2 at `0x0134B60C` from
literal `Caption` at `0x00FD45FC`, through `0x00447260` at
`0x00F17E2E`. At `0x006FC1F7`, the callback body constructs the separate
lookup name from `_onShowUIClip` at `0x00FD467C`, then requests the call
through `0x00CC7A90` at `0x006FC216`.

With initialized and preserved strings, valid vectors, normal helper
completion and the retained callback binding, the resulting Lua frame is:

```text
(self, "Caption", actorClass, textOperand, integers..., strings...)
```

The native route does not itself resolve the actor-class word to a visible
speaker name or select a quest's text bank.

## Actor-class clip

The nominal evaluator `0x00825FA0` obtains the same typed context and M
guards. At `0x00826012`, it reads `u8(D+3)` and passes that actor ordinal
to `0x00A1FFA0`. Its subsequent type check must match before the selected
class-value path proceeds.

It reads mode `u32(D+0x10)` at `0x00826044`. Mode 1 uses literal
`u32(D+0x14)`. Mode 0 queries parameter slot 17 with index
`u32(D+0x14)` at `0x0082606F`. Other modes leave that path. The selected
result and target's constructed name are supplied to `0x00613960` at
`0x008260A5`. That delegate rejects class value `0xFFFFFFFF` or a null
first explicit word before its call to `0x006121A0`. Its broader actor
replacement effects are not established here.

## Selected man50250 records

Source `client/cut/man50250/man50250` is 15,086,704 bytes, SHA-256
`fd3c01f588cd11668d863f90788cec0a31058e5e8341a9412578074315aefac7`.
The SCB at `0x9D94B4` is 445,568 bytes, SHA-256
`a240c14388339088cb8947f95fa68edc958c6c5a337cbd548167454b3d50b6ff`.
The [scheduler layout](cut-scheduler-clip-layout.md) and
`tools/scb_clip_scheduler.py` parse its actor and clip records.

All 14 `RaptureCaptionClip` records select key-entry byte 1, have zero
argument mask at `+0x24`, and carry text operands 129-142 exactly once.
At file offset `0x9EE750`, the record's actor-class input is `6000244`
and its text operand is 137. Under the native route above, its scalar
callback prefix is therefore `(self,"Caption",6000244,137)`.

The only `RaptureCharaActorClassIdClip`, at file offset `0x9DB760`, has
actor ordinal 5, mode 0 and parameter index 1. Actor 9's separate dictionary
record, at `0x9D97E0`, is labeled `m854t0` and has numeric token `6000244`
at `+0x30`. The clip's ordinal cannot be treated as an override for that
record. Actual supplied parameters, actor construction and appearance,
caption formatting and rendered/audio outcomes remain runtime questions.
