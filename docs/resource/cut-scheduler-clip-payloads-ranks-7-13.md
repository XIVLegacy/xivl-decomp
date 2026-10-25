# Cut scheduler clip payloads, ranks 7 to 13

This page records how the retail 1.23b client reads six more clip classes of an
embedded `SEDBSCB` scheduler: RaptureNqMotionBlendOnOffClip,
RaptureFacialPoseForSchClip, RaptureFacialLipSyncClip, RaptureMesClip,
DepthOfFieldClip and KillClip. They are ranks 7, 8, 10, 11, 12 and 13 of the
[clip class census](cut-scheduler-clip-class-census.md). Rank 9 is
BindActorClip, which the [cut scheduler clip layout](cut-scheduler-clip-layout.md#actor-binding)
covers. The first six classes are on the
[clip payloads page](cut-scheduler-clip-payloads.md). The start and end ticks of
every documented class are on the
[clip timing page](cut-scheduler-clip-timing.md). D is the serialized clip
record, all offsets are relative to D unless a row says otherwise, and all
values are little-endian.

## Evidence

The evidence identity, tools and tier labels are those of the
[first payloads page](cut-scheduler-clip-payloads.md#evidence). The manifest
[scb_clip_classes.json](../../config/scb_clip_classes.json) holds every
instruction sequence, table and string below as a byte check, and
`tools/verify_scb_clip_classes.py --exe <ffxivgame.exe>` compares them with the
pinned image. The `--install` option checks the OBSERVED invariants against the
`client/cut` tree described in the census.

## Registration

The insert calls use the same registry and the same call `0x0063f710` as the
[first six classes](cut-scheduler-clip-payloads.md#registration).

| Class | Group | Key literal | Factory | Insert site | Second insert |
| --- | --- | --- | --- | --- | --- |
| RaptureNqMotionBlendOnOffClip | Rapture | `0x00fbcae0` | `0x00639ab0` | `0x00635f5b` | none |
| RaptureFacialPoseForSchClip | Rapture | `0x00fbcf68` | `0x0063b370` | `0x00636747` | none |
| RaptureFacialLipSyncClip | Rapture | `0x00fbcf0c` | `0x0063b1c0` | `0x006366bd` | key `0x00fbd3dc` at `0x0063715b` |
| RaptureMesClip | Rapture | `0x00fbc81c` | `0x00638a60` | `0x00635a1c` | none |
| DepthOfFieldClip | base | `0x0109b580` | `0x00a4ead0` | `0x00a4c8c8` | none |
| KillClip | base | `0x0109b6d4` | `0x00a4e210` | `0x00a4c634` | key `0x0109b6d4` at `0x00a4d08d` |

The four Rapture classes are inserted by manager slot 3 at `0x00635760` after
its call to the base registrar `0x00a4c2a0`. None of them has a key in the base
group and none is preceded by a call to the erase method `0x0063f920`.
DepthOfFieldClip and KillClip are inserted by the base registrar. Each class
therefore has one factory in the registry reached through `0x00a02360`.

The second inserts of RaptureFacialLipSyncClip and KillClip use the same
factory. The registry operand of each is the result of a call to `0x00a65600`
plus 8, made directly before the insert. That registry is the second one named
on the first page. Its owner and its readers were not followed.

## Objects and entry points

A factory stores its object size when its size argument is nonnull and
otherwise allocates that many bytes, as described for the
[Lay factory](showhide-clip-dispatch.md#registry-and-construction). Each
constructor chain ends in the shared base constructor `0x00a21260` and then
installs the primary vtable at +0 and a secondary table at +8.

| Class | Object bytes | Constructor | Primary vtable | Secondary table | RTTI |
| --- | ---: | --- | --- | --- | --- |
| RaptureNqMotionBlendOnOffClip | `0x38` | `0x0081ab00` | `0x0100a83c` | `0x0100a830` | RaptureNqMotionBlendOnOffClip |
| RaptureFacialPoseForSchClip | `0x38` | `0x00827080` | `0x010248dc` | `0x010248d0` | RaptureFacialPoseForSchClip |
| RaptureFacialLipSyncClip | `0x38` | `0x008269f0` | `0x010229d4` | `0x010229c8` | RaptureFacialLipSyncClip |
| RaptureMesClip | `0x58` | `0x00813c70` | `0x00ffb0a4` | `0x00ffb098` | RaptureMesClip |
| DepthOfFieldClip | `0x60` | `0x00df0290` | `0x011370a4` | `0x01137098` | Plugins DepthOfFieldClip |
| KillClip | `0x38` | `0x00dec780` | `0x01135744` | `0x01135738` | Plugins KillClip |

| Class | Slot 1 | Slot 2 | Slot 3 | Slot 4 | Slot 8 | Slot 29 |
| --- | --- | --- | --- | --- | --- | --- |
| RaptureNqMotionBlendOnOffClip | `0x0081abc0` | `0x00a20f90` | base | base | base | `0x00defc20` |
| RaptureFacialPoseForSchClip | `0x00827140` | `0x00a20f90` | base | base | base | `0x00830580` |
| RaptureFacialLipSyncClip | `0x00826ab0` | `0x00a20f90` | base | base | base | `0x00defc20` |
| RaptureMesClip | `0x00813cf0` | `0x00a20f90` | `0x00813bb0` | base | base | `0x00823ce0` |
| DepthOfFieldClip | `0x00df0350` | `0x00df02e0` | `0x00df0050` | `0x00df00d0` | `0x00df0040` | `0x00df0c80` |
| KillClip | `0x00dec7f0` | `0x00a20f90` | base | base | base | `0x00dec7d0` |

The base methods are those of the
[first page](cut-scheduler-clip-payloads.md#objects-and-entry-points): slot 2
`0x00a20f90`, slot 3 `0x00a21000`, slot 4 `0x00a21010` and slot 8 `0x00a21080`.
Slot 29 is the record end for the first three classes. It is not the record end
for RaptureMesClip. It is the record end of only some DepthOfFieldClip and
KillClip records, as listed below.

The slot 1 method of every class here calls the base method `0x00a20f30`
first. The header words and the class key entry are described on the first page
and the clip index word on the
[timing page](cut-scheduler-clip-timing.md#clip-index-word).

OBSERVED: the @CCPT entry byte +2, which the class key entry carries at +4, has
one value per class.

| Class | Entry byte +2 |
| --- | ---: |
| RaptureNqMotionBlendOnOffClip | 1 |
| KillClip | 1 |
| RaptureFacialPoseForSchClip | 0 |
| RaptureFacialLipSyncClip | 0 |
| RaptureMesClip | 3 |
| DepthOfFieldClip | 3 |

Only RaptureMesClip reads the key entry byte among these classes. Its slot 1
branches on the values 2 and 3, and the install holds only 3.

## RaptureNqMotionBlendOnOffClip

Slot 1 resolves the actor record with `u8(D+3)`, requires its slot 7 to be true
and casts the slot 23 result from IActor to CharaActor. It calls CharaActor
method `0x0065d260` with the boolean `u32(D+0x10) != 0` and `u32(D+0x14)`. That
method stores the value at CharaActor +0x134c, or -1 when the boolean is false,
and passes the stored value to `0x007ad020` with ECX at actor +0x4c0.
`0x007ad020` visits a list under that object, converts two entry times from
ticks to frames by multiplying by 30.0 and dividing by 300000.0, and compares
the frame difference with the value. Slot 29 returns D+0x18, the record end.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 4 | `u32` switch | `0x0081ac11` |
| `0x14` | 4 | `u32` value | `0x0081ac15` |

INFERRED: the switch turns a blend on or off and the value is a length in motion
frames. The role of the two entry times inside `0x007ad020` was not traced.

OBSERVED: all 11,142 records are 0x18 bytes. The switch is 1 in 6,555 and 0 in
4,587 records. The value is at most 500 and is 15 in 10,335 records.

## RaptureFacialPoseForSchClip

Slot 1 resolves the actor record, casts its slot 23 result to CharaActor and
calls `0x0065e8d0` with the address D+0x10, `s32(D+0x30)` and `s32(D+0x34)`.
After the CharaActor slot 154 guard that method calls `0x007a6210` with ECX at
actor +0x1bc and a fourth argument of -1. `0x007a6210` compares the first four
bytes of the name with `off` (`0x00fe711c`). A match copies `cfxf_default`
(`0x00fe6f00`) instead of the name. For any other name than `cfxf_default` it
requires the lookups `0x00662720` with the tags `bcm` and `btm` to return
nonzero. It copies at most 15 characters into a 16 byte buffer at +0x190 of the
component, stores the low 16 bits of the first number with bit 16 set at +0x17c
and stores the numbers as floats at +0x174 and +0x178. Slot 29 returns D+0x38,
the record end.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 32 | name, NUL terminated ASCII | `0x008271a3` |
| `0x30` | 4 | `s32` | `0x0082719f` |
| `0x34` | 4 | `s32` | `0x0082719b` |

For a name other than `off` the first number is stored at +0x17c, the second as
a float at +0x174 and the constant -1 as a float at +0x178. The meaning of the
numbers was not traced.

INFERRED: the name is a facial pose resource name.

OBSERVED: all 9,081 records are 0x38 bytes. Every name starts with `cfxf_`, has
at most 15 characters and is followed by zero bytes. The first number is within
0 to 9 and the second within 0 to 1200.

## RaptureFacialLipSyncClip

Slot 1 resolves the actor record, casts its slot 23 result to CharaActor and
calls `0x0065e980` with `u32(D+0x10)` and `u32(D+0x14)`. After the CharaActor
slot 154 guard that method calls `0x007a6fc0` with ECX at actor +0x35c.
While the float at +0x1c of that component is positive, `0x007a6fc0` keeps only
the first-word values 2 and 3, stored at +0x28 and +0x2c. Otherwise it stores
both words, sets byte +0x30, picks the name at index `(first == 1) + 2 * second`
of the table below and resolves it through `0x00662720` with the tags `bcm` and
`btm` into +0x10 and +0x14 of the component. Slot 29 returns D+0x18, the record
end.

| Index | Name at `0x00fe7200 + 0x10 * index` |
| ---: | --- |
| 0 | `cfxl_lip_nor1` |
| 1 | `cfxl_lip_nor2` |
| 2 | `cfxl_lip_wis1` |
| 3 | `cfxl_lip_wis2` |
| 4 | `cfxl_lip_sho1` |
| 5 | `cfxl_lip_sho2` |

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 4 | `u32` mode | `0x00826b0f` |
| `0x14` | 4 | `u32` set | `0x00826b0b` |

INFERRED: the second word selects a lip set and the first word a variant. The
effect of the first-word values 4 to 7 was not traced.

OBSERVED: all 7,441 records are 0x18 bytes. The mode is within 0 to 7 and is 0
in 5,797 records. The set is 0 in 6,982, 1 in 233 and 2 in 226 records.

## RaptureMesClip

Slot 1 needs a target found through `0x00a20030` and `0x0060b360`. It then reads
the key entry byte +4 (`0x00813d87`). The value 2 copies `u8(D+0x1c)`,
`u8(D+0x1d)`, `u32(D+0x18)` and `u32(D+0x14)`. The value 3 does the same and
then walks `u32(D+0x20)`. Bits 1 to 7 select entries of the object at scheduler
+0xf0 through its slot 17, and bits 20 to 24 select entries through its slot 29.
Any other value returns after the base begin.

A zero `u8(D+0x1c)` calls `0x00813f60`, which posts message type 0xb of 0x14
bytes through `0x007ce7b0`. A nonzero value calls `0x00813f10`, which posts type
0xa of 0x1c bytes only when its first argument is nonnull. Slot 1 then calls slot
12 itself, with 1 when `u8(D+0x1d)` is nonzero and with 2 otherwise, whether or
not anything was posted. Slot 3 at `0x00813bb0` calls `0x00a1c620` when the key
entry byte is 2 or 3 and `u8(D+0x1e)` is nonzero. That function sets scheduler +0xc8
to 2 when it is 0. Slot 29 is the method `0x00823ce0`, shared with
ShowHideClip, and returns D+0x14, which is not the record end. The record is
0x24 bytes.

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x14` | 4 | `u32` message word 0 | `0x00813db9` | Copied into the message block. |
| `0x18` | 4 | `u32` message word 1 | `0x00813db6` | Copied into the message block. |
| `0x1c` | 1 | `u8` route byte | `0x00813db2` | Selects the post method. |
| `0x1d` | 1 | `u8` phase byte | `0x00813dc0` | Selects the phase 1 or 2 after the type 0xa post. |
| `0x1e` | 1 | `u8` | `0x00813bc1` | Read by slot 3. |
| `0x20` | 4 | `u32` entry mask | `0x00813df0` | Read when the key entry byte is 3. |

The words at +0x10 and +0x1f are not read. The phase word and its effect on the
scheduler time are on the [timing page](cut-scheduler-clip-timing.md#phase-word).

The receiver of the type 0xa and 0xb messages, the meaning of the two message
words, the objects the mask selects and the use of scheduler +0xc8 beyond the
[tick test](cut-scheduler-clip-timing.md#begin) are UNKNOWN.

OBSERVED: all 6,301 records are 0x24 bytes with the key entry byte 3, so the key
2 branch has no install data. The word at +0x10 is 1. The bytes +0x1c, +0x1d
and +0x1e are 0 or 1 and +0x1f is 0. The mask only sets bits 1 to 7 and 20 to
24. The route and phase bytes are 1 and 1 in 1,650, 1 and 0 in 4,081, 0 and 0 in
553 and 0 and 1 in 17 records.

## DepthOfFieldClip

Slot 1 fetches the target object from the object at clip +0x24 and copies a 32
byte block from its slot 12. It moves bits 0, 1 and 4 to 10 of `u32(D+0x14)`
into bits 0 to 8 of clip +0x38 (`0x00df03d7` to `0x00df0482`).

| Bit of `u32(D+0x14)` | Bit of clip +0x38 | Use |
| ---: | ---: | --- |
| 0 | 0 | Enable. It is the first byte of the block. |
| 1 | 1 | Selects the preset branch. |
| 2, 3 | none | Not read. |
| 4 to 10 | 2 to 8 | The float at `+0x18 + 4 * (bit - 4)` is present. |
| 11 to 31 | none | Not read. |

With bit 1 set, slot 1 passes `u16(D+0x18)` to scheduler lookup `0x00a20090`
(`0x00df0494`), builds a preset from the record that lookup returns and stores
its values at clip +0x3c to +0x5c. With bit 1 clear, each present bit replaces
one of the seven floats of the block with the float at +0x18 to +0x30
(`0x00df0721` to `0x00df0789`). Slot 1 ends by calling slot 13 of the target
object with the clip and the block, whose first byte is bit 0 of clip +0x38.

When bits 0 and 1 are both set, slot 2 at `0x00df02e0` passes the elapsed ticks
divided by the ticks per frame to `0x00df0120`, and slot 3 at `0x00df0050` sends
the fetched block back with its first byte 0. Slot 4 at `0x00df00d0` releases
the objects at clip +0x3c and +0x5c. Slot 8 returns `u32(D+0x10)`. Slot 29
returns D+0x34, which is the end of a 0x34 byte record and lies 0x18 bytes past
the end of a 0x1c byte record.

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x10` | 4 | `u32` end tick | `0x00df0043` | Returned by slot 8. |
| `0x14` | 4 | `u32` flag word | `0x00df03d7` | Routed as above. |
| `0x18` | 2 | `u16` preset index | `0x00df0494` | Bit 1 set. |
| `0x18`, `0x1c`, `0x20`, `0x24`, `0x28`, `0x2c`, `0x30` | 4 each | `f32` parameter 0 to 6 | `0x00df0721` | Bit 1 clear and bit 4 to 10 of the flag word set. |

INFERRED: the seven floats are depth of field parameters and a preset is a
stored set of them that the clip blends over its duration. The names rest on the
class name and the argument flow. The values the preset branch stores and the
object that slot 13 of the target belongs to were not traced.

OBSERVED: the 5,780 records with bit 1 clear are 0x34 bytes and have an end tick
equal to the start tick. The 465 records with bit 1 set are 0x1c bytes, hold
`u16(D+0x1a)` as 0 and have an end tick above the start tick. The flag word only
sets bits 0, 1 and 4 to 10. It is 0x7f1 in 5,464, 0x7f3 in 462, 0x7f0 in 260,
0x5f1 in 44 and 0x5f0 in 11 records.

## KillClip

Slot 1 reads the count `n = s16(D+0x10)` (`0x00dec809`). For each of n `s16`
indexes from D+0x14 it calls the scheduler clip lookup `0x00a1ffb0`. A clip that
is found gets its slot 18 called with 1, which sets its abort bit. A missing
clip logs an error that carries its position k in the list, not the clip index. A count of zero or less runs no
iteration. Slot 29 returns `D + ((2n + 0x19) & ~3)`. That equals the record end
`(2n + 0x17) & ~3` for odd n and lies 4 bytes past it for even n.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 2 | `s16` count n | `0x00dec809` |
| `0x14 + 2k` | 2 | `s16` clip index | `0x00dec820` |

The abort bit ends the listed clip at the next end test, as described under
[the end rule](cut-scheduler-clip-timing.md#end).

INFERRED: the clip ends the clips it lists. The lookup returns null outside the
clip list, so an index of -1 logs the error.

OBSERVED: the record is `(0x14 + 2n + 3) & ~3` bytes in all 4,995 records, from
20 to 60 bytes. The count is within 0 to 19, `s16(D+0x12)` is 0 and every byte
after the last index is 0. A listed index of -1 occurs in 229 records. Every other
listed index lies inside the clip list of the scheduler, and none names the clip
itself. The count is 1 in 4,078 records, 2 in 402 and 0 in 90.

## Open items

- The code that returns a RaptureMesClip phase word to 0 or sets its abort bit is
  UNKNOWN, possibly the message reply path.
- The receiver of the RaptureMesClip messages and the meaning of its words and
  mask are UNKNOWN.
- The objects behind the DepthOfFieldClip preset branch and behind slot 13 of
  its target are UNKNOWN.
- The meaning of the second number of RaptureFacialPoseForSchClip and of the
  RaptureFacialLipSyncClip mode values 4 to 7 is UNKNOWN.
- The registry that receives the second inserts of RaptureFacialLipSyncClip and
  KillClip was not walked.
- The names blend, facial pose, lip set, depth of field and preset are INFERRED
  and rest on the install value ranges and on argument flow.

## Verification

`python tools/verify_scb_clip_classes.py` checks the manifest, the census table
and the mutation cases. `--exe <ffxivgame.exe>` compares the byte checks with
the pinned image. `--install <game-root>` recomputes the census and checks every
record of the twenty classes against the OBSERVED invariants in the manifest.
`python tools/test_scb_clip_classes.py` covers the parser, the invariants and the
verifier with synthetic schedulers.
