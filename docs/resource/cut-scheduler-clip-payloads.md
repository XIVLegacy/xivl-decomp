# Cut scheduler clip payloads

This page records how the retail 1.23b client reads six clip classes of an
embedded `SEDBSCB` scheduler: SetPosClip, MotionClip, ShowHideClip,
RapturePhysicsLookAtClip, ClipSyncClip and LocalCameraClip. They are ranks 1
to 6 of the [clip class census](cut-scheduler-clip-class-census.md) and the
[cut scheduler clip layout](cut-scheduler-clip-layout.md) leaves them
undocumented. For each class the page gives the registration, the object, the
entry points and the record fields that the entry points read. The
[second payloads page](cut-scheduler-clip-payloads-ranks-7-13.md) covers six
more classes, and the [clip timing page](cut-scheduler-clip-timing.md) covers
the start and end ticks of all twelve. D is the serialized clip record, and all
offsets are relative to D unless a row says otherwise. All values are
little-endian.

## Evidence

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Capstone 5.0.7 in x86-32 mode and ghidra-cli queries on Ghidra 12.1.4 produced
the listings. Both were leads. [The manifest](../../config/scb_clip_classes.json)
holds every instruction sequence, table and string below as a byte check, and
`tools/verify_scb_clip_classes.py --exe <ffxivgame.exe>` compares them with the
pinned image. These are static observations without target execution.

