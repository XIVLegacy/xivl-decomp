# Cut scheduler clip timing

This page records how the retail 1.23b client starts and ends the clips of an
embedded `SEDBSCB` scheduler. It names the clip fields that carry the start and
the end, the unit of those fields, and the scheduler routines that compare them
with the scheduler time. It covers the twenty classes of the
[first payloads page](cut-scheduler-clip-payloads.md), the
[second payloads page](cut-scheduler-clip-payloads-ranks-7-13.md) and the
[third payloads page](cut-scheduler-clip-payloads-ranks-14-22.md). The record D
and its offsets are those of the [clip layout](cut-scheduler-clip-layout.md#clip-records).
All values are little-endian. S is the scheduler object, and `S+0x..` names a
field of it.

## Evidence

The evidence identity, tools and tier labels are those of the
[first payloads page](cut-scheduler-clip-payloads.md#evidence). The manifest
[scb_clip_classes.json](../../config/scb_clip_classes.json) records the timing
instruction/constant byte checks in `staticChecks` and their relationships in
its `timing` section.
`tools/verify_scb_clip_classes.py --exe <ffxivgame.exe>` compares them with the
pinned image, and `--install <game-root>` checks the OBSERVED invariants against
the `client/cut` tree.
The additional [ActionClip limit writers](#actionclip-limit-writers) use the
separate original-PE byte/decode verification described in the linked
[child-delivery reproduction](scb-clip-factory-composition.md#other-checked-entries-and-reproduction).

## Time unit

The scheduler clock counts ticks.

| Item | Locator | Value |
| --- | --- | --- |
| Ticks per second constant | double at `0x00f67020` | 300000.0 |
| Ticks per frame | dword at `0x012ed240` | 10010 in the image |
| Ticks per frame setter | `0x00a20490` | Stores the truncation of 300000.0 / f for a float f. |
| Frame rate helper | `0x00a204b0` | Returns 300000.0 / ticks per frame as a float. |
| Clock advance | `0x00a204d0` | Adds the truncation of ticks per frame times the clock rate to the 64 bit time at clock +8. A nonzero pending delta at clock +0x10 is added to that step once and cleared, and a negative sum is clamped to 0. |
| Pending delta setter | `0x00a203c0` | Stores its dword argument at clock +0x10, or adds it when its byte argument is nonzero. |
| Clock rate | `0x00a203b0` | Returns the float at clock +0x14, or 1.0 when the byte at clock +0x18 is 0. |
| Clock initializer | `0x00a20310` | Sets the time to 0, the rate to 1.0 and the byte at +0x18 to 1. |
| Motion frame rate constant | double at `0x00fa7b18` | 30.0 |
| Frame number | `0x00a1b860` through `0x00a203e0` | The 64 bit clock time at clock +8 divided by ticks per frame, as a signed 64 bit division. |
| Milliseconds per second constant | double at `0x00f91c48` | 1000.0 |

The scheduler keeps its clock at S+0xd0. The motion code converts ticks to frames
by multiplying by 30.0 and dividing by 300000.0 (`0x007ad020`, used by the
[blend clip](cut-scheduler-clip-payloads-ranks-7-13.md#rapturenqmotionblendonoffclip)).
DepthOfFieldClip slot 2 divides the elapsed ticks by the ticks per frame.

INFERRED: a tick is 1/300000 of a second and f is a frame rate in frames per
second, so a frame is 300000 / f ticks. The image value 10010 is the truncation
of 300000 / 29.97. A value of f of 30 gives 10000 ticks per frame, and 10000
divides every start tick in the install.

UNKNOWN: the float passed to the setter comes from slot 2 of the object that
slot 4 of the object at `[manager +0x74] + 4` returns in `0x00a025c0`, and from
the observer method `0x00a036e0`. The type and the value were not traced, so
the frame rate of a running client is not established. A consumer that needs a
frame length must take f as a CALIBRATION input. The only static ticks per frame
is the image value.

## Clip start and end

Every clip is an object that holds D at clip +0x14 and a key entry at +0x18.
The scheduler reads the timing of a clip through these virtual methods.

| Slot | Method | Reads |
| ---: | --- | --- |
| 7 | `0x00a21070` | `u32(D+4)`, the start tick |
| 8 | `0x00a21080` | Jumps through slot 7, so the end tick is the start tick unless a class overrides it. |
| 9 | `0x00a210a0` | Clip +0x28, the elapsed ticks. |
| 11 | `0x00a210e0` | Clip +0x1c, the state. |
| 13 | `0x00a21100` | Clip +0x20, the phase word. |
| 19 | `0x00a21160` | Bit 16 of clip +0x30, the abort bit. |

Slot 12 (`0x00a210f0`) sets the phase word and slot 18 (`0x00a21140`) sets the
abort bit. The duration of a clip is slot 8 minus slot 7. A class that does not
override slot 8 has a duration of 0.

| Class | Start | End | Phase and abort calls |
| --- | --- | --- | --- |
| SetPosClip | `u32(D+4)` | the start tick | none |
| MotionClip | `u32(D+4)` | `u32(D+0x18)` | slot 18 with 1 and slot 12 with 0 or 2 |
| ShowHideClip | `u32(D+4)` | the start tick | none |
| RapturePhysicsLookAtClip | `u32(D+4)` | the start tick | none |
| ClipSyncClip | `u32(D+4)` | the start tick | slot 12 with 1 at begin and 0 when its clips are done |
| LocalCameraClip | `u32(D+4)` | `u32(D+0x10)` plus clip +0x110 | none |
| RaptureNqMotionBlendOnOffClip | `u32(D+4)` | the start tick | none |
| RaptureFacialPoseForSchClip | `u32(D+4)` | the start tick | none |
| RaptureFacialLipSyncClip | `u32(D+4)` | the start tick | none |
| RaptureMesClip | `u32(D+4)` | the start tick | slot 12 with 1 or 2 on the type 0xa route |
| DepthOfFieldClip | `u32(D+4)` | `u32(D+0x10)` | none |
| KillClip | `u32(D+4)` | the start tick | slot 18 with 1 on the clips it lists |
| RaptureNqIdleSetClip | `u32(D+4)` | the start tick | none |
| RaptureAutoMoveClip | `u32(D+4)` | the start tick | slot 12 with 2 while the move runs and 0 when it ends or is aborted |
| RaptureBlackFadeClip | `u32(D+4)` | the start tick | slot 12 with 2 at begin and 0 after the fade frames |
| RaptureSoundClip | `u32(D+4)` | the start tick | slot 12 with 0 for a slot 0 clip, 2 (1 with the preload bit) for a nonzero slot, 0 when aborted |
| ActionClip | `u32(D+4)` | `u32(D+0x18)` | slot 12 with 0 at the loop limit |
| RaptureScaleClip | `u32(D+4)` | the start tick | none |
| RaptureTurnClip | `u32(D+4)` | the start tick | slot 12 with 2 while the turn runs, slot 18 with 1 and slot 12 with 0 after 90 updates |
| RaptureBgmClip | `u32(D+4)` | the start tick | none |

Clip +0x110 of LocalCameraClip starts at 0. The begin method stores the 64 bit
word S+0xa0 at clip +0x108. When bit 2 of `u8(D+0x1a)` is set behind the key
entry gate of the
[first page](cut-scheduler-clip-payloads.md#localcameraclip), the update method
stores the change of S+0xa0 since the begin at +0x110. Slot 8 adds the low dword.

OBSERVED: every start tick is a multiple of 10000, so at 300000 ticks per second
a start falls on a 1/30 second boundary. The MotionClip end tick is above the
start in 21,988 records, equal in 38 and below in 59, so a negative duration
occurs. Of the 22,085 end ticks 19,529 are multiples of 10000. The LocalCameraClip
end field is never below the start, and 11 of its records set bit 2 of
`u8(D+0x1a)`. The DepthOfFieldClip end equals the start in its 5,780 parameter
records and is above it in its 465 preset records.

## Scheduler time

The routine `0x00a1e540` advances a scheduler by one frame step. It calls the
wrapper `0x00a1e380` and then the clock advance `0x00a204d0` (call sites
`0x00a1e5dd` and `0x00a1e5e4` when S+0x1cc is nonzero, and `0x00a1e68f` and
`0x00a1e69a` otherwise). How often it repeats within one frame step was not
traced.

The wrapper first runs the pass over the running clips, `0x00a1e190`. When the
held clip count at S+0x4c is not above 0 it then computes the scheduler time t as
the low 32 bits of the clock time minus the dword at S+0xa0 minus the dword at
S+0x98 (`0x00a1e405` to `0x00a1e412`) and calls the tick `0x00a1d330` with t at
`0x00a1e419`. While the held count is above 0 it adds the step to S+0xa0
instead (`0x00a1e3a1`, `0x00a1e4d2`), so t does not advance.

Before it computes t the wrapper calls scheduler slot 14. When that returns 1 and
bit 1 of the byte at S+0x91 is set, it clears the bit and adds one step, the
truncation of ticks per frame times the clock rate, to the 64 bit word at S+0xa0
(`0x00a1e3ab` to `0x00a1e3f8`), which holds t back by one step. After the tick it
walks the list at S+0x3c (`0x00a1e41e` to `0x00a1e4bd`). A clip with its abort
bit set whose slot 40 name is `ActionClip` gets slot 3 and then slot 4 with a
context that holds S and is removed from that list.

## Begin

The tick scans the list at S+0x30 from the cursor at S+0x94. The list holds the
clips of one block, as described under
[the clip index word](#clip-index-word). For each clip it applies these steps.

1. A clip is skipped when bit 1 of the byte at S+0x90 is clear and slot 30
   returns true (`0x00a1d44f` to `0x00a1d464`). Slot 30 is `0x00b73290`, which
   loads 1 and returns, in the primary vtable (vtable +0x78) of nineteen of the
   twenty classes. ActionClip has `0x005c5c80`, which returns false, so an
   ActionClip is never skipped by this test. For the other nineteen a clip
   begins only while bit 1 of S+0x90 is set. It is also
   skipped when the test `0x00a26b00` on `s16(D+8)`, read through slot 22 and checked against the
   object at S+0x58, is false (`0x00a1d46a` to `0x00a1d47f`).
2. When t is below the start tick of the clip, slot 7, the scan stops and the
   cursor stays at that clip (`0x00a1d485` to `0x00a1d49e`).
3. Only a clip whose state is 0 or 4 can begin (`0x00a1d5c8` to `0x00a1d5e9`).
   While S+0xc8 is nonzero, a clip with bit 2 of `u8(D+0x0b)` set is moved to
   state 4 instead (`0x00a1d5ed` to `0x00a1d610`).
4. A per-clip begun flag in the array behind S+0xbc blocks a second begin when it
   is 1 and bit 0 of `u8(D+0x0b)` is set. Otherwise the tick sets the flag
   (`0x00a1d657` to `0x00a1d67c` and `0x00a1d6bf` to `0x00a1d6d0`).
5. The tick builds a context that carries t minus the start tick and calls slot 1
   with it (`0x00a1d6d0` to `0x00a1d6fa`). The base begin `0x00a20f30` stores
   that overshoot as the elapsed ticks at clip +0x28, sets the state to 1 and the
   phase word to 0. Every class here calls it first.
6. The clip joins a local list. When its phase word is 1 after slot 1, the scan
   stops with the cursor after the clip (`0x00a1d709` to `0x00a1d71c`).

The scan ends when the list is exhausted, when step 2 stops it, or after step 6.

## Update

After the scan the tick calls slot 2 with the context on each clip of the local
list (`0x00a1d7b3` to `0x00a1d7d2`) and then runs the end test below. The base
update `0x00a20f90` clears clip +0x2c and moves state 1 to state 2 without
stepping. In any other state it stores the truncation of ticks per frame times
the clock rate at clip +0x2c and adds that to the elapsed ticks at clip +0x28,
unless the sum would reach 0x7fffffff. The first update of a clip therefore
leaves the elapsed ticks at the overshoot. The running pass `0x00a1e190`
applies the same update and end test to the clips in the list at S+0x3c, and the
wrapper runs it before the tick. A clip that does not end and does not have phase
1 joins that list (`0x00a1d8d8` to `0x00a1d95c`).

## End

The tick and the running pass end a clip when both of these hold, or when its
abort bit is set (`0x00a1d7d4` to `0x00a1d81d`, `0x00a1e1e5` to `0x00a1e232`).

- The elapsed ticks from slot 9 are not below the end tick minus the start tick.
  The subtraction and the comparison are signed 32 bit (`0x00a1d7e8` to
  `0x00a1d7f5`), as is the comparison of t with the start tick (`0x00a1d498`).
  A record whose end is below its start has a negative duration and passes this
  test on its first end check.
- The state is 2 and the phase word is 0.

The end calls slot 3 and then slot 4 with the context (`0x00a1d81f` to
`0x00a1d83b`). The base methods store state 3 and state 4. MotionClip,
LocalCameraClip, DepthOfFieldClip, RaptureAutoMoveClip (slot 3 is a jump to the
base method) and ActionClip override both, and RaptureMesClip and RaptureSoundClip
override slot 3. KillClip ends a listed clip by setting its abort bit.

## Phase word

The phase word is clip +0x20. Phase 0 lets the duration end the clip, and any
other phase stops that. A clip that did not end and has phase 1 is recorded
through scheduler slot 15 (`0x00a1d9b0`) in the held list at S+0x44, whose head
is S+0x48 and whose count is S+0x4c. The tick also adds t minus the start tick to
the dword at S+0xa0 (`0x00a1d894` to `0x00a1d8d1`), so the time derived from the
same clock is the start tick again. While the held count is above 0 the wrapper
runs no tick, and t stays there. The running pass updates each held clip and ends
it, with slot 3 and slot 4, when its state is 2 and its phase word is 0. That test
has no duration check (`0x00a1e2ac` to `0x00a1e344`). When a clip that held phase
1 ends, the tick scans again from the cursor with the same t (`0x00a1d881` to
`0x00a1d88f` and the loop test at `0x00a1d96c`).

ClipSyncClip, MotionClip and RaptureMesClip call slot 12, and so do
RaptureAutoMoveClip, RaptureBlackFadeClip, RaptureSoundClip, ActionClip and
RaptureTurnClip. Phase 2 of MotionClip and RaptureMesClip keeps a clip from
ending by duration without holding the scheduler time. Phase 2 of the five newer
classes does the same. RaptureSoundClip can also take phase 1, which holds the
scheduler time like the RaptureMesClip phase 1.

## Clips that count frames or updates

RaptureBlackFadeClip stores the scheduler frame number (`0x00a1b860`) at clip
+0x38 at begin. Slot 2 ends the clip when the frame number reaches that value
plus `s32(D+0x18)`, which is 30 for a negative field. Its duration is therefore
counted in frames of the scheduler clock and not in ticks of the record. The
fade setter receives the same length as ticks at a fixed 10000 per frame, the
product of the frames and 300000 over 30, which does not follow the ticks per
frame setter. RaptureTurnClip counts calls to slot 2 and ends after 90 of them
at the latest. Both are STATIC and described on the
[third payloads page](cut-scheduler-clip-payloads-ranks-14-22.md#raptureblackfadeclip).

RaptureAutoMoveClip holds phase 2 while the move it handed to the actor runs.
RaptureSoundClip slot 2 sets phase 0 when the start helper returns false, which
is the case for every slot 0 clip (1,916 of the 2,763 install records) and means
the clip is one-shot, and after an abort or the end of a preload. A clip with a
nonzero sound slot (847 records) holds phase 2 after the start, so what ends
such a clip is UNKNOWN. ActionClip slot 8 returns `u32(D+0x18)`, which is below
the start tick in 637 of its 816 looping records in the install and so gives a
negative duration, as for MotionClip.

## Clip index word

The shared header word `s16(D+0x0c)`, read through slot 20, is the index of the
clip in the list of all clips of the scheduler. The builder `0x00a28510`, called
from `0x00a1c7f8`, creates that list over every block in record order. The list
builder `0x00a28210`, called from `0x00a1ba90`, looks up each clip record of a
block at its index word in that list (`0x00a28287` to `0x00a28295`) and appends
the clip to the list at S+0x30. The clip lookup of ClipSyncClip and KillClip,
`0x00a1b790`, indexes the same list.

OBSERVED: in all 222,491 records the index word equals the position of the clip
in the clip list that the parser builds. Within a block the start ticks do not
decrease, over 213,369 consecutive pairs. They can decrease at a block boundary,
so a block starts its own timeline.

## ActionClip limit writers

The [child scheduler construction path](scb-clip-factory-composition.md#actionclip-child-scheduler-delivery)
also qualifies several writers of the limit at ActionClip L+0x44. Constructor
`0x00a179c0` clears it at `0x00a179ff`; slot 1 clears it again at
`0x00a1819a`. In slot 49 `0x00a17a30`, D is the fourth argument,
loaded at `0x00a17b64`, and the selected branches write:

| Condition | Limit stored at L+0x44 | Store VA |
|---|---|---|
| `u8(D+0x14)&0x40` nonzero | 1 | `0x00a17be4` |
| Otherwise, `s8(D+0x15)<0` | -1 | `0x00a17bf5` |
| Otherwise | `s8(D+0x15)+s16(AX)` returned by `0x00a012e0` | `0x00a17c17` |

The call to `0x00a012e0` at `0x00a17c06` pushes zero last and a value
whose low word is `s8(D+0x16)+1` first. That helper's implementation and
return semantics remain unqualified; no random or range interpretation follows.
These are authenticated writers, rather than an exhaustive writer census or
selected values. Slot 2 compares L+0x58 with the limit, when it differs from -1, at
`0x00a185e0..0x00a185f3` and calls `0x00a210f0(L,0)` on equality,
under the phase-store contract above.

## Open items

- The block that a selected scheduler plays, the block header fields and the
  scheduler length are UNKNOWN. The
  [secondary-collection caller relation](scb-clip-factory-composition.md#secondary-collection-callers-and-callback-context)
  qualifies the `0x00a1cb20` and `0x00a1e710` calls to `0x00a1ba90` and
  their receiver/argument flow. It does not establish delivery of a constructed
  combat scheduler to that updater, selected block values or block order.
  The [ActionClip child path](scb-clip-factory-composition.md#actionclip-child-scheduler-delivery)
  qualifies a separately constructed AccScheduler's conditional delivery;
  it does not identify the root combat instance.
  The [queue base-Scheduler item loop](action-queue-scheduler-handoff.md#retained-item-and-scheduler-update)
  qualifies another same-instance delivery under retained J/list/state conditions;
  its actual framework-slot entry and selected callback remain unproved.
- The frame rate source of the ticks per frame setter is UNKNOWN.
- The use of S+0x1cc, the repetition of the wrapper within a frame step and the
  seek branches of the tick were not traced.
- The names tick and phase word are INFERRED from the arithmetic and the base
  accessors. The manifest keeps the name phase for the word at clip +0x20.
- The code that sets bit 1 of the byte at S+0x90, which gates every documented
  clip, and bit 1 of the byte at S+0x91 is UNKNOWN.
- The code that returns a RaptureMesClip phase word from 1 to 0 or sets its abort
  bit is UNKNOWN, possibly the message reply path. Until it runs, phase 1 holds
  the scheduler time under the rules above.
- The effect of bit 1 of `u8(D+0x0b)` and of `u8(D+0x0a)` is UNKNOWN.
- The selected ActionClip limit and the return semantics of its
  `0x00a012e0` input remain UNKNOWN; the checked writers are described above.

## Verification

`python tools/verify_scb_clip_classes.py` checks the manifest, including its
`timing` section, the census table and the mutation cases. `--exe <ffxivgame.exe>`
compares the byte checks with the pinned image. `--install <game-root>` checks the
timing invariants listed above on every record.
`python tools/test_scb_clip_classes.py` covers them with synthetic schedulers.
