# Cut scheduler clip payloads, ranks 14 to 22

This page records how the retail 1.23b client reads eight more clip classes of
an embedded `SEDBSCB` scheduler: RaptureNqIdleSetClip, RaptureAutoMoveClip,
RaptureBlackFadeClip, RaptureSoundClip, ActionClip, RaptureScaleClip,
RaptureTurnClip and RaptureBgmClip. They are ranks 14 to 17 and 19 to 22 of the
[clip class census](cut-scheduler-clip-class-census.md). Rank 18 is
RaptureMapCollisionOnOffClip, which the
[cut scheduler clip layout](cut-scheduler-clip-layout.md#clip-records) covers.
The first twelve classes are on the
[first payloads page](cut-scheduler-clip-payloads.md) and the
[second payloads page](cut-scheduler-clip-payloads-ranks-7-13.md). The start and
end ticks of every documented class are on the
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
`client/cut` tree described in the census. Ghidra decompiler output served only
as a lead for the call shapes and every offset below was confirmed in the
disassembly.

## Registration

The insert calls use the same registry and the same call `0x0063f710` as the
[first six classes](cut-scheduler-clip-payloads.md#registration).

| Class | Group | Key literal | Factory | Insert site | Second insert |
| --- | --- | --- | --- | --- | --- |
| RaptureNqIdleSetClip | Rapture | `0x00fbcac8` | `0x00639a20` | `0x00635f31` | none |
| RaptureAutoMoveClip | Rapture | `0x00fbc7a8` | `0x00638790` | `0x00635943` | none |
| RaptureBlackFadeClip | Rapture | `0x00fbc9f8` | `0x006395a0` | `0x00635dc1` | none |
| RaptureSoundClip | Rapture | `0x00fbcb00` | `0x00639b40` | `0x00635f8d` | key `0x00fbd4fc` at `0x006372e1` |
| ActionClip | base and Rapture | `0x00fbd240` | `0x0063c210` | `0x00636d1c` | see below |
| RaptureScaleClip | Rapture | `0x00fbcdfc` | `0x0063ac20` | `0x006364f1` | none |
| RaptureTurnClip | Rapture | `0x00fbc77c` | `0x00638670` | `0x006358e7` | none |
| RaptureBgmClip | Rapture | `0x00fbcb5c` | `0x00639d80` | `0x00636045` | none |

The insert site is the store of the key literal to the stack. The store of the
factory and the call to `0x0063f710` follow it. Seven classes are inserted by
manager slot 3 at `0x00635760` with no key in the base group and no call to the
erase method `0x0063f920` before them. Each has one factory in the registry
reached through `0x00a02360`.

RaptureSoundClip is inserted a second time at `0x006372e1` with the same
factory. The registry operand of that insert is the result of a call to
`0x00a65600` plus 8, the second registry named on the first page. Its owner and
readers were not followed.

ActionClip is registered like SetPosClip. The base registrar `0x00a4c2a0`
inserts the key literal `0x01097a30` with the factory `0x00a4dd40` at
`0x00a4c4c9` and inserts it again at `0x00a4cf7c` into the second registry. The
Rapture group stores a second copy of the key literal (`0x00fbd234`) and calls
`0x0063f920` with it at `0x00636cfe`, then inserts `0x00fbd240` with the factory
`0x0063c210`. INFERRED: the registry then holds the Rapture factory, on the
same reading of the erase call as for the other overridden classes.

## Objects and entry points

A factory stores its object size when its size argument is nonnull and
otherwise allocates that many bytes, as described for the
[Lay factory](showhide-clip-dispatch.md#registry-and-construction). Each
constructor chain ends in the shared base constructor `0x00a21260` and then
installs the primary vtable at +0 and a secondary table at +8.

| Class | Object bytes | Constructor | Primary vtable | Secondary table | RTTI |
| --- | ---: | --- | --- | --- | --- |
| RaptureNqIdleSetClip | `0x38` | `0x0081a8d0` | `0x01009de4` | `0x01009dd8` | RaptureNqIdleSetClip |
| RaptureAutoMoveClip | `0x40` | `0x00811820` | `0x00ff851c` | `0x00ff8510` | RaptureAutoMoveClip |
| RaptureBlackFadeClip | `0x40` | `0x00818f30` | `0x01005ccc` | `0x01005cc0` | RaptureBlackFadeClip |
| RaptureSoundClip | `0x54` | `0x0081ada0` | `0x0100b2a4` | `0x0100b298` | RaptureSoundClip |
| ActionClip | `0x68` | `0x0082fb30` | `0x01033bec` | `0x01033be0` | RaptureActionClip |
| RaptureScaleClip | `0x38` | `0x00825040` | `0x0101c264` | `0x0101c258` | RaptureScaleClip |
| RaptureTurnClip | `0x3c` | `0x008113b0` | `0x00ff7054` | `0x00ff7048` | RaptureTurnClip |
| RaptureBgmClip | `0x38` | `0x0081bef0` | `0x0100d4a4` | `0x0100d498` | RaptureBgmClip |

The ActionClip constructor calls the constructor `0x00a179c0` of the base class
ActionClip, whose vtable is `0x01096f6c` and whose RTTI name is Plugins
ActionClip. That constructor calls `0x00a21260`. The Rapture constructor then
installs its own vtable and also stores `0x01033b54` at clip +0x38.

| Class | Slot 1 | Slot 2 | Slot 3 | Slot 4 | Slot 8 | Slot 29 | Slot 30 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RaptureNqIdleSetClip | `0x0081a990` | `0x00a20f90` | base | base | base | `0x00a172a0` | `0x00b73290` |
| RaptureAutoMoveClip | `0x00811c80` | `0x00811910` | `0x00811810` | `0x00811c30` | base | `0x00813000` | `0x00b73290` |
| RaptureBlackFadeClip | `0x00818ff0` | `0x00818ef0` | base | base | base | `0x00823ce0` | `0x00b73290` |
| RaptureSoundClip | `0x0081aea0` | `0x0081b240` | `0x0081b430` | base | base | `0x00823ce0` | `0x00b73290` |
| ActionClip | `0x0082fc60` | `0x0082fcb0` | `0x00a176e0` | `0x00a17cb0` | `0x00a176d0` | `0x00813000` | `0x005c5c80` |
| RaptureScaleClip | `0x00825100` | `0x00a20f90` | base | base | base | `0x00823ce0` | `0x00b73290` |
| RaptureTurnClip | `0x00811470` | `0x008114f0` | base | base | base | `0x00823ce0` | `0x00b73290` |
| RaptureBgmClip | `0x0081bfb0` | `0x00a20f90` | base | base | base | `0x00823ce0` | `0x00b73290` |

The base methods are those of the
[first page](cut-scheduler-clip-payloads.md#objects-and-entry-points): slot 2
`0x00a20f90`, slot 3 `0x00a21000`, slot 4 `0x00a21010` and slot 8 `0x00a21080`.
Slot 29 `0x00a172a0` returns D+0x30 and `0x00813000` returns D+0x28. Both are the
record end of the classes that use them. `0x00823ce0` returns D+0x14, which is
the record end of RaptureTurnClip and the first class field of the other four.
Slot 3 of RaptureAutoMoveClip is a jump to the base method `0x00a21000`.

Slot 30 of ActionClip is `0x005c5c80`, which clears AL and returns. The other
seven classes keep the method `0x00b73290` that returns true. The
[timing page](cut-scheduler-clip-timing.md#begin) describes the tick test that
reads it.

The slot 1 method of every class here calls the base method `0x00a20f30`
first, except that ActionClip reaches it through `0x00a18170`. The header words
and the class key entry are described on the first page and the clip index word
on the [timing page](cut-scheduler-clip-timing.md#clip-index-word).

## Class key entry

OBSERVED: the @CCPT entry byte +2, which the class key entry carries at +4, has
one value per class except RaptureSoundClip.

| Class | Entry byte +2 |
| --- | ---: |
| RaptureNqIdleSetClip | 0 |
| RaptureAutoMoveClip | 3 |
| RaptureBlackFadeClip | 0 |
| RaptureSoundClip | 2 in 23 records and 3 in 2,740 |
| ActionClip | 1 |
| RaptureScaleClip | 1 |
| RaptureTurnClip | 0 |
| RaptureBgmClip | 0 |

Only RaptureSoundClip and RaptureScaleClip read the key entry byte. Both
branch on it, and the install holds only the values listed.

## RaptureNqIdleSetClip

Slot 1 resolves the actor record with `u8(D+3)`, requires its slot 7 to be true
and casts the slot 23 result from IActor to CharaActor. It calls CharaActor
method `0x0065e3e0` with the address D+0x10 (`0x0081a9eb`, call at
`0x0081a9f0`). After the slot 154 guard that method copies the string into the
16 byte buffer at +4 of the component at actor +0x3ac through `0x007a4fb0`
(`0x0065e405`). The copy is bounded to 16 bytes including the NUL and calls the
invalid parameter handler when no NUL lies inside that bound. The method then
passes the same address to `0x0065c4c0` (`0x0065e40d`). Slot 29 returns D+0x30,
the record end.

`0x0065c4c0` does nothing unless bit 9 of the dword at actor +0x2b70 is set.
It then works on the state object at actor +0x590. With a nonnull address it
calls `0x007b9420` twice with table index 0, the keys `cbnm_id0`
(`0x00fc037c`) and `cbbm_id0` (`0x00fc0388`) and the address as the value, and
then calls `0x007a4130` with 0 (`0x0065c50b`, `0x0065c51a`, `0x0065c523`). The
routine `0x007b9420` calls `0x004528f0`, which looks up the key in the table at
state +0x320 plus 12 times the index and inserts it when it is missing
(INFERRED from its search and insert shape), and assigns the string to the entry
it returns. With a null address `0x0065c4c0` calls `0x007b49a0` with index 0
and the same two keys (`0x0065c4e4`, `0x0065c4f1`), which calls `0x007c8290` on
that table.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 32 | name, NUL terminated ASCII | `0x0081a9eb` |

INFERRED: the string is the name of an idle motion set and the clip substitutes
it for the two idle body motion names. The substitution rests on the keys, which
the [idle bank page](../actor/idle-bank-selection.md#installed-files)
shows the state code passing as `cbnm_id0`. `0x007c8290` is read as an erase by
key from its call shape only.

OBSERVED: all 4,442 records are 0x30 bytes and the key entry byte is 0. Every
name has at most 15 characters, is followed by zero bytes to +0x2f and is one of
12 values. `cbfm_id0` occurs in 4,117 records, `cbfm_id1` to `cbfm_id3` in 254,
`cbbm_id0` in 55, `cbnm_id0` in 5 and six other names in 11.

## RaptureAutoMoveClip

Slot 1 reads `u16(D+0x26)` and passes it to the scheduler lookup `0x00a20090`
(`0x00811ceb`), the lookup that DepthOfFieldClip uses for its preset. It
fetches the target object through slot 9 of the object at clip +0x24, resolves
the actor record with `u8(D+3)`, requires slot 7 to be true and needs a
nonnull slot 23 result and the component at that result plus 0x11b4. The record
that the lookup returned holds a data window. Its address plus 0x20 and its
dword at +0x10 go to `0x00810fa0` (`0x00811d64`), whose result is stored at clip
+0x3c. From that set the method fetches an entry named `TracePoint`
(`0x00ff7ca8`) and from it three curves through `0x00810fe0`, with the names
`tx`, `ty` and `tz` that the pointers at `0x01097e24` select.

The method then builds a parameter block on its stack from the record. Offsets
below are relative to the block.

| Block offset | Source |
| --- | --- |
| +0x04 to +0x0c | The list of 16 byte points, filled below. |
| +0x10 | `u16(D+0x14)` plus 1. |
| +0x14 | `f32(D+0x20)`. |
| +0x18 | `s32(D+0x1c)` when `s32(D+0x18)` is nonzero, otherwise -1. |
| +0x1c | 1 when `u16(D+0x24)` is 0, otherwise 0. |
| +0x1d | 1 when `u16(D+0x16)` is nonzero, otherwise 0. |

With `u16(D+0x24)` equal to 0 all n points come from the curves, each
evaluated at float(i) for i below n = `s32(D+0x10)`. Otherwise point 0 comes
from slot 34 of the actor object and points 1 to n-1 from the curves. When the
scheduler test `0x00a1c050` returns true, each curve point also goes through
slot 18 of the target object with the scheduler origin, as for SetPosClip. The
method appends every point through `0x008125f0`, calls CharaActor method
`0x0065eaf0` with the block (`0x008122ca`) and then slot 12 with 2. The path
builder `0x007ae8b0` that receives the block divides the length of each segment
by the speed divided by 30.0 (the motion frame rate constant `0x00fa7b18`), so
the speed is the float at block +0x14.

Slot 2 first runs the base update. A true slot 19 result (the abort bit) calls
`0x0065eb20` on the CharaActor (`0x00811999`) and slot 12 with 0. While the
scheduler word at S+0xc8 is zero the method calls `0x0065eb60` (`0x00811c0d`)
and calls slot 12 with 2 when it returns true and with 0 otherwise. While S+0xc8
is nonzero it evaluates the three curves at float(n) (`0x00811a27`), cancels the
move through `0x0065eb20` and sets the actor position through `0x0065e660`
(`0x00811bee`). Slot 4 releases the objects at clip +0x38 and +0x3c.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 4 | `s32` point count n | `0x00811e6c` |
| `0x14` | 2 | `u16` mode word | `0x00811e33` |
| `0x16` | 2 | `u16` block flag | `0x00811e4b` |
| `0x18` | 4 | `s32` hold switch | `0x00811e18` |
| `0x1c` | 4 | `s32` hold value | `0x00811e22` |
| `0x20` | 4 | `f32` speed | `0x00811e3e` |
| `0x24` | 2 | `u16` start mode word | `0x00811e59` |
| `0x26` | 2 | `u16` track index | `0x00811ceb` |

INFERRED: the record names a movement path stored as curves in a track, n is
the number of path points, `u16(D+0x24)` selects a path that starts at the
authored curve or at the actor position and the clip ends when the move
ends. The scheduler chunk behind the table of `0x00a20090`, the meaning of the
words at +0x14, +0x16, +0x18 and +0x1c after the handoff and the effect of
`0x0065eaf0` on the actor are UNKNOWN.

OBSERVED: all 3,381 records are 0x28 bytes and the key entry byte is 3. The
count is within 2 to 20 and is 2 in 2,400 records. The mode word is 0 in 2,086,
1 in 1,189 and 2 in 106 records. The block flag is 1 in 154 records. The hold
switch is 1 in 2,061 and 0 in 1,320 records, and the hold value is nonzero in
833 and at most 350. The speed is above 0 and at most 35.0, from 0.3 in the
install. The start mode word is 0 in 1,659 and 1 in 1,722 records. The track
index is within 0 to 159 and takes 158 values.

## RaptureBlackFadeClip

Slot 1 calls the base begin and obtains the owner through `0x00a20030` and
`0x0060b360`. It needs the object at +0x6c of the result and an address 0x138
past it that is nonzero (`0x00819041` to `0x00819054`). On the first call it
writes two static float4 values: entry 0 of the pointer table at `0x012ca3e8` is
(0, 0, 0, 1.0) at `0x0134e22c` and entry 1 is (0, 0, 0, 0) at `0x0134e21c`.

The method reads `s32(D+0x18)` and replaces a negative value by 30. It computes
ticks as the value times 300000 over 30 and passes the object at +0x6c, the
pointer selected by `u32(D+0x14)` and the address of the tick count to
`0x00612400` (`0x00819124`). That setter copies the float4 to +0x16c of the
object, the ticks to +0x17c and sets bit 1 and clears bit 0 of the dword at
+0x168. The method then stores the scheduler frame number at clip +0x38 and that
number plus the frames at clip +0x3c (`0x0081913b`, `0x00819140`) and calls slot
12 with 2. The frame number is the clock time divided by the ticks per frame,
through `0x00a1b860` and `0x00a203e0`, as recorded on the
[timing page](cut-scheduler-clip-timing.md#time-unit). Slot 2 runs the base
update and calls slot 12 with 0 when clip +0x3c is not above the frame number
(`0x00818f0c`). Slot 29 returns D+0x14, which is not the record end.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x14` | 4 | `u32` color selector | `0x0081910f` |
| `0x18` | 4 | `s32` fade frames | `0x008190e4` |

The word at +0x10 is not read. INFERRED: the clip fades the screen toward opaque
black for selector 0 and toward clear black for selector 1, over the given
number of frames at 30 per second. Which direction each selector gives and what
the setter's bit 1 starts were not traced.

OBSERVED: all 3,095 records are 0x1c bytes with the key entry byte 0. The word at
+0x10 is 0. The selector is 0 in 1,726 and 1 in 1,369 records. The frames are
within -1 to 120, with -1 in 353, 30 in 881 and 29 in 467 records.

## RaptureSoundClip

The start helper `0x0081af60` runs from slot 2. It reads the key entry byte at
`0x0081afa4` and jumps through the table at `0x0081b134`. Key entry byte 0 sends
`u32(D+0x14)` and key entry byte 1 sends `u32(D+0x18)` to the play entry
`0x007cf100` with slot 0 and the volume 1.0 (`0x0081afbb`, `0x0081afe0`). Bytes
2 and 3 take the same branch, described next. The helper returns false on the
key entry byte 0 and 1 branches and when the owner lookup or the selector fails.
It also returns false when the selector leaves its byte output clear and slot 14
of the scheduler returns 2 (`0x0081b095` to `0x0081b0b0`). On the array path it
ignores the return value of `0x007cf100` and returns whether `u32(D+0x18)` is
nonzero (`cmp` and `setne` at `0x0081b123`, `0x0081b129`), so the result is true
exactly for a nonzero sound slot.

The volume is the float at +0x1c capped at 1.0 (`0x0081b003`). The flag word at
+0x14 yields three bits that go to the selector `0x0081ac50` together with the
actor record from `u8(D+3)`, the entry count and the address of the result slot.
The count is `u32(D+0x24)` shifted right by 2 (`0x0081b04e`).

| Bit of `u32(D+0x14)` | Use |
| ---: | --- |
| 0 | Fifth stack argument of `0x007cf100`, in the first call and in the preload replay of slot 2, and bit 3 of clip +0x38 (`0x0081b0c3` to `0x0081b0e1`). The selector fails when the actor ordinal names no actor. |
| 1 | The index is the low byte of `0x006b6cd0` on the component at actor +0x2b5c, or 0 when that is not below the count. |
| 2 | The index is the byte of the table at `0x0100aa33` selected by the dword at actor +0x13dc, or 0 when that dword is not within 1 to 18. |
| 3 | Read by slot 1 for key entry byte 2 only, see below. |

With neither bit 1 nor bit 2 set the index is 0. The element at
`D + 0x28 + 4 * index` (`0x0081b0cf`) is stored at clip +0x44 and passed to
`0x007cf100` with the slot `u32(D+0x18)` (`0x0081b0b6`), the volume,
`u32(D+0x20)` (`0x0081b0de`), bit 0 and 1.0 at the call `0x0081b11c`. The volume
is negated when bit 2 of clip +0x38 is set, and a negative volume makes the play
entry pass only the resource id to slot 8 of the object that `0x0060b280`
returns on the component at +0x114 (`0x007cf122` to `0x007cf138`) without
playing it. The play entry takes slot 0 as the shared request object
`0x0132f3b4` at `0x007cf2e9`. A slot n above 0 selects element n-1 of the vector
of 0x18 byte elements whose begin is at +0xbc and end at +0xc0 and grows the
vector when it is shorter (`0x007cf2f5`). With slot 0 and bit 0 set the entry
scans the vector at +0xcc for an element for which `0x00b52b70` returns false
and plays through it (`0x007cf159`).

The constructor increments the global counter `0x0134e2a8` through
`InterlockedIncrement` when the key entry byte is 2 and bit 3 of `u32(D+0x14)`
is set (`0x0081ae0f` to `0x0081ae29`).

Slot 1 clears bits 0 to 4 of the byte at clip +0x38 and the words at clip +0x40,
+0x44, +0x4c and +0x50, and stores 1.0 at +0x48. When bit 2 of S+0x91 is set it
calls slot 12 with 2. Otherwise only key entry byte 2 continues: it copies bit 3
of `u32(D+0x14)` (`0x0081aef0`) to bit 2 of clip +0x38 and, when that is set,
increments the global counter `0x0134e2ac` and calls slot 12 with 1 if it then
equals the counter `0x0134e2a8` and with 2 otherwise.

Slot 2 runs the base update. While bit 0 of clip +0x38 is clear it tests S+0xc8,
bit 7 of S+0x91 and bit 7 of S+0x90 (`0x0081b262`, `0x0081b276`, `0x0081b28a`).
Any of them aborts the clip through slot 18 and calls slot 12 with 0. Otherwise,
with bit 2 of S+0x91 clear, it converts the elapsed ticks to milliseconds as
ticks over 300000.0 times 1000.0 (`0x00f67020`, `0x00f91c48`), calls the start
helper (`0x0081b2f2`), stores the helper result in bit 1 and sets bit 0, so the
start runs once. It then calls slot 12 (`0x0081b2f7`) with phase 2 (1 when bit 2
of clip +0x38 is set) if the helper returned true, and with bit 2 of clip +0x38
as the phase if it returned false. Without the preload bit a false result
therefore ends the clip with phase 0 at once, which is the case for every slot 0
clip, and a nonzero slot holds phase 2. Later updates of a started clip leave
the phase alone. The phase changes that remain belong to the abort path and to
the preload path described next. From `0x0081b321`, when bits 0 and 2 of clip
+0x38 are set and clip +0x50 is nonnull, slot 2 issues the negated-volume call
again until bit 4 is set. A true result from the play entry sets bit 4 and
decrements both counters through `InterlockedDecrement` (`0x0081b387` to
`0x0081b397`). With bit 4 set and the counter `0x0134e2a8` at 0 it plays with
the stored volume, clears bit 2 and calls slot 12 with 2 when bit 1 is set and
with 0 otherwise. Slot 3 runs the base end and, when bits 0 and 1 are set, the
stop helper `0x0081b150`, so a slot 0 clip is never stopped. For key entry byte
0 or 1 the stop helper calls `0x007ce6d0` with 0 and 0. For 2 and 3 it finds the
actor again and calls `0x007ce6d0` with `u32(D+0x18)` and 1000.

| Offset | Width | Field | Read at | Branch |
| --- | ---: | --- | --- | --- |
| `0x14` | 4 | `u32` resource id | `0x0081afbb` | Key entry byte 0. |
| `0x18` | 4 | `u32` resource id | `0x0081afe0` | Key entry byte 1. |
| `0x14` | 4 | `u32` flag word | `0x0081b021` | Key entry byte 2 or 3. |
| `0x18` | 4 | `u32` sound slot | `0x0081b0b6` | Key entry byte 2 or 3. |
| `0x1c` | 4 | `f32` volume | `0x0081b003` | Key entry byte 2 or 3. |
| `0x20` | 4 | `u32` play argument | `0x0081b0de` | Key entry byte 2 or 3. |
| `0x24` | 4 | `u32` array byte length | `0x0081b04e` | Key entry byte 2 or 3. |
| `0x28 + 4k` | 4 | `u32` resource id | `0x0081b0cf` | Key entry byte 2 or 3. |

The word at +0x10 is not read. Slot 29 returns D+0x14, which is not the record
end, and the record is `0x28 + u32(D+0x24)` bytes.

INFERRED: the array entries are sound resource ids and the clip plays one of
them at the given sound slot with the given volume. The meaning of the flag bits
is inferred from their use as an index source and a positional switch. Also
INFERRED: the counters `0x0134e2a8` and `0x0134e2ac` and bit 3 make key entry
byte 2 clips request their sounds first and play them together once all have
been requested. The sound objects behind the slots and the receiver of
`u32(D+0x20)` are UNKNOWN. The key entry byte 0 and 1 branches and the bit 3
path have no install data. What ends a sound clip of nonzero slot, which holds
phase 2 after its one start, is UNKNOWN.

OBSERVED: the 2,763 records are 44 bytes in 2,715, 48 in 1, 64 in 28 and 76 in
19, which are 1, 2, 6 and 9 array entries. The word at +0x10 is 0 and bit 3 of
the flag word is never set. The flag word is 0 in 2,486, 1 in 213, 2 in 26 and 3
and 4 in 19 each. The slot is within 0 to 3. It is 0 in 1,916 records, 1,897
with key entry byte 3 and 19 with key entry byte 2, and nonzero in 847. The
volume is 1.0 in 2,739 records and between 0 and 0.8 in the other 24.
`u32(D+0x20)` is within 0 to 21000.

## ActionClip

Slot 1 `0x0082fc60` calls the shared begin `0x00a18170`. That method zeroes the
loop counter at clip +0x58, runs the base begin, sets clip +0x44 and +0x54 to 0,
the byte at +0x40 to 0 and stores 1.0 at +0x5c. It fetches the objects returned by
slots 9 and 12 of the owner at clip +0x24. When either is null it logs an error
and calls slot 18 with 1. It does the same for a negative `s32(D+0x10)`
(`0x00a181d3`). Otherwise it resolves the actor record from `u8(D+3)`, takes the
table object returned by `0x00a20040` and looks the id up through that object's
methods at +0x10 and +0xc. Bit 3 of `u8(D+0x14)` (`0x00a1824e`) chooses which of
the methods at +8 and +0xc of the object that owner slot 12 returned receives
the clip and the lookup results. A null result logs an error
and aborts. Otherwise the result is stored at clip +0x48 and the clip runs its
slot 49 method, at once or after the result reports ready.

Back in slot 1 the Rapture class calls slot 41, which returns the object at
clip +0x3c, and tests bit 5 of the byte at +8 of the record that slot 27 of the
embedded object at +0xf0 of that object returns. A set bit aborts the clip.

Slot 2 `0x0082fcb0` returns at once when the counter at clip +0x50 is below 2
and slot 48 rejects the update. It then calls slot 42. A result whose first byte
is 1 and second byte is nonzero runs only the base update and, when the byte at
clip +0x40 is set, `0x00a1f3c0` and `0x00a17eb0`. Any other result runs the
shared update `0x00a18480`, which acts only while the byte at clip +0x40 is
nonzero. The test of bit 7 of `u8(D+0x14)` (`0x00a184fc`) splits two branches.
Both read the position through slot 31 of the table at clip +0x38 (vtable +0x7c)
and slot 39 of the clip (vtable +0x9c). The first slot 39 call passes no
argument. They finish a pass when the sum reaches `s32(D+0x20)` (`0x00a18528`,
`0x00a185b0`) and the dword at +0x4c of the object at clip +0x3c is not above 0.
In the single pass branch the pass counter at clip +0x58 becomes 1. In the loop
branch it increments, and unless it equals the limit at clip +0x44 the action
restarts through `0x00a1c640` at the position minus `s32(D+0x20)` plus the slot
39 result plus `s32(D+0x1c)` (`0x00a18556`, `0x00a1857d`). The slot 39 call of
the restart receives the boolean `s32(D+0x1c) > 0` (`setg` at `0x00a1856b`).
Both branches end a finished pass with a call to slot 44 (vtable +0xb0) at
`0x00a185c5`, the restart path included. A limit other than -1 that equals the
counter calls slot 12 with 0. Slot 3 `0x00a176e0` runs the base end and, when
the byte at clip +0x40 is nonzero, stores 3 at clip +0x54 and calls slot 44 if
clip +0x3c is nonnull. Slot 4 `0x00a17cb0` runs the base stop and releases the
objects at clip +0x3c and +0x48. Slot 8 `0x00a176d0` returns `s32(D+0x18)`, the
end tick. Slot 29 returns D+0x28, the record end.

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x10` | 4 | `s32` action id | `0x00a181d3` | Negative aborts. Otherwise the lookup id. |
| `0x14` | 1 | `u8` flags | `0x00a1824e` | Bit 3 chooses the start method. |
| `0x14` | 1 | `u8` flags | `0x00a184fc` | Bit 7 selects the loop branch of slot 2. |
| `0x18` | 4 | `s32` end tick | `0x00a176d3` | Returned by slot 8. |
| `0x1c` | 4 | `s32` | `0x00a18556` | Loop restart offset. Slot 39 receives `s32(D+0x1c) > 0`. |
| `0x20` | 4 | `s32` | `0x00a18528` | Threshold the position plus slot 39 result is compared with. |

The byte at +0x15 and the words at +0x24 to +0x27 are not read by the traced
methods. INFERRED: the clip starts an actor action by id and the loop branch
repeats it. Slot 30 returns false, so the begin test of the scheduler tick
never skips the clip for a clear bit 1 of S+0x90, as the
[timing page](cut-scheduler-clip-timing.md#begin) records. The object returned
by `0x00a20040`, the lookup tables behind it, the two start methods, the
writers of clip +0x44 and what slot 49 does are UNKNOWN.

OBSERVED: all 2,142 records are 0x28 bytes and the key entry byte is 1. The id
is within 1 to 1331. The flag word `u32(D+0x14)` only sets bits 7 to 15: bit 7
is set in 816 records, which are the loop branch, and bit 3 is never set. In the
other 1,326 records the end tick is above the start tick. Among the loop
records the end is above the start in 158, equal in 21 and below in 637. The
word at +0x1c is 0 in all loop records and nonzero in 5 others. The word at +0x24
is 100 in 2,140 records, 50 in one and 60 in one.

## RaptureScaleClip

Slot 1 resolves the actor record, requires its slot 7 and a CharaActor cast
and reads the key entry byte (`0x0082517d`). Mode word `u32(D+0x14)` and the
branch below choose among three CharaActor setters, each guarded by slot 154.

| Key entry byte | Mode word | Call |
| ---: | ---: | --- |
| 0 | 0 | `0x00662b20` with `u32(D+0x18)` and `u32(D+0x1c)` |
| 0 | 1 | `0x0065e390` with the vector (f32(D+0x18) x 3, 1.0), `u32(D+0x1c)` and 0 |
| 0 | other | `0x0065e320` with `u32(D+0x1c)` |
| 1 | 0 | `0x00662b20` with `u32(D+0x18)` and `u32(D+0x20)` |
| 1 | 1 | `0x0065e390` with the same vector, `u32(D+0x20)` and `u8(D+0x1c) != 0` |
| 1 | other | `0x0065e320` with `u32(D+0x20)` |

Other key entry bytes make slot 1 return after the cast. The setter
`0x00662b20` calls `0x00661710`. When the dword at actor +0x2b10 is nonzero that
routine maps the preset index through `0x0065b5c0`, which asks the component
there (`0x00852150`, or `0x00852160` when the dword at actor +0xe0 is
`0x7fffffff`). It builds a uniform vector from the float and calls `0x00853090`
with the vector and the rounded length. The setter `0x0065e390` reaches the same
routine for a zero flag and `0x008530e0` for a nonzero flag through `0x0065c0d0`.
The setter `0x0065e320` calls `0x008533e0`, which builds a uniform vector from
the preset stored at +8 of its own state and stores the length at +0x30. Each
setter converts the length to float, adding 2^32 to a negative value, before
rounding.
Slot 29 returns D+0x14, which is not the record end.

| Offset | Width | Field | Read at | Branch |
| --- | ---: | --- | --- | --- |
| `0x14` | 4 | `u32` mode word | `0x00825188`, `0x00825213` | Key entry byte 0 or 1. |
| `0x18` | 4 | `u32` preset index | `0x00825296` | Mode word 0. |
| `0x18` | 4 | `f32` uniform factor | `0x008251b3`, `0x00825242` | Mode word 1. |
| `0x1c` | 4 | `u32` length | `0x0082518e` | Key entry byte 0. |
| `0x1c` | 1 | `u8` flag | `0x0082523e` | Key entry byte 1 and mode word 1. |
| `0x20` | 4 | `u32` length | `0x00825219` | Key entry byte 1. |

The word at +0x10 is not read. INFERRED: mode 0 selects a size preset by index,
mode 1 a uniform scale factor, and the length is the duration of the change.
The unit of the length and the value table behind `0x0065b5c0` were not traced.

OBSERVED: all 1,784 records are 0x24 bytes and the key entry byte is 1. The
word at +0x10 is 0. The mode word is 1 in 1,385 records and 0 in 399. In mode 0
the preset index is within 0 to 4 and in mode 1 the factor is between 0.25 and
8.0. The flag at +0x1c is 0 or 1 and 1 in 278 records. The length is 0 in 702, 1
in 1,077, 3 in 4 and 15 in 1 record.

## RaptureTurnClip

Slot 1 calls the base begin and needs the object that slot 9 of the object at
clip +0x24 returns. It resolves the actor record, requires its slot 7 and passes
`f32(D+0x10)` (`0x008114be`) to CharaActor method `0x0065e4a0` (`0x008114c6`).
After the slot 154 guard that method calls `0x007a4500` and `0x007b0530`. The
latter multiplies the angle by 16777216.0, divides it by the float 2 pi at
`0x012c4d20` and keeps the low 24 bits as the direction. Slot 1 then clears the
counter at clip +0x38 and calls slot 12 with 2.

Slot 2 starts with the base begin `0x00a20f30` and not the base update that the
other slot 2 methods call, so it stores state 1 at clip +0x1c on every update.
It then adds 1 to the counter at clip +0x38 and, if the counter is below 90 and
the predicate `0x007a4cc0` on the component at actor +0xbf0 holds, calls slot 12
with 2. The predicate tests that the dword at +0x1b8 of that component equals 1.
Otherwise it calls slot 18 with 1 and slot 12 with 0. When the actor record or
its CharaActor cast is missing it calls slot 12 with 0 at once. Slot 29 returns
D+0x14, the record end.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 4 | `f32` turn angle | `0x008114be` |

INFERRED: the angle is a target direction in radians and the clip holds the
scheduler until the turn ends or 90 updates pass. The effect of `0x007a4500` and
of the state value 1 at the component were not traced.

OBSERVED: all 1,776 records are 0x14 bytes and the key entry byte is 0. The
angle is within plus or minus pi and is 0.0 in 35 records.

## RaptureBgmClip

Slot 1 calls the base begin and obtains the owner through `0x00a20030` and
`0x0060b360`. It resolves the actor record from `u8(D+3)` and compares the
result of record slot 4 with the index that `0x00a1bb80` returns for the name
`ProxyActor` (the pointer at `0x012b7f34`), which makes a flag. It needs the
object at +0x68 of the owner result and calls `0x00611900` with that object,
`u32(D+0x20)`, `s32(D+0x14)`, `s32(D+0x18)` and the flag (`0x0081c03b`). That
forwarder calls `0x0060f630` with the same four values. The method compares the
first value with the current music id returned by `0x004b57b0`. When they differ
it calls `0x004b56c0` with the second value and 1, then `0x004b57d0`, and sets
the latch byte at +0x160, but only while that latch and the flag are both clear.
It then calls `0x004b57a0` with 0 and `0x004b7650` with the first value, 1.0 and
the second value. A clear flag stores 1 in the byte at +0x161 afterwards. A set
flag adds a call to `0x004b5690` with 2000 at the start of the method. The third
value is not read. Slot 29 returns D+0x14, which is not the record end.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x14` | 4 | `s32` fade time | `0x0081c032` |
| `0x18` | 4 | `s32` word | `0x0081c02e` |
| `0x20` | 4 | `u32` music resource id | `0x0081c036` |

The words at +0x10 and +0x1c are not read. INFERRED: the clip requests a music
change with a fade time in milliseconds. The `0x07fe0000` range of the id is the
resource id range that the
[music song table](music-song-table.md#request-derivation) derives from a song
id, and the flag marks a clip whose actor record is the ProxyActor class. The
receiver of `0x004b7650` and the meaning of the flag beyond `0x0060f630` were
not traced.

OBSERVED: all 1,610 records are 0x24 bytes with the key entry byte 0. The word at
+0x10 is 0 and the word at +0x1c is 4. The fade time is within 0 to 30000 and 0
in 463 records. The word at +0x18 is within 0 to 10000 and 0 in 1,594 records. The
high 16 bits of the resource id are `0x07fe` and the low 16 bits take 84 values
from 2 to 109.

## Open items

- The effect of CharaActor method `0x0065eaf0` on the actor and the meaning of
  the RaptureAutoMoveClip words at +0x14, +0x16, +0x18 and +0x1c after the
  handoff are UNKNOWN.
- The scheduler chunk behind the table of `0x00a20090` is UNKNOWN.
- The sound objects, the receiver of the RaptureSoundClip play argument and the
  owner of the counters `0x0134e2a8` and `0x0134e2ac` are UNKNOWN. The key entry
  byte 0 and 1 branches and the bit 3 path have no install data.
- The action tables behind ActionClip, its start methods and slot 49, and the
  writers of its clip +0x44 limit are UNKNOWN.
- The direction of each RaptureBlackFadeClip selector, the unit of the
  RaptureScaleClip length and the effect of the RaptureTurnClip state value are
  UNKNOWN.
- The registry that receives the second inserts of RaptureSoundClip and
  ActionClip was not walked.
- The names idle set, point, speed, fade, scale and action id are INFERRED and
  rest on the install value ranges and on argument flow.

## Verification

`python tools/verify_scb_clip_classes.py` checks the manifest, the census table
and the mutation cases. `--exe <ffxivgame.exe>` compares the byte checks with
the pinned image. `--install <game-root>` recomputes the census and checks every
record of the twenty classes against the OBSERVED invariants in the manifest.
`python tools/test_scb_clip_classes.py` covers the parser, the invariants and the
verifier with synthetic schedulers.
