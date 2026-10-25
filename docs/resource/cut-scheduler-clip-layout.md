# Cut scheduler clip layout

This page gives the byte layout through which the retail 1.23b client turns an
embedded `SEDBSCB` scheduler into clip objects. It covers
RaptureActorBindClip, RaptureCharaVisualClip and RaptureMapCollisionOnOffClip,
and it is enough to scan decoded scheduler spans for those clips and the
characters they target. All values are little-endian. All conditions are
static; a match identifies authored data, not observed playback.

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
| `+0x04` | u32 | Raw value returned by clip slot 7 |
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
classes of these schedulers. The [clip payloads page](cut-scheduler-clip-payloads.md)
documents six more, which read class fields from +0x10 or +0x14.