Tier labels follow the manifest. STATIC means the cited instructions or tables
show it. INFERRED means a name or role fits the static facts and the install
data but no instruction names it. OBSERVED means a count or range over the
install. UNKNOWN means the path was not followed. The install is the
`client/cut` tree described in the [census](cut-scheduler-clip-class-census.md#method).

## Registration

Manager slot 3 at `0x00635760` calls the base registrar `0x00a4c2a0` at
`0x00635899` and then inserts the Rapture group. Both insert through
`0x0063f710` into the registry object reached from the singleton returned by
`0x00a02360`, the object named in the
[layout page](cut-scheduler-clip-layout.md#registration). Insertion keeps the
earlier factory for an equal key.

| Class | Key literal in the base group | Base factory | Key in the Rapture group | Rapture factory |
| --- | --- | --- | --- | --- |
| SetPosClip | `0x0109b7ac` | `0x00a4db10` | `0x00fbd160` | `0x0063bd90` |
| MotionClip | `0x0109b778` | `0x00a4dcd0` | `0x00fbd228` | `0x0063c180` |
| ShowHideClip | `0x0109b79c` | `0x00a4db80` | `0x00fbd1b4` | `0x0063bf40` |
| RapturePhysicsLookAtClip | none | none | `0x00fbcce4` | `0x0063a5f0` |
| ClipSyncClip | `0x0109b7e8` | `0x0063c570` | none | none |
| LocalCameraClip | `0x0109b804` | `0x00a4d950` | none | none |

The Rapture group registers SetPosClip, MotionClip and ShowHideClip after a
call to `0x0063f920` with a second copy of the key literal (`0x00fbd154`,
`0x00fbd21c`, `0x00fbd1a4`). The
[Attribute registration page](attribute-resource-and-scene-binding.md) reads
that call as an erase by key, so the registry then holds the Rapture factories
for those three classes (INFERRED, the body of `0x0063f920` was not walked).
ClipSyncClip and LocalCameraClip keep their base factories. The Rapture group
also registers some keys in the registry of a second singleton returned by
`0x00a65600` and in the Lay registry. Those groups were not followed.

## Objects and entry points

A factory stores its object size when its size argument is nonnull and
otherwise allocates that many bytes, as described for the
[Lay factory](showhide-clip-dispatch.md#registry-and-construction). Each constructor
installs the primary vtable at +0 and a secondary table at +8.

| Class | Object bytes | Constructor | Primary vtable | RTTI | Slot 1 | Slot 2 | Slot 29 |
| --- | ---: | --- | --- | --- | --- | --- | --- |
| SetPosClip | `0x38` | `0x00829fc0` | `0x0102f44c` | RaptureSetPosClip | `0x0082a080` | `0x00a20f90` | `0x00829f90` |
| MotionClip | `0x60` | `0x0082ef70` | `0x010337ec` | RaptureMotionClip | `0x0082f120` | `0x0082f4f0` | `0x0082faa0` |
| ShowHideClip | `0x38` | `0x0082ddb0` | `0x0103146c` | RaptureShowHideClip | `0x0082de70` | `0x00a20f90` | `0x00823ce0` |
| RapturePhysicsLookAtClip | `0x38` | `0x00821a80` | `0x0101564c` | RapturePhysicsLookAtClip | `0x00821b40` | `0x00821a70` | `0x00823ce0` |
| ClipSyncClip | `0x3c` | `0x00a100d0` | `0x0109604c` | Plugins ClipSyncClip | `0x00a100b0` | `0x00a10120` | `0x00a10210` |
| LocalCameraClip | `0x120` | `0x00a0c5d0` | `0x01095874` | Plugins LocalCameraClip | `0x00a0c740` | `0x00a0bfd0` | `0x00df1040` |

Slot 1 of every class calls the base method `0x00a20f30`, which stores state 1
at clip +0x1c and a value from `0x00a20110` at clip +0x24. Slot 2 is the update
method. The base update `0x00a20f90` moves state 1 to state 2. Slots 3 and 4,
`0x00a21000` and `0x00a21010`, store state 3 and state 4. Slot 29 returns an
address relative to D, listed per class below. It is the record end for
SetPosClip, ShowHideClip and MotionClip and is not the record end for the other
three classes. Slot 7 is the start tick and slot 8 the end tick. Slot 8 jumps
through slot 7 unless the class overrides it, as the
[timing page](cut-scheduler-clip-timing.md#clip-start-and-end) records.

## Record header words

Base accessors read the header words that every class shares.

| Slot | Method | Reads |
| ---: | --- | --- |
| 5 | `0x00a21030` | `u8(D+2)`, the class ordinal |
| 6 | `0x00a21060` | `u8(D+3)`, the actor ordinal |
| 7 | `0x00a21070` | `u32(D+4)`, the start tick |
| 22 | `0x00a21190` | `s16(D+0x08)` |
| 23 | `0x00a211a0` | `u8(D+0x0a)` |
| 25, 27, 26 | `0x00a211b0`, `0x00a211d0`, `0x00a211f0` | bits 0, 1 and 2 of `u8(D+0x0b)` |
| 20 | `0x00a21170` | `s16(D+0x0c)` |

The scheduler tick reads `s16(D+0x08)` as a filter, bit 0 of `u8(D+0x0b)` to
block a second begin and bit 2 to skip a clip while S+0xc8 is set, as the
[timing page](cut-scheduler-clip-timing.md#begin) records. `s16(D+0x0c)` is the
index of the clip in the scheduler clip list, as the
[timing page](cut-scheduler-clip-timing.md#clip-index-word) records. The callers
of the accessors for `u8(D+0x0a)` and for bit 1 of `u8(D+0x0b)` were not traced,
so what those words select is UNKNOWN. SetPosClip, MotionClip, ShowHideClip,
ClipSyncClip and LocalCameraClip read class fields from +0x10, so the bytes from
+0x10 to +0x13 belong to the class and not to the shared header.

## Class key entry

The clip builder `0x00a28510` computes keyEntry as the table at +0x0c of its
owner plus 8 * `u8(D+2)` (`0x00a28617`) and passes it to the factory. The
constructor chain stores it at clip +0x18, as the
[layout page](cut-scheduler-clip-layout.md#registration) records. The scheduler
resource loader `0x00a24400` calls `0x00a27210` for the `@CCPT` chunk
(`0x00a244d4`, tag string at `0x01098234`). For each four byte name entry that
routine writes 8 bytes: the address of the String table entry named by
`u16(entry)` at key entry +0, and `u8(entry+2)` at key entry +4 (`0x00a27234`).
STATIC for both functions.

INFERRED: the table the builder reads is the table `0x00a27210` fills, because
both use 8 byte entries with a string address at +0 and the entry is the class
key. The owner object that holds the table was not traced. What the @CCPT entry
byte +2 means beyond the LocalCameraClip gate is UNKNOWN.

OBSERVED: every record of a class sees one value of the @CCPT entry byte +2.

| Class | Entry byte +2 |
| --- | ---: |
| SetPosClip | 1 |
| ShowHideClip | 1 |
| ClipSyncClip | 1 |
| MotionClip | 2 |
| RapturePhysicsLookAtClip | 0 |
| LocalCameraClip | 4 |

## SetPosClip

RaptureSetPosClip slot 1 resolves the actor record with `u8(D+3)`, requires
its slot 7 to be true, and casts the slot 23 result from IActor to CharaActor.
It fetches three vectors, A, B and C, from the object that slot 9 of the
object at clip +0x24 returns, through that object's slots 12, 14 and 16. Each
float whose flag byte is nonzero replaces the fetched component. When the
scheduler test `0x00a1c050` returns true, slot 18 of the target object
receives the three vectors and the scheduler origin. The method then calls
CharaActor setters `0x0065e660` with vector A and `0x0065e690` with vector B.
Both receive the inverse of bit 0 of `u8(D+0x37)`. Vector C reaches only the
slot 18 call. Slot 29 returns D+0x40, the record end.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10`, `0x14`, `0x18` | 4 each | vector A x, y, z (f32) | `0x0082a130` |
| `0x1c`, `0x20`, `0x24` | 4 each | vector B x, y, z (f32) | `0x0082a130` |
| `0x28`, `0x2c`, `0x30` | 4 each | vector C x, y, z (f32) | `0x0082a130` |
| `0x34`, `0x35`, `0x36` | 1 each | vector A override flags | `0x0082a130` |
| `0x38`, `0x39`, `0x3a` | 1 each | vector B override flags | `0x0082a130` |
| `0x3c`, `0x3d`, `0x3e` | 1 each | vector C override flags | `0x0082a130` |
| `0x37` | 1 | bit 0 inverted into the setter flag | `0x0082a2b5` |

Bytes +0x3b and +0x3f are not read. INFERRED: vector A is a position, B is a
rotation in radians and C is a scale. The install records hold A components
between -2824.9 and 3563.5, B components within plus or minus pi and C
components between 0.1 and 8.0, with the A and C ranges pinned to one decimal
in the manifest. The setter `0x0065e660` stores its vector at
CharaActor +0x11f0 when CharaActor slot 154 returns 0.

OBSERVED: all 33,078 records are 0x40 bytes, the flag bytes are 0 or 1 and
bytes +0x3b and +0x3f are 0.

## MotionClip

RaptureMotionClip slot 1 ends the clip at once when `s32(D+0x10)` is negative,
by calling its slot 18 with 1 and its slot 12 with 0. Otherwise it passes the
index to slot 3 of the scheduler object at +0x74, which fills a 16 byte name
record. Three outcomes follow. When slot 7 of the actor record is false, its
slot 23 result is null or the actor address plus 0x11b4 is zero
(`0x0082f203`, `0x0082f214`, `0x0082f21b`), the method calls slot 12 with 0 and
returns. When the name starts with `c` or `e`, has `m`, `f` or `h` second and
`_` third, it calls slot 18 with 1 and then slot 12 with 0
(`0x0082f240` to `0x0082f249`, then `0x0082f24b`). In every other case it builds
a motion request and passes the name and the request to `0x007b9920` with ECX
at actor +0x4c0. Slot 8 returns `u32(D+0x18)`.
Slot 29 returns D+0x2c, the record end.

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x10` | 4 | `s32` name index | `0x0082f16e` | Index into the name object. |
| `0x14` | 1 | `u8` flags | `0x0082f2fb` | Bit 2 goes to a request byte. Bit 6 goes to clip +0x44 and a request byte. Bit 7 selects the selector branches. |
| `0x15` | 1 | `u8` selector | `0x0082f3cc` | `0xff` becomes -1, any other value becomes the signed value minus 1. It reaches the request only when bit 7 of +0x14 is set and bit 6 is clear. With bit 6 set the request word is -1 or 0 by bit 7. With both clear it is 0. A result of -1 makes the clip call its slot 12 with 2. |
| `0x18` | 4 | `u32` | `0x0082f110` | Returned by slot 8, the end tick. |
| `0x1c` | 4 | `s32` | `0x0082f32b` | Divided by 10000, truncated toward zero, low 16 bits to a request word. |
| `0x20` | 4 | `s32` | `0x0082f32b` | The same division. The request word is 0 when bit 6 of +0x14 is set, or when bit 7 is set and the selector result is nonzero. |
| `0x24` | 2 | `u16` | `0x0082f38e` | Divided by 100.0 and multiplied by a local float that starts at 1.0. |
| `0x26` | 1 | `u8` | `0x0082f2fb` | Nonzero goes to a request byte. |

Bytes +0x16, +0x17, +0x27 and the word at +0x28 are not read by slots 1, 2 or
3. INFERRED: the 16 byte record is a motion name and the request starts that
motion on the actor. Names that start with `cm_`, `cf_`, `ch_`, `em_`, `ef_` or
`eh_` end the clip without starting a motion. The u16 at +0x24 reads as a speed
in percent.

OBSERVED: all 22,085 records are 0x2c bytes. No name index is negative. The
words at +0x18, +0x1c and +0x20 are multiples of 10000. The u16 at +0x24 is 100
in 20,305 records.

## ShowHideClip

RaptureShowHideClip slot 1 resolves the actor record, casts its slot 23
result from IActor to RaptureActor, and requires a nonnull pointer at
RaptureActor +0x9c. It calls slot 2 of that pointer with the RaptureActor and
the boolean `u8(D+0x10) == 1`. It reads no record field beyond the actor
ordinal and this byte. Slot 29 returns
D+0x14, the record end. The Lay registry holds a different class under the
same key, described in [ShowHideClip registration and value
dispatch](showhide-clip-dispatch.md).

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 1 | operand byte | `0x0082decf` |

INFERRED: 1 shows and 0 hides. The object at RaptureActor +0x9c was not
identified (UNKNOWN). The constructor at `0x007ced70` does not store it.

OBSERVED: all 21,581 records are 0x14 bytes, the operand is 0 in 9,399 and 1 in
12,182, and bytes +0x11 to +0x13 are 0.

## RapturePhysicsLookAtClip

RapturePhysicsLookAtClip slot 1 casts the actor record to CharaActor and runs
two passes with pass index 0 and 1. Pass 0 runs when `u8(D+0x15)` is nonzero
and pass 1 when `u8(D+0x16)` is nonzero. Each pass switches on `u32(D+0x1c)`.
Any value other than 0, 1 and 2 runs no branch. Slot 29 is the method
`0x00823ce0`, shared with ShowHideClip, and returns D+0x14, which is not the
record end. The record is 0x34 bytes.

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x14` | 1 | `u8` | `0x00821c10` | Pushed to the setters. In mode 0 a nonzero value selects the target branch. |
| `0x15` | 1 | `u8` | `0x00821c10` | Enables pass 0. |
| `0x16` | 1 | `u8` | `0x00821c10` | Enables pass 1. |
| `0x17` | 1 | `u8` | `0x00821c10` | Pushed to the setters. |
| `0x18` | 4 | `f32` | `0x00821c10` | The first float argument of the mode 1 and mode 2 calls, ahead of the angles or vector (`fstp [esp]` at `0x00821e93` and `0x00821e39`). |
| `0x1c` | 4 | `u32` mode | `0x00821c80` | Selects the branch. |
| `0x20` | 4 | mode 0 `s32` | `0x00821ecb` | Target actor ordinal, looked up and cast to CharaActor. A negative value with `u8(D+0x14)` nonzero takes the error branch at `0x00821fae`. |
| `0x24` | 4 | mode 0 `s32` | `0x00821f1d` | Node index passed to `0x00a20080`. |
| `0x28`, `0x2c`, `0x30` | 4 each | mode 0 `f32` | `0x00821f2a` | Offset vector passed to `0x0065e730`. |
| `0x20`, `0x24` | 4 each | mode 1 `f32` | `0x00821e50` | Multiplied by pi, divided by 180.0 and passed to `0x00662be0`. |
| `0x20`, `0x24`, `0x28` | 4 each | mode 2 `f32` | `0x00821c9e` | Vector passed to `0x0065e7c0`. |

The error branches at `0x00821fae`, `0x00821fd9` and `0x00822004` format the
texts "Target specification missing", "Target is not an IActor" and "Target is
not a CharaActor" (the client strings are Shift-JIS Japanese, checked as bytes
in the manifest). INFERRED: mode 1 carries two angles in degrees and mode 2 a
world point. The meaning of the two passes and of the node index was not traced.

OBSERVED: all 15,619 records are 0x34 bytes. The mode is 0 in 13,320, 1 in
2,169 and 2 in 130 records. The mode 1 angles lie within plus or minus 90.
The flag bytes are 0 or 1.

## ClipSyncClip

ClipSyncClip slot 1 clears the byte at clip +0x38 and calls slot 12 with 1.
Slot 2 reads `u16(D+0x10)`. Its low 15 bits are the count n of `s16` clip
indexes at +0x14, two bytes each. Each index goes through the scheduler clip
lookup `0x00a1b790`, which returns 0 outside the clip array. With bit 15 set
the clip calls its slot 12 with 0 once every listed clip is in state 3 or 4.
With bit 15 clear it does so as soon as one listed clip is. A lookup that
returns 0 leaves the clip running. Slot 29 returns D + ((2n + 0x19) & ~3).
That equals the record end (2n + 0x17) & ~3 for odd n. For even n it lies 4
bytes past the record end, so for n = 0 it gives D+0x18 on a 0x14 byte record.

| Offset | Width | Field | Read at |
| --- | ---: | --- | --- |
| `0x10` | 2 | `u16` mode bit and count n | `0x00a10135` |
| `0x14 + 2k` | 2 | `s16` clip index | `0x00a10152` |

OBSERVED: the record is (0x14 + 2n + 3) & ~3 bytes in all 14,001 records, from
20 to 76 bytes. `u16(D+0x12)` is 0. Every index is inside the clip list of its
scheduler and none names its own clip. The census lists the eleven sizes. Of
the 14,001 records 247 have even n, which are the records whose slot 29 result
lies past the record end.

## LocalCameraClip

LocalCameraClip slot 1 reads `u8(D+3)`, `u16(D+0x14)`, `s16(D+0x0c)`,
`s16(D+8)`, `s16(D+0x1c)`, `u16(D+0x16)` and `u8(D+0x18)`. It looks up
`s16(D+8)` and `s16(D+0x1c)` through `0x00a1b7c0` and stores at clip +0x74
whether the result byte is nonzero. It pushes `u16(D+0x16)` and `u8(D+0x18)` as
arguments of the virtual calls at `0x00a0ca27` and `0x00a0ca50`.

Every read of `u8(D+0x1a)` in slots 1 and 2 first loads the class key entry
from clip +0x18 and skips the read when `u8(key+4)` is below 4
(`mov eax,[esi+0x18]; cmp byte ptr [eax+4],4; jb` at `0x00a0ca52`, `0x00a0c11e`
and `0x00a0c15d`). The source of that byte is described under [Class key
entry](#class-key-entry). When it is at least 4 and bit 0 of `u8(D+0x1a)` is
set, slot 1 passes D+0x20 to `0x00a0b9c0`, which reads the bind record below.
Slot 2 tests bit 0 and bit 2 of `u8(D+0x1a)` behind the same gate. Slot 8
returns `u32(D+0x10)` plus clip +0x110, which the
[timing page](cut-scheduler-clip-timing.md#clip-start-and-end) describes. Slot 29
returns D+0x24, the first extension float. The extension itself starts at D+0x20.

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x08` | 2 | `s16` | `0x00a0c9c7` | Lookup argument with +0x1c. |
| `0x0c` | 2 | `s16` | `0x00a0c7b7` | Argument of `0x00a20120`. |
| `0x10` | 4 | `u32` | `0x00a0b830` | Slot 8 value, the end tick before the origin change. |
| `0x14` | 2 | `u16` | `0x00a0c7b7` | Argument of `0x00a20090`. |
| `0x16` | 2 | `u16` | `0x00a0ca0a` | Argument of the virtual call at `0x00a0ca27`. |
| `0x18` | 1 | `u8` | `0x00a0ca3b` | Argument of the virtual call at `0x00a0ca50`. |
| `0x1a` | 1 | `u8` flags | `0x00a0ca5a` | With key entry byte +4 at least 4, bit 0 gates the extension in slots 1 and 2 and bit 2 is read by slot 2. Without it none of the bits is read. Bit 1 is not read by slots 1 or 2. |
| `0x1c` | 2 | `s16` | `0x00a0c9c7` | Lookup argument with +0x08. |
| `0x20` | 2 | extension `s16` | `0x00a0b9dc` | Actor ordinal. A negative value takes the error branch at `0x00a0b9e4`. |
| `0x22` | 2 | extension `s16` | `0x00a0ba4a` | Part index passed to `0x00a20080`. A negative value takes the error branch at `0x00a0ba53`. |
| `0x24` to `0x38` | 4 each | extension `f32` x 6 | `0x00a0bb7f` | Copied to the bind record at clip +0x80, offsets +0x08 to +0x1c. |
| `0x3c`, `0x3d` | 1 each | extension `u8` | `0x00a0bb7f` | Copied to bind record bytes +0x20 and +0x21. |

The error branches log "Bind Camera" texts about an invalid bind actor and an
invalid bind part name, stored as Shift-JIS bytes in the manifest.

OBSERVED: the key entry byte +4 is 4 for every LocalCameraClip record, so the
gate passes throughout this install. The record is 0x40 bytes in the 1,484
records with bit 0 of +0x1a set and 0x20 bytes in the other 12,404. In the long records the extension actor
ordinal is an actor of the scheduler and the part index is not negative.
`u16(D+0x16)` and `u16(D+0x1e)` are 0.

## Open items

- The concrete type behind RaptureActor +0x9c, which receives the ShowHideClip
  operand, is UNKNOWN.
- The meaning of the shared header byte at +0x0a and of bit 1 of the byte at
  +0x0b is UNKNOWN.
- The meaning of the @CCPT entry byte +2, which key entry +4 carries, is
  UNKNOWN beyond the LocalCameraClip extension gate, and the owner of the table
  that the clip builder reads is INFERRED.
- The body of `0x0063f920` was not walked, so the effective factory of the
  three overridden classes rests on the INFERRED erase reading.
- The names position, rotation, scale, motion name, degrees and show or hide
  are INFERRED and rest on the install value ranges and on argument flow.

## Verification

`python tools/verify_scb_clip_classes.py` checks the manifest, the census table
and the mutation cases. `--exe <ffxivgame.exe>` compares the byte checks with
the pinned image. `--install <game-root>` recomputes the census, checks
every record of the six classes against the OBSERVED invariants in the
manifest and compares the pinned distributions and ranges: the ShowHideClip
operand, the MotionClip speed word, the PhysicsLookAtClip mode, the ClipSyncClip
count parity and the SetPosClip position and scale ranges. `python tools/test_scb_clip_classes.py` covers the parser, the
invariants and the verifier with synthetic schedulers.
