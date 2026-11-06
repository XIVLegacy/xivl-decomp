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
effects follow the [class-to-appearance request](cut-actor-class-update.md).

## Integer vector to ScheduleRegister

The native integer-vector handoff preserves the index on the following
conditional path. Let V be `_play`'s produced integer vector, Q the local
start record, M the receiving CutManagerActor, C its selected controller,
W the controller's CutSceneObject and S that wrapper's Scheduler. Valid
storage, normal returns, retained tables/pointers and no intervening overwrite
are conditions throughout. The selected record/register composition below
covers at most eight integer entries; the copy loops do not themselves clamp
an arbitrary count to that capacity.

### Request and actor copies

Argument splitter `0x006EED90` packs a leading run of Boolean cells into
one mask entry, then appends consecutive integer cells in their original
order. Its mask append is at `0x006EEECC`; integer appends are at
`0x006EEF38`. Thus a leading true followed by seven integers produces
eight integer entries, with mask 1 at index 0 and the first integer at
index 1. This describes native typed cells, not their unobserved Lua inputs.

The `_play` adapter and
[local scene transport](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport)
lead through `0x0076D610 -> 0x0055FFE0`. In the latter's accepted state-0
branch, `0x00560026` calls record builder `0x00567930`, then
`0x00560035` posts selector 0 with size `0xE8`. The builder stores the
integer count at Q+`0x20` and copies V[i] to Q+`0x24+4*i`
(`0x00567980`, `0x00567985`, `0x005679A3..0x005679AE`). The
published selector-0 route reaches `0x0061C4D0` on the resolved M.

That consumer stores the count at M+`0x274` and copies the corresponding
words from Q+`0x24` to M+`0x278`, incrementing both addresses by 4 without
changing the index (`0x0061C50C..0x0061C535`). Its later call to the
begin helper is separate from applying these saved parameters.

### Setup and indexed writes

The state-1 branch of setup callback `0x0061C930` calls
`0x0061C8A0` on its successfully cast actor at `0x0061C992`. If the
latter obtains nonzero M+`0x178`, it calls controller C=`u32(M+0x174)`
slots 1 and 2, then `0x00611BA0`, then `0x0060EF00` at
`0x0061C91F`. With the
[default embedded controller](../net/ferry-scene-notice-producer.md#default-result-controller),
slot 1 selects `0x007DFEC0`. That method stores the returned wrapper
from `0x00635690` at C+4 at `0x007DFF65`. Wrapper construction and
the returned Scheduler relation are owned by the
[base Scheduler and wrapper contract](action-queue-scheduler-handoff.md#base-scheduler-and-wrapper-return).

`0x0060EF00` first calls controller slot 6, byte offset `0x18`, for
indexes 0 through 7 with word `0xFFFFFFFF` (`0x0060EF15..0x0060EF2B`).
It then reads count M+`0x274`, starts at M+`0x278` and calls the same
slot with `(i,savedWord[i])` in increasing index order
(`0x0060EF6B..0x0060EF9D`). No index adjustment occurs between those
loads and calls.

The original default table `0x00FEF2B0` cell `0x00FEF2C8` selects
`0x007DFD80`. That bridge returns with `RET 8` without a write when
W=`u32(C+4)` is zero. Otherwise it calls W's slot 50, byte offset
`0xC8`, with plain-RET/no explicit arguments, then tail-jumps to the
returned interface's slot 18, byte offset `0x48`. The original
`(index,value)` stack words remain intact for that final setter.

CutSceneObject constructor `0x00A05000` installs table `0x010942E4`
at `0x00A05027` and stores its incoming S at W+8 at `0x00A0502D`.
Cell `0x010943AC` selects slot-50 getter `0x00A03AF0`. Its slot-14
predicate `0x00A03600` tests W+8; the getter returns S+`0xF0` for
nonzero S and zero otherwise. The controller does not test that returned
interface for zero. A successful handoff therefore requires both W and S
to be nonzero, with the named tables retained.

### Actual parameter interface and read

Scheduler constructor `0x00A1EBE0` forms S+`0xF0` at
`0x00A1ED3B` and constructs it through `0x00A29A50` at
`0x00A1ED45`. This embedded object I has its own table `0x010989AC`,
installed at `0x00A29A53`, and eight zero-initialized words at I+4
through I+`0x20`. Original RTTI COL `0x01186760`, descriptor
`0x012EDE7C`, identifies it as
`.?AVScheduleRegister@Scheduler@Cut@Engine@CDev@SQEX@@`.

| Interface operation | Original cell / target | Complete body, end exclusive | Word operation |
| --- | --- | --- | --- |
| Slot 17, getter | `0x010989F0` / `0x00A29AC0` | `[0x00A29AC0,0x00A29ACB)` | Return `u32le(I+4+4*i)`; `RET 4` |
| Slot 18, setter | `0x010989F4` / `0x00A29AD0` | `[0x00A29AD0,0x00A29ADF)` | Store value to `I+4+4*i`; `RET 8` |

Neither body checks its index or adjusts its receiver. These are raw 32-bit
word operations, without a signedness interpretation. The outer Scheduler
table `0x01097A54` also has a slot 17, targeting `0x00A1CB60`; that is
a different interface and is not selected by this clip's S+`0xF0` load.

The [retained-item update](action-queue-scheduler-handoff.md#retained-item-and-scheduler-update)
supplies the constructed S to the
[updater/context chain](scb-clip-factory-composition.md#secondary-collection-callers-and-callback-context).
In root tick `0x00A1D330`, `0x00A1D34E` retains its incoming S in ESI.
`0x00A1D35F..0x00A1D368` passes that S to local context constructor
`0x00A1FF70`, whose store at `0x00A1FF81` writes it to F+`0x0C`.
The tick reuses this F for the begin call at `0x00A1D6FA` after the
time setter `0x00A200F0`, which writes only F+`0x10`.
Earlier clip callbacks also receive F, so retention of F+`0x0C` through
those calls remains a condition; a whole-program exclusion of callback
writes is not claimed.

At `0x00826055..0x00826071`, the actor-class evaluator obtains
S=`u32(F+0x0C)` through `0x00A20070`, then loads the table at
S+`0xF0`, supplies I=S+`0xF0`, and calls its slot 17 with the record's
index. On the same retained S after the indexed write:

```text
getter(i) = u32le(S + 0xF4 + 4*i) = savedWord[i] = V[i]
index 1  = u32le(S + 0xF8)       = u32le(M + 0x27C)
```

For the selected `man50250` record, actor 5 therefore consumes V[1].
The leading Boolean mask, when present, remains V[0]. If the apply path
resets the register but supplies fewer than two entries, index 1 retains
`0xFFFFFFFF`; the existing class-value delegate rejects that sentinel.
This closes the static index relationship. It does not establish actual
arguments, successful object construction, a reached callback, retained
runtime state or appearance. The class word's
[queued appearance effects](cut-actor-class-update.md) are a separate contract.

Original file-backed checks used the Python/pefile/Capstone versions above,
entry-seeded instruction flow with finite bounds, original cells/RTTI and
independent operand checks for the copies and indexed operations. A fresh
read-only Ghidra 12.1.3/JDK 21 run used `DecompileToText.java`, SHA-256
`83a31add99e86bea0128470e4cc52b193e373ebc342cf060861bd3c17339668a`,
for the builder, actor consumer/apply/setup, controller/wrapper bridge and
ScheduleRegister constructor/getter/setter. All twelve selected decompilations
completed without an analysis timeout. Ghidra did not recover the controller's
indirect tail-call signature; the original instructions and cells establish
its receiver and stack forwarding. No target execution was used.

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
