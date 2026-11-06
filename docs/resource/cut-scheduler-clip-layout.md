# Cut scheduler clip layout

This page gives the byte layout through which the retail 1.23b client turns an
embedded `SEDBSCB` scheduler into clip objects. It covers
RaptureActorBindClip, RaptureCharaVisualClip and RaptureMapCollisionOnOffClip,
and it is enough to scan decoded scheduler spans for those clips and the
characters they target. The [CameraClip](#cameraclip) section at the end
covers the engine camera clip. All values are little-endian. All conditions
are static, and a match identifies authored data, not observed playback.

## Registration

CDevSchedulerManager slot 3, `0x00635760`, inserts these keys into
CutScenePlayer's clip registry `u32(C+0x6C)+8` through `0x0063F710`:

| Key | Literal | Factory | Object |
| --- | --- | --- | --- |
| `RaptureMapCollisionOnOffClip` | `0x00FBCA28` | `0x006396C0` | `0x38` bytes, vtable `0x01006A1C` |
| `RaptureCharaVisualClip` | `0x00FBCE80` | `0x0063AEF0` | `0x38` bytes, vtable `0x0101F61C` |
| `BindActorClip` | `0x00FBD1D4` | `0x0063BFD0` | `0x48` bytes, vtable `0x01031E14` |

The base SchedulerManager does not register the first two keys. Both
constructor chains end in `0x00A21260`, which stores constructor formal 1, the
serialized clip record `D`, at `clip+0x14` and the class-key entry at
`clip+0x18`.

## Resource header and directory

The generic reader checks the bytes `SEDB` and folds the next four bytes,
skipping NUL and space and lowercasing, to the tag `0x00736362` (`scb`).
CutSceneResourceNodeFactory, vtable `0x0109D12C`, is registered under that
tag. CutSceneResourceNode copies the span with offsets preserved. For span
start `R`:

```
u32(R+0x08) >= 2;  u8(R+0x0C) == 0
Q = R + u16(R+0x0E);  string(Q) == "@CRST";  u8(Q+0x08) == 1
nDir = s16(Q+0x0A);  E = Q + 16*u16(Q+0x06)
K[j] = E + u32(Q+0x10+4*j)
T = K[first j with strncmp(K[j]+0x10, "String", 8) == 0] + 0x20
Str(k) = T + s16(T+2*k)          NUL-terminated
```

The root chunk `K[0]` has `s8(K[0]+0x1C)` children. The first child is at
`K[0]+16*u16(K[0]+6)`, and each next child adds `16*u16(child+6)`. Children
are identified by the six bytes at the child start, including the NUL:

| Child | Contents |
| --- | --- |
| `@CATT` | Actor class names: `s16(+0x0A)` entries at `+0x10+4*k`, name `Str(u16(entry))` |
| `@CCPT` | Clip class names, same form |
| `@CACT` | Actor records `AD[0] = child+0x10`, `AD[i+1] = AD[i] + s16(AD[i])`, `s16(child+0x0A)` records |
| `@CBKT` | `s16(+0x0A)` blocks; block headers from `child+0x10`, each next at `+16*u16(header+6)` |
| `@CTRK` | Track table, not used for clip selection |

The actor container appends every `@CACT` factory result, including null
results, so an actor ordinal is exactly the `@CACT` record index. The class of
record `i` is `@CATT` entry `u8(AD[i]+2)`. The
[raid cutscene numeric actor records](raid-cutscene-actor-records.md) are
records of this form. Their words `+0x20 = 5` and `+0x2C = 2` and
nonnegative token at `+0x30` satisfy the ProxyActor binding conditions below.

## Clip records

For block header `H`, the body is `X = H+0x10`. It holds `u16(X+0x18)` clip
records starting at `D[0] = X+0x30`, with `D[k+1] = D[k] + s16(D[k])`. The
builder `0x00A28510` resolves the clip class from `u8(D+2)` without a bounds
check and calls the registered factory with `(D, keyEntry, arena, 0)`:

| Offset | Width | Meaning |
| --- | --- | --- |
| `+0x00` | s16 | Link to the next record |
| `+0x02` | u8 | `@CCPT` clip class ordinal |
| `+0x03` | u8 | Actor ordinal, an `@CACT` index |
| `+0x04` | u32 | Start tick, returned by clip slot 7. See the [clip timing](cut-scheduler-clip-timing.md#clip-start-and-end) |
| `+0x08` | s16, u8, u8 | Header words read by accessors, see the [clip payloads](cut-scheduler-clip-payloads.md#record-header-words) |
| `+0x0c` | s16 | Index of the clip in the scheduler clip list, see the [clip timing](cut-scheduler-clip-timing.md#clip-index-word) |
| `+0x10` | | Class-specific bytes. The classes read below start at `+0x14`. The slot 29 result differs per class, see the [clip payloads](cut-scheduler-clip-payloads.md#objects-and-entry-points) |

RaptureMapCollisionOnOffClip slot 1, `0x00819AD0`, reads:

| Field | Use |
| --- | --- |
| `u8(D+0x14)` | Passes `!= 0` to CharaActor slot 110, flag bit 3 |
| `u32(D+0x18)` | Value 1 targets actor ordinal `u8(D+3)`; any other value broadcasts to CutManagerActor's actor vector |

The single-actor branch requires the record's class to be `ProxyActor` and its
slot 23 to return an actor. The broadcast reaches every cut-created actor in
the vector.

RaptureCharaVisualClip slot 1, `0x00825D70`, takes actor ordinal `u8(D+3)`,
requires record slot 7 and a CharaActor cast of record slot 23, and calls
`0x00662CE0(actor, u32(D+0x14), u32(D+0x18))`. The selector at `D+0x14`
chooses an appearance field: values 6 to 46 write one of four appearance
categories, and a negative value restores the saved word. After the
`slot154() == 0` guard, every selector reaches `0x0065D730`, which arms the
CharaActor model load.

## Actor binding

RaptureActorBindClip slot 1, `0x0082E160`, uses `u8(D+3)` with the same actor
lookup. A cut character is created through `0x0061BD90` only when all of these
hold:

- the record's class is `ProxyActor`;
- with `f = u8(AD+0x20)`, `(f & 5) == 5`;
- the query word `q` is `AD+0x64` when `f` bit 1 is set, otherwise `AD+0x2C`,
  and `u32(q) == 2` and `s32(q+4) >= 0`;
- the record has no bound actor yet;
- the resource-type check at `0x0082E1F8` does not find the folded type
  `vfx`.

The created actor is bound to RaptureProxyActor `+0xD0`. Its actor ID is
stored at `+0xF0` through `0x0082DF10`. ProxyActor slot 23, `0x00622C30`,
later resolves that ID, so visual and single-actor collision clips with the
same actor ordinal reach the created character. Without a bind, slot 23
returns null and those clips do nothing.

## Scan predicate

A scheduler span is a candidate for admitting an Attribute's group 1 to a
cut character when it contains:

1. a BindActorClip with actor ordinal `i` meeting the binding conditions
   above;
2. a RaptureCharaVisualClip with `u8(D+3) == i`;
3. a RaptureMapCollisionOnOffClip with `u8(D+0x14) != 0` and either
   `u32(D+0x18) == 1` with `u8(D+3) == i`, or `u32(D+0x18) != 1`.

The scan cannot establish the create branch at runtime, allocation and lookup
success, block execution order, or the activation and flag conditions in
[Attribute resource and native physics registration](attribute-resource-and-scene-binding.md).
The [cut collision clip corpus](cut-collision-clip-corpus.md) applies this
predicate to the 1.23b `client/cut` tree.

The [clip class census](cut-scheduler-clip-class-census.md) ranks all 102 clip
classes of these schedulers. The [clip payloads page](cut-scheduler-clip-payloads.md),
the [second payloads page](cut-scheduler-clip-payloads-ranks-7-13.md) and the
[third payloads page](cut-scheduler-clip-payloads-ranks-14-22.md)
document twenty more, which read class fields from +0x10 or +0x14. The
[clip timing page](cut-scheduler-clip-timing.md) covers the start tick at +4 and
the end tick of every documented class.

## CameraClip

The engine camera clip, rank 27 of the [census](cut-scheduler-clip-class-census.md)
with 1,154 records of 36 bytes in 31 schedulers. Every statement below is
STATIC, read from the instructions at the cited address of the executable
identified in the [cut camera page](cut-camera-ccb.md#identity-and-method),
unless labelled otherwise. The function bodies the reads come from are
identified by the SHA-256 of their bytes, so a later reader can confirm the
same code was read without the bytes being recorded here:

| Function | VA | Bytes | SHA-256 of the bytes |
| --- | --- | ---: | --- |
| Factory | `0x00a4d9c0` | 112 (to the next factory at `0x00a4da30`) | `b4f2e6370af0e922be47422f58e1c02e9b29a8e517c43ca031483178692f8160` |
| Constructor | `0x00a0af70` | 113 | `aff7ed399c9a22ce8657b6467590d4a7f579ae51e2db9c5404b07090d0455bf7` |
| Slot 1 | `0x00a0b0a0` | 1017 | `b98e9b86eb972bcb0ae3ff5de2aac76661f4a57c9cc2d08cffad10388d3ff158` |
| Slot 2 | `0x00a0a9a0` | 1482 | `951035082d32c393e1ebd5d33b35eb550c9192c897e5ed2474f3d8153f3df0a0` |
| Slot 8 | `0x00a0a210` | 7 | `e79f75ab585fc5f0d305427d1f1b0b205b8de259a2c2ec08bf73c52291830d43` |

Registration. The base registration routine `0x00a4cdb0`, which inserts the
LocalCameraClip key (`0x0109b804`, factory `0x00a4d950`) at `0x00a4ce1d`,
inserts the key literal `CameraClip` at `0x0109b7f8` with the factory
`0x00a4d9c0` at `0x00a4ce44` through the same `0x0063f710` into the registry
`+8` of the singleton returned by `0x00a65600`. The factory stores or
allocates `0xe4` bytes (`0x00a4d9dd`, `0x00a4d9fa`) and calls the constructor
`0x00a0af70`, which runs the base chain `0x00a21260` and installs the primary
vtable `0x010955dc` at +0 and `0x010955d4` at +8 (`0x00a0afa3`,
`0x00a0afa9`). The RTTI name is `SQEX::CDev::Engine::Cut::Plugins::CameraClip`
(`0x012b79e0`). The slots read below are slot 1 `0x00a0b0a0`, slot 2
`0x00a0a9a0`, slot 8 `0x00a0a210`, slot 9 `0x00a210a0` (the base elapsed
ticks of the [timing page](cut-scheduler-clip-timing.md#clip-start-and-end))
and slot 29 `0x00df1040`, the LocalCameraClip value.

Slot 8 returns `u32(D+0x10)` and nothing else (the whole body is the two loads
at `0x00a0a210`), so the clip ends by the rule of the timing page with no
runtime term. Slot 1 first runs the base begin `0x00a20f30` and resolves the
scheduler owner through slots of the object at clip +0x24. It then reads the
record `D` at clip +0x14:

| Offset | Width | Field | Read at | Use |
| --- | ---: | --- | --- | --- |
| `0x03` | 1 | `u8` | `0x00a0b19f` | Actor ordinal, the argument of the actor lookup `0x00a1ffa0`, stored at clip +0x60. |
| `0x08` | 2 | `s16` | `0x00a0b33e` | With `s16(D+0x1e)`, the arguments of `0x00a20070`. The byte that `0x00a1b7c0` returns for that result, compared with 0, is stored at clip +0x68 (`0x00a0b35d`), the form LocalCameraClip stores at clip +0x74. |
| `0x10` | 4 | `u32` | `0x00a0a210` | Slot 8, the end tick. |
| `0x14` | 4 | `s32` | `0x00a0b12a` | Camera resource index. A negative value formats the warning literal at `0x010956fc` (ASCII part `[WARNING][CameraClip]`, then `Resource`, then `index=%d`) with the value, logs it through `0x012651b4` and calls slot 18 with 1 (`0x00a0b17c` to `0x00a0b186`), which the timing page records as the abort route, and returns. A nonnegative value is pushed at `0x00a0b1c5` to the method at +0x10 of the table object `0x00a20040` returns, and at `0x00a0b1dd` to the method at +0xc of the stack object built from it. A null result formats the literal at `0x010956c0` (ASCII part `[WARNIGN][CameraClip]`, then `Resource`, then `index=%d`, as spelt in the image) with the value and takes the same abort route (`0x00a0b20a` to `0x00a0b257`). |
| `0x18` | 2 | `u16` | `0x00a0b37c` | Argument of `0x00a20040` together with the `std::string` at clip +0x44 (the SSO test `cmp dword ptr [esi+0x58], 0x10` at `0x00a0b360` selects the buffer), then the string is passed to the method at +4 of the owner object (`0x00a0b399`) and to the method at +0x14 (`0x00a0b3d7`). |
| `0x1a` | 2 | `u16` | `0x00a0b3a9` | Read only when the class key entry byte `u8(key+4)` is at least 3 (`cmp byte ptr [ecx+4], 3` then `jc` at `0x00a0b39e`), otherwise the constant 0x10 (`0x00a0b3a2`). Passed with the string to the method at +0x14. |
| `0x1c` | 1 | `u8` | `0x00a0b3e6` | Bit 0, read only when the key entry byte is at least 4 (`0x00a0b3dc`). When set, slot 1 asks the owner object's method at +0x24 for a listener (`0x00a0b3f1`). A null one logs the literal at `0x01095684` (ASCII part `[ERROR] Bind Camera`, then `CharactorListener`), and otherwise `D+0x22` (`add edi, 0x22` at `0x00a0b3fe`) is passed with clip +0x6c and clip +0x90 to `0x00a0a350`. |
| `0x1e` | 2 | `s16` | `0x00a0b33a` | Second argument of `0x00a20070`, see `0x08`. |

The object at +0xc of the stack object is the camera resource and the result
of its +0xc method becomes the camera object at clip +0x3c (INFERRED from the
update below, which evaluates clip +0x3c the way LocalCameraClip evaluates its
own +0x3c, and from the warning text). The tables behind `0x00a20040` and
`0x00a20070` were not traced here, as the
[third payloads page](cut-scheduler-clip-payloads-ranks-14-22.md) also leaves
them. Whether the resource index selects a `CCB` container of the same file in
file order, as the LocalCameraClip value is OBSERVED to do, was not measured
(UNKNOWN). The bind record at `D+0x22` starts two bytes before the end of
every 36 byte install record, so a bind record longer than two bytes reads
past it (no install record sets the bit), and its layout is UNKNOWN.

Slot 2, the update, asserts `pCamera != 0` with the source name
`.\CameraClip.cpp` (`0x01095578`) and the function name
`SQEX::CDev::Engine::Cut::Plugins::CameraClip::OnIdle` (`0x0109558c`) when
the owner object's method at +0xc returns no camera for the string at clip
+0x44. It then calls the clip's own slot 9 (`mov eax, [edx+0x24]` then `call eax` at
`0x00a0aadc` to `0x00a0aae1`), stores the elapsed ticks at `0x00a0aaeb` and
calls `0x00a112e0` on the object at clip +0x3c at `0x00a0aaef`, the
CameraObject pose update of the [cut camera page](cut-camera-ccb.md#evaluation-at-a-frame)
with the same argument form as LocalCameraClip. The reads of
`u8(D+0x1c)` bit 0 behind the key entry byte 4 gate at `0x00a0aaf7` select
the bind path of the update. The pose is handed to the owner object through
its methods at +0x28, +0x18, +0x20 and +0x24 (`0x00a0ab01` onwards), which were
not traced.
